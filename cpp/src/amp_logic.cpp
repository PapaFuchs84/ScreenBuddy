// (c) 2026 FuchsLabs - siehe amp_logic.h
#include "amp_logic.h"
#include <algorithm>
#include <cmath>

namespace fl::audio {

AmpColor parse_amp_color(const std::string& s) {
    if (s == "theme") return AmpColor::Theme;
    if (s == "gradient") return AmpColor::Gradient;
    return AmpColor::Level;
}

uint32_t amp_seed(const std::string& title, const std::string& artist) {
    uint32_t h = 2166136261u;   // FNV-1a
    for (unsigned char c : title + "\x1f" + artist) { h ^= c; h *= 16777619u; }
    return h;
}

namespace {
float frac(double x) { return float(x - std::floor(x)); }
float hash01(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return float(x & 0xFFFFFF) / float(0x1000000);
}
}  // namespace

void simulate_levels(double t, uint32_t seed, float* out, int bands) {
    const double bpm = 84.0 + double(seed % 64u);
    const double beat = t * bpm / 60.0;
    const float kick = std::exp(-5.0f * frac(beat));                 // Bass-Schlag auf jeder Zählzeit
    const float snare = std::exp(-6.0f * frac(beat + 0.5)) * (std::fmod(std::floor(beat), 2.0) == 1.0 ? 1.f : 0.35f);
    const float hat = std::exp(-9.0f * frac(beat * 2.0));            // Achtel für die Höhen
    for (int b = 0; b < bands; ++b) {
        const float pos = bands > 1 ? float(b) / float(bands - 1) : 0.f;   // 0 = Bass … 1 = Höhen
        const float ph = hash01(seed + uint32_t(b) * 977u) * 6.2831853f;
        const float wob = 0.5f + 0.5f * std::sin(float(t) * (0.9f + 2.3f * hash01(seed ^ uint32_t(b * 31 + 7))) + ph);
        const float drive = (1.f - pos) * kick + (1.f - std::fabs(pos - 0.45f) * 2.f > 0 ? (1.f - std::fabs(pos - 0.45f) * 2.f) : 0.f) * snare * 0.8f + pos * hat * 0.7f;
        float v = 0.18f + 0.30f * wob + 0.50f * std::min(1.f, drive);
        v *= 1.05f - 0.35f * pos;                                      // Höhen etwas leiser, wie bei echter Musik
        out[b] = std::clamp(v, 0.f, 1.f);
    }
}

void level_rgb(float v, uint8_t rgb[3]) {
    static const float green[3] = {46, 204, 113}, yellow[3] = {241, 196, 15}, red[3] = {231, 76, 60};
    v = std::clamp(v, 0.f, 1.f);
    const float *a, *b;
    float k;
    if (v < 0.6f) { a = green; b = yellow; k = v / 0.6f; }
    else { a = yellow; b = red; k = (v - 0.6f) / 0.4f; }
    for (int i = 0; i < 3; ++i) rgb[i] = uint8_t(std::lround(a[i] + (b[i] - a[i]) * k));
}

}  // namespace fl::audio
