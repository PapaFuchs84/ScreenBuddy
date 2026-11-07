#include "game_detect.h"
#include "game_logo.h"
#include <windows.h>
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace fl::game;
int checks=0;
void check(bool ok){++checks;if(!ok)throw std::runtime_error("game check "+std::to_string(checks));}
int main(){try {
 auto acf=parse_acf(R"("AppState" { "appid" "42" "name" "Example Game" "installdir" "Example" })",L"C:/Steam/steamapps");
 check(acf && acf->steam_id=="42" && acf->name=="Example Game");
 check(!parse_acf(R"("AppState" { "appid" "42" "name" "X" "installdir" "../../escape" })",L"C:/Steam/steamapps"));
 check(!parse_acf("broken",L"C:/Steam/steamapps"));
 check(!parse_acf("\"AppState\" { \"name\" \"unterminated",L"C:/Steam"));
 auto libs=parse_libraries(R"("libraryfolders" { "0" { "path" "C:\\Steam" "apps" { "42" "1" } } "1" "D:\\Games" })");check(libs.size()==2);
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
 std::cout<<checks<<" game checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
