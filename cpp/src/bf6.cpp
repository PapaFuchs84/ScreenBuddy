// © 2026 FuchsLabs
#include "bf6.h"

#include <windows.h>

#include <algorithm>

#include "http.h"
#include "util.h"

namespace fl::bf6 {

using json = nlohmann::json;

static const char* API = "https://api.gametools.network/bf6";
// Plattform-Kürzel in der Konfiguration → Werte der Stats-API, in Reihenfolge der Versuche.
// Seit Nov. 2026 antwortet die API für PC-Konten nur noch mit "pc" ("ea" liefert 404); "ea" bleibt als Rückfall.
static const std::map<std::string, std::vector<std::string>> STATS_PLATFORM = {
    {"pc", {"pc", "ea"}},   {"ea", {"pc", "ea"}},   {"steam", {"pc", "ea"}},  {"epic", {"pc", "ea"}},
    {"psn", {"psn", "ps5", "ps4"}}, {"ps", {"psn", "ps5", "ps4"}},
    {"xbox", {"xboxseries", "xboxone", "xbox"}},    {"xbl", {"xboxseries", "xboxone", "xbox"}}};

std::vector<std::string> platform_candidates(const std::string& plat) {
    auto it = STATS_PLATFORM.find(lower_ascii(plat));
    return it != STATS_PLATFORM.end() ? it->second : STATS_PLATFORM.at("pc");
}

std::vector<std::string> name_candidates(const std::string& name) {
    std::vector<std::string> v{name};
    if (std::string l = lower_ascii(name); l != name) v.push_back(l);
    return v;
}

bool is_server_error(int status) { return status >= 500 && status <= 599; }

std::string failure_text(bool source_down) { return source_down ? "Quelle gestört" : "nicht gefunden"; }

Entry parse_entry_full(const std::string& entry) {
    std::string e = trim(entry), id;
    // "#id" am Ende: nur reine Ziffern ab 6 Stellen gelten als Spieler-ID (Gamertag-Zahlen wie "#1234" bleiben im Namen)
    if (auto h = e.rfind('#'); h != std::string::npos && h > 0) {
        std::string tail = e.substr(h + 1);
        if (tail.size() >= 6 && std::all_of(tail.begin(), tail.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            id = tail;
            e = trim(e.substr(0, h));
        }
    }
    auto at = e.rfind('@');
    if (at == std::string::npos || at == 0) return {e, "pc", id};
    std::string name = e.substr(0, at), plat = lower_ascii(e.substr(at + 1));
    if (!STATS_PLATFORM.count(plat)) return {e, "pc", id};
    return {name, plat, id};
}

std::pair<std::string, std::string> parse_entry(const std::string& entry) {
    Entry e = parse_entry_full(entry);
    return {e.name, e.platform};
}

std::string json_id(const json& v) {
    std::string s;
    if (v.is_number_integer()) s = std::to_string(v.get<long long>());
    else if (v.is_string()) s = trim(v.get<std::string>());
    return s == "0" || s == "false" ? std::string() : s;
}

// Erstes vorhandenes Feld aus einer Schlüsselliste (die API benennt ID und Name je nach Endpunkt unterschiedlich)
static const json* first_field(const json& o, std::initializer_list<const char*> keys) {
    if (!o.is_object()) return nullptr;
    for (const char* k : keys)
        if (auto it = o.find(k); it != o.end() && !it->is_null()) return &*it;
    return nullptr;
}

std::string pick_search_id(const json& response, const std::string& name) {
    const json* list = &response;
    if (response.is_object() && response.contains("results")) list = &response["results"];
    if (!list->is_array()) return {};
    std::string lname = lower_ascii(name), ci;
    for (auto& item : *list) {
        const json* n = first_field(item, {"name", "userName", "displayName", "platformUserHandle"});
        const json* i = first_field(item, {"id", "playerId", "personaId", "nucleusId"});
        if (!n || !n->is_string() || !i) continue;
        std::string id = json_id(*i), got = n->get<std::string>();
        if (id.empty()) continue;
        if (got == name) return id;  // exakter Treffer gewinnt
        if (ci.empty() && lower_ascii(got) == lname) ci = id;
    }
    return ci;
}
static std::optional<double> num(const json& j, const char* k) {
    if (!j.is_object() || !j.contains(k) || !j[k].is_number()) return std::nullopt;
    return j[k].get<double>();
}

static const json* profile(const json& data) {
    // Spielerprofil aus /profile – die API liefert zwei Formate (mit und ohne "other")
    const json* d = &data;
    if (!d->is_object()) return nullptr;
    if (d->contains("other")) {
        if (!(*d)["other"].is_array() || (*d)["other"].empty()) return nullptr;
        d = &(*d)["other"][0];
    }
    if (!d->is_object() || !d->contains("playerProfiles") || !(*d)["playerProfiles"].is_array() ||
        (*d)["playerProfiles"].empty())
        return nullptr;
    const json& pp = (*d)["playerProfiles"][0];
    if (!pp.is_object()) return nullptr;
    const json& card = pp.contains("playerCard") && pp["playerCard"].is_object() && !pp["playerCard"].empty() ? pp["playerCard"] : pp;
    return card.contains("rank") && !card["rank"].is_null() ? &pp : nullptr;
}

Bf6Stats::Bf6Stats(const ojson& cfg) {
    if (cfg.contains("players") && cfg["players"].is_array())
        for (auto& e : cfg["players"]) {
            std::string s = e.is_string() ? e.get<std::string>() : e.dump();
            if (!trim(s).empty()) entries_.push_back(s);
        }
    int mins = cfg.contains("refresh_minutes") && cfg["refresh_minutes"].is_number() ? cfg["refresh_minutes"].get<int>() : 15;
    refresh_ = std::max(5, mins) * 60;
    for (auto& e : entries_) {
        Bf6Player p;
        p.query = e;
        p.name = parse_entry(e).first;
        players_.push_back(p);
    }
}

Bf6Stats::~Bf6Stats() { stop(); }

void Bf6Stats::stop() {
    stop_ = true;
    if (th_.joinable()) th_.join();
}

bool Bf6Stats::sleep_for(double seconds) {
    double end = mono() + seconds;
    while (mono() < end) {
        if (stop_) return false;
        Sleep(100);
    }
    return !stop_;
}

std::vector<Bf6Player> Bf6Stats::players() {
    std::lock_guard lk(mu_);
    return players_;
}

std::optional<std::time_t> Bf6Stats::updated() {
    std::lock_guard lk(mu_);
    return updated_;
}

void Bf6Stats::start(std::function<void()> on_update) {
    th_ = std::thread([this, on_update] {
        while (!stop_) {
            double t = mono();
            std::vector<Bf6Player> fresh;
            for (auto& e : entries_) {
                if (stop_) return;
                fresh.push_back(fetch(e));
                if (!sleep_for(1)) return;  // API schonen
            }
            int ok = 0;
            for (auto& p : fresh) ok += p.error.empty();
            {
                std::lock_guard lk(mu_);
                players_ = fresh;
                updated_ = std::time(nullptr);
            }
            LOGI("bf6", "BF6: %d/%d Profile aktualisiert (%.1f s)", ok, (int)fresh.size(), mono() - t);
            on_update();
            if (!sleep_for(refresh_)) return;
        }
    });
}

std::optional<json> Bf6Stats::get(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params,
                                  bool* server_error) {
    std::string url = std::string(API) + "/" + path + "/?" + http::query(params);
    auto r = http::get(url, {{"User-Agent", UA}}, 45000);
    if (server_error && is_server_error(r.status)) *server_error = true;
    if (r.status != 200) {
        LOGD("bf6", "BF6 %s → HTTP %d %s", path.c_str(), r.status, r.error.c_str());
        return std::nullopt;
    }
    try {
        return json::parse(r.body);
    } catch (const std::exception& e) {
        LOGD("bf6", "BF6 %s → %s", path.c_str(), e.what());
        return std::nullopt;
    }
}

ImagePtr Bf6Stats::icon(const std::string& url) {
    if (url.empty()) return nullptr;
    {
        std::lock_guard lk(mu_);
        if (auto it = icons_.find(url); it != icons_.end()) return it->second;
    }
    auto r = http::get(url, {{"User-Agent", UA}}, 45000);
    if (r.status != 200) {
        LOGD("bf6", "Rang-Icon %s nicht geladen: HTTP %d", url.c_str(), r.status);
        return nullptr;
    }
    auto img = decode_image(r.body);
    if (!img) return nullptr;
    auto p = std::make_shared<const Image>(std::move(*img));
    std::lock_guard lk(mu_);
    icons_[url] = p;
    return p;
}

Bf6Player Bf6Stats::fetch(const std::string& entry) {
    Entry en = parse_entry_full(entry);
    const std::string& name = en.name;
    Bf6Player p;
    p.query = entry;
    p.name = name;
    // ID aus dem Eintrag, sonst aus dem Speicher (frühere Namenssuche); mit ID wird per playerid abgefragt, der Name dient nur der Anzeige
    std::string id = en.id;
    if (id.empty()) {
        std::lock_guard lk(mu_);
        if (auto it = ids_.find(entry); it != ids_.end()) id = it->second;
    }
    std::optional<json> stats;
    std::string used_name = name, used_plat = platform_candidates(en.platform).front();  // für /profile
    bool outage = false;   // mind. eine Antwort war HTTP 5xx: "nicht gefunden" ist dann nicht belegt
    bool by_name = false;  // Treffer kam aus der Namenssuche (nur dann wird die ID zurückgeschrieben)
    auto usable = [](const std::optional<json>& r) {
        return r && r->is_object() && !r->empty() &&
               !(r->contains("hasResults") && (*r)["hasResults"].is_boolean() && !(*r)["hasResults"].get<bool>());
    };
    auto stats_by_id = [&](const std::string& sid, const std::vector<std::string>& pls) {
        for (auto& pl : pls) {
            if (stop_) return false;
            auto r = get("stats", {{"playerid", sid}, {"platform", pl}, {"format_values", "false"}, {"categories", "multiplayer"}}, &outage);
            if (usable(r)) {
                stats = std::move(r);
                used_plat = pl;
                return true;
            }
        }
        return false;
    };
    for (int i = 0; i < 2 && !stats; ++i) {  // die API antwortet gelegentlich sporadisch mit 404/500
        // zweite Runde (Wiederholung bei Aussetzern) nur mit dem ersten Paar, damit die API nicht überlastet wird
        auto pls = platform_candidates(en.platform);
        auto nms = name_candidates(name);
        if (i > 0) { pls.resize(1); nms.resize(1); }
        if (!id.empty() && stats_by_id(id, pls)) break;
        if (stop_) return p;
        for (auto& pl : pls) {
            for (auto& nm : nms) {
                if (stop_) return p;
                auto r = get("stats", {{"name", nm}, {"platform", pl}, {"format_values", "false"}, {"categories", "multiplayer"}}, &outage);
                if (usable(r)) {
                    stats = std::move(r);
                    used_name = nm;
                    used_plat = pl;
                    by_name = true;
                    break;
                }
            }
            if (stats) break;
        }
        if (!stats && i == 0 && !sleep_for(2)) break;
    }
    // Letzter Ausweg: Spielersuche; nimmt nur Treffer mit gleichem Namen (ohne Groß/Klein) und fragt dann per ID ab
    if (!stats && !stop_) {
        for (auto& nm : name_candidates(name)) {
            auto r = get("player", {{"name", nm}, {"limit", "10"}}, &outage);
            std::string sid = r ? pick_search_id(*r, name) : std::string();
            if (!sid.empty() && sid != id && stats_by_id(sid, platform_candidates(en.platform))) {
                by_name = true;
                break;
            }
        }
    }
    if (stats) {
        p.kd = num(*stats, "killDeath");
        p.win_pct = num(*stats, "winPercent");
        p.kpm = num(*stats, "killsPerMinute");
        if (auto secs = num(*stats, "secondsPlayed"); secs && *secs) p.hours = *secs / 3600;
        if (auto it = stats->find("id"); it != stats->end()) p.id = json_id(*it);
        if (p.id.empty()) p.id = id;
        // Auto-Heilung: gefundene ID merken und einmalig an den Aufrufer melden (Config), sofern der Name zur Antwort passt
        const json* got = first_field(*stats, {"userName", "name"});
        bool same = !got || !got->is_string() || got->get<std::string>().empty() || lower_ascii(got->get<std::string>()) == lower_ascii(name);
        if (by_name && en.id.empty() && !p.id.empty() && same) {
            bool first = false;
            {
                std::lock_guard lk(mu_);
                first = ids_.emplace(entry, p.id).second;
                if (!first) ids_[entry] = p.id;
            }
            if (first && on_heal_) on_heal_(entry, trim(entry) + "#" + p.id);
        }
    }
    std::vector<std::vector<std::pair<std::string, std::string>>> tries;
    if (!p.id.empty()) tries.push_back({{"playerid", p.id}, {"platform", used_plat}});
    tries.push_back({{"name", used_name}, {"platform", used_plat}});
    std::optional<json> pp;
    for (auto& params : tries) {
        auto data = get("profile", params, &outage);
        if (data)
            if (const json* found = profile(*data)) {
                pp = *found;
                break;
            }
    }
    if (pp) {
        const json& card = pp->contains("playerCard") && (*pp)["playerCard"].is_object() && !(*pp)["playerCard"].empty()
                               ? (*pp)["playerCard"]
                               : *pp;
        if (card["rank"].is_number()) p.rank = card["rank"].get<int>();
        auto str = [](const json& j, const char* k) {
            return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string();
        };
        p.rank_name = str(*pp, "rankName");
        if (p.rank_name.empty()) p.rank_name = str(card, "rankName");
        if (card.contains("rankImage") && card["rankImage"].is_object()) p.rank_icon = icon(str(card["rankImage"], "small"));
    }
    if (!stats && !pp) {
        p.error = failure_text(outage);
        // Bei Netzwerkfehlern und gestörter Quelle (5xx) lieber die alten Werte behalten; ids_ bleibt unberührt
        std::lock_guard lk(mu_);
        for (auto& o : players_)
            if (o.query == entry && (o.rank || o.kd)) return o;
    }
    return p;
}

}  // namespace fl::bf6
