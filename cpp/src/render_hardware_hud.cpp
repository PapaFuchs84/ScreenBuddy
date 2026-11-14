// © 2026 FuchsLabs
#include "render_hardware_hud.h"
#include "i18n.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fl::render {
namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr Color BG{5, 8, 18}, PANEL{9, 17, 31}, CYAN{58, 228, 245}, PINK{222, 78, 240};
constexpr Color WHITE{225, 246, 255}, MUTED{121, 151, 177}, GRID{10, 22, 36}, DIM_C{14, 66, 80}, DIM_P{61, 25, 77};

const char* label(const char* de, const char* en) { return i18n::lang()=="en"?en:de; }
Font font(const Canvas& c, float size, bool bold = false) { return {std::max(7.f, size*c.s()), bold, L"Bahnschrift"}; }
Color mix(Color a, Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return {uint8_t(a.r + (b.r-a.r)*t), uint8_t(a.g + (b.g-a.g)*t), uint8_t(a.b + (b.b-a.b)*t)};
}
std::optional<double> valid(std::optional<double> n) {
    return n && std::isfinite(*n) && *n >= 0 ? n : std::nullopt;
}
std::string number(std::optional<double> n, int decimals = 0) {
    n = valid(n);
    if (!n) return "--";
    std::ostringstream s; s << std::fixed << std::setprecision(decimals) << *n; return s.str();
}
std::string value(std::optional<double> n, const char* unit, int decimals = 0) {
    return number(n, decimals) + (valid(n) ? std::string(" ") + unit : "");
}
std::string fit(Canvas& c, std::string text, const Font& f, float width) {
    if (c.textlength(text, f) <= width) return text;
    while (!text.empty() && c.textlength(text + "...", f) > width) {
        // Never leave half of a UTF-8 character behind.
        size_t end = text.size() - 1;
        while (end && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80) --end;
        text.resize(end);
    }
    return text + "...";
}
void wire(Canvas& c, const std::vector<PointF>& points, Color col, float width = 1.6f) {
    c.line(points, mix(BG, col, .10f), width + 7);
    c.line(points, mix(BG, col, .25f), width + 3);
    c.line(points, col, width);
}
std::vector<PointF> hex(float x, float y, float rx, float ry) {
    return {{x-rx*.52f,y-ry},{x+rx*.52f,y-ry},{x+rx,y},{x+rx*.52f,y+ry},{x-rx*.52f,y+ry},{x-rx,y},{x-rx*.52f,y-ry}};
}
void outline(Canvas& c, float x, float y, float rx, float ry, Color col, bool glow=true) {
    if (glow) wire(c, hex(x,y,rx,ry), col);
    else c.line(hex(x,y,rx,ry),col,1);
}
void ring(Canvas& c, float x, float y, float r, Color col) {
    c.ellipse_outline(x-r,y-r,x+r,y+r,mix(BG,col,.18f),5);
    c.ellipse_outline(x-r,y-r,x+r,y+r,col,1.6f);
}
void arc(Canvas& c,float x,float y,float r,float start,float sweep,Color col,float width=2) {
    std::vector<PointF> pts;
    const int steps=std::max(2,int(std::abs(sweep)*r/10));
    for(int i=0;i<=steps;++i) {
        const float a=start+sweep*i/steps;
        pts.push_back({x+r*std::cos(a),y+r*std::sin(a)});
    }
    c.line(pts,col,width);
}
void orbit(Canvas& c,float x,float y,float r,double tick,Color col) {
    for(int i=0;i<24;++i) {
        const float a=i*2*PI/24;
        const float wave=.5f+.5f*std::sin(float(tick)*2.6f-i*.48f);
        arc(c,x,y,r,a,.16f,mix(DIM_C,col,wave*.7f),1.6f);
    }
    const float phase=float(std::fmod(tick*.9,double(2*PI)));
    arc(c,x,y,r+5,phase,.85f,col,3);
    arc(c,x,y,r+5,phase+PI,.32f,PINK,2.2f);
    arc(c,x,y,r-5,-phase*.65f,1.1f,mix(DIM_C,PINK,.8f),1.5f);
}
void circuit(Canvas& c,const std::vector<PointF>& pts,Color col,double tick,float offset=0) {
    if(pts.size()<2) return;
    wire(c,pts,mix(BG,col,.35f),1.2f);
    float length=0;
    for(size_t i=1;i<pts.size();++i) length+=std::hypot(pts[i].x-pts[i-1].x,pts[i].y-pts[i-1].y);
    if(length<=0) return;
    for(int trail=0;trail<5;++trail) {
        float distance=float(std::fmod(tick*110+offset+length-trail*7,length));
        if(distance<0) distance+=length;
        for(size_t i=1;i<pts.size();++i) {
            const float segment=std::hypot(pts[i].x-pts[i-1].x,pts[i].y-pts[i-1].y);
            if(distance<=segment && segment>0) {
                const float t=distance/segment;
                const float x=pts[i-1].x+(pts[i].x-pts[i-1].x)*t,y=pts[i-1].y+(pts[i].y-pts[i-1].y)*t;
                const float r=trail?1.5f:2.8f;
                c.ellipse(x-r,y-r,x+r,y+r,mix(BG,col,1.f-trail*.15f)); break;
            }
            distance-=segment;
        }
    }
}
void fan(Canvas& c, float x, float y, float r, double tick, Color col) {
    ring(c,x,y,r,col);
    c.ellipse_outline(x-r*.83f,y-r*.83f,x+r*.83f,y+r*.83f,mix(BG,col,.35f),1);
    const float phase = float(std::fmod(tick*3.1, double(2*PI)));
    for (int i=0;i<7;++i) {
        const float a=phase+i*2*PI/7;
        std::vector<PointF> p;
        for (const auto [rr, da] : std::array<std::pair<float,float>,4>{{{.19f,0.f},{.78f,.24f},{.73f,.56f},{.26f,.72f}}})
            p.push_back({x+r*rr*std::cos(a+da),y+r*rr*std::sin(a+da)});
        const Color blade=i==0?CYAN:col;
        c.polygon(p,mix(BG,blade,i==0?.78f:.39f)); p.push_back(p.front()); c.line(p,blade,1.2f);
    }
    arc(c,x,y,r*.9f,-phase*.7f,.65f,CYAN,2);
    c.ellipse(x-r*.12f,y-r*.12f,x+r*.12f,y+r*.12f,col);
}
// Original schematic assets drawn in a 300 x 220 coordinate system.
void component(Canvas& c, float x, float y, float size, int page, double tick) {
    const float k=size/300;
    auto p=[&](float a,float b){return PointF{x+a*k,y+b*k};};
    auto path=[&](std::initializer_list<PointF> pts, Color color, float width=1.7f){
        std::vector<PointF> q; for (auto v:pts) q.push_back(p(v.x,v.y)); wire(c,q,color,width*k);
    };
    if (page==1) {
        path({{10,65},{278,65},{290,77},{290,159},{278,171},{10,171},{10,65}},CYAN,2);
        path({{2,49},{2,184},{22,184}},PINK,2);
        for(int i=0;i<3;++i) fan(c,x+(60+i*88)*k,y+118*k,35*k,tick+i*.25,PINK);
        path({{48,172},{48,186},{167,186},{167,172}},CYAN);
        for(int i=0;i<11;++i) c.rectangle(x+(53+i*10)*k,y+174*k,x+(58+i*10)*k,y+183*k,mix(BG,CYAN,.6f));
        path({{246,65},{246,49},{274,49},{274,65}},PINK);
    } else if (page==2) {
        path({{73,30},{227,30},{244,47},{244,190},{227,207},{73,207},{56,190},{56,47},{73,30}},PINK,2);
        path({{89,59},{211,59},{216,64},{216,174},{211,179},{89,179},{84,174},{84,64},{89,59}},CYAN,2);
        for(int i=0;i<9;++i) {
            float a=83.f+i*17.f;
            path({{a,17},{a,30}},CYAN,1.3f);path({{a,207},{a,220}},CYAN,1.3f);
            path({{42,51+i*17.f},{56,51+i*17.f}},PINK,1.3f);path({{244,51+i*17.f},{258,51+i*17.f}},PINK,1.3f);
        }
        for(int r=0;r<3;++r) for(int j=0;j<3;++j) {
            const bool lit=(r*3+j+int(tick*2))%5<2;
            const auto a=p(114.f+j*26.f,83.f+r*26.f);
            c.rectangle(a.x,a.y,a.x+17*k,a.y+17*k,lit?CYAN:DIM_C);
        }
    } else {
        path({{77,12},{226,12},{246,32},{246,211},{77,211},{77,12}},PINK,2);
        path({{89,25},{220,25},{231,36},{231,196},{89,196},{89,25}},CYAN);
        path({{103,48},{170,48},{170,121},{103,121},{103,48}},PINK);
        path({{123,68},{149,68},{149,95},{123,95},{123,68}},CYAN);
        for(int i=0;i<3;++i) fan(c,x+202*k,y+(64+i*50)*k,18*k,tick+i*.2,CYAN);
        for(int i=0;i<3;++i) path({{102,139+i*12.f},{174,139+i*12.f}},CYAN,2);
        path({{111,122},{111,130},{184,130},{184,102}},PINK);
        path({{107,212},{107,220}},CYAN,3);path({{218,212},{218,220}},CYAN,3);
        orbit(c,x+156*k,y+111*k,132*k,tick,CYAN);
    }
}
void temperature(Canvas& c, float x, float y, float r, std::optional<double> temp, const std::string& label,double tick) {
    const Color color=valid(temp)&&*temp>=85?Color{255,107,94}:CYAN;
    c.polygon(hex(x,y,r*.94f,r*.72f),PANEL);
    outline(c,x,y,r,r*.77f,color);
    outline(c,x,y,r*.9f,r*.67f,PINK,false);
    const auto edge=hex(x,y,r*1.035f,r*.805f);
    circuit(c,edge,mix(color,PINK,.25f),tick,40);
    std::string caption=label;
    if(r*c.s()<110) caption=label.find("GPU")!=std::string::npos?"GPU TEMP.":"CPU TEMP.";
    const Font captionFont=font(c,r*.105f,true);
    c.text(x,y-r*.42f,fit(c,caption,captionFont,r*1.45f),captionFont,MUTED,"mm");
    c.text(x,y+r*.015f,number(temp),font(c,r*.48f,true),WHITE,"mm");
    c.text(x,y+r*.38f,valid(temp)?"°C":"SENSOR --",font(c,r*.13f,true),color,"mm");
}
struct Metric { const char* label; std::string val; std::optional<double> pct; Color col; };
void metric(Canvas& c,float x,float y,float width,float height,const Metric& m,double tick,int ordinal) {
    c.clip_push(x,y,x+width-1,y+height-1);
    c.rectangle(x,y,x+width,y+height-1,PANEL);
    c.line({{x,y+height-1},{x+width,y+height-1}},mix(GRID,m.col,.25f),1);
    c.rectangle(x,y+7,x+2,y+height-8,m.col);
    const float fs=std::min(25.f,height*.33f);
    const Font captionFont=font(c,std::min(12.f,height*.18f),true);
    Font valueFont=font(c,fs,true);
    const float available=std::max(1.f,width-24);
    const bool compact=c.textlength(m.label,captionFont)+c.textlength(m.val,valueFont)+12>available;
    const float labelY=compact?.17f:.3f,valueY=compact?.52f:.3f;
    const float valueHeight=compact?height*.34f:height*.5f;
    // The font API uses physical pixels, including its one-pixel semibold adjustment.
    valueFont.size=std::min(valueFont.size,std::max(7.f,valueHeight*c.s()-1));
    c.text(x+13,y+height*labelY,fit(c,m.label,captionFont,available),captionFont,MUTED,"lm");
    c.text(compact?x+13:x+width-10,y+height*valueY,fit(c,m.val,valueFont,available),valueFont,WHITE,compact?"lm":"rm");
    const float by=y+height*(compact?.83f:.73f);
    c.rectangle(x+13,by,x+width-10,by+2,GRID);
    auto n=valid(m.pct);
    if(n) c.rectangle(x+13,by,x+13+(width-23)*float(std::clamp(*n,0.,100.)/100.),by+2,m.col);
    // Decorative travelling indicator is kept separate from the measured bar.
    const float cursor=float(std::fmod(tick*.48+ordinal*.17,1.));
    c.rectangle(x+width-5,y+6+(height-18)*cursor,x+width-3,y+10+(height-18)*cursor,m.col);
    c.clip_pop();
}
std::optional<double> percent(std::optional<double> a,std::optional<double> b) {
    return valid(a)&&valid(b)&&*b>0 ? std::optional<double>(*a/ *b*100.) : std::nullopt;
}
std::optional<double> gib(std::optional<double> mib) {return valid(mib)?std::optional<double>(*mib/1024.):std::nullopt;}

} // namespace

Image hardware_hud(int width,int height,const SysSnapshot& snap,double tick,int page_seconds) {
    width=std::max(width,1); height=std::max(height,1);
    tick=std::isfinite(tick)&&tick>=0?std::fmod(tick,86400.):0;
    page_seconds=std::clamp(page_seconds,3,60);
    const int page=int(tick/page_seconds)%3;
    const double age=std::fmod(tick,double(page_seconds));
    const float entrance=1.f-std::pow(1.f-std::clamp(float(age/.65),0.f,1.f),3.f);
    const bool portrait=width<float(height)*1.15f;
    const float scale=portrait?width/620.f:std::min(width/1000.f,height/360.f);
    const float w=width/scale,h=height/scale;
    Canvas c(width,height,scale,BG);
    c.clip_push(0,0,w,h);

    // Faint animated circuitry leaves the foreground values easy to read.
    // Spacing grows for extreme aspect ratios so the grid stays at most ~24 × 16 cells.
    const float step_x=std::max(94.f,w/24.f),step_y=std::max(54.f,h/16.f);
    int row=0;
    for(float yy=65;yy<h-35;yy+=step_y,++row) for(float xx=25;xx<w;xx+=step_x) {
        const float phase=float(tick)*1.7f-xx*.009f-yy*.014f;
        const float glow=std::pow(std::max(0.f,std::sin(phase)),8.f)*.26f;
        outline(c,xx+((row+1)%2?step_x/2:0),yy,28,24,mix(GRID,DIM_C,glow),false);
    }
    const float scan=float(std::fmod(tick*90.,double(std::max(w,1.f))));
    c.rectangle(scan,52,scan+1,h-36,Color{9,27,41});
    circuit(c,{{20,38},{w*.25f,38},{w*.25f+15,23},{w-20,23}},CYAN,tick);
    circuit(c,{{20,h-25},{w*.67f,h-25},{w*.67f+14,h-39},{w-20,h-39}},PINK,tick,170);
    const Font headerFont=font(c,portrait?13.f:12.f,true);
    c.text(24,18,fit(c,"SCREENBUDDY / HARDWARE HUD",headerFont,w-150),headerFont,CYAN,"lm");
    const std::array<const char*,3> pages{label("SYSTEMÜBERSICHT","SYSTEM OVERVIEW"),label("GPU-TELEMETRIE","GPU TELEMETRY"),label("CPU-TELEMETRIE","CPU TELEMETRY")};
    c.text(w-24,43,pages[page],font(c,portrait?15.f:14.f,true),PINK,"rm");
    for(int i=0;i<3;++i) c.rectangle(w-100+i*26,11,w-80+i*26,14,i==page?CYAN:DIM_C);
    // A brief schematic sweep marks each page change; readings remain visible throughout.
    if(entrance<1) c.line({{20+(w-40)*entrance,58},{20+(w-40)*entrance,h-45}},mix(BG,CYAN,(1-entrance)*.6f),2);
    const float top=69,bottom=h-46,content=std::max(150.f,bottom-top);

    if(page==0) {
        const std::array<Metric,6> rows{{
            {label("CPU-LAST","CPU LOAD"),value(snap.cpu_pct,"%"),snap.cpu_pct,CYAN},
            {label("GPU-LAST","GPU LOAD"),value(snap.gpu_pct,"%"),snap.gpu_pct,PINK},
            {label("GPU-TEMP.","GPU TEMP"),value(snap.gpu_temp,"°C"),std::nullopt,CYAN},
            {label("ARBEITSSPEICHER","MEMORY"),value(snap.ram_pct,"%"),snap.ram_pct,PINK},
            {label("CPU-LÜFTER","CPU FAN"),value(snap.cpu_fan,"RPM"),std::nullopt,CYAN},
            {label("GPU-LEISTUNG","GPU POWER"),value(snap.gpu_watts,"W"),std::nullopt,PINK}
        }};
        const bool wide=!portrait&&w/h>2.65f;
        const float centerY=portrait?top+content*.23f:(wide?top+content*.5f:top+content*.3f);
        const float artSize=portrait?245.f:(wide?std::min(w*.29f,content*1.05f):std::min(w*.31f,content*.62f));
        const float artX=portrait?10.f:(wide?w*.17f-artSize*.52f:w*.04f);
        component(c,artX-10*(1-entrance),centerY-artSize*.37f,artSize,0,tick);
        const float tempX=portrait?w*.73f:(wide?w*.845f:w*.72f);
        const float radius=portrait?128.f:(wide?std::min(w*.143f,content*.51f):std::min(w*.19f,content*.32f));
        temperature(c,tempX,centerY,radius,snap.cpu_temp,label("CPU-TEMPERATUR","CPU TEMPERATURE"),tick);
        if(wide) {
            const float mx=w*.335f,mw=w*.325f,gap=14,mh=(content-20)/3;
            for(int i=0;i<6;++i) metric(c,mx+(i%2)*(mw+gap)/2,top+8+(i/2)*mh,(mw-gap)/2,mh-6,rows[i],tick,i);
            circuit(c,{{w*.29f,centerY},{w*.31f,centerY},{mx-8,centerY-28}},CYAN,tick,70);
            circuit(c,{{mx+mw+8,centerY},{tempX-radius-24,centerY},{tempX-radius-7,centerY-17}},PINK,tick,150);
        } else {
            if(!portrait) circuit(c,{{w*.34f,centerY},{w*.43f,centerY},{w*.47f,centerY-25},{tempX-radius-8,centerY-25}},CYAN,tick);
            const int cols=portrait?2:3,nrows=portrait?3:2;
            const float metricY=portrait?top+content*.5f:top+content*.62f;
            const float gap=portrait?22.f:25.f,margin=portrait?25.f:32.f;
            const float mw=(w-2*margin-(cols-1)*gap)/cols,mh=(bottom-metricY)/nrows;
            for(int i=0;i<6;++i) metric(c,margin+(i%cols)*(mw+gap),metricY+(i/cols)*mh,mw,mh-6,rows[i],tick,i);
        }
    } else {
        const bool gpu=page==1;
        const std::string name=gpu?snap.gpu_name:snap.cpu_name;
        const Font nameFont=font(c,portrait?22.f:21.f,true);
        c.text(26,top+10,fit(c,name,nameFont,w-52),nameFont,WHITE,"lm");
        const float areaTop=top+39,areaH=bottom-areaTop;
        const float centerY=portrait?areaTop+areaH*.23f:areaTop+areaH*.5f;
        const float artSize=portrait?270.f:std::min(w*.315f,areaH*(gpu?2.05f:1.32f));
        const float artX=portrait?4.f:w*.177f-artSize*.5f;
        if(!portrait) orbit(c,w*.177f,centerY,std::min(artSize*.47f,areaH*.44f),tick,CYAN);
        component(c,artX-10*(1-entrance),centerY-artSize*.395f,artSize,page,tick);
        const float radius=portrait?130.f:std::min(w*.155f,areaH*.51f);
        const float tempX=portrait?w*.75f:w*.835f;
        temperature(c,tempX,centerY,radius,gpu?snap.gpu_temp:snap.cpu_temp,gpu?label("GPU-TEMPERATUR","GPU TEMPERATURE"):label("CPU-TEMPERATUR","CPU TEMPERATURE"),tick);
        std::array<Metric,4> rows;
        if(gpu) rows={{{label("AUSLASTUNG","UTILIZATION"),value(snap.gpu_pct,"%"),snap.gpu_pct,CYAN},
                       {"VRAM",value(gib(snap.gpu_mem_used),"GiB",1),percent(snap.gpu_mem_used,snap.gpu_mem_total),PINK},
                       {label("LEISTUNG","POWER"),value(snap.gpu_watts,"W"),std::nullopt,CYAN},
                       {label("LÜFTER","FAN"),value(snap.gpu_fan,"%"),snap.gpu_fan,PINK}}};
        else rows={{{label("AUSLASTUNG","UTILIZATION"),value(snap.cpu_pct,"%"),snap.cpu_pct,CYAN},
                    {label("TAKT","CLOCK"),value(snap.cpu_ghz,"GHz",2),std::nullopt,PINK},
                    {label("LEISTUNG","POWER"),value(snap.cpu_watts,"W"),std::nullopt,CYAN},
                    {label("LÜFTER","FAN"),value(snap.cpu_fan,"RPM"),std::nullopt,PINK}}};
        const float mx=portrait?26.f:w*.355f,my=portrait?areaTop+areaH*.49f:areaTop+3;
        const float mw=portrait?w-52:w*.285f,mh=(bottom-my)/4;
        for(int i=0;i<4;++i) metric(c,mx,my+i*mh,mw,mh-5,rows[i],tick,i);
        if(!portrait) {
            circuit(c,{{w*.325f,centerY},{mx-18,centerY},{mx-8,areaTop+mh*.5f}},CYAN,tick,40);
            circuit(c,{{mx+mw+10,centerY},{tempX-radius-27,centerY},{tempX-radius-7,centerY-18}},PINK,tick,120);
        }
    }
    const std::string sensor=snap.hwinfo?label("HWiNFO + WINDOWS / -- = NICHT VERFÜGBAR","HWiNFO + WINDOWS / -- = UNAVAILABLE"):label("WINDOWS / HWiNFO FÜR CPU-SENSOREN / -- = NICHT VERFÜGBAR","WINDOWS / HWiNFO FOR CPU SENSORS / -- = UNAVAILABLE");
    const Font footerFont=font(c,10);
    const std::string schematic=label("LÜFTER: SCHEMATISCH","FANS: SCHEMATIC");
    const float footerBudget=std::max(1.f,w-64-c.textlength(schematic,footerFont));
    c.text(24,h-12,fit(c,sensor,footerFont,footerBudget),footerFont,MUTED,"lm");
    c.text(w-24,h-12,schematic,footerFont,MUTED,"rm");
    c.clip_pop();
    return c.finish();
}
} // namespace fl::render
