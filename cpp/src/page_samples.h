// Gemeinsame Beispieldaten für Tests, Demo und den späteren Editor.
#pragma once
#include "render.h"
namespace fl::render::samples {
inline constexpr std::time_t CLOCK = 1792606500;
spotify::Snapshot spotify();
std::vector<Friend> friends();
std::vector<Bf6Player> players();
std::vector<StreamItem> stream();
StreamLogos stream_logos();
std::vector<HaEntity> ha();
SysSnapshot system();
ai::Snapshot ai_status();
std::vector<Post> posts(const std::string& lang = "de");
}  // namespace fl::render::samples
