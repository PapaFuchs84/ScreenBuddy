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
