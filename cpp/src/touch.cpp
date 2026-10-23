// © 2026 FuchsLabs
#include "touch.h"
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <algorithm>
#include "bf6.h"
#include "http.h"
#include "util.h"

namespace fl::touch {
static bool digits(const std::string& s) {
    return !s.empty() && s.size() <= 20 && s.find_first_not_of("0123456789") == std::string::npos &&
           s.find_first_not_of('0') != std::string::npos;
}

Targets friend_targets(const Friend& person) {
    Targets out;
    // Bevorzugt online erreichbare Konten, innerhalb beider Gruppen Discord > Steam > Xbox.
    for (bool online : {true, false}) {
        for (const char* platform : {"discord", "steam", "xbox"}) {
            bool present = std::find(person.online_on.begin(), person.online_on.end(), platform) != person.online_on.end();
            if (present != online) continue;
            auto it = person.ids.find(platform);
            if (it == person.ids.end()) continue;
            const auto& id = it->second;
            if (std::string(platform) == "discord" && digits(id))
                out.push_back({"discord://-/users/" + id, "discord"});
            else if (std::string(platform) == "steam" && digits(id))
                out.push_back({"steam://friends/message/" + id, "steam"});
            else if (std::string(platform) == "xbox" && !trim(id).empty())
                out.push_back({"https://account.xbox.com/Profile?gamertag=" + http::url_encode(id), ""});
        }
    }
    return out;
}

Targets player_targets(const Bf6Player& player) {
    if (!player.error.empty()) return {};
    auto [name, platform] = bf6::parse_entry(player.query);
    if (name.empty()) return {};
    std::string web_platform = platform == "ps" || platform == "psn" ? "psn" :
                               platform == "xbox" || platform == "xbl" ? "xboxseries" : "ea";
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

const Targets* at(const std::vector<Tile>& tiles, int x, int y) {
    for (const auto& t : tiles) {
        const auto& b = t.box;
        if (x < b.x || x >= b.x + b.w || y < b.y || y >= b.y + b.h) continue;
        int row = t.rss ? render::RssRenderer::row_at(b.w, b.h, y - b.y) : render::Renderer::row_at(b.w, b.h, y - b.y);
        return row >= 0 && row < (int)t.rows.size() && !t.rows[row].empty() ? &t.rows[row] : nullptr;
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
}  // namespace fl::touch
