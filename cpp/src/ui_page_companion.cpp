// © 2026 FuchsLabs
// Verbindung: Companion-App (Android) – Grundwerte und eigene Seitenauswahl.
#include "ui_pages.h"

namespace fl::ui {
namespace {

class CompanionPage : public Page {
public:
    PageInfo info() const override {
        return {"companion", Group::Connection, Text(L"Companion"),
                T(L"Android-App im Heimnetz: Ausgabe, Port und eigene Seitenauswahl",
                  L"Android app on your home network: output, port and its own page selection")};
    }
    bool active(const ojson& cfg) const override {
        return flag(cfg, "companion", "enabled") || cfg.at("display").value("output", "") == "network";
    }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        const ojson& companion = host.config().at("companion");
        Card* base = root.add<Card>(T(L"Companion-Ausgabe", L"Companion output"),
            T(L"Zusätzlich zum USB-Display bzw. Bildschirm. Änderungen gelten nach Speichern und Neustart.",
              L"In addition to the USB display or monitor. Changes apply after saving and restarting."));
        form_.toggle(*base, "companion", "enabled", T(L"Companion zusätzlich aktivieren", L"Enable additional Companion output"));
        form_.number(*base, "companion", "port", T(L"TLS-Port", L"TLS port"),
                     T(L"1024–65535; nie der Port der Einstellungen (8765)", L"1024–65535; never the settings port (8765)"), 1024, 65535);
        form_.slider(*base, "companion", "page_seconds", T(L"Seitenwechsel", L"Page interval"), T(L"3–600 Sekunden", L"3–600 seconds"),
                     3, 600, Text(L"s"));
        Card* pages = root.add<Card>(T(L"Seiten für Companion", L"Pages for Companion"),
            T(L"Eigene Auswahl und Reihenfolge. Haken = anzeigen; ▲/▼ oder Alt+Pfeiltaste sortiert um.",
              L"Own selection and order. Check = show; ▲/▼ or Alt+arrow key reorders."));
        list_ = pages->add<ItemList>(page_items(companion.at("pages"), companion.at("pages")));
        list_->name = T(L"Companion-Seiten", L"Companion pages");
        list_->selected = 0;
        placeholder(host, root, T(L"Geräte koppeln und QR-Code", L"Pair devices and QR code"),
                    T(L"Kopplung per QR-Code (2 Minuten gültig), gekoppelte Geräte anzeigen und entfernen.",
                      L"Pairing via QR code (valid for 2 minutes), list and remove paired devices."));
    }
    void collect(Host&, ojson& patch) override {
        form_.collect(patch);
        auto& pages = patch["companion"]["pages"] = ojson::array();
        for (const auto& item : list_->items) if (item.enabled) pages.push_back(item.key);
    }
private:
    Form form_;
    ItemList* list_ = nullptr;
};

}  // namespace
std::unique_ptr<Page> make_companion_page() { return std::make_unique<CompanionPage>(); }
}  // namespace fl::ui
