// © 2026 FuchsLabs – native rendering of validated ScreenBuddy plugin data.
#pragma once

#include "gfx.h"
#include "plugins.h"
#include "plugin_schedule.h"

namespace fl::plugins {
struct FadeCache { Image from, to; int width = 0, height = 0; size_t revision = 0; std::string signature; };
void blend_rect(Image& dst, const Image& from, const Image& to, zones::Rect rect, float mix);
void reveal_rect(Image& dst, const Image& from, const Image& to, zones::Rect rect, float mix);
Image draw_choices(const Page& page, int width, int height, const std::vector<schedule::ZoneChoice>& choices,
                   std::map<std::string,std::shared_ptr<FadeCache>>& caches);
// Caller holds render::mutex() and has applied the requested theme.
Image draw_page(const Page& page, int width, int height, bool sample = false);
}
