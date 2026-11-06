// © 2026 FuchsLabs
#include "plugin_render.h"
#include <set>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>

#include "plugin_isolation.h"
#include "plugin_render_testing.h"
#include "plugin_zones.h"
#include "render.h"
#include "util.h"

namespace fl::plugins {
std::vector<zones::Rect> list_rows(zones::Rect a, zones::Density d, float k, bool label, size_t count) {
    std::vector<zones::Rect> out;
    if(a.width<=0 || a.height<=0 || k<=0) return out;
    if (d==zones::Density::compact) { if(count) out.push_back(a); return out; }
    float top=a.y+(label ? 22 : 6)*k, pitch=18*k;
    size_t fits=size_t(std::max(0.f,(a.y+a.height-6*k-top)/pitch));
    size_t shown=std::min({count,fits,d==zones::Density::regular ? size_t(2) : size_t(5)});
    for(size_t i=0;i<shown;++i) out.push_back({a.x,top+i*pitch,a.width,pitch});
    return out;
}
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
    std::vector<PluginTarget>* targets = nullptr;
    const std::string* plugin = nullptr;
    const testing::DrawHooks* hooks = nullptr;  // nur Tests (plugin_render_testing.h); in der App immer nullptr
};

// Block-Schlüssel für Log und Test-Hook: v2 die Block-ID, v1 "#<Position im Manifest>".
std::string block_key(const RenderedWidget& widget) {
    return widget.id.empty() ? "#" + std::to_string(widget.index) : widget.id;
}
void after_card(const testing::DrawHooks* hooks, Canvas& canvas, const RenderedWidget& widget) {
    if (hooks && hooks->after_card) hooks->after_card(canvas, block_key(widget));
}
void after_content(const testing::DrawHooks* hooks, Canvas& canvas, const RenderedWidget& widget) {
    if (hooks && hooks->after_content) hooks->after_content(canvas, block_key(widget));
}
// Ein Log-Eintrag je Plugin, Block und Stufe höchstens alle 60 s (Zeichnen läuft in jedem Frame). Der Text ist fest:
// nie Daten, Einstellungswerte oder Ausnahmetexte.
void report(const std::string& plugin, const std::string& block, isolation::Stage stage) noexcept {
    try {
        static std::mutex mutex;
        static isolation::RateLimiter limiter;
        {
            std::lock_guard lock(mutex);
            if (!limiter.allow(isolation::limiter_key(plugin, block, stage), isolation::RateLimiter::Clock::now())) return;
        }
        LOGW("plugins", "%s", isolation::failure_message(plugin, block, stage).c_str());
    } catch (...) {}  // Logging darf die Seite nie gefährden
}

void collect(std::vector<PluginTarget>* targets, const std::string& plugin, const RenderedWidget& w,
             const zones::Rect& area, float scale, const std::vector<zones::Rect>& rows) {
    if (!targets || w.tap_action.empty()) return;
    auto add=[&](zones::Rect r, const std::string& url) {
        if(w.tap_action=="open_link" && url.empty()) return;
        float right=std::min(r.x+r.width,area.x+area.width), bottom=std::min(r.y+r.height,area.y+area.height);
        r.x=std::max(r.x,area.x); r.y=std::max(r.y,area.y);
        r.width=right-r.x; r.height=bottom-r.y;
        if(r.width<=0 || r.height<=0) return;
        int x=int(std::floor(r.x*scale)), y=int(std::floor(r.y*scale));
        targets->push_back({{x,y,int(std::ceil((r.x+r.width)*scale))-x,int(std::ceil((r.y+r.height)*scale))-y},
            plugin,w.id,w.zone,w.tap_action,url});
    };
    if(w.type=="list") {
        for(size_t i=0;i<rows.size();++i) add(rows[i],i<w.item_urls.size() ? w.item_urls[i] : "");
    } else add(area,w.tap_url);
}

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
        after_card(look_.hooks, canvas, widget_);
        const float inset = std::min(look_.inset, area.width / 6);
        content(canvas, {area.x + inset, area.y, area.width - 2 * inset, area.height}, density);
        after_content(look_.hooks, canvas, widget_);
        if(look_.targets && !widget_.tap_action.empty()) {
            auto rows=list_rows(area,density,k(),has_label(),widget_.items.size());
            collect(look_.targets,*look_.plugin,widget_,area,canvas.s(),rows);
        }
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
        const auto rows=list_rows(inner,density,k(),has_label(),items.size());
        const size_t shown=rows.size();
        std::string more;
        if (density == zones::Density::detailed && items.size() > shown) more = "+" + std::to_string(items.size() - shown);
        const float more_w = more.empty() ? 0 : canvas.textlength(more, {11, true}) + 6;
        // "+N" steht in der Kopfzeile neben der Bezeichnung; ohne Bezeichnung in der Zeile des ersten Eintrags, dessen Breite
        // dafür um die Breite von "+N" gekürzt wird (sonst läge es über dem Text).
        const float first_row_y = rows.empty() ? inner.y : rows[0].y + rows[0].height / 2;
        label(canvas, inner, has_label() ? more_w : 0);
        if (!more.empty())
            canvas.text(inner.x + inner.width, has_label() ? inner.y + 12 * k() : first_row_y, more, {11, true}, colors().muted, "rm");
        for (size_t i = 0; i < shown; ++i)
            canvas.text(inner.x, rows[i].y + rows[i].height / 2,
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

// Clip-Stapel eines Blocks: optional ein Clip auf die Blockfläche; beim Verlassen (auch durch eine Ausnahme) wird der
// Stapel exakt auf die Tiefe von vorher zurückgesetzt, also auch Clips geschlossen, die der Block selbst offen ließ.
class ClipScope {
public:
    ClipScope(Canvas& canvas, const zones::Rect* area) : canvas_(canvas), depth_(canvas.clip_depth()), own_(area ? 1 : 0) {
        if (area) canvas_.clip_push(area->x, area->y, area->x + area->width - 1, area->y + area->height - 1);
    }
    ~ClipScope() { unwind(depth_); }
    // Nach einem Fehler: nur die inneren Clips des Blocks schließen, der eigene Clip bleibt für das Löschen aktiv.
    void unwind_inner() { unwind(depth_ + own_); }
    ClipScope(const ClipScope&) = delete;
    ClipScope& operator=(const ClipScope&) = delete;
private:
    void unwind(int depth) { while (canvas_.clip_depth() > depth) canvas_.clip_pop(); }
    Canvas& canvas_;
    const int depth_, own_;
};

// Fehlerisolierung: zeichnet einen Block mit Clip auf seine Fläche; wirft er (auch halb fertig oder nach dem letzten
// Text), wird seine Fläche wieder mit dem Seitenhintergrund gefüllt (der Block ist ausgeblendet), der Clip-Stapel
// wiederhergestellt und ein ratenbegrenzter Log-Eintrag geschrieben. Weil der Clip auf genau der Löschfläche liegt,
// kann nichts außerhalb übrig bleiben. Die übrigen Blöcke und der Fußtext werden normal gezeichnet.
template <class Draw>
void isolated(Canvas& canvas, const Page& page, const RenderedWidget& widget, const zones::Rect& area, Color bg, Draw&& draw, std::vector<PluginTarget>* targets = nullptr) {
    const auto target_count=targets ? targets->size() : 0;
    ClipScope scope(canvas, &area);
    try {
        draw();
    } catch (...) {
        if(targets) targets->resize(target_count);
        try {
            scope.unwind_inner();
            canvas.rectangle(area.x, area.y, area.x + area.width - 1, area.y + area.height - 1, bg);
        } catch (...) {}
        report(page.id, block_key(widget), isolation::Stage::draw);
    }
}
void draw_zoned(Canvas& canvas, const Page& page, const Look& look, const zones::Rect& area, float gap) {
    std::vector<std::unique_ptr<PluginBlock>> blocks;
    std::vector<zones::BlockInput> inputs;
    for (const auto& widget : page.widgets) {
        if (blocks.size() >= static_cast<size_t>(zones::max_blocks)) break;
        // Auch das Messen ist abgesichert: ein Block, der dabei wirft, passt nie (bleibt aus); Indizes bleiben gleich.
        try {
            blocks.push_back(make_block(widget, look));
            inputs.push_back(zones::describe(*blocks.back(), widget.zone));
        } catch (...) {
            if (blocks.size() > inputs.size()) blocks.back().reset();
            else blocks.push_back(nullptr);
            zones::BlockInput never{widget.id, widget.zone, {}};
            for (auto& size : never.min.min) size = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
            inputs.push_back(std::move(never));
            report(page.id, block_key(widget), isolation::Stage::draw);
        }
    }
    const zones::Plan plan = zones::plan(area, *page.layout, inputs, gap);
    for (size_t i = 0; i < plan.blocks.size() && i < blocks.size(); ++i) {
        const auto& placed = plan.blocks[i];
        if (!placed.visible() || !blocks[i]) continue;  // zu wenig Fläche: ausgeblendet statt überlappt
        isolated(canvas, page, page.widgets[i], placed.area, look.colors.bg,
                 [&] { blocks[i]->draw(canvas, placed.area, *placed.density); },look.targets);
    }
}

// Automatisches Raster (v1 und v2 ohne layout): Zeichnung unverändert, je Karte abgesichert und auf das Kartenrechteck
// geclippt (Text, der bei kleiner Skalierung über die Karte hinausragen würde, wird abgeschnitten; die Snapshots sind
// unverändert, siehe fl-plugin-snapshot-tests).
void draw_grid(Canvas& canvas, const Page& page, const render::Theme& theme, const testing::DrawHooks* hooks, float logical_w,
               float logical_h, float margin, float gap, float inset, float card_radius, float header, float footer, std::vector<PluginTarget>* targets) {
    const size_t count = std::min<size_t>(page.widgets.size(), 6);
    const int columns = count <= 2 ? 1 : 2;
    const int rows = std::max(1, static_cast<int>((count + columns - 1) / columns));
    const float card_w = (logical_w - 2 * margin - (columns - 1) * gap) / columns;
    const float card_h = (logical_h - header - footer - margin - (rows - 1) * gap) / rows;
    for (size_t index = 0; index < count; ++index) {
        const auto& widget = page.widgets[index];
        const int row = static_cast<int>(index) / columns, column = static_cast<int>(index) % columns;
        const float x = margin + column * (card_w + gap), y = header + row * (card_h + gap);
        // Clip- und Löschfläche = Kartenrechteck plus 1 logisches Pixel ringsum (rounded_rectangle schließt x + card_w /
        // y + card_h ein, die Kantenglättung reicht bei gebrochenen Koordinaten ein Pixel weiter); der Abstand zur Nachbarkarte
        // ist größer, Nachbarn werden also nie berührt.
        isolated(canvas, page, widget, {x - 1, y - 1, card_w + 3, card_h + 3}, theme.bg, [&] {
            canvas.rounded_rectangle(x, y, x + card_w, y + card_h, card_radius, theme.card);
            after_card(hooks, canvas, widget);
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
            after_content(hooks, canvas, widget);
            std::vector<zones::Rect> rows;
            if(widget.type=="list") for(size_t i=0;i<widget.items.size();++i) {
                float cy=content_y+i*18; if(cy>y+card_h-7) break;
                rows.push_back({x,cy-9,card_w+1,18});
            }
            collect(targets,page.id,widget,{x,y,card_w+1,card_h+1},canvas.s(),rows);
        },targets);
    }
}

Image draw_page_with(const Page& page, int width, int height, bool sample, const testing::DrawHooks* hooks, std::vector<PluginTarget>* targets = nullptr) {
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
    // Blöcke, deren Datenauswertung in render_page fehlschlug, fehlen in page.widgets; hier nur der Log-Eintrag.
    for (const auto& key : page.failed) report(page.id, key, isolation::Stage::data);
    try {
        // Manifest v2 mit layout: Zonen-Plan statt automatischem Raster (v1 und v2 ohne layout zeichnen unverändert).
        if (page.layout)
            draw_zoned(canvas, page, {theme, inset, card_radius, std::max(1.f, 1.f / scale), targets, &page.id, hooks},
                       {margin, header, logical_w - 2 * margin, logical_h - header - footer - margin}, gap);
        else draw_grid(canvas, page, theme, hooks, logical_w, logical_h, margin, gap, inset, card_radius, header, footer, targets);
    } catch (...) {}  // nur außerhalb der Blöcke möglich (z. B. kein Speicher für den Plan): Kopf und Fuß bleiben
    const char* state = sample ? "Beispieldaten – keine Live-Daten" :
                        page.missing_data ? "Plugin-Daten fehlen" : "ScreenBuddy Plugin";
    canvas.text(margin, logical_h - 8, state, {10, false}, page.missing_data ? theme.warn : theme.muted, "lm");
    return canvas.finish();
}
}

// Pixelrechtecke bleiben auf die Bildgrenzen beschränkt; außerhalb bleibt B.
void blend_rect(Image& dst, const Image& from, const Image& to, zones::Rect r, float mix) {
    if (from.w != to.w || from.h != to.h || dst.w != to.w || dst.h != to.h) return;
    int weight = int(std::clamp(mix,0.f,1.f)*256);
    for (int y=std::max(0,int(std::floor(r.y))); y<std::min(dst.h,int(std::ceil(r.y+r.height))); ++y)
        for (int x=std::max(0,int(std::floor(r.x))); x<std::min(dst.w,int(std::ceil(r.x+r.width))); ++x) {
            size_t i=size_t(y)*dst.w+x; uint32_t pixel=0;
            for (int shift=0;shift<32;shift+=8) pixel |= (((from.px[i]>>shift & 255)*(256-weight)+(to.px[i]>>shift & 255)*weight)/256)<<shift;
            dst.px[i]=pixel;
        }
}
void reveal_rect(Image& dst, const Image& from, const Image& to, zones::Rect r, float mix) {
    if (from.w != to.w || from.h != to.h || dst.w != to.w || dst.h != to.h) return;
    int top=std::max(0,int(std::floor(r.y))), bottom=std::min(dst.h,int(std::ceil(r.y+r.height)));
    // Schritte teilen globale Übertragungsbänder, auch bei versetzter Kachel.
    int first_band=top/8, bands=(bottom+7)/8-first_band;
    int edge = mix <= 0 ? top : mix >= 1 ? bottom
        : std::min(bottom,(first_band+int(std::ceil(bands*std::clamp(mix,0.f,1.f))))*8);
    for (int y=top;y<bottom;++y) for (int x=std::max(0,int(std::floor(r.x)));x<std::min(dst.w,int(std::ceil(r.x+r.width)));++x) {
        size_t i=size_t(y)*dst.w+x; dst.px[i]=y<edge ? to.px[i] : from.px[i];
    }
}
DrawOut draw_choices_out(const Page& page, int w, int h, const std::vector<schedule::ZoneChoice>& choices,
                   std::map<std::string,std::shared_ptr<FadeCache>>& caches) {
    if (choices.empty() || !page.layout) return draw_page_out(page,w,h);
    auto selected = [&](bool from) {
        Page p=page;
        std::erase_if(p.widgets,[&](const auto& widget) {
            auto c=std::find_if(choices.begin(),choices.end(),[&](const auto& c){return c.zone==widget.zone;});
            if (c==choices.end()) return false;
            auto block=from && c->from_block ? c->from_block : c->block;
            return !block || widget.id != *block;
        });
        return p;
    };
    Image result;
    bool fading = std::any_of(choices.begin(),choices.end(),[](const auto& c){return c.from_block.has_value();});
    if (!fading) { caches.clear(); return draw_page_out(selected(false),w,h); }
    const float scale=render::scale_for(w,h);
    const auto design=theme::design(render::theme().style);
    const float spacing=design.spacing/12.f, margin=10*spacing;
    auto plan=zones::plan({margin,39,w/scale-2*margin,h/scale-39-19-margin},*page.layout,{},7*spacing);
    // Gemeinsamer Stand A/B je Kachel, nur bei Beginn/Größe/Theme neu zeichnen.
    std::string signature;
    for (const auto& c:choices) signature+=c.zone+":"+c.from_block.value_or("")+">"+c.block.value_or("")+";";
    auto& cache=caches[page.id]; if (!cache) cache=std::make_shared<FadeCache>();
    if (cache->width!=w || cache->height!=h || cache->revision!=render::theme_revision() || cache->signature!=signature) {
        auto old=draw_page_out(selected(true),w,h);
        cache->from=std::move(old.image); cache->from_targets=std::move(old.targets);
        auto drawn=draw_page_out(selected(false),w,h);
        cache->to=std::move(drawn.image); cache->targets=std::move(drawn.targets);
        cache->width=w; cache->height=h; cache->revision=render::theme_revision(); cache->signature=signature;
    }
    result=cache->to;
    for (const auto& c:choices) if (c.from_block) {
        auto zone=std::find_if(plan.zones.begin(),plan.zones.end(),[&](const auto& z){return z.id==c.zone;});
        if (zone==plan.zones.end()) continue;
        auto r=zone->area; r.x*=scale; r.y*=scale; r.width*=scale; r.height*=scale;
        if (c.shape==schedule::Shape::reveal) reveal_rect(result,cache->from,cache->to,r,c.mix);
        else blend_rect(result,cache->from,cache->to,r,c.mix);
    }
    auto targets=cache->targets;
    for(const auto& t:cache->from_targets)
        if(std::any_of(choices.begin(),choices.end(),[&](const auto& c){return c.zone==t.zone && c.from_block;})) targets.push_back(t);
    for(auto& t:targets) for(const auto& c:choices) if(c.zone==t.zone && c.from_block) t.transitioning=true;
    return {std::move(result),std::move(targets)};
}
Image draw_choices(const Page& p,int w,int h,const std::vector<schedule::ZoneChoice>& c,std::map<std::string,std::shared_ptr<FadeCache>>& caches) {
    return draw_choices_out(p,w,h,c,caches).image;
}

DrawOut draw_page_out(const Page& page, int width, int height, const schedule::Result* selection, bool sample) {
    DrawOut out;
    if (selection) { std::map<std::string,std::shared_ptr<FadeCache>> caches; return draw_choices_out(page,width,height,selection->zones,caches); }
    if (!page.layout || std::none_of(page.layout->zones.begin(),page.layout->zones.end(),[](const auto& z){return z.cycle.has_value();}))
        { out.image=draw_page_with(page,width,height,sample,nullptr,&out.targets); return out; }
    Page selected=page;
    std::set<std::string> seen;
    std::erase_if(selected.widgets,[&](const auto& w){
        auto z=std::find_if(page.layout->zones.begin(),page.layout->zones.end(),[&](const auto& z){return z.id==w.zone;});
        return z!=page.layout->zones.end() && z->cycle && (w.missing || !seen.insert(w.zone).second);
    });
    out.image=draw_page_with(selected,width,height,sample,nullptr,&out.targets); return out;
 }
Image draw_page(const Page& p,int w,int h,bool sample) { return draw_page_out(p,w,h,nullptr,sample).image; }
namespace testing {
Image draw_page(const Page& page, int width, int height, bool sample, const DrawHooks& hooks) {
    return draw_page_with(page, width, height, sample, &hooks);
}
}
}