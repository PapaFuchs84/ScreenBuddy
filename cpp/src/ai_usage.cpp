// © 2026 FuchsLabs — clean-room read-only stdio client, no model requests.
#include "ai_status.h"
#include "i18n.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <cwchar>

namespace fl::ai {
std::string usage_reason(const std::string& reason) {
    const bool en=i18n::lang()=="en";
    if(reason=="not_in_path") return en?"CLI not in PATH":"CLI nicht im PATH";
    if(reason=="signed_out_isolated_home") return en?"Isolated CLI: signed out":"Isolierte CLI: abgemeldet";
    if(reason=="api_key_no_limits") return en?"API key: no account limits":"API-Key: keine Kontolimits";
    if(reason=="unsupported_version") return en?"CLI interface too old/unsupported":"CLI-Schnittstelle zu alt/unbekannt";
    if(reason=="no_verified_usage_command") return en?"No verified CLI usage command":"Kein belegter CLI-Nutzungsbefehl";
    if(reason=="cli_failed_or_timeout" || reason=="query_failed") return en?"CLI failed; 15 min pause":"CLI-Fehler; 15 Min. Pause";
    if(reason=="limits_not_supplied") return en?"Limits not supplied":"Limits nicht geliefert";
    if(reason=="stale") return en?"Data older than 15 min":"Daten älter als 15 Min.";
    if(reason=="disabled") return en?"Automatic queries disabled":"Automatische Abfrage aus";
    if(reason.empty()) return "";
    return en?"No usage data":"Keine Nutzungsdaten";
}

UsageResult codex_usage(const Json& reply) {
    UsageResult out;out.source="codex-app-server";
    try {
        if(reply.contains("error")) {out.reason="query_failed";return out;}
        const auto& result=reply.at("result");
        const auto& limits=result.contains("rateLimitsByLimitId") && result["rateLimitsByLimitId"].contains("codex")?
            result["rateLimitsByLimitId"]["codex"]:result.at("rateLimits");
        for(auto key:{"primary","secondary"}) {
            if(!limits.contains(key) || limits[key].is_null()) continue;
            const auto& w=limits[key];
            if(!w.at("windowDurationMins").is_number_integer()) {out.reason="invalid_data";return out;}
            const int minutes=w.at("windowDurationMins").get<int>();
            const char* name=minutes==300?"five_hour":minutes==10080?"week":nullptr;
            if(!name) continue; // Never label an arbitrary window as 5h/week.
            auto candidate=Json{{"provider","codex"},{"session_id","limits"},{"hook_event_name","Usage"},{name,w.at("usedPercent")}};
            const std::string reset=std::string(name)+"_reset";
            if(w.contains("resetsAt") && !w["resetsAt"].is_null()) candidate[reset]=w["resetsAt"];
            if(!parse(candidate.dump())) {out.reason="invalid_data";return out;}
            out.metrics[name]=candidate[name];if(candidate.contains(reset)) out.metrics[reset]=candidate[reset];
        }
        out.ok=!out.metrics.empty();out.reason=out.ok?"":"limits_not_supplied";
    } catch(...) {out.metrics=Json::object();out.reason="unsupported_version";}
    return out;
}
namespace {
struct Handle {
    HANDLE h=INVALID_HANDLE_VALUE;
    Handle()=default;Handle(const Handle&)=delete;
    ~Handle(){if(h!=INVALID_HANDLE_VALUE && h) CloseHandle(h);}
};
std::filesystem::path executable(const wchar_t* name) {
    wchar_t path[32768];
    if(SearchPathW(nullptr,name,L".exe",32768,path,nullptr)) return path;
    // npm wrappers cannot be executed with CreateProcess; resolve the official bundled binary.
    if(std::wstring(name)==L"codex" && SearchPathW(nullptr,L"codex.cmd",nullptr,32768,path,nullptr)) {
        auto base=std::filesystem::path(path).parent_path()/L"node_modules/@openai/codex/node_modules/@openai/codex-win32-x64/vendor/x86_64-pc-windows-msvc/bin/codex.exe";
        if(std::filesystem::is_regular_file(base)) return base;
    }
    return {};
}
// Child receives an isolated home, no API credentials and no inherited stdio/log file.
// Bounded pipes and a kill-on-close job also contain a hung CLI.
std::mutex child_mutex;
struct Child {
    std::unique_lock<std::mutex> serial{child_mutex};
    Handle process,job,input,output;std::string pending;ULONGLONG deadline;
    Child(const std::filesystem::path& exe,const std::wstring& args,const std::filesystem::path& dir):deadline(GetTickCount64()+12000) {
        std::filesystem::create_directories(dir);
        std::vector<std::wstring> environment;
        LPWCH block=GetEnvironmentStringsW();
        if(!block) throw std::runtime_error("environment");
        for(auto p=block;*p;p+=wcslen(p)+1) {
            std::wstring v=p;auto pos=v.find(L'=');auto key=v.substr(0,pos);
            std::transform(key.begin(),key.end(),key.begin(),towupper);
            if(key!=L"PATH" && key!=L"PATHEXT" && key!=L"SYSTEMROOT" && key!=L"WINDIR" && key!=L"COMSPEC" && key!=L"OS" && key!=L"PROCESSOR_ARCHITECTURE" && key!=L"NUMBER_OF_PROCESSORS") continue;
            environment.push_back(v);
        }
        FreeEnvironmentStringsW(block);
        for(auto key:{L"CODEX_HOME",L"CLAUDE_CONFIG_DIR",L"HOME",L"USERPROFILE",L"APPDATA",L"LOCALAPPDATA",L"TEMP",L"TMP"}) environment.push_back(std::wstring(key)+L"="+dir.wstring());
        std::sort(environment.begin(),environment.end(),[](const auto& a,const auto& b){return _wcsicmp(a.c_str(),b.c_str())<0;});
        std::vector<wchar_t> env;for(const auto& v:environment) {env.insert(env.end(),v.begin(),v.end());env.push_back(0);}env.push_back(0);
        SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};Handle childIn,childOut;
        if(!CreatePipe(&childIn.h,&input.h,&sa,0) || !CreatePipe(&output.h,&childOut.h,&sa,0)) throw std::runtime_error("pipe");
        SetHandleInformation(input.h,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output.h,HANDLE_FLAG_INHERIT,0);
        Handle nul;nul.h=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
        STARTUPINFOEXW extended{};auto& si=extended.StartupInfo;si.cb=sizeof(extended);si.dwFlags=STARTF_USESTDHANDLES;
        si.hStdInput=childIn.h;si.hStdOutput=childOut.h;si.hStdError=nul.h;
        SIZE_T attributeSize=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeSize);
        std::vector<unsigned char> attributeStorage(attributeSize);
        extended.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
        if(!InitializeProcThreadAttributeList(extended.lpAttributeList,1,0,&attributeSize)) throw std::runtime_error("attributes");
        struct Attributes {LPPROC_THREAD_ATTRIBUTE_LIST p;~Attributes(){DeleteProcThreadAttributeList(p);}} attributes{extended.lpAttributeList};
        HANDLE handles[]={childIn.h,childOut.h,nul.h};
        if(!UpdateProcThreadAttribute(extended.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof handles,nullptr,nullptr)) throw std::runtime_error("handles");
        job.h=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!job.h || !SetInformationJobObject(job.h,JobObjectExtendedLimitInformation,&limits,sizeof limits)) throw std::runtime_error("job");
        auto command=L"\""+exe.wstring()+L"\" "+args;PROCESS_INFORMATION pi{};
        if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT|CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT|CREATE_SUSPENDED,env.data(),dir.c_str(),&si,&pi)) throw std::runtime_error("launch");
        process.h=pi.hProcess;Handle thread;thread.h=pi.hThread;
        if(!AssignProcessToJobObject(job.h,process.h)) {TerminateProcess(process.h,1);throw std::runtime_error("job");}
        ResumeThread(thread.h);
    }
    ~Child(){if(process.h!=INVALID_HANDLE_VALUE) {TerminateJobObject(job.h,0);WaitForSingleObject(process.h,1000);}}
    void send(const Json& j) {
        const auto data=j.dump()+"\n";DWORD n=0;
        if(!WriteFile(input.h,data.data(),DWORD(data.size()),&n,nullptr) || n!=data.size()) throw std::runtime_error("write");
    }
    std::string line(const std::atomic<bool>& enabled) {
        while(enabled && GetTickCount64()<deadline) {
            auto end=pending.find('\n');if(end!=std::string::npos) {auto out=pending.substr(0,end);pending.erase(0,end+1);return out;}
            DWORD available=0;
            if(!PeekNamedPipe(output.h,nullptr,0,nullptr,&available,nullptr)) break;
            if(available) {char buffer[4096];DWORD n=0;if(!ReadFile(output.h,buffer,std::min<DWORD>(available,sizeof buffer),&n,nullptr)) break;pending.append(buffer,n);
                if(pending.size()>262144) throw std::runtime_error("size");}
            else {if(WaitForSingleObject(process.h,0)==WAIT_OBJECT_0) break;Sleep(15);}
        }
        throw std::runtime_error("timeout");
    }
    Json reply(int id,const std::atomic<bool>& enabled) {
        for(int n=0;n<256;++n) {auto j=Json::parse(line(enabled));if(j.value("id",Json())==id) return j;}
        throw std::runtime_error("protocol");
    }
    std::string help(const std::atomic<bool>& enabled) {
        std::string out;try {while(true) {out+=line(enabled)+"\n";if(out.size()>262144) throw std::runtime_error("size");}}
        catch(...) {DWORD code=1;if(!GetExitCodeProcess(process.h,&code) || code!=0) throw;}
        return out;
    }
};
UsageResult query(const std::filesystem::path& exe,const std::filesystem::path& dir,const std::atomic<bool>& enabled) {
    UsageResult out;out.source="codex-app-server";
    if(exe.empty()) {out.reason="not_in_path";return out;}
    try {
        {Child child(exe,L"app-server --help",dir);if(child.help(enabled).find("stdio")==std::string::npos) {out.reason="unsupported_version";return out;}}
        Child child(exe,L"app-server --listen stdio://",dir);
        child.send({{"id",1},{"method","initialize"},{"params",{{"clientInfo",{{"name","screenbuddy_usage"},{"version","1"}}}}}});
        if(child.reply(1,enabled).contains("error")) {out.reason="unsupported_version";return out;}
        child.send({{"method","initialized"}});
        child.send({{"id",2},{"method","account/read"},{"params",{{"refreshToken",false}}}});
        const auto account=child.reply(2,enabled);
        if(account.contains("error")) {out.reason="unsupported_version";return out;}
        const auto& a=account.at("result").at("account");
        if(a.is_null()) {out.reason="signed_out_isolated_home";return out;}
        if(a.value("type",std::string())!="chatgpt") {out.reason="api_key_no_limits";return out;}
        child.send({{"id",3},{"method","account/rateLimits/read"}});
        return codex_usage(child.reply(3,enabled));
    } catch(...) {out.reason="cli_failed_or_timeout";return out;}
}
}
struct UsageWorker::Impl {
    Model& model;std::filesystem::path directory;std::atomic<bool> enabled=false;
    bool stop=false;std::mutex mutex;std::condition_variable cv;std::thread thread;
    Impl(Model& m,std::filesystem::path p):model(m),directory(std::move(p)),thread([this]{run();}) {}
    ~Impl() {{std::lock_guard lock(mutex);stop=true;enabled=false;}cv.notify_all();thread.join();}
    void run() {
        std::unique_lock lock(mutex);
        auto next=std::chrono::steady_clock::time_point::min();
        while(!stop) {
            cv.wait(lock,[&]{return stop || enabled.load();});if(stop) break;
            if(std::chrono::steady_clock::now()<next) {cv.wait_until(lock,next,[&]{return stop || !enabled.load();});continue;}
            lock.unlock();
            bool ok=false;bool claude_failed=false;
            try {
                // Serialized: Claude help completes before any Codex process starts.
                auto claude=executable(L"claude");std::string reason="not_in_path";
                if(!claude.empty()) {try {Child help(claude,L"--help",directory/L"claude");help.help(enabled);reason="no_verified_usage_command";}
                    catch(...) {reason="cli_failed_or_timeout";claude_failed=true;}}
                if(enabled) model.usage("claude",Json::object(),"claude-cli",reason,std::time(nullptr));
                if(enabled && !claude_failed) {auto result=query(executable(L"codex"),directory/L"codex",enabled);ok=result.ok;
                    if(enabled) model.usage("codex",result.metrics,result.source,result.reason,std::time(nullptr));}
            } catch(...) {if(enabled) model.usage("codex",Json::object(),"codex-app-server","cli_failed_or_timeout",std::time(nullptr));}
            lock.lock();next=std::chrono::steady_clock::now()+std::chrono::minutes(ok && !claude_failed?5:15);
        }
    }
};
UsageWorker::UsageWorker(Model& model,std::filesystem::path p):d_(std::make_unique<Impl>(model,std::move(p))) {}
UsageWorker::~UsageWorker()=default;
void UsageWorker::enable(bool enabled) {
    {std::lock_guard lock(d_->mutex);d_->enabled=enabled;}
    if(!enabled) for(auto provider:{"claude","codex"}) d_->model.usage(provider,Json::object(),"none","disabled",std::time(nullptr));
    d_->cv.notify_all();
}
} // namespace fl::ai
