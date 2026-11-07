// © 2026 FuchsLabs – Anordnung installierter v2-Plugins. Keine Manifest-/Konfigurationsschreibwege.
#include "ui_pages.h"
#include "custom_view_editor.h"
#include "ui_layout_grid.h"
#include "ui_editor.h"
#include "ui_tile_palette.h"
#include "native_settings.h"
#include <cwctype>
#include "app.h"
#include "util.h"
#include <algorithm>
#include <oleacc.h>

namespace fl::ui {
namespace {
namespace pe = plugins::edit;
using plugins::Json;
class DisplayTab : public Button {
public:
    bool selected;
    DisplayTab(Text text,bool active,std::function<void()> action):Button(std::move(text),active?Primary:Secondary,std::move(action)),selected(active) {}
    Role role() const override {return Role::NavTab;}
    bool checked() const override {return selected;}
};
// Filter-Chip der Kachelpalette: flache Plakette, Auswahl wie ein Reiter.
class FilterChip : public Button {
public:
    bool selected;
    FilterChip(Text text,bool active,std::function<void()> action):Button(std::move(text),Secondary,std::move(action)),selected(active) {}
    Role role() const override {return Role::NavTab;}
    bool checked() const override {return selected;}
    float pref_width(Ctx& c) override {return c.text_width(c.s(text),Font::Small)+26;}
    float height(Ctx&,float) override {return 28;}
    void paint(Ctx& c) override {
        if(selected) { c.fill(rc,c.th.accent,rc.bottom-rc.top); c.text(c.s(text),rc,Font::Small,c.th.on_accent,DWRITE_TEXT_ALIGNMENT_CENTER); }
        else {
            c.frame(rc,c.th.line,rc.bottom-rc.top);
            c.text(c.s(text),rc,Font::Small,c.th.text,DWRITE_TEXT_ALIGNMENT_CENTER);
            if(hot&&enabled) { auto f=c.th.text; f.a=.08f; c.fill(rc,f,rc.bottom-rc.top); }
        }
    }
};
std::wstring lowered(std::wstring s) { for(auto& c:s) c=wchar_t(std::towlower(c)); return s; }
class LayoutPage : public Page {
    std::string initial_;
public:
    explicit LayoutPage(std::string initial):initial_(std::move(initial)) {}
    PageInfo info() const override {
        return {"layout", Group::App, T(L"Seite anordnen",L"Arrange page"),
            T(L"Blöcke im Raster anordnen · Vorschau mit Beispieldaten",L"Arrange blocks in a grid · preview with sample data")};
    }
    void build(Host& host, Column& root) override {
        host_=&host;
        // Nach „Verwerfen“ können alle bisherigen Widgets bereits zerstört sein.
        grid_=nullptr; blocks_=nullptr; zone_id_=seconds_=literal_=nullptr;
        apply_=undo_=redo_=reset_=error_=nullptr; validation_=nullptr; shell_=nullptr; pages_list_=nullptr; size_chip_=nullptr; intro_=nullptr; selection_=nullptr;
        intro_=root.add<Card>(T(L"In drei Schritten",L"In three steps"),T(L"So ordnest du eine Seite an.",L"How to arrange a page."));
        intro_->add<Label>(T(L"1   Kachel hinzufügen: aus der Palette auf die Fläche ziehen oder anklicken.",L"1   Add a tile: drag it from the palette onto the canvas, or click it."));
        intro_->add<Label>(T(L"2   Kachel anklicken und rechts einstellen.",L"2   Click a tile and adjust it on the right."));
        intro_->add<Label>(T(L"3   Übernehmen: speichert die Anordnung und zeigt sie auf dem Display.",L"3   Apply: saves the arrangement and shows it on the display."));
        intro_->add<ButtonRow>()->add(T(L"Verstanden",L"Got it"),Button::Secondary,[this] { dismiss_intro(); });
        intro_->visible=false;  // erst nach dem Laden (gemerkt?) sichtbar
        tabs_=root.add<ButtonRow>();
        rebuild_tabs();
        // Kopfleiste: Seitenauswahl, Gerätegröße, Rückgängig/Wiederholen/Übernehmen/Zurücksetzen (Letztere je Seite neu, siehe editor()).
        header_=root.add<Flow>();
        choice_=header_->add<Combo>(std::vector<Text>{},-1);
        choice_->name=host.plugins().builtin_layout_get?T(L"Seite",L"Page"):T(L"Aktiviertes Plugin",L"Enabled plugin");
        choice_->width=240;
        size_chip_=header_->add<Chip>(T(L"480 × 320",L"480 × 320"));
        choice_->on_change=[this] { choose(choice_->index); };
        status_=root.add<Label>(T(L"Wird geladen …",L"Loading …"));
        body_=root.add<Column>();
        if (!entries_.empty()) { fill_choices(); choose(std::clamp(chosen_,0,int(entries_.size())-1),true); }
        clean(root);
    }
    void shown(Host&) override { visible_=true; reload_=true; tick(*host_,host_->status()); }
    void hidden(Host& h) override { if(grid_) grid_->cancel(h.surface()); visible_=false; ++generation_; h.preview(std::nullopt); }
    void tick(Host&,const Status&) override {
        if (!visible_) return;
        if (host_->busy()) { update_buttons(); return; }
        if (reload_) { load(); return; }
        if(GetTickCount64()-display_poll_>2000 && host_->plugins().layout_displays) {
            display_poll_=GetTickCount64();auto h=host_;
            h->run([this,h]() -> std::function<void()> {auto displays=custom::active_displays(h->plugins().layout_displays());return [this,displays] {
                if(displays==displays_)return;displays_=displays;
                if(!device_.empty() && std::none_of(displays_.begin(),displays_.end(),[&](const auto& d){return d.value("id",std::string())==device_;})) {
                    if(grid_)grid_->cancel(host_->surface());device_.clear();history_=pe::EditHistory{};other_history_=pe::EditHistory{};uncommitted_=false;switch_pending_=true;reload_=true;
                    host_->toast(T(L"Display getrennt: Standard-Tab gewählt. Gespeicherte Anordnung bleibt erhalten.",L"Display disconnected: selected standard tab. Saved layout retained."));
                }rebuild_tabs();host_->relayout();
            };});return;
        }
        if (pending_ && !previewing_ && (!grid_ || !grid_->carrying())) preview();
        update_buttons();
    }
private:
    struct Entry { std::string id; Text name; Json data; bool builtin=false; std::map<std::string,bool> inherited; };
    Host* host_=nullptr;
    Column* body_=nullptr;
    ButtonRow* tabs_=nullptr;
    Flow* header_=nullptr;
    Chip* size_chip_=nullptr;
    Card* intro_=nullptr;
    Label* selection_=nullptr;
    bool intro_seen_=false;
    EditorShell* shell_=nullptr;
    ItemList* pages_list_=nullptr;
    TextField* search_=nullptr;
    Flow* chips_=nullptr;
    TilePalette* palette_=nullptr;
    std::vector<FilterChip*> chip_tabs_;
    std::vector<std::string> chip_ids_;
    std::map<std::string,TilePalette::Thumb> thumbs_;  // Mini-Vorschauen je Seite, Kachel und Anzeige-Theme
    std::string category_;
    bool tiles_loading_=false;
    bool section_open_[3]{}, section_touched_[3]{};
    float chrome_aspect_=1.5f;
    uint64_t thumb_version_=0;
    Note* inheritance_note_=nullptr;
    Json displays_=Json::array();
    std::string device_;
    bool switch_pending_=false;
    uint64_t display_poll_=GetTickCount64();
    Json read_layout(bool built,const std::string& id) {
        auto& a=host_->plugins();
        return a.device_layout_get?a.device_layout_get(built,id,device_):built?a.builtin_layout_get(id):a.arrangement_get(id);
    }
    bool unsaved() const {return history_.can_undo() || (catalog_ && other_history_.can_undo()) || uncommitted_;}
    Json display() const {for(const auto& d:displays_) if(d.value("id",std::string())==device_) return d;return Json::object();}
    void switch_device(const std::string& id) {
        if(id==device_ || host_->busy()) return;
        if(unsaved() && !host_->confirm(T(L"Ungesicherte Änderungen verwerfen und das Display wechseln?",L"Discard unsaved changes and switch display?"))) return;
        if(grid_) grid_->cancel(host_->surface());
        if(zone_id_) zone_id_->on_commit={};if(seconds_) seconds_->on_commit={};if(literal_) literal_->on_commit={};
        host_->surface().focus(choice_,true);body_->visible=false;host_->relayout();
        device_=id;switch_pending_=true;portrait_=display().value("height",320)>display().value("width",480);
        history_=pe::EditHistory{};other_history_=pe::EditHistory{};uncommitted_=false;
        ++generation_;pending_=false;reload_=true;rebuild_tabs();
    }
    void rebuild_tabs() {
        if(!tabs_) return;
        tabs_->buttons.clear();
        auto add=[this](Text label,const std::string& id) {tabs_->buttons.push_back(std::make_unique<DisplayTab>(std::move(label),device_==id,[this,id]{switch_device(id);}));};
        add(T(L"Standard für alle Displays",L"Standard for all displays"),"");
        for(const auto& d:displays_) {
            auto id=d.value("id",std::string());auto name=widen(d.value("name",id));
            auto w=d.value("width",480),h=d.value("height",320);
            auto suffix=L" · "+std::to_wstring(w)+L" × "+std::to_wstring(h);
            Text label(name+suffix+(h>w?L" · Hoch":L" · Quer")+(d.value("online",false)?L"":L" · offline"),
                name+suffix+(h>w?L" · Portrait":L" · Landscape")+(d.value("online",false)?L"":L" · offline"));
            if(chosen_>=0 && chosen_<int(entries_.size())) {
                const auto& states=entries_[chosen_].inherited;auto it=states.find(id);
                if(it!=states.end()) {label.de+=it->second?L" · erbt Standard":L" · eigene Anordnung";label.en+=it->second?L" · inherits standard":L" · own arrangement";}
            }
            add(label,id);
        }
        clean(*tabs_);
    }
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
    pe::EditHistory history_, other_history_;
    const builtin::Catalog* catalog_=nullptr;
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
        apply_->enabled=idle && valid_ && !uncommitted_ && (catalog_?bool(host_->plugins().builtin_layout_apply):bool(host_->plugins().arrangement_apply));
        reset_->enabled=idle && (catalog_?bool(host_->plugins().builtin_layout_reset):bool(host_->plugins().arrangement_reset));
        undo_->enabled=history_.can_undo(); redo_->enabled=history_.can_redo();
        if(error_) error_->enabled=!valid_;
        choice_->enabled=idle;
        host_->surface().invalidate();
    }
    void load() {
        auto h=host_; reload_=false;
        if ((!h->plugins().list || !h->plugins().arrangement_get) && !h->plugins().builtin_layout_get) {
            status_->text=T(L"Anordnung ist in diesem Build nicht angebunden.",L"Arrangement is not connected in this build."); return;
        }
        const bool seen_before=intro_seen_;
        h->run([this,h,seen_before]() -> std::function<void()> {
            const bool seen=seen_before || (h->plugins().editor_intro_seen && h->plugins().editor_intro_seen());
            std::vector<Entry> entries;
            auto displays=h->plugins().layout_displays?custom::active_displays(h->plugins().layout_displays()):Json::array();
            if(h->plugins().list && h->plugins().arrangement_get)
                for(auto& e:h->plugins().list()) if(e.enabled) entries.push_back({e.id,Text(widen(e.name)),read_layout(false,e.id)});
            if(h->plugins().builtin_layout_get) for(const auto& c:builtin::catalogs()) if(c.enabled)
                entries.push_back({c.key,Text(widen(c.label.de),widen(c.label.en)),read_layout(true,c.key),true});
            if(h->plugins().device_layout_get) for(auto& entry:entries) for(const auto& d:displays) {
                auto id=d.value("id",std::string());
                entry.inherited[id]=h->plugins().device_layout_get(entry.builtin,entry.id,id).value("inherited",true);
            }
            return [this,seen,entries=std::move(entries),displays=std::move(displays)] {
                intro_seen_=seen; if(intro_) intro_->visible=!seen;
                displays_=displays;if(display().value("tv",false)) portrait_=false;rebuild_tabs();
                std::string selected=chosen_>=0 && chosen_<int(entries_.size()) ? (entries_[chosen_].builtin?"builtin:":"plugin:")+entries_[chosen_].id : "";
                if(!initial_.empty()) {selected=std::move(initial_);initial_.clear();}
                entries_=entries; chosen_=0;
                for(size_t i=0;i<entries_.size();++i) if((entries_[i].builtin?"builtin:":"plugin:")+entries_[i].id==selected) chosen_=int(i);
                const bool preserve=!switch_pending_;switch_pending_=false;
                fill_choices(); choose(entries_.empty()?-1:chosen_,preserve);
            };
        });
    }
    void fill_choices() { choice_->options.clear(); for(auto& e:entries_) choice_->options.push_back(Text((e.builtin?L"Eingebaut · ":L"Plugins · ")+e.name.de,(e.builtin?L"Built-in · ":L"Plugins · ")+e.name.en)); choice_->index=chosen_; }
    void choose(int i, bool preserve=false) {
        if(!preserve && i!=chosen_ && (history_.can_undo() || (catalog_ && other_history_.can_undo()) || uncommitted_) &&
            !host_->confirm(T(L"Ungesicherte Änderungen verwerfen und die Seite wechseln?",L"Discard unsaved changes and switch page?"))) {
            choice_->index=chosen_; if(pages_list_) pages_list_->selected=chosen_; host_->surface().invalidate(); return;
        }
        reported_.clear();
        for(Widget* w:{static_cast<Widget*>(undo_),static_cast<Widget*>(redo_),static_cast<Widget*>(apply_),static_cast<Widget*>(reset_)}) if(w&&header_) header_->remove(w);
        shell_=nullptr; pages_list_=nullptr; search_=nullptr; chips_=nullptr; palette_=nullptr; chip_tabs_.clear(); chip_ids_.clear(); selection_=nullptr;
        ++generation_; pending_=false; chosen_=i; choice_->index=i;rebuild_tabs();
        // Alte Eingabefelder nicht während ihres Abbaus erneut abschließen.
        if(zone_id_) zone_id_->on_commit={}; if(seconds_) seconds_->on_commit={}; if(literal_) literal_->on_commit={};
        body_->clear(); body_->visible=true;inheritance_note_=nullptr;grid_=nullptr; blocks_=nullptr; apply_=undo_=redo_=reset_=error_=nullptr;
        zone_id_=seconds_=literal_=nullptr; validation_=nullptr;
        if(i<0 || i>=int(entries_.size())) {
            catalog_=nullptr; status_->text=T(L"Keine Seite verfügbar.",L"No page available."); host_->relayout(); return;
        }
        const auto& data=entries_[i].data;
        const auto* previous_catalog=catalog_;
        catalog_=entries_[i].builtin?builtin::find(entries_[i].id):nullptr;
        if(catalog_) {
            auto parsed=builtin::parse(*catalog_,data.value("overlay",Json()).dump());
            if(!parsed) { status_->text=T(L"Anordnung konnte nicht geladen werden.",L"Could not load arrangement.");host_->relayout();return; }
            if(!preserve || previous_catalog!=catalog_) {
                history_.reset(portrait_?parsed.overlay->portrait.value_or(parsed.overlay->model):parsed.overlay->model);
                other_history_.reset(portrait_?parsed.overlay->model:parsed.overlay->portrait.value_or(parsed.overlay->model));
            }
            base_={};block_=0;uncommitted_=false;
            status_->text=T(L"Schematisches Raster. Standard bleibt pixelgleich; eigene Anordnung – Abstände folgen dem Raster.",
                            L"Schematic grid. Standard stays pixel-identical; custom arrangement – spacing follows the grid.");
            if(data.value("state",std::string())=="stale") status_->text=T(L"Anordnung passt nicht mehr zum Katalog. Auf Standard zurücksetzen.",L"Arrangement no longer matches the catalog. Reset to standard.");
            if(data.value("state",std::string())=="invalid") status_->text=T(L"Gespeicherte Anordnung ungültig. Standard aktiv; Zurücksetzen entfernt die fehlerhafte Datei.",L"Saved arrangement is invalid. Standard active; reset removes the invalid file.");
            editor();sync();clean(*body_);clean(*header_);host_->relayout();request_tiles();return;
        }
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
        editor(); sync(); clean(*body_);clean(*header_); host_->relayout(); request_tiles();
    }
    builtin::Overlay builtin_overlay() const {
        return {catalog_->version,portrait_?other_history_.current():model(),
            portrait_?model():other_history_.current()};
    }
    void accept(pe::Model next, std::string group={}) {
        if(catalog_) for(const auto& spec:catalog_->blocks) if(spec.required &&
            std::none_of(next.blocks.begin(),next.blocks.end(),[&](const auto& b){return b["id"]==spec.id && b.value("shown",false);})) {
            host_->toast(T(L"Pflichtblock kann nicht entfernt werden.",L"Required block cannot be removed."),ToastKind::Error);sync();return;
        }
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
        shell_=body_->add<EditorShell>();
        for(int i=0;i<3;++i) if(section_touched_[i]) shell_->set_open(EditorShell::Section(i),section_open_[i]);
        shell_->on_toggle=[this] {  // Auf-/Zuklappen überlebt den Neuaufbau beim Seitenwechsel
            for(int i=0;i<3;++i) if(shell_->touched(EditorShell::Section(i))) { section_touched_[i]=true; section_open_[i]=shell_->is_open(EditorShell::Section(i)); }
            host_->relayout();
        };
        // Kopfleiste: Rückgängig, Wiederholen, Übernehmen, Zurücksetzen (Seitenauswahl und Größe leben in build()).
        undo_=header_->add<Button>(T(L"Rückgängig",L"Undo"),Button::Secondary,[this] { undo(); });
        redo_=header_->add<Button>(T(L"Wiederholen",L"Redo"),Button::Secondary,[this] { redo(); });
        apply_=header_->add<Button>(T(L"Übernehmen",L"Apply"),Button::Primary,[this] { apply(false); });
        reset_=header_->add<Button>(!device_.empty()?T(L"Von Standard erben",L"Inherit from standard"):catalog_?T(L"Auf Standard zurücksetzen",L"Reset to standard"):T(L"Auf Manifest zurücksetzen",L"Reset to manifest"),Button::Danger,[this] { apply(true); });
        // Linke Bereiche: Kacheln, Ebenen, Seiten.
        auto& palette=*shell_->section[EditorShell::Palette];
        search_=palette.add<TextField>(TextField::Single,L""); search_->limit=64;
        search_->hint=T(L"Kacheln suchen",L"Search tiles"); search_->name=T(L"Kacheln suchen",L"Search tiles");
        search_->on_change=[this] { if(palette_) { palette_->query=lowered(search_->text()); palette_->current=0; host_->relayout(); } };
        chips_=palette.add<Flow>(); chips_->gap_x=6; chips_->gap_y=6; chip_tabs_.clear(); chip_ids_.clear();
        const std::pair<const char*,Text> categories[]={{"",T(L"Alle",L"All")},{"system",T(L"System",L"System")},{"time",T(L"Zeit",L"Time")},{"media",T(L"Medien",L"Media")},{"games",T(L"Spiele",L"Games")},{"other",T(L"Sonstiges",L"Other")}};
        for(const auto& [id,label]:categories) {
            const std::string key=id; auto chip=chips_->add<FilterChip>(label,key==category_,[this,key] { select_category(key); });
            chip->name=label; chip_tabs_.push_back(chip); chip_ids_.push_back(key);
        }
        palette_=palette.add<TilePalette>(); palette_->surface=&host_->surface(); palette_->category=category_;
        palette_->thumb_of=[this](const std::string& id) -> const TilePalette::Thumb* { auto it=thumbs_.find(thumb_key(id)); return it==thumbs_.end()?nullptr:&it->second; };
        auto& layers=*shell_->section[EditorShell::Layers];
        blocks_=layers.add<LayoutBlockList>(std::vector<ItemList::Item>{}); blocks_->name=T(L"Ebenenliste",L"Layer list");
        blocks_->on_undo=[this] { undo(); }; blocks_->on_redo=[this] { redo(); };
        blocks_->on_select=[this](int b) {  // Auswahl in der Ebenenliste wählt die Kachel auch in der Fläche
            block_=b; sync_block();
            if(grid_ && b>=0 && grid_->shown_block(size_t(b))) grid_->select_block(size_t(b),host_->surface());
            host_->relayout();
        };
        blocks_->on_change=[this] {
            auto n=model(); auto reordered=Json::array();
            for(auto& item:blocks_->items) for(auto b:n.blocks) if(b["id"]==item.key) { b["shown"]=item.enabled; reordered.push_back(b); break; }
            n.blocks=std::move(reordered); block_=blocks_->selected; accept(std::move(n),blocks_->history_group);
        };
        layers.add<Note>(T(L"Haken = gezeigt. Reihenfolge mit Alt+Pfeil oder ▲/▼.",L"Check = shown. Reorder with Alt+arrow or ▲/▼."));
        auto& pages=*shell_->section[EditorShell::PageList];
        pages_list_=pages.add<ItemList>(std::vector<ItemList::Item>{}); pages_list_->name=T(L"Seitenliste",L"Page list");
        pages_list_->checkable=false; pages_list_->reorder=false; pages_list_->track_dirty=false;
        pages_list_->on_select=[this](int i) { if(i!=chosen_) choose(i); };
        // Mitte: Fläche.
        auto& center=shell_->center;
        center.title=T(L"Fläche",L"Canvas");
        if(!device_.empty()) {
            const bool inherited=entries_[chosen_].data.value("inherited",true);
            inheritance_note_=center.add<Note>(inherited?T(L"Erbt von Standard. Übernehmen legt eine eigene Anordnung an.",L"Inherits from standard. Apply creates an own arrangement."):T(L"Eigene Anordnung. Zurücksetzen aktiviert die Vererbung vom Standard.",L"Own arrangement. Reset restores inheritance from standard."));
            auto d=display();
            if(device_!="network" && !device_.starts_with("serial:") && !device_.starts_with("monitor:") && host_->plugins().layout_device_tv) {
                auto tv=center.add<ButtonRow>();
                tv->add(d.value("tv",false)?T(L"Android-TV: nur Querformat (ändern)",L"Android TV: landscape only (change)"):T(L"Als Android-TV markieren",L"Mark as Android TV"),Button::Secondary,[this] {
                    if(unsaved() && !host_->confirm(T(L"Ungesicherte Änderungen verwerfen?",L"Discard unsaved changes?"))) return;
                    const auto id=device_;const bool tv=!display().value("tv",false);auto h=host_;
                    h->run([this,h,id,tv]() -> std::function<void()> {h->plugins().layout_device_tv(id,tv);return [this]{history_=pe::EditHistory{};other_history_=pe::EditHistory{};uncommitted_=false;switch_pending_=true;reload_=true;};});
                });
            }
        }
        grid_=center.add<GridEditor>(); grid_->canvas=true; grid_->model=model(); palette_->grid=grid_;
        grid_->block_label=[this](const std::string& id,bool en) { auto t=block_label(id); return t.get(en); };
        grid_->on_edit=[this](auto n,auto group) { accept(std::move(n),std::move(group)); };
        grid_->on_select=[this] {
            sync_zone();
            if(grid_->focused_block>=0 && blocks_ && grid_->focused_block<int(model().blocks.size()) && block_!=grid_->focused_block) { block_=grid_->focused_block; blocks_->selected=block_; sync_block(); }
            host_->surface().invalidate();
        };
        grid_->on_area=[this](int a) {  // F6 erreicht die Kachelpalette, wenn sie offen ist
            if(a!=0 || !palette_ || !shell_ || !shell_->is_open(EditorShell::Palette)) return;
            const auto v=palette_->visible(); palette_->current=0;
            for(size_t i=0;i<v.size();++i) if(int(v[i])==grid_->palette_block) palette_->current=int(i);
            host_->surface().focus(palette_,true);
        };
        grid_->on_reject=[this] { reject(); };
        grid_->on_undo=[this] { undo(); }; grid_->on_redo=[this] { redo(); };
        auto zoom=center.add<ButtonRow>();
        zoom->add(T(L"Zoom −",L"Zoom −"),Button::Secondary,[this] { grid_->zoom=std::max(.5f,grid_->zoom-.15f); host_->relayout(); });
        zoom->add(T(L"Zoom +",L"Zoom +"),Button::Secondary,[this] { grid_->zoom=std::min(1.f,grid_->zoom+.15f); host_->relayout(); });
        zoom->add(T(L"Passend",L"Fit"),Button::Secondary,[this] { grid_->zoom=1.f; host_->relayout(); });
        center.add<Note>(T(L"Bausteine ziehen, Zone am Titel verschieben, Griffe zum Vergrößern ziehen; Ablage entfernt. F6/Strg+Tab: Leiste, Raster, Ablage. Pfeile: Ziel; Leertaste: aufnehmen; Enter: ablegen/wählen/anlegen; Esc: abbrechen. Aufgenommen: Strg+Auf/Ab = Stapelposition. Umschalt+Pfeil: Größe; Alt+Pfeil: verschieben; Entf: entfernen; Strg+Z/Y: Rückgängig/Wiederholen. Vorschau nach dem Ablegen. Texte mit Enter oder Fokuswechsel abschließen.",
            L"Drag blocks, move zones by their title, drag handles to resize; drop on trash to remove. F6/Ctrl+Tab: palette, grid, trash. Arrows: target; Space: pick up; Enter: drop/select/create; Esc: cancel. While carrying: Ctrl+Up/Down = stack position. Shift+arrow: resize; Alt+arrow: move; Delete: remove; Ctrl+Z/Y: undo/redo. Preview after dropping. Commit text with Enter or by leaving the field."));
        // Rechts: Eigenschaften der gewählten Kachel, dann der Seite.
        auto& props=shell_->props;
        auto info=props.add<Card>(T(L"Auswahl",L"Selection"));
        selection_=info->add<Label>(Text()); selection_->name=T(L"Auswahl",L"Selection");
        auto zone=props.add<Card>(T(L"Gewählte Zone",L"Selected zone"));
        zone_id_=zone->row<TextField>(T(L"Zonen-ID",L"Zone ID"),Text(),TextField::Single,L""); zone_id_->limit=64;
        zone_id_->on_change=[this] { text_pending(); }; zone_id_->on_commit=[this] { commit_zone(); };
        cycle_=zone->row<Segment>(T(L"Wechsel",L"Cycle"),Text(),std::vector<Text>{T(L"Aus",L"Off"),Text(L"rotate"),Text(L"first_available")},0);
        transition_=zone->row<Segment>(T(L"Übergang",L"Transition"),Text(),std::vector<Text>{Text(L"fade"),Text(L"none")},0);
        cycle_->on_change=transition_->on_change=[this] { commit_zone(); };
        if(catalog_) {zone->items[1]->visible=false;zone->items[2]->visible=false;}
        auto split=zone->add<ButtonRow>();
        split->add(T(L"Horizontal teilen",L"Split horizontally"),Button::Secondary,[this] { operation([this](auto& n) { return pe::split_zone(n,grid_->selected,true); }); });
        split->add(T(L"Vertikal teilen",L"Split vertically"),Button::Secondary,[this] { operation([this](auto& n) { return pe::split_zone(n,grid_->selected,false); }); });
        auto bc=props.add<Card>(T(L"Gewählte Kachel",L"Selected tile"),T(L"Zone, Wechselzeit und „Anzeigen, wenn“ des gewählten Eintrags der Ebenenliste.",L"Zone, cycle time and “Show if” of the entry selected in the layer list."));
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
        if(catalog_) for(size_t i=1;i<bc->items.size();++i) bc->items[i]->visible=false;
        auto raster=props.add<Card>(T(L"Seite: Raster",L"Page: grid"));
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
        auto pv=props.add<Card>(T(L"Live-Vorschau",L"Live preview"),catalog_?T(L"Beispieldaten der Seite, derselbe Renderer wie auf dem Display. Kopf und Fuß sind fest.",L"Page sample data, the same renderer as on the display. Header and footer are fixed."):T(L"Beispieldaten aus dem Plugin, derselbe Seitenrenderer wie auf dem Display.",L"Plugin sample data, the same page renderer as on the display."));
        auto orientation=pv->row<Segment>(T(L"Ausrichtung",L"Orientation"),Text(),std::vector<Text>{T(L"Quer",L"Landscape"),T(L"Hoch",L"Portrait")},portrait_?1:0);
        orientation->enabled=!display().value("tv",false);
        orientation->on_change=[this,orientation] {
            if(grid_ && grid_->carrying()) grid_->cancel(host_->surface());
            if(catalog_ && uncommitted_) { orientation->index=portrait_?1:0;host_->toast(T(L"Eingabe zuerst abschließen.",L"Commit the field first."),ToastKind::Error);return; }
            const bool next=orientation->index==1;
            if(catalog_ && next!=portrait_) std::swap(history_,other_history_);
            portrait_=next;++generation_;pending_=true;sync();
        };
        validation_=props.add<Label>(Text());
        error_=props.add<ButtonRow>()->add(T(L"Fehlerstelle auswählen",L"Select invalid field"),Button::Secondary,[this] {
            if(issue_block_>=0) { block_=issue_block_; blocks_->selected=block_; sync_block(); host_->relayout(); host_->surface().focus(block_zone_,true); }
            else { if(issue_zone_>=0) grid_->selected=issue_zone_; sync_zone(); host_->surface().focus(grid_,true); }
        });
        pending_=true;
    }
    // ---- Kachelpalette: Namen, Kategorien, Mini-Vorschauen -------------------------------------------------------------
    Text block_label(const std::string& id) const {
        if(catalog_) for(const auto& spec:catalog_->blocks) if(spec.id==id) return Text(widen(spec.label.de),widen(spec.label.en));
        return Text(widen(id));
    }
    std::string block_category(const std::string& id) const {
        if(catalog_) {
            const auto& key=catalog_->key;
            return key=="system"?"system":(key=="spotify"||key=="streaming")?"media":(key=="bf6"||key=="friends")?"games":"other";
        }
        for(const auto& w:base_.widgets) if(w.id==id) return w.type=="clock"?"time":(w.type=="metric"||w.type=="bar")?"system":"other";
        return "other";
    }
    bool required_block(const std::string& id) const {
        if(catalog_) for(const auto& spec:catalog_->blocks) if(spec.id==id) return spec.required;
        return false;
    }
    std::string theme_identity() const {
        const auto& cfg=host_->config();
        if(!cfg.is_object() || !cfg.contains("display") || !cfg["display"].is_object()) return "default";
        const auto& d=cfg["display"];
        return d.value("theme",std::string())+"|"+std::to_string(std::hash<std::string>{}(d.contains("theme_inline")?d["theme_inline"].dump():std::string()));
    }
    std::string page_key() const { return catalog_?"b:"+catalog_->key:"p:"+base_.id; }
    std::string thumb_key(const std::string& id) const { return page_key()+"|"+id+"|192x108|"+theme_identity(); }
    void select_category(const std::string& id) {
        category_=id;
        for(size_t i=0;i<chip_tabs_.size();++i) { chip_tabs_[i]->selected=chip_ids_[i]==id; chip_tabs_[i]->kind=chip_tabs_[i]->selected?Button::Primary:Button::Secondary; }
        if(palette_) { palette_->category=id; palette_->current=0; }
        host_->relayout();
    }
    void sync_palette() {
        if(!palette_) return;
        palette_->tiles.clear();
        for(const auto& b:model().blocks) {
            const auto id=b["id"].get<std::string>();
            palette_->tiles.push_back({id,block_label(id),block_category(id),b.value("shown",false),required_block(id)});
        }
        palette_->grid=grid_;
        if(palette_->current>=int(palette_->tiles.size())) palette_->current=0;
    }
    // Mini-Vorschauen im Arbeits-Thread: eine Seite mit Standardanordnung bzw. je Plugin-Widget eine Einzelzone,
    // derselbe Seitenrenderer und dieselben Beispieldaten wie die Live-Vorschau. Treffer bleiben je Anzeige-Theme im Cache.
    void request_tiles() {
        std::vector<std::string> need;
        for(const auto& b:model().blocks) { const auto id=b["id"].get<std::string>(); if(!thumbs_.count(thumb_key(id))) need.push_back(id); }
        if(need.empty()) return;
        tiles_loading_=true;
        auto h=host_; const auto* catalog=catalog_; const auto base=base_; const auto current=model();
        std::vector<std::string> keys; for(const auto& id:need) keys.push_back(thumb_key(id));
        h->run([this,h,catalog,base,current,need,keys]() -> std::function<void()> {
            std::vector<TilePalette::Thumb> out(need.size());
            try {
                std::string error;
                if(catalog) {
                    const auto standard=builtin::standard(*catalog);
                    auto page=h->app().page_preview(catalog->key,builtin::to_json(standard),{480,320},error);
                    auto layout=pe::geometry(standard.model);
                    plugins::zones::Plan plan;
                    if(layout) plan=plugins::zones::plan({8,36,464,266},*layout,builtin::inputs(*catalog,standard.model),6);
                    for(size_t n=0;n<need.size() && page;++n) {
                        plugins::zones::Rect area{8,36,464,266};
                        for(const auto& pb:plan.blocks) if(pb.id==need[n] && pb.visible()) area=pb.area;
                        const int x0=std::clamp(int(std::floor(area.x)),0,page->w-1),y0=std::clamp(int(std::floor(area.y)),0,page->h-1);
                        const int w=std::clamp(int(std::ceil(area.width)),1,page->w-x0),hgt=std::clamp(int(std::ceil(area.height)),1,page->h-y0);
                        Image crop(w,hgt); bool uniform=true;
                        for(int y=0;y<hgt;++y) for(int x=0;x<w;++x) {
                            const auto px=page->at(x0+x,y0+y); crop.px[size_t(y)*w+x]=px;
                            if(px!=page->at(x0,y0)) uniform=false;
                        }
                        out[n].image=std::make_shared<Image>(thumbnail(crop,192,108)); out[n].no_data=uniform;
                    }
                } else {
                    for(size_t n=0;n<need.size();++n) {
                        pe::Model m=current;
                        m.layout={{"columns",Json::array({1})},{"rows",Json::array({1})},{"zones",Json::array({{{"id","tile"},{"column",0},{"row",0}}})}};
                        for(auto& b:m.blocks) { b["zone"]="tile"; b["shown"]=b["id"]==need[n]; b.erase("seconds"); }
                        auto merged=plugins::merge_arrangement(base,m); if(!merged) continue;
                        auto image=h->app().plugin_preview(merged.manifest->definition.dump(),error,{480,320},true);
                        if(image) out[n].image=std::make_shared<Image>(thumbnail(*image,192,128));
                    }
                }
            } catch(const std::exception&) {}
            return [this,keys,out=std::move(out)] {
                tiles_loading_=false;
                for(size_t n=0;n<keys.size();++n) { auto t=out[n]; t.version=++thumb_version_; thumbs_[keys[n]]=std::move(t); }
                if(visible_) host_->surface().invalidate();
            };
        });
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
    // Gerätegröße der Vorschau und Seitenliste (reine Darstellung).
    void sync_chrome() {
        auto d=display();
        float w=480,h=320;
        if(!device_.empty()) { w=float(d.value("usable_width",480)); h=float(d.value("usable_height",320)); }
        if(!d.value("tv",false) && (h>w)!=portrait_) std::swap(w,h);
        if(device_.empty()) { w=portrait_?320.f:480.f; h=portrait_?480.f:320.f; }
        chrome_aspect_=w/std::max(1.f,h);
        if(size_chip_) {
            const auto size=std::to_wstring(int(w))+L" × "+std::to_wstring(int(h));
            size_chip_->text=Text(size+(portrait_?L" · Hochformat":L" · Querformat"),size+(portrait_?L" · Portrait":L" · Landscape"));
        }
        if(pages_list_) {
            pages_list_->items.clear();
            for(const auto& e:entries_) pages_list_->items.push_back({e.id,e.name,e.builtin?T(L"Eingebaut",L"Built-in"):T(L"Plugin",L"Plugin"),false});
            pages_list_->selected=chosen_;
        }
    }
    void dismiss_intro() {
        intro_seen_=true; if(intro_) intro_->visible=false;
        auto h=host_; if(h->plugins().editor_intro_done) h->run([h]() -> std::function<void()> { h->plugins().editor_intro_done(); return {}; });
        h->relayout();
    }
    // Name, Zone und Größe (Spalten × Zeilen) der Auswahl, nur lesend.
    void sync_selection() {
        if(!selection_ || !grid_) return;
        std::wstring de,en; const auto& m=model();
        if(block_>=0 && block_<int(m.blocks.size())) {
            const auto label=block_label(m.blocks[block_]["id"].get<std::string>()); const bool shown=m.blocks[block_].value("shown",false);
            de=L"Kachel: "+label.de+(shown?L"":L" (verborgen)"); en=L"Tile: "+label.en+(shown?L"":L" (hidden)");
        }
        const int z=grid_->selected; auto g=pe::geometry(m);
        if(g && z>=0 && z<int(g->zones.size())) {
            const auto& zone=g->zones[z]; const auto size=std::to_wstring(zone.column_span)+L" × "+std::to_wstring(zone.row_span);
            de+=std::wstring(de.empty()?L"":L"\n")+L"Zone "+widen(zone.id)+L" · Größe "+size+L" Zellen"; en+=std::wstring(en.empty()?L"":L"\n")+L"Zone "+widen(zone.id)+L" · size "+size+L" cells";
        }
        if(de.empty()) { de=L"Nichts gewählt. Eigenschaften der Seite: Raster und Vorschau unten."; en=L"Nothing selected. Page properties: grid and preview below."; }
        selection_->text=Text(de,en);
    }
    void sync_zone() {
        if(!grid_) return; syncing_=true; sync_selection();
        int z=grid_->selected;
        bool selected=z>=0 && z<int(model().layout["zones"].size());
        zone_id_->enabled=selected; cycle_->enabled=transition_->enabled=selected && !catalog_;
        if(selected) {
            auto& j=model().layout["zones"][z]; zone_id_->set_text(widen(j["id"].get<std::string>()));
            cycle_->index=!j.contains("cycle")?0:j["cycle"]["mode"]=="rotate"?1:2;
            transition_->index=j.contains("cycle") && j["cycle"].value("transition","fade")=="none"?1:0;
        }
        syncing_=false;
    }
    void sync_block() {
        if(!blocks_) return; sync_selection();
        const bool selected=block_>=0 && block_<int(model().blocks.size());
        block_zone_->enabled=selected;path_->enabled=op_->enabled=type_->enabled=literal_->enabled=selected && !catalog_;
        if(!selected) { seconds_->enabled=false; return; }
        syncing_=true; auto& b=model().blocks[block_]; auto& zs=model().layout["zones"];
        block_zone_->options.clear(); block_zone_->index=0;
        for(size_t i=0;i<zs.size();++i) { block_zone_->options.push_back(Text(widen(zs[i]["id"].get<std::string>()))); if(zs[i]["id"]==b["zone"]) block_zone_->index=int(i); }
        auto& z=zs[block_zone_->index]; seconds_->enabled=!catalog_ && z.contains("cycle") && z["cycle"]["mode"]=="rotate";
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
        sync_chrome(); grid_->display_aspect=chrome_aspect_;
        grid_->model=model(); sync_palette(); grid_->selected=std::clamp(grid_->selected,-1,int(model().layout["zones"].size())-1);
        auto l=pe::geometry(model());
        if(l) { grid_->column=std::clamp(grid_->column,0,int(l->columns.size())-1); grid_->row=std::clamp(grid_->row,0,int(l->rows.size())-1); }
        for(int a=0;a<2;++a) for(int i=0;i<6;++i) {
            auto& axis=model().layout[a==0?"columns":"rows"];
            weight_rows_[a][i]->visible=i<int(axis.size()); if(i<int(axis.size())) weights_[a][i]->val=axis[i];
        }
        blocks_->items.clear(); for(auto& b:model().blocks) {
            Text label(widen(b["id"].get<std::string>()));
            if(catalog_) for(const auto& spec:catalog_->blocks) if(b["id"]==spec.id)
                label=Text(widen(spec.label.de)+(spec.required?L" (Pflicht)":L""),widen(spec.label.en)+(spec.required?L" (required)":L""));
            blocks_->items.push_back({b["id"],label,Text(widen(b["zone"].get<std::string>())),b["shown"]});
        }
        block_=model().blocks.empty()?-1:std::clamp(block_,0,int(model().blocks.size())-1); blocks_->selected=block_;
        plugins::ParseResult result;
        if(catalog_) {result.error=builtin::validate(*catalog_,builtin_overlay());valid_=result.error.empty();}
        else {result=plugins::merge_arrangement(base_,model());valid_=bool(result);}
        issue_zone_=issue_block_=-1;
        const auto& zs=model().layout["zones"];
        if(!valid_) for(size_t z=0;z<zs.size();++z) {
            int count=0; for(auto& b:model().blocks) if(b["zone"]==zs[z]["id"] && b.value("shown",true)) ++count;
            if((result.error=="Every zone needs at least one block." && count==0) || (result.error=="At most 4 blocks per zone allowed." && count>4)) { issue_zone_=int(z); break; }
        }
        if(!valid_ && issue_zone_<0) issue_block_=block_;
        validation_->text=valid_?T(L"Anordnung gültig.",L"Arrangement valid."):Text(widen(pe::diagnostic(result.error,false)),widen(pe::diagnostic(result.error,true)));
        if(catalog_ && !valid_) validation_->text=T(L"Anordnung ungültig: Jede Zone braucht Blöcke, Pflichtblöcke müssen sichtbar bleiben und in Quer/Hoch passen.",L"Invalid arrangement: every zone needs blocks; required blocks must stay visible and fit landscape/portrait.");
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
        if(reset && !host_->confirm(!device_.empty()?T(L"Eigene Anordnung löschen und wieder vom Standard erben?",L"Remove own arrangement and inherit from standard again?"):catalog_?T(L"Anordnung und Backup löschen und den pixelgleichen Standard wiederherstellen?",L"Remove arrangement and backup and restore the pixel-identical standard?"):T(L"Anordnung und Backup löschen und die Manifest-Anordnung wiederherstellen?",L"Remove the arrangement and its backup and restore the manifest layout?"))) return;
        auto h=host_; auto id=entries_[chosen_].id;const bool built=bool(catalog_); auto overlay=built?builtin::to_json(builtin_overlay()):plugins::to_json(model()); ++generation_; pending_=false;
        const auto serial=generation_;const auto device=device_;
        h->run([this,h,id,overlay,reset,serial,built,device]() -> std::function<void()> {
            if(h->plugins().device_layout_apply && h->plugins().device_layout_reset) {
                if(reset) h->plugins().device_layout_reset(built,id,device);else h->plugins().device_layout_apply(built,id,device,overlay);
            }
            else if(built) {if(reset) h->plugins().builtin_layout_reset(id);else h->plugins().builtin_layout_apply(id,overlay);}
            else {if(reset) h->plugins().arrangement_reset(id); else h->plugins().arrangement_apply(id,overlay);}
            auto data=h->plugins().device_layout_get?h->plugins().device_layout_get(built,id,device):built?h->plugins().builtin_layout_get(id):h->plugins().arrangement_get(id);
            return [this,h,id,data,serial,built,device] {
                for(auto& e:entries_) if(e.id==id && e.builtin==built) {e.data=data;e.inherited[device]=data.value("inherited",true);}
                rebuild_tabs();
                if(inheritance_note_) inheritance_note_->text=data.value("inherited",true)?T(L"Erbt von Standard. Übernehmen legt eine eigene Anordnung an.",L"Inherits from standard. Apply creates an own arrangement."):T(L"Eigene Anordnung. Zurücksetzen aktiviert die Vererbung vom Standard.",L"Own arrangement. Reset restores inheritance from standard.");
                if(chosen_>=0 && entries_[chosen_].id==id && entries_[chosen_].builtin==built && grid_ && serial==generation_) {
                    auto parsed=built?plugins::ArrangementResult{}:plugins::parse_arrangement(data.value("overlay",Json()).dump());
                    auto builtin_parsed=built?builtin::parse(*catalog_,data.value("overlay",Json()).dump()):builtin::Result{};
                    if(parsed || builtin_parsed) {
                        if(built) {
                            history_.reset(portrait_?builtin_parsed.overlay->portrait.value_or(builtin_parsed.overlay->model):builtin_parsed.overlay->model);
                            other_history_.reset(portrait_?builtin_parsed.overlay->model:builtin_parsed.overlay->portrait.value_or(builtin_parsed.overlay->model));
                        } else history_.reset(*parsed.arrangement); uncommitted_=false; pending_=true;
                        status_->text=built?T(L"Standard bleibt pixelgleich; eigene Anordnung – Abstände folgen dem Raster.",L"Standard stays pixel-identical; custom arrangement – spacing follows the grid."):T(L"Nur die Anordnung wird gespeichert. Inhalte und Freigaben bleiben erhalten.",L"Only arrangement is saved. Content and consent are preserved.");
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
        const bool built=bool(catalog_);
        auto merged=built?plugins::ParseResult{}:plugins::merge_arrangement(base_,model()); if(!built && !merged) return;
        const auto serial=generation_; auto h=host_; auto json=built?builtin::to_json(builtin_overlay()).dump():merged.manifest->definition.dump();
        const auto key=built?catalog_->key:std::string();
        auto d=display();
        auto size=device_.empty()?(portrait_?std::pair{320,480}:std::pair{480,320}):std::pair{d.value("usable_width",480),d.value("usable_height",320)};
        if(!d.value("tv",false) && (size.second>size.first)!=portrait_) std::swap(size.first,size.second);
        previewing_=true;
        h->run([this,h,json,size,serial,built,key]() -> std::function<void()> {
            std::string error; std::optional<Image> image;
            try { image=built?h->app().page_preview(key,Json::parse(json),size,error):h->app().plugin_preview(json,error,size,true); } catch(const std::exception&) { error="preview"; }
            return [this,h,image=std::move(image),serial] {
                previewing_=false;
                if(visible_ && serial==generation_ && image) h->preview(image);
            };
        });
    }
};
}
std::unique_ptr<Page> make_layout_page(std::string selected) { return std::make_unique<LayoutPage>(std::move(selected)); }
}
