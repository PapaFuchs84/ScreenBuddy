// © 2026 FuchsLabs – Manifest v2 (Zonen-Layout) und Zonen-Geometrie, Phase 2a.
// Rein: keine Dateien, kein Netz, keine Grafik, kein Windows-API.
// Standalone: c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp src/plugin_zones.cpp tests/plugin_zones_tests.cpp
#include "plugins.h"
#include "plugins_testing.h"
#include "plugin_isolation.h"
#include "plugin_zones.h"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <cstdio>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

using namespace fl::plugins;
namespace z = fl::plugins::zones;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

// ---------------------------------------------------------------- Manifest v2
static Json v1() {
    return {{"schema_version", 1}, {"id", "zonen-test"}, {"name", "Zonen"}, {"version", "1.0.0"},
        {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}};
}
static Json v2_plain() {
    auto j = v1();
    j["schema_version"] = 2;
    j["widgets"][0]["id"] = "uhr";
    return j;
}
static Json v2() {
    Json j = {{"schema_version", 2}, {"id", "zonen-test"}, {"name", "Zonen"}, {"version", "1.0.0"}};
    j["sources"] = Json::array({{{"id", "feed"}, {"url", "https://api.example.com/v1"},
        {"sample", {{"temp", 21}, {"load", 40}, {"rows", Json::array({{{"t", "A"}}, {{"t", "B"}}})}}}}});
    j["layout"] = {{"columns", {2, 1}}, {"rows", {1, 1}}, {"zones", Json::array({
        {{"id", "main"}, {"column", 0}, {"row", 0}, {"row_span", 2}},
        {{"id", "side"}, {"column", 1}, {"row", 0}},
        {{"id", "foot"}, {"column", 1}, {"row", 1}}})}};
    j["widgets"] = Json::array({
        {{"type", "metric"}, {"id", "temp"}, {"zone", "main"}, {"label", "Temperatur"}, {"value", "{{feed.temp}}"}, {"unit", "°C"}},
        {{"type", "bar"}, {"id", "load"}, {"zone", "main"}, {"value", "{{feed.load}}"}, {"min", 0}, {"max", 100}},
        {{"type", "list"}, {"id", "rows"}, {"zone", "side"}, {"items", "feed.rows"}, {"item_template", "{{item.t}}"}, {"max_items", 2}},
        {{"type", "clock"}, {"id", "uhr"}, {"zone", "foot"}, {"format", "time"}}});
    return j;
}
static bool accepts(const Json& j) { return static_cast<bool>(parse_manifest(j.dump())); }
static bool rejects(const std::function<void(Json&)>& mutation) { auto j = v2(); mutation(j); return !accepts(j); }
static std::string error_of(const Json& j) { return parse_manifest(j.dump()).error; }

static void test_v1_unchanged() {
    // v1 kennt weder layout noch Block-IDs noch zone; Grenzen bleiben 6 Widgets.
    CHECK(accepts(v1()));
    auto parsed = parse_manifest(v1().dump());
    CHECK(parsed && parsed.manifest->schema_version == 1 && !parsed.manifest->layout && parsed.manifest->widgets[0].id.empty());
    auto with_layout = v1(); with_layout["layout"] = v2()["layout"];
    CHECK(!accepts(with_layout));
    CHECK(error_of(with_layout) == "Unknown or misplaced field.");
    auto with_id = v1(); with_id["widgets"][0]["id"] = "uhr";
    CHECK(!accepts(with_id));
    auto with_zone = v1(); with_zone["widgets"][0]["zone"] = "main";
    CHECK(!accepts(with_zone));
    auto seven = v1();
    for (int i = 0; i < 6; ++i) seven["widgets"].push_back(seven["widgets"][0]);
    CHECK(!accepts(seven));
    CHECK(error_of(seven) == "A plugin needs 1 to 6 widgets.");
    // Schema-Nummer: nur 1 und 2 als ganze Zahl.
    for (Json bad : {Json(0), Json(3), Json(2.0), Json(1.0), Json("2"), Json(true), Json(-2), Json(nullptr)}) {
        auto j = v2(); j["schema_version"] = bad; CHECK(!accepts(j));
    }
    auto three = v1(); three["schema_version"] = 3;
    CHECK(error_of(three) == "Integer outside allowed range.");
}

static void test_v2_accepts() {
    auto plain = parse_manifest(v2_plain().dump());
    CHECK(plain && plain.manifest->schema_version == 2 && !plain.manifest->layout && plain.manifest->widgets[0].id == "uhr");
    auto zoned_plain = v2_plain(); zoned_plain["widgets"][0]["zone"] = "main"; // zone ohne layout
    CHECK(!accepts(zoned_plain));

    auto parsed = parse_manifest(v2().dump());
    CHECK(parsed);
    if (!parsed) { std::printf("v2 error: %s\n", parsed.error.c_str()); return; }
    const auto& m = *parsed.manifest;
    CHECK(m.layout && m.layout->columns == std::vector<int>({2, 1}) && m.layout->rows == std::vector<int>({1, 1}));
    CHECK(m.layout->zones.size() == 3 && m.layout->zones[0].row_span == 2 && m.layout->zones[0].column_span == 1);
    CHECK(z::valid(*m.layout));
    CHECK(m.widgets.size() == 4 && m.widgets[0].id == "temp" && m.widgets[0].zone == "main" && m.widgets[3].zone == "foot");
    // Rundreise über definition (so speichert plugin_store.cpp).
    auto again = parse_manifest(m.definition.dump());
    CHECK(again && again.manifest->definition == m.definition && again.manifest->layout->zones.size() == 3);
    // Rendermodell trägt IDs, Zonen und Layout weiter; Werte wie in v1.
    auto page = render_page(m, {sample_data(m), Json::object(), "12:00", "06.11.2026"});
    CHECK(page.layout && page.layout->zones.size() == 3);
    CHECK(page.widgets.size() == 4 && page.widgets[0].id == "temp" && page.widgets[0].zone == "main" && page.widgets[0].text == "21");
    CHECK(page.widgets[1].bar_fraction && std::abs(*page.widgets[1].bar_fraction - 0.4) < 1e-9);
    CHECK(page.widgets[2].items.size() == 2 && page.widgets[3].text == "12:00" && !page.missing_data);

    // Ohne layout auch in v2 höchstens 6 Widgets (automatisches Raster); mit layout 12 (4 Zonen à 3), 13 nicht.
    auto six = v2_plain();
    six["widgets"] = Json::array();
    for (int i = 0; i < 6; ++i) six["widgets"].push_back({{"type", "clock"}, {"id", "c" + std::to_string(i)}, {"format", "time"}});
    CHECK(accepts(six));
    six["widgets"].push_back({{"type", "clock"}, {"id", "c6"}, {"format", "time"}});
    CHECK(!accepts(six));
    CHECK(error_of(six) == "A plugin needs 1 to 6 widgets.");
    auto twelve = v2_plain();
    twelve["layout"] = {{"columns", {1, 1}}, {"rows", {1, 1}}, {"zones", Json::array({
        {{"id", "a"}, {"column", 0}, {"row", 0}}, {{"id", "b"}, {"column", 1}, {"row", 0}},
        {{"id", "c"}, {"column", 0}, {"row", 1}}, {{"id", "d"}, {"column", 1}, {"row", 1}}})}};
    twelve["widgets"] = Json::array();
    for (int i = 0; i < 12; ++i)
        twelve["widgets"].push_back({{"type", "clock"}, {"id", "c" + std::to_string(i)}, {"zone", std::string(1, static_cast<char>('a' + i % 4))}, {"format", "time"}});
    CHECK(accepts(twelve));
    twelve["widgets"].push_back({{"type", "clock"}, {"id", "c12"}, {"zone", "a"}, {"format", "time"}});
    CHECK(!accepts(twelve));
    CHECK(error_of(twelve) == "A plugin with a layout needs 1 to 12 widgets.");

    // Maximales Raster 6x6 mit 8 Zonen.
    auto big = v2();
    big["layout"] = {{"columns", {1, 1, 1, 1, 1, 12}}, {"rows", {12, 1, 1, 1, 1, 1}}, {"zones", Json::array()}};
    big["widgets"] = Json::array();
    for (int i = 0; i < 8; ++i) {
        big["layout"]["zones"].push_back({{"id", "z" + std::to_string(i)}, {"column", i % 6}, {"row", i / 6}});
        big["widgets"].push_back({{"type", "clock"}, {"id", "b" + std::to_string(i)}, {"zone", "z" + std::to_string(i)}, {"format", "date"}});
    }
    CHECK(accepts(big));
}

static void test_v2_rejects() {
    // Block-IDs: Pflicht, Bezeichnerformat, eindeutig.
    CHECK(rejects([](auto& j) { j["widgets"][0].erase("id"); }));
    CHECK(rejects([](auto& j) { j["widgets"][1]["id"] = "temp"; }));
    CHECK(error_of([] { auto j = v2(); j["widgets"][1]["id"] = "temp"; return j; }()) == "Invalid or duplicate block ID.");
    for (Json bad : {Json(""), Json("Temp"), Json("1temp"), Json("te-mp"), Json(std::string(41, 'a')), Json(7), Json("t\xE2\x80\xAE")})
        CHECK(rejects([&](auto& j) { j["widgets"][0]["id"] = bad; }));
    // Zonen der Blöcke.
    CHECK(rejects([](auto& j) { j["widgets"][0].erase("zone"); }));
    CHECK(rejects([](auto& j) { j["widgets"][0]["zone"] = "nirgends"; }));
    CHECK(rejects([](auto& j) { j["widgets"][0]["zone"] = 1; }));
    CHECK(rejects([](auto& j) { j["widgets"][3]["zone"] = "main"; })); // "foot" bliebe leer
    CHECK(error_of([] { auto j = v2(); j["widgets"][3]["zone"] = "main"; return j; }()) == "Every zone needs at least one block.");
    CHECK(rejects([](auto& j) { // 5 Blöcke in einer Zone
        for (int i = 0; i < 3; ++i) j["widgets"].push_back({{"type", "clock"}, {"id", "x" + std::to_string(i)}, {"zone", "main"}, {"format", "time"}});
    }));
    // Layout-Struktur.
    CHECK(rejects([](auto& j) { j["layout"] = Json::array(); }));
    CHECK(rejects([](auto& j) { j["layout"]["extra"] = 1; }));
    CHECK(rejects([](auto& j) { j["layout"].erase("columns"); }));
    CHECK(rejects([](auto& j) { j["layout"].erase("zones"); }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"] = Json::array(); }));
    CHECK(rejects([](auto& j) { j["layout"]["columns"] = Json::array(); }));
    CHECK(rejects([](auto& j) { j["layout"]["columns"] = {1, 1, 1, 1, 1, 1, 1}; }));
    CHECK(rejects([](auto& j) { j["layout"]["rows"] = {1, 1, 1, 1, 1, 1, 1}; }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][0]["zones"] = Json::array(); })); // keine Verschachtelung
    CHECK(rejects([](auto& j) { j["layout"]["zones"][0]["layout"] = j["layout"]; }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][0]["blocks"] = Json::array({"temp"}); }));
    // Zahlenformat der Anteile und Positionen: nur ganze Zahlen im Bereich.
    for (Json bad : {Json(0), Json(13), Json(-1), Json(1.5), Json(2.0), Json(1e0), Json("2"), Json(true), Json(nullptr),
                     Json(std::numeric_limits<long long>::max()), Json(std::numeric_limits<unsigned long long>::max())}) {
        CHECK(rejects([&](auto& j) { j["layout"]["columns"][0] = bad; }));
        CHECK(rejects([&](auto& j) { j["layout"]["rows"][1] = bad; }));
    }
    for (Json bad : {Json(-1), Json(2), Json(0.0), Json("0"), Json(1e300), Json(std::numeric_limits<unsigned long long>::max())})
        CHECK(rejects([&](auto& j) { j["layout"]["zones"][0]["column"] = bad; }));
    {
        const auto base = v2().dump();
        const std::string marker = "\"columns\":[2,1]";
        CHECK(base.find(marker) != std::string::npos);
        for (const char* number : {"1e999", "01", "NaN", "Infinity", "-0.0", "2.", "0x2", "+2", "2e0", "1.0000000000000002"}) {
            auto text = base;
            text.replace(text.find(marker), marker.size(), std::string("\"columns\":[") + number + ",1]");
            CHECK(!parse_manifest(text));
        }
        auto exact = base; // gleiches Manifest als Text: angenommen
        CHECK(parse_manifest(exact));
    }
    // Zonen: Duplikate, außerhalb, Spannweite, Überlappung.
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["id"] = "main"; }));
    CHECK(error_of([] { auto j = v2(); j["layout"]["zones"][1]["id"] = "main"; return j; }()) == "Invalid or duplicate zone ID.");
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["id"] = "Side"; }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1].erase("id"); }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1].erase("row"); }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["column"] = 2; }));  // außerhalb
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["row"] = 2; }));     // außerhalb
    CHECK(rejects([](auto& j) { j["layout"]["zones"][0]["row_span"] = 3; })); // ragt hinaus
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["column_span"] = 0; }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][1]["column_span"] = -1; }));
    CHECK(rejects([](auto& j) { j["layout"]["zones"][0]["column_span"] = 2; })); // überlappt side/foot
    CHECK(error_of([] { auto j = v2(); j["layout"]["zones"][0]["column_span"] = 2; return j; }()) == "Zones must not overlap.");
    CHECK(rejects([](auto& j) { j["layout"]["zones"][2]["row"] = 0; }));    // foot auf side
    CHECK(rejects([](auto& j) { // 9 Zonen
        j["layout"] = {{"columns", {1, 1, 1}}, {"rows", {1, 1, 1}}, {"zones", Json::array()}};
        j["widgets"] = Json::array();
        for (int i = 0; i < 9; ++i) {
            j["layout"]["zones"].push_back({{"id", "z" + std::to_string(i)}, {"column", i % 3}, {"row", i / 3}});
            j["widgets"].push_back({{"type", "clock"}, {"id", "b" + std::to_string(i)}, {"zone", "z" + std::to_string(i)}, {"format", "date"}});
        }
    }));
    // Doppelte Schlüssel im Layout (strenger JSON-Parser).
    auto text = v2().dump();
    auto dup = text; dup.replace(dup.find("\"columns\""), 9, "\"rows\":[1],\"columns\"");
    CHECK(!parse_manifest(dup));
    auto dup_zone = text; dup_zone.replace(dup_zone.find("\"column\":0"), 10, "\"column\":0,\"column\":1");
    CHECK(!parse_manifest(dup_zone));
    // Bestehende v1-Regeln gelten auch in v2 (Beispiele).
    CHECK(rejects([](auto& j) { j["widgets"][0]["value"] = "{{settings.missing}}"; }));
    CHECK(rejects([](auto& j) { j["widgets"][0]["label"] = "A\xE2\x80\x8B" "B"; }));
    CHECK(rejects([](auto& j) { j["widgets"][0]["extra"] = 1; }));
}

static void test_store_rules() {
    auto store = v2();
    store["description"] = "Zonen-Testplugin mit Beispieldaten";
    store["license"] = "MIT"; store["author"] = "FuchsLabs"; store["category"] = "info";
    store["min_app_version"] = "2.10.0";
    auto ok = parse_manifest(store.dump());
    CHECK(ok && store_problems(*ok.manifest).empty());
    store["min_app_version"] = "2.11.3";
    ok = parse_manifest(store.dump());
    CHECK(ok && store_problems(*ok.manifest).empty());
    store["min_app_version"] = "2.9.6";
    auto old = parse_manifest(store.dump());
    CHECK(old && store_problems(*old.manifest).size() == 1 &&
          store_problems(*old.manifest)[0] == "Store: schema_version 2 needs min_app_version 2.10.0 or newer.");
    CHECK(old && !app_compatible(*old.manifest, "2.9.5") && app_compatible(*old.manifest, "2.10.0"));
    // v1 bekommt keine neue Store-Regel.
    auto v1_store = v1();
    v1_store["description"] = "Uhr ohne Netzwerkzugriff"; v1_store["license"] = "MIT"; v1_store["author"] = "A";
    v1_store["category"] = "info"; v1_store["min_app_version"] = "2.5.0";
    auto v1_ok = parse_manifest(v1_store.dump());
    CHECK(v1_ok && store_problems(*v1_ok.manifest).empty());
}

// ---------------------------------------------------------------- Dichtestufen und Block-Schnittstelle
// Konkreter Block für Tests: zeichnet nichts (Zeichnen kommt in Teil b).
struct MeasureBlock : z::TableBlock {
    using z::TableBlock::TableBlock;
    void draw(fl::Canvas&, const z::Rect&, z::Density) const override {}
};
static z::Thresholds th(z::Size c, z::Size r, z::Size d) { z::Thresholds t; t.min = {c, r, d}; return t; }

static void test_density() {
    z::DensityTable table;
    for (const auto& type : z::DensityTable::types()) CHECK(table.find(type) && z::valid(*table.find(type)));
    CHECK(table.find("gauge") == nullptr);
    const auto metric_defaults = *table.find("metric");  // Wertkopie, bleibt nach set()/reset() unverändert
    const auto& metric = metric_defaults;
    CHECK(!z::choose(metric, {1, 1}));
    CHECK(z::choose(metric, metric.min[0]) == z::Density::compact);
    CHECK(z::choose(metric, metric.min[1]) == z::Density::regular);
    CHECK(z::choose(metric, metric.min[2]) == z::Density::detailed);
    CHECK(z::choose(metric, {metric.min[2].width, metric.min[1].height}) == z::Density::regular); // beide Maße zählen
    CHECK(!z::choose(metric, {NAN, 100}));
    CHECK(!z::choose(metric, {INFINITY, INFINITY})); // nicht endliche Fläche passt nie
    // Monotonie: größere Fläche ergibt nie eine niedrigere Stufe.
    for (const auto& type : z::DensityTable::types()) {
        const auto& t = *table.find(type);
        int last = -1;
        bool monotone = true;
        for (float s = 0; s <= 200; s += 2) {
            auto d = z::choose(t, {s * 1.5f, s});
            const int level = d ? static_cast<int>(*d) : -1;
            if (level < last) monotone = false;
            last = level;
        }
        CHECK(monotone && last == 2);
    }
    // Einstellbar je Bausteintyp, nur gültige Schwellen.
    CHECK(table.set("metric", th({10, 10}, {20, 20}, {30, 30})));
    CHECK(table.find("metric")->min[2].width == 30);
    CHECK(z::choose(*table.find("metric"), {25, 25}) == z::Density::regular);
    CHECK(!table.set("metric", th({20, 20}, {10, 10}, {30, 30})));  // nicht aufsteigend
    CHECK(!table.set("metric", th({0, 10}, {20, 20}, {30, 30})));   // null
    CHECK(!table.set("metric", th({-1, 10}, {20, 20}, {30, 30})));
    CHECK(!table.set("metric", th({NAN, 10}, {20, 20}, {30, 30})));
    CHECK(!table.set("metric", th({10, 10}, {20, 20}, {INFINITY, 30})));
    CHECK(!table.set("metric", th({10, 10}, {20, 20}, {20000, 30}))); // über max_area
    CHECK(!table.set("gauge", th({10, 10}, {20, 20}, {30, 30})));
    CHECK(table.find("metric")->min[2].width == 30); // alter Wert bleibt
    // reset() stellt die Standardwerte wieder her: Vergleich mit einer frischen Tabelle (nicht mit einer Referenz
    // in dieselbe Tabelle, die sich mit ändern würde) und mit den Werten aus der Zeit vor dem set().
    CHECK(table.find("metric")->min[2].width == 30);
    table.reset();
    {
        const z::DensityTable fresh;
        for (const auto& type : z::DensityTable::types()) {
            const auto* now = table.find(type);
            const auto* base = fresh.find(type);
            CHECK(now && base);
            if (!now || !base) continue;
            for (size_t i = 0; i < z::density_count; ++i)
                CHECK(now->min[i].width == base->min[i].width && now->min[i].height == base->min[i].height);
        }
        CHECK(table.find("metric")->min[2].width == metric_defaults.min[2].width);
        CHECK(table.find("metric")->min[2].width != 30);
    }

    // Block-Schnittstelle: TableBlock liefert Mindestgrößen aus der Tabelle.
    // Lebensdauer: temporäre Tabellen (const und nicht const) sind verboten, benannte Tabellen erlaubt.
    static_assert(!std::is_constructible_v<MeasureBlock, std::string, std::string, z::DensityTable>);
    static_assert(!std::is_constructible_v<MeasureBlock, std::string, std::string, z::DensityTable&&>);
    static_assert(!std::is_constructible_v<MeasureBlock, std::string, std::string, const z::DensityTable&&>);
    static_assert(std::is_constructible_v<MeasureBlock, std::string, std::string, z::DensityTable&>);
    static_assert(std::is_constructible_v<MeasureBlock, std::string, std::string, const z::DensityTable&>);
    ++passed;  // die Prüfungen oben sind Übersetzungszeit; dieser Zähler hält sie in der Testzahl sichtbar
    MeasureBlock block("temp", "metric", table);
    CHECK(block.id() == "temp" && block.min_size(z::Density::regular).width == table.find("metric")->min[1].width);
    auto input = z::describe(block, "main");
    CHECK(input.id == "temp" && input.zone == "main" && z::valid(input.min));
    MeasureBlock unknown("x", "gauge", table);
    CHECK(!z::valid(z::describe("x", "gauge", table, "main").min));
    CHECK(z::describe("t", "metric", table, "main").min.min[1].width == table.find("metric")->min[1].width);
    CHECK(!z::valid(z::describe(unknown, "main").min)); // unbekannter Typ passt nie
    struct Broken : z::Block {
        std::string name = "kaputt";
        const std::string& id() const override { return name; }
        z::Size min_size(z::Density d) const override { return d == z::Density::compact ? z::Size{50, 50} : z::Size{10, 10}; }
        void draw(fl::Canvas&, const z::Rect&, z::Density) const override {}
    } broken;
    auto broken_input = z::describe(broken, "main");
    CHECK(!z::choose(broken_input.min, {1000, 1000})); // nicht aufsteigend → nie sichtbar
}

// ---------------------------------------------------------------- Geometrie
static bool inside(const z::Rect& inner, const z::Rect& outer) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width + 1e-3f &&
           inner.y + inner.height <= outer.y + outer.height + 1e-3f;
}
static bool overlap(const z::Rect& a, const z::Rect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}
static bool sane(const z::Plan& p, const z::Rect& area) {
    for (size_t i = 0; i < p.zones.size(); ++i) {
        if (p.zones[i].area.width < 0 || p.zones[i].area.height < 0 || !inside(p.zones[i].area, area)) return false;
        for (size_t k = i + 1; k < p.zones.size(); ++k) if (overlap(p.zones[i].area, p.zones[k].area)) return false;
    }
    for (size_t i = 0; i < p.blocks.size(); ++i) {
        const auto& b = p.blocks[i];
        if (!b.visible()) { if (b.area.width != 0 || b.area.height != 0) return false; continue; }
        bool in_zone = false;
        for (const auto& zone : p.zones) if (zone.id == b.zone && inside(b.area, zone.area)) in_zone = true;
        if (!in_zone) return false;
        for (size_t k = i + 1; k < p.blocks.size(); ++k)
            if (p.blocks[k].visible() && overlap(b.area, p.blocks[k].area)) return false;
    }
    return true;
}
static z::Layout grid(std::vector<int> c, std::vector<int> r, std::vector<z::Zone> zs) { return {std::move(c), std::move(r), std::move(zs)}; }

static void test_geometry() {
    const z::DensityTable table;
    auto block = [&](std::string id, std::string zone, std::string type) {
        return z::describe(std::move(id), type, table, std::move(zone));
    };
    // Exakte Kanten: 2:1 auf 300 Breite ohne Abstand.
    auto layout = grid({2, 1}, {1, 1}, {{"main", 0, 0, 1, 2}, {"side", 1, 0, 1, 1}, {"foot", 1, 1, 1, 1}});
    CHECK(z::valid(layout));
    std::vector<z::BlockInput> blocks{block("temp", "main", "metric"), block("load", "main", "bar"),
                                      block("rows", "side", "list"), block("uhr", "foot", "clock")};
    auto p = z::plan({0, 0, 300, 200}, layout, blocks, 0);
    CHECK(p.zones.size() == 3 && p.blocks.size() == 4);
    CHECK(p.zones[0].area.x == 0 && p.zones[0].area.width == 200 && p.zones[0].area.height == 200);
    CHECK(p.zones[1].area.x == 200 && p.zones[1].area.width == 100 && p.zones[1].area.height == 100);
    CHECK(p.zones[2].area.y == 100 && p.zones[2].area.height == 100);
    CHECK(p.blocks[0].area.height == 100 && p.blocks[1].area.y == 100); // zwei Blöcke gestapelt
    CHECK(sane(p, {0, 0, 300, 200}));
    // Mit Abstand: Nachbarn teilen sich exakt die Kante + gap, letzte Kante am Rand.
    p = z::plan({10, 39, 460, 262}, layout, blocks, 7);
    CHECK(p.zones[0].area.x == 10 && p.zones[1].area.x == p.zones[0].area.x + p.zones[0].area.width + 7);
    CHECK(p.zones[1].area.x + p.zones[1].area.width == 470);
    CHECK(p.zones[2].area.y + p.zones[2].area.height == 301 && p.zones[2].area.y == p.zones[1].area.y + p.zones[1].area.height + 7);
    CHECK(p.blocks[1].area.y == p.blocks[0].area.y + p.blocks[0].area.height + 7);
    CHECK(sane(p, {10, 39, 460, 262}));
    // Ungerade Anteile: Summe der Breiten = Gesamtbreite, keine Lücke, keine Überlappung.
    auto thirds = grid({1, 1, 1}, {1}, {{"a", 0, 0, 1, 1}, {"b", 1, 0, 1, 1}, {"c", 2, 0, 1, 1}});
    std::vector<z::BlockInput> three{block("x", "a", "clock"), block("y", "b", "clock"), block("w", "c", "clock")};
    for (float width : {100.f, 101.f, 99.5f, 333.f, 479.f}) {
        auto t = z::plan({0, 0, width, 100}, thirds, three, 0);
        float sum = 0;
        for (const auto& zone : t.zones) sum += zone.area.width;
        CHECK(std::abs(sum - width) < 1e-3f && t.zones[1].area.x == t.zones[0].area.x + t.zones[0].area.width &&
              t.zones[2].area.x + t.zones[2].area.width == width && sane(t, {0, 0, width, 100}));
    }
    // Hoch- und Querformat, mehrere Auflösungen: immer innerhalb, nie überlappend.
    for (auto [w, h] : std::vector<std::pair<float, float>>{{480.f, 320.f}, {320.f, 480.f}, {240.f, 160.f}, {160.f, 240.f}, {1920.f, 480.f}, {800.f, 1280.f}}) {
        auto q = z::plan({0, 0, w, h}, layout, blocks, 6);
        CHECK(sane(q, {0, 0, w, h}));
    }
    // Zu kleine Fläche: Blöcke werden ausgeblendet statt zu überlappen, zuerst der letzte nicht passende.
    auto tiny = z::plan({0, 0, 160, 120}, layout, blocks, 6);
    CHECK(sane(tiny, {0, 0, 160, 120}));
    CHECK(tiny.blocks[2].visible() == false);       // Liste passt in 51x57 nicht (mind. 60x18 Breite)
    auto none = z::plan({0, 0, 20, 20}, layout, blocks, 2);
    CHECK(sane(none, {0, 0, 20, 20}));
    bool all_hidden = true;
    for (const auto& b : none.blocks) if (b.visible()) all_hidden = false;
    CHECK(all_hidden);
    // Ausblenden schafft Platz: zwei Blöcke, nur einer passt in voller Höhe.
    auto single = grid({1}, {1}, {{"z", 0, 0, 1, 1}});
    std::vector<z::BlockInput> stacked{block("a", "z", "metric"), block("b", "z", "metric")};
    const auto& metric = *table.find("metric");
    const float h = metric.min[1].height * 1.5f;
    auto s = z::plan({0, 0, 200, h}, single, stacked, 0);
    CHECK(s.blocks[0].visible() && s.blocks[1].visible()); // beide passen kompakt (je 31.5 hoch)
    s = z::plan({0, 0, 200, metric.min[0].height * 1.5f}, single, stacked, 0);
    CHECK(s.blocks[0].visible() && !s.blocks[1].visible() && s.blocks[0].area.height == metric.min[0].height * 1.5f);
    CHECK(s.blocks[0].density == z::Density::compact);
    // Mehr als 4 Blöcke in einer Zone: der Rest bleibt aus (Parser lässt das nicht zu, Geometrie ist trotzdem robust).
    std::vector<z::BlockInput> many;
    for (int i = 0; i < 6; ++i) many.push_back(block("b" + std::to_string(i), "z", "clock"));
    auto m = z::plan({0, 0, 400, 400}, single, many, 0);
    CHECK(m.blocks[3].visible() && !m.blocks[4].visible() && !m.blocks[5].visible() && sane(m, {0, 0, 400, 400}));
    // Blöcke ohne bekannte Zone bleiben aus.
    auto orphan = z::plan({0, 0, 400, 400}, single, {block("o", "nirgends", "clock")}, 0);
    CHECK(!orphan.blocks[0].visible());
    // Ungültige Flächen und Abstände: leerer Plan, alle Blöcke aus.
    for (z::Rect bad : {z::Rect{0, 0, 0, 0}, z::Rect{0, 0, -10, 100}, z::Rect{0, 0, 100, -1}, z::Rect{0, 0, NAN, 100},
                        z::Rect{0, 0, INFINITY, 100}, z::Rect{NAN, 0, 100, 100}, z::Rect{0, 0, 20000, 100}, z::Rect{1e30f, 0, 100, 100}}) {
        auto e = z::plan(bad, layout, blocks, 0);
        bool hidden = e.zones.size() == 3;
        for (const auto& b : e.blocks) if (b.visible()) hidden = false;
        for (const auto& zone : e.zones) if (zone.area.width != 0) hidden = false;
        CHECK(hidden);
    }
    for (float gap : {-1.f, NAN, INFINITY, 1000.f}) {
        auto e = z::plan({0, 0, 480, 320}, layout, blocks, gap);
        bool hidden = true;
        for (const auto& b : e.blocks) if (b.visible()) hidden = false;
        CHECK(hidden);
    }
    // Sehr große, noch gültige Fläche.
    auto large = z::plan({0, 0, 16384, 16384}, layout, blocks, 12);
    CHECK(sane(large, {0, 0, 16384, 16384}) && large.blocks[0].density == z::Density::detailed);
    // Ungültige Layouts (direkt an plan übergeben) ergeben nichts Sichtbares.
    for (auto bad : {grid({}, {1}, {{"a", 0, 0, 1, 1}}), grid({1}, {1}, {}), grid({0}, {1}, {{"a", 0, 0, 1, 1}}),
                     grid({13}, {1}, {{"a", 0, 0, 1, 1}}), grid({1}, {1}, {{"a", 1, 0, 1, 1}}), grid({1}, {1}, {{"a", 0, 0, 2, 1}}),
                     grid({1, 1}, {1}, {{"a", 0, 0, 2, 1}, {"b", 1, 0, 1, 1}}), grid({1, 1}, {1}, {{"a", 0, 0, 1, 1}, {"a", 1, 0, 1, 1}}),
                     grid({1}, {1}, {{"A", 0, 0, 1, 1}}), grid({1}, {1}, {{"a", -1, 0, 1, 1}}), grid({1}, {1}, {{"a", 0, 0, 1, 0}})}) {
        CHECK(!z::valid(bad));
        auto e = z::plan({0, 0, 480, 320}, bad, {block("x", "a", "clock")}, 0);
        CHECK(!e.blocks[0].visible());
    }
}

// ---------------------------------------------------------------- Fuzz-nahe Manifeste (fester Seed)
static Json random_value(std::mt19937& rng) {
    switch (rng() % 12) {
        case 0: return nullptr;
        case 1: return static_cast<int>(rng() % 20) - 5;
        case 2: return 1.5;
        case 3: return "main";
        case 4: return Json::array({1, 2});
        case 5: return Json::object();
        case 6: return true;
        case 7: return std::numeric_limits<long long>::min();
        case 8: return 1e300;
        case 9: return "z" + std::to_string(rng() % 4);
        case 10: return Json::array({{{"id", "q"}, {"column", 0}, {"row", 0}}});
        default: return static_cast<int>(rng() % 3);
    }
}
static Json* random_node(Json& root, std::mt19937& rng) {
    Json* node = &root;
    for (int depth = 0; depth < 5 && node->is_structured() && !node->empty() && rng() % 3; ++depth) {
        auto it = node->begin();
        std::advance(it, static_cast<long>(rng() % node->size()));
        node = &*it;
    }
    return node;
}
static void test_fuzz() {
    std::mt19937 rng(20261106u);
    int accepted = 0, exceptions = 0, broken = 0;
    const z::DensityTable table;
    for (int round = 0; round < 4000; ++round) {
        Json j = round % 2 ? v2() : v2_plain();
        const int edits = 1 + static_cast<int>(rng() % 4);
        for (int e = 0; e < edits; ++e) {
            Json* node = random_node(j, rng);
            if (node->is_object() && !node->empty() && rng() % 2) {
                auto it = node->begin(); std::advance(it, static_cast<long>(rng() % node->size()));
                if (rng() % 2) { const std::string key = it.key(); node->erase(key); } else it.value() = random_value(rng);
            } else if (node->is_array() && rng() % 2) node->push_back(random_value(rng));
            else *node = random_value(rng);
        }
        std::string text = j.dump();
        if (rng() % 5 == 0 && !text.empty()) text.resize(rng() % text.size()); // abgeschnittenes JSON
        try {
            auto parsed = parse_manifest(text);
            if (!parsed) continue;
            ++accepted;
            const auto& m = *parsed.manifest;
            const size_t limit = m.layout ? 12 : 6;
            bool ok = !m.widgets.empty() && m.widgets.size() <= limit;
            if (m.layout) {
                ok = ok && z::valid(*m.layout);
                std::vector<z::BlockInput> inputs;
                for (const auto& w : m.widgets) {
                    ok = ok && !w.id.empty() && !w.zone.empty();
                    inputs.push_back(z::describe(MeasureBlock(w.id, w.type, table), w.zone));
                }
                for (auto [w, h] : std::vector<std::pair<float, float>>{{480.f, 320.f}, {160.f, 240.f}})
                    ok = ok && sane(z::plan({0, 0, w, h}, *m.layout, inputs, 6), {0, 0, w, h});
            }
            auto again = parse_manifest(m.definition.dump());
            ok = ok && again && again.manifest->definition == m.definition;
            if (!ok) ++broken;
        } catch (...) { ++exceptions; }
    }
    CHECK(exceptions == 0);
    CHECK(broken == 0);
    CHECK(accepted > 0);
    std::printf("fuzz: 4000 manifests, %d accepted\n", accepted);
}

// ---------------------------------------------------------------- Phase 3: bedingte Anzeige (show_if)
static Json v2_cond() {
    auto j = v2();
    j["settings"] = Json::array({
        {{"key", "token"}, {"label", "Token"}, {"type", "secret"}},
        {{"key", "city"}, {"label", "Ort"}, {"type", "text"}, {"default", "Hamburg"}},
        {{"key", "limit"}, {"label", "Grenze"}, {"type", "number"}, {"default", 30}}});
    j["sources"][0]["headers"] = {{"Authorization", "Bearer {{settings.token}}"}};
    j["widgets"][0]["show_if"] = {{"path", "feed.temp"}};
    j["widgets"][1]["show_if"] = {{"path", "feed.load"}, {"op", ">="}, {"value", 50}};
    j["widgets"][2]["show_if"] = {{"path", "feed.rows"}};
    j["widgets"][3]["show_if"] = {{"path", "settings.city"}, {"op", "!="}, {"value", "Berlin"}};
    return j;
}
static Json with_condition(const Json& show_if) { auto j = v2_cond(); j["widgets"][0]["show_if"] = show_if; return j; }
static std::vector<std::string> shown_ids(const Page& p) { std::vector<std::string> out; for (const auto& w : p.widgets) out.push_back(w.id); return out; }
using Ids = std::vector<std::string>;

static void test_conditions_parse() {
    auto parsed = parse_manifest(v2_cond().dump());
    CHECK(parsed);
    if (!parsed) { std::printf("v2_cond error: %s\n", parsed.error.c_str()); return; }
    const auto& m = *parsed.manifest;
    CHECK(m.widgets[0].show_if && m.widgets[0].show_if->path == "feed.temp" && m.widgets[0].show_if->op.empty() &&
          m.widgets[0].show_if->value.is_null());
    CHECK(m.widgets[1].show_if && m.widgets[1].show_if->op == ">=" && m.widgets[1].show_if->value == 50);
    CHECK(m.widgets[3].show_if && m.widgets[3].show_if->value == "Berlin");
    auto again = parse_manifest(m.definition.dump()); // Rundreise wie plugin_store.cpp
    CHECK(again && again.manifest->definition == m.definition && again.manifest->widgets[1].show_if->op == ">=");
    // Gültige Formen: ganze Quelle vorhanden, Feld vorhanden, Vergleich gegen Text, Zahl, Wahrheitswert; Grenzen genau getroffen.
    const std::string long_field(condition_path_limit - 5, 'a');
    for (const Json& ok : {Json{{"path", "feed"}}, Json{{"path", "feed.rows.0.t"}, {"op", "=="}, {"value", "A"}},
                           Json{{"path", "settings.limit"}, {"op", "<="}, {"value", -3.5}},
                           Json{{"path", "settings.city"}, {"op", "=="}, {"value", "Hamburg"}},
                           Json{{"path", "feed.flag"}, {"op", "!="}, {"value", false}},
                           Json{{"path", "feed.n"}, {"op", ">"}, {"value", 1e300}},
                           Json{{"path", "feed.a.b.c.d.e.f.g"}}, Json{{"path", "feed." + long_field}},
                           Json{{"path", "feed.t"}, {"op", "!="}, {"value", std::string(condition_text_limit, 'x')}},
                           Json{{"path", "feed.t"}, {"op", "=="}, {"value", "Gr\xC3\xBC\xC3\x9F" "e"}}}) {
        CHECK(accepts(with_condition(ok)));
        if (!accepts(with_condition(ok))) std::printf("rejected: %s (%s)\n", ok.dump().c_str(), error_of(with_condition(ok)).c_str());
    }
    // v2 ohne layout kennt show_if ebenfalls; v1 nicht (unverändert "Unknown or misplaced field.").
    auto plain = v2_plain();
    plain["settings"] = Json::array({{{"key", "city"}, {"label", "Ort"}, {"type", "text"}}});
    plain["widgets"][0]["show_if"] = {{"path", "settings.city"}};
    CHECK(accepts(plain));
    auto old = v1();
    old["settings"] = plain["settings"];
    old["widgets"][0]["show_if"] = {{"path", "settings.city"}};
    CHECK(!accepts(old) && error_of(old) == "Unknown or misplaced field.");
    // Secrets nie in Bedingungen: weder als Vorhanden-Test noch im Vergleich (eigene, feste Meldung).
    for (const Json& secret : {Json{{"path", "settings.token"}}, Json{{"path", "settings.token"}, {"op", "=="}, {"value", "x"}},
                               Json{{"path", "settings.token"}, {"op", "!="}, {"value", "PRIVATE"}}}) {
        CHECK(!accepts(with_condition(secret)));
        CHECK(error_of(with_condition(secret)) == "Secret settings cannot appear in conditions.");
    }
    // Abgelehnt, jeweils mit fester Meldung (nie mit Manifestwerten).
    const std::vector<std::pair<Json, std::string>> bad = {
        {Json{{"path", "settings.nope"}}, "Unknown setting reference."},
        {Json{{"path", "settings"}}, "Setting reference must have exactly one key."},
        {Json{{"path", "settings.city.x"}}, "Setting reference must have exactly one key."},
        {Json{{"path", "other.x"}}, "Condition must reference a source or a setting."},
        {Json{{"path", "time"}}, "Condition must reference a source or a setting."},
        {Json{{"path", "date"}}, "Condition must reference a source or a setting."},
        {Json{{"path", "item.t"}}, "Condition must reference a source or a setting."},
        {Json{{"path", "feed..temp"}}, "Invalid condition path."},
        {Json{{"path", "feed.te-mp"}}, "Invalid condition path."},
        {Json{{"path", ".feed"}}, "Invalid condition path."},
        {Json{{"path", "feed."}}, "Invalid condition path."},
        {Json{{"path", "feed.temp "}}, "Invalid condition path."},
        {Json{{"path", "feed.temp>3"}}, "Invalid condition path."},
        {Json{{"path", "{{feed.temp}}"}}, "Invalid condition path."},
        {Json{{"path", "feed.a.b.c.d.e.f.g.h"}}, "Invalid condition path."},
        {Json{{"path", "feed." + long_field + "a"}}, "String length outside allowed range."},
        {Json{{"path", ""}}, "String length outside allowed range."},
        {Json{{"path", 5}}, "Missing or invalid string field."},
        {Json::object(), "Missing or invalid string field."},
        {Json{{"path", "feed.temp"}, {"op", "="}, {"value", 1}}, "Unsupported condition operator."},
        {Json{{"path", "feed.temp"}, {"op", "<>"}, {"value", 1}}, "Unsupported condition operator."},
        {Json{{"path", "feed.temp"}, {"op", "in"}, {"value", 1}}, "Unsupported condition operator."},
        {Json{{"path", "feed.temp"}, {"op", "&&"}, {"value", 1}}, "Unsupported condition operator."},
        {Json{{"path", "feed.temp"}, {"op", "==="}, {"value", 1}}, "String length outside allowed range."},
        {Json{{"path", "feed.temp"}, {"op", ""}, {"value", 1}}, "String length outside allowed range."},
        {Json{{"path", "feed.temp"}, {"op", 1}, {"value", 1}}, "Missing or invalid string field."},
        {Json{{"path", "feed.temp"}, {"op", "=="}}, "A comparison needs a value."},
        {Json{{"path", "feed.temp"}, {"value", 1}}, "A condition value needs an operator."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", nullptr}}, "Condition value must be text, a number or a boolean."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", Json::array({1})}}, "Condition value must be text, a number or a boolean."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", Json::object()}}, "Condition value must be text, a number or a boolean."},
        {Json{{"path", "feed.temp"}, {"op", "<"}, {"value", "10"}}, "Only numbers can be compared with <, <=, > or >=."},
        {Json{{"path", "feed.temp"}, {"op", ">="}, {"value", true}}, "Only numbers can be compared with <, <=, > or >=."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", ""}}, "String length outside allowed range."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", std::string(condition_text_limit + 1, 'x')}}, "String length outside allowed range."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", "a\nb"}}, "Control, bidirectional or zero-width characters are not allowed in display text."},
        {Json{{"path", "feed.temp"}, {"op", "=="}, {"value", "a\xE2\x80\xAE" "b"}}, "Control, bidirectional or zero-width characters are not allowed in display text."},
        {Json{{"path", "feed"}, {"op", "=="}, {"value", 1}}, "Comparisons need a field path."},
        {Json{{"path", "feed.temp"}, {"not", true}}, "Unknown or misplaced field."},
        {Json{{"path", "feed.temp"}, {"and", Json::array()}}, "Unknown or misplaced field."},
        {Json("feed.temp > 3"), "Expected an object."},
        {Json::array({"feed.temp"}), "Expected an object."},
        {Json(true), "Expected an object."},
        {Json(nullptr), "Expected an object."}};
    for (const auto& [condition, message] : bad) {
        const auto j = with_condition(condition);
        CHECK(!accepts(j));
        CHECK(error_of(j) == message);
        if (error_of(j) != message) std::printf("%s -> '%s' (expected '%s')\n", condition.dump().c_str(), error_of(j).c_str(), message.c_str());
    }
    // Strenger JSON-Parser auch in show_if: doppelte Schlüssel und nicht endliche Zahlen kippen das ganze Manifest.
    const auto text = v2_cond().dump();
    const std::string marker = "{\"path\":\"feed.temp\"}";
    CHECK(text.find(marker) != std::string::npos);
    auto dup = text; dup.replace(dup.find(marker), marker.size(), "{\"path\":\"feed.temp\",\"path\":\"settings.token\"}");
    CHECK(!parse_manifest(dup));
    auto inf = text; inf.replace(inf.find(marker), marker.size(), "{\"path\":\"feed.temp\",\"op\":\">\",\"value\":1e999}");
    CHECK(!parse_manifest(inf));
    // Ein einziger ungültiger show_if lehnt das komplette Manifest ab (nicht nur den Block).
    auto one_bad = v2_cond(); one_bad["widgets"][3]["show_if"]["op"] = "~";
    CHECK(!accepts(one_bad));
}

// Wertzustands-Tabelle: Zeilen = Wert an `path`, Spalten = Bedingung; T = Block sichtbar. Dieselbe Tabelle steht in
// docs/PLUGIN-SYSTEM.md ("Bedingte Anzeige").
static void test_condition_table() {
    const std::vector<std::pair<const char*, Condition>> columns = {
        {"vorhanden", {"f.x", "", nullptr}}, {"==0", {"f.x", "==", 0}}, {"!=0", {"f.x", "!=", 0}}, {">0", {"f.x", ">", 0}},
        {">=0", {"f.x", ">=", 0}}, {"<1", {"f.x", "<", 1}}, {"==abc", {"f.x", "==", "abc"}}, {"!=abc", {"f.x", "!=", "abc"}},
        {"==true", {"f.x", "==", true}}, {"==false", {"f.x", "==", false}}, {"!=false", {"f.x", "!=", false}}};
    struct Row { const char* name; std::optional<Json> value; const char* expected; };
    const std::vector<Row> rows = {
        //                                          vorh ==0 !=0 >0 >=0 <1 ==abc !=abc ==t ==f !=f
        {"fehlend", std::nullopt,                   "F    F   F   F  F   F  F     F     F   F   F"},
        {"null", Json(nullptr),                     "F    F   F   F  F   F  F     F     F   F   F"},
        {"nicht endlich", Json(NAN),                "F    F   F   F  F   F  F     F     F   F   F"},
        {"\"\"", Json(""),                          "F    F   F   F  F   F  F     F     F   F   F"},
        {"[]", Json::array(),                       "F    F   F   F  F   F  F     F     F   F   F"},
        {"{}", Json::object(),                      "F    F   F   F  F   F  F     F     F   F   F"},
        {"false", Json(false),                      "T    F   F   F  F   F  F     F     F   T   F"},
        {"true", Json(true),                        "T    F   F   F  F   F  F     F     T   F   T"},
        {"0", Json(0),                              "T    T   F   F  T   T  F     F     F   F   F"},
        {"-0.0", Json(-0.0),                        "T    T   F   F  T   T  F     F     F   F   F"},
        {"2.5", Json(2.5),                          "T    F   T   T  T   F  F     F     F   F   F"},
        {"-1", Json(-1),                            "T    F   T   F  F   T  F     F     F   F   F"},
        {"2^64-1", Json(18446744073709551615ull),   "T    F   T   T  T   F  F     F     F   F   F"},
        {"\"0\"", Json("0"),                        "T    F   F   F  F   F  F     T     F   F   F"},
        {"\"abc\"", Json("abc"),                    "T    F   F   F  F   F  T     F     F   F   F"},
        {"\"ABC\"", Json("ABC"),                    "T    F   F   F  F   F  F     T     F   F   F"},
        {"\" \"", Json(" "),                        "T    F   F   F  F   F  F     T     F   F   F"},
        {"\"true\"", Json("true"),                  "T    F   F   F  F   F  F     T     F   F   F"},
        {"[1]", Json::array({1}),                   "T    F   F   F  F   F  F     F     F   F   F"},
        {"{a:1}", Json{{"a", 1}},                   "T    F   F   F  F   F  F     F     F   F   F"}};
    for (const auto& row : rows) {
        std::string expected;
        for (char c : std::string(row.expected)) if (c == 'T' || c == 'F') expected += c;
        CHECK(expected.size() == columns.size());
        std::string actual;
        for (const auto& [name, condition] : columns) actual += condition_holds(condition, row.value ? &*row.value : nullptr) ? 'T' : 'F';
        CHECK(actual == expected);
        if (actual != expected) std::printf("table row %s: %s (expected %s)\n", row.name, actual.c_str(), expected.c_str());
    }
    // Unbekannte Operatoren oder Literaltypen (nie vom Parser erzeugt) schlagen geschlossen fehl.
    const Json one(1);
    CHECK(!condition_holds({"f.x", "~", 1}, &one) && !condition_holds({"f.x", "==", nullptr}, &one) &&
          !condition_holds({"f.x", "==", Json::array({1})}, &one) && !condition_holds({"f.x", "<", "1"}, &one));
}

static void test_condition_render() {
    auto parsed = parse_manifest(v2_cond().dump());
    CHECK(parsed);
    if (!parsed) return;
    const auto& m = *parsed.manifest;
    RenderContext ctx{sample_data(m), Json::object(), "12:00", "06.11.2026"};
    // Vorschau mit Beispieldaten (so rendert App::plugin_preview): load 40 >= 50 ist falsch.
    auto page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"temp", "rows", "uhr"}) && page.hidden == Ids({"load"}) && !page.missing_data && page.failed.empty());
    for (size_t i = 0; i < page.widgets.size(); ++i) CHECK(page.widgets[i].index == (i == 0 ? 0u : i + 1));
    // Live ohne Antwort (Vorschau "missing"): alle Blöcke mit Quellbedingung aus, Fußzeile meldet fehlende Daten.
    ctx.sources.clear();
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"uhr"}) && page.hidden == Ids({"temp", "load", "rows"}) && page.missing_data);
    // Antwort da, Felder fehlen: ausgeblendet, aber keine "fehlenden Daten" (genau dafür ist show_if da).
    ctx.sources = {{"feed", {{"other", 1}}}};
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"uhr"}) && !page.missing_data);
    // null / leer / Wert: null und [] blenden aus, 80 >= 50 zeigt den Balken.
    ctx.sources = {{"feed", {{"temp", nullptr}, {"load", 80}, {"rows", Json::array()}}}};
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"load", "uhr"}) && !page.missing_data);
    CHECK(page.widgets.size() == 2 && page.widgets[0].bar_fraction && std::abs(*page.widgets[0].bar_fraction - 0.8) < 1e-9);
    // Typen werden nicht umgewandelt: "80" ist keine Zahl. 0 ist vorhanden.
    ctx.sources = {{"feed", {{"temp", 0}, {"load", "80"}, {"rows", Json::array({{{"t", "A"}}})}}}};
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"temp", "rows", "uhr"}) && page.widgets[0].text == "0");
    // Einstellungen: Standardwert, überschrieben, ungültig (dann wie fehlend und "Daten fehlen").
    ctx.sources = sample_data(m);
    ctx.settings = {{"city", "Berlin"}, {"token", "PRIVATE"}};
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"temp", "rows"}) && page.hidden == Ids({"load", "uhr"}));
    ctx.settings = {{"city", 5}};
    page = render_page(m, ctx);
    CHECK(shown_ids(page) == Ids({"temp", "rows"}) && page.missing_data);
    // Beispieldaten fehlen ganz (Quelle ohne sample): in der Vorschau ist die Quelle leer, also alle Quellblöcke aus.
    auto no_sample = v2_cond(); no_sample["sources"][0].erase("sample");
    auto ns = parse_manifest(no_sample.dump());
    CHECK(ns);
    if (ns) {
        RenderContext preview{sample_data(*ns.manifest), Json::object(), "12:00", "06.11.2026"};
        auto p = render_page(*ns.manifest, preview);
        CHECK(shown_ids(p) == Ids({"uhr"}) && !p.missing_data);
        auto whole = no_sample; whole["widgets"][0]["show_if"] = {{"path", "feed"}};
        auto w = parse_manifest(whole.dump());
        CHECK(w && render_page(*w.manifest, preview).hidden.front() == "temp");
        preview.sources = {{"feed", {{"x", 1}}}};
        CHECK(w && shown_ids(render_page(*w.manifest, preview)).front() == "temp"); // ganze Quelle vorhanden
    }
    // Zonen: eine Zone, deren Blöcke alle ausgeblendet sind, bleibt leer; die Geometrie bleibt gültig, nichts fließt um.
    ctx.settings = Json::object();
    ctx.sources.clear();
    page = render_page(m, ctx);
    const z::DensityTable table;
    std::vector<z::BlockInput> inputs;
    for (const auto& w : page.widgets) inputs.push_back(z::describe(w.id, w.type, table, w.zone));
    const auto plan = z::plan({10, 39, 460, 252}, *page.layout, inputs, 7);
    CHECK(plan.zones.size() == 3 && plan.blocks.size() == 1 && plan.blocks[0].visible() && sane(plan, {10, 39, 460, 252}));
    CHECK(plan.zones[2].area.width < 200);  // "foot" behält seine Rasterfläche, auch wenn "main" leer ist
}

// Fehlerinjektion in die Datenauswertung über den Test-Hook (plugins_testing.h; die App ruft render_page ohne Hook).
static void test_data_isolation() {
    auto parsed = parse_manifest(v2_cond().dump());
    CHECK(parsed);
    if (!parsed) return;
    const auto& m = *parsed.manifest;
    const RenderContext ctx{sample_data(m), Json::object(), "12:00", "06.11.2026"};
    const auto reference = render_page(m, ctx);
    testing::DataHooks none;
    auto same = testing::render_page(m, ctx, none);
    CHECK(shown_ids(same) == shown_ids(reference) && same.hidden == reference.hidden && same.failed.empty());
    for (const char* target : {"temp", "rows", "uhr"}) {
        testing::DataHooks hooks;
        hooks.before_block = [&](const std::string& key) { if (key == target) throw std::runtime_error("GEHEIM-123"); };
        auto page = testing::render_page(m, ctx, hooks);
        CHECK(page.failed == Ids({target}) && page.missing_data);
        Ids expected;
        for (const auto& w : reference.widgets) if (w.id != target) expected.push_back(w.id);
        CHECK(shown_ids(page) == expected);
        for (const auto& w : page.widgets) // übrige Blöcke unverändert
            for (const auto& r : reference.widgets)
                if (r.id == w.id) CHECK(r.text == w.text && r.items == w.items && r.bar_fraction == w.bar_fraction && r.index == w.index);
    }
    testing::DataHooks odd;  // keine std::exception
    odd.before_block = [](const std::string& key) { if (key == "rows") throw 42; };
    CHECK(testing::render_page(m, ctx, odd).failed == Ids({"rows"}));
    testing::DataHooks all;
    all.before_block = [](const std::string&) { throw std::bad_alloc(); };
    auto empty = testing::render_page(m, ctx, all);
    CHECK(empty.widgets.empty() && empty.hidden.empty() && empty.failed.size() == 4 && empty.missing_data);
    // Fehler außerhalb jedes Blocks (äußere Fehlergrenze): Seite ohne Blöcke, Daten fehlen, ID/Titel bleiben, keine Ausnahme.
    for (int kind = 0; kind < 2; ++kind) {
        testing::DataHooks outside;
        outside.outside_blocks = [kind] { if (kind == 0) throw std::bad_alloc(); throw 5; };
        bool threw = false;
        Page page;
        try { page = testing::render_page(m, ctx, outside); } catch (...) { threw = true; }
        CHECK(!threw && page.widgets.empty() && page.hidden.empty() && page.failed.empty() && page.missing_data &&
              page.id == m.id && page.title == m.name && !page.layout);
    }
    // v1: Schlüssel "#<Index>".
    auto old = parse_manifest(v1().dump());
    CHECK(old);
    if (old) {
        testing::DataHooks first;
        first.before_block = [](const std::string& key) { if (key == "#0") throw std::runtime_error("x"); };
        auto page = testing::render_page(*old.manifest, {}, first);
        CHECK(page.failed == Ids({"#0"}) && page.widgets.empty());
    }
}

static void test_rate_limiter() {
    using namespace std::chrono_literals;
    namespace iso = fl::plugins::isolation;
    iso::RateLimiter limiter(60s, 3);
    const iso::RateLimiter::Clock::time_point t0{};
    CHECK(limiter.allow("a", t0) && !limiter.allow("a", t0 + 59s) && limiter.allow("a", t0 + 60s) && !limiter.allow("a", t0 + 61s));
    CHECK(limiter.allow("b", t0) && limiter.allow("c", t0));
    CHECK(!limiter.allow("d", t0 + 1s) && limiter.tracked() == 3);   // voll, nichts abgelaufen: verworfen statt zu wachsen
    CHECK(limiter.allow("d", t0 + 61s) && limiter.tracked() == 2);   // b und c abgelaufen und entfernt, a (60 s) bleibt
    CHECK(iso::safe_id("offline-bedingt") == "offline-bedingt" && iso::safe_id("#3") == "#3" && iso::safe_id("") == "?");
    const auto hostile = iso::safe_id("Token=GEHEIM\n%s{{settings.token}}" + std::string(100, 'x'));
    CHECK(hostile.size() == 48 && hostile.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-#?") == std::string::npos);
    CHECK(iso::failure_message("p", "#1", iso::Stage::data) == "Plugin p: Block #1 ausgeblendet (Fehler beim Auswerten der Daten)");
    CHECK(iso::failure_message("p", "temp", iso::Stage::draw) == "Plugin p: Block temp ausgeblendet (Fehler beim Zeichnen)");
    CHECK(iso::limiter_key("p", "b", iso::Stage::data) != iso::limiter_key("p", "b", iso::Stage::draw));
}

// Fuzz-nahe Bedingungen (eigener fester Seed; der Fuzz oben bleibt unverändert).
static Json random_condition(std::mt19937& rng) {
    static const Json paths[] = {"feed", "feed.temp", "feed.rows.0.t", "settings.token", "settings.city", "settings.limit", "time",
                                 "item.t", "settings", "x.y", "", "feed..t", 7, nullptr, "feed.a.b.c.d.e.f.g.h"};
    static const Json ops[] = {"==", "!=", "<", "<=", ">", ">=", "=", "", "in", 3, nullptr};
    static const Json values[] = {0, -1.5, 1e300, "abc", "", true, false, nullptr, Json::array({1}), Json::object(), "a\nb"};
    Json c = Json::object();
    if (rng() % 8) c["path"] = paths[rng() % std::size(paths)];
    if (rng() % 2) c["op"] = ops[rng() % std::size(ops)];
    if (rng() % 2) c["value"] = values[rng() % std::size(values)];
    if (rng() % 10 == 0) c["extra"] = 1;
    return rng() % 12 ? c : Json("feed.temp > 3");
}
static Json random_data(std::mt19937& rng, int depth = 0) {
    switch (rng() % (depth < 2 ? 9 : 6)) {
        case 0: return nullptr;
        case 1: return static_cast<int>(rng() % 200) - 100;
        case 2: return std::numeric_limits<double>::infinity();
        case 3: return rng() % 2 ? "" : "Berlin";
        case 4: return rng() % 2 == 1;
        case 5: return 50.5;
        case 6: return Json::array({random_data(rng, depth + 1)});
        case 7: return Json::object();
        default: return Json{{"t", random_data(rng, depth + 1)}};
    }
}
static void test_fuzz_conditions() {
    std::mt19937 rng(20261107u);
    int accepted = 0, exceptions = 0, broken = 0, conditional = 0;
    for (int round = 0; round < 4000; ++round) {
        Json j = v2_cond();
        const int edits = 1 + static_cast<int>(rng() % 3);
        for (int e = 0; e < edits; ++e) {
            // frühere Änderungen können widgets (oder das ganze Manifest) ersetzt haben: dann nur noch freie Änderungen
            const bool widgets = j.is_object() && j.contains("widgets") && j["widgets"].is_array() && !j["widgets"].empty();
            Json* w = widgets ? &j["widgets"][rng() % j["widgets"].size()] : nullptr;
            const auto choice = rng() % 8;
            if (w && w->is_object() && choice < 6) (*w)["show_if"] = random_condition(rng);
            else if (w && w->is_object() && choice == 6) w->erase("show_if");
            else *random_node(j, rng) = random_value(rng);
        }
        if (rng() % 5 == 0 && j.is_object()) j["schema_version"] = rng() % 2 ? 2 : 1;
        try {
            auto parsed = parse_manifest(j.dump());
            if (!parsed) continue;
            ++accepted;
            const auto& m = *parsed.manifest;
            bool ok = true;
            for (const auto& w : m.widgets) {
                if (!w.show_if) continue;
                ++conditional;
                ok = ok && m.schema_version == 2;
                const auto& c = *w.show_if;
                const auto head = c.path.substr(0, c.path.find('.'));
                if (head == "settings")
                    for (const auto& s : m.settings) if ("settings." + s.key == c.path) ok = ok && s.type != "secret";
                ok = ok && head != "time" && head != "date" && head != "item";
                if (!c.op.empty()) ok = ok && !c.value.is_null() && !c.value.is_structured() &&
                                        (c.value.is_number() || c.op == "==" || c.op == "!=");
                else ok = ok && c.value.is_null();
            }
            for (int r = 0; r < 4; ++r) {  // Auswertung mit zufälligen Antworten und Einstellungen: nie eine Ausnahme
                RenderContext ctx;
                if (rng() % 4) ctx.sources["feed"] = {{"temp", random_data(rng)}, {"load", random_data(rng)}, {"rows", random_data(rng)}};
                if (rng() % 2) ctx.settings = {{"city", random_data(rng)}, {"limit", random_data(rng)}, {"token", "PRIVATE"}};
                const auto page = render_page(m, ctx);
                ok = ok && page.failed.empty() && page.widgets.size() + page.hidden.size() == m.widgets.size();
            }
            auto again = parse_manifest(m.definition.dump());
            ok = ok && again && again.manifest->definition == m.definition;
            if (!ok) ++broken;
        } catch (...) { ++exceptions; }
    }
    CHECK(exceptions == 0);
    CHECK(broken == 0);
    CHECK(accepted > 0 && conditional > 0);
    std::printf("fuzz show_if: 4000 manifests, %d accepted, %d conditional blocks\n", accepted, conditional);
}

int main() {
    test_v1_unchanged(); test_v2_accepts(); test_v2_rejects(); test_store_rules();
    test_density(); test_geometry(); test_fuzz();
    test_conditions_parse(); test_condition_table(); test_condition_render(); test_data_isolation(); test_rate_limiter();
    test_fuzz_conditions();
    std::printf("plugin zones: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
