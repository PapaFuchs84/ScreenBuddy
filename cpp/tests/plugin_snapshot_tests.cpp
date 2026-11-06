// Render snapshots of the shipped example plugins (plugin_render.cpp draw_page).
// Fixed sample data, fixed clock/date, fixed sizes (landscape 480x320, portrait 320x480), fixed theme.
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
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "plugin_render.h"
#include "plugins.h"
#include "render.h"

namespace fs = std::filesystem;
using namespace fl;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

namespace {
struct Size { const char* name; int w, h; };
constexpr Size kSizes[] = {{"landscape", 480, 320}, {"portrait", 320, 480}};
constexpr const char* kTheme = "dunkel";  // fixed; "windows" would depend on the OS setting

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

Image draw(const plugins::Page& page, const Size& size, bool sample) {
    std::lock_guard lock(render::mutex());
    render::apply_theme({{"theme", kTheme}});
    return plugins::draw_page(page, size.w, size.h, sample);
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

    // name -> (hash, image); name = "<id>.<variant>.<orientation>"
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
            for (const auto& size : kSizes) {
                const Image first = draw(page, size, variant.sample);
                CHECK(first.w == size.w && first.h == size.h);
                const std::string hash = hash_image(first);
                CHECK(hash == hash_image(draw(page, size, variant.sample)));  // deterministic within one process
                actual[manifest.id + "." + variant.name + "." + size.name] = {hash, first};
            }
        }
    }

    const fs::path hash_file = snapshots / "hashes.txt";
    if (update) {
        fs::create_directories(snapshots);
        std::ofstream out(hash_file, std::ios::binary);
        out << "# Render snapshot baseline (FNV-1a 64 of size+ARGB pixels), theme " << kTheme
            << ", clock 12:34, date 2026-01-01. Rewrite: fl-plugin-snapshot-tests update\n";
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
