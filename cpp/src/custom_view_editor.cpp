// © 2026 FuchsLabs – Clean-Room, existing ScreenBuddy gestures and bounded view model.
#include "custom_view_editor.h"
#include <algorithm>
namespace fl::custom {
View game_mode_template(std::string id,bool english) {
    View v;v.id=std::move(id);v.name=english?"Game-Mode":"Game-Mode";v.standard.columns={5,6,4,4};v.standard.rows={1,1,1};
    // Cockpit: logo+name left, FPS hero in the middle, hardware on the right (column,row,column span,row span).
    struct Cell {const char* block;int column,row,columns,rows;};
    const std::vector<Cell> cells={{"system.game_logo",0,0,1,2},{"system.fps",1,0,1,2},{"system.gpu",2,0,1,1},{"system.cpu",3,0,1,1},{"system.vram",2,1,2,1},
        {"system.game_time",0,2,1,1},{"system.ping",1,2,1,1},{"system.frametime",2,2,2,1}};
    for(size_t i=0;i<cells.size();++i){Tile t;t.source="system";t.block=cells[i].block;t.zone={"game_"+std::to_string(i),cells[i].column,cells[i].row,cells[i].columns,cells[i].rows};v.standard.tiles.push_back(t);}
    return v;
}

plugins::Arrangement editor_model(const Layout& layout,const std::vector<Tile>& palette) {
    plugins::Arrangement model;model.custom_draft=true;model.manifest_sha256.assign(64,'0');
    model.layout={{"columns",layout.columns},{"rows",layout.rows},{"zones",Json::array()}};model.blocks=Json::array();
    auto add=[&](const Tile& tile,bool shown) {
        Json b={{"id","tile_"+std::to_string(model.blocks.size())},{"source",tile.source},{"block",tile.block},
                {"zone",tile.zone.id},{"shown",shown},{"density",tile.density}};
        if(tile.tap)b["tap"]=tap_json(*tile.tap);
        if(tile.show_if) {b["show_if"]={{"path",tile.show_if->path}};if(!tile.show_if->op.empty()) {b["show_if"]["op"]=tile.show_if->op;b["show_if"]["value"]=tile.show_if->value;}}
        model.blocks.push_back(b);
    };
    for(const auto& tile:layout.tiles) {
        const auto& z=tile.zone;
        model.layout["zones"].push_back({{"id",z.id},{"column",z.column},{"row",z.row},{"column_span",z.column_span},{"row_span",z.row_span}});
        add(tile,true);
    }
    for(const auto& tile:palette) {
        if(model.blocks.size()>=256) break;
        if(std::none_of(layout.tiles.begin(),layout.tiles.end(),[&](const auto& t){return t.source==tile.source && t.block==tile.block;})) add(tile,false);
    }
    return model;
}
std::optional<Layout> editor_layout(const plugins::Arrangement& model) {
    if(!model.custom_draft || !plugins::edit::geometry(model)) return {};
    try {
        View view;view.id="editor";view.name="Editor";
        Json j=to_json(view);j["layout"]={{"columns",model.layout.at("columns")},{"rows",model.layout.at("rows")},{"tiles",Json::array()}};
        for(const auto& b:model.blocks) if(b.at("shown").get<bool>()) {
            auto zone=std::find_if(model.layout["zones"].begin(),model.layout["zones"].end(),[&](const auto& z){return z.at("id")==b.at("zone");});
            if(zone==model.layout["zones"].end()) return {};
            Json tile={{"source",b.at("source")},{"block",b.at("block")},{"zone",{{"id",zone->at("id")},{"column",zone->at("column")},{"row",zone->at("row")}}},
                {"column_span",zone->value("column_span",1)},{"row_span",zone->value("row_span",1)},{"density",b.value("density",2)}};
            if(b.contains("tap")) tile["tap"]=b["tap"];
            if(b.contains("show_if")) tile["show_if"]=b["show_if"];
            j["layout"]["tiles"].push_back(tile);
        }
        auto parsed=parse(j.dump());if(parsed) return parsed->standard;
    } catch(const Json::exception&) {}
    return {};
}
void prune_editor_zones(plugins::Arrangement& model) {
    auto& zones=model.layout["zones"];
    zones.erase(std::remove_if(zones.begin(),zones.end(),[&](const auto& z) {
        return std::none_of(model.blocks.begin(),model.blocks.end(),[&](const auto& b){return b.value("shown",false) && b["zone"]==z["id"];});
    }),zones.end());
}
Json active_displays(const Json& input) {
    Json result=Json::array();if(!input.is_array()) return result;
    for(const auto& d:input) if(d.is_object() && d.value("online",false)) result.push_back(d);
    return result;
}
}
