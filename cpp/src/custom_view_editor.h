// © 2026 FuchsLabs – internal editor adapter, no plugin schema changes.
#pragma once
#include "custom_view.h"
#include "layout_edit.h"
namespace fl::custom {
View game_mode_template(std::string id, bool english=false);
plugins::Arrangement editor_model(const Layout&,const std::vector<Tile>& palette);
std::optional<Layout> editor_layout(const plugins::Arrangement&);
void prune_editor_zones(plugins::Arrangement&);
plugins::Json active_displays(const plugins::Json&);
}
