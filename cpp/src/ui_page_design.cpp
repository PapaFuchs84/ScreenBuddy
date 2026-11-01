// © 2026 FuchsLabs
// Farben & Design: Farbschema des Displays (Hell, Dunkel, An Windows anpassen, eigene Themes) samt Theme-Editor.
#include "ui_pages.h"

#include <algorithm>
#include <filesystem>

#include <windows.h>
#include <commdlg.h>

#include "app.h"
#include "native_settings.h"
#include "render.h"
#include "theme.h"
#include "util.h"

namespace fl::ui {
namespace {

namespace th = fl::theme;

Text color_name(const std::string& key) {
    if (key == "bg") return T(L"Hintergrund", L"Background");
    if (key == "card") return T(L"Karten", L"Cards");
    if (key == "line") return T(L"Linien", L"Lines");
    if (key == "header") return T(L"Kopfzeile & Balken", L"Header & bars");
    if (key == "accent") return T(L"Akzentschrift", L"Accent text");
    if (key == "text") return T(L"Schrift", L"Text");
    if (key == "muted") return T(L"Nebentext", L"Secondary text");
    if (key == "success") return T(L"Erfolg / online", L"Success / online");
    return T(L"Fehler / Warnung", L"Error / warning");
}

class DesignPage : public Page {
public:
    PageInfo info() const override {
        return {"design", Group::App, T(L"Farben & Design", L"Colors & design"),
                T(L"Farbschema des Displays – Hell, Dunkel, an Windows angepasst oder eigene Themes. Die Vorschau rechts zeigt die Wahl sofort",
                  L"Color scheme of the display – light, dark, matched to Windows or your own themes. The preview shows your choice right away")};
    }
    void build(Host& host, Column& root) override {
        host_ = &host;
        previewing_ = again_ = false;
        visible_ = false;
        ++preview_generation_;
        cfg_theme_ = host.config().at("display").value("theme", "hell");
        Card* card = root.add<Card>(T(L"Farbschema", L"Color scheme"),
            T(L"Gilt für USB-Display, Bildschirm und Companion. Standard ist das FuchsLabs-Design.",
              L"Applies to USB display, monitor and Companion. The default is the FuchsLabs design."));
        combo_ = card->row<Combo>(T(L"Farbschema", L"Color scheme"), T(L"Eigene Themes siehe unten", L"See below for your own themes"),
                                  std::vector<Text>{}, 0);
        combo_->width = 300;
        combo_->on_change = [this] { selection_changed(); };
        info_ = card->add<Note>(Text(), Note::Info);
        problems_ = card->add<Note>(Text(), Note::Warn);

        Card* store = root.add<Card>(T(L"Themes aus dem Store", L"Themes from the store"),
            T(L"Themes bestehen nur aus neun Farben, ohne Programmcode, und sind wie die Plugins signiert. Nach der Installation ist das Theme oben ausgewählt; übernommen wird es mit „Speichern & anwenden“. Der Abruf lässt sich unter „Plugin-Store“ ausschalten.",
              L"Themes are just nine colors, no program code, and signed like the plugins. After installing, the theme is selected above; it is kept with “Save & apply”. The lookup can be switched off under “Plugin store”."));
        store_status_ = store->row<Label>(T(L"Stand", L"State"), Text(), T(L"Wird geladen …", L"Loading …"), Font::Small, Label::Muted);
        store_list_ = store->add<ItemList>(std::vector<ItemList::Item>{});
        store_list_->checkable = false;
        store_list_->reorder = false;
        store_list_->track_dirty = false;
        store_list_->name = T(L"Verfügbare Themes", L"Available themes");
        store_list_->empty_text = T(L"Keine Themes im Store.", L"No themes in the store.");
        store_list_->on_select = [this](int i) {
            store_selected_ = i >= 0 && i < int(store_.themes.size()) ? store_.themes[size_t(i)].id : "";
            update_store_buttons();
        };
        auto sr = store->add<ButtonRow>();
        store_refresh_ = sr->add(T(L"Store aktualisieren", L"Refresh store"), Button::Secondary, [this] { load_store(true); });
        store_install_ = sr->add(T(L"Theme installieren", L"Install theme"), Button::Primary, [this] { install_store_theme(); });

        Card* tools = root.add<Card>(T(L"Eigene Themes", L"Your own themes"),
            T(L"Themes liegen als .json-Dateien im Ordner themes der Installation und werden sofort gespeichert. "
              L"Die Auswahl oben wird erst mit „Speichern & anwenden“ übernommen.",
              L"Themes are stored as .json files in the installation’s themes folder and saved immediately. "
              L"The choice above is only kept with “Save & apply”."));
        auto r1 = tools->add<ButtonRow>();
        r1->add(T(L"Neu", L"New"), Button::Primary, [this] { create_new(); });
        duplicate_ = r1->add(T(L"Duplizieren", L"Duplicate"), Button::Secondary, [this] { duplicate(); });
        remove_ = r1->add(T(L"Löschen", L"Delete"), Button::Danger, [this] { remove(); });
        auto r2 = tools->add<ButtonRow>();
        r2->add(T(L"Importieren …", L"Import …"), Button::Secondary, [this] { import_file(); });
        export_ = r2->add(T(L"Exportieren …", L"Export …"), Button::Secondary, [this] { export_file(); });
        editor_ = root.add<Card>();
        reload(cfg_theme_);
        if (store_loaded_) fill_store();
        update_store_buttons();
    }
    void tick(Host&, const Status&) override { update_store_buttons(); }
    void collect(Host&, ojson& patch) override {
        if (combo_ && combo_->index >= 0 && combo_->index < int(keys_.size())) patch["display"]["theme"] = keys_[size_t(combo_->index)];
    }
    void shown(Host&) override {
        visible_ = true;
        load_store(false);
        if (combo_ && th::Store().list().size() != themes_.size()) reload(selected_key());   // z. B. Theme aus dem Store installiert
    }
    void hidden(Host&) override {
        visible_ = false;
        again_ = false;
        ++preview_generation_;
    }

private:
    Host* host_ = nullptr;
    Combo* combo_ = nullptr;
    Note *info_ = nullptr, *problems_ = nullptr, *warn_ = nullptr;
    Card* editor_ = nullptr;
    Button *duplicate_ = nullptr, *remove_ = nullptr, *export_ = nullptr;
    TextField* name_ = nullptr;
    Segment* base_ = nullptr;
    Combo* style_ = nullptr;
    Toggle* dark_ = nullptr;
    std::vector<std::pair<std::string, ColorField*>> fields_;
    std::vector<th::Custom> themes_;
    std::vector<std::string> keys_;
    std::string cfg_theme_;
    bool previewing_ = false, again_ = false, visible_ = false;
    uint64_t preview_generation_ = 0;
    // Store-Themes (signierter Index; Aufrufe laufen im Arbeits-Thread)
    Label* store_status_ = nullptr;
    ItemList* store_list_ = nullptr;
    Button *store_refresh_ = nullptr, *store_install_ = nullptr;
    NativeStoreState store_;
    std::string store_selected_;
    bool store_loaded_ = false;

    const NativeThemeCard* store_theme() const {
        auto it = std::find_if(store_.themes.begin(), store_.themes.end(), [this](auto& t) { return t.id == store_selected_; });
        return it == store_.themes.end() ? nullptr : &*it;
    }
    Text store_state_text() const {
        if (store_.state == "disabled")
            return T(L"Der Plugin-Store ist ausgeschaltet. Unter „Plugin-Store“ einschalten und „Speichern & anwenden“.",
                     L"The plugin store is switched off. Switch it on under “Plugin store” and “Save & apply”.");
        if (store_.state == "unavailable")
            return T(L"Der Store ist in dieser Version noch nicht verfügbar (kein Signaturschlüssel hinterlegt).", L"The store is not available in this version yet (no signing key pinned).");
        if (store_.state == "invalid")
            return T(L"Der Store lieferte ungültige oder veraltete Daten. Es werden die zuletzt geprüften Daten gezeigt.",
                     L"The store returned invalid or outdated data. The last verified data is shown.");
        if (store_.state == "offline")
            return T(L"Der Store ist nicht erreichbar und es gibt noch keine geprüften Daten.", L"The store is not reachable and there is no verified data yet.");
        return store_.from_cache ? T(L"Geprüfte Daten aus dem Zwischenspeicher (Store zuletzt nicht erreichbar oder noch nicht abgefragt).",
                                     L"Verified data from the cache (store not reachable or not queried yet).")
                                 : T(L"Verbunden. Index und Sperrliste sind geprüft.", L"Connected. Index and revocation list verified.");
    }
    void update_store_buttons() {
        if (!store_install_) return;
        auto& a = host_->plugins();
        const bool idle = !host_->busy();
        const bool live = store_.state == "available" || store_.state == "invalid" || store_.state == "offline";
        const auto* t = store_theme();
        store_refresh_->enabled = idle && bool(a.store_refresh) && store_.state != "unavailable" && store_.state != "disabled";
        store_install_->enabled = idle && live && t && t->status == "not_installed" && bool(a.store_theme_install);
        store_install_->text = t && t->status == "installed" ? T(L"Bereits installiert", L"Already installed") : T(L"Theme installieren", L"Install theme");
        host_->surface().invalidate();
    }
    void fill_store() {
        if (!store_list_) return;
        store_status_->text = store_state_text();
        std::vector<ItemList::Item> items;
        for (const auto& t : store_.themes) {
            const std::wstring meta = widen(t.author + " · " + t.license + (t.description.empty() ? "" : " · " + t.description));
            const auto st = t.status == "installed" ? T(L"installiert", L"installed") : T(L"nicht installiert", L"not installed");
            items.push_back({t.id, Text(widen(t.name + "  " + t.version)), Text(meta + L" · " + st.get(false), meta + L" · " + st.get(true)), t.status == "installed"});
        }
        store_list_->items = items;
        store_list_->original = items;
        store_list_->selected = -1;
        for (size_t i = 0; i < store_.themes.size(); ++i) if (store_.themes[i].id == store_selected_) store_list_->selected = int(i);
        if (store_list_->selected < 0) store_selected_.clear();
        update_store_buttons();
    }
    void load_store(bool force) {
        auto& a = host_->plugins();
        if (host_->busy() || !a.store_list) return;
        Host* h = host_;
        h->run([this, h, force]() -> std::function<void()> {
            auto& actions = h->plugins();
            if (actions.store_refresh) actions.store_refresh(force);   // höchstens einmal am Tag, erzwungen höchstens einmal pro Minute
            NativeStoreState result = actions.store_list("", "");
            return [this, result = std::move(result)] { store_ = result; store_loaded_ = true; fill_store(); };
        });
        update_store_buttons();
    }
    void install_store_theme() {
        const auto* sel = store_theme();
        if (host_->busy() || !sel || sel->status != "not_installed" || !host_->plugins().store_theme_install) return;
        Host* h = host_;
        h->run([this, h, id = sel->id, name = sel->name]() -> std::function<void()> {
            std::string error;
            try { h->plugins().store_theme_install(id); } catch (const std::exception& e) { error = e.what(); }
            NativeStoreState result = h->plugins().store_list("", "");
            return [this, h, error, name, result = std::move(result)] {
                store_ = result;
                store_loaded_ = true;
                fill_store();
                if (!error.empty()) { h->toast(Text(widen(error)), ToastKind::Error); return; }
                std::string pick = selected_key();
                for (const auto& t : th::Store().list())
                    if (t.name == name) pick = std::string(th::CUSTOM_PREFIX) + t.id;
                reload(pick);
                h->toast(T(L"Theme installiert und ausgewählt. Mit „Speichern & anwenden“ übernehmen.", L"Theme installed and selected. Keep it with “Save & apply”."), ToastKind::Success);
            };
        });
        update_store_buttons();
    }

    bool en() const { return host_->english(); }
    std::string selected_key() const {
        return combo_ && combo_->index >= 0 && combo_->index < int(keys_.size()) ? keys_[size_t(combo_->index)] : "hell";
    }
    const th::Custom* selected_custom() const {
        const std::string key = selected_key();
        if (key.rfind(th::CUSTOM_PREFIX, 0) != 0) return nullptr;
        const std::string id = key.substr(std::string(th::CUSTOM_PREFIX).size());
        for (const auto& t : themes_) if (t.id == id) return &t;
        return nullptr;
    }
    int index_of(const std::string& key) const {
        auto it = std::find(keys_.begin(), keys_.end(), key);
        return it == keys_.end() ? -1 : int(it - keys_.begin());
    }

    void reload(const std::string& select) {
        std::vector<std::string> problems;
        themes_ = th::Store().list(&problems);
        keys_ = {"hell", "dunkel", "windows"};
        std::vector<Text> labels = {T(L"Hell (FuchsLabs)", L"Light (FuchsLabs)"), T(L"Dunkel (FuchsLabs)", L"Dark (FuchsLabs)"),
                                    T(L"An Windows anpassen", L"Match Windows")};
        for (const auto& t : themes_) {
            keys_.push_back(std::string(th::CUSTOM_PREFIX) + t.id);
            labels.push_back(Text(widen(t.name)));
        }
        combo_->options = labels;
        int idx = index_of(select);
        combo_->index = idx < 0 ? 0 : idx;
        int saved = index_of(cfg_theme_);
        combo_->original = saved < 0 ? combo_->index : saved;
        if (problems.empty()) {
            problems_->visible = false;
        } else {
            problems_->visible = true;
            problems_->text = Text(L"Diese Theme-Dateien wurden ignoriert (ungültig): " + widen(fl::join(problems, "; ")),
                                   L"These theme files were ignored (invalid): " + widen(fl::join(problems, "; ")));
        }
        selection_changed();
    }

    void selection_changed() {
        update_info();
        rebuild_editor();
        const bool custom = selected_custom() != nullptr;
        if (duplicate_) duplicate_->enabled = true;
        if (remove_) remove_->enabled = custom;
        if (export_) export_->enabled = custom;
        host_->relayout();
        preview();
    }

    void update_info() {
        const std::string key = selected_key();
        if (key != th::WINDOWS_ID) { info_->visible = false; return; }
        const auto w = th::read_windows_settings();
        info_->visible = true;
        if (w.high_contrast) {
            info_->text = T(L"Windows-Kontrastdesign aktiv: Das Display nutzt die Systemfarben (Hintergrund, Text, Akzent). "
                            L"Es folgt Änderungen sofort; Kopfzeile und Balken verwenden dann die Akzentfarbe.",
                            L"Windows high-contrast mode is on: the display uses the system colors (background, text, accent). "
                            L"It follows changes right away; header and bars then use the accent color.");
        } else {
            std::wstring accent = w.has_accent ? widen(strprintf("#%02X%02X%02X", w.accent.r, w.accent.g, w.accent.b)) : L"–";
            info_->text = Text(std::wstring(L"Aktuell von Windows: ") + (w.light ? L"Hell" : L"Dunkel") + L" · Akzent " + accent +
                               L". Das Display folgt Änderungen live. Kopfzeile und Fuchs-Logo bleiben Fuchs-Orange; "
                               L"Text und Balken werden bei zu geringem Kontrast automatisch angepasst.",
                               std::wstring(L"Currently from Windows: ") + (w.light ? L"light" : L"dark") + L" · accent " + accent +
                               L". The display follows changes live. Header and fox logo stay fox orange; "
                               L"text and bars are adjusted automatically when contrast is too low.");
        }
    }

    // ---------------------------------------------------------------- Editor
    void rebuild_editor() {
        editor_->clear();
        fields_.clear();
        name_ = nullptr; base_ = nullptr; style_ = nullptr; dark_ = nullptr; warn_ = nullptr;
        const th::Custom* c = selected_custom();
        if (!c) {
            editor_->title = T(L"Theme-Editor", L"Theme editor");
            editor_->desc = T(L"Wähle oben ein eigenes Theme oder lege mit „Neu“ oder „Duplizieren“ eines an. "
                              L"„Duplizieren“ auf „An Windows anpassen“ friert die aktuellen Windows-Farben als eigenes Theme ein.",
                              L"Pick one of your themes above, or create one with “New” or “Duplicate”. "
                              L"“Duplicate” on “Match Windows” freezes the current Windows colors as your own theme.");
            return;
        }
        editor_->title = Text(L"Theme bearbeiten: " + widen(c->name), L"Edit theme: " + widen(c->name));
        editor_->desc = T(L"Farben als #RRGGBB oder über das Farbfeld. Die Vorschau rechts zeigt Änderungen sofort; "
                          L"gespeichert wird erst mit „Theme speichern“.",
                          L"Colors as #RRGGBB or via the color box. The preview on the right shows changes right away; "
                          L"they are only kept with “Save theme”.");
        name_ = editor_->row<TextField>(T(L"Name", L"Name"), T(L"1–40 Zeichen, ohne < > \" \\ / & `", L"1–40 characters, without < > \" \\ / & `"),
                                        TextField::Single, widen(c->name));
        name_->limit = 40;
        name_->track_dirty = false;
        name_->on_change = [this] { changed(); };
        base_ = editor_->row<Segment>(T(L"Basis", L"Base"), T(L"Designprofile: lädt die Hell-/Dunkel-Palette und ersetzt die neun Farbwerte. FuchsLabs: nur Zusatzfarben.", L"Design profiles: loads the light/dark palette and replaces all nine colors. FuchsLabs: extra colors only."),
                                      std::vector<Text>{T(L"Hell", L"Light"), T(L"Dunkel", L"Dark")}, c->base == "dunkel" ? 1 : 0);
        base_->track_dirty = false;
        base_->on_change = [this] { apply_profile_palette(); changed(); };
        style_ = editor_->row<Combo>(T(L"Designsystem", L"Design system"),
            T(L"Formen, Abstände, Schrift und Interaktionsfeedback; das Einstellungsfenster folgt nach Anwenden.", L"Shapes, spacing, typography and interaction feedback; settings follow after applying."),
            std::vector<Text>{Text(L"FuchsLabs"), Text(L"Cupertino"), Text(L"Fluent"), Text(L"Material")}, int(c->style));
        style_->track_dirty = false;
        style_->on_change = [this] { apply_profile_palette(); changed(); };
        dark_ = editor_->row<Toggle>(T(L"Dunkles Design", L"Dark design"), T(L"Kennzeichnet das Theme als dunkel", L"Marks the theme as dark"), c->dark);
        dark_->track_dirty = false;
        dark_->on_change = [this] {
            if (style_ && style_->index != 0) { base_->index = dark_->on ? 1 : 0; apply_profile_palette(); }
            changed();
        };
        for (const char* key : th::COLOR_KEYS) {
            Color col = *c->colors.slot(key);
            ColorField* f = editor_->row<ColorField>(color_name(key), Text(), strprintf("#%02X%02X%02X", col.r, col.g, col.b));
            f->name = color_name(key);
            f->track_dirty = false;
            f->field.track_dirty = false;
            f->field.on_change = [this] { changed(); };
            fields_.push_back({key, f});
        }
        warn_ = editor_->add<Note>(Text(), Note::Info);
        auto row = editor_->add<ButtonRow>();
        row->add(T(L"Theme speichern", L"Save theme"), Button::Primary, [this] { save_theme(); });
        row->add(T(L"Kontrast korrigieren", L"Fix contrast"), Button::Secondary, [this] { fix_contrast(); });
        row->add(T(L"Zurücksetzen", L"Reset"), Button::Ghost, [this] { rebuild_editor(); host_->relayout(); preview(); });
        update_warnings();
    }

    std::optional<th::Custom> edited(std::string& err) const {
        if (!name_) { err = "-"; return std::nullopt; }
        ojson j = {{"version", th::FORMAT_VERSION}, {"name", narrow(name_->text())}, {"base", base_->index == 1 ? "dunkel" : "hell"},
                   {"dark", dark_->on}, {"colors", ojson::object()}};
        j["style"] = th::style_name(static_cast<th::Style>(style_->index));
        for (auto& [key, f] : fields_) j["colors"][key] = f->hex();
        auto t = th::parse(j, err);
        if (t) if (const th::Custom* sel = selected_custom()) t->id = sel->id;
        return t;
    }

    void update_warnings() {
        if (!warn_) return;
        std::string err;
        auto t = edited(err);
        if (!t) {
            warn_->kind = Note::Warn;
            warn_->text = T(L"Bitte Name (1–40 Zeichen) und alle Farben als #RRGGBB eingeben.",
                            L"Please enter a name (1–40 characters) and all colors as #RRGGBB.");
        } else {
            const auto warn = th::contrast_warnings(t->colors);
            if (warn.empty()) {
                warn_->kind = Note::Info;
                warn_->text = T(L"Alles gut lesbar. Die Schrift auf der Kopfzeile wählt die App selbst (Navy oder Weiß).",
                                L"Everything is easy to read. The app picks the header text color itself (navy or white).");
            } else {
                std::wstring de, en_;
                for (const auto& w : warn) {
                    auto a = color_name(w.a), b = color_name(w.b);
                    std::wstring ratio = widen(format_num(w.ratio, 1)), ratio_en = widen(strprintf("%.1f", w.ratio));
                    de += (de.empty() ? L"" : L"; ") + a.de + L" auf " + b.de + L" (" + ratio + L":1)";
                    en_ += (en_.empty() ? L"" : L"; ") + a.en + L" on " + b.en + L" (" + ratio_en + L":1)";
                }
                warn_->kind = Note::Warn;
                warn_->text = Text(L"Schwer lesbar: " + de + L". Mehr Unterschied zwischen hell und dunkel wählen oder „Kontrast korrigieren“.",
                                   L"Hard to read: " + en_ + L". Choose more difference between light and dark, or use “Fix contrast”.");
            }
        }
    }

    void apply_profile_palette() {
        if (!style_ || style_->index == 0 || !dark_) return;
        dark_->on = base_->index == 1;
        const auto palette = th::profile_palette(static_cast<th::Style>(style_->index), dark_->on);
        for (auto& [key, field] : fields_) {
            const auto color = *palette.slot(key);
            field->set_hex(strprintf("#%02X%02X%02X", color.r, color.g, color.b));
        }
    }

    void changed() {
        update_warnings();
        host_->relayout();
        preview();
    }

    void fix_contrast() {
        std::string err;
        auto t = edited(err);
        if (!t) { host_->toast(T(L"Erst Name und Farben korrigieren.", L"Fix the name and colors first."), ToastKind::Error); return; }
        const auto fixed = th::correct_contrast(t->colors);
        for (auto& [key, f] : fields_) {
            Color c = *fixed.slot(key);
            f->set_hex(strprintf("#%02X%02X%02X", c.r, c.g, c.b));
        }
        changed();
    }

    void save_theme() {
        std::string err;
        auto t = edited(err);
        if (!t) {
            host_->toast(T(L"Bitte Name (1–40 Zeichen) und alle Farben als #RRGGBB eingeben.", L"Please enter a name (1–40 characters) and all colors as #RRGGBB."), ToastKind::Error);
            return;
        }
        th::Custom c = *t;
        if (!th::Store().save(c, err)) { host_->toast(Text(L"Speichern nicht möglich: " + widen(err), L"Cannot save: " + widen(err)), ToastKind::Error); return; }
        host_->toast(T(L"Theme gespeichert.", L"Theme saved."), ToastKind::Success);
        host_->app().system_theme_changed();   // läuft das Theme gerade auf dem Display, sofort neu zeichnen
        reload(std::string(th::CUSTOM_PREFIX) + c.id);
    }

    // ---------------------------------------------------------------- Aktionen
    std::string fresh_name(const std::string& base) const {
        auto taken = [&](const std::string& n) { for (const auto& t : themes_) if (t.name == n) return true; return false; };
        std::string name = base;
        for (int i = 2; taken(name) && i < 100; ++i) name = base + " " + std::to_string(i);
        return name;
    }
    void create_new() {
        th::Custom c;
        c.name = fresh_name(en() ? "New theme" : "Neues Theme");
        c.base = "hell";
        c.dark = false;
        c.colors = th::builtin(false);
        std::string err;
        if (!th::Store().save(c, err)) { host_->toast(Text(L"Anlegen nicht möglich: " + widen(err), L"Cannot create: " + widen(err)), ToastKind::Error); return; }
        reload(std::string(th::CUSTOM_PREFIX) + c.id);
    }
    void duplicate() {
        std::string err;
        const std::string key = selected_key();
        if (const th::Custom* sel = selected_custom()) {
            auto d = th::Store().duplicate(sel->id, err);
            if (!d) { host_->toast(Text(L"Duplizieren nicht möglich: " + widen(err), L"Cannot duplicate: " + widen(err)), ToastKind::Error); return; }
            reload(std::string(th::CUSTOM_PREFIX) + d->id);
            return;
        }
        th::Resolved r = th::resolve(key, th::Store());
        th::Custom c;
        c.name = fresh_name(key == "dunkel" ? (en() ? "Dark (copy)" : "Dunkel (Kopie)")
                            : key == th::WINDOWS_ID ? (en() ? "Windows (copy)" : "Windows (Kopie)") : (en() ? "Light (copy)" : "Hell (Kopie)"));
        c.base = r.base;
        c.dark = r.dark;
        c.colors = r.colors;
        if (!th::Store().save(c, err)) { host_->toast(Text(L"Duplizieren nicht möglich: " + widen(err), L"Cannot duplicate: " + widen(err)), ToastKind::Error); return; }
        reload(std::string(th::CUSTOM_PREFIX) + c.id);
    }
    void remove() {
        const th::Custom* sel = selected_custom();
        if (!sel) return;
        const std::wstring name = widen(sel->name);
        std::wstring prompt = en() ? L"Delete theme “" + name + L"”? This cannot be undone." : L"Theme „" + name + L"“ löschen? Das lässt sich nicht rückgängig machen.";
        if (MessageBoxW(host_->window(), prompt.c_str(), L"ScreenBuddy", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        std::string err;
        const std::string id = sel->id;
        if (!th::Store().remove(id, err)) { host_->toast(Text(L"Löschen nicht möglich: " + widen(err), L"Cannot delete: " + widen(err)), ToastKind::Error); return; }
        host_->toast(T(L"Theme gelöscht.", L"Theme deleted."), ToastKind::Success);
        reload("hell");
    }
    void import_file() {
        wchar_t filename[32768]{};
        OPENFILENAMEW picker{sizeof(picker)};
        picker.hwndOwner = host_->window();
        picker.lpstrFile = filename;
        picker.nMaxFile = DWORD(std::size(filename));
        picker.lpstrFilter = L"Theme (*.json)\0*.json\0\0";
        picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
        if (!GetOpenFileNameW(&picker)) {
            if (CommDlgExtendedError()) host_->toast(T(L"Dateiauswahl konnte nicht geöffnet werden.", L"Cannot open the file picker."), ToastKind::Error);
            return;
        }
        std::string err;
        auto t = th::Store().import_file(std::filesystem::path(filename), err);
        if (!t) { host_->toast(Text(L"Import abgelehnt: " + widen(err), L"Import rejected: " + widen(err)), ToastKind::Error); return; }
        host_->toast(T(L"Theme importiert.", L"Theme imported."), ToastKind::Success);
        reload(std::string(th::CUSTOM_PREFIX) + t->id);
    }
    void export_file() {
        const th::Custom* sel = selected_custom();
        if (!sel) return;
        wchar_t filename[MAX_PATH]{};
        wcsncpy_s(filename, widen(sel->id + ".json").c_str(), _TRUNCATE);
        OPENFILENAMEW picker{sizeof(picker)};
        picker.hwndOwner = host_->window();
        picker.lpstrFile = filename;
        picker.nMaxFile = DWORD(std::size(filename));
        picker.lpstrFilter = L"Theme (*.json)\0*.json\0\0";
        picker.lpstrDefExt = L"json";
        picker.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
        if (!GetSaveFileNameW(&picker)) return;
        std::string err;
        if (!th::Store().export_file(sel->id, std::filesystem::path(filename), err)) {
            host_->toast(Text(L"Export nicht möglich: " + widen(err), L"Cannot export: " + widen(err)), ToastKind::Error);
            return;
        }
        host_->toast(T(L"Theme exportiert.", L"Theme exported."), ToastKind::Success);
    }

    // Live-Vorschau im Vorschaubereich; läuft nie doppelt, Änderungen währenddessen lösen genau einen Folgeaufruf aus.
    void preview() {
        if (!visible_) return;
        ojson display = {{"theme", selected_key()}};
        if (name_) {
            std::string err;
            if (auto t = edited(err)) display = {{"theme", selected_key()}, {"theme_inline", th::to_json(*t)}};
            else return;   // ungültige Eingabe: letzte Vorschau stehen lassen
        }
        if (previewing_) { again_ = true; return; }
        previewing_ = true;
        again_ = false;
        Host* h = host_;
        const auto generation = preview_generation_;
        h->run([this, h, display, generation]() -> std::function<void()> {
            auto frame = h->app().preview_theme(display);
            return [this, h, frame, generation] {
                previewing_ = false;
                if (visible_ && generation == preview_generation_ && frame) h->preview(frame);
                if (visible_ && again_) preview();
            };
        });
    }
};

}  // namespace

std::unique_ptr<Page> make_design_page() { return std::make_unique<DesignPage>(); }

}  // namespace fl::ui
