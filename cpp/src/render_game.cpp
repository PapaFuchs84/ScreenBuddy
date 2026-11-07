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
    static const std::vector<std::string> ids={"system.cpu","system.gpu","system.fps","system.frametime","system.fps_low","system.gpu_temp","system.gpu_clock","system.gpu_fan","system.gpu_power","system.vram","system.cpu_clock","system.cpu_cores","system.cpu_temp","system.ping","system.jitter","system.game_name","system.game_time","system.game_logo"};
    return std::find(ids.begin(),ids.end(),block)!=ids.end();
}
plugins::Json metric_values(const SysSnapshot& s,const std::string& name,std::optional<double> elapsed){
    plugins::Json data={{"game_name",name},{"cpu_cores",s.cpu_cores},{"fps_reason",s.fps_reason}};
    auto add=[&](const char* key,const std::optional<double>& value){data[key]=value?plugins::Json(*value):plugins::Json(nullptr);};
    add("cpu_pct",s.cpu_pct);add("gpu_pct",s.gpu_pct);add("cpu",s.cpu_pct);add("gpu",s.gpu_pct);add("fps",s.fps);add("frametime",s.frametime_ms);add("fps_low",s.fps_low);add("gpu_temp",s.gpu_temp);add("gpu_clock",s.gpu_mhz);add("gpu_fan",s.gpu_fan);add("gpu_power",s.gpu_watts);
    add("cpu_clock",s.cpu_ghz);add("cpu_temp",s.cpu_temp);add("ping",s.ping_ms);add("jitter",s.jitter_ms);add("game_time",elapsed);add("vram_used",s.gpu_mem_used);add("vram_total",s.gpu_mem_total);return data;
}
Image draw_metric(const custom::Tile& tile,const custom::Layout& layout,int w,int h,const SysSnapshot& snapshot,const std::string& name,std::optional<double> elapsed,const ImagePtr& logo){
    float scale=render::scale_for(w,h);const auto& theme=render::theme();Canvas c(w,h,scale,theme.bg);
    auto plan=plugins::zones::plan({8,36,std::round(w/scale)-16,std::round(h/scale)-54},custom::tile_layout(tile,layout),{},6);
    if(plan.zones.empty())return c.finish();auto a=plan.zones.front().area;
    c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,theme.card);c.clip_push(a.x+4,a.y+4,a.x+a.width-5,a.y+a.height-5);
    std::string title=tile.block;for(const auto& b:builtin::find("system")->blocks)if(b.id==tile.block)title=i18n::lang()=="en"?b.label.en:b.label.de;
    Font label{float(std::clamp(a.height/5,14.f,22.f)),true},value{float(std::clamp(a.height/(tile.density==0?2.6f:2.2f),24.f,52.f)),true};
    while(label.size>10 && c.textlength(title,label)>a.width-16)label.size-=.5f;
    c.text(a.x+8,a.y+6,title,label,theme.accent);
    auto data=metric_values(snapshot,name,elapsed);auto key=tile.block.substr(7);std::string text="n/a",unit;
    if(key=="game_name" || key=="game_logo")text=name.empty()?"n/a":name;
    else if(key=="game_time" && elapsed){int t=int(std::max(0.,*elapsed));char b[64];snprintf(b,sizeof b,"%02d:%02d:%02d",t/3600,(t/60)%60,t%60);text=b;}
    else if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total){char b[96];snprintf(b,sizeof b,"%.0f / %.0f MiB",*snapshot.gpu_mem_used,*snapshot.gpu_mem_total);text=b;}
    else if(key=="cpu_cores" && !snapshot.cpu_cores.empty()) {double mean=0;for(auto x:snapshot.cpu_cores)mean+=x;char b[80];snprintf(b,sizeof b,"%.0f %% · %zu",mean/snapshot.cpu_cores.size(),snapshot.cpu_cores.size());text=b;}
    else if(data.contains(key) && data[key].is_number()) {
        if(key=="frametime" || key=="ping" || key=="jitter")unit=" ms";
        if(key=="cpu" || key=="gpu")unit=" %";
        if(key=="gpu_temp" || key=="cpu_temp")unit=" °C";
        if(key=="gpu_clock")unit=" MHz";if(key=="cpu_clock")unit=" GHz";if(key=="gpu_fan")unit=" %";if(key=="gpu_power")unit=" W";
        char b[64];snprintf(b,sizeof b,key=="cpu_clock"?"%.2f":"%.1f",data[key].get<double>());text=b+unit;
    }
    if(key=="game_logo" && logo && !logo->empty()) {float fit=std::min((a.width-16)*scale/logo->w,(a.height-36)*scale/logo->h);if(fit>0){auto image=resize(*logo,std::max(1,int(logo->w*fit)),std::max(1,int(logo->h*fit)));c.image(image,a.x+(a.width-image.w/scale)/2,a.y+28);}}
    else {while(value.size>12 && c.textlength(text,value)>a.width-16)value.size-=.5f;
        while(text.size()>4 && c.textlength(text,value)>a.width-16){auto points=to_u32(text);if(points.size()<3)break;points.resize(points.size()-2);text=from_u32(points)+"…";}c.text(a.x+8,a.y+a.height*.56f,text,value,theme.text,"lm");}
    if(tile.density!=0 && a.height>=44){
        const auto& history=key=="fps"?snapshot.fps_hist:snapshot.frametime_hist;
        if(key=="fps" || key=="frametime") {
            double maximum=1;for(auto v:history)if(std::isfinite(v))maximum=std::max(maximum,v);
            std::vector<PointF> points;
            auto flush=[&]{if(points.size()>1)c.line(points,theme.accent,2);points.clear();};
            for(size_t i=0;i<history.size();++i) {
                if(!std::isfinite(history[i])){flush();continue;}
                points.push_back({a.x+8+float(i)*(a.width-16)/59.f,a.y+a.height-8-float(history[i]/maximum)*a.height*.16f});
            }flush();
        } else {
            auto load=key=="cpu"?snapshot.cpu_pct:key=="gpu"?snapshot.gpu_pct:std::optional<double>{};
            if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total && *snapshot.gpu_mem_total>0)load=100* *snapshot.gpu_mem_used/ *snapshot.gpu_mem_total;
            if(load){c.rounded_rectangle(a.x+8,a.y+a.height-12,a.x+a.width-9,a.y+a.height-8,2,theme.muted);
                c.rounded_rectangle(a.x+8,a.y+a.height-12,a.x+8+(a.width-16)*float(std::clamp(*load/100,0.,1.)),a.y+a.height-8,2,theme.accent);}
            if(key=="cpu_cores" && !snapshot.cpu_cores.empty()){float dx=(a.width-16)/snapshot.cpu_cores.size();for(size_t i=0;i<snapshot.cpu_cores.size();++i)c.rectangle(a.x+8+i*dx,a.y+a.height-8-float(snapshot.cpu_cores[i]/100)*a.height*.16f,a.x+8+(i+1)*dx-1,a.y+a.height-8,theme.accent);}
        }
    }
    c.clip_pop();return c.finish();
}
}
