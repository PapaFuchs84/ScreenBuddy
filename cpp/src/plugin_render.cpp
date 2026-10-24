// © 2026 FuchsLabs
#include "plugin_render.h"

#include <algorithm>
#include <cmath>

#include "render.h"
#include "util.h"

namespace fl::plugins {
namespace {
std::string fit(Canvas& canvas, const std::string& raw, Font font, float width) {
    std::string value = render::clean_text(raw);
    if (canvas.textlength(value, font) <= width) return value;
    std::u32string points = to_u32(value);
    size_t low = 0, high = points.size();
    while (low < high) {
        const size_t middle = (low + high + 1) / 2;
        if (canvas.textlength(from_u32(points.substr(0, middle)) + "…", font) <= width) low = middle;
        else high = middle - 1;
    }
    return from_u32(points.substr(0, low)) + "…";
}
}

Image draw_page(const Page& page, int width, int height, bool sample) {
    if (width < 1 || height < 1) return {};
    const auto& theme = render::theme();
    const float scale = render::scale_for(width, height);
    Canvas canvas(width, height, scale, theme.bg);
    const float logical_w = width / scale, logical_h = height / scale;
    const float margin = 10, gap = 7, header = 39, footer = 19;
    canvas.rectangle(0, 0, logical_w, 32, theme.orange);
    canvas.text(margin, 16, fit(canvas, page.title, {16, true}, logical_w - 2 * margin), {16, true}, theme.head_text, "lm");
    const size_t count = std::min<size_t>(page.widgets.size(), 6);
    const int columns = count <= 2 ? 1 : 2;
    const int rows = std::max(1, static_cast<int>((count + columns - 1) / columns));
    const float card_w = (logical_w - 2 * margin - (columns - 1) * gap) / columns;
    const float card_h = (logical_h - header - footer - margin - (rows - 1) * gap) / rows;
    for (size_t index = 0; index < count; ++index) {
        const auto& widget = page.widgets[index];
        const int row = static_cast<int>(index) / columns, column = static_cast<int>(index) % columns;
        const float x = margin + column * (card_w + gap), y = header + row * (card_h + gap);
        canvas.rounded_rectangle(x, y, x + card_w, y + card_h, 9, theme.card);
        const float inset = 9, usable = card_w - inset * 2;
        if (!widget.label.empty())
            canvas.text(x + inset, y + 15, fit(canvas, widget.label, {11, true}, usable), {11, true}, theme.muted, "lm");
        const float content_y = y + (widget.label.empty() ? 20 : 35);
        if (widget.type == "list") {
            for (size_t i = 0; i < widget.items.size(); ++i) {
                const float line_y = content_y + static_cast<float>(i) * 18;
                if (line_y > y + card_h - 7) break;
                canvas.text(x + inset, line_y, fit(canvas, widget.items[i], {12, false}, usable),
                            {12, false}, theme.text, "lm");
            }
        } else {
            const Font value_font{widget.type == "metric" || widget.type == "clock" ? 20.f : 14.f,
                                  widget.type == "metric" || widget.type == "clock"};
            std::string value = widget.text;
            if (!widget.unit.empty() && widget.type == "metric") value += " " + widget.unit;
            canvas.text(x + inset, content_y, fit(canvas, value, value_font, usable), value_font, theme.text, "lm");
            if (widget.type == "bar" && widget.bar_fraction) {
                const float bar_y = std::min(y + card_h - 14, content_y + 22);
                const float fraction = static_cast<float>(std::clamp(*widget.bar_fraction, 0.0, 1.0));
                canvas.rounded_rectangle(x + inset, bar_y, x + inset + usable, bar_y + 7, 3, theme.line);
                if (fraction > 0)
                    canvas.rounded_rectangle(x + inset, bar_y, x + inset + usable * fraction, bar_y + 7, 3, theme.accent);
            }
        }
    }
    const char* state = sample ? "Beispieldaten – keine Live-Daten" :
                        page.missing_data ? "Plugin-Daten fehlen" : "ScreenBuddy Plugin";
    canvas.text(margin, logical_h - 8, state, {10, false}, page.missing_data ? theme.warn : theme.muted, "lm");
    return canvas.finish();
}
}
