// © 2026 FuchsLabs – Raster mit Tastatur und MSAA.
#include "ui_layout_grid.h"
#include "util.h"
#include <algorithm>
#include <oleacc.h>

namespace fl::ui {
using namespace plugins::edit;
GridEditor::GridEditor() { track_dirty = false; name = T(L"Anordnungsraster", L"Arrangement grid"); }
float GridEditor::height(Ctx&, float w) {
    auto l = geometry(model);
    return l ? std::max(32.f, w / float(l->columns.size())) * float(l->rows.size()) : 64.f;
}
int GridEditor::zone_at() const {
    auto l = geometry(model); if (!l) return -1;
    for (size_t i = 0; i < l->zones.size(); ++i) {
        const auto& z = l->zones[i];
        if (column >= z.column && column < z.column + z.column_span && row >= z.row && row < z.row + z.row_span) return int(i);
    }
    return -1;
}
void GridEditor::paint(Ctx& c) {
    auto l = geometry(model); if (!l) return;
    const float cw = std::max(32.f, (rc.right - rc.left) / float(l->columns.size()));
    const float ch = (rc.bottom - rc.top) / float(l->rows.size());
    const int visible_columns=std::max(1,int((rc.right-rc.left)/cw));
    if(column<first_column) first_column=column;
    if(column>=first_column+visible_columns) first_column=column-visible_columns+1;
    first_column=std::clamp(first_column,0,std::max(0,int(l->columns.size())-visible_columns));
    const float left=rc.left-first_column*cw;
    c.rt->PushAxisAlignedClip(rc,D2D1_ANTIALIAS_MODE_ALIASED);
    for (size_t y = 0; y < l->rows.size(); ++y) for (size_t x = 0; x < l->columns.size(); ++x)
        c.frame({left+x*cw, rc.top+y*ch, left+(x+1)*cw, rc.top+(y+1)*ch}, c.th.muted);
    for (size_t i = 0; i < l->zones.size(); ++i) {
        const auto& z = l->zones[i];
        Rect r{left+z.column*cw+3, rc.top+z.row*ch+3, left+(z.column+z.column_span)*cw-3, rc.top+(z.row+z.row_span)*ch-3};
        c.fill(r, c.th.card);
        c.frame(r, c.th.text, 2, int(i) == selected ? 2.f : 1.f);
        c.text(widen(z.id), {r.left+5,r.top,r.right-5,r.bottom}, Font::Body, c.th.text);
    }
    c.frame({left+column*cw+5, rc.top+row*ch+5, left+(column+1)*cw-5, rc.top+(row+1)*ch-5}, c.th.text, 0, 2);
    c.rt->PopAxisAlignedClip();
}
std::wstring GridEditor::value(bool en) const {
    auto l = geometry(model);
    if (!l || selected < 0 || selected >= int(l->zones.size()))
        return (en ? L"Free cell, column " : L"Freie Zelle, Spalte ") + std::to_wstring(column+1) + (en ? L", row " : L", Zeile ") + std::to_wstring(row+1);
    const auto& z = l->zones[selected]; int blocks = 0;
    for (auto& b : model.blocks) if (b["zone"] == z.id && b.value("shown", true)) ++blocks;
    auto range = [](int p, int n) { return std::to_wstring(p+1) + (n > 1 ? L"–" + std::to_wstring(p+n) : L""); };
    auto value=L"Zone " + widen(z.id) + (en ? L", column " : L", Spalte ") + range(z.column,z.column_span) +
        (en ? L", row " : L", Zeile ") + range(z.row,z.row_span) + L", " +
        (z.cycle ? widen(z.cycle->mode) : (en ? L"no cycle" : L"kein Wechsel")) + L", " + std::to_wstring(blocks) + (en ? L" blocks" : L" Blöcke");
    if(column!=z.column || row!=z.row) value+=(en?L", cell focus column ":L", Zellfokus Spalte ")+std::to_wstring(column+1)+(en?L", row ":L", Zeile ")+std::to_wstring(row+1);
    return value;
}
bool GridEditor::input(UINT vk, bool shift, bool alt, bool control) {
    if (control && (vk == 'Z' || vk == 'Y')) {
        auto f = vk == 'Y' || shift ? on_redo : on_undo; if (f) f(); return true;
    }
    auto l = geometry(model); if (!l) return false;
    column = std::clamp(column, 0, int(l->columns.size())-1); row = std::clamp(row, 0, int(l->rows.size())-1);
    int dx = vk == VK_LEFT ? -1 : vk == VK_RIGHT ? 1 : 0;
    int dy = vk == VK_UP ? -1 : vk == VK_DOWN ? 1 : 0;
    auto next = model; bool ok = false; std::string group;
    if (dx || dy) {
        if (!shift && !alt) {
            column = std::clamp(column+dx,0,int(l->columns.size())-1); row = std::clamp(row+dy,0,int(l->rows.size())-1);
            return true; // Zellfokus und ausgewählte Zone sind getrennt; Enter/Leertaste wählt.
        }
        group = (alt ? "move:" : "size:") + std::to_string(selected);
        ok = alt ? move_zone(next, selected, dx, dy) : resize_zone(next, selected, dx, dy);
    } else if (vk == VK_RETURN || vk == VK_SPACE) {
        selected = zone_at();
        if (selected >= 0) { if (on_select) on_select(); return true; }
        ok = add_zone(next, column, row); if (ok) selected = int(next.layout["zones"].size())-1;
    } else if (vk == VK_DELETE) {
        ok = delete_zone(next, selected); if (ok) selected = 0;
    } else return false;
    if (ok) { model = next; if (on_edit) on_edit(std::move(next),group); }
    else if (on_reject) on_reject();
    return true;
}
bool GridEditor::key(UINT vk, Surface& s) {
    if (!input(vk, GetKeyState(VK_SHIFT)<0, GetKeyState(VK_MENU)<0, GetKeyState(VK_CONTROL)<0)) return false;
    s.notify(this, EVENT_OBJECT_VALUECHANGE); s.invalidate(); return true;
}
void GridEditor::activate(Surface& s) { key(VK_RETURN,s); }
void GridEditor::release(Point p, Surface& s) {
    auto l = geometry(model); if (!l) return;
    column = std::clamp(first_column+int((p.x-rc.left)/std::max(32.f,(rc.right-rc.left)/l->columns.size())),0,int(l->columns.size())-1);
    row = std::clamp(int((p.y-rc.top)/((rc.bottom-rc.top)/l->rows.size())),0,int(l->rows.size())-1);
    key(VK_RETURN,s);
}
bool LayoutBlockList::key(UINT vk, Surface& s) {
    if (GetKeyState(VK_CONTROL)<0 && (vk=='Z' || vk=='Y')) {
        auto f = vk=='Y' || GetKeyState(VK_SHIFT)<0 ? on_redo : on_undo; if(f) f(); return true;
    }
    history_group=(vk==VK_UP || vk==VK_DOWN) && (GetKeyState(VK_MENU)<0 || GetKeyState(VK_CONTROL)<0) && selected>=0 && selected<int(items.size())
        ? "block:"+items[selected].key : "";
    bool handled=ItemList::key(vk,s); history_group.clear(); return handled;
}
void LayoutBlockList::paint(Ctx& c) {
    // Kontrastreiche Auswahl und Haken nur im Editor; die globale Listenpalette bleibt unverändert.
    auto th=c.th; th.accent=th.text; th.on_accent=th.card; th.muted=th.text;
    Ctx local{c.rt,c.kit,th,c.brush,c.device,c.dim}; ItemList::paint(local);
}
}
