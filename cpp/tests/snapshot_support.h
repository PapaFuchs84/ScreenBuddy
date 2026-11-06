// Gemeinsame, unveränderte Snapshot-Hilfen (Plugin- und eingebaute Seiten).
#pragma once
#include <filesystem>
#include <fstream>
#include <cstdio>
#include "render.h"
#include "util.h"
namespace fl::snapshot {
using namespace fl;
namespace fs = std::filesystem;
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

void write_png(const fs::path& path, const Image& image) {
    std::ofstream out(path, std::ios::binary);
    out << encode_png(image);
}
}  // namespace fl::snapshot
