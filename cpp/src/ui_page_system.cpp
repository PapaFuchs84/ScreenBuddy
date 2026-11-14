// © 2026 FuchsLabs
// Datenquelle System (CPU, GPU, RAM, Netzwerk, Laufwerk).
#include "ui_pages.h"

namespace fl::ui {
namespace {

class SystemPage : public Page {
public:
    PageInfo info() const override {
        return {"system", Group::Sources, Text(L"System"),
                T(L"CPU, Grafikkarte, Speicher, Netzwerk und Laufwerk", L"CPU, graphics card, memory, network and disk")};
    }
    bool shows_status() const override { return true; }
    bool active(const ojson& cfg) const override { return flag(cfg, "system", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "system", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "system", "title", T(L"Titel", L"Title"));
        Card* hud = root.add<Card>(Text(L"Hardware HUD"),
            T(L"Animierte Systemansicht mit Vektorgrafik. Übersicht, GPU und CPU wechseln alle 8 Sekunden. Fehlende Sensorwerte erscheinen als --.",
              L"Animated system view with vector graphics. Overview, GPU and CPU rotate every 8 seconds. Missing sensors appear as --."));
        form_.toggle(*hud, "system", "hardware_hud", T(L"Hardware HUD anzeigen", L"Show Hardware HUD"));
        form_.segment(*hud, "system", "hud_colors", T(L"Farben", L"Colours"),
                      T(L"Design übernimmt die Farben des aktiven Designs; Neon ist der klassische Cyan-/Magenta-Look.",
                        L"Design uses the colours of the active design; Neon is the classic cyan/magenta look."),
                      {"theme", "neon", "custom"}, {T(L"Design", L"Design"), Text(L"Neon"), T(L"Eigene", L"Custom")});
        form_.color(*hud, "system", "hud_color_primary", T(L"Hauptfarbe", L"Primary colour"),
                    T(L"Nur bei „Eigene“. Leer = Akzentfarbe des Designs.", L"Custom only. Empty = design accent colour."));
        form_.color(*hud, "system", "hud_color_secondary", T(L"Zweitfarbe", L"Secondary colour"),
                    T(L"Nur bei „Eigene“. Leer = Kopfzeilenfarbe des Designs.", L"Custom only. Empty = design header colour."));
        form_.color(*hud, "system", "hud_color_background", T(L"Hintergrund", L"Background"),
                    T(L"Nur bei „Eigene“. Leer = Hintergrund des Designs. Zu schwach kontrastierende Farben werden für die Lesbarkeit angepasst.",
                      L"Custom only. Empty = design background. Colours with too little contrast are adjusted for readability."));
        hud->add<Note>(T(L"Die Animationsrate für alle Ansichten stellst du unter Anzeige → Animationen ein.",
                         L"Set the animation rate for all views under Display → Animations."));
        Card* sensors = root.add<Card>(T(L"Sensoren", L"Sensors"), T(L"Leere Felder = automatisch erkennen.", L"Empty fields = detect automatically."));
        form_.text(*sensors, "system", "net_adapter", T(L"Netzwerkadapter", L"Network adapter"), T(L"Leer = automatisch", L"Empty = automatic"));
        form_.segment(*sensors, "system", "net_unit", T(L"Netzwerkeinheit", L"Network unit"), {}, {"bit", "byte"},
                      {Text(L"bit/s"), Text(L"Byte/s")});
        form_.text(*sensors, "system", "disk", T(L"Laufwerk", L"Disk"), T(L"z. B. C:", L"e.g. C:"));
        form_.text(*sensors, "system", "cpu_temp_sensor", T(L"CPU-Temperatursensor", L"CPU temperature sensor"), T(L"Leer = automatisch", L"Empty = automatic"));
        form_.text(*sensors, "system", "cpu_fan_sensor", T(L"CPU-Lüftersensor", L"CPU fan sensor"), T(L"Leer = automatisch", L"Empty = automatic"));
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
};

}  // namespace
std::unique_ptr<Page> make_system_page() { return std::make_unique<SystemPage>(); }
}  // namespace fl::ui
