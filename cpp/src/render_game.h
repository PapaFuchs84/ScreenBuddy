// © 2026 FuchsLabs
#pragma once
#include "custom_view.h"
#include "models.h"
#include "render.h"
namespace fl::game {
Color level_color(const render::Theme&,double pct); // accent <80, warn >=80, hot >=90
Color mix_color(Color a,Color b,float t);
std::string upper_ascii(std::string);
bool metric_tile(const std::string& block);
plugins::Json metric_values(const SysSnapshot&,const std::string& name={},std::optional<double> elapsed={});
Image draw_metric(const custom::Tile&,const custom::Layout&,int w,int h,const SysSnapshot&,const std::string& name={},std::optional<double> elapsed={},const ImagePtr& logo={});
}
