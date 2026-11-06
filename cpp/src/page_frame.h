// © 2026 FuchsLabs – gemeinsamer Einstieg für feste Seitenanordnungen.
#pragma once
#include <functional>
#include <vector>

#include "gfx.h"
#include "plugin_zones.h"

namespace fl::render {
// Eingebauter Rahmen: die bisherigen Renderer zeichnen Kopf und Fuß unverändert.
struct Rahmen {
    int width, height;
    float scale;
    Color background;
    std::function<void(Canvas&)> header, footer;
};
struct Platzierung {
    const plugins::zones::Block* block;
    plugins::zones::PlacedBlock placed;
};
Image draw_framed(const Rahmen& frame, const std::vector<Platzierung>& blocks);
} // namespace fl::render
