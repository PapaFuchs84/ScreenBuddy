// © 2026 FuchsLabs
#include "game_runtime.h"
#include "util.h"
#include <future>
#include <windows.h>
#include <objbase.h>
namespace fl::game {
Runtime::Runtime(std::filesystem::path cache,std::string additional,std::string excluded,std::function<void(uint32_t)> changed):cache_(std::move(cache)),discovery_(additional,excluded),changed_(std::move(changed)){}
Runtime::~Runtime(){stop();}
void Runtime::start(){if(thread_.joinable())return;stop_=false;thread_=std::thread([this]{run();});}
void Runtime::stop(){stop_=true;if(thread_.joinable())thread_.join();}
Snapshot Runtime::snapshot()const{std::lock_guard lock(mutex_);return snapshot_;}
void Runtime::run(){
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    std::future<ImagePtr> job;uint64_t job_generation=0,requested=0;
    while(!stop_){
        try {
            auto previous=detector_.active();const auto now=mono();
            if(detector_.update(discovery_.poll(previous,now),now)) {
                auto active=detector_.active();auto icon=active?executable_icon(active->executable):ImagePtr{};
                {std::lock_guard lock(mutex_);snapshot_.game=active;snapshot_.logo=icon;snapshot_.started=detector_.started();++snapshot_.generation;}
                if(changed_)changed_(active?active->pid:0);
            }
            if(job.valid() && job.wait_for(std::chrono::seconds(0))==std::future_status::ready){
                auto logo=job.get();uint32_t pid=0;bool publish=false;
                {std::lock_guard lock(mutex_);if(snapshot_.game && snapshot_.generation==job_generation){snapshot_.logo=logo;pid=snapshot_.game->pid;publish=true;}}
                if(publish && changed_)changed_(pid);
            }
            auto snap=snapshot();
            if(snap.game && requested!=snap.generation && !job.valid()) {
                requested=snap.generation;job_generation=snap.generation;
                job=std::async(std::launch::async,[cache=cache_,game=*snap.game]{CoInitializeEx(nullptr,COINIT_MULTITHREADED);ImagePtr image;try{image=LogoCache(cache).load(game);}catch(...){}CoUninitialize();return image;});
            }
        }catch(...){} // unavailable launcher/files/processes never terminate the display
        for(int i=0;i<10 && !stop_;++i)Sleep(50);
    }
    if(job.valid())job.wait();CoUninitialize();
}
}
