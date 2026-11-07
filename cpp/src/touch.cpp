// © 2026 FuchsLabs
#include "touch.h"
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <algorithm>
#include "bf6.h"
#include "http.h"
#include "plugins.h"
#include <cmath>
#include "util.h"

namespace fl::touch {
const PluginTarget* plugin_at(const std::vector<Tile>& tiles, int x, int y) {
    for (const auto& tile : tiles) {
        int tx=x-tile.box.x, ty=y-tile.box.y;
        if (tx<0 || ty<0 || tx>=tile.box.w || ty>=tile.box.h) continue;
        for (const auto& t : tile.plugin) {
            const auto& r=t.rect_px;
            if (tx>=r.x && ty>=r.y && tx<r.x+r.w && ty<r.y+r.h) return &t;
        }
    }
    return nullptr;
}

void stabilize_plugins(std::vector<Tile>& current, const std::vector<Tile>& previous, double now) {
    for(auto& tile:current) for(auto& t:tile.plugin) {
        t.stable_since=now;
        for(const auto& old_tile:previous) for(const auto& old:old_tile.plugin) {
            const auto& a=t.rect_px; const auto& b=old.rect_px;
            if(!t.transitioning && !old.transitioning && t.plugin==old.plugin && t.block==old.block && t.zone==old.zone &&
               t.action==old.action && t.url==old.url && t.generation==old.generation &&
               tile.box.x==old_tile.box.x && tile.box.y==old_tile.box.y && tile.box.w==old_tile.box.w && tile.box.h==old_tile.box.h &&
               a.x==b.x && a.y==b.y && a.w==b.w && a.h==b.h) t.stable_since=old.stable_since;
        }
    }
}
PluginDecision plugin_decide(const std::vector<Tile>& tiles,int x,int y,double now,double stable_min,
                            const std::function<PluginAccess(const std::string&)>& access) {
    auto t=plugin_at(tiles,x,y); if(!t) return {};
    PluginDecision out; out.kind=PluginDecision::swallow;
    if(t->transitioning || !std::isfinite(now) || !std::isfinite(t->stable_since) || now-t->stable_since<stable_min) return out;
    auto grant=access(t->plugin);
    if(!grant.enabled || grant.generation!=t->generation) return out;
    out.plugin=t->plugin;out.zone=t->zone;out.generation=t->generation;
    if(t->action=="next" && !t->zone.empty()) {out.kind=PluginDecision::next;return out;}
    if(t->action!="open_link" || !grant.consent || t->url.size()>2048) return out;
    plugins::Manifest m;m.links=grant.hosts;
    if(!plugins::tap_url_valid(t->url) || !plugins::link_allowed(m,t->url)) return out;
    auto url=http::parse_url(t->url);
    if(!url.valid || url.scheme!="https" || url.port!=443 ||
       std::find(grant.hosts.begin(),grant.hosts.end(),url.host)==grant.hosts.end()) return out;
    out.kind=PluginDecision::open_link;out.url=t->url;return out;
}
bool plugin_dispatch(const PluginDecision& decision,double now,std::map<std::string,double>& last_open,
                     const std::function<bool(const std::string&)>& launch) {
    if(decision.kind!=PluginDecision::open_link || !std::isfinite(now)) return false;
    auto it=last_open.find(decision.plugin);
    if(it!=last_open.end() && now-it->second<2) return false;
    last_open[decision.plugin]=now;
    return launch(decision.url);
}

static const wchar_t* const SPEEDTEST_URL = L"https://www.speedtest.net/de";
static bool digits(const std::string& s) {
    return !s.empty() && s.size() <= 20 && s.find_first_not_of("0123456789") == std::string::npos &&
           s.find_first_not_of('0') != std::string::npos;
}

Targets friend_targets(const Friend& person) {
    Targets out;
    // Feste Reihenfolge: Steam-Chat, sonst Discord-Profil (mit "Nachricht"-Knopf), sonst Xbox-Profil im Browser.
    // Ist die App eines Ziels nicht registriert oder startet nicht, rückt das nächste Ziel nach.
    auto steam = person.ids.find("steam");
    if (steam != person.ids.end() && digits(steam->second))
        out.push_back({"steam://friends/message/" + steam->second, "steam"});
    auto discord = person.ids.find("discord");
    if (discord != person.ids.end() && digits(discord->second))
        out.push_back({"discord://-/users/" + discord->second, "discord"});
    auto xbox = person.ids.find("xbox");
    if (xbox != person.ids.end() && !trim(xbox->second).empty())
        out.push_back({"https://account.xbox.com/Profile?gamertag=" + http::url_encode(xbox->second), ""});
    auto psn = person.ids.find("psn");
    if (psn != person.ids.end() && !trim(psn->second).empty())
        out.push_back({"https://my.playstation.com/profile/" + http::url_encode(psn->second), ""});
    return out;
}

Targets player_targets(const Bf6Player& player) {
    if (!player.error.empty()) return {};
    auto [name, platform, id] = bf6::parse_entry_full(player.query);
    if (name.empty()) return {};
    // Die Webseite kennt andere Plattformkürzel als die Stats-API ("ea" ergibt dort eine leere Seite).
    std::string web_platform = platform == "ps" || platform == "psn" ? "ps4" :
                               platform == "xbox" || platform == "xbl" ? "xboxone" : "pc";
    // Mit Spieler-ID (aus dem Eintrag oder von der Abfrage) zeigt der Link auf tracker.gg (gleiche ID) – unabhängig vom Namen
    if (id.empty()) id = player.id;
    if (!id.empty() && std::all_of(id.begin(), id.end(), [](char c) { return c >= '0' && c <= '9'; }))
        return {{"https://tracker.gg/bf6/profile/" + id + "/overview", ""}};
    return {{"https://gametools.network/stats/" + web_platform + "/name/" + http::url_encode(name) + "?game=bf6", ""}};
}

template<class T, class F>
static Rows page_rows(const std::vector<T>& items, int page, int per, F targets) {
    Rows out;
    if (per <= 0) return out;
    int n = std::max(1, (int)((items.size() + per - 1) / per));
    int start = (((page % n) + n) % n) * per;
    for (int i = start; i < (int)items.size() && i < start + per; ++i) out.push_back(targets(items[i]));
    return out;
}

Rows friend_rows(std::vector<Friend> people, int page, int per) {
    return page_rows(render::sort_friends(std::move(people)), page, per, friend_targets);
}
Rows player_rows(std::vector<Bf6Player> players, int page, int per) {
    return page_rows(render::sort_players(std::move(players)), page, per, player_targets);
}
Rows post_rows(const std::vector<Post>& posts) {
    Rows out;
    for (size_t i = 0; i < posts.size() && i < render::RssRenderer::MAX_ROWS; ++i) {
        std::string link = trim(posts[i].link);
        auto url = http::parse_url(link);
        bool controls = std::any_of(link.begin(), link.end(), [](unsigned char c) { return c < 32 || c == 127; });
        out.push_back(url.valid && !controls ? Targets{{link, ""}} : Targets{});
    }
    return out;
}

Rows stream_rows(const std::vector<StreamItem>& items) {
    Rows out;
    for (size_t i = 0; i < items.size() && i < render::StreamRenderer::MAX_ROWS; ++i) {
        const auto& item = items[i];
        out.push_back(item.id > 0 ? Targets{{"https://www.themoviedb.org/" +
            std::string(item.series ? "tv/" : "movie/") + std::to_string(item.id), ""}} : Targets{});
    }
    return out;
}

const Targets* at(const std::vector<Tile>& tiles, int x, int y) {
    for (const auto& t : tiles) {
        const auto& b = t.box;
        if (x < b.x || x >= b.x + b.w || y < b.y || y >= b.y + b.h) continue;
        if(t.arranged) {
            for(const auto& target:t.arranged_targets) {const auto& r=target.rect_px;
                if(x-b.x>=r.x && x-b.x<r.x+r.w && y-b.y>=r.y && y-b.y<r.y+r.h) return target.links.empty()?nullptr:&target.links;}
            return nullptr;
        }
        int row = t.streaming ? render::StreamRenderer::row_at(b.w, b.h, y - b.y) :
                  t.rss ? render::RssRenderer::row_at(b.w, b.h, y - b.y) : render::Renderer::row_at(b.w, b.h, y - b.y);
        return row >= 0 && row < (int)t.rows.size() && !t.rows[row].empty() ? &t.rows[row] : nullptr;
    }
    return nullptr;
}

const std::function<void()>* action_at(const std::vector<Tile>& tiles, int x, int y) {
    for (const auto& t : tiles) {
        const auto& b = t.box;
        if (x < b.x || x >= b.x + b.w || y < b.y || y >= b.y + b.h) continue;
        if(t.arranged) {
            for(const auto& target:t.arranged_targets) {const auto& r=target.rect_px;
                if(x-b.x>=r.x && x-b.x<r.x+r.w && y-b.y>=r.y && y-b.y<r.y+r.h) return target.action?&target.action:nullptr;}
            return nullptr;
        }
        int i = t.sys ? render::SysRenderer::zone_at(b.w, b.h, x - b.x, y - b.y)
                      : t.spotify ? render::SpotifyRenderer::button_at(b.w, b.h, x - b.x, y - b.y)
                      : t.ha ? render::HaRenderer::cell_at(b.w, b.h, t.cells.size(), x - b.x, y - b.y) : -1;
        if (!t.ha && !t.sys && !t.spotify) return nullptr;
        return i >= 0 && i < (int)t.cells.size() && t.cells[i] ? &t.cells[i] : nullptr;
    }
    return nullptr;
}

bool dispatch(const Targets& targets, const std::function<bool(const std::string&)>& available,
              const std::function<bool(const std::string&)>& launch) {
    for (const auto& target : targets)
        if ((target.protocol.empty() || available(target.protocol)) && launch(target.url)) return true;
    return false;
}

bool open(const Targets& targets) {
    return dispatch(targets, [](const std::string& protocol) {
        DWORD length = 0;
        HRESULT hr = AssocQueryStringW(ASSOCF_IS_PROTOCOL, ASSOCSTR_COMMAND, widen(protocol).c_str(), nullptr, nullptr, &length);
        return (hr == S_FALSE || hr == E_POINTER) && length > 1;
    }, [](const std::string& url) {
        auto wide = widen(url);
        SHELLEXECUTEINFOW info{sizeof(info)};
        info.fMask = SEE_MASK_FLAG_NO_UI;
        info.lpVerb = L"open";
        info.lpFile = wide.c_str();
        info.nShow = SW_SHOWNORMAL;
        return ShellExecuteExW(&info) != FALSE;
    });
}
bool open_system(int zone) {
    const wchar_t* file = nullptr;
    const wchar_t* params = nullptr;
    switch (zone) {
        case render::SysRenderer::ZONE_RAM: file = L"taskmgr.exe"; break;
        case render::SysRenderer::ZONE_DISK: file = L"control.exe"; params = L"appwiz.cpl"; break;
        case render::SysRenderer::ZONE_NET: file = SPEEDTEST_URL; break;
        default: return false;
    }
    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask = SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"open";
    info.lpFile = file;
    info.lpParameters = params;
    info.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&info) != FALSE;
}
}  // namespace fl::touch
