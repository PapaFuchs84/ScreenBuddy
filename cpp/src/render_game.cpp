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
// Fixed, strongly saturated signal colours: the theme's warn/hot are pastel in some designs and unreadable as a load warning.
Color level_color(const render::Theme& theme,double pct){return pct>=90?Color{255,77,61}:pct>=80?Color{255,179,0}:theme.accent;}
Color mix_color(Color a,Color b,float t){auto m=[&](uint8_t x,uint8_t y){return uint8_t(std::lround(x+(y-x)*std::clamp(t,0.f,1.f)));};return {m(a.r,b.r),m(a.g,b.g),m(a.b,b.b)};}
std::string upper_ascii(std::string s){for(auto& ch:s)if(ch>='a' && ch<='z')ch=char(ch-32);return s;}
namespace {
void ring_arc(Canvas& c,float cx,float cy,float outer,float inner,double fraction,Color color){
    if(fraction<=0)return;const double pi=3.14159265358979323846;int n=std::max(2,int(std::ceil(fraction*72)));
    std::vector<PointF> p;p.reserve(size_t(n+1)*2);
    for(int i=0;i<=n;++i){double a=-pi/2+fraction*2*pi*i/n;p.push_back({cx+float(std::cos(a))*outer,cy+float(std::sin(a))*outer});}
    for(int i=n;i>=0;--i){double a=-pi/2+fraction*2*pi*i/n;p.push_back({cx+float(std::cos(a))*inner,cy+float(std::sin(a))*inner});}
    c.polygon(p,color);
}
}
Image draw_metric(const custom::Tile& tile,const custom::Layout& layout,int w,int h,const SysSnapshot& snapshot,const std::string& name,std::optional<double> elapsed,const ImagePtr& logo){
    float scale=render::scale_for(w,h);const auto& theme=render::theme();Canvas c(w,h,scale,theme.bg);
    auto plan=plugins::zones::plan({8,36,std::round(w/scale)-16,std::round(h/scale)-54},custom::tile_layout(tile,layout),{},6);
    if(plan.zones.empty())return c.finish();auto a=plan.zones.front().area;
    auto key=tile.block.substr(7);
    const bool hero=key=="fps" && a.height>=100;
    if(hero){c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,mix_color(theme.card,theme.accent,.55f));c.rounded_rectangle(a.x+1.5f,a.y+1.5f,a.x+a.width-2.5f,a.y+a.height-2.5f,6.5f,theme.card);}
    else c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,theme.card);
    c.rounded_rectangle(a.x,a.y+a.height*.18f,a.x+2.5f,a.y+a.height*.82f,1.2f,theme.accent);
    c.clip_push(a.x+4,a.y+4,a.x+a.width-5,a.y+a.height-5);
    std::string title=tile.block;for(const auto& b:builtin::find("system")->blocks)if(b.id==tile.block)title=i18n::lang()=="en"?b.label.en:b.label.de;
    if(snapshot.fps_reason=="hwinfo_presentmon"){if(key=="fps")title="FPS · HWiNFO";else if(key=="fps_low")title="FPS · 1 % Low · HWiNFO";else if(key=="frametime")title="Frametime · HWiNFO";}
    title=upper_ascii(title);
    Font label{float(std::clamp(a.height/7,11.f,16.f)),true},value{float(std::clamp(a.height/(hero?2.0f:tile.density==0?2.6f:2.2f),24.f,hero?104.f:52.f)),true};
    while(label.size>9 && c.textlength(title,label)>a.width-16)label.size-=.5f;
    c.text(a.x+10,a.y+6,title,label,theme.muted);
    auto data=metric_values(snapshot,name,elapsed);std::string text="n/a",unit;std::optional<double> ringload;
    if(key=="game_name" || key=="game_logo")text=name.empty()?"n/a":name;
    else if(key=="game_time" && elapsed){int t=int(std::max(0.,*elapsed));char b[64];snprintf(b,sizeof b,"%02d:%02d:%02d",t/3600,(t/60)%60,t%60);text=b;}
    else if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total){char b[96];snprintf(b,sizeof b,"%.0f / %.0f",*snapshot.gpu_mem_used,*snapshot.gpu_mem_total);text=b;unit="MiB";}
    else if(key=="cpu_cores" && !snapshot.cpu_cores.empty()) {double mean=0;for(auto x:snapshot.cpu_cores)mean+=x;char b[80];snprintf(b,sizeof b,"%.0f %% · %zu",mean/snapshot.cpu_cores.size(),snapshot.cpu_cores.size());text=b;}
    else if(data.contains(key) && data[key].is_number()) {
        if(key=="frametime" || key=="ping" || key=="jitter")unit="ms";
        if(key=="cpu" || key=="gpu")unit="%";
        if(key=="fps" || key=="fps_low")unit="FPS";
        if(key=="gpu_temp" || key=="cpu_temp")unit="°C";
        if(key=="gpu_clock")unit="MHz";if(key=="cpu_clock")unit="GHz";if(key=="gpu_fan")unit="%";if(key=="gpu_power")unit="W";
        char b[64];snprintf(b,sizeof b,key=="cpu_clock"?"%.2f":(key=="fps"||key=="fps_low"||key=="cpu"||key=="gpu")?"%.0f":"%.1f",data[key].get<double>());text=b;
        if(key=="cpu" || key=="gpu")ringload=data[key].get<double>();
    }
    const bool ringed=ringload && a.height>=70 && a.width/a.height<=1.7f;
    if(key=="game_logo" && logo && !logo->empty()) {float fit=std::min((a.width-16)*scale/logo->w,(a.height-36)*scale/logo->h);if(fit>0){auto image=resize(*logo,std::max(1,int(logo->w*fit)),std::max(1,int(logo->h*fit)));c.image(image,a.x+(a.width-image.w/scale)/2,a.y+28);}}
    else if(ringed){
        float d=std::min(a.height-34,a.width-20),cx=a.x+a.width/2,cy=a.y+a.height/2+11,r=d/2,th=std::max(5.f,d*.12f);
        auto track=mix_color(theme.card,theme.muted,.3f);ring_arc(c,cx,cy,r,r-th,1,track);ring_arc(c,cx,cy,r,r-th,std::clamp(*ringload/100,0.,1.),level_color(theme,*ringload));
        Font rv{std::clamp(d*.30f,14.f,48.f),true};c.text(cx,cy,text,rv,theme.text,"mm");
    }
    else {
        Font unit_font{std::max(11.f,value.size*.42f),true};
        auto width=[&]{return c.textlength(text,value)+(unit.empty()?0.f:c.textlength(unit,unit_font)+4);};
        while(value.size>12 && width()>a.width-16){value.size-=.5f;unit_font.size=std::max(10.f,value.size*.42f);}
        while(text.size()>4 && width()>a.width-16){auto points=to_u32(text);if(points.size()<3)break;points.resize(points.size()-2);text=from_u32(points)+"…";}
        float y=a.y+a.height*(hero?.5f:.56f);c.text(a.x+10,y,text,value,theme.text,"lm");
        if(!unit.empty())c.text(a.x+10+c.textlength(text,value)+4,y+value.size*.1f,unit,unit_font,theme.accent,"lm");
    }
    if((tile.density!=0 || hero) && a.height>=44 && !ringed){
        const auto& history=key=="fps"?snapshot.fps_hist:snapshot.frametime_hist;
        if(key=="fps" || key=="frametime") {
            double maximum=1;for(auto v:history)if(std::isfinite(v))maximum=std::max(maximum,v);
            const float base=a.y+a.height-8,amp=hero?.3f:.16f;
            std::vector<PointF> points;
            auto flush=[&]{if(points.size()>1){auto area=points;area.push_back({points.back().x,base});area.push_back({points.front().x,base});c.polygon(area,mix_color(theme.card,theme.accent,.22f));c.line(points,theme.accent,2);}points.clear();};
            for(size_t i=0;i<history.size();++i) {
                if(!std::isfinite(history[i])){flush();continue;}
                points.push_back({a.x+8+float(i)*(a.width-16)/59.f,base-float(history[i]/maximum)*a.height*amp});
            }flush();
        } else {
            auto load=key=="cpu"?snapshot.cpu_pct:key=="gpu"?snapshot.gpu_pct:std::optional<double>{};
            if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total && *snapshot.gpu_mem_total>0)load=100* *snapshot.gpu_mem_used/ *snapshot.gpu_mem_total;
            if(load){c.rounded_rectangle(a.x+8,a.y+a.height-13,a.x+a.width-9,a.y+a.height-8,2.5f,mix_color(theme.card,theme.muted,.3f));
                c.rounded_rectangle(a.x+8,a.y+a.height-13,a.x+8+(a.width-16)*float(std::clamp(*load/100,0.,1.)),a.y+a.height-8,2.5f,level_color(theme,*load));}
            if(key=="cpu_cores" && !snapshot.cpu_cores.empty()){float dx=(a.width-16)/snapshot.cpu_cores.size();for(size_t i=0;i<snapshot.cpu_cores.size();++i)c.rectangle(a.x+8+i*dx,a.y+a.height-8-float(snapshot.cpu_cores[i]/100)*a.height*.16f,a.x+8+(i+1)*dx-1,a.y+a.height-8,level_color(theme,snapshot.cpu_cores[i]));}
        }
    }
    c.clip_pop();return c.finish();
}
}
