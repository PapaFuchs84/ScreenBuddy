// © 2026 FuchsLabs – Raster mit Maus, Tastatur und MSAA. Clean-Room.
#include "ui_layout_grid.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <oleacc.h>

namespace fl::ui {
using namespace plugins::edit;
namespace {
bool inside(Rect r, Point p) { return p.x>=r.left && p.x<r.right && p.y>=r.top && p.y<r.bottom; }
}
GridEditor::GridEditor() { track_dirty = false; name = T(L"Anordnungsraster", L"Arrangement grid"); }
void GridEditor::children(std::vector<Widget*>& out) {
    palette_.owner=this; palette_.track_dirty=false;
    palette_.name=T(L"Bausteinleiste",L"Block palette");
    palette_.rc={rc.left,rc.top,rc.right,rc.top+palette_height()}; out.push_back(&palette_);
}
std::wstring GridEditor::PaletteInfo::value(bool en) const {
    std::wstring result;
    if(owner) for(const auto& b:owner->model.blocks) {
        if(!result.empty()) result+=L", ";
        result+=widen(b["id"].get<std::string>());
        result+=b.value("shown",true)?(en?L" shown":L" gezeigt"):(en?L" hidden":L" verborgen");
    }
    return result;
}
int GridEditor::palette_columns() const { return std::max(1,int((rc.right-rc.left)/144)); }
float GridEditor::palette_height() const { return 26.f+32.f*float((model.blocks.size()+palette_columns()-1)/palette_columns()); }
float GridEditor::height(Ctx&, float w) {
    auto l=geometry(model); int cols=std::max(1,int(w/144));
    return l ? 26.f+32.f*float((model.blocks.size()+cols-1)/cols)+
        (display_aspect>0?w/display_aspect:std::max(120.f,w/float(l->columns.size()))*float(l->rows.size()))+44.f : 64.f;
}
Rect GridEditor::grid_rect() const { return {rc.left,rc.top+palette_height(),rc.right,std::max(rc.top+palette_height()+32,rc.bottom-44)}; }
Rect GridEditor::palette_rect(size_t b) const {
    int cols=palette_columns(); float w=(rc.right-rc.left)/cols;
    return {rc.left+float(b%cols)*w+2,rc.top+26+float(b/cols)*32,rc.left+float(b%cols+1)*w-2,rc.top+54+float(b/cols)*32};
}
Rect GridEditor::trash_rect() const { return {rc.left,std::max(grid_rect().bottom+4,rc.bottom-36),rc.right,rc.bottom}; }
Rect GridEditor::zone_rect(size_t i) const {
    auto l=geometry(model); if(!l || i>=l->zones.size()) return {};
    auto g=grid_rect(); const auto& z=l->zones[i];
    float cw=std::max(32.f,(g.right-g.left)/float(l->columns.size())), ch=(g.bottom-g.top)/float(l->rows.size());
    return {g.left+(z.column-first_column)*cw+3,g.top+z.row*ch+3,g.left+(z.column+z.column_span-first_column)*cw-3,g.top+(z.row+z.row_span)*ch-3};
}
Rect GridEditor::block_rect(size_t b) const {
    auto l=geometry(model); if(!l || b>=model.blocks.size() || !model.blocks[b].value("shown",true)) return {};
    for(size_t z=0;z<l->zones.size();++z) if(model.blocks[b]["zone"]==l->zones[z].id) {
        auto r=zone_rect(z); int count=0, index=0;
        for(size_t i=0;i<model.blocks.size();++i) if(model.blocks[i]["zone"]==l->zones[z].id && model.blocks[i].value("shown",true)) { if(i<b) ++index; ++count; }
        float h=std::min(28.f,std::max(1.f,(r.bottom-r.top-28)/std::max(1,count)));
        return {r.left+6,r.top+26+index*h,r.right-6,r.top+26+(index+1)*h-2};
    }
    return {};
}
int GridEditor::zone_at() const {
    auto l=geometry(model); if(!l) return -1;
    for(size_t i=0;i<l->zones.size();++i) {
        const auto& z=l->zones[i];
        if(column>=z.column && column<z.column+z.column_span && row>=z.row && row<z.row+z.row_span) return int(i);
    }
    return -1;
}
void GridEditor::cell(Point p) {
    auto l=geometry(model); if(!l) return; auto g=grid_rect();
    column=std::clamp(first_column+int(std::floor((p.x-g.left)/std::max(32.f,(g.right-g.left)/l->columns.size()))),0,int(l->columns.size())-1);
    row=std::clamp(int(std::floor((p.y-g.top)/((g.bottom-g.top)/l->rows.size()))),0,int(l->rows.size())-1);
}
int GridEditor::edges(Point p,int zone) const {
    if(zone<0) return 0; auto r=zone_rect(zone);
    if(p.x<r.left-4 || p.x>r.right+4 || p.y<r.top-4 || p.y>r.bottom+4) return 0;
    int e=0;
    if(std::abs(p.x-r.left)<=5) e|=1; else if(std::abs(p.x-r.right)<=5) e|=2;
    if(std::abs(p.y-r.top)<=5) e|=4; else if(std::abs(p.y-r.bottom)<=5) e|=8;
    return e;
}
bool GridEditor::candidate(plugins::Arrangement& next) const {
    if(!active_) return false;
    if(trash_) return action_==Action::Block ? remove_block(next,source_) && !leaves_empty_zone(model,next) : action_==Action::Zone && delete_zone(next,source_);
    if(column<0 || row<0) return false;
    if(action_==Action::Block) {
        int z=zone_at();
        // Ein Ziel, das eine Zone ohne gezeigten Block hinterlässt, passt nicht (jede Zone braucht einen Block).
        return (z>=0 ? move_block(next,source_,z,stack_) : place_block(next,source_,column,row)) && !leaves_empty_zone(model,next);
    }
    if(action_==Action::Zone) return move_zone(next,source_,column-start_column_,row-start_row_);
    if(action_==Action::Resize) {
        int dx=column-start_column_,dy=row-start_row_;
        return change(next,[&](auto& n) {
            auto& z=n.layout["zones"].at(source_);
            if(edges_&1) z["column"]=z["column"].template get<int>()+dx;
            if(edges_&4) z["row"]=z["row"].template get<int>()+dy;
            z["column_span"]=z.value("column_span",1)+((edges_&2)?dx:0)-((edges_&1)?dx:0);
            z["row_span"]=z.value("row_span",1)+((edges_&8)?dy:0)-((edges_&4)?dy:0);
        });
    }
    return false;
}
bool GridEditor::target_valid() const { return probe(model,[this](auto& n) { return candidate(n); }); }
void GridEditor::reset_drag() { down_=active_=keyboard_=trash_=false; action_=Action::None; }
// Abbruch: Eine per Maus begonnene Geste hat die Auswahl schon beim Drücken verändert; sie kehrt zurück.
void GridEditor::cancel_drag() {
    if(down_ && !keyboard_) {
        auto l=geometry(model); int n=l?int(l->zones.size()):0;
        selected=std::clamp(prev_selected_,-1,n-1);
    }
    reset_drag();
}
std::vector<int> GridEditor::chips(int zone) const {
    std::vector<int> out; auto l=geometry(model);
    if(!l || zone<0 || zone>=int(l->zones.size())) return out;
    for(size_t b=0;b<model.blocks.size();++b)
        if(model.blocks[b]["zone"]==l->zones[zone].id && model.blocks[b].value("shown",true)) out.push_back(int(b));
    return out;
}
int GridEditor::chip_zone() const {
    int z=zone_at(); if(z<0 || focused_block<0) return -1;
    auto c=chips(z); return std::find(c.begin(),c.end(),focused_block)!=c.end() ? z : -1;
}
// Pfeil auf einem Block-Chip: -1 = vorheriger, +1 = nächster; vor dem ersten Chip verlässt der Fokus die Chips.
bool GridEditor::step_chip(int direction) {
    int z=zone_at(); auto c=chips(z); if(c.empty()) return false;
    auto it=std::find(c.begin(),c.end(),focused_block); int at=it==c.end()?-1:int(it-c.begin());
    if(at<0) at=direction>0?0:int(c.size())-1;
    else at+=direction;
    if(at<0) { focused_block=-1; return true; }
    focused_block=c[std::min(at,int(c.size())-1)]; return true;
}
GridEditor::Snap GridEditor::snap() const {
    return {column,row,selected,focused_block,area,palette_block,(long long)stack_,active_,trash_,(long long)action_,first_column};
}
void GridEditor::finish() {
    if(column<0 && !trash_) { cancel_drag(); return; } // Loslassen außerhalb des Rasters: stiller Abbruch, kein Toast
    auto next=model; bool ok=candidate(next);
    const auto action=action_; const auto source=source_; const bool trash=trash_, mouse=!keyboard_;
    std::string id; if(action==Action::Block && source<model.blocks.size()) id=model.blocks[source].value("id","");
    reset_drag();
    if(ok && !equal(model,next)) {
        // Auswahl folgt dem bewegten Objekt, nicht der Zelle unter der Maus.
        auto g=geometry(next); int zones=g?int(g->zones.size()):0, sel=selected;
        if(action==Action::Zone || action==Action::Resize) sel=trash?0:int(source);
        if(action==Action::Block) {
            focused_block=-1;
            for(size_t b=0;b<next.blocks.size();++b) if(next.blocks[b].value("id","")==id) {
                if(!trash && g) for(size_t z=0;z<g->zones.size();++z) if(next.blocks[b]["zone"]==g->zones[z].id) { sel=int(z); focused_block=int(b); }
                break;
            }
        }
        selected=std::clamp(sel,-1,zones-1); model=next; dirty_=true;
        if(on_edit) on_edit(std::move(next),{});
    } else if(!ok) {
        if(mouse) { auto g=geometry(model); selected=std::clamp(prev_selected_,-1,g?int(g->zones.size())-1:-1); }
        if(on_reject) on_reject();
    }
}
void GridEditor::pointer(Kind kind,Point p) {
    if(kind==Kind::Cancel) { cancel_drag(); return; }
    auto l=geometry(model); if(!l) return;
    if(kind==Kind::Down) {
        reset_drag(); origin_=p; down_=true; stack_=0; prev_selected_=selected; moved_=false;
        for(size_t b=0;b<model.blocks.size();++b) if(inside(palette_rect(b),p)) {
            area=0; palette_block=int(b); source_=b; action_=Action::Block; return;
        }
        if(!inside(grid_rect(),p)) { area=2; return; }
        area=1; cell(p); start_column_=column; start_row_=row;
        edges_=edges(p,selected);
        if(edges_) { source_=selected; action_=Action::Resize; return; }
        selected=zone_at(); focused_block=-1;
        for(size_t b=0;b<model.blocks.size();++b) if(inside(block_rect(b),p)) {
            source_=b; focused_block=int(b); action_=Action::Block; return;
        }
        if(selected>=0) { source_=selected; action_=Action::Zone; }
        return;
    }
    if(!down_ || keyboard_) return;
    if(kind==Kind::Move || kind==Kind::Up) {
        const bool beyond=std::abs(p.x-origin_.x)>=threshold_x_ || std::abs(p.y-origin_.y)>=threshold_y_;
        if(beyond) moved_=true;
        if(!active_ && action_!=Action::None && beyond) active_=true;
        if(active_) {
            cell(p); trash_=inside(trash_rect(),p); stack_=0;
            int z=zone_at();
            if(z>=0 && action_==Action::Block) {
                for(size_t b=0;b<model.blocks.size();++b) if(b!=source_ && model.blocks[b]["zone"]==l->zones[z].id && model.blocks[b].value("shown",true)) {
                    auto r=block_rect(b); if(p.y>(r.top+r.bottom)/2) ++stack_;
                }
            }
            // Einrastziel außerhalb von Raster/Ablage ist ausdrücklich ungültig.
            if(!trash_ && !inside(grid_rect(),p)) { column=-1; row=-1; }
        }
        if(kind==Kind::Up) {
            if(active_) finish();
            else {
                const bool empty_drag=moved_ && action_==Action::None;
                down_=false; action_=Action::None;
                // Ziehen von einer freien Zelle legt nichts an (weder an der Druck- noch an der Loslasszelle).
                if(area==1 && empty_drag) selected=std::clamp(prev_selected_,-1,int(l->zones.size())-1);
                else if(area==1) { selected=zone_at(); if(selected>=0) { if(on_select) on_select(); } else input(VK_RETURN); }
            }
        }
    }
}
LPCWSTR GridEditor::cursor(Point p) const {
    if(active_ && !target_valid()) return IDC_NO;
    if(active_ && action_!=Action::Resize) return IDC_SIZEALL;
    int e=active_ && action_==Action::Resize ? edges_ : edges(p,selected);
    if((e&3) && (e&12)) return ((e&1 && e&4) || (e&2 && e&8)) ? IDC_SIZENWSE : IDC_SIZENESW;
    if(e&3) return IDC_SIZEWE; if(e&12) return IDC_SIZENS;
    return inside(grid_rect(),p) ? IDC_SIZEALL : IDC_HAND;
}
void GridEditor::paint(Ctx& c) {
    auto l=geometry(model); if(!l) return; auto g=grid_rect();
    const float cw=std::max(32.f,(g.right-g.left)/float(l->columns.size()));
    const float ch=(g.bottom-g.top)/float(l->rows.size());
    const int visible=std::max(1,int((g.right-g.left)/cw));
    if(column>=0) { if(column<first_column) first_column=column; if(column>=first_column+visible) first_column=column-visible+1; }
    first_column=std::clamp(first_column,0,std::max(0,int(l->columns.size())-visible));
    const float left=g.left-first_column*cw;
    c.rt->PushAxisAlignedClip(rc,D2D1_ANTIALIAS_MODE_ALIASED);
    c.text(c.kit.english?L"Blocks · Space: pick up":L"Bausteine · Leertaste: aufnehmen",{rc.left,rc.top,rc.right,rc.top+24},Font::Body,c.th.text);
    for(size_t b=0;b<model.blocks.size();++b) {
        auto r=palette_rect(b); c.fill(r,c.th.card); c.frame(r,c.th.text,2,area==0 && palette_block==int(b)?2.f:1.f);
        c.text(widen(model.blocks[b]["id"].get<std::string>()),{r.left+5,r.top,r.right-5,r.bottom},Font::Body,c.th.text);
    }
    c.rt->PushAxisAlignedClip(g,D2D1_ANTIALIAS_MODE_ALIASED);
    for(size_t y=0;y<l->rows.size();++y) for(size_t x=0;x<l->columns.size();++x)
        c.frame({left+x*cw,g.top+y*ch,left+(x+1)*cw,g.top+(y+1)*ch},c.th.text);
    for(size_t i=0;i<l->zones.size();++i) {
        auto r=zone_rect(i); c.fill(r,c.th.card); c.frame(r,c.th.text,2,int(i)==selected?2.f:1.f);
        c.text(widen(l->zones[i].id),{r.left+6,r.top,r.right-6,r.top+24},Font::Body,c.th.text);
        if(int(i)==selected) for(float x:{r.left,(r.left+r.right)/2,r.right}) for(float y:{r.top,(r.top+r.bottom)/2,r.bottom}) {
            if(x==(r.left+r.right)/2 && y==(r.top+r.bottom)/2) continue;
            c.fill({x-4,y-4,x+4,y+4},c.th.text);
        }
    }
    for(size_t b=0;b<model.blocks.size();++b) {
        auto r=block_rect(b); if(r.right<=r.left) continue;
        c.fill(r,c.th.card); if(int(b)==focused_block) c.frame(r,c.th.text,2,3.f); else c.frame(r,c.th.text,2);
        c.text(widen(model.blocks[b]["id"].get<std::string>()),{r.left+3,r.top,r.right-3,r.bottom},Font::Body,c.th.text);
    }
    if(column>=0 && row>=0) c.frame({left+column*cw+5,g.top+row*ch+5,left+(column+1)*cw-5,g.top+(row+1)*ch-5},c.th.text,0,2);
    if(active_ && !trash_) {
        auto copy=model; bool valid=candidate(copy);
        int x=std::clamp(column,0,int(l->columns.size())-1),y=std::clamp(row,0,int(l->rows.size())-1);
        Rect r{left+x*cw+3,g.top+y*ch+3,left+(x+1)*cw-3,g.top+(y+1)*ch-3};
        if(valid && action_!=Action::Block && source_<copy.layout["zones"].size()) {
            auto z=geometry(copy)->zones[source_]; r={left+z.column*cw+3,g.top+z.row*ch+3,left+(z.column+z.column_span)*cw-3,g.top+(z.row+z.row_span)*ch-3};
        }
        auto fill=c.th.card; fill.a=.75f; c.fill(r,fill);
        c.brush->SetColor(c.th.text); c.rt->DrawRectangle(r,c.brush,2,valid?nullptr:c.kit.dashed());
        c.text(valid?(c.kit.english?L"Drop":L"Ablegen"):(c.kit.english?L"does not fit":L"passt nicht"),r,Font::Body,c.th.text);
    }
    c.rt->PopAxisAlignedClip();
    auto trash=trash_rect(); c.fill(trash,c.th.card); c.frame(trash,c.th.text,2,area==2 || trash_?3.f:1.f);
    c.text(c.kit.english?L"Trash · Delete: return blocks to palette":L"Ablage · Entf: Bausteine zurück in die Leiste",{trash.left+6,trash.top,trash.right-6,trash.bottom},Font::Body,c.th.text);
    c.rt->PopAxisAlignedClip();
}
std::wstring GridEditor::value(bool en) const {
    auto l=geometry(model); if(!l) return en?L"Invalid grid":L"Ungültiges Raster";
    if(active_) {
        std::wstring source=action_==Action::Block ? (en?L"Block ":L"Block ")+widen(model.blocks.at(source_)["id"].get<std::string>()) : L"Zone "+widen(l->zones.at(source_).id);
        int z=zone_at();
        return source+(en?L" picked up, target ":L" aufgenommen, Ziel ")+(trash_?(en?L"trash":L"Ablage"):z>=0?L"Zone "+widen(l->zones[z].id):(en?L"free cell ":L"freie Zelle ")+std::to_wstring(column+1)+L", "+std::to_wstring(row+1))+
            (action_==Action::Block?(en?L", stack position ":L", Stapelposition ")+std::to_wstring(stack_+1):L"")+
            (target_valid()?(en?L", valid":L", gültig"):(en?L", does not fit":L", passt nicht"));
    }
    if(area==0 && palette_block>=0 && palette_block<int(model.blocks.size())) return (en?L"Palette, block ":L"Bausteinleiste, Block ")+widen(model.blocks[palette_block]["id"].get<std::string>())+(en?L", Space to pick up":L", Leertaste zum Aufnehmen");
    if(area==2) return en?L"Trash, Enter to drop":L"Ablage, Enter zum Ablegen";
    if(selected<0 || selected>=int(l->zones.size())) return (en?L"Free cell, column ":L"Freie Zelle, Spalte ")+std::to_wstring(column+1)+(en?L", row ":L", Zeile ")+std::to_wstring(row+1);
    const auto& z=l->zones[selected]; int blocks=0;
    for(auto& b:model.blocks) if(b["zone"]==z.id && b.value("shown",true)) ++blocks;
    auto range=[](int p,int n) { return std::to_wstring(p+1)+(n>1?L"–"+std::to_wstring(p+n):L""); };
    auto v=L"Zone "+widen(z.id)+(en?L", column ":L", Spalte ")+range(z.column,z.column_span)+(en?L", row ":L", Zeile ")+range(z.row,z.row_span)+L", "+
        (z.cycle?widen(z.cycle->mode):(en?L"no cycle":L"kein Wechsel"))+L", "+std::to_wstring(blocks)+(en?L" blocks":L" Blöcke");
    if(column!=z.column || row!=z.row) v+=(en?L", cell focus column ":L", Zellfokus Spalte ")+std::to_wstring(column+1)+(en?L", row ":L", Zeile ")+std::to_wstring(row+1);
    if(chip_zone()>=0) v+=(en?L", block chip ":L", Block-Chip ")+widen(model.blocks[focused_block]["id"].get<std::string>())+
        (en?L" selected, Space to pick up, Delete to return to palette":L" gewählt, Leertaste zum Aufnehmen, Entf zurück in die Leiste");
    return v;
}
bool GridEditor::input(UINT vk,bool shift,bool alt,bool control) {
    if(vk==VK_ESCAPE) { const bool busy=down_ || active_; cancel_drag(); return busy; } // ohne Geste: Esc gehört dem Aufrufer
    if(control && (vk=='Z' || vk=='Y')) { cancel_drag(); auto f=vk=='Y' || shift?on_redo:on_undo; if(f) f(); return true; }
    auto l=geometry(model); if(!l) return false;
    if(vk==VK_F6 || (control && vk==VK_TAB)) { area=(area+(shift?2:1))%3; if(active_) trash_=area==2; return true; }
    column=std::clamp(column,0,int(l->columns.size())-1); row=std::clamp(row,0,int(l->rows.size())-1);
    int dx=vk==VK_LEFT?-1:vk==VK_RIGHT?1:0,dy=vk==VK_UP?-1:vk==VK_DOWN?1:0;
    if(active_) {
        if(vk==VK_RETURN) { finish(); return true; }
        if(control && dy && action_==Action::Block) {
            int count=0, z=zone_at();
            if(z>=0) for(size_t b=0;b<model.blocks.size();++b)
                if(b!=source_ && model.blocks[b]["zone"]==l->zones[z].id && model.blocks[b].value("shown",true)) ++count;
            stack_=size_t(std::clamp(int(stack_)+dy,0,count)); return true;
        }
        if(dx || dy) { column=std::clamp(column+dx,0,int(l->columns.size())-1); row=std::clamp(row+dy,0,int(l->rows.size())-1); stack_=0; return true; }
        return true;
    }
    if(area==0) {
        const int last=std::max(0,int(model.blocks.size())-1);
        palette_block=std::clamp(palette_block,0,last); // leeres/geschrumpftes Inventar: kein ungültiger Index
        if(dx || dy) { palette_block=std::clamp(palette_block+dx+dy*palette_columns(),0,last); return true; }
        if(model.blocks.empty()) return false;
        if(vk==VK_SPACE) { action_=Action::Block; source_=palette_block; active_=keyboard_=true; area=1; stack_=0; return true; }
        if(vk==VK_DELETE) {
            auto next=model;
            if(remove_block(next,palette_block) && !leaves_empty_zone(model,next)) { model=next; dirty_=true; if(on_edit) on_edit(next,{}); } else if(on_reject) on_reject();
            return true;
        }
        return false;
    }
    if(area==2) return false;
    // Block-Chips im Raster: Strg+Pfeil hoch/runter betritt die Chips der Zone unter dem Zellfokus,
    // Pfeil hoch/runter wählt weiter, Leertaste nimmt auf, Entf gibt den Block an die Leiste zurück.
    if(chip_zone()<0) focused_block=-1;
    if(focused_block<0 && control && !shift && !alt && dy && step_chip(dy)) return true;
    if(focused_block>=0) {
        if(!control && !shift && !alt && dy) { step_chip(dy); return true; }
        if(vk==VK_SPACE) {
            action_=Action::Block; source_=size_t(focused_block); start_column_=column; start_row_=row;
            active_=keyboard_=true; stack_=0; return true;
        }
        if(vk==VK_DELETE) {
            auto next=model;
            if(remove_block(next,size_t(focused_block)) && !leaves_empty_zone(model,next)) { model=next; focused_block=-1; dirty_=true; if(on_edit) on_edit(next,{}); }
            else if(on_reject) on_reject();
            return true;
        }
        if(dx) focused_block=-1; // Pfeil seitwärts verlässt die Chips und bewegt den Zellfokus
    }
    auto next=model; bool ok=false; std::string group;
    if(dx || dy) {
        if(!shift && !alt) { column=std::clamp(column+dx,0,int(l->columns.size())-1); row=std::clamp(row+dy,0,int(l->rows.size())-1); return true; }
        group=(alt?"move:":"size:")+std::to_string(selected);
        ok=alt?move_zone(next,selected,dx,dy):resize_zone(next,selected,dx,dy);
    } else if(vk==VK_RETURN || vk==VK_SPACE) {
        selected=zone_at();
        if(selected>=0) {
            if(vk==VK_SPACE) { action_=Action::Zone; source_=selected; start_column_=column; start_row_=row; active_=keyboard_=true; }
            if(on_select) on_select(); return true;
        }
        ok=add_zone(next,column,row); if(ok) selected=int(next.layout["zones"].size())-1;
    } else if(vk==VK_DELETE) { ok=delete_zone(next,selected); if(ok) selected=0; }
    else return false;
    if(ok) { model=next; dirty_=true; if(on_edit) on_edit(std::move(next),group); } else if(on_reject) on_reject();
    return true;
}
// MSAA: Zielwechsel, Auswahl- oder Modelländerung melden, nicht jede Mausbewegung innerhalb derselben Zelle.
void GridEditor::announce(Surface& s,const Snap& before,bool was_active) {
    if(was_active!=active_) s.notify(this,active_?EVENT_SYSTEM_DRAGDROPSTART:EVENT_SYSTEM_DRAGDROPEND);
    if(dirty_ || snap()!=before) { ++value_events; s.notify(this,EVENT_OBJECT_VALUECHANGE); }
    dirty_=false; s.invalidate();
}
bool GridEditor::key(UINT vk,Surface& s) {
    bool was=active_; auto before=snap(); if(!input(vk,GetKeyState(VK_SHIFT)<0,GetKeyState(VK_MENU)<0,GetKeyState(VK_CONTROL)<0)) return false;
    announce(s,before,was); return true;
}
void GridEditor::activate(Surface& s) { key(VK_RETURN,s); }
void GridEditor::press(Point p,Surface& s) {
    threshold_x_=GetSystemMetrics(SM_CXDRAG)*96.f/s.dpi(); threshold_y_=GetSystemMetrics(SM_CYDRAG)*96.f/s.dpi();
    bool was=active_; auto before=snap(); pointer(Kind::Down,p); announce(s,before,was);
}
void GridEditor::drag(Point p,Surface& s) {
    bool was=active_; auto before=snap();
    auto now=GetTickCount64(); auto g=grid_rect();
    if(active_ && now-scroll_time_>=150) {
        auto l=geometry(model); float cw=l?std::max(32.f,(g.right-g.left)/float(l->columns.size())):32;
        int visible=std::max(1,int((g.right-g.left)/cw));
        if(l && inside(g,p)) {
            if(p.x<g.left+12) first_column=std::max(0,first_column-1);
            if(p.x>g.right-12) first_column=std::min(std::max(0,int(l->columns.size())-visible),first_column+1);
        }
        float previous=s.scroll(),client_y=p.y-previous;
        if(client_y<20) s.scroll_to(s.scroll()-32);
        else if(client_y>s.size().height-20) s.scroll_to(s.scroll()+32);
        p.y+=s.scroll()-previous;
        scroll_time_=now;
    }
    pointer(Kind::Move,p); announce(s,before,was);
}
void GridEditor::release(Point p,Surface& s) { bool was=active_; auto before=snap(); pointer(Kind::Up,p); announce(s,before,was); }
void GridEditor::cancel(Surface& s) { bool was=active_; auto before=snap(); pointer(Kind::Cancel,{}); announce(s,before,was); }
bool LayoutBlockList::key(UINT vk, Surface& s) {
    if (GetKeyState(VK_CONTROL)<0 && (vk=='Z' || vk=='Y')) {
        auto f = vk=='Y' || GetKeyState(VK_SHIFT)<0 ? on_redo : on_undo; if(f) f(); return true;
    }
    history_group=(vk==VK_UP || vk==VK_DOWN) && (GetKeyState(VK_MENU)<0 || GetKeyState(VK_CONTROL)<0) && selected>=0 && selected<int(items.size())
        ? "block:"+items[selected].key : "";
    bool handled=ItemList::key(vk,s); history_group.clear(); return handled;
}
void LayoutBlockList::paint(Ctx& c) {
    // Kontrastreiche Auswahl und Haken nur im Editor; die globale Listenpalette bleibt unverändert.
    auto th=c.th; th.accent=th.text; th.on_accent=th.card; th.muted=th.text;
    Ctx local{c.rt,c.kit,th,c.brush,c.device,c.dim}; ItemList::paint(local);
}
}
