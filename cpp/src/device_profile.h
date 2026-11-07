// © 2026 FuchsLabs
// Gerätefläche und Touch-Umrechnung, ohne Plattformaufrufe.
#pragma once

#include <algorithm>

#include "output.h"

namespace fl::touch { struct Tile; }

namespace fl::device {
struct Insets {
    int top = 0, right = 0, bottom = 0, left = 0;
    bool operator==(const Insets&) const = default;
};
struct Profile {
    Orientation orientation = Orientation::Landscape;
    Insets keepout_physical;
    bool physical_rotates = false;
};
inline Insets select_keepout(Insets common, std::optional<Insets> own, bool sync) {
    return !sync && own ? *own : common;
}
// TV ist eine lokale PC-Profilangabe: Protokoll v1 enthält keinen Gerätetyp.
inline std::pair<int,int> layout_size(std::pair<int,int> size,bool tv) {
    if(tv && size.first<size.second) std::swap(size.first,size.second);
    return size;
}
struct Area {
    int x = 0, y = 0, w = 0, h = 0;
    bool operator==(const Area&) const = default;
};
Insets to_frame(const Insets& physical, Orientation orientation);
Area usable(std::pair<int, int> frame, const Profile& profile);
std::optional<std::pair<int, int>> to_content(const Area& area, int x, int y);
Image place(const Image& content, std::pair<int, int> frame, const Area& area, Color bg);
Image compose_view(const Image& content, std::pair<int, int> frame, const Area& area, Color bg);
void shift_tiles(std::vector<touch::Tile>& tiles, const Area& area);
// Ohne Ziel wird step genutzt. Ein Randtipp wird vollständig verbraucht.
struct Tap { bool blocked; int x = 0, y = 0, step = 0; };
Tap resolve_tap(const Area& area, int x, int y);
// Header-only: der plattformneutrale Companion-Store benötigt keine Grafikobjekte.
inline Insets parse_insets(const ojson& value) {
    auto edge = [&](const char* key) {
        if (!value.is_object() || !value.contains(key) || !value[key].is_number_integer()) return 0;
        if (value[key].is_number_unsigned()) return int(std::min<uint64_t>(value[key].get<uint64_t>(), 1000));
        return int(std::clamp<int64_t>(value[key].get<int64_t>(), 0, 1000));
    };
    return {edge("top"), edge("right"), edge("bottom"), edge("left")};
}

inline ojson insets_json(const Insets& insets) {
    return {{"top", insets.top}, {"right", insets.right}, {"bottom", insets.bottom}, {"left", insets.left}};
}
}  // namespace fl::device
