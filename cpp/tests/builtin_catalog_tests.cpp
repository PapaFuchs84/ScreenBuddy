// © 2026 FuchsLabs – reine Katalogprüfung, ohne Renderer und Dateien.
#include "builtin_catalog.h"
#include "layout_edit.h"
#include "device_overlays.h"
#include <cstdio>
#include <algorithm>
using namespace fl::builtin;
int main() {
    int ok=0,fail=0;
    auto check=[&](bool value,int line){if(value) ++ok; else {++fail; std::printf("FAIL %d\n",line);}};
#define CHECK(x) check(bool(x),__LINE__)
    CHECK(find("ai")); CHECK(!find("homeassistant")); CHECK(!find("../rss"));
    for(const auto& c:catalogs()) {
        for(const auto& b:c.blocks) CHECK(zones::valid(b.thresholds));
        auto a=standard(c); CHECK(validate(c,a).empty()); CHECK(is_standard(c,a));
        CHECK(parse(c,to_json(a).dump())); CHECK(c.enabled==((c.key=="ai" || c.key=="rss" || c.key=="streaming" || c.key=="bf6" || c.key=="friends" || c.key=="discord" || c.key=="system" || c.key=="spotify")));
        for(auto& z:a.model.layout["zones"]) {z["column_span"]=1;z["row_span"]=1;}
        CHECK(is_standard(c,a));
        a.catalog_version++; CHECK(!validate(c,a).empty()); CHECK(!parse(c,to_json(a).dump()));
        a=standard(c);a.model.blocks[0]["id"]="unknown"; CHECK(!validate(c,a).empty());
        a=standard(c);a.model.blocks.erase(a.model.blocks.begin()); CHECK(!validate(c,a).empty());
        for(size_t i=0;i<c.blocks.size();++i) if(c.blocks[i].required) {
            a=standard(c);a.model.blocks[i]["shown"]=false;CHECK(!validate(c,a).empty());
            a=standard(c);a.portrait->blocks[i]["shown"]=false;CHECK(!validate(c,a).empty());
        }
        a=standard(c);a.model.layout["rows"][0]=3;CHECK(validate(c,a).empty());CHECK(!is_standard(c,a));
        auto j=to_json(a);j["private"]=true;CHECK(!parse(c,j.dump()));
        CHECK(!parse(c,std::string(16385,' ')));CHECK(!parse(c,"{\"schema_version\":1,\"schema_version\":1}"));
        a=standard(c);a.model.blocks[0]["zone"]="missing";CHECK(!validate(c,a).empty());
        a=standard(c);a.model.blocks[0]["show_if"]={{"path","source.ok"}};CHECK(!validate(c,a).empty());
        a=standard(c);a.model.layout["zones"][0]["cycle"]={{"mode","rotate"}};CHECK(!validate(c,a).empty());
        a=standard(c);a.portrait.reset();CHECK(is_standard(c,a));
        a=standard(c);for(auto* model:{&a.model,&*a.portrait}) for(auto& weight:model->layout["rows"]) weight=weight.get<int>()*2;
        CHECK(is_standard(c,a));
        CHECK(inputs(c,c.standard).size()==std::count_if(c.blocks.begin(),c.blocks.end(),[](const auto& b){return !b.tile_only;}));
        for(bool portrait:{false,true}) {
            const auto& model=portrait?c.portrait:c.standard;
            auto plan=zones::plan({8,36,portrait?304.f:464.f,portrait?426.f:266.f},*fl::plugins::edit::geometry(model),inputs(c,model),6);
            for(const auto& block:plan.blocks) CHECK(block.visible());
        }
    }
    {
        const auto& ai=*find("ai");auto a=standard(ai);
        CHECK(a.model.blocks.size()==8);CHECK(a.model.layout["columns"]==Json({1,1}));CHECK(a.model.layout["rows"]==Json({1,1,1,1}));
        CHECK(a.model.blocks[0]["id"]=="ai.claude.state");CHECK(a.model.blocks[7]["id"]=="ai.codex.tokens_session");
        a.model.blocks[0]["id"]="ai.claude";CHECK(!validate(ai,a).empty()); // palette-only block never alters full-page inventory
    }
    for(const auto* id:{"ai.claude","ai.codex","ai.limits"}) {
        auto a=standard(*find("ai"));
        a.single_tile=true;a.portrait.reset();a.model.layout["columns"]={1};a.model.layout["rows"]={1};
        a.model.layout["zones"]=Json::array({{{"id","tile"},{"column",0},{"row",0}}});
        a.model.blocks=Json::array({{{"id",id},{"zone","tile"},{"shown",true}}});
        CHECK(validate(*find("ai"),a).empty());CHECK(!parse(*find("ai"),to_json(a).dump()));
        a.model.blocks[0]["shown"]=false;CHECK(!validate(*find("ai"),a).empty());
    }
    // Geräte-Container: strikt, begrenzt und bei Fehler transaktional.
    {
        using namespace fl::device;
        OverlayMap map;
        const auto& c=*find("rss");const auto overlay=to_json(standard(c));
        for(int i=0;i<16;++i) map["device-"+std::to_string(i)]=overlay;
        CHECK(overlay_fits(map));
        OverlayMap parsed;
        auto validate=[&](const auto& bytes){return bool(parse(c,bytes));};
        CHECK(parse_overlays(overlays_json(map).dump(),parsed,validate));CHECK(parsed==map);
        map["device-16"]=overlay;CHECK(!overlay_fits(map));
        CHECK(!parse_overlays(overlays_json(map).dump(),parsed,validate));CHECK(parsed.size()==16);
        CHECK(!parse_overlays(std::string(overlay_bytes_limit+1,' '),parsed,validate));
        CHECK(!parse_overlays(R"({"schema_version":1,"schema_version":1,"devices":{}})",parsed,validate));
        map.clear();map[""]=overlay;CHECK(!overlay_fits(map));
        map.clear();map[std::string(513,'a')]=overlay;CHECK(!overlay_fits(map));
        map.clear();map["bad\nID"]=overlay;CHECK(!overlay_fits(map));
        map.clear();map["device"]=Json{{"bad",true}};CHECK(!parse_overlays(overlays_json(map).dump(),parsed,validate));
        map.clear();for(int i=0;i<15;++i) map["id-"+std::to_string(i)]=std::string(16300,'x');
        CHECK(overlay_fits(map));map["sixteen"]=std::string(16300,'x');CHECK(!overlay_fits(map));
        CHECK(!parse_overlays(overlays_json(map).dump(),parsed,[](const auto&){return true;}));
        CHECK(!valid_layout_id(""));CHECK(valid_layout_id("monitor:\\\\DISPLAY2"));
    }
    std::printf("%d passed, %d failed\n",ok,fail); return fail?1:0;
}
