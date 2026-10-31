// © 2026 FuchsLabs
// ui_kit: Zeichnen, Layout, Fokus, Bildlauf und Barrierefreiheit des nativen Einstellungsfensters (siehe ui_kit.h).
#include "ui_kit.h"

#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <initguid.h>  // CLSID_AccPropServices, PROPID_ACC_NAME
#include <oleacc.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <sstream>

#include "brand.h"
#include "native_settings.h"
#include "theme.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {

// ---------------------------------------------------------------- Farben
Rgba rgb(uint8_t r, uint8_t g, uint8_t b, float a) { return {r / 255.f, g / 255.f, b / 255.f, a}; }
Rgba from(const Color& c, float a) { return rgb(c.r, c.g, c.b, a); }
static Rgba alpha(Rgba c, float a) { c.a *= a; return c; }
static COLORREF colorref(Rgba c) { return RGB(int(c.r * 255 + .5f), int(c.g * 255 + .5f), int(c.b * 255 + .5f)); }

const Theme& Theme::get(bool dark) {
    static const Theme light = [] {
        Theme t; t.dark = false;
        t.bg = from(brand::CREME); t.card = from(brand::SAND); t.line = from(brand::LINE); t.text = from(brand::NAVY);
        t.muted = from(brand::NEBEL); t.side = from(brand::ORANGE); t.side_text = from(brand::NAVY);
        t.side_muted = from(brand::NAVY, .62f); t.nav_active = from(brand::CREME); t.input = from(brand::WHITE);
        t.accent = from(brand::ORANGE); t.accent_text = from(brand::GLUT); t.on_accent = from(brand::NAVY);
        t.header = from(brand::ORANGE); t.header_text = from(brand::NAVY); t.success = rgb(31, 160, 85);
        t.danger = rgb(178, 34, 34); t.preview_bg = from(brand::NAVY_TIEF);
        return t;
    }();
    static const Theme night = [] {
        Theme t; t.dark = true;
        t.bg = from(brand::NAVY_TIEF); t.card = from(brand::NAVY); t.line = from(brand::NAVY_LINIE); t.text = from(brand::CREME);
        t.muted = rgb(169, 179, 200); t.side = rgb(10, 17, 32); t.side_text = from(brand::CREME);
        t.side_muted = rgb(169, 179, 200); t.nav_active = from(brand::NAVY); t.input = from(brand::NAVY_TIEF);
        t.accent = from(brand::ORANGE); t.accent_text = from(brand::ORANGE); t.on_accent = from(brand::NAVY);
        t.header = from(brand::ORANGE); t.header_text = from(brand::NAVY); t.success = rgb(46, 204, 113);
        t.danger = rgb(255, 120, 110); t.preview_bg = from(brand::NAVY_TIEF);
        return t;
    }();
    return dark ? night : light;
}

void Kit::use_design(const ojson& display) {
    auto resolved = fl::theme::resolve(display.value("theme", std::string("hell")), fl::theme::Store());
    design = fl::theme::design(resolved.style);
    designed_ = resolved.style != fl::theme::Style::FuchsLabs;
    for (auto& f : formats_) f.Reset();
    for (auto& e : ellipsis_) e.Reset();
    if (!designed_) return;
    for (int mode = 0; mode < 2; ++mode) {
    const auto p = fl::theme::correct_contrast(bool(mode) == resolved.dark ? resolved.colors : fl::theme::profile_palette(resolved.style, mode != 0));
    design_theme_[mode] = Theme::get(mode != 0);
    auto& t = design_theme_[mode];
    t.bg = from(p.bg); t.card = from(p.card); t.input = from(p.bg); t.line = from(p.line);
    t.text = t.side_text = from(p.text); t.muted = t.side_muted = from(p.muted);
    t.side = from(p.bg); t.nav_active = from(p.card);
    t.accent = t.accent_text = from(p.accent); t.on_accent = from(fl::theme::readable_on(p.accent));
    t.header = from(p.bg); t.header_text = from(p.text);
    t.success = from(p.success); t.danger = from(p.danger); t.dark = mode != 0;
    }
}

void Kit::use_windows(bool on) {
    windows_ = on;
    if (!on) return;
    const auto w = fl::theme::read_windows_settings();
    const auto p = fl::theme::windows_palette(w);
    dark = !w.light;
    win_theme_ = designed_ ? design_theme_[dark ? 1 : 0] : Theme::get(dark);   // Profil: Profilpalette statt FuchsLabs-Orange
    win_theme_.accent = from(w.high_contrast ? p.accent : (w.has_accent ? w.accent : p.accent));
    win_theme_.on_accent = from(fl::theme::readable_on(w.high_contrast ? p.accent : (w.has_accent ? w.accent : p.accent)));
    win_theme_.accent_text = from(p.accent);   // Akzent als Schrift: kontrastkorrigiert
}

bool system_prefers_dark() {
    DWORD value = 1, size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return false;
    return value == 0;
}

// ---------------------------------------------------------------- Kit
Kit::Kit() {
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_.GetAddressOf())))
        throw std::runtime_error("Native settings: D2D1CreateFactory");
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(dw_.GetAddressOf()))))
        throw std::runtime_error("Native settings: DWriteCreateFactory");
}
Kit::~Kit() = default;

IDWriteTextFormat* Kit::format(Font f) {
    auto& slot = formats_[int(f)];
    if (slot) return slot.Get();
    struct Spec { const wchar_t* family; DWRITE_FONT_WEIGHT weight; float size; };
    static const Spec specs[] = {
        {L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, 14},      // Body
        {L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, 12},      // Small
        {L"Segoe UI", DWRITE_FONT_WEIGHT_SEMI_BOLD, 14},   // Strong
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_BOLD, 26},     // H1
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_SEMI_BOLD, 11},// Section
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_SEMI_BOLD, 14},// Nav
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_BOLD, 22},     // Tile
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_BOLD, 14},     // Button
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_BOLD, 20},     // Brand
        {L"Bahnschrift", DWRITE_FONT_WEIGHT_SEMI_BOLD, 12},// Label
    };
    const auto& s = specs[int(f)];
    float size = s.size;
    if (design.style == fl::theme::Style::Cupertino && f == Font::H1) size = 24;
    if (design.style == fl::theme::Style::Fluent && f == Font::H1) size = 28;
    if (design.style == fl::theme::Style::Material) {
        if (f == Font::H1) size = 34;
        if (f == Font::Nav || f == Font::Strong) size = 16;
        if (f == Font::Tile) size = 32;
    }
    if (FAILED(dw_->CreateTextFormat(design.style == fl::theme::Style::FuchsLabs ? s.family : design.style == fl::theme::Style::Fluent ? L"Segoe UI Variable" : L"Segoe UI", nullptr, s.weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                     size, L"de-de", slot.GetAddressOf())))
        throw std::runtime_error("Native settings: CreateTextFormat");
    return slot.Get();
}

IDWriteInlineObject* Kit::ellipsis(Font f) {
    auto& slot = ellipsis_[int(f)];
    if (!slot) dw_->CreateEllipsisTrimmingSign(format(f), slot.GetAddressOf());
    return slot.Get();
}

ID2D1StrokeStyle* Kit::dashed() {
    if (!dashed_) {
        auto props = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
                                                 D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_DASH, 0.f);
        d2d_->CreateStrokeStyle(props, nullptr, 0, dashed_.GetAddressOf());
    }
    return dashed_.Get();
}

// ---------------------------------------------------------------- Zeichnen
static Rect inflate(Rect r, float d) { return {r.left - d, r.top - d, r.right + d, r.bottom + d}; }
static bool inside(const Rect& r, Point p) { return p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom; }
static float rw(const Rect& r) { return r.right - r.left; }
static float rh(const Rect& r) { return r.bottom - r.top; }

void Ctx::fill(const Rect& r, Rgba c, float radius) const {
    brush->SetColor(c); brush->SetOpacity(dim);
    if (radius > 0) rt->FillRoundedRectangle(D2D1::RoundedRect(r, radius, radius), brush);
    else rt->FillRectangle(r, brush);
}
void Ctx::frame(const Rect& r, Rgba c, float radius, float width) const {
    brush->SetColor(c); brush->SetOpacity(dim);
    Rect i = inflate(r, -width / 2);
    if (radius > 0) rt->DrawRoundedRectangle(D2D1::RoundedRect(i, radius, radius), brush, width);
    else rt->DrawRectangle(i, brush, width);
}
void Ctx::line(Point a, Point b, Rgba c, float width) const {
    brush->SetColor(c); brush->SetOpacity(dim);
    rt->DrawLine(a, b, brush, width);
}
void Ctx::circle(Point c, float radius, Rgba color) const {
    brush->SetColor(color); brush->SetOpacity(dim);
    rt->FillEllipse(D2D1::Ellipse(c, radius, radius), brush);
}

static ComPtr<IDWriteTextLayout> make_layout(Kit& kit, const std::wstring& t, Font f, float w, float h, bool wrap) {
    ComPtr<IDWriteTextLayout> layout;
    if (FAILED(kit.dwrite()->CreateTextLayout(t.c_str(), UINT32(t.size()), kit.format(f), std::max(1.f, w), std::max(1.f, h),
                                              layout.GetAddressOf()))) return {};
    layout->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
    if (!wrap) {
        DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
        layout->SetTrimming(&trimming, kit.ellipsis(f));
    }
    return layout;
}

void Ctx::text(const std::wstring& t, const Rect& r, Font f, Rgba c, DWRITE_TEXT_ALIGNMENT align, bool wrap, bool vcenter) const {
    if (t.empty()) return;
    auto layout = make_layout(kit, t, f, rw(r), wrap ? 100000.f : rh(r), wrap);
    if (!layout) return;
    layout->SetTextAlignment(align);
    float y = r.top;
    if (vcenter) {
        DWRITE_TEXT_METRICS m{};
        layout->GetMetrics(&m);
        y = r.top + (rh(r) - m.height) / 2;
    }
    brush->SetColor(c); brush->SetOpacity(dim);
    rt->DrawTextLayout({r.left, y}, layout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
}
float Ctx::text_height(const std::wstring& t, float width, Font f) const {
    if (t.empty()) return 0;
    auto layout = make_layout(kit, t, f, width, 100000.f, true);
    DWRITE_TEXT_METRICS m{};
    if (layout) layout->GetMetrics(&m);
    return std::ceil(m.height);
}
float Ctx::text_width(const std::wstring& t, Font f) const {
    if (t.empty()) return 0;
    auto layout = make_layout(kit, t, f, 100000.f, 1000.f, false);
    DWRITE_TEXT_METRICS m{};
    if (layout) layout->GetMetrics(&m);
    return std::ceil(m.widthIncludingTrailingWhitespace);
}

// ---------------------------------------------------------------- Entsorgung
static thread_local std::vector<WidgetPtr> retired;
void retire(std::vector<WidgetPtr>& items) {
    for (auto& w : items) retired.push_back(std::move(w));
    items.clear();
}
void flush_retired() {
    auto old = std::move(retired);
    retired.clear();
    old.clear();
}

// ---------------------------------------------------------------- Container
float Widget::layout(Ctx& c, float x, float y, float w) {
    float h = height(c, w);
    rc = {x, y, x + w, y + h};
    return h;
}

void Column::clear() { retire(items); }
float Column::layout(Ctx& c, float x, float y, float w) {
    float cy = y + pad_y;
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
void Column::paint(Ctx&) {}
void Column::children(std::vector<Widget*>& out) { for (auto& i : items) out.push_back(i.get()); }

Row::Row(Text l, Text d, WidgetPtr c, bool w) : label(std::move(l)), desc(std::move(d)), control(std::move(c)), wide(w) {
    if (control && control->name.empty()) control->name = label;
    if (auto l = dynamic_cast<Label*>(control.get())) l->align_right = true;
}
float Row::layout(Ctx& c, float x, float y, float w) {
    const float pad = 11;
    float cw = control ? std::min(control->pref_width(c), w) : 0;
    bool stack = wide || (control && w - cw - 24 < 200);
    float label_w = stack || !control ? w : w - cw - 24;
    float lh = c.text_height(c.s(label), label_w, Font::Body);
    float dh = desc.empty() ? 0 : c.text_height(c.s(desc), label_w, Font::Small);
    float text_h = lh + (dh > 0 ? 2 + dh : 0);
    float total;
    if (!control) total = text_h + 2 * pad;
    else if (stack) {
        float ch = control->layout(c, x, y + pad + text_h + 8, wide ? w : std::min(std::max(cw, 1.f), w));
        total = pad + text_h + 8 + ch + pad;
    } else {
        float ch = control->height(c, cw);
        total = std::max(text_h, ch) + 2 * pad;
        control->layout(c, x + w - cw, y + (total - ch) / 2, cw);
    }
    rc = {x, y, x + w, y + total};
    float ty = stack || !control ? y + pad : y + (total - text_h) / 2;
    label_rc = {x, ty, x + label_w, ty + lh};
    desc_rc = {x, ty + lh + 2, x + label_w, ty + lh + 2 + dh};
    return total;
}
void Row::paint(Ctx& c) {
    c.text(c.s(label), label_rc, Font::Body, c.th.text, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
    if (!desc.empty()) c.text(c.s(desc), desc_rc, Font::Small, c.th.muted, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
}
void Row::children(std::vector<Widget*>& out) { if (control) out.push_back(control.get()); }
bool Row::clickable() const { return control && dynamic_cast<Toggle*>(control.get()) && control->enabled; }
void Row::press(Point, Surface& s) {
    if (auto t = dynamic_cast<Toggle*>(control.get()); t && t->enabled) { s.focus(t, false); t->activate(s); }
}

float Card::layout(Ctx& c, float x, float y, float w) {
    auto& g = *this;
    g.dividers.clear();
    float inner = w - 2 * pad_x;
    float cy = y + pad_y;
    g.title_h = title.empty() ? 0 : c.text_height(c.s(title), inner - (placeholder ? 90 : 0), Font::Nav);
    g.desc_h = desc.empty() ? 0 : c.text_height(c.s(desc), inner, Font::Small);
    if (g.title_h > 0 || g.desc_h > 0) cy += 10 + g.title_h + (g.desc_h > 0 ? 3 + g.desc_h : 0) + 4;
    bool first = true;
    for (auto& item : items) {
        if (!item->visible) continue;
        bool row = dynamic_cast<Row*>(item.get()) != nullptr;
        if (!first || g.title_h > 0 || g.desc_h > 0) g.dividers.push_back(cy);
        float pad = row ? 0.f : c.kit.design.spacing - 2;
        cy += pad;
        cy += item->layout(c, x + pad_x, cy, inner);
        cy += pad;
        first = false;
    }
    if (first && (g.title_h > 0 || g.desc_h > 0)) cy += 6;
    cy += pad_y;
    rc = {x, y, x + w, cy};
    return cy - y;
}
void Card::paint(Ctx& c) {
    auto& g = *this;
    c.fill(rc, c.th.card, c.kit.design.style == fl::theme::Style::FuchsLabs ? 12.f : c.kit.design.card_radius);
    if (placeholder) {
        c.brush->SetColor(c.th.accent); c.brush->SetOpacity(c.dim);
        c.rt->DrawRoundedRectangle(D2D1::RoundedRect(inflate(rc, -1), 12, 12), c.brush, 1.5f, c.kit.dashed());
        const std::wstring badge = c.kit.english ? L"COMING" : L"FOLGT";
        float bw = c.text_width(badge, Font::Label) + 18;
        Rect b{rc.right - pad_x - bw, rc.top + 10, rc.right - pad_x, rc.top + 30};
        c.fill(b, c.th.accent, 10);
        c.text(badge, b, Font::Label, c.th.on_accent, DWRITE_TEXT_ALIGNMENT_CENTER);
    } else if (!c.kit.design.grouped && !c.kit.design.tonal)
        c.frame(rc, c.th.line, c.kit.design.style == fl::theme::Style::FuchsLabs ? 12.f : c.kit.design.card_radius);
    float x = rc.left + pad_x, y = rc.top + pad_y + 10;
    if (g.title_h > 0) {
        c.text(c.s(title), {x, y, rc.right - pad_x - (placeholder ? 90.f : 0.f), y + g.title_h}, Font::Nav, c.th.text,
               DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
        y += g.title_h + 3;
    }
    if (g.desc_h > 0) c.text(c.s(desc), {x, y, rc.right - pad_x, y + g.desc_h}, Font::Small, c.th.muted, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
    if (!c.kit.design.tonal) for (float d : g.dividers) c.line({rc.left + pad_x, d}, {rc.right - pad_x, d}, c.th.line);
}

// ---------------------------------------------------------------- Anzeige
float Heading::height(Ctx& c, float w) {
    return c.text_height(c.s(title), w, Font::H1) + (subtitle.empty() ? 0 : 4 + c.text_height(c.s(subtitle), w, Font::Body));
}
void Heading::paint(Ctx& c) {
    float th = c.text_height(c.s(title), rw(rc), Font::H1);
    c.text(c.s(title), {rc.left, rc.top, rc.right, rc.top + th}, Font::H1, c.th.text, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
    if (!subtitle.empty())
        c.text(c.s(subtitle), {rc.left, rc.top + th + 4, rc.right, rc.bottom}, Font::Body, c.th.muted, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
}

static Rgba tone_color(const Ctx& c, Label::Tone t) {
    switch (t) {
    case Label::Muted: return c.th.muted;
    case Label::Accent: return c.th.accent_text;
    case Label::Danger: return c.th.danger;
    case Label::Success: return c.th.success;
    default: return c.th.text;
    }
}
float Label::height(Ctx& c, float w) {
    if (!wrap) return c.text_height(L"Ag", w, font);
    return std::max(c.text_height(c.s(text), w, font), text.empty() ? 0.f : 1.f);
}
void Label::paint(Ctx& c) {
    c.text(c.s(text), rc, font, tone_color(c, tone), align_right ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING, wrap, !wrap);
}

float Note::height(Ctx& c, float w) { return c.text_height(c.s(text), w - 30, Font::Small) + 20; }
void Note::paint(Ctx& c) {
    Rgba bar = kind == Warn ? c.th.danger : c.th.accent;
    c.fill(rc, alpha(c.th.accent, c.th.dark ? .10f : .12f), 8);
    c.fill({rc.left, rc.top, rc.left + 4, rc.bottom}, bar, 2);
    c.text(c.s(text), {rc.left + 16, rc.top + 10, rc.right - 12, rc.bottom - 10}, Font::Small, c.th.text,
           DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
}

void Tiles::paint(Ctx& c) {
    if (tiles.empty()) return;
    if (c.kit.design.tonal && tiles.size() > 1) {
        const float hero_right = rc.left + rw(rc) * .52f;
        const Rect hero{rc.left, rc.top, hero_right, rc.bottom};
        c.fill(hero, c.th.card, 28);
        c.text(c.s(tiles[0].label), {hero.left + 20,hero.top + 20,hero.right - 20,hero.top + 46}, Font::Body, c.th.muted);
        c.text(c.s(tiles[0].value), {hero.left + 20,hero.top + 58,hero.right - 20,hero.bottom - 24}, Font::Tile, c.th.accent_text);
        const float rh_ = (rh(rc) - 8 * float(tiles.size() - 2)) / float(tiles.size() - 1);
        for (size_t i = 1; i < tiles.size(); ++i) {
            const float y = rc.top + float(i - 1) * (rh_ + 8);
            Rect r{hero_right + 12,y,rc.right,y + rh_}; c.fill(r, c.th.card, 16);
            c.text(c.s(tiles[i].label), {r.left + 14,r.top + 5,r.right - 12,r.top + 24}, Font::Small, c.th.muted);
            c.text(c.s(tiles[i].value), {r.left + 14,r.top + 22,r.right - 12,r.bottom - 4}, Font::Strong, c.th.text);
        }
        return;
    }
    const float gap = 10, n = float(tiles.size());
    const bool grouped = c.kit.design.grouped;
    if (grouped) c.fill(rc, c.th.card, 12);
    float tw = (rw(rc) - (grouped ? 0 : gap) * (n - 1)) / n;
    for (size_t i = 0; i < tiles.size(); ++i) {
        Rect r{rc.left + i * (tw + (grouped ? 0 : gap)), rc.top, rc.left + i * (tw + (grouped ? 0 : gap)) + tw, rc.bottom};
        if (!grouped) {
            const float radius = c.kit.design.style == fl::theme::Style::FuchsLabs ? 12.f : c.kit.design.card_radius;
            c.fill(r, c.th.card, radius); c.frame(r, c.th.line, radius);
        }
        else if (i) c.line({r.left,r.top + 14},{r.left,r.bottom - 14},c.th.line);
        c.text(c.s(tiles[i].value), {r.left + 12, r.top + 10, r.right - 8, r.top + 40}, Font::Tile, c.th.accent_text);
        c.text(c.s(tiles[i].label), {r.left + 12, r.top + 42, r.right - 8, r.bottom - 8}, Font::Small, c.th.muted);
    }
}
std::wstring Tiles::value(bool en) const {
    std::wstring out;
    for (const auto& t : tiles) out += t.label.get(en) + L": " + t.value.get(en) + L"; ";
    return out;
}

float ImageView::layout(Ctx& c, float x, float y, float w) {
    if (fixed_w > 0) w = std::min(w, fixed_w);
    float h = height(c, w);
    rc = {x, y, x + w, y + h};
    return h;
}
float ImageView::height(Ctx&, float w) {
    if (image && !image->empty()) return w * float(image->h) / float(std::max(1, image->w));
    return w * aspect;
}
void ImageView::paint(Ctx& c) {
    c.fill(rc, c.th.preview_bg, 8);
    if (image && !image->empty()) {
        if (!bitmap_ || made_version_ != version || made_device_ != c.device) {
            bitmap_.Reset();
            auto props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            c.rt->CreateBitmap(D2D1::SizeU(UINT32(image->w), UINT32(image->h)), image->px.data(), UINT32(image->w * 4), props,
                               bitmap_.GetAddressOf());
            made_version_ = version; made_device_ = c.device;
        }
        if (bitmap_) {
            float scale = std::min(rw(rc) / image->w, rh(rc) / image->h);
            float w = image->w * scale, h = image->h * scale;
            Rect dst{rc.left + (rw(rc) - w) / 2, rc.top + (rh(rc) - h) / 2, 0, 0};
            dst.right = dst.left + w; dst.bottom = dst.top + h;
            ComPtr<ID2D1RoundedRectangleGeometry> clip;
            c.kit.d2d()->CreateRoundedRectangleGeometry(D2D1::RoundedRect(rc, 8, 8), clip.GetAddressOf());
            if (clip) c.rt->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip.Get()), nullptr);
            c.rt->DrawBitmap(bitmap_.Get(), dst, c.dim, crisp ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            if (clip) c.rt->PopLayer();
        }
    } else if (!empty_text.empty()) {
        c.text(c.s(empty_text), inflate(rc, -12), Font::Small, from(brand::CREME, .8f), DWRITE_TEXT_ALIGNMENT_CENTER, true, true);
    }
    if (framed) c.frame(rc, c.th.accent, 8, 2);
}

// ---------------------------------------------------------------- Steuerelemente
void Toggle::paint(Ctx& c) {
    Rect track{rc.right - 44, rc.top + (rh(rc) - 24) / 2, rc.right - 2, 0};
    track.bottom = track.top + 24;
    c.fill(track, on ? c.th.accent : c.kit.design.style == fl::theme::Style::FuchsLabs && !c.th.dark ? rgb(214, 186, 152) : c.th.line, 12);
    if (c.kit.design.tonal && !on) c.frame(track, c.th.muted, 12, 2);
    float kx = on ? track.right - 12 : track.left + 12;
    const bool designed = c.kit.design.style != fl::theme::Style::FuchsLabs;
    // Profile: Knopf folgt der Akzentfarbe (hell auf dunkler, dunkel auf heller Spur); aus = Umrissfarbe gegen die Spur
    c.circle({kx, track.top + 12}, c.kit.design.tonal && !on ? 6.f : 9.f, !designed ? from(brand::WHITE) : on ? c.th.on_accent : c.th.muted);
    if (hot && c.kit.design.tonal) c.circle({kx, track.top + 12}, 15, alpha(c.th.accent, .12f));
}
void Toggle::set(bool v, Surface& s) {
    if (v == on) return;
    on = v;
    s.notify(this, EVENT_OBJECT_STATECHANGE);
    s.changed(this);
}
bool Toggle::key(UINT vk, Surface& s) {
    if (vk == VK_SPACE || vk == VK_RETURN) { set(!on, s); return true; }
    return false;
}
void Toggle::release(Point p, Surface& s) { if (inside(rc, p)) set(!on, s); }
void Toggle::activate(Surface& s) { set(!on, s); }

std::wstring Slider::value(bool en) const {
    std::wstring s = std::to_wstring(val);
    if (!suffix.empty()) s += L" " + suffix.get(en);
    return s;
}
void Slider::paint(Ctx& c) {
    const float value_w = 64;
    Rect track{rc.left + 8, rc.top + rh(rc) / 2 - 3, rc.right - value_w - 8, rc.top + rh(rc) / 2 + 3};
    float t = maximum > minimum ? float(val - minimum) / float(maximum - minimum) : 0;
    c.fill(track, c.th.dark ? c.th.line : rgb(232, 204, 170), 3);
    Rect done = track; done.right = track.left + rw(track) * t;
    c.fill(done, c.th.accent, 3);
    Point knob{track.left + rw(track) * t, (track.top + track.bottom) / 2};
    c.circle(knob, 10, c.th.dark ? c.th.card : c.th.bg);
    c.circle(knob, 7, c.th.accent);
    c.text(value(c.kit.english), {rc.right - value_w, rc.top, rc.right, rc.bottom}, Font::Label, c.th.text, DWRITE_TEXT_ALIGNMENT_TRAILING);
}
void Slider::set(int v, Surface& s) {
    v = std::clamp(v, minimum, maximum);
    if (v == val) return;
    val = v;
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.changed(this);
}
void Slider::drag(Point p, Surface& s) {
    float left = rc.left + 8, right = rc.right - 72;
    float t = std::clamp((p.x - left) / std::max(1.f, right - left), 0.f, 1.f);
    int raw = minimum + int(std::lround(t * float(maximum - minimum) / float(step))) * step;
    set(raw, s);
}
bool Slider::key(UINT vk, Surface& s) {
    switch (vk) {
    case VK_LEFT: case VK_DOWN: set(val - step, s); return true;
    case VK_RIGHT: case VK_UP: set(val + step, s); return true;
    case VK_PRIOR: set(val + page, s); return true;
    case VK_NEXT: set(val - page, s); return true;
    case VK_HOME: set(minimum, s); return true;
    case VK_END: set(maximum, s); return true;
    }
    return false;
}

float Segment::pref_width(Ctx& c) {
    float w = 0;
    for (auto& o : options) w += c.text_width(c.s(o), Font::Label) + (header_style ? 22 : 28);
    return w + 2;
}
void Segment::paint(Ctx& c) {
    stops.clear();
    float total = 0;
    std::vector<float> widths;
    for (auto& o : options) { widths.push_back(c.text_width(c.s(o), Font::Label) + (header_style ? 22 : 28)); total += widths.back(); }
    float scale = total > 0 ? (rw(rc) - 2) / total : 1;
    const bool profile_header = header_style && c.kit.design.style != fl::theme::Style::FuchsLabs;
    Rgba base = profile_header ? c.th.card : header_style ? from(brand::NAVY, .13f) : c.th.input;
    c.fill(rc, base, header_style ? 14.f : 8.f);
    if (!header_style) c.frame(rc, c.th.line, 8);
    float x = rc.left + 1;
    for (size_t i = 0; i < options.size(); ++i) {
        float w = widths[i] * scale;
        Rect r{x, rc.top + 1, x + w, rc.bottom - 1};
        bool sel = int(i) == index;
        if (sel) c.fill(r, profile_header ? c.th.accent : header_style ? from(brand::NAVY) : c.th.accent, header_style ? 13.f : 7.f);
        Rgba fg = profile_header ? (sel ? c.th.on_accent : c.th.text) : header_style ? (sel ? from(brand::CREME) : from(brand::NAVY)) : (sel ? c.th.on_accent : c.th.muted);
        c.text(c.s(options[i]), r, Font::Label, fg, DWRITE_TEXT_ALIGNMENT_CENTER);
        x += w;
        stops.push_back(x);
    }
}
void Segment::set(int i, Surface& s) {
    i = std::clamp(i, 0, int(options.size()) - 1);
    if (i == index) return;
    index = i;
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.changed(this);
}
bool Segment::key(UINT vk, Surface& s) {
    switch (vk) {
    case VK_LEFT: case VK_UP: set(index - 1, s); return true;
    case VK_RIGHT: case VK_DOWN: set(index + 1, s); return true;
    case VK_HOME: set(0, s); return true;
    case VK_END: set(int(options.size()) - 1, s); return true;
    case VK_SPACE: set((index + 1) % int(options.size()), s); return true;
    }
    return false;
}
void Segment::release(Point p, Surface& s) {
    if (!inside(rc, p)) return;
    for (size_t i = 0; i < stops.size(); ++i) if (p.x < stops[i]) { set(int(i), s); return; }
}
std::wstring Segment::value(bool en) const { return index >= 0 && index < int(options.size()) ? options[index].get(en) : L""; }

void Combo::paint(Ctx& c) {
    c.fill(rc, c.th.input, 8);
    c.frame(rc, c.th.line, 8);
    c.text(value(c.kit.english), {rc.left + 12, rc.top, rc.right - 30, rc.bottom}, Font::Body, c.th.text);
    float cx = rc.right - 17, cy = (rc.top + rc.bottom) / 2;
    c.line({cx - 5, cy - 2}, {cx, cy + 3}, c.th.muted, 1.6f);
    c.line({cx, cy + 3}, {cx + 5, cy - 2}, c.th.muted, 1.6f);
}
void Combo::set(int i, Surface& s) {
    i = std::clamp(i, 0, int(options.size()) - 1);
    if (i == index) return;
    index = i;
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.changed(this);
}
void Combo::activate(Surface& s) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    for (size_t i = 0; i < options.size(); ++i)
        AppendMenuW(menu, MF_STRING | (int(i) == index ? MF_CHECKED : 0), UINT_PTR(i + 1), options[i].get(s.kit.english).c_str());
    POINT pt{s.px(rc.left), s.px(rc.bottom - s.scroll()) + 2};
    ClientToScreen(s.hwnd(), &pt);
    int chosen = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY, pt.x, pt.y, s.hwnd(), nullptr);
    DestroyMenu(menu);
    if (chosen > 0) set(chosen - 1, s);
}
bool Combo::key(UINT vk, Surface& s) {
    if (vk == VK_SPACE || vk == VK_RETURN || vk == VK_F4 || (vk == VK_DOWN && GetKeyState(VK_MENU) < 0)) { activate(s); return true; }
    if (vk == VK_UP) { set(index - 1, s); return true; }
    if (vk == VK_DOWN) { set(index + 1, s); return true; }
    return false;
}
void Combo::release(Point p, Surface& s) { if (inside(rc, p)) activate(s); }
std::wstring Combo::value(bool en) const { return index >= 0 && index < int(options.size()) ? options[index].get(en) : L""; }

float Button::pref_width(Ctx& c) { return c.text_width(c.s(text), Font::Button) + 40; }
void Button::paint(Ctx& c) {
    const float radius = std::min(c.kit.design.button_radius, rh(rc) / 2);
    switch (kind) {
    case Primary:
        if (enabled && c.kit.design.style == fl::theme::Style::FuchsLabs) for (int i = 3; i >= 1; --i) c.fill(inflate(rc, float(i) * 3), alpha(c.th.accent, .07f * float(4 - i)), 8 + float(i) * 3);
        c.fill(rc, c.th.accent, radius);
        c.text(c.s(text), rc, Font::Button, c.th.on_accent, DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    case Secondary:
        c.fill(rc, c.th.dark ? c.th.bg : from(brand::WHITE, .55f), radius);
        if (c.kit.design.tonal) c.fill(rc, alpha(c.th.accent, c.th.dark ? .16f : .10f), radius);
        c.frame(rc, c.th.line, radius);
        c.text(c.s(text), rc, Font::Button, c.th.text, DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    case Danger:
        c.frame(rc, c.th.danger, radius, 1.5f);
        c.text(c.s(text), rc, Font::Button, c.th.danger, DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    case Ghost:
        c.text(c.s(text), rc, Font::Button, c.th.accent_text, DWRITE_TEXT_ALIGNMENT_CENTER);
        break;
    }
    if (pressed && enabled && c.kit.design.style != fl::theme::Style::FuchsLabs) {
        const float opacity = c.kit.design.tonal ? .18f : c.kit.design.grouped ? .22f : .12f;
        c.fill(rc, alpha(c.th.text, opacity), radius);
        if (c.kit.design.underline_selection) c.frame(inflate(rc, -2), alpha(c.th.text, .3f), radius, 1);
    }
    if (hot && enabled) {
        if (c.kit.design.style == fl::theme::Style::FuchsLabs) c.fill(rc, c.th.dark ? from(brand::WHITE, .07f) : from(brand::NAVY, .06f), radius);
        else c.fill(rc, c.th.dark ? alpha(c.th.text, c.kit.design.tonal ? .12f : .06f) : alpha(c.th.text, c.kit.design.tonal ? .08f : .04f), radius);
    }
}
bool Button::key(UINT vk, Surface& s) {
    if (vk == VK_SPACE || vk == VK_RETURN) { activate(s); return true; }
    return false;
}
void Button::release(Point p, Surface& s) { if (inside(rc, p)) activate(s); }
void Button::activate(Surface&) {
    if (!enabled || !on_click) return;
    auto action = on_click;  // Der Klick darf diese Schaltfläche entfernen (Seite neu aufbauen).
    action();
}

float ButtonRow::layout(Ctx& c, float x, float y, float w) {
    float cx = x, cy = y;
    for (auto& b : buttons) {
        if (!b->visible) continue;
        float bw = std::min(b->pref_width(c), w);
        if (cx > x && cx + bw > x + w) { cx = x; cy += 46; }
        b->layout(c, cx, cy, bw);
        cx += bw + 10;
    }
    rc = {x, y, x + w, cy + 36};
    return cy + 36 - y;
}
void ButtonRow::children(std::vector<Widget*>& out) { for (auto& b : buttons) out.push_back(b.get()); }

// ---------------------------------------------------------------- Textfeld (EDIT)
static LRESULT CALLBACK edit_proc(HWND h, UINT m, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR ref) {
    auto* s = reinterpret_cast<Surface*>(ref);
    auto* w = static_cast<Widget*>(GetPropW(h, L"fl.w"));
    const bool multi = (GetWindowLongW(h, GWL_STYLE) & ES_MULTILINE) != 0;
    switch (m) {
    case WM_KEYDOWN:
        if (wp == VK_TAB) { s->keyboard_cues = true; s->tab(GetKeyState(VK_SHIFT) >= 0); return 0; }
        break;
    case WM_CHAR:
        if (wp == VK_TAB || (wp == VK_RETURN && !multi)) return 0;
        if (wp == 1) { SendMessageW(h, EM_SETSEL, 0, -1); return 0; }  // Strg+A
        break;
    case WM_SETFOCUS:
        if (w) s->native_focus(w);
        break;
    case WM_KILLFOCUS:
        InvalidateRect(s->hwnd(), nullptr, FALSE);
        break;
    case WM_MOUSEWHEEL:
        if (!multi) return SendMessageW(s->hwnd(), m, wp, lp);
        break;
    case WM_NCDESTROY:
        if (auto* field = dynamic_cast<TextField*>(w)) field->detach_native();
        RemovePropW(h, L"fl.w");
        RemoveWindowSubclass(h, edit_proc, 1);
        break;
    }
    return DefSubclassProc(h, m, wp, lp);
}

static void set_acc_name(HWND hwnd, const std::wstring& name) {
    ComPtr<IAccPropServices> services;
    if (FAILED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(services.GetAddressOf())))) return;
    services->SetHwndPropStr(hwnd, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME, name.c_str());
}

TextField::TextField(Kind k, std::wstring v) : kind(k), original(k == Secret ? L"" : v), pending_(k == Secret ? L"" : std::move(v)) {
    if (kind == Number) { width = 120; limit = 6; }
    if (kind == Multi) limit = 16384;
}
TextField::~TextField() {
    if (edit_ && IsWindow(edit_)) DestroyWindow(edit_);
}
std::wstring TextField::text() const {
    if (!edit_) return pending_;
    int n = GetWindowTextLengthW(edit_);
    std::wstring t(size_t(n) + 1, L'\0');
    GetWindowTextW(edit_, t.data(), n + 1);
    t.resize(size_t(n));
    return t;
}
void TextField::set_text(const std::wstring& t) {
    if (edit_) SetWindowTextW(edit_, t.c_str());
    else pending_ = t;
}
float TextField::height(Ctx&, float) { return kind == Multi ? float(lines) * 20 + 18 : 34; }
bool TextField::dirty() const {
    if (!track_dirty) return false;
    std::wstring t = text();
    return kind == Secret ? !t.empty() : t != original;
}
void TextField::paint(Ctx& c) {
    c.fill(rc, c.th.input, 8);
    bool focused = edit_ && GetFocus() == edit_;
    c.frame(rc, focused ? c.th.accent : c.th.line, 8, focused ? 2.f : 1.f);
}
void TextField::focus_native() { if (edit_) SetFocus(edit_); }
void TextField::apply_hint(Surface& s) {
    if (!edit_) return;
    std::wstring h = hint.get(s.kit.english);
    if (kind == Secret && h.empty())
        h = secret_set ? (s.kit.english ? L"Saved – leave empty to keep" : L"Gespeichert – leer lassen zum Beibehalten")
                       : (s.kit.english ? L"Not configured yet" : L"Noch nicht eingerichtet");
    if (kind != Multi) SendMessageW(edit_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(h.c_str()));
    std::wstring acc = name.get(s.kit.english);
    if (kind == Secret) acc += s.kit.english ? (secret_set ? L" (saved, leave empty to keep)" : L" (not set)")
                                             : (secret_set ? L" (gespeichert, leer lassen zum Beibehalten)" : L" (nicht gesetzt)");
    set_acc_name(edit_, acc);
}
void TextField::restyle(Surface& s) {
    if (!edit_) return;
    SendMessageW(edit_, WM_SETFONT, reinterpret_cast<WPARAM>(s.edit_font()), TRUE);
    SendMessageW(edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
    SetWindowTheme(edit_, s.kit.dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);  // Bildlaufleiste passend zum Theme
    apply_hint(s);
    InvalidateRect(edit_, nullptr, TRUE);
}
void TextField::place(Surface& s, bool shown) {
    if (!edit_ && shown && !created_) {
        DWORD style = WS_CHILD | WS_TABSTOP;
        if (kind == Multi) style |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL;
        else style |= ES_AUTOHSCROLL;
        if (kind == Secret) style |= ES_PASSWORD;
        if (kind == Number) style |= ES_NUMBER;
        edit_ = CreateWindowExW(0, L"EDIT", L"", style, 0, 0, 10, 10, s.hwnd(), nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!edit_) return;
        created_ = true;
        SendMessageW(edit_, EM_SETLIMITTEXT, WPARAM(limit), 0);
        if (kind == Secret) SendMessageW(edit_, EM_SETPASSWORDCHAR, L'\x25CF', 0);
        SetWindowTextW(edit_, pending_.c_str());
        SetPropW(edit_, L"fl.w", static_cast<Widget*>(this));
        SetWindowSubclass(edit_, edit_proc, 1, reinterpret_cast<DWORD_PTR>(&s));
        restyle(s);
    }
    if (!edit_) return;
    if (!shown) { ShowWindow(edit_, SW_HIDE); return; }
    EnableWindow(edit_, enabled);
    TEXTMETRICW tm{};
    HDC dc = GetDC(edit_);
    HGDIOBJ old = SelectObject(dc, s.edit_font());
    GetTextMetricsW(dc, &tm);
    SelectObject(dc, old);
    ReleaseDC(edit_, dc);
    const int line = tm.tmHeight;
    int left = s.px(rc.left + 11), right = s.px(rc.right - 8);
    int top = s.px(rc.top - s.scroll()), bottom = s.px(rc.bottom - s.scroll());
    if (kind == Multi) { top += s.px(8); bottom -= s.px(8); }
    else { top += (bottom - top - line) / 2; bottom = top + line; }
    SetWindowPos(edit_, nullptr, left, top, std::max(1, right - left), std::max(1, bottom - top),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

// ---------------------------------------------------------------- Farbfeld
static Rgba hex_rgba(const std::string& hex, Rgba none) {
    auto n = logic::normalize_hex(hex);
    if (!n) return none;
    auto v = [&](size_t i) { return uint8_t(std::stoi(n->substr(i, 2), nullptr, 16)); };
    return rgb(v(1), v(3), v(5));
}
ColorField::ColorField(const std::string& h) : field(TextField::Single, widen(h)), original_hex(h) {
    field.limit = 7;
    field.hint = Text(L"#RRGGBB");
    field.name = T(L"Farbwert", L"Color value");
}
std::string ColorField::hex() const {
    auto n = logic::normalize_hex(narrow(field.text()));
    return n ? *n : std::string();
}
bool ColorField::valid() const {
    std::string t = trim(narrow(field.text()));
    return t.empty() || logic::normalize_hex(t).has_value();
}
std::wstring ColorField::value(bool) const { return widen(effective()); }
void ColorField::set_hex(const std::string& h) { field.set_text(widen(h)); }
float ColorField::layout(Ctx& c, float x, float y, float w) {
    rc = {x, y, x + w, y + 34};
    field.layout(c, x + 46, y, std::max(60.f, w - 46));
    return 34;
}
void ColorField::paint(Ctx& c) {
    Rect sw{rc.left, rc.top + 2, rc.left + 38, rc.bottom - 2};
    c.fill(sw, hex_rgba(effective(), c.th.input), 8);
    c.frame(sw, valid() ? c.th.line : c.th.danger, 8, valid() ? 1.f : 2.f);
}
bool ColorField::key(UINT vk, Surface& s) {
    if (vk == VK_SPACE || vk == VK_RETURN) { pick(s); return true; }
    return false;
}
void ColorField::release(Point p, Surface& s) {
    if (p.x < rc.left + 40 && inside(rc, p)) pick(s);
}
void ColorField::pick(Surface& s) {
    static COLORREF custom[16] = {};
    Rgba cur = hex_rgba(effective(), rgb(0, 0, 0));
    CHOOSECOLORW cc{sizeof cc};
    cc.hwndOwner = GetAncestor(s.hwnd(), GA_ROOT);
    cc.rgbResult = RGB(BYTE(cur.r * 255 + .5f), BYTE(cur.g * 255 + .5f), BYTE(cur.b * 255 + .5f));
    cc.lpCustColors = custom;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
    if (!ChooseColorW(&cc)) return;
    set_hex(strprintf("#%02X%02X%02X", GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult)));
    s.invalidate();
}

// ---------------------------------------------------------------- Liste
float ItemList::row_height() const {
    bool details = std::any_of(items.begin(), items.end(), [](auto& i) { return !i.detail.empty(); });
    return details ? 52.f : 38.f;
}
float ItemList::height(Ctx&, float) { return items.empty() ? 48.f : float(items.size()) * row_height() + 2; }
void ItemList::paint(Ctx& c) {
    c.fill(rc, c.th.input, 8);
    c.frame(rc, c.th.line, 8);
    if (items.empty()) {
        c.text(c.s(empty_text), inflate(rc, -12), Font::Small, c.th.muted, DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }
    const float h = row_height();
    for (size_t i = 0; i < items.size(); ++i) {
        Rect r{rc.left + 1, rc.top + 1 + h * i, rc.right - 1, rc.top + 1 + h * (i + 1)};
        if (int(i) == selected) {
            c.fill(r, alpha(c.th.accent, .20f), 6);
            c.fill({r.left, r.top + 6, r.left + 3, r.bottom - 6}, c.th.accent, 1.5f);
        } else if (i > 0) c.line({r.left + 12, r.top}, {r.right - 12, r.top}, alpha(c.th.line, .8f));
        float x = r.left + 14;
        if (checkable) {
            Rect box{x, r.top + h / 2 - 9, x + 18, r.top + h / 2 + 9};
            if (items[i].enabled) {
                c.fill(box, c.th.accent, 4);
                c.line({box.left + 4, box.top + 9.5f}, {box.left + 7.5f, box.top + 13}, c.th.on_accent, 2);
                c.line({box.left + 7.5f, box.top + 13}, {box.left + 14, box.top + 5.5f}, c.th.on_accent, 2);
            } else c.frame(box, c.th.muted, 4, 1.5f);
            x += 30;
        }
        float right = reorder ? r.right - 70 : r.right - 10;
        if (items[i].detail.empty()) c.text(c.s(items[i].title), {x, r.top, right, r.bottom}, Font::Body, c.th.text);
        else {
            c.text(c.s(items[i].title), {x, r.top + 6, right, r.top + 26}, Font::Strong, c.th.text);
            c.text(c.s(items[i].detail), {x, r.top + 26, right, r.bottom - 6}, Font::Small, c.th.muted);
        }
        if (reorder) {
            for (int part = 0; part < 2; ++part) {
                bool possible = part == 0 ? i > 0 : i + 1 < items.size();
                Rect b{r.right - 64 + part * 30.f, r.top + h / 2 - 12, r.right - 40 + part * 30.f, r.top + h / 2 + 12};
                c.fill(b, alpha(c.th.line, possible ? .9f : .3f), 6);
                float cx = (b.left + b.right) / 2, cy = (b.top + b.bottom) / 2, d = part == 0 ? -1.f : 1.f;
                Rgba fg = possible ? c.th.text : alpha(c.th.muted, .5f);
                c.line({cx - 5, cy - 2.5f * d}, {cx, cy + 2.5f * d}, fg, 1.8f);
                c.line({cx, cy + 2.5f * d}, {cx + 5, cy - 2.5f * d}, fg, 1.8f);
            }
        }
    }
}
bool ItemList::hit_check(Point p, int& index, int& part) const {
    if (!inside(rc, p) || items.empty()) return false;
    const float h = row_height();
    index = int((p.y - rc.top - 1) / h);
    if (index < 0 || index >= int(items.size())) return false;
    part = 0;
    if (checkable && p.x < rc.left + 44) part = 1;
    if (reorder) {
        if (p.x >= rc.right - 65 && p.x < rc.right - 39) part = 2;
        else if (p.x >= rc.right - 35 && p.x < rc.right - 9) part = 3;
    }
    return true;
}
void ItemList::select(int i, Surface& s) {
    if (items.empty()) return;
    i = std::clamp(i, 0, int(items.size()) - 1);
    if (i == selected) return;
    selected = i;
    if (on_select) on_select(i);
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.invalidate();
}
void ItemList::toggle(int i, Surface& s) {
    if (!checkable || i < 0 || i >= int(items.size())) return;
    items[size_t(i)].enabled = !items[size_t(i)].enabled;
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.changed(this);
}
void ItemList::move(int i, int delta, Surface& s) {
    int j = i + delta;
    if (!reorder || i < 0 || j < 0 || i >= int(items.size()) || j >= int(items.size())) return;
    std::swap(items[size_t(i)], items[size_t(j)]);
    selected = j;
    s.notify(this, EVENT_OBJECT_VALUECHANGE);
    s.changed(this);
}
void ItemList::release(Point p, Surface& s) {
    int index = 0, part = 0;
    if (!hit_check(p, index, part)) return;
    if (part == 2) { move(index, -1, s); return; }
    if (part == 3) { move(index, 1, s); return; }
    select(index, s);
    if (part == 1) toggle(index, s);
}
void ItemList::activate(Surface& s) { toggle(selected, s); }
bool ItemList::key(UINT vk, Surface& s) {
    const bool move_mode = GetKeyState(VK_MENU) < 0 || GetKeyState(VK_CONTROL) < 0;
    switch (vk) {
    case VK_UP: if (move_mode) move(selected, -1, s); else select(selected < 0 ? 0 : selected - 1, s); return true;
    case VK_DOWN: if (move_mode) move(selected, 1, s); else select(selected + 1, s); return true;
    case VK_HOME: select(0, s); return true;
    case VK_END: select(int(items.size()) - 1, s); return true;
    case VK_SPACE: toggle(selected, s); return true;
    }
    return false;
}
bool ItemList::dirty() const {
    if (!track_dirty || items.size() != original.size()) return track_dirty;
    for (size_t i = 0; i < items.size(); ++i)
        if (items[i].key != original[i].key || items[i].enabled != original[i].enabled) return true;
    return false;
}
std::wstring ItemList::value(bool en) const {
    if (selected < 0 || selected >= int(items.size())) return en ? L"no selection" : L"keine Auswahl";
    const auto& item = items[size_t(selected)];
    std::wstring v = item.title.get(en);
    if (!item.detail.empty()) v += L", " + item.detail.get(en);
    if (checkable) v += item.enabled ? (en ? L", on" : L", an") : (en ? L", off" : L", aus");
    v += L", " + std::to_wstring(selected + 1) + (en ? L" of " : L" von ") + std::to_wstring(items.size());
    return v;
}

// ---------------------------------------------------------------- Navigation
void NavItem::paint(Ctx& c) {
    if (rail) {
        const float cx = (rc.left + rc.right) / 2, cy = rc.top + 22;
        const Rect indicator{rc.left + 10,rc.top + 3,rc.right - 10,rc.top + 39};
        if (active || hot || pressed) c.fill(indicator, alpha(c.th.accent, active ? .18f : pressed ? .16f : .08f), 18);
        const Rgba ink = active ? c.th.accent_text : c.th.text;
        if (icon == 0) {
            for (int y = 0; y < 2; ++y) for (int x = 0; x < 2; ++x)
                c.frame({cx - 9 + x * 10.f,cy - 9 + y * 10.f,cx - 2 + x * 10.f,cy - 2 + y * 10.f}, ink, 2, 1.5f);
        } else if (icon == 1) {
            c.frame({cx - 10,cy - 8,cx + 10,cy + 5}, ink, 3, 1.6f);
            c.line({cx,cy + 5},{cx,cy + 10},ink,1.6f); c.line({cx - 6,cy + 10},{cx + 6,cy + 10},ink,1.6f);
        } else if (icon == 2) {
            for (int i = 0; i < 3; ++i) c.fill({cx - 10 + i * 8.f,cy + 7 - i * 5.f,cx - 5 + i * 8.f,cy + 10},ink,2);
        } else if (icon == 3) {
            c.frame({cx - 9,cy - 8,cx + 9,cy + 9},ink,4,1.6f); c.circle({cx,cy - 9},4,ink); c.circle({cx + 9,cy + 1},4,ink);
        } else {
            for (int i = 0; i < 3; ++i) {
                const float y = cy - 8 + i * 8.f, x = cx + (i == 1 ? 4.f : -4.f);
                c.line({cx - 10,y},{cx + 10,y},ink,1.5f); c.circle({x,y},3,ink);
            }
        }
        c.text(c.s(text), {rc.left,rc.top + 43,rc.right,rc.bottom},Font::Small,
            active ? c.th.accent_text : c.th.muted,DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }
    if (active) {
        c.fill(rc, c.kit.design.grouped ? alpha(c.th.accent, c.th.dark ? .22f : .14f) : c.th.nav_active,
               c.kit.design.tonal ? rh(rc) / 2 : c.kit.design.button_radius);
        if (c.kit.design.underline_selection)
            c.fill({rc.left + 14, rc.top + 7, rc.left + 17, rc.bottom - 7}, c.th.accent, 1.5f);
        else if (!c.kit.design.grouped && !c.kit.design.tonal)
            c.fill({rc.left, rc.top + 4, rc.left + 3, rc.bottom - 4}, c.th.accent, 1.5f);
    } else if (hot) c.fill(rc, c.th.dark ? from(brand::WHITE, .05f) : from(brand::NAVY, .07f), 8);
    const float shift = c.kit.design.underline_selection ? 11.f : 0.f;   // Fluent: Auswahlstrich bei +14..17 nicht überdecken
    if (has_dot) c.circle({rc.left + 16 + shift, (rc.top + rc.bottom) / 2}, 4, dot_on ? c.th.success : alpha(c.th.side_muted, .45f));
    c.text(c.s(text), {rc.left + 30 + shift, rc.top, rc.right - 8, rc.bottom}, Font::Nav,
           active ? (c.kit.design.grouped ? c.th.accent_text : c.th.text) : c.th.side_text);
}
bool NavItem::key(UINT vk, Surface& s) {
    if (vk == VK_UP || vk == VK_DOWN) { if (on_step) on_step(vk == VK_UP ? -1 : 1); return true; }
    if (vk == VK_SPACE || vk == VK_RETURN) { activate(s); return true; }
    return false;
}
void NavItem::release(Point p, Surface& s) { if (inside(rc, p)) activate(s); }
void NavItem::activate(Surface&) { if (on_open) { auto f = on_open; f(); } }

// ---------------------------------------------------------------- Barrierefreiheit (MSAA)
class Accessible : public IAccessible {
public:
    explicit Accessible(Surface* s) : surface_(s) {}
    void detach() { surface_ = nullptr; }
    std::vector<Widget*> list() const {
        std::vector<Widget*> out;
        if (!surface_) return out;
        for (Widget* w : surface_->flatten(false)) {
            if (w->role() == Role::Group || w->role() == Role::Edit || dynamic_cast<Portal*>(w)) continue;
            out.push_back(w);
        }
        return out;
    }
    Widget* child(const VARIANT& v) const {
        if (v.vt != VT_I4 || v.lVal == CHILDID_SELF) return nullptr;
        auto l = list();
        return v.lVal >= 1 && v.lVal <= long(l.size()) ? l[size_t(v.lVal - 1)] : nullptr;
    }
    long id_of(Widget* w) const {
        auto l = list();
        auto it = std::find(l.begin(), l.end(), w);
        return it == l.end() ? CHILDID_SELF : long(it - l.begin()) + 1;
    }
    bool valid(const VARIANT& v) const { return v.vt == VT_I4 && (v.lVal == CHILDID_SELF || child(v)); }
    static BSTR bstr(const std::wstring& s) { return SysAllocStringLen(s.data(), UINT(s.size())); }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** out) override {
        if (!out) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDispatch || riid == IID_IAccessible) { *out = static_cast<IAccessible*>(this); AddRef(); return S_OK; }
        *out = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override { ULONG r = InterlockedDecrement(&refs_); if (!r) delete this; return r; }
    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* n) override { if (n) *n = 0; return S_OK; }
    STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*) override { return E_NOTIMPL; }
    STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*) override { return E_NOTIMPL; }
    // IAccessible
    STDMETHODIMP get_accParent(IDispatch** p) override {
        if (!surface_) return E_FAIL;
        return AccessibleObjectFromWindow(surface_->hwnd(), OBJID_WINDOW, IID_IDispatch, reinterpret_cast<void**>(p));
    }
    STDMETHODIMP get_accChildCount(long* n) override { if (!surface_) return E_FAIL; *n = long(list().size()); return S_OK; }
    STDMETHODIMP get_accChild(VARIANT, IDispatch** p) override { *p = nullptr; return S_FALSE; }
    STDMETHODIMP get_accName(VARIANT v, BSTR* out) override {
        *out = nullptr;
        if (!surface_ || !valid(v)) return E_INVALIDARG;
        const bool en = surface_->kit.english;
        if (Widget* w = child(v)) {
            std::wstring name = w->name.get(en);
            if (name.empty()) name = w->value(en);
            if (auto b = dynamic_cast<Button*>(w); b && w->name.empty()) name = b->text.get(en);
            if (auto n = dynamic_cast<NavItem*>(w)) name = n->text.get(en);
            if (auto h = dynamic_cast<Heading*>(w)) name = h->title.get(en) + L". " + h->subtitle.get(en);
            *out = bstr(name);
        } else {
            wchar_t title[256]{};
            GetWindowTextW(surface_->hwnd(), title, 256);
            *out = bstr(title);
        }
        return *out ? S_OK : S_FALSE;
    }
    STDMETHODIMP get_accValue(VARIANT v, BSTR* out) override {
        *out = nullptr;
        Widget* w = surface_ ? child(v) : nullptr;
        if (!w) return S_FALSE;
        Role r = w->role();
        if (r != Role::Slider && r != Role::Choice && r != Role::List) return S_FALSE;
        *out = bstr(w->value(surface_->kit.english));
        return S_OK;
    }
    STDMETHODIMP get_accDescription(VARIANT, BSTR* out) override { *out = nullptr; return S_FALSE; }
    STDMETHODIMP get_accRole(VARIANT v, VARIANT* out) override {
        if (!surface_ || !valid(v)) return E_INVALIDARG;
        out->vt = VT_I4;
        Widget* w = child(v);
        if (!w) { out->lVal = ROLE_SYSTEM_CLIENT; return S_OK; }
        switch (w->role()) {
        case Role::Button: out->lVal = ROLE_SYSTEM_PUSHBUTTON; break;
        case Role::Toggle: out->lVal = ROLE_SYSTEM_CHECKBUTTON; break;
        case Role::Slider: out->lVal = ROLE_SYSTEM_SLIDER; break;
        case Role::Choice: out->lVal = ROLE_SYSTEM_COMBOBOX; break;
        case Role::Edit: out->lVal = ROLE_SYSTEM_TEXT; break;
        case Role::List: out->lVal = ROLE_SYSTEM_LIST; break;
        case Role::NavTab: out->lVal = ROLE_SYSTEM_PAGETAB; break;
        case Role::Group: out->lVal = ROLE_SYSTEM_GROUPING; break;
        case Role::Image: out->lVal = ROLE_SYSTEM_GRAPHIC; break;
        default: out->lVal = ROLE_SYSTEM_STATICTEXT; break;
        }
        return S_OK;
    }
    STDMETHODIMP get_accState(VARIANT v, VARIANT* out) override {
        if (!surface_ || !valid(v)) return E_INVALIDARG;
        out->vt = VT_I4; out->lVal = 0;
        Widget* w = child(v);
        if (!w) { if (GetFocus() == surface_->hwnd()) out->lVal |= STATE_SYSTEM_FOCUSED; return S_OK; }
        if (w->focusable()) out->lVal |= STATE_SYSTEM_FOCUSABLE;
        if (w == surface_->focused() && GetFocus() == surface_->hwnd()) out->lVal |= STATE_SYSTEM_FOCUSED;
        if (!w->enabled) out->lVal |= STATE_SYSTEM_UNAVAILABLE;
        if (w->checked()) out->lVal |= w->role() == Role::NavTab ? STATE_SYSTEM_SELECTED : STATE_SYSTEM_CHECKED;
        D2D1_SIZE_F size = surface_->size();
        if (w->rc.bottom - surface_->scroll() < 0 || w->rc.top - surface_->scroll() > size.height) out->lVal |= STATE_SYSTEM_OFFSCREEN;
        return S_OK;
    }
    STDMETHODIMP get_accHelp(VARIANT, BSTR* out) override { *out = nullptr; return S_FALSE; }
    STDMETHODIMP get_accHelpTopic(BSTR* f, VARIANT, long*) override { *f = nullptr; return S_FALSE; }
    STDMETHODIMP get_accKeyboardShortcut(VARIANT, BSTR* out) override { *out = nullptr; return S_FALSE; }
    STDMETHODIMP get_accFocus(VARIANT* out) override {
        out->vt = VT_EMPTY;
        if (!surface_ || GetFocus() != surface_->hwnd()) return S_FALSE;
        long id = surface_->focused() ? id_of(surface_->focused()) : CHILDID_SELF;
        out->vt = VT_I4; out->lVal = id;
        return S_OK;
    }
    STDMETHODIMP get_accSelection(VARIANT* out) override { out->vt = VT_EMPTY; return S_FALSE; }
    STDMETHODIMP get_accDefaultAction(VARIANT v, BSTR* out) override {
        *out = nullptr;
        Widget* w = surface_ ? child(v) : nullptr;
        if (!w || !w->focusable()) return S_FALSE;
        const bool en = surface_->kit.english;
        *out = bstr(w->role() == Role::Toggle ? (en ? L"Toggle" : L"Umschalten") : (en ? L"Press" : L"Drücken"));
        return S_OK;
    }
    STDMETHODIMP accSelect(long flags, VARIANT v) override {
        Widget* w = surface_ ? child(v) : nullptr;
        if (!w) return E_INVALIDARG;
        if (flags & SELFLAG_TAKEFOCUS) surface_->focus(w, true);
        return S_OK;
    }
    STDMETHODIMP accLocation(long* x, long* y, long* cx, long* cy, VARIANT v) override {
        if (!surface_ || !valid(v)) return E_INVALIDARG;
        RECT r{};
        if (Widget* w = child(v)) {
            r = {surface_->px(w->rc.left), surface_->px(w->rc.top - surface_->scroll()),
                 surface_->px(w->rc.right), surface_->px(w->rc.bottom - surface_->scroll())};
        } else GetClientRect(surface_->hwnd(), &r);
        POINT a{r.left, r.top};
        ClientToScreen(surface_->hwnd(), &a);
        *x = a.x; *y = a.y; *cx = r.right - r.left; *cy = r.bottom - r.top;
        return S_OK;
    }
    STDMETHODIMP accNavigate(long dir, VARIANT start, VARIANT* out) override {
        out->vt = VT_EMPTY;
        if (!surface_) return E_FAIL;
        auto l = list();
        long n = long(l.size());
        long from = start.vt == VT_I4 ? start.lVal : CHILDID_SELF, to = 0;
        switch (dir) {
        case NAVDIR_FIRSTCHILD: if (from != CHILDID_SELF || !n) return S_FALSE; to = 1; break;
        case NAVDIR_LASTCHILD: if (from != CHILDID_SELF || !n) return S_FALSE; to = n; break;
        case NAVDIR_NEXT: case NAVDIR_DOWN: if (from == CHILDID_SELF || from >= n) return S_FALSE; to = from + 1; break;
        case NAVDIR_PREVIOUS: case NAVDIR_UP: if (from <= 1) return S_FALSE; to = from - 1; break;
        default: return S_FALSE;
        }
        out->vt = VT_I4; out->lVal = to;
        return S_OK;
    }
    STDMETHODIMP accHitTest(long x, long y, VARIANT* out) override {
        out->vt = VT_EMPTY;
        if (!surface_) return E_FAIL;
        POINT p{x, y};
        ScreenToClient(surface_->hwnd(), &p);
        RECT client{};
        GetClientRect(surface_->hwnd(), &client);
        if (!PtInRect(&client, p)) return S_FALSE;
        Point doc{p.x * 96.f / surface_->dpi(), p.y * 96.f / surface_->dpi() + surface_->scroll()};
        auto l = list();
        long id = CHILDID_SELF;
        for (size_t i = 0; i < l.size(); ++i) if (inside(l[i]->rc, doc)) id = long(i) + 1;  // innerstes zuletzt
        out->vt = VT_I4; out->lVal = id;
        return S_OK;
    }
    STDMETHODIMP accDoDefaultAction(VARIANT v) override {
        Widget* w = surface_ ? child(v) : nullptr;
        if (!w || !w->live()) return E_INVALIDARG;
        w->activate(*surface_);
        return S_OK;
    }
    STDMETHODIMP put_accName(VARIANT, BSTR) override { return E_NOTIMPL; }
    STDMETHODIMP put_accValue(VARIANT, BSTR) override { return E_NOTIMPL; }
private:
    Surface* surface_;
    LONG refs_ = 1;
};

// ---------------------------------------------------------------- Surface
static constexpr wchar_t SurfaceClass[] = L"ScreenBuddyNativeSettings";
static constexpr wchar_t ContentClass[] = L"ScreenBuddySettingsContent";

static void register_classes() {
    static bool done = false;
    if (done) return;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    for (const wchar_t* name : {SurfaceClass, ContentClass}) {
        WNDCLASSEXW k{sizeof(k)};
        k.style = CS_DBLCLKS;
        k.lpfnWndProc = Surface::proc;
        k.hInstance = instance;
        k.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        k.hbrBackground = nullptr;
        k.hIcon = name == SurfaceClass ? LoadIconW(instance, MAKEINTRESOURCEW(1)) : nullptr;
        k.lpszClassName = name;
        if (!RegisterClassExW(&k) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Native settings: RegisterClassExW");
    }
    done = true;
}

Surface::Surface(Kit& k, HWND parent, const wchar_t* name, bool scrollable) : kit(k), scrollable_(scrollable) {
    register_classes();
    DWORD style = parent ? (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP) : (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);
    hwnd_ = CreateWindowExW(parent ? WS_EX_CONTROLPARENT : 0, parent ? ContentClass : SurfaceClass, name, style,
                            CW_USEDEFAULT, CW_USEDEFAULT, parent ? 10 : 1180, parent ? 10 : 760, parent, nullptr,
                            GetModuleHandleW(nullptr), this);
    if (!hwnd_) throw std::runtime_error("Native settings: CreateWindowExW");
    dpi_ = GetDpiForWindow(hwnd_);
    acc_ = new Accessible(this);
    make_edit_resources();
}
Surface::~Surface() {
    if (acc_) { acc_->detach(); acc_->Release(); acc_ = nullptr; }
    if (hwnd_ && IsWindow(hwnd_)) {
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
        DestroyWindow(hwnd_);
    }
    if (edit_font_) DeleteObject(edit_font_);
    if (input_brush_) DeleteObject(input_brush_);
}

LRESULT CALLBACK Surface::proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto self = reinterpret_cast<Surface*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        self = static_cast<Surface*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
    if (msg == WM_NCDESTROY) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        self->hwnd_ = nullptr;
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return self->handle(msg, wp, lp);
}

void Surface::make_edit_resources() {
    if (edit_font_) DeleteObject(edit_font_);
    edit_font_ = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (input_brush_) DeleteObject(input_brush_);
    input_brush_ = CreateSolidBrush(colorref(kit.theme().input));
}

void Surface::create_target() {
    if (rt_) return;
    RECT r{};
    GetClientRect(hwnd_, &r);
    auto props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), float(dpi_), float(dpi_));
    auto hprops = D2D1::HwndRenderTargetProperties(hwnd_, D2D1::SizeU(UINT32(std::max(1L, r.right)), UINT32(std::max(1L, r.bottom))));
    if (FAILED(kit.d2d()->CreateHwndRenderTarget(props, hprops, rt_.GetAddressOf()))) return;
    rt_->CreateSolidColorBrush(D2D1::ColorF(0), brush_.GetAddressOf());
    ++device_;
}

D2D1_SIZE_F Surface::size() const {
    RECT r{};
    GetClientRect(hwnd_, &r);
    return {r.right * 96.f / dpi_, r.bottom * 96.f / dpi_};
}

void Surface::set_root(Widget* root) {
    if (root_ == root) return;
    if (root_) {
        std::vector<Widget*> stack{root_};
        while (!stack.empty()) {
            Widget* w = stack.back(); stack.pop_back();
            w->place(*this, false);
            std::vector<Widget*> kids; w->children(kids);
            stack.insert(stack.end(), kids.begin(), kids.end());
        }
    }
    root_ = root;
    focus_ = hover_ = capture_ = nullptr;
    tip_text_.clear(); tip_shown_ = false;
    scroll_ = 0;
    relayout();
}

std::vector<Widget*> Surface::flatten(bool focusable_only) const {
    std::vector<Widget*> out;
    struct Walk {
        static void go(Widget* w, bool enabled, bool only, std::vector<Widget*>& out) {
            if (!w || !w->visible) return;
            enabled = enabled && w->enabled;
            if (!only || (w->focusable() && enabled)) out.push_back(w);
            std::vector<Widget*> kids; w->children(kids);
            for (Widget* k : kids) go(k, enabled, only, out);
        }
    };
    Walk::go(root_, true, focusable_only, out);
    return out;
}

void Surface::place_all() {
    struct Walk {
        static void go(Surface& s, Widget* w, bool shown) {
            shown = shown && w->visible;
            w->place(s, shown);
            std::vector<Widget*> kids; w->children(kids);
            for (Widget* k : kids) go(s, k, shown);
        }
    };
    if (root_) Walk::go(*this, root_, true);
}

void Surface::relayout() {
    if (!hwnd_) return;
    create_target();
    if (!rt_) return;
    D2D1_SIZE_F sz = size();
    Ctx c{rt_.Get(), kit, kit.theme(), brush_.Get(), device_};
    if (on_layout) doc_h_ = on_layout(c, sz);
    else if (root_) doc_h_ = root_->layout(c, 0, 0, sz.width);
    else doc_h_ = 0;
    scroll_ = scrollable_ ? std::clamp(scroll_, 0.f, std::max(0.f, doc_h_ - sz.height)) : 0;
    auto all = flatten(false);
    auto known = [&](Widget* w) { return w && std::find(all.begin(), all.end(), w) != all.end(); };
    if (!known(focus_)) focus_ = nullptr;
    if (!known(hover_)) hover_ = nullptr;
    if (capture_ && !known(capture_)) { capture_ = nullptr; if (GetCapture() == hwnd_) ReleaseCapture(); }
    place_all();
    invalidate();
}

void Surface::invalidate() { if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE); }

void Surface::changed(Widget* w) {
    if (w && w->on_change) { auto f = w->on_change; f(); }
    if (on_changed) on_changed(w);
    invalidate();
}

void Surface::notify(Widget* w, DWORD event) {
    long id = acc_ ? acc_->id_of(w) : CHILDID_SELF;
    if (id != CHILDID_SELF) NotifyWinEvent(event, hwnd_, OBJID_CLIENT, id);
}

void Surface::focus(Widget* w, bool keyboard) {
    if (auto p = dynamic_cast<Portal*>(w)) {
        focus_ = w;
        if (p->target) { p->target->keyboard_cues = keyboard; p->target->focus_edge(true); }
        return;
    }
    focus_ = w;
    keyboard_cues = keyboard;
    if (auto t = dynamic_cast<TextField*>(w); t && t->edit()) t->focus_native();
    else if (GetFocus() != hwnd_) SetFocus(hwnd_);
    if (w && keyboard) ensure_visible(w);
    if (w) notify(w, EVENT_OBJECT_FOCUS);
    invalidate();
}

void Surface::native_focus(Widget* w) {
    if (focus_ != w) { focus_ = w; ensure_visible(w); }
    if (on_focus_in) on_focus_in();
    invalidate();
}

void Surface::focus_edge(bool first) {
    auto list = flatten(true);
    if (list.empty()) { if (on_tab_out) on_tab_out(first); return; }
    Widget* w = first ? list.front() : list.back();
    if (auto p = dynamic_cast<Portal*>(w); p && p->target) { focus_ = w; p->target->focus_edge(first); return; }
    focus(w, keyboard_cues);
}

bool Surface::tab(bool forward) {
    auto list = flatten(true);
    auto it = std::find(list.begin(), list.end(), focus_);
    long index = it == list.end() ? (forward ? -1 : long(list.size())) : long(it - list.begin());
    long next = index + (forward ? 1 : -1);
    if (next < 0 || next >= long(list.size())) {
        if (on_tab_out) { on_tab_out(forward); return false; }
        if (list.empty()) return false;
        next = forward ? 0 : long(list.size()) - 1;
    }
    Widget* w = list[size_t(next)];
    if (auto p = dynamic_cast<Portal*>(w); p && p->target) {
        focus_ = w;
        p->target->keyboard_cues = true;
        p->target->focus_edge(forward);
        invalidate();
        return true;
    }
    focus(w, true);
    return true;
}

void Surface::ensure_visible(Widget* w) {
    if (!scrollable_ || !w) return;
    float view = size().height;
    if (w->rc.top - 12 < scroll_) scroll_to(w->rc.top - 12);
    else if (w->rc.bottom + 12 > scroll_ + view) scroll_to(std::min(w->rc.top - 12, w->rc.bottom + 12 - view));
}

void Surface::scroll_to(float y) {
    float max_scroll = std::max(0.f, doc_h_ - size().height);
    y = std::clamp(y, 0.f, max_scroll);
    if (std::abs(y - scroll_) < 0.01f) return;
    scroll_ = y;
    place_all();
    invalidate();
}

void Surface::set_dpi(UINT dpi) {
    dpi_ = dpi;
    if (rt_) rt_->SetDpi(float(dpi), float(dpi));
    restyle();
}

void Surface::restyle() {
    make_edit_resources();
    for (Widget* w : flatten(false)) w->restyle(*this);
    if (root_) {  // auch unsichtbare Felder
        std::vector<Widget*> stack{root_};
        while (!stack.empty()) {
            Widget* w = stack.back(); stack.pop_back();
            if (!w->visible) w->restyle(*this);
            std::vector<Widget*> kids; w->children(kids);
            stack.insert(stack.end(), kids.begin(), kids.end());
        }
    }
    relayout();
}

Rect Surface::thumb() const {
    D2D1_SIZE_F sz = size();
    if (!scrollable_ || doc_h_ <= sz.height + 1) return {0, 0, 0, 0};
    float h = std::max(32.f, sz.height * sz.height / doc_h_);
    float t = scroll_ / std::max(1.f, doc_h_ - sz.height);
    float top = 4 + (sz.height - 8 - h) * t;
    return {sz.width - 9, top, sz.width - 3, top + h};
}

Widget* Surface::hit(Point p) const {
    struct Walk {
        static Widget* go(Widget* w, Point p, bool enabled) {
            if (!w || !w->visible) return nullptr;
            enabled = enabled && w->enabled;
            std::vector<Widget*> kids; w->children(kids);
            for (auto it = kids.rbegin(); it != kids.rend(); ++it)
                if (Widget* found = go(*it, p, enabled)) return found;
            if (enabled && w->clickable() && inside(w->rc, p)) return w;
            return nullptr;
        }
    };
    return Walk::go(root_, p, true);
}

Point Surface::doc_point(LPARAM lp) const {
    return {GET_X_LPARAM(lp) * 96.f / dpi_, GET_Y_LPARAM(lp) * 96.f / dpi_ + scroll_};
}

constexpr UINT_PTR TIP_TIMER = 0x7A11;
constexpr UINT TIP_DELAY_MS = 500;

void Surface::tip_hide() {
    KillTimer(hwnd_, TIP_TIMER);
    tip_text_.clear();
    if (tip_shown_) { tip_shown_ = false; invalidate(); }
}

void Surface::tip_update(Widget* w, Point doc, LPARAM lp) {
    std::wstring text = w ? w->tip(doc, kit.english) : std::wstring();
    tip_at_ = {GET_X_LPARAM(lp) * 96.f / dpi_, GET_Y_LPARAM(lp) * 96.f / dpi_};
    if (text == tip_text_ && !text.empty()) return;   // gleicher Hinweis: nicht neu starten, Position bleibt
    tip_hide();
    if (text.empty()) return;
    tip_text_ = std::move(text);
    SetTimer(hwnd_, TIP_TIMER, TIP_DELAY_MS, nullptr);
}

void Surface::paint() {
    PAINTSTRUCT ps;
    BeginPaint(hwnd_, &ps);
    EndPaint(hwnd_, &ps);
    create_target();
    if (!rt_) return;
    const Theme& th = kit.theme();
    D2D1_SIZE_F sz = size();
    rt_->BeginDraw();
    rt_->SetTransform(D2D1::Matrix3x2F::Identity());
    rt_->Clear(th.bg);
    Ctx c{rt_.Get(), kit, th, brush_.Get(), device_};
    if (on_background) on_background(c, sz);
    rt_->SetTransform(D2D1::Matrix3x2F::Translation(0, -scroll_));
    Rect view{0, scroll_, sz.width, scroll_ + sz.height};
    struct Walk {
        static void go(Ctx& c, Widget* w, const Rect& view) {
            if (!w || !w->visible) return;
            if (w->rc.bottom < view.top - 20 || w->rc.top > view.bottom + 20) return;
            float saved = c.dim;
            if (!w->enabled) c.dim *= .45f;
            w->paint(c);
            std::vector<Widget*> kids; w->children(kids);
            for (Widget* k : kids) go(c, k, view);
            c.dim = saved;
        }
    };
    Walk::go(c, root_, view);
    HWND focused_window = GetFocus();
    bool has_focus = focused_window == hwnd_ || (focused_window && GetParent(focused_window) == hwnd_);
    if (focus_ && keyboard_cues && has_focus && focus_->wants_focus_ring() && focus_->visible && !dynamic_cast<TextField*>(focus_))
        c.frame(inflate(focus_->rc, 3), dynamic_cast<NavItem*>(focus_) && !th.dark ? th.on_accent : th.accent, 10, 2);
    rt_->SetTransform(D2D1::Matrix3x2F::Identity());
    Rect t = thumb();
    if (rw(t) > 0) c.fill(t, alpha(th.muted, thumb_drag_ ? .8f : .45f), 3);
    if (on_overlay) on_overlay(c, sz);
    if (tip_shown_ && !tip_text_.empty()) {
        const float tw = std::min(c.text_width(tip_text_, Font::Small), 320.f);
        const float th_ = c.text_height(tip_text_, tw, Font::Small);
        const float w = tw + 20, h = th_ + 12;
        float x = std::clamp(tip_at_.x + 12, 4.f, std::max(4.f, sz.width - w - 4));
        float y = tip_at_.y + 22;
        if (y + h > sz.height - 4) y = std::max(4.f, tip_at_.y - h - 8);
        Rect box{x, y, x + w, y + h};
        c.fill({box.left + 1, box.top + 2, box.right + 1, box.bottom + 3}, alpha(th.text, .12f), 6);   // leichter Schatten
        c.fill(box, th.card, 6);
        c.frame(box, th.line, 6, 1);
        c.text(tip_text_, {box.left + 10, box.top + 6, box.right - 10, box.bottom - 6}, Font::Small, th.text, DWRITE_TEXT_ALIGNMENT_LEADING, true, false);
    }
    if (rt_->EndDraw() == D2DERR_RECREATE_TARGET) {
        rt_.Reset(); brush_.Reset();
        invalidate();
    }
}

LRESULT Surface::handle(UINT msg, WPARAM wp, LPARAM lp) {
    if (on_message) {
        LRESULT result = 0;
        if (on_message(msg, wp, lp, result)) return result;
    }
    switch (msg) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paint(); return 0;
    case WM_SIZE:
        if (rt_) rt_->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        if (on_resize) on_resize();
        relayout();
        return 0;
    case WM_DPICHANGED: {
        auto r = reinterpret_cast<RECT*>(lp);
        dpi_ = HIWORD(wp);
        if (rt_) rt_->SetDpi(float(dpi_), float(dpi_));
        SetWindowPos(hwnd_, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
        restyle();
        return 0;
    }
    case WM_DPICHANGED_AFTERPARENT:
        set_dpi(GetDpiForWindow(hwnd_));
        return 0;
    case WM_GETDLGCODE: return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
    case WM_SETFOCUS:
        if (on_focus_in) on_focus_in();
        if (focus_) if (auto t = dynamic_cast<TextField*>(focus_); t && t->edit()) { t->focus_native(); return 0; }
        invalidate();
        return 0;
    case WM_KILLFOCUS: invalidate(); return 0;
    case WM_MOUSEMOVE: {
        Point p = doc_point(lp);
        if (!tracking_) {
            TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, hwnd_, 0};
            tracking_ = TrackMouseEvent(&t);
        }
        if (thumb_drag_) {
            D2D1_SIZE_F sz = size();
            float y = GET_Y_LPARAM(lp) * 96.f / dpi_ - thumb_grab_;
            float h = rh(thumb());
            scroll_to((y - 4) / std::max(1.f, sz.height - 8 - h) * (doc_h_ - sz.height));
            return 0;
        }
        if (capture_) { tip_hide(); capture_->drag(p, *this); return 0; }
        Widget* w = hit(p);
        if (w != hover_) {
            if (hover_) hover_->hot = false;
            hover_ = w;
            if (hover_) hover_->hot = true;
            invalidate();
        }
        tip_update(w, p, lp);
        return 0;
    }
    case WM_TIMER:
        if (wp == TIP_TIMER) {
            KillTimer(hwnd_, TIP_TIMER);
            if (!tip_text_.empty() && hover_) { tip_shown_ = true; invalidate(); }
            return 0;
        }
        break;
    case WM_MOUSELEAVE:
        tip_hide();
        tracking_ = false;
        if (hover_) { hover_->hot = false; hover_ = nullptr; invalidate(); }
        return 0;
    case WM_SETCURSOR:
        if (reinterpret_cast<HWND>(wp) == hwnd_ && LOWORD(lp) == HTCLIENT) {
            SetCursor(LoadCursorW(nullptr, hover_ && hover_->clickable() ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
        tip_hide();
        keyboard_cues = false;
        Point p = doc_point(lp);
        Rect t = thumb();
        if (rw(t) > 0 && GET_X_LPARAM(lp) * 96.f / dpi_ >= t.left - 4) {
            float y = GET_Y_LPARAM(lp) * 96.f / dpi_;
            if (y >= t.top && y <= t.bottom) { thumb_drag_ = true; thumb_grab_ = y - t.top; }
            else scroll_to(scroll_ + (y < t.top ? -1 : 1) * size().height * .9f);
            SetCapture(hwnd_);
            invalidate();
            return 0;
        }
        Widget* w = hit(p);
        if (!w) { if (GetFocus() != hwnd_) SetFocus(hwnd_); return 0; }
        capture_ = w;
        w->pressed = true;
        invalidate();
        SetCapture(hwnd_);
        if (w->focusable()) focus(w, false);
        w->press(p, *this);
        return 0;
    }
    case WM_LBUTTONUP: {
        // ReleaseCapture sends WM_CAPTURECHANGED synchronously, which clears both fields.
        Widget* w = capture_;
        if (w) w->pressed = false;
        invalidate();
        const bool dragged_thumb = thumb_drag_;
        capture_ = nullptr;
        thumb_drag_ = false;
        if (GetCapture() == hwnd_) ReleaseCapture();
        if (dragged_thumb) { invalidate(); return 0; }
        if (w) w->release(doc_point(lp), *this);
        return 0;
    }
    case WM_CAPTURECHANGED:
        if (capture_) capture_->pressed = false;
        capture_ = nullptr;
        invalidate();
        if (thumb_drag_) { thumb_drag_ = false; invalidate(); }
        return 0;
    case WM_MOUSEWHEEL:
        if (scrollable_) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            tip_hide();
            scroll_to(scroll_ - float(delta) / WHEEL_DELTA * 56.f);
            return 0;
        }
        break;
    case WM_KEYDOWN: case WM_SYSKEYDOWN: {
        const UINT vk = UINT(wp);
        if (msg == WM_SYSKEYDOWN && vk != VK_UP && vk != VK_DOWN) break;
        keyboard_cues = true;
        if (vk == VK_TAB) { tab(GetKeyState(VK_SHIFT) >= 0); return 0; }
        if (focus_ && focus_->live() && focus_->key(vk, *this)) { invalidate(); return 0; }
        if (scrollable_) {
            float view = size().height;
            switch (vk) {
            case VK_PRIOR: scroll_to(scroll_ - view * .9f); return 0;
            case VK_NEXT: scroll_to(scroll_ + view * .9f); return 0;
            case VK_HOME: scroll_to(0); return 0;
            case VK_END: scroll_to(doc_h_); return 0;
            case VK_UP: scroll_to(scroll_ - 40); return 0;
            case VK_DOWN: scroll_to(scroll_ + 40); return 0;
            }
        }
        invalidate();
        if (msg == WM_SYSKEYDOWN) break;
        return 0;
    }
    case WM_CHAR: return 0;
    case WM_COMMAND:
        if (HIWORD(wp) == EN_CHANGE && lp) {
            if (auto w = static_cast<Widget*>(GetPropW(reinterpret_cast<HWND>(lp), L"fl.w"))) changed(w);
            return 0;
        }
        break;
    case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wp);
        const Theme& th = kit.theme();
        SetTextColor(dc, colorref(th.text));
        SetBkColor(dc, colorref(th.input));
        return reinterpret_cast<LRESULT>(input_brush_);
    }
    case WM_GETOBJECT:
        if (static_cast<long>(lp) == OBJID_CLIENT && acc_) return LresultFromObject(IID_IAccessible, wp, acc_);
        break;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

// ---------------------------------------------------------------- Form
const ojson& Form::value(const char* sec, const char* key) const {
    static const ojson none;
    const ojson& cfg = host_->config();
    if (!cfg.contains(sec) || !cfg.at(sec).contains(key)) return none;
    return cfg.at(sec).at(key);
}
int Form::index_of(const std::vector<std::string>& values, const std::string& v) {
    auto it = std::find(values.begin(), values.end(), v);
    return it == values.end() ? 0 : int(it - values.begin());
}
static std::string as_text(const ojson& v) {
    if (v.is_string()) return v.get<std::string>();
    if (v.is_null()) return "";
    return v.dump();
}
Toggle* Form::toggle(Card& card, const char* sec, const char* key, Text label, Text desc) {
    const ojson& v = value(sec, key);
    auto w = card.row<Toggle>(std::move(label), std::move(desc), v.is_boolean() && v.get<bool>());
    bindings_.push_back({sec, key, Kind::Toggle, w});
    return w;
}
TextField* Form::text(Card& card, const char* sec, const char* key, Text label, Text desc, Text hint) {
    auto w = card.row<TextField>(std::move(label), std::move(desc), TextField::Single, widen(as_text(value(sec, key))));
    w->hint = std::move(hint);
    w->limit = 300;
    bindings_.push_back({sec, key, Kind::Text, w});
    return w;
}
TextField* Form::secret(Card& card, const char* sec, const char* key, Text label, Text desc) {
    auto w = card.row<TextField>(std::move(label), std::move(desc), TextField::Secret, L"");
    const ojson& set = value(sec, (std::string(key) + "_set").c_str());
    w->secret_set = set.is_boolean() && set.get<bool>();
    w->limit = 300;
    bindings_.push_back({sec, key, Kind::Secret, w});
    return w;
}
TextField* Form::number(Card& card, const char* sec, const char* key, Text label, Text desc, int minimum, int maximum) {
    auto w = card.row<TextField>(std::move(label), std::move(desc), TextField::Number, widen(as_text(value(sec, key))));
    bindings_.push_back({sec, key, Kind::Number, w, minimum, maximum});
    return w;
}
TextField* Form::list(Card& card, const char* sec, const char* key, Text label, Text desc, int lines) {
    std::wstring joined;
    const ojson& v = value(sec, key);
    if (v.is_array())
        for (const auto& item : v) { if (!joined.empty()) joined += L"\r\n"; joined += widen(as_text(item)); }
    auto w = card.wide_row<TextField>(std::move(label), std::move(desc), TextField::Multi, joined);
    w->lines = lines;
    bindings_.push_back({sec, key, Kind::List, w});
    return w;
}
Slider* Form::slider(Card& card, const char* sec, const char* key, Text label, Text desc, int minimum, int maximum, Text suffix) {
    const ojson& v = value(sec, key);
    int n = v.is_number() ? v.get<int>() : minimum;
    auto w = card.row<Slider>(std::move(label), std::move(desc), minimum, maximum, n, std::move(suffix));
    bindings_.push_back({sec, key, Kind::Slider, w, minimum, maximum});
    return w;
}
Segment* Form::segment(Card& card, const char* sec, const char* key, Text label, Text desc,
                       std::vector<std::string> values, std::vector<Text> labels) {
    int i = index_of(values, as_text(value(sec, key)));
    auto w = card.row<Segment>(std::move(label), std::move(desc), std::move(labels), i);
    bindings_.push_back({sec, key, Kind::Segment, w, 0, 0, std::move(values)});
    return w;
}
Combo* Form::combo(Card& card, const char* sec, const char* key, Text label, Text desc,
                   std::vector<std::string> values, std::vector<Text> labels) {
    int i = index_of(values, as_text(value(sec, key)));
    auto w = card.row<Combo>(std::move(label), std::move(desc), std::move(labels), i);
    bindings_.push_back({sec, key, Kind::Combo, w, 0, 0, std::move(values)});
    return w;
}
void Form::collect(ojson& patch) const {
    const bool en = host_ && host_->english();
    for (const auto& b : bindings_) {
        auto& section = patch[b.sec];
        switch (b.kind) {
        case Kind::Toggle: section[b.key] = static_cast<Toggle*>(b.w)->on; break;
        case Kind::Slider: section[b.key] = static_cast<Slider*>(b.w)->val; break;
        case Kind::Segment: section[b.key] = b.values.at(size_t(static_cast<Segment*>(b.w)->index)); break;
        case Kind::Combo: section[b.key] = b.values.at(size_t(static_cast<Combo*>(b.w)->index)); break;
        case Kind::Text: section[b.key] = trim(narrow(static_cast<TextField*>(b.w)->text())); break;
        case Kind::Secret: {
            std::string t = trim(narrow(static_cast<TextField*>(b.w)->text()));
            if (!t.empty()) section[b.key] = t;  // leer = gespeicherten Wert behalten
            break;
        }
        case Kind::Number: {
            std::string t = trim(narrow(static_cast<TextField*>(b.w)->text()));
            size_t end = 0;
            int n = 0;
            try { n = std::stoi(t, &end); } catch (...) { end = std::string::npos; }
            if (t.empty() || end != t.size())
                throw Invalid(en ? "Enter a valid number." : "Bitte eine gültige Zahl eingeben.", b.w);
            if (n < b.minimum || n > b.maximum)
                throw Invalid(strprintf(en ? "Allowed range: %d to %d." : "Erlaubt sind %d bis %d.", b.minimum, b.maximum), b.w);
            section[b.key] = n;
            break;
        }
        case Kind::List: {
            section[b.key] = ojson::array();
            std::istringstream lines(narrow(static_cast<TextField*>(b.w)->text()));
            std::string line;
            while (std::getline(lines, line)) if (!trim(line).empty()) section[b.key].push_back(trim(line));
            break;
        }
        }
    }
}

// ---------------------------------------------------------------- Helfer
Card* placeholder(Host& host, Column& root, Text title, Text text) {
    Card* card = root.add<Card>(std::move(title), std::move(text));
    card->placeholder = true;
    card->add<Note>(T(L"Noch nicht nativ umgesetzt.", L"Not native yet."), Note::Soon);
    (void)host;  // der frühere Übergangsweg ins Browser-Fenster ist entfernt (alle Seiten sind nativ)
    return card;
}

std::wstring source_title(const std::string& key, bool en) {
    if (key == "system") return L"System";
    if (key == "friends") return en ? L"Friends" : L"Freunde";
    if (key == "discord") return L"Discord";
    if (key == "bf6") return L"Battlefield 6";
    if (key == "rss") return en ? L"Blog updates (RSS)" : L"Blog-Updates (RSS)";
    if (key == "streaming") return en ? L"New on streaming" : L"Streaming-Neustarts";
    if (key == "homeassistant") return L"Home Assistant";
    if (key == "spotify") return L"Spotify";
    return widen(key);
}

int count_dirty(Widget* root) {
    if (!root) return 0;
    int n = root->track_dirty && root->dirty() ? 1 : 0;
    std::vector<Widget*> kids;
    root->children(kids);
    for (Widget* k : kids) n += count_dirty(k);
    return n;
}

}  // namespace fl::ui
