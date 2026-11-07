// © 2026 FuchsLabs – uses the existing premultiplied-alpha fade path.
#include "game_intro.h"
#include "util.h"
#include <algorithm>
#include <cmath>
namespace fl::game {
namespace { float smooth(double v){float x=float(std::clamp(v,0.,1.));return x*x*(3-2*x);} }
IntroPhase intro_phase(double elapsed,bool reduced){
    if(!std::isfinite(elapsed))return {1,1,-1,1,true};
    double t=std::max(0.,elapsed);
    if(reduced)return {1,1,-1,smooth(t/.35),t>=.35};
    return {smooth(t/.6),.88f+.12f*smooth(t/.8),float((t-.65)/1.1),smooth((t-2.05)/.45),t>=2.5};
}
void Intro::begin(Image previous,double now,bool reduced){cache_={};cache_.from=std::move(previous);since_=now;reduced_=reduced;}
bool Intro::active(double now)const{return since_>=0 && !intro_phase(now-since_,reduced_).done;}
void Intro::reset(){cache_={};since_=-1;}
Image Intro::draw(const Image& target,const ImagePtr& logo,const std::string& name,double now){
    auto phase=intro_phase(now-since_,reduced_);if(phase.done)return target;
    if(reduced_) {auto result=target;if(cache_.from.w==target.w && cache_.from.h==target.h)plugins::blend_rect(result,cache_.from,target,{0,0,float(target.w),float(target.h)},phase.fade);return result;}
    Canvas canvas(target.w,target.h,1,{0,0,0});
    if(!cache_.from.empty()) {auto previous=cache_.from.w==target.w && cache_.from.h==target.h?cache_.from:resize(cache_.from,target.w,target.h);canvas.image(previous,0,0,1-.85f*phase.reveal);}
    if(logo && !logo->empty()) {
        float fit=std::min(target.w*.72f/logo->w,target.h*.48f/logo->h)*phase.scale;
        auto scaled=resize(*logo,std::max(1,int(logo->w*fit)),std::max(1,int(logo->h*fit)));
        if(phase.shine>=0 && phase.shine<=1)for(int y=0;y<scaled.h;++y)for(int x=0;x<scaled.w;++x){auto& px=scaled.px[size_t(y)*scaled.w+x];float stripe=std::max(0.f,1-std::abs(float(x)/scaled.w-phase.shine)*10)*.3f;unsigned a=px>>24;unsigned r=(px>>16)&255,g=(px>>8)&255,b=px&255;r+=unsigned((a-r)*stripe);g+=unsigned((a-g)*stripe);b+=unsigned((a-b)*stripe);px=(a<<24)|(r<<16)|(g<<8)|b;}
        canvas.image(scaled,float(target.w-scaled.w)/2,float(target.h-scaled.h)/2-18,phase.reveal);
    }
    Font f{float(std::clamp(target.h/15,12,32)),true};auto title=name.empty()?"n/a":name;
    while(title.size()>4 && canvas.textlength(title,f)>target.w*.85f) {auto points=to_u32(title);points.resize(points.size()-2);title=from_u32(points)+"…";}
    canvas.text(target.w*.5f,target.h*(logo?.8f:.5f),title,f,{240,240,240},"mm");
    cache_.to=canvas.finish();auto result=cache_.to;
    if(phase.fade>0)plugins::blend_rect(result,cache_.to,target,{0,0,float(target.w),float(target.h)},phase.fade);
    return result;
}
}
