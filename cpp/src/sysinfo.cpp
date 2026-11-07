// © 2026 FuchsLabs
#include "sysinfo.h"

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#include <powrprof.h>
#include <dxgi1_4.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <cmath>
#include <numeric>
#include <limits>
#include <cstdlib>

#include <algorithm>
#include <cstring>
#include <regex>

#include "util.h"

namespace fl::sysinfo {

// ---------- Netzwerk ----------
static std::optional<uint32_t> default_interface() {
    // ifIndex des Adapters, über den Internet-Verkehr läuft (wie Windows' eigene Routenwahl)
    DWORD idx = 0;
    IPAddr dest = 0x01010101;  // 1.1.1.1 (network byte order: 1.1.1.1)
    if (GetBestInterface(dest, &idx) == NO_ERROR) return idx;
    return std::nullopt;
}

template <class F>
static void for_each_if(F f) {
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR) return;
    for (ULONG i = 0; i < table->NumEntries; ++i) f(table->Table[i]);
    FreeMibTable(table);
}

static std::optional<uint32_t> interface_index(const std::string& name) {
    // ifIndex zu einem Adapternamen (z. B. "Skynet") oder einem Teil der Beschreibung
    std::string want = casefold(name);
    std::optional<uint32_t> hit;
    for_each_if([&](const MIB_IF_ROW2& r) {
        if (hit) return;
        if (casefold(narrow(r.Alias)) == want || casefold(narrow(r.Description)).find(want) != std::string::npos)
            hit = r.InterfaceIndex;
    });
    return hit;
}

ojson list_interfaces() {
    // Aktive Netzwerkadapter (ohne Loopback und virtuelle Hilfsadapter)
    static const std::regex skip("WAN Miniport|Filter|Scheduler|Extension Adapter|Loopback|Pseudo");
    ojson out = ojson::array();
    std::vector<std::string> seen;
    for_each_if([&](const MIB_IF_ROW2& r) {
        std::string alias = narrow(r.Alias), desc = narrow(r.Description);
        if (r.OperStatus != IfOperStatusUp || r.Type == IF_TYPE_SOFTWARE_LOOPBACK || alias.empty()) return;
        if (std::regex_search(desc + " " + alias, skip)) return;
        if (std::count(seen.begin(), seen.end(), alias)) return;
        seen.push_back(alias);
        out.push_back({{"name", alias}, {"description", desc}});
    });
    return out;
}

// ---------- CPU ----------
std::pair<std::string, int> cpu_name() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &k))
        return {"CPU", 0};
    wchar_t buf[256] = {};
    DWORD sz = sizeof buf, mhz = 0, msz = sizeof mhz;
    RegQueryValueExW(k, L"ProcessorNameString", nullptr, nullptr, (BYTE*)buf, &sz);
    RegQueryValueExW(k, L"~MHz", nullptr, nullptr, (BYTE*)&mhz, &msz);
    RegCloseKey(k);
    static const std::regex rx(
        R"(\(R\)|\(TM\)|^AMD |^Intel |\s+\d+-Core.*$|\s+Processor.*$|\s+with .*$|\s+CPU\s+@.*$)");
    std::string name = std::regex_replace(trim(narrow(buf)), rx, "");
    name = collapse_spaces(name);
    return {name.empty() ? "CPU" : name, (int)mhz};
}

struct SysMonitor::Pdh {
    PDH_HQUERY q = nullptr;
    PDH_HCOUNTER cpu = nullptr, perf = nullptr, cores = nullptr, engines = nullptr, memory = nullptr;
    Pdh() {
        if (PdhOpenQueryW(nullptr, 0, &q) != ERROR_SUCCESS) { q = nullptr; return; }
        // wie im Task-Manager ("% Processor Utility" statt Leerlaufzeit)
        if (PdhAddEnglishCounterW(q, L"\\Processor Information(_Total)\\% Processor Utility", 0, &cpu) != ERROR_SUCCESS)
            cpu = nullptr;
        if (PdhAddEnglishCounterW(q, L"\\Processor Information(_Total)\\% Processor Performance", 0, &perf) != ERROR_SUCCESS)
            perf = nullptr;
        if(PdhAddEnglishCounterW(q,L"\\Processor Information(*)\\% Processor Utility",0,&cores)!=ERROR_SUCCESS)cores=nullptr;
        if(PdhAddEnglishCounterW(q,L"\\GPU Engine(*)\\Utilization Percentage",0,&engines)!=ERROR_SUCCESS)engines=nullptr;
        if(PdhAddEnglishCounterW(q,L"\\GPU Adapter Memory(*)\\Dedicated Usage",0,&memory)!=ERROR_SUCCESS)memory=nullptr;
        PdhCollectQueryData(q);
    }
    ~Pdh() {
        if (q) PdhCloseQuery(q);
    }
    std::vector<std::pair<std::wstring,double>> values(PDH_HCOUNTER counter) {
        std::vector<std::pair<std::wstring,double>> out;if(!counter)return out;DWORD size=0,count=0;
        if(PdhGetFormattedCounterArrayW(counter,PDH_FMT_DOUBLE,&size,&count,nullptr)!=PDH_MORE_DATA || size>4*1024*1024)return out;
        std::vector<unsigned char> buffer(size);
        auto items=reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if(PdhGetFormattedCounterArrayW(counter,PDH_FMT_DOUBLE,&size,&count,items)!=ERROR_SUCCESS)return out;
        for(DWORD i=0;i<count;++i)if((items[i].FmtValue.CStatus==PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus==PDH_CSTATUS_NEW_DATA) && std::isfinite(items[i].FmtValue.doubleValue))out.emplace_back(items[i].szName,items[i].FmtValue.doubleValue);
        return out;
    }
    std::optional<double> value(PDH_HCOUNTER c) {
        if (!c) return std::nullopt;
        PDH_FMT_COUNTERVALUE v{};
        if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &v) != ERROR_SUCCESS)
            return std::nullopt;
        if(v.CStatus!=PDH_CSTATUS_VALID_DATA && v.CStatus!=PDH_CSTATUS_NEW_DATA)return {};
        return std::isfinite(v.doubleValue)?std::optional<double>(v.doubleValue):std::nullopt;
    }
};

// ---------- HWiNFO ----------
std::optional<std::vector<Reading>> hwinfo_readings() {
    // Nur öffnen, nie anlegen
    HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Global\\HWiNFO_SENS_SM2");
    if (!h) return std::nullopt;
    std::optional<std::vector<Reading>> result;
    const uint8_t* view = (const uint8_t*)MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
    if (view) {
        MEMORY_BASIC_INFORMATION mbi{};
        size_t avail = VirtualQuery(view, &mbi, sizeof mbi) ? mbi.RegionSize : 0;
        auto u32 = [&](size_t o) { uint32_t v; memcpy(&v, view + o, 4); return v; };
        if (avail >= 44 && u32(0) == 0x53695748) {  // "HWiS"; "DEAD" = HWiNFO beendet
            uint32_t s_off = u32(20), s_size = u32(24), s_num = u32(28), r_off = u32(32), r_size = u32(36), r_num = u32(40);
            uint64_t need = std::max<uint64_t>((uint64_t)s_off + (uint64_t)s_size * s_num, (uint64_t)r_off + (uint64_t)r_size * r_num);
            if (need <= avail && s_size >= 264 && r_size >= 292) {
                auto cstr = [&](size_t o, size_t n) {
                    std::wstring w;
                    for (size_t i = 0; i < n && view[o + i]; ++i) w += (wchar_t)view[o + i];  // Latin-1
                    return narrow(w);
                };
                std::vector<std::string> sensors;
                for (uint32_t i = 0; i < s_num; ++i) {
                    size_t o = s_off + (size_t)i * s_size;
                    std::string n = cstr(o + 136, 128);
                    sensors.push_back(n.empty() ? cstr(o + 8, 128) : n);
                }
                std::vector<Reading> out;
                for (uint32_t i = 0; i < r_num; ++i) {
                    size_t o = r_off + (size_t)i * r_size;
                    Reading r;
                    r.type = u32(o);
                    uint32_t sidx = u32(o + 4);
                    r.label = cstr(o + 140, 128);
                    if (r.label.empty()) r.label = cstr(o + 12, 128);
                    r.unit = cstr(o + 268, 16);
                    memcpy(&r.value, view + o + 284, 8);
                    r.sensor = sidx < sensors.size() ? sensors[sidx] : "";
                    out.push_back(std::move(r));
                }
                result = std::move(out);
            }
        }
        UnmapViewOfFile(view);
    }
    CloseHandle(h);
    return result;
}

static std::optional<double> pick(const std::vector<Reading>& readings, uint32_t type, std::vector<std::string> wanted,
                                  const std::string& sensor_hint = "") {
    wanted.erase(std::remove(wanted.begin(), wanted.end(), std::string()), wanted.end());
    std::vector<const Reading*> cands;
    std::string hint = casefold(sensor_hint);
    for (auto& r : readings)
        if (r.type == type && (hint.empty() || casefold(r.sensor).find(hint) != std::string::npos)) cands.push_back(&r);
    for (auto& w : wanted)
        for (auto* r : cands)
            if (casefold(r->label) == casefold(w)) return r->value;
    for (auto& w : wanted)
        for (auto* r : cands)
            if (casefold(r->label).find(casefold(w)) != std::string::npos) return r->value;
    return std::nullopt;
}

ojson hwinfo_sensors() {
    auto r = hwinfo_readings();
    if (!r) return {{"available", false}, {"temps", ojson::array()}, {"fans", ojson::array()}};
    auto names = [&](uint32_t t) {
        std::vector<std::string> out;
        for (auto& x : *r)
            if (x.type == t && !std::count(out.begin(), out.end(), x.label)) out.push_back(x.label);
        return out;
    };
    return {{"available", true}, {"temps", names(SENSOR_TEMP)}, {"fans", names(SENSOR_FAN)}};
}


void MetricHistory::sample(SysSnapshot& s,uint32_t pid,double now) {
    if(pid!=pid_){pid_=pid;sampled_=-1;fps_.clear();times_.clear();}
    const double tick=std::floor(now);
    if(sampled_<0 || tick>sampled_) {
        const double missing=std::numeric_limits<double>::quiet_NaN();
        if(sampled_>=0)for(int i=1;i<std::min(60.,tick-sampled_);++i){fps_.push_back(missing);times_.push_back(missing);}
        fps_.push_back(s.fps.value_or(missing));times_.push_back(s.frametime_ms.value_or(missing));sampled_=tick;
        while(fps_.size()>60)fps_.pop_front();while(times_.size()>60)times_.pop_front();
    }
    s.fps_hist=fps_;s.frametime_hist=times_;
}
void FrameHistory::present(double t){
    if(!std::isfinite(t) || (!times_.empty() && t<=times_.back()))return;
    if(!times_.empty() && t-times_.back()>2)times_.clear();
    times_.push_back(t);while(times_.size()>1200 || (!times_.empty() && t-times_.front()>10))times_.pop_front();
}
FrameStats FrameHistory::stats(double now) const {
    FrameStats out;if(times_.size()<2 || now<times_.back() || now-times_.back()>2)return out;
    std::vector<double> durations;
    for(size_t i=1;i<times_.size();++i)if(times_[i]>=now-10)durations.push_back((times_[i]-times_[i-1])*1000);
    if(durations.empty())return out;
    const double average=std::accumulate(durations.begin(),durations.end(),0.)/durations.size();
    if(average<=0)return out;out.intervals=durations.size();out.fps=1000/average;out.frametime_ms=average;
    for(auto d:durations){out.history.push_back(d);if(out.history.size()>120)out.history.pop_front();}
    // 1% low requires >=100 intervals; never invent it from two frames.
    if(durations.size()>=100){std::sort(durations.rbegin(),durations.rend());size_t n=(durations.size()+99)/100;out.low=1000/(std::accumulate(durations.begin(),durations.begin()+n,0.)/n);}
    return out;
}
struct SysMonitor::PresentTrace {
    static inline const GUID provider={0xca11c036,0x0102,0x4a2d,{0xa6,0xad,0xf0,0x3c,0xfe,0xd5,0xd3,0xc9}};
    TRACEHANDLE session=0,consumer=INVALID_PROCESSTRACE_HANDLE;
    std::wstring name;
    std::vector<unsigned char> properties;
    std::thread thread;
    std::atomic<uint32_t> pid{0};
    std::mutex mutex;
    struct Chain {FrameHistory frames;double last=0;};
    std::map<uint64_t,Chain> chains;
    double frequency=1;
    DWORD error=ERROR_SUCCESS;
    ULONG lost=0;
    bool tried=false;
    EVENT_TRACE_PROPERTIES* props(){return reinterpret_cast<EVENT_TRACE_PROPERTIES*>(properties.data());}
    PresentTrace(){LARGE_INTEGER f;QueryPerformanceFrequency(&f);frequency=double(f.QuadPart);}
    ~PresentTrace(){stop();}
    void stop(){
        if(session){ControlTraceW(session,name.c_str(),props(),EVENT_TRACE_CONTROL_STOP);session=0;}
        if(consumer!=INVALID_PROCESSTRACE_HANDLE){CloseTrace(consumer);consumer=INVALID_PROCESSTRACE_HANDLE;}
        if(thread.joinable())thread.join();
    }
    static bool property(EVENT_RECORD* e,const wchar_t* name,void* result,ULONG bytes){PROPERTY_DATA_DESCRIPTOR d{};d.PropertyName=reinterpret_cast<ULONGLONG>(name);d.ArrayIndex=ULONG_MAX;return TdhGetProperty(e,0,nullptr,1,&d,bytes,static_cast<PBYTE>(result))==ERROR_SUCCESS;}
    static void WINAPI event(EVENT_RECORD* e){
        auto self=static_cast<PresentTrace*>(e->UserContext);
        if(e->EventHeader.ProcessId!=self->pid.load() || e->EventHeader.ProviderId!=provider || (e->EventHeader.EventDescriptor.Id!=42 && e->EventHeader.EventDescriptor.Id!=55))return;
        uint64_t swap=0;ULONG flags=0;
        if(!property(e,L"pIDXGISwapChain",&swap,sizeof swap) || !property(e,L"Flags",&flags,sizeof flags) || (flags&1))return;
        double t=e->EventHeader.TimeStamp.QuadPart/self->frequency;
        std::lock_guard lock(self->mutex);
        if(e->EventHeader.ProcessId!=self->pid)return;
        if(!self->chains.contains(swap) && self->chains.size()>=8)return;
        auto& c=self->chains[swap];c.last=t;c.frames.present(t);
    }
    void start(){
        tried=true;name=L"ScreenBuddy.Game."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64());
        properties.assign(sizeof(EVENT_TRACE_PROPERTIES)+(name.size()+1)*sizeof(wchar_t),0);
        auto p=props();p->Wnode.BufferSize=ULONG(properties.size());p->Wnode.Flags=WNODE_FLAG_TRACED_GUID;p->Wnode.ClientContext=1;
        p->LogFileMode=EVENT_TRACE_REAL_TIME_MODE;p->LoggerNameOffset=sizeof(EVENT_TRACE_PROPERTIES);p->BufferSize=64;p->MinimumBuffers=2;p->MaximumBuffers=16;p->FlushTimer=1;
        error=StartTraceW(&session,name.c_str(),p);if(error!=ERROR_SUCCESS){session=0;return;}
        error=EnableTraceEx2(session,&provider,EVENT_CONTROL_CODE_ENABLE_PROVIDER,TRACE_LEVEL_VERBOSE,0x8000000000000002ull,0,0,nullptr);
        if(error!=ERROR_SUCCESS){stop();return;}
        EVENT_TRACE_LOGFILEW log{};log.LoggerName=name.data();log.ProcessTraceMode=PROCESS_TRACE_MODE_REAL_TIME|PROCESS_TRACE_MODE_EVENT_RECORD|PROCESS_TRACE_MODE_RAW_TIMESTAMP;log.EventRecordCallback=&event;log.Context=this;
        consumer=OpenTraceW(&log);if(consumer==INVALID_PROCESSTRACE_HANDLE){error=GetLastError();stop();return;}
        const auto handle=consumer;thread=std::thread([handle]{auto trace=handle;ProcessTrace(&trace,1,nullptr,nullptr);});
    }
    void target(uint32_t value){
        if(pid.exchange(value)!=value){std::lock_guard lock(mutex);chains.clear();}
        if(!value && session){stop();tried=false;}
        if(value && !tried)start();
    }
    FrameStats snapshot(double now){
        if(session && ControlTraceW(session,name.c_str(),props(),EVENT_TRACE_CONTROL_QUERY)==ERROR_SUCCESS && props()->EventsLost!=lost){lost=props()->EventsLost;std::lock_guard lock(mutex);chains.clear();return {};}
        std::lock_guard lock(mutex);FrameStats best;size_t count=0;
        for(const auto& [key,c]:chains) {auto s=c.frames.stats(now);if(s.intervals>count){count=s.intervals;best=std::move(s);}}
        return best;
    }
    std::string reason()const {if(!pid)return "no_game";if(error==ERROR_ACCESS_DENIED)return "etw_access_denied";if(error!=ERROR_SUCCESS)return "etw_unavailable";return "dxgi_present_calls";}
};
struct SysMonitor::GpuApi {
    HMODULE nvml=nullptr,adl=nullptr;bool initialized=false;void* device=nullptr;void* context=nullptr;int adapter=-1;
    template<class F>static F symbol(HMODULE dll,const char* name){return reinterpret_cast<F>(dll?GetProcAddress(dll,name):nullptr);}
    // ABI declarations from the public NVML and AMD ADL interfaces. Read only.
    struct Util {unsigned gpu,memory;};struct Memory {unsigned long long total,free,used;};
    struct Activity {int size,engine,memory,voltage,activity,level,bus_speed,bus_lanes,max_lanes,reserved;};
    struct Adapter {int size,index;char udid[256];int bus,device,function,vendor;char name[256],display[256];int present,exist;char driver[256],driver_ext[256],pnp[256];int osdisplay;};
    std::string amd_name;
    struct Temperature {int size,value;};struct Fan {int size,type,speed,flags;};
    static void* __stdcall allocate(int size){return size>0?std::malloc(size):nullptr;}
    GpuApi(){
        nvml=LoadLibraryExW(L"nvml.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        auto init=symbol<int(*)()>(nvml,"nvmlInit_v2");auto get=symbol<int(*)(unsigned,void**)>(nvml,"nvmlDeviceGetHandleByIndex_v2");
        if(init && get && init()==0){initialized=true;if(get(0,&device)!=0)device=nullptr;}
        if(device)return;
        adl=LoadLibraryExW(L"atiadlxx.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        auto create=symbol<int(*)(void*(__stdcall*)(int),int,void**)>(adl,"ADL2_Main_Control_Create");
        auto count=symbol<int(*)(void*,int*)>(adl,"ADL2_Adapter_NumberOfAdapters_Get");auto active=symbol<int(*)(void*,int,int*)>(adl,"ADL2_Adapter_Active_Get");
        auto info=symbol<int(*)(void*,Adapter*,int)>(adl,"ADL2_Adapter_AdapterInfo_Get");
        if(create && count && active && info && create(&allocate,1,&context)==0){int n=0;if(count(context,&n)==0 && n>0 && n<128){std::vector<Adapter> adapters(n);for(auto& a:adapters)a.size=sizeof(Adapter);
            if(info(context,adapters.data(),int(adapters.size()*sizeof(Adapter)))==0)for(const auto& a:adapters){int enabled=0;if(active(context,a.index,&enabled)==0 && enabled){adapter=a.index;amd_name=std::string(a.name,strnlen(a.name,sizeof a.name));break;}}}}
    }
    ~GpuApi(){if(context)if(auto f=symbol<int(*)(void*)>(adl,"ADL2_Main_Control_Destroy"))f(context);if(adl)FreeLibrary(adl);if(initialized)if(auto f=symbol<int(*)()>(nvml,"nvmlShutdown"))f();if(nvml)FreeLibrary(nvml);}
    void sample(SysSnapshot& s){
        if(device){
            auto scalar=[&](const char* name)->std::optional<double>{auto f=symbol<int(*)(void*,unsigned*)>(nvml,name);unsigned v=0;return f && f(device,&v)==0?std::optional<double>(v):std::nullopt;};
            if(auto f=symbol<int(*)(void*,Util*)>(nvml,"nvmlDeviceGetUtilizationRates")){Util v{};if(f(device,&v)==0)s.gpu_pct=v.gpu;}
            if(auto f=symbol<int(*)(void*,unsigned,unsigned*)>(nvml,"nvmlDeviceGetTemperature")){unsigned v=0;if(f(device,0,&v)==0)s.gpu_temp=v;}
            if(auto f=symbol<int(*)(void*,unsigned,unsigned*)>(nvml,"nvmlDeviceGetClockInfo")){unsigned v=0;if(f(device,0,&v)==0)s.gpu_mhz=v;}
            s.gpu_fan=scalar("nvmlDeviceGetFanSpeed");if(auto v=scalar("nvmlDeviceGetPowerUsage"))s.gpu_watts=*v/1000;
            if(auto f=symbol<int(*)(void*,Memory*)>(nvml,"nvmlDeviceGetMemoryInfo")){Memory m{};if(f(device,&m)==0){s.gpu_mem_used=double(m.used)/(1024*1024);s.gpu_mem_total=double(m.total)/(1024*1024);}}
            if(auto f=symbol<int(*)(void*,char*,unsigned)>(nvml,"nvmlDeviceGetName")){char name[128]{};if(f(device,name,sizeof name)==0)s.gpu_name=name;}
        } else if(context && adapter>=0){
            s.gpu_name=amd_name;
            if(auto f=symbol<int(*)(void*,int,Activity*)>(adl,"ADL2_Overdrive5_CurrentActivity_Get")){Activity v{};v.size=sizeof v;if(f(context,adapter,&v)==0){s.gpu_pct=std::clamp(double(v.activity),0.,100.);s.gpu_mhz=v.engine/100.;}}
            if(auto f=symbol<int(*)(void*,int,int,Temperature*)>(adl,"ADL2_Overdrive5_Temperature_Get")){Temperature v{sizeof v,0};if(f(context,adapter,0,&v)==0)s.gpu_temp=v.value/1000.;}
            if(auto f=symbol<int(*)(void*,int,int,Fan*)>(adl,"ADL2_Overdrive5_FanSpeed_Get")){Fan v{sizeof v,1,0,0};if(f(context,adapter,0,&v)==0)s.gpu_fan=v.speed;}
            if(auto f=symbol<int(*)(void*,int,int,int*)>(adl,"ADL2_Overdrive6_CurrentPower_Get")){int v=0;if(f(context,adapter,0,&v)==0 && v>=0)s.gpu_watts=v/256.;}
        }
    }
};

// ---------- Sammler ----------
SysMonitor::SysMonitor(const ojson& cfg) {
    auto s = [&](const char* k) { return cfg.contains(k) && cfg[k].is_string() ? cfg[k].get<std::string>() : std::string(); };
    net_adapter_ = trim(s("net_adapter"));
    ping_target_ = s("ping_target");
    cpu_temp_sensor_ = s("cpu_temp_sensor");
    cpu_fan_sensor_ = s("cpu_fan_sensor");
    auto [name, mhz] = cpu_name();
    snap_.cpu_name = name;
    base_mhz_ = mhz;
    std::string disk = s("disk");
    if (disk.empty()) disk = "C:";
    while (!disk.empty() && disk.back() == '\\') disk.pop_back();
    snap_.disk_label = disk;
    published_ = snap_;
}

SysMonitor::~SysMonitor() { stop(); }

void SysMonitor::start() {
    th_ = std::thread([this] { run(); });
}

void SysMonitor::stop() {
    stop_ = true;
    if (th_.joinable()) th_.join();
}

SysSnapshot SysMonitor::snapshot() {
    std::lock_guard lk(mu_);
    return published_;
}

void SysMonitor::run() {
    while (!stop_) {
        double t = mono();
        try {
            sample();
            std::lock_guard lk(mu_);
            published_ = snap_;
        } catch (const std::exception& e) {
            LOGW("sysinfo", "Systemwerte konnten nicht gelesen werden: %s", e.what());
        }
        double wait = std::max(0.1, 1.0 - (mono() - t));
        for (double w = 0; w < wait && !stop_; w += 0.05) Sleep(50);
    }
}

void SysMonitor::sample() {
    SysSnapshot& s = snap_;
    s.cpu_pct.reset();s.cpu_ghz.reset();
    if (!pdh_) pdh_ = std::make_unique<Pdh>();  // beim ersten Aufruf anlegen; Werte gibt es ab dem zweiten Aufruf
    if (pdh_->q && PdhCollectQueryData(pdh_->q) == ERROR_SUCCESS) {
        if (auto v = pdh_->value(pdh_->cpu)) s.cpu_pct = std::min(100.0, *v);
        if (auto v = pdh_->value(pdh_->perf); v && base_mhz_) s.cpu_ghz = base_mhz_ * *v / 100 / 1000;
    }
    s.cpu_cores.clear();
    for(const auto& [name,value]:pdh_->values(pdh_->cores))if(name.find(L"_Total")==std::wstring::npos)s.cpu_cores.push_back(std::clamp(value,0.,100.));
    SYSTEM_INFO si{};GetSystemInfo(&si);
    struct CpuPower {ULONG number,max,current,limit,max_idle,current_idle;};
    std::vector<CpuPower> power(si.dwNumberOfProcessors);
    if(!power.empty() && CallNtPowerInformation(ProcessorInformation,nullptr,0,power.data(),ULONG(power.size()*sizeof(CpuPower)))==0) {
        double mhz=0;for(const auto& core:power)mhz+=core.current;if(mhz>0)s.cpu_ghz=mhz/power.size()/1000.;
    }
    if(!trace_)trace_=std::make_unique<PresentTrace>();
    const auto pid=game_pid_.load();trace_->target(pid);auto frames=trace_->snapshot(mono());
    s.fps=frames.fps;s.frametime_ms=frames.frametime_ms;s.fps_low=frames.low;metric_history_.sample(s,pid,mono());s.fps_reason=trace_->reason();
    sample_ping();
    MEMORYSTATUSEX ms{sizeof ms};
    if (GlobalMemoryStatusEx(&ms)) {
        s.ram_total = ms.ullTotalPhys / double(1ull << 30);
        s.ram_used = (ms.ullTotalPhys - ms.ullAvailPhys) / double(1ull << 30);
        s.ram_pct = *s.ram_used / *s.ram_total * 100;
    }
    ULARGE_INTEGER freeb{}, total{};
    if (GetDiskFreeSpaceExW(widen(s.disk_label + "\\").c_str(), nullptr, &total, &freeb))
        s.disk_pct = total.QuadPart ? std::optional<double>((1 - (double)freeb.QuadPart / total.QuadPart) * 100) : std::nullopt;
    sample_net();
    if (mono() >= gpu_next_) {
        gpu_next_ = mono() + 2;
        sample_gpu();
        sample_hwinfo();
    }
}

void SysMonitor::sample_net() {
    SysSnapshot& s = snap_;
    if (!net_adapter_.empty() && !net_idx_) net_idx_ = interface_index(net_adapter_);
    std::optional<uint32_t> idx = net_adapter_.empty() ? default_interface() : net_idx_;
    MIB_IF_ROW2 row{};
    bool ok = false;
    if (idx) {
        row.InterfaceIndex = *idx;
        ok = GetIfEntry2(&row) == NO_ERROR;
    }
    if (!ok) {
        s.net_name = "kein Netzwerk";
        s.net_down = s.net_up = std::nullopt;
        net_prev_t_.reset();
        return;
    }
    double now = mono();
    bool have_prev = net_prev_t_ && net_prev_idx_ == *idx && now > *net_prev_t_;
    double dt = have_prev ? now - *net_prev_t_ : 0;
    uint64_t pin = net_prev_in_, pout = net_prev_out_;
    net_prev_t_ = now;
    net_prev_idx_ = *idx;
    net_prev_in_ = row.InOctets;
    net_prev_out_ = row.OutOctets;
    s.net_name = narrow(row.Alias);
    s.net_link = row.ReceiveLinkSpeed ? std::optional<uint64_t>(row.ReceiveLinkSpeed) : std::nullopt;
    if (have_prev) {
        // Zähler können beim Adapter-Reset zurückspringen → negativ = 0
        s.net_down = (row.InOctets >= pin ? double(row.InOctets - pin) : 0.0) / dt;
        s.net_up = (row.OutOctets >= pout ? double(row.OutOctets - pout) : 0.0) / dt;
        s.down_hist.push_back(*s.net_down);
        s.up_hist.push_back(*s.net_up);
        while (s.down_hist.size() > 60) s.down_hist.pop_front();
        while (s.up_hist.size() > 60) s.up_hist.pop_front();
    }
}

void SysMonitor::sample_gpu() {
    auto& s=snap_;s.gpu_pct.reset();s.gpu_temp.reset();s.gpu_fan.reset();s.gpu_mhz.reset();s.gpu_watts.reset();s.gpu_mem_used.reset();s.gpu_mem_total.reset();
    if(!gpu_)gpu_=std::make_unique<GpuApi>();
    gpu_->sample(s);
    if(!s.gpu_pct)for(const auto& [name,value]:pdh_->values(pdh_->engines))s.gpu_pct=std::max(s.gpu_pct.value_or(0),std::clamp(value,0.,100.));
    // PDH is adapter-wide. QueryVideoMemoryInfo would describe ScreenBuddy's own
    // process budget/usage, not the game's usage, so it must not be labelled VRAM used.
    if(!gpu_->device && !s.gpu_mem_used) {double used=0;auto memory=pdh_->values(pdh_->memory);for(const auto& [name,value]:memory)used+=std::max(0.,value);if(!memory.empty())s.gpu_mem_used=used/(1024*1024);}
    if(!gpu_->device && !s.gpu_mem_total) {
        IDXGIFactory1* factory=nullptr;if(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),reinterpret_cast<void**>(&factory)))) {
            double total=0;for(UINT i=0;;++i){IDXGIAdapter1* adapter=nullptr;if(factory->EnumAdapters1(i,&adapter)==DXGI_ERROR_NOT_FOUND)break;if(!adapter)break;DXGI_ADAPTER_DESC1 desc{};if(SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))total+=double(desc.DedicatedVideoMemory);adapter->Release();}
            factory->Release();if(total>0)s.gpu_mem_total=total/(1024*1024);
        }
    }
}
void SysMonitor::sample_ping() {
    auto& s=snap_;s.ping_ms.reset();s.jitter_ms.reset();if(ping_target_.empty())return;
    IN_ADDR address{};if(InetPtonW(AF_INET,widen(ping_target_).c_str(),&address)!=1)return;
    HANDLE h=IcmpCreateFile();if(h==INVALID_HANDLE_VALUE)return;
    char request[]="ScreenBuddy";std::vector<unsigned char> buffer(sizeof(ICMP_ECHO_REPLY)+sizeof request+8);
    DWORD count=IcmpSendEcho(h,address.S_un.S_addr,request,sizeof request,nullptr,buffer.data(),DWORD(buffer.size()),200);
    if(count){auto reply=reinterpret_cast<ICMP_ECHO_REPLY*>(buffer.data());if(reply->Status==IP_SUCCESS){s.ping_ms=reply->RoundTripTime;if(ping_previous_)s.jitter_ms=std::abs(*s.ping_ms-*ping_previous_);ping_previous_=s.ping_ms;}else ping_previous_.reset();}else ping_previous_.reset();
    IcmpCloseHandle(h);
}

void SysMonitor::sample_hwinfo() {
    SysSnapshot& s = snap_;
    auto r = hwinfo_readings();
    s.hwinfo = r.has_value();
    s.cpu_temp.reset();s.cpu_fan.reset();s.disk_temp.reset();
    if (!r || r->empty()) return;
    s.cpu_temp = pick(*r, SENSOR_TEMP, {cpu_temp_sensor_, "CPU (Tctl/Tdie)", "CPU Package", "CPU Die (average)", "CPU"});
    s.cpu_fan = pick(*r, SENSOR_FAN,
                     {cpu_fan_sensor_, "CPU", "CPU_OPT", "AIO Pump", "AIO Pumpe", "Pumpe", "Chassis1"});
    std::vector<Reading> disk;
    for (auto& x : *r) {
        auto seg = split(x.sensor, "[").back();
        if (x.sensor.find("S.M.A.R.T") != std::string::npos && seg.find(s.disk_label) != std::string::npos) disk.push_back(x);
    }
    s.disk_temp = pick(disk, SENSOR_TEMP, {"Drive Temperature", "Laufwerkstemperatur"});
}

}  // namespace fl::sysinfo
