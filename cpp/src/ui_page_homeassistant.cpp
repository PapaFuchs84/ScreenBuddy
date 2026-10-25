// © 2026 FuchsLabs
// Datenquelle Home Assistant (Adresse, Zugriffstoken, bis zu 4 Entitäten).
#include "ui_pages.h"

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
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "homeassistant", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "homeassistant", "title", T(L"Titel", L"Title"));
        Card* conn = root.add<Card>(T(L"Verbindung", L"Connection"),
                                    T(L"Die Werte werden alle 15 Sekunden abgefragt.", L"Values are polled every 15 seconds."));
        state_ = conn->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        form_.text(*conn, "homeassistant", "url", T(L"Adresse", L"URL"), {}, Text(L"http://homeassistant.local:8123"));
        form_.secret(*conn, "homeassistant", "token", T(L"Zugriffstoken", L"Access token"),
                     T(L"Langlebiges Token; leer lassen = gespeichertes behalten", L"Long-lived token; leave empty to keep the saved one"));
        Card* ent = root.add<Card>(T(L"Entitäten", L"Entities"));
        form_.list(*ent, "homeassistant", "entities", T(L"Entitäts-IDs", L"Entity IDs"),
                   T(L"Höchstens 4, eine pro Zeile (z. B. sensor.wohnzimmer_temperatur)", L"Up to 4, one per line (e.g. sensor.living_room_temperature)"), 4);
        placeholder(host, root, T(L"Entitäten auswählen", L"Pick entities"),
                    T(L"Auswahlliste aller Entitäten aus Home Assistant mit Suche.", L"Searchable list of all entities from Home Assistant."));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        if (!state_ || !s.valid) return;
        Label::Tone tone = Label::Muted;
        state_->text = connection_text(s.connections.contains("homeassistant") ? s.connections.at("homeassistant") : ojson(), tone);
        state_->tone = tone;
    }
private:
    Form form_;
    Label* state_ = nullptr;
};

}  // namespace
std::unique_ptr<Page> make_homeassistant_page() { return std::make_unique<HomeAssistantPage>(); }
}  // namespace fl::ui
