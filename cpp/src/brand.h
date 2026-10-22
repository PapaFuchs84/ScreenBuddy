// © 2026 FuchsLabs
// FuchsLabs-CI: Farben und das Fuchs-Logo (nur Flächen, daher bis 16 px scharf).
// Hauptfarben sind Fuchs-Orange und Creme; Navy ist die Schrift- und Kontrastfarbe.
#pragma once

#include "gfx.h"

namespace fl::brand {

inline constexpr Color ORANGE{247, 127, 0};      // Fuchs-Orange #F77F00 – Flächen, Kopfzeile, Balken, Logo-Grund
inline constexpr Color GLUT{176, 82, 0};         // Glut #B05200 – Orange als Schrift auf Creme
inline constexpr Color CREME{255, 244, 230};     // Creme #FFF4E6 – Hintergrund, Fuchskopf
inline constexpr Color SAND{250, 228, 200};      // Sand #FAE4C8 – Karten auf Creme
inline constexpr Color LINE{238, 210, 176};      // Trennlinien #EED2B0
inline constexpr Color NAVY{20, 33, 61};         // Navy #14213D – Schrift, Augen; im Dunkelmodus Karten
inline constexpr Color NAVY_TIEF{14, 22, 40};    // Navy tief #0E1628 – Hintergrund im Dunkelmodus
inline constexpr Color NAVY_LINIE{39, 57, 95};   // Trennlinien im Dunkelmodus #27395F
inline constexpr Color NEBEL{104, 96, 92};       // Nebel #68605C – Nebentext
inline constexpr Color PFIRSICH{253, 214, 170};  // Pfirsich #FDD6AA – Fuchsbrust
inline constexpr Color WHITE{255, 255, 255};

}  // namespace fl::brand
