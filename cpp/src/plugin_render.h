// © 2026 FuchsLabs – native rendering of validated ScreenBuddy plugin data.
#pragma once

#include "gfx.h"
#include "plugins.h"

namespace fl::plugins {
// Caller holds render::mutex() and has applied the requested theme.
Image draw_page(const Page& page, int width, int height, bool sample = false);
}
