// © 2026 FuchsLabs
// Datentypen des geräteseitigen Seitenübergangs (Companion-Fähigkeit xfade1, docs/COMPANION-PROTOKOLL.md).
// Eigener, abhängigkeitsfreier Header, damit output.h/app.h ihn ohne companion.h (und device_profile.h) einbinden können.
#pragma once
#include <cstdint>
#include <string>

namespace fl::companion {

inline constexpr const char* CAP_XFADE1 = "xfade1";   // Client animiert Seitenübergänge selbst
inline constexpr int TRANSITION_HOLD_EXTRA_MS = 80;   // Frame-Versand ruht ms + 80 ms nach Übergangsnachricht + Zielbild

struct Rgb {
    uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Rgb&) const = default;
};

struct Transition {
    std::string style = "hud";   // "hud" | "slide" | "fade"
    int dir = 1;                 // +1 nächste Seite, -1 vorherige (wie view.transition_dir)
    int ms = 450;
    Rgb accent, accent2, shade;
    uint64_t id = 0;             // nur serverintern (je Übergang eindeutig); nie auf der Leitung
};

}  // namespace fl::companion
