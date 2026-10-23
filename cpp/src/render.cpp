// © 2026 FuchsLabs
#include "render.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <regex>

#include "brand.h"
#include "i18n.h"
#include "util.h"

namespace fl::render {

namespace {

constexpr int HEADER_H = 34, FOOTER_H = 18, ROW_H = 44, AVATAR = 32;

Theme g_theme;
std::mutex g_mutex;

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

std::optional<Color> parse_color(const std::string& in) {
    std::string s = trim(in);
    while (!s.empty() && s[0] == '#') s.erase(0, 1);
    static const std::regex rx("[0-9a-fA-F]{6}");
    if (!std::regex_match(s, rx)) return std::nullopt;
    auto h = [&](int i) { return (uint8_t)std::stoi(s.substr(i, 2), nullptr, 16); };
    return Color{h(0), h(2), h(4)};
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
    Theme& t = g_theme;
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

void apply_theme(const ojson& dp) {
    auto str = [&](const char* k, const char* def) {
        return dp.contains(k) && dp[k].is_string() ? dp[k].get<std::string>() : std::string(def);
    };
    if (str("theme", "hell") == "eigen") {
        std::map<std::string, std::string> custom;
        for (const char* k : {"bg", "card", "header", "accent", "text", "muted"}) custom[k] = str((std::string("color_") + k).c_str(), "");
        set_theme(str("color_base", "hell"), custom);
    } else {
        set_theme(str("theme", "hell"));
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

Font Renderer::font(int size, bool bold) const { return Font{(float)std::max(6, pyround_i(size * s)), bold}; }

int Renderer::rows_per_page() const { return std::max(1, ((int)h - HEADER_H - FOOTER_H) / ROW_H); }

int Renderer::row_at(int w_px, int h_px, int y_px) {
    float sc = scale_for(w_px, h_px);
    float lh = sc == 1 ? (float)h_px : (float)pyround_i(h_px / sc);
    float ly = y_px / sc, top = HEADER_H + 2;
    int per = std::max(1, ((int)lh - HEADER_H - FOOTER_H) / ROW_H);
    if (ly < top || ly >= lh - FOOTER_H || ly >= top + per * ROW_H) return -1;
    return (int)((ly - top) / ROW_H);
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
    d.rectangle(0, 0, w, HEADER_H - 1, t.orange);
    float x = 8;
    d.fox(x, (float)((HEADER_H - ICON) / 2), pyround_i(ICON * s), false);
    x += ICON + 6;
    std::string clock = clock_hhmm(now ? now : std::time(nullptr));
    d.text(w - 8, HEADER_H / 2, clock, f_title, t.head_text, "rm");
    float cx = w - 8 - d.textlength(clock, f_title) - 12;
    float title_width = cx - x - (info.empty() ? 0 : d.textlength(tr(info), f_small) + 10);
    d.text(x, HEADER_H / 2, fit(d, tr(title), f_title, std::max(0.f, title_width)), f_title, t.head_text, "lm");
    d.text(cx, HEADER_H / 2, info, f_small, t.head_text, "rm");
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
        for (int i = 0; i < per && page * per + i < (int)friends.size(); ++i) row(d, friends[page * per + i], body_top + i * ROW_H);
    }
    footer(d, n > 1 ? strprintf("Seite %d/%d", ((page % n) + n) % n + 1, n) : "");
    return d.finish();
}

float Renderer::badges(Canvas& d, const std::vector<std::string>& platforms, float right, float cy) {
    // Kleine Plattform-Marken (D/S/X) rechts oben in der Zeile; Rückgabe: belegte Breite
    static const std::map<std::string, std::pair<std::string, Color>> BADGE = {
        {"discord", {"D", {88, 101, 242}}}, {"steam", {"S", {35, 98, 148}}}, {"xbox", {"X", {16, 124, 16}}}};
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
    float ax = 10, ay = y + (ROW_H - AVATAR) / 2;
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
        max_w -= badges(d, f.platforms, w - 10, y + 13);
    Color name_col = f.status != "offline" ? t.text : t.muted;
    std::string line2 = !f.activity.empty() ? f.activity : (STATUS_LABEL.count(f.status) ? STATUS_LABEL.at(f.status) : "");
    if (!line2.empty()) {
        d.text(tx, y + 13, fit(d, f.name, f_name, max_w), f_name, name_col, "lm");
        Color act_col = starts_with_any(f.activity, {"Spielt", "Streamt"}) ? t.accent : t.muted;
        d.text(tx, y + 31, fit(d, line2, f_act, max_w), f_act, act_col, "lm");
    } else {
        d.text(tx, y + ROW_H / 2, fit(d, f.name, f_name, max_w), f_name, name_col, "lm");
    }
    d.line({{tx, y + ROW_H - 1}, {w - 10, y + ROW_H - 1}}, t.line, 1);
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
            bf_row(d, players[page * per + i], page * per + i + 1, body_top + i * ROW_H);
    }
    std::string left = updated ? "Stand " + clock_hhmm(*updated) : "";
    if (n > 1) left = strprintf("Seite %d/%d", page + 1, n) + (left.empty() ? "" : " · " + left);
    footer(d, left);
    return d.finish();
}

void Bf6Renderer::bf_row(Canvas& d, const Bf6Player& p, int pos, float y) {
    const Theme& t = g_theme;
    float cy = y + ROW_H / 2;
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
    if (p.kd) d.text(sx, y + 13, strprintf("K/D %.2f", *p.kd), f_stat, t.text, "rm");
    if (!detail.empty()) d.text(sx, y + 31, detail, f_act, t.muted, "rm");
    float max_w = sx - stat_w - tx;
    d.text(tx, y + 13, fit(d, p.name, f_name, max_w), f_name, p.error.empty() ? t.text : t.muted, "lm");
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
    d.text(tx, y + 31, fit(d, line2, f_act, max_w), f_act, col, "lm");
    d.line({{tx, y + ROW_H - 1}, {w - 10, y + ROW_H - 1}}, t.line, 1);
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

Image SysRenderer::render(const SysSnapshot& sn, std::time_t now) {
    const Theme& t = g_theme;
    Canvas d(W, H, s, t.bg);
    header(d, now, sn.hwinfo ? "" : "ohne HWiNFO");
    float top = HEADER_H + 4, bot = h - FOOTER_H - 4, gap = 6;
    float lw = (float)floordiv(w - 3 * gap, 2);
    float lx = gap, rx = gap * 2 + lw;
    float rw = w - rx - gap;
    float half = (float)floordiv(bot - top - gap, 2);
    usage_card(d, lx, top, lw, half, "CPU", sn.cpu_name, sn.cpu_pct,
               {{"Temp", num(sn.cpu_temp, 0, "°C"), temp_col(sn.cpu_temp)},
                {"Takt", num(sn.cpu_ghz, 2, "GHz"), t.text},
                {"Lüfter", num(sn.cpu_fan, 0, "RPM"), t.text}});
    std::string vram = sn.gpu_mem_used && sn.gpu_mem_total && *sn.gpu_mem_total
                           ? num(*sn.gpu_mem_used / 1024, 1) + "/" + num(*sn.gpu_mem_total / 1024, 0) + " GB"
                           : "–";
    usage_card(d, lx, top + half + gap, lw, bot - top - half - gap, "GPU", sn.gpu_name, sn.gpu_pct,
               {{"Temp", num(sn.gpu_temp, 0, "°C"), temp_col(sn.gpu_temp)},
                {"Lüfter", num(sn.gpu_fan, 0, "%"), t.text},
                {"VRAM", vram, t.text}});
    float h_small = 62;
    std::string ram = sn.ram_used ? num(sn.ram_used, 1) + " / " + num(sn.ram_total, 0) + " GB" : "–";
    bar_card(d, rx, top, rw, h_small, "RAM", ram, sn.ram_pct);
    std::string disk = sn.disk_label + " " + (sn.disk_temp ? num(sn.disk_temp, 0, "°C") : tr("belegt"));
    bar_card(d, rx, top + h_small + gap, rw, h_small, "Datenträger", disk, sn.disk_pct);
    float ny = top + 2 * (h_small + gap);
    net_card(d, rx, ny, rw, bot - ny, sn);
    footer(d, "");
    return d.finish();
}

void SysRenderer::card(Canvas& d, float x, float y, float cw, float ch, const std::string& ctitle, const std::string& sub) {
    const Theme& t = g_theme;
    d.rounded_rectangle(x, y, x + cw, y + ch, 8, t.card);
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
        Color c = *pct >= 90 ? t.hot : t.orange;
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
    arrow(d, x + 16, y + 56, false, t.orange);
    d.text(x + cw - 10, y + 56, fmt_rate(sn.net_up, net_unit), f_val, t.text, "rm");
    // Verlauf der letzten 60 s (Download grün, Upload orange)
    float gx0 = x + 10, gx1 = x + cw - 10, gy0 = y + 70, gy1 = y + ch - 8;
    if (gy1 - gy0 < 10) return;
    double peak = 1.0;
    for (double v : sn.down_hist) peak = std::max(peak, v);
    for (double v : sn.up_hist) peak = std::max(peak, v);
    for (auto [hist, c] : {std::make_pair(&sn.down_hist, t.down), std::make_pair(&sn.up_hist, t.orange)}) {
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
