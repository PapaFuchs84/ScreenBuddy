// © 2026 FuchsLabs
#include "plugin_render.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "plugin_zones.h"
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

// ---------------------------------------------------------------- Zonen-Layout (Manifest v2 mit layout)
// Zentrale Schwellen aller Plugin-Bausteine. Sie sind aus der Zeichengeometrie unten abgeleitet (Logische Pixel;
// Seitenränder, Schriftgrößen und Zeilenabstand siehe die Block-Klassen) und werden in docs/PLUGIN-SYSTEM.md
// zusammen mit dem Beispiel offline-zonen kalibriert. Ein Block zeichnet bei seiner Mindestfläche vollständig.
const zones::DensityTable& density_table() {
    static const zones::DensityTable table;
    return table;
}

using Colors = render::Theme;
struct Look {
    const Colors& colors;
    float inset;   // waagerechter Innenabstand der Karte (aus dem Rahmenstil)
    float radius;  // Eckenradius der Karte (aus dem Rahmenstil)
    // Schriftgrößen sind Bildpunkte und werden von der Skalierung der Zeichenfläche nicht mitskaliert: bei Skalierung s < 1
    // ist eine Zeile in logischen Pixeln 1/s mal so hoch. k = max(1, 1/s) dehnt Schwellen und senkrechte/waagerechte
    // Zeichengeometrie entsprechend; bei Skalierung >= 1 ist k = 1 (unverändertes Bild).
    float k = 1;
};

constexpr float kPadY = 6;      // oberer/unterer Innenabstand (bei k = 1)
constexpr float kLabelRow = 22; // Höhe der Bezeichnungszeile (Schrift 11, Mitte bei +12)
constexpr float kLinePitch = 18;

// Karte und Inhalt eines Plugin-Bausteins; die Mindestgrößen kommen aus der zentralen DensityTable.
class PluginBlock : public zones::TableBlock {
public:
    PluginBlock(const RenderedWidget& widget, const Look& look)
        : zones::TableBlock(widget.id, widget.type, density_table()), widget_(widget), look_(look) {}
    // Mindestfläche der Tabelle (für Skalierung 1) mal k; unendlich (unbekannter Typ) bleibt unendlich.
    zones::Size min_size(zones::Density density) const final {
        const zones::Size base = zones::TableBlock::min_size(density);
        return {base.width * look_.k, base.height * look_.k};
    }
    void draw(Canvas& canvas, const zones::Rect& area, zones::Density density) const final {
        const float x0 = area.x, y0 = area.y, x1 = area.x + area.width - 1, y1 = area.y + area.height - 1;
        canvas.rounded_rectangle(x0, y0, x1, y1, look_.radius, look_.colors.card);
        const float inset = std::min(look_.inset, area.width / 6);
        content(canvas, {area.x + inset, area.y, area.width - 2 * inset, area.height}, density);
    }
protected:
    // `inner`: Fläche ohne waagerechten Innenabstand; Texte werden gekürzt, nie über den Rand gezeichnet.
    virtual void content(Canvas& canvas, const zones::Rect& inner, zones::Density density) const = 0;
    const RenderedWidget& widget() const { return widget_; }
    const Colors& colors() const { return look_.colors; }
    bool has_label() const { return !widget_.label.empty(); }
    float k() const { return look_.k; }
    float pad() const { return kPadY * look_.k; }
    float label_row() const { return kLabelRow * look_.k; }
    float pitch() const { return kLinePitch * look_.k; }
    void label(Canvas& canvas, const zones::Rect& inner, float reserved_right = 0) const {
        if (!has_label()) return;
        canvas.text(inner.x, inner.y + 12 * k(), fit(canvas, widget_.label, {11, true}, inner.width - reserved_right), {11, true},
                    colors().muted, "lm");
    }
    // Mitte des Bereichs unter der Bezeichnungszeile (oder der ganzen Fläche ohne Bezeichnung), bis `bottom_reserved` vom Rand.
    float centre(const zones::Rect& inner, bool with_label, float bottom_reserved = 0) const {
        const float top = inner.y + (with_label && has_label() ? label_row() : 0);
        const float bottom = inner.y + inner.height - (with_label && has_label() ? pad() : 0) - bottom_reserved;
        return (top + bottom) / 2;
    }
    void single_line(Canvas& canvas, const zones::Rect& inner, const std::string& value, Font font, float y) const {
        canvas.text(inner.x, y, fit(canvas, value, font, inner.width), font, colors().text, "lm");
    }
private:
    const RenderedWidget& widget_;
    Look look_;
};

// Bricht an Leerzeichen um; mehr als `max_lines` Zeilen: der Rest wird an die letzte Zeile gehängt (fit() kürzt mit …).
std::vector<std::string> wrap(Canvas& canvas, const std::string& raw, Font font, float width, size_t max_lines) {
    const std::string value = render::clean_text(raw);
    std::vector<std::string> lines;
    std::string line;
    size_t position = 0;
    while (position <= value.size()) {
        size_t end = value.find(' ', position);
        if (end == std::string::npos) end = value.size();
        const std::string word = value.substr(position, end - position);
        position = end + 1;
        if (word.empty()) continue;
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (line.empty() || canvas.textlength(candidate, font) <= width) line = candidate;
        else { lines.push_back(line); line = word; }
    }
    if (!line.empty()) lines.push_back(line);
    if (max_lines > 0 && lines.size() > max_lines) {
        for (size_t i = max_lines; i < lines.size(); ++i) lines[max_lines - 1] += " " + lines[i];
        lines.resize(max_lines);
    }
    return lines;
}

class TextBlock final : public PluginBlock {
public:
    using PluginBlock::PluginBlock;
protected:
    void content(Canvas& canvas, const zones::Rect& inner, zones::Density density) const override {
        if (density == zones::Density::compact) {
            single_line(canvas, inner, widget().text, {13, false}, centre(inner, false));
            return;
        }
        label(canvas, inner);
        if (density == zones::Density::regular) {
            single_line(canvas, inner, widget().text, {14, false}, centre(inner, true));
            return;
        }
        const float top = inner.y + (has_label() ? label_row() : pad());
        const float room = inner.y + inner.height - pad() - top;
        const size_t max_lines = std::max<size_t>(1, static_cast<size_t>(room / pitch()));
        const auto lines = wrap(canvas, widget().text, {14, false}, inner.width, max_lines);
        for (size_t i = 0; i < lines.size(); ++i)
            canvas.text(inner.x, top + pitch() / 2 + static_cast<float>(i) * pitch(),
                        fit(canvas, lines[i], {14, false}, inner.width), {14, false}, colors().text, "lm");
    }
};

// Wert in großer Schrift (Kennzahl und Uhr); ausführlich: größere Schrift, Einheit klein dahinter.
class ValueBlock : public PluginBlock {
public:
    using PluginBlock::PluginBlock;
protected:
    bool with_unit() const { return widget().type == "metric" && !widget().unit.empty(); }
    void content(Canvas& canvas, const zones::Rect& inner, zones::Density density) const override {
        std::string value = widget().text;
        if (density == zones::Density::compact) {
            if (with_unit()) value += " " + widget().unit;
            // kompakt: Zeile aus Bezeichnung (links, klein) und Wert (rechts), wenn beides Platz hat; sonst nur der Wert
            const Font font{18, true};
            const std::string shown = render::clean_text(value);
            const float y = centre(inner, false), width = canvas.textlength(shown, font), room = inner.width - width - 8;
            if (has_label() && width <= inner.width && room >= 36) {
                canvas.text(inner.x, y, fit(canvas, widget().label, {11, true}, room), {11, true}, colors().muted, "lm");
                canvas.text(inner.x + inner.width, y, shown, font, colors().text, "rm");
            } else single_line(canvas, inner, value, font, y);
            return;
        }
        label(canvas, inner);
        if (density == zones::Density::regular) {
            if (with_unit()) value += " " + widget().unit;
            single_line(canvas, inner, value, {20, true}, centre(inner, true));
            return;
        }
        const Font small{12, false};
        Font big{26, true};  // größte Schrift 26..20, in die der Wert passt (sonst 20 mit …)
        for (float size : {26.f, 24.f, 22.f, 20.f}) {
            big.size = size;
            if (canvas.textlength(render::clean_text(value), big) <= inner.width) break;
        }
        const float y = centre(inner, true);
        const std::string shown = fit(canvas, value, big, inner.width);
        canvas.text(inner.x, y, shown, big, colors().text, "lm");
        if (with_unit() && shown == render::clean_text(value)) {
            const float used = canvas.textlength(shown, big) + 5;
            if (used + 12 <= inner.width)
                canvas.text(inner.x + used, y + 4 * k(), fit(canvas, widget().unit, small, inner.width - used), small, colors().muted, "lm");
        }
    }
};

class BarBlock final : public PluginBlock {
public:
    using PluginBlock::PluginBlock;
protected:
    void track(Canvas& canvas, const zones::Rect& inner, float top) const {
        if (!widget().bar_fraction) return;
        const float fraction = static_cast<float>(std::clamp(*widget().bar_fraction, 0.0, 1.0));
        canvas.rounded_rectangle(inner.x, top, inner.x + inner.width, top + 7 * k(), 3, colors().line);
        if (fraction > 0) canvas.rounded_rectangle(inner.x, top, inner.x + inner.width * fraction, top + 7 * k(), 3, colors().accent);
    }
    void content(Canvas& canvas, const zones::Rect& inner, zones::Density density) const override {
        std::string value = widget().text;
        if (!widget().unit.empty()) value += " " + widget().unit;
        if (density == zones::Density::compact) {
            if (widget().bar_fraction) track(canvas, inner, inner.y + inner.height / 2 - 4 * k());
            else single_line(canvas, inner, value, {12, false}, inner.y + inner.height / 2);
            return;
        }
        std::string percent;
        if (density == zones::Density::detailed && widget().bar_fraction)
            percent = std::to_string(static_cast<int>(std::lround(*widget().bar_fraction * 100))) + " %";
        const float percent_w = percent.empty() ? 0 : canvas.textlength(percent, {11, false}) + 6;
        label(canvas, inner, percent_w);
        if (!percent.empty())
            canvas.text(inner.x + inner.width, inner.y + 12 * k(), percent, {11, false}, colors().muted, "rm");
        const float bar_top = inner.y + inner.height - pad() - 8 * k();
        const float middle = (inner.y + (has_label() ? label_row() : 0) + bar_top) / 2;
        const Font font{density == zones::Density::detailed ? 20.f : 14.f, density == zones::Density::detailed};
        single_line(canvas, inner, value, font, widget().bar_fraction ? middle : centre(inner, true));
        track(canvas, inner, bar_top);
    }
};

class ListBlock final : public PluginBlock {
public:
    using PluginBlock::PluginBlock;
protected:
    void content(Canvas& canvas, const zones::Rect& inner, zones::Density density) const override {
        const auto& items = widget().items;
        if (density == zones::Density::compact) {
            if (!items.empty()) single_line(canvas, inner, items.front(), {12, false}, centre(inner, false));
            return;
        }
        const float top = inner.y + (has_label() ? label_row() : pad());
        const size_t fits = std::max<size_t>(1, static_cast<size_t>((inner.y + inner.height - pad() - top) / pitch()));
        const size_t shown = std::min({items.size(), fits, density == zones::Density::regular ? size_t{2} : size_t{5}});
        std::string more;
        if (density == zones::Density::detailed && items.size() > shown) more = "+" + std::to_string(items.size() - shown);
        const float more_w = more.empty() ? 0 : canvas.textlength(more, {11, true}) + 6;
        // "+N" steht in der Kopfzeile neben der Bezeichnung; ohne Bezeichnung in der Zeile des ersten Eintrags, dessen Breite
        // dafür um die Breite von "+N" gekürzt wird (sonst läge es über dem Text).
        const float first_row_y = top + pitch() / 2;
        label(canvas, inner, has_label() ? more_w : 0);
        if (!more.empty())
            canvas.text(inner.x + inner.width, has_label() ? inner.y + 12 * k() : first_row_y, more, {11, true}, colors().muted, "rm");
        for (size_t i = 0; i < shown; ++i)
            canvas.text(inner.x, first_row_y + static_cast<float>(i) * pitch(),
                        fit(canvas, items[i], {12, false}, inner.width - (i == 0 && !has_label() ? more_w : 0)), {12, false},
                        colors().text, "lm");
    }
};

std::unique_ptr<PluginBlock> make_block(const RenderedWidget& widget, const Look& look) {
    if (widget.type == "metric" || widget.type == "clock") return std::make_unique<ValueBlock>(widget, look);
    if (widget.type == "bar") return std::make_unique<BarBlock>(widget, look);
    if (widget.type == "list") return std::make_unique<ListBlock>(widget, look);
    return std::make_unique<TextBlock>(widget, look);  // "text"; unbekannte Typen passen nie (DensityTable) und bleiben aus
}

// Clip auf die Blockfläche, auch wenn ein Block eine Ausnahme wirft (sonst bliebe der Clip offen).
class ClipScope {
public:
    ClipScope(Canvas& canvas, const zones::Rect& area) : canvas_(canvas) {
        canvas_.clip_push(area.x, area.y, area.x + area.width - 1, area.y + area.height - 1);
    }
    ~ClipScope() { canvas_.clip_pop(); }
    ClipScope(const ClipScope&) = delete;
    ClipScope& operator=(const ClipScope&) = delete;
private:
    Canvas& canvas_;
};

void draw_zoned(Canvas& canvas, const Page& page, const Look& look, const zones::Rect& area, float gap) {
    std::vector<std::unique_ptr<PluginBlock>> blocks;
    std::vector<zones::BlockInput> inputs;
    for (const auto& widget : page.widgets) {
        if (blocks.size() >= static_cast<size_t>(zones::max_blocks)) break;
        blocks.push_back(make_block(widget, look));
        inputs.push_back(zones::describe(*blocks.back(), widget.zone));
    }
    const zones::Plan plan = zones::plan(area, *page.layout, inputs, gap);
    for (size_t i = 0; i < plan.blocks.size() && i < blocks.size(); ++i) {
        const auto& placed = plan.blocks[i];
        if (!placed.visible()) continue;  // zu wenig Fläche: ausgeblendet statt überlappt
        ClipScope clip(canvas, placed.area);  // Clip pro Block; die Fehlerisolierung folgt in Phase 3
        blocks[i]->draw(canvas, placed.area, *placed.density);
    }
}
}

Image draw_page(const Page& page, int width, int height, bool sample) {
    if (width < 1 || height < 1) return {};
    const auto& theme = render::theme();
    const float scale = render::scale_for(width, height);
    Canvas canvas(width, height, scale, theme.bg);
    const float logical_w = width / scale, logical_h = height / scale;
    // Rahmenstil aus dem Theme: Kartenradius und Abstandsmaß (FuchsLabs: Radius 8, Abstand 12 = bisherige Maße 10/7/9)
    const theme::Design design = theme::design(theme.style);
    const float spacing_scale = design.spacing / 12.f;
    const float margin = 10 * spacing_scale, gap = 7 * spacing_scale, inset = 9 * spacing_scale;
    const float card_radius = design.card_radius, header = 39, footer = 19;
    canvas.rectangle(0, 0, logical_w, 32, theme.orange);
    canvas.text(margin, 16, fit(canvas, page.title, {16, true}, logical_w - 2 * margin), {16, true}, theme.head_text, "lm");
    // Manifest v2 mit layout: Zonen-Plan statt automatischem Raster (v1 und v2 ohne layout zeichnen unverändert unten).
    if (page.layout)
        draw_zoned(canvas, page, {theme, inset, card_radius, std::max(1.f, 1.f / scale)},
                   {margin, header, logical_w - 2 * margin, logical_h - header - footer - margin}, gap);
    const size_t count = page.layout ? 0 : std::min<size_t>(page.widgets.size(), 6);
    const int columns = count <= 2 ? 1 : 2;
    const int rows = std::max(1, static_cast<int>((count + columns - 1) / columns));
    const float card_w = (logical_w - 2 * margin - (columns - 1) * gap) / columns;
    const float card_h = (logical_h - header - footer - margin - (rows - 1) * gap) / rows;
    for (size_t index = 0; index < count; ++index) {
        const auto& widget = page.widgets[index];
        const int row = static_cast<int>(index) / columns, column = static_cast<int>(index) % columns;
        const float x = margin + column * (card_w + gap), y = header + row * (card_h + gap);
        canvas.rounded_rectangle(x, y, x + card_w, y + card_h, card_radius, theme.card);
        const float usable = card_w - inset * 2;
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
