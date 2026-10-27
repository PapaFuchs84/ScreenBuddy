// © 2026 FuchsLabs
// ui_kit – selbst gezeichnetes UI-Kit des nativen Einstellungsfensters (Direct2D/DirectWrite, ohne Browser).
//
// ÜBERBLICK
//   Kit      geteilte Fabriken (D2D/DirectWrite), Schriften, Sprache (DE/EN) und Hell/Dunkel.
//   Surface  ein Fenster (HWND) mit eigenem D2D-Renderziel (doppelt gepuffert), Widget-Baum, Fokus/Tab, Bildlauf,
//            MSAA-Barrierefreiheit (IAccessible → von UIA/Sprachausgabe gelesen) und Per-Monitor-v2-DPI.
//            Alle Koordinaten sind DIP (1/96 Zoll); Textfelder sind echte EDIT-Fenster (IME, Zwischenablage).
//   Widget   Baustein: Column, Card, Row, Heading, Tiles, Label, Note, Toggle, Slider, Segment, Combo, Button,
//            ButtonRow, TextField (Text/Passwort/Zahl/mehrzeilig), ItemList (Auswahl, Ein/Aus, Reihenfolge),
//            ImageView, NavItem. Texte sind immer Paare (Text{de, en}) und folgen der Sprache sofort.
//   Form     bindet Widgets an Konfigurationsschlüssel (section.key) und schreibt beim Speichern den Patch.
//
// SEITEN-SCHNITTSTELLE (für weitere Seiten, z. B. cpp/src/ui_page_<name>.cpp)
//   1. Klasse von ui::Page ableiten:
//        PageInfo info() const            – id ("discord"), Gruppe in der Navigation, Titel, Untertitel.
//        void build(Host&, Column& root)  – Widgets anlegen. Werte aus host.config() lesen (öffentliche Sicht:
//                                           Geheimnisse sind leer, <key>_set sagt, ob etwas gespeichert ist).
//                                           Wird beim ersten Öffnen und nach „Verwerfen“ erneut aufgerufen;
//                                           alte Widget-Zeiger sind danach ungültig.
//        void collect(Host&, ojson& patch) – nur geänderte/zuständige Schlüssel in den Patch schreiben
//                                           (Form::collect erledigt das). Ungültige Eingabe: throw ui::Invalid.
//        bool active(const ojson& cfg)    – grüner Punkt in der Navigation (z. B. Quelle aktiviert).
//        void shown(Host&) / hidden(Host&) – beim Anzeigen bzw. Verlassen einer Seite.
//        void tick(Host&, const Status&)   – einmal pro Sekunde für die sichtbare Seite.
//   2. In ui_pages.cpp eine Zeile in create_pages() ergänzen (Fabrikfunktion make_<name>_page()).
//      Bewusst keine statische Selbstregistrierung: flcore ist eine statische Bibliothek, der Linker würde
//      unreferenzierte Seiten verwerfen.
//   3. Regeln: Alles in build/collect/tick läuft im UI-Thread. Langsames (Netz, Dateien, App-Aufrufe mit
//      Sperren) nur über host.run(job): job läuft im Arbeits-Thread und gibt eine Fortsetzung zurück, die im
//      UI-Thread läuft. Fortsetzungen greifen über die Seite (this) auf Widgets zu, nie über gefangene
//      Widget-Zeiger. Fehler (Exceptions) werden als Toast gezeigt. Konfiguration wird nur über „Speichern &
//      anwenden“ geschrieben (config::merge + config::save inkl. DPAPI, danach Neustart). Sofort wirksame
//      Aktionen (Plugins, Autostart) setzen track_dirty = false an ihren Widgets.
//   4. Geheimnisse: TextField::secret – leer lassen heißt „gespeicherten Wert behalten“; nie Klartext anzeigen.
#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"

namespace fl {
class App;
struct NativePluginActions;
}

namespace fl::ui {

using Microsoft::WRL::ComPtr;
using Rect = D2D1_RECT_F;
using Point = D2D1_POINT_2F;
using Rgba = D2D1_COLOR_F;

// Zweisprachiger Text; ein einzelner String gilt für beide Sprachen (Namen, IDs).
struct Text {
    std::wstring de, en;
    Text() = default;
    Text(const wchar_t* both) : de(both ? both : L""), en(de) {}
    Text(std::wstring both) : de(std::move(both)), en(de) {}
    Text(std::wstring d, std::wstring e) : de(std::move(d)), en(std::move(e)) {}
    const std::wstring& get(bool english) const { return english && !en.empty() ? en : de; }
    bool empty() const { return de.empty() && en.empty(); }
};
inline Text T(const wchar_t* de, const wchar_t* en) { return Text(std::wstring(de), std::wstring(en)); }

enum class Font { Body, Small, Strong, H1, Section, Nav, Tile, Button, Brand, Label };

struct Theme {
    bool dark = false;
    Rgba bg, card, line, text, muted, side, side_text, side_muted, nav_active, input, accent, accent_text,
        on_accent, header, header_text, success, danger, preview_bg;
    static const Theme& get(bool dark);
};
Rgba rgb(uint8_t r, uint8_t g, uint8_t b, float a = 1.f);
Rgba from(const Color& c, float a = 1.f);
bool system_prefers_dark();

class Kit {
public:
    Kit();
    ~Kit();
    Kit(const Kit&) = delete;
    Kit& operator=(const Kit&) = delete;
    ID2D1Factory* d2d() const { return d2d_.Get(); }
    IDWriteFactory* dwrite() const { return dw_.Get(); }
    IDWriteTextFormat* format(Font f);
    IDWriteInlineObject* ellipsis(Font f);
    ID2D1StrokeStyle* dashed();
    const Theme& theme() const { return Theme::get(dark); }
    bool english = false, dark = false;
private:
    ComPtr<ID2D1Factory> d2d_;
    ComPtr<IDWriteFactory> dw_;
    ComPtr<IDWriteTextFormat> formats_[10];
    ComPtr<IDWriteInlineObject> ellipsis_[10];
    ComPtr<ID2D1StrokeStyle> dashed_;
};

class Surface;

// Zeichen- und Messkontext (DIP). dim < 1 zeichnet deaktivierte Widgets blass.
struct Ctx {
    ID2D1RenderTarget* rt;
    Kit& kit;
    const Theme& th;
    ID2D1SolidColorBrush* brush;
    uint64_t device = 0;  // wechselt, wenn das Renderziel neu entsteht (Bitmaps neu anlegen)
    float dim = 1.f;
    const std::wstring& s(const Text& t) const { return t.get(kit.english); }
    void fill(const Rect& r, Rgba c, float radius = 0) const;
    void frame(const Rect& r, Rgba c, float radius = 0, float width = 1) const;
    void line(Point a, Point b, Rgba c, float width = 1) const;
    void circle(Point c, float radius, Rgba color) const;
    // Einzeilig mit „…“ oder umbrochen; vertikal mittig, wenn vcenter.
    void text(const std::wstring& t, const Rect& r, Font f, Rgba c, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
              bool wrap = false, bool vcenter = true) const;
    float text_height(const std::wstring& t, float width, Font f) const;
    float text_width(const std::wstring& t, Font f) const;
};

enum class Role { Static, Button, Toggle, Slider, Choice, Edit, List, NavTab, Group, Image };

class Widget {
public:
    virtual ~Widget() = default;
    Rect rc{};                 // nach layout(): Lage im Dokument der Surface (DIP)
    bool visible = true, enabled = true;
    bool track_dirty = true;   // false: zählt nicht als ungespeicherte Änderung (sofort wirksame Aktionen)
    bool hot = false;          // Maus darüber
    Text name;                 // Name für Sprachausgabe; Row setzt ihn auf ihre Beschriftung
    std::function<void()> on_change;

    virtual float layout(Ctx& c, float x, float y, float w);  // setzt rc, liefert Höhe
    virtual float height(Ctx&, float) { return 0; }
    virtual float pref_width(Ctx&) { return 0; }             // Steuerelemente rechts in einer Row
    virtual void paint(Ctx&) {}
    virtual void children(std::vector<Widget*>&) {}
    virtual bool focusable() const { return false; }
    virtual bool key(UINT, Surface&) { return false; }
    virtual void press(Point, Surface&) {}
    virtual void drag(Point, Surface&) {}
    virtual void release(Point, Surface&) {}
    virtual void activate(Surface&) {}                       // Leertaste/Eingabe/Standardaktion
    virtual bool clickable() const { return focusable(); }
    virtual void place(Surface&, bool shown) {}              // native Kindfenster nachführen
    virtual void restyle(Surface&) {}                        // Sprache/Theme/DPI geändert
    virtual bool dirty() const { return false; }
    virtual Role role() const { return Role::Static; }
    virtual std::wstring value(bool) const { return {}; }
    virtual bool checked() const { return false; }
    virtual bool wants_focus_ring() const { return true; }
    bool live() const { return visible && enabled; }
};
using WidgetPtr = std::unique_ptr<Widget>;

// ---------- Container ----------
class Column : public Widget {
public:
    std::vector<WidgetPtr> items;
    float gap = 14, pad_x = 0, pad_y = 0;
    template<class W, class... A> W* add(A&&... a) {
        auto w = std::make_unique<W>(std::forward<A>(a)...);
        W* raw = w.get(); items.push_back(std::move(w)); return raw;
    }
    void clear();  // alte Widgets leben bis zum Ende der aktuellen Nachricht weiter (flush_retired)
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
    void children(std::vector<Widget*>& out) override;
    Role role() const override { return Role::Group; }
};

class Row : public Widget {
public:
    Text label, desc;
    WidgetPtr control;
    bool wide = false;  // Steuerelement unter der Beschriftung in voller Breite (Listen, mehrzeilige Felder)
    Rect label_rc{}, desc_rc{};
    Row(Text l, Text d, WidgetPtr c, bool w = false);
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
    void children(std::vector<Widget*>& out) override;
    void press(Point p, Surface& s) override;
    bool clickable() const override;
};

class Card : public Column {
public:
    Text title, desc;
    bool placeholder = false;  // „folgt“-Karte (gestrichelt, Plakette FOLGT)
    float title_h = 0, desc_h = 0;
    std::vector<float> dividers;
    Card(Text t = {}, Text d = {}) : title(std::move(t)), desc(std::move(d)) { gap = 0; pad_x = 16; pad_y = 6; }
    template<class W, class... A> W* row(Text label, Text desc, A&&... a) {
        auto w = std::make_unique<W>(std::forward<A>(a)...);
        W* raw = w.get(); add<Row>(std::move(label), std::move(desc), std::move(w)); return raw;
    }
    template<class W, class... A> W* wide_row(Text label, Text desc, A&&... a) {
        auto w = std::make_unique<W>(std::forward<A>(a)...);
        W* raw = w.get(); add<Row>(std::move(label), std::move(desc), std::move(w), true); return raw;
    }
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
};

// ---------- Anzeige ----------
class Heading : public Widget {
public:
    Text title, subtitle;
    Heading(Text t, Text s) : title(std::move(t)), subtitle(std::move(s)) {}
    float height(Ctx& c, float w) override;
    void paint(Ctx& c) override;
};

class Label : public Widget {
public:
    enum Tone { Normal, Muted, Accent, Danger, Success };
    Text text;
    Font font = Font::Body;
    Tone tone = Normal;
    bool wrap = true;
    float width = 300;  // Breite als Steuerelement rechts in einer Row
    bool align_right = false;
    Label(Text t, Font f = Font::Body, Tone tone_ = Normal) : text(std::move(t)), font(f), tone(tone_) {}
    float pref_width(Ctx&) override { return width; }
    float height(Ctx& c, float w) override;
    void paint(Ctx& c) override;
    std::wstring value(bool en) const override { return text.get(en); }
};

class Note : public Widget {
public:
    enum Kind { Info, Warn, Soon };
    Text text;
    Kind kind;
    Note(Text t, Kind k = Info) : text(std::move(t)), kind(k) {}
    float height(Ctx& c, float w) override;
    void paint(Ctx& c) override;
    std::wstring value(bool en) const override { return text.get(en); }
};

class Tiles : public Widget {
public:
    struct Tile { Text value, label; };
    std::vector<Tile> tiles;
    float height(Ctx&, float) override { return 70; }
    void paint(Ctx& c) override;
    std::wstring value(bool en) const override;
};

class ImageView : public Widget {
public:
    std::optional<Image> image;
    Text empty_text;
    float aspect = 0.625f;     // Höhe/Breite ohne Bild
    bool framed = true;
    uint64_t version = 0;      // nach Änderung von image erhöhen
    float fixed_w = 0;         // > 0: feste Breite in DIP (links ausgerichtet), z. B. QR-Code
    bool crisp = false;        // harte Pixelkanten statt weichzeichnen (QR-Code)
    float layout(Ctx& c, float x, float y, float w) override;
    float height(Ctx& c, float w) override;
    void paint(Ctx& c) override;
    Role role() const override { return Role::Image; }
private:
    ComPtr<ID2D1Bitmap> bitmap_;
    uint64_t made_version_ = ~0ull, made_device_ = ~0ull;
};

// ---------- Steuerelemente ----------
class Toggle : public Widget {
public:
    bool on, original;
    explicit Toggle(bool v) : on(v), original(v) {}
    float pref_width(Ctx&) override { return 46; }
    float height(Ctx&, float) override { return 26; }
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override;
    bool dirty() const override { return on != original; }
    Role role() const override { return Role::Toggle; }
    bool checked() const override { return on; }
    void set(bool v, Surface& s);
};

class Slider : public Widget {
public:
    int minimum, maximum, val, original, step = 1, page = 10;
    Text suffix;
    Slider(int mn, int mx, int v, Text suf = {}) : minimum(mn), maximum(mx), val(v), original(v), suffix(std::move(suf)) {}
    float pref_width(Ctx&) override { return 260; }
    float height(Ctx&, float) override { return 26; }
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void press(Point p, Surface& s) override { drag(p, s); }
    void drag(Point p, Surface& s) override;
    bool dirty() const override { return val != original; }
    Role role() const override { return Role::Slider; }
    std::wstring value(bool en) const override;
    void set(int v, Surface& s);
};

class Segment : public Widget {
public:
    std::vector<Text> options;
    int index, original;
    bool header_style = false;
    std::vector<float> stops;  // Trennstellen (nach dem Zeichnen)
    Segment(std::vector<Text> o, int i) : options(std::move(o)), index(i), original(i) {}
    float pref_width(Ctx& c) override;
    float height(Ctx&, float) override { return header_style ? 28.f : 32.f; }
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    bool dirty() const override { return index != original; }
    Role role() const override { return Role::Choice; }
    std::wstring value(bool en) const override;
    void set(int i, Surface& s);
};

class Combo : public Widget {
public:
    std::vector<Text> options;
    int index, original;
    float width = 260;
    Combo(std::vector<Text> o, int i) : options(std::move(o)), index(i), original(i) {}
    float pref_width(Ctx&) override { return width; }
    float height(Ctx&, float) override { return 34; }
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override;
    bool dirty() const override { return index != original; }
    Role role() const override { return Role::Choice; }
    std::wstring value(bool en) const override;
    void set(int i, Surface& s);
};

class Button : public Widget {
public:
    enum Kind { Primary, Secondary, Danger, Ghost };
    Text text;
    Kind kind;
    std::function<void()> on_click;
    Button(Text t, Kind k, std::function<void()> f) : text(std::move(t)), kind(k), on_click(std::move(f)) {}
    float pref_width(Ctx& c) override;
    float height(Ctx&, float) override { return 36; }
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override;
    Role role() const override { return Role::Button; }
};

class ButtonRow : public Widget {
public:
    std::vector<std::unique_ptr<Button>> buttons;
    Button* add(Text t, Button::Kind k, std::function<void()> f) {
        buttons.push_back(std::make_unique<Button>(std::move(t), k, std::move(f))); return buttons.back().get();
    }
    float layout(Ctx& c, float x, float y, float w) override;
    void children(std::vector<Widget*>& out) override;
    Role role() const override { return Role::Group; }
};

// Echtes EDIT-Fenster in selbst gezeichnetem Rahmen.
class TextField : public Widget {
public:
    enum Kind { Single, Secret, Number, Multi };
    Kind kind;
    std::wstring original;
    Text hint;                  // Platzhalter (einzeilig)
    int lines = 4, limit = 1024;
    float width = 260;
    bool secret_set = false;    // Secret: ein Wert ist gespeichert
    TextField(Kind k, std::wstring v);
    ~TextField() override;
    std::wstring text() const;
    void set_text(const std::wstring& t);
    float pref_width(Ctx&) override { return width; }
    float height(Ctx&, float) override;
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    void place(Surface& s, bool shown) override;
    void restyle(Surface& s) override;
    bool dirty() const override;
    Role role() const override { return Role::Edit; }
    std::wstring value(bool) const override { return kind == Secret ? L"" : text(); }
    HWND edit() const { return edit_; }
    void detach_native() { edit_ = nullptr; }
    void focus_native();
private:
    HWND edit_ = nullptr;
    std::wstring pending_;
    bool created_ = false;
    void apply_hint(Surface& s);
};

// Farbfeld: Farbfläche (Klick öffnet den Windows-Farbwähler) plus Hex-Eingabe #RRGGBB. Leer = Farbe der Vorlage (fallback).
class ColorField : public Widget {
public:
    TextField field;
    std::string fallback;        // Vorlagenfarbe #RRGGBB, wenn das Feld leer ist
    std::string original_hex;
    explicit ColorField(const std::string& hex);
    std::string hex() const;     // normalisiert (#RRGGBB) oder leer (leer oder ungültig)
    bool valid() const;          // leer oder gültig
    std::string effective() const { auto h = hex(); return h.empty() ? fallback : h; }
    float pref_width(Ctx&) override { return 210; }
    float height(Ctx&, float) override { return 34; }
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
    void children(std::vector<Widget*>& out) override { out.push_back(&field); }
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override { pick(s); }
    Role role() const override { return Role::Button; }
    std::wstring value(bool) const override;
    void pick(Surface& s);
    void set_hex(const std::string& hex);
};

// Liste: Auswahl, optional Ein/Aus-Haken und Umsortieren (▲/▼, Alt+Pfeil).
class ItemList : public Widget {
public:
    struct Item { std::string key; Text title, detail; bool enabled = false; };
    std::vector<Item> items, original;
    int selected = -1;
    bool checkable = true, reorder = true;
    Text empty_text;
    std::function<void(int)> on_select;
    explicit ItemList(std::vector<Item> v) : items(v), original(std::move(v)) {}
    float height(Ctx& c, float w) override;
    void paint(Ctx& c) override;
    bool focusable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override;
    bool dirty() const override;
    Role role() const override { return Role::List; }
    std::wstring value(bool en) const override;
    void select(int i, Surface& s);
    void toggle(int i, Surface& s);
    void move(int i, int delta, Surface& s);
    float row_height() const;
private:
    bool hit_check(Point p, int& index, int& part) const;  // part: 0 Zeile, 1 Haken, 2 hoch, 3 runter
};

// Navigationseintrag (Seitenleiste)
class NavItem : public Widget {
public:
    Text text;
    std::string id;
    bool active = false, dot_on = false;
    std::function<void()> on_open;
    std::function<void(int)> on_step;  // Pfeil hoch/runter
    NavItem(Text t, std::string i) : text(std::move(t)), id(std::move(i)) {}
    float height(Ctx&, float) override { return 34; }
    void paint(Ctx& c) override;
    bool focusable() const override { return active; }  // nur der aktive Eintrag ist Tab-Stopp, Pfeile wechseln
    bool clickable() const override { return true; }
    bool key(UINT vk, Surface& s) override;
    void release(Point p, Surface& s) override;
    void activate(Surface& s) override;
    Role role() const override { return Role::NavTab; }
    bool checked() const override { return active; }
};

// Platzhalter in der Tab-Reihenfolge: leitet den Fokus in eine andere Surface (Inhaltsbereich).
class Portal : public Widget {
public:
    Surface* target = nullptr;
    bool focusable() const override { return target != nullptr; }
    bool clickable() const override { return false; }
    bool wants_focus_ring() const override { return false; }
};

// ---------- Surface ----------
class Accessible;

class Surface {
public:
    Surface(Kit& kit, HWND parent, const wchar_t* name, bool scrollable);
    ~Surface();
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    HWND hwnd() const { return hwnd_; }
    Kit& kit;
    void set_root(Widget* root);  // nicht besitzend
    Widget* root() const { return root_; }
    void relayout();
    void invalidate();
    void changed(Widget* w);      // Wert geändert (vom Widget aufgerufen)
    void focus(Widget* w, bool keyboard = true);
    Widget* focused() const { return focus_; }
    void mark(Widget* w) { focus_ = w; }  // Fokusposition setzen, ohne den Tastaturfokus zu holen
    void focus_edge(bool first);  // erstes/letztes fokussierbares Widget
    bool tab(bool forward);
    void ensure_visible(Widget* w);
    void notify(Widget* w, DWORD event);
    void set_dpi(UINT dpi);
    UINT dpi() const { return dpi_; }
    HFONT edit_font() const { return edit_font_; }
    D2D1_SIZE_F size() const;     // DIP
    float scroll() const { return scroll_; }
    void scroll_to(float y);
    void restyle();               // Sprache/Theme
    int px(float dip) const { return int(dip * dpi_ / 96.f + 0.5f); }
    std::vector<Widget*> flatten(bool focusable_only) const;

    std::function<void(Widget*)> on_changed;
    std::function<void(bool forward)> on_tab_out;
    std::function<void(Ctx&, D2D1_SIZE_F)> on_background, on_overlay;
    std::function<float(Ctx&, D2D1_SIZE_F)> on_layout;  // eigenes Layout (Rahmenfenster); liefert Dokumenthöhe
    std::function<void()> on_resize;
    std::function<void()> on_focus_in;
    std::function<bool(UINT, WPARAM, LPARAM, LRESULT&)> on_message;  // true = erledigt

    // intern (Fensterprozedur, EDIT-Unterklassen)
    static LRESULT CALLBACK proc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);
    void native_focus(Widget* w);
    bool keyboard_cues = false;
    HBRUSH input_brush() const { return input_brush_; }
private:
    HWND hwnd_ = nullptr;
    bool scrollable_;
    Widget* root_ = nullptr;
    Widget* focus_ = nullptr;
    Widget* capture_ = nullptr;
    Widget* hover_ = nullptr;
    ComPtr<ID2D1HwndRenderTarget> rt_;
    ComPtr<ID2D1SolidColorBrush> brush_;
    uint64_t device_ = 0;
    UINT dpi_ = 96;
    float scroll_ = 0, doc_h_ = 0;
    bool thumb_drag_ = false;
    float thumb_grab_ = 0;
    HFONT edit_font_ = nullptr;
    HBRUSH input_brush_ = nullptr;
    Accessible* acc_ = nullptr;
    bool tracking_ = false;
    void paint();
    void create_target();
    Widget* hit(Point doc) const;
    Point doc_point(LPARAM lp) const;
    Rect thumb() const;
    void make_edit_resources();
    void place_all();
};

// ---------- Seiten ----------
struct Invalid : std::runtime_error {
    Widget* widget;
    Invalid(const std::string& message, Widget* w) : std::runtime_error(message), widget(w) {}
};

enum class Group { General, Sources, Connection, App };

struct PageInfo {
    std::string id;
    Group group;
    Text title, subtitle;
};

// Momentaufnahme der App, einmal pro Sekunde im Arbeits-Thread geholt.
struct Status {
    bool display_ok = false;
    std::string display_error, output;
    std::pair<int, int> size{0, 0};
    ojson connections = ojson::object();
    ojson update = ojson::object();
    ojson navigation = ojson::object();
    std::optional<bool> autostart;  // einmalig geladen
    ojson companion = ojson::object();  // NetworkDisplay::status(); leer, wenn Companion aus ist
    bool valid = false;
};

enum class ToastKind { Info, Success, Error };

class Host {
public:
    virtual ~Host() = default;
    virtual App& app() = 0;
    virtual const ojson& config() const = 0;        // öffentliche Sicht beim Öffnen des Fensters
    virtual NativePluginActions& plugins() = 0;
    virtual bool english() const = 0;
    virtual void relayout() = 0;                    // Struktur/Sichtbarkeit einer Seite geändert
    virtual void toast(const Text& text, ToastKind kind = ToastKind::Info) = 0;
    virtual void run(std::function<std::function<void()>()> job) = 0;  // Arbeits-Thread → Fortsetzung im UI-Thread
    virtual bool busy() const = 0;
    virtual void navigate(const std::string& id) = 0;
    virtual HWND window() const = 0;
    virtual void preview(std::optional<Image> frame) = 0;  // Vorschau überschreiben; nullopt = wieder live
    virtual const Status& status() const = 0;
    virtual Surface& surface() = 0;                 // Inhaltsbereich (z. B. für Toggle::set)
};

class Page {
public:
    virtual ~Page() = default;
    virtual PageInfo info() const = 0;
    virtual void build(Host& host, Column& root) = 0;
    virtual void collect(Host&, ojson&) {}
    virtual bool active(const ojson&) const { return false; }
    virtual void shown(Host&) {}
    virtual void hidden(Host&) {}
    virtual void tick(Host&, const Status&) {}
};

// Bindet Widgets an section.key der Konfiguration.
class Form {
public:
    Form() = default;
    explicit Form(Host& h) : host_(&h) {}
    Toggle* toggle(Card& card, const char* sec, const char* key, Text label, Text desc = {});
    TextField* text(Card& card, const char* sec, const char* key, Text label, Text desc = {}, Text hint = {});
    TextField* secret(Card& card, const char* sec, const char* key, Text label, Text desc = {});
    TextField* number(Card& card, const char* sec, const char* key, Text label, Text desc, int minimum, int maximum);
    TextField* list(Card& card, const char* sec, const char* key, Text label, Text desc = {}, int lines = 4);
    Slider* slider(Card& card, const char* sec, const char* key, Text label, Text desc, int minimum, int maximum, Text suffix = {});
    Segment* segment(Card& card, const char* sec, const char* key, Text label, Text desc,
                     std::vector<std::string> values, std::vector<Text> labels);
    Combo* combo(Card& card, const char* sec, const char* key, Text label, Text desc,
                 std::vector<std::string> values, std::vector<Text> labels);
    void collect(ojson& patch) const;  // wirft Invalid
    const ojson& value(const char* sec, const char* key) const;
private:
    enum class Kind { Toggle, Text, Secret, Number, List, Slider, Segment, Combo };
    struct Binding { std::string sec, key; Kind kind; Widget* w; int minimum = 0, maximum = 0; std::vector<std::string> values; };
    Host* host_ = nullptr;
    std::vector<Binding> bindings_;
    static int index_of(const std::vector<std::string>& values, const std::string& v);
};

// Platzhalterkarte „noch nicht nativ“ für künftige Seiten (aktuell von keiner Seite genutzt).
Card* placeholder(Host& host, Column& root, Text title, Text text);

// Entfernte Widgets (Column::clear) erst nach der laufenden Nachricht löschen; die Nachrichtenschleife ruft flush_retired.
void retire(std::vector<WidgetPtr>& items);
void flush_retired();

// Kleine Helfer für Seiten
std::wstring source_title(const std::string& key, bool english);
int count_dirty(Widget* root);

}  // namespace fl::ui
