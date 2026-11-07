// © 2026 FuchsLabs – Fensteraufbau des Anordnungs-Editors (Phase 7b). Clean-Room.
#include "ui_editor.h"
#include <algorithm>

namespace fl::ui {
namespace {
float rw(const Rect& r) { return r.right - r.left; }
float rh(const Rect& r) { return r.bottom - r.top; }
Rect inflate(Rect r, float d) { return {r.left - d, r.top - d, r.right + d, r.bottom + d}; }
Rgba alpha(Rgba c, float a) { c.a = a; return c; }
}

void Chip::paint(Ctx& c) {
    c.fill(rc, alpha(c.th.accent, c.th.dark ? .16f : .10f), rh(rc) / 2);
    c.frame(rc, c.th.line, rh(rc) / 2);
    c.text(c.s(text), inflate(rc, -6), Font::Body, c.th.text, DWRITE_TEXT_ALIGNMENT_CENTER);
}

void Flow::remove(Widget* w) {
    for (auto it = items.begin(); it != items.end(); ++it) if (it->get() == w) {
        std::vector<WidgetPtr> one;
        one.push_back(std::move(*it));
        items.erase(it);
        retire(one);
        return;
    }
}
float Flow::layout(Ctx& c, float x, float y, float w) {
    float cx = x, cy = y, line = 0;
    std::vector<Widget*> row;
    auto settle = [&] {  // Elemente einer Zeile vertikal mittig
        for (Widget* item : row) {
            const float h = rh(item->rc), dy = (line - h) / 2;
            if (dy > 0) { item->rc.top += dy; item->rc.bottom += dy; }
        }
        row.clear();
    };
    for (auto& item : items) {
        if (!item->visible) continue;
        float pw = item->pref_width(c);
        float iw = std::min(pw > 0 ? pw : w, w);
        if (cx > x && cx + iw > x + w) { settle(); cy += line + gap_y; cx = x; line = 0; }
        float h = item->layout(c, cx, cy, iw);
        // Kinder eines Elements (z. B. Combo hat keine) folgen der Verschiebung nicht; Elemente sind einstufig.
        line = std::max(line, h);
        row.push_back(item.get());
        cx += iw + gap_x;
    }
    settle();
    rc = {x, y, x + w, cy + line};
    return cy + line - y;
}
void Flow::children(std::vector<Widget*>& out) { for (auto& i : items) out.push_back(i.get()); }

void RailItem::paint(Ctx& c) {
    const float r = std::min(c.kit.design.button_radius, 14.f);
    if (open) {
        c.fill(rc, alpha(c.th.accent, c.th.dark ? .20f : .14f), r);
        c.fill({rc.left, rc.top + 10, rc.left + 3, rc.bottom - 10}, c.th.accent, 1.5f);
    }
    if (hot && enabled) c.fill(rc, alpha(c.th.text, c.th.dark ? .08f : .05f), r);
    const Rgba ink = c.th.text;
    const float cx = (rc.left + rc.right) / 2, top = rc.top + 8;
    const Rect g{cx - 10, top, cx + 10, top + 20};
    switch (glyph) {
    case Tiles:   // 2 x 2 Kacheln
        for (int i = 0; i < 4; ++i) {
            const float x = g.left + (i % 2) * 11, y = g.top + (i / 2) * 11;
            c.frame({x, y, x + 9, y + 9}, ink, 2, 1.6f);
        }
        break;
    case Layers:  // gestapelte Ebenen
        for (int i = 0; i < 3; ++i) c.frame({g.left + 1, g.top + 1.f + i * 6, g.right - 1, g.top + 8.f + i * 6}, ink, 2, 1.6f);
        break;
    case Pages:   // Blatt
        c.frame({g.left + 3, g.top, g.right - 3, g.bottom}, ink, 2, 1.6f);
        c.line({g.left + 6, g.top + 7}, {g.right - 6, g.top + 7}, ink, 1.4f);
        c.line({g.left + 6, g.top + 12}, {g.right - 6, g.top + 12}, ink, 1.4f);
        break;
    }
    c.text(c.s(text), {rc.left + 2, rc.top + 30, rc.right - 2, rc.bottom - 2}, Font::Small, c.th.text, DWRITE_TEXT_ALIGNMENT_CENTER);
}

float Panel::layout(Ctx& c, float x, float y, float w) {
    title_h_ = title.empty() ? 0 : c.text_height(c.s(title), w - 2 * pad_x, Font::Nav);
    float cy = y + pad_y + (title_h_ > 0 ? title_h_ + gap : 0);
    bool first = true;
    for (auto& item : items) {
        if (!item->visible) continue;
        if (!first) cy += gap;
        cy += item->layout(c, x + pad_x, cy, w - 2 * pad_x);
        first = false;
    }
    cy += pad_y;
    rc = {x, y, x + w, cy};
    return cy - y;
}
void Panel::paint(Ctx& c) {
    const float radius = c.kit.design.style == fl::theme::Style::FuchsLabs ? 12.f : c.kit.design.card_radius;
    c.fill(rc, c.th.card, radius);
    if (!c.kit.design.grouped && !c.kit.design.tonal) c.frame(rc, c.th.line, radius);
    if (title_h_ > 0)
        c.text(c.s(title), {rc.left + pad_x, rc.top + pad_y, rc.right - pad_x, rc.top + pad_y + title_h_}, Font::Nav, c.th.text,
               DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
}

EditorShell::EditorShell() {
    track_dirty = false;
    left.gap = 12;
    props.gap = 12;
    center.gap = 10;
    for (int i = 0; i < 3; ++i) section[i] = left.add<Panel>();
    const Text names[3] = {T(L"Kacheln", L"Tiles"), T(L"Ebenen", L"Layers"), T(L"Seiten", L"Pages")};
    const RailItem::Glyph glyphs[3] = {RailItem::Tiles, RailItem::Layers, RailItem::Pages};
    for (int i = 0; i < 3; ++i) {
        auto item = std::make_unique<RailItem>(names[i], glyphs[i], [this, i] {
            set_open(Section(i), !open_[i]);
            if (on_toggle) on_toggle();
        });
        item->name = names[i];
        rail[i] = item.get();
        rail_.push_back(std::move(item));
    }
    section[Palette]->title = names[0];
    section[Layers]->title = names[1];
    section[PageList]->title = names[2];
}

float EditorShell::layout(Ctx& c, float x, float y, float w) {
    mode = mode_for(w);
    if (!touched_[Palette]) open_[Palette] = mode != Narrow;
    bool any = false;
    for (int i = 0; i < 3; ++i) {
        section[i]->visible = open_[i];
        rail[i]->open = open_[i];
        any = any || open_[i];
    }
    const float gap = 12, rail_w = 64, left_w = 232, props_w = 252, rail_h = 56;
    float bottom = y;
    if (mode == Narrow) {
        const float rw_ = (w - 16) / 3;
        for (int i = 0; i < 3; ++i) rail[i]->layout(c, x + i * (rw_ + 8), y, rw_);
        rail_rc_ = {x, y, x + w, y + rail_h};
        float cy = y + rail_h + gap;
        left.visible = any;
        if (any) cy += left.layout(c, x, cy, w) + gap;
        cy += center.layout(c, x, cy, w) + gap;
        cy += props.layout(c, x, cy, w);
        rc = {x, y, x + w, cy};
        return cy - y;
    }
    float rail_y = y + 8;
    for (int i = 0; i < 3; ++i) { rail[i]->layout(c, x, rail_y, rail_w); rail_y += rail_h + 4; }
    rail_rc_ = {x, y, x + rail_w, rail_y + 4};
    float cx = x + rail_w + gap, hl = 0;
    left.visible = any;
    if (any) { hl = left.layout(c, cx, y, left_w); cx += left_w + gap; }
    const float avail = x + w - cx;
    bool side = mode == Wide || !any;
    float center_w = side ? avail - props_w - gap : avail;
    if (center_w < 220) { side = false; center_w = avail; }
    const float hc = center.layout(c, cx, y, center_w);
    bottom = y + std::max({hl, hc, rh(rail_rc_)});
    if (side) bottom = std::max(bottom, y + props.layout(c, cx + center_w + gap, y, props_w));
    else bottom = std::max(bottom, y + hc + gap + props.layout(c, cx, y + hc + gap, center_w));
    rc = {x, y, x + w, bottom};
    return bottom - y;
}
void EditorShell::paint(Ctx& c) {
    const float radius = c.kit.design.style == fl::theme::Style::FuchsLabs ? 12.f : c.kit.design.card_radius;
    c.fill(rail_rc_, c.th.card, radius);
    if (!c.kit.design.grouped && !c.kit.design.tonal) c.frame(rail_rc_, c.th.line, radius);
}
void EditorShell::children(std::vector<Widget*>& out) {
    for (auto& r : rail_) out.push_back(r.get());
    out.push_back(&left);
    out.push_back(&center);
    out.push_back(&props);
}

}  // namespace fl::ui
