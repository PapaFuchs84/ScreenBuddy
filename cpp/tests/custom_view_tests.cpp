// © 2026 FuchsLabs
#include <winsock2.h>
#include "custom_view.h"
#include "custom_view_editor.h"
#include "page_samples.h"
#include "app.h"
#include "webui.h"
#include "netguard.h"
#include "http.h"
#include "render.h"
#include <fstream>
#include <iostream>
#include <windows.h>
using namespace fl;
namespace fs=std::filesystem;
static int checks=0,failures=0;
#define CHECK(x) do {++checks;if(!(x)){++failures;std::cerr<<__LINE__<<": " #x "\n";}}while(0)
int main() try {
    auto root=fs::temp_directory_path()/("fl-custom-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    custom::Store store(root/"views");
    custom::View v;v.id="overview";v.name="Overview";
    custom::Tile tile;tile.source="system";tile.block="system.cpu";tile.zone={"cpu",0,0,1,1};v.standard.tiles={tile};
    auto j=custom::to_json(v);
    CHECK(custom::parse(j.dump()));CHECK(store.save(v).empty());CHECK(store.get(v.id));CHECK(!store.get("missing"));
    CHECK(!store.get("../escape"));CHECK(!custom::valid_id("CON"));
    for(auto id:{"", "con","nul","prn","com1","lpt9","../x","x.y","A","x/y","x\\y","x:","ä"}) CHECK(!custom::valid_id(id));
    CHECK(custom::valid_id(std::string(48,'a')));CHECK(!custom::valid_id(std::string(49,'a')));
    CHECK(!custom::parse(std::string(custom::file_limit+1,' ')));
    CHECK(!custom::parse("{\"id\":1,\"id\":2}"));CHECK(!custom::parse("[]"));CHECK(!custom::parse("{"));
    auto bad=[&](auto edit){auto next=j;edit(next);CHECK(!custom::parse(next.dump()));};
    bad([](auto& x){x["schema_version"]=2;});bad([](auto& x){x["schema_version"]=1.0;});
    bad([](auto& x){x["extra"]=true;});bad([](auto& x){x["name"]="";});bad([](auto& x){x["name"]=std::string(129,'x');});
    bad([](auto& x){x["id"]="../bad";});
    for(int n:{0,7,-1,1000000}) {bad([&](auto& x){x["layout"]["columns"]=n;});bad([&](auto& x){x["layout"]["rows"]=n;});}
    bad([](auto& x){x["layout"]["tiles"][0]["zone"]["column"]=1;});
    bad([](auto& x){x["layout"]["tiles"][0]["row_span"]=0;});
    bad([](auto& x){x["layout"]["tiles"][0]["source"]="unknown";});
    bad([](auto& x){x["layout"]["tiles"][0]["source"]="plugin:../bad";});
    bad([](auto& x){x["layout"]["tiles"].push_back(x["layout"]["tiles"][0]);});
    for(auto condition:{plugins::Json{{"path","data.cpu_pct"},{"op","??"},{"value",1}},plugins::Json{{"path","a..b"}},plugins::Json{{"path","a"},{"value",true}},plugins::Json{{"path","a"},{"op","=="},{"value",nullptr}}})
        bad([&](auto& x){x["layout"]["tiles"][0]["show_if"]=condition;});
    auto many=j;many["layout"]["columns"]={1,1,1,1,1,1};many["layout"]["rows"]={1,1,1,1,1,1};many["layout"]["tiles"]=plugins::Json::array();
    for(int i=0;i<25;++i) {auto b=j["layout"]["tiles"][0];b["zone"]={{"id","t"+std::to_string(i)},{"column",i%6},{"row",i/6}};many["layout"]["tiles"].push_back(b);CHECK(bool(custom::parse(many.dump()))==(i<24));}
    for(int i=0;i<16;++i) v.devices["display-"+std::to_string(i)]=v.standard;
    v.devices["display-0"].tiles[0].block="system.ram";
    CHECK(store.save(v).empty());auto loaded=store.get(v.id);CHECK(loaded && loaded->devices.size()==16);
    CHECK(loaded->effective("unknown").tiles[0].block=="system.cpu");CHECK(loaded->effective("display-0").tiles[0].block=="system.ram");
    CHECK(fs::exists(root/"views/overview.view.json.bak"));
    v.devices["overflow"]=v.standard;CHECK(!store.save(v).empty());CHECK(store.get(v.id)->devices.size()==16);v.devices.erase("overflow");
    std::ifstream backup(root/"views/overview.view.json.bak");std::string bytes((std::istreambuf_iterator<char>(backup)),{});CHECK(custom::parse(bytes)->devices.empty());backup.close();
    std::ofstream(root/"views/broken.view.json")<<"{";CHECK(!store.get("broken"));
    std::ofstream(root/"views/wrong.view.json")<<j.dump();CHECK(!store.get("wrong"));
    std::ofstream(root/"views/huge.view.json")<<std::string(custom::file_limit+1,' ');CHECK(!store.get("huge"));
    tile.show_if=plugins::Condition{"data.cpu_pct",">=",0};
    CHECK(custom::visible(tile,{{"cpu_pct",0}}));CHECK(!custom::visible(tile,{}));CHECK(!custom::visible(tile,{{"cpu_pct","0"}}));
    CHECK(config::is_page_key("custom:overview"));CHECK(!config::is_page_key("custom:../bad"));
    auto cfg=config::defaults();for(auto s:{"system","discord","friends","bf6","rss","streaming","homeassistant","spotify","companion"}) cfg[s]["enabled"]=false;
    cfg["ui"]["port"]=18865;cfg["ui"]["update_check"]=false;
    cfg=config::merge(cfg,{{"display",{{"pages",{"custom:missing","custom:overview","system"}},{"visible_pages",{"custom:missing","custom:overview","system"}}}},{"companion",{{"pages",{"custom:overview","custom:missing"}}}}});
    CHECK(cfg["display"]["pages"][0]=="custom:missing");
    App app(cfg,root/"config.toml",nullptr,root/"preview");
    CHECK(app.active_pages()==std::vector<std::string>{"custom:overview"});CHECK(app.active_pages(true)==std::vector<std::string>{"custom:overview"});
    const std::vector<std::string> own={"custom:missing"};CHECK(app.active_pages(true,&own).empty());
    // P9-8: explicit/implicit pages, Companion overrides and tap grants share the reservation.
    for(bool explicit_page:{false,true})for(bool enabled:{false,true}) {
        auto game_cfg=cfg;game_cfg["game_mode"]["enabled"]=enabled;game_cfg["game_mode"]["view"]="custom:overview";
        if(!explicit_page){game_cfg["display"]["pages"]=ojson::array();game_cfg["display"]["visible_pages"]=ojson::array();game_cfg["companion"]["pages"]=ojson::array();}
        App reserved(game_cfg,root/"config.toml",nullptr,root/"preview");
        const std::vector<std::string> device_pages={"custom:overview"};
        for(auto allowed:{reserved.active_pages(),reserved.active_pages(true),reserved.active_pages(true,&device_pages)}) {
            CHECK((std::find(allowed.begin(),allowed.end(),"custom:overview")==allowed.end())==enabled);
            custom::Tile target;target.source="system";target.tap=custom::Tap{"page","custom:overview",""};
            custom::DrawOut drawn;bool switched=false;
            auto tap=custom::tap_override("other",{allowed,{},[&](const std::string&){switched=true;}});
            CHECK(tap(target,{0,0,100,100},drawn)==!enabled);
            CHECK(drawn.targets.empty()==enabled);
            if(!drawn.targets.empty()){drawn.targets.front().action();CHECK(switched);}
        }
    }
    CHECK(config::defaults()["display"]["pages"]==ojson(config::PAGES));CHECK(config::PAGES==std::vector<std::string>({"system","friends","discord","bf6","rss","streaming","homeassistant","spotify","ai"}));
    gfx_init();SysSnapshot snap;snap.cpu_pct=42;snap.ram_pct=23;
    render::SysRenderer renderer(480,320,"System","bit");
    auto draw=[&](const custom::Layout& layout,std::optional<SysSnapshot> data) {
        custom::View view;view.standard=layout;
        return custom::draw(view,"",480,320,[&](const auto& t,const auto& l,int,int)->std::optional<Image>{
            if(!data || !custom::visible(t,{{"cpu_pct",data->cpu_pct.value_or(0)}})) return {};
            auto overlay=custom::single_builtin(t,l);if(!overlay || t.source!="system") return {};
            return renderer.render_arranged(*data,*overlay,1700000000).image;
        });
    };
    auto image=draw(v.standard,snap);
    Image blank(480,320,render::theme().bg);CHECK(image.px!=blank.px);
    CHECK(draw(v.standard,std::nullopt).px==blank.px);
    auto overlay=custom::single_builtin(v.standard.tiles[0],v.standard);CHECK(overlay);
    CHECK(builtin::validate(*builtin::find("system"),*overlay).empty());
    auto original=renderer.render_arranged(snap,*overlay,1700000000).image;
    CHECK(image.at(100,100)==original.at(100,100));CHECK(image.at(0,0)==blank.at(0,0));CHECK(image.at(100,319)==blank.at(100,319));
    auto hidden=v.standard;hidden.tiles[0].show_if=plugins::Condition{"data.cpu_pct",">",99};CHECK(draw(hidden,snap).px==blank.px);
    auto isolated=v.standard;auto unsupported=tile;unsupported.source="homeassistant";isolated.tiles.insert(isolated.tiles.begin(),unsupported);CHECK(draw(isolated,snap).px==image.px);
    // Composition uses the selected device layout; exceptions affect one tile only.
    custom::View two;two.standard.columns={1,1};tile.show_if.reset();tile.zone={"left",0,0,1,1};
    auto right=tile;right.zone={"right",1,0,1,1};two.standard.tiles={tile,right};
    auto failing=custom::draw(two,"",480,320,[&](const auto& t,const auto&,int w,int h)->std::optional<Image>{
        if(t.zone.id=="left") throw std::runtime_error("synthetic failure");return Image(w,h,Color{0x12,0x34,0x56});
    });
    CHECK(failing.at(50,100)==blank.at(50,100));CHECK(failing.at(400,100)!=blank.at(400,100));
    two.devices["monitor"]=two.standard;two.devices["monitor"].tiles.clear();
    auto solid=[](const auto&,const auto&,int w,int h)->std::optional<Image>{return Image(w,h,Color{0x12,0x34,0x56});};
    CHECK(custom::draw(two,"monitor",480,320,solid).px==blank.px);
    CHECK(custom::draw(two,"unknown",480,320,solid).px!=blank.px);
    CHECK(custom::draw(two,"",0,320,solid).px.empty());
    CHECK(custom::draw(two,"",480,320,[](const auto&,const auto&,int,int)->std::optional<Image>{return Image(1,1);}).px==blank.px);
    // Single-tile contract never weakens complete-page parsing.
    for(const auto& catalog:builtin::catalogs()) for(const auto& block:catalog.blocks) {
        auto t=v.standard.tiles.front();t.source=catalog.key;t.block=block.id;
        auto single=custom::single_builtin(t,v.standard);
        CHECK(single);
        if(single) {
            CHECK(builtin::validate(catalog,*single).empty());
            CHECK(!builtin::parse(catalog,builtin::to_json(*single).dump()));
            auto invalid=*single;invalid.model.blocks[0]["id"]="unknown";CHECK(!builtin::validate(catalog,invalid).empty());
        }
    }
    // P8-4: one snapshot drives public conditions, missing data, animation and tile targets.
    {
        // Real receiver and settings-server sockets cannot share one loopback endpoint.
        CHECK(!netguard::is_global_ip("127.0.0.1"));
        ai::Model network;int port=0;
        {
            ai::Receiver receiver(network,std::string(64,'a'));const bool started=receiver.start(0);CHECK(started);if(!started)throw std::runtime_error("Dynamic receiver failed: "+std::to_string(WSAGetLastError()));port=receiver.port();
            auto port_cfg=cfg;port_cfg["ui"]["port"]=port;
            App isolated(port_cfg,root/"port-config.toml",nullptr,root/"port-preview");
            SettingsServer ui(isolated,[]{});CHECK(!ui.start());
        }
        {
            auto port_cfg=cfg;port_cfg["ui"]["port"]=port;
            App isolated(port_cfg,root/"port-config.toml",nullptr,root/"port-preview");
            SettingsServer ui(isolated,[]{});CHECK(ui.start());
            CHECK(http::get("http://127.0.0.1:"+std::to_string(port)+"/api/pages",{},1500).status==200);
            ai::Receiver receiver(network,std::string(64,'b'));CHECK(!receiver.start(port));
            ui.stop();
        }
        const auto now=render::samples::CLOCK;const auto snap=render::samples::ai_status();
        ai::Model empty;const auto missing=empty.snapshot(now);
        CHECK(ai::tile_values(snap,now)["claude"]["context"]==42);
        CHECK(ai::tile_values(snap,now)["codex"]["five_hour"].is_null());
        for(const auto* id:{"ai.claude","ai.codex","ai.limits"}) {
            CHECK(ai::tile_present(snap,id));CHECK(!ai::tile_present(missing,id));
            custom::View av;av.id="ai-test";av.name="AI test";
            custom::Tile at{"ai",id,{"ai",0,0,1,1}};at.tap=custom::Tap{"page","system",""};av.standard.tiles={at};
            CHECK(custom::parse(custom::to_json(av).dump()));
            CHECK(store.save(av).empty());CHECK(store.get(av.id)->standard.tiles[0].tap->page=="system");
            const auto palette=app.custom_palette();CHECK(std::count_if(palette.begin(),palette.end(),[&](const auto& t){return t.source=="ai" && t.block==id;})==1);
            for(auto size:{std::pair{480,320},std::pair{320,480},std::pair{1920,1080}}) {
                render::AiRenderer ar(size.first,size.second);
                auto overlay=custom::single_builtin(at,av.standard);CHECK(overlay);
                CHECK(builtin::validate(*builtin::find("ai"),*overlay).empty());
                auto image=ar.render_arranged(snap,*overlay,0,now).image;
                CHECK(image.px!=Image(size.first,size.second,render::theme().bg).px);
                std::string destination;custom::TapEnv env;env.pages={"system"};env.go=[&](const auto& key){destination=key;};
                auto out=custom::draw_out(av,"",size.first,size.second,[&](const auto&,const auto&,int,int)->std::optional<custom::TileDraw>{return custom::TileDraw{image};},
                    custom::tap_override("ai-test",env));
                CHECK(out.targets.size()==1);if(!out.targets.empty()) {CHECK(bool(out.targets[0].action));out.targets[0].action();CHECK(destination=="system");}
            }
            CHECK(app.custom_preview(av,"",{480,320})->px!=blank.px);
            av.standard.tiles[0].show_if=plugins::Condition{"data.claude.context","==",42};
            CHECK(app.custom_preview(av,"",{480,320})->px!=blank.px);
            av.standard.tiles[0].show_if->value=43;
            CHECK(app.custom_preview(av,"",{480,320})->px==blank.px);
            at.show_if=plugins::Condition{"data.codex.five_hour","",nullptr};CHECK(!custom::visible(at,ai::tile_values(snap,now)));
            at.show_if=plugins::Condition{"data.claude.state","==","working"};CHECK(custom::visible(at,ai::tile_values(snap,now)));
        }
        render::AiRenderer ar(480,320);custom::View av;av.id="ai-live";av.name="AI";
        av.standard.tiles={custom::Tile{"ai","ai.claude",{"ai",0,0,1,1}}};
        const auto overlay=*custom::single_builtin(av.standard.tiles[0],av.standard);
        CHECK(ar.render_arranged(snap,overlay,0,now).image.px!=ar.render_arranged(snap,overlay,.4,now).image.px);
        CHECK(ar.render_arranged(missing,overlay,0,now).image.px==blank.px);
        auto bad=custom::to_json(av);bad["layout"]["tiles"][0]["block"]="ai.private-session-id";CHECK(!custom::parse(bad.dump()));
        // App route, not just the painter: model event -> published frame -> tile page target.
        {
            const auto local=root/"ai-app";custom::Store views(local/"views");
            custom::View first;first.id="ai-first";first.name="AI first";
            first.standard.tiles={custom::Tile{"ai","ai.claude",{"claude",0,0,1,1}}};
            first.standard.tiles[0].tap=custom::Tap{"page","custom:ai-second",""};
            first.standard.tiles[0].show_if=plugins::Condition{"data.claude.state","==","working"};
            auto second=first;second.id="ai-second";second.name="AI second";second.standard.tiles[0].tap.reset();
            CHECK(views.save(first).empty());CHECK(views.save(second).empty());
            auto live_cfg=cfg;live_cfg["ai"]["enabled"]=true;live_cfg["display"]["pages"]={"custom:ai-first","custom:ai-second"};live_cfg["display"]["visible_pages"]=live_cfg["display"]["pages"];
            App live(live_cfg,local/"config.toml",nullptr,local/"preview"); // suppress receiver and real CLI
            CHECK(!live.ai_status().running());live.render_primary_for_test();auto before=live.last_frame();CHECK(before);
            CHECK(live.ai_status().model().ingest(R"({"provider":"claude","session_id":"synthetic","hook_event_name":"PreToolUse","tool_name":"Read","model":"example"})",std::time(nullptr)));
            live.render_primary_for_test();auto after=live.last_frame();CHECK(after && before && after->px!=before->px);
            CHECK(live.page_navigation()["current"][0]=="custom:ai-first");
            if(after) {live.open_item_at(after->w*3/4,after->h/2);live.render_primary_for_test();CHECK(live.page_navigation()["current"][0]=="custom:ai-second");}
            CHECK(live.ai_status().model().ingest(R"({"provider":"claude","session_id":"synthetic","hook_event_name":"Stop"})",std::time(nullptr)));
            live.render_primary_for_test();CHECK(live.last_frame()->px==before->px);
        }
        ai::Model stopped;CHECK(stopped.ingest(R"({"provider":"claude","session_id":"sample","hook_event_name":"SessionEnd"})",now));
        CHECK(ai::tile_present(stopped.snapshot(now),"ai.claude")); // observed off is data
        ai::Model usage;usage.usage("codex",{{"five_hour",0}},"sample","",now);
        CHECK(ai::tile_present(usage.snapshot(now),"ai.codex"));CHECK(ai::tile_present(usage.snapshot(now),"ai.limits"));
        CHECK(!ai::tile_present(usage.snapshot(now),"ai.claude"));
    }
    // Native entity renderer shares the exact cell painting with the standard HA page.
    render::HaRenderer ha_renderer(480,320,"HA");auto entity=render::samples::ha().front();
    auto native=ha_renderer.render({entity},true,"",1700000000,1700000000);
    auto cell=render::HaRenderer::cells(1,480,320).front();
    auto entity_image=ha_renderer.render_entity(entity,{cell.x0,cell.y0,cell.x1-cell.x0,cell.y1-cell.y0});
    CHECK(entity_image.px!=blank.px);
    bool same_entity=true;for(int y=int(cell.y0);y<int(cell.y1);++y)for(int x=int(cell.x0);x<int(cell.x1);++x) same_entity=same_entity && entity_image.at(x,y)==native.at(x,y);
    CHECK(same_entity);CHECK(ha_renderer.render_entity(entity,{cell.x0,cell.y0,cell.x1-cell.x0,cell.y1-cell.y0},0).px!=entity_image.px);CHECK(ha_renderer.render_entity(entity,{8,36,20,20}).px==blank.px);
    // Every content source uses the real source renderer, with additive view composition.
    for(const auto& c:builtin::catalogs()) for(const auto& b:c.blocks) {
        custom::View sample;sample.id="sample";sample.name="Sample";auto t=tile;t.source=c.key;t.block=b.id;t.zone={"tile",0,0,1,1};t.show_if.reset();sample.standard.tiles={t};
        auto picture=app.custom_preview(sample,"",{480,320});CHECK(picture);
        if(picture && !b.required) CHECK(picture->px!=blank.px);
        auto before=builtin::to_json(builtin::standard(c));
        for(int d=0;d<3;++d){sample.standard.tiles[0].density=d;CHECK(custom::parse(custom::to_json(sample).dump()));CHECK(app.custom_preview(sample,"",{480,320}));}
        CHECK(builtin::to_json(builtin::standard(c))==before);
    }
    custom::View conditional;conditional.id="conditions";conditional.name="Conditions";
    auto sample_tile=tile;sample_tile.zone={"cpu",0,0,1,1};sample_tile.show_if=plugins::Condition{"data.cpu_pct","==",*render::samples::system().cpu_pct};conditional.standard.tiles={sample_tile};
    CHECK(app.custom_preview(conditional,"",{480,320})->px!=blank.px);
    conditional.standard.tiles[0].show_if->value=*render::samples::system().cpu_pct+1;
    CHECK(app.custom_preview(conditional,"",{480,320})->px==blank.px);
    auto invalid_density=j;invalid_density["layout"]["tiles"][0]["density"]=3;CHECK(!custom::parse(invalid_density.dump()));
    invalid_density["layout"]["tiles"][0]["density"]=1.5;CHECK(!custom::parse(invalid_density.dump()));
    // A custom draft can hold 24 tiles; plugin documents still keep their 12-block/8-zone limits.
    custom::Layout twentyfour;twentyfour.columns=twentyfour.rows={1,1,1,1,1,1};
    for(int i=0;i<24;++i){auto t=tile;t.show_if.reset();t.zone={"cell_"+std::to_string(i),i%6,i/6,1,1};twentyfour.tiles.push_back(t);}
    auto draft=custom::editor_model(twentyfour,app.custom_palette());CHECK(plugins::edit::geometry(draft));CHECK(custom::editor_layout(draft)->tiles.size()==24);
    CHECK(!plugins::parse_arrangement(plugins::to_json(draft).dump()));
    CHECK(plugins::edit::remove_block(draft,0));custom::prune_editor_zones(draft);CHECK(custom::editor_layout(draft)->tiles.size()==23);
    CHECK(plugins::edit::place_block(draft,0,0,0));CHECK(custom::editor_layout(draft)->tiles.size()==24);
    auto overlap=draft;overlap.blocks[0]["zone"]=overlap.blocks[1]["zone"];CHECK(!custom::editor_layout(overlap));
    auto empty=custom::editor_model(custom::Layout{},app.custom_palette());CHECK(custom::editor_layout(empty));CHECK(plugins::edit::geometry(empty));
    CHECK(plugins::edit::place_block(empty,0,0,0));CHECK(custom::editor_layout(empty)->tiles.size()==1);
    plugins::edit::EditHistory edits;edits.reset(empty);auto removed=empty;size_t shown=0;for(size_t i=0;i<removed.blocks.size();++i)if(removed.blocks[i].value("shown",false))shown=i;CHECK(plugins::edit::remove_block(removed,shown));custom::prune_editor_zones(removed);CHECK(edits.push(removed));CHECK(edits.undo());CHECK(custom::editor_layout(edits.current())->tiles.size()==1);CHECK(edits.redo());CHECK(custom::editor_layout(edits.current())->tiles.empty());
    auto listed=store.list();CHECK(!listed.empty());CHECK(std::none_of(listed.begin(),listed.end(),[](const auto& view){return view.id=="broken" || view.id=="wrong" || view.id=="huge";}));
    CHECK(!store.erase("../bad").empty());CHECK(store.erase("overview").empty());CHECK(!store.get("overview"));CHECK(!fs::exists(root/"views/overview.view.json.bak"));CHECK(store.save(v).empty());
    auto active=custom::active_displays(plugins::Json::array({{{"id","a"},{"online",true}},{{"id","b"},{"online",false}}}));CHECK(active.size()==1 && active[0]["id"]=="a");
    // Real plugin renderer: stable block selection, source conditions and missing data.
    auto parsed=plugins::parse_manifest(R"({"schema_version":2,"id":"fixture","name":"Fixture","version":"1.0.0",
      "sources":[{"id":"reading","url":"https://example.com/data"}],
      "widgets":[{"id":"value","type":"metric","label":"Value","value":"{{reading.value}}"},
                 {"id":"static","type":"text","text":"Static"}]})");
    CHECK(parsed);if(!parsed) {std::cerr<<parsed.error<<"\n";return 1;}
    custom::View pv;auto pt=v.standard.tiles.front();pt.source="plugin:fixture";pt.block="value";pv.standard.tiles={pt};
    plugins::RenderContext context;context.time="12:34";context.date="2026-11-07";
    auto plugin_draw=[&](const auto& t,const auto& l,int w,int h){return custom::draw_plugin(t,l,*parsed.manifest,context,w,h);};
    CHECK(custom::draw(pv,"",480,320,plugin_draw).px==blank.px);
    context.sources["reading"]={{"value",42}};
    auto plugin_image=custom::draw(pv,"",480,320,plugin_draw);CHECK(plugin_image.px!=blank.px);
    pv.standard.tiles[0].density=0;CHECK(custom::draw(pv,"",480,320,plugin_draw).px!=plugin_image.px);pv.standard.tiles[0].density=2;
    CHECK(plugin_image.at(0,0)==blank.at(0,0));CHECK(plugin_image.at(100,319)==blank.at(100,319));
    pv.standard.tiles[0].show_if=plugins::Condition{"data.reading.value",">",99};
    CHECK(custom::draw(pv,"",480,320,plugin_draw).px==blank.px);
    pv.standard.tiles[0].show_if.reset();pv.standard.tiles[0].block="absent";
    CHECK(custom::draw(pv,"",480,320,plugin_draw).px==blank.px);
    pv.standard.tiles[0].block="static";context.sources.clear();
    CHECK(custom::draw(pv,"",480,320,plugin_draw).px!=blank.px);
    parsed.manifest->schema_version=1;CHECK(!custom::draw_plugin(pt,pv.standard,*parsed.manifest,context,480,320));
    parsed.manifest->schema_version=2;pv.standard.tiles[0].source="plugin:other";
    CHECK(custom::draw(pv,"",480,320,plugin_draw).px==blank.px);
    // Failed validation leaves target and backup untouched.
    auto invalid=v;invalid.standard.rows={0};CHECK(!store.save(invalid).empty());
    CHECK(store.get(v.id)->standard.rows==std::vector<int>{1});
    auto invalid_device=j;invalid_device["devices"]={{"",j["layout"]}};CHECK(!custom::parse(invalid_device.dump()));
    auto negative=j;negative["layout"]["tiles"][0]["zone"]["column"]=-1;CHECK(!custom::parse(negative.dump()));
    auto out_of_bounds=j;out_of_bounds["layout"]["tiles"][0]["column_span"]=2;CHECK(!custom::parse(out_of_bounds.dump()));
    auto secret=j;secret["layout"]["tiles"][0]["show_if"]={{"path","settings.token"}};CHECK(!custom::parse(secret.dump()));
    auto good_condition=j;good_condition["layout"]["tiles"][0]["show_if"]={{"path","data.cpu_pct"},{"op",">="},{"value",0}};CHECK(custom::parse(good_condition.dump()));
    // Store count is bounded, including pre-existing invalid documents.
    custom::Store capped(root/"capped");auto copy=v;copy.devices.clear();
    for(size_t i=0;i<custom::view_limit;++i) {copy.id="view-"+std::to_string(i);CHECK(capped.save(copy).empty());}
    copy.id="overflow";CHECK(!capped.save(copy).empty());copy.id="view-0";CHECK(capped.save(copy).empty());
    // Reparse points and directory targets must not be followed.
    fs::create_directory(root/"views/directory.view.json");CHECK(!store.get("directory"));
    copy.id="directory";CHECK(!store.save(copy).empty());
    fs::create_directory(root/"outside");
    auto link=root/"linked";
    if(CreateSymbolicLinkW(link.c_str(),(root/"outside").c_str(),SYMBOLIC_LINK_FLAG_DIRECTORY|SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        custom::Store unsafe(link);CHECK(!unsafe.save(v).empty());CHECK(!unsafe.get(v.id));fs::remove(link);
    } else {std::cerr<<"Symlink creation unavailable; reparse test not run\n";}
    // ---- P8-3: individual touch targets per tile and per display overlay ----
    {
        using plugins::Json;
        auto plugin_tile=[&](const char* zone,int c,int r){custom::Tile t;t.source="plugin:fixture";t.block="value";t.zone={zone,c,r,1,1};return t;};
        custom::View tv;tv.id="touch";tv.name="Touch";tv.standard.tiles={plugin_tile("a",0,0)};
        auto parse_tap_doc=[&](const std::string& source,const Json& tap)->std::optional<custom::View>{
            auto doc=custom::to_json(tv);doc["layout"]["tiles"][0]["source"]=source;doc["layout"]["tiles"][0]["tap"]=tap;
            return custom::parse(doc.dump());
        };
        // Default stays byte-identical: no tap key unless a tile has an individual target.
        CHECK(!custom::to_json(tv)["layout"]["tiles"][0].contains("tap"));
        for(const Json& good:{Json{{"action","page"},{"page","system"}},Json{{"action","page"},{"page","custom:other"}},Json{{"action","page"},{"page","plugin:fixture"}},
                              Json{{"action","open_link"},{"url","https://example.com/a?b=1"}},Json{{"action","next"}}}) {
            auto parsed=parse_tap_doc("plugin:fixture",good);CHECK(parsed && parsed->standard.tiles[0].tap);
            if(parsed && parsed->standard.tiles[0].tap) {
                CHECK(custom::tap_json(*parsed->standard.tiles[0].tap)==good);
                auto again=custom::parse(custom::to_json(*parsed).dump());CHECK(again && again->standard.tiles[0].tap && custom::tap_json(*again->standard.tiles[0].tap)==good);
            }
        }
        // Anything invalid falls back to Standard; the document itself stays usable and is saved without the tap.
        const std::string long_link="https://example.com/"+std::string(2040,'a');
        for(const Json& bad:{Json{{"action","page"},{"page","custom:../x"}},Json{{"action","page"},{"page",""}},Json{{"action","page"},{"page","unknown"}},Json{{"action","page"},{"page",7}},
                             Json{{"action","page"}},Json{{"action","page"},{"page","system"},{"extra",1}},Json{{"action","open_link"},{"url","http://example.com/"}},
                             Json{{"action","open_link"},{"url",long_link}},Json{{"action","open_link"},{"url","https://example.com/../x"}},Json{{"action","open_link"}},
                             Json{{"action","next"},{"page","system"}},Json{{"action","run"}},Json{{"action",1}},Json::object(),Json::array(),Json("page"),Json(nullptr),Json(1)}) {
            auto parsed=parse_tap_doc("plugin:fixture",bad);CHECK(parsed);
            if(parsed) {CHECK(!parsed->standard.tiles[0].tap);CHECK(!custom::to_json(*parsed)["layout"]["tiles"][0].contains("tap"));}
        }
        // Link and next are only for plugin sources (Phase 4c); page works for every source.
        for(const char* source:{"system","spotify","homeassistant"}) {
            auto doc=custom::to_json(tv);doc["layout"]["tiles"][0]["source"]=source;doc["layout"]["tiles"][0]["block"]=std::string(source)=="system"?"system.cpu":std::string(source)=="spotify"?"spotify.text":"sensor.x";
            for(const Json& tap:{Json{{"action","open_link"},{"url","https://example.com/"}},Json{{"action","next"}}}) {
                doc["layout"]["tiles"][0]["tap"]=tap;auto parsed=custom::parse(doc.dump());CHECK(parsed && !parsed->standard.tiles[0].tap);
            }
            doc["layout"]["tiles"][0]["tap"]={{"action","page"},{"page","friends"}};auto page=custom::parse(doc.dump());CHECK(page && page->standard.tiles[0].tap);
        }
        // Per display overlay: each layout stores its own targets; unknown devices inherit Standard.
        custom::View own=tv;own.standard.tiles[0].tap=custom::Tap{"page","rss",""};
        own.devices["serial:COM7"]=own.standard;own.devices["serial:COM7"].tiles[0].tap=custom::Tap{"page","bf6",""};
        own.devices["monitor:sample"]=own.standard;own.devices["monitor:sample"].tiles[0].tap.reset();
        CHECK(store.save(own).empty());
        auto stored=store.get("touch");CHECK(stored);
        if(stored) {
            CHECK(stored->effective("").tiles[0].tap && stored->effective("").tiles[0].tap->page=="rss");
            CHECK(stored->effective("serial:COM7").tiles[0].tap && stored->effective("serial:COM7").tiles[0].tap->page=="bf6");
            CHECK(!stored->effective("monitor:sample").tiles[0].tap);
            CHECK(stored->effective("unknown-phone").tiles[0].tap && stored->effective("unknown-phone").tiles[0].tap->page=="rss");
        }
        // The editor draft keeps the target through its model and back (and drops nothing on re-layout).
        auto draft=custom::editor_model(own.standard,app.custom_palette());auto relayout=custom::editor_layout(draft);
        CHECK(relayout && relayout->tiles.size()==1 && relayout->tiles[0].tap && relayout->tiles[0].tap->page=="rss");

        // Touch hit mapping: each tile's target equals the pixels the tile paints, on every device geometry.
        const std::vector<std::string> cycle={"system","friends","rss","bf6","streaming","discord"};
        custom::View grid;grid.id="grid";grid.name="Grid";grid.standard.columns={1,2,1};grid.standard.rows={1,1};
        for(int i=0;i<6;++i) {custom::Tile t;t.source="system";t.block="system.cpu";t.zone={"z"+std::to_string(i),i%3,i/3,1,1};
            if(i!=4) t.tap=custom::Tap{"page",cycle[(i+1)%cycle.size()],""};   // tile 4 stays Default
            grid.standard.tiles.push_back(t);}
        grid.devices["serial:COM7"]=grid.standard;grid.devices["serial:COM7"].columns={1,1};grid.devices["serial:COM7"].rows={1};
        grid.devices["serial:COM7"].tiles={grid.standard.tiles[0],grid.standard.tiles[1]};grid.devices["serial:COM7"].tiles[0].zone={"z0",0,0,1,1};grid.devices["serial:COM7"].tiles[1].zone={"z1",1,0,1,1};
        grid.devices["serial:COM7"].tiles[0].tap=custom::Tap{"page","spotify",""};
        CHECK(custom::parse(custom::to_json(grid).dump()));
        auto tint=[](int i){return Color{uint8_t(30+i*31),uint8_t(220-i*17),uint8_t(60+i*9)};};
        auto index_of=[](const custom::Tile& t){return std::stoi(t.zone.id.substr(1));};
        auto tint_px=[&](int i){return Image(1,1,tint(i)).at(0,0);};
        auto source_draw=[&](const custom::Tile& t,const custom::Layout&,int w,int h)->std::optional<custom::TileDraw> {
            custom::TileDraw d{Image(w,h,tint(index_of(t))),{},{}};
            d.targets.push_back({{0,0,w,h},{{"https://example.org/standard",""}},{}});   // the source's own Standard behaviour
            return d;
        };
        struct Geometry {int w,h;};
        for(const std::string device:{std::string(),std::string("serial:COM7"),std::string("unknown-phone")})
            for(auto g:{Geometry{480,320},Geometry{800,480},Geometry{320,480},Geometry{1920,1080},Geometry{1280,720},Geometry{240,320}}) {
                std::vector<std::string> requested;
                custom::TapEnv env;env.pages=cycle;env.pages.push_back("spotify");env.go=[&](const std::string& page){requested.push_back(page);};
                auto out=custom::draw_out(grid,device,g.w,g.h,source_draw,custom::tap_override(grid.id,env));
                const auto& layout=grid.effective(device);
                CHECK(out.image.w==g.w && out.image.h==g.h);
                CHECK(out.targets.size()==layout.tiles.size());
                std::vector<touch::Tile> taps;touch::Tile page_tile;page_tile.box={0,0,g.w,g.h};page_tile.arranged=true;page_tile.arranged_targets=out.targets;taps.push_back(page_tile);
                for(size_t a=0;a<out.targets.size();++a) {
                    const auto r=out.targets[a].rect_px;
                    CHECK(r.w>0 && r.h>0 && r.x>=0 && r.y>=0 && r.x+r.w<=g.w && r.y+r.h<=g.h);
                    for(size_t b=a+1;b<out.targets.size();++b) {const auto q=out.targets[b].rect_px;CHECK(r.x+r.w<=q.x || q.x+q.w<=r.x || r.y+r.h<=q.y || q.y+q.h<=r.y);}
                }
                for(const auto& tile:layout.tiles) {
                    const int i=index_of(tile);bool matched=false;
                    // The target belongs to the tile exactly when its corner and centre pixels carry that tile's colour.
                    for(const auto& target:out.targets) {
                        const auto r=target.rect_px;const int cx=r.x+r.w/2,cy=r.y+r.h/2;
                        if(!(out.image.at(cx,cy)==tint_px(i))) continue;
                        matched=true;
                        CHECK(out.image.at(r.x,r.y)==tint_px(i) && out.image.at(r.x+r.w-1,r.y+r.h-1)==tint_px(i));
                        if(r.x>0) CHECK(!(out.image.at(r.x-1,cy)==tint_px(i)));
                        if(r.x+r.w<g.w) CHECK(!(out.image.at(r.x+r.w,cy)==tint_px(i)));
                        requested.clear();
                        const auto* action=touch::action_at(taps,cx,cy);
                        if(tile.tap) {CHECK(action);if(action)(*action)();CHECK(requested==std::vector<std::string>{tile.tap->page});CHECK(!touch::at(taps,cx,cy));}
                        else {CHECK(!action);auto links=touch::at(taps,cx,cy);CHECK(links && links->size()==1 && links->front().url=="https://example.org/standard");CHECK(requested.empty());}
                        if(tile.tap) CHECK(touch::action_at(taps,r.x,r.y) && touch::action_at(taps,r.x+r.w-1,r.y+r.h-1));   // whole tile responds, edge to edge
                    }
                    CHECK(matched);
                }
                CHECK(!touch::action_at(taps,1,1));CHECK(!touch::at(taps,g.w-1,g.h-1));   // header and footer are outside every tile
            }
        // Pages outside the current cycle, missing context or a missing go callback resolve to Standard.
        {
            custom::View one;one.id="one";one.name="One";custom::Tile t;t.source="system";t.block="system.cpu";t.zone={"z0",0,0,1,1};t.tap=custom::Tap{"page","friends",""};one.standard.tiles={t};
            auto run=[&](custom::TapEnv env){return custom::draw_out(one,"",480,320,source_draw,custom::tap_override("one",env));};
            custom::TapEnv env;env.pages={"friends"};env.go=[](const std::string&){};
            auto allowed=run(env);CHECK(allowed.targets.size()==1 && allowed.targets[0].action && allowed.targets[0].links.empty());
            auto not_in_cycle=env;not_in_cycle.pages={"system"};auto fallback=run(not_in_cycle);CHECK(fallback.targets.size()==1 && !fallback.targets[0].action && !fallback.targets[0].links.empty());
            auto no_go=env;no_go.go=nullptr;CHECK(!run(no_go).targets[0].action);
            CHECK(!run(custom::TapEnv{}).targets[0].action);
            CHECK(run(env).plugin.empty());
        }
        // Link targets: need an enabled plugin with Open links consent, a declared host and the current generation.
        {
            custom::View link;link.id="link";link.name="Link";auto t=plugin_tile("l",0,0);t.tap=custom::Tap{"open_link","","https://news.example.org/a"};link.standard.tiles={t};
            auto plugin_draw=[&](const custom::Tile& tile,const custom::Layout&,int w,int h)->std::optional<custom::TileDraw>{(void)tile;return custom::TileDraw{Image(w,h,Color{9,9,9}),{},{}};};
            bool consent=true;uint64_t generation=3;std::vector<std::string> hosts={"news.example.org"};
            custom::TapEnv env;env.pages={"system"};env.go=[](const std::string&){};
            env.grant=[&](const std::string& id)->std::optional<custom::LinkGrant>{
                if(id!="fixture" || !consent) return {};
                return custom::LinkGrant{"fixture",generation,[&](const std::string& url){plugins::Manifest m;m.links=hosts;return plugins::link_allowed(m,url);}};
            };
            auto run=[&]{return custom::draw_out(link,"",480,320,plugin_draw,custom::tap_override("link",env));};
            auto granted=run();CHECK(granted.plugin.size()==1 && granted.targets.empty());
            if(granted.plugin.size()==1) {
                const auto& target=granted.plugin[0];CHECK(target.action=="open_link" && target.url=="https://news.example.org/a" && target.plugin=="fixture" && target.generation==3 && target.block=="link/l/override");
                touch::Tile tile;tile.box={0,0,480,320};tile.plugin=granted.plugin;tile.plugin[0].stable_since=0;
                touch::PluginAccess access{true,true,3,hosts};
                auto decide=[&](touch::PluginAccess a,double now=10){return touch::plugin_decide({tile},tile.plugin[0].rect_px.x+3,tile.plugin[0].rect_px.y+3,now,1.0,[&](const std::string&){return a;});};
                CHECK(decide(access).kind==touch::PluginDecision::open_link && decide(access).url=="https://news.example.org/a");
                auto no_consent=access;no_consent.consent=false;CHECK(decide(no_consent).kind==touch::PluginDecision::swallow);
                auto stale=access;stale.generation=4;CHECK(decide(stale).kind==touch::PluginDecision::swallow);
                auto other_host=access;other_host.hosts={"other.example.org"};CHECK(decide(other_host).kind==touch::PluginDecision::swallow);
                auto disabled=access;disabled.enabled=false;CHECK(decide(disabled).kind==touch::PluginDecision::swallow);
                CHECK(decide(access,.5).kind==touch::PluginDecision::swallow);   // not yet stable
            }
            consent=false;auto denied=run();CHECK(denied.plugin.empty());       // no consent: Standard
            consent=true;hosts={"other.example.org"};CHECK(run().plugin.empty()); // host not declared: Standard
            hosts={"news.example.org"};env.grant=nullptr;CHECK(run().plugin.empty());
        }
    }
    // ---- P8-3: end to end through the real App (draw, tap, page request, next render) ----
    {
        auto root2=root/"touch-app";custom::Store views(root2/"views");
        auto make_tile=[&](const char*,const char* zone){custom::Tile t;t.source="plugin:fixture";t.block="static";t.zone={zone,0,0,1,1};return t;};
        const std::string fixture_manifest=R"({"schema_version":2,"id":"fixture","name":"Fixture","version":"1.0.0","widgets":[{"id":"static","type":"text","text":"Static"}]})";
        custom::View first,second,third;first.id="first";first.name="First";second.id="second";second.name="Second";third.id="third";third.name="Third";
        first.standard.tiles={make_tile("system.cpu","cpu")};second.standard.tiles={make_tile("system.ram","ram")};third.standard.tiles={make_tile("system.gpu","gpu")};
        auto config_for=[&](const std::vector<std::string>& pages){
            auto c=config::defaults();for(auto s:{"system","discord","friends","bf6","rss","streaming","homeassistant","spotify","companion"}) c[s]["enabled"]=false;
            c["ui"]["port"]=18866;c["ui"]["update_check"]=false;
            return config::merge(c,{{"display",{{"pages",pages},{"visible_pages",pages}}}});};
        auto current=[&](App& a){auto n=a.page_navigation();return n["current"].empty()?std::string():n["current"][0].get<std::string>();};
        // Standard behaviour: a tap in the right half steps forward, in the left half back. Fractions locate the tap on the frame.
        auto tap=[&](App& a,double fx,double fy){auto frame=a.last_frame();CHECK(frame);if(frame)a.open_item_at(int(frame->w*fx),int(frame->h*fy));a.render_primary_for_test();};
        const std::vector<std::string> all={"custom:first","custom:second","custom:third"};
        auto reset_views=[&](std::optional<custom::Tap> tap_on_first){first.standard.tiles[0].tap=tap_on_first;CHECK(views.save(first).empty());CHECK(views.save(second).empty());CHECK(views.save(third).empty());};
        // Without an individual target a right-half tap steps to the next page (Standard) ...
        reset_views(std::nullopt);
        {
            App app(config_for(all),root2/"config.toml",nullptr,root2/"preview");{std::string error;CHECK(app.plugin_import(fixture_manifest,error));}
            app.render_primary_for_test();CHECK(current(app)=="custom:first");
            tap(app,.75,.5);CHECK(current(app)=="custom:second");
            tap(app,.25,.5);CHECK(current(app)=="custom:first");
        }
        // ... with a target the whole tile jumps there, in either half, and does not step.
        reset_views(custom::Tap{"page","custom:third",""});
        {
            App app(config_for(all),root2/"config.toml",nullptr,root2/"preview");{std::string error;CHECK(app.plugin_import(fixture_manifest,error));}
            app.render_primary_for_test();CHECK(current(app)=="custom:first");
            tap(app,.75,.5);CHECK(current(app)=="custom:third");
        }
        {
            App app(config_for(all),root2/"config.toml",nullptr,root2/"preview");{std::string error;CHECK(app.plugin_import(fixture_manifest,error));}
            app.render_primary_for_test();tap(app,.25,.5);CHECK(current(app)=="custom:third");   // left half: Standard would also wrap to third, so check the right half above
            tap(app,.75,.5);CHECK(current(app)=="plugin:fixture");   // third has Standard behaviour: forward steps on to the (auto-listed) plugin page
        }
        reset_views(custom::Tap{"page","custom:second",""});
        {
            App app(config_for(all),root2/"config.toml",nullptr,root2/"preview");{std::string error;CHECK(app.plugin_import(fixture_manifest,error));}
            app.render_primary_for_test();tap(app,.25,.5);CHECK(current(app)=="custom:second");  // Standard left tap would have gone to third
        }
        // A target outside the active cycle (here: the disabled Friends page) resolves to Standard.
        reset_views(custom::Tap{"page","friends",""});
        {
            App app(config_for(all),root2/"config.toml",nullptr,root2/"preview");{std::string error;CHECK(app.plugin_import(fixture_manifest,error));}
            app.render_primary_for_test();tap(app,.75,.5);CHECK(current(app)=="custom:second");
        }
    }    fs::remove_all(root);
    std::cout<<checks-failures<<" passed, "<<failures<<" failed\n";return failures?1:0;
}


catch(const std::exception& e) {std::cerr<<"Test exception: "<<e.what()<<"\n";return 1;}
