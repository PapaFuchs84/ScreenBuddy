// © 2026 FuchsLabs — clean-room read-only HTTPS/isolated stdio, no model requests.
#include "ai_status.h"
#include "i18n.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <cwchar>
#include <winhttp.h>
#include <cmath>
#include <cstdio>
#include <charconv>

namespace fl::ai {
std::string usage_reason(const std::string& reason) {
    const bool en=i18n::lang()=="en";
    if(reason=="not_in_path") return en?"CLI not found (PATH/install locations)":"CLI nicht gefunden (PATH/Installationsorte)";
    if(reason=="signed_out") return en?"Codex: not signed in":"Codex: nicht angemeldet";
    if(reason=="signed_out_isolated_home") return en?"Isolated CLI: signed out":"Isolierte CLI: abgemeldet";
    if(reason=="api_key_no_limits") return en?"API key: no account limits":"API-Key: keine Kontolimits";
    if(reason=="unsupported_version") return en?"CLI interface too old/unsupported":"CLI-Schnittstelle zu alt/unbekannt";
    if(reason=="no_verified_usage_command") return en?"Usage only through status line":"Nutzung nur über Statuszeile";
    if(reason=="cli_failed_or_timeout" || reason=="query_failed") return en?"CLI failed; 15 min pause":"CLI-Fehler; 15 Min. Pause";
    if(reason=="limits_not_supplied") return en?"Limits not supplied":"Limits nicht geliefert";
    if(reason=="stale") return en?"Data older than 15 min":"Daten älter als 15 Min.";
    if(reason=="not_signed_in") return en?"Not signed in":"Nicht angemeldet";
    if(reason=="expired_claude") return en?"Token expired: start Claude once":"Token abgelaufen: Claude einmal starten";
    if(reason=="expired_codex") return en?"Token expired: start Codex once":"Token abgelaufen: Codex einmal starten";
    if(reason=="invalid_login") return en?"Login file missing fields or invalid":"Login-Datei unvollständig oder ungültig";
    if(reason=="invalid_data") return en?"Invalid usage response; 15 min pause":"Ungültige Nutzungsantwort; 15 Min. Pause";
    if(reason=="usage_forbidden") return en?"Access denied (403); 15 min pause":"Zugriff verweigert (403); 15 Min. Pause";
    if(reason=="usage_rate_limited") return en?"Rate limited (429); 15 min pause":"Abfragelimit (429); 15 Min. Pause";
    if(reason=="usage_failed") return en?"Usage query failed; 15 min pause":"Nutzungsabfrage fehlgeschlagen; 15 Min. Pause";
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
// All parser allocations and decoded strings are wiped on release.
template<class T> struct WipeAllocator : std::allocator<T> {
    using value_type=T;
    template<class U> struct rebind {using other=WipeAllocator<U>;};
    WipeAllocator()=default;
    template<class U> WipeAllocator(const WipeAllocator<U>&) {}
    void deallocate(T* p,size_t n) {SecureZeroMemory(p,n*sizeof(T));std::allocator<T>::deallocate(p,n);}
};
struct SecretString : std::basic_string<char,std::char_traits<char>,WipeAllocator<char>> {
    using Base=std::basic_string<char,std::char_traits<char>,WipeAllocator<char>>;
    using Base::Base;using Base::operator=;
    SecretString()=default;
    SecretString(const SecretString&)=default;
    ~SecretString(){if(!empty()) SecureZeroMemory(data(),capacity()+1);}
};
using SecretJson=nlohmann::basic_json<std::map,std::vector,SecretString,bool,int64_t,uint64_t,double,WipeAllocator>;
namespace {
// Do not use JSON syntax-error diagnostics for credentials: those may copy input
// fragments into ordinary exception strings. This bounded reader only throws a
// constant message and keeps all decoded strings in wiping storage.
class LoginJson {
    std::string_view input;size_t pos=0;
    [[noreturn]] void bad(){throw std::runtime_error("Invalid login JSON.");}
    void space(){while(pos<input.size() && (input[pos]==' ' || input[pos]=='\t' || input[pos]=='\r' || input[pos]=='\n')) ++pos;}
    bool take(char c){space();if(pos<input.size() && input[pos]==c){++pos;return true;}return false;}
    unsigned hex4(){unsigned v=0;for(int n=0;n<4;++n){if(pos==input.size()) bad();char c=input[pos++];int x=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(x<0) bad();v=(v<<4)|x;}return v;}
    SecretString string(){
        if(!take('"')) bad();SecretString out;
        while(pos<input.size()) {
            unsigned char c=input[pos++];if(c=='"') return out;if(c<32) bad();
            if(c!='\\'){out.push_back(c);continue;}
            if(pos==input.size()) bad();c=input[pos++];
            switch(c){
            case '"':case '\\':case '/':out.push_back(c);break;
            case 'b':out.push_back('\b');break;case 'f':out.push_back('\f');break;
            case 'n':out.push_back('\n');break;case 'r':out.push_back('\r');break;case 't':out.push_back('\t');break;
            case 'u':{
                auto v=hex4();
                if(v>=0xd800 && v<=0xdbff){if(pos+2>input.size() || input[pos++]!='\\' || input[pos++]!='u') bad();auto low=hex4();if(low<0xdc00 || low>0xdfff) bad();v=0x10000+((v-0xd800)<<10)+(low-0xdc00);}
                else if(v>=0xdc00 && v<=0xdfff) bad();
                if(v<0x80) out.push_back(char(v));
                else if(v<0x800){out.push_back(char(0xc0|(v>>6)));out.push_back(char(0x80|(v&63)));}
                else if(v<0x10000){out.push_back(char(0xe0|(v>>12)));out.push_back(char(0x80|((v>>6)&63)));out.push_back(char(0x80|(v&63)));}
                else {out.push_back(char(0xf0|(v>>18)));out.push_back(char(0x80|((v>>12)&63)));out.push_back(char(0x80|((v>>6)&63)));out.push_back(char(0x80|(v&63)));}break;}
            default:bad();
            }
        }bad();
    }
    SecretJson value(int depth){
        if(depth>32) bad();space();if(pos==input.size()) bad();
        if(input[pos]=='"') return string();
        if(take('{')){auto out=SecretJson::object();if(take('}')) return out;do{auto key=string();if(!take(':') || out.contains(key)) bad();out[key]=value(depth+1);}while(take(','));if(!take('}')) bad();return out;}
        if(take('[')){auto out=SecretJson::array();if(take(']')) return out;do{out.push_back(value(depth+1));}while(take(','));if(!take(']')) bad();return out;}
        for(auto literal:{"null","true","false"}){auto length=std::char_traits<char>::length(literal);if(input.substr(pos,length)==literal){pos+=length;if(literal[0]=='n') return nullptr;return literal[0]=='t';}}
        auto start=pos;if(input[pos]=='-') ++pos;
        if(pos==input.size()) bad();
        if(input[pos]=='0') ++pos;else {if(input[pos]<'1' || input[pos]>'9') bad();while(pos<input.size() && input[pos]>='0' && input[pos]<='9') ++pos;}
        if(pos<input.size() && input[pos]=='.'){auto first=++pos;while(pos<input.size() && input[pos]>='0' && input[pos]<='9') ++pos;if(first==pos) bad();}
        if(pos<input.size() && (input[pos]=='e' || input[pos]=='E')){++pos;if(pos<input.size() && (input[pos]=='+' || input[pos]=='-')) ++pos;auto first=pos;while(pos<input.size() && input[pos]>='0' && input[pos]<='9') ++pos;if(first==pos) bad();}
        double number=0;auto parsed=std::from_chars(input.data()+start,input.data()+pos,number);
        if(parsed.ec!=std::errc{} || parsed.ptr!=input.data()+pos || !std::isfinite(number)) bad();return number;
    }
public:
    explicit LoginJson(std::string_view text):input(text){}
    SecretJson parse(){auto out=value(0);space();if(pos!=input.size()) bad();return out;}
};
struct Internet {
    HINTERNET h=nullptr;
    explicit Internet(HINTERNET v):h(v){}
    ~Internet(){if(h) WinHttpCloseHandle(h);}
};
UsageHttpReply https_usage(bool claude,std::string_view token,std::string_view account) {
    UsageHttpReply out;
    Internet session(WinHttpOpen(L"ScreenBuddy-Usage/1",WINHTTP_ACCESS_TYPE_NO_PROXY,nullptr,nullptr,0));
    if(!session.h || !WinHttpSetTimeouts(session.h,3000,3000,3000,3000)) return out;
    Internet connection(WinHttpConnect(session.h,claude?L"api.anthropic.com":L"chatgpt.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection.h) return out;
    Internet request(WinHttpOpenRequest(connection.h,L"GET",claude?L"/api/oauth/usage":L"/backend-api/wham/usage",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.h) return out;
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
    if(!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof redirects) ||
       !WinHttpSetOption(request.h,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof disabled)) return out;
    // Default Windows certificate/hostname verification stays enabled. Never follow redirects.
    std::vector<wchar_t,WipeAllocator<wchar_t>> header;
    auto append=[&](std::string_view v){for(unsigned char c:v) header.push_back(c);};
    append("Authorization: Bearer ");append(token);append("\r\n");
    if(claude) append("anthropic-beta: oauth-2025-04-20\r\n");
    else if(!account.empty()) {append("ChatGPT-Account-Id: ");append(account);append("\r\n");}
    if(!WinHttpSendRequest(request.h,header.data(),DWORD(header.size()),WINHTTP_NO_REQUEST_DATA,0,0,0)) return out;
    SecureZeroMemory(header.data(),header.size()*sizeof(wchar_t));
    if(!WinHttpReceiveResponse(request.h,nullptr)) return out;
    DWORD status=0,size=sizeof status;
    if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr)) return out;
    out.status=int(status);if(status!=200) return out; // never expose error response bodies
    const auto deadline=GetTickCount64()+12000;
    char buffer[4096];DWORD count=0;
    do {
        if(GetTickCount64()>deadline || !WinHttpReadData(request.h,buffer,sizeof buffer,&count) || out.body.size()+count>65536) return {};
        out.body.append(buffer,count);
    } while(count);
    return out;
}
bool header_safe(std::string_view s) {
    return !s.empty() && s.size()<=16384 && std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=33 && c<=126;});
}
std::optional<int64_t> iso_reset(const std::string& s) {
    int y,m,d,h,n,sec,used=0;
    if(sscanf_s(s.c_str(),"%d-%d-%dT%d:%d:%d%n",&y,&m,&d,&h,&n,&sec,&used)!=6 || y<1970 || y>2200 || m<1 || m>12 || d<1 || d>31 || h<0 || h>23 || n<0 || n>59 || sec<0 || sec>59) return {};
    size_t pos=used;
    if(pos<s.size() && s[pos]=='.') {auto first=++pos;while(pos<s.size() && s[pos]>='0' && s[pos]<='9') ++pos;if(pos==first) return {};}
    int offset=0;
    if(s.substr(pos)=="Z") {}
    else {int oh,om,consumed=0;if(pos>=s.size() || (s[pos]!='+' && s[pos]!='-') || sscanf_s(s.c_str()+pos+1,"%d:%d%n",&oh,&om,&consumed)!=2 || pos+1+consumed!=s.size() || oh<0 || oh>23 || om<0 || om>59) return {};offset=(oh*60+om)*60*(s[pos]=='+'?1:-1);}
    std::tm tm{};tm.tm_year=y-1900;tm.tm_mon=m-1;tm.tm_mday=d;tm.tm_hour=h;tm.tm_min=n;tm.tm_sec=sec;
    auto t=_mkgmtime64(&tm);if(t<0 || tm.tm_mon!=m-1 || tm.tm_mday!=d) return {};return t-offset;
}
}
int usage_delay(bool success){return success?180:900;}
UsageResult endpoint_usage(bool claude,const Json& reply) {
    UsageResult out;out.source=claude?"claude-oauth-usage":"codex-wham-usage";
    try {
        auto add=[&](const Json& w,const char* name) {
            const auto& percent=w.at(claude?"utilization":"used_percent");
            if(!percent.is_number() || !std::isfinite(percent.get<double>())) throw std::runtime_error("invalid");
            out.metrics[name]=std::clamp(percent.get<double>(),0.0,100.0);
            const auto reset=w.find(claude?"resets_at":"reset_at");
            if(reset!=w.end() && !reset->is_null()) {
                int64_t value=0;
                if(claude) {auto parsed=iso_reset(reset->get<std::string>());if(!parsed) throw std::runtime_error("invalid");value=*parsed;}
                else {if(!reset->is_number_integer()) throw std::runtime_error("invalid");value=reset->get<int64_t>();}
                if(value<0 || value>1000000000000LL) throw std::runtime_error("invalid");
                out.metrics[std::string(name)+"_reset"]=value;
            }
        };
        if(claude) {for(auto [key,name]:{std::pair{"five_hour","five_hour"},{"seven_day","week"}}) if(reply.contains(key) && !reply[key].is_null()) add(reply[key],name);}
        else {const auto& limits=reply.at("rate_limit");for(auto key:{"primary_window","secondary_window"}) {
            if(!limits.contains(key) || limits[key].is_null()) continue;
            const auto& w=limits[key];if(!w.at("limit_window_seconds").is_number_integer()) throw std::runtime_error("invalid");
            auto seconds=w["limit_window_seconds"].get<int64_t>();
            if(seconds==18000) add(w,"five_hour");else if(seconds==604800) add(w,"week");
        }}
        out.ok=!out.metrics.empty();out.reason=out.ok?"":"limits_not_supplied";
    } catch(...) {out.metrics=Json::object();out.reason="invalid_data";}
    return out;
}
UsageResult login_usage(bool claude,const std::filesystem::path& file,const UsageTransport& transport) {
    UsageResult out;out.source=claude?"claude-oauth-usage":"codex-wham-usage";
    out.reason="not_signed_in";
    try {
        // ReadFile avoids an unwiped stdio/filebuf copy of the credential bytes.
        struct LoginFile {HANDLE h;~LoginFile(){if(h!=INVALID_HANDLE_VALUE) CloseHandle(h);}} input{CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr)};
        if(input.h==INVALID_HANDLE_VALUE) return out;
        LARGE_INTEGER size{};
        if(!GetFileSizeEx(input.h,&size) || size.QuadPart<=0 || size.QuadPart>262144) {out.reason="invalid_login";return out;}
        SecretString raw;raw.resize(size_t(size.QuadPart));DWORD read=0;
        if(!ReadFile(input.h,raw.data(),DWORD(raw.size()),&read,nullptr) || read!=raw.size()) {out.reason="invalid_login";return out;}
        auto j=LoginJson({raw.data(),raw.size()}).parse();
        const auto& data=j.at(claude?"claudeAiOauth":"tokens");
        const auto& token=data.at(claude?"accessToken":"access_token").get_ref<const SecretString&>();
        std::string_view access(token.data(),token.size()),account;
        if(!claude && data.contains("account_id") && !data["account_id"].is_null()) {
            const auto& a=data["account_id"].get_ref<const SecretString&>();account={a.data(),a.size()};
            if(!header_safe(account)) {out.reason="invalid_login";return out;}
        }
        if(!header_safe(access)) {out.reason="invalid_login";return out;}
        if(claude && data.contains("expiresAt") && data["expiresAt"].is_number() && data["expiresAt"].template get<double>()<=double(std::time(nullptr))*1000) {out.reason="expired_claude";return out;}
        // JWT expiry is optional; opaque tokens are handled by the provider's 401 response.
        if(!claude) {
            auto dot=access.find('.'),end=dot==access.npos?access.npos:access.find('.',dot+1);
            if(end!=access.npos) {
                SecretString payload;unsigned bits=0;int count=0;
                for(auto c:access.substr(dot+1,end-dot-1)) {
                    int v=c>='A'&&c<='Z'?c-'A':c>='a'&&c<='z'?c-'a'+26:c>='0'&&c<='9'?c-'0'+52:c=='-'?62:c=='_'?63:-1;
                    if(v<0) break;
                    bits=(bits<<6)|v;count+=6;if(count>=8) {count-=8;payload.push_back(char((bits>>count)&255));}
                }
                try {auto jwt=LoginJson({payload.data(),payload.size()}).parse();
                    if(jwt.contains("exp") && jwt["exp"].is_number() && jwt["exp"].template get<double>()<=double(std::time(nullptr))) {out.reason="expired_codex";return out;}
                } catch(...) {} // opaque/non-JWT token: let the provider decide
            }
        }
        auto response=transport(claude,access,account);
        if(response.status==401) out.reason=claude?"expired_claude":"expired_codex";
        else if(response.status==403) out.reason="usage_forbidden";
        else if(response.status==429) out.reason="usage_rate_limited";
        else if(response.status!=200) out.reason="usage_failed";
        else if(response.body.size()>65536) out.reason="invalid_data";
        else {auto reply=Json::parse(response.body,nullptr,false);if(reply.is_discarded()) out.reason="invalid_data";else return endpoint_usage(claude,reply);}
    } catch(...) {out.reason="invalid_login";}
    return out;
}
namespace {
struct Handle {
    HANDLE h=INVALID_HANDLE_VALUE;
    Handle()=default;Handle(const Handle&)=delete;
    ~Handle(){if(h!=INVALID_HANDLE_VALUE && h) CloseHandle(h);}
};
std::filesystem::path env_path(const wchar_t* key) {
    wchar_t value[32768];auto n=GetEnvironmentVariableW(key,value,32768);
    return n && n<32768?std::filesystem::path(value):std::filesystem::path{};
}
std::filesystem::path executable(const wchar_t* name) {
    wchar_t path[32768];
    auto n=SearchPathW(nullptr,name,L".exe",32768,path,nullptr);
    if(n && n<32768) return path;
    if(std::wstring(name)==L"codex") {
        n=SearchPathW(nullptr,L"codex.cmd",nullptr,32768,path,nullptr);
        if(n && n<32768) {
            auto base=std::filesystem::path(path).parent_path()/L"node_modules/@openai/codex/node_modules/@openai/codex-win32-x64/vendor/x86_64-pc-windows-msvc/bin/codex.exe";
            if(std::filesystem::is_regular_file(base)) return base;
        }
    }
    if(std::wstring(name)==L"claude") {
        // Metadata only: never open scripts, settings or credential files.
        for(auto entry:{std::pair{L"USERPROFILE",L".local/bin/claude.exe"},
                        std::pair{L"APPDATA",L"npm/claude.exe"},
                        std::pair{L"LOCALAPPDATA",L"Programs/Claude/claude.exe"},
                        std::pair{L"LOCALAPPDATA",L"Programs/claude-code/claude.exe"}}) {
            auto base=env_path(entry.first);if(base.empty()) continue;
            auto candidate=base/entry.second;
            if(std::filesystem::is_regular_file(candidate)) return candidate;
        }
    }
    return {};
}
// Child uses isolated paths unless the provider has explicit account consent.
// Only named non-secret environment paths are obtained; API credentials are never copied.
// Bounded pipes and a kill-on-close job also contain a hung CLI.
std::mutex child_mutex;
struct Child {
    std::unique_lock<std::mutex> serial{child_mutex};
    Handle process,job,input,output;std::string pending;ULONGLONG deadline;
    Child(const std::filesystem::path& exe,const std::wstring& args,const std::filesystem::path& dir,bool account=false):deadline(GetTickCount64()+12000) {
        std::filesystem::create_directories(dir);
        std::vector<std::wstring> environment;
        auto inherit=[&](const wchar_t* key) {
            auto value=env_path(key);if(!value.empty()) environment.push_back(std::wstring(key)+L"="+value.wstring());
        };
        for(auto key:{L"PATH",L"PATHEXT",L"SYSTEMROOT",L"WINDIR",L"COMSPEC",L"OS",L"PROCESSOR_ARCHITECTURE",L"NUMBER_OF_PROCESSORS"}) inherit(key);
        for(auto key:{L"CODEX_HOME",L"CLAUDE_CONFIG_DIR",L"HOME",L"USERPROFILE",L"APPDATA",L"LOCALAPPDATA"}) {
            if(account) inherit(key);
            else environment.push_back(std::wstring(key)+L"="+dir.wstring());
        }
        for(auto key:{L"TEMP",L"TMP"}) environment.push_back(std::wstring(key)+L"="+dir.wstring());
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
UsageResult query(const std::filesystem::path& exe,const std::filesystem::path& dir,const std::atomic<bool>& enabled,bool account) {
    UsageResult out;out.source="codex-app-server";
    if(exe.empty()) {out.reason="not_in_path";return out;}
    try {
        {Child child(exe,L"app-server --help",dir);if(child.help(enabled).find("stdio")==std::string::npos) {out.reason="unsupported_version";return out;}}
        Child child(exe,L"app-server --listen stdio://",dir,account);
        child.send({{"id",1},{"method","initialize"},{"params",{{"clientInfo",{{"name","screenbuddy_usage"},{"version","1"}}}}}});
        if(child.reply(1,enabled).contains("error")) {out.reason="unsupported_version";return out;}
        child.send({{"method","initialized"}});
        child.send({{"id",2},{"method","account/read"},{"params",{{"refreshToken",false}}}});
        const auto accountReply=child.reply(2,enabled);
        if(accountReply.contains("error")) {out.reason="unsupported_version";return out;}
        const auto& a=accountReply.at("result").at("account");
        if(a.is_null()) {out.reason=account?"signed_out":"signed_out_isolated_home";return out;}
        if(a.value("type",std::string())!="chatgpt") {out.reason="api_key_no_limits";return out;}
        child.send({{"id",3},{"method","account/rateLimits/read"}});
        return codex_usage(child.reply(3,enabled));
    } catch(...) {out.reason="cli_failed_or_timeout";return out;}
}
}
struct UsageWorker::Impl {
    Model& model;std::filesystem::path directory;std::atomic<bool> enabled=false;
    bool codex_account=false,claude_account=false;UsageQuery query_override;
    bool stop=false;std::mutex mutex;std::condition_variable cv;std::thread thread;
    Impl(Model& m,std::filesystem::path p,bool codex,bool claude,UsageQuery q):model(m),directory(std::move(p)),codex_account(codex),claude_account(claude),query_override(std::move(q)),thread([this]{run();}) {}
    ~Impl() {{std::lock_guard lock(mutex);stop=true;enabled=false;}cv.notify_all();thread.join();}
    void run() {
        std::unique_lock lock(mutex);
        std::chrono::steady_clock::time_point next[2]{};
        while(!stop) {
            cv.wait(lock,[&]{return stop || enabled.load();});if(stop) break;
            auto now=std::chrono::steady_clock::now();
            auto due=std::min(next[0],next[1]);
            if(now<due) {cv.wait_until(lock,due,[&]{return stop || !enabled.load();});continue;}
            for(int i=0;i<2 && !stop;++i) {
                if(!enabled || std::chrono::steady_clock::now()<next[i]) continue;
                const bool claude=i==0,account=claude?claude_account:codex_account;
                lock.unlock();UsageResult result;
                if(account) result.source=claude?"claude-oauth-usage":"codex-wham-usage";
                try {
                    if(account) {
                        if(query_override) result=query_override(claude);
                        else {auto home=env_path(L"USERPROFILE");
                            if(home.empty()) result.reason="not_signed_in";
                            else result=login_usage(claude,home/(claude?L".claude/.credentials.json":L".codex/auth.json"),https_usage);
                        }
                    } else if(!claude) result=query(executable(L"codex"),directory/L"codex",enabled,false);
                    else {
                        result.source="claude-cli";auto exe=executable(L"claude");result.reason="not_in_path";
                        if(!exe.empty()) {Child help(exe,L"--help",directory/L"claude");help.help(enabled);result.reason="no_verified_usage_command";}
                    }
                } catch(...) {result.reason="usage_failed";}
                if(enabled) model.usage(claude?"claude":"codex",result.metrics,result.source,result.reason,std::time(nullptr));
                lock.lock();next[i]=std::chrono::steady_clock::now()+std::chrono::seconds(usage_delay(result.ok));
            }
        }
    }
};
UsageWorker::UsageWorker(Model& model,std::filesystem::path p,bool codex,bool claude,UsageQuery q):d_(std::make_unique<Impl>(model,std::move(p),codex,claude,std::move(q))) {}
UsageWorker::~UsageWorker()=default;
void UsageWorker::enable(bool enabled) {
    {std::lock_guard lock(d_->mutex);d_->enabled=enabled;}
    if(!enabled) for(auto provider:{"claude","codex"}) d_->model.usage(provider,Json::object(),"none","disabled",std::time(nullptr));
    d_->cv.notify_all();
}
} // namespace fl::ai
