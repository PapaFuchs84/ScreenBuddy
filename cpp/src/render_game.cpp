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
    // ---- Fixed grid: one inner padding, content rect centred in the tile, label on top, optional chart/bar at the bottom ----
    const float PAD=std::clamp(a.height*.11f,5.f,10.f),GAP=std::clamp(a.height*.07f,3.f,8.f),BAR=std::clamp(a.height*.08f,4.f,6.f);   // padding scales with the tile so small tiles keep room for the value
    const float cx=a.x+a.width/2;
    const bool hero=key=="fps" && a.height>=100;
    if(hero){c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,mix_color(theme.card,theme.accent,.55f));c.rounded_rectangle(a.x+1.5f,a.y+1.5f,a.x+a.width-2.5f,a.y+a.height-2.5f,6.5f,theme.card);}
    else c.rounded_rectangle(a.x,a.y,a.x+a.width-1,a.y+a.height-1,8,theme.card);
    c.rounded_rectangle(a.x,a.y+a.height*.32f,a.x+2.5f,a.y+a.height*.68f,1.2f,theme.accent);   // accent stripe, vertically centred
    c.clip_push(a.x+3,a.y+3,a.x+a.width-4,a.y+a.height-4);
    std::string title=tile.block;for(const auto& b:builtin::find("system")->blocks)if(b.id==tile.block)title=i18n::lang()=="en"?b.label.en:b.label.de;
    if(snapshot.fps_reason=="hwinfo_presentmon"){if(key=="fps")title="FPS · HWiNFO";else if(key=="fps_low")title="FPS · 1 % Low · HWiNFO";else if(key=="frametime")title="Frametime · HWiNFO";}
    title=upper_ascii(title);
    auto F=[&](float logical){return Font{logical*scale,true};};   // Font sizes are real pixels, positions are logical units: always go through F
    float lab=std::clamp(a.height/10.f,8.f,12.f);
    const float content_w=a.width-2*PAD;
    while(lab>8 && c.textlength(title,F(lab))>content_w)lab-=.5f;
    c.text(cx,a.y+PAD+lab*.5f,title,F(lab),theme.muted,"mm");
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
    const bool ringed=ringload && a.height>=70 && a.width/a.height<=2.4f;
    const bool widgets=(tile.density!=0 || hero) && a.height>=44 && !ringed;
    size_t valid_samples=0;if(key=="fps" || key=="frametime")for(auto v:(key=="fps"?snapshot.fps_hist:snapshot.frametime_hist))if(std::isfinite(v))++valid_samples;
    const bool chart=widgets && (key=="fps" || key=="frametime") && valid_samples>1;   // without history the number takes the whole tile
    std::optional<double> load;
    if(widgets && !chart){load=key=="cpu"?snapshot.cpu_pct:key=="gpu"?snapshot.gpu_pct:std::optional<double>{};
        if(key=="vram" && snapshot.gpu_mem_used && snapshot.gpu_mem_total && *snapshot.gpu_mem_total>0)load=100* *snapshot.gpu_mem_used/ *snapshot.gpu_mem_total;}
    const bool cores=widgets && key=="cpu_cores" && !snapshot.cpu_cores.empty();
    // content rect (below the label, inside the padding) and the space the bottom widget takes
    const float top=a.y+PAD+lab+GAP,bottom=a.y+a.height-PAD;
    const bool side=chart && !hero && a.width>=a.height*2.0f;   // wide chart tile: value on the left, chart on the right (full content height)
    const float chart_h=chart?(side?std::max(12.f,bottom-top):std::clamp(a.height*(hero?.30f:.26f),12.f,90.f)):0;
    const float reserve=side?0.f:chart?chart_h+GAP:(load||cores)?(cores?std::clamp(a.height*.16f,8.f,40.f):BAR)+GAP:0;
    const float avail_bottom=bottom-reserve,avail_h=std::max(10.f,avail_bottom-top),cy=(top+avail_bottom)/2;
    const float value_w=side?content_w*.34f:content_w,value_cx=side?a.x+PAD+value_w/2:cx;   // area the number is centred in
    // Optical vertical centring: digits sit on the baseline, so the line-box middle ("lm"/"mm") is shifted by K * font size.
    constexpr float K=0.045f;
    if(key=="game_logo" && logo && !logo->empty()) {
        const float fit=std::min(content_w*scale/logo->w,(bottom-top)*scale/logo->h);
        if(fit>0){auto image=resize(*logo,std::max(1,int(logo->w*fit)),std::max(1,int(logo->h*fit)));c.image(image,cx-image.w/scale/2,top+((bottom-top)-image.h/scale)/2);}
    }
    else if(ringed){
        const bool wide=a.width>=a.height*1.25f;   // wide: number left, ring right-aligned; square: number inside the ring
        const float d=std::max(20.f,wide?std::min(avail_h,content_w*.42f):std::min(avail_h,content_w)),r=d/2,th=std::max(5.f,d*.11f);
        const float rcx=wide?a.x+a.width-PAD-r:cx;
        const auto track=mix_color(theme.card,theme.muted,.3f);
        ring_arc(c,rcx,cy,r,r-th,1,track);ring_arc(c,rcx,cy,r,r-th,std::clamp(*ringload/100,0.,1.),level_color(theme,*ringload));
        if(wide){
            const float region_l=a.x+PAD,region_r=rcx-r-PAD,region_c=(region_l+region_r)/2;
            float big=std::clamp(avail_h*.74f,12.f,92.f),small=std::max(7.f,big*.40f);
            auto total=[&]{return c.textlength(text,F(big))+c.textlength("%",F(small))+big*.14f;};
            while(big>10 && total()>(region_r-region_l)*.9f){big-=.5f;small=std::max(7.f,big*.40f);}   // keep air on both sides of the number
            const float x0=region_c-total()/2,y=cy+big*K;
            c.text(x0,y,text,F(big),theme.text,"lm");c.text(x0+c.textlength(text,F(big))+big*.14f,y+(big-small)*.36f,"%",F(small),theme.accent,"lm");
        } else {
            const float inner=d-2*th,big=std::clamp(inner*.56f,10.f,110.f),small=std::max(7.f,big*.36f);
            const float block=big+small*.9f,y0=cy-block/2;   // number + percent sign stacked, centred together
            c.text(rcx,y0+big*.5f+big*K,text,F(big),theme.text,"mm");c.text(rcx,y0+big+small*.45f,"%",F(small),theme.accent,"mm");
        }
    }
    else {
        float value=std::clamp(avail_h*(hero?.80f:.78f)*(text=="n/a"?.62f:1.f),12.f,hero?130.f:90.f),unit_size=std::max(7.f,value*.40f);
        auto gap=[&]{return unit.empty()?0.f:value*.14f;};
        auto width=[&]{return c.textlength(text,F(value))+(unit.empty()?0.f:c.textlength(unit,F(unit_size))+gap());};
        while(value>9 && width()>value_w){value-=.5f;unit_size=std::max(7.f,value*.40f);}
        while(text.size()>4 && width()>value_w){auto points=to_u32(text);if(points.size()<3)break;points.resize(points.size()-2);text=from_u32(points)+"…";}
        const float x0=value_cx-width()/2,y=cy+value*K;
        c.text(x0,y,text,F(value),theme.text,"lm");
        if(!unit.empty())c.text(x0+c.textlength(text,F(value))+gap(),y+(value-unit_size)*.36f,unit,F(unit_size),theme.accent,"lm");   // unit sits on the value's baseline
    }
    if(chart){
        const auto& history=key=="fps"?snapshot.fps_hist:snapshot.frametime_hist;
        // One continuous line: invalid samples are bridged, newest sample at the right edge, scaled between min and max of the window.
        // Robust scale: 5th..95th percentile, so a start-up outlier does not flatten the whole line (values outside are clamped).
        std::vector<double> sorted;for(auto v:history)if(std::isfinite(v))sorted.push_back(v);
        std::sort(sorted.begin(),sorted.end());const size_t valid=sorted.size();
        const double minimum=valid?sorted[size_t(valid*.05)]:0,maximum=valid?sorted[std::min(valid-1,size_t(valid*.95))]:0;
        const float base=bottom,left=side?a.x+PAD+content_w*.42f:a.x+PAD,span=side?content_w*.58f:content_w;
        std::vector<PointF> points;const size_t count=history.size(),shift=count<60?60-count:0;
        for(size_t i=0;i<count;++i){
            if(!std::isfinite(history[i]))continue;
            const double norm=maximum>minimum?std::clamp((history[i]-minimum)/(maximum-minimum),0.,1.):.5;
            points.push_back({left+float(i+shift)*span/59.f,base-float(.12+.88*norm)*chart_h});
        }
        if(valid>1){auto area=points;area.push_back({points.back().x,base});area.push_back({points.front().x,base});c.polygon(area,mix_color(theme.card,theme.accent,.22f));c.line(points,theme.accent,2);}
    }
    else if(load){
        const float y0=bottom-BAR,x0=a.x+PAD,x1=a.x+a.width-PAD;
        c.rounded_rectangle(x0,y0,x1,bottom,BAR/2,mix_color(theme.card,theme.muted,.3f));
        const float fill=(x1-x0)*float(std::clamp(*load/100,0.,1.));if(fill>BAR)c.rounded_rectangle(x0,y0,x0+fill,bottom,BAR/2,level_color(theme,*load));
    }
    else if(cores){
        const float ch=reserve-GAP,dx=content_w/snapshot.cpu_cores.size();
        for(size_t i=0;i<snapshot.cpu_cores.size();++i)c.rectangle(a.x+PAD+i*dx+1,bottom-float(std::clamp(snapshot.cpu_cores[i],0.,100.)/100)*ch,a.x+PAD+(i+1)*dx-1,bottom,level_color(theme,snapshot.cpu_cores[i]));
    }
    c.clip_pop();return c.finish();
}
}
