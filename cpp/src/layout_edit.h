// © 2026 FuchsLabs – reines Editor-Modell, ohne Dateien und Renderer.
#pragma once
#include "plugins.h"
#include <cstdint>
#include <functional>

namespace fl::plugins::edit {
using Model = Arrangement;
bool equal(const Model& a, const Model& b);
std::optional<zones::Layout> geometry(const Model& m);
// Transaktion: Bei einem Strukturfehler bleibt das Original bitgleich.
bool change(Model& m, const std::function<void(Model&)>& action);
bool move_zone(Model& m, size_t zone, int dx, int dy);
bool resize_zone(Model& m, size_t zone, int dx, int dy);
bool add_zone(Model& m, int column, int row);
bool split_zone(Model& m, size_t zone, bool horizontal);
bool delete_zone(Model& m, size_t zone);
bool assign_block(Model& m, size_t block, size_t zone);
// Zusammengesetzte Ablageoperationen: verborgenes Inventar bleibt im Overlay.
bool place_block(Model& m, size_t block, int column, int row);
bool remove_block(Model& m, size_t block);
bool move_block(Model& m, size_t block, size_t zone, size_t index);
bool probe(const Model& m, const std::function<bool(Model&)>& operation);
// Wahr, wenn eine Zone, die vorher einen gezeigten Block hatte, danach keinen mehr hat (Ziel soll "passt nicht" melden).
bool leaves_empty_zone(const Model& before, const Model& after);
bool resize_axis(Model& m, bool columns, int delta);
bool weight(Model& m, bool columns, size_t index, int value);
std::string diagnostic(std::string_view error, bool english);

class EditHistory {
public:
    void reset(Model model);
    // group leer = eigene Aktion; gleiche Pfeilserie innerhalb 600 ms zusammenfassen.
    bool push(Model model, std::string group = {}, uint64_t milliseconds = 0);
    bool undo();
    bool redo();
    bool can_undo() const { return cursor_ > 0; }
    bool can_redo() const { return cursor_ + 1 < states_.size(); }
    const Model& current() const { return states_.at(cursor_); }
    size_t steps() const { return states_.empty() ? 0 : states_.size() - 1; }
private:
    std::vector<Model> states_;
    size_t cursor_ = 0;
    std::string group_;
    uint64_t time_ = 0;
};
}
