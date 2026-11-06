// © 2026 FuchsLabs – schematisches Raster, kein Seitenrenderer.
#pragma once
#include "ui_kit.h"
#include "layout_edit.h"

namespace fl::ui {
class GridEditor : public Widget {
public:
    plugins::Arrangement model;
    int column = 0, row = 0, selected = 0;
    int first_column = 0; // horizontaler Ausschnitt bei weniger als 32 DIP je Zelle
    std::function<void(plugins::Arrangement, std::string)> on_edit;
    std::function<void()> on_select, on_undo, on_redo, on_reject;
    GridEditor();
    float height(Ctx&, float) override;
    void paint(Ctx&) override;
    bool focusable() const override { return true; }
    Role role() const override { return Role::List; }
    std::wstring value(bool english) const override;
    bool key(UINT, Surface&) override;
    bool system_arrows() const override { return true; }
    void release(Point, Surface&) override;
    void activate(Surface&) override;
    // Derselbe Tastenweg für UI und fensterlose Tests (keine simulierten Tastenzustände).
    bool input(UINT key, bool shift = false, bool alt = false, bool control = false);
private:
    int zone_at() const;
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
