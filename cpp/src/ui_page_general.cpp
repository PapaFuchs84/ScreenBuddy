// © 2026 FuchsLabs
// Allgemein: Übersicht, Anzeige, Seiten & Reihenfolge.
#include "ui_pages.h"

#include <algorithm>

#include "app.h"
#include "native_settings.h"
#include "util.h"

namespace fl::ui {
namespace {

class OverviewPage : public Page {
public:
    PageInfo info() const override {
        return {"overview", Group::General, T(L"Übersicht", L"Overview"),
                T(L"Zustand von Display, Quellen und Verbindungen", L"Status of display, sources and connections")};
    }
    bool active(const ojson&) const override { return true; }
    void build(Host& host, Column& root) override {
        host_ = &host;
        tiles_ = root.add<Tiles>();
        tiles_->name = T(L"Kennzahlen", L"Key figures");
        Card* conn = root.add<Card>(T(L"Verbindungen", L"Connections"), T(L"Wird jede Sekunde aktualisiert.", L"Refreshed every second."));
        static const char* keys[] = {"discord", "steam", "xbox", "homeassistant", "spotify"};
        static const wchar_t* names[] = {L"Discord", L"Steam", L"Xbox", L"Home Assistant", L"Spotify"};
        for (int i = 0; i < 5; ++i) {
            states_[i] = conn->row<Label>(Text(names[i]), Text(), T(L"…", L"…"), Font::Small, Label::Muted);
            states_[i]->wrap = true;
            keys_[i] = keys[i];
        }
        Card* jump = root.add<Card>(T(L"Schnellzugriff", L"Shortcuts"));
        auto row = jump->add<ButtonRow>();
        Host* h = &host;
        row->add(T(L"Anzeige", L"Display"), Button::Secondary, [h] { h->navigate("display"); });
        row->add(T(L"Seiten & Reihenfolge", L"Pages & order"), Button::Secondary, [h] { h->navigate("pages"); });
        row->add(T(L"Datenquellen", L"Data sources"), Button::Secondary, [h] { h->navigate("system"); });
        row->add(Text(L"Companion"), Button::Secondary, [h] { h->navigate("companion"); });
        row->add(Text(L"Plugins"), Button::Secondary, [h] { h->navigate("plugins"); });
        Card* setup = root.add<Card>(T(L"Einrichtung in 3 Schritten", L"Set up in 3 steps"),
            T(L"Wähle die Ausgabe, richte deine Inhalte ein und bestimme die Seiten auf dem Display.",
              L"Choose an output, set up your content, then choose the pages shown on the display."));
        auto steps = setup->add<ButtonRow>();
        steps->add(T(L"1 · Ausgabe wählen", L"1 · Choose output"), Button::Secondary,
            [h] { h->navigate("display"); });
        steps->add(T(L"2 · Datenquellen einrichten", L"2 · Set up data sources"), Button::Secondary,
            [h] { h->navigate("system"); });
        steps->add(T(L"3 · Seiten wählen", L"3 · Choose pages"), Button::Secondary,
            [h] { h->navigate("pages"); });
        root.add<Note>(T(L"Änderungen werden erst mit „Speichern & anwenden“ übernommen; ScreenBuddy startet dafür kurz neu. "
                         L"Strg+S speichert ebenfalls.",
                         L"Changes take effect with “Save & apply”; ScreenBuddy restarts briefly. Ctrl+S saves as well."));
        update(host.status());
    }
    void tick(Host&, const Status& s) override { update(s); }
private:
    Host* host_ = nullptr;
    Tiles* tiles_ = nullptr;
    Label* states_[5]{};
    const char* keys_[5]{};
    void update(const Status& s) {
        const ojson& cfg = host_->config();
        int sources = 0;
        for (const auto& k : config::PAGES) if (flag(cfg, k.c_str(), "enabled")) ++sources;
        int pages = cfg.at("display").at("visible_pages").is_array() ? int(cfg.at("display").at("visible_pages").size()) : 0;
        std::wstring size = s.size.first > 0 ? std::to_wstring(s.size.first) + L"×" + std::to_wstring(s.size.second) : L"–";
        tiles_->tiles = {
            {s.valid ? (s.display_ok ? T(L"Verbunden", L"Connected") : T(L"Getrennt", L"Offline")) : Text(L"…"), Text(L"Display")},
            {Text(size), T(L"Auflösung", L"Resolution")},
            {Text(std::to_wstring(sources) + L" / " + std::to_wstring(config::PAGES.size())), T(L"Aktive Quellen", L"Active sources")},
            {Text(std::to_wstring(pages)), T(L"Seiten auf dem Display", L"Pages on the display")},
        };
        for (int i = 0; i < 5; ++i) {
            if (!states_[i]) continue;
            Label::Tone tone = Label::Muted;
            ojson c = s.connections.is_object() && s.connections.contains(keys_[i]) ? s.connections.at(keys_[i]) : ojson();
            states_[i]->text = s.valid ? connection_text(c, tone) : T(L"Wird geladen …", L"Loading …");
            states_[i]->tone = tone;
        }
        host_->surface().relayout();
    }
};

class DisplayPage : public Page {
public:
    PageInfo info() const override {
        return {"display", Group::General, T(L"Anzeige", L"Display"),
                T(L"Display-Typ, Helligkeit und Seitenwechsel", L"Display type, brightness and page interval")};
    }
    bool active(const ojson&) const override { return true; }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        tiles_ = root.add<Tiles>();
        tiles_->name = T(L"Kennzahlen", L"Key figures");
        Card* card = root.add<Card>();
        output_ = form_.segment(*card, "display", "output", T(L"Display-Typ", L"Display type"),
            T(L"Kleines USB-Display, normaler Bildschirm oder Android/Netzwerk", L"Small USB display, regular monitor or Android/network"),
            {"serial", "monitor", "network"}, {Text(L"USB"), T(L"Monitor", L"Monitor"), T(L"Netzwerk", L"Network")});
        seconds_ = form_.slider(*card, "display", "page_seconds", T(L"Anzeigedauer pro Seite", L"Time per page"),
            T(L"3–600 Sekunden", L"3–600 seconds"), 3, 600, Text(L"s"));
        brightness_ = form_.slider(*card, "display", "brightness", T(L"Helligkeit", L"Brightness"),
            T(L"Display-Helligkeit (0–100)", L"Display brightness (0–100)"), 0, 100, Text(L"%"));
        brightness_->step = 5;
        form_.segment(*card, "display", "orientation", T(L"Ausrichtung", L"Orientation"),
            T(L"Falls das Bild auf dem Kopf steht", L"If the picture is upside down"),
            {"landscape", "reverse_landscape", "portrait", "reverse_portrait"},
            {T(L"Quer", L"Landscape"), T(L"Quer 180°", L"Landscape 180°"), T(L"Hoch", L"Portrait"), T(L"Hoch 180°", L"Portrait 180°")});
        Card* conn = root.add<Card>(T(L"Anschluss", L"Connection"));
        form_.text(*conn, "display", "port", T(L"COM-Port", L"COM port"),
            T(L"AUTO oder z. B. COM3 (nur USB-Display)", L"AUTO or e.g. COM3 (USB display only)"), Text(L"AUTO"));
        saved_monitor_ = host.config().at("display").value("monitor", std::string());
        monitor_values_.clear();
        monitor_ = conn->row<Combo>(T(L"Bildschirm", L"Monitor"),
            T(L"Automatisch oder einen erkannten Bildschirm wählen (nur HDMI/USB-C)",
              L"Choose automatic or a detected monitor (HDMI/USB-C only)"),
            std::vector<Text>{T(L"Automatisch (erster weiterer Bildschirm)", L"Automatic (first secondary monitor)")}, 0);
        monitor_->name = T(L"Bildschirm", L"Monitor");
        auto monitor_actions = conn->add<ButtonRow>();
        monitor_actions->add(T(L"Bildschirme neu suchen", L"Refresh monitors"), Button::Ghost,
            [this] { refresh_monitors(); refresh_touch(); });
        form_.toggle(*conn, "display", "reset_on_start", T(L"Beim Start zurücksetzen", L"Reset on startup"),
            T(L"Verhindert verschobene Bilder", L"Prevents shifted images"));
        form_.toggle(*conn, "display", "touch", T(L"Touch-Steuerung", L"Touch control"),
            T(L"Seiten per Tippen wechseln und Schalter bedienen", L"Switch pages and toggles by tapping"));
        form_.toggle(*conn, "display", "screen_off_on_exit", T(L"Display beim Beenden ausschalten", L"Turn display off on exit"));
        touch_card_ = root.add<Card>(T(L"Touch-Bildschirm zuordnen", L"Assign touchscreen"),
            T(L"Die Zuordnung speichert Windows für den angeschlossenen Touchscreen.",
              L"Windows stores the assignment for the connected touchscreen."));
        touch_status_ = touch_card_->add<Label>(T(L"Touch-Geräte werden geprüft …", L"Checking touch devices …"),
            Font::Small, Label::Muted);
        auto touch_actions = touch_card_->add<ButtonRow>();
        touch_setup_ = touch_actions->add(T(L"Windows-Assistent öffnen", L"Open Windows wizard"), Button::Secondary,
            [this] { setup_touch(); });
        touch_actions->add(T(L"Erneut prüfen", L"Check again"), Button::Ghost, [this] { refresh_touch(); });
        touch_card_->add<Note>(T(L"Im Windows-Assistenten mit Enter andere Bildschirme überspringen, dann den gewünschten "
                                 L"Touchscreen berühren. Esc bricht ab. Die Vollbildausgabe pausiert währenddessen. "
                                 L"Danach „Erneut prüfen“ wählen.",
                                 L"In the Windows wizard, press Enter to skip other screens, then touch the intended "
                                 L"screen. Esc cancels. Fullscreen output pauses during setup. Choose “Check again” afterwards."));
        for (Widget* w : std::initializer_list<Widget*>{output_, seconds_, brightness_}) w->on_change = [this] { update(); };
        monitor_->on_change = [this] { refresh_touch(); };
        refresh_monitors(false);
        update();
    }
    void collect(Host&, ojson& patch) override {
        form_.collect(patch);
        patch["display"]["monitor"] = monitor_values_.at(size_t(monitor_->index));
    }
private:
    Form form_;
    Host* host_ = nullptr;
    Tiles* tiles_ = nullptr;
    Segment* output_ = nullptr;
    Slider* seconds_ = nullptr;
    Slider* brightness_ = nullptr;
    Combo* monitor_ = nullptr;
    std::vector<std::string> monitor_values_;
    std::string saved_monitor_;
    Card* touch_card_ = nullptr;
    Label* touch_status_ = nullptr;
    Button* touch_setup_ = nullptr;
    void refresh_monitors(bool layout = true) {
        const std::string chosen = monitor_values_.empty() ? saved_monitor_ : monitor_values_.at(size_t(monitor_->index));
        std::vector<std::string> values{""};
        std::vector<Text> labels{T(L"Automatisch (erster weiterer Bildschirm)", L"Automatic (first secondary monitor)")};
        try {
            for (const auto& m : list_monitors()) {
                if (m.device.empty()) continue;
                if (std::find(values.begin(), values.end(), m.device) != values.end()) continue;
                values.push_back(m.device);
                const std::wstring name = m.name.empty() ? T(L"Bildschirm", L"Monitor").de : widen(m.name);
                const std::wstring size = L" · " + std::to_wstring(m.w) + L" × " + std::to_wstring(m.h);
                labels.emplace_back(name + size + (m.primary ? L" · Hauptbildschirm" : L"") + L" (" + widen(m.device) + L")",
                                    name + size + (m.primary ? L" · main screen" : L"") + L" (" + widen(m.device) + L")");
            }
        } catch (const std::exception&) {
            host_->toast(T(L"Bildschirme konnten nicht gelesen werden.", L"Could not read monitors."), ToastKind::Error);
        }
        for (const auto& id : {saved_monitor_, chosen}) {
            if (id.empty() || std::find(values.begin(), values.end(), id) != values.end()) continue;
            values.push_back(id);
            labels.emplace_back(widen(id) + L" (gerade nicht angeschlossen)", widen(id) + L" (currently disconnected)");
        }
        const auto index_of = [&values](const std::string& id) {
            const auto it = std::find(values.begin(), values.end(), id);
            return it == values.end() ? 0 : int(it - values.begin());
        };
        monitor_->options = std::move(labels);
        monitor_->index = index_of(chosen);
        monitor_->original = index_of(saved_monitor_);
        monitor_values_ = std::move(values);
        if (layout) host_->relayout();
    }
    void refresh_touch() {
        if (!touch_card_ || output_->index != 1) return;
        try {
            const ojson info = touch_info();
            const bool available = info.value("available", false);
            touch_setup_->enabled = available;
            if (!available) {
                touch_status_->text = T(L"Kein Touchscreen erkannt. USB-Touchkabel und Treiber prüfen; HDMI überträgt nur das Bild.",
                                        L"No touchscreen detected. Check the USB touch cable and driver; HDMI carries only the picture.");
                touch_status_->tone = Label::Danger;
            } else {
                std::string selected = monitor_values_.at(size_t(monitor_->index));
                if (selected.empty()) {
                    for (const auto& m : list_monitors()) if (!m.primary) { selected = m.device; break; }
                }
                bool mapped = false;
                if (info.contains("devices") && info["devices"].is_array())
                    for (const auto& device : info["devices"])
                        if (device.is_object() && device.value("monitor", "") == selected && !selected.empty()) mapped = true;
                if (mapped) {
                    touch_status_->text = T(L"Windows hat dem gewählten Bildschirm einen Touchscreen zugeordnet.",
                                            L"Windows has assigned a touchscreen to the selected display.");
                    touch_status_->tone = Label::Success;
                } else if (info.value("mapping_known", false)) {
                    touch_status_->text = T(L"Touch ist einem anderen Bildschirm zugeordnet. Starte den Windows-Assistenten.",
                                            L"Touch is assigned to another display. Start the Windows wizard.");
                    touch_status_->tone = Label::Danger;
                } else {
                    touch_status_->text = T(L"Windows meldet keine eindeutige Touch-Zuordnung. Bei falschem Ziel den Windows-Assistenten starten.",
                                            L"Windows does not report a reliable touch assignment. Start the Windows wizard if taps go to another screen.");
                    touch_status_->tone = Label::Muted;
                }
            }
        } catch (const std::exception&) {
            touch_setup_->enabled = false;
            touch_status_->text = T(L"Touch-Geräte konnten nicht geprüft werden.", L"Touch devices could not be checked.");
            touch_status_->tone = Label::Danger;
        }
        host_->relayout();
    }
    void setup_touch() {
        if (!touch_setup_->enabled || host_->busy()) return;
        touch_setup_->enabled = false;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            bool available = false, opened = false, failed = false;
            try {
                available = touch_info().value("available", false);
                if (available) opened = h->app().setup_touch();
            } catch (const std::exception&) { failed = true; }
            return [this, h, available, opened, failed] {
                if (failed)
                    h->toast(T(L"Touch-Einrichtung konnte nicht gestartet werden.",
                               L"Touch setup could not be started."), ToastKind::Error);
                else if (!available)
                    h->toast(T(L"Kein Touchscreen erkannt. USB-Touchkabel und Treiber prüfen.",
                               L"No touchscreen detected. Check the USB touch cable and driver."), ToastKind::Error);
                else if (!opened)
                    h->toast(T(L"Die Windows-Touch-Einrichtung konnte nicht geöffnet werden.",
                               L"Windows touch setup could not be opened."), ToastKind::Error);
                else
                    h->toast(T(L"Mit Enter zum Touchscreen wechseln, dann diesen berühren. Esc bricht ab.",
                               L"Press Enter until the touchscreen appears, then touch it. Esc cancels."), ToastKind::Info);
                refresh_touch();
            };
        });
    }
    void update() {
        const bool monitor_output = output_->index == 1;
        if (touch_card_->visible != monitor_output) {
            touch_card_->visible = monitor_output;
            host_->relayout();
        }
        if (monitor_output) refresh_touch();
        const ojson& vis = host_->config().at("display").at("visible_pages");
        tiles_->tiles = {
            {Text(std::to_wstring(seconds_->val) + L" s"), T(L"Seitenwechsel", L"Page interval")},
            {Text(std::to_wstring(brightness_->val) + L" %"), T(L"Helligkeit", L"Brightness")},
            {Text(std::to_wstring(vis.is_array() ? vis.size() : 0)), T(L"Aktive Seiten", L"Active pages")},
            {output_->options[size_t(output_->index)], T(L"Ausgabe", L"Output")},
        };
    }
};

class PagesPage : public Page {
public:
    PageInfo info() const override {
        return {"pages", Group::General, T(L"Seiten & Reihenfolge", L"Pages & order"),
                T(L"Welche Seiten das Display zeigt und in welcher Reihenfolge", L"Which pages the display shows and in which order")};
    }
    bool active(const ojson&) const override { return true; }
    void build(Host& host, Column& root) override {
        const ojson& dp = host.config().at("display");
        Card* card = root.add<Card>(T(L"Seiten auf USB-Display / Bildschirm", L"Pages on USB display / monitor"),
            T(L"Haken = anzeigen. Umsortieren mit ▲/▼ oder Alt+Pfeiltaste, Leertaste schaltet ein/aus.",
              L"Check = show. Reorder with ▲/▼ or Alt+arrow key; Space toggles."));
        list_ = card->add<ItemList>(page_items(dp.at("pages"), dp.at("visible_pages")));
        list_->name = T(L"Seiten und Reihenfolge", L"Pages and order");
        list_->selected = 0;
        root.add<Note>(T(L"Die markierten Seiten erscheinen in dieser Reihenfolge, wenn die zugehörige Quelle aktiv und eingerichtet ist. "
                         L"Aktivierte Plugins stehen ebenfalls in der Liste. Bei reiner Netzwerk-Ausgabe gilt die Liste unter Companion.",
                         L"Checked pages appear in this order when their source is enabled and configured. "
                         L"Enabled plugins are listed too. Network-only output uses the list under Companion."));
        host_ = &host;
        plugins_loaded_ = false;   // die Liste wurde neu gebaut (auch nach „Verwerfen“): aus der Konfiguration neu aufbauen
        load_plugins();
    }
    void shown(Host&) override { load_plugins(); }
    void collect(Host& host, ojson& patch) override {
        auto& dp = patch["display"];
        dp["pages"] = ojson::array();
        dp["visible_pages"] = ojson::array();
        for (const auto& item : list_->items) {
            dp["pages"].push_back(item.key);
            if (item.enabled) dp["visible_pages"].push_back(item.key);
        }
        if (!plugins_loaded_) {
            // Pluginliste nicht geladen: bisherige Plugin-Einträge unverändert mitschreiben, damit nichts verloren geht
            const ojson& cur = host.config().at("display");
            auto copy = [&](const char* key) {
                if (!cur.contains(key) || !cur.at(key).is_array()) return;
                for (const auto& k : cur.at(key))
                    if (k.is_string() && k.get<std::string>().rfind("plugin:", 0) == 0) dp[key].push_back(k);
            };
            copy("pages");
            copy("visible_pages");
        }
    }
private:
    Host* host_ = nullptr;
    ItemList* list_ = nullptr;
    bool plugins_loaded_ = false;
    void load_plugins() {
        if (!host_ || !list_ || !host_->plugins().list || host_->busy()) return;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            auto entries = h->plugins().list();
            return [this, h, entries = std::move(entries)] {
                std::vector<PluginPageInfo> plugins;
                for (const auto& e : entries)
                    if (e.enabled) plugins.push_back({"plugin:" + e.id, Text(widen(e.name))});
                const ojson& dp = h->config().at("display");
                const bool edited = plugins_loaded_ || list_->dirty();  // später nur abgleichen: config() zeigt den Stand beim Öffnen
                std::vector<ItemList::Item> items;
                if (!edited) {
                    items = page_items(dp.at("pages"), dp.at("visible_pages"), plugins);
                } else {
                    // Der Nutzer hat schon umsortiert: seine Liste behalten, Plugins nur abgleichen
                    for (auto& i : list_->items)
                        if (i.key.rfind("plugin:", 0) != 0 ||
                            std::any_of(plugins.begin(), plugins.end(), [&](auto& p) { return p.first == i.key; }))
                            items.push_back(i);
                    for (auto& p : plugins)
                        if (std::none_of(items.begin(), items.end(), [&](auto& i) { return i.key == p.first; }))
                            items.push_back({p.first, p.second, T(L"Plugin", L"Plugin"), true});
                }
                const int sel = list_->selected;
                list_->items = items;
                if (!edited) list_->original = items;
                list_->selected = sel < int(items.size()) ? sel : 0;
                plugins_loaded_ = true;
                h->relayout();   // Listenhöhe und nachfolgende Elemente an die neue Zeilenzahl anpassen
            };
        });
    }
};

}  // namespace

std::unique_ptr<Page> make_overview_page() { return std::make_unique<OverviewPage>(); }
std::unique_ptr<Page> make_display_page() { return std::make_unique<DisplayPage>(); }
std::unique_ptr<Page> make_pages_page() { return std::make_unique<PagesPage>(); }

}  // namespace fl::ui
