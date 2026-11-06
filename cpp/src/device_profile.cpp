// © 2026 FuchsLabs
#include "device_profile.h"

#include <algorithm>
#include <array>

#include "touch.h"

namespace fl::device {
Insets to_frame(const Insets& physical, Orientation orientation) {
    // Reihenfolge: oben, rechts, unten, links; Portrait ist die Grundlage.
    static constexpr int table[4][4] = {{0,1,2,3}, {2,3,0,1}, {1,2,3,0}, {3,0,1,2}};
    const std::array<int, 4> values{physical.top, physical.right, physical.bottom, physical.left};
    auto index = static_cast<unsigned>(orientation);
    if (index >= 4) index = 0;
    return {values[table[index][0]], values[table[index][1]], values[table[index][2]], values[table[index][3]]};
}

Area usable(std::pair<int, int> frame, const Profile& profile) {
    auto insets = profile.physical_rotates ? to_frame(profile.keepout_physical, profile.orientation) : profile.keepout_physical;
    int limit = std::max(0, std::min(frame.first, frame.second) / 3);
    insets.top = std::clamp(insets.top, 0, limit);
    insets.right = std::clamp(insets.right, 0, limit);
    insets.bottom = std::clamp(insets.bottom, 0, limit);
    insets.left = std::clamp(insets.left, 0, limit);
    Area area{insets.left, insets.top, frame.first - insets.left - insets.right, frame.second - insets.top - insets.bottom};
    if (area.w < 160 || area.h < 120) return {0, 0, frame.first, frame.second};
    return area;
}

std::optional<std::pair<int, int>> to_content(const Area& area, int x, int y) {
    if (x < area.x || y < area.y || x - area.x >= area.w || y - area.y >= area.h) return std::nullopt;
    return std::pair{x - area.x, y - area.y};
}

Image place(const Image& content, std::pair<int, int> frame, const Area& area, Color bg) {
    if (area == Area{0, 0, frame.first, frame.second} && content.w == frame.first && content.h == frame.second) return content;
    Image out(frame.first, frame.second, bg);
    const int x0 = std::max(0, -area.x), y0 = std::max(0, -area.y);
    const int x1 = std::min({content.w, area.w, out.w-area.x});
    const int y1 = std::min({content.h, area.h, out.h-area.y});
    if (x1 > x0) for (int y = y0; y < y1; ++y)
        std::copy_n(content.px.begin()+size_t(y)*content.w+x0, x1-x0,
                    out.px.begin()+size_t(area.y+y)*out.w+area.x+x0);
    return out;
}

Image compose_view(const Image& content, std::pair<int, int> frame, const Area& area, Color bg) {
    return place(content, frame, area, bg);
}

void shift_tiles(std::vector<touch::Tile>& tiles, const Area& area) {
    for (auto& tile : tiles) {
        tile.box.x += area.x;
        tile.box.y += area.y;
    }
}

Tap resolve_tap(const Area& area, int x, int y) {
    auto position = to_content(area, x, y);
    if (!position) return {true};
    return {false, position->first, position->second, position->first < area.w / 3 ? -1 : 1};
}

}  // namespace fl::device
