// © 2026 FuchsLabs – Fensteraufbau des Anordnungs-Editors (Phase 7b): Kopfleiste, linke Schiene, Drei-Spalten-Hülle.
// Reine Darstellung auf dem ui_kit; keine Datenlogik. Clean-Room: nur docs/PHASE7B-EDITOR-OBERFLAECHE.md.
#pragma once
#include "ui_kit.h"

namespace fl::ui {

// Beschriftete Plakette ohne Bedienung (z. B. Gerätegröße der Vorschau).
class Chip : public Widget {
public:
    Text text;
    explicit Chip(Text t) : text(std::move(t)) { track_dirty = false; }
    float pref_width(Ctx& c) override { return c.text_width(c.s(text), Font::Body) + 28; }
    float height(Ctx&, float) override { return 32; }
    void paint(Ctx& c) override;
    std::wstring value(bool en) const override { return text.get(en); }
};

// Umbrechende Zeile beliebiger Steuerelemente (Kopfleiste).
class Flow : public Widget {
public:
    std::vector<WidgetPtr> items;
    float gap_x = 10, gap_y = 8;
    template<class W, class... A> W* add(A&&... a) {
        auto w = std::make_unique<W>(std::forward<A>(a)...);
        W* raw = w.get(); items.push_back(std::move(w)); return raw;
    }
    void remove(Widget* w);  // das Widget lebt bis zum Ende der laufenden Nachricht weiter (retire)
    float layout(Ctx& c, float x, float y, float w) override;
    void children(std::vector<Widget*>& out) override;
    Role role() const override { return Role::Group; }
};

// Schaltfläche der linken Schiene: Symbol über Beschriftung; schaltet einen Bereich auf oder zu.
class RailItem : public Button {
public:
    enum Glyph { Tiles, Layers, Pages };
    Glyph glyph;
    bool open = false;
    RailItem(Text t, Glyph g, std::function<void()> f) : Button(std::move(t), Ghost, std::move(f)), glyph(g) {}
    float pref_width(Ctx&) override { return 64; }
    float height(Ctx&, float) override { return 56; }
    void paint(Ctx& c) override;
    Role role() const override { return Role::NavTab; }
    bool checked() const override { return open; }
    std::wstring value(bool en) const override { return open ? (en ? L"open" : L"geöffnet") : (en ? L"closed" : L"eingeklappt"); }
};

// Karte mit Titel; Inhalt untereinander, ohne Trennlinien (linke Bereiche, Fläche).
class Panel : public Column {
public:
    Text title;
    explicit Panel(Text t = {}) : title(std::move(t)) { gap = 10; pad_x = 12; pad_y = 12; }
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
private:
    float title_h_ = 0;
};

// Drei-Spalten-Hülle: Schiene | Bereiche (Kacheln, Ebenen, Seiten) | Fläche | Eigenschaften.
// Breit: alles nebeneinander. Mittel: Eigenschaften rutschen unter die Fläche, sobald ein Bereich offen ist.
// Schmal: alles untereinander, die Kachelpalette ist anfangs eingeklappt.
class EditorShell : public Widget {
public:
    enum Mode { Wide, Medium, Narrow };
    enum Section { Palette = 0, Layers = 1, PageList = 2 };
    Column left;                 // Bereiche, jeder ein Panel
    Panel center;                // Fläche
    Column props;                // Eigenschaften (Karten)
    Panel* section[3]{};
    RailItem* rail[3]{};
    Mode mode = Wide;
    std::function<void()> on_toggle;  // nach Auf-/Zuklappen: Seite neu anordnen
    EditorShell();
    bool is_open(Section s) const { return open_[s]; }
    bool touched(Section s) const { return touched_[s]; }
    void set_open(Section s, bool value) { open_[s] = value; touched_[s] = true; }
    float layout(Ctx& c, float x, float y, float w) override;
    void paint(Ctx& c) override;
    void children(std::vector<Widget*>& out) override;
    Role role() const override { return Role::Group; }
    static Mode mode_for(float width) { return width >= 900 ? Wide : width >= 600 ? Medium : Narrow; }
private:
    std::vector<WidgetPtr> rail_;
    Rect rail_rc_{};
    bool open_[3]{true, true, false}, touched_[3]{};
};

}  // namespace fl::ui
