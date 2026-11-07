// © 2026 FuchsLabs – reine Modell-/Historientests, ohne Netz.
#include "layout_edit.h"
#include <cstdio>
using namespace fl::plugins;
using namespace fl::plugins::edit;
static int passed=0, failed=0;
#define CHECK(x) do { if(x) ++passed; else { ++failed; std::printf("FAIL %d: %s\n",__LINE__,#x); } } while(0)
static Model fixture() {
    return {std::string(64,'0'), {{"columns",{1,1,1}}, {"rows",{1,1,1}},
        {"zones", {{{"id","one"},{"column",0},{"row",0}},{{"id","two"},{"column",2},{"row",2}}}}},
        {{{"id","a"},{"zone","one"},{"shown",true}},{{"id","b"},{"zone","two"},{"shown",true}}}};
}
int main() {
    auto m=fixture(), base=m;
    CHECK(geometry(m)); CHECK(parse_arrangement(to_json(m).dump()));
    CHECK(move_zone(m,0,1,0)); CHECK(m.layout["zones"][0]["column"]==1);
    CHECK(move_zone(m,0,0,1)); CHECK(!move_zone(m,0,1,1));
    auto before=m; CHECK(!move_zone(m,0,-9,0)); CHECK(equal(m,before));
    CHECK(!resize_zone(m,0,3,0)); CHECK(equal(m,before));
    m=base; CHECK(resize_zone(m,0,1,0)); CHECK(resize_zone(m,0,0,1));
    CHECK(!resize_zone(m,0,-2,0)); CHECK(!resize_zone(m,0,0,-2));
    CHECK(split_zone(m,0,true)); CHECK(m.layout["zones"].size()==3);
    CHECK(!split_zone(m,0,true)); CHECK(delete_zone(m,2));
    CHECK(assign_block(m,1,0)); CHECK(m.blocks[1]["zone"]=="one"); CHECK(!assign_block(m,9,0));
    m=base; CHECK(delete_zone(m,0)); CHECK(m.blocks[0]["zone"]=="two");
    CHECK(!delete_zone(m,0)); CHECK(!delete_zone(m,99));
    m=base; CHECK(add_zone(m,1,1)); before=m; CHECK(!add_zone(m,1,1)); CHECK(equal(m,before));
    CHECK(resize_axis(m,true,1)); CHECK(resize_axis(m,true,-1)); CHECK(!resize_axis(m,true,-1));
    CHECK(weight(m,true,0,12)); CHECK(!weight(m,true,0,13)); CHECK(!weight(m,false,0,0));
    CHECK(!weight(m,false,9,1));
    m=base; for(int i=0;i<4;++i) m.blocks.push_back({{"id","c"+std::to_string(i)},{"zone","two"},{"shown",true}});
    CHECK(delete_zone(m,0)); CHECK(!m.blocks[0]["shown"].get<bool>());
    EditHistory h; h.reset(base); CHECK(!h.can_undo()); CHECK(!h.can_redo()); CHECK(!h.push(base));
    m=base; move_zone(m,0,1,0); CHECK(h.push(m,"move:0",100));
    auto first=m; move_zone(m,0,0,1); CHECK(h.push(m,"move:0",699)); CHECK(h.steps()==1);
    CHECK(h.undo()); CHECK(equal(h.current(),base)); CHECK(h.redo()); CHECK(equal(h.current(),m));
    CHECK(!h.redo()); CHECK(h.undo()); CHECK(h.push(first)); CHECK(!h.can_redo());
    h.reset(base); m=base;
    for(int i=0;i<120;++i) { weight(m,true,0,i%12+1); h.push(m); }
    CHECK(h.steps()==100); int n=0; while(h.undo()) ++n; CHECK(n==100);
    n=0; while(h.redo()) ++n; CHECK(n==100); CHECK(equal(h.current(),m));
    h.reset(base); m=base; move_zone(m,0,1,0); h.push(m,"a",1); move_zone(m,0,0,1); h.push(m,"a",602); CHECK(h.steps()==2);
    h.reset(base); m=base; move_zone(m,0,1,0); h.push(m,"a",1); move_zone(m,0,0,1); h.push(m,"b",2); CHECK(h.steps()==2);
    CHECK(diagnostic("unknown",false).find("ungültig")!=std::string::npos);
    CHECK(diagnostic("Arrangement requires manifest v2.",true)=="Arrangement requires manifest v2.");
    for(auto error:{"Plugin is not available.","Plugin library is not available.","Plugin is not installed.",
        "Arrangement does not match the plugin version.","Arrangement files contain an unsafe link.",
        "Arrangement file is unavailable.","Arrangement backup could not be saved.",
        "Arrangement could not be saved atomically.","Arrangement backup could not be removed.","Arrangement could not be removed."}) {
        CHECK(diagnostic(error,true)==error); CHECK(diagnostic(error,false)!=error);
    }
    m=base; m.blocks[0]["show_if"]={{"path","sample.n"},{"op","=="},{"value",1}};
    h.reset(m); auto numeric=m; numeric.blocks[0]["show_if"]["value"]=1.0;
    CHECK(!equal(m,numeric)); CHECK(h.push(numeric)); CHECK(h.undo()); CHECK(to_json(h.current()).dump()==to_json(m).dump());
    // P7-2: Probe und Ablage sind atomar, Inventar/IDs bleiben vollständig erhalten.
    m=base; before=m;
    CHECK(probe(m,[](auto& n) { return place_block(n,0,1,1); })); CHECK(equal(m,before));
    CHECK(!probe(m,[](auto& n) { return place_block(n,0,-1,1); })); CHECK(equal(m,before));
    for (auto point : {std::pair{-1,0}, {0,-1}, {3,0}, {0,3}}) {
        m=base; CHECK(!place_block(m,0,point.first,point.second)); CHECK(equal(m,base));
    }
    m=base; CHECK(place_block(m,0,1,1)); CHECK(m.layout["zones"].size()==3);
    CHECK(m.blocks.size()==2); CHECK(m.blocks[0]["zone"]=="two" || m.blocks[1]["zone"]=="two");
    m=base; CHECK(remove_block(m,0)); CHECK(!m.blocks[0]["shown"].get<bool>());
    CHECK(place_block(m,0,2,2)); CHECK(m.layout["zones"].size()==2);
    CHECK(m.blocks[0]["id"]=="a"); CHECK(m.blocks[0]["shown"].get<bool>());
    CHECK(move_block(m,0,1,1)); CHECK(m.blocks[1]["id"]=="a");
    before=m; CHECK(!move_block(m,0,99,0)); CHECK(equal(m,before));
    CHECK(!move_block(m,99,0,0)); CHECK(equal(m,before));
    CHECK(!move_block(m,0,1,99)); CHECK(equal(m,before));
    CHECK(!remove_block(m,99)); CHECK(equal(m,before));
    CHECK(!place_block(m,99,1,1)); CHECK(equal(m,before));
    m=base; m.blocks[0]["seconds"]=4; CHECK(move_block(m,0,1,1)); CHECK(!m.blocks[1].contains("seconds"));
    m=base; for(int i=0;i<3;++i) m.blocks.push_back({{"id","extra"+std::to_string(i)},{"zone","two"},{"shown",true}});
    before=m; CHECK(!place_block(m,0,2,2)); CHECK(equal(m,before)); // fünfter gezeigter Block
    CHECK(remove_block(m,4)); CHECK(place_block(m,0,2,2));
    m=base; m.layout["columns"]={1,1,1,1};
    for(auto cell : {std::pair{1,0}, {2,0}, {3,0}, {0,1}, {1,1}, {2,1}}) CHECK(add_zone(m,cell.first,cell.second));
    before=m; CHECK(!place_block(m,0,3,1)); CHECK(equal(m,before)); // neunte Zone
    m=base; for(int i=0;i<10;++i) m.blocks.push_back({{"id","hidden"+std::to_string(i)},{"zone","one"},{"shown",false}});
    CHECK(place_block(m,11,1,1)); CHECK(m.blocks.size()==12);
    m.blocks.push_back({{"id","thirteenth"},{"zone","one"},{"shown",false}}); before=m;
    CHECK(!place_block(m,12,1,2)); CHECK(equal(m,before));
    for(auto field : {"id","zone","shown"}) {
        m=base; m.blocks[1].erase(field); before=m;
        CHECK(!place_block(m,0,1,1)); CHECK(equal(m,before));
    }
    m=base; CHECK(delete_zone(m,0)); CHECK(!m.blocks[0]["shown"].get<bool>());
    CHECK(m.blocks[0]["zone"]=="two"); CHECK(m.blocks.size()==base.blocks.size());
    // Befund 2: Ziele, die eine Zone ohne gezeigten Block hinterlassen, sind erkennbar.
    m=base; auto after=m; CHECK(remove_block(after,0)); CHECK(leaves_empty_zone(m,after));
    after=m; CHECK(move_block(after,0,1,0)); CHECK(leaves_empty_zone(m,after));
    after=m; CHECK(place_block(after,0,1,1)); CHECK(leaves_empty_zone(m,after));
    after=m; CHECK(delete_zone(after,0)); CHECK(!leaves_empty_zone(m,after));
    m.blocks.push_back({{"id","c"},{"zone","one"},{"shown",true}});
    after=m; CHECK(remove_block(after,0)); CHECK(!leaves_empty_zone(m,after));
    after=m; CHECK(move_block(after,0,1,0)); CHECK(!leaves_empty_zone(m,after));
    after=m; CHECK(remove_block(after,2)); CHECK(remove_block(after,0)); CHECK(leaves_empty_zone(m,after));
    CHECK(!leaves_empty_zone(m,m));
    m=base; m.blocks[0]["shown"]=false; after=m; CHECK(move_block(after,0,1,0)); CHECK(!leaves_empty_zone(m,after)); // vorher schon leer
    std::printf("Layout edit: %d passed, %d failed\n",passed,failed); return failed?1:0;
}
