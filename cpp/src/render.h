// © 2026 FuchsLabs
// Zeichnet die Seiten. Alle Maße sind für das 3,5"-Display (480 × 320) ausgelegt; größere Displays zeichnen in
// denselben logischen Koordinaten, die Leinwand rechnet sie auf echte Pixel um (Schrift wird groß gerendert).
#pragma once

#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "models.h"

namespace fl::render {

// Farben des aktuellen Designs (hell, dunkel oder eigen)
struct Theme {
    Color bg, card, line, orange, accent, text, muted, head_text, warn, hot, down;
    std::map<std::string, Color> status;  // online | idle | dnd | offline
};
const Theme& theme();
void set_theme(const std::string& name, const std::map<std::string, std::string>& custom = {});
void apply_theme(const ojson& display);   // aus den Display-Einstellungen
std::mutex& mutex();                      // Zeichnen und Design wechseln nur unter dieser Sperre

std::string hex_color(Color c);
std::optional<Color> parse_color(const std::string& s);
double contrast(Color a, Color b);
ojson presets_json();                     // Farben der Vorlagen für die Einstellungsseite

std::string clean_text(const std::string& text);  // entfernt Emoji/Symbole, die als Kästchen erschienen
std::string fmt_rate(std::optional<double> bytes_per_s, const std::string& unit);

struct Box {
    int x, y, w, h;
};
// Breite Leisten (z. B. 1920 × 720) zeigen mehrere Seiten nebeneinander. Rückgabe: je Kachel x, y, Breite, Höhe.
std::vector<Box> tile_layout(int w, int h, int pages = 99);
Image compose(int w, int h, const std::vector<Box>& boxes, const std::vector<Image>& frames);
float scale_for(int w, int h);

extern const std::map<std::string, int> STATUS_ORDER;

class Renderer {
public:
    Renderer(int w, int h, std::string title);
    virtual ~Renderer() = default;

    int rows_per_page() const;
    int pages(size_t items) const;
    Image welcome(const std::string& title, const std::string& text);
    Image render_friends(std::vector<Friend> friends, int page = 0, bool connected = true, const std::string& message = "",
                         std::time_t now = 0);

protected:
    struct Ctx;
    int W, H;          // echte Pixel
    float s;           // Vergrößerung
    float w, h;        // logische Größe
    std::string title;
    Font f_title, f_small, f_name, f_act, f_big;
    static constexpr int ICON = 26;

    Font font(int size, bool bold = false) const;
    void header(Canvas& d, std::time_t now, const std::string& info);
    void footer(Canvas& d, const std::string& left);
    void message(Canvas& d, const std::string& text);
    std::string fit(Canvas& d, const std::string& text, const Font& f, float max_w);
    float badges(Canvas& d, const std::vector<std::string>& platforms, float right, float cy);
    void row(Canvas& d, const Friend& f, float y);
    void avatar(Canvas& d, const Friend& f, float x, float y, bool faded);
};

class Bf6Renderer : public Renderer {
public:
    Bf6Renderer(int w, int h, std::string title);
    Image render(std::vector<Bf6Player> players, int page, std::optional<std::time_t> updated, std::time_t now = 0);

private:
    Font f_stat, f_pos;
    void bf_row(Canvas& d, const Bf6Player& p, int pos, float y);
};

class SysRenderer : public Renderer {
public:
    SysRenderer(int w, int h, std::string title, std::string net_unit);
    Image render(const SysSnapshot& snap, std::time_t now = 0);

private:
    std::string net_unit;
    Font f_card, f_huge, f_val, f_lbl;
    struct RowVal {
        std::string label, value;
        Color col;
    };
    Color temp_col(std::optional<double> t) const;
    void card(Canvas& d, float x, float y, float w, float h, const std::string& title, const std::string& sub = "");
    void bar(Canvas& d, float x, float y, float w, std::optional<double> pct, float h = 6);
    void usage_card(Canvas& d, float x, float y, float w, float h, const std::string& title, const std::string& sub,
                    std::optional<double> pct, const std::vector<RowVal>& rows);
    void bar_card(Canvas& d, float x, float y, float w, float h, const std::string& title, const std::string& detail,
                  std::optional<double> pct);
    void arrow(Canvas& d, float cx, float cy, bool down, Color col);
    void net_card(Canvas& d, float x, float y, float w, float h, const SysSnapshot& s);
};

class RssRenderer : public Renderer {
public:
    RssRenderer(int w, int h, std::string title);
    Image render(const std::vector<Post>& posts, std::optional<std::time_t> updated, std::time_t now = 0);
    static constexpr int MAX_ROWS = 4;

private:
    Font f_post, f_src;
    std::vector<std::string> wrap(Canvas& d, const std::string& text, const Font& f, float max_w, int lines = 2);
};

std::string ago(std::optional<std::time_t> t, std::time_t now = 0);
std::vector<Friend> demo_friends();
std::vector<Bf6Player> demo_players();

}  // namespace fl::render
