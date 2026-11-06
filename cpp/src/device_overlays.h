// © 2026 FuchsLabs – begrenzte Geräte-Overlays; IDs werden nie als Dateipfad benutzt.
#pragma once
#include "plugins.h"

namespace fl::device {
inline constexpr size_t overlay_count_limit = 16;
inline constexpr size_t overlay_bytes_limit = 256 * 1024;
inline bool valid_layout_id(std::string_view id) {
    if (id.empty() || id.size() > 512) return false;
    for (unsigned char c : id) if (c < 0x20 || c >= 0x7f) return false;
    return true;
}
using OverlayMap = std::map<std::string, plugins::Json>;
inline plugins::Json overlays_json(const OverlayMap& map) {
    return {{"schema_version",1},{"devices",map}};
}
template<class Validate>
bool parse_overlays(std::string_view text, OverlayMap& out, Validate validate) {
    auto j=plugins::parse_strict_json(text,overlay_bytes_limit-plugins::arrangement_limit);
    if(!j || !j->is_object() || j->size()!=2 || !j->contains("schema_version") || !(*j)["schema_version"].is_number_integer() || j->value("schema_version",plugins::Json())!=1 ||
       !j->contains("devices") || !(*j)["devices"].is_object() || (*j)["devices"].size()>overlay_count_limit) return false;
    OverlayMap next;
    for(auto it=(*j)["devices"].begin();it!=(*j)["devices"].end();++it) {
        const auto bytes=it.value().dump();
        if(!valid_layout_id(it.key()) || bytes.size()>plugins::arrangement_limit || !validate(bytes)) return false;
        next.emplace(it.key(),it.value());
    }
    out=std::move(next); return true;
}
inline bool overlay_fits(const OverlayMap& map) {
    if(map.size()>overlay_count_limit) return false;
    for(const auto& [id,j]:map) if(!valid_layout_id(id) || j.dump().size()>plugins::arrangement_limit) return false;
    return overlays_json(map).dump().size()<=overlay_bytes_limit-plugins::arrangement_limit;
}
} // namespace fl::device
