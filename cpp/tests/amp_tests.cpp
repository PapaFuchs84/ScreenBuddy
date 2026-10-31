// (c) 2026 FuchsLabs - Tests für die Amplifier-Logik (Simulation, Farben); unter Linux: tests/build-linux.sh
#include "amp_logic.h"
#include <cmath>
#include <cstdio>
using namespace fl::audio;
static int passed = 0, failed = 0;
#define CHECK(c) do { if (c) ++passed; else { ++failed; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

int main() {
    CHECK(parse_amp_color("theme") == AmpColor::Theme);
    CHECK(parse_amp_color("gradient") == AmpColor::Gradient);
    CHECK(parse_amp_color("level") == AmpColor::Level);
    CHECK(parse_amp_color("quatsch") == AmpColor::Level);
    CHECK(amp_seed("A", "B") == amp_seed("A", "B"));
    CHECK(amp_seed("A", "B") != amp_seed("B", "A"));
    // Simulation: Bereich 0..1, deterministisch, bewegt sich, nicht flach
    const uint32_t seed = amp_seed("Titel", "Künstler");
    float a[16], b[16];
    bool all_in = true, moves = false, varies = false;
    for (double t = 0; t < 30; t += 0.07) {
        simulate_levels(t, seed, a, 16);
        simulate_levels(t, seed, b, 16);
        float lo = 1, hi = 0;
        for (int i = 0; i < 16; ++i) {
            if (!(a[i] >= 0 && a[i] <= 1) || a[i] != b[i]) all_in = false;
            lo = std::fmin(lo, a[i]); hi = std::fmax(hi, a[i]);
        }
        if (hi - lo > 0.15f) varies = true;
    }
    float c[16];
    simulate_levels(0.0, seed, a, 16);
    simulate_levels(0.3, seed, c, 16);
    for (int i = 0; i < 16; ++i) if (std::fabs(a[i] - c[i]) > 0.05f) moves = true;
    CHECK(all_in); CHECK(moves); CHECK(varies);
    simulate_levels(1.0, seed, a, 1);   // ein Band: kein Absturz
    // Farben: Grün → Gelb → Rot
    uint8_t lo[3], mid[3], hi[3];
    level_rgb(0.f, lo); level_rgb(0.6f, mid); level_rgb(1.f, hi);
    CHECK(lo[1] > lo[0] && lo[1] > lo[2]);          // grün
    CHECK(mid[0] > 200 && mid[1] > 150 && mid[2] < 40);   // gelb
    CHECK(hi[0] > hi[1] && hi[0] > hi[2]);          // rot
    level_rgb(-3.f, lo); level_rgb(7.f, hi);        // außerhalb: begrenzt
    CHECK(lo[1] > 200 && hi[0] > 200);
    std::printf("%d bestanden, %d fehlgeschlagen\n", passed, failed);
    return failed ? 1 : 0;
}
