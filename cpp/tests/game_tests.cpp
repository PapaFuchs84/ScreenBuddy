#include "game_detect.h"
#include "game_logo.h"
#include "game_intro.h"
#include "sysinfo.h"
#include "render_game.h"
#include "custom_view_editor.h"
#include "config.h"
#include <windows.h>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fl::game;
int checks=0;
void check(bool ok,std::source_location where=std::source_location::current()){++checks;if(!ok)throw std::runtime_error("game check "+std::to_string(checks)+" at "+where.file_name()+":"+std::to_string(where.line()));}
#include "game_tests/registry_tests.h"
#include "game_tests/protected_process_tests.h"
int main(){try {
 registry_game_checks();
 protected_process_checks();
 auto acf=parse_acf(R"("AppState" { "appid" "42" "name" "Example Game" "installdir" "Example" })",L"C:/Steam/steamapps");
 check(acf && acf->steam_id=="42" && acf->name=="Example Game");
 check(!parse_acf(R"("AppState" { "appid" "42" "name" "X" "installdir" "../../escape" })",L"C:/Steam/steamapps"));
 check(!parse_acf("broken",L"C:/Steam/steamapps"));
 check(!parse_acf("\"AppState\" { \"name\" \"unterminated",L"C:/Steam"));
 auto libs=parse_libraries(R"("libraryfolders" { "0" { "path" "C:\\Steam" "label" "" "apps" { "42" "1" } } "1" "D:\\Games" })");check(libs.size()==2);
 auto epic=parse_epic(R"({"AppName":"id","DisplayName":"Example","InstallLocation":"C:/Games/Example","LaunchExecutable":"bin/game.exe"})");check(epic && epic->name=="Example");
 check(!parse_epic(R"({"AppName":"id","DisplayName":"Example","InstallLocation":"C:/Games/Example","LaunchExecutable":"../../escape.exe"})"));
 check(!parse_epic("[]"));check(!beneath(L"C:/GamesOther/game.exe",L"C:/Games"));
 Game g;g.id="test";g.pid=123;Detector d;
 check(!d.update(g,0) && d.state()==State::starting && !d.active());
 check(!d.update(g,2.99));check(d.update(g,3) && d.active() && d.started()==3);
 check(!d.update({},4) && d.state()==State::ending && d.active());
 check(!d.update(g,8.9) && d.state()==State::running);
 check(!d.update({},9));check(!d.update({},13.99));check(d.update({},14) && !d.active());
 check(!d.update(g,15));check(!d.update({},17));check(d.state()==State::idle);
 g.pid=124;check(!d.update(g,18));Game h=g;h.pid=125;check(!d.update(h,20));check(!d.update(h,22));check(d.update(h,23));
 check(!d.update({},22) && d.state()==State::running); // reject backwards clock
 fl::gfx_init();
 check(LogoCache::fresh(0));check(LogoCache::fresh(604799));check(!LogoCache::fresh(604800));check(!LogoCache::fresh(-1));
 check(LogoCache::urls("../bad").empty());check(LogoCache::urls("42").size()==3);
 const auto dir=std::filesystem::current_path()/L"cpp/build/game-logo-test";
 std::filesystem::create_directories(dir);int calls=0;std::string png=fl::encode_png(fl::Image(16,8,{20,40,60}));
 LogoCache cache(dir,[&](const std::string&){++calls;return png;});Game logo;logo.steam_id="42";
 check(bool(cache.load(logo)) && calls==1);check(bool(cache.load(logo)) && calls==1);
 std::filesystem::last_write_time(dir/L"42.png",std::filesystem::file_time_type::clock::now()-std::chrono::hours(169));
 check(bool(cache.load(logo)) && calls==2);
 {std::ofstream f(dir/L"42.png",std::ios::binary);f<<"corrupt";}
 check(bool(cache.load(logo)) && calls==3);
 LogoCache failed(dir,[](const std::string&){return std::string();});logo.steam_id="43";check(!failed.load(logo));
 LogoCache fallback(dir,[&](const std::string& url){++calls;return url.ends_with("header.jpg")?png:std::string();});
 check(bool(fallback.load(logo)) && calls==5);
 logo.steam_id="../../escape";check(!cache.load(logo) && calls==5);
 std::filesystem::remove(dir/L"42.png");std::filesystem::remove(dir/L"43.png");std::filesystem::remove(dir);
 check(!intro_phase(0,false).done && intro_phase(0,false).reveal==0);
 check(intro_phase(2.5,false).done && intro_phase(2.5,false).fade==1);
 check(!intro_phase(.34,true).done && intro_phase(.35,true).done);
 check(intro_phase(.1,true).scale==1 && intro_phase(.1,true).shine==-1);
 fl::Image previous(160,100,{10,20,30}),target(160,100,{40,50,60});Intro intro;intro.begin(previous,10,false);
 check(intro.active(12.49));check(!intro.active(12.5));check(intro.draw(target,{},"Example",12.5).px==target.px);
 intro.begin(previous,20,true);check(intro.draw(target,{},"Example",20).px==previous.px);intro.reset();check(!intro.active(20));
 fl::sysinfo::FrameHistory frames;check(!frames.stats(0).fps);frames.present(0);check(!frames.stats(0).fps);
 for(int i=1;i<=120;++i)frames.present(i/60.);auto stats=frames.stats(2);check(stats.fps && std::abs(*stats.fps-60)<.01);check(stats.low && std::abs(*stats.low-60)<.01);check(stats.history.size()==120);
 frames.present(1);check(frames.stats(2).history.size()==120);check(!frames.stats(4.01).fps);frames.present(5);check(!frames.stats(5).fps);frames.present(5.02);check(frames.stats(5.02).fps && !frames.stats(5.02).low);
 for(const auto& block:fl::builtin::find("system")->blocks)if(block.tile_only){check(metric_tile(block.id));fl::custom::Layout layout;fl::custom::Tile tile;tile.source="system";tile.block=block.id;tile.zone={"tile",0,0,1,1};check(bool(fl::custom::single_builtin(tile,layout)));check(!draw_metric(tile,layout,480,320,fl::SysSnapshot{}).empty());}
 check(metric_values(fl::SysSnapshot{}).at("fps").is_null());
 auto templ=fl::custom::game_mode_template("test-game");check(bool(fl::custom::parse(fl::custom::to_json(templ).dump())));check(templ.standard.tiles.size()==8);check(templ.standard.tiles[1].block=="system.fps" && templ.standard.tiles[1].zone.row_span==2);
 fl::sysinfo::MetricHistory history;fl::SysSnapshot readings;
 for(int i=0;i<75;++i){readings.fps=60+i;readings.frametime_ms=1000./(60+i);history.sample(readings,123,i);}
 check(readings.fps_hist.size()==60 && readings.fps_hist.front()==75 && readings.fps_hist.back()==134);
 check(readings.frametime_hist.size()==60);
 readings.fps.reset();readings.frametime_ms.reset();history.sample(readings,123,77);
 check(std::isnan(readings.fps_hist.back()) && std::isnan(readings.fps_hist[58]));
 history.sample(readings,124,78);check(readings.fps_hist.size()==1);
 const auto package=std::filesystem::current_path()/L"cpp/build/game-package-test";
 std::filesystem::create_directories(package/L"Assets");
 {std::ofstream f(package/L"AppxManifest.xml");f<<R"(<Package><Applications><Application Executable="game.exe"><uap:VisualElements Square44x44Logo="Assets/logo.png"/></Application></Applications></Package>)";}
 for(auto size:{44,256}){auto bytes=fl::encode_png(fl::Image(size,size,{20,40,60}));std::ofstream f(package/L"Assets"/(size==44?L"logo.png":L"logo.targetsize-256.png"),std::ios::binary);f.write(bytes.data(),bytes.size());}
 auto exe_icon=executable_icon(std::filesystem::current_path()/L"cpp/build/screenbuddy-release/ScreenBuddy.exe");check(exe_icon && exe_icon->w==256);
 for(auto id:{"gog:example","ubisoft:example"}){Game local;local.id=id;local.executable=std::filesystem::current_path()/L"cpp/build/screenbuddy-release/ScreenBuddy.exe";check(bool(cache.load(local)));}
 Game packaged;packaged.id="xbox:example";packaged.executable=package/L"game.exe";check(bool(cache.load(packaged)));
 auto asset=package_logo(package/L"game.exe");check(asset && asset->w==256);check(!package_logo(package/L"other.exe"));
 std::filesystem::remove_all(package);
 fl::custom::Layout metric_layout;fl::custom::Tile metric;metric.source="system";metric.zone={"metric",0,0,1,1};
 readings.fps=120;readings.fps_hist={90,120,110};metric.block="system.fps";
 auto with_history=draw_metric(metric,metric_layout,480,320,readings);readings.fps_hist.clear();
 check(with_history.px!=draw_metric(metric,metric_layout,480,320,readings).px);
 {const auto& th=fl::render::theme();check(level_color(th,50)==th.accent);const fl::Color warn{255,179,0},hot{255,77,61};check(level_color(th,80)==warn);check(level_color(th,89.9)==warn);check(level_color(th,90)==hot);check(level_color(th,100)==hot);
  check(upper_ascii("Frametime · Verlauf")=="FRAMETIME · VERLAUF");check(mix_color({0,0,0},{100,200,50},.5f)==fl::Color{50,100,25});check(mix_color({0,0,0},{10,10,10},2.f)==fl::Color{10,10,10});
  // Ring (quadratische Kachel, GPU) und Balken-Rueckfall (breite Kachel) liefern verschiedene Bilder; hoher Wert aendert Farbe/Winkel.
  fl::custom::Tile gpu=metric;gpu.block="system.gpu";readings.gpu_pct=50;auto low=draw_metric(gpu,metric_layout,480,320,readings);readings.gpu_pct=95;check(low.px!=draw_metric(gpu,metric_layout,480,320,readings).px);
  readings.fps=0;auto unit_img=draw_metric(metric,metric_layout,480,320,readings);check(!unit_img.empty());}
 {using fl::sysinfo::Reading;std::vector<Reading> r={{8,"PresentMon","Framerate Presented (avg)","FPS",143.5},{8,"PresentMon","Framerate Presented (1% low)","FPS",120.25},{8,"PresentMon","Frame Time Presented (avg)","ms",6.97},{8,"PresentMon","Framerate Displayed (avg)","FPS",60},{8,"CPU [#0]: AMD","Framerate Presented (avg)","FPS",1}};
  auto v=fl::sysinfo::presentmon_values(r);check(v.fps && *v.fps==143.5);check(v.low && *v.low==120.25);check(v.frametime_ms && *v.frametime_ms==6.97);
  r[0].value=-1;r[2].value=std::nan("");auto bad=fl::sysinfo::presentmon_values(r);check(!bad.fps && !bad.frametime_ms && bad.low);   // invalid/negative values are ignored, other sensor groups are never used
  check(!fl::sysinfo::presentmon_values({}).fps);
  fl::SysSnapshot hw;hw.fps=90;hw.fps_reason="hwinfo_presentmon";fl::custom::Layout hl;fl::custom::Tile ht;ht.source="system";ht.block="system.fps";ht.zone={"hw",0,0,1,1};hw.fps_hist={80,90};auto hi=draw_metric(ht,hl,480,320,hw);hw.fps_reason="dxgi_present_calls";check(hi.px!=draw_metric(ht,hl,480,320,hw).px);}
 {using fl::sysinfo::Reading;std::vector<Reading> loc={{8,"PresentMon [bf6.exe]","Bildrate Presented (avg)","FPS",100.5,"Framerate Presented (avg)"},{8,"PresentMon [bf6.exe]","Bildrate Presented (1% low)","FPS",80,"Framerate Presented (1% low)"},{8,"PresentMon [claude.exe]","Bildrate Presented (avg)","FPS",129,"Framerate Presented (avg)"}};
  auto mine=fl::sysinfo::presentmon_values(loc,"BF6.exe");check(mine.fps && *mine.fps==100.5 && mine.low && *mine.low==80);   // localized labels, only the detected game group
  auto other=fl::sysinfo::presentmon_values(loc,"nothere.exe");check(!other.fps);check(fl::sysinfo::presentmon_values(loc).fps.has_value());}
 {fl::SysSnapshot g;g.frametime_ms=16;fl::custom::Layout gl;fl::custom::Tile gt;gt.source="system";gt.block="system.frametime";gt.zone={"g",0,0,1,1};auto none=draw_metric(gt,gl,480,320,g);
  g.frametime_hist={16,std::nan(""),18,std::nan(""),std::nan(""),17,20};auto gaps=draw_metric(gt,gl,480,320,g);check(gaps.px!=none.px);   // gaps are bridged into one line, no crash
  g.frametime_hist={std::nan(""),std::nan("")};check(draw_metric(gt,gl,480,320,g).px==none.px);
  // start-up outlier must not flatten the line: with the outlier the image equals the one where the outlier is replaced by the window maximum
  g.frametime_hist.clear();for(int i=0;i<40;++i)g.frametime_hist.push_back(20+(i%5));auto flat_ref=g.frametime_hist;g.frametime_hist[0]=5000;auto with_outlier=draw_metric(gt,gl,480,320,g);g.frametime_hist=flat_ref;g.frametime_hist[0]=24;auto bounded=draw_metric(gt,gl,480,320,g);check(with_outlier.px==bounded.px);}   // fewer than two valid samples draw nothing
 auto cfg=fl::config::defaults();check(cfg["game_mode"]["enabled"]==false);
 auto configured=fl::config::merge(cfg,{{"game_mode",{{"enabled",true},{"view","custom:test-game"},{"ping_target","127.0.0.1"},{"additional_games","C:\\Games\\Example.exe\nD:\\Games\\Other.exe"}}}});
 check(configured["game_mode"]["view"]=="custom:test-game");check(configured["game_mode"]["additional_games"].get<std::string>().find('\n')!=std::string::npos);
 for(const auto& patch:std::vector<fl::ojson>{{{"game_mode",{{"view","../bad"}}}},{{"game_mode",{{"ping_target","999.1.1.1"}}}},{{"game_mode",{{"additional_games","relative.exe"}}}}}){bool rejected=false;try{fl::config::merge(cfg,patch);}catch(...){rejected=true;}check(rejected);}
 check(stats.intervals==120);
 if(const char* out=std::getenv("FL_GAME_PREVIEW_DIR")){   // developer aid: cockpit gallery + margin/centring measurements, not part of the checks
  namespace fs=std::filesystem;fs::create_directories(out);fl::render::set_theme("dunkel",{{"bg","#131118"},{"card","#201f26"},{"accent","#f57c00"},{"text","#e8e4ee"},{"muted","#a29cab"}});
  auto make=[](double fps,double gpu,double cpu,double ft,double ping,double used){fl::SysSnapshot s;s.fps=fps;s.gpu_pct=gpu;s.cpu_pct=cpu;s.frametime_ms=ft;s.ping_ms=ping;s.gpu_mem_used=used;s.gpu_mem_total=8188;
   for(int i=0;i<60;++i){s.fps_hist.push_back(fps+8*std::sin(i/4.0));s.frametime_hist.push_back(ft+ft*.12*std::sin(i/5.0));}return s;};
  struct Scene{std::string name;int w,h;fl::SysSnapshot s;std::string game;double elapsed;};
  std::vector<Scene> scenes={{"bf6-redmi",1520,720,make(49,99,47,20.5,8,7769),"Battlefield 6",39},{"digits3-redmi",1520,720,make(144,100,100,6.9,128.5,8100),"Elden Ring",12345},
   {"digits1-redmi",1520,720,make(7,5,3,142.9,1,900),"Hades",5},{"longname-redmi",1520,720,make(60,80,90,16.7,12,4000),"The Elder Scrolls V: Skyrim Special Edition Anniversary Edition",3600},
   {"na-redmi",1520,720,fl::SysSnapshot{},"",0},{"bf6-480",480,320,make(49,99,47,20.5,8,7769),"Battlefield 6",39},{"digits3-480",480,320,make(144,100,100,6.9,128.5,8100),"Elden Ring",12345}};
  fl::Image logo_image(256,256,fl::Color{167,139,216});auto logo=std::make_shared<const fl::Image>(logo_image);
  auto templ=fl::custom::game_mode_template("preview");const auto& th=fl::render::theme();
  auto close_to=[](uint32_t px,fl::Color col,int tol){int r=(px>>16)&255,g=(px>>8)&255,b=px&255;return std::abs(r-col.r)<=tol&&std::abs(g-col.g)<=tol&&std::abs(b-col.b)<=tol;};
  for(const auto& sc:scenes){
   fl::Image composite;
   for(const auto& tile:templ.standard.tiles){
    auto img=draw_metric(tile,templ.standard,sc.w,sc.h,sc.s,sc.game,sc.elapsed,logo);const uint32_t bg=img.px[0];
    if(composite.empty())composite=img;else for(size_t i=0;i<img.px.size();++i)if(img.px[i]!=bg)composite.px[i]=img.px[i];
    int x0=img.w,y0=img.h,x1=-1,y1=-1;for(int y=0;y<img.h;++y)for(int x=0;x<img.w;++x)if(img.at(x,y)!=bg){x0=std::min(x0,x);y0=std::min(y0,y);x1=std::max(x1,x);y1=std::max(y1,y);}
    auto ink=[&](fl::Color col,int ya,int yb,int& ix0,int& iy0,int& ix1,int& iy1){ix0=img.w;iy0=img.h;ix1=-1;iy1=-1;for(int y=std::max(ya,y0+6);y<std::min(yb,y1-5);++y)for(int x=x0+10;x<x1-8;++x)if(close_to(img.at(x,y),col,24)){ix0=std::min(ix0,x);iy0=std::min(iy0,y);ix1=std::max(ix1,x);iy1=std::max(iy1,y);}};
    int lx0,ly0,lx1,ly1,vx0,vy0,vx1,vy1;ink(th.muted,y0,y0+(y1-y0)/4,lx0,ly0,lx1,ly1);ink(th.text,ly1+1,y1,vx0,vy0,vx1,vy1);
    if(vx1>=0)std::printf("%-16s %-18s card %4d,%4d-%4d,%4d | label x %4d..%4d bottom %4d | value x %4d..%4d y %4d..%4d | L %3d R %3d (dx %+d) above %3d below %3d\n",sc.name.c_str(),tile.block.c_str(),x0,y0,x1,y1,lx0,lx1,ly1,vx0,vx1,vy0,vy1,vx0-x0,x1-vx1,(vx0+vx1)/2-(x0+x1)/2,vy0-ly1,y1-vy1);
   }
   std::ofstream(fs::path(out)/(sc.name+".png"),std::ios::binary)<<fl::encode_png(composite);
  }
 }
 std::cout<<checks<<" game checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
