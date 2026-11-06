// © 2026 FuchsLabs – Kachelpalette des Anordnungs-Editors (Phase 7b P7b-2): Raster kleiner Kacheln mit Mini-Vorschau.
// Zeichnet nur; die Mini-Bilder kommen als fertige Image-Objekte von der Seite (derselbe Renderer, Beispieldaten).
#pragma once
#include "ui_layout_grid.h"
#include <map>

namespace fl::ui {

class TilePalette : public Widget {
public:
    struct Tile {
        std::string id;            // Block-ID (Modellreihenfolge = Index in GridEditor::model.blocks)
        Text label;
        std::string category;      // all-Filter siehe matches(); system | time | media | games | other
        bool placed = false, required = false;
    };
    struct Thumb {
        std::shared_ptr<const Image> image;  // leer: noch nicht geladen oder nicht darstellbar
        bool no_data = false;                // Renderer zeichnet mit Beispieldaten nichts („keine Daten“)
        uint64_t version = 0;
    };
    std::vector<Tile> tiles;
    GridEditor* grid = nullptr;            // Ziel für Ziehen, Einfügen und Auswählen
    std::function<const Thumb*(const std::string& id)> thumb_of;
    std::string category;                  // leer: alle
    std::wstring query;                    // kleingeschrieben
    int current = 0;                       // Cursor: Position in visible()
    Surface* surface = nullptr;            // für Fokusanzeige
    TilePalette();

    std::vector<size_t> visible() const;   // Indizes in tiles nach Suche und Filter
    static bool matches(const Tile& tile, const std::string& category, const std::wstring& query, bool english);
    Rect tile_rect(size_t position) const; // Position in visible()
    int columns() const;
    float height(Ctx&, float w) override;
    void paint(Ctx&) override;
    bool focusable() const override { return true; }
    bool clickable() const override { return true; }
    Role role() const override { return Role::List; }
    std::wstring value(bool english) const override;
    bool key(UINT, Surface&) override;
    void press(Point, Surface&) override;
    void drag(Point, Surface&) override;
    void release(Point, Surface&) override;
    void cancel(Surface&) override;
    bool autoscroll() const override { return grid && grid->autoscroll(); }
    LPCWSTR cursor_for(Point) const;
    LPCWSTR cursor(Point p) const override { return cursor_for(p); }
    std::wstring tip(Point, bool english) const override;
    void activate(Surface&) override;       // Enter/Klick auf die Kachel unter dem Cursor
    int tile_at(Point) const;               // Position in visible() oder -1
private:
    int pressed_ = -1;                      // Position der gedrückten Kachel
    Point origin_{};
    float threshold_ = 4;
    std::map<std::string, ComPtr<ID2D1Bitmap>> bitmaps_;
    std::map<std::string, uint64_t> made_;
    uint64_t device_ = ~0ull;
    void click(size_t position, Surface&);
};

}  // namespace fl::ui
