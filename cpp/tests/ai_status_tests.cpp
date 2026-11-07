// © 2026 FuchsLabs — all file writes stay in one unique temporary directory.
#include "ai_status.h"
#include "render.h"
#include "http.h"
#include "secure.h"
#include "i18n.h"
#include "plugins.h"
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace {
using namespace fl;
using ai::Json;
int passed=0,failed=0;
#define AI_CHECK(c) do {if(c) ++passed;else {++failed;std::printf("AI FAIL line %d: %s\n",__LINE__,#c);}} while(0)
std::string read_file(const std::filesystem::path& p) {std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void write_file(const std::filesystem::path& p,const std::string& s) {std::ofstream f(p,std::ios::binary);f<<s;}
Json event(const char* provider,const char* id,const char* kind) {return {{"provider",provider},{"session_id",id},{"hook_event_name",kind}};}
struct Temp {
    std::filesystem::path dir,reservation;
    Temp() {
        wchar_t base[MAX_PATH],name[MAX_PATH];
        if(!GetTempPathW(MAX_PATH,base) || !GetTempFileNameW(base,L"sai",0,name)) throw std::runtime_error("Temp directory unavailable.");
        // Keep the reservation file alive until teardown so concurrent tests cannot reuse it.
        reservation=name;dir=std::wstring(name)+L".dir";
        try {std::filesystem::create_directory(dir);} catch(...) {DeleteFileW(name);throw;}
    }
    ~Temp(){std::error_code ec;std::filesystem::remove_all(dir,ec);std::filesystem::remove(reservation,ec);}
};
}
int test_ai_status(const std::filesystem::path& output = {}) {
    passed=failed=0;Temp temp;
    auto start=event("claude","a","SessionStart");
    AI_CHECK(ai::parse(start.dump()).has_value());
    for(const auto& bad:{"{}","[]","null","{","{\"provider\":\"claude\",\"provider\":\"codex\"}"}) AI_CHECK(!ai::parse(bad));
    AI_CHECK(!ai::parse(std::string(8193,' ')));
    for(auto key:{"provider","session_id","hook_event_name"}) {auto j=start;j[key]=42;AI_CHECK(!ai::parse(j.dump()));}
    for(auto pair:{std::pair{"provider","other"},{"session_id","private path name"},{"hook_event_name","unknown"},{"tool_name","secret\nvalue"},{"model","bad model"}}) {
        auto j=start;j[pair.first]=pair.second;AI_CHECK(!ai::parse(j.dump()));
    }
    {auto j=start;j["model"]="claude-opus-5-5[1m]";auto e=ai::parse(j.dump());AI_CHECK(e.has_value() && e->model=="claude-opus-5-5[1m]");}
    {auto j=start;j["model"]="[]";AI_CHECK(!ai::parse(j.dump()));}
    for(const char* bad:{"claude;rm","x y","m{1}"}) {auto j=start;j["model"]=bad;AI_CHECK(!ai::parse(j.dump()));}
    for(auto metric:{"context","five_hour","week"}) for(int n:{-1,101}) {auto j=start;j[metric]=n;AI_CHECK(!ai::parse(j.dump()));}
    for(auto metric:{"cost_today","tokens_today","five_hour_reset","week_reset"}) {auto j=start;j[metric]=-1;AI_CHECK(!ai::parse(j.dump()));}
    {auto j=start;j["tokens_today"]=1.5;AI_CHECK(!ai::parse(j.dump()));}
    ai::Model model;const std::time_t now=1800000000;
    auto ingest=[&](Json j,std::time_t t){AI_CHECK(model.ingest(j.dump(),t));};
    auto tiles=model.tiles(now);AI_CHECK(tiles["claude"]["state"]=="off");AI_CHECK(tiles["codex"]["tokens_today"].is_null());
    ingest(start,now);ingest(event("codex","b","UserPromptSubmit"),now+1);
    auto permission=event("claude","a","PermissionRequest");permission["tool_name"]="Bash";permission["model"]="claude-sonnet";
    ingest(permission,now+2);
    auto snap=model.snapshot(now+5);AI_CHECK(snap.sessions.size()==2);AI_CHECK(snap.sessions[0].state=="waiting");AI_CHECK(snap.sessions[1].state=="working");
    AI_CHECK(model.tiles(now+5)["claude"]["state_seconds"]==3);
    ingest(permission,now+6);AI_CHECK(model.tiles(now+7)["claude"]["state_seconds"]==5);
    ingest(event("claude","c","SessionStart"),now+7);AI_CHECK(model.snapshot(now+8).sessions[2].state=="idle");
    auto metrics=event("claude","a","PreToolUse");metrics["context"]=42;metrics["five_hour"]=23;metrics["week"]=7;
    metrics["five_hour_reset"]=now+500;metrics["week_reset"]=now+72000;metrics["cost_today"]=1.25;
    ingest(metrics,now+8);AI_CHECK(model.tiles(now+9)["claude"]["cost_today"]==1.25);
    AI_CHECK(model.tiles(now+86400)["claude"]["cost_today"].is_null());
    AI_CHECK(model.tiles(now+86400)["claude"]["five_hour"]==23);
    ingest(event("claude","a","Stop"),now+10);AI_CHECK(model.tiles(now+10)["claude"]["state"]=="idle");
    AI_CHECK(model.tiles(now+10)["claude"]["tool"].is_null());
    ingest(event("claude","a","SessionEnd"),now+11);ingest(permission,now+12);
    AI_CHECK(model.snapshot(now+12).sessions.size()==2);
    ingest(start,now+13);AI_CHECK(model.snapshot(now+13).sessions.size()==3);
    ai::Model stable;
    for(auto id:{"z","a","b","c"}) AI_CHECK(stable.ingest(event("codex",id,"Stop").dump(),now));
    AI_CHECK(stable.snapshot(now).sessions[0].id=="a");AI_CHECK(stable.tiles(now)["codex"]["s3"]["state"]=="idle");
    ai::Model bounded;
    for(int i=0;i<128;++i) {auto j=event("codex","x","SessionStart");j["session_id"]=std::to_string(i);AI_CHECK(bounded.ingest(j.dump(),now));}
    AI_CHECK(!bounded.ingest(event("codex","overflow","SessionStart").dump(),now));
    AI_CHECK(bounded.ingest(event("codex","0","SessionEnd").dump(),now+1));
    AI_CHECK(bounded.ingest(event("codex","overflow","SessionStart").dump(),now+2));
    // Usage snapshots never change hook session state or invent daily totals.
    ai::Model usage;
    AI_CHECK(usage.ingest(event("claude","measured","PermissionRequest").dump(),now));
    auto sample=event("claude","measured","Usage");sample["source"]="claude-statusline";
    sample["cost_session"]=0.125;sample["five_hour"]=0;sample["week"]=51;sample["context"]=30;
    AI_CHECK(usage.ingest(sample.dump(),now+1));
    auto measured=usage.tiles(now+2)["claude"];
    AI_CHECK(measured["state"]=="waiting");AI_CHECK(measured["cost_session"]==0.125);
    AI_CHECK(measured["cost_today"].is_null());AI_CHECK(measured["tokens_today"].is_null());
    AI_CHECK(measured["updated_at"]==now+1);AI_CHECK(measured["source"]=="claude-statusline");
    sample.erase("five_hour");sample.erase("week");AI_CHECK(usage.ingest(sample.dump(),now+3));
    AI_CHECK(usage.tiles(now+3)["claude"]["five_hour"].is_null());
    AI_CHECK(usage.tiles(now+3)["claude"]["reason"]=="limits_not_supplied");
    usage.usage("claude",Json::object(),"claude-cli","not_in_path",now+4);
    AI_CHECK(usage.tiles(now+4)["claude"]["updated_at"]==now+3);
    AI_CHECK(usage.tiles(now+4)["claude"]["source"]=="claude-statusline");
    auto badUsage=sample;badUsage["tokens_session"]=1.5;AI_CHECK(!ai::parse(badUsage.dump()));
    auto reply=Json{{"result",{{"rateLimits",{{"primary",{{"usedPercent",37},{"windowDurationMins",300},{"resetsAt",now+300}}},
        {"secondary",{{"usedPercent",83},{"windowDurationMins",10080},{"resetsAt",now+600}}}}}}}};
    auto decoded=ai::codex_usage(reply);AI_CHECK(decoded.ok);AI_CHECK(decoded.metrics["five_hour"]==37);
    AI_CHECK(decoded.metrics["week"]==83);AI_CHECK(decoded.metrics["five_hour_reset"]==now+300);
    AI_CHECK(!decoded.metrics.contains("tokens_today"));
    auto shortWindow=reply;shortWindow["result"]["rateLimits"]["primary"]["windowDurationMins"]=15;
    shortWindow["result"]["rateLimits"]["secondary"]=nullptr;
    AI_CHECK(!ai::codex_usage(shortWindow).ok);AI_CHECK(ai::codex_usage(shortWindow).metrics.empty());
    for(const auto& invalid:{Json(-1),Json(101),Json("37"),Json(true)}) {
        auto bad=reply;bad["result"]["rateLimits"]["primary"]["usedPercent"]=invalid;
        AI_CHECK(!ai::codex_usage(bad).ok);
    }
    AI_CHECK(!ai::codex_usage(Json{{"error",{{"message","synthetic error"}}}}).ok);
    AI_CHECK(!ai::codex_usage(Json::object()).ok);
    usage.usage("codex",decoded.metrics,decoded.source,"",now+5);
    usage.usage("codex",Json::object(),"codex-app-server","query_failed",now+6);
    AI_CHECK(usage.tiles(now+6)["codex"]["updated_at"]==now+5);
    AI_CHECK(usage.tiles(now+6)["codex"]["checked_at"]==now+6);
    AI_CHECK(usage.tiles(now+6)["codex"]["five_hour"]==37);
    // Status line conflicts remain private and unchanged; owned entries remove selectively.
    const auto statusOnly=temp.dir/L"status-only.json";
    const auto settings=temp.dir/L"settings.json";
    const auto command=ai::hook_command(temp.dir/L"screenbuddy-ai-hook.ps1","claude");
    bool unsafe_rejected=false;
    try {ai::hook_command(temp.dir/L"bad$path.ps1","claude");} catch(...) {unsafe_rejected=true;}
    AI_CHECK(unsafe_rejected);
    Json original={{"env",{{"PRIVATE_TEST_TOKEN","synthetic-secret-do-not-display"}}},{"unrelated",{{"enabled",true}}},{"hooks",{{"SessionEnd",Json::array()},{"Stop",Json::array({{{"hooks",Json::array({{{"type","command"},{"command","echo foreign"}}})}},{{"hooks",Json::array()}}})}}}};
    write_file(settings,original.dump());
    write_file(statusOnly,Json{{"statusLine",{{"type","command"},{"command",command}}}}.dump());
    AI_CHECK(!Json::parse(ai::plan_hooks(statusOnly,command,true).after).contains("statusLine"));
    write_file(statusOnly,Json{{"statusLine",{{"type","command"},{"command","foreign-private-statusline"}}}}.dump());
    auto foreign=ai::plan_hooks(statusOnly,command,false);
    AI_CHECK(Json::parse(foreign.after)["statusLine"]["command"]=="foreign-private-statusline");
    AI_CHECK(ai::hook_preview(foreign,command).find("foreign-private-statusline")==std::string::npos);
    auto plan=ai::plan_hooks(settings,command,false);
    AI_CHECK(Json::parse(plan.after)["statusLine"]["command"]==command);AI_CHECK(read_file(settings)==original.dump());
    AI_CHECK(!std::filesystem::exists(settings.wstring()+L".bak"));
    auto preview=ai::hook_preview(plan,command);
    AI_CHECK(preview.find("synthetic-secret-do-not-display")==std::string::npos);
    AI_CHECK(preview.find("PRIVATE_TEST_TOKEN")==std::string::npos);
    AI_CHECK(preview.find("echo foreign")==std::string::npos);
    AI_CHECK(Json::parse(preview)["after"]["SessionStart"][0]["command"]==command);
    const auto scratch=std::filesystem::path(settings.wstring()+L".screenbuddy-ai.tmp");
    write_file(scratch,"orphaned previous write");
    ai::apply_hooks(plan);AI_CHECK(!std::filesystem::exists(scratch));AI_CHECK(read_file(settings.wstring()+L".bak")==original.dump());
    AI_CHECK(Json::parse(read_file(settings))["hooks"]["Stop"].size()==3);
    auto again=ai::plan_hooks(settings,command,false);ai::apply_hooks(again);
    AI_CHECK(Json::parse(read_file(settings))["hooks"]["Stop"].size()==3);
    AI_CHECK(read_file(settings.wstring()+L".bak")==original.dump());
    auto removal=ai::plan_hooks(settings,command,true);ai::apply_hooks(removal);
    auto removed=Json::parse(read_file(settings));
    AI_CHECK(removed["unrelated"]==original["unrelated"]);
    AI_CHECK(removed["hooks"]["Stop"]==original["hooks"]["Stop"]);
    AI_CHECK(removed["hooks"]["SessionEnd"]==original["hooks"]["SessionEnd"]);
    for(const auto& groups:removed["hooks"]) for(const auto& group:groups) for(const auto& hook:group["hooks"])
        AI_CHECK(hook.value("command",std::string())!=command);
    auto noop=ai::plan_hooks(settings,command,true);AI_CHECK(noop.before==noop.after);
    const auto codex_command=ai::hook_command(temp.dir/L"screenbuddy-ai-hook.ps1","codex");
    auto codex_plan=ai::plan_hooks(temp.dir/L"hooks.json",codex_command,false);
    auto codex_json=Json::parse(codex_plan.after);AI_CHECK(codex_json["hooks"].contains("Interrupt"));
    AI_CHECK(!codex_json["hooks"].contains("PostToolUseFailure"));
    auto conflict=ai::plan_hooks(settings,command,false);write_file(settings,"{}");
    bool rejected=false;try {ai::apply_hooks(conflict);} catch(...) {rejected=true;}AI_CHECK(rejected);AI_CHECK(read_file(settings)=="{}");
    write_file(settings,"bad json");rejected=false;try {ai::plan_hooks(settings,command,false);} catch(...) {rejected=true;}AI_CHECK(rejected);
    write_file(settings,"{\"hooks\":{},\"hooks\":{}}");rejected=false;try {ai::plan_hooks(settings,command,false);} catch(...) {rejected=true;}AI_CHECK(rejected);
    // Replacement failure must remove our temp and allow a subsequent retry.
    const auto retry_file=temp.dir/L"retry.json";write_file(retry_file,"{}");
    auto retry=ai::plan_hooks(retry_file,command,false);
    // A locked destination preserves readable snapshot bytes but rejects replacement.
    HANDLE lock=CreateFileW(retry.file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    AI_CHECK(lock!=INVALID_HANDLE_VALUE);
    rejected=false;try {ai::apply_hooks(retry);} catch(...) {rejected=true;}
    if(lock!=INVALID_HANDLE_VALUE) CloseHandle(lock);
    AI_CHECK(rejected);AI_CHECK(!std::filesystem::exists(retry.file.wstring()+L".screenbuddy-ai.tmp"));
    ai::apply_hooks(retry);AI_CHECK(read_file(retry.file)==retry.after);
    // Dangling links must be inspected as links, never followed or overwritten.
    write_file(settings,"{}");
    const auto link_target=temp.dir/L"absent-target";
    if(CreateSymbolicLinkW(scratch.c_str(),link_target.c_str(),SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        auto linked=ai::plan_hooks(settings,command,false);
        rejected=false;try {ai::apply_hooks(linked);} catch(...) {rejected=true;}
        AI_CHECK(rejected);AI_CHECK(!std::filesystem::exists(link_target));
        AI_CHECK(std::filesystem::is_symlink(std::filesystem::symlink_status(scratch)));
        std::filesystem::remove(scratch);
    } else std::printf("AI SKIP: symlink creation unavailable.\n");
    auto missing=ai::plan_hooks(temp.dir/L"missing.json",command,true);ai::apply_hooks(missing);AI_CHECK(!std::filesystem::exists(missing.file));
    {
        std::filesystem::create_directory(temp.dir/L"invalid-secret");
        write_file(temp.dir/L"invalid-secret"/L"key.dpapi","plaintext");
        ai::Service invalid(temp.dir/L"invalid-secret");
        AI_CHECK(!invalid.start(ai::PORT));AI_CHECK(!invalid.running());
        ai::Service service(temp.dir/L"receiver");
        AI_CHECK(!service.start(ai::RESERVED_PORT)); // rejected before bind; never contacts this port
        bool dpapi=false;
        try {auto probe=secure::protect("synthetic-ai-probe","ScreenBuddy.ai.v1");dpapi=secure::unprotect(probe,"ScreenBuddy.ai.v1")=="synthetic-ai-probe";} catch(...) {}
        if(dpapi) {
            service.prepare_adapter();
            auto encrypted=read_file(temp.dir/L"receiver"/L"key.dpapi");AI_CHECK(secure::is_protected(encrypted));
            auto key=secure::unprotect(encrypted,"ScreenBuddy.ai.v1");AI_CHECK(key.size()==64);
            auto script=read_file(service.script_path());AI_CHECK(script.find(key)==std::string::npos);
        } else {
            std::printf("AI SKIP: DPAPI roundtrip unavailable in this execution environment. Production service must fail closed.\n");
            AI_CHECK(!service.start(ai::PORT));AI_CHECK(!service.start(ai::PORT));AI_CHECK(!service.running());
            AI_CHECK(!std::filesystem::exists(temp.dir/L"receiver"/L"key.dpapi"));
        }
        ai::Model network_model;
        std::string key(64,'a'); // synthetic, ephemeral; never a stored production key
        ai::Receiver receiver(network_model,key);
        bool started=receiver.start(0);AI_CHECK(started);
        if(started) {
            http::Request req;req.url="http://127.0.0.1:"+std::to_string(receiver.port())+"/events";req.method="POST";req.timeout_ms=1500;req.body=start.dump();
            AI_CHECK(http::fetch(req).status==403);
            req.headers={{"X-ScreenBuddy-AI-Key","wrong"}};AI_CHECK(http::fetch(req).status==403);
            req.headers={{"X-ScreenBuddy-AI-Key",key},{"Origin","https://example.com"}};AI_CHECK(http::fetch(req).status==403);
            req.headers={{"X-ScreenBuddy-AI-Key",key}};AI_CHECK(http::fetch(req).status==204);
            AI_CHECK(network_model.snapshot(now).sessions.size()==1);
            req.body="bad";AI_CHECK(http::fetch(req).status==400);
            req.body=std::string(9000,'x');AI_CHECK(http::fetch(req).status==413);
            req.method="OPTIONS";AI_CHECK(http::fetch(req).status!=204);
        }
    }
    Json manifest={{"schema_version",2},{"id","ai-test"},{"name","AI test"},{"version","1.0.0"},
        {"widgets",Json::array({{{"id","state"},{"type","text"},{"text","{{ai.codex.state}}"}}})}};
    auto parsed=plugins::parse_manifest(manifest.dump());AI_CHECK(parsed);
    if(parsed) {
        plugins::RenderContext context;context.sources.emplace("ai",model.tiles(now+13));
        auto page=plugins::render_page(*parsed.manifest,context);AI_CHECK(page.widgets[0].text=="working");
    }
    manifest["links"]={"example.com"};
    manifest["widgets"][0]["tap"]={{"action","open_link"},{"url","https://example.com/{{ai.codex.state}}"}};
    AI_CHECK(!plugins::parse_manifest(manifest.dump()));
    // The renderer uses the same snapshots as the data source, no artificial live counters.
    render::apply_theme(ojson{{"theme",theme::DEFAULT_ID}});
    auto render_snapshot=model.snapshot(now+13);
    render::AiRenderer renderer(480,320);
    auto initial=renderer.render(render_snapshot,10,now+13), next=renderer.render(render_snapshot,10.2,now+13);
    AI_CHECK(initial.w==480 && initial.h==320);AI_CHECK(initial.px!=next.px);
    auto fading_snapshot=render_snapshot;fading_snapshot.sessions.clear();
    auto fading=renderer.render(fading_snapshot,10.25,now+13);
    AI_CHECK(fading.px!=next.px);
    renderer.render(fading_snapshot,11.25,now+13);
    auto faded=renderer.render(fading_snapshot,12.25,now+13);
    render::AiRenderer no_rows(480,320);
    AI_CHECK(faded.px==no_rows.render(fading_snapshot,12.25,now+13).px);
    const auto& aiCatalog=*builtin::find("ai");auto arranged=builtin::standard(aiCatalog);
    for(const auto& spec:aiCatalog.blocks) {auto tile=renderer.render_tile(spec.id,render_snapshot,now+13);AI_CHECK(!tile.empty());}
    AI_CHECK(!renderer.render_arranged(render_snapshot,arranged,10,now+13).image.empty());
    if(!output.empty()) {
        std::filesystem::create_directories(output);
        write_file(output/"fading.png",encode_png(fading));
        write_file(output/"arranged.png",encode_png(renderer.render_arranged(render_snapshot,arranged,10,now+13).image));
        ingest(permission,now+14);
        auto codex_tool=event("codex","b","PreToolUse");codex_tool["tool_name"]="apply_patch";codex_tool["model"]="example-codex";codex_tool["tokens_today"]=1234;
        ingest(codex_tool,now+15);render_snapshot=model.snapshot(now+15);
        for(auto lang:{"de","en"}) {
            render::AiRenderer localized(480,320),missing_renderer(480,320);
            i18n::set_lang(lang);localized.render(render_snapshot,10,now+15);
            auto frame=localized.render(render_snapshot,10.6,now+15);write_file(output/(std::string(lang)+".png"),encode_png(frame));
            ai::Model empty;auto missing_frame=missing_renderer.render(empty.snapshot(now),10,now);write_file(output/(std::string(lang)+"-missing.png"),encode_png(missing_frame));
        }
    }
    i18n::set_lang("de");
    std::printf("AI status: %d passed, %d failed\n",passed,failed);return failed;
}

// A copied test binary acts as a synthetic CLI. It never accepts model turns.
int ai_cli_fixture(int argc,char** argv) {
    if(argc>=2 && std::string(argv[argc-1])=="--help") {std::cout<<"synthetic stdio CLI\n";return 0;}
    if(argc<2 || std::string(argv[1])!="app-server") return 91;
    std::string line;int count=0;
    while(std::getline(std::cin,line)) {
        auto j=Json::parse(line);const auto method=j.value("method",std::string());
        if(method=="initialized") continue;
        Json result;
        if(method=="initialize") result=Json::object();
        else if(method=="account/read") result={{"account",{{"type","chatgpt"}}}};
        else if(method=="account/rateLimits/read") result={{"rateLimits",{{"primary",{{"usedPercent",37},{"windowDurationMins",300},{"resetsAt",1800000300}}},{"secondary",nullptr}}}};
        else return 92;
        if(method=="account/rateLimits/read" && std::filesystem::exists("fail-limits")) {
            write_file("fixture-count",std::to_string(++count));std::cout<<Json{{"id",j["id"]},{"error",{{"code",-1},{"message","synthetic"}}}}.dump()<<std::endl;continue;
        }
        write_file("fixture-count",std::to_string(++count));
        if(std::filesystem::exists("hang-request")) Sleep(60000);
        std::cout<<Json{{"id",j["id"]},{"result",result}}.dump()<<std::endl;
    }
    return 0;
}
int test_ai_cli(bool real) {
    passed=failed=0;Temp temp;
    wchar_t oldPath[32768],self[32768];GetEnvironmentVariableW(L"PATH",oldPath,32768);GetModuleFileNameW(nullptr,self,32768);
    if(!real) {
        std::filesystem::copy_file(self,temp.dir/L"codex.exe");std::filesystem::copy_file(self,temp.dir/L"claude.exe");
        SetEnvironmentVariableW(L"PATH",(temp.dir.wstring()+L";"+oldPath).c_str());
    }
    ai::Model model;
    {ai::UsageWorker worker(model,temp.dir/L"isolated");
        Sleep(50);AI_CHECK(!std::filesystem::exists(temp.dir/L"isolated"));
        worker.enable(true);
        for(int i=0;i<400;++i) {if(!model.tiles(std::time(nullptr))["codex"]["checked_at"].is_null()) break;Sleep(50);}
        const auto m=model.tiles(std::time(nullptr))["codex"];
        AI_CHECK(!m["checked_at"].is_null());
        if(real) std::printf("Real Codex probe: source=%s reason=%s limits=%s\n",m["source"].get<std::string>().c_str(),m["reason"].get<std::string>().c_str(),m["five_hour"].is_null()?"n/a":"present");
        else {AI_CHECK(m["five_hour"]==37);AI_CHECK(m["tokens_today"].is_null());AI_CHECK(m["tokens_session"].is_null());
            AI_CHECK(read_file(temp.dir/L"isolated/codex/fixture-count")=="3");}
        worker.enable(false);worker.enable(true);Sleep(150);
        if(!real) AI_CHECK(read_file(temp.dir/L"isolated/codex/fixture-count")=="3"); // toggling cannot bypass cooldown
        worker.enable(false);
    }
    if(!real) {
        const auto failedDir=temp.dir/L"failure";std::filesystem::create_directories(failedDir/L"codex");write_file(failedDir/L"codex/fail-limits","");
        ai::Model failedModel;
        {ai::UsageWorker worker(failedModel,failedDir);worker.enable(true);
            for(int i=0;i<300;++i) {if(!failedModel.tiles(std::time(nullptr))["codex"]["checked_at"].is_null()) break;Sleep(20);}
            AI_CHECK(failedModel.tiles(std::time(nullptr))["codex"]["reason"]=="query_failed");
            worker.enable(false);worker.enable(true);Sleep(100);
            AI_CHECK(read_file(failedDir/L"codex/fixture-count")=="3");worker.enable(false);
        }
        const auto hungDir=temp.dir/L"hung";std::filesystem::create_directories(hungDir/L"codex");write_file(hungDir/L"codex/hang-request","");
        ai::Model hungModel;auto before=GetTickCount64();
        {ai::UsageWorker worker(hungModel,hungDir);worker.enable(true);
            for(int i=0;i<300 && !std::filesystem::exists(hungDir/L"codex/fixture-count");++i) Sleep(20);
            AI_CHECK(std::filesystem::exists(hungDir/L"codex/fixture-count"));before=GetTickCount64();worker.enable(false);
        }
        AI_CHECK(GetTickCount64()-before<2000);
    }
    SetEnvironmentVariableW(L"PATH",oldPath);
    std::printf("AI CLI %s: %d passed, %d failed\n",real?"real":"synthetic",passed,failed);return failed?1:0;
}
