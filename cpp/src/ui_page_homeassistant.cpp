// © 2026 FuchsLabs
// Datenquelle Home Assistant (Adresse, Zugriffstoken, bis zu 4 Entitäten).
#include "ui_pages.h"

#include <algorithm>

#include "app.h"
#include "ha.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

class HomeAssistantPage : public Page {
public:
    PageInfo info() const override {
        return {"homeassistant", Group::Sources, Text(L"Home Assistant"),
                T(L"Sensoren und Schalter aus deinem Smart Home", L"Sensors and switches from your smart home")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "homeassistant", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "homeassistant", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "homeassistant", "title", T(L"Titel", L"Title"));
        Card* conn = root.add<Card>(T(L"Verbindung", L"Connection"),
                                    T(L"Die Werte werden alle 15 Sekunden abgefragt.", L"Values are polled every 15 seconds."));
        state_ = conn->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        url_ = form_.text(*conn, "homeassistant", "url", T(L"Adresse", L"URL"), {}, Text(L"http://homeassistant.local:8123"));
        token_ = form_.secret(*conn, "homeassistant", "token", T(L"Zugriffstoken", L"Access token"),
                              T(L"Langlebiges Token; leer lassen = gespeichertes behalten", L"Long-lived token; leave empty to keep the saved one"));
        result_ = conn->row<Label>(T(L"Prüfung", L"Check"), Text(), Text(L"–"), Font::Small, Label::Muted);
        check_ = conn->add<ButtonRow>()->add(T(L"Verbindung prüfen & Entitäten laden", L"Check connection & load entities"),
                                             Button::Secondary, [this] { load(); });

        Card* ent = root.add<Card>(T(L"Entitäten", L"Entities"),
                                   T(L"Höchstens 4. Sensoren zeigen den Wert, Schalter (nur mit Touch) lassen sich per Tipp umlegen.",
                                     L"Up to 4. Sensors show the value, switches (touch only) can be toggled by tapping."));
        entities_ = form_.list(*ent, "homeassistant", "entities", T(L"Entitäts-IDs", L"Entity IDs"),
                               T(L"Eine pro Zeile, z. B. sensor.wohnzimmer_temperatur – optional mit Anzeigename: sensor.x|Wohnzimmer",
                                 L"One per line, e.g. sensor.living_room_temperature – optionally with a name: sensor.x|Living room"), 4);
        entities_->on_change = [this] { if (!syncing_) refill(); };

        Card* pick = root.add<Card>(T(L"Entitäten auswählen", L"Pick entities"),
                                    T(L"Zuerst oben „Verbindung prüfen & Entitäten laden“. Haken setzen trägt die ID oben ein.",
                                      L"First press “Check connection & load entities” above. Checking an entry adds its ID above."));
        info_ = pick->row<Label>(T(L"Liste", L"List"), Text(), T(L"Noch nicht geladen.", L"Not loaded yet."), Font::Small, Label::Muted);
        search_ = pick->row<TextField>(T(L"Suche", L"Search"), Text(), TextField::Single, L"");
        search_->width = 300;
        search_->track_dirty = false;
        search_->hint = T(L"Entität suchen …", L"Search entities …");
        search_->name = T(L"Entitäten suchen", L"Search entities");
        search_->on_change = [this] { refill(); };
        list_ = pick->add<ItemList>(std::vector<ItemList::Item>{});
        list_->name = T(L"Entitäten", L"Entities");
        list_->reorder = false;
        list_->track_dirty = false;
        list_->empty_text = T(L"Nichts gefunden.", L"Nothing found.");
        list_->on_change = [this] { sync_from_list(); };
        refill(false);
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        if (check_) check_->enabled = !host_->busy();
        if (!state_ || !s.valid) return;
        Label::Tone tone = Label::Muted;
        state_->text = connection_text(s.connections.contains("homeassistant") ? s.connections.at("homeassistant") : ojson(), tone);
        state_->tone = tone;
    }

private:
    struct Entity { std::string id, name, state, unit; };
    Form form_;
    Host* host_ = nullptr;
    Label *state_ = nullptr, *result_ = nullptr, *info_ = nullptr;
    TextField *url_ = nullptr, *token_ = nullptr, *entities_ = nullptr, *search_ = nullptr;
    ItemList* list_ = nullptr;
    Button* check_ = nullptr;
    std::vector<Entity> loaded_;
    bool syncing_ = false, touch_ = false;

    void load() {
        // Eingegebene Werte; leere ersetzt der Arbeits-Thread durch die gespeicherten
        std::string url = trim(narrow(url_->text())), token = trim(narrow(token_->text()));
        result_->text = T(L"Prüfe …", L"Checking …");
        result_->tone = Label::Muted;
        Host* h = host_;
        const ojson& cfg = h->config();
        const bool companion = flag(cfg, "companion", "enabled") || cfg.at("display").value("output", "") == "network";
        h->run([this, h, url, token, companion]() mutable -> std::function<void()> {
            const ojson saved = h->app().cfg()["homeassistant"];
            auto str = [&](const char* k) { return saved.contains(k) && saved[k].is_string() ? trim(saved[k].get<std::string>()) : std::string(); };
            if (url.empty()) url = str("url");
            if (token.empty()) token = str("token");
            const bool touch = h->app().touch_enabled() || companion;
            std::vector<Entity> entities;
            Text message;
            Label::Tone tone = Label::Danger;
            try {
                const auto list = ha::list_entities(url, token, touch);
                for (const auto& e : list)
                    entities.push_back({e.value("id", ""), e.value("name", ""), e.value("state", ""), e.value("unit", "")});
                message = Text(L"Verbunden: " + std::to_wstring(entities.size()) + L" Entitäten gefunden",
                               L"Connected: " + std::to_wstring(entities.size()) + L" entities found");
                tone = Label::Success;
            } catch (const ha::HaError& e) {
                message = Text(widen(e.what()));
            }
            token.clear();
            const bool ok = tone == Label::Success;
            return [this, entities = std::move(entities), message, tone, touch, ok]() mutable {
                result_->text = message;
                result_->tone = tone;
                if (ok) { loaded_ = std::move(entities); touch_ = touch; }
                refill();
            };
        });
    }
    void refill(bool layout = true) {
        if (!list_ || !entities_ || !search_) return;
        auto lines = logic::lines_of(narrow(entities_->text()));
        std::vector<std::string> names;
        std::vector<bool> picked;
        for (const auto& e : loaded_) {
            names.push_back(e.name + " " + e.id);
            picked.push_back(logic::has_entity(lines, e.id));
        }
        const auto order = logic::filter_indices(names, picked, narrow(search_->text()), 40);
        std::vector<ItemList::Item> items;
        for (size_t i : order) {
            const Entity& e = loaded_[i];
            std::wstring detail = widen(e.id + " · " + e.state + (e.unit.empty() ? "" : " " + e.unit));
            items.push_back({e.id, Text(widen(e.name)), Text(detail), picked[i]});
        }
        list_->items = items;
        list_->original = items;
        if (list_->selected >= int(items.size()) || (list_->selected < 0 && !items.empty())) list_->selected = items.empty() ? -1 : 0;
        if (loaded_.empty())
            info_->text = T(L"Noch nicht geladen.", L"Not loaded yet.");
        else {
            std::wstring more = loaded_.size() > items.size() ? L" – Suche nutzen" : L"";
            std::wstring more_en = loaded_.size() > items.size() ? L" – use the search" : L"";
            info_->text = Text(std::to_wstring(loaded_.size()) + (touch_ ? L" Sensoren und Schalter" : L" Sensoren (Schalter erscheinen mit Touch)") + more,
                               std::to_wstring(loaded_.size()) + (touch_ ? L" sensors and switches" : L" sensors (switches appear with touch)") + more_en);
        }
        if (layout) host_->relayout();
    }
    void sync_from_list() {
        auto lines = logic::lines_of(narrow(entities_->text()));
        bool full = false;
        for (auto& item : list_->items) {
            if (item.enabled) {
                if (logic::add_entity(lines, item.key, ha::MAX_ENTITIES) == "full") { item.enabled = false; full = true; }
            } else {
                logic::remove_entity(lines, item.key);
            }
        }
        if (full) host_->toast(T(L"Es passen höchstens 4 Entitäten auf die Seite.", L"At most 4 entities fit on the page."), ToastKind::Error);
        syncing_ = true;
        entities_->set_text(widen(logic::lines_join(lines)));
        syncing_ = false;
    }
};

}  // namespace
std::unique_ptr<Page> make_homeassistant_page() { return std::make_unique<HomeAssistantPage>(); }
}  // namespace fl::ui
