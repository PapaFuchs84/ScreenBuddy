// Render snapshots of the shipped example plugins (plugin_render.cpp draw_page).
// Fixed sample data, fixed clock/date, fixed sizes (landscape 480x320, portrait 320x480, small 240x160 and 160x240), fixed theme matrix.
// Offline: no network, no port 8765, no audio, no settings store. Local Direct2D/DirectWrite rendering only.
// Usage: fl-plugin-snapshot-tests [update] [examples-dir] [snapshot-dir]
//   default: compare against the baseline in snapshot-dir (hashes.txt, PNGs are for human review)
//   update : rewrite the baseline (hashes.txt + PNGs)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "plugin_render.h"
#include "plugins.h"
#include "render.h"
#include "theme.h"

namespace fs = std::filesystem;
using namespace fl;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

namespace {
struct Size { const char* name; int w, h; };
constexpr Size kSizes[] = {{"landscape", 480, 320}, {"portrait", 320, 480},
                           {"landscape-small", 240, 160}, {"portrait-small", 160, 240}};
// Theme matrix. "dunkel" is the reference theme (unprefixed names = Phase-0 baseline keys); every theme is rendered in all four
// sizes (both orientations, normal and small); all others are prefixed "<theme>.". Nothing depends on the OS: "windows" is an inline theme
// built from fixed WindowsSettings (the real "windows" key would read the registry), custom themes are inline (no files).
struct ThemeCase { std::string name; ojson inline_theme; };  // inline_theme null: built-in key = name

theme::Custom make_custom(const std::string& name, theme::Style style, bool dark, const theme::Palette& colors) {
    theme::Custom custom;
    custom.name = name;
    custom.style = style;
    custom.base = dark ? "dunkel" : "hell";
    custom.dark = dark;
    custom.colors = colors;
    return custom;
}

std::vector<ThemeCase> theme_cases() {
    std::vector<ThemeCase> cases{{"dunkel", nullptr}, {"hell", nullptr}};
    theme::WindowsSettings windows;  // fixed: light, accent 0,120,212, no high contrast
    windows.light = true;
    windows.has_accent = true;
    cases.push_back({"windows", theme::to_json(make_custom("Windows fest", theme::Style::FuchsLabs, false,
                                                           theme::windows_palette(windows)))});
    theme::Palette own = theme::builtin(false);
    own.bg = {232, 240, 236}; own.card = {250, 253, 251}; own.header = {0, 110, 90}; own.accent = {0, 96, 80};
    own.text = {16, 40, 32}; own.muted = {70, 96, 86};
    own = theme::correct_contrast(own);
    cases.push_back({"custom", theme::to_json(make_custom("Eigen fest", theme::Style::FuchsLabs, false, own))});
    for (auto style : {theme::Style::Cupertino, theme::Style::Fluent, theme::Style::Material})
        cases.push_back({std::string("custom-") + theme::style_name(style),
                         theme::to_json(make_custom("Profil fest", style, false, theme::profile_palette(style, false)))});
    return cases;
}
std::string hash_image(const Image& image) {
    uint64_t h = 1469598103934665603ull;  // FNV-1a 64 over size + ARGB pixels
    auto mix = [&](uint32_t v) { for (int i = 0; i < 4; ++i) { h ^= (v >> (8 * i)) & 0xFF; h *= 1099511628211ull; } };
    mix(static_cast<uint32_t>(image.w));
    mix(static_cast<uint32_t>(image.h));
    for (uint32_t p : image.px) mix(p);
    char text[17];
    std::snprintf(text, sizeof text, "%016llx", static_cast<unsigned long long>(h));
    return text;
}

Image draw(const plugins::Page& page, const ThemeCase& theme_case, const Size& size, bool sample) {
    std::lock_guard lock(render::mutex());
    if (theme_case.inline_theme.is_null()) render::apply_theme({{"theme", theme_case.name}});
    else render::apply_theme({{"theme_inline", theme_case.inline_theme}});
    return plugins::draw_page(page, size.w, size.h, sample);
}

// Contrast rule for everything plugin pages draw as text: body text and card labels (text/muted on card), page title
// (head_text on header band), footer note (muted on bg, warn on bg for "data missing"). Reports violations; never changes a palette.
// Known violations reported to the palette owners (Codex review round 2); palettes are deliberately NOT changed here.
// Each entry must still violate (a fixed palette must be removed from this list) so the list cannot go stale.
std::map<std::string, bool> g_known_findings = {};
struct ContrastInput { std::string name; Color bg, card, header, head_text, text, muted, warn; };
void check_contrast(const ContrastInput& in) {
    struct Pair { const char* what; Color fg, back; };
    const Pair pairs[] = {{"text/bg", in.text, in.bg}, {"text/card", in.text, in.card}, {"muted/card", in.muted, in.card},
                          {"header text/header", in.head_text, in.header}, {"footer muted/bg", in.muted, in.bg},
                          {"footer warn/bg", in.warn, in.bg}};
    for (const auto& pair : pairs) {
        const double ratio = theme::contrast(pair.fg, pair.back);
        const bool ok = ratio >= theme::MIN_TEXT;
        const auto known = g_known_findings.find(in.name + "|" + pair.what);
        if (!ok && known != g_known_findings.end()) {
            known->second = true;
            ++passed;
            std::printf("KNOWN CONTRAST FINDING %s: %s %.2f:1 (needs %.1f:1)\n", in.name.c_str(), pair.what, ratio, theme::MIN_TEXT);
            continue;
        }
        CHECK(ok);
        if (!ok) std::printf("CONTRAST VIOLATION %s: %s %.2f:1 (needs %.1f:1)\n", in.name.c_str(), pair.what, ratio, theme::MIN_TEXT);
    }
}
ContrastInput contrast_input(const std::string& name) {  // from the live render theme (call under render::mutex())
    const auto& t = render::theme();
    return {name, t.bg, t.card, t.orange, t.head_text, t.text, t.muted, t.warn};
}
ContrastInput contrast_input(const std::string& name, const theme::Resolved& r) {  // mirrors render.cpp set_resolved
    const bool dark = r.base == "dunkel";
    const Color warn_light{240, 178, 50}, warn_dark{150, 95, 0};
    const Color warn = (dark || theme::contrast(warn_light, r.colors.bg) > theme::contrast(warn_dark, r.colors.bg)) ? warn_light : warn_dark;
    return {name, r.colors.bg, r.colors.card, r.colors.header, theme::readable_on(r.colors.header), r.colors.text,
            r.colors.muted, warn};
}

void write_png(const fs::path& path, const Image& image) {
    std::ofstream out(path, std::ios::binary);
    out << encode_png(image);
}
}

int main(int argc, char** argv) {
    bool update = false;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "update") update = true;
        else positional.push_back(argv[i]);
    }
    const fs::path examples = positional.size() > 0 ? positional[0] : FL_PLUGIN_EXAMPLE_DIR;
    const fs::path snapshots = positional.size() > 1 ? positional[1] : FL_PLUGIN_SNAPSHOT_DIR;
    gfx_init();

    std::vector<fs::path> files;
    for (const auto& file : fs::recursive_directory_iterator(examples))
        if (file.is_regular_file() && file.path().filename().string().ends_with(".flplugin.json")) files.push_back(file.path());
    std::sort(files.begin(), files.end());
    CHECK(!files.empty());

    // name -> (hash, image); name = "[<theme>.]<id>.<variant>.<orientation>" (no prefix for dunkel)
    const auto themes = theme_cases();
    for (const auto& theme_case : themes) {  // every theme must be valid and pass the text contrast rule (>= 4.5:1)
        if (!theme_case.inline_theme.is_null()) {
            std::string err;
            auto parsed_theme = theme::parse(theme_case.inline_theme, err);
            CHECK(parsed_theme);
            if (!parsed_theme) { std::printf("Invalid theme %s: %s\n", theme_case.name.c_str(), err.c_str()); continue; }
        }
        std::lock_guard lock(render::mutex());  // contrast of what the renderer really uses (built-in hell/dunkel included)
        if (theme_case.inline_theme.is_null()) render::apply_theme({{"theme", theme_case.name}});
        else render::apply_theme({{"theme_inline", theme_case.inline_theme}});
        check_contrast(contrast_input(theme_case.name));
    }
    {  // built-in "windows" key resolved with fixed settings (the registry is never read): light, dark, accent, high contrast
        theme::WindowsSettings light;
        light.light = true; light.has_accent = true;
        theme::WindowsSettings dark = light;
        dark.light = false;
        theme::WindowsSettings plain;           // no accent
        theme::WindowsSettings high = light;
        high.high_contrast = true;
        const theme::Store store(fs::temp_directory_path() / "fl-plugin-snapshot-no-themes");  // never created or read
        const std::pair<const char*, const theme::WindowsSettings*> windows_cases[] = {
            {"windows-light", &light}, {"windows-dark", &dark}, {"windows-no-accent", &plain}, {"windows-high-contrast", &high}};
        for (const auto& [name, settings] : windows_cases) {
            const auto resolved = theme::resolve("windows", store, settings);
            CHECK(resolved.found);
            check_contrast(contrast_input(name, resolved));
        }
        for (bool is_dark : {false, true})  // dark profile palettes of the optional styles
            for (auto style : {theme::Style::Cupertino, theme::Style::Fluent, theme::Style::Material})
                check_contrast(contrast_input(std::string("profile-") + theme::style_name(style) + (is_dark ? "-dark" : "-light"),
                    theme::resolve_custom(make_custom("p", style, is_dark, theme::profile_palette(style, is_dark)))));
    }
    for (const auto& [key, seen] : g_known_findings) {
        CHECK(seen);
        if (!seen) std::printf("Known contrast finding no longer violates (remove from list): %s\n", key.c_str());
    }
    std::map<std::string, std::pair<std::string, Image>> actual;
    for (const auto& file : files) {
        std::ifstream input(file, std::ios::binary);
        const std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        auto parsed = plugins::parse_manifest(data);
        CHECK(parsed);
        if (!parsed) { std::printf("Invalid example: %s (%s)\n", file.filename().string().c_str(), parsed.error.c_str()); continue; }
        const auto& manifest = *parsed.manifest;
        plugins::RenderContext context;
        context.time = "12:34";
        context.date = "2026-01-01";
        struct Variant { const char* name; bool sample; bool with_data; };
        std::vector<Variant> variants{{"sample", true, true}};
        if (!manifest.sources.empty()) variants.push_back({"missing", false, false});  // live mode without response
        for (const auto& variant : variants) {
            context.sources = variant.with_data ? plugins::sample_data(manifest) : decltype(context.sources){};
            const auto page = plugins::render_page(manifest, context);
            for (const auto& theme_case : themes) for (const auto& size : kSizes) {
                const Image first = draw(page, theme_case, size, variant.sample);
                CHECK(first.w == size.w && first.h == size.h);
                const std::string hash = hash_image(first);
                CHECK(hash == hash_image(draw(page, theme_case, size, variant.sample)));  // deterministic within one process
                const std::string prefix = theme_case.name == "dunkel" ? "" : theme_case.name + ".";
                actual[prefix + manifest.id + "." + variant.name + "." + size.name] = {hash, first};
            }
        }
    }

    // Phase 2b: v2 ohne layout zeichnet pixelidentisch wie v1 (derselbe Inhalt, nur schema_version 2 und Block-IDs).
    if (fs::exists(examples / "offline-focus.flplugin.json")) {
        std::ifstream input(examples / "offline-focus.flplugin.json", std::ios::binary);
        auto j = plugins::Json::parse(std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()));
        j["schema_version"] = 2;
        const char* ids[] = {"uhr", "dauer", "heute", "fortschritt"};
        for (size_t i = 0; i < 4; ++i) j["widgets"][i]["id"] = ids[i];
        auto v2 = plugins::parse_manifest(j.dump());
        CHECK(v2);
        if (v2) {
            CHECK(!v2.manifest->layout);
            plugins::RenderContext context;
            context.time = "12:34";
            context.date = "2026-01-01";
            context.sources = plugins::sample_data(*v2.manifest);
            const auto page = plugins::render_page(*v2.manifest, context);
            CHECK(!page.layout);
            for (const auto& theme_case : themes) for (const auto& size : kSizes) {
                const std::string prefix = theme_case.name == "dunkel" ? "" : theme_case.name + ".";
                auto base = actual.find(prefix + "offline-focus.sample." + size.name);
                CHECK(base != actual.end());
                if (base != actual.end()) CHECK(hash_image(draw(page, theme_case, size, true)) == base->second.first);
            }
        }
    }
    // Zonen-Seite mit 12 Blöcken und Extremtexten: gezeichnet in allen Größen, deterministisch, nichts in den Seitenrändern
    // (Clipping pro Block, Texte werden gekürzt) und ein anderes Bild als dasselbe Manifest ohne layout.
    {
        plugins::Json j = {{"schema_version", 2}, {"id", "zonen-stress"}, {"name", "Stress"}, {"version", "1.0.0"}};
        j["layout"] = {{"columns", {1, 1}}, {"rows", {1, 1}}, {"zones", plugins::Json::array({
            {{"id", "a"}, {"column", 0}, {"row", 0}}, {{"id", "b"}, {"column", 1}, {"row", 0}},
            {{"id", "c"}, {"column", 0}, {"row", 1}}, {{"id", "d"}, {"column", 1}, {"row", 1}}})}};
        const std::string longtext(300, 'W');
        j["widgets"] = plugins::Json::array();
        const char* types[] = {"text", "metric", "bar"};
        for (int i = 0; i < 12; ++i) {
            plugins::Json w = {{"type", types[i % 3]}, {"id", "b" + std::to_string(i)}, {"zone", std::string(1, char('a' + i / 3))},
                               {"label", longtext}};
            if (i % 3 == 0) w["text"] = longtext;
            else { w["value"] = i % 3 == 2 ? "55" : longtext; if (i % 3 == 2) { w["min"] = 0; w["max"] = 100; } }
            j["widgets"].push_back(w);
        }
        auto stress = plugins::parse_manifest(j.dump());
        CHECK(stress);
        if (stress) {
            plugins::RenderContext context;
            const auto page = plugins::render_page(*stress.manifest, context);
            CHECK(page.layout && page.widgets.size() == 12);
            for (const auto& size : kSizes) {
                const Image image = draw(page, themes[0], size, true);
                CHECK(image.w == size.w && image.h == size.h);
                CHECK(hash_image(image) == hash_image(draw(page, themes[0], size, true)));
                const float s = render::scale_for(size.w, size.h);
                const int y = static_cast<int>(60 * s);  // innerhalb der Kartenfläche, links/rechts neben dem Raster
                const uint32_t bg_pixel = image.at(1, y);
                CHECK(image.at(image.w - 2, y) == bg_pixel);
                CHECK(image.at(static_cast<int>(3 * s), y) == bg_pixel);
            }
        }
    }
    // Canvas::clip_push/clip_pop: nur das Rechteck wird übermalt (x1/y1 eingeschlossen), Schachtelung = Schnittmenge,
    // ein offener Clip bei finish() ist unschädlich, und ohne Clip bleibt alles wie zuvor.
    {
        const Color paper{255, 255, 255}, ink{200, 0, 0};
        auto pixel = [](const Image& image, int x, int y) { return image.at(x, y); };
        Canvas free_canvas(40, 30, 1, paper);
        free_canvas.rectangle(0, 0, 39, 29, ink);
        const Image all = free_canvas.finish();
        CHECK(pixel(all, 0, 0) == pixel(all, 39, 29) && pixel(all, 0, 0) != 0xFFFFFFFFu);  // ohne Clip: alles übermalt
        Canvas canvas(40, 30, 1, paper);
        canvas.clip_push(10, 10, 19, 19);
        canvas.rectangle(0, 0, 39, 29, ink);
        canvas.clip_pop();
        canvas.rectangle(30, 0, 32, 2, ink);  // nach dem Pop wieder uneingeschränkt
        canvas.clip_push(0, 0, 39, 29);
        canvas.clip_push(5, 5, 7, 7);
        canvas.text(0, 6, "WWWWWWWWWWWW", {12, true}, ink, "lm");  // Text wird mitgeclippt
        canvas.clip_pop();
        canvas.clip_push(25, 25, 24, 24);  // leer/umgekehrt: nichts sichtbar
        canvas.rectangle(0, 0, 39, 29, ink);
        canvas.clip_pop();
        canvas.clip_pop();
        canvas.clip_pop();  // zu viel Pop: ohne Wirkung
        canvas.clip_push(1, 1, 2, 2);  // bleibt offen: finish() räumt auf
        const Image image = canvas.finish();
        const uint32_t white = 0xFFFFFFFFu, red = pixel(all, 0, 0);
        CHECK(pixel(image, 15, 15) == red && pixel(image, 10, 10) == red && pixel(image, 19, 19) == red);
        CHECK(pixel(image, 9, 15) == white && pixel(image, 20, 15) == white && pixel(image, 15, 9) == white && pixel(image, 15, 20) == white);
        CHECK(pixel(image, 0, 0) == white && pixel(image, 39, 29) == white && pixel(image, 25, 25) == white);
        CHECK(pixel(image, 31, 1) == red);  // nach dem Pop gezeichnet
        bool outside_clean = true;  // alles außerhalb der erlaubten Flächen (Text nur im 3x3-Clip 5..7) bleibt weiß
        for (int y = 0; y < 30; ++y) for (int x = 0; x < 40; ++x)
            if (!(x >= 30 && x <= 32 && y <= 2) && !(x >= 10 && x <= 19 && y >= 10 && y <= 19) && !(x >= 5 && x <= 7 && y >= 5 && y <= 7) &&
                pixel(image, x, y) != white) outside_clean = false;
        CHECK(outside_clean);
    }

    // Codex-Befund 3: umgekehrte Clip-Grenzen werden VOR der Rundung erkannt. Bei Skalierung 0,5 werden (25,25,24,24) zu
    // (12.5,12.5,12,12): nach pyround wäre das Rechteck nicht mehr umgekehrt und würde ein Pixel sichtbar lassen.
    {
        const Color paper{255, 255, 255}, ink{200, 0, 0};
        for (float scale : {0.5f, 1.f, 2.f}) {
            Canvas canvas(40, 30, scale, paper);
            canvas.clip_push(25, 25, 24, 24);
            canvas.rectangle(0, 0, 80, 60, ink);
            canvas.clip_pop();
            canvas.clip_push(25, 5, 24, 40);  // nur x umgekehrt
            canvas.rectangle(0, 0, 80, 60, ink);
            canvas.clip_pop();
            canvas.clip_push(5, 25, 40, 24);  // nur y umgekehrt
            canvas.rectangle(0, 0, 80, 60, ink);
            canvas.clip_pop();
            const Image image = canvas.finish();
            bool clean = true;
            for (int y = 0; y < image.h; ++y) for (int x = 0; x < image.w; ++x) if (image.at(x, y) != 0xFFFFFFFFu) clean = false;
            CHECK(clean);
        }
        Canvas half(40, 30, 0.5f, paper);  // gültiges Rechteck bei Skalierung 0,5 bleibt sichtbar
        half.clip_push(10, 10, 29, 29);
        half.rectangle(0, 0, 80, 60, ink);
        half.clip_pop();
        const Image image = half.finish();
        CHECK(image.at(7, 7) != 0xFFFFFFFFu && image.at(2, 2) == 0xFFFFFFFFu);
        // Ausnahme-Unwinding: ein Clip, der durch eine Ausnahme offen bleibt, wird von ~Canvas geschlossen (kein Absturz),
        // die RAII-Klammer des Renderers (ClipScope) arbeitet nach demselben Muster (clip_push im Konstruktor, clip_pop im Destruktor).
        struct Scope {
            Canvas& canvas;
            explicit Scope(Canvas& c) : canvas(c) { canvas.clip_push(0, 0, 9, 9); }
            ~Scope() { canvas.clip_pop(); }
        };
        Canvas guarded(40, 30, 1, paper);
        bool thrown = false;
        try {
            Scope scope(guarded);
            guarded.rectangle(0, 0, 39, 29, ink);
            throw std::runtime_error("block failed");
        } catch (const std::runtime_error&) { thrown = true; }
        CHECK(thrown);
        guarded.rectangle(20, 20, 22, 22, ink);  // nach dem Unwinding wieder uneingeschränkt
        const Image after = guarded.finish();
        CHECK(after.at(5, 5) != 0xFFFFFFFFu && after.at(21, 21) != 0xFFFFFFFFu && after.at(15, 15) == 0xFFFFFFFFu);
        bool leaked = false;
        try {
            Canvas leaking(40, 30, 1, paper);
            leaking.clip_push(0, 0, 4, 4);
            throw std::runtime_error("canvas dropped with an open clip");
        } catch (const std::runtime_error&) { leaked = true; }
        CHECK(leaked);
    }
    // Codex-Befund 2: Liste ausführlich ohne Bezeichnung: "+N" steht in der Zeile des ersten Eintrags und überlagert ihn nicht
    // (5 lange Einträge, Zone 136x82: columns {3,7}, rows {1,2}; Fläche 460x252, Abstand 7). Referenz: dieselbe Seite mit leerem
    // ersten Eintrag; im Streifen von "+N" müssen beide Bilder pixelgleich sein.
    {
        auto list_page = [](bool first_empty) {
            plugins::Page page;
            page.id = "liste";
            page.title = "Liste";
            plugins::RenderedWidget widget;
            widget.type = "list";
            widget.id = "eintraege";
            widget.zone = "z";
            for (int i = 0; i < 5; ++i) widget.items.push_back(first_empty && i == 0 ? "" : std::string(60, 'W'));
            page.widgets.push_back(widget);
            plugins::zones::Layout layout;
            layout.columns = {3, 7};
            layout.rows = {1, 2};
            layout.zones = {{"z", 0, 0, 1, 1}};
            page.layout = layout;
            return page;
        };
        const Size landscape = kSizes[0];
        const Image full = draw(list_page(false), themes[0], landscape, true), reference = draw(list_page(true), themes[0], landscape, true);
        // Zone x 10..146, Innenabstand 9: Text endet bei x = 137; "+2" (Schrift 11) belegt etwa die letzten 14 Pixel
        bool strip_equal = true, first_line_differs = false;
        for (int y = 47; y < 62; ++y) {
            for (int x = 126; x < 137; ++x) if (full.at(x, y) != reference.at(x, y)) strip_equal = false;
            for (int x = 20; x < 100; ++x) if (full.at(x, y) != reference.at(x, y)) first_line_differs = true;
        }
        CHECK(strip_equal);
        CHECK(first_line_differs);  // der erste Eintrag wird weiter links wirklich gezeichnet
        // die folgenden Zeilen sind unverändert (Zeile 2 und 3 ohne Einfluss von "+N")
        bool rows_equal = true;
        for (int y = 65; y < 112; ++y) for (int x = 10; x < 146; ++x) if (full.at(x, y) != reference.at(x, y)) rows_equal = false;
        CHECK(rows_equal);
    }
    // Codex-Befund 1: Schrift wird nicht mitskaliert; bei Skalierung 0,5 (240x160, 160x240) gelten Schwellen und Zeilenmaße
    // das Doppelte. Sechs Zeilen à 36 logische Pixel: bei 480x320 passt ein Textblock (kompakt 22), bei 240x160 nicht (44).
    {
        auto strip_page = [](size_t rows_used) {
            plugins::Page page;
            page.id = "streifen";
            page.title = "Streifen";
            plugins::zones::Layout layout;
            layout.columns = {1};
            layout.rows = std::vector<int>(rows_used, 1);
            for (size_t i = 0; i < rows_used; ++i) layout.zones.push_back({"z" + std::to_string(i), 0, static_cast<int>(i), 1, 1});
            page.layout = layout;
            plugins::RenderedWidget widget;
            widget.type = "text";
            widget.id = "t";
            widget.zone = rows_used == 1 ? "z0" : "z3";  // Zeile 4 von 6: logisch y 169..205
            widget.text = "Kurz";
            page.widgets.push_back(widget);
            return page;
        };
        const auto six = strip_page(6), one = strip_page(1);
        auto centre_pixel = [](const Image& image, float logical_y) {
            const float s = render::scale_for(image.w, image.h);
            return image.at(static_cast<int>(240 * s), static_cast<int>(logical_y * s));
        };
        auto background_pixel = [](const Image& image) { return image.at(1, image.h / 2); };
        const Size big = kSizes[0], small = kSizes[2];
        const Image big_six = draw(six, themes[0], big, true), small_six = draw(six, themes[0], small, true);
        CHECK(centre_pixel(big_six, 187) != background_pixel(big_six));      // Zone 3 (Zeile 4 von 6): Karte da
        CHECK(centre_pixel(small_six, 187) == background_pixel(small_six));  // bei 0,5: ausgeblendet statt überlappt
        const Image small_one = draw(one, themes[0], small, true);
        CHECK(centre_pixel(small_one, 160) != background_pixel(small_one));  // große Zone: auch bei 0,5 gezeichnet
        // Zeilen überlappen bei 0,5 nicht: Liste mit 5 Einträgen in einer großen Zone, zwischen den Zeilen (Zeilenabstand 36
        // logische = 18 Pixel) bleibt über den Text hinaus Kartenfläche frei: Zeile i belegt höchstens Pixel ±9 um ihre Mitte.
        plugins::Page lines;
        lines.id = "zeilen";
        lines.title = "Zeilen";
        plugins::zones::Layout single;
        single.columns = {1};
        single.rows = {1};
        single.zones = {{"z", 0, 0, 1, 1}};
        lines.layout = single;
        plugins::RenderedWidget list;
        list.type = "list";
        list.id = "l";
        list.zone = "z";
        for (int i = 0; i < 5; ++i) list.items.push_back("Eintrag");
        lines.widgets.push_back(list);
        const Image list_small = draw(lines, themes[0], small, true);
        // Auf einer Pixelzeile ohne Text (Mitte zwischen zwei Zeilen) darf höchstens die Kartenfarbe liegen.
        // Zeilen bei 0,5: top = 39 + 12 = 51 logisch, Mitte der Zeile i = 51 + 18 + 36 i → Pixel 34.5 + 18 i; Lücken bei Pixel 43.5 + 18 i
        // sind ohne Schrift (12 px hoch => ±6 um die Mitte) nur dann leer, wenn sich Zeilen nicht überlappen.
        bool gaps_clean = true;
        for (int i = 0; i < 4; ++i) {
            const int gap_y = 43 + 18 * i;
            const uint32_t card_pixel = list_small.at(6, gap_y);
            for (int x = 6; x < 114; ++x) if (list_small.at(x, gap_y) != card_pixel) gaps_clean = false;
        }
        CHECK(gaps_clean);
    }

    const fs::path hash_file = snapshots / "hashes.txt";
    if (update) {
        fs::create_directories(snapshots);
        std::ofstream out(hash_file, std::ios::binary);
        out << "# Render snapshot baseline (FNV-1a 64 of size+ARGB pixels), themes dunkel (reference, unprefixed) hell windows custom custom-cupertino custom-fluent custom-material (prefixed), sizes landscape portrait landscape-small portrait-small, clock 12:34, date 2026-01-01. Rewrite: fl-plugin-snapshot-tests update\n";
        for (const auto& [name, entry] : actual) {
            out << name << ' ' << entry.first << '\n';
            write_png(snapshots / (name + ".png"), entry.second);
        }
        CHECK(static_cast<bool>(out));
        std::printf("Baseline written: %zu snapshots in %s\n", actual.size(), snapshots.string().c_str());
    } else {
        std::map<std::string, std::string> baseline;
        std::ifstream in(hash_file, std::ios::binary);
        CHECK(static_cast<bool>(in));
        if (!in) std::printf("Baseline missing: %s (run with 'update')\n", hash_file.string().c_str());
        for (std::string line; std::getline(in, line);) {
            if (line.empty() || line[0] == '#') continue;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream fields(line);
            std::string name, hash;
            if (fields >> name >> hash) baseline[name] = hash;
        }
        const fs::path diff_dir = fs::temp_directory_path() / "fl-plugin-snapshot-actual";
        for (const auto& [name, entry] : actual) {
            auto found = baseline.find(name);
            CHECK(found != baseline.end());
            if (found == baseline.end()) { std::printf("No baseline for %s\n", name.c_str()); continue; }
            const bool same = found->second == entry.first;
            CHECK(same);
            if (!same) {
                fs::create_directories(diff_dir);
                write_png(diff_dir / (name + ".png"), entry.second);
                std::printf("Snapshot differs: %s (baseline %s, actual %s); actual image: %s\n", name.c_str(),
                            found->second.c_str(), entry.first.c_str(), (diff_dir / (name + ".png")).string().c_str());
            }
        }
        for (const auto& [name, hash] : baseline) {
            CHECK(actual.contains(name));  // stale baseline entry
            if (!actual.contains(name)) std::printf("Stale baseline entry: %s\n", name.c_str());
        }
    }
    std::printf("Plugin snapshots: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
