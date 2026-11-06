// © 2026 FuchsLabs – ausschließlich vorhandene Datenquellen; Freischaltung je Seitenpaket.
#include "builtin_catalog.h"
#include "layout_edit.h"
#include <algorithm>
#include <limits>
#include <set>
#include <numeric>
namespace fl::builtin {
namespace {
zones::Thresholds thresholds(bool hint) {
    const float inf=zones::max_area;
    return {{{{hint?48.f:80.f, hint?24.f:44.f}, {hint?96.f:160.f,hint?32.f:66.f}, {inf,inf}}}};
}
Catalog make(std::string key, Label label, std::vector<BlockSpec> blocks) {
    Catalog c; c.key=std::move(key); c.label=std::move(label); c.blocks=std::move(blocks);
    c.standard.manifest_sha256.assign(64,'0');
    c.standard.layout={{"columns",{1}}, {"rows",Json::array()}, {"zones",Json::array()}};
    c.standard.blocks=Json::array();
    for(size_t i=0;i<c.blocks.size();++i) {
        auto z="zone_"+std::to_string(i);
        c.standard.layout["rows"].push_back(c.blocks[i].required?1:4);
        c.standard.layout["zones"].push_back({{"id",z},{"column",0},{"row",i}});
        c.standard.blocks.push_back({{"id",c.blocks[i].id},{"zone",z},{"shown",true}});
    }
    if(c.blocks.size()>2) {
        c.standard.layout["columns"]={1,1};c.standard.layout["rows"]=Json::array();
        for(size_t i=0;i<(c.blocks.size()+1)/2;++i) c.standard.layout["rows"].push_back(1);
        for(size_t i=0;i<c.blocks.size();++i) {
            c.standard.layout["zones"][i]["column"]=i%2;
            c.standard.layout["zones"][i]["row"]=i/2;
        }
    }
    c.portrait=c.standard;
    c.enabled=c.key=="rss";
    return c;
}
BlockSpec spec(std::string id, Label label, bool required=false, std::string action={}) {
    return {std::move(id),std::move(label),thresholds(required),1,required,std::move(action)};
}
Json canonical(const plugins::Arrangement& model) {
    auto j=plugins::to_json(model); j.erase("manifest_sha256");
    for(const auto* axis:{"columns","rows"}) {
        auto weights=j["layout"][axis].get<std::vector<int>>();int divisor=0;
        for(int weight:weights) divisor=std::gcd(divisor,weight);
        for(auto& weight:j["layout"][axis]) weight=weight.get<int>()/divisor;
    }
    for(auto& z:j["layout"]["zones"]) { if(!z.contains("column_span")) z["column_span"]=1; if(!z.contains("row_span")) z["row_span"]=1; }
    return j;
}
std::string check(const Catalog& c,const plugins::Arrangement& m) {
    auto parsed=plugins::parse_arrangement(plugins::to_json(m).dump());
    if(!parsed || !plugins::edit::geometry(m)) return "Invalid built-in arrangement.";
    if(m.blocks.size()!=c.blocks.size()) return "Every catalog block is required in the inventory.";
    std::set<std::string> seen;
    for(const auto& b:m.blocks) {
        auto it=std::find_if(c.blocks.begin(),c.blocks.end(),[&](const auto& s){return b["id"]==s.id;});
        if(it==c.blocks.end() || !seen.insert(it->id).second) return "Unknown catalog block.";
        if(it->required && !b.at("shown").get<bool>()) return "Required block must remain visible.";
        if(b.contains("show_if") || b.contains("seconds")) return "Built-in conditions and cycles are not enabled.";
        if(std::none_of(m.layout["zones"].begin(),m.layout["zones"].end(),[&](const auto& z){return b["zone"]==z["id"];})) return "Unknown block zone.";
    }
    for(const auto& z:m.layout["zones"]) {
        if(z.contains("cycle")) return "Built-in conditions and cycles are not enabled.";
        int count=0; for(const auto& b:m.blocks) if(b["zone"]==z["id"] && b["shown"].get<bool>()) ++count;
        if(count<1 || count>zones::max_blocks_per_zone) return "Every zone needs one to four shown blocks.";
    }
    return {};
}
}
const std::vector<Catalog>& catalogs() {
    static const std::vector<Catalog> all={
        make("rss",{"RSS","RSS"},{spec("rss.liste",{"Beiträge","Posts"},false,"post"),spec("rss.hinweis",{"Zustand","Status"},true)}),
        make("streaming",{"Streaming","Streaming"},{spec("streaming.liste",{"Titel","Titles"},false,"title"),spec("streaming.hinweis",{"Zustand","Status"},true)}),
        make("bf6",{"BF6","BF6"},{spec("bf6.rangliste",{"Rangliste","Ranking"},false,"player"),spec("bf6.hinweis",{"Zustand","Status"},true)}),
        make("friends",{"Freunde","Friends"},{spec("freunde.liste",{"Freunde","Friends"},false,"profile"),spec("freunde.hinweis",{"Zustand","Status"},true)}),
        make("discord",{"Discord","Discord"},{spec("freunde.liste",{"Freunde","Friends"},false,"profile"),spec("freunde.hinweis",{"Zustand","Status"},true)}),
        make("system",{"System","System"},{spec("system.cpu",{"CPU","CPU"}),spec("system.gpu",{"GPU","GPU"}),spec("system.ram",{"RAM","RAM"},false,"ram"),spec("system.datentraeger",{"Datenträger","Disk"},false,"disk"),spec("system.netz",{"Netzwerk","Network"},false,"network")}),
        make("spotify",{"Spotify","Spotify"},{spec("spotify.cover",{"Cover","Cover"},false,"app"),spec("spotify.text",{"Titel","Title"}),spec("spotify.pegel",{"Pegel","Levels"}),spec("spotify.fortschritt",{"Fortschritt","Progress"}),spec("spotify.tasten",{"Tasten","Controls"},false,"command"),spec("spotify.hinweis",{"Zustand","Status"},true)})
    };
    return all;
}
const Catalog* find(std::string_view key) { for(const auto& c:catalogs()) if(c.key==key) return &c; return nullptr; }
Overlay standard(const Catalog& c) { return {c.version,c.standard,c.portrait}; }
Json to_json(const Overlay& a) {
    Json j={{"schema_version",1},{"catalog_version",a.catalog_version},{"layout",a.model.layout},{"blocks",a.model.blocks}};
    if(a.portrait) j["portrait"]={{"layout",a.portrait->layout},{"blocks",a.portrait->blocks}};
    return j;
}
std::string validate(const Catalog& c,const Overlay& a) {
    if(a.catalog_version!=c.version) return "Catalog version is stale.";
    auto error=check(c,a.model); if(!error.empty()) return error;
    if(a.portrait) { error=check(c,*a.portrait); if(!error.empty()) return error; }
    for(bool portrait:{false,true}) {
        const auto& model=portrait?a.portrait.value_or(a.model):a.model;
        const auto layout=plugins::edit::geometry(model);
        auto plan=zones::plan({8,36,portrait?304.f:464.f,portrait?426.f:266.f},*layout,inputs(c,model),6);
        for(const auto& spec:c.blocks) if(spec.required && std::none_of(plan.blocks.begin(),plan.blocks.end(),[&](const auto& b){return b.id==spec.id && b.visible();}))
            return "Required block does not fit the page.";
    }
    return {};
}
Result parse(const Catalog& c,std::string_view text) {
    try {
        auto parsed=plugins::parse_strict_json(text,plugins::arrangement_limit);
        if(!parsed || !parsed->is_object()) return {{},"Invalid built-in overlay JSON."};
        const auto& j=*parsed;
        for(auto it=j.begin();it!=j.end();++it) if(it.key()!="schema_version" && it.key()!="catalog_version" && it.key()!="layout" && it.key()!="blocks" && it.key()!="portrait") return {{},"Unknown overlay field."};
        if(!j.at("schema_version").is_number_integer() || j.at("schema_version")!=1 || !j.at("catalog_version").is_number_integer()) return {{},"Invalid overlay version."};
        Overlay a{j.at("catalog_version").get<int>(),{std::string(64,'0'),j.at("layout"),j.at("blocks")},{}};
        if(j.contains("portrait")) {
            const auto& p=j.at("portrait");
            if(!p.is_object() || p.size()!=2) return {{},"Invalid portrait overlay."};
            a.portrait=plugins::Arrangement{std::string(64,'0'),p.at("layout"),p.at("blocks")};
        }
        auto error=validate(c,a); if(!error.empty()) return {{},error};
        return {std::move(a),{}};
    } catch(const Json::exception&) { return {{},"Invalid built-in overlay value."}; }
}
bool is_standard(const Catalog& c,const plugins::Arrangement& model,bool portrait) {
    return check(c,model).empty() && canonical(model)==canonical(portrait?c.portrait:c.standard);
}
bool is_standard(const Catalog& c,const Overlay& a) {
    return validate(c,a).empty() && canonical(a.model)==canonical(c.standard) &&
        canonical(a.portrait.value_or(a.model))==canonical(c.portrait);
}
std::vector<zones::BlockInput> inputs(const Catalog& c,const plugins::Arrangement& m) {
    std::vector<zones::BlockInput> out;
    for(const auto& b:m.blocks) if(b.value("shown",false))
        for(const auto& s:c.blocks) if(b["id"]==s.id) out.push_back({s.id,b["zone"],s.thresholds});
    return out;
}
} // namespace fl::builtin
