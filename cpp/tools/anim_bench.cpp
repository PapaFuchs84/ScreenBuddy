// © 2026 FuchsLabs
// Animation smoothness benchmark (synthetic sample data only, no devices, no accounts).
// Renders each animated view over consecutive ticks at typical output sizes and reports
// per-frame render and JPEG encode time. Usage: fl-anim-bench [frames] [out.json]
#include <windows.h>
#include <objbase.h>
#include "config.h"
#include "render.h"
#include "render_hardware_hud.h"
#include "page_samples.h"
#include "game_intro.h"
#include "render_game.h"
#include "custom_view.h"
#include "custom_view_editor.h"
#include "i18n.h"
#include "util.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

using namespace fl;
using clock_type = std::chrono::steady_clock;

struct Stat { double avg = 0, p50 = 0, p95 = 0, max = 0; };
static Stat stat(std::vector<double> v) {
    Stat s;
    if (v.empty()) return s;
    std::sort(v.begin(), v.end());
    double sum = 0; for (double x : v) sum += x;
    s.avg = sum / v.size();
    s.p50 = v[v.size() / 2];
    s.p95 = v[std::min(v.size() - 1, size_t(v.size() * 0.95))];
    s.max = v.back();
    return s;
}

int main(int argc, char** argv) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);   // less noise from background work
    gfx_init();
    i18n::set_lang(std::getenv("FL_BENCH_LANG") ? std::getenv("FL_BENCH_LANG") : "de");   // FL_BENCH_LANG=en measures the English UI
    if (argc > 2 && std::string(argv[1]) == "transition") {
        // Writes HUD-scan/slide/fade frames System -> AI as PNGs into argv[2] and times them.
        // Optional: FL_BENCH_THEME (e.g. "dunkel"), FL_BENCH_SIZE ("720x1520" for portrait).
        if (const char* theme = std::getenv("FL_BENCH_THEME")) render::apply_theme({{"theme", theme}});
        int tw = 1520, th = 720;
        if (const char* size = std::getenv("FL_BENCH_SIZE")) std::sscanf(size, "%dx%d", &tw, &th);
        auto sys = render::samples::system(); auto ai = render::samples::ai_status();
        render::SysRenderer sysR(tw, th, "System", "bit"); render::AiRenderer aiR(tw, th);
        const Image a = sysR.render(sys, render::samples::CLOCK), b = aiR.render(ai, 1.0, render::samples::CLOCK);
        const auto design = render::hud_palette(config::defaults()["system"], render::theme());
        const auto neon = render::hud_neon_palette();
        struct Variant { const char* name; render::PageTransition style; const render::HudPalette* pal; };
        const Variant styles[] = {{"hud", render::PageTransition::Hud, &design}, {"hudneon", render::PageTransition::Hud, &neon},
                                  {"slide", render::PageTransition::Slide, &design}, {"fade", render::PageTransition::Fade, &design}};
        for (const auto& [name, style, pal] : styles) {
            std::vector<double> ms;
            for (int i = 1; i < 10; ++i) {
                auto t0 = clock_type::now();
                Image f = render::page_transition(a, b, i / 10.0, style, 1, pal->primary, pal->secondary, render::transition_shade(pal->bg, pal->text));
                ms.push_back(std::chrono::duration<double, std::milli>(clock_type::now() - t0).count());
                if (i % 2 == 1) { const auto png = encode_png(f); std::ofstream(std::string(argv[2]) + "/" + name + "-" + std::to_string(i) + ".png", std::ios::binary).write(png.data(), png.size()); }
            }
            const Stat s = stat(ms);
            std::printf("transition %-6s avg %.2f ms p95 %.2f ms (%dx%d)\n", name, s.avg, s.p95, tw, th);
        }
        CoUninitialize();
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "codec") {
        // PNG encode (settings preview) and resize (logos, covers) of a full frame; best of three passes.
        render::AiRenderer aiR(1920, 720);
        const Image frame = aiR.render(render::samples::ai_status(), 1.0, render::samples::CLOCK);
        const auto best = [](auto&& fn) { double b = 1e9; for (int pass = 0; pass < 3; ++pass) { auto t0 = clock_type::now(); for (int i = 0; i < 20; ++i) fn(); b = std::min(b, std::chrono::duration<double, std::milli>(clock_type::now() - t0).count() / 20); } return b; };
        const double png = best([&] { (void)encode_png(frame); });
        const double down = best([&] { (void)resize(frame, 960, 360); });
        std::printf("codec png %.2f ms, resize %.2f ms (1920x720)\n", png, down);
        CoUninitialize();
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "game") {
        // Game-Mode cockpit (template view, 8 tiles) drawn like the app does: one source call per tile.
        const auto logo = std::make_shared<const Image>(fox_image(256, false));
        auto view = custom::game_mode_template("bench");
        SysSnapshot snap; snap.fps = 120; snap.gpu_pct = 97; snap.cpu_pct = 41; snap.frametime_ms = 8.3; snap.ping_ms = 12; snap.gpu_mem_used = 7000; snap.gpu_mem_total = 12288;
        for (int i = 0; i < 60; ++i) { snap.fps_hist.push_back(120 + 8 * std::sin(i / 4.0)); snap.frametime_hist.push_back(8.3 + std::sin(i / 5.0)); }
        for (const auto& [w, h] : std::vector<std::pair<int, int>>{{1520, 720}, {1920, 720}, {2534, 1574}}) {
            std::vector<double> ms;
            Image last;
            for (int i = 0; i < 40; ++i) {
                auto t0 = clock_type::now();
                last = custom::draw(view, "", w, h, [&](const custom::Tile& t, const custom::Layout& l, int tw, int th) -> std::optional<Image> {
                    return game::draw_metric(t, l, tw, th, snap, "Battlefield 6", 3600 + i / 30.0, logo); });
                ms.push_back(std::chrono::duration<double, std::milli>(clock_type::now() - t0).count());
            }
            const Stat s = stat(ms);
            std::printf("game %dx%d avg %.2f ms p95 %.2f ms\n", w, h, s.avg, s.p95);
            if (argc > 2) { const auto png = encode_png(last); std::ofstream(std::string(argv[2]) + "/game-" + std::to_string(w) + ".png", std::ios::binary).write(png.data(), png.size()); }
        }
        CoUninitialize();
        return 0;
    }
    if (argc > 2 && std::string(argv[1]) == "intro") {
        // Game-Mode intro: times every frame of the 2.5 s intro at 60 fps and writes key frames as PNGs.
        const auto logo = std::make_shared<const Image>(fox_image(512, false));
        const auto pal = render::hud_neon_palette();
        for (const auto& [w, h] : std::vector<std::pair<int, int>>{{1520, 720}, {2534, 1574}}) {
            auto sys = render::samples::system(); auto ai = render::samples::ai_status();
            render::SysRenderer sysR(w, h, "System", "bit"); render::AiRenderer aiR(w, h);
            const Image prev = aiR.render(ai, 1.0, render::samples::CLOCK), target = sysR.render(sys, render::samples::CLOCK);
            game::Intro intro;
            intro.begin(prev, 0, false);
            std::vector<double> ms;
            for (int i = 0; i <= 150; ++i) {
                const double t = i / 60.0;
                auto t0 = clock_type::now();
                Image f = intro.draw(target, logo, "Battlefield 6", t, game::intro_colors(pal));
                ms.push_back(std::chrono::duration<double, std::milli>(clock_type::now() - t0).count());
                if (w == 1520 && (i == 12 || i == 40 || i == 70 || i == 100 || i == 130)) {
                    const auto png = encode_png(f);
                    std::ofstream(std::string(argv[2]) + "/intro-" + std::to_string(i) + ".png", std::ios::binary).write(png.data(), png.size());
                }
            }
            const Stat s = stat(ms);
            std::printf("intro %dx%d avg %.2f ms p95 %.2f ms max %.2f ms\n", w, h, s.avg, s.p95, s.max);
        }
        CoUninitialize();
        return 0;
    }
    const int frames = argc > 1 ? std::max(5, std::atoi(argv[1])) : 60;
    const std::string out_path = argc > 2 ? argv[2] : "";
    render::apply_theme(config::defaults()["display"]);

    auto sys = render::samples::system();
    auto sp = render::samples::spotify();
    sp.cover = std::make_shared<const Image>(fox_image(640, false));   // realistic cover (scaled on every frame before R29)
    auto friends = render::samples::friends();
    for (size_t i = 0; i < friends.size(); ++i) friends[i].avatar = std::make_shared<const Image>(fox_image(128 + int(i), i % 2 == 0));   // Discord avatars are 128 px
    auto ai = render::samples::ai_status();
    const std::time_t clock = render::samples::CLOCK;
    std::vector<float> bars(16, .5f);

    struct Size { int w, h; const char* name; };
    const std::vector<Size> sizes = {{480, 320, "usb-480x320"}, {1920, 720, "bar-1920x720"}, {2534, 1574, "pad-2534x1574"}};
    std::string json = "[\n";
    bool first = true;
    double total_render = 0, total_jpeg = 0;
    std::printf("%-16s %-10s %8s %8s %8s %8s %9s %9s %7s\n", "size", "view", "avg_ms", "p50_ms", "p95_ms", "max_ms", "jpeg_avg", "jpeg_p95", "fps");
    for (const auto& size : sizes) {
        render::SysRenderer sysR(size.w, size.h, "System", "bit");
        render::SpotifyRenderer spR(size.w, size.h, "Spotify");
        render::AiRenderer aiR(size.w, size.h);
        render::Renderer frR(size.w, size.h, "Discord-Freunde");
        const auto palette = render::hud_neon_palette();
        const std::vector<std::pair<const char*, std::function<Image(double)>>> views = {
            {"hud", [&](double t) { return render::hardware_hud(size.w, size.h, sys, t, 8, palette); }},
            {"ai", [&](double t) { return aiR.render(ai, t, clock); }},
            {"spotify", [&](double t) {
                 for (size_t i = 0; i < bars.size(); ++i) bars[i] = float(.15 + .8 * (.5 + .5 * std::sin(t * 5 + i * .7)));
                 return spR.render(sp, t, &bars, 1, true, clock);
             }},
            {"system", [&](double) { return sysR.render(sys, clock); }},
            {"friends", [&](double) { return frR.render_friends(friends, 0, true, "", clock); }},
        };
        for (const auto& [name, draw] : views) {
            // Three passes; the pass with the lowest render average counts (background load on a
            // desktop machine only ever adds time, so the minimum is the most repeatable figure).
            Stat r, j;
            for (int pass = 0; pass < 3; ++pass) {
                std::vector<double> render_ms, jpeg_ms;
                (void)draw(0.0);  // warm-up: fonts, caches
                for (int i = 0; i < frames; ++i) {
                    const double tick = 1.0 + i / 30.0;
                    auto t0 = clock_type::now();
                    Image img = draw(tick);
                    auto t1 = clock_type::now();
                    std::string jpg = encode_jpeg(img, 0.8f);
                    auto t2 = clock_type::now();
                    render_ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
                    jpeg_ms.push_back(std::chrono::duration<double, std::milli>(t2 - t1).count());
                }
                const Stat rp = stat(render_ms), jp = stat(jpeg_ms);
                if (pass == 0 || rp.avg < r.avg) { r = rp; j = jp; }   // both figures from the same pass
            }
            total_render += r.avg; total_jpeg += j.avg;
            const double fps = 1000.0 / std::max(r.avg, 0.001);
            std::printf("%-16s %-10s %8.2f %8.2f %8.2f %8.2f %9.2f %9.2f %7.1f\n", size.name, name, r.avg, r.p50, r.p95, r.max, j.avg, j.p95, fps);
            char line[512];
            std::snprintf(line, sizeof line,
                "%s  {\"size\":\"%s\",\"view\":\"%s\",\"render_avg\":%.3f,\"render_p50\":%.3f,\"render_p95\":%.3f,\"render_max\":%.3f,\"jpeg_avg\":%.3f,\"jpeg_p95\":%.3f}",
                first ? "" : ",\n", size.name, name, r.avg, r.p50, r.p95, r.max, j.avg, j.p95);
            json += line;
            first = false;
        }
    }
    std::printf("TOTAL render %.2f ms, jpeg %.2f ms (sum of all cases)\n", total_render, total_jpeg);
    json += "\n]\n";
    if (!out_path.empty()) { std::ofstream f(out_path, std::ios::binary); f << json; }
    CoUninitialize();
    return 0;
}
