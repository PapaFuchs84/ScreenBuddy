// © 2026 FuchsLabs – Kachelpalette des Anordnungs-Editors (Phase 7b P7b-2). Clean-Room.
#include "ui_tile_palette.h"
#include "render_game.h"
#include <algorithm>
#include <cmath>
#include <cwctype>

namespace fl::ui {
namespace {
constexpr float kMinTile = 92, kGap = 8, kLabelH = 36;
float rw(const Rect& r) { return r.right - r.left; }
float rh(const Rect& r) { return r.bottom - r.top; }
Rect inflate(Rect r, float d) { return {r.left - d, r.top - d, r.right + d, r.bottom + d}; }
Rgba alpha(Rgba c, float a) { c.a = a; return c; }
bool inside(const Rect& r, Point p) { return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom; }
std::wstring lower(std::wstring s) { for (auto& c : s) c = wchar_t(std::towlower(c)); return s; }
}

TilePalette::TilePalette() {
    track_dirty = false;
    name = T(L"Kachelpalette", L"Tile palette");
}

std::string TilePalette::source_category(const std::string& source,const std::string& block) {
    if(source=="system" && block!="system.cpu" && block!="system.gpu" && game::metric_tile(block))return "game";
    return source.starts_with("plugin:")?"plugin:":source;
}

bool TilePalette::matches(const Tile& tile, const std::string& cat, const std::wstring& q, bool en) {
    if (!cat.empty() && tile.category != cat) return false;
    if (q.empty()) return true;
    return lower(tile.label.get(en)).find(q) != std::wstring::npos || lower(tile.label.get(!en)).find(q) != std::wstring::npos ||
           lower(std::wstring(tile.id.begin(), tile.id.end())).find(q) != std::wstring::npos;
}
std::vector<size_t> TilePalette::visible() const {
    std::vector<size_t> out;
    const bool en = surface && surface->kit.english;
    for (size_t i = 0; i < tiles.size(); ++i) if (matches(tiles[i], category, query, en)) out.push_back(i);
    return out;
}
int TilePalette::columns() const {
    const float w = rw(rc);
    return std::max(1, int((w + kGap) / (kMinTile + kGap)));
}
float TilePalette::height(Ctx& c, float w) {
    (void)c;
    const int cols = std::max(1, int((w + kGap) / (kMinTile + kGap)));
    const float tw = (w - (cols - 1) * kGap) / cols, th = tw * 9.f / 16.f + kLabelH + 6;
    const size_t n = visible().size();
    if (n == 0) return 48;
    const size_t rows = (n + cols - 1) / cols;
    return float(rows) * th + float(rows - 1) * kGap;
}
Rect TilePalette::tile_rect(size_t position) const {
    const int cols = columns();
    const float w = rw(rc), tw = (w - (cols - 1) * kGap) / cols, th = tw * 9.f / 16.f + kLabelH + 6;
    const float x = rc.left + float(position % cols) * (tw + kGap), y = rc.top + float(position / cols) * (th + kGap);
    return {x, y, x + tw, y + th};
}
int TilePalette::tile_at(Point p) const {
    const auto v = visible();
    for (size_t i = 0; i < v.size(); ++i) if (inside(tile_rect(i), p)) return int(i);
    return -1;
}
LPCWSTR TilePalette::cursor_for(Point p) const {
    if (grid && grid->carrying()) return grid->target_valid() ? IDC_SIZEALL : IDC_NO;
    return tile_at(p) >= 0 ? IDC_HAND : IDC_ARROW;
}
std::wstring TilePalette::tip(Point p, bool en) const {
    const auto v = visible(); int at = tile_at(p);
    if (at < 0) return {};
    const auto& t = tiles[v[size_t(at)]];
    return t.label.get(en) + (en ? L" · drag to the canvas or click to add" : L" · auf die Fläche ziehen oder klicken");
}
// MSAA-Wert: zuerst die Kachel am Cursor (Zählung, Name, Zustand, Bedienung), danach die ganze Liste.
std::wstring TilePalette::value(bool en) const {
    const auto v = visible();
    std::wstring out;
    if (v.empty()) out = en ? L"No tile matches. " : L"Keine Kachel passt. ";
    else {
        const size_t at = size_t(std::clamp(current, 0, int(v.size()) - 1));
        const auto& t = tiles[v[at]];
        out = (en ? L"Tile " : L"Kachel ") + std::to_wstring(at + 1) + (en ? L" of " : L" von ") + std::to_wstring(v.size()) + L": " + t.label.get(en) +
              (t.placed ? (en ? L", placed" : L", platziert") : (en ? L", free" : L", frei")) +
              (en ? L". Space: pick up, Enter: select or add. " : L". Leertaste: aufnehmen, Enter: wählen oder hinzufügen. ");
    }
    if (v.size() != tiles.size()) out += (en ? L"Filtered: " : L"Gefiltert: ") + std::to_wstring(v.size()) + (en ? L" of " : L" von ") + std::to_wstring(tiles.size()) + L". ";
    out += en ? L"List: " : L"Liste: ";
    bool first = true;
    for (const auto& t : tiles) {
        if (!first) out += L", ";
        first = false;
        out += t.label.get(en);
        out += t.placed ? (en ? L" shown" : L" gezeigt") : (en ? L" hidden" : L" verborgen");
    }
    return out;
}

void TilePalette::paint(Ctx& c) {
    if (c.device != device_) { bitmaps_.clear(); made_.clear(); device_ = c.device; }
    const auto v = visible();
    if (v.empty()) {
        c.text(c.kit.english ? L"No tiles match." : L"Keine Kachel passt zur Suche.", rc, Font::Small, c.th.muted, DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }
    const bool focused = surface && surface->focused() == this;
    const bool carrying = grid && grid->carrying() && grid->area == 0;
    for (size_t pos = 0; pos < v.size(); ++pos) {
        const auto& t = tiles[v[pos]];
        const Rect r = tile_rect(pos);
        const bool source = carrying && grid->palette_block == int(v[pos]);
        c.fill(r, alpha(c.th.text, c.th.dark ? .07f : .05f), 8);
        c.frame(r, c.th.line, 8);
        const float tw = rw(r), thumb_h = tw * 9.f / 16.f;
        const Rect thumb{r.left + 4, r.top + 4, r.right - 4, r.top + 4 + thumb_h - 4};
        c.fill(thumb, c.th.preview_bg, 5);
        const Thumb* th = thumb_of ? thumb_of(t.id) : nullptr;
        if (th && th->image && !th->image->empty() && !th->no_data) {
            auto& bitmap = bitmaps_[t.id];
            if (!bitmap || made_[t.id] != th->version) {
                bitmap.Reset();
                auto props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
                c.rt->CreateBitmap(D2D1::SizeU(UINT32(th->image->w), UINT32(th->image->h)), th->image->px.data(), UINT32(th->image->w * 4), props, bitmap.GetAddressOf());
                made_[t.id] = th->version;
            }
            if (bitmap) {
                const float s = std::min(rw(thumb) / th->image->w, rh(thumb) / th->image->h);
                const float w = th->image->w * s, h = th->image->h * s;
                const Rect dst{thumb.left + (rw(thumb) - w) / 2, thumb.top + (rh(thumb) - h) / 2, thumb.left + (rw(thumb) - w) / 2 + w, thumb.top + (rh(thumb) - h) / 2 + h};
                c.rt->DrawBitmap(bitmap.Get(), dst, c.dim, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            }
        } else if (th && th->no_data) {
            c.text(c.kit.english ? L"no data" : L"keine Daten", inflate(thumb, -3), Font::Small, rgb(235, 235, 235), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else if (th) {  // Eintrag im Cache, aber ohne Bild: Renderer lieferte nichts (nicht „lädt noch“)
            c.text(c.kit.english ? L"no preview" : L"keine Vorschau", inflate(thumb, -3), Font::Small, rgb(235, 235, 235), DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            c.text(L"…", thumb, Font::Small, rgb(235, 235, 235), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        const Rect label{r.left + 6, thumb.bottom + 3, r.right - 6, r.bottom - 2};
        c.text(c.s(t.label), {label.left, label.top, label.right, label.top + 18}, Font::Small, c.th.text);
        const std::wstring state = (t.required ? (c.kit.english ? L"required · " : L"Pflicht · ") : L"") +
            std::wstring(t.placed ? (c.kit.english ? L"placed" : L"platziert") : (c.kit.english ? L"free" : L"frei"));
        c.text(state, {label.left, label.top + 17, label.right, label.bottom}, Font::Small, c.th.muted);
        if (t.placed) c.frame(r, alpha(c.th.accent, .8f), 8, 1.5f);
        if (source) c.fill(r, alpha(c.th.bg, .55f), 8);
        if (focused && int(pos) == current) c.frame(inflate(r, 1), focus_color(c.th), 8, 2.5f);
    }
}

void TilePalette::click(size_t position, Surface& s) {
    const auto v = visible();
    if (!grid || position >= v.size()) return;
    const size_t block = v[position];
    if (block >= grid->model.blocks.size()) return;
    if (grid->shown_block(block)) grid->select_block(block, s);
    else grid->insert_first_free(block, s);
    s.invalidate();
}
void TilePalette::activate(Surface& s) { click(size_t(std::max(0, current)), s); }

bool TilePalette::key(UINT vk, Surface& s) {
    const auto v = visible();
    if (v.empty()) return false;
    current = std::clamp(current, 0, int(v.size()) - 1);
    const int cols = columns();
    int next = current;
    switch (vk) {
    case VK_LEFT: next = current - 1; break;
    case VK_RIGHT: next = current + 1; break;
    case VK_UP: next = current - cols; break;
    case VK_DOWN: next = current + cols; break;
    case VK_HOME: next = 0; break;
    case VK_END: next = int(v.size()) - 1; break;
    case VK_TAB:  // nur Strg+Tab wie F6; Tab allein gehört der Surface
        if (GetKeyState(VK_CONTROL) >= 0) return false;
        [[fallthrough]];
    case VK_F6:   // Bereichswechsel wie im Raster: weiter zur Fläche bzw. (Umschalt) zum Papierkorb
        if(!grid) return false;
        grid->input(vk, GetKeyState(VK_SHIFT) < 0, false, vk == VK_TAB);
        if(grid->area != 0) { s.focus(grid, true); s.notify(grid, EVENT_OBJECT_VALUECHANGE); }
        s.invalidate(); return true;
    case VK_RETURN: click(size_t(current), s); return true;
    case VK_SPACE:
        if (grid) { s.focus(grid, true); grid->pickup(v[size_t(current)], s); }
        return true;
    case VK_DELETE:
        if (grid && grid->shown_block(v[size_t(current)])) {
            const int saved = grid->area; grid->palette_block = int(v[size_t(current)]); grid->area = 0;
            grid->input(VK_DELETE); grid->area = saved; s.invalidate();
        }
        return true;
    default: return false;
    }
    current = std::clamp(next, 0, int(v.size()) - 1);
    if (grid) grid->palette_block = int(v[size_t(current)]);
    if (surface) surface->ensure_visible(this);
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.invalidate();
    return true;
}

void TilePalette::press(Point p, Surface& s) {
    threshold_ = GetSystemMetrics(SM_CXDRAG) * 96.f / s.dpi();
    pressed_ = tile_at(p);
    origin_ = p;
    if (pressed_ < 0) return;
    current = pressed_;
    const auto v = visible();
    if (grid) { grid->palette_block = int(v[size_t(pressed_)]); grid->start_external(v[size_t(pressed_)], p, s); }
    s.invalidate();
}
void TilePalette::drag(Point p, Surface& s) {
    if (pressed_ >= 0 && grid) grid->drag(p, s);
}
void TilePalette::release(Point p, Surface& s) {
    if (pressed_ < 0) return;
    const int pos = pressed_;
    pressed_ = -1;
    const bool carried = grid && grid->carrying();
    if (grid) grid->release(p, s);
    const bool moved = std::abs(p.x - origin_.x) >= threshold_ || std::abs(p.y - origin_.y) >= threshold_;
    if (!carried && !moved) click(size_t(pos), s);
    s.invalidate();
}
void TilePalette::cancel(Surface& s) {
    if (pressed_ >= 0 && grid) grid->cancel(s);
    pressed_ = -1;
}

}  // namespace fl::ui
