// © 2026 FuchsLabs – reiner Katalog eingebauter Seiten, ohne Dateien oder Renderer.
#pragma once
#include "plugins.h"
namespace fl::builtin {
namespace zones = plugins::zones;
using plugins::Json;
struct Label { std::string de, en; };
struct BlockSpec {
    std::string id;
    Label label;
    zones::Thresholds thresholds;
    int maximum = 1;
    bool required = false;
    std::string action;
};
struct Catalog {
    std::string key;
    Label label;
    int version = 1;
    bool enabled = false;
    std::vector<BlockSpec> blocks;
    plugins::Arrangement standard, portrait;
};
struct Overlay {
    int catalog_version = 1;
    plugins::Arrangement model;
    std::optional<plugins::Arrangement> portrait;
    // Internal single-tile contract; never accepted from serialized page overlays.
    bool single_tile = false;
    int tile_density = 2;
};
struct Result {
    std::optional<Overlay> overlay;
    std::string error;
    explicit operator bool() const { return overlay.has_value(); }
};
const std::vector<Catalog>& catalogs();
const Catalog* find(std::string_view key);
Overlay standard(const Catalog& c);
Json to_json(const Overlay& overlay);
Result parse(const Catalog& c, std::string_view text);
std::string validate(const Catalog& c, const Overlay& overlay);
// Semantische Gleichheit: explizite Standard-Spannweiten zählen ebenfalls als Standard.
bool is_standard(const Catalog& c, const Overlay& overlay);
bool is_standard(const Catalog& c, const plugins::Arrangement& model, bool portrait);
zones::Plan tile_plan(zones::Plan plan, const Overlay& overlay);
std::vector<zones::BlockInput> inputs(const Catalog& c, const plugins::Arrangement& model);
} // namespace fl::builtin
