// © 2026 FuchsLabs
#include "page_frame.h"

namespace fl::render {
Image draw_framed(const Rahmen& frame, const std::vector<Platzierung>& blocks) {
    Canvas d(frame.width, frame.height, frame.scale, frame.background);
    frame.header(d);
    // Wie bisher: kein Clip und keine Fehlerisolierung für eingebaute Seiten.
    for (const auto& entry : blocks)
        if (entry.placed.visible()) entry.block->draw(d, entry.placed.area, *entry.placed.density);
    frame.footer(d);
    return d.finish();
}
} // namespace fl::render
