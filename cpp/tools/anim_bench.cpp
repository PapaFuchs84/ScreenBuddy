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
    gfx_init();
    i18n::set_lang("de");
    if (argc > 2 && std::string(argv[1]) == "transition") {
        // Writes HUD-scan/slide/fade frames System -> AI (1520x720) as PNGs into argv[2] and times them.
        auto sys = render::samples::system(); auto ai = render::samples::ai_status();
        render::SysRenderer sysR(1520, 720, "System", "bit"); render::AiRenderer aiR(1520, 720);
        const Image a = sysR.render(sys, render::samples::CLOCK), b = aiR.render(ai, 1.0, render::samples::CLOCK);
        const auto pal = render::hud_palette(config::defaults()["system"], render::theme());
        const std::pair<const char*, render::PageTransition> styles[] = {{"hud", render::PageTransition::Hud}, {"slide", render::PageTransition::Slide}, {"fade", render::PageTransition::Fade}};
        for (const auto& [name, style] : styles) {
            std::vector<double> ms;
            for (int i = 1; i < 10; ++i) {
                auto t0 = clock_type::now();
                Image f = render::page_transition(a, b, i / 10.0, style, 1, pal.primary, pal.secondary);
                ms.push_back(std::chrono::duration<double, std::milli>(clock_type::now() - t0).count());
                if (i % 2 == 1) { const auto png = encode_png(f); std::ofstream(std::string(argv[2]) + "/" + name + "-" + std::to_string(i) + ".png", std::ios::binary).write(png.data(), png.size()); }
            }
            const Stat s = stat(ms);
            std::printf("transition %-6s avg %.2f ms p95 %.2f ms (1520x720)\n", name, s.avg, s.p95);
        }
        CoUninitialize();
        return 0;
    }
    const int frames = argc > 1 ? std::max(5, std::atoi(argv[1])) : 60;
    const std::string out_path = argc > 2 ? argv[2] : "";
    render::apply_theme(config::defaults()["display"]);

    auto sys = render::samples::system();
    auto sp = render::samples::spotify();
    auto ai = render::samples::ai_status();
    const std::time_t clock = render::samples::CLOCK;
    std::vector<float> bars(16, .5f);

    struct Size { int w, h; const char* name; };
    const std::vector<Size> sizes = {{480, 320, "usb-480x320"}, {1920, 720, "bar-1920x720"}, {2534, 1574, "pad-2534x1574"}};
    std::string json = "[\n";
    bool first = true;
    std::printf("%-16s %-10s %8s %8s %8s %8s %9s %9s %7s\n", "size", "view", "avg_ms", "p50_ms", "p95_ms", "max_ms", "jpeg_avg", "jpeg_p95", "fps");
    for (const auto& size : sizes) {
        render::SysRenderer sysR(size.w, size.h, "System", "bit");
        render::SpotifyRenderer spR(size.w, size.h, "Spotify");
        render::AiRenderer aiR(size.w, size.h);
        const auto palette = render::hud_neon_palette();
        const std::vector<std::pair<const char*, std::function<Image(double)>>> views = {
            {"hud", [&](double t) { return render::hardware_hud(size.w, size.h, sys, t, 8, palette); }},
            {"ai", [&](double t) { return aiR.render(ai, t, clock); }},
            {"spotify", [&](double t) {
                 for (size_t i = 0; i < bars.size(); ++i) bars[i] = float(.15 + .8 * (.5 + .5 * std::sin(t * 5 + i * .7)));
                 return spR.render(sp, t, &bars, 1, true, clock);
             }},
            {"system", [&](double) { return sysR.render(sys, clock); }},
        };
        for (const auto& [name, draw] : views) {
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
            const Stat r = stat(render_ms), j = stat(jpeg_ms);
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
    json += "\n]\n";
    if (!out_path.empty()) { std::ofstream f(out_path, std::ios::binary); f << json; }
    CoUninitialize();
    return 0;
}
