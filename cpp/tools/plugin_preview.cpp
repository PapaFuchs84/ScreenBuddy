// © 2026 FuchsLabs – renders store preview images for plugin manifests (maintainers only; Windows, needs the app's renderer).
// Uses the same page renderer as the app with each source's sample data and a fixed clock, so results are reproducible.
// Usage: fl-plugin-preview <plugins-root> <out-dir>     reads <root>/<id>/<id>.flplugin.json, writes <out-dir>/<id>.png (480x320)
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "config.h"
#include "gfx.h"
#include "plugin_render.h"
#include "plugins.h"
#include "render.h"

namespace fs = std::filesystem;
using namespace fl;

int main(int argc, char** argv) {
    if (argc != 3) { std::puts("Usage: fl-plugin-preview <plugins-root> <out-dir>"); return 2; }
    gfx_init();
    std::vector<fs::path> files;
    for (const auto& e : fs::recursive_directory_iterator(argv[1]))
        if (e.is_regular_file() && e.path().filename().string().ends_with(".flplugin.json")) files.push_back(e.path());
    std::sort(files.begin(), files.end());
    fs::create_directories(argv[2]);
    int failed = 0;
    for (const auto& f : files) {
        std::ifstream in(f, std::ios::binary);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        auto parsed = plugins::parse_manifest(data);
        if (!parsed) { std::printf("FAIL %s: %s\n", f.string().c_str(), parsed.error.c_str()); ++failed; continue; }
        plugins::RenderContext context;
        context.sources = plugins::sample_data(*parsed.manifest);
        context.time = "12:34";
        context.date = "2026-01-01";
        auto page = plugins::render_page(*parsed.manifest, context);
        std::lock_guard lock(render::mutex());
        render::apply_theme(config::defaults()["display"]);
        Image img = plugins::draw_page(page, 480, 320, true);
        const std::string png = encode_png(img);
        std::ofstream out(fs::path(argv[2]) / (parsed.manifest->id + ".png"), std::ios::binary);
        out << png;
        std::printf("%s.png: %zu bytes\n", parsed.manifest->id.c_str(), png.size());
        if (!out) ++failed;
    }
    return failed ? 1 : 0;
}
