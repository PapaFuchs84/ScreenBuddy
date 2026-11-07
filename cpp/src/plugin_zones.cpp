// © 2026 FuchsLabs
#include "plugin_zones.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace fl::plugins::zones {
namespace {
bool finite_positive(float v) { return std::isfinite(v) && v > 0 && v <= max_area; }
bool identifier(std::string_view s) {
    if (s.empty() || s.size() > 40 || !(s[0] >= 'a' && s[0] <= 'z')) return false;
    return std::all_of(s.begin(), s.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; });
}
bool weights(const std::vector<int>& w, int limit) {
    return !w.empty() && static_cast<int>(w.size()) <= limit &&
           std::all_of(w.begin(), w.end(), [](int x) { return x >= 1 && x <= max_weight; });
}
// Start/Ende jeder Spur (Spalte oder Zeile). Kanten aus kumulierten Anteilen, gerundet auf ganze logische
// Pixel; die letzte Kante liegt exakt auf origin + total. false = kein Platz (Abstände fressen alles).
bool tracks(float origin, float total, const std::vector<int>& w, float gap, std::vector<float>& start, std::vector<float>& end) {
    const size_t n = w.size();
    const double avail = static_cast<double>(total) - static_cast<double>(gap) * static_cast<double>(n - 1);
    if (!(avail > 0)) return false;
    long long sum = 0;
    for (int x : w) sum += x;
    auto offset = [&](long long cum) {
        if (cum <= 0) return 0.0;
        if (cum >= sum) return avail;
        return std::min(std::round(avail * static_cast<double>(cum) / static_cast<double>(sum)), avail);
    };
    start.assign(n, 0); end.assign(n, 0);
    long long cum = 0;
    for (size_t i = 0; i < n; ++i) {
        const double base = static_cast<double>(origin) + static_cast<double>(gap) * static_cast<double>(i);
        start[i] = static_cast<float>(base + offset(cum));
        cum += w[i];
        end[i] = static_cast<float>(base + offset(cum));
    }
    return true;
}
Thresholds make(Size c, Size r, Size d) { Thresholds t; t.min = {c, r, d}; return t; }
} // namespace

bool valid(const Thresholds& t) {
    for (size_t i = 0; i < density_count; ++i) {
        if (!finite_positive(t.min[i].width) || !finite_positive(t.min[i].height)) return false;
        if (i && (t.min[i].width < t.min[i - 1].width || t.min[i].height < t.min[i - 1].height)) return false;
    }
    return true;
}

std::optional<Density> choose(const Thresholds& t, Size area) {
    if (!valid(t) || !std::isfinite(area.width) || !std::isfinite(area.height)) return std::nullopt;
    for (size_t i = density_count; i-- > 0;)
        if (area.width >= t.min[i].width && area.height >= t.min[i].height) return static_cast<Density>(i);
    return std::nullopt;
}

const std::vector<std::string>& DensityTable::types() {
    static const std::vector<std::string> list{"text", "metric", "bar", "list", "clock"};
    return list;
}
DensityTable::DensityTable() { reset(); }
void DensityTable::reset() {
    // Logische Pixel (480x320-Plugin-Seite: Nutzfläche etwa 460x262). Kompakt = nur Wert,
    // normal = Bezeichnung + Wert, ausführlich = zusätzlich Details. Phase 2b: aus der Zeichengeometrie von
    // plugin_render.cpp abgeleitet (Bezeichnungszeile 22, Rand oben/unten 6, Zeilenabstand 18, Schrift 11..26).
    //   text:   ausführlich = Wert umbrochen (2 Zeilen)       metric/clock: ausführlich = große Schrift, Einheit klein
    //   bar:    ausführlich = Zahl groß + Prozent am Kopf     list: normal höchstens 2 Zeilen, ausführlich bis 5 und "+N"
    entries_ = {
        {"text",   make({56, 22}, {80, 42}, {120, 64})},
        {"metric", make({48, 28}, {72, 50}, {110, 66})},
        {"bar",    make({56, 22}, {80, 58}, {120, 72})},
        {"list",   make({60, 22}, {96, 46}, {130, 82})},
        {"clock",  make({52, 28}, {72, 50}, {110, 66})},
    };
}
const Thresholds* DensityTable::find(std::string_view type) const {
    for (const auto& [name, t] : entries_) if (name == type) return &t;
    return nullptr;
}
bool DensityTable::set(std::string_view type, const Thresholds& thresholds) {
    if (!valid(thresholds)) return false;
    for (auto& [name, t] : entries_) if (name == type) { t = thresholds; return true; }
    return false;
}

Size TableBlock::min_size(Density density) const {
    const auto* t = table_ ? table_->find(type_) : nullptr;
    const auto index = static_cast<size_t>(density);
    if (!t || index >= density_count) return {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    return t->min[index];
}

BlockInput describe(const Block& block, std::string zone) {
    BlockInput input{block.id(), std::move(zone), {}};
    for (size_t i = 0; i < density_count; ++i) input.min.min[i] = block.min_size(static_cast<Density>(i));
    if (!valid(input.min)) {
        const float never = std::numeric_limits<float>::infinity();
        for (auto& s : input.min.min) s = {never, never};
    }
    return input;
}

BlockInput describe(std::string id, std::string_view type, const DensityTable& table, std::string zone) {
    BlockInput input{std::move(id), std::move(zone), {}};
    const auto* t = table.find(type);
    const float never = std::numeric_limits<float>::infinity();
    if (t) input.min = *t;
    else for (auto& s : input.min.min) s = {never, never};
    return input;
}

bool valid(const Layout& layout,size_t zone_limit,bool allow_empty) {
    if (!weights(layout.columns, max_columns) || !weights(layout.rows, max_rows)) return false;
    if ((!allow_empty && layout.zones.empty()) || layout.zones.size() > zone_limit) return false;
    const int columns = static_cast<int>(layout.columns.size()), rows = static_cast<int>(layout.rows.size());
    std::set<std::string> ids;
    std::vector<char> used(static_cast<size_t>(columns * rows), 0);
    for (const auto& z : layout.zones) {
        if (!identifier(z.id) || !ids.insert(z.id).second) return false;
        if (z.column < 0 || z.column >= columns || z.row < 0 || z.row >= rows) return false;
        if (z.column_span < 1 || z.column_span > columns - z.column || z.row_span < 1 || z.row_span > rows - z.row) return false;
        for (int r = z.row; r < z.row + z.row_span; ++r)
            for (int c = z.column; c < z.column + z.column_span; ++c) {
                auto& cell = used[static_cast<size_t>(r * columns + c)];
                if (cell) return false;
                cell = 1;
            }
    }
    return true;
}

Plan plan(const Rect& area, const Layout& layout, const std::vector<BlockInput>& blocks, float gap) {
    Plan out;
    for (const auto& z : layout.zones) out.zones.push_back({z.id, {}});
    for (const auto& b : blocks) out.blocks.push_back({b.id, b.zone, {}, std::nullopt});
    if (!std::isfinite(area.x) || !std::isfinite(area.y) || std::abs(area.x) > max_area || std::abs(area.y) > max_area ||
        !finite_positive(area.width) || !finite_positive(area.height) ||
        !std::isfinite(gap) || gap < 0 || gap > max_area || !valid(layout)) return out;
    std::vector<float> col_start, col_end, row_start, row_end;
    if (!tracks(area.x, area.width, layout.columns, gap, col_start, col_end) ||
        !tracks(area.y, area.height, layout.rows, gap, row_start, row_end)) return out;
    for (size_t zi = 0; zi < layout.zones.size(); ++zi) {
        const auto& z = layout.zones[zi];
        const auto c0 = static_cast<size_t>(z.column), c1 = static_cast<size_t>(z.column + z.column_span - 1);
        const auto r0 = static_cast<size_t>(z.row), r1 = static_cast<size_t>(z.row + z.row_span - 1);
        const Rect zone{col_start[c0], row_start[r0], col_end[c1] - col_start[c0], row_end[r1] - row_start[r0]};
        out.zones[zi].area = zone;
        // Blöcke dieser Zone in Manifest-Reihenfolge untereinander; nicht passende werden ausgeblendet
        // (immer der letzte nicht passende zuerst), dann wird der Platz neu verteilt.
        std::vector<size_t> visible;
        for (size_t bi = 0; bi < blocks.size(); ++bi) if (blocks[bi].zone == z.id) visible.push_back(bi);
        if (visible.size() > static_cast<size_t>(max_blocks_per_zone)) visible.resize(max_blocks_per_zone);
        while (!visible.empty()) {
            std::vector<float> start, end;
            const std::vector<int> equal(visible.size(), 1);
            std::optional<size_t> misfit;
            if (!tracks(zone.y, zone.height, equal, gap, start, end) || zone.width <= 0) misfit = visible.size() - 1;
            else {
                for (size_t k = 0; k < visible.size(); ++k) {
                    auto& placed = out.blocks[visible[k]];
                    placed.area = {zone.x, start[k], zone.width, end[k] - start[k]};
                    placed.density = choose(blocks[visible[k]].min, {placed.area.width, placed.area.height});
                    if (!placed.density) misfit = k;
                }
            }
            if (!misfit) break;
            out.blocks[visible[*misfit]].area = {};
            out.blocks[visible[*misfit]].density.reset();
            visible.erase(visible.begin() + static_cast<std::ptrdiff_t>(*misfit));
            for (auto index : visible) { out.blocks[index].area = {}; out.blocks[index].density.reset(); }
        }
    }
    return out;
}
} // namespace fl::plugins::zones
