// © 2026 FuchsLabs – Eigene Ansichten, Clean-Room using ScreenBuddy ui_kit.
#include "ui_pages.h"
#include "ui_editor.h"
#include "ui_tile_palette.h"
#include "custom_view_editor.h"
#include "app.h"
#include "native_settings.h"
#include "util.h"
#include <algorithm>
#include <cwctype>
namespace fl::ui {
namespace {
using plugins::Json;
namespace edit=plugins::edit;
class ViewTab : public Button {
public:
    bool selected;
    ViewTab(Text label,bool value,std::function<void()> action):Button(std::move(label),value?Primary:Secondary,std::move(action)),selected(value) {}
    Role role() const override {return Role::NavTab;}
    bool checked() const override {return selected;}
};
class CustomViewsPage : public Page {
public:
    PageInfo info() const override {return {"custom_views",Group::App,T(L"Eigene Ansichten",L"Custom views"),T(L"Quellen kombinieren · pro Display · Vorschau mit Beispieldaten",L"Combine sources · per display · preview with sample data")};}
    void build(Host& host,Column& root) override {
        host_=&host;root_=&root;tabs_=root.add<Flow>();header_=root.add<Flow>();
        undo_=header_->add<Button>(T(L"Rückgängig",L"Undo"),Button::Secondary,[this]{undo(false);});
        redo_=header_->add<Button>(T(L"Wiederholen",L"Redo"),Button::Secondary,[this]{undo(true);});
        apply_=header_->add<Button>(T(L"Übernehmen",L"Apply"),Button::Primary,[this]{save();});
        header_->add<Button>(T(L"Von Standard erben",L"Inherit from standard"),Button::Secondary,[this]{inherit();});
        status_=root.add<Label>(T(L"Wird geladen …",L"Loading …"));
        shell_=root.add<EditorShell>();shell_->set_open(EditorShell::Layers,false);shell_->set_open(EditorShell::PageList,true);
        shell_->on_toggle=[this]{host_->relayout();};
        std::swap(shell_->left.items[0],shell_->left.items[2]);
        auto& views=*shell_->section[EditorShell::PageList];views.title=T(L"Seiten",L"Pages");
        views.add<ButtonRow>()->add(T(L"+ Neue Ansicht",L"+ New view"),Button::Primary,[this]{create();});
        views_=views.add<ItemList>(std::vector<ItemList::Item>{});views_->name=T(L"Ansichten",L"Views");views_->checkable=false;views_->reorder=false;
        views_->on_select=[this](int i){if(i>=int(documents_.size()))open_standard(i-int(documents_.size()));else choose(i);};
        auto management=views.add<ButtonRow>();management->add(T(L"Umbenennen",L"Rename"),Button::Secondary,[this]{if(name_) host_->surface().focus(name_,true);});
        management->add(T(L"Löschen",L"Delete"),Button::Danger,[this]{erase();});
        auto& palette=*shell_->section[EditorShell::Palette];
        filter_=palette.add<Combo>(std::vector<Text>{T(L"Alle Quellen",L"All sources"),Text(L"System"),Text(L"Spotify"),Text(L"RSS"),Text(L"Streaming"),Text(L"BF6"),T(L"Freunde",L"Friends"),Text(L"Discord"),Text(L"Home Assistant"),Text(L"Plugins"),T(L"KI-Status",L"AI Status")},1);
        filter_->name=T(L"Quelle",L"Source");filter_->on_change=[this]{sync_palette();host_->relayout();};
        auto search=palette.add<TextField>(TextField::Single,L"");search->hint=T(L"Kacheln suchen",L"Search tiles");search->name=search->hint;search->limit=64;
        search->on_change=[this,search]{palette_->query=search->text();for(auto& c:palette_->query)c=wchar_t(std::towlower(c));palette_->current=0;host_->relayout();};
        palette_=palette.add<TilePalette>();palette_->surface=&host_->surface();
        palette_->thumb_of=[this](const std::string& id)->const TilePalette::Thumb* {
            auto b=block(id);if(!b) return nullptr;auto it=thumbs_.find(key(*b));return it==thumbs_.end()?nullptr:&it->second;
        };
        shell_->center.title=T(L"Fläche",L"Canvas");
        size_=shell_->center.add<Chip>(Text(L"480 × 320"));
        grid_=shell_->center.add<GridEditor>();grid_->canvas=true;grid_->name=T(L"Ansichtsfläche",L"View canvas");palette_->grid=grid_;
        grid_->block_label=[this](const std::string& id,bool en){auto b=block(id);return b?label(*b).get(en):widen(id);};
        grid_->tile_image=[this](const std::string& id)->std::shared_ptr<const Image>{auto b=block(id);if(!b)return {};auto it=thumbs_.find(key(*b));return it==thumbs_.end()?nullptr:it->second.image;};
        grid_->on_edit=[this](auto next,auto group){accept(std::move(next),std::move(group));};
        grid_->on_undo=[this]{undo(false);};grid_->on_redo=[this]{undo(true);};
        grid_->on_select=[this]{sync_properties();host_->relayout();};
        grid_->on_reject=[this]{host_->toast(T(L"Passt nicht: belegte Zelle, Größe oder Grenze prüfen.",L"Does not fit: check occupied cells, size or limits."),ToastKind::Error);};
        grid_->on_area=[this](int area){if(area==0)host_->surface().focus(palette_,true);};
        shell_->center.add<Note>(T(L"Klicken oder ziehen · F6: Bereiche · Leertaste: aufnehmen · Enter: ablegen · Esc: abbrechen · Umschalt+Pfeil: Größe · Entf: Papierkorb · Strg+Z/Y",L"Click or drag · F6: areas · Space: pick up · Enter: drop · Esc: cancel · Shift+arrow: size · Delete: trash · Ctrl+Z/Y"));
        auto preview=shell_->center.add<Card>(T(L"Vorschau mit Beispieldaten",L"Preview with sample data"));
        preview_=preview->add<ImageView>();preview_->empty_text=T(L"Kachel hinzufügen",L"Add a tile");
        auto props=shell_->props.add<Card>(T(L"Eigenschaften",L"Properties"));
        name_=props->wide_row<TextField>(T(L"Name der Ansicht",L"View name"),Text(),TextField::Single,L"");name_->limit=128;
        name_->on_change=[this]{if(!syncing_) {name_pending_=true;update_buttons();}};
        name_->on_commit=[this]{commit_name();};
        selection_=props->add<Label>(Text());
        columns_=props->row<Combo>(T(L"Breite (Zellen)",L"Width (cells)"),Text(),std::vector<Text>{Text(L"1"),Text(L"2"),Text(L"3"),Text(L"4"),Text(L"5"),Text(L"6")},0);
        rows_=props->row<Combo>(T(L"Höhe (Zellen)",L"Height (cells)"),Text(),columns_->options,0);
        columns_->on_change=rows_->on_change=[this]{resize();};
        density_=props->wide_row<Combo>(T(L"Dichte (höchstens)",L"Density (maximum)"),Text(),std::vector<Text>{T(L"Kompakt",L"Compact"),T(L"Normal",L"Regular"),T(L"Automatisch",L"Automatic")},2);
        density_->on_change=[this]{properties();};
        tap_mode_=props->wide_row<Combo>(T(L"Touch-Ziel",L"Touch target"),Text(),std::vector<Text>{T(L"Standard",L"Default"),T(L"Individuell",L"Custom")},0);
        tap_action_=props->wide_row<Combo>(T(L"Aktion",L"Action"),Text(),std::vector<Text>{T(L"Seite wechseln zu …",L"Switch to page …"),T(L"Link am PC öffnen",L"Open link on PC"),T(L"Nächster Eintrag (Plugin-Wechselfeld)",L"Next item (plugin cycle)")},0);
        tap_page_=props->wide_row<Combo>(T(L"Zielseite",L"Target page"),Text(),std::vector<Text>{},-1);
        tap_url_=props->wide_row<TextField>(T(L"HTTPS-Link",L"HTTPS link"),T(L"Nur Plugin-Hosts mit Freigabe „Links öffnen“; sonst Standard.",L"Only plugin hosts with Open links consent; otherwise Default."),TextField::Single,L"");tap_url_->limit=2048;
        tap_mode_->on_change=tap_action_->on_change=tap_page_->on_change=[this]{touch_properties();};
        tap_url_->on_change=[this]{if(!syncing_){property_pending_=true;update_buttons();}};
        tap_url_->on_commit=[this]{touch_properties();};
        path_=props->wide_row<TextField>(T(L"Anzeigen wenn: data.…",L"Show if: data.…"),T(L"Leer = immer. Nur öffentliche Quelldaten.",L"Empty = always. Public source data only."),TextField::Single,L"");path_->limit=128;
        op_=props->wide_row<Combo>(T(L"Vergleich",L"Comparison"),Text(),std::vector<Text>{T(L"Vorhanden",L"Present"),Text(L"=="),Text(L"!="),Text(L"<"),Text(L"<="),Text(L">"),Text(L">=")},0);
        literal_=props->wide_row<TextField>(T(L"Wert (JSON)",L"Value (JSON)"),T(L"Zahl, true/false oder Text in Anführungszeichen",L"Number, true/false or quoted text"),TextField::Single,L"");literal_->limit=256;
        path_->on_change=literal_->on_change=op_->on_change=[this]{if(!syncing_){property_pending_=true;update_buttons();}};
        props->add<ButtonRow>()->add(T(L"Bedingung bestätigen",L"Confirm condition"),Button::Secondary,[this]{properties();});
        auto raster=shell_->props.add<Card>(T(L"Raster der Ansicht",L"View grid"));
        for(bool col:{true,false}) {auto buttons=raster->add<ButtonRow>();for(int delta:{1,-1}) buttons->add(col?(delta>0?T(L"Spalte +",L"Column +"):T(L"Spalte −",L"Column −")):(delta>0?T(L"Zeile +",L"Row +"):T(L"Zeile −",L"Row −")),Button::Secondary,[this,col,delta]{if(!loaded_)return;auto n=history_.current();if(edit::resize_axis(n,col,delta))accept(n);});}
        legacy_root_=root.add<Column>();legacy_root_->visible=false;
        clear_dirty(root);loaded_=false;
    }
    void shown(Host&) override {visible_=true;if(!loaded_)load();}
    void hidden(Host& host) override {if(legacy_)legacy_->hidden(host);visible_=false;if(grid_)grid_->cancel(host.surface());host.preview(std::nullopt);}
    void tick(Host&,const Status&) override {
        if(!visible_ || host_->busy())return;
        if(legacy_){legacy_->tick(*host_,host_->status());return;}
        if(GetTickCount64()-poll_>2000) {poll_=GetTickCount64();refresh_displays();return;}
        if(pending_ && loaded_ && !grid_->carrying())render_preview();
        update_buttons();
    }
private:
    std::unique_ptr<Page> legacy_;Column* legacy_root_=nullptr;
    std::vector<std::pair<std::string,Text>> standard_pages_;
    Host* host_=nullptr;Column* root_=nullptr;Flow *tabs_=nullptr,*header_=nullptr;EditorShell* shell_=nullptr;
    ItemList* views_=nullptr;TilePalette* palette_=nullptr;GridEditor* grid_=nullptr;ImageView* preview_=nullptr;
    Combo *tap_mode_=nullptr,*tap_action_=nullptr,*tap_page_=nullptr;TextField* tap_url_=nullptr;
    std::vector<std::string> page_keys_;
    Combo *filter_=nullptr,*columns_=nullptr,*rows_=nullptr,*density_=nullptr,*op_=nullptr;
    TextField *name_=nullptr,*path_=nullptr,*literal_=nullptr;Label *status_=nullptr,*selection_=nullptr;Chip* size_=nullptr;
    Button *undo_=nullptr,*redo_=nullptr,*apply_=nullptr;
    std::vector<custom::View> documents_;std::vector<custom::Tile> inventory_;std::map<std::string,TilePalette::Thumb> thumbs_;
    edit::EditHistory history_;custom::View working_;Json displays_=Json::array();std::string device_;
    int chosen_=-1;bool loaded_=false,visible_=false,syncing_=false,pending_=false,name_pending_=false,property_pending_=false,metadata_dirty_=false;
    uint64_t poll_=0,generation_=0,thumb_version_=0;
    custom::Store store() const {return custom::Store(host_->app().cfg_path().parent_path()/L"views");}
    static void clear_dirty(Widget& w){w.track_dirty=false;std::vector<Widget*> children;w.children(children);for(auto child:children)clear_dirty(*child);}
    const Json* block(const std::string& id) const {if(!loaded_)return nullptr;for(const auto& b:history_.current().blocks)if(b["id"]==id)return &b;return nullptr;}
    std::string key(const Json& b)const{return b.value("source",std::string())+"|"+b.value("block",std::string());}
    Text label(const Json& b) const {
        auto source=b.value("source",std::string()),id=b.value("block",std::string());
        if(auto c=builtin::find(source))for(const auto& s:c->blocks)if(s.id==id)return Text(widen(c->label.de+" · "+s.label.de),widen(c->label.en+" · "+s.label.en));
        return Text(widen(source+" · "+id));
    }
    bool dirty()const{return loaded_ && (history_.can_undo() || metadata_dirty_ || name_pending_ || property_pending_);}
    bool discard(){return !dirty() || host_->confirm(T(L"Ungesicherte Änderungen verwerfen?",L"Discard unsaved changes?"));}
    Json display()const{for(const auto& d:displays_)if(d.value("id",std::string())==device_)return d;return Json::object();}
    std::pair<int,int> dimensions()const {auto d=display();return {d.value("usable_width",480),d.value("usable_height",320)};}
    void load() {
        host_->run([this]()->std::function<void()>{auto docs=store().list();auto palette=host_->app().custom_palette();auto displays=host_->plugins().layout_displays?custom::active_displays(host_->plugins().layout_displays()):Json::array();
            return [this,docs=std::move(docs),palette=std::move(palette),displays]{documents_=docs;inventory_=palette;displays_=displays;refresh_list();if(!documents_.empty())choose(0,true);else {tabs();status_->text=T(L"Mit + Neue Ansicht beginnen.",L"Start with + New view.");}thumbnails();};});
    }
    void refresh_list(){views_->items.clear();for(const auto& v:documents_)views_->items.push_back({v.id,Text(widen(v.name)),Text(),false});
        standard_pages_.clear();for(const auto& c:builtin::catalogs())if(c.enabled && (c.key!="ai" || host_->config()["ai"].value("enabled",false)))standard_pages_.push_back({"builtin:"+c.key,Text(widen(c.label.de),widen(c.label.en))});
        if(host_->plugins().list)for(const auto& e:host_->plugins().list())if(e.enabled)standard_pages_.push_back({"plugin:"+e.id,Text(widen(e.name))});
        for(const auto& [id,label]:standard_pages_)views_->items.push_back({id,label,T(L"Standard / Plugin",L"Standard / plugin"),false});
        tap_page_->options.clear();page_keys_.clear();for(const auto& key:config::PAGES){page_keys_.push_back(key);tap_page_->options.push_back(source_text(key));}
        for(const auto& v:documents_){page_keys_.push_back("custom:"+v.id);tap_page_->options.push_back(Text(widen(v.name)));}
        if(host_->plugins().list)for(const auto& e:host_->plugins().list())if(e.enabled) {page_keys_.push_back("plugin:"+e.id);tap_page_->options.push_back(Text(widen(e.name)));}views_->selected=chosen_;host_->relayout();}
    void open_standard(int index) {
        if(index<0 || index>=int(standard_pages_.size()) || host_->busy() || !discard())return;
        host_->preview(std::nullopt);
        for(auto& child:root_->items)child->visible=false;
        legacy_root_->items.clear();legacy_root_->visible=true;
        legacy_root_->add<ButtonRow>()->add(T(L"Zurück zu Eigene Ansichten",L"Back to Custom views"),Button::Secondary,[this]{
            if(!host_->confirm(T(L"Zurück zur Seitenliste? Ungesicherte Änderungen im Seiteneditor werden verworfen.",L"Back to the page list? Unsaved page editor changes will be discarded.")))return;
            host_->run([this]()->std::function<void()>{return [this]{legacy_->hidden(*host_);legacy_.reset();legacy_root_->items.clear();for(auto& child:root_->items)child->visible=true;legacy_root_->visible=false;host_->relayout();};});
        });
        legacy_=make_layout_page(standard_pages_[index].first);legacy_->build(*host_,*legacy_root_);legacy_->shown(*host_);host_->relayout();
    }
    void choose(int index,bool force=false){
        if(index<0 || index>=int(documents_.size()))return;
        if(!force && !discard()){views_->selected=chosen_;return;}
        chosen_=index;working_=documents_[index];metadata_dirty_=false;reset_draft();refresh_list();
    }
    void reset_draft(){
        history_.reset(custom::editor_model(working_.effective(device_),inventory_));loaded_=true;name_pending_=property_pending_=false;
        syncing_=true;name_->set_text(widen(working_.name));syncing_=false;grid_->selected=-1;grid_->focused_block=-1;
        ++generation_;pending_=true;sync();tabs();
    }
    void create(){
        if(host_->busy() || !discard())return;
        if(documents_.size()>=custom::view_limit){host_->toast(T(L"Höchstens 64 Ansichten.",L"At most 64 views."),ToastKind::Error);return;}
        custom::View view;for(int i=1;;++i){view.id="ansicht-"+std::to_string(i);if(std::none_of(documents_.begin(),documents_.end(),[&](const auto& v){return v.id==view.id;}))break;}
        view.name=host_->english()?"My view":"Meine Ansicht";view.standard.columns={1,1,1};view.standard.rows={1,1};documents_.push_back(view);choose(int(documents_.size())-1,true);metadata_dirty_=true;update_buttons();
    }
    void erase(){
        if(!loaded_ || host_->busy() || !host_->confirm(T(L"Ansicht einschließlich Geräte-Layouts und Backup löschen?",L"Delete view including device layouts and backup?")))return;
        auto id=working_.id;host_->run([this,id]()->std::function<void()>{auto error=store().erase(id);if(!error.empty())throw std::runtime_error(error);host_->app().mark_dirty();return [this,id]{
            documents_.erase(std::remove_if(documents_.begin(),documents_.end(),[&](const auto& v){return v.id==id;}),documents_.end());
            loaded_=false;chosen_=-1;refresh_list();if(!documents_.empty())choose(0,true);else {grid_->model={};palette_->tiles.clear();preview_->image.reset();++preview_->version;status_->text=T(L"Mit + Neue Ansicht beginnen.",L"Start with + New view.");}update_buttons();};});
    }
    void commit_name(){if(!loaded_ || syncing_)return;auto next=working_;next.name=narrow(name_->text());if(!custom::parse(custom::to_json(next).dump())){name_->invalid=true;return;}working_.name=next.name;name_->invalid=false;metadata_dirty_=metadata_dirty_ || name_pending_;name_pending_=false;update_buttons();}
    void save(){
        if(!loaded_ || host_->busy())return;commit_name();if(name_pending_ || property_pending_)return;
        auto layout=custom::editor_layout(history_.current());if(!layout)return;auto view=working_;if(device_.empty())view.standard=*layout;else view.devices[device_]=*layout;
        if(!custom::parse(custom::to_json(view).dump())){host_->toast(T(L"Ansicht ungültig oder Gerätelimit erreicht.",L"Invalid view or device limit reached."),ToastKind::Error);return;}
        host_->run([this,view]()->std::function<void()>{auto error=store().save(view);if(!error.empty())throw std::runtime_error(error);host_->app().mark_dirty();return [this,view]{working_=view;documents_[chosen_]=view;metadata_dirty_=false;reset_draft();refresh_list();host_->toast(T(L"Ansicht übernommen und im Seitenzyklus verfügbar.",L"View applied and available in the page cycle."),ToastKind::Success);};});
    }
    void inherit(){if(!loaded_ || device_.empty() || host_->busy() || !discard())return;
        auto view=documents_[chosen_];view.devices.erase(device_);
        host_->run([this,view]()->std::function<void()>{auto error=store().save(view);if(!error.empty())throw std::runtime_error(error);host_->app().mark_dirty();return [this,view]{documents_[chosen_]=working_=view;metadata_dirty_=false;reset_draft();};});
    }
    void tabs(){
        tabs_->items.clear();auto add=[this](Text text,std::string id){tabs_->add<ViewTab>(text,id==device_,[this,id]{if(id==device_ || host_->busy() || !discard())return;device_=id;metadata_dirty_=false;if(loaded_){working_=documents_[chosen_];reset_draft();}else tabs();});};
        add(T(L"Standard",L"Standard"),"");for(const auto& d:displays_){auto id=d.value("id",std::string());auto text=widen(d.value("name",id))+L" · "+std::to_wstring(d.value("width",480))+L" × "+std::to_wstring(d.value("height",320));add(Text(text),id);}host_->relayout();
    }
    void refresh_displays(){
        if(!host_->plugins().layout_displays)return;
        host_->run([this]()->std::function<void()>{auto displays=custom::active_displays(host_->plugins().layout_displays());return [this,displays]{if(displays==displays_)return;displays_=displays;
            if(!device_.empty() && std::none_of(displays_.begin(),displays_.end(),[&](const auto& d){return d.value("id",std::string())==device_;})) {
                // Retain this device's draft in the working view, without writing it to disk.
                if(loaded_)if(auto layout=custom::editor_layout(history_.current())){working_.devices[device_]=*layout;metadata_dirty_=true;}
                device_.clear();if(loaded_)reset_draft();
            }tabs();};});
    }
    void sync_palette(){
        if(!loaded_)return;palette_->tiles.clear();static const char* sources[]={"","system","spotify","rss","streaming","bf6","friends","discord","homeassistant","plugin:","ai"};
        palette_->category=sources[filter_->index];for(const auto& b:history_.current().blocks){auto source=b.value("source",std::string());palette_->tiles.push_back({b["id"],label(b),source.starts_with("plugin:")?"plugin:":source,b.value("shown",false),false});}palette_->current=0;
    }
    int selected_block()const{if(!loaded_ || !grid_)return -1;if(grid_->focused_block>=0 && grid_->focused_block<int(history_.current().blocks.size()))return grid_->focused_block;
        if(grid_->selected>=0 && grid_->selected<int(history_.current().layout["zones"].size())){auto id=history_.current().layout["zones"][grid_->selected]["id"];for(size_t i=0;i<history_.current().blocks.size();++i)if(history_.current().blocks[i].value("shown",false)&&history_.current().blocks[i]["zone"]==id)return int(i);}return -1;}
    void sync_properties(){
        syncing_=true;int i=selected_block();const bool selected=i>=0;columns_->enabled=rows_->enabled=density_->enabled=path_->enabled=op_->enabled=literal_->enabled=selected;
        selection_->text=selected?label(history_.current().blocks[i]):T(L"Kachel wählen oder hinzufügen.",L"Select or add a tile.");
        if(selected){const auto& b=history_.current().blocks[i];density_->index=b.value("density",2);
            const auto& zones=history_.current().layout["zones"];for(const auto& z:zones)if(z["id"]==b["zone"]){columns_->index=z.value("column_span",1)-1;rows_->index=z.value("row_span",1)-1;}
            path_->set_text(L"");literal_->set_text(L"");op_->index=0;if(b.contains("show_if")){const auto& c=b["show_if"];path_->set_text(widen(c.value("path",std::string())));const std::vector<std::string> ops={"","==","!=","<","<=",">",">="};auto it=std::find(ops.begin(),ops.end(),c.value("op",std::string()));op_->index=it==ops.end()?0:int(it-ops.begin());if(c.contains("value"))literal_->set_text(widen(c["value"].dump()));}}
        tap_mode_->enabled=selected;tap_action_->enabled=tap_page_->enabled=tap_url_->enabled=false;
        tap_mode_->index=0;tap_action_->index=0;tap_page_->index=-1;tap_url_->set_text(L"");
        if(selected) {const auto& b=history_.current().blocks[i];
            if(b.contains("tap")) {const auto& t=b["tap"];tap_mode_->index=1;tap_action_->index=t.value("action",std::string())=="open_link"?1:t.value("action",std::string())=="next"?2:0;tap_url_->set_text(widen(t.value("url",std::string())));auto it=std::find(page_keys_.begin(),page_keys_.end(),t.value("page",std::string()));if(it!=page_keys_.end())tap_page_->index=int(it-page_keys_.begin());}
            tap_action_->enabled=tap_mode_->index==1;tap_page_->enabled=tap_action_->enabled && tap_action_->index==0;
            tap_url_->enabled=tap_action_->enabled && tap_action_->index==1 && b.value("source",std::string()).starts_with("plugin:");
        }
        syncing_=false;
    }
    void touch_properties() {
        if(syncing_ || !loaded_)return;const int i=selected_block();if(i<0)return;auto n=history_.current();auto& b=n.blocks[i];b.erase("tap");
        if(tap_mode_->index==1) {Json t={{"action",tap_action_->index==0?"page":tap_action_->index==1?"open_link":"next"}};
            if(tap_action_->index==0 && tap_page_->index>=0 && tap_page_->index<int(page_keys_.size()))t["page"]=page_keys_[tap_page_->index];
            if(tap_action_->index==1)t["url"]=narrow(tap_url_->text());
            if(auto valid=custom::parse_tap(t,b.value("source",std::string())))b["tap"]=custom::tap_json(*valid);
            else { // Keep the controls editable while choosing a target; Apply stores Default.
                tap_action_->enabled=true;tap_page_->enabled=tap_action_->index==0;tap_url_->enabled=tap_action_->index==1 && b.value("source",std::string()).starts_with("plugin:");
                history_.push(n,{},GetTickCount64());grid_->model=history_.current();property_pending_=false;++generation_;pending_=true;
                status_->text=tap_action_->index==0?T(L"Touch-Ziel unvollständig: Zielseite wählen, sonst gilt Standard.",L"Touch target incomplete: choose a target page, otherwise Default applies.")
                    :T(L"Touch-Ziel ungültig oder für diese Quelle nicht erlaubt: es gilt Standard.",L"Touch target invalid or not allowed for this source: Default applies.");
                update_buttons();host_->relayout();return;
            }
        }
        property_pending_=false;accept(n);
    }
    void sync(){grid_->model=history_.current();grid_->selected=std::clamp(grid_->selected,-1,int(grid_->model.layout["zones"].size())-1);auto [w,h]=dimensions();grid_->display_aspect=float(w)/h;size_->text=Text(std::to_wstring(w)+L" × "+std::to_wstring(h));sync_palette();sync_properties();
        status_->text=device_.empty()?T(L"Standard für alle Displays · Quellen müssen eingerichtet und aktiv sein.",L"Standard for all displays · sources must be configured and enabled."):working_.devices.contains(device_)?T(L"Eigene Anordnung für dieses Display.",L"Own layout for this display."):T(L"Erbt Standard · Übernehmen legt ein Geräte-Layout an.",L"Inherits standard · Apply creates a device layout.");update_buttons();host_->relayout();}
    void accept(edit::Model next,std::string group={}){
        if(!loaded_)return;custom::prune_editor_zones(next);if(!custom::editor_layout(next)){host_->toast(T(L"Ungültige Kachel, Bedingung oder belegte Zone.",L"Invalid tile, condition or occupied zone."),ToastKind::Error);grid_->model=history_.current();return;}
        if(history_.push(next,group,GetTickCount64())){++generation_;pending_=true;}property_pending_=false;sync();
    }
    void undo(bool redo){if(!loaded_)return;if(redo?history_.redo():history_.undo()){property_pending_=false;++generation_;pending_=true;sync();}}
    void resize(){if(syncing_ || !loaded_)return;auto n=history_.current();const int i=selected_block();if(i<0)return;for(size_t z=0;z<n.layout["zones"].size();++z)if(n.layout["zones"][z]["id"]==n.blocks[i]["zone"]){auto old=n.layout["zones"][z];if(edit::resize_zone(n,z,columns_->index+1-old.value("column_span",1),rows_->index+1-old.value("row_span",1)))accept(n);else {sync_properties();host_->toast(T(L"Größe passt nicht ins Raster.",L"Size does not fit the grid."),ToastKind::Error);}break;}}
    void properties(){
        if(syncing_ || !loaded_)return;const int i=selected_block();if(i<0)return;auto n=history_.current();auto& b=n.blocks[i];b["density"]=density_->index;b.erase("show_if");
        try{auto path=narrow(path_->text());if(!path.empty()){b["show_if"]={{"path",path}};if(op_->index>0){const char* ops[]={"","==","!=","<","<=",">",">="};b["show_if"]["op"]=ops[op_->index];b["show_if"]["value"]=Json::parse(narrow(literal_->text()));}}
            if(!custom::editor_layout(n))throw std::runtime_error("condition");accept(n);
        }catch(const std::exception&){property_pending_=true;host_->toast(T(L"Bedingung ungültig: data-Pfad und typisierten JSON-Wert prüfen.",L"Invalid condition: check data path and typed JSON value."),ToastKind::Error);update_buttons();}
    }
    void update_buttons(){undo_->enabled=loaded_&&history_.can_undo();redo_->enabled=loaded_&&history_.can_redo();apply_->enabled=loaded_&&!host_->busy()&&!property_pending_;}
    void thumbnails(){
        const auto inventory=inventory_;host_->run([this,inventory]()->std::function<void()>{std::map<std::string,TilePalette::Thumb> out;for(auto tile:inventory){custom::View v;v.id="sample";v.name="Sample";tile.zone={"tile",0,0,1,1};v.standard.tiles={tile};auto image=host_->app().custom_preview(v,"",{480,320});TilePalette::Thumb thumb;
                if(image){Image crop(464,266);for(int y=0;y<266;++y)for(int x=0;x<464;++x)crop.px[size_t(y)*464+x]=image->at(x+8,y+36);thumb.no_data=std::all_of(crop.px.begin(),crop.px.end(),[&](auto px){return px==crop.px.front();});thumb.image=std::make_shared<Image>(thumbnail(crop,192,108));}else thumb.no_data=true;out[tile.source+"|"+tile.block]=thumb;}
            return [this,out=std::move(out)]{thumbs_=out;for(auto& [id,t]:thumbs_)t.version=++thumb_version_;grid_->tile_version=thumb_version_;host_->surface().invalidate();};});
    }
    void render_preview(){pending_=false;auto layout=custom::editor_layout(history_.current());if(!layout)return;auto view=working_;if(device_.empty())view.standard=*layout;else view.devices[device_]=*layout;
        auto device=device_;auto size=dimensions();auto generation=generation_;host_->run([this,view,device,size,generation]()->std::function<void()>{auto image=host_->app().custom_preview(view,device,size);return [this,image=std::move(image),generation]{if(generation!=generation_ || !visible_)return;preview_->image=image;++preview_->version;host_->relayout();};});
    }
};
}
std::unique_ptr<Page> make_custom_views_page(){return std::make_unique<CustomViewsPage>();}
}
