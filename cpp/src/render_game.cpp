// © 2026 FuchsLabs – additive single tiles; standard system page unchanged.
#include "render_game.h"
#include "render.h"
#include "i18n.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace fl::game {
bool metric_tile(const std::string& block){
    static const std::vector<std::string> ids={"system.fps","system.frametime","system.fps_low","system.gpu_temp","system.gpu_clock","system.gpu_fan","system.gpu_power","system.vram","system.cpu_clock","system.cpu_cores","system.cpu_temp","system.ping","system.jitter","system.game_name","system.game_time","system.game_logo"};
    return std::find(ids.begin(),ids.end(),block)!=ids.end();
}
plugins::Json metric_values(const SysSnapshot& s,const std::string& name,std::optional<double> elapsed){
    plugins::Json data={{"game_name",name},{"cpu_cores",s.cpu_cores},{"fps_reason",s.fps_reason}};
    auto add=[&](const char* key,const std::optional<double>& value){data[key]=value?plugins::Json(*value):plugins::Json(nullptr);};
    add("fps",s.fps);add("frametime",s.frametime_ms);add("fps_low",s.fps_low);add("gpu_temp",s.gpu_temp);add("gpu_clock",s.gpu_mhz);add("gpu_fan",s.gpu_fan);add("gpu_power",s.gpu_watts);
    add("cpu_clock",s.cpu_ghz);add("cpu_temp",s.cpu_temp);add("ping",s.ping_ms);add("jitter",s.jitter_ms);add("game_time",elapsed);add("vram_used",s.gpu_mem_used);add("vram_total",s.gpu_mem_total);return data;
}
Image draw_metric(const custom::Tile& tile,const custom::Layout& layout,int w,int h,const SysSnapshot& snapshot,const std::string& name,std::optional<double> elapsed,const ImagePtr& logo){
    float scale=render::scale_for(w,h);const auto& theme=render::theme();Canvas c(w,h,scale,theme.bg);
    auto plan=plugins::zones::plan({8,36,std::round(w/scale)-16,std::round(h/scale)-54},custom::tile_layout(tile,layout),{},6);
    if(plan.zones.empty())return c.finish();auto a=plan.zones.front().area;
    c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,theme.card);c.clip_push(a.x+4,a.y+4,a.x+a.width-5,a.y+a.height-5);
    std::string title=tile.block;for(const auto& b:builtin::find("system")->blocks)if(b.id==tile.block)title=i18n::lang()=="en"?b.label.en:b.label.de;
    Font label{float(std::clamp(a.height/7,10.f,15.f)),false},value{float(std::clamp(a.height/4,12.f,27.f)),true};
    c.text(a.x+8,a.y+8,title,label,theme.muted);
    auto data=metric_values(snapshot,name,elapsed);auto key=tile.block.substr(7);std::string text="n/a",unit;
    if(key=="game_name" || key=="game_logo")text=name.empty()?"n/a":name;
    else if(key=="game_time" && elapsed){int t=int(std::max(0.,*elapsed));char b[64];snprintf(b,sizeof b,"%02d:%02d:%02d",t/3600,(t/60)%60,t%60);text=b;}
    else if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total){char b[96];snprintf(b,sizeof b,"%.0f / %.0f MiB",*snapshot.gpu_mem_used,*snapshot.gpu_mem_total);text=b;}
    else if(key=="cpu_cores" && !snapshot.cpu_cores.empty()) {double mean=0;for(auto x:snapshot.cpu_cores)mean+=x;char b[80];snprintf(b,sizeof b,"%.0f %% · %zu",mean/snapshot.cpu_cores.size(),snapshot.cpu_cores.size());text=b;}
    else if(data.contains(key) && data[key].is_number()) {
        if(key=="frametime" || key=="ping" || key=="jitter")unit=" ms";
        if(key=="gpu_temp" || key=="cpu_temp")unit=" °C";
        if(key=="gpu_clock")unit=" MHz";if(key=="cpu_clock")unit=" GHz";if(key=="gpu_fan")unit=" %";if(key=="gpu_power")unit=" W";
        char b[64];snprintf(b,sizeof b,key=="cpu_clock"?"%.2f":"%.1f",data[key].get<double>());text=b+unit;
    }
    if(key=="game_logo" && logo && !logo->empty()) {float fit=std::min((a.width-16)*scale/logo->w,(a.height-36)*scale/logo->h);if(fit>0){auto image=resize(*logo,std::max(1,int(logo->w*fit)),std::max(1,int(logo->h*fit)));c.image(image,a.x+(a.width-image.w/scale)/2,a.y+28);}}
    else {while(text.size()>4 && c.textlength(text,value)>a.width-16){auto points=to_u32(text);if(points.size()<3)break;points.resize(points.size()-2);text=from_u32(points)+"…";}c.text(a.x+8,a.y+std::min(a.height*.48f,48.f),text,value,theme.text,"lm");}
    if(tile.density!=0 && a.height>=90){
        std::vector<double> values;if(key=="cpu_cores")values=snapshot.cpu_cores;else if(key=="frametime")values.assign(snapshot.frametime_hist.begin(),snapshot.frametime_hist.end());
        if(!values.empty()) {double max=key=="cpu_cores"?100.:std::max(1.,*std::max_element(values.begin(),values.end()));float dx=(a.width-16)/values.size();for(size_t i=0;i<values.size();++i){float height=float(std::clamp(values[i]/max,0.,1.))*(a.height*.28f);c.rectangle(a.x+8+i*dx,a.y+a.height-8-height,a.x+8+(i+1)*dx-1,a.y+a.height-8,theme.accent);}}
    }
    c.clip_pop();return c.finish();
}
}
