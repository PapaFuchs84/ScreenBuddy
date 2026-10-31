// © 2026 FuchsLabs
#include "render.h"
#include "theme.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <regex>

#include "amp_logic.h"
#include "brand.h"
#include "ha.h"
#include "i18n.h"
#include "util.h"

namespace fl::render {

namespace {

constexpr int HEADER_H = 34, FOOTER_H = 18, AVATAR = 32, LOGO_SIZE = 20;   // LOGO_SIZE: Anbieter-Logo (Streaming)
constexpr float SYS_GAP = 6, SYS_SMALL = 62;   // Systemansicht: Abstand und Höhe der Karten RAM / Datenträger

Theme g_theme;
std::mutex g_mutex;
int row_height() { return g_theme.style == theme::Style::Cupertino ? 48 : g_theme.style == theme::Style::Material ? 52 : 44; }

Color mix(Color a, Color b, double t) {
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)std::nearbyint(x + (y - x) * t); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b)};
}

int pyround_i(double v) { return (int)std::nearbyint(v); }
int floordiv(double a, double b) { return (int)std::floor(a / b); }

std::string num(std::optional<double> v, int decimals = 0, const std::string& unit = "") {
    if (!v) return "–";
    return format_num(*v, decimals) + (unit.empty() ? "" : " " + unit);
}

const std::map<std::string, std::string> STATUS_LABEL = {
    {"online", "Online"}, {"idle", "Abwesend"}, {"dnd", "Bitte nicht stören"}, {"offline", "Offline"}};

struct Preset {
    Color bg, card, header, accent, text, muted;
};
const std::map<std::string, Preset> PRESETS = {
    {"hell", {brand::CREME, brand::SAND, brand::ORANGE, brand::GLUT, brand::NAVY, brand::NEBEL}},
    {"dunkel", {brand::NAVY_TIEF, brand::NAVY, brand::ORANGE, brand::ORANGE, brand::CREME, {150, 160, 180}}},
};

// Einmal beim Laden: helles Design als Ausgangszustand
struct InitTheme {
    InitTheme() { set_theme("hell"); }
} g_init_theme;

}  // namespace

const std::map<std::string, int> STATUS_ORDER = {{"online", 0}, {"idle", 1}, {"dnd", 2}, {"offline", 3}};

const Theme& theme() { return g_theme; }
std::mutex& mutex() { return g_mutex; }

std::string hex_color(Color c) { return strprintf("#%02X%02X%02X", c.r, c.g, c.b); }

static inline bool is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline uint8_t hex_val(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    return 0;
}

std::optional<Color> parse_color(const std::string& in) {
    std::string s = trim(in);
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 6) return std::nullopt;
    for (char c : s) if (!is_hex(c)) return std::nullopt;
    uint8_t r = (uint8_t)((hex_val(s[0]) << 4) | hex_val(s[1]));
    uint8_t g = (uint8_t)((hex_val(s[2]) << 4) | hex_val(s[3]));
    uint8_t b = (uint8_t)((hex_val(s[4]) << 4) | hex_val(s[5]));
    return Color{r, g, b};
}

double contrast(Color a, Color b) {
    // Kontrastverhältnis nach WCAG (1 … 21); ab 4,5 gut lesbar
    auto lum = [](Color c) {
        double v[3] = {c.r / 255.0, c.g / 255.0, c.b / 255.0};
        for (auto& x : v) x = x <= 0.03928 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
        return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2];
    };
    double la = lum(a), lb = lum(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

void set_theme(const std::string& name, const std::map<std::string, std::string>& custom) {
    struct BarSync { Theme& t; ~BarSync() { t.bar = t.orange; } } bar_sync{g_theme};
    Theme& t = g_theme;
    t.style = theme::Style::FuchsLabs;
    t.orange = brand::ORANGE;
    t.head_text = brand::NAVY;
    if (name == "dunkel") {
        t.bg = brand::NAVY_TIEF; t.card = brand::NAVY; t.line = brand::NAVY_LINIE;
        t.accent = brand::ORANGE; t.text = brand::CREME; t.muted = {150, 160, 180};
        t.status = {{"online", {35, 165, 90}}, {"idle", {240, 178, 50}}, {"dnd", {242, 63, 67}}, {"offline", {128, 132, 142}}};
        t.warn = {240, 178, 50}; t.hot = {242, 63, 67}; t.down = {35, 165, 90};
    } else {
        t.bg = brand::CREME; t.card = brand::SAND; t.line = brand::LINE;
        t.accent = brand::GLUT; t.text = brand::NAVY; t.muted = brand::NEBEL;
        t.status = {{"online", {22, 140, 72}}, {"idle", {200, 140, 0}}, {"dnd", {215, 45, 50}}, {"offline", {140, 140, 140}}};
        t.warn = {190, 120, 0}; t.hot = {215, 45, 50}; t.down = {22, 140, 72};
    }
    std::map<std::string, Color> c;
    for (auto& [k, v] : custom) {
        if (v.empty()) continue;
        if (auto col = parse_color(v)) c[k] = *col;  // ungültige Farbe: Vorlage behalten
    }
    auto get = [&](const char* k, Color def) { auto it = c.find(k); return it == c.end() ? def : it->second; };
    t.bg = get("bg", t.bg); t.card = get("card", t.card); t.orange = get("header", t.orange);
    t.accent = get("accent", t.accent); t.text = get("text", t.text); t.muted = get("muted", t.muted);
    if (c.count("bg") || c.count("muted")) t.line = mix(t.bg, t.muted, 0.2);
    if (c.count("header"))  // Kopfzeilenschrift: Navy oder Weiß, je nachdem was besser lesbar ist
        t.head_text = contrast(brand::NAVY, t.orange) >= contrast(brand::WHITE, t.orange) ? brand::NAVY : brand::WHITE;
}

void set_resolved(const theme::Resolved& r) {
    Theme& t = g_theme;
    t.style = r.style;
    const theme::Palette& p = r.colors;
    const bool dark = r.base == "dunkel";
    t.bg = p.bg; t.card = p.card; t.line = p.line; t.orange = p.header; t.accent = p.accent; t.text = p.text; t.muted = p.muted;
    t.head_text = theme::readable_on(p.header);
    t.bar = r.bar;
    t.status = dark ? std::map<std::string, Color>{{"online", p.success}, {"idle", {240, 178, 50}}, {"dnd", p.danger}, {"offline", {128, 132, 142}}}
                    : std::map<std::string, Color>{{"online", p.success}, {"idle", {180, 126, 0}}, {"dnd", p.danger}, {"offline", {136, 136, 136}}};
    t.warn = dark ? Color{240, 178, 50} : Color{150, 95, 0};
    t.hot = p.danger; t.down = p.success;
}

void apply_theme(const ojson& dp) {
    auto str = [&](const char* k, const char* def) {
        return dp.contains(k) && dp[k].is_string() ? dp[k].get<std::string>() : std::string(def);
    };
    if (dp.contains("theme_inline") && dp["theme_inline"].is_object()) {   // ungespeicherte Vorschau des Theme-Editors
        std::string err;
        if (auto t = theme::parse(dp["theme_inline"], err)) { set_resolved(theme::resolve_custom(*t)); return; }
    }
    const std::string key = str("theme", "hell");
    if (key == "windows" || key.rfind("custom:", 0) == 0) {
        set_resolved(theme::resolve(key, theme::Store()));
    } else if (key == "eigen") {
        std::map<std::string, std::string> custom;
        for (const char* k : {"bg", "card", "header", "accent", "text", "muted"}) custom[k] = str((std::string("color_") + k).c_str(), "");
        set_theme(str("color_base", "hell"), custom);
    } else {
        set_theme(key);
    }
}

ojson presets_json() {
    ojson out = ojson::object();
    for (auto& [name, p] : PRESETS)
        out[name] = {{"bg", hex_color(p.bg)}, {"card", hex_color(p.card)}, {"header", hex_color(p.header)},
                     {"accent", hex_color(p.accent)}, {"text", hex_color(p.text)}, {"muted", hex_color(p.muted)}};
    return out;
}

std::string clean_text(const std::string& text) {
    std::u32string out;
    for (char32_t o : to_u32(text)) {
        if (o >= 0x1F000 || (o >= 0x2600 && o <= 0x27BF) || (o >= 0xFE00 && o <= 0xFE0F) || o == 0x200D) continue;
        out += o;
    }
    return collapse_spaces(from_u32(out));
}

std::string fmt_rate(std::optional<double> bps, const std::string& unit) {
    if (!bps) return "–";
    struct Step { const char* name; double div; };
    static const Step bytes[] = {{"B/s", 1}, {"KB/s", 1e3}, {"MB/s", 1e6}, {"GB/s", 1e9}};
    static const Step bits[] = {{"bit/s", 1}, {"kbit/s", 1e3}, {"Mbit/s", 1e6}, {"Gbit/s", 1e9}};
    const Step* steps = unit == "byte" ? bytes : bits;
    double v = unit == "byte" ? *bps : *bps * 8;
    Step st = steps[0];
    for (int i = 0; i < 4; ++i) {
        st = steps[i];
        if (v < st.div * 1000) break;
    }
    double x = v / st.div;
    if (x >= 100 || st.div == 1) return strprintf("%.0f %s", x, st.name);
    return format_num(x, 1) + " " + st.name;
}

std::vector<Box> tile_layout(int w, int h, int pages) {
    int cols = 1;
    if (w > h) cols = std::max(1, std::min({pages, 4, pyround_i((double)w / h / 1.5)}));
    int gap = cols == 1 ? 0 : std::max(2, pyround_i(h / 160.0));
    int tw = (w - gap * (cols - 1)) / cols;
    std::vector<Box> boxes;
    for (int i = 0; i < cols; ++i) boxes.push_back({i * (tw + gap), 0, i < cols - 1 ? tw : w - i * (tw + gap), h});
    return boxes;
}

Image compose(int w, int h, const std::vector<Box>& boxes, const std::vector<Image>& frames) {
    if (frames.size() == 1) return frames[0];
    Image img(w, h, g_theme.line);
    for (size_t i = 0; i < frames.size() && i < boxes.size(); ++i) paste(img, frames[i], boxes[i].x, boxes[i].y);
    return img;
}

float scale_for(int w, int h) {
    return w >= h ? std::min(w / 480.f, h / 320.f) : std::min(w / 320.f, h / 480.f);
}

std::string ago(std::optional<std::time_t> d, std::time_t now) {
    if (!d) return "";
    if (!now) now = std::time(nullptr);
    long long mins = (long long)std::floor((double)(now - *d) / 60.0);
    if (mins < 1) return "gerade eben";
    if (mins < 60) return strprintf("vor %lld min", mins);
    if (mins < 48 * 60) return strprintf("vor %lld h", mins / 60);
    return strprintf("vor %lld Tagen", mins / 1440);
}

// ---------- Grundseite (Discord/Freunde) ----------
Renderer::Renderer(int w_, int h_, std::string title_) : W(w_), H(h_), title(std::move(title_)) {
    s = scale_for(W, H);
    if (s == 1) { w = (float)W; h = (float)H; }
    else { w = (float)pyround_i(W / s); h = (float)pyround_i(H / s); }
    f_title = font(17, true);
    f_small = font(12);
    f_name = font(16, true);
    f_act = font(13);
    f_big = font(18, true);
}

Font Renderer::font(int size, bool bold) const { Font result{(float)std::max(6, pyround_i(size * s)), bold};
    if (g_theme.style != theme::Style::FuchsLabs) result.family = g_theme.style == theme::Style::Material ? L"Roboto" : g_theme.style == theme::Style::Fluent ? L"Segoe UI Variable" : L"Segoe UI";
    return result; }

int Renderer::rows_per_page() const { return std::max(1, ((int)h - HEADER_H - FOOTER_H) / row_height()); }

int Renderer::row_at(int w_px, int h_px, int y_px) {
    float sc = scale_for(w_px, h_px);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float ly = y_px / sc, top = HEADER_H + 2;
    int per = std::max(1, ((int)lh - HEADER_H - FOOTER_H) / row_height());
    if (ly < top || ly >= lh - FOOTER_H || ly >= top + per * row_height()) return -1;
    return (int)((ly - top) / row_height());
}

int Renderer::pages(size_t items) const {
    int per = rows_per_page();
    return std::max(1, (int)((items + per - 1) / per));
}

std::string Renderer::fit(Canvas& d, const std::string& text, const Font& f, float max_w) {
    std::string t = tr(clean_text(text));
    if (d.textlength(t, f) <= max_w) return t;
    std::u32string u = to_u32(t);
    // längster Anfang, der mit "…" passt (Breite wächst mit der Länge, deshalb Halbierungssuche)
    size_t lo = 0, hi = u.size();
    while (lo < hi) {
        size_t mid = (lo + hi + 1) / 2;
        if (d.textlength(from_u32(u.substr(0, mid)) + "…", f) <= max_w) lo = mid;
        else hi = mid - 1;
    }
    return from_u32(u.substr(0, lo)) + "…";
}

void Renderer::message(Canvas& d, const std::string& text) {
    // Mehrzeilige, zentrierte Hinweis- oder Fehlermeldung (bis zu 4 Zeilen)
    auto words = split(clean_text(tr(text)), " ");
    std::vector<std::string> lines;
    std::string cur;
    for (auto& wd : words) {
        if (wd.empty()) continue;
        std::string test = trim(cur + " " + wd);
        if (d.textlength(test, f_big) <= w - 40) cur = test;
        else { lines.push_back(cur); cur = wd; }
    }
    lines.push_back(cur);
    if (lines.size() > 4) lines.resize(4);
    float y = h / 2 - (lines.size() - 1) * 14.f;
    for (auto& ln : lines) {
        d.text(w / 2, y, ln, f_big, g_theme.muted, "mm");
        y += 28;
    }
}

Image Renderer::welcome(const std::string& wtitle, const std::string& text) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    int band = (int)(h * 0.42);
    d.rectangle(0, 0, w, (float)band, t.orange);
    int fw = (int)(band * 0.62);
    Font f_mark = font(30, true);
    d.text(w / 2 + fw / 2.f + 4, band / 2.f, "FuchsLabs", f_mark, t.head_text, "mm");
    float tw = d.textlength("FuchsLabs", f_mark);
    d.fox((float)(int)(w / 2 - tw / 2 - fw / 2.f - 4), (float)((band - fw) / 2), pyround_i(fw * s), false);
    auto words = split(clean_text(tr(text)), " ");
    std::vector<std::string> lines;
    std::string cur;
    for (auto& wd : words) {
        if (wd.empty()) continue;
        std::string test = trim(cur + " " + wd);
        if (d.textlength(test, f_act) <= w - 40) cur = test;
        else { lines.push_back(cur); cur = wd; }
    }
    lines.push_back(cur);
    if (lines.size() > 4) lines.resize(4);
    float y = band + (h - band) / 2 - lines.size() * 10.f - 4;
    d.text(w / 2, y, wtitle, f_big, t.text, "mm");
    for (size_t i = 0; i < lines.size(); ++i) d.text(w / 2, y + 30 + i * 20.f, lines[i], f_act, t.muted, "mm");
    return d.finish();
}

void Renderer::header(Canvas& d, std::time_t now, const std::string& info) {
    // Orange Kopfzeile mit Creme-Fuchs; Schrift in Navy (bzw. Weiß bei dunkler eigener Kopfzeilenfarbe)
    const Theme& t = g_theme;
    d.rectangle(0, 0, w, HEADER_H - 1, t.style == theme::Style::FuchsLabs ? t.orange : t.bg);
    if (t.style != theme::Style::FuchsLabs) d.line({{8, HEADER_H - 1}, {w - 8, HEADER_H - 1}}, t.line, 1);
    float x = t.style == theme::Style::FuchsLabs ? 8.f : theme::design(t.style).spacing;
    d.fox(x, (float)((HEADER_H - ICON) / 2), pyround_i(ICON * s), false);
    x += ICON + 6;
    std::string clock = clock_hhmm(now ? now : std::time(nullptr));
    d.text(w - 8, HEADER_H / 2, clock, f_title, t.style == theme::Style::FuchsLabs ? t.head_text : t.muted, "rm");
    float cx = w - 8 - d.textlength(clock, f_title) - 12;
    float title_width = cx - x - (info.empty() ? 0 : d.textlength(tr(info), f_small) + 10);
    d.text(x, HEADER_H / 2, fit(d, tr(title), f_title, std::max(0.f, title_width)), f_title, t.style == theme::Style::FuchsLabs ? t.head_text : t.text, "lm");
    d.text(cx, HEADER_H / 2, info, f_small, t.style == theme::Style::FuchsLabs ? t.head_text : t.muted, "rm");
}

void Renderer::footer(Canvas& d, const std::string& left) {
    const Theme& t = g_theme;
    float fy = h - FOOTER_H;
    d.line({{0, fy}, {w, fy}}, t.line, 1);
    if (!left.empty()) d.text(8, fy + FOOTER_H / 2, left, f_small, t.muted, "lm");
    d.text(w - 8, fy + FOOTER_H / 2, "FuchsLabs", f_small, t.accent, "rm");
}

std::vector<Friend> sort_friends(std::vector<Friend> friends) {
    std::stable_sort(friends.begin(), friends.end(), [](const Friend& a, const Friend& b) {
        auto oa = STATUS_ORDER.count(a.status) ? STATUS_ORDER.at(a.status) : 9;
        auto ob = STATUS_ORDER.count(b.status) ? STATUS_ORDER.at(b.status) : 9;
        if (oa != ob) return oa < ob;
        return casefold(a.name) < casefold(b.name);
    });
    return friends;
}

Image Renderer::render_friends(std::vector<Friend> friends, int page, bool connected, const std::string& msg,
                               std::time_t now) {
    const Theme& t = g_theme;
    friends = sort_friends(std::move(friends));
    Canvas d(W, H, s, t.bg);
    int online = (int)std::count_if(friends.begin(), friends.end(), [](auto& f) { return f.status != "offline"; });
    header(d, now, std::to_string(online) + " online");
    float body_top = HEADER_H + 2;
    int n = pages(friends.size());
    if (!connected) {
        message(d, msg.empty() ? "Keine Verbindung zu Discord …" : msg);
    } else if (friends.empty()) {
        message(d, "Gerade ist niemand online");
    } else {
        int per = rows_per_page();
        page = ((page % n) + n) % n;
        for (int i = 0; i < per && page * per + i < (int)friends.size(); ++i) row(d, friends[page * per + i], body_top + i * row_height());
    }
    footer(d, n > 1 ? strprintf("Seite %d/%d", ((page % n) + n) % n + 1, n) : "");
    return d.finish();
}

float Renderer::badges(Canvas& d, const std::vector<std::string>& platforms, float right, float cy) {
    // Kleine Plattform-Marken (D/S/X/P) rechts oben in der Zeile; Rückgabe: belegte Breite
    static const std::map<std::string, std::pair<std::string, Color>> BADGE = {
        {"discord", {"D", {88, 101, 242}}}, {"steam", {"S", {35, 98, 148}}}, {"xbox", {"X", {16, 124, 16}}},
        {"psn", {"P", {0, 67, 156}}}};
    float x = right;
    for (auto it = platforms.rbegin(); it != platforms.rend(); ++it) {
        auto b = BADGE.count(*it) ? BADGE.at(*it) : std::make_pair(std::string("?"), g_theme.muted);
        d.rounded_rectangle(x - 16, cy - 8, x, cy + 8, 4, b.second);
        d.text(x - 8, cy, b.first, f_small, brand::WHITE, "mm");
        x -= 20;
    }
    return right - x + 8;
}

void Renderer::avatar(Canvas& d, const Friend& f, float x, float y, bool faded) {
    // offline: wie Version 1.x stark ausgeblendet (45 % Deckkraft zweimal überlagert ≈ 20 %)
    const Theme& t = g_theme;
    float op = faded ? 0.45f * 0.45f : 1.f;
    int size = pyround_i(AVATAR * s);
    if (f.avatar && !f.avatar->empty()) {
        d.round_image(*f.avatar, x, y, size, op);
        return;
    }
    // Platzhalter: Initiale auf Orange
    float px = std::nearbyint(x * s), py = std::nearbyint(y * s);
    d.ellipse(px / s, py / s, (px + size - 1) / s, (py + size - 1) / s, faded ? mix(t.bg, t.orange, op) : t.orange);
    std::wstring wn = widen(f.name.empty() ? "?" : f.name);
    std::wstring ch = wn.substr(0, (wn.size() > 1 && IS_HIGH_SURROGATE(wn[0])) ? 2 : 1);
    CharUpperBuffW(ch.data(), (DWORD)ch.size());
    Font fi{(float)(int)(size * 0.55), true};
    d.text((px + size / 2.f) / s, (py + size / 2.f) / s, narrow(ch), fi, faded ? mix(t.bg, t.head_text, op) : t.head_text,
           "mm");
}

void Renderer::row(Canvas& d, const Friend& f, float y) {
    const Theme& t = g_theme;
    float ax = 10, ay = y + (row_height() - AVATAR) / 2;
    const float l1 = y + row_height() / 2.f - 9, l2 = l1 + 18;   // zwei Zeilen vertikal zentriert (44: 13/31)
    avatar(d, f, ax, ay, f.status == "offline");
    // Statuspunkt unten rechts am Avatar, wie in Discord
    float r = 6, cx = ax + AVATAR - 4, cy = ay + AVATAR - 4;
    d.ellipse(cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2, t.bg);
    Color col = t.status.count(f.status) ? t.status.at(f.status) : t.status.at("offline");
    if (f.mobile && f.status != "offline") {  // Handy-Symbol statt Punkt
        d.rounded_rectangle(cx - 4, cy - r, cx + 4, cy + r, 2, col);
        d.rectangle(cx - 2, cy - r + 2, cx + 2, cy + r - 4, t.bg);
    } else {
        d.ellipse(cx - r, cy - r, cx + r, cy + r, col);
        if (f.status == "idle") d.ellipse(cx - r - 1, cy - r - 1, cx + 1, cy + 1, t.bg);
        else if (f.status == "dnd") d.rectangle(cx - 4, cy - 1, cx + 4, cy + 1, t.bg);
        else if (f.status == "offline") d.ellipse(cx - 3, cy - 3, cx + 3, cy + 3, t.bg);
    }
    float tx = ax + AVATAR + 12;
    float max_w = w - tx - 10;
    if (f.platforms.size() > 1 || (f.platforms.size() == 1 && f.platforms[0] != "discord"))
        max_w -= badges(d, f.platforms, w - 10, l1);
    Color name_col = f.status != "offline" ? t.text : t.muted;
    std::string line2 = !f.activity.empty() ? f.activity : (STATUS_LABEL.count(f.status) ? STATUS_LABEL.at(f.status) : "");
    if (!line2.empty()) {
        d.text(tx, l1, fit(d, f.name, f_name, max_w), f_name, name_col, "lm");
        Color act_col = starts_with_any(f.activity, {"Spielt", "Streamt"}) ? t.accent : t.muted;
        d.text(tx, l2, fit(d, line2, f_act, max_w), f_act, act_col, "lm");
    } else {
        d.text(tx, y + row_height() / 2, fit(d, f.name, f_name, max_w), f_name, name_col, "lm");
    }
    d.line({{tx, y + row_height() - 1}, {w - 10, y + row_height() - 1}}, t.line, 1);
}

// ---------- Battlefield 6 ----------
std::vector<Bf6Player> sort_players(std::vector<Bf6Player> p) {
    std::stable_sort(p.begin(), p.end(), [](const Bf6Player& a, const Bf6Player& b) {
        if (a.rank.has_value() != b.rank.has_value()) return a.rank.has_value();
        int ra = a.rank.value_or(0), rb = b.rank.value_or(0);
        if (ra != rb) return ra > rb;
        double ka = a.kd.value_or(0), kb = b.kd.value_or(0);
        if (ka != kb) return ka > kb;
        return casefold(a.name) < casefold(b.name);
    });
    return p;
}

Bf6Renderer::Bf6Renderer(int w_, int h_, std::string t) : Renderer(w_, h_, std::move(t)) {
    f_stat = font(15, true);
    f_pos = font(14, true);
}

Image Bf6Renderer::render(std::vector<Bf6Player> players, int page, std::optional<std::time_t> updated, std::time_t now) {
    const Theme& t = g_theme;
    players = sort_players(std::move(players));
    Canvas d(W, H, s, t.bg);
    int found = (int)std::count_if(players.begin(), players.end(), [](auto& p) { return p.error.empty(); });
    header(d, now, std::to_string(found) + " Spieler");
    float body_top = HEADER_H + 2;
    int n = pages(players.size());
    page = ((page % n) + n) % n;
    if (players.empty()) {
        d.text(w / 2, h / 2, "Keine Spieler in config.toml", f_big, t.muted, "mm");
    } else if (!updated) {
        d.text(w / 2, h / 2, "Lade Battlefield-Profile …", f_big, t.muted, "mm");
    } else {
        int per = rows_per_page();
        for (int i = 0; i < per && page * per + i < (int)players.size(); ++i)
            bf_row(d, players[page * per + i], page * per + i + 1, body_top + i * row_height());
    }
    std::string left = updated ? "Stand " + clock_hhmm(*updated) : "";
    if (n > 1) left = strprintf("Seite %d/%d", page + 1, n) + (left.empty() ? "" : " · " + left);
    footer(d, left);
    return d.finish();
}

void Bf6Renderer::bf_row(Canvas& d, const Bf6Player& p, int pos, float y) {
    const Theme& t = g_theme;
    float cy = y + row_height() / 2;
    const float l1 = cy - 9, l2 = cy + 9;   // zwei Zeilen vertikal zentriert (44: 13/31)
    Color pos_col = pos <= 3 && p.error.empty() ? t.accent : t.muted;
    d.text(24, cy, std::to_string(pos) + ".", f_pos, pos_col, "rm");
    float ix = 30;
    if (p.rank_icon && !p.rank_icon->empty()) {
        int n = pyround_i((AVATAR + 4) * s);
        Image icon = thumbnail(*p.rank_icon, n, n);
        float iw = icon.w / s, ih = icon.h / s;  // logische Größe
        d.image(icon, ix + (float)floordiv(AVATAR + 4 - iw, 2), cy - (float)floordiv(ih, 2));
    } else if (p.rank) {
        d.ellipse_outline(ix + 2, cy - 16, ix + 34, cy + 16, t.orange, 2);
        d.text(ix + 18, cy, std::to_string(*p.rank), f_small, t.text, "mm");
    }
    float tx = ix + AVATAR + 12;
    // Rechte Spalte: K/D groß, Siegquote und Spielstunden klein.
    std::string detail;
    if (p.win_pct) detail = tr(strprintf("%.0f %% Siege", *p.win_pct));
    if (p.hours && std::isfinite(*p.hours) && *p.hours >= 0)
        detail += (detail.empty() ? "" : " · ") + strprintf("%.0f h", *p.hours);
    float stat_w = std::max(92.f, d.textlength(tr(detail), f_act) + 8), sx = w - 10;
    if (p.kd) d.text(sx, l1, strprintf("K/D %.2f", *p.kd), f_stat, t.text, "rm");
    if (!detail.empty()) d.text(sx, l2, detail, f_act, t.muted, "rm");
    float max_w = sx - stat_w - tx;
    d.text(tx, l1, fit(d, p.name, f_name, max_w), f_name, p.error.empty() ? t.text : t.muted, "lm");
    std::string line2;
    Color col;
    if (!p.error.empty()) {
        line2 = p.error;
        col = t.status.at("dnd");
    } else {
        line2 = p.rank ? "Rang " + std::to_string(*p.rank) : "Rang unbekannt";
        if (!p.rank_name.empty()) line2 += " · " + p.rank_name;
        col = t.accent;
    }
    d.text(tx, l2, fit(d, line2, f_act, max_w), f_act, col, "lm");
    d.line({{tx, y + row_height() - 1}, {w - 10, y + row_height() - 1}}, t.line, 1);
}

// ---------- System ----------
SysRenderer::SysRenderer(int w_, int h_, std::string t, std::string unit)
    : Renderer(w_, h_, std::move(t)), net_unit(std::move(unit)) {
    f_card = font(14, true);
    f_huge = font(30, true);
    f_val = font(15, true);
    f_lbl = font(12);
}

Color SysRenderer::temp_col(std::optional<double> t) const {
    const Theme& th = g_theme;
    return t && *t >= 85 ? th.hot : t && *t >= 75 ? th.warn : th.text;
}

std::array<SysRenderer::Region, 5> SysRenderer::regions(float width, float height, theme::Style style) {
    // Material/Cupertino: unterer Abstand zur Fußzeile wie der seitliche Rand (8 px), nicht nur 4 px.
    const bool airy = style == theme::Style::Material || style == theme::Style::Cupertino;
    const float top = HEADER_H + 4.f, bottom = height - FOOTER_H - (airy ? 8.f : 4.f);
    const float body = std::max(0.f, bottom - top);
    if (style == theme::Style::Cupertino) {
        std::array<Region, 5> result{};
        const float x = 12.f, row = body / 5.f, cw = std::max(0.f, width - 24.f);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = {x, top + float(i) * row, cw, row,
                         i < 2 ? -1 : int(i) - 2};
        return result;
    }
    if (style == theme::Style::Material) {
        const float x = 8.f, gap = 8.f, cw = std::max(0.f, width - 16.f);
        const float hero = std::min(94.f, body * .38f);
        const float lower_top = top + hero + gap;
        const float lower_h = std::max(0.f, bottom - lower_top);
        const float gpu_w = std::floor((cw - gap) * .45f);
        const float right = x + gpu_w + gap, right_w = std::max(0.f, cw - gpu_w - gap);
        const float small = std::max(0.f, (lower_h - 2.f * gap) / 3.f);
        return {{{x, top, cw, hero, -1}, {x, lower_top, gpu_w, lower_h, -1},
                 {right, lower_top, right_w, small, ZONE_RAM},
                 {right, lower_top + small + gap, right_w, small, ZONE_DISK},
                 {right, lower_top + 2.f * (small + gap), right_w, small, ZONE_NET}}};
    }
    // Preserve the original FuchsLabs layout exactly. Fluent keeps the compact
    // dashboard composition, with its own margins. Keep the property-card height
    // large enough to separate its detail line from the progress bar.
    const bool fluent = style == theme::Style::Fluent;
    const float gap = fluent ? 8.f : SYS_GAP;
    const float small = SYS_SMALL;
    const float left_w = float(floordiv(width - 3.f * gap, 2));
    const float right = 2.f * gap + left_w, right_w = width - right - gap;
    const float half = float(floordiv(body - gap, 2));
    const float net_top = top + 2.f * (small + gap);
    return {{{gap, top, left_w, half, -1},
             {gap, top + half + gap, left_w, body - half - gap, -1},
             {right, top, right_w, small, ZONE_RAM},
             {right, top + small + gap, right_w, small, ZONE_DISK},
             {right, net_top, right_w, bottom - net_top, ZONE_NET}}};
}

int SysRenderer::zone_at(int w_px, int h_px, int x_px, int y_px) {
    if (w_px <= 0 || h_px <= 0 || x_px < 0 || y_px < 0 || x_px >= w_px || y_px >= h_px) return -1;
    const float sc = scale_for(w_px, h_px);
    const float logical_w = sc == 1 ? float(w_px) : float(pyround_i(w_px / sc));
    const float logical_h = sc == 1 ? float(h_px) : float(pyround_i(h_px / sc));
    const float x = x_px / sc, y = y_px / sc;
    for (const auto& area : regions(logical_w, logical_h, g_theme.style))
        if (x >= area.x && x < area.x + area.w && y >= area.y && y < area.y + area.h)
            return area.zone;
    return -1;
}

Image SysRenderer::render(const SysSnapshot& sn, std::time_t now) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, now, sn.hwinfo ? "" : "ohne HWiNFO");
    const auto areas = regions(w, h, t.style);
    if (t.style == theme::Style::Cupertino) {
        grouped_system(d, areas, sn);
    } else if (t.style == theme::Style::Material) {
        expressive_system(d, areas, sn);
    } else {
        const auto& cpu = areas[0]; const auto& gpu = areas[1];
        const auto& ram_area = areas[2]; const auto& disk_area = areas[3]; const auto& net = areas[4];
        usage_card(d, cpu.x, cpu.y, cpu.w, cpu.h, "CPU", sn.cpu_name, sn.cpu_pct,
                   {{"Temp", num(sn.cpu_temp, 0, "°C"), temp_col(sn.cpu_temp)},
                    {"Takt", num(sn.cpu_ghz, 2, "GHz"), t.text},
                    {"Lüfter", num(sn.cpu_fan, 0, "RPM"), t.text}});
        std::string vram = sn.gpu_mem_used && sn.gpu_mem_total && *sn.gpu_mem_total
                               ? num(*sn.gpu_mem_used / 1024, 1) + "/" + num(*sn.gpu_mem_total / 1024, 0) + " GB" : "–";
        usage_card(d, gpu.x, gpu.y, gpu.w, gpu.h, "GPU", sn.gpu_name, sn.gpu_pct,
                   {{"Temp", num(sn.gpu_temp, 0, "°C"), temp_col(sn.gpu_temp)},
                    {"Lüfter", num(sn.gpu_fan, 0, "%"), t.text}, {"VRAM", vram, t.text}});
        std::string ram = sn.ram_used ? num(sn.ram_used, 1) + " / " + num(sn.ram_total, 0) + " GB" : "–";
        bar_card(d, ram_area.x, ram_area.y, ram_area.w, ram_area.h, "RAM", ram, sn.ram_pct);
        std::string disk = sn.disk_label + " " + (sn.disk_temp ? num(sn.disk_temp, 0, "°C") : tr("belegt"));
        bar_card(d, disk_area.x, disk_area.y, disk_area.w, disk_area.h, "Datenträger", disk, sn.disk_pct);
        net_card(d, net.x, net.y, net.w, net.h, sn);
    }
    footer(d, "");
    return d.finish();
}

void SysRenderer::grouped_system(Canvas& d, const std::array<Region, 5>& areas, const SysSnapshot& sn) {
    const Theme& t = g_theme;
    const auto& first = areas.front(); const auto& last = areas.back();
    d.rounded_rectangle(first.x, first.y, first.x + first.w, last.y + last.h, 14, t.card);
    const std::string vram = sn.gpu_mem_used && sn.gpu_mem_total && *sn.gpu_mem_total
        ? num(*sn.gpu_mem_used / 1024, 1) + "/" + num(*sn.gpu_mem_total / 1024, 0) + " GB" : "–";
    const std::array<std::string, 5> labels = {"CPU", "GPU", "RAM", "Datenträger", "Netzwerk"};
    const std::array<std::string, 5> names = {sn.cpu_name, sn.gpu_name,
        sn.ram_used ? num(sn.ram_used, 1) + " / " + num(sn.ram_total, 0) + " GB" : "–",
        sn.disk_label, sn.net_name};
    const std::array<std::string, 5> details = {
        "Temp " + num(sn.cpu_temp, 0, "°C") + " · " + tr("Takt") + " " + num(sn.cpu_ghz, 2, "GHz") + " · " + tr("Lüfter") + " " + num(sn.cpu_fan, 0, "RPM"),
        "Temp " + num(sn.gpu_temp, 0, "°C") + " · " + tr("Lüfter") + " " + num(sn.gpu_fan, 0, "%") + " · VRAM " + vram,
        tr("belegt") + " · " + num(sn.ram_used, 1, "GB"),
        sn.disk_temp ? "Temp " + num(sn.disk_temp, 0, "°C") : tr("belegt"),
        "Download " + fmt_rate(sn.net_down, net_unit) + " · Upload " + fmt_rate(sn.net_up, net_unit)};
    const std::array<std::optional<double>, 5> values = {sn.cpu_pct, sn.gpu_pct, sn.ram_pct, sn.disk_pct, std::nullopt};
    const Font label = font(13, true), detail = font(11), value = font(18, true);
    for (size_t i = 0; i < areas.size(); ++i) {
        const auto& a = areas[i];
        const float right = a.x + a.w - 12;
        const float label_y = a.y + 14, detail_y = a.y + 33;
        const std::string reading = i == 4 ? fmt_rate(sn.net_down, net_unit) : num(values[i], 0, "%");
        const float reading_left = right - d.textlength(reading, value);
        const float name_x = a.x + 112;
        d.text(a.x + 12, label_y, labels[i], label, t.text, "lm");
        d.text(name_x, label_y, fit(d, names[i], detail, std::max(0.f, reading_left - name_x - 12)), detail, t.muted, "lm");
        d.text(right, label_y, reading, value, i == 0 ? temp_col(sn.cpu_temp) : i == 1 ? temp_col(sn.gpu_temp) : t.text, "rm");
        d.text(a.x + 12, detail_y, fit(d, details[i], detail, a.w - 24), detail, t.muted, "lm");
        if (values[i]) bar(d, a.x + 12, a.y + a.h - 7, a.w - 24, values[i], 3);
        if (i + 1 < areas.size()) d.line({{a.x + 12, a.y + a.h}, {right, a.y + a.h}}, t.line, 1);
    }
}

void SysRenderer::expressive_system(Canvas& d, const std::array<Region, 5>& areas, const SysSnapshot& sn) {
    const Theme& t = g_theme;
    const auto& cpu = areas[0]; const auto& gpu = areas[1];
    const Font tiny = font(11), value = font(14, true), hero = font(32, true);
    card(d, cpu.x, cpu.y, cpu.w, cpu.h, "CPU", sn.cpu_name);
    d.text(cpu.x + 14, cpu.y + 53, num(sn.cpu_pct, 0, "%"), hero, t.text, "lm");
    bar(d, cpu.x + 14, cpu.y + cpu.h - 14, 98, sn.cpu_pct, 5);
    const std::array<RowVal, 3> cpu_details = {{{"Temp", num(sn.cpu_temp, 0, "°C"), temp_col(sn.cpu_temp)},
        {"Takt", num(sn.cpu_ghz, 2, "GHz"), t.text}, {"Lüfter", num(sn.cpu_fan, 0, "RPM"), t.text}}};
    const float column = (cpu.w - 142) / 3;
    for (size_t i = 0; i < cpu_details.size(); ++i) {
        const float x = cpu.x + 132 + float(i) * column;
        d.text(x, cpu.y + 42, cpu_details[i].label, tiny, t.muted, "lm");
        d.text(x, cpu.y + 64, fit(d, cpu_details[i].value, value, column - 8), value, cpu_details[i].col, "lm");
    }
    d.rounded_rectangle(gpu.x, gpu.y, gpu.x + gpu.w, gpu.y + gpu.h, 20, t.card);
    d.text(gpu.x + 12, gpu.y + 15, "GPU", f_card, t.accent, "lm");
    d.text(gpu.x + 12, gpu.y + 35, fit(d, sn.gpu_name, tiny, gpu.w - 24), tiny, t.muted, "lm");
    d.text(gpu.x + 12, gpu.y + 63, num(sn.gpu_pct, 0, "%"), font(24, true), t.text, "lm");
    bar(d, gpu.x + 12, gpu.y + 83, gpu.w - 24, sn.gpu_pct, 5);
    d.text(gpu.x + 12, gpu.y + 105, "Temp " + num(sn.gpu_temp, 0, "°C"), tiny, temp_col(sn.gpu_temp), "lm");
    d.text(gpu.x + gpu.w - 12, gpu.y + 105, num(sn.gpu_fan, 0, "%") + " " + tr("Lüfter"), tiny, t.muted, "rm");
    const std::string vram = sn.gpu_mem_used && sn.gpu_mem_total && *sn.gpu_mem_total
        ? num(*sn.gpu_mem_used / 1024, 1) + "/" + num(*sn.gpu_mem_total / 1024, 0) + " GB" : "–";
    d.text(gpu.x + 12, gpu.y + 129, fit(d, "VRAM " + vram, tiny, gpu.w - 24), tiny, t.muted, "lm");
    auto compact = [&](const Region& a, const std::string& title, const std::string& description, std::optional<double> pct) {
        d.rounded_rectangle(a.x, a.y, a.x + a.w, a.y + a.h, 12, t.card);
        d.text(a.x + 10, a.y + 12, title, font(12, true), t.accent, "lm");
        d.text(a.x + a.w - 10, a.y + 12, num(pct, 0, "%"), value, t.text, "rm");
        d.text(a.x + 10, a.y + 29, fit(d, description, tiny, a.w - 20), tiny, t.muted, "lm");
        bar(d, a.x + 10, a.y + a.h - 7, a.w - 20, pct, 3);
    };
    compact(areas[2], "RAM", sn.ram_used ? num(sn.ram_used, 1) + " / " + num(sn.ram_total, 0) + " GB" : "–", sn.ram_pct);
    compact(areas[3], "Datenträger", sn.disk_label + (sn.disk_temp ? " · " + num(sn.disk_temp, 0, "°C") : ""), sn.disk_pct);
    const auto& net = areas[4];
    d.rounded_rectangle(net.x, net.y, net.x + net.w, net.y + net.h, 12, t.card);
    d.text(net.x + 10, net.y + 12, "Netzwerk", font(12, true), t.accent, "lm");
    d.text(net.x + net.w - 10, net.y + 12, fit(d, sn.net_name, tiny, net.w - 100), tiny, t.muted, "rm");
    // The compact network cell prioritizes both live rates over the old chart.
    arrow(d, net.x + 15, net.y + 32, true, t.down);
    d.text(net.x + 28, net.y + 32, fit(d, fmt_rate(sn.net_down, net_unit), tiny, net.w / 2 - 36), tiny, t.text, "lm");
    arrow(d, net.x + net.w / 2 + 10, net.y + 32, false, t.bar);
    d.text(net.x + net.w - 10, net.y + 32, fit(d, fmt_rate(sn.net_up, net_unit), tiny, net.w / 2 - 30), tiny, t.text, "rm");
}

void SysRenderer::card(Canvas& d, float x, float y, float cw, float ch, const std::string& ctitle, const std::string& sub) {
    const Theme& t = g_theme;
    d.rounded_rectangle(x, y, x + cw, y + ch, theme::design(t.style).card_radius, t.card);
    d.text(x + 10, y + 13, ctitle, f_card, t.accent, "lm");
    if (!sub.empty()) {
        float tx = x + 16 + d.textlength(ctitle, f_card);
        d.text(tx, y + 13, fit(d, sub, f_lbl, x + cw - 10 - tx), f_lbl, t.muted, "lm");
    }
}

void SysRenderer::bar(Canvas& d, float x, float y, float bw, std::optional<double> pct, float bh) {
    const Theme& t = g_theme;
    float r = (float)floordiv(bh, 2);
    d.rounded_rectangle(x, y, x + bw, y + bh, r, t.bg);
    if (pct && *pct) {
        Color c = *pct >= 90 ? t.hot : t.bar;
        d.rounded_rectangle(x, y, x + std::max(bh, (float)(int)(bw * std::min(*pct, 100.0) / 100)), y + bh, r, c);
    }
}

void SysRenderer::usage_card(Canvas& d, float x, float y, float cw, float ch, const std::string& ctitle,
                             const std::string& sub, std::optional<double> pct, const std::vector<RowVal>& rows) {
    const Theme& t = g_theme;
    card(d, x, y, cw, ch, ctitle, sub);
    d.text(x + 10, y + 50, num(pct, 0, "%"), f_huge, t.text, "lm");
    bar(d, x + 10, y + 74, 84, pct);
    float cx = x + 104;
    float step = (float)floordiv(ch - 30, (double)rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        float ry = y + 30 + step * i + (float)floordiv(step, 2);
        d.text(cx, ry, rows[i].label, f_lbl, t.muted, "lm");
        d.text(x + cw - 10, ry, rows[i].value, f_val, rows[i].col, "rm");
    }
}

void SysRenderer::bar_card(Canvas& d, float x, float y, float cw, float ch, const std::string& ctitle,
                           const std::string& detail, std::optional<double> pct) {
    const Theme& t = g_theme;
    card(d, x, y, cw, ch, ctitle);
    d.text(x + cw - 10, y + 13, num(pct, 0, "%"), f_val, t.text, "rm");
    d.text(x + 10, y + 36, detail, f_lbl, t.muted, "lm");
    bar(d, x + 10, y + ch - 16, cw - 20, pct);
}

void SysRenderer::arrow(Canvas& d, float cx, float cy, bool down, Color c) {
    // Pfeile als Formen: die CI-Schrift hat keine Pfeilzeichen
    float sg = down ? 1.f : -1.f;  // Schaft zeigt von der Spitze weg, Spitze in Pfeilrichtung
    d.line({{cx, cy - 6 * sg}, {cx, cy}}, c, 3);
    d.polygon({{cx - 6, cy}, {cx + 6, cy}, {cx, cy + 7 * sg}}, c);
}

void SysRenderer::net_card(Canvas& d, float x, float y, float cw, float ch, const SysSnapshot& sn) {
    const Theme& t = g_theme;
    card(d, x, y, cw, ch, "Netzwerk", sn.net_name);
    arrow(d, x + 16, y + 36, true, t.down);
    d.text(x + cw - 10, y + 36, fmt_rate(sn.net_down, net_unit), f_val, t.text, "rm");
    arrow(d, x + 16, y + 56, false, t.bar);
    d.text(x + cw - 10, y + 56, fmt_rate(sn.net_up, net_unit), f_val, t.text, "rm");
    // Verlauf der letzten 60 s (Download grün, Upload orange)
    float gx0 = x + 10, gx1 = x + cw - 10, gy0 = y + 70, gy1 = y + ch - 8;
    if (gy1 - gy0 < 10) return;
    double peak = 1.0;
    for (double v : sn.down_hist) peak = std::max(peak, v);
    for (double v : sn.up_hist) peak = std::max(peak, v);
    for (auto [hist, c] : {std::make_pair(&sn.down_hist, t.down), std::make_pair(&sn.up_hist, t.bar)}) {
        if (hist->size() < 2) continue;
        size_t n = hist->size();
        std::vector<PointF> pts;
        for (size_t i = 0; i < n; ++i)
            pts.push_back({gx1 - (float)(n - 1 - i) * (gx1 - gx0) / 59, gy1 - (float)((*hist)[i] / peak) * (gy1 - gy0)});
        d.line(pts, c, 2);
    }
}

// ---------- Blogs ----------
RssRenderer::RssRenderer(int w_, int h_, std::string t) : Renderer(w_, h_, std::move(t)) {
    f_post = font(17, true);
    f_src = font(13, true);
}

static std::string short_title(const std::string& title) {
    // Werbe-/Rubrik-Präfixe wie "heise+ |" oder "Anzeige:" weglassen
    static const std::wregex rx(LR"(^\s*(\S{1,12}\s*\|\s*|Anzeige:\s*|Deal:\s*))");
    return narrow(std::regex_replace(widen(title), rx, L"", std::regex_constants::format_first_only));
}

std::vector<std::string> RssRenderer::wrap(Canvas& d, const std::string& text, const Font& f, float max_w, int lines) {
    std::vector<std::string> words;
    for (auto& wd : split(clean_text(text), " "))
        if (!wd.empty()) words.push_back(wd);
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < words.size(); ++i) {
        std::string test = trim(cur + " " + words[i]);
        if (d.textlength(test, f) <= max_w) { cur = test; continue; }
        out.push_back(cur);
        cur = words[i];
        if ((int)out.size() == lines - 1) {
            std::vector<std::string> rest(words.begin() + i, words.end());
            out.push_back(fit(d, join(rest, " "), f, max_w));
            return out;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

int RssRenderer::row_at(int w_px, int h_px, int y_px) {
    // gleiche Aufteilung wie render(): logische Höhe, Zeilen zwischen Kopf- und Fußzeile
    float sc = scale_for(w_px, h_px);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float top = HEADER_H + 2, bot = lh - FOOTER_H, ly = y_px / sc;
    float rh = (float)floordiv(bot - top, MAX_ROWS);
    if (ly < top || ly >= bot || rh <= 0) return -1;
    int row = (int)((ly - top) / rh);
    return row < MAX_ROWS ? row : -1;
}

Image RssRenderer::render(const std::vector<Post>& posts, std::optional<std::time_t> updated, std::time_t now) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, now, "");
    float top = HEADER_H + 2, bot = h - FOOTER_H;
    if (!updated) {
        d.text(w / 2, h / 2, "Lade Feeds …", f_big, t.muted, "mm");
    } else if (posts.empty()) {
        d.text(w / 2, h / 2, "Keine Beiträge gefunden", f_big, t.muted, "mm");
    } else {
        size_t n = std::min(posts.size(), (size_t)MAX_ROWS);
        float rh = (float)floordiv(bot - top, MAX_ROWS);
        for (size_t i = 0; i < n; ++i) {
            const Post& p = posts[i];
            float y = top + i * rh;
            std::string age = ago(p.published);
            d.text(w - 10, y + 12, age, f_act, t.muted, "rm");
            float src_w = w - 30 - d.textlength(age, f_act);
            d.text(10, y + 12, fit(d, p.source, f_src, src_w), f_src, t.accent, "lm");
            auto ls = wrap(d, short_title(p.title), f_post, w - 20);
            for (size_t j = 0; j < ls.size(); ++j) d.text(10, y + 32 + j * 20.f, ls[j], f_post, t.text, "lm");
            if (i < n - 1) d.line({{10, y + rh - 1}, {w - 10, y + rh - 1}}, t.line, 1);
        }
    }
    footer(d, updated ? "Stand " + clock_hhmm(*updated) : "");
    return d.finish();
}

// ---------- Streaming-Neustarts ----------
StreamRenderer::StreamRenderer(int w_, int h_, std::string t) : Renderer(w_, h_, std::move(t)) {
    f_item = font(16, true);
    f_kind = font(13, true);
}

int StreamRenderer::row_at(int w_px, int h_px, int y_px) {
    float sc = scale_for(w_px, h_px);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float top = HEADER_H + 2, bot = lh - FOOTER_H, y = y_px / sc;
    float rh = (float)floordiv(bot - top, MAX_ROWS);
    if (y < top || y >= bot || rh <= 0) return -1;
    int row = (int)((y - top) / rh);
    return row < MAX_ROWS ? row : -1;
}

Image StreamRenderer::render(StreamState state, const std::vector<StreamItem>& items, std::optional<std::time_t> updated,
                             std::time_t now, const StreamLogos* logos) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, now, "");
    const float top = HEADER_H + 2, bot = h - FOOTER_H;
    if (state == StreamState::NoKey) {
        d.text(w / 2, h / 2 - 12, "Kein Key", f_big, t.muted, "mm");
        d.text(w / 2, h / 2 + 14, "TMDB-Key in den Einstellungen eintragen", f_act, t.muted, "mm");
    } else if (state == StreamState::Loading) {
        d.text(w / 2, h / 2, "Lade Daten …", f_big, t.muted, "mm");
    } else if (state == StreamState::Error) {
        d.text(w / 2, h / 2, "Keine Daten", f_big, t.muted, "mm");
    } else if (items.empty()) {
        d.text(w / 2, h / 2, "Keine Neustarts gefunden", f_big, t.muted, "mm");
    } else {
        const size_t n = std::min(items.size(), (size_t)MAX_ROWS);
        const float rh = (float)floordiv(bot - top, MAX_ROWS);
        const std::string lang = i18n::lang();
        for (size_t i = 0; i < n; ++i) {
            const StreamItem& it = items[i];
            const float y = top + i * rh;
            const std::string date = tmdb::format_date(it.date, lang);
            d.text(w - 10, y + 14, date, f_act, t.muted, "rm");
            const float title_w = w - 30 - d.textlength(date, f_act);
            d.text(10, y + 14, fit(d, it.title, f_item, title_w), f_item, t.text, "lm");
            const std::string kind = tr(it.series ? "Serie" : "Film");
            d.text(10, y + 32, kind, f_kind, t.accent, "lm");
            const float kx = 10 + d.textlength(kind, f_kind) + 10;
            // Anbieter: Logos (höchstens MAX_LOGOS, Rest „+n“); fehlt ein Logo, steht an seiner Stelle das Kürzel
            const int shown = tmdb::logos_shown(it.providers.size());
            float lx = kx;
            for (int k = 0; k < shown; ++k) {
                const StreamLogos::const_iterator none = logos ? logos->end() : StreamLogos::const_iterator();
                StreamLogos::const_iterator li = none;
                if (logos && (size_t)k < it.provider_ids.size()) li = logos->find(it.provider_ids[k]);
                if (logos && li != none && li->second && !li->second->empty()) {
                    const int px = pyround_i(LOGO_SIZE * s);
                    d.image(resize(*li->second, px, px), lx, y + 32 - LOGO_SIZE / 2.f);
                    lx += LOGO_SIZE + 4;
                } else {
                    const std::string& ab = it.providers[k];
                    d.text(lx, y + 32, ab, f_act, t.muted, "lm");
                    lx += d.textlength(ab, f_act) + 8;
                }
            }
            const std::string more = tmdb::logos_overflow(it.providers.size());
            if (!more.empty()) d.text(lx, y + 32, more, f_act, t.muted, "lm");
            if (i < n - 1) d.line({{10, y + rh - 1}, {w - 10, y + rh - 1}}, t.line, 1);
        }
    }
    // Pflichtangabe der Datenquellen (TMDB, Anbieterdaten JustWatch) steht immer in der Fußzeile
    footer(d, updated && state == StreamState::Ok ? "TMDB / JustWatch · Stand " + clock_hhmm(*updated) : "TMDB / JustWatch");
    return d.finish();
}

// ---------- Home Assistant ----------
static bool toggle_domain_name(const std::string& id) { return ha::toggle_domain(id); }
static std::string ha_format_value(const std::string& s) { return ha::format_value(s); }

HaRenderer::HaRenderer(int w_, int h_, std::string t) : Renderer(w_, h_, std::move(t)) {
    f_value = font(38, true);
    f_value_small = font(28, true);
    f_unit = font(17);
    f_name_ha = font(14, true);
}

std::vector<HaRenderer::Cell> HaRenderer::cells(size_t n, float w, float h) {
    std::vector<Cell> out;
    if (!n) return out;
    n = std::min(n, (size_t)4);
    float top = HEADER_H + 4, bot = h - FOOTER_H - 4, gap = 6;
    if (w < h) {   // Hochformat: Kacheln untereinander
        float ch = (bot - top - gap * (n - 1)) / n;
        for (size_t i = 0; i < n; ++i) out.push_back({gap, top + i * (ch + gap), w - gap, top + i * (ch + gap) + ch});
        return out;
    }
    float cw2 = (w - 3 * gap) / 2, ch2 = (bot - top - gap) / 2;
    if (n == 1) out.push_back({gap, top, w - gap, bot});
    else if (n == 2) {
        for (size_t i = 0; i < 2; ++i) out.push_back({gap + i * (cw2 + gap), top, gap + i * (cw2 + gap) + cw2, bot});
    } else {
        for (size_t i = 0; i < n; ++i) {
            size_t r = i / 2, c = i % 2;
            bool wide = n == 3 && i == 2;   // dritte Kachel über die ganze Breite
            float x0 = wide ? gap : gap + c * (cw2 + gap), x1 = wide ? w - gap : x0 + cw2;
            float y0 = top + r * (ch2 + gap);
            out.push_back({x0, y0, x1, y0 + ch2});
        }
    }
    return out;
}

int HaRenderer::cell_at(int w_px, int h_px, size_t n, int x_px, int y_px) {
    float sc = scale_for(w_px, h_px);
    float lw = sc == 1 ? (float)w_px : (float)pyround_i(w_px / sc);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float x = x_px / sc, y = y_px / sc;
    auto cs = cells(n, lw, lh);
    for (size_t i = 0; i < cs.size(); ++i)
        if (x >= cs[i].x0 && x < cs[i].x1 && y >= cs[i].y0 && y < cs[i].y1) return (int)i;
    return -1;
}

void HaRenderer::cell(Canvas& d, const Cell& c, const HaEntity& e, bool compact) {
    const Theme& t = g_theme;
    float cw = c.x1 - c.x0, ch = c.y1 - c.y0;
    d.rounded_rectangle(c.x0, c.y0, c.x1, c.y1, 8, t.card);
    d.text(c.x0 + 10, c.y0 + 15, fit(d, e.name, f_name_ha, cw - 20), f_name_ha, t.accent, "lm");
    float cy = c.y0 + 15 + (ch - 15) / 2 + 2;
    if (!e.available) {
        d.text(c.x0 + 10, cy - 4, e.state == "nicht gefunden" ? "nicht gefunden" : "Nicht verfügbar", f_unit, t.muted, "lm");
        return;
    }
    bool is_switch = e.toggle || (toggle_domain_name(e.id) && (e.state == "on" || e.state == "off"));
    if (is_switch) {
        // Schiebeschalter: Orange = an
        float tw = compact ? 40.f : 52.f, th = compact ? 22.f : 28.f, x = static_cast<float>(c.x0 + 12), y = cy - th / 2.f;
        d.rounded_rectangle(x, y, x + tw, y + th, th / 2, e.on ? t.bar : t.line);
        float r = th / 2 - 3, kx = e.on ? x + tw - th / 2 : x + th / 2;
        d.ellipse(kx - r, cy - r, kx + r, cy + r, e.on ? t.head_text : t.muted);
        d.text(x + tw + 12, cy, (e.on ? ha_format_value("on") : ha_format_value("off")), compact ? f_value_small : f_value, e.on ? t.text : t.muted, "lm");
        return;
    }
    std::string v = fit(d, ha_format_value(e.state), compact ? f_value_small : f_value, cw - 20);
    const Font& vf = compact ? f_value_small : f_value;
    d.text(c.x0 + 10, cy, v, vf, t.text, "lm");
    if (!e.unit.empty())
        d.text(c.x0 + 10 + d.textlength(v, vf) + 5, cy + (compact ? 4 : 6), fit(d, e.unit, f_unit, std::max(10.f, cw - 30 - d.textlength(v, vf))), f_unit, t.muted, "lm");
}

Image HaRenderer::render(const std::vector<HaEntity>& entities, bool has_data, const std::string& error,
                         std::optional<std::time_t> updated, std::time_t now) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, now, "");
    if (entities.empty()) {
        message(d, "Keine Entitäten eingetragen");
    } else if (!has_data) {
        message(d, error.empty() ? "Lade Home Assistant …" : error);
    } else {
        auto cs = cells(entities.size(), w, h);
        bool compact = entities.size() > 2 || w < h;
        for (size_t i = 0; i < cs.size() && i < entities.size(); ++i) cell(d, cs[i], entities[i], compact);
    }
    std::string left = has_data && !error.empty() ? fit(d, error, f_small, w - 110) : updated ? "Stand " + clock_hhmm(*updated) : "";
    footer(d, left);
    return d.finish();
}
// ---------- Spotify ----------
SpotifyRenderer::SpotifyRenderer(int w_, int h_, std::string t) : Renderer(w_, h_, std::move(t)) {
    f_track = font(22, true);
    f_artist = font(16);
    f_time = font(12);
}

SpotifyRenderer::Layout SpotifyRenderer::layout(float w, float h) {
    Layout l{};
    float top = HEADER_H + 8, bot = h - FOOTER_H - 8, ch = bot - top;
    l.btn_y = bot - 24;
    if (w >= h) {   // Querformat: Cover links, Text und Tasten rechts
        float cs = std::min(ch, w * 0.42f);
        l.cover_size = cs;
        l.cover_x = 14;
        l.cover_y = top + (ch - cs) / 2;
        l.text_x = 14 + cs + 18;
        l.text_w = w - 14 - l.text_x;
        l.title_y = top + 18;
        l.bar_y = bot - 76;
        l.times_y = l.bar_y + 14;
    } else {        // Hochformat: Cover oben mittig, darunter alles untereinander
        float cs = std::min(w - 40, ch * 0.46f);
        l.cover_size = cs;
        l.cover_x = (w - cs) / 2;
        l.cover_y = top;
        l.text_x = 20;
        l.text_w = w - 40;
        l.title_y = top + cs + 24;
        l.bar_y = bot - 76;
        l.times_y = l.bar_y + 14;
    }
    float mid = l.text_x + l.text_w / 2, step = std::min(64.f, l.text_w / 3.2f);
    l.btn_x[0] = mid - step; l.btn_x[1] = mid; l.btn_x[2] = mid + step;
    l.btn_r[0] = 18; l.btn_r[1] = 24; l.btn_r[2] = 18;
    return l;
}

int SpotifyRenderer::button_at(int w_px, int h_px, int x_px, int y_px) {
    float sc = scale_for(w_px, h_px);
    float lw = sc == 1 ? (float)w_px : (float)pyround_i(w_px / sc);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float x = x_px / sc, y = y_px / sc;
    Layout l = layout(lw, lh);
    if (x >= l.cover_x && x < l.cover_x + l.cover_size &&
        y >= l.cover_y && y < l.cover_y + l.cover_size) return COVER;
    for (int i = 0; i < 3; ++i) {
        // großzügige Trefferfläche (mindestens 44 Punkte), damit Finger sie sicher treffen
        float r = std::max(l.btn_r[i] + 6, 24.f);
        if (std::fabs(x - l.btn_x[i]) <= r && std::fabs(y - l.btn_y) <= r) return i;
    }
    return -1;
}

Image SpotifyRenderer::render(const spotify::Snapshot& snap, double now, const std::vector<float>* levels, int color, bool simulated) {
    using spotify::State;
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, 0, "");
    std::string footer_left = "Spotify";
    switch (snap.state) {
    case State::NotSetUp:
        message(d, "Spotify ist nicht fertig eingerichtet");
        break;
    case State::LoggedOut:
        message(d, "Bei Spotify anmelden: Einstellungen › Spotify");
        break;
    case State::Connecting:
        message(d, snap.error.empty() ? "Verbinde mit Spotify …" : snap.error);
        break;
    case State::Error:
        message(d, snap.error.empty() ? "Spotify nicht erreichbar" : snap.error);
        break;
    case State::Idle:
        message(d, "Es läuft gerade nichts – Wiedergabe in Spotify starten");
        break;
    default: {
        Layout l = layout(w, h);
        // Cover (oder dezenter Platzhalter)
        d.rounded_rectangle(l.cover_x, l.cover_y, l.cover_x + l.cover_size, l.cover_y + l.cover_size, 8, t.card);
        if (snap.cover && !snap.cover->empty()) {
            int px = std::max(1, pyround_i(l.cover_size * s));
            d.image(resize(*snap.cover, px, px), l.cover_x, l.cover_y);
        } else {
            d.text(l.cover_x + l.cover_size / 2, l.cover_y + l.cover_size / 2, "♪", font(40, true), t.muted, "mm");
        }
        bool wide = w >= h;
        float ty = l.title_y;
        d.text(l.text_x, ty, fit(d, snap.title, f_track, l.text_w), f_track, t.text, "lm");
        d.text(l.text_x, ty + 28, fit(d, snap.artist, f_artist, l.text_w), f_artist, t.accent, "lm");
        if (!snap.album.empty() && (wide || ty + 52 < l.bar_y - 10))
            d.text(l.text_x, ty + 50, fit(d, snap.album, f_time, l.text_w), f_time, t.muted, "lm");
        // Amplifier: Balken im freien Bereich zwischen Albumzeile und Fortschritt (nur Querformat, nur bei Wiedergabe)
        if (levels && !levels->empty() && wide && snap.playing) {
            const float ax = l.text_x, aw = l.text_w, ay0 = ty + 66, ay1 = l.bar_y - 14;
            const float ah = ay1 - ay0;
            if (ah >= 28) {
                const int n = (int)levels->size();
                const float gap = 3.f, bw = (aw - gap * (n - 1)) / n;
                for (int i = 0; i < n; ++i) {
                    float v = std::clamp((*levels)[(size_t)i], 0.f, 1.f);
                    float bh = std::max(2.f, v * ah);
                    float x0 = ax + i * (bw + gap);
                    Color c = v > 0.85f ? t.accent : t.bar;
                    if (color == 1) {
                        uint8_t rgb[3];
                        audio::level_rgb(v, rgb);
                        c = Color{rgb[0], rgb[1], rgb[2]};
                    } else if (color == 2) {   // Verlauf im Design: Balken → Akzent → Heiß
                        auto mix = [](Color a, Color b, float k) {
                            return Color{uint8_t(a.r + (b.r - a.r) * k), uint8_t(a.g + (b.g - a.g) * k), uint8_t(a.b + (b.b - a.b) * k)};
                        };
                        c = v < 0.6f ? mix(t.bar, t.accent, v / 0.6f) : mix(t.accent, t.hot, (v - 0.6f) / 0.4f);
                    }
                    d.rounded_rectangle(x0, ay1 - bh, x0 + bw, ay1, std::min(2.f, bw / 2), c);
                }
                if (simulated) d.text(ax + aw, ay0 + 6, tr("simuliert"), font(11), t.muted, "rm");
            }
        }
        // Fortschritt
        long long pos = spotify::live_progress(snap, now ? now : mono());
        float frac = snap.duration_ms > 0 ? std::min(1.f, (float)pos / snap.duration_ms) : 0.f;
        d.rounded_rectangle(l.text_x, l.bar_y, l.text_x + l.text_w, l.bar_y + 6, 3, t.line);
        if (frac > 0) d.rounded_rectangle(l.text_x, l.bar_y, l.text_x + std::max(6.f, l.text_w * frac), l.bar_y + 6, 3, t.bar);
        d.text(l.text_x, l.times_y, spotify::format_time(pos), f_time, t.muted, "lm");
        d.text(l.text_x + l.text_w, l.times_y, spotify::format_time(snap.duration_ms), f_time, t.muted, "rm");
        if (snap.controls) {   // Tasten nur, wenn Touch an ist – sonst wäre ein Tipp ohne Wirkung
            for (int i = 0; i < 3; ++i) {
                float cx = l.btn_x[i], cy = l.btn_y, r = l.btn_r[i];
                bool main = i == PLAY_PAUSE;
                d.ellipse(cx - r, cy - r, cx + r, cy + r, main ? t.bar : t.card);
                Color ink = main ? t.head_text : t.text;
                float k = r * 0.42f;
                if (i == PLAY_PAUSE && snap.playing) {
                    d.rectangle(cx - k * 0.8f, cy - k, cx - k * 0.2f, cy + k, ink);
                    d.rectangle(cx + k * 0.2f, cy - k, cx + k * 0.8f, cy + k, ink);
                } else if (i == PLAY_PAUSE) {
                    d.polygon({{cx - k * 0.6f, cy - k}, {cx - k * 0.6f, cy + k}, {cx + k, cy}}, ink);
                } else if (i == NEXT) {
                    d.polygon({{cx - k, cy - k}, {cx - k, cy + k}, {cx + k * 0.6f, cy}}, ink);
                    d.rectangle(cx + k * 0.7f, cy - k, cx + k, cy + k, ink);
                } else {
                    d.polygon({{cx + k, cy - k}, {cx + k, cy + k}, {cx - k * 0.6f, cy}}, ink);
                    d.rectangle(cx - k, cy - k, cx - k * 0.7f, cy + k, ink);
                }
            }
        }
        if (!snap.error.empty()) footer_left = snap.error;
        break;
    }
    }
    footer(d, fit(d, footer_left, f_small, w - 110));
    return d.finish();
}

spotify::Snapshot demo_spotify() {
    spotify::Snapshot s;
    s.state = spotify::State::Playing;
    s.title = "Numb";
    s.artist = "Linkin Park";
    s.album = "Meteora";
    s.progress_ms = 83000;
    s.duration_ms = 187000;
    s.playing = true;
    s.controls = true;
    return s;
}
// ---------- Beispieldaten ----------
std::vector<Friend> demo_friends() {
    auto f = [](uint64_t id, const char* name, const char* st, const char* act = "", bool mobile = false) {
        Friend x;
        x.user_id = id; x.name = name; x.status = st; x.activity = clean_text(act); x.mobile = mobile;
        return x;
    };
    return {f(1, "Fuchs", "online", "Spielt Battlefield 6 · 42 min"), f(2, "Basti", "dnd", "Streamt Battlefield 6"),
            f(3, "Lena", "idle", "🌙 gleich zurück"),                 f(4, "Kevin", "online", "Hört Numb – Linkin Park", true),
            f(5, "Mara", "online", ""),                                f(6, "Tom", "offline"),
            f(7, "Jonas", "online", "Spielt Counter-Strike 2 · 1 h 05 min"), f(8, "Sven", "idle", "")};
}

std::vector<StreamItem> demo_stream() {
    // Frei erfundene Beispieltitel für Vorschau und Tests
    auto mk = [](int id, const char* title, const char* date, bool series, std::vector<std::string> prov) {
        StreamItem it;
        it.id = id; it.title = title; it.date = date; it.series = series; it.providers = std::move(prov);
        static const std::map<std::string, int> ids = {{"NF", 8}, {"PV", 9}, {"D+", 337}, {"ATV+", 350}, {"WOW", 30}, {"RTL+", 2750}};
        for (auto& ab : it.providers) it.provider_ids.push_back(ids.at(ab));
        return it;
    };
    return {mk(1, "Der Leuchtturm am Ende der Welt", "2026-10-30", false, {"NF"}),
            mk(2, "Nordlicht – Staffel 2", "2026-10-28", true, {"PV", "D+"}),
            mk(3, "Fuchsbau", "2026-10-24", false, {"ATV+"}),
            mk(4, "Die Ersatzspieler von Hinterdorf", "2026-10-21", true, {"RTL+"}),
            mk(5, "Mitternachtsexpress", "2026-10-15", false, {"WOW", "NF"})};
}

StreamLogos demo_stream_logos() {
    // Platzhalter für Vorschau und Tests: einfarbige, abgerundete Kacheln mit dem Kürzel; kein Netz, keine Marken-Logos
    struct Spec { int id; const char* ab; Color c; };
    const Spec specs[] = {{8, "NF", {150, 30, 40}}, {9, "PV", {30, 110, 160}}, {337, "D+", {40, 60, 140}},
                          {350, "TV", {60, 60, 60}}, {30, "WOW", {170, 70, 30}}, {2750, "RTL", {130, 40, 130}}};
    StreamLogos out;
    for (const auto& sp : specs) {
        Canvas c(64, 64, 1.f, sp.c);
        c.text(32, 32, sp.ab, Font{22, true}, Color{255, 255, 255}, "mm");
        Image img = c.finish();
        tmdb::round_corners(img.px, img.w, img.h, 64 * 0.2f);
        out[sp.id] = std::make_shared<const Image>(std::move(img));
    }
    return out;
}

std::vector<HaEntity> demo_ha() {
    auto mk = [](const char* id, const char* name, const char* state, const char* unit, bool toggle) {
        HaEntity e;
        e.id = id; e.name = name; e.state = state; e.unit = unit; e.available = true;
        e.toggle = toggle; e.on = std::string(state) == "on";
        return e;
    };
    return {mk("sensor.wohnzimmer_temperatur", "Wohnzimmer", "21.53", "°C", false),
            mk("sensor.solar_leistung", "Solar", "2840", "W", false),
            mk("light.schreibtisch", "Schreibtischlampe", "on", "", true),
            mk("switch.kaffeemaschine", "Kaffeemaschine", "off", "", true)};
}

std::vector<Bf6Player> demo_players() {
    auto p = [](const char* n, int rank, const char* rn, double kd, double win, double kpm, double hrs) {
        Bf6Player x;
        x.query = x.name = n; x.rank = rank; x.rank_name = rn; x.kd = kd; x.win_pct = win; x.kpm = kpm; x.hours = hrs;
        return x;
    };
    Bf6Player typo;
    typo.query = typo.name = "Tippfehler";
    typo.error = "nicht gefunden";
    return {p("Fuchs", 174, "First Lieutenant III", 3.02, 51.6, 0.74, 189.9), p("Basti", 121, "Captain I", 1.48, 54.2, 0.61, 96.3),
            p("Lena", 88, "Sergeant Major II", 1.12, 49.0, 0.52, 61.0),       p("Kevin", 57, "Staff Sergeant III", 0.94, 47.5, 0.44, 40.2),
            p("Mara", 33, "Corporal II", 0.81, 50.3, 0.39, 18.7),             typo};
}

}  // namespace fl::render
