// © 2026 FuchsLabs – eigene Ansichten, ohne fremde Implementierung.
#pragma once
#include "builtin_catalog.h"
#include "device_overlays.h"
#include "gfx.h"
#include "touch.h"
#include <filesystem>
#include <functional>
#include <mutex>
namespace fl::custom {
using plugins::Json;
inline constexpr size_t file_limit = 256 * 1024, tile_limit = 24, view_limit = 64;
// Invalid optional overrides normalize to Standard; the document remains usable.
struct Tap { std::string action, page, url; };
std::optional<Tap> parse_tap(const Json& value, const std::string& source);
Json tap_json(const Tap& tap);
struct Tile {
    std::string source, block;
    plugins::zones::Zone zone;
    std::optional<plugins::Condition> show_if;
    int density = 2; // maximum: 0 compact, 1 regular, 2 automatic
    std::optional<Tap> tap;
};
struct Layout { std::vector<int> columns{1}, rows{1}; std::vector<Tile> tiles; };
struct View {
    std::string id, name;
    Layout standard;
    std::map<std::string,Layout> devices;
    const Layout& effective(const std::string& device) const;
};
bool valid_id(std::string_view id);
std::optional<View> parse(std::string_view bytes);
Json to_json(const View& view);
// No mutation on failure. Unknown/invalid files disappear from the page cycle.
class Store {
public:
    explicit Store(std::filesystem::path directory): directory_(std::move(directory)) {}
    std::optional<View> get(const std::string& id) const;
    std::string save(const View& view);
    std::vector<View> list() const;
    std::string erase(const std::string& id);
private:
    std::filesystem::path directory_;
    mutable std::mutex mutex_;
};
// Validated single-block contract; complete-page status requirements stay unchanged.
std::optional<builtin::Overlay> single_builtin(const Tile& tile, const Layout& layout);
plugins::zones::Layout tile_layout(const Tile& tile, const Layout& layout);
bool visible(const Tile& tile, const Json& public_data);
// Caller supplies only currently authorized live data and public settings.
std::optional<Image> draw_plugin(const Tile& tile, const Layout& layout, plugins::Manifest manifest,
                                 const plugins::RenderContext& context, int width, int height);
// Draw callbacks use existing source renderers at the final page dimensions.
// Copy only this tile's planned rectangle: source headers/footers never leak.
using DrawTile = std::function<std::optional<Image>(const Tile&,const Layout&,int,int)>;
struct TileDraw { Image image; std::vector<touch::ArrangedTarget> targets; std::vector<touch::PluginTarget> plugin; };
using DrawTileOut = std::function<std::optional<TileDraw>(const Tile&,const Layout&,int,int)>;
struct DrawOut { Image image; std::vector<touch::ArrangedTarget> targets; std::vector<touch::PluginTarget> plugin; };
// Overrides are resolved by the caller against current pages and plugin grants.
using Override = std::function<bool(const Tile&,render::Box,DrawOut&)>;
DrawOut draw_out(const View&,const std::string&,int,int,const DrawTileOut&,const Override& = {});
// Runtime context for individual touch targets (Phase 8 P8-3). Anything not
// granted here resolves to Standard: the source's own targets stay in force.
struct LinkGrant { std::string plugin; uint64_t generation = 0; std::function<bool(const std::string&)> allowed; };
struct TapEnv {
    std::vector<std::string> pages;                                              // pages currently in the cycle
    std::function<std::optional<LinkGrant>(const std::string&)> grant;           // plugin id -> consent + link check
    std::function<void(const std::string&)> go;                                  // request a page switch
};
Override tap_override(std::string view_id, TapEnv env);
Image draw(const View& view, const std::string& device, int width, int height, const DrawTile& source);
} // namespace fl::custom
