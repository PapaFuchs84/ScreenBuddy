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
    bool active(const ojson& cfg) const override { return flag(cfg, "system", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "system", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "system", "title", T(L"Titel", L"Title"));
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
