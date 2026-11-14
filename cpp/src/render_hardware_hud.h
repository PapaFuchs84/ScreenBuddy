// © 2026 FuchsLabs
#pragma once

#include "gfx.h"
#include "models.h"

namespace fl::render {
// Standalone, responsive hardware HUD. Tick is monotonic seconds; data is never simulated.
Image hardware_hud(int width, int height, const SysSnapshot& snapshot, double tick, int page_seconds = 8);
}
