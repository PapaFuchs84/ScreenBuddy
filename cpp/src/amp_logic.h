// (c) 2026 FuchsLabs - Reine Logik der Amplifier-Anzeige (plattformneutral, unter Linux testbar):
// simulierte Balken für Remote-Wiedergabe und Farbstufen. Es wird nichts aufgezeichnet oder aus dem Netz gelesen.
#pragma once
#include <cstdint>
#include <string>

namespace fl::audio {

// Farbmodus der Balken (Einstellung spotify.amp_color)
enum class AmpColor { Theme, Level, Gradient };
AmpColor parse_amp_color(const std::string& s);   // "theme" | "level" (Standard) | "gradient"

// Stabiler Startwert je Titel, damit derselbe Titel immer gleich "tanzt".
uint32_t amp_seed(const std::string& title, const std::string& artist);

// Simulierte Balken (0..1) zum Zeitpunkt t Sekunden. Tempo (84..147 BPM) und Muster folgen aus `seed`.
// Das ist KEINE Messung der Musik, sondern eine Animation; die Anzeige kennzeichnet sie als "simuliert".
void simulate_levels(double t, uint32_t seed, float* out, int bands);

// Pegelfarbe: Grün (niedrig) → Gelb → Rot (hoch), stufenlos. v in 0..1.
void level_rgb(float v, uint8_t rgb[3]);

}  // namespace fl::audio
