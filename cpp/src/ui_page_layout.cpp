// © 2026 FuchsLabs – Anordnung installierter v2-Plugins. Keine Manifest-/Konfigurationsschreibwege.
#include "ui_pages.h"
#include "ui_layout_grid.h"
#include "native_settings.h"
#include "app.h"
#include "util.h"
#include <algorithm>
#include <oleacc.h>

namespace fl::ui {
namespace {
namespace pe = plugins::edit;
using plugins::Json;
class LayoutPage : public Page {
public:
    PageInfo info() const override {
        return {"layout", Group::App, T(L"Plugin-Seite anordnen",L"Arrange plugin page"),
            T(L"Blöcke im Raster anordnen · Vorschau mit Beispieldaten",L"Arrange blocks in a grid · preview with sample data")};
    }
    void build(Host& host, Column& root) override {
        host_=&host;
        // Nach „Verwerfen“ können alle bisherigen Widgets bereits zerstört sein.
        grid_=nullptr; blocks_=nullptr; zone_id_=seconds_=literal_=nullptr;
        apply_=undo_=redo_=reset_=error_=nullptr; validation_=nullptr;
        auto card=root.add<Card>(T(L"Plugin auswählen",L"Select plugin"));
        choice_=card->row<Combo>(T(L"Aktiviertes Plugin",L"Enabled plugin"),Text(),std::vector<Text>{},-1);
        choice_->on_change=[this] { choose(choice_->index); };
        status_=root.add<Label>(T(L"Wird geladen …",L"Loading …"));
        body_=root.add<Column>();
        if (!entries_.empty()) { fill_choices(); choose(std::clamp(chosen_,0,int(entries_.size())-1),true); }
        clean(root);
    }
    void shown(Host&) override { visible_=true; reload_=true; tick(*host_,host_->status()); }
    void hidden(Host& h) override { visible_=false; ++generation_; h.preview(std::nullopt); }
    void tick(Host&,const Status&) override {
        if (!visible_) return;
        if (host_->busy()) { update_buttons(); return; }
        if (reload_) { load(); return; }
        if (pending_ && !previewing_) preview();
        update_buttons();
    }
private:
    struct Entry { std::string id; Text name; Json data; };
    Host* host_=nullptr;
    Column* body_=nullptr;
    Combo* choice_=nullptr;
    Label *status_=nullptr, *validation_=nullptr;
    GridEditor* grid_=nullptr;
    LayoutBlockList* blocks_=nullptr;
    TextField *zone_id_=nullptr, *seconds_=nullptr, *literal_=nullptr;
    Combo *block_zone_=nullptr, *path_=nullptr;
    Segment *cycle_=nullptr, *transition_=nullptr, *op_=nullptr, *type_=nullptr;
    Button *apply_=nullptr, *undo_=nullptr, *redo_=nullptr, *reset_=nullptr, *error_=nullptr;
    Slider *weights_[2][6]{};
    Row *weight_rows_[2][6]{};
    std::vector<Entry> entries_;
    std::vector<std::string> paths_;
    plugins::Manifest base_;
    pe::EditHistory history_;
    int chosen_=-1, block_=0;
    bool visible_=false, reload_=false, pending_=false, previewing_=false, valid_=false, syncing_=false, uncommitted_=false, portrait_=false;
    uint64_t generation_=0;
    int issue_zone_=-1, issue_block_=-1;
    std::map<TextField*,std::wstring> reported_;
    static void clean(Widget& w) {
        w.track_dirty=false; std::vector<Widget*> children; w.children(children); for(auto c:children) clean(*c);
    }
    const pe::Model& model() const { return history_.current(); }
    void update_buttons() {
        if (!apply_) return;
        const bool idle=!host_->busy();
        apply_->enabled=idle && valid_ && !uncommitted_ && bool(host_->plugins().arrangement_apply);
        reset_->enabled=idle && bool(host_->plugins().arrangement_reset);
        undo_->enabled=history_.can_undo(); redo_->enabled=history_.can_redo();
        if(error_) error_->enabled=!valid_;
        choice_->enabled=idle;
        host_->surface().invalidate();
    }
    void load() {
        auto h=host_; reload_=false;
        if (!h->plugins().list || !h->plugins().arrangement_get) {
            status_->text=T(L"Anordnung ist in diesem Build nicht angebunden.",L"Arrangement is not connected in this build."); return;
        }
        h->run([this,h]() -> std::function<void()> {
            std::vector<Entry> entries;
            for(auto& e:h->plugins().list()) if(e.enabled) entries.push_back({e.id,Text(widen(e.name)),h->plugins().arrangement_get(e.id)});
            return [this,entries=std::move(entries)] {
                std::string selected=chosen_>=0 && chosen_<int(entries_.size()) ? entries_[chosen_].id : "";
                entries_=entries; chosen_=0;
                for(size_t i=0;i<entries_.size();++i) if(entries_[i].id==selected) chosen_=int(i);
                fill_choices(); choose(entries_.empty()?-1:chosen_,true);
            };
        });
    }
    void fill_choices() { choice_->options.clear(); for(auto& e:entries_) choice_->options.push_back(e.name); choice_->index=chosen_; }
    void choose(int i, bool preserve=false) {
        if(!preserve && i!=chosen_ && (history_.can_undo() || uncommitted_) &&
            !host_->confirm(T(L"Ungesicherte Änderungen verwerfen und das Plugin wechseln?",L"Discard unsaved changes and switch plugin?"))) {
            choice_->index=chosen_; host_->surface().invalidate(); return;
        }
        reported_.clear();
        ++generation_; pending_=false; chosen_=i; choice_->index=i;
        // Alte Eingabefelder nicht während ihres Abbaus erneut abschließen.
        if(zone_id_) zone_id_->on_commit={}; if(seconds_) seconds_->on_commit={}; if(literal_) literal_->on_commit={};
        body_->clear(); grid_=nullptr; blocks_=nullptr; apply_=undo_=redo_=reset_=error_=nullptr;
        zone_id_=seconds_=literal_=nullptr; validation_=nullptr;
        if(i<0 || i>=int(entries_.size())) {
            status_->text=T(L"Kein aktiviertes Plugin installiert.",L"No enabled plugin installed."); host_->relayout(); return;
        }
        const auto& data=entries_[i].data;
        auto p=plugins::parse_manifest(data.value("definition",Json()).dump());
        if(!p || p.manifest->schema_version!=2) {
            status_->text=T(L"Anordnung braucht Manifest v2.",L"Arrangement requires manifest v2."); host_->relayout(); return;
        }
        const bool keep=preserve && !base_.id.empty() && base_.id==p.manifest->id && base_.definition==p.manifest->definition;
        auto previous=history_;
        base_=*p.manifest;
        auto a=plugins::parse_arrangement(data.value("overlay",Json()).dump());
        if(!a) { status_->text=T(L"Anordnung konnte nicht geladen werden.",L"Could not load arrangement."); host_->relayout(); return; }
        if(keep) history_=std::move(previous); else history_.reset(*a.arrangement);
        block_=0; uncommitted_=false;
        status_->text=data.value("state",std::string())=="stale"
            ? T(L"Anordnung passt nicht mehr zur Plugin-Version. Auf Manifest zurücksetzen.",L"Arrangement no longer matches the plugin version. Reset to manifest.")
            : T(L"Nur die Anordnung wird gespeichert. Inhalte und Freigaben bleiben erhalten.",L"Only arrangement is saved. Content and consent are preserved.");
        if(data.value("state",std::string())=="invalid") status_->text=T(L"Gespeicherte Anordnung ungültig. Manifest-Vorschlag prüfen oder zurücksetzen.",L"Saved arrangement is invalid. Review the manifest proposal or reset it.");
        editor(); sync(); clean(*body_); host_->relayout();
    }
    void accept(pe::Model next, std::string group={}) {
        if(history_.push(std::move(next),std::move(group),GetTickCount64())) { ++generation_; pending_=true; }
        uncommitted_=false; sync();
    }
    void invalid(TextField* field, const Text& message) {
        uncommitted_=true; field->invalid=true;
        auto value=field->text();
        auto it=reported_.find(field);
        if(it==reported_.end() || it->second!=value) { reported_[field]=value; host_->toast(message,ToastKind::Error); }
        update_buttons();
    }
    template<class F> void operation(F f, std::string group={}) {
        auto next=model(); if(f(next)) accept(std::move(next),std::move(group)); else reject();
    }
    void reject() {
        host_->toast(T(L"Raster: Aktion passt nicht. Grenzen, Überlappung und höchstens vier Blöcke je Zone prüfen.",
            L"Grid: action does not fit. Check limits, overlaps and at most four blocks per zone."),ToastKind::Error);
    }
    void undo() { if(history_.undo()) { ++generation_; pending_=true; uncommitted_=false; sync(); } }
    void redo() { if(history_.redo()) { ++generation_; pending_=true; uncommitted_=false; sync(); } }
    void text_pending() { if(!syncing_) { uncommitted_=true; ++generation_; pending_=false; update_buttons(); } }
    void editor() {
        body_->add<Note>(T(L"Tab wechselt das Feld. Raster: Pfeile = Zelle, Enter/Leertaste = wählen/anlegen, Umschalt+Pfeil = Größe, Alt+Pfeil = verschieben, Entf = löschen. Strg+Z/Y = Rückgängig/Wiederholen. Texte mit Enter oder Fokuswechsel abschließen.",
            L"Tab changes fields. Grid: arrows = cell, Enter/Space = select/create, Shift+arrow = resize, Alt+arrow = move, Delete = remove. Ctrl+Z/Y = undo/redo. Commit text with Enter or by leaving the field."));
        auto raster=body_->add<Card>(T(L"Raster",L"Grid"));
        grid_=raster->add<GridEditor>(); grid_->model=model();
        grid_->on_edit=[this](auto n,auto group) { accept(std::move(n),std::move(group)); };
        grid_->on_select=[this] { sync_zone(); host_->surface().invalidate(); };
        grid_->on_reject=[this] { reject(); };
        grid_->on_undo=[this] { undo(); }; grid_->on_redo=[this] { redo(); };
        for(int axis=0;axis<2;++axis) {
            auto buttons=raster->add<ButtonRow>();
            buttons->add(axis==0?T(L"Spalte +",L"Column +"):T(L"Zeile +",L"Row +"),Button::Secondary,[this,axis] { operation([=](auto& n) { return pe::resize_axis(n,axis==0,1); }); });
            buttons->add(axis==0?T(L"Spalte −",L"Column −"):T(L"Zeile −",L"Row −"),Button::Secondary,[this,axis] { operation([=](auto& n) { return pe::resize_axis(n,axis==0,-1); }); });
            for(int j=0;j<6;++j) {
                auto w=std::make_unique<Slider>(1,12,1); weights_[axis][j]=w.get(); w->page=1;
                weight_rows_[axis][j]=raster->add<Row>(Text((axis==0?L"Spaltenanteil ":L"Zeilenanteil ")+std::to_wstring(j+1),
                    (axis==0?L"Column weight ":L"Row weight ")+std::to_wstring(j+1)),Text(),std::move(w));
                weights_[axis][j]->on_change=[this,axis,j] { if(!syncing_) operation([=](auto& n) { return pe::weight(n,axis==0,j,weights_[axis][j]->val); },"weight:"+std::to_string(axis)+":"+std::to_string(j)); };
            }
        }
        auto zone=body_->add<Card>(T(L"Gewählte Zone",L"Selected zone"));
        zone_id_=zone->row<TextField>(T(L"Zonen-ID",L"Zone ID"),Text(),TextField::Single,L""); zone_id_->limit=64;
        zone_id_->on_change=[this] { text_pending(); }; zone_id_->on_commit=[this] { commit_zone(); };
        cycle_=zone->row<Segment>(T(L"Wechsel",L"Cycle"),Text(),std::vector<Text>{T(L"Aus",L"Off"),Text(L"rotate"),Text(L"first_available")},0);
        transition_=zone->row<Segment>(T(L"Übergang",L"Transition"),Text(),std::vector<Text>{Text(L"fade"),Text(L"none")},0);
        cycle_->on_change=transition_->on_change=[this] { commit_zone(); };
        auto split=zone->add<ButtonRow>();
        split->add(T(L"Horizontal teilen",L"Split horizontally"),Button::Secondary,[this] { operation([this](auto& n) { return pe::split_zone(n,grid_->selected,true); }); });
        split->add(T(L"Vertikal teilen",L"Split vertically"),Button::Secondary,[this] { operation([this](auto& n) { return pe::split_zone(n,grid_->selected,false); }); });
        auto bc=body_->add<Card>(T(L"Blöcke",L"Blocks"),T(L"Haken = gezeigt. Reihenfolge mit Alt+Pfeil oder ▲/▼.",L"Check = shown. Reorder with Alt+arrow or ▲/▼."));
        blocks_=bc->add<LayoutBlockList>(std::vector<ItemList::Item>{}); blocks_->name=T(L"Blockliste",L"Block list");
        blocks_->on_undo=[this] { undo(); }; blocks_->on_redo=[this] { redo(); };
        blocks_->on_select=[this](int b) { block_=b; sync_block(); host_->relayout(); };
        blocks_->on_change=[this] {
            auto n=model(); auto reordered=Json::array();
            for(auto& item:blocks_->items) for(auto b:n.blocks) if(b["id"]==item.key) { b["shown"]=item.enabled; reordered.push_back(b); break; }
            n.blocks=std::move(reordered); block_=blocks_->selected; accept(std::move(n),blocks_->history_group);
        };
        block_zone_=bc->row<Combo>(T(L"Zone des Blocks",L"Block zone"),Text(),std::vector<Text>{},0);
        block_zone_->on_change=[this] { commit_block(); };
        seconds_=bc->row<TextField>(T(L"Sekunden (rotate, 3–600)",L"Seconds (rotate, 3–600)"),Text(),TextField::Number,L"");
        seconds_->on_change=[this] { text_pending(); }; seconds_->on_commit=[this] { commit_block(); };
        build_paths(); std::vector<Text> paths; for(auto& p:paths_) paths.push_back(p.empty()?T(L"Immer",L"Always"):Text(widen(p)));
        path_=bc->row<Combo>(T(L"Anzeigen, wenn",L"Show if"),Text(),paths,0);
        path_->on_change=[this] { commit_block(); };
        op_=bc->row<Segment>(T(L"Vergleich",L"Comparison"),Text(),std::vector<Text>{T(L"Vorhanden",L"Present"),Text(L"=="),Text(L"!="),Text(L"<"),Text(L"<="),Text(L">"),Text(L">=")},0);
        type_=bc->row<Segment>(T(L"Werttyp",L"Value type"),Text(),std::vector<Text>{T(L"Text",L"Text"),T(L"Zahl",L"Number"),Text(L"true / false")},0);
        literal_=bc->row<TextField>(T(L"Vergleichswert",L"Comparison value"),Text(),TextField::Single,L""); literal_->limit=256;
        literal_->on_change=[this] { text_pending(); }; literal_->on_commit=[this] { commit_block(); };
        // Werttyp/Operator ändern nur den Entwurf, damit ein neuer Wert zuerst eingegeben werden kann.
        op_->on_change=type_->on_change=[this] { text_pending(); };
        bc->add<ButtonRow>()->add(T(L"Bedingung bestätigen",L"Confirm condition"),Button::Secondary,[this] { commit_block(); });
        auto pv=body_->add<Card>(T(L"Live-Vorschau",L"Live preview"),T(L"Beispieldaten aus dem Plugin, derselbe Seitenrenderer wie auf dem Display.",L"Plugin sample data, the same page renderer as on the display."));
        auto orientation=pv->row<Segment>(T(L"Ausrichtung",L"Orientation"),Text(),std::vector<Text>{T(L"Quer",L"Landscape"),T(L"Hoch",L"Portrait")},portrait_?1:0);
        orientation->on_change=[this,orientation] { portrait_=orientation->index==1; ++generation_; pending_=true; };
        validation_=body_->add<Label>(Text());
        error_=body_->add<ButtonRow>()->add(T(L"Fehlerstelle auswählen",L"Select invalid field"),Button::Secondary,[this] {
            if(issue_block_>=0) { block_=issue_block_; blocks_->selected=block_; sync_block(); host_->relayout(); host_->surface().focus(block_zone_,true); }
            else { if(issue_zone_>=0) grid_->selected=issue_zone_; sync_zone(); host_->surface().focus(grid_,true); }
        });
        auto buttons=body_->add<ButtonRow>();
        undo_=buttons->add(T(L"Rückgängig",L"Undo"),Button::Secondary,[this] { undo(); });
        redo_=buttons->add(T(L"Wiederholen",L"Redo"),Button::Secondary,[this] { redo(); });
        apply_=buttons->add(T(L"Übernehmen",L"Apply"),Button::Primary,[this] { apply(false); });
        reset_=buttons->add(T(L"Auf Manifest zurücksetzen",L"Reset to manifest"),Button::Danger,[this] { apply(true); });
        pending_=true;
    }
    void build_paths() {
        paths_={""};
        auto add=[this](const std::string& p) { if(std::find(paths_.begin(),paths_.end(),p)==paths_.end()) paths_.push_back(p); };
        std::function<void(const Json&,std::string,int)> walk=[&](const Json& j,std::string p,int depth) {
            if(paths_.size()>=256 || p.size()>128 || depth>8) return; add(p);
            if(j.is_object()) for(auto it=j.begin();it!=j.end();++it) walk(it.value(),p+"."+it.key(),depth+1);
        };
        for(auto& s:base_.sources) walk(s.sample,s.id,1);
        for(auto& s:base_.settings) if(s.type!="secret") add("settings."+s.key);
        for(auto& w:base_.widgets) if(w.show_if) add(w.show_if->path);
    }
    void sync_zone() {
        if(!grid_) return; syncing_=true;
        int z=grid_->selected;
        bool selected=z>=0 && z<int(model().layout["zones"].size());
        zone_id_->enabled=cycle_->enabled=transition_->enabled=selected;
        if(selected) {
            auto& j=model().layout["zones"][z]; zone_id_->set_text(widen(j["id"].get<std::string>()));
            cycle_->index=!j.contains("cycle")?0:j["cycle"]["mode"]=="rotate"?1:2;
            transition_->index=j.contains("cycle") && j["cycle"].value("transition","fade")=="none"?1:0;
        }
        syncing_=false;
    }
    void sync_block() {
        if(!blocks_) return;
        const bool selected=block_>=0 && block_<int(model().blocks.size());
        block_zone_->enabled=path_->enabled=op_->enabled=type_->enabled=literal_->enabled=selected;
        if(!selected) { seconds_->enabled=false; return; }
        syncing_=true; auto& b=model().blocks[block_]; auto& zs=model().layout["zones"];
        block_zone_->options.clear(); block_zone_->index=0;
        for(size_t i=0;i<zs.size();++i) { block_zone_->options.push_back(Text(widen(zs[i]["id"].get<std::string>()))); if(zs[i]["id"]==b["zone"]) block_zone_->index=int(i); }
        auto& z=zs[block_zone_->index]; seconds_->enabled=z.contains("cycle") && z["cycle"]["mode"]=="rotate";
        seconds_->set_text(b.contains("seconds")?std::to_wstring(b["seconds"].get<int>()):L"");
        path_->index=0; op_->index=0; type_->index=0; literal_->set_text(L"");
        if(b.contains("show_if")) {
            auto& c=b["show_if"]; auto it=std::find(paths_.begin(),paths_.end(),c["path"].get<std::string>());
            if(it!=paths_.end()) path_->index=int(it-paths_.begin());
            const std::vector<std::string> ops{"","==","!=","<","<=",">",">="};
            auto o=std::find(ops.begin(),ops.end(),c.value("op",std::string())); op_->index=o==ops.end()?0:int(o-ops.begin());
            if(c.contains("value")) { auto& v=c["value"]; type_->index=v.is_boolean()?2:v.is_number()?1:0; literal_->set_text(widen(v.is_string()?v.get<std::string>():v.dump())); }
        }
        syncing_=false;
    }
    void sync() {
        if(!grid_) return; syncing_=true;
        grid_->model=model(); grid_->selected=std::clamp(grid_->selected,-1,int(model().layout["zones"].size())-1);
        auto l=pe::geometry(model());
        if(l) { grid_->column=std::clamp(grid_->column,0,int(l->columns.size())-1); grid_->row=std::clamp(grid_->row,0,int(l->rows.size())-1); }
        for(int a=0;a<2;++a) for(int i=0;i<6;++i) {
            auto& axis=model().layout[a==0?"columns":"rows"];
            weight_rows_[a][i]->visible=i<int(axis.size()); if(i<int(axis.size())) weights_[a][i]->val=axis[i];
        }
        blocks_->items.clear(); for(auto& b:model().blocks) blocks_->items.push_back({b["id"],Text(widen(b["id"].get<std::string>())),Text(widen(b["zone"].get<std::string>())),b["shown"]});
        block_=model().blocks.empty()?-1:std::clamp(block_,0,int(model().blocks.size())-1); blocks_->selected=block_;
        auto result=plugins::merge_arrangement(base_,model()); valid_=bool(result);
        issue_zone_=issue_block_=-1;
        const auto& zs=model().layout["zones"];
        if(!valid_) for(size_t z=0;z<zs.size();++z) {
            int count=0; for(auto& b:model().blocks) if(b["zone"]==zs[z]["id"] && b.value("shown",true)) ++count;
            if((result.error=="Every zone needs at least one block." && count==0) || (result.error=="At most 4 blocks per zone allowed." && count>4)) { issue_zone_=int(z); break; }
        }
        if(!valid_ && issue_zone_<0) issue_block_=block_;
        validation_->text=valid_?T(L"Anordnung gültig.",L"Arrangement valid."):Text(widen(pe::diagnostic(result.error,false)),widen(pe::diagnostic(result.error,true)));
        if(!valid_ && (issue_zone_>=0 || issue_block_>=0)) {
            std::wstring where=issue_zone_>=0?L"Zone "+widen(zs[issue_zone_]["id"].get<std::string>()):L"Block "+widen(model().blocks[issue_block_]["id"].get<std::string>());
            validation_->text.de=where+L": "+validation_->text.de; validation_->text.en=where+L": "+validation_->text.en;
        }
        validation_->tone=Label::Normal;
        reported_.clear(); zone_id_->invalid=seconds_->invalid=literal_->invalid=false;
        syncing_=false; sync_zone(); sync_block();
        host_->surface().notify(grid_,EVENT_OBJECT_VALUECHANGE); update_buttons(); host_->relayout();
    }
    void commit_zone() {
        if(syncing_ || !visible_ || !grid_ || grid_->selected<0) return;
        auto n=model(); auto& z=n.layout["zones"][grid_->selected];
        auto old=z["id"]; z["id"]=narrow(zone_id_->text()); for(auto& b:n.blocks) if(b["zone"]==old) b["zone"]=z["id"];
        if(cycle_->index==0) z.erase("cycle");
        else z["cycle"]={{"mode",cycle_->index==1?"rotate":"first_available"},{"transition",transition_->index==0?"fade":"none"}};
        if(cycle_->index!=1) for(auto& b:n.blocks) if(b["zone"]==z["id"]) b.erase("seconds");
        if(!pe::geometry(n) || !plugins::parse_arrangement(plugins::to_json(n).dump())) {
            invalid(zone_id_,T(L"Zone: ID ungültig oder bereits vergeben.",L"Zone: ID is invalid or already used.")); return;
        }
        accept(std::move(n));
    }
    void commit_block() {
        if(syncing_ || !visible_ || !blocks_ || block_<0 || block_>=int(model().blocks.size())) return;
        auto n=model(); auto& b=n.blocks[block_];
        b["zone"]=n.layout["zones"][block_zone_->index]["id"];
        auto& z=n.layout["zones"][block_zone_->index]; bool rotate=z.contains("cycle") && z["cycle"]["mode"]=="rotate";
        TextField* invalid_field=seconds_;
        try {
            b.erase("seconds"); auto seconds=narrow(seconds_->text());
            if(rotate && !seconds.empty()) { size_t used=0; int v=std::stoi(seconds,&used); if(used!=seconds.size() || v<3 || v>600) throw std::runtime_error("seconds"); b["seconds"]=v; }
            invalid_field=literal_;
            b.erase("show_if");
            if(path_->index>0) {
                Json c={{"path",paths_.at(path_->index)}};
                if(op_->index>0) {
                    const std::vector<std::string> ops{"","==","!=","<","<=",">",">="}; c["op"]=ops.at(op_->index);
                    auto text=narrow(literal_->text()); Json value=text;
                    if(type_->index==1) { value=Json::parse(text); if(!value.is_number()) throw std::runtime_error("number"); }
                    if(type_->index==2) { if(text!="true" && text!="false") throw std::runtime_error("boolean"); value=text=="true"; }
                    c["value"]=value;
                }
                b["show_if"]=c;
            }
            // Auch ungültige semantische Entwürfe behalten, damit leere Zonen gefüllt werden können.
            if(!plugins::parse_arrangement(plugins::to_json(n).dump())) throw std::runtime_error("block");
        } catch(const std::exception&) {
            invalid(invalid_field,T(L"Block: Sekunden oder Vergleichswert ungültig.",L"Block: invalid seconds or comparison value.")); return;
        }
        accept(std::move(n));
    }
    void apply(bool reset) {
        if(host_->busy() || chosen_<0 || (!reset && (!valid_ || uncommitted_))) return;
        if(reset && !host_->confirm(T(L"Anordnung und Backup löschen und die Manifest-Anordnung wiederherstellen?",L"Remove the arrangement and its backup and restore the manifest layout?"))) return;
        auto h=host_; auto id=entries_[chosen_].id; auto overlay=plugins::to_json(model()); ++generation_; pending_=false;
        const auto serial=generation_;
        h->run([this,h,id,overlay,reset,serial]() -> std::function<void()> {
            if(reset) h->plugins().arrangement_reset(id); else h->plugins().arrangement_apply(id,overlay);
            auto data=h->plugins().arrangement_get(id);
            return [this,h,id,data,serial] {
                for(auto& e:entries_) if(e.id==id) e.data=data;
                if(chosen_>=0 && entries_[chosen_].id==id && grid_ && serial==generation_) {
                    auto parsed=plugins::parse_arrangement(data.value("overlay",Json()).dump());
                    if(parsed) {
                        history_.reset(*parsed.arrangement); uncommitted_=false; pending_=true;
                        status_->text=T(L"Nur die Anordnung wird gespeichert. Inhalte und Freigaben bleiben erhalten.",L"Only arrangement is saved. Content and consent are preserved.");
                        sync();
                    }
                }
                h->toast(T(L"Anordnung übernommen.",L"Arrangement applied."),ToastKind::Success);
            };
        }); update_buttons();
        // Nur die explizite Speicheraktion setzt den Fokus um, niemals on_commit/KILLFOCUS.
        auto focused=h->surface().focused();
        if(focused && !focused->enabled && (focused==apply_ || focused==reset_)) h->surface().focus(grid_,true);
    }
    void preview() {
        pending_=false;
        if(!visible_ || !grid_ || !valid_ || uncommitted_) return;
        auto merged=plugins::merge_arrangement(base_,model()); if(!merged) return;
        const auto serial=generation_; auto h=host_; auto json=merged.manifest->definition.dump();
        auto size=portrait_?std::pair{320,480}:std::pair{480,320}; previewing_=true;
        h->run([this,h,json,size,serial]() -> std::function<void()> {
            std::string error; std::optional<Image> image;
            try { image=h->app().plugin_preview(json,error,size,true); } catch(const std::exception&) { error="preview"; }
            return [this,h,image=std::move(image),serial] {
                previewing_=false;
                if(visible_ && serial==generation_ && image) h->preview(image);
            };
        });
    }
};
}
std::unique_ptr<Page> make_layout_page() { return std::make_unique<LayoutPage>(); }
}
