// © 2026 FuchsLabs
#include "bf6.h"

#include <windows.h>

#include "http.h"
#include "util.h"

namespace fl::bf6 {

using json = nlohmann::json;

static const char* API = "https://api.gametools.network/bf6";
// Plattform-Kürzel in der Konfiguration → Parameter der Stats-API (PC-Konten laufen über "ea")
static const std::map<std::string, std::string> STATS_PLATFORM = {
    {"pc", "ea"}, {"ea", "ea"}, {"steam", "ea"}, {"epic", "ea"}, {"psn", "psn"}, {"ps", "psn"},
    {"xbox", "xboxseries"}, {"xbl", "xboxseries"}};

std::pair<std::string, std::string> parse_entry(const std::string& entry) {
    std::string e = trim(entry);
    auto at = e.rfind('@');
    if (at == std::string::npos || at == 0) return {e, "pc"};
    std::string name = e.substr(0, at), plat = lower_ascii(e.substr(at + 1));
    if (!STATS_PLATFORM.count(plat)) return {e, "pc"};
    return {name, plat};
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

std::optional<json> Bf6Stats::get(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params) {
    std::string url = std::string(API) + "/" + path + "/?" + http::query(params);
    auto r = http::get(url, {{"User-Agent", UA}}, 45000);
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
    auto [name, plat] = parse_entry(entry);
    Bf6Player p;
    p.query = entry;
    p.name = name;
    std::optional<json> stats;
    for (int i = 0; i < 2; ++i) {  // die API antwortet gelegentlich sporadisch mit 404/500
        stats = get("stats", {{"name", name}, {"platform", STATS_PLATFORM.at(plat)}, {"format_values", "false"},
                              {"categories", "multiplayer"}});
        if (stats && stats->is_object() && !stats->empty()) break;
        stats.reset();
        if (!sleep_for(2)) break;
    }
    if (stats && stats->contains("hasResults") && (*stats)["hasResults"].is_boolean() && !(*stats)["hasResults"].get<bool>())
        stats.reset();
    if (stats) {
        p.kd = num(*stats, "killDeath");
        p.win_pct = num(*stats, "winPercent");
        p.kpm = num(*stats, "killsPerMinute");
        if (auto secs = num(*stats, "secondsPlayed"); secs && *secs) p.hours = *secs / 3600;
    }
    std::vector<std::vector<std::pair<std::string, std::string>>> tries;
    if (stats && stats->contains("id") && !(*stats)["id"].is_null()) {
        auto& id = (*stats)["id"];
        std::string sid = id.is_string() ? id.get<std::string>() : id.dump();
        if (!sid.empty() && sid != "0" && sid != "false") tries.push_back({{"playerid", sid}});
    }
    tries.push_back({{"name", name}});
    std::optional<json> pp;
    for (auto& params : tries) {
        auto data = get("profile", params);
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
        p.error = "nicht gefunden";
        // Bei Netzwerkfehlern lieber die alten Werte behalten
        std::lock_guard lk(mu_);
        for (auto& o : players_)
            if (o.query == entry && (o.rank || o.kd)) return o;
    }
    return p;
}

}  // namespace fl::bf6
