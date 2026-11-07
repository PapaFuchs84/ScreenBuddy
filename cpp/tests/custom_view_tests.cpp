// © 2026 FuchsLabs
#include "custom_view.h"
#include "custom_view_editor.h"
#include "page_samples.h"
#include "app.h"
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
    CHECK(config::defaults()["display"]["pages"]==ojson(config::PAGES));CHECK(config::PAGES==std::vector<std::string>({"system","friends","discord","bf6","rss","streaming","homeassistant","spotify"}));
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
    fs::remove_all(root);
    std::cout<<checks-failures<<" passed, "<<failures<<" failed\n";return failures?1:0;
}


catch(const std::exception& e) {std::cerr<<"Test exception: "<<e.what()<<"\n";return 1;}
