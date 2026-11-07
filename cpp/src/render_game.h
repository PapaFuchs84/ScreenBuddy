// © 2026 FuchsLabs
#pragma once
#include "custom_view.h"
#include "models.h"
namespace fl::game {
bool metric_tile(const std::string& block);
plugins::Json metric_values(const SysSnapshot&,const std::string& name={},std::optional<double> elapsed={});
Image draw_metric(const custom::Tile&,const custom::Layout&,int w,int h,const SysSnapshot&,const std::string& name={},std::optional<double> elapsed={},const ImagePtr& logo={});
}
