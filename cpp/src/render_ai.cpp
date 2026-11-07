// © 2026 FuchsLabs — frame-driven, independent implementation.
#include "render.h"
#include "brand.h"
#include "i18n.h"
#include "util.h"
#include "layout_edit.h"
#include <algorithm>
#include <cmath>

namespace fl::render {
namespace {
Color blend(Color a,Color b,float t) {
    return {uint8_t(a.r+(b.r-a.r)*t),uint8_t(a.g+(b.g-a.g)*t),uint8_t(a.b+(b.b-a.b)*t)};
}
std::string value(const ai::Json& j,const char* k,int decimals=0) {
    if(!j.contains(k) || !j[k].is_number()) return "n/a";
    return format_num(j[k].get<double>(),decimals);
}
std::string duration(std::time_t n) {
    n=std::max<std::time_t>(0,n);
    if(n<60) return std::to_string(n)+" s";
    if(n<3600) return std::to_string(n/60)+" m "+std::to_string(n%60)+" s";
    return std::to_string(n/3600)+" h "+std::to_string(n/60%60)+" m";
}
std::string state_label(const std::string& s) {
    return tr(s=="working"?"Arbeitet":s=="waiting"?"Wartet":s=="idle"?"Inaktiv":"Aus");
}
Color state_color(const std::string& s,const Theme& t) {
    return s=="working"?t.orange:s=="waiting"?Color{245,184,66}:t.muted;
}
}
Image AiRenderer::render(const ai::Snapshot& snap,double tick,std::time_t now) {
    const auto& t=theme();Canvas d(W,H,s,t.bg);
    const auto small=font(10), normal=font(12), name=font(14,true);
    d.text(10,7,"KI-Status",font(18,true),t.text);
    const auto& cm=snap.providers.at("claude");
    d.text(w-10,12,tr("Claude Sitzung (ca.)")+": "+value(cm,"cost_session",2)+(cm["cost_session"].is_number()?" USD":""),small,t.muted,"ra");
    const float cw=(w-26)/2;
    for(int i=0;i<2;++i) {
        const std::string provider=i==0?"claude":"codex";
        const auto& m=snap.providers.at(provider);
        auto it=snap.representatives.find(provider);
        const auto* session=it==snap.representatives.end()?nullptr:&it->second;
        std::string state=session?session->state:"off";
        const float x=10+i*(cw+6), y=34;
        d.rounded_rectangle(x,y,x+cw,y+158,8,t.card);
        d.text(x+10,y+8,i==0?"Claude Code":"Codex",name,t.text);
        float pulse=float((std::sin(tick*(state=="working"?4.5:2.6))+1)*.5);
        Color col=state_color(state,t);
        if(state=="working" || state=="waiting") col=blend(t.card,col,.4f+.6f*pulse);
        float r=state=="working"?3+1.3f*pulse:3;
        d.ellipse(x+cw-15-r,y+16-r,x+cw-15+r,y+16+r,col);
        d.text(x+10,y+30,state_label(state),normal,state_color(state,t));
        d.text(x+cw-10,y+31,session?duration(now-session->since):"n/a",small,t.muted,"ra");
        d.text(x+10,y+48,tr("Tool")+": "+fit(d,session && !session->tool.empty()?session->tool:"n/a",small,cw-48),small,t.text);
        d.text(x+10,y+63,tr("Modell")+": "+fit(d,session && !session->model.empty()?session->model:"n/a",small,cw-58),small,t.muted);
        d.text(x+10,y+78,i==0?tr("Kontext")+": "+value(m,"context")+(m["context"].is_number()?" %":""):
                   tr("Tokens Sitzung")+": "+value(m,"tokens_session"),small,t.muted);
        for(int b=0;b<2;++b) {
            const char* key=b==0?"five_hour":"week";
            const char* reset=b==0?"five_hour_reset":"week_reset";
            const float by=y+98+b*27;
            d.text(x+10,by,tr(b==0?"5 Stunden":"Woche")+" "+value(m,key)+(m[key].is_number()?" %":""),small,t.text);
            std::string countdown=m[reset].is_number()?duration(m[reset].get<std::time_t>()-now):"n/a";
            d.text(x+cw-10,by,countdown,small,t.muted,"ra");
            d.rounded_rectangle(x+10,by+15,x+cw-10,by+19,2,t.line);
            if(m[key].is_number() && m[key].get<double>()>0)
                d.rounded_rectangle(x+10,by+15,x+10+(cw-20)*float(m[key].get<double>()/100),by+19,2,t.orange);
        }
    }
    d.text(10,197,"Sitzungen",small,t.muted);
    float dt=previous_>0?float(std::clamp(tick-previous_,0.0,1.0)):0;previous_=tick;
    const float easing=1-std::exp(-dt*12);
    for(auto& [id,row]:rows_) row.present=false;
    const size_t count=std::min<size_t>(3,snap.sessions.size());
    for(size_t i=0;i<count;++i) {
        const auto& ss=snap.sessions[i];auto id=ss.provider+":"+ss.id;
        float target=211+float(i)*22;
        auto [it,added]=rows_.try_emplace(id,Row{ss,target+12,0,true,tick});
        auto& row=it->second;
        if(row.session.state!=ss.state) {row.changed=tick;row.opacity=.35f;}
        row.ordinal=i+1;row.session=ss;row.present=true;row.y+=(target-row.y)*easing;
    }
    d.clip_push(8,209,w-8,h-43);
    for(auto it=rows_.begin();it!=rows_.end();) {
        auto& row=it->second;
        row.opacity+=((row.present?1.f:0.f)-row.opacity)*easing;
        if(!row.present) row.y-=dt*24;
        if(!row.present && row.opacity<.02f) {it=rows_.erase(it);continue;}
        const auto& ss=row.session;
        Color bg=blend(t.bg,t.card,row.opacity);
        d.rounded_rectangle(10,row.y,w-10,row.y+21,4,bg);
        const auto col=state_color(ss.state,t);
        float light=float(.65+.35*std::sin(tick*(ss.state=="working"?4.5:2.6)));
        d.ellipse(17,row.y+9,23,row.y+15,blend(bg,col,row.opacity*((ss.state=="working" || ss.state=="waiting")?light:1)));
        // Fading rows retain their last visible ordinal; identities remain internal.
        auto label=(ss.provider=="claude"?"Claude ":"Codex ")+std::to_string(row.ordinal)+" · "+state_label(ss.state);
        d.text(29,row.y+5,label,small,blend(bg,t.text,row.opacity));
        d.text(w-18,row.y+5,duration(now-ss.since),small,blend(bg,t.muted,row.opacity),"ra");
        ++it;
    }
    if(count==0 && rows_.empty()) d.text(14,222,"Keine lokalen Sitzungsdaten",normal,t.muted);
    d.clip_pop();
    for(int i=0;i<2;++i) {
        const auto& m=snap.providers.at(i==0?"claude":"codex");
        const auto src=m.value("source",std::string("none"));
        const auto reason=m.value("reason",std::string("no_data"));
        std::string age=m.contains("updated_at") && m["updated_at"].is_number()?duration(now-m["updated_at"].get<std::time_t>()):"n/a";
        std::string status=reason;
        if(m.contains("updated_at") && m["updated_at"].is_number() && now-m["updated_at"].get<std::time_t>()>900) status="stale";
        const float x=10+i*(cw+6);
        const std::string srcName=src=="none"?tr("Keine Quelle"):src=="claude-statusline"?tr("Statuszeile"):src=="hook"?tr("Hooks"):src=="codex-app-server"?"Codex app-server":src;
        d.text(x,h-33,fit(d,srcName+" · "+age,small,cw),small,t.muted);
        d.text(x,h-18,fit(d,status.empty()?tr("Daten nur lokal"):ai::usage_reason(status),small,cw),small,t.muted);
    }
    return d.finish();
}
void AiRenderer::draw_tile(Canvas& d,const plugins::zones::Rect& area,const std::string& source,const ai::Snapshot& snap,std::time_t now) {
    const auto& t=theme();
    const auto catalog=builtin::find("ai");
    const auto spec=std::find_if(catalog->blocks.begin(),catalog->blocks.end(),[&](const auto& b){return b.id==source;});
    if(spec==catalog->blocks.end()) return;
    const auto end=source.rfind('.');const auto provider=source.substr(3,end-3),field=source.substr(end+1);
    const auto& m=snap.providers.at(provider);std::string text="n/a";
    if(field=="state") {const auto it=snap.representatives.find(provider);if(it!=snap.representatives.end()) text=state_label(it->second.state);}
    else if(m.contains(field) && m[field].is_number()) {
        text=value(m,field.c_str(),field=="cost_session"?2:0);
        if(field=="five_hour" || field=="week") text+=" %";
        if(field=="cost_session") text+=" USD";
    }
    d.clip_push(area.x,area.y,area.x+area.width,area.y+area.height);
    d.rounded_rectangle(area.x,area.y,area.x+area.width,area.y+area.height,5,t.card);
    const auto label=i18n::lang()=="en"?spec->label.en:spec->label.de;
    if(area.height>=42) {
        d.text(area.x+7,area.y+4,fit(d,label,font(10),area.width-14),font(10),t.muted);
        d.text(area.x+7,area.y+20,fit(d,text,font(12,true),area.width-14),font(12,true),t.text);
    } else d.text(area.x+5,area.y+4,fit(d,label+": "+text,font(10),area.width-10),font(10),t.text);
    if(area.height>=72) {
        std::string provenance=m.value("source",std::string("none"));
        if(m.contains("updated_at") && m["updated_at"].is_number()) provenance+=" · "+duration(now-m["updated_at"].get<std::time_t>());
        d.text(area.x+7,area.y+44,fit(d,provenance,font(9),area.width-14),font(9),t.muted);
        d.text(area.x+7,area.y+58,fit(d,ai::usage_reason(m.value("reason",std::string("no_data"))),font(9),area.width-14),font(9),t.muted);
    }
    d.clip_pop();
}
Image AiRenderer::render_tile(const std::string& source,const ai::Snapshot& snap,std::time_t now) {
    Canvas d(W,H,s,theme().bg);draw_tile(d,{0,0,w,h},source,snap,now);return d.finish();
}
BuiltinDrawOut AiRenderer::render_arranged(const ai::Snapshot& snap,const builtin::Overlay& overlay,double tick,std::time_t now) {
    const auto& c=*builtin::find("ai");
    if(!builtin::validate(c,overlay).empty()) return {render(snap,tick,now),{}};
    const bool portrait=H>W;const auto& model=portrait?overlay.portrait.value_or(overlay.model):overlay.model;
    const auto layout=plugins::edit::geometry(model);
    if(!layout) return {render(snap,tick,now),{}};
    const auto plan=plugins::zones::plan({8,36,w-16,h-54},*layout,builtin::inputs(c,model),6);
    Canvas d(W,H,s,theme().bg);header(d,now,"KI-Status");
    for(const auto& block:plan.blocks) if(block.visible()) draw_tile(d,block.area,block.id,snap,now);
    footer(d,tr("Daten nur lokal"));return {d.finish(),{}};
}
} // namespace fl::render
