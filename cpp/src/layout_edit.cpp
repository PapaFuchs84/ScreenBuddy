// © 2026 FuchsLabs – Clean-Room-Editorbausteine.
#include "layout_edit.h"
#include <algorithm>

namespace fl::plugins::edit {
bool equal(const Model& a, const Model& b) { return to_json(a).dump() == to_json(b).dump(); }
std::optional<zones::Layout> geometry(const Model& m) {
    try {
        zones::Layout l;
        l.columns = m.layout.at("columns").get<std::vector<int>>();
        l.rows = m.layout.at("rows").get<std::vector<int>>();
        for (const auto& j : m.layout.at("zones")) {
            zones::Zone z;
            z.id = j.at("id"); z.column = j.at("column"); z.row = j.at("row");
            z.column_span = j.value("column_span", 1); z.row_span = j.value("row_span", 1);
            if (j.contains("cycle")) z.cycle = zones::Cycle{j["cycle"].at("mode"), j["cycle"].value("transition", "fade")};
            l.zones.push_back(z);
        }
        if (zones::valid(l)) return l;
    } catch (const Json::exception&) {}
    return {};
}
bool change(Model& m, const std::function<void(Model&)>& action) {
    auto next = m;
    try { action(next); } catch (const Json::exception&) { return false; }
    if (!geometry(next) || !parse_arrangement(to_json(next).dump())) return false;
    m = std::move(next); return true;
}
bool move_zone(Model& m, size_t i, int dx, int dy) {
    return change(m, [=](Model& n) { auto& z = n.layout["zones"].at(i); z["column"] = z["column"].get<int>() + dx; z["row"] = z["row"].get<int>() + dy; });
}
bool resize_zone(Model& m, size_t i, int dx, int dy) {
    return change(m, [=](Model& n) { auto& z = n.layout["zones"].at(i); z["column_span"] = z.value("column_span", 1) + dx; z["row_span"] = z.value("row_span", 1) + dy; });
}
bool add_zone(Model& m, int col, int row) {
    return change(m, [=](Model& n) {
        std::string id;
        for (int i = 1; ; ++i) {
            id = "zone_" + std::to_string(i);
            if (std::none_of(n.layout["zones"].begin(), n.layout["zones"].end(), [&](auto& z) { return z["id"] == id; })) break;
        }
        n.layout["zones"].push_back({{"id", id}, {"column", col}, {"row", row}});
    });
}
bool split_zone(Model& m, size_t i, bool horizontal) {
    auto next = m; auto l = geometry(m);
    if (!l || i >= l->zones.size()) return false;
    auto z = l->zones[i]; const int span = horizontal ? z.column_span : z.row_span;
    if (span < 2) return false;
    const char* key = horizontal ? "column_span" : "row_span";
    next.layout["zones"][i][key] = span / 2;
    if (!add_zone(next, z.column + (horizontal ? span / 2 : 0), z.row + (horizontal ? 0 : span / 2))) return false;
    next.layout["zones"].back()[key] = span - span / 2;
    if (horizontal) next.layout["zones"].back()["row_span"] = z.row_span;
    else next.layout["zones"].back()["column_span"] = z.column_span;
    if (!geometry(next)) return false;
    m = std::move(next); return true;
}
bool delete_zone(Model& m, size_t i) {
    auto l = geometry(m);
    if (!l || l->zones.size() <= 1 || i >= l->zones.size()) return false;
    auto target = l->zones[i == 0 ? 1 : 0].id;
    int count = 0;
    for (auto& b : m.blocks) if ((b["zone"] == target || b["zone"] == l->zones[i].id) && b.value("shown", true)) ++count;
    if (count > zones::max_blocks_per_zone) return false;
    return change(m, [=](Model& n) {
        for (auto& b : n.blocks) if (b["zone"] == l->zones[i].id) b["zone"] = target;
        n.layout["zones"].erase(n.layout["zones"].begin() + i);
    });
}
bool assign_block(Model& m, size_t b, size_t z) {
    return change(m, [=](Model& n) { n.blocks.at(b)["zone"] = n.layout["zones"].at(z).at("id"); });
}
bool resize_axis(Model& m, bool columns, int delta) {
    return change(m, [=](Model& n) {
        auto& a = n.layout[columns ? "columns" : "rows"];
        if (delta > 0) a.push_back(1);
        else if (!a.empty()) a.erase(a.end() - 1);
    });
}
bool weight(Model& m, bool columns, size_t i, int v) {
    return change(m, [=](Model& n) { n.layout[columns ? "columns" : "rows"].at(i) = v; });
}
std::string diagnostic(std::string_view e, bool en) {
    struct Entry { const char *error, *de, *english; };
    static constexpr Entry table[] = {
        {"Plugin is not available.", "Plugin nicht verfügbar.", "Plugin is not available."},
        {"Plugin library is not available.", "Plugin-Bibliothek nicht verfügbar.", "Plugin library is not available."},
        {"Plugin is not installed.", "Plugin ist nicht installiert.", "Plugin is not installed."},
        {"Arrangement does not match the plugin version.", "Anordnung passt nicht zur Plugin-Version.", "Arrangement does not match the plugin version."},
        {"Arrangement files contain an unsafe link.", "Anordnungsdateien enthalten einen unsicheren Link.", "Arrangement files contain an unsafe link."},
        {"Arrangement file is unavailable.", "Anordnungsdatei nicht verfügbar.", "Arrangement file is unavailable."},
        {"Arrangement backup could not be saved.", "Anordnungsbackup konnte nicht gespeichert werden.", "Arrangement backup could not be saved."},
        {"Arrangement could not be saved atomically.", "Anordnung konnte nicht atomar gespeichert werden.", "Arrangement could not be saved atomically."},
        {"Arrangement backup could not be removed.", "Anordnungsbackup konnte nicht entfernt werden.", "Arrangement backup could not be removed."},
        {"Arrangement could not be removed.", "Anordnung konnte nicht entfernt werden.", "Arrangement could not be removed."},
        {"Every zone needs at least one block.", "Zone: Jede Zone braucht einen gezeigten Block.", "Zone: every zone needs a shown block."},
        {"At most 4 blocks per zone allowed.", "Zone: Höchstens vier gezeigte Blöcke sind erlaubt.", "Zone: at most four shown blocks are allowed."},
        {"Next requires a cycle zone.", "Block: Die bestehende Next-Aktion braucht eine Wechselzone.", "Block: the existing next action requires a cycle zone."},
        {"Zones must not overlap.", "Raster: Zonen dürfen nicht überlappen.", "Grid: zones must not overlap."},
        {"Invalid or duplicate zone ID.", "Zone: ID ungültig oder doppelt.", "Zone: invalid or duplicate ID."},
        {"Layout needs 1 to 8 zones.", "Raster: Eine bis acht Zonen sind erlaubt.", "Grid: one to eight zones are allowed."},
        {"Unsupported cycle mode.", "Zone: Wechselmodus ungültig.", "Zone: invalid cycle mode."},
        {"Unsupported cycle transition.", "Zone: Übergang ungültig.", "Zone: invalid transition."},
        {"Seconds requires a rotate zone.", "Block: Sekunden brauchen eine Wechselzone (rotate).", "Block: seconds require a rotate zone."},
        {"Arrangement requires manifest v2.", "Anordnung braucht Manifest v2.", "Arrangement requires manifest v2."},
        {"Block references an unknown zone.", "Block: Bitte eine vorhandene Zone wählen.", "Block: choose an existing zone."},
        {"Invalid arrangement.", "Raster: Anordnung ungültig. Bereiche und Zonen prüfen.", "Grid: invalid arrangement. Check ranges and zones."},
    };
    for (auto& x : table) if (e == x.error) return en ? x.english : x.de;
    return en ? "Invalid arrangement. Check zones, blocks and conditions." : "Anordnung ungültig. Zonen, Blöcke und Bedingungen prüfen.";
}
void EditHistory::reset(Model m) { states_ = {std::move(m)}; cursor_ = 0; group_.clear(); time_ = 0; }
bool EditHistory::push(Model m, std::string group, uint64_t ms) {
    if (states_.empty()) { reset(std::move(m)); return true; }
    if (equal(current(), m)) return false;
    const bool coalesce = !group.empty() && group == group_ && cursor_ > 0 && !can_redo() && ms >= time_ && ms - time_ <= 600;
    states_.resize(cursor_ + 1);
    if (coalesce) states_[cursor_] = std::move(m);
    else { states_.push_back(std::move(m)); ++cursor_; }
    if (states_.size() > 101) { states_.erase(states_.begin()); --cursor_; }
    group_ = std::move(group); time_ = ms; return true;
}
bool EditHistory::undo() { group_.clear(); if (!can_undo()) return false; --cursor_; return true; }
bool EditHistory::redo() { group_.clear(); if (!can_redo()) return false; ++cursor_; return true; }
}
