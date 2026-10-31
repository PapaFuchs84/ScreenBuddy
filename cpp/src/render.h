// © 2026 FuchsLabs
// Zeichnet die Seiten. Alle Maße sind für das 3,5"-Display (480 × 320) ausgelegt; größere Displays zeichnen in
// denselben logischen Koordinaten, die Leinwand rechnet sie auf echte Pixel um (Schrift wird groß gerendert).
#pragma once

#include <array>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "theme.h"
#include "gfx.h"
#include "models.h"
#include "spotify.h"

namespace fl::render {

// Farben des aktuellen Designs (hell, dunkel oder eigen)
struct Theme {
    theme::Style style = theme::Style::FuchsLabs;
    Color bg, card, line, orange, accent, text, muted, head_text, warn, hot, down;
    Color bar;   // Balken/Schalter; gleich der Kopfzeilenfarbe, bei "An Windows anpassen" die kontrastgeprüfte Akzentfarbe
    std::map<std::string, Color> status;  // online | idle | dnd | offline
};
const Theme& theme();
void set_theme(const std::string& name, const std::map<std::string, std::string>& custom = {});
void apply_theme(const ojson& display);   // theme: hell | dunkel | windows | custom:<id>; theme_inline = ungespeicherte Vorschau
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
std::vector<Friend> sort_friends(std::vector<Friend> friends);
std::vector<Bf6Player> sort_players(std::vector<Bf6Player> players);

class Renderer {
public:
    Renderer(int w, int h, std::string title);
    virtual ~Renderer() = default;

    int rows_per_page() const;
    static int row_at(int w, int h, int y);
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
    enum Zone { ZONE_RAM = 0, ZONE_DISK = 1, ZONE_NET = 2 };
    // Shared logical rectangles for drawing and hit testing, ordered CPU/GPU/RAM/Disk/Net.
    // CPU/GPU are informational (zone -1); the other regions open their Windows tool.
    struct Region { float x, y, w, h; int zone; };
    static std::array<Region, 5> regions(float w, float h, theme::Style style);
    // Welche systemspezifische Region liegt bei Pixel (x, y)? -1 = keine Aktion.
    static int zone_at(int w_px, int h_px, int x_px, int y_px);

private:
    void grouped_system(Canvas& d, const std::array<Region, 5>& areas, const SysSnapshot& snap);
    void expressive_system(Canvas& d, const std::array<Region, 5>& areas, const SysSnapshot& snap);
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
    // Welcher Beitrag liegt an Pixelzeile y (Kachel w × h)? -1 = keiner (Kopf-/Fußzeile). Für "Tippen öffnet Beitrag".
    static int row_at(int w, int h, int y);

private:
    Font f_post, f_src;
    std::vector<std::string> wrap(Canvas& d, const std::string& text, const Font& f, float max_w, int lines = 2);
};

// Streaming-Neustarts: bis zu 6 Zeilen (Titel, Datum, Film/Serie, Anbieter-Logos, Rückfall Kürzel), Fußzeile „TMDB / JustWatch“
class StreamRenderer : public Renderer {
public:
    StreamRenderer(int w, int h, std::string title);
    Image render(StreamState state, const std::vector<StreamItem>& items, std::optional<std::time_t> updated,
                 std::time_t now = 0, const StreamLogos* logos = nullptr);
    static constexpr int MAX_ROWS = tmdb::MAX_ITEMS;

private:
    Font f_item, f_kind;
};

// Home Assistant: bis zu 4 Kacheln (Sensorwert mit Einheit oder Schalter mit Schiebeschalter-Symbol)
class HaRenderer : public Renderer {
public:
    HaRenderer(int w, int h, std::string title);
    Image render(const std::vector<HaEntity>& entities, bool has_data, const std::string& error,
                 std::optional<std::time_t> updated, std::time_t now = 0);
    struct Cell {
        float x0, y0, x1, y1;   // logische Koordinaten
    };
    static std::vector<Cell> cells(size_t n, float w, float h);
    // Welche Kachel liegt bei Pixel (x, y) einer Kachelfläche w × h? -1 = keine. Für „Tippen schaltet“.
    static int cell_at(int w_px, int h_px, size_t n, int x_px, int y_px);

private:
    Font f_value, f_value_small, f_unit, f_name_ha;
    void cell(Canvas& d, const Cell& c, const HaEntity& e, bool compact);
};

// Spotify: Cover, Titel, Fortschritt und drei Tasten (Zurück, Wiedergabe/Pause, Weiter)
class SpotifyRenderer : public Renderer {
public:
    SpotifyRenderer(int w, int h, std::string title);
    // levels: optional 0..1 je Balken (Amplifier); nur im Querformat und bei laufender Wiedergabe gezeichnet
    // color: 0 Design (Balken, oben Akzent), 1 Pegel (Grün/Gelb/Rot), 2 Verlauf im Design (Balken → Akzent → Heiß)
    // simulated: Balken sind keine Messung (Remote-Wiedergabe) → kleiner Hinweis "simuliert"
    Image render(const spotify::Snapshot& snap, double now = 0, const std::vector<float>* levels = nullptr,
                 int color = 1, bool simulated = false);
    enum Button { PREVIOUS = 0, PLAY_PAUSE = 1, NEXT = 2 };
    // Welche Taste liegt bei Pixel (x, y) einer Kachelfläche w × h? -1 = keine. Für „Tippen steuert Spotify“.
    static int button_at(int w_px, int h_px, int x_px, int y_px);

private:
    struct Layout {
        float cover_x, cover_y, cover_size;
        float text_x, text_w, title_y, bar_y, times_y;
        float btn_y, btn_x[3], btn_r[3];
    };
    static Layout layout(float w, float h);
    Font f_track, f_artist, f_time;
};

std::string ago(std::optional<std::time_t> t, std::time_t now = 0);
spotify::Snapshot demo_spotify();
std::vector<HaEntity> demo_ha();
std::vector<StreamItem> demo_stream();
StreamLogos demo_stream_logos();   // Platzhalter-Kacheln zu den IDs aus demo_stream(), ohne Netz
std::vector<Friend> demo_friends();
std::vector<Bf6Player> demo_players();

}  // namespace fl::render
