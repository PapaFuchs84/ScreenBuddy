// © 2026 FuchsLabs – native rendering of validated ScreenBuddy plugin data.
#pragma once

#include "gfx.h"
#include "touch.h"
#include "plugins.h"
#include "plugin_schedule.h"

namespace fl::plugins {
using PluginTarget = touch::PluginTarget;
struct DrawOut { Image image; std::vector<PluginTarget> targets; };
std::vector<zones::Rect> list_rows(zones::Rect area, zones::Density density, float k, bool has_label, size_t count);
DrawOut draw_page_out(const Page&, int width, int height, const schedule::Result* selection = nullptr, bool sample = false);
struct FadeCache { Image from, to; std::vector<PluginTarget> targets, from_targets; int width = 0, height = 0; size_t revision = 0; std::string signature; };
void blend_rect(Image& dst, const Image& from, const Image& to, zones::Rect rect, float mix);
void reveal_rect(Image& dst, const Image& from, const Image& to, zones::Rect rect, float mix);
DrawOut draw_choices_out(const Page&, int, int, const std::vector<schedule::ZoneChoice>&, std::map<std::string,std::shared_ptr<FadeCache>>&);
Image draw_choices(const Page& page, int width, int height, const std::vector<schedule::ZoneChoice>& choices,
                   std::map<std::string,std::shared_ptr<FadeCache>>& caches);
// Caller holds render::mutex() and has applied the requested theme.
Image draw_page(const Page& page, int width, int height, bool sample = false);
}
