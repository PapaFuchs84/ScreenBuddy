// Copyright 2026 FuchsLabs
// M3 Expressive-inspired ScreenBuddy shell. Geometry is our Windows adaptation,
// not a claim that these dimensions are prescribed Material component tokens.
#pragma once

#include <array>
#include "ui_kit.h"

namespace fl::ui::shell {

struct MaterialShell {
    ui::Rect rail{}, header{}, body{}, inspector{}, save{}, discard{}, language{},
        appearance{}, preview_toggle{}, tabs{};
    std::array<ui::Rect, 5> destinations{};
    bool inspector_visible = false;
};

MaterialShell layout_material(float w, float h, bool show_inspector, bool dirty,
                              float appearance_width, float language_width);
void paint_material(ui::Ctx& c, const MaterialShell& layout,
                    const ui::Text& page_title, const ui::Text& subtitle);

}  // namespace fl::ui::shell
