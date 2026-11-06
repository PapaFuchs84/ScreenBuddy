// © 2026 FuchsLabs
#include "device_profile.h"
#include "touch.h"
#include <iostream>
#include <array>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <stdexcept>
using namespace fl;
int count=0;
void check(bool ok) { ++count; if(!ok) throw std::runtime_error("Geräteprofil-Prüfung fehlgeschlagen: "+std::to_string(count)); }
int main() { try {
    using namespace device;
    Insets p{10,20,30,40};
    check(to_frame(p,Orientation::Portrait)==p);
    check(to_frame(p,Orientation::ReversePortrait)==Insets{30,40,10,20});
    check(to_frame(p,Orientation::Landscape)==Insets{20,30,40,10});
    check(to_frame(p,Orientation::ReverseLandscape)==Insets{40,10,20,30});
    check(to_frame(p,static_cast<Orientation>(4))==p);
    check(to_frame(p,static_cast<Orientation>(255))==p);
    check(select_keepout(p, Insets{48,48,48,48}, true)==p);
    check(select_keepout(p, Insets{48,48,48,48}, false)==Insets{48,48,48,48});
    check(select_keepout(p, std::nullopt, false)==p);
    check(parse_insets(ojson{{"top",-1},{"right",1001},{"bottom",1.5},{"left","2"}})==Insets{0,1000,0,0});
    check(parse_insets(nullptr)==Insets{});
    check(parse_insets(ojson{{"top",true},{"right",nullptr},{"left",UINT64_MAX}})==Insets{0,0,0,1000});
    check(parse_insets(ojson{{"top",12}})==Insets{12,0,0,0});
    for (const auto& section : {"display", "companion"}) {
        auto cfg = config::defaults();
        check(cfg[section]["keepout"] == insets_json({}));
        check(config::merge(cfg, {{section,{{"keepout",insets_json({1,2,3,4})}}}})[section]["keepout"] == insets_json({1,2,3,4}));
        cfg[section]["keepout"] = insets_json({1,2,3,4});
        check(config::merge(cfg, {{section,{{"keepout",{{"top",5}}}}}})[section]["keepout"] == insets_json({5,2,3,4}));
        check(config::merge(cfg, {{section,{{"keepout",ojson::object()}}}})[section]["keepout"] == insets_json({1,2,3,4}));
        for(auto bad : {ojson(-1),ojson(1001),ojson(1.5),ojson(true),ojson("2"),ojson(nullptr)}) {
            bool rejected=false;
            try { config::merge(cfg, {{section,{{"keepout",{{"top",bad}}}}}}); }
            catch(const std::invalid_argument&) { rejected=true; }
            check(rejected);
        }
        bool rejected=false;
        try { config::merge(cfg, {{section,{{"keepout",{{"wrong",1}}}}}}); }
        catch(const std::invalid_argument&) { rejected=true; }
        check(rejected);
    }
    // Nur Beispieldaten ohne Geheimnisse: TOML muss die Objektstruktur erhalten.
    auto path = std::filesystem::temp_directory_path() / ("fl-device-profile-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".toml");
    auto cfg = config::defaults();
    cfg["display"]["keepout"] = insets_json({10,20,30,40});
    cfg["companion"]["keepout"] = insets_json({48,48,48,48});
    config::save(path,cfg);
    auto loaded = config::load(path);
    check(parse_insets(loaded["display"]["keepout"])==Insets{10,20,30,40});
    check(parse_insets(loaded["companion"]["keepout"])==Insets{48,48,48,48});
    std::filesystem::remove(path);
    { std::ofstream old(path); old << "[display]\norientation = \"portrait\"\n"; }
    loaded = config::load(path);
    check(loaded["display"]["keepout"]==insets_json({}));
    check(loaded["companion"]["keepout"]==insets_json({}));
    std::filesystem::remove(path);
    check(usable({480,320},{Orientation::Landscape,p,true})==Area{10,20,440,260});
    check(usable({480,320},{Orientation::ReverseLandscape,p,true})==Area{30,40,440,260});
    check(usable({320,480},{Orientation::Portrait,p,true})==Area{40,10,260,440});
    check(usable({320,480},{Orientation::ReversePortrait,p,true})==Area{20,30,260,440});
    check(usable({480,320},{Orientation::Landscape,{1000,1000,1000,1000},false})==Area{0,0,480,320});
    check(usable({320,240},{Orientation::Portrait,{60,80,60,80},false})==Area{80,60,160,120});
    check(usable({480,320},{Orientation::Portrait,{-10,999,0,0},false})==Area{0,0,374,320});
    check(usable({200,120},{Orientation::Portrait,{40,40,40,40},false})==Area{0,0,200,120});
    check(usable({320,240},{Orientation::Portrait,{60,0,61,0},false})==Area{0,0,320,240}); // h=119 only
    check(usable({320,240},{Orientation::Portrait,{0,80,0,81},false})==Area{80,0,160,240}); // edge clamp to 80
    check(usable({319,240},{Orientation::Portrait,{0,80,0,80},false})==Area{0,0,319,240}); // w=159 only
    Area a{40,10,400,280};
    for(auto xy: {std::pair{40,10},std::pair{439,289}}) check(to_content(a,xy.first,xy.second).has_value());
    for(auto xy: {std::pair{39,10},std::pair{440,10},std::pair{40,9},std::pair{40,290}}) check(!to_content(a,xy.first,xy.second));
    Image c(400,280,{1,2,3}); auto f=compose_view(c,{480,320},a,{9,8,7});
    bool pixels=true;
    for(int y=0;y<320;++y) for(int x=0;x<480;++x) pixels &= f.at(x,y)==(to_content(a,x,y)?0xff010203u:0xff090807u);
    check(pixels);  // 153.600 Pixel im ganzen Vollbild vergleichen.
    for (auto size : {std::pair{200,140}, std::pair{450,300}}) {
        Image different(size.first,size.second,{1,2,3});
        auto placed=place(different,{480,320},a,{9,8,7});
        check(compose_view(different,{480,320},a,{9,8,7}).px==placed.px);
        bool exact=true;
        for(int y=0;y<320;++y) for(int x=0;x<480;++x)
            exact &= placed.at(x,y)==(to_content(a,x,y) && x-a.x<size.first && y-a.y<size.second ? 0xff010203u : 0xff090807u);
        check(exact);
    }
    auto local = resolve_tap(a,40,10);
    check(!local.blocked && local.x==0 && local.y==0);
    local = resolve_tap(a,439,289);
    check(!local.blocked && local.x==399 && local.y==279);
    check(compose_view(c,{400,280},{0,0,400,280},{9,8,7}).px==c.px);
    for(auto spec: {std::array<int,3>{480,320,40},std::array<int,3>{2400,1080,0},std::array<int,3>{1920,1080,48}}) {
        auto area=usable({spec[0],spec[1]},{Orientation::Portrait,{0,0,0,spec[2]},false});
        check(area==Area{spec[2],0,spec[0]-spec[2],spec[1]});
        std::vector<touch::Tile> t{{{2,3,100,100}}}; shift_tiles(t,area); check(t[0].box.x==2+spec[2] && t[0].box.y==3);
        check(resolve_tap(area,20,20).blocked==(spec[2]>20));
        check(resolve_tap(area,area.x+area.w/3-1,20).step==-1);
        check(resolve_tap(area,area.x+area.w/3,20).step==1);
        touch::Tile tile{{0,0,area.w,area.h},true,{{{"https://example.org/device-"+std::to_string(spec[2]),""}}}};
        std::vector<touch::Tile> targets{tile};
        shift_tiles(targets,area);
        int row_y = area.y + int(60 * render::scale_for(area.w,area.h));
        const auto* hit = touch::at(targets,area.x+20,row_y);
        check(hit && hit->at(0).url=="https://example.org/device-"+std::to_string(spec[2]));
        check(!touch::at(targets,area.x-1,row_y));

    }
    std::cout<<count<<" checks passed\n"; return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;} }
