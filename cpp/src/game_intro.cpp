// © 2026 FuchsLabs – Game-Mode intro in the Hardware-HUD look; uses the premultiplied-alpha fade path.
#include "game_intro.h"
#include "util.h"
#include "render.h"
#include <algorithm>
#include <cmath>
namespace fl::game {
namespace {
float smooth(double v){float x=float(std::clamp(v,0.,1.));return x*x*(3-2*x);}
Color mixc(Color a,Color b,float t){t=std::clamp(t,0.f,1.f);return {uint8_t(a.r+(b.r-a.r)*t),uint8_t(a.g+(b.g-a.g)*t),uint8_t(a.b+(b.b-a.b)*t)};}
}
IntroPhase intro_phase(double elapsed,bool reduced){
    if(!std::isfinite(elapsed))return {1,1,-1,1,true};
    double t=std::max(0.,elapsed);
    if(reduced)return {1,1,-1,smooth(t/.35),t>=.35};
    return {smooth(t/.6),.88f+.12f*smooth(t/.8),float((t-.65)/1.1),smooth((t-2.05)/.45),t>=2.5};
}
void Intro::begin(Image previous,double now,bool reduced){cache_={};cache_.from=std::move(previous);since_=now;reduced_=reduced;logo_src_=nullptr;logo_scaled_={};}
bool Intro::active(double now)const{return since_>=0 && !intro_phase(now-since_,reduced_).done;}
void Intro::reset(){cache_={};since_=-1;logo_src_=nullptr;logo_scaled_={};}
Image Intro::draw(const Image& target,const ImagePtr& logo,const std::string& name,double now,const IntroColors& colors){
    const double elapsed=now-since_;
    auto phase=intro_phase(elapsed,reduced_);if(phase.done)return target;
    if(reduced_) {auto result=target;if(cache_.from.w==target.w && cache_.from.h==target.h)plugins::blend_rect(result,cache_.from,target,{0,0,float(target.w),float(target.h)},phase.fade);return result;}
    const int W=target.w,H=target.h;
    const float unit=std::max(1.f,std::min(W/480.f,H/320.f));
    // The previous frame is resized once (not per frame) if the output size changed.
    if(!cache_.from.empty() && (cache_.from.w!=W || cache_.from.h!=H))cache_.from=resize(cache_.from,W,H);
    // HUD elements go onto a transparent canvas; the dimmed previous page is mixed in below (one packed
    // pass instead of uploading and drawing the full-size bitmap with opacity in software Direct2D).
    Canvas canvas(W,H,1,colors.bg,0);
    // HUD grid fades in over the dimmed page.
    if(phase.reveal>.7f){   // only once the page is mostly dimmed, so the lines never read as dark scratches
        const Color grid=mixc(colors.bg,colors.primary,.13f*(phase.reveal-.7f)/.3f);
        const float step=std::round(24*unit);
        for(float x=std::fmod(W*.5f,step);x<W;x+=step)canvas.rectangle(x,0,x,float(H-1),grid);
        for(float y=std::fmod(H*.5f,step);y<H;y+=step)canvas.rectangle(0,y,float(W-1),y,grid);
    }
    // Vice Sunset: Retro-Szene (gestreifte Sonne am Horizont, perspektivisches Bodengitter) über dem Raster.
    if(render::theme().style==fl::theme::Style::Vice && phase.reveal>.4f){
        const float k=(phase.reveal-.4f)/.6f,hy=H*.64f,r=H*.34f,cx=W*.5f;
        const Color sun=mixc(colors.bg,Color{243,217,164},.9f*k),stripe=mixc(colors.bg,colors.primary,.6f*k);
        for(float y=hy-r;y<hy;y+=1.f){const float dy=hy-y,half=std::sqrt(std::max(0.f,r*r-(r-dy)*(r-dy)));
            const bool band=int(dy/(r*.1f))%2==1&&dy<r*.65f;
            if(half>0)canvas.rectangle(cx-half,y,cx+half,y+1.f,band?stripe:sun);}
        const Color line=mixc(colors.bg,colors.secondary,.55f*k);
        canvas.rectangle(0,hy,float(W-1),hy+1.f,line);
        for(int i=1;i<=6;++i){const float y=hy+(H-hy)*std::pow(i/6.f,2.f);canvas.rectangle(0,y,float(W-1),y,line);}
        for(int i=-8;i<=8;++i)canvas.line({{cx,hy},{cx+i*W*.16f,float(H)}},line,1);
        const Color ink=mixc(colors.bg,Color{11,5,24},k);   // Comic-Umriss und Palmen-Silhouetten
        canvas.ellipse_outline(cx-r,hy-r*2,cx+r,hy,ink,std::max(2.f,3*unit));
        const auto palm=[&](float x,float sc){const float u=unit*sc,ty=hy-70*u;
            canvas.line({{x,hy},{x+4*u,hy-35*u},{x+2*u,ty}},ink,std::max(2.f,4*u));
            for(int d=-1;d<=1;d+=2){canvas.line({{x+2*u,ty},{x+2*u+d*22*u,ty+2*u},{x+2*u+d*36*u,ty+16*u}},ink,std::max(2.f,3*u));
                canvas.line({{x+2*u,ty},{x+2*u+d*14*u,ty-14*u},{x+2*u+d*26*u,ty-12*u}},ink,std::max(2.f,3*u));}};
        palm(W*.12f,1.2f);palm(W*.2f,.8f);palm(W*.88f,1.2f);palm(W*.8f,.8f);
    }    // Logo (zoom quantised to 8 px steps, resized only when the step changes).
    float logo_w=0,logo_h=0,logo_x=0,logo_y=0;
    if(logo && !logo->empty()) {
        const float fit=std::min(W*.62f/logo->w,H*.42f/logo->h)*phase.scale;
        const int sw=std::max(1,int(std::round(logo->w*fit/8))*8),sh=std::max(1,int(std::lround(double(sw)*logo->h/logo->w)));
        if(logo_src_!=logo.get() || logo_scaled_.w!=sw || logo_scaled_.h!=sh){logo_scaled_=resize(*logo,sw,sh);logo_src_=logo.get();}
        logo_w=float(sw);logo_h=float(sh);logo_x=(W-logo_w)/2;logo_y=(H-logo_h)/2-H*.06f;
        if(phase.shine>=0 && phase.shine<=1){
            // Shine stripe: only the columns inside the stripe are touched.
            Image shined=logo_scaled_;
            const int x0=std::max(0,int((phase.shine-.1f)*sw)),x1=std::min(sw,int((phase.shine+.1f)*sw)+1);
            for(int y=0;y<sh;++y)for(int x=x0;x<x1;++x){auto& px=shined.px[size_t(y)*sw+x];
                const float stripe=std::max(0.f,1-std::abs(float(x)/sw-phase.shine)*10)*.3f;const unsigned a=px>>24;unsigned r=(px>>16)&255,g=(px>>8)&255,b=px&255;
                r+=unsigned((a-r)*stripe);g+=unsigned((a-g)*stripe);b+=unsigned((a-b)*stripe);px=(a<<24)|(r<<16)|(g<<8)|b;}
            canvas.image(shined,logo_x,logo_y,phase.reveal);
        } else canvas.image(logo_scaled_,logo_x,logo_y,phase.reveal);
    }
    // Title block: label, game name, progress bar with ticks.
    const float title_y=logo_w>0?logo_y+logo_h+H*.07f:H*.5f;
    Font label{std::clamp(std::round(9*unit),10.f,26.f),true};
    canvas.text(W*.5f,title_y-H*.055f,"GAME MODE",label,mixc(colors.bg,colors.secondary,phase.reveal),"mm");
    Font f{float(std::clamp(H/15,12,40)),true};if(render::theme().style==fl::theme::Style::Vice){f.bold=false;f.family=L"Bangers";f.size+=6;}auto title=name.empty()?"n/a":name;
    while(title.size()>4 && canvas.textlength(title,f)>W*.85f) {auto points=to_u32(title);points.resize(points.size()-2);title=from_u32(points)+"…";}
    canvas.text(W*.5f,title_y,title,f,mixc(colors.bg,colors.text,phase.reveal),"mm");
    const float bar_w=W*.36f,bar_h=std::max(2.f,std::round(3*unit)),bar_x=(W-bar_w)/2,bar_y=title_y+H*.06f;
    const float loaded=smooth(elapsed/2.0);
    canvas.rectangle(bar_x,bar_y,bar_x+bar_w,bar_y+bar_h,mixc(colors.bg,colors.primary,.22f*phase.reveal));
    if(loaded>0)canvas.rectangle(bar_x,bar_y,bar_x+bar_w*loaded,bar_y+bar_h,mixc(colors.bg,colors.primary,phase.reveal));
    for(int i=0;i<=10;++i){const float tx=bar_x+bar_w*i/10.f,tl=i%5==0?bar_h*3:bar_h*1.6f;
        canvas.rectangle(tx,bar_y+bar_h+2,tx+std::max(1.f,unit*.6f),bar_y+bar_h+2+tl,mixc(colors.bg,colors.secondary,.8f*phase.reveal));}
    // Corner brackets converge from the screen corners onto the logo/title block.
    {
        const float k=smooth(elapsed/.75);
        const float pad=std::round(12*unit);
        const float bx0=std::min(logo_w>0?logo_x:bar_x,bar_x)-pad*2,bx1=W-bx0;
        const float by0=(logo_w>0?logo_y:title_y-H*.1f)-pad,by1=bar_y+bar_h*4+pad;
        const float x0=pad+(bx0-pad)*k,x1=W-pad+(bx1-(W-pad))*k,y0=pad+(by0-pad)*k,y1=H-pad+(by1-(H-pad))*k;
        const float arm=std::round(26*unit),th=std::max(2.f,std::round(2*unit));
        const Color c=colors.primary;
        canvas.rectangle(x0,y0,x0+arm,y0+th,c);canvas.rectangle(x0,y0,x0+th,y0+arm,c);
        canvas.rectangle(x1-arm,y0,x1,y0+th,c);canvas.rectangle(x1-th,y0,x1,y0+arm,c);
        canvas.rectangle(x0,y1-th,x0+arm,y1,c);canvas.rectangle(x0,y1-arm,x0+th,y1,c);
        canvas.rectangle(x1-arm,y1-th,x1,y1,c);canvas.rectangle(x1-th,y1-arm,x1,y1,c);
        // Horizontal scan beam sweeps the block while the logo shines.
        if(phase.shine>=0 && phase.shine<=1){
            const float sy=by0+(by1-by0)*smooth(phase.shine);
            canvas.rectangle(bx0,sy-th*2,bx1,sy-th*2,mixc(colors.bg,colors.secondary,.6f));
            canvas.rectangle(bx0,sy,bx1,sy+th-1,mixc(colors.primary,Color{255,255,255},.35f));
            canvas.rectangle(bx0,sy+th*2,bx1,sy+th*2,mixc(colors.bg,colors.secondary,.6f));
        }
    }
    cache_.to=canvas.finish();
    {
        const uint32_t bgpx=0xFF000000u|uint32_t(colors.bg.r)<<16|uint32_t(colors.bg.g)<<8|colors.bg.b;
        const bool has_prev=cache_.from.w==W && cache_.from.h==H;
        const uint32_t t=has_prev?uint32_t(std::lround(256*(1-.85f*phase.reveal))):0,u=256-t;
        const uint32_t bg_rb=(bgpx&0x00FF00FFu)*u,bg_ag=((bgpx>>8)&0x00FF00FFu)*u;
        uint32_t* o=cache_.to.px.data();const uint32_t* prev=has_prev?cache_.from.px.data():nullptr;
        for(size_t i=0,n=size_t(W)*H;i<n;++i){
            const uint32_t q=prev?prev[i]:bgpx;
            const uint32_t base=((((q&0x00FF00FFu)*t+bg_rb)>>8)&0x00FF00FFu)|((((q>>8)&0x00FF00FFu)*t+bg_ag)&0xFF00FF00u);
            const uint32_t v=o[i],a=v>>24;
            if(a==0)o[i]=base;
            else if(a<255){const uint32_t k=256-a-(a>>7);   // premultiplied "over"
                o[i]=v+(((((base&0x00FF00FFu)*k)>>8)&0x00FF00FFu)|((((base>>8)&0x00FF00FFu)*k)&0xFF00FF00u));}
        }
    }
    if(phase.fade<=0)return cache_.to;
    auto result=cache_.to;
    plugins::blend_rect(result,cache_.to,target,{0,0,float(W),float(H)},phase.fade);
    return result;
}
}
