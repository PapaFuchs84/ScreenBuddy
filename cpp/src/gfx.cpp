// © 2026 FuchsLabs
#include "gfx.h"
#include <tuple>

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>

#include "brand.h"
#include "i18n.h"
#include "util.h"

using Microsoft::WRL::ComPtr;

namespace fl {

namespace {

ComPtr<ID2D1Factory> g_d2d;
ComPtr<IDWriteFactory> g_dw;
ComPtr<IWICImagingFactory> g_wic;
std::once_flag g_once;

void ensure_com() {
    thread_local bool done = false;
    if (!done) {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        done = true;
    }
}

void check(HRESULT hr, const char* what) {
    if (FAILED(hr)) throw std::runtime_error(strprintf("%s (0x%08X)", what, (unsigned)hr));
}

D2D1_COLOR_F col(Color c, float a = 1.f) { return D2D1::ColorF(c.r / 255.f, c.g / 255.f, c.b / 255.f, a); }

// Python rundet halbe Werte zur geraden Zahl (round(2.5) = 2) – für gleiche Lage wie Version 1.x
float pyround(float v) { return std::nearbyint(v); }

struct FontInfo {
    ComPtr<IDWriteTextFormat> fmt;
    float asc = 0, desc = 0;  // Pixel, beide positiv
};

std::mutex g_font_mutex;
std::map<std::tuple<bool, int, std::wstring>, FontInfo> g_fonts;

const FontInfo& font_info(const Font& f) {
    std::lock_guard lk(g_font_mutex);
    auto key = std::make_tuple(f.bold, (int)std::lround(f.size * 100), f.family);
    if (auto it = g_fonts.find(key); it != g_fonts.end()) return it->second;
    FontInfo fi;
    ComPtr<IDWriteFontCollection> coll;
    check(g_dw->GetSystemFontCollection(&coll), "Schriften");
    // CI-Schrift für Titel, Namen, Zahlen: Bahnschrift SemiBold (ab Windows 10 dabei); sonst Segoe UI
    std::wstring family = !f.family.empty() ? f.family : f.bold ? L"Bahnschrift" : L"Segoe UI";
    DWRITE_FONT_WEIGHT weight = f.bold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    float size = f.bold ? f.size + 1 : f.size;  // wie Version 1.x (Bahnschrift eine Stufe größer)
    UINT32 idx = 0;
    BOOL exists = FALSE;
    coll->FindFamilyName(family.c_str(), &idx, &exists);
    if (!exists) {
        family = L"Segoe UI";
        weight = f.family.empty() ? DWRITE_FONT_WEIGHT_BOLD : f.bold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        size = f.size;
        coll->FindFamilyName(family.c_str(), &idx, &exists);
    }
    if (exists) {
        ComPtr<IDWriteFontFamily> fam;
        ComPtr<IDWriteFont> font;
        if (SUCCEEDED(coll->GetFontFamily(idx, &fam)) &&
            SUCCEEDED(fam->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font))) {
            DWRITE_FONT_METRICS m;
            font->GetMetrics(&m);
            fi.asc = size * m.ascent / m.designUnitsPerEm;
            fi.desc = size * m.descent / m.designUnitsPerEm;
        }
    }
    if (fi.asc == 0) {
        fi.asc = size * 0.9f;
        fi.desc = size * 0.25f;
    }
    check(g_dw->CreateTextFormat(family.c_str(), nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                 size, L"de-de", &fi.fmt),
          "Schriftformat");
    fi.fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return g_fonts.emplace(key, std::move(fi)).first->second;
}

ComPtr<IDWriteTextLayout> layout(const std::wstring& w, const FontInfo& fi) {
    ComPtr<IDWriteTextLayout> l;
    check(g_dw->CreateTextLayout(w.c_str(), (UINT32)w.size(), fi.fmt.Get(), 100000.f, 10000.f, &l), "Textlayout");
    return l;
}

ComPtr<IWICBitmap> wic_from(const Image& img) {
    ComPtr<IWICBitmap> b;
    check(g_wic->CreateBitmapFromMemory(img.w, img.h, GUID_WICPixelFormat32bppPBGRA, img.w * 4,
                                        (UINT)(img.px.size() * 4), (BYTE*)img.px.data(), &b),
          "Bild");
    return b;
}

Image image_from(IWICBitmapSource* src) {
    UINT w = 0, h = 0;
    src->GetSize(&w, &h);
    Image out;
    out.w = (int)w;
    out.h = (int)h;
    out.px.resize(size_t(w) * h);
    WICRect r{0, 0, (INT)w, (INT)h};
    check(src->CopyPixels(&r, w * 4, (UINT)(out.px.size() * 4), (BYTE*)out.px.data()), "Bildpunkte");
    return out;
}

}  // namespace

Image::Image(int w_, int h_, Color c, uint8_t a) : w(w_), h(h_) {
    float fa = a / 255.f;
    uint32_t v = (uint32_t)a << 24 | (uint32_t)std::lround(c.r * fa) << 16 | (uint32_t)std::lround(c.g * fa) << 8 |
                 (uint32_t)std::lround(c.b * fa);
    px.assign(size_t(w) * h, v);
}

void gfx_init() {
    ensure_com();
    std::call_once(g_once, [] {
        D2D1_FACTORY_OPTIONS opt{};
        check(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory), &opt, (void**)g_d2d.GetAddressOf()),
              "Direct2D");
        check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)g_dw.GetAddressOf()),
              "DirectWrite");
        check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_wic)), "WIC");
    });
}

// ---------- Bilder ----------
std::optional<Image> decode_image(const std::string& bytes) {
    ensure_com();
    if (bytes.size() < 12) return std::nullopt;
    auto b = (const unsigned char*)bytes.data();
    // Format selbst erkennen und nur diesen Decoder benutzen (keine weiteren installierten Codecs)
    GUID fmt;
    if (!memcmp(b, "\x89PNG\r\n\x1a\n", 8)) fmt = GUID_ContainerFormatPng;
    else if (b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) fmt = GUID_ContainerFormatJpeg;
    else if (!memcmp(b, "GIF87a", 6) || !memcmp(b, "GIF89a", 6)) fmt = GUID_ContainerFormatGif;
    else if (!memcmp(b, "RIFF", 4) && !memcmp(b + 8, "WEBP", 4)) fmt = GUID_ContainerFormatWebp;
    else return std::nullopt;
    try {
        ComPtr<IWICStream> st;
        check(g_wic->CreateStream(&st), "Stream");
        check(st->InitializeFromMemory((BYTE*)bytes.data(), (DWORD)bytes.size()), "Stream");
        ComPtr<IWICBitmapDecoder> dec;
        check(g_wic->CreateDecoder(fmt, nullptr, &dec), "Decoder");
        check(dec->Initialize(st.Get(), WICDecodeMetadataCacheOnDemand), "Decoder");
        ComPtr<IWICBitmapFrameDecode> frame;
        check(dec->GetFrame(0, &frame), "Frame");
        UINT w = 0, h = 0;
        frame->GetSize(&w, &h);
        if (w == 0 || h == 0 || w > 4096 || h > 4096) return std::nullopt;  // Schutz vor Riesenbildern
        ComPtr<IWICFormatConverter> conv;
        check(g_wic->CreateFormatConverter(&conv), "Konverter");
        check(conv->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0,
                               WICBitmapPaletteTypeCustom),
              "Konverter");
        return image_from(conv.Get());
    } catch (const std::exception& e) {
        LOGD("gfx", "Bild nicht lesbar: %s", e.what());
        return std::nullopt;
    }
}

std::string encode_png(const Image& img) {
    ensure_com();
    ComPtr<IStream> stream;
    check(CreateStreamOnHGlobal(nullptr, TRUE, &stream), "Stream");
    ComPtr<IWICBitmapEncoder> enc;
    check(g_wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc), "PNG");
    check(enc->Initialize(stream.Get(), WICBitmapEncoderNoCache), "PNG");
    ComPtr<IWICBitmapFrameEncode> frame;
    check(enc->CreateNewFrame(&frame, nullptr), "PNG");
    check(frame->Initialize(nullptr), "PNG");
    check(frame->SetSize(img.w, img.h), "PNG");
    bool opaque = true;
    for (uint32_t p : img.px)
        if ((p >> 24) != 255) { opaque = false; break; }
    WICPixelFormatGUID pf = opaque ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
    check(frame->SetPixelFormat(&pf), "PNG");
    auto bmp = wic_from(img);
    ComPtr<IWICFormatConverter> conv;
    check(g_wic->CreateFormatConverter(&conv), "Konverter");
    check(conv->Initialize(bmp.Get(), pf, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom), "Konverter");
    check(frame->WriteSource(conv.Get(), nullptr), "PNG");
    check(frame->Commit(), "PNG");
    check(enc->Commit(), "PNG");
    HGLOBAL hg = nullptr;
    GetHGlobalFromStream(stream.Get(), &hg);
    STATSTG stat{};
    stream->Stat(&stat, STATFLAG_NONAME);
    std::string out((size_t)stat.cbSize.QuadPart, '\0');
    void* p = GlobalLock(hg);
    memcpy(out.data(), p, out.size());
    GlobalUnlock(hg);
    return out;
}

std::string encode_jpeg(const Image& img, float quality) {
    // Companion-App: Vollbild als JPEG (WIC-JPEG-Encoder nimmt 24 bpp BGR; Alpha fällt weg, die Seiten sind deckend)
    ensure_com();
    ComPtr<IStream> stream;
    check(CreateStreamOnHGlobal(nullptr, TRUE, &stream), "Stream");
    ComPtr<IWICBitmapEncoder> enc;
    check(g_wic->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &enc), "JPEG");
    check(enc->Initialize(stream.Get(), WICBitmapEncoderNoCache), "JPEG");
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;
    check(enc->CreateNewFrame(&frame, &props), "JPEG");
    PROPBAG2 opt{};
    opt.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
    VARIANT v{};  // entspricht VariantInit (ohne oleaut32)
    v.vt = VT_R4;
    v.fltVal = quality < 0.f ? 0.f : quality > 1.f ? 1.f : quality;
    check(props->Write(1, &opt, &v), "JPEG-Qualität");
    check(frame->Initialize(props.Get()), "JPEG");
    check(frame->SetSize(img.w, img.h), "JPEG");
    WICPixelFormatGUID pf = GUID_WICPixelFormat24bppBGR;
    check(frame->SetPixelFormat(&pf), "JPEG");
    auto bmp = wic_from(img);
    ComPtr<IWICFormatConverter> conv;
    check(g_wic->CreateFormatConverter(&conv), "Konverter");
    check(conv->Initialize(bmp.Get(), pf, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom), "Konverter");
    check(frame->WriteSource(conv.Get(), nullptr), "JPEG");
    check(frame->Commit(), "JPEG");
    check(enc->Commit(), "JPEG");
    HGLOBAL hg = nullptr;
    GetHGlobalFromStream(stream.Get(), &hg);
    STATSTG stat{};
    stream->Stat(&stat, STATFLAG_NONAME);
    std::string out((size_t)stat.cbSize.QuadPart, '\0');
    void* p = GlobalLock(hg);
    memcpy(out.data(), p, out.size());
    GlobalUnlock(hg);
    return out;
}

Image resize(const Image& img, int w, int h) {
    ensure_com();
    if (img.w == w && img.h == h) return img;
    auto bmp = wic_from(img);
    ComPtr<IWICBitmapScaler> sc;
    check(g_wic->CreateBitmapScaler(&sc), "Skalierer");
    // Verkleinern: Fant (flächenmittelnd, wie Lanczos sauber ohne Moiré); Vergrößern: hochwertig kubisch
    auto mode = (w < img.w && h < img.h) ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic;
    check(sc->Initialize(bmp.Get(), w, h, mode), "Skalierer");
    return image_from(sc.Get());
}

Image thumbnail(const Image& img, int max_w, int max_h) {
    if (img.w <= max_w && img.h <= max_h) return img;
    double r = std::min((double)max_w / img.w, (double)max_h / img.h);
    return resize(img, std::max(1, (int)std::lround(img.w * r)), std::max(1, (int)std::lround(img.h * r)));
}

void paste(Image& dst, const Image& src, int x, int y) {
    int start_y = std::max(0, -y);
    int end_y = std::min(src.h, dst.h - y);
    if (start_y >= end_y) return;

    int start_x = std::max(0, -x);
    int end_x = std::min(src.w, dst.w - x);
    if (start_x >= end_x) return;

    size_t copy_pixels = (size_t)(end_x - start_x);
    for (int yy = start_y; yy < end_y; ++yy) {
        int ty = y + yy;
        std::memcpy(&dst.px[size_t(ty) * dst.w + (x + start_x)],
                    &src.px[size_t(yy) * src.w + start_x],
                    copy_pixels * sizeof(uint32_t));
    }
}

// ---------- Fuchs ----------
namespace {
using Pts = std::vector<PointF>;
// Fuchskopf im 64er-Raster
const Pts HEAD = {{9, 7}, {25, 19}, {39, 19}, {55, 7}, {55, 31}, {32, 57}, {9, 31}};
const Pts EARS[] = {{{13, 14}, {22, 21}, {13, 26}}, {{51, 14}, {42, 21}, {51, 26}}};
const Pts CHEEKS[] = {{{9, 31}, {27, 38}, {32, 57}}, {{55, 31}, {37, 38}, {32, 57}}};
const Pts INK[] = {{{28, 51}, {36, 51}, {32, 57}},                 // Nase
                   {{21, 30}, {25, 27}, {29, 30}, {25, 32}},       // Augen
                   {{35, 30}, {39, 27}, {43, 30}, {39, 32}}};

void fill_poly(ID2D1RenderTarget* rt, const Pts& pts, float ox, float oy, float k, Color c) {
    ComPtr<ID2D1PathGeometry> g;
    g_d2d->CreatePathGeometry(&g);
    ComPtr<ID2D1GeometrySink> sink;
    g->Open(&sink);
    sink->BeginFigure(D2D1::Point2F(ox + pts[0].x * k, oy + pts[0].y * k), D2D1_FIGURE_BEGIN_FILLED);
    for (size_t i = 1; i < pts.size(); ++i) sink->AddLine(D2D1::Point2F(ox + pts[i].x * k, oy + pts[i].y * k));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    ComPtr<ID2D1SolidColorBrush> br;
    rt->CreateSolidColorBrush(col(c), &br);
    rt->FillGeometry(g.Get(), br.Get());
}

void draw_fox(ID2D1RenderTarget* rt, float x, float y, int size, bool badge) {
    float pad = 0;
    float s = (float)size;
    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if (badge) {
        ComPtr<ID2D1SolidColorBrush> br;
        rt->CreateSolidColorBrush(col(brand::ORANGE), &br);
        float r = s * 0.22f;
        rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + s, y + s), r, r), br.Get());
        pad = size >= 24 ? 0.08f : 0.03f;
    }
    float k = s * (1 - 2 * pad) / 64, o = s * pad;
    fill_poly(rt, HEAD, x + o, y + o, k, brand::CREME);
    if (size >= 24)
        for (auto& e : EARS) fill_poly(rt, e, x + o, y + o, k, brand::GLUT);
    for (auto& c : CHEEKS) fill_poly(rt, c, x + o, y + o, k, brand::PFIRSICH);
    for (auto& p : INK) fill_poly(rt, p, x + o, y + o, k, brand::NAVY);
}
}  // namespace

// ---------- Leinwand ----------
struct Canvas::Impl {
    int w, h;
    ComPtr<IWICBitmap> bmp;
    ComPtr<ID2D1RenderTarget> rt;
    struct BrushEntry {
        uint32_t color = 0;
        ComPtr<ID2D1SolidColorBrush> brush;
    };
    BrushEntry brushes[16]{};
    int brush_count = 0, next_evict = 0;
    bool done = false;

    ID2D1SolidColorBrush* brush(Color c) {
        uint32_t k = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | (uint32_t)c.b;
        for (int i = 0; i < brush_count; ++i) {
            if (brushes[i].color == k) return brushes[i].brush.Get();
        }
        ComPtr<ID2D1SolidColorBrush> b;
        rt->CreateSolidColorBrush(col(c), &b);
        if (!b) return nullptr;
        // Cache voll: ältesten Eintrag ersetzen, damit der Cache den Pinsel weiter besitzt (sonst Zeiger auf zerstörten Pinsel)
        const int slot = brush_count < 16 ? brush_count++ : next_evict++ % 16;
        brushes[slot] = {k, b};
        return brushes[slot].brush.Get();
    }
    ComPtr<ID2D1Bitmap> bitmap(const Image& img) {
        ComPtr<ID2D1Bitmap> b;
        auto props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        check(rt->CreateBitmap(D2D1::SizeU(img.w, img.h), img.px.data(), img.w * 4, props, &b), "Bitmap");
        return b;
    }
};

Canvas::Canvas(int w, int h, float s, Color bg, uint8_t bg_alpha) : d_(std::make_unique<Impl>()), s_(s) {
    gfx_init();
    d_->w = w;
    d_->h = h;
    check(g_wic->CreateBitmap(w, h, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &d_->bmp), "Bild");
    auto props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
                                              D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                                              96.f, 96.f);
    check(g_d2d->CreateWicBitmapRenderTarget(d_->bmp.Get(), props, &d_->rt), "Zeichenfläche");
    // Graustufen-Glättung: das Display hat keine Subpixel, ClearType gäbe Farbsäume
    d_->rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    ComPtr<IDWriteRenderingParams> rp;
    if (SUCCEEDED(g_dw->CreateCustomRenderingParams(1.8f, 0.5f, 0.f, DWRITE_PIXEL_GEOMETRY_FLAT,
                                                    DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, &rp)))
        d_->rt->SetTextRenderingParams(rp.Get());
    d_->rt->BeginDraw();
    d_->rt->Clear(col(bg, bg_alpha / 255.f));
}

Canvas::~Canvas() {
    if (!d_->done) d_->rt->EndDraw();
}

void Canvas::rectangle(float x0, float y0, float x1, float y1, Color fill) {
    float s = s_;
    if (s != 1) { x0 = pyround(x0 * s); y0 = pyround(y0 * s); x1 = pyround(x1 * s); y1 = pyround(y1 * s); }
    if (x1 < x0 || y1 < y0) return;
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    d_->rt->FillRectangle(D2D1::RectF(std::floor(x0), std::floor(y0), std::floor(x1) + 1, std::floor(y1) + 1),
                          d_->brush(fill));
}

void Canvas::rounded_rectangle(float x0, float y0, float x1, float y1, float radius, Color fill) {
    float s = s_;
    if (s != 1) {
        x0 = pyround(x0 * s); y0 = pyround(y0 * s); x1 = pyround(x1 * s); y1 = pyround(y1 * s);
        if (radius) radius = std::max(1.f, pyround(radius * s));
    }
    if (x1 < x0 || y1 < y0) return;
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    float r = std::min(radius, std::min(x1 - x0 + 1, y1 - y0 + 1) / 2);
    d_->rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x0, y0, x1 + 1, y1 + 1), r, r), d_->brush(fill));
}

void Canvas::ellipse(float x0, float y0, float x1, float y1, Color fill) {
    float s = s_;
    if (s != 1) { x0 = pyround(x0 * s); y0 = pyround(y0 * s); x1 = pyround(x1 * s); y1 = pyround(y1 * s); }
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    float rx = (x1 + 1 - x0) / 2, ry = (y1 + 1 - y0) / 2;
    d_->rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x0 + rx, y0 + ry), rx, ry), d_->brush(fill));
}

void Canvas::ellipse_outline(float x0, float y0, float x1, float y1, Color c, float width) {
    float s = s_;
    if (s != 1) {
        x0 = pyround(x0 * s); y0 = pyround(y0 * s); x1 = pyround(x1 * s); y1 = pyround(y1 * s);
        width = std::max(1.f, pyround(width * s));
    }
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    float rx = (x1 + 1 - x0) / 2, ry = (y1 + 1 - y0) / 2;  // Pillow zeichnet den Rand innerhalb des Rechtecks
    d_->rt->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x0 + rx, y0 + ry), rx - width / 2, ry - width / 2), d_->brush(c),
                        width);
}

void Canvas::line(const std::vector<PointF>& in, Color c, float width) {
    if (in.size() < 2) return;
    float s = s_;
    std::vector<PointF> pts = in;
    if (s != 1) {
        for (auto& p : pts) { p.x = pyround(p.x * s); p.y = pyround(p.y * s); }
        width = std::max(1.f, pyround(width * s));
    }
    // Waagerechte/senkrechte Strecken pixelgenau wie Pillow (Pixelreihe einschließlich Endpunkt)
    if (pts.size() == 2 && (pts[0].x == pts[1].x || pts[0].y == pts[1].y)) {
        float half = std::floor((width - 1) / 2);
        float xa = std::min(pts[0].x, pts[1].x), xb = std::max(pts[0].x, pts[1].x);
        float ya = std::min(pts[0].y, pts[1].y), yb = std::max(pts[0].y, pts[1].y);
        d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
        D2D1_RECT_F r = pts[0].y == pts[1].y
                            ? D2D1::RectF(std::floor(xa), std::floor(ya) - half, std::floor(xb) + 1, std::floor(ya) - half + width)
                            : D2D1::RectF(std::floor(xa) - half, std::floor(ya), std::floor(xa) - half + width, std::floor(yb) + 1);
        d_->rt->FillRectangle(r, d_->brush(c));
        return;
    }
    ComPtr<ID2D1PathGeometry> g;
    g_d2d->CreatePathGeometry(&g);
    ComPtr<ID2D1GeometrySink> sink;
    g->Open(&sink);
    sink->BeginFigure(D2D1::Point2F(pts[0].x + .5f, pts[0].y + .5f), D2D1_FIGURE_BEGIN_HOLLOW);
    for (size_t i = 1; i < pts.size(); ++i) sink->AddLine(D2D1::Point2F(pts[i].x + .5f, pts[i].y + .5f));
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    ComPtr<ID2D1StrokeStyle> style;
    g_d2d->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
                                                         D2D1_LINE_JOIN_ROUND),
                             nullptr, 0, &style);
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    d_->rt->DrawGeometry(g.Get(), d_->brush(c), width, style.Get());
}

void Canvas::polygon(const std::vector<PointF>& in, Color fill) {
    if (in.size() < 3) return;
    float s = s_;
    ComPtr<ID2D1PathGeometry> g;
    g_d2d->CreatePathGeometry(&g);
    ComPtr<ID2D1GeometrySink> sink;
    g->Open(&sink);
    auto P = [&](const PointF& p) {
        float x = s != 1 ? pyround(p.x * s) : p.x, y = s != 1 ? pyround(p.y * s) : p.y;
        return D2D1::Point2F(x + .5f, y + .5f);
    };
    sink->BeginFigure(P(in[0]), D2D1_FIGURE_BEGIN_FILLED);
    for (size_t i = 1; i < in.size(); ++i) sink->AddLine(P(in[i]));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    d_->rt->FillGeometry(g.Get(), d_->brush(fill));
}

float Canvas::textlength(const std::string& text, const Font& f) {
    std::string t = tr(text);
    if (t.empty()) return 0;
    auto& fi = font_info(f);
    auto l = layout(widen(t), fi);
    DWRITE_TEXT_METRICS m;
    l->GetMetrics(&m);
    return m.widthIncludingTrailingWhitespace / s_;
}

void Canvas::text(float x, float y, const std::string& text, const Font& f, Color c, const char* anchor) {
    std::string t = tr(text);
    if (t.empty()) return;
    auto& fi = font_info(f);
    auto l = layout(widen(t), fi);
    DWRITE_TEXT_METRICS m;
    l->GetMetrics(&m);
    DWRITE_LINE_METRICS lm;
    UINT32 n = 0;
    l->GetLineMetrics(&lm, 1, &n);
    float px = x * s_, py = y * s_;
    float wdt = m.widthIncludingTrailingWhitespace;
    if (anchor[0] == 'm') px -= wdt / 2;
    else if (anchor[0] == 'r') px -= wdt;
    float baseline;  // Grundlinie in Pixeln
    switch (anchor[1]) {
        case 'm': baseline = py + (fi.asc - fi.desc) / 2; break;
        case 's': baseline = py; break;
        case 'd': baseline = py - fi.desc; break;
        default: baseline = py + fi.asc; break;  // 'a': Oberkante der Oberlängen
    }
    d_->rt->DrawTextLayout(D2D1::Point2F(px, baseline - lm.baseline), l.Get(), d_->brush(c),
                           D2D1_DRAW_TEXT_OPTIONS_NONE);
}

void Canvas::image(const Image& img, float x, float y, float opacity) {
    if (img.empty()) return;
    auto b = d_->bitmap(img);
    float px = pyround(x * s_), py = pyround(y * s_);
    d_->rt->DrawBitmap(b.Get(), D2D1::RectF(px, py, px + img.w, py + img.h), opacity,
                       D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}

void Canvas::round_image(const Image& img, float x, float y, int dsz, float opacity) {
    if (img.empty()) return;
    Image sc = resize(img, dsz, dsz);
    auto b = d_->bitmap(sc);
    float px = pyround(x * s_), py = pyround(y * s_);
    ComPtr<ID2D1BitmapBrush> br;
    check(d_->rt->CreateBitmapBrush(b.Get(), &br), "Pinsel");
    br->SetTransform(D2D1::Matrix3x2F::Translation(px, py));
    br->SetOpacity(opacity);
    d_->rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    float r = dsz / 2.f;
    d_->rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(px + r, py + r), r, r), br.Get());
}

void Canvas::fox(float x, float y, int size, bool badge) {
    draw_fox(d_->rt.Get(), pyround(x * s_), pyround(y * s_), size, badge);
}

Image Canvas::finish() {
    if (!d_->done) {
        d_->rt->EndDraw();
        d_->done = true;
    }
    return image_from(d_->bmp.Get());
}

Image fox_image(int size, bool badge) {
    Canvas c(size, size, 1, {0, 0, 0}, 0);
    c.fox(0, 0, size, badge);
    return c.finish();
}

}  // namespace fl
