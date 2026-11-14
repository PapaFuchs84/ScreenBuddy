// © 2026 FuchsLabs
#pragma once

#include "gfx.h"
#include "models.h"
#include "render.h"

namespace fl::render {
// Colours of the hardware HUD. The derived tones (grid, dimmed accents, scan line)
// are part of the palette so the classic neon look stays bit-identical.
struct HudPalette {
    Color bg, panel, primary, secondary, text, muted, warn;
    Color grid, dim_primary, dim_secondary, scan;
};
HudPalette hud_neon_palette();
// Fills grid/dim/scan from bg, primary and secondary.
HudPalette hud_derive(HudPalette p);
// system.hud_colors: "theme" (default, from the active skin), "neon" or "custom"
// (system.hud_color_primary/_secondary/_background as #RRGGBB; empty = from the skin).
// Unreadable colours fall back to readable ones; invalid values are ignored.
HudPalette hud_palette(const ojson& system, const Theme& skin);

// Standalone, responsive hardware HUD. Tick is monotonic seconds; data is never simulated.
Image hardware_hud(int width, int height, const SysSnapshot& snapshot, double tick, int page_seconds = 8,
                   const HudPalette& palette = hud_neon_palette());
}
