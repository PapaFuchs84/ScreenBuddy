// © 2026 FuchsLabs
#include "theme.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <set>
#include <system_error>

#include "brand.h"
#include "util.h"

namespace fl::theme {

namespace fs = std::filesystem;

// ---------------------------------------------------------------- Palette
Color* Palette::slot(const std::string& k) {
    if (k == "bg") return &bg;
    if (k == "card") return &card;
    if (k == "line") return &line;
    if (k == "header") return &header;
    if (k == "accent") return &accent;
    if (k == "text") return &text;
    if (k == "muted") return &muted;
    if (k == "success") return &success;
    if (k == "danger") return &danger;
    return nullptr;
}
const Color* Palette::slot(const std::string& k) const { return const_cast<Palette*>(this)->slot(k); }
bool Palette::operator==(const Palette& o) const {
    for (const char* k : COLOR_KEYS) if (!(*slot(k) == *o.slot(k))) return false;
    return true;
}

Palette builtin(bool dark) {
    Palette p{};
    if (dark) {
        p.bg = brand::NAVY_TIEF; p.card = brand::NAVY; p.line = brand::NAVY_LINIE; p.header = brand::ORANGE;
        p.accent = brand::ORANGE; p.text = brand::CREME; p.muted = {150, 160, 180};
        p.success = {35, 165, 90}; p.danger = {242, 63, 67};
    } else {
        p.bg = brand::CREME; p.card = brand::SAND; p.line = brand::LINE; p.header = brand::ORANGE;
        p.accent = brand::GLUT; p.text = brand::NAVY; p.muted = brand::NEBEL;
        p.success = {22, 140, 72}; p.danger = {215, 45, 50};
    }
    return p;
}

// ---------------------------------------------------------------- Kontrast
static double lum(Color c) {
    double v[3] = {c.r / 255.0, c.g / 255.0, c.b / 255.0};
    for (auto& x : v) x = x <= 0.03928 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
    return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2];
}
double contrast(Color a, Color b) {
    double la = lum(a), lb = lum(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}
Color readable_on(Color bg) { return contrast(brand::NAVY, bg) >= contrast(brand::WHITE, bg) ? brand::NAVY : brand::WHITE; }

static Color mixc(Color a, Color b, double t) {
    auto m = [&](uint8_t x, uint8_t y) { return (uint8_t)std::clamp((int)std::lround(x + (y - x) * t), 0, 255); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b)};
}

namespace {
struct Rule { const char* fg; const char* bg; double min; };
// Text und Akzentschrift auf Hintergrund und Karte; Linien werden nicht geprüft (reine Zier)
constexpr Rule RULES[] = {{"text", "bg", MIN_TEXT},    {"text", "card", MIN_TEXT},   {"muted", "bg", MIN_TEXT},
                          {"muted", "card", MIN_TEXT}, {"accent", "bg", MIN_TEXT},   {"accent", "card", MIN_TEXT},
                          {"success", "card", MIN_BAR}, {"danger", "card", MIN_BAR}, {"header", "card", MIN_BAR}};
}  // namespace

std::vector<Warning> contrast_warnings(const Palette& p) {
    std::vector<Warning> out;
    for (const auto& r : RULES) {
        double c = contrast(*p.slot(r.fg), *p.slot(r.bg));
        if (c + 1e-9 < r.min) out.push_back({r.fg, r.bg, std::round(c * 100) / 100, r.min});
    }
    return out;
}

// Farbe in Richtung Schwarz/Weiß schieben, bis sie zu allen Hintergründen den Mindestkontrast erreicht
static Color push(Color c, const std::vector<Color>& bgs, double need) {
    auto ok = [&](Color x) { for (Color b : bgs) if (contrast(x, b) < need) return false; return true; };
    if (ok(c)) return c;
    double avg = 0;
    for (Color b : bgs) avg += lum(b);
    avg /= (double)bgs.size();
    const Color first = avg > 0.4 ? Color{0, 0, 0} : Color{255, 255, 255};
    const Color second = avg > 0.4 ? Color{255, 255, 255} : Color{0, 0, 0};
    for (Color target : {first, second})
        for (int i = 1; i <= 50; ++i) {
            Color x = mixc(c, target, i / 50.0);
            if (ok(x)) return x;
        }
    return first;
}

Palette correct_contrast(const Palette& in, std::vector<std::string>* changed) {
    Palette p = in;
    auto fix = [&](const char* key, std::vector<const char*> on, double need) {
        std::vector<Color> bgs;
        for (const char* b : on) bgs.push_back(*p.slot(b));
        Color* c = p.slot(key);
        Color n = push(*c, bgs, need);
        if (!(n == *c)) { *c = n; if (changed) changed->push_back(key); }
    };
    fix("text", {"bg", "card"}, MIN_TEXT);
    fix("muted", {"bg", "card"}, MIN_TEXT);
    fix("accent", {"bg", "card"}, MIN_TEXT);
    fix("success", {"card"}, MIN_BAR);
    fix("danger", {"card"}, MIN_BAR);
    // Kopfzeile (Balken): nur Hintergrund der Karten zählt; Fuchs-Orange auf Creme erreicht das nicht und bleibt Marke
    return p;
}

// ---------------------------------------------------------------- Windows
WindowsSettings read_windows_settings() {
    WindowsSettings w;
    DWORD v = 1, size = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &v, &size) == ERROR_SUCCESS)
        w.light = v != 0;
    DWORD acc = 0;
    size = sizeof(acc);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", L"AccentColor", RRF_RT_REG_DWORD, nullptr,
                     &acc, &size) == ERROR_SUCCESS) {
        w.accent = {(uint8_t)(acc & 0xFF), (uint8_t)((acc >> 8) & 0xFF), (uint8_t)((acc >> 16) & 0xFF)};  // 0xAABBGGRR
        w.has_accent = true;
    } else {
        size = sizeof(acc);
        if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", L"ColorizationColor", RRF_RT_REG_DWORD,
                         nullptr, &acc, &size) == ERROR_SUCCESS) {
            w.accent = {(uint8_t)((acc >> 16) & 0xFF), (uint8_t)((acc >> 8) & 0xFF), (uint8_t)(acc & 0xFF)};  // 0xAARRGGBB
            w.has_accent = true;
        }
    }
    HIGHCONTRASTW hc{};
    hc.cbSize = sizeof(hc);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) && (hc.dwFlags & HCF_HIGHCONTRASTON)) {
        auto col = [](int idx) { COLORREF c = GetSysColor(idx); return Color{GetRValue(c), GetGValue(c), GetBValue(c)}; };
        w.high_contrast = true;
        w.hc_bg = col(COLOR_WINDOW);
        w.hc_text = col(COLOR_WINDOWTEXT);
        w.hc_accent = col(COLOR_HOTLIGHT);
        w.hc_muted = col(COLOR_GRAYTEXT);
        w.light = lum(w.hc_bg) > 0.4;
    }
    return w;
}

Palette windows_palette(const WindowsSettings& w) {
    Palette p = builtin(!w.light);
    if (w.high_contrast) {
        // Systemfarben, damit Windows-Kontrastdesigns (auch Farbenblind-Varianten) auf dem Display ankommen.
        p.bg = w.hc_bg; p.card = w.hc_bg; p.line = w.hc_text; p.text = w.hc_text; p.muted = w.hc_muted;
        p.accent = w.hc_accent; p.header = w.hc_accent; p.success = w.hc_accent; p.danger = w.hc_text;
        return correct_contrast(p);
    }
    if (w.light) {
        p.bg = {243, 243, 243}; p.card = {255, 255, 255}; p.line = {222, 222, 222};
        p.text = {26, 26, 26}; p.muted = {96, 96, 96};
    } else {
        p.bg = {32, 32, 32}; p.card = {43, 43, 43}; p.line = {64, 64, 64};
        p.text = {255, 255, 255}; p.muted = {170, 170, 170};
    }
    if (w.has_accent) p.accent = w.accent;
    else p.accent = w.light ? brand::GLUT : brand::ORANGE;
    // Kopfzeile und Logo bleiben Fuchs-Orange; nur die Akzentfarbe kommt von Windows
    return correct_contrast(p);
}

// ---------------------------------------------------------------- Prüfung
static bool utf8_ok(const std::string& s, size_t& chars) {
    chars = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (!n || i + n > s.size()) return false;
        uint32_t cp = n == 1 ? c : c & (0xFF >> (n + 1));
        for (size_t k = 1; k < n; ++k) {
            unsigned char cc = (unsigned char)s[i + k];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((n == 2 && cp < 0x80) || (n == 3 && cp < 0x800) || (n == 4 && cp < 0x10000) || cp > 0x10FFFF ||
            (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp < 0xA0) || (cp >= 0x202A && cp <= 0x202E) ||
            (cp >= 0x2066 && cp <= 0x2069) || cp == 0x200B || cp == 0xFEFF || cp == 0x2028 || cp == 0x2029)
            return false;
        // Zeichen, die in HTML/Pfaden/Meldungen Ärger machen könnten
        if (cp == '<' || cp == '>' || cp == '"' || cp == '\\' || cp == '/' || cp == '&' || cp == '`') return false;
        i += n;
        ++chars;
    }
    return true;
}

bool valid_name(const std::string& name) {
    size_t chars = 0;
    if (name.empty() || name.size() > MAX_NAME * 4 || !utf8_ok(name, chars)) return false;
    if (chars == 0 || chars > MAX_NAME) return false;
    return name == trim(name);
}

static const std::set<std::string> RESERVED = {"hell", "dunkel", "windows", "eigen", "custom", "con", "prn", "aux", "nul"};
bool valid_id(const std::string& id) {
    if (id.empty() || id.size() > MAX_NAME || RESERVED.count(id)) return false;
    if (id.front() == '-' || id.back() == '-') return false;
    for (char c : id) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    return true;
}

std::string slugify(const std::string& name) {
    std::string out;
    for (unsigned char c : name) {
        char l = (char)std::tolower(c);
        if ((l >= 'a' && l <= 'z') || (l >= '0' && l <= '9')) out += l;
        else if (!out.empty() && out.back() != '-') out += '-';
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    if (out.size() > MAX_NAME) out.resize(MAX_NAME);
    while (!out.empty() && out.back() == '-') out.pop_back();
    if (out.empty()) out = "theme";
    if (RESERVED.count(out)) out += "-theme";
    return out;
}

static bool strict_hex(const std::string& s, Color& out) {
    if (s.size() != 7 || s[0] != '#') return false;
    uint8_t v[3];
    for (int i = 0; i < 3; ++i) {
        int n = 0;
        for (int k = 1; k <= 2; ++k) {
            char c = s[1 + i * 2 + (k - 1)];
            int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (d < 0) return false;
            n = n * 16 + d;
        }
        v[i] = (uint8_t)n;
    }
    out = {v[0], v[1], v[2]};
    return true;
}

std::optional<Custom> parse(const ojson& j, std::string& err) {
    err.clear();
    auto fail = [&](std::string m) -> std::optional<Custom> { err = std::move(m); return std::nullopt; };
    if (!j.is_object()) return fail("kein JSON-Objekt");
    static const std::set<std::string> top = {"version", "name", "base", "dark", "colors"};
    for (auto& [k, v] : j.items()) if (!top.count(k)) return fail("unbekannter Schlüssel: " + k.substr(0, 40));
    for (const char* k : {"version", "name", "base", "dark", "colors"}) if (!j.contains(k)) return fail(std::string("fehlt: ") + k);
    if (!j["version"].is_number_integer() || j["version"].get<long long>() != FORMAT_VERSION) return fail("version muss 1 sein");
    if (!j["name"].is_string() || !valid_name(j["name"].get<std::string>())) return fail("ungültiger Name (1–40 Zeichen, keine Sonderzeichen < > \" \\ / & `)");
    if (!j["base"].is_string() || (j["base"] != "hell" && j["base"] != "dunkel")) return fail("base muss \"hell\" oder \"dunkel\" sein");
    if (!j["dark"].is_boolean()) return fail("dark muss true/false sein");
    if (!j["colors"].is_object()) return fail("colors fehlt");
    Custom t;
    t.name = j["name"].get<std::string>();
    t.base = j["base"].get<std::string>();
    t.dark = j["dark"].get<bool>();
    const ojson& c = j["colors"];
    for (auto& [k, v] : c.items()) {
        bool known = false;
        for (const char* ck : COLOR_KEYS) if (k == ck) known = true;
        if (!known) return fail("unbekannte Farbe: " + k.substr(0, 40));
    }
    for (const char* k : COLOR_KEYS) {
        if (!c.contains(k)) return fail(std::string("Farbe fehlt: ") + k);
        Color col;
        if (!c[k].is_string() || !strict_hex(c[k].get<std::string>(), col))
            return fail(std::string("Farbe ") + k + " muss #RRGGBB sein");
        *t.colors.slot(k) = col;
    }
    t.id = slugify(t.name);
    return t;
}

ojson to_json(const Custom& t) {
    ojson c = ojson::object();
    for (const char* k : COLOR_KEYS) {
        Color col = *t.colors.slot(k);
        c[k] = strprintf("#%02X%02X%02X", col.r, col.g, col.b);
    }
    return {{"version", FORMAT_VERSION}, {"name", t.name}, {"base", t.base}, {"dark", t.dark}, {"colors", c}};
}

std::optional<Custom> migrate_legacy(const ojson& d, const std::string& name) {
    if (!d.is_object()) return std::nullopt;
    auto s = [&](const char* k) { return d.contains(k) && d[k].is_string() ? d[k].get<std::string>() : std::string(); };
    const bool dark = s("color_base") == "dunkel";
    Custom t;
    t.name = name;
    t.base = dark ? "dunkel" : "hell";
    t.dark = dark;
    t.colors = builtin(dark);
    bool any_bg_muted = false;
    for (const char* k : {"bg", "card", "header", "accent", "text", "muted"}) {
        std::string v = s((std::string("color_") + k).c_str());
        Color col;
        if (v.empty() || !strict_hex(v, col)) continue;
        *t.colors.slot(k) = col;
        if (std::string(k) == "bg" || std::string(k) == "muted") any_bg_muted = true;
    }
    if (any_bg_muted) t.colors.line = mixc(t.colors.bg, t.colors.muted, 0.2);
    t.id = slugify(name);
    return t;
}

// ---------------------------------------------------------------- Ablage
fs::path themes_dir() { return install_dir() / L"themes"; }

Store::Store(fs::path dir) : dir_(std::move(dir)) {}

static std::optional<std::string> read_small(const fs::path& f, std::string& err) {
    std::error_code ec;
    auto sz = fs::file_size(f, ec);
    if (ec) { err = "Datei nicht lesbar"; return std::nullopt; }
    if (sz > MAX_FILE_BYTES) { err = "Datei zu groß (max. 8 KB)"; return std::nullopt; }
    std::ifstream in(f, std::ios::binary);
    if (!in) { err = "Datei nicht lesbar"; return std::nullopt; }
    std::string s((size_t)sz, '\0');
    in.read(s.data(), (std::streamsize)sz);
    if ((size_t)in.gcount() != sz) { err = "Datei nicht lesbar"; return std::nullopt; }
    return s;
}

static std::optional<Custom> parse_text(const std::string& text, std::string& err) {
    if (text.size() > MAX_FILE_BYTES) { err = "Text zu groß (max. 8 KB)"; return std::nullopt; }
    std::string body = text;
    if (body.compare(0, 3, "\xEF\xBB\xBF") == 0) body.erase(0, 3);
    ojson j = ojson::parse(body, nullptr, false);
    if (j.is_discarded()) { err = "kein gültiges JSON"; return std::nullopt; }
    return parse(j, err);
}

std::vector<Custom> Store::list(std::vector<std::string>* problems) const {
    std::vector<Custom> out;
    std::error_code ec;
    if (!fs::is_directory(dir_, ec)) return out;
    for (auto& e : fs::directory_iterator(dir_, ec)) {
        if (out.size() >= MAX_THEMES) break;
        if (!e.is_regular_file(ec) || e.path().extension() != L".json") continue;
        std::string id = u8(e.path().stem());
        std::string err;
        if (!valid_id(id)) { if (problems) problems->push_back(id + ": ungültiger Dateiname"); continue; }
        auto text = read_small(e.path(), err);
        auto t = text ? parse_text(*text, err) : std::nullopt;
        if (!t) { if (problems) problems->push_back(id + ": " + err); continue; }
        t->id = id;
        out.push_back(std::move(*t));
    }
    std::sort(out.begin(), out.end(), [](const Custom& a, const Custom& b) { return casefold(a.name) < casefold(b.name); });
    return out;
}

std::optional<Custom> Store::load(const std::string& id, std::string* err) const {
    std::string e;
    if (!valid_id(id)) { if (err) *err = "ungültige Theme-ID"; return std::nullopt; }
    auto text = read_small(dir_ / widen(id + ".json"), e);
    auto t = text ? parse_text(*text, e) : std::nullopt;
    if (!t) { if (err) *err = e; return std::nullopt; }
    t->id = id;
    return t;
}

std::string Store::unique_id(const std::string& base) const {
    std::error_code ec;
    std::string id = base;
    for (int n = 2; fs::exists(dir_ / widen(id + ".json"), ec) && n < 1000; ++n) {
        std::string suffix = "-" + std::to_string(n);
        id = base.substr(0, MAX_NAME - suffix.size()) + suffix;
    }
    return id;
}

bool Store::save(Custom& t, std::string& err) const {
    err.clear();
    if (!valid_name(t.name)) { err = "ungültiger Name"; return false; }
    if (t.base != "hell" && t.base != "dunkel") { err = "base ungültig"; return false; }
    std::error_code ec;
    fs::create_directories(dir_, ec);
    if (t.id.empty()) {
        if (list().size() >= MAX_THEMES) { err = "zu viele Themes (max. 64)"; return false; }
        t.id = unique_id(slugify(t.name));
    }
    if (!valid_id(t.id)) { err = "ungültige Theme-ID"; return false; }
    const fs::path file = dir_ / widen(t.id + ".json"), tmp = dir_ / widen(t.id + ".json.tmp");
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) { err = "Schreiben nicht möglich"; return false; }
        out << to_json(t).dump(2) << "\n";
        if (!out.good()) { err = "Schreiben nicht möglich"; return false; }
    }
    fs::rename(tmp, file, ec);
    if (ec) { fs::remove(tmp, ec); err = "Schreiben nicht möglich"; return false; }
    return true;
}

bool Store::remove(const std::string& id, std::string& err) const {
    err.clear();
    if (!valid_id(id)) { err = "ungültige Theme-ID"; return false; }
    std::error_code ec;
    if (!fs::remove(dir_ / widen(id + ".json"), ec) || ec) { err = "Theme nicht gefunden"; return false; }
    return true;
}

std::optional<Custom> Store::duplicate(const std::string& id, std::string& err) const {
    auto t = load(id, &err);
    if (!t) return std::nullopt;
    const std::string suffix = " (Kopie)";
    // Name kürzen, ohne UTF-8 zu zerreißen (valid_name zählt Zeichen)
    std::string name = t->name;
    while (!valid_name(name + suffix) && !name.empty()) {
        size_t cut = name.size() - 1;
        while (cut > 0 && ((unsigned char)name[cut] & 0xC0) == 0x80) --cut;
        name.resize(cut);
    }
    t->name = name + suffix;
    t->id.clear();
    if (!save(*t, err)) return std::nullopt;
    return t;
}

bool Store::rename(const std::string& id, const std::string& new_name, std::string& err) const {
    auto t = load(id, &err);
    if (!t) return false;
    if (!valid_name(new_name)) { err = "ungültiger Name"; return false; }
    t->name = new_name;   // Dateiname (ID) bleibt, damit Auswahlen gültig bleiben
    return save(*t, err);
}

std::optional<Custom> Store::import_text(const std::string& text, std::string& err) const {
    auto t = parse_text(text, err);
    if (!t) return std::nullopt;
    t->id.clear();
    if (!save(*t, err)) return std::nullopt;
    return t;
}

std::optional<Custom> Store::import_file(const fs::path& file, std::string& err) const {
    if (file.extension() != L".json") { err = "nur .json-Dateien"; return std::nullopt; }
    auto text = read_small(file, err);
    if (!text) return std::nullopt;
    return import_text(*text, err);
}

bool Store::export_file(const std::string& id, const fs::path& file, std::string& err) const {
    auto t = load(id, &err);
    if (!t) return false;
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) { err = "Schreiben nicht möglich"; return false; }
    out << to_json(*t).dump(2) << "\n";
    if (!out.good()) { err = "Schreiben nicht möglich"; return false; }
    return true;
}

bool migrate_config(ojson& cfg, const Store& store) {
    if (!cfg.contains("display") || !cfg["display"].is_object()) return false;
    ojson& d = cfg["display"];
    if (!d.contains("theme") || d["theme"] != "eigen") return false;
    auto custom = migrate_legacy(d, "Eigene Farben");
    std::string err;
    if (custom && store.save(*custom, err)) {
        d["theme"] = std::string(CUSTOM_PREFIX) + custom->id;
    } else {
        d["theme"] = custom ? custom->base : "hell";   // Ablage nicht beschreibbar: wenigstens Hell/Dunkel behalten
    }
    for (const char* k : {"color_bg", "color_card", "color_header", "color_accent", "color_text", "color_muted"}) d[k] = "";
    d["color_base"] = "hell";
    return true;
}

// ---------------------------------------------------------------- Auflösen
bool valid_theme_key(const std::string& k) {
    if (k == "hell" || k == "dunkel" || k == WINDOWS_ID) return true;
    return k.rfind(CUSTOM_PREFIX, 0) == 0 && valid_id(k.substr(std::strlen(CUSTOM_PREFIX)));
}

Resolved resolve_custom(const Custom& t) {
    Resolved r;
    r.colors = t.colors;
    r.bar = t.colors.header;
    r.dark = t.dark;
    r.base = t.base;
    return r;
}

Resolved resolve(const std::string& key, const Store& store, const WindowsSettings* ws) {
    Resolved r;
    if (key == "dunkel" || key == "hell") {
        r.dark = key == "dunkel";
        r.base = key;
        r.colors = builtin(r.dark);
        r.bar = r.colors.header;
        return r;
    }
    if (key == WINDOWS_ID) {
        WindowsSettings w = ws ? *ws : read_windows_settings();
        r.windows = true;
        r.dark = !w.light;
        r.base = r.dark ? "dunkel" : "hell";
        r.colors = windows_palette(w);
        // Balken in der Akzentfarbe, gegen Hintergrund und Karte mindestens 3:1
        r.bar = push(w.high_contrast ? r.colors.header : r.colors.accent, {r.colors.bg, r.colors.card}, MIN_BAR);
        return r;
    }
    if (key.rfind(CUSTOM_PREFIX, 0) == 0)
        if (auto t = store.load(key.substr(std::strlen(CUSTOM_PREFIX)))) return resolve_custom(*t);
    r = resolve("hell", store, ws);
    r.found = false;
    return r;
}

}  // namespace fl::theme
