// © 2026 FuchsLabs
#include "tmdb_logic.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <set>

#include <nlohmann/json.hpp>

namespace fl::tmdb {
namespace {

using json = nlohmann::json;

std::string trim_ascii(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

std::string pct(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

std::string query(const std::vector<std::pair<std::string, std::string>>& params) {
    std::string out;
    for (const auto& [k, v] : params) {
        if (!out.empty()) out += '&';
        out += pct(k) + "=" + pct(v);
    }
    return out;
}

bool digits(const std::string& s, size_t max_len) {
    return !s.empty() && s.size() <= max_len && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

// Kurzname für unbekannte Anbieter
std::string derive_abbreviation(const std::string& name_in) {
    std::string name = trim_ascii(name_in);
    bool plus = false;
    for (const char* suffix : {" Plus", "+"}) {
        const std::string suf = suffix;
        if (name.size() > suf.size() && name.compare(name.size() - suf.size(), suf.size(), suf) == 0) {
            name.erase(name.size() - suf.size());
            plus = true;
            break;
        }
    }
    std::vector<std::string> words;
    std::string cur;
    for (unsigned char c : name) {
        if (std::isalnum(c) && c < 0x80) cur += (char)c;
        else if (!cur.empty()) { words.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) words.push_back(cur);
    std::string out;
    if (words.size() == 1) out = words[0].size() <= 5 ? words[0] : words[0].substr(0, 3);
    else
        for (size_t i = 0; i < words.size() && out.size() < 3; ++i) out += (char)std::toupper((unsigned char)words[i][0]);
    if (out.empty()) return "";
    return out + (plus ? "+" : "");
}

// Tageszahl → Datum (proleptisch gregorianisch, UTC)
void civil_from_days(long long z, long long& y, unsigned& m, unsigned& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += m <= 2;
}

bool valid_date(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (!std::isdigit((unsigned char)s[i])) return false;
    int mo = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    return mo >= 1 && mo <= 12 && d >= 1 && d <= 31;
}

}  // namespace

// ---------------------------------------------------------------- Schlüssel
KeyKind key_kind(const std::string& key_in) {
    const std::string key = trim_ascii(key_in);
    if (key.size() < 16 || key.size() > 900) return KeyKind::None;
    for (unsigned char c : key)
        if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-')) return KeyKind::None;   // Schutz vor Header-/URL-Einschleusung
    if (key.rfind("eyJ", 0) == 0 || key.size() > 64) return KeyKind::Bearer;                // Read Access Token (JWT)
    return KeyKind::V3;
}

// ---------------------------------------------------------------- Region, Anbieter
std::string normalize_region(const std::string& text) {
    std::string t = trim_ascii(text);
    if (t.size() != 2 || !std::isalpha((unsigned char)t[0]) || !std::isalpha((unsigned char)t[1])) return "";
    for (auto& c : t) c = (char)std::toupper((unsigned char)c);
    return t;
}

std::string region_for(const std::string& lang, const std::string& override_region) {
    if (auto r = normalize_region(override_region); !r.empty()) return r;
    return lang == "en" ? "US" : "DE";
}

std::vector<Selected> parse_selection(const std::vector<std::string>& lines) {
    std::vector<Selected> out;
    std::set<int> seen;
    for (const auto& line : lines) {
        const auto bar = line.find('|');
        const std::string id = trim_ascii(line.substr(0, bar));
        if (!digits(id, 7)) continue;
        const int n = std::stoi(id);
        if (n <= 0 || !seen.insert(n).second) continue;
        out.push_back({n, bar == std::string::npos ? "" : trim_ascii(line.substr(bar + 1))});
        if ((int)out.size() >= MAX_PROVIDERS) break;
    }
    return out;
}

std::string abbreviation(int id, const std::string& name) {
    static const std::map<int, const char*> known = {{8, "NF"}, {9, "PV"}, {119, "PV"}, {2100, "PV"}, {337, "D+"},
                                                     {350, "ATV+"}, {30, "WOW"}, {298, "RTL+"}, {2750, "RTL+"}};
    if (auto it = known.find(id); it != known.end()) return it->second;
    auto d = derive_abbreviation(name);
    return d.empty() ? "#" + std::to_string(id) : d;
}

std::vector<std::string> default_provider_lines() {
    // TMDB-Provider-IDs (Region DE): Netflix 8, Prime Video 9 (alt: 119), Disney+ 337, Apple TV+ 350, WOW 30, RTL+ 2750 (298 nur Altbestand).
    // Die Liste in den Einstellungen lädt die echten Anbieter aus TMDB; ungültige IDs bleiben ohne Treffer.
    return {"8|Netflix", "9|Prime Video", "119|Prime Video", "337|Disney+", "350|Apple TV+", "30|WOW", "2750|RTL+"};
}

std::optional<std::vector<Provider>> parse_providers(const std::string& text) {
    json j = json::parse(text, nullptr, false);
    if (!j.is_object() || !j.contains("results") || !j["results"].is_array()) return std::nullopt;
    std::vector<Provider> out;
    for (const auto& p : j["results"]) {
        if (!p.is_object() || !p.contains("provider_id") || !p["provider_id"].is_number_integer() ||
            !p.contains("provider_name") || !p["provider_name"].is_string())
            continue;
        Provider r;
        r.id = p["provider_id"].get<int>();
        r.name = trim_ascii(p["provider_name"].get<std::string>());
        if (r.id <= 0 || r.name.empty()) continue;
        out.push_back(std::move(r));
    }
    return out;
}

std::vector<Provider> merge_providers(std::vector<Provider> a, const std::vector<Provider>& b) {
    for (const auto& p : b)
        if (std::none_of(a.begin(), a.end(), [&](const Provider& x) { return x.id == p.id; })) a.push_back(p);
    std::stable_sort(a.begin(), a.end(), [](const Provider& x, const Provider& y) {
        std::string lx = x.name, ly = y.name;
        for (auto& c : lx) c = (char)std::tolower((unsigned char)c);
        for (auto& c : ly) c = (char)std::tolower((unsigned char)c);
        return lx != ly ? lx < ly : x.id < y.id;
    });
    return a;
}

// ---------------------------------------------------------------- Anfragen
std::string providers_path(bool series, const std::string& region, const std::string& lang) {
    return std::string("/watch/providers/") + (series ? "tv" : "movie") + "?" +
           query({{"watch_region", region}, {"language", lang == "en" ? "en-US" : "de-DE"}});
}

std::string discover_path(bool series, const std::string& region, const std::vector<Selected>& providers,
                          const std::string& from, const std::string& to, const std::string& lang) {
    std::string ids;
    for (const auto& p : providers) ids += (ids.empty() ? "" : "|") + std::to_string(p.id);
    const std::string date_key = series ? "first_air_date" : "primary_release_date";
    return std::string("/discover/") + (series ? "tv" : "movie") + "?" +
           query({{"watch_region", region},
                  {"with_watch_providers", ids},
                  {"with_watch_monetization_types", "flatrate"},
                  {"sort_by", date_key + ".desc"},
                  {date_key + ".gte", from},
                  {date_key + ".lte", to},
                  {"include_adult", "false"},
                  {"language", lang == "en" ? "en-US" : "de-DE"},
                  {"page", "1"}});
}

std::string item_providers_path(bool series, int id) {
    return std::string("/") + (series ? "tv/" : "movie/") + std::to_string(id) + "/watch/providers";
}

std::string with_key(const std::string& path, const std::string& key_in) {
    const std::string key = trim_ascii(key_in);
    if (key_kind(key) != KeyKind::V3) return path;
    return path + (path.find('?') == std::string::npos ? "?" : "&") + "api_key=" + pct(key);
}

std::vector<std::pair<std::string, std::string>> auth_headers(const std::string& key_in) {
    const std::string key = trim_ascii(key_in);
    std::vector<std::pair<std::string, std::string>> h = {{"Accept", "application/json"}};
    if (key_kind(key) == KeyKind::Bearer) h.push_back({"Authorization", "Bearer " + key});
    return h;
}

// ---------------------------------------------------------------- Antworten
std::optional<std::vector<Item>> parse_discover(const std::string& text, bool series, const std::string& from,
                                                const std::string& to) {
    json j = json::parse(text, nullptr, false);
    if (!j.is_object() || !j.contains("results") || !j["results"].is_array()) return std::nullopt;
    std::vector<Item> out;
    for (const auto& r : j["results"]) {
        if (!r.is_object()) continue;
        const char* title_key = series ? "name" : "title";
        const char* date_key = series ? "first_air_date" : "release_date";
        if (!r.contains("id") || !r["id"].is_number_integer() || !r.contains(title_key) || !r[title_key].is_string() ||
            !r.contains(date_key) || !r[date_key].is_string())
            continue;
        Item it;
        it.id = r["id"].get<int>();
        it.title = trim_ascii(r[title_key].get<std::string>());
        it.date = r[date_key].get<std::string>();
        it.series = series;
        if (it.id <= 0 || it.title.empty() || !valid_date(it.date)) continue;
        if ((!from.empty() && it.date < from) || (!to.empty() && it.date > to)) continue;   // nur das Zeitfenster
        out.push_back(std::move(it));
    }
    return out;
}

std::vector<Item> merge_items(std::vector<Item> movies, std::vector<Item> series, size_t max) {
    std::vector<Item> all = std::move(movies);
    all.insert(all.end(), std::make_move_iterator(series.begin()), std::make_move_iterator(series.end()));
    std::stable_sort(all.begin(), all.end(), [](const Item& a, const Item& b) {
        if (a.date != b.date) return a.date > b.date;
        return a.title < b.title;
    });
    std::vector<Item> out;
    for (auto& it : all) {
        if (std::any_of(out.begin(), out.end(), [&](const Item& x) { return x.id == it.id && x.series == it.series; })) continue;
        out.push_back(std::move(it));
        if (out.size() >= max) break;
    }
    return out;
}

std::vector<std::string> match_providers(const std::string& text, const std::string& region, const std::vector<Selected>& sel) {
    std::vector<std::string> out;
    json j = json::parse(text, nullptr, false);
    if (!j.is_object() || !j.contains("results") || !j["results"].is_object() || !j["results"].contains(region)) return out;
    const json& r = j["results"][region];
    if (!r.is_object() || !r.contains("flatrate") || !r["flatrate"].is_array()) return out;
    for (const auto& p : r["flatrate"]) {
        if (!p.is_object() || !p.contains("provider_id") || !p["provider_id"].is_number_integer()) continue;
        const int id = p["provider_id"].get<int>();
        auto s = std::find_if(sel.begin(), sel.end(), [&](const Selected& x) { return x.id == id; });
        if (s == sel.end()) continue;
        std::string name = s->name;
        if (name.empty() && p.contains("provider_name") && p["provider_name"].is_string()) name = p["provider_name"].get<std::string>();
        const std::string ab = abbreviation(id, name);
        if (std::find(out.begin(), out.end(), ab) == out.end()) out.push_back(ab);
    }
    return out;
}

// ---------------------------------------------------------------- Datum
std::string date_ymd(std::time_t utc) {
    long long days = (long long)utc / 86400;
    if ((long long)utc % 86400 < 0) --days;
    long long y;
    unsigned m, d;
    civil_from_days(days, y, m, d);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04lld-%02u-%02u", y, m, d);
    return buf;
}

std::string add_days(std::time_t utc, int days) { return date_ymd(utc + (std::time_t)days * 86400); }

std::string format_date(const std::string& ymd, const std::string& lang) {
    if (!valid_date(ymd)) return "";
    const std::string mm = ymd.substr(5, 2), dd = ymd.substr(8, 2);
    return lang == "en" ? mm + "/" + dd : dd + "." + mm + ".";
}

int clamp_items(long long n) { return (int)std::clamp<long long>(n, 1, MAX_ITEMS); }
int clamp_refresh(long long minutes) { return (int)std::clamp<long long>(minutes, MIN_REFRESH_MINUTES, 720); }

}  // namespace fl::tmdb
