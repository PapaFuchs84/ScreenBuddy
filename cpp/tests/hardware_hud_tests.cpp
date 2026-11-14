// Local Hardware HUD checks; synthetic readings are never presented as live telemetry.
#include "render_hardware_hud.h"
#include "sysinfo.h"
#include "config.h"
#include "page_samples.h"
#include "i18n.h"
#include <windows.h>
#include <objbase.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace fl;
namespace fs = std::filesystem;
static int passed=0,failed=0;
#define CHECK(x) do { if(x) ++passed; else {++failed;std::printf("FAIL line %d: %s\n",__LINE__,#x);} } while(0)
static void save(const fs::path& path,const Image& image) {
    const auto bytes=encode_png(image);std::ofstream f(path,std::ios::binary);f.write(bytes.data(),std::streamsize(bytes.size()));
    if(!f) throw std::runtime_error("PNG write failed");
}
int main(int argc,char** argv) {
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);gfx_init();i18n::set_lang("en");
    using namespace sysinfo;
    const auto power=[](uint32_t type,std::string sensor,std::string label,std::string unit,double value,std::string original="") {
        return Reading{type,std::move(sensor),std::move(label),std::move(unit),value,std::move(original)};
    };
    CHECK(!cpu_package_watts({}));
    CHECK(cpu_package_watts({power(SENSOR_POWER,"CPU [#0]: Intel Core","CPU Package Power","W",68)})==68);
    CHECK(cpu_package_watts({power(SENSOR_POWER,"CPU [#0]: AMD Ryzen","CPU PPT","W",88)})==88);
    CHECK(cpu_package_watts({power(SENSOR_POWER,"CPU [#0]: Intel Core","Umbenannt","W",51,"CPU Package Power")})==51);
    CHECK(cpu_package_watts({power(SENSOR_POWER,"CPU","CPU Package Power"," W ",0)})==0);
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"GPU [#0]: NVIDIA","CPU Package Power","W",68)}));
    CHECK(!cpu_package_watts({power(SENSOR_TEMP,"CPU","CPU Package Power","W",68)}));
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"CPU","CPU Package Power","mW",68)}));
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"CPU","CPU Package Power","W",-1)}));
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"CPU","CPU Package Power","W",std::numeric_limits<double>::quiet_NaN())}));
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"CPU","CPU Package Power","W",std::numeric_limits<double>::infinity())}));
    CHECK(!cpu_package_watts({power(SENSOR_POWER,"CPU","IA Cores Power","W",68)}));
    CHECK(cpu_package_watts({power(SENSOR_POWER,"CPU","CPU PPT","W",88),power(SENSOR_POWER,"CPU","CPU Package Power","W",77)})==77);
    auto cfg=config::defaults();CHECK(!cfg["system"]["hardware_hud"].get<bool>());
    CHECK(config::merge(cfg,{{"system",{{"hardware_hud",true}}}})["system"]["hardware_hud"].get<bool>());
    CHECK(cfg["system"]["hardware_hud_fps"]==30);
    for(int fps:{5,10,15,20,30,60})
        CHECK(config::merge(cfg,{{"system",{{"hardware_hud_fps",fps}}}})["system"]["hardware_hud_fps"]==fps);
    CHECK(config::merge(cfg,{{"system",{{"hardware_hud_fps",4}}}})["system"]["hardware_hud_fps"]==5);
    CHECK(config::merge(cfg,{{"system",{{"hardware_hud_fps",61}}}})["system"]["hardware_hud_fps"]==60);
    // Global animation rate keeps old local HUD configurations and patches usable.
    CHECK(cfg["display"]["animation_fps"]==30);
    for(int fps:{5,10,15,20,30,60}) {
        auto merged=config::merge(cfg,{{"display",{{"animation_fps",fps}}}});
        CHECK(merged["display"]["animation_fps"]==fps);
        CHECK(merged["system"]["hardware_hud_fps"]==fps);
    }
    CHECK(config::merge(cfg,{{"display",{{"animation_fps",4}}}})["display"]["animation_fps"]==5);
    CHECK(config::merge(cfg,{{"display",{{"animation_fps",61}}}})["display"]["animation_fps"]==60);
    CHECK(config::merge(cfg,{{"display",{{"animation_fps",1e100}}}})["display"]["animation_fps"]==60);
    const auto migrated=config::merge(cfg,{{"system",{{"hardware_hud_fps",60}}}});
    CHECK(migrated["display"]["animation_fps"]==60);
    CHECK(config::merge(migrated,{{"display",{{"brightness",40}}}})["display"]["animation_fps"]==60);
    ojson bothA;bothA["system"]={{"hardware_hud_fps",60}};bothA["display"]={{"animation_fps",20}};
    ojson bothB;bothB["display"]={{"animation_fps",20}};bothB["system"]={{"hardware_hud_fps",60}};
    CHECK(config::merge(cfg,bothA)["display"]["animation_fps"]==20);
    CHECK(config::merge(cfg,bothB)["display"]["animation_fps"]==20);
    auto legacy=cfg;legacy["display"].erase("animation_fps");legacy["system"]["hardware_hud_fps"]=60;
    CHECK(config::merge(legacy,{{"ui",{{"language","en"}}}})["display"]["animation_fps"]==60);
    const fs::path configRoot=fs::temp_directory_path()/("fl-animation-rate-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    fs::create_directory(configRoot);
    const auto legacyPath=configRoot/"legacy.toml",explicitPath=configRoot/"explicit.toml",savedPath=configRoot/"saved.toml";
    {std::ofstream f(legacyPath);f<<"[system]\nhardware_hud_fps=60\n";}
    CHECK(config::load(legacyPath)["display"]["animation_fps"]==60);
    {std::ofstream f(explicitPath);f<<"[system]\nhardware_hud_fps=60\n[display]\nanimation_fps=15\n";}
    const auto explicitLoad=config::load(explicitPath);
    CHECK(explicitLoad["display"]["animation_fps"]==15);
    CHECK(explicitLoad["system"]["hardware_hud_fps"]==15);
    config::save(savedPath,legacy);
    CHECK(!legacy["display"].contains("animation_fps")); // save normalizes its own copy
    CHECK(config::load(savedPath)["display"]["animation_fps"]==60);
    for(const auto& path:{legacyPath,explicitPath,savedPath}) {std::error_code error;fs::remove(path,error);fs::remove(fs::path(path.wstring()+L".bak"),error);}
    {std::error_code error;fs::remove(configRoot,error);} // empty test-owned directory only
    // Ending an older idle session can remove a row without changing the
    // maximum visible revision: App animation tracking must include row identity.
    ai::Model rowChanges;
    CHECK(rowChanges.ingest(R"({"provider":"codex","session_id":"older","hook_event_name":"SessionStart"})",1800000000));
    CHECK(rowChanges.ingest(R"({"provider":"codex","session_id":"newer","hook_event_name":"SessionStart"})",1800000001));
    const auto rowsBefore=rowChanges.snapshot(1800000001);
    CHECK(rowChanges.ingest(R"({"provider":"codex","session_id":"older","hook_event_name":"SessionEnd"})",1800000002));
    const auto rowsAfter=rowChanges.snapshot(1800000002);
    const auto maxVisibleRevision=[](const ai::Snapshot& snapshot) {
        uint64_t revision=0;for(const auto& row:snapshot.sessions)revision=std::max(revision,row.revision);
        for(const auto& [provider,row]:snapshot.representatives)revision=std::max(revision,row.revision);return revision;
    };
    CHECK(rowsBefore.sessions.size()==2 && rowsAfter.sessions.size()==1);
    CHECK(maxVisibleRevision(rowsBefore)==maxVisibleRevision(rowsAfter));
    auto snap=render::samples::system();snap.cpu_watts=68;snap.gpu_watts=145;snap.gpu_mhz=2100;
    snap.cpu_name="Intel Core i7-12700K / SYNTHETIC SAMPLE";snap.gpu_name="NVIDIA RTX 3070 / SYNTHETIC SAMPLE";
    struct Size {int w,h;};
    const Size sizes[]={{1520,720},{720,1520},{1200,360},{1920,480},{1920,720},{800,480},{480,320},{480,800},{320,480}};
    fs::path out=argc>1?fs::path(argv[1]):fs::path();if(!out.empty())fs::create_directories(out);
    for(const auto size:sizes) for(int page=0;page<3;++page) {
        auto image=render::hardware_hud(size.w,size.h,snap,1+page*8);
        CHECK(image.w==size.w && image.h==size.h && image.px.size()==size_t(size.w)*size.h);
        const auto again=render::hardware_hud(size.w,size.h,snap,1+page*8);CHECK(image.px==again.px);
        if(!out.empty())save(out/("sample-"+std::to_string(size.w)+"x"+std::to_string(size.h)+"-"+std::to_string(page)+".png"),image);
    }
    if(!out.empty()) {
        i18n::set_lang("de");
        for(const auto size:{Size{1520,720},Size{720,1520},Size{1200,360},Size{1920,480},Size{480,800}}) for(int page=0;page<3;++page)
            save(out/("sample-de-"+std::to_string(size.w)+"x"+std::to_string(size.h)+"-"+std::to_string(page)+".png"),render::hardware_hud(size.w,size.h,snap,1+page*8));
        auto longSnapshot=snap;
        longSnapshot.cpu_name="Intel Core Ultra 9 285K / a deliberately very long synthetic CPU description";
        longSnapshot.gpu_name="NVIDIA GeForce RTX 5090 / a deliberately very long synthetic GPU description";
        longSnapshot.cpu_fan=12345;longSnapshot.cpu_watts=999.9;longSnapshot.ram_pct=100;
        longSnapshot.gpu_watts=599.9;longSnapshot.gpu_mem_used=32768;longSnapshot.gpu_mem_total=32768;
        SysSnapshot missing1200;
        for(const char* lang:{"de","en"}) {
            i18n::set_lang(lang);
            for(int page=0;page<3;++page) for(int variant=0;variant<3;++variant) {
                const auto& input=variant==0?snap:variant==1?longSnapshot:missing1200;
                for(int phase=0;phase<2;++phase) {
                    const double tick=page*8+phase*2;
                    save(out/("sample-"+std::string(lang)+"-1200x360-variant"+std::to_string(variant)+"-page"+std::to_string(page)+"-phase"+std::to_string(phase)+".png"),render::hardware_hud(1200,360,input,tick));
                }
            }
        }
        i18n::set_lang("de");
        fs::create_directories(out/"animation");
        for(int frame=0;frame<40;++frame) {
            std::ostringstream name;name<<"frame-"<<std::setfill('0')<<std::setw(3)<<frame<<".png";
            save(out/"animation"/name.str(),render::hardware_hud(1200,360,snap,9+frame*.05));
        }
        i18n::set_lang("en");
    }
    const auto first=render::hardware_hud(800,480,snap,1);
    CHECK(first.px!=render::hardware_hud(800,480,snap,1.3).px);
    CHECK(render::hardware_hud(800,480,snap,9,8).px!=render::hardware_hud(800,480,snap,9,60).px);
    CHECK(render::hardware_hud(800,480,snap,17,8).px!=render::hardware_hud(800,480,snap,17,60).px);
    CHECK(render::hardware_hud(800,480,snap,9,1).px==render::hardware_hud(800,480,snap,9,3).px);
    CHECK(render::hardware_hud(800,480,snap,-1).px==render::hardware_hud(800,480,snap,0).px);
    CHECK(render::hardware_hud(800,480,snap,std::numeric_limits<double>::quiet_NaN()).px==render::hardware_hud(800,480,snap,0).px);
    SysSnapshot missing;for(int page=0;page<3;++page) {
        auto image=render::hardware_hud(1920,480,missing,1+page*8);CHECK(!image.empty());
        if(!out.empty())save(out/("sample-missing-sensors-"+std::to_string(page)+".png"),image);
    }
    auto invalid=snap;invalid.cpu_temp=std::numeric_limits<double>::quiet_NaN();invalid.cpu_watts=-10;invalid.gpu_temp=std::numeric_limits<double>::infinity();
    CHECK(!render::hardware_hud(480,800,invalid,17).empty());

    // Colours: default follows the active design, Neon keeps the classic look, Custom overrides per field.
    {
        const auto rejected=[&](const ojson& patch){ try { (void)config::merge(cfg,patch); return false; } catch(const std::exception&) { return true; } };
        CHECK(cfg["system"]["hud_colors"]=="theme");
        CHECK(cfg["system"]["hud_color_primary"]=="" && cfg["system"]["hud_color_secondary"]=="" && cfg["system"]["hud_color_background"]=="");
        for(const char* mode:{"theme","neon","custom"}) CHECK(config::merge(cfg,{{"system",{{"hud_colors",mode}}}})["system"]["hud_colors"]==mode);
        CHECK(rejected({{"system",{{"hud_colors","rainbow"}}}}));
        CHECK(config::merge(cfg,{{"system",{{"hud_color_primary","#12ab34"}}}})["system"]["hud_color_primary"]=="#12AB34");
        CHECK(config::merge(cfg,{{"system",{{"hud_color_primary",""}}}})["system"]["hud_color_primary"]=="");
        CHECK(rejected({{"system",{{"hud_color_secondary","red"}}}}));
        CHECK(rejected({{"system",{{"hud_color_background","#12345"}}}}));

        render::Theme dark;dark.bg={12,18,32};dark.card={22,32,52};dark.accent={240,140,40};dark.orange={90,170,255};
        dark.text={240,240,235};dark.muted={150,160,175};dark.warn={235,80,70};
        render::Theme light=dark;light.bg={250,244,232};light.card={240,230,212};light.text={20,30,50};light.muted={95,100,110};
        const auto same=[](Color a,Color b){return a==b;};
        const auto neon=render::hud_neon_palette();
        ojson system=cfg["system"];
        system["hud_colors"]="neon";
        const auto n=render::hud_palette(system,dark);
        CHECK(same(n.bg,neon.bg)&&same(n.primary,neon.primary)&&same(n.secondary,neon.secondary)&&same(n.grid,neon.grid)&&same(n.scan,neon.scan));
        // The classic look is unchanged: an explicit Neon palette renders exactly like the default.
        CHECK(render::hardware_hud(800,480,snap,9,8,n).px==render::hardware_hud(800,480,snap,9).px);
        system["hud_colors"]="theme";
        const auto t=render::hud_palette(system,dark);
        CHECK(same(t.bg,dark.bg)&&same(t.primary,dark.accent)&&same(t.secondary,dark.orange)&&same(t.text,dark.text)&&same(t.warn,dark.warn));
        CHECK(render::hardware_hud(800,480,snap,9,8,t).px!=render::hardware_hud(800,480,snap,9).px);
        const auto lt=render::hud_palette(system,light);
        CHECK(same(lt.bg,light.bg)&&render::contrast(lt.text,lt.bg)>=4.5&&render::contrast(lt.muted,lt.bg)>=2.5);
        CHECK(render::contrast(lt.primary,lt.bg)>=1.8&&render::contrast(lt.secondary,lt.bg)>=1.8);
        // Custom: set fields win, empty or invalid fields fall back to the design.
        system["hud_colors"]="custom";system["hud_color_primary"]="#00FF88";system["hud_color_secondary"]="";system["hud_color_background"]="not a colour";
        const auto c=render::hud_palette(system,dark);
        CHECK(same(c.primary,Color{0,255,136})&&same(c.secondary,dark.orange)&&same(c.bg,dark.bg));
        system["hud_color_background"]="#FFFFFF";system["hud_color_primary"]="#FFFFF0";system["hud_color_secondary"]="#FFFFF0";
        const auto low=render::hud_palette(system,dark);
        CHECK(same(low.bg,Color{255,255,255})&&render::contrast(low.text,low.bg)>=4.5);
        CHECK(render::contrast(low.primary,low.bg)>=1.8&&!same(low.primary,low.secondary));
        // Unknown mode behaves like the design default.
        system["hud_colors"]=42;
        const auto fallback=render::hud_palette(system,dark);
        CHECK(same(fallback.primary,dark.accent)&&same(fallback.bg,dark.bg));
        const auto image=render::hardware_hud(800,480,snap,9,8,c);
        CHECK(std::any_of(image.px.begin(),image.px.end(),[](uint32_t p){return ((p>>16)&255)==0&&((p>>8)&255)==255&&(p&255)==136;}));
        if(!out.empty()) for(const auto& [name,palette]:std::vector<std::pair<std::string,render::HudPalette>>{{"theme-dark",t},{"theme-light",lt},{"custom",c}})
            save(out/("sample-colours-"+name+".png"),render::hardware_hud(1200,360,snap,9,8,palette));
    }
    std::printf("Hardware HUD: %d passed, %d failed\n",passed,failed);CoUninitialize();return failed?1:0;
}


