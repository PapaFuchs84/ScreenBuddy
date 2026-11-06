// Shipped documentation fixtures: offline only; no HTTP, settings store or hardware.
#include "plugins.h"
#include "plugin_zones.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

using namespace fl::plugins;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

int main(int argc, char** argv) {
    const auto directory = std::filesystem::path(argc > 1 ? argv[1] : FL_PLUGIN_EXAMPLE_DIR);
    std::set<std::string> ids;
    bool focus_seen = false, weather_seen = false, zones_seen = false;
    try {
        for (const auto& file : std::filesystem::recursive_directory_iterator(directory)) {
            if (!file.is_regular_file() || !file.path().filename().string().ends_with(".flplugin.json")) continue;
            std::ifstream input(file.path(), std::ios::binary);
            const std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            auto parsed = parse_manifest(data);
            CHECK(parsed);
            if (!parsed) {
                std::printf("Invalid example: %s (%s)\n", file.path().filename().string().c_str(), parsed.error.c_str());
                continue;
            }
            const auto& manifest = *parsed.manifest;
            CHECK(ids.insert(manifest.id).second);
            RenderContext sample;
            sample.sources = sample_data(manifest);
            sample.time = "12:34";
            sample.date = "2026-01-01";
            auto page = render_page(manifest, sample);
            CHECK(!page.missing_data);
            CHECK(page.widgets.size() == manifest.widgets.size());
            for (const auto& source : manifest.sources) {
                CHECK(parse_source_data(source.sample.dump()));
                CHECK(prepare_request(manifest, source, Json::object()));
            }
            if (manifest.id == "offline-focus") {
                focus_seen = true;
                CHECK(manifest.sources.empty());
                CHECK(page.widgets.size() == 4);
                if (page.widgets.size() == 4) {
                    CHECK(page.widgets[0].text == "12:34");
                    CHECK(page.widgets[1].text == "25");
                    CHECK(page.widgets[3].bar_fraction == 0.4);
                    sample.settings = {{"focus", "Ship a small improvement"}, {"minutes", 45}, {"progress", 75}};
                    page = render_page(manifest, sample);
                    CHECK(!page.missing_data);
                    CHECK(page.widgets[1].text == "45");
                    CHECK(page.widgets[2].text == "Ship a small improvement");
                    CHECK(page.widgets[3].bar_fraction == 0.75);
                }
            } else if (manifest.id == "offline-zonen") {
                // Manifest v2 mit Zonen-Layout: 7 bis 12 Blöcke, offline, store-tauglich, alle Blöcke passen in beiden Ausrichtungen.
                zones_seen = true;
                CHECK(manifest.schema_version == 2);
                CHECK(manifest.sources.empty());
                CHECK(manifest.layout.has_value() && fl::plugins::zones::valid(*manifest.layout));
                CHECK(manifest.widgets.size() >= 7 && manifest.widgets.size() <= 12);
                CHECK(store_problems(manifest).empty());
                CHECK(page.layout.has_value());
                CHECK(page.widgets.size() == manifest.widgets.size());
                std::set<std::string> block_ids;
                for (const auto& widget : page.widgets) CHECK(!widget.id.empty() && !widget.zone.empty() && block_ids.insert(widget.id).second);
                if (manifest.layout) {
                    const fl::plugins::zones::DensityTable table;
                    std::vector<fl::plugins::zones::BlockInput> inputs;
                    for (const auto& widget : manifest.widgets) inputs.push_back(fl::plugins::zones::describe(widget.id, widget.type, table, widget.zone));
                    // logische Nutzflächen der Plugin-Seite (Seitenrand 10, Kopf 39, Fuß 19): 480x320 und 320x480, Abstand 7
                    for (const auto& area : {fl::plugins::zones::Rect{10, 39, 460, 252}, fl::plugins::zones::Rect{10, 39, 300, 412}}) {
                        const auto plan = fl::plugins::zones::plan(area, *manifest.layout, inputs, 7);
                        CHECK(plan.blocks.size() == manifest.widgets.size());
                        for (const auto& block : plan.blocks) CHECK(block.visible());
                    }
                }
                CHECK(page.widgets.size() == 8);
                if (page.widgets.size() == 8) {
                    CHECK(page.widgets[0].text == "12:34" && page.widgets[1].text == "2026-01-01");
                    CHECK(page.widgets[4].text == "25" && page.widgets[5].bar_fraction == 0.6);
                    sample.settings = {{"focus", "Ship"}, {"minutes", 45}, {"progress", 75}, {"rounds", 4}, {"breaks", 1}, {"motto", "Go"}};
                    page = render_page(manifest, sample);
                    CHECK(!page.missing_data);
                    CHECK(page.widgets[2].text == "Go" && page.widgets[3].text == "Ship" && page.widgets[4].text == "45");
                    CHECK(page.widgets[5].bar_fraction == 0.75 && page.widgets[6].text == "4" && page.widgets[7].text == "1");
                }
            } else if (manifest.id == "public-weather") {
                weather_seen = true;
                CHECK(manifest.sources.size() == 1);
                if (manifest.sources.size() == 1) {
                    const auto& source = manifest.sources[0];
                    CHECK(source.host == "api.open-meteo.com");
                    CHECK(source.setting_keys == std::vector<std::string>({"latitude", "longitude"}));
                    auto request = prepare_request(manifest, source, {{"latitude", 50.5}, {"longitude", 8.25}, {"place", "Local label"}});
                    CHECK(request);
                    if (request) {
                        CHECK(request.request->url.find("latitude=50.5&longitude=8.25") != std::string::npos);
                        CHECK(request.request->url.find("Local") == std::string::npos);
                        CHECK(request.request->headers.at("Accept") == "application/json");
                    }
                    CHECK(manifest.refresh_seconds == 900);
                    CHECK(page.widgets.size() == 5);
                    if (page.widgets.size() == 5) {
                        CHECK(page.widgets[1].text == "18.5");
                        CHECK(page.widgets[2].bar_fraction == 0.62);
                    }
                    // Live mode has no samples: missing response must stay visibly missing.
                    sample.sources.clear();
                    page = render_page(manifest, sample);
                    CHECK(page.missing_data);
                    if (page.widgets.size() == 5) {
                        CHECK(page.widgets[1].text != "18.5");
                        CHECK(!page.widgets[2].bar_fraction);
                    }
                }
            }
            std::printf("Parsed and rendered: %s\n", file.path().filename().string().c_str());
        }
    } catch (const std::exception&) {
        ++failed;
        std::puts("Example fixture read/validation failed.");
    }
    // The default shipped set must retain the documented tutorials' fixtures.
    if (argc == 1) { CHECK(focus_seen); CHECK(weather_seen); CHECK(zones_seen); CHECK(ids.contains("offline-clock")); }
    CHECK(!ids.empty());
    std::printf("Plugin examples: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
