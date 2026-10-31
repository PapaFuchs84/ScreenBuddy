// © 2026 FuchsLabs
// Zeichnen mit Direct2D/DirectWrite in ein Speicherbild (WIC), Bilder laden/speichern mit WIC.
// Koordinaten und Formen folgen den Regeln von Pillow (Version 1.x): Rechtecke schließen x1/y1 ein,
// Texte haben Anker wie "lm" (links, Mitte) – so lassen sich die Seiten 1:1 übertragen.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fl {

struct Color {
    uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Color&) const = default;
};

// Bild mit vormultipliziertem Alpha, Pixel als 0xAARRGGBB (Speicherreihenfolge B, G, R, A)
struct Image {
    int w = 0, h = 0;
    std::vector<uint32_t> px;
    Image() = default;
    Image(int w_, int h_, Color c = {}, uint8_t a = 255);
    bool empty() const { return w == 0 || h == 0; }
    uint32_t at(int x, int y) const { return px[size_t(y) * w + x]; }
};
using ImagePtr = std::shared_ptr<const Image>;

void gfx_init();  // einmal pro Prozess, vor dem ersten Zeichnen

// Bilder: nur gängige Webformate (PNG, JPEG, GIF, WebP) – fremde Daten, kleine Angriffsfläche
std::optional<Image> decode_image(const std::string& bytes);
std::string encode_png(const Image& img);
std::string encode_jpeg(const Image& img, float quality = 0.8f);  // Companion-App (Qualität 0..1)
Image resize(const Image& img, int w, int h);           // hochwertig (Fant/Kubisch)
Image thumbnail(const Image& img, int max_w, int max_h);  // Seitenverhältnis bleibt, nur verkleinern
void paste(Image& dst, const Image& src, int x, int y);   // ohne Mischen (Kacheln)

struct Font {
    float size = 12;   // Pixel
    bool bold = false; // Bahnschrift SemiBold, sonst Segoe UI
    std::wstring family; // optional installed design-system font; falls back to Segoe UI
};

struct PointF {
    float x, y;
};

class Canvas {
public:
    // w/h: echte Pixel; s: Vergrößerung gegenüber den logischen Koordinaten (480 × 320 = 1)
    Canvas(int w, int h, float s, Color bg, uint8_t bg_alpha = 255);
    ~Canvas();
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    float s() const { return s_; }

    // Formen in logischen Koordinaten (wie Pillow: x1/y1 gehören zum Rechteck)
    void rectangle(float x0, float y0, float x1, float y1, Color fill);
    void rounded_rectangle(float x0, float y0, float x1, float y1, float radius, Color fill);
    void ellipse(float x0, float y0, float x1, float y1, Color fill);
    void ellipse_outline(float x0, float y0, float x1, float y1, Color col, float width);
    void line(const std::vector<PointF>& pts, Color col, float width = 1);
    void polygon(const std::vector<PointF>& pts, Color fill);

    // Text: anchor wie Pillow ("lm", "mm", "rm"), Text wird vorher übersetzt (i18n)
    void text(float x, float y, const std::string& text, const Font& f, Color col, const char* anchor = "la");
    float textlength(const std::string& text, const Font& f);  // logische Breite

    // Bilder an logischer Stelle (Größe in echten Pixeln wie geliefert)
    void image(const Image& img, float x, float y, float opacity = 1.f);
    // rundes Bild (Avatar) mit Durchmesser d in echten Pixeln
    void round_image(const Image& img, float x, float y, int d, float opacity = 1.f);
    // Fuchs-Logo, size in echten Pixeln, Lage logisch
    void fox(float x, float y, int size, bool badge);

    Image finish();

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
    float s_;
};

// Fuchs-Logo als eigenes Bild (Tray, Installer)
Image fox_image(int size, bool badge = true);

}  // namespace fl
