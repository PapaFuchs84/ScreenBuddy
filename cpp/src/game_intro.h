// © 2026 FuchsLabs
#pragma once
#include "plugin_render.h"
#include "render_hardware_hud.h"
namespace fl::game {
struct IntroPhase { float reveal=0, scale=1, shine=-1, fade=0; bool done=false; };
IntroPhase intro_phase(double elapsed, bool reduced);
// Colours of the intro, taken from the Hardware-HUD palette (design, neon or custom).
struct IntroColors { Color bg{5,8,18}, primary{58,228,245}, secondary{222,78,240}, text{225,246,255}; };
inline IntroColors intro_colors(const render::HudPalette& p) { return {render::transition_shade(p.bg,p.text),p.primary,p.secondary,{240,244,250}}; }
class Intro {
public:
    void begin(Image previous,double now,bool reduced);
    bool active(double now) const;
    Image draw(const Image& target,const ImagePtr& logo,const std::string& name,double now,const IntroColors& colors={});
    void reset();
private:
    plugins::FadeCache cache_;
    double since_=-1;
    bool reduced_=false;
    // Scaled logo cache: the zoom is quantised so the expensive resize runs every few frames only.
    const Image* logo_src_=nullptr;
    Image logo_scaled_;
};
}
