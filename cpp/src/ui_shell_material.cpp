// Copyright 2026 FuchsLabs
#include "ui_shell_material.h"

#include <algorithm>
#include <cmath>

namespace fl::ui::shell {
namespace {

// All values are DIP. This deliberately uses a compact destination rail rather
// than repeating the full ScreenBuddy page list as a sidebar. The host supplies
// the five destination widgets and the contextual tabs below the headline.
float bounded(float value, float lo, float hi, float fallback) {
    return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
}
ui::Rect rect(float left, float top, float right, float bottom) {
    return {left, top, std::max(left, right), std::max(top, bottom)};
}
bool present(const ui::Rect& r) { return r.right > r.left && r.bottom > r.top; }
ui::Rgba mix(ui::Rgba a, ui::Rgba b, float amount) {
    return {a.r + (b.r - a.r) * amount, a.g + (b.g - a.g) * amount,
            a.b + (b.b - a.b) * amount, 1.f};
}

}  // namespace

MaterialShell layout_material(float width, float height, bool show_inspector,
                              bool dirty, float appearance_width, float language_width) {
    const float w = bounded(width, 640.f, 8192.f, 980.f);
    const float h = bounded(height, 480.f, 8192.f, 680.f);
    MaterialShell result;
    constexpr float rail_width = 96.f;
    constexpr float margin = 24.f;
    constexpr float gutter = 16.f;
    const float left = rail_width + margin;
    const float right = w - margin;

    result.rail = rect(0, 0, rail_width, h);
    for (size_t i = 0; i < result.destinations.size(); ++i) {
        const float top = 104.f + float(i) * 72.f;
        result.destinations[i] = rect(8, top, rail_width - 8, top + 64.f);
    }

    // Global controls stay above the content, irrespective of whether the
    // optional inspector is open. They never overlap the page headline.
    result.preview_toggle = rect(right - 48.f, 16, right, 52);
    const float language = bounded(language_width, 48.f, 112.f, 72.f);
    const float appearance = bounded(appearance_width, 100.f, 280.f, 180.f);
    result.language = rect(result.preview_toggle.left - 12.f - language, 16,
                           result.preview_toggle.left - 12.f, 52);
    result.appearance = rect(std::max(left, result.language.left - 12.f - appearance), 16,
                             result.language.left - 12.f, 52);

    result.inspector_visible = show_inspector;
    float content_right = right;
    if (show_inspector) {
        const float inspector_width = std::clamp(w * .24f, 264.f, 344.f);
        result.inspector = rect(right - inspector_width, 64, right, h - margin);
        content_right = result.inspector.left - gutter;
    }
    result.header = rect(left, 72, content_right, 140);
    result.tabs = rect(left, 152, content_right, 196);
    const float content_bottom = h - (dirty ? 112.f : margin);
    result.body = rect(left, 212, content_right, content_bottom);

    // A single floating action group appears only for pending changes. It is
    // visually separate from page content and follows the available body width.
    if (dirty) {
        constexpr float save_width = 148.f, discard_width = 104.f, gap = 8.f;
        const float bottom = h - 32.f;
        result.save = rect(content_right - save_width, bottom - 48.f, content_right, bottom);
        result.discard = rect(result.save.left - gap - discard_width, bottom - 48.f,
                              result.save.left - gap, bottom);
    }
    return result;
}

void paint_material(ui::Ctx& c, const MaterialShell& layout,
                    const ui::Text& page_title, const ui::Text& subtitle) {
    // Semantic tonal layers derive exclusively from the active theme. Dark
    // mode therefore retains its hierarchy instead of receiving white overlays.
    const auto rail_surface = mix(c.th.bg, c.th.card, .65f);
    const auto content_surface = mix(c.th.bg, c.th.card, .22f);
    const auto inspector_surface = mix(c.th.card, c.th.accent, c.th.dark ? .06f : .035f);
    c.fill(layout.rail, rail_surface);

    const ui::Rect content = rect(layout.header.left - 12.f, layout.header.top - 8.f,
                                   layout.body.right + 12.f, layout.body.bottom + 12.f);
    c.fill(content, content_surface, 28.f);

    c.text(L"ScreenBuddy", rect(layout.header.left, 16, layout.appearance.left - 16.f, 52),
           ui::Font::Brand, c.th.text, DWRITE_TEXT_ALIGNMENT_LEADING);
    const ui::Rect title = rect(layout.header.left, layout.header.top,
                                 layout.header.right, layout.header.top + 38.f);
    c.text(c.s(page_title), title, ui::Font::H1, c.th.text,
           DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
    if (!subtitle.empty()) {
        c.text(c.s(subtitle), rect(title.left, title.bottom + 4.f,
                                    title.right, layout.header.bottom),
               ui::Font::Body, c.th.muted, DWRITE_TEXT_ALIGNMENT_LEADING, false, true);
    }

    if (layout.inspector_visible && present(layout.inspector))
        c.fill(layout.inspector, inspector_surface, 24.f);

    // These are surfaces only. The host paints its actual, accessible widgets
    // into destinations/tabs/control rectangles and retains command semantics.
    if (present(layout.save)) {
        const ui::Rect actions = rect(layout.discard.left - 12.f, layout.save.top - 12.f,
                                      layout.save.right + 12.f, layout.save.bottom + 12.f);
        c.fill(actions, mix(c.th.card, c.th.accent, c.th.dark ? .10f : .06f), 28.f);
        c.frame(actions, mix(c.th.line, c.th.accent, .15f), 28.f);
    }
}

}  // namespace fl::ui::shell
