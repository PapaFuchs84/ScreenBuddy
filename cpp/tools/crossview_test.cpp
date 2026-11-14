// Local cross-view device harness. Synthetic samples only; never reads accounts or changes production settings.
#include <windows.h>
#include <objbase.h>
#include "config.h"
#include "render.h"
#include "render_game.h"
#include "custom_view.h"
#include "page_samples.h"
#include "output.h"
#include "i18n.h"
#include "util.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
using namespace fl;
namespace fs=std::filesystem;
namespace {
SysSnapshot sample_system(double tick) {
    auto s=render::samples::system();s.cpu_name="BEISPIEL / CPU";s.gpu_name="BEISPIEL / GPU";
    s.cpu_pct=45+30*std::sin(tick*2);s.gpu_pct=55+35*std::cos(tick*1.7);
    s.fps=65+12*std::sin(tick*1.8);s.frametime_ms=1000. / *s.fps;s.fps_low=52;s.fps_reason="sample";
    for(int i=0;i<60;++i)s.frametime_hist.push_back(15+4*std::sin(tick*2+i*.3));return s;
}
std::vector<float> levels(double tick) {
    std::vector<float> v(16);for(int i=0;i<16;++i)v[i]=float(.12+.83*(.5+.5*std::sin(tick*5+i*.71)));return v;
}
Image frame(const std::string& kind,int w,int h,double tick) {
    auto sys=sample_system(tick);auto sp=render::samples::spotify();sp.title="SYNTHETISCHE TESTDATEN";sp.artist="Kein Spotify-Konto / keine Kontosteuerung";
    sp.progress_ms=int(std::fmod(83000+tick*1000,187000));sp.fetched_at=mono();auto bars=levels(tick);
    auto ai=render::samples::ai_status();for(auto& s:ai.sessions)s.model="LOCAL_TEST_SAMPLE";for(auto& [key,s]:ai.representatives)s.model="LOCAL_TEST_SAMPLE";constexpr std::time_t clock=render::samples::CLOCK;
    if(kind=="system")return render::SysRenderer(w,h,"System / BEISPIELDATEN","bit").render(sys,clock);
    if(kind=="spotify")return render::SpotifyRenderer(w,h,"Spotify / BEISPIELDATEN").render(sp,tick,&bars,1,true,clock);
    if(kind=="ai")return render::AiRenderer(w,h).render(ai,tick,clock);
    custom::View view;view.id="crossview-samples";view.name="SYNTHETISCHE TESTDATEN";view.standard.columns={1,1};view.standard.rows={1,1};
    for(int i=0;i<4;++i) {
        custom::Tile t;t.source=kind=="game"?"system":i==0?"system":i==1?"spotify":"ai";
        if(kind=="game") {const char* ids[]={"system.fps","system.frametime","system.game_name","system.game_time"};t.block=ids[i];}
        else t.block=i==0?"system.cpu":i==1?"spotify.pegel":i==2?"ai.codex":"ai.claude";
        t.zone={"tile"+std::to_string(i),i%2,i/2,1,1};view.standard.tiles.push_back(t);
    }
    return custom::draw(view,"",w,h,[&](const custom::Tile& tile,const custom::Layout& layout,int tw,int th)->std::optional<Image> {
        if(kind=="game")return game::draw_metric(tile,layout,tw,th,sys,"LOCAL_TEST_GAME",tick);
        auto overlay=custom::single_builtin(tile,layout);if(!overlay)return {};
        if(tile.source=="system")return render::SysRenderer(tw,th,"BEISPIELDATEN","bit").render_arranged(sys,*overlay,clock).image;
        if(tile.source=="spotify")return render::SpotifyRenderer(tw,th,"BEISPIELDATEN").render_arranged(sp,*overlay,tick,&bars,1,true,clock).image;
        return render::AiRenderer(tw,th).render_arranged(ai,*overlay,tick,clock).image;
    });
}
void save_png(const fs::path& path,const Image& image) {
    auto bytes=encode_png(image);std::ofstream f(path,std::ios::binary);f.write(bytes.data(),std::streamsize(bytes.size()));
}
}
int main(int argc,char** argv) try {
    std::string serial,store,kind="spotify";int seconds=20,fps=30,port=18771,settings=18775;bool animated_metadata=false;fs::path output="crossview-output";
    for(int i=1;i<argc;++i) {
        std::string a=argv[i];auto next=[&](){if(i+1>=argc)throw std::runtime_error("Missing argument");return std::string(argv[++i]);};
        if(a=="--animated-metadata")animated_metadata=true;else if(a=="--serial")serial=next();else if(a=="--companion")store=next();else if(a=="--case")kind=next();else if(a=="--seconds")seconds=std::stoi(next());else if(a=="--producer-fps")fps=std::stoi(next());else if(a=="--port")port=std::stoi(next());else if(a=="--settings-port")settings=std::stoi(next());else if(a=="--output")output=fs::path(next());else throw std::runtime_error("Unknown argument");
    }
    if((serial.empty()==store.empty()) || seconds<1 || seconds>600 || fps<1 || fps>60 || (kind!="system"&&kind!="spotify"&&kind!="ai"&&kind!="mixed"&&kind!="game"&&kind!="rotate"))throw std::runtime_error("Select serial OR companion; valid case, 1..60 producer fps, 1..600 seconds");
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);gfx_init();i18n::set_lang("de");render::apply_theme(config::defaults()["display"]);fs::create_directories(output);
    std::unique_ptr<SerialDisplay> usb;std::unique_ptr<NetworkDisplay> net;std::mutex sizes_mu;std::map<std::string,std::pair<int,int>> sizes;std::atomic<int> offset{0};
    if(!serial.empty()) {usb=std::make_unique<SerialDisplay>(serial,Orientation::Landscape,50,false);usb->connect();}
    else {
        net=std::make_unique<NetworkDisplay>(port,settings,fs::path(store),480,320);net->set_independent(true);
        net->on_device_resize=[&](const std::string& id,int w,int h){std::lock_guard lock(sizes_mu);sizes[id]={w,h};};
        net->on_device_swipe=[&](const std::string&,int d){offset+=d;};net->connect();
        auto s=net->status();ojson public_status={{"running",s.value("running",false)},{"addresses",s["addresses"]},{"fingerprint",s["fingerprint"]},{"sample_data",true},{"static_metadata_fps",0},{"animated_metadata_enabled",animated_metadata},{"producer_fps",fps},{"case",kind}};
        std::ofstream(output/"server.private.json")<<public_status.dump(2);
    }
    const double start=mono();double next=start;size_t shows=0;long long rows=0;double show_ms=0;std::string saved;
    const std::vector<std::string> kinds={"system","spotify","ai","mixed","game"};
    while(mono()-start<seconds) {
        const double tick=mono()-start;std::string selected=kind;
        if(kind=="rotate")selected=kinds[((int(tick/4)+offset.load())%4+4)%4];
        if(usb) {
            auto [w,h]=usb->size();auto image=frame(selected,w,h,tick);if(saved!=selected){save_png(output/(selected+"-"+std::to_string(w)+"x"+std::to_string(h)+"-SAMPLES.png"),image);saved=selected;}
            const auto before=mono();rows+=usb->show(image);show_ms+=(mono()-before)*1000;++shows;
        } else {
            for(const auto& id:net->client_ids()) {
                std::pair<int,int> dimensions{480,320};{std::lock_guard lock(sizes_mu);if(sizes.contains(id))dimensions=sizes.at(id);}
                auto [w,h]=dimensions;auto image=frame(selected,w,h,tick);
                if(saved!=selected){save_png(output/(selected+"-"+std::to_string(w)+"x"+std::to_string(h)+"-SAMPLES.png"),image);saved=selected;}
                // Explicit test-only classification: static system stays ordinary; animated samples may use trusted producer pacing.
                net->show_for(id,image,{{"current",ojson::array({"sample:"+selected})}},animated_metadata&&selected!="system"?fps:0);++shows;
            }
        }
        next=std::max(next+1./fps,mono());const double rest=next-mono();if(rest>0)Sleep(DWORD(std::max(1.,std::ceil(rest*1000))));
    }
    const double elapsed=mono()-start;ojson result={{"mode",usb?"serial":"companion"},{"case",kind},{"synthetic_samples",true},{"producer_fps",fps},{"static_metadata_fps",0},{"animated_metadata_enabled",animated_metadata},{"seconds",elapsed},{"show_calls",shows},{"rows_transferred",rows},{"mean_show_ms",shows?show_ms/shows:0.}};
    if(usb){auto st=usb->transfer();result["last_transfer_s"]=st.last_seconds;if(st.bytes_per_second)result["estimated_bytes_per_second"]=*st.bytes_per_second;usb->close();}
    if(net)net->close();std::ofstream(output/"result.json")<<result.dump(2);std::cout<<result.dump()<<"\n";return 0;
} catch(const std::exception& e){std::cerr<<"Local sample harness failed: "<<e.what()<<"\n";return 1;}
