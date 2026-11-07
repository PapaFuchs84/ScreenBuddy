// © 2026 FuchsLabs
#include "ai_status.h"
#include "secure.h"
#include "plugins.h"
#include <httplib.h>
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>
#include <stdexcept>

namespace fl::ai {
namespace {
std::string read(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot read AI hook file.");
    f.seekg(0, std::ios::end);
    if (f.tellg() > 1024 * 1024) throw std::runtime_error("AI hook file is too large.");
    f.seekg(0); return {std::istreambuf_iterator<char>(f), {}};
}
void write(const std::filesystem::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(data.data(), data.size()); f.close();
    if (!f) throw std::runtime_error("Cannot write AI hook file.");
}
bool token(const std::string& s, size_t limit) {
    return !s.empty() && s.size() <= limit && std::all_of(s.begin(),s.end(),[](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='_' || c=='-' || c=='.' || c==':' || c=='/';
    });
}
std::string day(std::time_t now) {
    std::tm t{}; localtime_s(&t,&now); char b[16]; std::strftime(b,sizeof b,"%Y-%m-%d",&t); return b;
}
int rank(const std::string& s) { return s=="waiting"?0:s=="working"?1:s=="idle"?2:3; }
const char* fields[] = {"context","five_hour","week","five_hour_reset","week_reset","cost_today","tokens_today","cost_session","tokens_session"};
Json empty_metrics() { Json j=Json::object(); for(auto k:fields) j[k]=nullptr; j["source"]="none"; j["reason"]="no_data"; j["checked_at"]=nullptr; j["updated_at"]=nullptr; return j; }
}
std::optional<Event> parse(std::string_view body) {
    if(body.size()>8192) return {};
    try {
        auto parsed=plugins::parse_strict_json(body,8192);
        if(!parsed || !parsed->is_object()) return {};
        const auto& j=*parsed;
        Event e; e.provider=j.at("provider").get<std::string>(); e.session=j.at("session_id").get<std::string>();
        e.kind=j.at("hook_event_name").get<std::string>();
        if((e.provider!="claude" && e.provider!="codex") || !token(e.session,128)) return {};
        static const std::vector<std::string> kinds={"SessionStart","SessionEnd","UserPromptSubmit","PreToolUse","PostToolUse","PostToolUseFailure","PermissionRequest","Stop","Interrupt","Usage"};
        if(std::find(kinds.begin(),kinds.end(),e.kind)==kinds.end()) return {};
        if(j.contains("source")) {e.source=j.at("source").get<std::string>(); if(e.source!="claude-statusline" && e.source!="hook") return {}; }
        for(auto [key,out]:{std::pair{"tool_name",&e.tool},{"model",&e.model}}) {
            if(j.contains(key)) { *out=j.at(key).get<std::string>(); std::string probe=*out;
                if(std::string(key)=="model") probe.erase(std::remove_if(probe.begin(),probe.end(),[](char c){return c=='['||c==']';}),probe.end()); // e.g. "claude-opus-5-5[1m]"
                if(!out->empty() && (probe.empty() || !token(probe,80))) return {}; }
        }
        for(auto k:fields) if(j.contains(k) && !j[k].is_null()) {
            if(!j[k].is_number()) return {};
            double v=j[k].get<double>();
            if(!std::isfinite(v) || v<0 || v>1e15) return {};
            if((std::string(k)=="context" || std::string(k)=="five_hour" || std::string(k)=="week") && v>100) return {};
            if((std::string(k).find("reset")!=std::string::npos || std::string(k)=="tokens_today" || std::string(k)=="tokens_session") && std::floor(v)!=v) return {};
            e.metrics[k]=v;
        }
        return e;
    } catch(const Json::exception&) { return {}; }
}
bool Model::ingest(std::string_view body,std::time_t now) {
    auto e=parse(body); if(!e) return false;
    std::lock_guard lk(mutex_);
    if(day_!=day(now)) { for(auto& [p,m]:metrics_) {m["cost_today"]=nullptr;m["tokens_today"]=nullptr;} day_=day(now); }
    if(e->kind=="Usage") {
        if(!metrics_.count(e->provider)) metrics_[e->provider]=empty_metrics();
        auto& m=metrics_[e->provider];
        // Status lines are snapshots, so absent windows must clear older measurements.
        for(auto k:fields) if(std::string(k)!="cost_today" && std::string(k)!="tokens_today") m[k]=nullptr;
        for(auto& [k,v]:e->metrics) m[k]=v;
        m["source"]=e->source.empty()?"hook":e->source;
        m["checked_at"]=now;m["updated_at"]=now;
        m["reason"]=(m["five_hour"].is_null() && m["week"].is_null())?"limits_not_supplied":"";
        updated_=now;return true; // measurement must not create/resurrect/change session state
    }
    auto key=e->provider+":"+e->session;
    auto it=sessions_.find(key);
    if(it==sessions_.end()) {
        // Bound memory. Never evict an active session to accept unbounded input.
        if(sessions_.size()>=128) {
            auto old=std::min_element(sessions_.begin(),sessions_.end(),[](const auto& a,const auto& b) {
                if((a.second.state=="off")!=(b.second.state=="off")) return a.second.state=="off";
                return a.second.updated<b.second.updated;
            });
            if(old->second.state!="off") return false;
            sessions_.erase(old);
        }
        it=sessions_.emplace(key,Session{e->provider,e->session,"off",{}, {},now,now,0}).first;
    }
    auto& s=it->second;
    // Late tool/stop deliveries cannot resurrect a session after SessionEnd.
    if(s.state=="off" && s.revision && e->kind!="SessionStart") return true;
    auto state=e->kind=="SessionEnd"?"off":e->kind=="PermissionRequest"?"waiting":
        (e->kind=="SessionStart" || e->kind=="Stop" || e->kind=="Interrupt")?"idle":"working";
    if(s.state!=state) {s.state=state;s.since=now;}
    s.updated=now;s.revision=++revision_;updated_=now;
    if(!e->model.empty()) s.model=e->model;
    if(!e->tool.empty()) s.tool=e->tool;
    if(e->kind=="Stop" || e->kind=="SessionEnd" || e->kind=="UserPromptSubmit" || e->kind=="Interrupt") s.tool.clear();
    if(!metrics_.count(e->provider)) metrics_[e->provider]=empty_metrics();
    for(auto& [k,v]:e->metrics) metrics_[e->provider][k]=v;
    if(!e->metrics.empty()) {auto& m=metrics_[e->provider];m["source"]="hook";m["updated_at"]=now;m["checked_at"]=now;m["reason"]="";}
    return true;
}
void Model::usage(const std::string& provider,const Json& values,const std::string& source,const std::string& reason,std::time_t now) {
    if(provider!="claude" && provider!="codex") return;
    auto e=Json{{"provider",provider},{"session_id","usage"},{"hook_event_name","Usage"}};
    for(auto k:fields) if(values.contains(k)) e[k]=values[k];
    const auto checked=parse(e.dump());if(!checked) return;
    std::lock_guard lk(mutex_);
    if(!metrics_.count(provider)) metrics_[provider]=empty_metrics();
    auto& m=metrics_[provider];
    // Failed attempts retain last success and its timestamp, but never claim freshness.
    if(reason.empty()) {for(auto k:fields) if(std::string(k)!="cost_today" && std::string(k)!="tokens_today") m[k]=nullptr;
        for(auto& [k,v]:checked->metrics) m[k]=v;
        m["source"]=source;m["updated_at"]=now;updated_=now;}
    m["attempt_source"]=source;
    if(m["updated_at"].is_null()) m["source"]=source;
    m["cli_reason"]=reason;
    if(m["updated_at"].is_null() || provider=="codex" || reason=="disabled") m["reason"]=reason;
    m["checked_at"]=now;
}
Snapshot Model::snapshot(std::time_t now) const {
    std::lock_guard lk(mutex_); Snapshot out;out.updated=updated_;
    for(auto provider:{"claude","codex"}) {
        auto it=metrics_.find(provider); out.providers[provider]=it==metrics_.end()?empty_metrics():it->second;
        if(day_!=day(now)) {out.providers[provider]["cost_today"]=nullptr;out.providers[provider]["tokens_today"]=nullptr;}
    }
    for(auto& [key,s]:sessions_) {
        if(s.state!="off") out.sessions.push_back(s);
        auto best=out.representatives.find(s.provider);
        if(best==out.representatives.end() || rank(s.state)<rank(best->second.state) ||
           (rank(s.state)==rank(best->second.state) && s.updated>best->second.updated)) out.representatives[s.provider]=s;
    }
    std::sort(out.sessions.begin(),out.sessions.end(),[](const auto& a,const auto& b) {
        if(rank(a.state)!=rank(b.state)) return rank(a.state)<rank(b.state);
        if(a.updated!=b.updated) return a.updated>b.updated;
        return a.provider+":"+a.id<b.provider+":"+b.id;
    });
    return out;
}
Json Model::tiles(std::time_t now) const {
    return tile_values(snapshot(now),now);
}
bool tile_present(const Snapshot& snap,const std::string& block) {
    auto present=[&](const std::string& provider) {
        auto it=snap.providers.find(provider);
        return snap.representatives.contains(provider) || (it!=snap.providers.end() && it->second.contains("updated_at") && it->second["updated_at"].is_number());
    };
    if(block=="ai.limits") return present("claude") || present("codex");
    if(block=="ai.claude" || block.starts_with("ai.claude.")) return present("claude");
    if(block=="ai.codex" || block.starts_with("ai.codex.")) return present("codex");
    return false;
}
Json tile_values(const Snapshot& snap,std::time_t now) {
    Json out=Json::object();
    for(auto p:{"claude","codex"}) {
        auto& j=out[p]; j=snap.providers.at(p);j["state"]="off";j["tool"]=nullptr;j["model"]=nullptr;j["state_seconds"]=nullptr;
        if(auto it=snap.representatives.find(p);it!=snap.representatives.end()) {
            const auto& s=it->second;
            j["state"]=s.state;j["state_seconds"]=std::max<std::time_t>(0,now-s.since);
            if(!s.tool.empty()) j["tool"]=s.tool;
            if(!s.model.empty()) j["model"]=s.model;
        }
        size_t n=0;
        for(auto& s:snap.sessions) if(s.provider==p) {
            Json row={{"state",s.state},{"tool",s.tool.empty()?Json(nullptr):Json(s.tool)},{"model",s.model.empty()?Json(nullptr):Json(s.model)},
                {"state_seconds",std::max<std::time_t>(0,now-s.since)}};
            if(n==0) for(auto k:{"state","tool","model","state_seconds"}) j[k]=row[k];
            if(n<3) j["s"+std::to_string(n+1)]=row;
            ++n;
        }
        for(size_t i=std::min<size_t>(n,3);i<3;++i) j["s"+std::to_string(i+1)]=nullptr;
    }
    return out;
}
std::string hook_command(const std::filesystem::path& script,const std::string& provider) {
    if(provider!="claude" && provider!="codex") throw std::runtime_error("Invalid AI provider.");
    auto utf8=script.generic_u8string();std::string path(utf8.begin(),utf8.end());
    if(path.find_first_of("\"\r\n%!$`;&|<>^")!=std::string::npos) throw std::runtime_error("Unsupported AI adapter path.");
    return "powershell.exe -NoLogo -NoProfile -NonInteractive -File \""+path+"\" -Provider "+provider;
}
HookPlan plan_hooks(const std::filesystem::path& file,const std::string& command,bool remove) {
    HookPlan p;p.file=file;p.existed=std::filesystem::exists(file);p.before=p.existed?read(file):"";
    auto parsed=p.existed?plugins::parse_strict_json(p.before,1024*1024):std::optional<Json>(Json::object());
    if(!parsed) throw std::runtime_error("Invalid AI hook configuration.");
    Json j=*parsed;
    if(!j.is_object() || (j.contains("hooks") && !j["hooks"].is_object())) throw std::runtime_error("Invalid AI hook configuration.");
    if(!j.contains("hooks")) { if(remove && !j.contains("statusLine")) {p.after=p.before;return p;} j["hooks"]=Json::object(); }
    bool changed=false;
    for(auto event:{"SessionStart","SessionEnd","UserPromptSubmit","PreToolUse","PostToolUse","PostToolUseFailure","PermissionRequest","Stop","Interrupt"}) {
        auto& hooks=j["hooks"];
        if(hooks.contains(event)) {
            if(!hooks[event].is_array()) throw std::runtime_error("Invalid AI hook event.");
            auto& groups=hooks[event];
            for(auto group=groups.begin();group!=groups.end();) {
                if(!group->is_object() || !group->contains("hooks") || !(*group)["hooks"].is_array()) {++group;continue;}
                auto& list=(*group)["hooks"];
                const auto before=list.size();
                list.erase(std::remove_if(list.begin(),list.end(),[&](const Json& h){return h.is_object() && h.value("type",Json())=="command" && h.value("command",Json())==command;}),list.end());
                bool removed=list.size()!=before;changed=changed || removed;
                if(removed && list.empty() && group->size()==1) group=groups.erase(group);else ++group;
            }
            // Preserve an event's empty array: it may have existed before installation.
        }
        // Interrupt is Codex-specific; PostToolUseFailure is Claude-specific.
        bool codex=command.ends_with("-Provider codex");
        if(!remove && !(codex && std::string(event)=="PostToolUseFailure") && !(!codex && std::string(event)=="Interrupt")) {
            changed=true;
            if(!hooks.contains(event)) hooks[event]=Json::array();
            hooks[event].push_back({{"hooks",Json::array({{{"type","command"},{"command",command},{"timeout",3}}})}});
        }
    }
    if(command.ends_with("-Provider claude")) {
        const bool own=j.contains("statusLine") && j["statusLine"].is_object() && j["statusLine"].value("command",Json())==command;
        if(remove && own) {j.erase("statusLine");changed=true;}
        else if(!remove && !j.contains("statusLine")) {j["statusLine"]={{"type","command"},{"command",command}};changed=true;}
    }
    if(remove && !changed) {p.after=p.before;return p;}
    if(j["hooks"].empty()) j.erase("hooks");
    p.after=j.dump(2)+"\n"; return p;
}
std::string hook_preview(const HookPlan& p,const std::string& command) {
    auto owned=[&](const std::string& bytes) {
        Json out=Json::object();
        if(bytes.empty()) return out;
        const auto j=Json::parse(bytes);
        if(j.contains("statusLine") && j["statusLine"].is_object() && j["statusLine"].value("command",Json())==command)
            out["statusLine"]={{"type","command"},{"command",command}};
        if(!j.contains("hooks")) return out;
        for(auto event=j["hooks"].begin();event!=j["hooks"].end();++event) {
            // Only known event names and the exact owned command can enter the preview.
            if(event.key()=="Usage" || !parse(Json{{"provider","claude"},{"session_id","preview"},{"hook_event_name",event.key()}}.dump())) continue;
            for(const auto& group:event.value()) {
                if(!group.is_object() || !group.contains("hooks") || !group["hooks"].is_array()) continue;
                for(const auto& h:group["hooks"])
                    if(h.is_object() && h.value("type",Json())=="command" && h.value("command",Json())==command)
                        out[event.key()].push_back(Json{{"type","command"},{"command",command}});
            }
        }
        return out;
    };
    return Json{{"before",owned(p.before)},{"after",owned(p.after)}}.dump(2)+"\n";
}
void apply_hooks(const HookPlan& p) {
    if(std::filesystem::exists(p.file)!=p.existed || (p.existed && read(p.file)!=p.before)) throw std::runtime_error("AI hook file changed; preview again.");
    if(p.after==p.before || (!p.existed && p.after.empty())) return;
    std::filesystem::create_directories(p.file.parent_path());
    if(p.existed) {
        auto bak=p.file;bak+=L".bak";
        if(!std::filesystem::exists(bak)) std::filesystem::copy_file(p.file,bak);
    }
    auto temp=p.file;temp+=L".screenbuddy-ai.tmp";
    // Inspect the link itself, including dangling symlinks; never write through it.
    const auto status=std::filesystem::symlink_status(temp);
    if(std::filesystem::exists(status)) {
        if(!std::filesystem::is_regular_file(status)) throw std::runtime_error("Unsafe AI temporary file.");
        std::filesystem::remove(temp); // stale regular file from an interrupted apply
    }
    // CREATE_NEW also rejects a file/link created after the status check.
    HANDLE handle=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create AI temporary file.");
    try {
        DWORD written=0;
        const bool ok=WriteFile(handle,p.after.data(),static_cast<DWORD>(p.after.size()),&written,nullptr) && written==p.after.size() && FlushFileBuffers(handle);
        const bool closed=CloseHandle(handle);handle=INVALID_HANDLE_VALUE;
        if(!ok || !closed) throw std::runtime_error("Cannot write AI hook file.");
        if(!MoveFileExW(temp.c_str(),p.file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace AI hook file.");
    } catch(...) {
        if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle);
        std::error_code ignored;std::filesystem::remove(temp,ignored);
        throw;
    }
}
struct Service::Impl {
    std::filesystem::path directory;
    Model model;
    std::unique_ptr<Receiver> receiver;
    std::unique_ptr<UsageWorker> usage;
    std::string key;
    int port=PORT;
};
Service::Service(std::filesystem::path directory):d_(std::make_unique<Impl>()) {d_->directory=std::move(directory);}
Service::~Service() = default;
Model& Service::model(){return d_->model;}
void Service::enable_usage(bool enabled) {if(!d_->usage) d_->usage=std::make_unique<UsageWorker>(d_->model,d_->directory/L"cli");d_->usage->enable(enabled);}
bool Service::running() const{return d_->receiver && d_->receiver->running();}
std::filesystem::path Service::script_path() const{return d_->directory/L"screenbuddy-ai-hook.ps1";}
void Service::prepare_adapter() {
    std::filesystem::create_directories(d_->directory);
    auto keyfile=d_->directory/L"key.dpapi";
    if(d_->key.empty()) {
        if(std::filesystem::exists(keyfile)) {
            auto encrypted=read(keyfile);
            if(!secure::is_protected(encrypted)) throw std::runtime_error("Invalid AI secret storage.");
            d_->key=secure::unprotect(encrypted,"ScreenBuddy.ai.v1");
        } else {
            unsigned char random[32];
            if(BCryptGenRandom(nullptr,random,sizeof random,BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0) throw std::runtime_error("Cannot create AI secret.");
            const char* hex="0123456789abcdef";
            std::string generated;
            for(auto c:random) {generated+=hex[c>>4];generated+=hex[c&15];}
            // Commit in-memory key only after durable protected storage succeeded.
            write(keyfile,secure::protect(generated,"ScreenBuddy.ai.v1"));
            d_->key=std::move(generated);
        }
    }
    if(d_->key.size()!=64 || !std::all_of(d_->key.begin(),d_->key.end(),[](char c){return (c>='0' && c<='9') || (c>='a' && c<='f');})) {
        d_->key.clear();throw std::runtime_error("Cannot unlock AI secret.");
    }
    // Reads only metadata from stdin. Never reads transcripts, prompts or tool arguments.
    const std::string script=R"PS(param([ValidateSet('claude','codex')][string]$Provider)
$ErrorActionPreference = 'Stop'
try {
  $raw = [Console]::In.ReadToEnd()
  if ($raw.Length -gt 1048576) { exit 0 }
  $event = $raw | ConvertFrom-Json
  $body = @{provider=$Provider; session_id=$event.session_id; hook_event_name=$event.hook_event_name}
  if ($Provider -eq 'claude') {
    if (-not $event.hook_event_name) { $body.hook_event_name='Usage'; $body.source='claude-statusline' }
    if ($event.cost.total_cost_usd -is [ValueType]) { $body.cost_session=$event.cost.total_cost_usd }
    if ($event.context_window.used_percentage -is [ValueType]) { $body.context=$event.context_window.used_percentage }
    foreach ($pair in @(@('five_hour','five_hour'),@('seven_day','week'))) {
      $window=$event.rate_limits.($pair[0])
      if ($window.used_percentage -is [ValueType]) { $body[$pair[1]]=$window.used_percentage }
      if ($window.resets_at -is [ValueType]) { $body[$pair[1]+'_reset']=$window.resets_at }
    }
    if ($event.model.id -is [string]) { $body.model=$event.model.id }
  }
  foreach ($name in @('tool_name','model')) {
    if ($event.$name -is [string] -and $event.$name -match '^[a-zA-Z0-9_.:/\[\]-]{1,80}$') { $body[$name]=$event.$name }
  }
  Add-Type -AssemblyName System.Security
  $cipher = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'key.dpapi'))
  if (-not $cipher.StartsWith('dpapi:')) { exit 0 }
  $bytes = [Convert]::FromBase64String($cipher.Substring(6))
  $plain = [Security.Cryptography.ProtectedData]::Unprotect($bytes,[Text.Encoding]::UTF8.GetBytes('ScreenBuddy.ai.v1'),[Security.Cryptography.DataProtectionScope]::CurrentUser)
  $key = [Text.Encoding]::UTF8.GetString($plain)
  $client = New-Object Net.WebClient
  $client.Proxy = $null
  $client.Headers['X-ScreenBuddy-AI-Key'] = $key
  $client.Headers['Content-Type'] = 'application/json'
  $client.Encoding = [Text.Encoding]::UTF8
  $request = $client.UploadStringTaskAsync('http://127.0.0.1:)PS"+std::to_string(d_->port)+R"PS(/events',($body | ConvertTo-Json -Compress))
  if (-not $request.Wait(1200)) { $client.CancelAsync() }
  $client.Dispose()
} catch { }
exit 0
)PS";
    write(script_path(),script);
}
bool Service::start(int port) {
    if(port==RESERVED_PORT || port<1024 || port>65535 || d_->receiver) return false;
    d_->port=port;
    try {prepare_adapter();} catch(...) {return false;}
    d_->receiver=std::make_unique<Receiver>(d_->model,d_->key);
    return d_->receiver->start(port);
}
struct Receiver::Impl {
    Model& model;
    std::string key;
    int port=0;
    httplib::Server server;
    std::thread thread;
    Impl(Model& m,std::string k):model(m),key(std::move(k)){}
};
Receiver::Receiver(Model& model,std::string key):d_(std::make_unique<Impl>(model,std::move(key))) {}
Receiver::~Receiver() {d_->server.stop();if(d_->thread.joinable()) d_->thread.join();}
int Receiver::port() const {return d_->port;}
bool Receiver::running() const {return d_->server.is_running();}
bool Receiver::start(int port) {
    if(port==RESERVED_PORT || (port!=0 && port<1024) || port>65535 || d_->thread.joinable() || d_->key.size()!=64) return false;
    auto& server=d_->server;
    // Windows SO_REUSEADDR can let independent receivers share a port and mix tests.
    server.set_socket_options([](socket_t sock) {
        httplib::set_socket_opt(sock,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,1);
    });
    server.new_task_queue=[] {return new httplib::ThreadPool(2);};
    server.set_keep_alive_max_count(8);
    server.set_payload_max_length(8192);
    server.set_read_timeout(2,0);server.set_write_timeout(2,0);
    server.Post("/events",[this](const httplib::Request& req,httplib::Response& res) {
        // A custom header, no CORS, no query-string credentials. Reject browser-origin requests.
        const auto supplied=req.get_header_value("X-ScreenBuddy-AI-Key");
        unsigned diff=static_cast<unsigned>(supplied.size()^d_->key.size());
        for(size_t i=0;i<d_->key.size();++i) diff|=static_cast<unsigned char>(d_->key[i])^(i<supplied.size()?static_cast<unsigned char>(supplied[i]):0);
        if(diff || req.has_header("Origin") || req.has_header("Sec-Fetch-Site")) {res.status=403;return;}
        res.status=d_->model.ingest(req.body,std::time(nullptr))?204:400;
    });
    if(port==0) {
        // Select an available dynamic-range port without ever probing reserved ports.
        bool bound=false;
        const auto seed=GetTickCount64()+uint64_t(GetCurrentProcessId())*2654435761ULL;
        for(int attempt=0;attempt<16384;++attempt) {
            port=49152+static_cast<int>((seed+attempt)%16384);
            if(server.bind_to_port("127.0.0.1",port)) {bound=true;break;}
        }
        if(!bound) return false;
    } else if(!server.bind_to_port("127.0.0.1",port)) return false;
    d_->port=port;
    d_->thread=std::thread([this]{d_->server.listen_after_bind();});
    server.wait_until_ready();return server.is_running();
}
} // namespace fl::ai
