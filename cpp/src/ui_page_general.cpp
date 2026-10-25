// © 2026 FuchsLabs
// Allgemein: Übersicht, Anzeige, Seiten & Reihenfolge.
#include "ui_pages.h"

#include "app.h"
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
        static const char* keys[] = {"discord", "steam", "xbox", "homeassistant"};
        static const wchar_t* names[] = {L"Discord", L"Steam", L"Xbox", L"Home Assistant"};
        for (int i = 0; i < 4; ++i) {
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
        root.add<Note>(T(L"Änderungen werden erst mit „Speichern & anwenden“ übernommen; ScreenBuddy startet dafür kurz neu. "
                         L"Strg+S speichert ebenfalls.",
                         L"Changes take effect with “Save & apply”; ScreenBuddy restarts briefly. Ctrl+S saves as well."));
        update(host.status());
    }
    void tick(Host&, const Status& s) override { update(s); }
private:
    Host* host_ = nullptr;
    Tiles* tiles_ = nullptr;
    Label* states_[4]{};
    const char* keys_[4]{};
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
        for (int i = 0; i < 4; ++i) {
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
        form_.text(*conn, "display", "monitor", T(L"Monitor-ID", L"Monitor ID"),
            T(L"Leer = automatisch (nur Ausgabe auf Bildschirm)", L"Empty = automatic (monitor output only)"));
        form_.toggle(*conn, "display", "reset_on_start", T(L"Beim Start zurücksetzen", L"Reset on startup"),
            T(L"Verhindert verschobene Bilder", L"Prevents shifted images"));
        form_.toggle(*conn, "display", "touch", T(L"Touch-Steuerung", L"Touch control"),
            T(L"Seiten per Tippen wechseln und Schalter bedienen", L"Switch pages and toggles by tapping"));
        form_.toggle(*conn, "display", "screen_off_on_exit", T(L"Display beim Beenden ausschalten", L"Turn display off on exit"));
        for (Widget* w : std::initializer_list<Widget*>{output_, seconds_, brightness_}) w->on_change = [this] { update(); };
        update();
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
    Host* host_ = nullptr;
    Tiles* tiles_ = nullptr;
    Segment* output_ = nullptr;
    Slider* seconds_ = nullptr;
    Slider* brightness_ = nullptr;
    void update() {
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
                         L"Bei reiner Netzwerk-Ausgabe gilt die Liste unter Companion.",
                         L"Checked pages appear in this order when their source is enabled and configured. "
                         L"Network-only output uses the list under Companion."));
    }
    void collect(Host&, ojson& patch) override {
        auto& dp = patch["display"];
        dp["pages"] = ojson::array();
        dp["visible_pages"] = ojson::array();
        for (const auto& item : list_->items) {
            dp["pages"].push_back(item.key);
            if (item.enabled) dp["visible_pages"].push_back(item.key);
        }
    }
private:
    ItemList* list_ = nullptr;
};

}  // namespace

std::unique_ptr<Page> make_overview_page() { return std::make_unique<OverviewPage>(); }
std::unique_ptr<Page> make_display_page() { return std::make_unique<DisplayPage>(); }
std::unique_ptr<Page> make_pages_page() { return std::make_unique<PagesPage>(); }

}  // namespace fl::ui
