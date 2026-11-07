// © 2026 FuchsLabs
#pragma once
#include "plugin_render.h"
namespace fl::game {
struct IntroPhase { float reveal=0, scale=1, shine=-1, fade=0; bool done=false; };
IntroPhase intro_phase(double elapsed, bool reduced);
class Intro {
public:
    void begin(Image previous,double now,bool reduced);
    bool active(double now) const;
    Image draw(const Image& target,const ImagePtr& logo,const std::string& name,double now);
    void reset();
private:
    plugins::FadeCache cache_;
    double since_=-1;
    bool reduced_=false;
};
}
