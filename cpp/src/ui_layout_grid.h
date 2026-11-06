// © 2026 FuchsLabs – schematisches Raster, kein Seitenrenderer.
#pragma once
#include "ui_kit.h"
#include "layout_edit.h"
#include <array>

namespace fl::ui {
class GridEditor : public Widget {
public:
    plugins::Arrangement model;
    float display_aspect = 0; // 0: bisheriges schematisches Raster; sonst Gerätefläche.
    // P7b-3 „Fläche“: Gerätegrößen-Rahmen, dezentes Raster, gestrichelte Ablageplätze, Geist, Griffe, Papierkorb.
    // Aus (Standard) bleibt Zeichnung und Trefferfläche des bisherigen Rasters bitgleich.
    bool canvas = false;
    float zoom = 1.f;         // 1 = Passend (Rahmen füllt die Breite), kleiner = herausgezoomt
    int column = 0, row = 0, selected = 0;
    int first_column = 0; // horizontaler Ausschnitt bei weniger als 32 DIP je Zelle
    std::function<void(plugins::Arrangement, std::string)> on_edit;
    std::function<void()> on_select, on_undo, on_redo, on_reject;
    std::function<void(int)> on_area;   // Bereichswechsel per F6/Strg+Tab (0 Palette, 1 Fläche, 2 Papierkorb)
    GridEditor();
    float height(Ctx&, float) override;
    void paint(Ctx&) override;
    void children(std::vector<Widget*>&) override;
    bool focusable() const override { return true; }
    Role role() const override { return Role::List; }
    std::wstring value(bool english) const override;
    bool key(UINT, Surface&) override;
    bool system_arrows() const override { return true; }
    enum class Kind { Down, Move, Up, Cancel };
    void pointer(Kind, Point);
    void press(Point, Surface&) override;
    void drag(Point, Surface&) override;
    void release(Point, Surface&) override;
    void cancel(Surface&) override;
    LPCWSTR cursor(Point) const override;
    bool autoscroll() const override { return active_ && !keyboard_; }
    bool carrying() const { return active_; }
    bool target_valid() const;
    Rect grid_rect() const;
    Rect palette_rect(size_t block) const;
    Rect trash_rect() const;
    Rect zone_rect(size_t zone) const;
    Rect block_rect(size_t block) const;
    int area = 1, palette_block = 0, focused_block = -1; // Leiste, Raster, Ablage
    unsigned value_events = 0; // gemeldete EVENT_OBJECT_VALUECHANGE (Test)
    void activate(Surface&) override;
    // Derselbe Tastenweg für UI und fensterlose Tests (keine simulierten Tastenzustände).
    bool input(UINT key, bool shift = false, bool alt = false, bool control = false);
    // Kachelpalette (P7b-2): Aufnehmen, Einfügen und Auswählen aus einem anderen Widget. Weitere Mausereignisse
    // der begonnenen Geste laufen über drag()/release()/cancel() dieses Widgets.
    void start_external(size_t block, Point, Surface&);
    bool pickup(size_t block, Surface&);          // Tastatur: Ziel ist der Zellfokus
    bool insert_first_free(size_t block, Surface&); // freie Zelle, sonst Zone mit Platz; true = Bearbeitung gemeldet
    void select_block(size_t block, Surface&);    // Zone und Chip eines gezeigten Blocks wählen
    bool shown_block(size_t block) const;
    std::function<std::wstring(const std::string&, bool)> block_label; // lesbarer Name (P7b-3); leer: Block-ID
private:
    class PaletteInfo : public Widget {
    public:
        GridEditor* owner = nullptr;
        Role role() const override { return Role::List; }
        std::wstring value(bool english) const override;
    } palette_;
    void paint_canvas(Ctx&);
    std::wstring label_of(const std::string& id, bool english) const;
    int zone_at() const;
    enum class Action { None, Block, Zone, Resize };
    Action action_ = Action::None;
    bool down_ = false, active_ = false, keyboard_ = false, trash_ = false, moved_ = false, dirty_ = false;
    int prev_selected_ = 0; // Auswahl vor dem Drücken, für Abbruch
    using Snap = std::array<long long, 11>;
    Snap snap() const;
    int chip_zone() const;                 // Zone unter dem Zellfokus, wenn focused_block ein gezeigter Block darin ist, sonst -1
    std::vector<int> chips(int zone) const; // gezeigte Blöcke einer Zone in Modellreihenfolge
    bool step_chip(int direction);
    void cancel_drag();
    size_t source_ = 0, stack_ = 0;
    int start_column_ = 0, start_row_ = 0, edges_ = 0;
    Point origin_{};
    float threshold_x_ = 4, threshold_y_ = 4;
    uint64_t scroll_time_ = 0;
    int palette_columns() const;
    float palette_height() const;
    int edges(Point, int zone) const;
    void cell(Point);
    bool candidate(plugins::Arrangement&) const;
    void finish();
    void reset_drag();
    void announce(Surface&, const Snap& before, bool was_active);
};
class LayoutBlockList : public ItemList {
public:
    using ItemList::ItemList;
    std::function<void()> on_undo, on_redo;
    std::string history_group;
    void paint(Ctx&) override;
    bool key(UINT, Surface&) override;
};
}
