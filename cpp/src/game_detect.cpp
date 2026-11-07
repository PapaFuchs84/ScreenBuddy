// © 2026 FuchsLabs – no process memory access, hooks or injection.
#include "game_detect.h"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <map>
#include <cmath>
#include "util.h"
namespace fl::game {
namespace {
using Json=nlohmann::json;
// Bounded Valve KeyValues parser: quoted/unquoted strings, comments and nested objects.
class Vdf {
    std::string_view s; size_t p=0, count=0;
    std::optional<std::string> token() {
        for(;;) {
            while(p<s.size() && isspace(static_cast<unsigned char>(s[p]))) ++p;
            if(p+1<s.size() && s.substr(p,2)=="//") {while(p<s.size() && s[p]!='\n')++p;} else break;
        }
        if(p==s.size())return std::nullopt;
        char c=s[p++];if(c=='{' || c=='}')return std::string(1,c);
        std::string out;
        if(c=='"') {
            bool closed=false;
            while(p<s.size()) {c=s[p++];if(c=='"'){closed=true;break;}if(c=='\\' && p<s.size() && (s[p]=='\\' || s[p]=='"')) c=s[p++];out+=c;}
            if(!closed)throw 1;
        } else {out+=c;while(p<s.size() && !isspace(static_cast<unsigned char>(s[p])) && s[p]!='{' && s[p]!='}')out+=s[p++];}
        if(out.size()>32768)throw 1;return out;
    }
    Json object(int depth,bool nested) {
        if(depth>16)throw 1;Json out=Json::object();
        for(;;) {auto k=token();if(!k){if(nested)throw 1;return out;}if(*k=="}"){if(!nested)throw 1;return out;}if(k->empty() || *k=="{" || ++count>20000)throw 1;
            auto v=token();if(!v || *v=="}")throw 1;out[*k]=*v=="{"?object(depth+1,true):Json(*v);}
    }
public:
    explicit Vdf(std::string_view text):s(text) {}
    Json parse(){if(s.size()>2*1024*1024)throw 1;return object(0,false);}
};
std::string read(const std::filesystem::path& p) {
    std::error_code ec;auto n=std::filesystem::file_size(p,ec);if(ec || n>2*1024*1024)return {};
    std::ifstream f(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});
}
std::string norm(const std::filesystem::path& p){return casefold(narrow(p.lexically_normal().wstring()));}
bool digits(const std::string& s){return !s.empty() && s.size()<16 && std::all_of(s.begin(),s.end(),[](char c){return c>='0' && c<='9';});}
std::vector<std::string> lines(const std::string& s){std::vector<std::string> out;for(auto& x:split(s,"\n"))if(!trim(x).empty())out.push_back(trim(x));return out;}
struct Process {DWORD pid;std::filesystem::path path;};
std::vector<Process> processes() {
    std::vector<Process> out;HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h==INVALID_HANDLE_VALUE)return out;
    PROCESSENTRY32W e{sizeof e};if(Process32FirstW(h,&e))do {
        HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.th32ProcessID);if(!p)continue;
        wchar_t path[32768];DWORD n=32768;if(QueryFullProcessImageNameW(p,0,path,&n))out.push_back({e.th32ProcessID,std::filesystem::path(std::wstring(path,n))});CloseHandle(p);
    }while(Process32NextW(h,&e));CloseHandle(h);return out;
}
std::filesystem::path steam_root(){wchar_t path[32768]{};DWORD n=sizeof path;return RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath",RRF_RT_REG_SZ,nullptr,path,&n)==ERROR_SUCCESS?std::filesystem::path(path):std::filesystem::path();}
DWORD running_app(){DWORD id=0,n=sizeof id;RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"RunningAppID",RRF_RT_REG_DWORD,nullptr,&id,&n);return id;}
class WindowsRegistry final : public Registry {
public:
    std::vector<std::wstring> subkeys(const std::wstring& key) const override {
        std::vector<std::wstring> out;HKEY h=nullptr;
        if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,key.c_str(),0,KEY_ENUMERATE_SUB_KEYS|KEY_WOW64_64KEY,&h)!=ERROR_SUCCESS)return out;
        for(DWORD i=0;i<10000;++i){wchar_t name[256];DWORD n=256;auto rc=RegEnumKeyExW(h,i,name,&n,nullptr,nullptr,nullptr,nullptr);
            if(rc==ERROR_NO_MORE_ITEMS)break;if(rc==ERROR_SUCCESS)out.emplace_back(name,n);}
        RegCloseKey(h);return out;
    }
    std::wstring value(const std::wstring& key,const wchar_t* name) const override {
        wchar_t text[32768]{};DWORD n=sizeof text;
        if(RegGetValueW(HKEY_LOCAL_MACHINE,key.c_str(),name,RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ|RRF_SUBKEY_WOW6464KEY,nullptr,text,&n)!=ERROR_SUCCESS)return {};
        return text;
    }
};
const WindowsRegistry windows_registry;
std::filesystem::path install_path(std::wstring text) {
    text=widen(trim(narrow(text)));
    if(text.size()>1 && text.front()==L'"' && text.back()==L'"')text=text.substr(1,text.size()-2);
    auto p=std::filesystem::path(text).lexically_normal();
    // A drive root must never turn every process on that volume into a game.
    if(!p.is_absolute() || p.relative_path().empty())return {};
    while(p.has_relative_path() && p.filename().empty())p=p.parent_path();
    return p;
}
}
bool beneath(const std::filesystem::path& child,const std::filesystem::path& root){auto c=norm(child),r=norm(root);while(!r.empty() && (r.back()=='\\' || r.back()=='/'))r.pop_back();return !r.empty() && c.size()>r.size() && c.compare(0,r.size(),r)==0 && (c[r.size()]=='\\' || c[r.size()]=='/');}
bool default_excluded(const std::filesystem::path& executable) {
    auto name=casefold(narrow(executable.filename().wstring()));
    static const std::vector<std::string> defaults={"explorer.exe","chrome.exe","msedge.exe","firefox.exe","vlc.exe","wmplayer.exe","steam.exe","epicgameslauncher.exe","applicationframehost.exe",
        "battle.net.exe","eadesktop.exe","eabackgroundservice.exe","origin.exe","ubisoftconnect.exe","upc.exe","galaxyclient.exe","galaxyclientservice.exe","rockstargameslauncher.exe","agent.exe","beservice.exe"};
    return std::find(defaults.begin(),defaults.end(),name)!=defaults.end() || name.find("launcher")!=std::string::npos ||
        name.starts_with("easyanticheat") || name.starts_with("blizzarderror") || name.find("crashhandler")!=std::string::npos;
}
bool matches_game(const Game& g,const std::filesystem::path& executable,bool foreground_fullscreen,const std::string& app) {
    if(default_excluded(executable))return false;
    bool exact=!g.executable.empty() && norm(g.executable)==norm(executable);
    bool known=!g.install.empty() && beneath(executable,g.install);
    // Only the user-supplied EXE list bypasses the window gate. Steam keeps its RunningAppID shortcut.
    return (exact && g.id.starts_with("exe:")) || (known && (foreground_fullscreen || (!g.steam_id.empty() && g.steam_id==app && app!="0")));
}
std::vector<Game> registry_games(const Registry& registry,const DirectoryExists& exists) {
    std::vector<Game> out;
    struct Uninstall {std::wstring name,publisher;std::filesystem::path install;};
    std::vector<Uninstall> uninstalls;
    const std::vector<std::wstring> bases={L"SOFTWARE\\WOW6432Node\\",L"SOFTWARE\\"};
    for(const auto& base:bases)for(const auto& id:registry.subkeys(base+L"Microsoft\\Windows\\CurrentVersion\\Uninstall")) {
        auto key=base+L"Microsoft\\Windows\\CurrentVersion\\Uninstall\\"+id;
        auto path=install_path(registry.value(key,L"InstallLocation"));if(path.empty())continue;
        uninstalls.push_back({registry.value(key,L"DisplayName"),registry.value(key,L"Publisher"),path});
    }
    auto add=[&](const std::string& source,std::wstring name,const std::filesystem::path& path,std::wstring exe=L"") {
        if(path.empty())return;
        if(name.empty())name=path.filename().wstring();
        auto label=casefold(trim(narrow(name)));
        if(label=="ea app" || label=="ea desktop" || label=="origin" || label=="gog galaxy" || label=="ubisoft connect" || label=="battle.net")return;
        if(default_excluded(std::filesystem::path(name).filename().wstring()+L".exe"))return;
        if(name.empty())return;
        Game g;g.id=source+":"+norm(path);g.name=narrow(name);g.install=path;
        if(!exe.empty()){g.executable=(path/std::filesystem::path(exe)).lexically_normal();
            if(!beneath(g.executable,path) || casefold(narrow(g.executable.extension().wstring()))!=".exe")g.executable.clear();}
        if(std::none_of(out.begin(),out.end(),[&](const Game& old){return norm(old.install)==norm(path);}))out.push_back(g);
    };
    for(const auto& base:bases) {
        for(const auto& id:registry.subkeys(base+L"GOG.com\\Games")) {
            auto key=base+L"GOG.com\\Games\\"+id;
            add("gog",registry.value(key,L"gameName"),install_path(registry.value(key,L"path")),registry.value(key,L"exe"));
        }
        for(const auto& id:registry.subkeys(base+L"Ubisoft\\Launcher\\Installs")) {
            auto key=base+L"Ubisoft\\Launcher\\Installs\\"+id;auto path=install_path(registry.value(key,L"InstallDir"));std::wstring name;
            for(const auto& u:uninstalls)if(norm(u.install)==norm(path) && !u.name.empty()){name=u.name;break;}
            add("ubisoft",name,path);
        }
        for(const auto& branch:{L"EA Games",L"Electronic Arts",L"Rockstar Games"})for(const auto& id:registry.subkeys(base+branch)) {
            auto key=base+branch+L"\\"+id;bool rockstar=std::wstring_view(branch)==L"Rockstar Games";
            add(rockstar?"rockstar":"ea",id,install_path(registry.value(key,rockstar?L"InstallFolder":L"Install Dir")));
        }
    }
    std::vector<std::filesystem::path> battle_roots;
    for(const auto& u:uninstalls) {
        auto publisher=casefold(trim(narrow(u.publisher)));auto name=casefold(narrow(u.name));
        if(publisher=="electronic arts" || publisher=="electronic arts, inc." || publisher=="electronic arts inc." || publisher=="ea")add("ea",u.name,u.install);
        if(publisher=="blizzard entertainment" || publisher=="blizzard entertainment, inc." || publisher=="blizzard entertainment inc.") {
            if(name.find("battle.net")!=std::string::npos)battle_roots.push_back(u.install);
            else add("blizzard",u.name,u.install);
        }
    }
    for(const auto& base:bases)for(const auto& branch:{L"Blizzard Entertainment\\Battle.net",L"Battle.net"}) {
        auto path=install_path(registry.value(base+branch,L"InstallPath"));if(!path.empty())battle_roots.push_back(path);
    }
    // Bounded, named game directories only; never treat the launcher directory as a game.
    for(const auto& root:battle_roots)for(const auto& name:{L"World of Warcraft",L"Diablo III",L"Diablo IV",L"StarCraft",L"StarCraft II",L"Hearthstone",L"Heroes of the Storm",L"Overwatch",L"Warcraft III",L"Warcraft III Reforged"})
        for(const auto& parent:{root,root.parent_path()}){auto path=parent/name;if(exists(path))add("blizzard",name,path);}
    return out;
}
std::optional<Game> parse_acf(std::string_view text,const std::filesystem::path& steamapps) {
    try {auto j=Vdf(text).parse().at("AppState");Game g;g.steam_id=j.at("appid").get<std::string>();g.name=j.at("name").get<std::string>();auto dir=j.at("installdir").get<std::string>();
        if(!digits(g.steam_id) || g.name.empty() || dir.empty())return {};g.install=(steamapps/L"common"/widen(dir)).lexically_normal();if(!beneath(g.install,steamapps/L"common"))return {};g.id="steam:"+g.steam_id;return g;
    }catch(...){return {};}
}
std::optional<Game> parse_epic(std::string_view text){try {
    if(text.size()>2*1024*1024)return {};auto j=Json::parse(text);Game g;g.id="epic:"+j.at("AppName").get<std::string>();g.name=j.at("DisplayName").get<std::string>();g.install=widen(j.at("InstallLocation").get<std::string>());
    if(g.id=="epic:" || g.name.empty() || !g.install.is_absolute())return {};
    if(j.contains("LaunchExecutable")) {g.executable=(g.install/widen(j.at("LaunchExecutable").get<std::string>())).lexically_normal();if(!beneath(g.executable,g.install))return {};}
    return g;
}catch(...){return {};}}
std::vector<std::filesystem::path> parse_libraries(std::string_view text){std::vector<std::filesystem::path> out;try {auto j=Vdf(text).parse().at("libraryfolders");for(auto& [k,v]:j.items())if(digits(k)) {auto p=std::filesystem::path(widen(v.is_string()?v.get<std::string>():v.at("path").get<std::string>()));if(p.is_absolute())out.push_back(p);}}catch(...){out.clear();}return out;}
bool Detector::update(std::optional<Game> candidate,double now){
    if(!std::isfinite(now) || now<last_)return false;last_=now;auto before=active();
    switch(state_) {
    case State::idle: if(candidate){game_=candidate;since_=now;state_=State::starting;}break;
    case State::starting:
        if(!candidate){game_.reset();state_=State::idle;}
        else if(!game_ || !(*game_==*candidate)){game_=candidate;since_=now;}
        else if(now-since_>=3){state_=State::running;started_=now;}
        break;
    case State::running: if(!candidate || !(*candidate==*game_)){state_=State::ending;since_=now;}break;
    case State::ending:
        if(candidate && *candidate==*game_)state_=State::running;
        else if(now-since_>=5){game_.reset();state_=State::idle;if(candidate){game_=candidate;since_=now;state_=State::starting;}}
        break;
    }
    return before!=active();
}
Discovery::Discovery(std::string additional,std::string exclusions,const Registry* registry):additional_(lines(additional)),excluded_(lines(exclusions)),registry_(registry?registry:&windows_registry){}
void Discovery::refresh(){
    library_.clear();auto root=steam_root();if(!root.empty()) {
        auto libs=parse_libraries(read(root/L"steamapps"/L"libraryfolders.vdf"));libs.push_back(root);
        for(const auto& lib:libs){std::error_code ec;size_t n=0;for(std::filesystem::directory_iterator it(lib/L"steamapps",ec),end;!ec && it!=end && n++<10000;it.increment(ec))if(it->path().extension()==L".acf")if(auto g=parse_acf(read(it->path()),lib/L"steamapps"))library_.push_back(*g);}
    }
    wchar_t data[32768]{};DWORD n=GetEnvironmentVariableW(L"ProgramData",data,32768);if(n && n<32768){std::error_code ec;size_t count=0;for(std::filesystem::directory_iterator it(std::filesystem::path(data)/L"Epic/EpicGamesLauncher/Data/Manifests",ec),end;!ec && it!=end && count++<10000;it.increment(ec))if(it->path().extension()==L".item")if(auto g=parse_epic(read(it->path())))library_.push_back(*g);}
    auto extra=registry_games(*registry_,[](const std::filesystem::path& path){std::error_code ec;return std::filesystem::is_directory(path,ec);});
    library_.insert(library_.end(),extra.begin(),extra.end());
    for(const auto& path:additional_){Game g;g.executable=widen(path);if(!g.executable.is_absolute())continue;g.id="exe:"+norm(g.executable);g.name=narrow(g.executable.stem().wstring());library_.push_back(g);}
}
std::optional<Game> Discovery::poll(const std::optional<Game>& active,double now){
    if(now>=refresh_at_){refresh();refresh_at_=now+60;}
    DWORD fg=0;auto hwnd=GetForegroundWindow();GetWindowThreadProcessId(hwnd,&fg);RECT wr{};MONITORINFO mi{sizeof mi};bool full=false;
    if(hwnd && !IsIconic(hwnd) && GetWindowRect(hwnd,&wr) && GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONULL),&mi))full=wr.left<=mi.rcMonitor.left && wr.top<=mi.rcMonitor.top && wr.right>=mi.rcMonitor.right && wr.bottom>=mi.rcMonitor.bottom;
    QUERY_USER_NOTIFICATION_STATE state{};if(SUCCEEDED(SHQueryUserNotificationState(&state)) && state==QUNS_RUNNING_D3D_FULL_SCREEN)full=true;
    auto ps=processes();auto app=std::to_string(running_app());
    auto excluded=[&](const Process& p){auto name=casefold(narrow(p.path.filename().wstring()));
        if(default_excluded(p.path))return true;
        for(auto& x:excluded_)if(casefold(x)==name || casefold(x)==norm(p.path))return true;return false;};
    // Keep a confirmed process through Alt-Tab, never extend it using only RunningAppID.
    if(active)for(const auto& p:ps)if(p.pid==active->pid && norm(p.path)==norm(active->executable) && !excluded(p))return active;
    for(const auto& p:ps) {if(excluded(p))continue;
        for(auto g:library_) {
            if(!matches_game(g,p.path,p.pid==fg && full,app))continue;
            g.pid=p.pid;g.executable=p.path;return g;
        }
        // Xbox installs are discovered on fixed drives, with a foreground full-screen gate.
        if(p.pid==fg && full)for(wchar_t drive=L'A';drive<=L'Z';++drive){std::wstring root{drive,L':',L'\\'};if(GetDriveTypeW(root.c_str())!=DRIVE_FIXED)continue;auto base=std::filesystem::path(root)/L"XboxGames";if(beneath(p.path,base)){Game g;g.id="xbox:"+norm(p.path);g.name=narrow(p.path.stem().wstring());g.executable=p.path;g.pid=p.pid;return g;}}
    }
    return {};
}
}
