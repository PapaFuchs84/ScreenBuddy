// P5-0: feste Eingaben, keine Quellen/Server/Audio/Registry. Bitgleich, keine Toleranz.
#include "snapshot_support.h"
#include "page_samples.h"
#include "i18n.h"
#include "layout_edit.h"
#include "touch.h"
#include "plugin_render.h"
#include <windows.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace fl;
using namespace fl::render;
using namespace fl::snapshot;
namespace fs = std::filesystem;
namespace {
constexpr auto clock_value = samples::CLOCK;
constexpr double progress_time = 1000;
struct Size { const char* name; int w, h; };
std::vector<Size> sizes() {
    auto tile = tile_layout(1920, 720);
    return {{"landscape",480,320}, {"portrait",320,480}, {"small",240,160}, {"small-portrait",160,240},
            {"double",960,640}, {"tile",tile.at(0).w,tile.at(0).h}, {"safe-top-body",480,300},
            {"phone",1080,2400}, {"phone-wide",2400,1080}};
}
void require_fonts() {
    using Microsoft::WRL::ComPtr;
    ComPtr<IDWriteFactory> factory;
    ComPtr<IDWriteFontCollection> fonts;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                  reinterpret_cast<IUnknown**>(factory.GetAddressOf()))) ||
        FAILED(factory->GetSystemFontCollection(&fonts, TRUE)))
        throw std::runtime_error("DirectWrite-Systemschriften konnten nicht gelesen werden; kein Vergleich.");
    for (auto name : {L"Roboto", L"Segoe UI Variable"}) {
        UINT32 index = 0; BOOL exists = FALSE;
        if (FAILED(fonts->FindFamilyName(name, &index, &exists)) || !exists) {
            if (std::wstring(name) == L"Roboto")
                throw std::runtime_error("Schrift fehlt: Roboto. Installieren; keine Baseline erzeugt oder verglichen.");
            // Windows 11 registriert nur "Segoe UI Variable Text/Display/Small"; die App fragt "Segoe UI Variable" an,
            // DirectWrite loest das rechnerabhaengig (aber deterministisch) auf. Baseline ist rechnergebunden.
            UINT32 i2 = 0; BOOL ex2 = FALSE;
            if (FAILED(fonts->FindFamilyName(L"Segoe UI Variable Text", &i2, &ex2)) || !ex2)
                throw std::runtime_error("Schrift fehlt: Segoe UI Variable. Installieren; keine Baseline erzeugt oder verglichen.");
            std::fprintf(stderr, "HINWEIS: Familie 'Segoe UI Variable' nicht direkt registriert; Rechner-Fallback der App wird verwendet (rechnergebundene Baseline).\n");
        }
    }
}
enum class Hit { None, Rows, Rss, Stream, System, Spotify, Ha };
struct Case {
    std::string name;
    std::function<Image(int,int)> draw;
    Hit hit = Hit::None;
    size_t count = 0;
};
// Hash der unveränderten statischen Trefferfunktionen, einschließlich Listenmetadaten.
std::string hit_hash(const Case& c, int w, int h) {
    Image map((w+3)/4, (h+3)/4 + 1);
    std::fill(map.px.begin(), map.px.end(), 0);
    Renderer list(w,h,"");
    map.px[0] = list.rows_per_page(); map.px[1] = list.pages(c.count);
    for (int y=0; y<h; y+=4) for (int x=0; x<w; x+=4) {
        int value = -1;
        switch (c.hit) {
        case Hit::Rows: value = Renderer::row_at(w,h,y); break;
        case Hit::Rss: value = RssRenderer::row_at(w,h,y); break;
        case Hit::Stream: value = StreamRenderer::row_at(w,h,y); break;
        case Hit::System: value = SysRenderer::zone_at(w,h,x,y); break;
        case Hit::Spotify: value = SpotifyRenderer::button_at(w,h,x,y); break;
        case Hit::Ha: value = HaRenderer::cell_at(w,h,c.count,x,y); break;
        default: break;
        }
        map.px[size_t(y/4+1)*map.w + x/4] = static_cast<uint32_t>(value);
    }
    return hash_image(map);
}
std::vector<Case> cases(const std::string& lang) {
    std::vector<Case> out;
    auto solid = std::make_shared<const Image>(Canvas(64,64,1.f,{30,120,90}).finish());
    auto posts = samples::posts(lang);
    for (auto state : {"data","loading","empty"}) {
        std::string st=state;
        out.push_back({"rss."+st,[=](int w,int h) {
            return RssRenderer(w,h,"Blog-Updates").render(st=="empty" ? std::vector<Post>{} : posts,
                st=="loading" ? std::nullopt : std::optional<std::time_t>(clock_value),clock_value);
        },Hit::Rss,st=="data" ? posts.size() : 0});
    }
    auto logos = samples::stream_logos();
    for (auto state : {"logos","abbreviations","overflow","nokey","loading","error","empty"}) {
        std::string st=state; auto items=samples::stream();
        if (st=="overflow") { items[0].providers={"NF","PV","D+","ATV+","WOW","RTL+"}; items[0].provider_ids={8,9,337,350,30,2750}; }
        StreamState status= st=="nokey" ? StreamState::NoKey : st=="loading" ? StreamState::Loading :
                            st=="error" ? StreamState::Error : StreamState::Ok;
        if (st=="empty" || status!=StreamState::Ok) items.clear();
        out.push_back({"streaming."+st,[=](int w,int h) {
            return StreamRenderer(w,h,"Neu bei Streaming").render(status,items,
                status==StreamState::Ok ? std::optional<std::time_t>(clock_value) : std::nullopt,
                clock_value,st=="abbreviations" ? nullptr : &logos);
        },Hit::Stream,items.size()});
    }
    for (auto state : {"page1","page2","error","loading","empty","rank-no-icon","rank-icon"}) {
        std::string st=state; auto players=samples::players();
        // Genug Einträge für eine zweite Seite in jeder logischen Größe.
        if (st=="page1" || st=="page2") {
            for (int i=0;i<40;++i) { auto p=players[0]; p.name="Spieler "+std::to_string(i); players.push_back(p); }
        } else if (st=="error") players={players.back()};
        else if (st=="empty" || st=="loading") players.clear();
        else { players={players[0]}; if(st=="rank-icon") players[0].rank_icon=solid; }
        out.push_back({"bf6."+st,[=](int w,int h) {
            return Bf6Renderer(w,h,"Battlefield 6 · Rangliste").render(players,st=="page2" ? 1 : 0,
                st=="loading" ? std::nullopt : std::optional<std::time_t>(clock_value),clock_value);
        },Hit::Rows,players.size()});
    }
    for (auto title : {"friends","discord"}) for (auto state : {"page1","page2","disconnected","empty","avatar","offline","mobile","platforms"}) {
        std::string st=state, key=title; auto friends=samples::friends();
        if (st=="page1" || st=="page2") {
            for(int i=0;i<40;++i) { auto f=friends[0]; f.name="Freund "+std::to_string(i); friends.push_back(f); }
        } else if(st=="empty") friends.clear();
        else if(st!="disconnected") {
            friends={friends[0]}; auto& f=friends[0];
            if(st=="avatar") f.avatar=solid;
            if(st=="offline") f.status="offline";
            if(st=="mobile") f.mobile=true;
            if(st=="platforms") { f.platforms={"discord","steam","xbox","psn"}; f.online_on=f.platforms; }
        }
        out.push_back({key+"."+st,[=](int w,int h) {
            return Renderer(w,h,key=="discord" ? "Discord-Freunde" : "Freunde").render_friends(friends,
                st=="page2" ? 1 : 0,st!="disconnected","Verbindung getrennt – bitte später erneut versuchen",clock_value);
        },Hit::Rows,friends.size()});
    }
    const std::vector<float> levels={.05f,.35f,.55f,.8f,.65f,.9f,1.f,.7f,.5f,.75f,.85f,.6f,.45f,.25f,.15f};
    for (auto state : {"playing-0","playing-1","playing-2","simulated","paused","no-controls","last-played","idle",
                       "not-set-up","logged-out","connecting","connecting-error","error","error-detail","cover","footer-error"}) {
        std::string st=state; auto snap=samples::spotify(); snap.fetched_at=progress_time;
        int color=st=="playing-0" ? 0 : st=="playing-2" ? 2 : 1;
        if(st=="paused") { snap.state=spotify::State::Paused; snap.playing=false; }
        if(st=="no-controls") snap.controls=false;
        if(st=="last-played" || st=="idle") { snap.state=spotify::State::Idle; snap.playing=false; snap.controls=false; snap.last_played=st=="last-played"; }
        if(st=="not-set-up") snap.state=spotify::State::NotSetUp;
        if(st=="logged-out") snap.state=spotify::State::LoggedOut;
        if(st=="connecting" || st=="connecting-error") snap.state=spotify::State::Connecting;
        if(st=="error" || st=="error-detail") snap.state=spotify::State::Error;
        if(st=="connecting-error" || st=="error-detail" || st=="footer-error") snap.error="Beispiel: Wiedergabe nicht erreichbar";
        if(st=="cover") snap.cover=solid;
        out.push_back({"spotify."+st,[=](int w,int h) {
            return SpotifyRenderer(w,h,"Spotify").render(snap,progress_time,&levels,color,st=="simulated",clock_value);
        },Hit::Spotify});
    }
    for(auto state : {"full","no-hwinfo","null","temp75","temp85","usage90","history0","history1"}) {
        std::string st=state; auto snap=samples::system();
        if(st=="no-hwinfo") snap.hwinfo=false;
        if(st=="null") snap=SysSnapshot{};
        if(st=="temp75" || st=="temp85") snap.cpu_temp=snap.gpu_temp=snap.disk_temp=st=="temp75" ? 75 : 85;
        if(st=="usage90") snap.cpu_pct=snap.gpu_pct=snap.ram_pct=snap.disk_pct=95;
        if(st=="history0" || st=="history1") { snap.down_hist.clear(); snap.up_hist.clear(); if(st=="history1") { snap.down_hist.push_back(1e6); snap.up_hist.push_back(2e5); } }
        out.push_back({"system."+st,[=](int w,int h) { return SysRenderer(w,h,"System","bit").render(snap,clock_value); },Hit::System});
    }
    // HA und Willkommen sind ausschließlich Kontrollbilder, keine Migrationsziele.
    auto ha=samples::ha();
    if(lang=="en") { ha[0].name="Living room"; ha[2].name="Desk lamp"; ha[3].name="Coffee maker"; }
    for(int n : {4,3,2,1,0}) {
        auto entities=ha; entities.resize(n);
        if(n==2) entities={ha[2],ha[0]};
        out.push_back({"control-ha."+std::to_string(n),[=](int w,int h) {
            return HaRenderer(w,h,"Home Assistant").render(entities,n!=0,n ? "" : "Home Assistant ist nicht erreichbar – Adresse und Netzwerk prüfen",
                n ? std::optional<std::time_t>(clock_value) : std::nullopt,clock_value);
        },Hit::Ha,entities.size()});
    }
    out.push_back({"control-welcome",[](int w,int h) {
        return Renderer(w,h,"").welcome("Willkommen!","Öffne die Einstellungen über das Fuchs-Symbol im Infobereich.");
    }});
    return out;
}
}
// P7-7: zusätzliche Rasterbasis. Die vorhandene Built-in-/Plugin-Basis wird nie beschrieben.
static int rss_layout_snapshots(bool record,const fs::path& output) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7-rss-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("Neue RSS-Basis existiert bereits; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Additive RSS-Rasterbasis fehlt.");for(std::string key,hash;in>>key>>hash;) if(!expected.emplace(key,hash).second) throw std::runtime_error("Doppelte RSS-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"RSS-Raster: "<<what<<'\n';}};
    const auto& catalog=*builtin::find("rss");
    auto defaults=builtin::standard(catalog);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            RssRenderer r(size.w,size.h,"RSS");
            auto posts=samples::posts(lang);
            for(size_t i=0;i<posts.size();++i) posts[i].link="https://example.org/post/"+std::to_string(i);
            auto more=posts;for(int i=0;i<6;++i) more.push_back(posts[i%posts.size()]);
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            for(int state=0;state<3;++state) {
                auto data=state==2?std::vector<Post>{}:posts;
                auto updated=state==1?std::optional<std::time_t>{}:std::optional<std::time_t>{clock_value};
                require(hash_image(r.render(data,updated,clock_value))==hash_image(r.render_arranged(data,updated,defaults,0,clock_value).image),prefix+" standard");
            }
            auto portrait_only=defaults;portrait_only.portrait->layout["rows"]={3,1};
            auto landscape_only=defaults;landscape_only.model.layout["rows"]={3,1};
            require(r.uses_arrangement(portrait_only)==(size.h>size.w),prefix+" portrait-only switch");
            require(r.uses_arrangement(landscape_only)==(size.h<=size.w),prefix+" landscape-only switch");
            const auto& unchanged=size.h>size.w?landscape_only:portrait_only;
            require(hash_image(r.render(posts,clock_value,clock_value))==hash_image(r.render_arranged(posts,clock_value,unchanged,0,clock_value).image),prefix+" untouched orientation fixed");
            for(int variant=0;variant<4;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    if(variant==0) m->layout["rows"]={3,1};
                    if(variant==1) {
                        m->layout["columns"]={1,1};m->layout["rows"]={1};
                        m->layout["zones"][0]["column"]=1;m->layout["zones"][0]["row"]=0;
                        m->layout["zones"][1]["column"]=0;m->layout["zones"][1]["row"]=0;
                    }
                    if(variant==2) m->layout["rows"]={1,4};
                    if(variant==3) {
                        m->blocks[0]["shown"]=false;m->blocks[1]["zone"]=m->blocks[0]["zone"];
                        m->layout["zones"].erase(m->layout["zones"].begin()+1);m->layout["rows"]={1};
                    }
                }
                if(variant==2) overlay.portrait->layout["rows"]={1,6};
                require(builtin::validate(catalog,overlay).empty(),prefix+" valid");
                const int per=r.arranged_rows(overlay);
                require(per>=0 && per<=4,prefix+" capacity");
                require(r.pages(more.size(),overlay)==(per?int((more.size()+per-1)/per):1),prefix+" pages");
                for(int state=0;state<4;++state) {
                    const bool empty=state==2,loading=state==1;
                    auto data=empty?std::vector<Post>{}:more;
                    const int page=state==3?r.pages(data.size(),overlay)-1:0;
                    auto drawn=r.render_arranged(data,loading?std::optional<std::time_t>{}:clock_value,overlay,page,clock_value);
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    actual[key]=hash_image(drawn.image);
                    require(drawn.targets.size()==size_t(loading || empty || !per?0:std::min(per,int(data.size())-page*per)),key+" target count");
                    touch::Tile tile{{17,23,size.w,size.h},true,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {
                        require(target.item<data.size() && target.item==size_t(page*per+tile.arranged_targets.size()),key+" snapshot index");
                        tile.arranged_targets.push_back({target.rect_px,{{data.at(target.item).link,""}},{}});
                    }
                    std::vector<touch::Tile> tiles{std::move(tile)};
                    // Unabhängige Sollgeometrie aus dem Katalogplan, nicht aus den gelieferten Zielen.
                    const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                    const float scale=scale_for(size.w,size.h);
                    const float w=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                    const float h=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                    auto plan=plugins::zones::plan({8,36,w-16,h-54},*plugins::edit::geometry(model),builtin::inputs(catalog,model),6);
                    uint64_t touch_hash=1469598103934665603ull;bool same=true;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& b:plan.blocks) if(!loading && !empty && per && b.id=="rss.liste" && b.visible())
                            for(int row=0;row<per && page*per+row<int(data.size());++row) {
                                const int left=int(std::nearbyint(b.area.x*scale)),right=int(std::nearbyint((b.area.x+b.area.width)*scale));
                                const int top=int(std::nearbyint((b.area.y+std::floor(b.area.height*row/per))*scale));
                                const int bottom=int(std::nearbyint((b.area.y+std::floor(b.area.height*(row+1)/per))*scale));
                                if(x>=left && x<right && y>=top && y<bottom) hit=page*per+row;
                            }
                        const auto* actual_hit=touch::at(tiles,x+17,y+23);
                        same=same && ((hit<0 && !actual_hit) || (hit>=0 && actual_hit && actual_hit->front().url==data.at(hit).link));
                        touch_hash^=uint64_t(hit+1);touch_hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(touch_hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("RSS-Rasterprüfung fehlgeschlagen; keine neue Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n';if(!out) throw std::runtime_error("Neue RSS-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [key,hash]:actual) require(expected.contains(key) && expected[key]==hash,key+" hash");require(expected.size()==actual.size(),"complete hash matrix");}
    // Erst nach dem Hashlauf mit WIC schreiben, damit die Vergleichsbasis unverändert bleibt.
    i18n::set_lang("de");apply_theme({{"theme","hell"}});
    auto preview=builtin::standard(catalog);preview.model.layout["rows"]={3,1};preview.portrait->layout["rows"]={3,1};
    write_png(output/"p7-rss-layout-preview.png",RssRenderer(480,320,"RSS").render_arranged(samples::posts(),clock_value,preview,0,clock_value).image);
    std::cout<<"RSS layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}

// P7-8: ausschließlich additive Streaming-Rasterbasis.
static int stream_layout_snapshots(bool record,const fs::path& output) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7-streaming-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("Neue Streaming-Basis existiert bereits; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Additive Streaming-Rasterbasis fehlt.");for(std::string key,hash;in>>key>>hash;) if(!expected.emplace(key,hash).second) throw std::runtime_error("Doppelte Streaming-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"Streaming-Raster: "<<what<<'\n';}};
    const auto& catalog=*builtin::find("streaming");
    auto defaults=builtin::standard(catalog);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            StreamRenderer r(size.w,size.h,"Streaming");
            auto items=samples::stream();
            auto logos=samples::stream_logos();
            for(size_t i=0;i<items.size();++i) {items[i].id=int(i)+100;items[i].series=i%2;}
            auto more=items;for(int i=0;i<6;++i) more.push_back(items[i%items.size()]);
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            for(int state=0;state<3;++state) {
                auto data=state==2?std::vector<StreamItem>{}:items;
                auto updated=state==1?std::optional<std::time_t>{}:std::optional<std::time_t>{clock_value};
                require(hash_image(r.render(state==1?StreamState::Loading:StreamState::Ok,data,updated,clock_value,&logos))==hash_image(r.render_arranged(state==1?StreamState::Loading:StreamState::Ok,data,updated,defaults,0,clock_value,&logos).image),prefix+" standard");
            }
            auto portrait_only=defaults;portrait_only.portrait->layout["rows"]={3,1};
            auto landscape_only=defaults;landscape_only.model.layout["rows"]={3,1};
            require(r.uses_arrangement(portrait_only)==(size.h>size.w),prefix+" portrait-only switch");
            require(r.uses_arrangement(landscape_only)==(size.h<=size.w),prefix+" landscape-only switch");
            const auto& unchanged=size.h>size.w?landscape_only:portrait_only;
            require(hash_image(r.render(StreamState::Ok,items,clock_value,clock_value,&logos))==hash_image(r.render_arranged(StreamState::Ok,items,clock_value,unchanged,0,clock_value,&logos).image),prefix+" untouched orientation fixed");
            for(int variant=0;variant<4;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    if(variant==0) m->layout["rows"]={3,1};
                    if(variant==1) {
                        m->layout["columns"]={1,1};m->layout["rows"]={1};
                        m->layout["zones"][0]["column"]=1;m->layout["zones"][0]["row"]=0;
                        m->layout["zones"][1]["column"]=0;m->layout["zones"][1]["row"]=0;
                    }
                    if(variant==2) m->layout["rows"]={1,4};
                    if(variant==3) {
                        m->blocks[0]["shown"]=false;m->blocks[1]["zone"]=m->blocks[0]["zone"];
                        m->layout["zones"].erase(m->layout["zones"].begin()+1);m->layout["rows"]={1};
                    }
                }
                if(variant==2) overlay.portrait->layout["rows"]={1,6};
                require(builtin::validate(catalog,overlay).empty(),prefix+" valid");
                const int per=r.arranged_rows(overlay);
                require(per>=0 && per<=StreamRenderer::MAX_ROWS,prefix+" capacity");
                require(r.pages(more.size(),overlay)==(per?int((more.size()+per-1)/per):1),prefix+" pages");
                for(int state=0;state<7;++state) {
                    const bool empty=state==2,loading=state==1 || state==4 || state==5;
                    const auto status=state==1?StreamState::Loading:state==4?StreamState::NoKey:state==5?StreamState::Error:StreamState::Ok;
                    auto data=empty?std::vector<StreamItem>{}:more;
                    const int page=state==3?r.pages(data.size(),overlay)-1:0;
                    auto drawn=r.render_arranged(status,data,loading?std::optional<std::time_t>{}:clock_value,overlay,page,clock_value,state==6?nullptr:&logos);
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    actual[key]=hash_image(drawn.image);
                    require(drawn.targets.size()==size_t(loading || empty || !per?0:std::min(per,int(data.size())-page*per)),key+" target count");
                    touch::Tile tile{{17,23,size.w,size.h},true,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {
                        require(target.item<data.size() && target.item==size_t(page*per+tile.arranged_targets.size()),key+" snapshot index");
                        tile.arranged_targets.push_back({target.rect_px,touch::stream_rows({data.at(target.item)}).front(),{}});
                    }
                    std::vector<touch::Tile> tiles{std::move(tile)};
                    // Unabhängige Sollgeometrie aus dem Katalogplan, nicht aus den gelieferten Zielen.
                    const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                    const float scale=scale_for(size.w,size.h);
                    const float w=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                    const float h=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                    auto plan=plugins::zones::plan({8,36,w-16,h-54},*plugins::edit::geometry(model),builtin::inputs(catalog,model),6);
                    uint64_t touch_hash=1469598103934665603ull;bool same=true;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& b:plan.blocks) if(!loading && !empty && per && b.id=="streaming.liste" && b.visible())
                            for(int row=0;row<per && page*per+row<int(data.size());++row) {
                                const int left=int(std::nearbyint(b.area.x*scale)),right=int(std::nearbyint((b.area.x+b.area.width)*scale));
                                const int top=int(std::nearbyint((b.area.y+std::floor(b.area.height*row/per))*scale));
                                const int bottom=int(std::nearbyint((b.area.y+std::floor(b.area.height*(row+1)/per))*scale));
                                if(x>=left && x<right && y>=top && y<bottom) hit=page*per+row;
                            }
                        const auto* actual_hit=touch::at(tiles,x+17,y+23);
                        same=same && ((hit<0 && !actual_hit) || (hit>=0 && actual_hit && actual_hit->front().url==touch::stream_rows({data.at(hit)}).front().front().url));
                        touch_hash^=uint64_t(hit+1);touch_hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(touch_hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("Streaming-Rasterprüfung fehlgeschlagen; keine neue Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n';if(!out) throw std::runtime_error("Neue Streaming-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [key,hash]:actual) require(expected.contains(key) && expected[key]==hash,key+" hash");require(expected.size()==actual.size(),"complete hash matrix");}
    // Erst nach dem Hashlauf mit WIC schreiben, damit die Vergleichsbasis unverändert bleibt.
    i18n::set_lang("de");apply_theme({{"theme","hell"}});
    auto preview=builtin::standard(catalog);preview.model.layout["rows"]={3,1};preview.portrait->layout["rows"]={3,1};
    write_png(output/"p7-streaming-layout-preview.png",StreamRenderer(480,320,"Streaming").render_arranged(StreamState::Ok,samples::stream(),clock_value,preview,0,clock_value).image);
    std::cout<<"Streaming layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}

// P7-9: additive BF6-Rasterbasis, bestehende Basis bleibt unverändert.
static int bf6_layout_snapshots(bool record,const fs::path& output) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7-bf6-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("Neue BF6-Basis existiert bereits; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Additive BF6-Rasterbasis fehlt.");for(std::string key,hash;in>>key>>hash;) if(!expected.emplace(key,hash).second) throw std::runtime_error("Doppelte BF6-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"BF6-Raster: "<<what<<'\n';}};
    const auto& catalog=*builtin::find("bf6");
    auto defaults=builtin::standard(catalog);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            Bf6Renderer r(size.w,size.h,"BF6");
            auto players=sort_players(samples::players());
            std::reverse(players.begin(),players.end()); // Eingang absichtlich unsortiert.
            auto more=players;for(int i=0;i<6;++i) more.push_back(players[i%players.size()]);more=sort_players(std::move(more));
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            for(int state=0;state<3;++state) {
                auto data=state==2?std::vector<Bf6Player>{}:players;
                auto updated=state==1?std::optional<std::time_t>{}:std::optional<std::time_t>{clock_value};
                require(hash_image(r.render(data,0,updated,clock_value))==hash_image(r.render_arranged(data,updated,defaults,0,clock_value).image),prefix+" standard");
            }
            auto portrait_only=defaults;portrait_only.portrait->layout["rows"]={3,1};
            auto landscape_only=defaults;landscape_only.model.layout["rows"]={3,1};
            require(r.uses_arrangement(portrait_only)==(size.h>size.w),prefix+" portrait-only switch");
            require(r.uses_arrangement(landscape_only)==(size.h<=size.w),prefix+" landscape-only switch");
            const auto& unchanged=size.h>size.w?landscape_only:portrait_only;
            require(hash_image(r.render(players,0,clock_value,clock_value))==hash_image(r.render_arranged(players,clock_value,unchanged,0,clock_value).image),prefix+" untouched orientation fixed");
            for(int variant=0;variant<4;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    if(variant==0) m->layout["rows"]={3,1};
                    if(variant==1) {
                        m->layout["columns"]={1,1};m->layout["rows"]={1};
                        m->layout["zones"][0]["column"]=1;m->layout["zones"][0]["row"]=0;
                        m->layout["zones"][1]["column"]=0;m->layout["zones"][1]["row"]=0;
                    }
                    if(variant==2) m->layout["rows"]={1,4};
                    if(variant==3) {
                        m->blocks[0]["shown"]=false;m->blocks[1]["zone"]=m->blocks[0]["zone"];
                        m->layout["zones"].erase(m->layout["zones"].begin()+1);m->layout["rows"]={1};
                    }
                }
                if(variant==2) overlay.portrait->layout["rows"]={1,6};
                require(builtin::validate(catalog,overlay).empty(),prefix+" valid");
                const int per=r.arranged_rows(overlay);
                require(per>=0 && per<=r.rows_per_page(),prefix+" capacity");
                require(r.pages(more.size(),overlay)==(per?int((more.size()+per-1)/per):1),prefix+" pages");
                for(int state=0;state<4;++state) {
                    const bool empty=state==2,loading=state==1;
                    auto data=empty?std::vector<Bf6Player>{}:more;
                    const int page=state==3?r.pages(data.size(),overlay)-1:0;
                    auto drawn=r.render_arranged(data,loading?std::optional<std::time_t>{}:clock_value,overlay,page,clock_value);
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    actual[key]=hash_image(drawn.image);
                    require(drawn.targets.size()==size_t(loading || empty || !per?0:std::min(per,int(data.size())-page*per)),key+" target count");
                    touch::Tile tile{{17,23,size.w,size.h},true,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {
                        require(target.item<data.size() && target.item==size_t(page*per+tile.arranged_targets.size()),key+" snapshot index");
                        tile.arranged_targets.push_back({target.rect_px,touch::player_targets(data.at(target.item)),{}});
                    }
                    std::vector<touch::Tile> tiles{std::move(tile)};
                    // Unabhängige Sollgeometrie aus dem Katalogplan, nicht aus den gelieferten Zielen.
                    const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                    const float scale=scale_for(size.w,size.h);
                    const float w=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                    const float h=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                    auto plan=plugins::zones::plan({8,36,w-16,h-54},*plugins::edit::geometry(model),builtin::inputs(catalog,model),6);
                    uint64_t touch_hash=1469598103934665603ull;bool same=true;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& b:plan.blocks) if(!loading && !empty && per && b.id=="bf6.rangliste" && b.visible())
                            for(int row=0;row<per && page*per+row<int(data.size());++row) {
                                const int left=int(std::nearbyint(b.area.x*scale)),right=int(std::nearbyint((b.area.x+b.area.width)*scale));
                                const int top=int(std::nearbyint((b.area.y+std::floor(b.area.height*row/per))*scale));
                                const int bottom=int(std::nearbyint((b.area.y+std::floor(b.area.height*(row+1)/per))*scale));
                                if(x>=left && x<right && y>=top && y<bottom) hit=page*per+row;
                            }
                        const auto* actual_hit=touch::at(tiles,x+17,y+23);
                        const auto links=hit<0?touch::Targets{}:touch::player_targets(data.at(hit));
                        same=same && (links.empty()?!actual_hit:actual_hit && actual_hit->front().url==links.front().url);
                        touch_hash^=uint64_t(hit+1);touch_hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(touch_hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("BF6-Rasterprüfung fehlgeschlagen; keine neue Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n';if(!out) throw std::runtime_error("Neue BF6-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [key,hash]:actual) require(expected.contains(key) && expected[key]==hash,key+" hash");require(expected.size()==actual.size(),"complete hash matrix");}
    // Erst nach dem Hashlauf mit WIC schreiben, damit die Vergleichsbasis unverändert bleibt.
    i18n::set_lang("de");apply_theme({{"theme","hell"}});
    auto preview=builtin::standard(catalog);preview.model.layout["rows"]={3,1};preview.portrait->layout["rows"]={3,1};
    write_png(output/"p7-bf6-layout-preview.png",Bf6Renderer(480,320,"BF6").render_arranged(samples::players(),clock_value,preview,0,clock_value).image);
    std::cout<<"BF6 layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}


// P7-10: gemeinsamer Katalog, unabhängige Hashdatei je Seite.
static int friends_layout_snapshots(bool record,const fs::path& output,const std::string& key) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/("p7-"+key+"-hashes.txt");
    if(record && fs::exists(file)) throw std::runtime_error("Neue Freunde/Discord-Basis existiert bereits; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Additive Freunde/Discord-Rasterbasis fehlt.");for(std::string key,hash;in>>key>>hash;) if(!expected.emplace(key,hash).second) throw std::runtime_error("Doppelte Freunde/Discord-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"Freunde/Discord-Raster: "<<what<<'\n';}};
    const auto& catalog=*builtin::find(key);
    auto defaults=builtin::standard(catalog);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            Renderer r(size.w,size.h,key=="discord"?"Discord":"Freunde");
            auto players=samples::friends();
            for(size_t i=0;i<players.size();++i) {
                players[i].ids={{"discord",std::to_string(100+i)}};
                players[i].platforms=i%2?std::vector<std::string>{"discord","steam","xbox","psn"}:std::vector<std::string>{"discord"};
                if(i%3==0) players[i].avatar=std::make_shared<Image>(32,32,Color{40,120,180});
            }
            players.back().ids.clear(); // Gezeichnete Zeile ohne Profilziel.
            players=sort_friends(std::move(players));
            std::reverse(players.begin(),players.end()); // Eingang absichtlich unsortiert.
            auto more=players;for(int i=0;i<6;++i) more.push_back(players[i%players.size()]);more=sort_friends(std::move(more));
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            for(int state=0;state<3;++state) {
                auto data=state==2?std::vector<Friend>{}:players;
                auto updated=state==1?std::optional<std::time_t>{}:std::optional<std::time_t>{clock_value};
                require(hash_image(r.render_friends(data,0,bool(updated),"",clock_value))==hash_image(r.render_friends_arranged(data,bool(updated),"",defaults,0,clock_value).image),prefix+" standard");
            }
            auto portrait_only=defaults;portrait_only.portrait->layout["rows"]={3,1};
            auto landscape_only=defaults;landscape_only.model.layout["rows"]={3,1};
            require(r.uses_friends_arrangement(portrait_only)==(size.h>size.w),prefix+" portrait-only switch");
            require(r.uses_friends_arrangement(landscape_only)==(size.h<=size.w),prefix+" landscape-only switch");
            const auto& unchanged=size.h>size.w?landscape_only:portrait_only;
            require(hash_image(r.render_friends(players,0,true,"",clock_value))==hash_image(r.render_friends_arranged(players,true,"",unchanged,0,clock_value).image),prefix+" untouched orientation fixed");
            for(int variant=0;variant<4;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    if(variant==0) m->layout["rows"]={3,1};
                    if(variant==1) {
                        m->layout["columns"]={1,1};m->layout["rows"]={1};
                        m->layout["zones"][0]["column"]=1;m->layout["zones"][0]["row"]=0;
                        m->layout["zones"][1]["column"]=0;m->layout["zones"][1]["row"]=0;
                    }
                    if(variant==2) m->layout["rows"]={1,4};
                    if(variant==3) {
                        m->blocks[0]["shown"]=false;m->blocks[1]["zone"]=m->blocks[0]["zone"];
                        m->layout["zones"].erase(m->layout["zones"].begin()+1);m->layout["rows"]={1};
                    }
                }
                if(variant==2) overlay.portrait->layout["rows"]={1,6};
                require(builtin::validate(catalog,overlay).empty(),prefix+" valid");
                const int per=r.friend_arranged_rows(overlay);
                require(per>=0 && per<=r.rows_per_page(),prefix+" capacity");
                require(r.friend_pages(more.size(),overlay)==(per?int((more.size()+per-1)/per):1),prefix+" pages");
                for(int state=0;state<4;++state) {
                    const bool empty=state==2,loading=state==1;
                    auto data=empty?std::vector<Friend>{}:more;
                    const int page=state==3?r.friend_pages(data.size(),overlay)-1:0;
                    auto drawn=r.render_friends_arranged(data,!loading,"",overlay,page,clock_value);
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    actual[key]=hash_image(drawn.image);
                    require(drawn.targets.size()==size_t(loading || empty || !per?0:std::min(per,int(data.size())-page*per)),key+" target count");
                    touch::Tile tile{{17,23,size.w,size.h},true,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {
                        require(target.item<data.size() && target.item==size_t(page*per+tile.arranged_targets.size()),key+" snapshot index");
                        tile.arranged_targets.push_back({target.rect_px,touch::friend_targets(data.at(target.item)),{}});
                    }
                    std::vector<touch::Tile> tiles{std::move(tile)};
                    // Unabhängige Sollgeometrie aus dem Katalogplan, nicht aus den gelieferten Zielen.
                    const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                    const float scale=scale_for(size.w,size.h);
                    const float w=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                    const float h=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                    auto plan=plugins::zones::plan({8,36,w-16,h-54},*plugins::edit::geometry(model),builtin::inputs(catalog,model),6);
                    uint64_t touch_hash=1469598103934665603ull;bool same=true;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& b:plan.blocks) if(!loading && !empty && per && b.id=="freunde.liste" && b.visible())
                            for(int row=0;row<per && page*per+row<int(data.size());++row) {
                                const int left=int(std::nearbyint(b.area.x*scale)),right=int(std::nearbyint((b.area.x+b.area.width)*scale));
                                const int top=int(std::nearbyint((b.area.y+std::floor(b.area.height*row/per))*scale));
                                const int bottom=int(std::nearbyint((b.area.y+std::floor(b.area.height*(row+1)/per))*scale));
                                if(x>=left && x<right && y>=top && y<bottom) hit=page*per+row;
                            }
                        const auto* actual_hit=touch::at(tiles,x+17,y+23);
                        const auto links=hit<0?touch::Targets{}:touch::friend_targets(data.at(hit));
                        same=same && (links.empty()?!actual_hit:actual_hit && actual_hit->front().url==links.front().url);
                        touch_hash^=uint64_t(hit+1);touch_hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(touch_hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("Freunde/Discord-Rasterprüfung fehlgeschlagen; keine neue Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n';if(!out) throw std::runtime_error("Neue Freunde/Discord-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [key,hash]:actual) require(expected.contains(key) && expected[key]==hash,key+" hash");require(expected.size()==actual.size(),"complete hash matrix");}
    // Erst nach dem Hashlauf mit WIC schreiben, damit die Vergleichsbasis unverändert bleibt.
    i18n::set_lang("de");apply_theme({{"theme","hell"}});
    auto preview=builtin::standard(catalog);preview.model.layout["rows"]={3,1};preview.portrait->layout["rows"]={3,1};
    write_png(output/("p7-"+key+"-layout-preview.png"),Renderer(480,320,key=="discord"?"Discord":"Freunde").render_friends_arranged(samples::friends(),true,"",preview,0,clock_value).image);
    std::cout<<"Freunde/Discord layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}


// P7-11: ausschließlich additive Rasterbasis; bestehende Hashdateien bleiben gesperrt.
static int system_layout_snapshots(bool record,const fs::path& output) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7-system-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("System-Rasterbasis existiert; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("System-Rasterbasis fehlt.");for(std::string k,v;in>>k>>v;) if(!expected.emplace(k,v).second) throw std::runtime_error("Doppelte System-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"System-Raster: "<<what<<'\n';}};
    const auto& c=*builtin::find("system");const auto defaults=builtin::standard(c);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            SysRenderer r(size.w,size.h,"System","MB/s");auto snap=samples::system();
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            require(hash_image(r.render(snap,clock_value))==hash_image(r.render_arranged(snap,defaults,clock_value).image),prefix+" standard");
            auto one=defaults;one.portrait->layout["rows"]={2,1,1};
            require(r.uses_arrangement(one)==(size.h>size.w),prefix+" orientation switch");
            if(size.h<=size.w) require(hash_image(r.render(snap,clock_value))==hash_image(r.render_arranged(snap,one,clock_value).image),prefix+" unchanged fixed");
            auto invalid=defaults;invalid.catalog_version++;
            require(hash_image(r.render(snap,clock_value))==hash_image(r.render_arranged(snap,invalid,clock_value).image),prefix+" invalid fallback");
            for(int variant=0;variant<6;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    m->layout["rows"]={2,1,1};
                    if(variant==1) { // Jede Karte erhält Platz für ihre vollständige Variante.
                        m->layout["columns"]={1};m->layout["rows"]={1};
                        m->layout["zones"]=builtin::Json::array({{{"id","all"},{"column",0},{"row",0}}});
                        for(auto& b:m->blocks) {b["shown"]=b["id"]=="system.netz";b["zone"]="all";}
                    }
                    if(variant==2) { // Cupertino: zwei Karten in einer Gruppe, andere zurück in die Leiste.
                        m->layout["columns"]={1};m->layout["rows"]={1};
                        m->layout["zones"]=builtin::Json::array({{{"id","all"},{"column",0},{"row",0}}});
                        for(auto& b:m->blocks) {b["shown"]=b["id"]=="system.cpu" || b["id"]=="system.ram";b["zone"]="all";}
                    }
                    if(variant==3) m->layout["rows"]={12,1,1}; // Zu kleine Werkzeugkarten haben keine Ziele.
                    if(variant==4 || variant==5) {
                        m->layout["columns"]={1};m->layout["rows"]={1};
                        m->layout["zones"]=builtin::Json::array({{{"id","all"},{"column",0},{"row",0}}});
                        for(auto& b:m->blocks) {b["shown"]=b["id"]==(variant==4?"system.cpu":"system.gpu");b["zone"]="all";}
                    }
                }
                require(builtin::validate(c,overlay).empty(),prefix+" valid variant");
                const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                const float scale=scale_for(size.w,size.h);
                const float lw=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                const float lh=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                const auto plan=plugins::zones::plan({8,36,lw-16,lh-54},*plugins::edit::geometry(model),builtin::inputs(c,model),6);
                for(int state=0;state<4;++state) {
                    snap=samples::system();
                    if(state<3) {snap.cpu_temp=state==0?74:state==1?75:85;snap.gpu_temp=snap.cpu_temp;snap.disk_temp=snap.cpu_temp;snap.cpu_pct=state==2?90:89;}
                    else snap=SysSnapshot{};
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    auto drawn=r.render_arranged(snap,overlay,clock_value);
                    require(drawn.image.w==size.w && drawn.image.h==size.h,key+" dimensions");actual[key]=hash_image(drawn.image);
                    int selected=-1;touch::Tile tile{{17,23,size.w,size.h},false,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {const int item=int(target.item);require(item>=0 && item<3,key+" action index");tile.arranged_targets.push_back({target.rect_px,{},[&,item]{selected=item;}});}
                    const std::vector<touch::Tile> tiles={tile};bool same=true;uint64_t hash=1469598103934665603ull;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& b:plan.blocks) if(b.visible()) for(size_t i=2;i<c.blocks.size();++i) if(b.id==c.blocks[i].id) {
                            const int left=int(std::nearbyint(b.area.x*scale)),right=int(std::nearbyint((b.area.x+b.area.width)*scale));
                            const int top=int(std::nearbyint(b.area.y*scale)),bottom=int(std::nearbyint((b.area.y+b.area.height)*scale));
                            if(x>=left && x<right && y>=top && y<bottom) hit=int(i)-2;
                        }
                        const auto* action=touch::action_at(tiles,x+17,y+23);selected=-1;if(action) (*action)();same=same && selected==hit;
                        hash^=uint64_t(hit+1);hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("System-Rasterprüfung fehlgeschlagen; keine Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [k,v]:actual) out<<k<<' '<<v<<'\n';if(!out) throw std::runtime_error("System-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [k,v]:actual) require(expected.contains(k) && expected[k]==v,k+" hash");require(actual.size()==expected.size(),"complete matrix");}
    i18n::set_lang("de");
    for(const auto& tc:theme_cases()) {
        if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
        auto preview=defaults;preview.model.layout["rows"]={3,2,2};preview.portrait->layout["rows"]={3,2,2};
        write_png(output/("p7-system-"+tc.name+"-preview.png"),SysRenderer(480,320,"System","MB/s").render_arranged(samples::system(),preview,clock_value).image);
    }
    std::cout<<"System layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}


// P7-12: eingefrorene Spotify-Zustände und synthetische Pegel, ohne Audiogerät.
static int spotify_layout_snapshots(bool record,const fs::path& output) {
    const fs::path file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7-spotify-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("Spotify-Rasterbasis existiert; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Spotify-Rasterbasis fehlt.");for(std::string k,v;in>>k>>v;) if(!expected.emplace(k,v).second) throw std::runtime_error("Doppelte Spotify-Rasterbasis.");}
    int failures=0,checked=0;
    auto require=[&](bool ok,const std::string& what){++checked;if(!ok){++failures;std::cerr<<"Spotify-Raster: "<<what<<'\n';}};
    const auto& c=*builtin::find("spotify");const auto defaults=builtin::standard(c);
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& tc:theme_cases()) for(const auto& size:sizes()) {
            if(lang=="en" && (tc.name!="dunkel" || std::string(size.name)!="landscape")) continue;
            if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});else apply_theme({{"theme_inline",tc.inline_theme}});
            SpotifyRenderer r(size.w,size.h,"Spotify");
            const std::string prefix=lang+"."+tc.name+"."+size.name;
            auto one=defaults;one.portrait->layout["rows"]={2,1,1};
            require(r.uses_arrangement(one)==(size.h>size.w),prefix+" orientation switch");
            auto snap=samples::spotify();
            if(size.h<=size.w) require(hash_image(r.render(snap,progress_time,nullptr,1,false,clock_value))==hash_image(r.render_arranged(snap,one,progress_time,nullptr,1,false,clock_value).image),prefix+" unchanged fixed");
            auto invalid=defaults;invalid.catalog_version++;
            require(hash_image(r.render(snap,progress_time,nullptr,1,false,clock_value))==hash_image(r.render_arranged(snap,invalid,progress_time,nullptr,1,false,clock_value).image),prefix+" invalid fallback");
            const auto cover=std::make_shared<const Image>(Canvas(64,64,1.f,{30,120,90}).finish());
            for(int variant=0;variant<4;++variant) {
                auto overlay=defaults;
                for(auto* m:{&overlay.model,&*overlay.portrait}) {
                    m->layout["rows"]={2,1,1};
                    if(variant==1) {
                        const auto zone=m->blocks[0]["zone"];m->blocks[0]["zone"]=m->blocks[1]["zone"];m->blocks[1]["zone"]=zone;
                        m->layout["rows"]={1,2,1};
                    }
                    if(variant==2) {
                        m->layout["columns"]={1};m->layout["rows"]={3,1};
                        m->layout["zones"]=builtin::Json::array({{{"id","main"},{"column",0},{"row",0}},{{"id","hint"},{"column",0},{"row",1}}});
                        for(auto& b:m->blocks) {const bool hint=b["id"]=="spotify.hinweis";b["shown"]=hint || b["id"]=="spotify.text" || b["id"]=="spotify.fortschritt" || b["id"]=="spotify.tasten";b["zone"]=hint?"hint":"main";}
                    }
                    if(variant==3) {
                        m->layout["columns"]={1};m->layout["rows"]={1};
                        m->layout["zones"]=builtin::Json::array({{{"id","all"},{"column",0},{"row",0}}});
                        for(auto& b:m->blocks) {b["shown"]=b["id"]=="spotify.hinweis";b["zone"]="all";}
                    }
                }
                require(builtin::validate(c,overlay).empty(),prefix+" valid variant");
                const auto& model=size.h>size.w?*overlay.portrait:overlay.model;
                const float scale=scale_for(size.w,size.h);
                const float lw=scale==1?float(size.w):float(std::nearbyint(size.w/scale));
                const float lh=scale==1?float(size.h):float(std::nearbyint(size.h/scale));
                const auto plan=plugins::zones::plan({8,36,lw-16,lh-54},*plugins::edit::geometry(model),builtin::inputs(c,model),6);
                for(int state=0;state<16;++state) {
                    snap=samples::spotify();snap.fetched_at=progress_time;
                    const int color=state<3?state:1;const bool simulated=state==3;
                    std::vector<float> levels={.05f,.35f,.55f,.8f,.65f,.9f,1.f,.7f,.5f,.75f,.85f,.6f,.45f,.25f,.15f};
                    if(state==4) {snap.state=spotify::State::Paused;snap.playing=false;}
                    if(state==5) snap.controls=false;
                    if(state==6 || state==7) {snap.state=spotify::State::Idle;snap.playing=false;snap.controls=false;snap.last_played=state==6;}
                    if(state==8) snap.state=spotify::State::NotSetUp;
                    if(state==9) snap.state=spotify::State::LoggedOut;
                    if(state==10) snap.state=spotify::State::Connecting;
                    if(state==11 || state==12) snap.state=spotify::State::Error;
                    if(state==12) snap.error="Beispiel: keine Verbindung";
                    if(state==13 || state==0) snap.cover=cover;
                    if(state==14) snap.error="Beispiel: vorübergehend verzögert";
                    if(state==15) levels.assign(256,.9f);
                    const auto key=prefix+".v"+std::to_string(variant)+".s"+std::to_string(state);
                    if(variant==0) require(hash_image(r.render(snap,progress_time,&levels,color,simulated,clock_value))==hash_image(r.render_arranged(snap,defaults,progress_time,&levels,color,simulated,clock_value).image),key+" standard");
                    auto drawn=r.render_arranged(snap,overlay,progress_time,&levels,color,simulated,clock_value);
                    require(drawn.image.w==size.w && drawn.image.h==size.h,key+" dimensions");actual[key]=hash_image(drawn.image);
                    const bool live=(snap.state==spotify::State::Playing || snap.state==spotify::State::Paused) ||
                                    (snap.state==spotify::State::Idle && snap.last_played);
                    std::vector<BuiltinTarget> expected_targets;
                    auto add=[&](float x,float y,float width,float height,size_t item) {
                        const int left=int(std::nearbyint(x*scale)),right=int(std::nearbyint((x+width)*scale));
                        const int top=int(std::nearbyint(y*scale)),bottom=int(std::nearbyint((y+height)*scale));
                        expected_targets.push_back({{left,top,right-left,bottom-top},item});
                    };
                    for(const auto& b:plan.blocks) if(b.visible() && live) {
                        const auto& a=b.area;
                        if(b.id=="spotify.cover" && snap.cover && !snap.cover->empty()) {
                            const float side=std::min(a.width,a.height);add(a.x+(a.width-side)/2,a.y+(a.height-side)/2,side,side,3);
                        }
                        if(b.id=="spotify.tasten" && snap.controls && !snap.last_played)
                            for(size_t i=0;i<3;++i) add(a.x+(float(i)+.5f)*a.width/3-24,a.y+a.height/2-24,48,48,i);
                    }
                    require(drawn.targets.size()==expected_targets.size(),key+" visible actions");
                    int selected=-1;touch::Tile tile{{17,23,size.w,size.h},false,{}};tile.arranged=true;
                    for(const auto& target:drawn.targets) {const int item=int(target.item);require(item>=0 && item<4,key+" action index");if(item<3) require(target.rect_px.w/scale>=44 && target.rect_px.h/scale>=44,key+" minimum target");tile.arranged_targets.push_back({target.rect_px,{},[&,item]{selected=item;}});}
                    const std::vector<touch::Tile> tiles={tile};bool same=true;uint64_t hash=1469598103934665603ull;
                    for(int y=0;y<size.h;y+=4) for(int x=0;x<size.w;x+=4) {
                        int hit=-1;
                        for(const auto& target:expected_targets) {const auto& a=target.rect_px;if(x>=a.x && x<a.x+a.w && y>=a.y && y<a.y+a.h) {hit=int(target.item);break;}}
                        const auto* action=touch::action_at(tiles,x+17,y+23);selected=-1;if(action) (*action)();same=same && selected==hit;
                        hash^=uint64_t(hit+1);hash*=1099511628211ull;
                    }
                    require(same,key+" 4px hit map");actual[key+".touch"]=std::to_string(hash);
                }
            }
        }
    }
    if(record && failures) throw std::runtime_error("Spotify-Rasterprüfung fehlgeschlagen; keine Basis geschrieben.");
    if(record) {std::ofstream out(file,std::ios::binary);for(const auto& [k,v]:actual) out<<k<<' '<<v<<'\n';if(!out) throw std::runtime_error("Spotify-Rasterbasis nicht geschrieben.");}
    else {for(const auto& [k,v]:actual) require(expected.contains(k) && expected[k]==v,k+" hash");require(actual.size()==expected.size(),"complete matrix");}
    i18n::set_lang("de");apply_theme({{"theme","hell"}});
    auto preview=defaults;preview.model.layout["rows"]={2,1,1};
    std::vector<float> levels={.05f,.35f,.55f,.8f,.65f,.9f,1.f,.7f,.5f,.75f,.85f,.6f,.45f,.25f,.15f};
    write_png(output/"p7-spotify-layout-preview.png",SpotifyRenderer(480,320,"Spotify").render_arranged(samples::spotify(),preview,progress_time,&levels,1,true,clock_value).image);
    std::cout<<"Spotify layout: "<<actual.size()/2<<" images/hit maps, "<<checked<<" checks, "<<failures<<" errors\n";
    return failures;
}


// P7b-0: ausschließlich neue Hashdatei; alte Theme-Matrizen bleiben unverändert.
static int material_orange_snapshots(bool record,const fs::path& output) {
    const auto file=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path()/"layout-snapshots"/"p7b0-material-orange-hashes.txt";
    if(record && fs::exists(file)) throw std::runtime_error("Material-Orange-Basis existiert; Überschreiben verboten.");
    std::map<std::string,std::string> expected,actual;
    if(!record) {std::ifstream in(file);if(!in) throw std::runtime_error("Material-Orange-Basis fehlt.");for(std::string k,v;in>>k>>v;) if(!expected.emplace(k,v).second) throw std::runtime_error("Doppelte Material-Orange-Basis.");}
    std::vector<std::pair<fs::path,Image>> previews;
    const std::vector<Size> matrix={{"landscape",480,320},{"portrait",320,480},{"small",240,160}};
    for(const auto& lang:{std::string("de"),std::string("en")}) {
        i18n::set_lang(lang);
        for(const auto& size:matrix) {
            std::lock_guard lock(render::mutex());apply_theme({{"theme",theme::DEFAULT_ID}});
            for(const auto& c:cases(lang)) {
                const auto key=lang+".material-orange."+size.name+"."+c.name;
                auto image=c.draw(size.w,size.h);actual[key]=hash_image(image);actual[key+".touch"]=hit_hash(c,size.w,size.h);
                if(lang=="de" && std::string(size.name)=="landscape" && (c.name=="system.full" || c.name=="spotify.playing-0")) previews.emplace_back(output/("p7b0-"+c.name+".png"),std::move(image));
            }
            for(const auto* name:{"offline-clock","offline-zonen"}) {
                const auto path=fs::path(FL_BUILTIN_SNAPSHOT_DIR).parent_path().parent_path()/"examples"/"plugins"/(std::string(name)+".flplugin.json");
                std::ifstream in(path);std::string text((std::istreambuf_iterator<char>(in)),{});auto parsed=plugins::parse_manifest(text);
                if(!parsed) throw std::runtime_error("Plugin-Themabeispiel ungültig.");
                plugins::RenderContext context;context.sources=plugins::sample_data(*parsed.manifest);context.time="12:34";context.date="2026-01-01";
                for(const auto& setting:parsed.manifest->settings) if(setting.type!="secret")context.settings[setting.key]=setting.default_value;
                auto drawn=plugins::draw_page_out(plugins::render_page(*parsed.manifest,context),size.w,size.h);
                const auto key=lang+".material-orange."+size.name+".plugin-"+name;
                actual[key]=hash_image(drawn.image);
                if(lang=="de" && std::string(size.name)=="landscape" && std::string(name)=="offline-zonen") previews.emplace_back(output/"p7b0-plugin.png",std::move(drawn.image));
            }
        }
    }
    int failures=0;
    if(record) {std::ofstream out(file);for(const auto& [k,v]:actual)out<<k<<' '<<v<<'\n';if(!out) throw std::runtime_error("Neue Themebasis nicht geschrieben.");}
    else {for(const auto& [k,v]:actual) if(!expected.contains(k) || expected.at(k)!=v) {++failures;std::cerr<<"Material Orange abweichend: "<<k<<'\n';}if(expected.size()!=actual.size())++failures;}
    // PNG erst nach allen Hashes: WIC beeinflusst auf diesem Rechner spätere Textdarstellung.
    for(const auto& [path,image]:previews)write_png(path,image);
    std::cout<<"Material Orange: "<<actual.size()<<" additive image/hit hashes, "<<failures<<" errors\n";
    return failures;
}
int main(int argc,char** argv) {
    try {
        _putenv_s("TZ","UTC0"); _tzset();
        const std::string mode=argc>1 ? argv[1] : "pruefen";
        if(argc>3 || (mode!="export" && argc>2) || (mode!="pruefen" && mode!="prüfen" && mode!="update" && mode!="export" && mode!="record-rss-layout" && mode!="record-streaming-layout" && mode!="record-bf6-layout" && mode!="record-friends-layout" && mode!="record-system-layout" && mode!="record-spotify-layout" && mode!="record-material-orange") ||
           (mode=="export" && argc!=3)) throw std::runtime_error("Aufruf: fl-builtin-snapshot-tests [pruefen|update|export <Ordner>]");
        require_fonts(); gfx_init();
        const fs::path root=mode=="export" ? fs::path(argv[2]) : fs::path(FL_BUILTIN_SNAPSHOT_DIR);
        std::map<std::string,std::string> expected, actual;
        if(mode!="update" && mode!="export") {
            std::ifstream in(root/"hashes.txt");
            if(!in) throw std::runtime_error("Built-in-Baseline fehlt: hashes.txt (explizit update ausführen).");
            for(std::string line;std::getline(in,line);) {
                if(line.empty() || line[0]=='#') continue;
                std::istringstream fields(line); std::string key,hash;
                if(!(fields>>key>>hash) || !expected.emplace(key,hash).second) throw std::runtime_error("Ungültige/doppelte Baseline-Zeile: "+line);
            }
        } else fs::create_directories(root);
        int failures=0; size_t images=0;
        std::vector<std::pair<fs::path,decltype(cases("de").front().draw(1,1))>> deferred;
        const auto matrix=sizes();
        // Zwei Durchlaeufe: PNG-Schreiben (WIC) veraendert sonst die spaetere Textdarstellung (Hash-Lauf vs. Vergleich).
        // Durchlauf 1 hasht/vergleicht ohne jede PNG-Ausgabe; Durchlauf 2 (nur update/export) rendert erneut und schreibt PNGs.
        auto sweep=[&](bool png_pass) {
        for(const auto& lang : {std::string("de"),std::string("en")}) {
            i18n::set_lang(lang);
            for(const auto& tc : theme_cases()) {
                if(lang=="en" && tc.name!="dunkel" && tc.name!="custom-material") continue;
                for(const auto& size : matrix) {
                    if(lang=="en" && std::string(size.name)!="landscape" && std::string(size.name)!="portrait") continue;
                    std::lock_guard lock(render::mutex());
                    // Auch die künstlichen Logos entstehen erst nach dem festen Theme.
                    if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});
                    else apply_theme({{"theme_inline",tc.inline_theme}});
                    for(const auto& c : cases(lang)) {
                        std::string key=lang+"."+tc.name+"."+size.name+"."+c.name;
                        if(png_pass) {
                            if(mode=="export" || (lang=="de" && tc.name=="dunkel")) write_png(root/(key+".png"),c.draw(size.w,size.h));
                            continue;
                        }
                        // Jeder Aufruf baut einen neuen Zeichner nach apply_theme.
                        auto image=c.draw(size.w,size.h);
                        if(image.w!=size.w || image.h!=size.h) throw std::runtime_error("Falsche Bildgröße: "+c.name);
                        if(!actual.emplace(key,hash_image(image)).second ||
                           !actual.emplace(key+".touch",hit_hash(c,size.w,size.h)).second)
                            throw std::runtime_error("Doppelter Snapshot-Schlüssel: "+key);
                        ++images;
                        bool differs=false;
                        for(const auto& k : {key,key+".touch"}) if(mode!="update" && mode!="export" &&
                            (!expected.contains(k) || expected.at(k)!=actual.at(k))) {
                            ++failures; differs=true; std::cerr<<"Abweichung: "<<k<<" erwartet "<<(expected.contains(k) ? expected.at(k) : "FEHLT")<<" aktuell "<<actual.at(k)<<'\n';
                        }
                        if(differs) {
                            auto dir=fs::temp_directory_path()/"fl-builtin-snapshot-actual"; fs::create_directories(dir);
                            deferred.emplace_back(dir/(key+".png"),image);
                            std::ofstream(dir/(key+".txt"))<<actual[key]<<"\n"<<actual[key+".touch"]<<"\n";
                        }
                    }
                    if(!png_pass) std::cout<<lang<<' '<<tc.name<<' '<<size.name<<" fertig\n"<<std::flush;
                }
            }
        }
        };
        sweep(false);
        for(const auto& [path,image] : deferred) write_png(path,image);
        if(mode=="update" || mode=="export") sweep(true);
        if(images!=4221 || actual.size()!=8442) throw std::runtime_error("Unvollständige P5-0-Matrix (erwartet 4221 Bilder + Trefferkarten).");
        if(mode=="update" || mode=="export") {
            std::ofstream file(root/"hashes.txt",std::ios::binary);
            file<<"# P5-0 FNV-1a 64: Größe+ARGB; .touch: 4px-Raster, erste Metadatenzeile rows_per_page/pages(n). TZ=UTC0 clock=1792606500 mono=1000. Rechnergebundene DirectWrite-Baseline.\n";
            for(const auto& [k,v]:actual) file<<k<<' '<<v<<'\n';
            if(!file) throw std::runtime_error("Hashes konnten nicht geschrieben werden.");
        } else for(const auto& [k,v]:expected) if(!actual.contains(k)) { ++failures; std::cerr<<"Veralteter Eintrag: "<<k<<'\n'; }
        if((mode=="record-rss-layout" || mode=="record-streaming-layout" || mode=="record-bf6-layout" || mode=="record-friends-layout" || mode=="record-system-layout" || mode=="record-spotify-layout") && failures) throw std::runtime_error("Bestehende Baseline abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") failures+=rss_layout_snapshots(mode=="record-rss-layout",fs::path(argv[0]).parent_path());
        if(failures && mode.starts_with("record-")) throw std::runtime_error("Vorhandene Rasterbasis abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") failures+=stream_layout_snapshots(mode=="record-streaming-layout",fs::path(argv[0]).parent_path());
        if(failures && mode.starts_with("record-")) throw std::runtime_error("Vorhandene Rasterbasis abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") failures+=bf6_layout_snapshots(mode=="record-bf6-layout",fs::path(argv[0]).parent_path());
        if(failures && mode.starts_with("record-")) throw std::runtime_error("Vorhandene Rasterbasis abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") for(const std::string key:{"friends","discord"})
            failures+=friends_layout_snapshots(mode=="record-friends-layout",fs::path(argv[0]).parent_path(),key);
        if(failures && mode.starts_with("record-")) throw std::runtime_error("Vorhandene Rasterbasis abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") failures+=system_layout_snapshots(mode=="record-system-layout",fs::path(argv[0]).parent_path());
        if(failures && mode.starts_with("record-")) throw std::runtime_error("Vorhandene Rasterbasis abweichend; keine neue Basis geschrieben.");
        if(mode!="update" && mode!="export") failures+=spotify_layout_snapshots(mode=="record-spotify-layout",fs::path(argv[0]).parent_path());
        if(mode=="record-material-orange" && failures) throw std::runtime_error("Vorhandene Baseline abweichend; neue Themebasis nicht geschrieben.");
        if(mode!="update" && mode!="export") failures+=material_orange_snapshots(mode=="record-material-orange",fs::path(argv[0]).parent_path());
        std::cout<<"Built-in snapshots: "<<images<<" Bilder, "<<actual.size()/2<<" Trefferkarten, "<<failures<<" Fehler\n";
        return failures ? 1 : 0;
    } catch(const std::exception& e) { std::cerr<<"Built-in snapshots ABBRUCH: "<<e.what()<<'\n'; return 2; }
}

