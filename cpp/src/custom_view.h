// © 2026 FuchsLabs – eigene Ansichten, ohne fremde Implementierung.
#pragma once
#include "builtin_catalog.h"
#include "device_overlays.h"
#include "gfx.h"
#include <filesystem>
#include <functional>
#include <mutex>
namespace fl::custom {
using plugins::Json;
inline constexpr size_t file_limit = 256 * 1024, tile_limit = 24, view_limit = 64;
struct Tile {
    std::string source, block;
    plugins::zones::Zone zone;
    std::optional<plugins::Condition> show_if;
    int density = 2; // maximum: 0 compact, 1 regular, 2 automatic
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
Image draw(const View& view, const std::string& device, int width, int height, const DrawTile& source);
} // namespace fl::custom
