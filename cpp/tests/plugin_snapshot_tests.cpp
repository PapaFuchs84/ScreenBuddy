// Render snapshots of the shipped example plugins (plugin_render.cpp draw_page).
// Fixed sample data, fixed clock/date, fixed sizes (landscape 480x320, portrait 320x480, small 240x160 and 160x240), fixed theme matrix.
// Offline: no network, no port 8765, no audio, no settings store. Local Direct2D/DirectWrite rendering only.
// Usage: fl-plugin-snapshot-tests [update] [examples-dir] [snapshot-dir]
//   default: compare against the baseline in snapshot-dir (hashes.txt, PNGs are for human review)
//   update : rewrite the baseline (hashes.txt + PNGs)
#include <algorithm>
#include <process.h>
#include <chrono>
#include "output.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "plugin_render.h"
#include "plugin_render_testing.h"
#include "plugins.h"
#include "render.h"
#include "theme.h"
#include "util.h"
#include "snapshot_support.h"
#include "app.h"
using namespace fl::snapshot;

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
    { // P7b-0: Kontrast additiv, ohne die bestehenden Snapshot-Schlüssel zu ändern.
        std::lock_guard lock(render::mutex());
        render::apply_theme({{"theme",theme::DEFAULT_ID}});
        check_contrast(contrast_input(theme::DEFAULT_ID));
        CHECK(render::theme().style==theme::Style::Material);
        CHECK(theme::contrast(render::theme().accent,render::theme().card)>=4.5);
    }
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
        // P6-5: Editor -> Overlay-Prüfung -> echte App-Vorschau, alle Beispiel-v2-Plugins.
        // Keine zweite Zeichenimplementierung, keine Änderung der veröffentlichten Hashdatei.
        if(manifest.schema_version==2) {
            auto arranged=plugins::merge_arrangement(manifest,plugins::arrangement_from(manifest));
            CHECK(arranged);
            if(arranged) for(const auto& tc:themes) {
                auto cfg=config::defaults();
                for(auto section:{"companion","system","discord","friends","bf6","rss","streaming","homeassistant","spotify"}) cfg[section]["enabled"]=false;
                cfg["ui"]["update_check"]=false; cfg["ui"]["port"]=18865;
                cfg["display"]["theme"]=tc.name;
                if(!tc.inline_theme.is_null()) cfg["display"]["theme_inline"]=tc.inline_theme;
                const auto temp=fs::temp_directory_path()/("fl-editor-preview-"+std::to_string(_getpid()));
                struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path,ec); CHECK(!ec); } } cleanup{temp};
                App app(cfg,temp/"test.toml",nullptr,temp/"preview");
                plugins::RenderContext c; c.time="12:34"; c.date="2026-01-01"; c.sources=plugins::sample_data(manifest);
                auto page=plugins::render_page(manifest,c);
                for(const auto& size:kSizes) {
                    std::string error;
                    auto image=app.plugin_preview(arranged.manifest->definition.dump(),error,{size.w,size.h},true);
                    CHECK(image && error.empty());
                    if(image) CHECK(hash_image(*image)==hash_image(draw(page,tc,size,true)));
                }
            }
        }
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

    // P4b: Einträge, leer, Überblendung und Band-Vorhang; alle Themes und Orientierungen.
    {
        std::ifstream f(examples / "offline-wechselfeld.flplugin.json");
        auto parsed=plugins::parse_manifest(std::string((std::istreambuf_iterator<char>(f)),{})); CHECK(parsed);
        if (parsed) for (const auto& tc:themes) for (const auto& size:kSizes) {
            plugins::RenderContext ctx;auto page=plugins::render_page(*parsed.manifest,ctx);
            std::lock_guard lk(render::mutex());
            ojson dp={{"theme",tc.name}};if(!tc.inline_theme.is_null())dp["theme_inline"]=tc.inline_theme;
            render::apply_theme(dp);
            using namespace plugins::schedule;
            std::vector<ZoneChoice> choices{{"main","eins",{},Shape::cut,1}};
            std::map<std::string,std::shared_ptr<plugins::FadeCache>> caches;
            auto a=plugins::draw_choices(page,size.w,size.h,choices,caches);
            choices[0].block="zwei";auto b=plugins::draw_choices(page,size.w,size.h,choices,caches);
            CHECK(hash_image(a)!=hash_image(b));
            actual[tc.name+".wechsel.eins."+size.name]={hash_image(a),a};
            actual[tc.name+".wechsel.zwei."+size.name]={hash_image(b),b};
            choices[0].from_block="eins";choices[0].shape=Shape::fade;choices[0].mix=.5;
            auto mixed=plugins::draw_choices(page,size.w,size.h,choices,caches);
            CHECK(hash_image(mixed)!=hash_image(a) && hash_image(mixed)!=hash_image(b));
            actual[tc.name+".wechsel.fade."+size.name]={hash_image(mixed),mixed};
            auto start=std::chrono::steady_clock::now();
            for(int i=0;i<10;++i) plugins::draw_choices(page,size.w,size.h,choices,caches);
            double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/10;
            std::printf("cache %dx%d %.3f ms\n",size.w,size.h,ms);
            choices[0].shape=Shape::reveal;
            auto reveal=plugins::draw_choices(page,size.w,size.h,choices,caches);
            actual[tc.name+".wechsel.reveal."+size.name]={hash_image(reveal),reveal};
            choices[0].from_block.reset();choices[0].block.reset();
            auto empty=plugins::draw_choices(page,size.w,size.h,choices,caches);
            actual[tc.name+".wechsel.empty."+size.name]={hash_image(empty),empty};
        }
        if (parsed) for (auto size : {Size{"pc",480,320},Size{"wide",1920,720},Size{"large",2400,1080}}) {
            auto page=plugins::render_page(*parsed.manifest,{});
            std::lock_guard lk(render::mutex());render::apply_theme({{"theme","dunkel"}});
            using namespace plugins::schedule;
            std::vector<ZoneChoice> choices{{"main","zwei","eins",Shape::fade,.5}};
            std::map<std::string,std::shared_ptr<plugins::FadeCache>> cache;
            auto begin=std::chrono::steady_clock::now();
            plugins::draw_page(page,size.w,size.h);
            double full=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
            plugins::draw_choices(page,size.w,size.h,choices,cache);
            begin=std::chrono::steady_clock::now();
            for(int i=0;i<10;++i)plugins::draw_choices(page,size.w,size.h,choices,cache);
            double step=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/10;
            std::printf("budget %dx%d full %.3f ms cache %.3f ms\n",size.w,size.h,full,step);
        }
        Image a(16,32),b(16,32,{255,255,255}),prev=a;
        size_t bytes=0;
        for(int k=1;k<=4;++k) {
            Image next=b;plugins::reveal_rect(next,a,b,{0,0,16,32},k/4.f);
            for(auto [y,end]:changed_bands(prev,next))bytes+=size_t(end-y)*16*2;
            prev=next;
        }
        CHECK(bytes==32*16*2);
        CHECK(bytes==plugins::schedule::serial_bytes(a,b));
        // Teilbänder oben/unten dürfen bei versetzter Kachel nur einmal kosten.
        for(int offset : {1,5,9}) {
            Image old(16,48), target=old;
            for(int y=offset;y<offset+31;++y) for(int x=0;x<16;++x) target.px[size_t(y)*16+x]=0xffffffff;
            auto previous=old;size_t cost=0;
            for(int k=1;k<=4;++k) {
                auto next=old;plugins::reveal_rect(next,old,target,{0,float(offset),16,31},k/4.f);
                cost+=plugins::schedule::serial_bytes(previous,next);previous=next;
            }
            CHECK(cost==plugins::schedule::serial_bytes(old,target));
        }
        size_t fade_bytes=0;prev=a;
        for(int k=1;k<=4;++k) {
            Image next=b;plugins::blend_rect(next,a,b,{0,0,16,32},k/4.f);
            fade_bytes+=plugins::schedule::serial_bytes(prev,next);prev=next;
        }
        CHECK(fade_bytes==4*bytes);
        Image dst=b;plugins::blend_rect(dst,a,b,{4,4,8,8},.5);
        CHECK(dst.at(0,0)==b.at(0,0) && dst.at(4,4)==0xff7f7f7f);
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

    // ---------------------------------------------------------------- Phase 3: bedingte Anzeige und Fehlerisolierung
    // Vorschau ohne Daten mit Bedingungen: dieselbe v2-Seite mit Beispieldaten (Vorschau), ohne Antwort (live, "missing") und
    // mit Antwort ohne passende Werte ("partial"). Neue Snapshot-Einträge "bedingt-quelle.*" (Referenz-Theme dunkel).
    {
        plugins::Json j = {{"schema_version", 2}, {"id", "bedingt-quelle"}, {"name", "Bedingt mit Quelle"}, {"version", "1.0.0"}};
        j["sources"] = plugins::Json::array({{{"id", "feed"}, {"url", "https://api.example.com/status"},
            {"sample", {{"temp", 21.5}, {"alerts", plugins::Json::array({{{"t", "Sturm"}}, {{"t", "Glatteis"}}})}, {"online", true}, {"load", 72}}}}});
        j["layout"] = {{"columns", {1, 1}}, {"rows", {1, 1}}, {"zones", plugins::Json::array({
            {{"id", "a"}, {"column", 0}, {"row", 0}}, {{"id", "b"}, {"column", 1}, {"row", 0}},
            {{"id", "c"}, {"column", 0}, {"row", 1}}, {{"id", "d"}, {"column", 1}, {"row", 1}}})}};
        j["widgets"] = plugins::Json::array({
            {{"type", "metric"}, {"id", "temp"}, {"zone", "a"}, {"label", "Temperatur"}, {"value", "{{feed.temp}}"}, {"unit", "°C"},
             {"show_if", {{"path", "feed.temp"}}}},
            {{"type", "list"}, {"id", "alerts"}, {"zone", "b"}, {"label", "Warnungen"}, {"items", "feed.alerts"}, {"item_template", "{{item.t}}"},
             {"max_items", 3}, {"show_if", {{"path", "feed.alerts"}}}},
            {{"type", "text"}, {"id", "online"}, {"zone", "c"}, {"label", "Status"}, {"text", "Online"},
             {"show_if", {{"path", "feed.online"}, {"op", "=="}, {"value", true}}}},
            {{"type", "bar"}, {"id", "load"}, {"zone", "c"}, {"label", "Last"}, {"value", "{{feed.load}}"}, {"min", 0}, {"max", 100},
             {"show_if", {{"path", "feed.load"}, {"op", ">="}, {"value", 50}}}},
            {{"type", "clock"}, {"id", "uhr"}, {"zone", "d"}, {"label", "Uhr"}, {"format", "time"}}});
        auto parsed = plugins::parse_manifest(j.dump());
        CHECK(parsed);
        if (parsed) {
            const auto& manifest = *parsed.manifest;
            plugins::RenderContext context;
            context.time = "12:34";
            context.date = "2026-01-01";
            context.sources = plugins::sample_data(manifest);
            const auto sample = plugins::render_page(manifest, context);
            CHECK(sample.widgets.size() == 5 && sample.hidden.empty() && !sample.missing_data);
            context.sources.clear();
            const auto missing = plugins::render_page(manifest, context);
            CHECK(missing.widgets.size() == 1 && missing.hidden.size() == 4 && missing.missing_data);
            context.sources = {{"feed", {{"temp", nullptr}, {"alerts", plugins::Json::array()}, {"online", false}, {"load", 10}}}};
            const auto partial = plugins::render_page(manifest, context);
            CHECK(partial.widgets.size() == 1 && partial.hidden.size() == 4 && !partial.missing_data);
            const std::pair<const char*, const plugins::Page*> variants[] = {{"sample", &sample}, {"missing", &missing}, {"partial", &partial}};
            for (const auto& size : kSizes) {
                std::map<std::string, std::string> hashes;
                for (const auto& [name, page] : variants) {
                    const bool preview = std::string(name) == "sample";
                    const Image image = draw(*page, themes[0], size, preview);
                    CHECK(image.w == size.w && image.h == size.h);
                    hashes[name] = hash_image(image);
                    CHECK(hashes[name] == hash_image(draw(*page, themes[0], size, preview)));
                    actual[std::string("bedingt-quelle.") + name + "." + size.name] = {hashes[name], image};
                }
                CHECK(hashes["sample"] != hashes["missing"] && hashes["missing"] != hashes["partial"]);  // Fußtext unterscheidet missing/partial
            }
        }
    }

    // Fehlerinjektion pro Block (Test-Hook aus plugin_render_testing.h; die App zeichnet immer ohne Hook).
    auto draw_hooked = [](const plugins::Page& page, const ThemeCase& theme_case, const Size& size, bool sample,
                          const plugins::testing::DrawHooks& hooks) {
        std::lock_guard lock(render::mutex());
        if (theme_case.inline_theme.is_null()) render::apply_theme({{"theme", theme_case.name}});
        else render::apply_theme({{"theme_inline", theme_case.inline_theme}});
        return plugins::testing::draw_page(page, size.w, size.h, sample, hooks);
    };
    auto load_page = [&](const char* file) -> std::optional<plugins::Page> {
        std::ifstream input(examples / file, std::ios::binary);
        auto parsed = plugins::parse_manifest(std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()));
        if (!parsed) return std::nullopt;
        plugins::RenderContext context;
        context.time = "12:34";
        context.date = "2026-01-01";
        context.sources = plugins::sample_data(*parsed.manifest);
        return plugins::render_page(*parsed.manifest, context);
    };
    // Prüft: außerhalb von `rect` (±1 Pixel Kante) pixelgleich zur Referenz, innerhalb (−1 Pixel) nur Seitenhintergrund,
    // und in der Referenz war dort wirklich etwas gezeichnet.
    struct Box { int x0, y0, x1, y1; };
    auto only_erased = [](const Image& reference, const Image& image, Box box, uint32_t bg) {
        bool outside_same = true, inside_bg = true, drawn_before = false;
        for (int y = 0; y < image.h; ++y) for (int x = 0; x < image.w; ++x) {
            const bool near = x >= box.x0 - 1 && x <= box.x1 + 1 && y >= box.y0 - 1 && y <= box.y1 + 1;
            const bool inner = x >= box.x0 + 1 && x <= box.x1 - 1 && y >= box.y0 + 1 && y <= box.y1 - 1;
            if (!near && image.at(x, y) != reference.at(x, y)) outside_same = false;
            if (inner && image.at(x, y) != bg) inside_bg = false;
            if (inner && reference.at(x, y) != bg) drawn_before = true;
        }
        return outside_same && inside_bg && drawn_before;
    };
    auto box_of = [](const plugins::zones::Rect& r) {
        return Box{static_cast<int>(std::floor(r.x)), static_cast<int>(std::floor(r.y)), static_cast<int>(std::floor(r.x + r.width - 1)),
                   static_cast<int>(std::floor(r.y + r.height - 1))};
    };
    float margin = 10, gap = 7;  // Rahmenstil des Referenz-Themes (wie draw_page)
    {
        std::lock_guard lock(render::mutex());
        render::apply_theme({{"theme", themes[0].name}});
        const float spacing = theme::design(render::theme().style).spacing / 12.f;
        margin = 10 * spacing;
        gap = 7 * spacing;
    }
    const Size landscape = kSizes[0];  // 480x320: Skalierung 1, logische Koordinaten = Pixel
    if (auto zoned = load_page("offline-zonen.flplugin.json")) {
        const Image reference = draw(*zoned, themes[0], landscape, true);
        const uint32_t bg = reference.at(1, 100);
        const plugins::zones::DensityTable table;
        std::vector<plugins::zones::BlockInput> inputs;
        for (const auto& w : zoned->widgets) inputs.push_back(plugins::zones::describe(w.id, w.type, table, w.zone));
        const auto plan = plugins::zones::plan({margin, 39, 480 - 2 * margin, 320 - 39 - 19 - margin}, *zoned->layout, inputs, gap);
        auto area = [&](const std::string& id) {
            for (const auto& b : plan.blocks) if (b.id == id) return b.area;
            return plugins::zones::Rect{};
        };
        for (const char* target : {"uhr", "motto", "dauer", "pausen"}) {  // erster, gestapelter, einzelner, letzter Block
            plugins::testing::DrawHooks hooks;
            hooks.after_card = [&](Canvas&, const std::string& key) { if (key == target) throw std::runtime_error("GEHEIM-123"); };
            const Image image = draw_hooked(*zoned, themes[0], landscape, true, hooks);
            CHECK(only_erased(reference, image, box_of(area(target)), bg));
        }
        // Ein Block, der eigene Clips offen lässt und dann wirft: der Clip-Stapel wird vor dem Löschen und danach exakt
        // wiederhergestellt (gleiches Bild wie ohne die offenen Clips); ohne Ausnahme bleiben offene Clips eines Blocks
        // ebenfalls folgenlos für alle anderen Blöcke und den Fußtext.
        plugins::testing::DrawHooks thrower;
        thrower.after_card = [](Canvas&, const std::string& key) { if (key == "dauer") throw 7; };
        plugins::testing::DrawHooks leaker;
        leaker.after_card = [](Canvas& canvas, const std::string& key) {
            if (key != "dauer") return;
            for (int i = 0; i < 3; ++i) canvas.clip_push(0, 0, 1, 1);
            throw std::runtime_error("leak");
        };
        CHECK(hash_image(draw_hooked(*zoned, themes[0], landscape, true, leaker)) == hash_image(draw_hooked(*zoned, themes[0], landscape, true, thrower)));
        plugins::testing::DrawHooks quiet_leak;
        quiet_leak.after_card = [](Canvas& canvas, const std::string& key) { if (key == "uhr") canvas.clip_push(0, 0, 1, 1); };
        const Image leaked = draw_hooked(*zoned, themes[0], landscape, true, quiet_leak);
        const Box uhr = box_of(area("uhr"));
        bool rest_same = true;
        for (int y = 0; y < leaked.h; ++y) for (int x = 0; x < leaked.w; ++x)
            if (!(x >= uhr.x0 - 1 && x <= uhr.x1 + 1 && y >= uhr.y0 - 1 && y <= uhr.y1 + 1) && leaked.at(x, y) != reference.at(x, y)) rest_same = false;
        CHECK(rest_same);
        // Alle Blöcke werfen: nur Kopf und Fuß bleiben (Fußzeile unverändert lesbar).
        plugins::testing::DrawHooks all;
        all.after_card = [](Canvas&, const std::string&) { throw std::bad_alloc(); };
        const Image empty = draw_hooked(*zoned, themes[0], landscape, true, all);
        plugins::Page none = *zoned;
        none.widgets.clear();
        CHECK(hash_image(empty) == hash_image(draw(none, themes[0], landscape, true)));
        // Ohne Hook-Auslösung: pixelgleich zur App-Zeichnung.
        plugins::testing::DrawHooks idle;
        idle.after_card = [](Canvas&, const std::string&) {};
        CHECK(hash_image(draw_hooked(*zoned, themes[0], landscape, true, idle)) == hash_image(reference));
    } else CHECK(false);
    // v1-Raster (offline-focus, 4 Karten): eine werfende Karte wird gelöscht, die anderen bleiben pixelgleich.
    if (auto grid = load_page("offline-focus.flplugin.json")) {
        const Image reference = draw(*grid, themes[0], landscape, true);
        const uint32_t bg = reference.at(1, 100);
        const float card_w = (480 - 2 * margin - gap) / 2, card_h = (320 - 39 - 19 - margin - gap) / 2;
        for (size_t index = 0; index < grid->widgets.size() && index < 4; ++index) {
            const std::string key = "#" + std::to_string(grid->widgets[index].index);
            plugins::testing::DrawHooks hooks;
            hooks.after_card = [&](Canvas&, const std::string& k) { if (k == key) throw std::runtime_error("GEHEIM-123"); };
            const Image image = draw_hooked(*grid, themes[0], landscape, true, hooks);
            const float x = margin + static_cast<float>(index % 2) * (card_w + gap), y = 39 + static_cast<float>(index / 2) * (card_h + gap);
            CHECK(only_erased(reference, image, box_of({x, y, card_w + 1, card_h + 1}), bg));
        }
    } else CHECK(false);
    // Codex-Review Phase 3: Wurf nach dem Text und mehrere Skalierungen (0,5 / 1 / 2). Der Clip liegt auch im v1-Raster auf der
    // Löschfläche (Karte plus 1 logisches Pixel), deshalb kann außerhalb nichts übrig bleiben, auch wenn Text bei kleiner Skalierung
    // über die Karte hinausreichen würde. Erwartete Fläche in echten Pixeln: logisches Rechteck mal Skalierung.
    {
        const Size scales[] = {kSizes[2], kSizes[0], {"double", 960, 640}};  // 240x160 (0,5), 480x320 (1), 960x640 (2)
        auto pixel_box = [](const plugins::zones::Rect& r, float s) {
            return Box{static_cast<int>(std::floor(r.x * s)), static_cast<int>(std::floor(r.y * s)),
                       static_cast<int>(std::floor((r.x + r.width - 1) * s)), static_cast<int>(std::floor((r.y + r.height - 1) * s))};
        };
        auto grid = load_page("offline-focus.flplugin.json");
        auto zoned = load_page("offline-zonen.flplugin.json");
        CHECK(grid && zoned);
        for (const auto& size : scales) {
            const float s = render::scale_for(size.w, size.h);
            if (grid) {
                const Image reference = draw(*grid, themes[0], size, true);
                const uint32_t bg = reference.at(1, reference.h / 2);
                const float card_w = (480 - 2 * margin - gap) / 2, card_h = (320 - 39 - 19 - margin - gap) / 2;
                for (size_t index = 0; index < grid->widgets.size() && index < 4; ++index) {
                    const std::string key = "#" + std::to_string(grid->widgets[index].index);
                    for (int when = 0; when < 2; ++when) {  // 0: nach der Karte, 1: nach dem gesamten Inhalt
                        plugins::testing::DrawHooks hooks;
                        auto fail = [key](Canvas&, const std::string& k) { if (k == key) throw std::runtime_error("GEHEIM-123"); };
                        if (when == 0) hooks.after_card = fail; else hooks.after_content = fail;
                        const Image image = draw_hooked(*grid, themes[0], size, true, hooks);
                        const float x = margin + static_cast<float>(index % 2) * (card_w + gap), y = 39 + static_cast<float>(index / 2) * (card_h + gap);
                        const bool ok = only_erased(reference, image, pixel_box({x - 1, y - 1, card_w + 3, card_h + 3}, s), bg);
                        CHECK(ok);
                        if (!ok) std::printf("grid erase failed: %s card %zu hook %d\n", size.name, index, when);
                    }
                }
            }
            if (zoned) {
                const Image reference = draw(*zoned, themes[0], size, true);
                const uint32_t bg = reference.at(1, reference.h / 2);
                const float k = std::max(1.f, 1.f / s);  // wie draw_page: Schwellen bei Skalierung < 1 mal 1/s
                const plugins::zones::DensityTable table;
                std::vector<plugins::zones::BlockInput> inputs;
                for (const auto& w : zoned->widgets) {
                    auto input = plugins::zones::describe(w.id, w.type, table, w.zone);
                    for (auto& m : input.min.min) m = {m.width * k, m.height * k};
                    inputs.push_back(std::move(input));
                }
                const auto plan = plugins::zones::plan({margin, 39, 480 - 2 * margin, 320 - 39 - 19 - margin}, *zoned->layout, inputs, gap);
                for (const auto& placed : plan.blocks) {
                    if (!placed.visible()) continue;
                    plugins::testing::DrawHooks hooks;
                    const std::string target = placed.id;
                    hooks.after_content = [target](Canvas&, const std::string& key) { if (key == target) throw std::runtime_error("GEHEIM-123"); };
                    const Image image = draw_hooked(*zoned, themes[0], size, true, hooks);
                    const bool ok = only_erased(reference, image, pixel_box(placed.area, s), bg);
                    CHECK(ok);
                    if (!ok) std::printf("zone erase failed: %s block %s\n", size.name, target.c_str());
                }
            }
        }
    }
    // Log: fester Text ohne Daten/Ausnahmetext, höchstens ein Eintrag je Plugin, Block und Stufe pro Minute.
    if (auto probe = load_page("offline-zonen.flplugin.json")) {
        probe->id = "log-probe";
        probe->failed = {"kaputt"};
        const fs::path log_file = fs::temp_directory_path() / "fl-plugin-isolation-test.log";
        std::error_code ec;
        fs::remove(log_file, ec);
        log_init(log_file, false);
        plugins::testing::DrawHooks hooks;
        hooks.after_card = [](Canvas&, const std::string& key) { if (key == "dauer") throw std::runtime_error("GEHEIM-123"); };
        for (int i = 0; i < 3; ++i) draw_hooked(*probe, themes[0], landscape, true, hooks);
        log_init({}, false);
        std::ifstream in(log_file, std::ios::binary);
        const std::string log((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        fs::remove(log_file, ec);
        auto count = [&](const std::string& needle) {
            size_t n = 0;
            for (size_t at = log.find(needle); at != std::string::npos; at = log.find(needle, at + 1)) ++n;
            return n;
        };
        CHECK(count("Plugin log-probe: Block dauer ausgeblendet (Fehler beim Zeichnen)") == 1);
        CHECK(count("Plugin log-probe: Block kaputt ausgeblendet (Fehler beim Auswerten der Daten)") == 1);
        CHECK(count("\n") == 2);
        CHECK(log.find("GEHEIM") == std::string::npos && log.find("Ruhig") == std::string::npos && log.find("Fokus") == std::string::npos);
    } else CHECK(false);

    CHECK(plugins::list_rows({10,20,80,10},plugins::zones::Density::regular,1,true,3).empty());
    CHECK(plugins::list_rows({10,20,80,10},plugins::zones::Density::detailed,1,false,3).empty());
    // Ziele und Pixel stammen aus demselben Zeichenlauf; alte Hashes werden nie neu geschrieben.
    for (bool zoned:{false,true}) for (auto size:kSizes) {
        plugins::Page p; p.id="tap-test";p.title="Tap";
        plugins::zones::Layout layout;layout.columns={1};layout.rows={1};
        plugins::zones::Zone zone;zone.id="zone";layout.zones={zone};if(zoned) p.layout=layout;
        plugins::RenderedWidget w;w.id="card";w.zone="zone";w.type="text";w.text="Tap";
        w.tap_action="open_link";w.tap_url="https://news.example.org/rendered";p.widgets={w};
        auto out=plugins::draw_page_out(p,size.w,size.h);
        CHECK(out.image.px==plugins::draw_page(p,size.w,size.h).px);CHECK(out.targets.size()==1);
        if(!out.targets.empty()) {
            auto r=out.targets[0].rect_px;
            CHECK(r.x>=0 && r.y>=int(std::floor(39*render::scale_for(size.w,size.h))) && r.x+r.w<=size.w && r.y+r.h<=size.h);
            plugins::testing::DrawHooks hooks;
            hooks.after_card=[](Canvas& c,const std::string&) { c.rectangle(0,0,10000,10000,{255,0,255}); throw 1; };
            // Der Fehlerpfad löscht genau die geclippte Kartenfläche. Jeder geänderte Pixel gehört zum Ziel.
            auto erased=plugins::testing::draw_page(p,size.w,size.h,false,hooks);
            bool contained=true;
            for(int y=0;y<size.h;++y) for(int x=0;x<size.w;++x)
                if(out.image.px[size_t(y)*size.w+x]!=erased.px[size_t(y)*size.w+x])
                    contained=contained && x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h;
            // Grid fault cleanup includes the card antialiasing margin; it is intentionally wider than its tap target.
            if(zoned) CHECK(contained);
        }
        w.type="list";w.items={"One","Two","Three"};w.item_urls={"https://news.example.org/1","https://news.example.org/2","https://news.example.org/3"};p.widgets={w};
        out=plugins::draw_page_out(p,size.w,size.h);
        CHECK(out.image.px==plugins::draw_page(p,size.w,size.h).px);
        CHECK(!out.targets.empty());
        for(size_t i=0;i<out.targets.size();++i) {
            CHECK(out.targets[i].url==w.item_urls[i]);
            if(i) CHECK(out.targets[i].rect_px.y>=out.targets[i-1].rect_px.y+out.targets[i-1].rect_px.h-1);
        }
    }

    {
        plugins::Page page;page.id="tap-transition";page.title="Transition";
        plugins::zones::Layout layout;layout.columns={1};layout.rows={1};
        plugins::zones::Zone zone;zone.id="zone";zone.cycle=plugins::zones::Cycle{"rotate","fade"};layout.zones={zone};page.layout=layout;
        plugins::RenderedWidget a;a.id="one";a.zone="zone";a.type="text";a.text="One";
        a.tap_action="open_link";a.tap_url="https://news.example.org/one";
        auto b=a;b.id="two";b.text="Two";b.tap_action.clear();b.tap_url.clear();page.widgets={a,b};
        std::map<std::string,std::shared_ptr<plugins::FadeCache>> cache;
        plugins::schedule::ZoneChoice c;c.zone="zone";c.block="two";c.from_block="one";c.shape=plugins::schedule::Shape::fade;c.mix=.5;
        auto out=plugins::draw_choices_out(page,480,320,{c},cache);
        CHECK(out.targets.size()==1 && out.targets[0].transitioning);
        CHECK(out.targets[0].url==a.tap_url);
        page.widgets[0].tap_url="https://news.example.org/not-published";
        auto cached=plugins::draw_choices_out(page,480,320,{c},cache);
        CHECK(cached.image.px==out.image.px && cached.targets[0].url==a.tap_url);
        c.from_block.reset();c.mix=1;
        CHECK(plugins::draw_choices_out(page,480,320,{c},cache).targets.empty());
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
