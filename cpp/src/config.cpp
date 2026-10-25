// © 2026 FuchsLabs
#include "config.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

#include <toml++/toml.hpp>

#include "secure.h"
#include "util.h"

namespace fl::config {

const std::vector<std::string> PAGES = {"system", "friends", "discord", "bf6", "rss", "homeassistant"};

using Key = std::pair<std::string, std::string>;

const ojson& defaults() {
    static const ojson d = [] {
        ojson j;
        j["discord"] = {{"enabled", true}, {"token", ""}, {"guild_ids", ojson::array()}, {"friend_ids", ojson::array()},
                        {"ignore_ids", ojson::array()}, {"show_offline", false}};
        j["display"] = {{"port", "AUTO"}, {"orientation", "landscape"}, {"brightness", 50}, {"reset_on_start", true},
                        {"page_seconds", 15}, {"pages", PAGES}, {"visible_pages", PAGES}, {"title", "Discord-Freunde"},
                        {"screen_off_on_exit", true}, {"theme", "hell"},
                        // Ausgabe: serial = USB-Display (3,5"), monitor = HDMI/USB-C-Bildschirm als Vollbildfenster
                        {"output", "serial"}, {"monitor", ""}, {"touch", true},
                        // Eigene Farben (theme = "eigen"): Vorlage plus #RRGGBB-Werte; leer = Farbe der Vorlage
                        {"color_base", "hell"}, {"color_bg", ""}, {"color_card", ""}, {"color_header", ""},
                        {"color_accent", ""}, {"color_text", ""}, {"color_muted", ""}};
        j["system"] = {{"enabled", true}, {"title", "System"}, {"net_adapter", ""}, {"net_unit", "bit"},
                       {"disk", "C:"}, {"cpu_temp_sensor", ""}, {"cpu_fan_sensor", ""}};
        j["friends"] = {{"enabled", false}, {"title", "Freunde"}, {"include_discord", true}, {"auto_merge", true},
                        {"show_offline", false}, {"steam_api_key", ""}, {"steam_id", ""}, {"xbox_api_key", ""},
                        {"links", ojson::array()}};
        j["bf6"] = {{"enabled", false}, {"players", ojson::array()}, {"refresh_minutes", 15},
                    {"title", "Battlefield 6 · Rangliste"}};
        j["rss"] = {{"enabled", false}, {"feeds", ojson::array()}, {"refresh_minutes", 30}, {"max_items", 4},
                    {"max_per_feed", 1}, {"title", "Blog-Updates"}};
        j["homeassistant"] = {{"enabled", false}, {"title", "Home Assistant"}, {"url", ""}, {"token", ""},
                              {"entities", ojson::array()}};
        // Companion-App (Android) im Heimnetz: standardmäßig aus; Port nie der der Einstellungsseite
        j["companion"] = {{"enabled", false}, {"port", 8777}, {"page_seconds", 15}, {"pages", PAGES}};
        j["ui"] = {{"enabled", true}, {"port", 8765}, {"setup_done", false}, {"language", "de"}, {"update_check", true}};
        return j;
    }();
    return d;
}

// (min, max) für Zahlenwerte
static const std::map<Key, std::pair<long long, long long>> LIMITS = {
    {{"display", "brightness"}, {0, 100}}, {{"display", "page_seconds"}, {3, 600}},
    {{"bf6", "refresh_minutes"}, {5, 240}}, {{"rss", "refresh_minutes"}, {10, 720}},
    {{"rss", "max_items"}, {1, 4}},        {{"rss", "max_per_feed"}, {1, 4}},
    {{"ui", "port"}, {1024, 65535}},
    {{"companion", "port"}, {1024, 65535}}, {{"companion", "page_seconds"}, {3, 600}},
};
static const std::map<Key, std::vector<std::string>> CHOICES = {
    {{"display", "orientation"}, {"landscape", "reverse_landscape", "portrait", "reverse_portrait"}},
    {{"system", "net_unit"}, {"bit", "byte"}},
    {{"display", "theme"}, {"hell", "dunkel", "eigen"}},
    {{"display", "color_base"}, {"hell", "dunkel"}},
    {{"display", "output"}, {"serial", "monitor", "network"}},
    {{"ui", "language"}, {"de", "en"}},
};
static const std::set<Key> ID_LISTS = {{"discord", "guild_ids"}, {"discord", "friend_ids"}, {"discord", "ignore_ids"}};
static const std::vector<Key> SECRETS = {{"discord", "token"}, {"friends", "steam_api_key"}, {"friends", "xbox_api_key"},
                                         {"homeassistant", "token"}};

bool is_color_key(const std::string& key) {
    static const std::set<std::string> k = {"color_bg", "color_card", "color_header", "color_accent", "color_text",
                                            "color_muted"};
    return k.count(key) > 0;
}

static bool is_secret(const std::string& sec, const std::string& key) {
    return std::find(SECRETS.begin(), SECRETS.end(), Key{sec, key}) != SECRETS.end();
}

// Wert als Text wie Pythons str()
static std::string py_str(const ojson& v) {
    if (v.is_string()) return v.get<std::string>();
    if (v.is_boolean()) return v.get<bool>() ? "True" : "False";
    if (v.is_null()) return "None";
    return v.dump();
}

// ---------- TOML lesen ----------
static ojson from_toml(const toml::node& n) {
    if (auto t = n.as_table()) {
        ojson o = ojson::object();
        for (auto& [k, v] : *t) o[std::string(k.str())] = from_toml(v);
        return o;
    }
    if (auto a = n.as_array()) {
        ojson arr = ojson::array();
        for (auto& v : *a) arr.push_back(from_toml(v));
        return arr;
    }
    if (auto s = n.as_string()) return s->get();
    if (auto i = n.as_integer()) return i->get();
    if (auto f = n.as_floating_point()) return f->get();
    if (auto b = n.as_boolean()) return b->get();
    std::ostringstream ss;  // Datum/Uhrzeit: als Text
    if (auto d = n.as_date()) ss << *d;
    else if (auto t = n.as_time()) ss << *t;
    else if (auto dt = n.as_date_time()) ss << *dt;
    return ss.str();
}

static ojson read_toml(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    std::string data((std::istreambuf_iterator<char>(f)), {});
    if (data.size() >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF)
        data.erase(0, 3);
    toml::table t = toml::parse(data, u8(path));  // wirft toml::parse_error
    return from_toml(t);
}

ojson load(const std::filesystem::path& path) {
    ojson cfg = defaults();
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        ojson user = read_toml(path);
        for (auto& [sec, vals] : user.items()) {
            if (!vals.is_object()) continue;
            if (!cfg.contains(sec)) cfg[sec] = ojson::object();
            for (auto& [k, v] : vals.items()) cfg[sec][k] = v;
        }
    }
    for (auto& [sec, key] : SECRETS) cfg[sec][key] = secure::unprotect(py_str(cfg[sec].value(key, ojson(""))));
    // Seiten, die eine ältere Konfiguration noch nicht kennt (z. B. "friends"), hinten anhängen
    std::vector<std::string> pages;
    if (cfg["display"]["pages"].is_array())
        for (auto& p : cfg["display"]["pages"])
            if (p.is_string() && std::count(PAGES.begin(), PAGES.end(), p.get<std::string>()) &&
                !std::count(pages.begin(), pages.end(), p.get<std::string>()))
                pages.push_back(p.get<std::string>());
    for (auto& p : PAGES)
        if (!std::count(pages.begin(), pages.end(), p)) pages.push_back(p);
    cfg["display"]["pages"] = pages;
    return cfg;
}

bool token_is_plaintext(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return false;
    ojson data = read_toml(path);
    for (auto& [sec, key] : SECRETS) {
        if (!data.contains(sec) || !data[sec].is_object() || !data[sec].contains(key)) continue;
        std::string v = py_str(data[sec][key]);
        if (!v.empty() && !secure::is_protected(v)) return true;
    }
    return false;
}

// ---------- Eingaben prüfen ----------
static double to_number(const ojson& v) {
    if (v.is_number()) return v.get<double>();
    if (v.is_boolean()) return v.get<bool>() ? 1 : 0;
    if (v.is_string()) {
        std::string s = trim(v.get<std::string>());
        if (!s.empty()) {
            char* end = nullptr;
            double d = strtod(s.c_str(), &end);
            if (end && *end == '\0' && std::isfinite(d)) return d;
        }
    }
    throw std::invalid_argument("Zahl erwartet");
}

static ojson coerce(const std::string& sec, const std::string& key, const ojson& value, const ojson& def) {
    Key k{sec, key};
    if (def.is_boolean()) {
        if (value.is_boolean()) return value;
        throw std::invalid_argument("Ja/Nein erwartet");
    }
    if (def.is_number_integer()) {
        long long v = (long long)std::trunc(to_number(value));
        if (auto it = LIMITS.find(k); it != LIMITS.end()) v = std::clamp(v, it->second.first, it->second.second);
        return v;
    }
    if (def.is_number_float()) return to_number(value);
    if (def.is_array()) {
        if (!value.is_array()) throw std::invalid_argument("Liste erwartet");
        if (ID_LISTS.count(k)) {
            std::set<unsigned long long> ids;
            for (auto& x : value) {
                std::string s = trim(py_str(x));
                if (s.empty()) continue;
                if (s.find_first_not_of("0123456789") != std::string::npos || s.size() > 20)
                    throw std::invalid_argument("Zahl erwartet");
                try {
                    ids.insert(std::stoull(s));
                } catch (...) {
                    throw std::invalid_argument("Zahl erwartet");
                }
            }
            return ojson(std::vector<unsigned long long>(ids.begin(), ids.end()));
        }
        if (k == Key{"companion", "pages"} || k == Key{"display", "visible_pages"}) {
            std::vector<std::string> out;
            for (auto& p : value)
                if (p.is_string() && std::count(PAGES.begin(), PAGES.end(), p.get<std::string>()) &&
                    !std::count(out.begin(), out.end(), p.get<std::string>())) out.push_back(p.get<std::string>());
            return out;
        }
        if (k == Key{"display", "pages"}) {
            std::vector<std::string> out;
            for (auto& p : value)
                if (p.is_string() && std::count(PAGES.begin(), PAGES.end(), p.get<std::string>()) &&
                    !std::count(out.begin(), out.end(), p.get<std::string>()))
                    out.push_back(p.get<std::string>());
            for (auto& p : PAGES)
                if (!std::count(out.begin(), out.end(), p)) out.push_back(p);
            return out;
        }
        std::vector<std::string> out;
        const size_t cap = k == Key{"homeassistant", "entities"} ? 4 : 50;
        for (auto& x : value) {
            std::u32string u = to_u32(trim(py_str(x)));
            if (u.size() > 200) u.resize(200);
            std::string s = from_u32(u);
            if (!s.empty() && !std::count(out.begin(), out.end(), s)) out.push_back(s);
        }
        if (out.size() > cap) out.resize(cap);
        return out;
    }
    std::u32string u = to_u32(trim(py_str(value)));
    if (u.size() > 300) u.resize(300);
    std::string v = from_u32(u);
    if (auto it = CHOICES.find(k); it != CHOICES.end() && !std::count(it->second.begin(), it->second.end(), v))
        throw std::invalid_argument("erlaubt: " + join(it->second, ", "));
    if (sec == "display" && is_color_key(key) && !v.empty()) {
        static const std::regex rx("#[0-9a-fA-F]{6}");
        if (!std::regex_match(v, rx)) throw std::invalid_argument("Farbe als #RRGGBB erwartet");
        for (auto& c : v) c = (char)toupper((unsigned char)c);
    }
    return v;
}

ojson merge(const ojson& cfg, const ojson& incoming) {
    ojson out = cfg;
    if (!incoming.is_object()) return out;
    const ojson& d = defaults();
    for (auto& [sec, vals] : incoming.items()) {
        if (!d.contains(sec) || !vals.is_object()) continue;
        for (auto& [key, value] : vals.items()) {
            if (!d[sec].contains(key)) continue;
            if (is_secret(sec, key) && trim(py_str(value)).empty()) continue;
            try {
                out[sec][key] = coerce(sec, key, value, d[sec][key]);
            } catch (const std::exception& e) {
                throw std::invalid_argument(sec + "." + key + ": " + e.what());
            }
        }
    }
    // Companion-Server und Einstellungsseite brauchen verschiedene Ports; 8765 bleibt der Einstellungsseite vorbehalten
    long long cport = out["companion"].value("port", 8777LL), uport = out["ui"].value("port", 8765LL);
    bool companion_used = out["companion"].value("enabled", false) || out["display"].value("output", "serial") == "network";
    if (companion_used && (cport == 8765 || cport == uport))
        throw std::invalid_argument("companion.port: muss sich vom Port der Einstellungsseite unterscheiden (nie 8765)");
    return out;
}

ojson public_view(const ojson& cfg) {
    ojson out = ojson::object();
    for (auto& [sec, vals] : cfg.items())
        if (defaults().contains(sec)) out[sec] = vals;
    for (auto& [sec, key] : SECRETS) {
        out[sec][key + "_set"] = !trim(py_str(cfg[sec].value(key, ojson("")))).empty();
        out[sec][key] = "";
    }
    for (auto& [sec, key] : ID_LISTS) {
        ojson arr = ojson::array();  // IDs als Text: JavaScript-Zahlen sind für 18 Stellen zu ungenau
        for (auto& x : out[sec][key]) arr.push_back(x.is_string() ? x.get<std::string>() : x.dump());
        out[sec][key] = arr;
    }
    return out;
}

// ---------- Schreiben ----------
static std::string toml_value(const ojson& v) {
    if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
    if (v.is_number_integer() || v.is_number_unsigned()) return v.dump();
    if (v.is_number_float()) {
        std::string s = v.dump();
        if (s.find_first_of(".eE") == std::string::npos) s += ".0";
        return s;
    }
    if (v.is_array()) {
        std::string s = "[";
        for (size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + toml_value(v[i]);
        return s + "]";
    }
    std::string s = py_str(v), out = "\"";
    for (unsigned char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n' || c == '\r') out += ' ';
        else if (c < 0x20 && c != '\t') out += strprintf("\\u%04X", c);
        else if (c == 0x7F) out += "\\u007F";
        else out += (char)c;
    }
    return out + "\"";
}

static const std::map<std::string, std::string> HEADERS = {
    {"discord", "Discord-Freunde (Bot-Token, Server, Personen)"},
    {"display", "Display und Seitenwechsel (pages = Reihenfolge der Seiten)"},
    {"system", "Systemansicht"},
    {"homeassistant", "Home Assistant (Adresse, Zugriffstoken, bis zu 4 Entitäten; Schalter nur mit Touch)"},
    {"friends", "Gemeinsame Freundesliste (Discord, Steam, Xbox); links = manuell verknüpfte Personen"},
    {"bf6", "Battlefield-6-Rangliste (EA-IDs, Konsole mit Suffix @psn / @xbox)"},
    {"rss", "Blog-Updates (RSS)"},
    {"companion", "Companion-App (Android) im Heimnetz: WebSocket über TLS, Kopplung per QR-Code; Kopplungen in companion.json"},
    {"ui", "Einstellungsoberfläche (läuft nur lokal auf diesem PC)"},
};

static void write_file(const std::filesystem::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(data.data(), (std::streamsize)data.size());
    if (!f) throw std::runtime_error("Konfiguration konnte nicht geschrieben werden");
}

void save(const std::filesystem::path& path, const ojson& cfg) {
    std::vector<std::string> lines = {"# ScreenBuddy – wird von der Einstellungsoberfläche geschrieben.",
                                      "# Von Hand ändern ist möglich; die App danach neu starten.", ""};
    for (auto& [sec, keys] : defaults().items()) {
        lines.push_back("# " + HEADERS.at(sec));
        lines.push_back("[" + sec + "]");
        for (auto& [key, def] : keys.items()) {
            ojson val = (cfg.contains(sec) && cfg[sec].contains(key)) ? cfg[sec][key] : def;
            if (is_secret(sec, key)) val = secure::protect(py_str(val));
            lines.push_back(key + " = " + toml_value(val));
        }
        lines.push_back("");
    }
    std::string text = join(lines, "\n");
    auto tmp = path;
    tmp.replace_extension(".toml.tmp");
    write_file(tmp, text);
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        bool plain = false;
        try {
            plain = token_is_plaintext(path);
        } catch (...) {
        }
        // Eine Sicherung mit Klartext-Token wäre ein Leck: dann die neue (verschlüsselte) Fassung sichern
        auto bak = path;
        bak.replace_extension(".toml.bak");
        if (plain) {
            write_file(bak, text);
        } else {
            std::filesystem::copy_file(path, bak, std::filesystem::copy_options::overwrite_existing, ec);
        }
    }
    if (!MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Konfiguration konnte nicht gespeichert werden");
}

}  // namespace fl::config
