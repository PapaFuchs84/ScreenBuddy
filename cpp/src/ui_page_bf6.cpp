// © 2026 FuchsLabs
// Datenquelle Battlefield 6 (Rangliste).
#include "ui_pages.h"

namespace fl::ui {
namespace {

class Bf6Page : public Page {
public:
    PageInfo info() const override {
        return {"bf6", Group::Sources, Text(L"Battlefield 6"),
                T(L"Rangliste deiner Mitspieler", L"Leaderboard of your fellow players")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "bf6", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "bf6", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "bf6", "title", T(L"Titel", L"Title"));
        form_.number(*page, "bf6", "refresh_minutes", T(L"Aktualisieren", L"Refresh"), T(L"5–240 Minuten", L"5–240 minutes"), 5, 240);
        Card* players = root.add<Card>(T(L"Spieler", L"Players"));
        form_.list(*players, "bf6", "players", T(L"EA-IDs", L"EA IDs"),
                   T(L"Eine pro Zeile; Konsole mit Zusatz @psn oder @xbox", L"One per line; console players with @psn or @xbox"), 5);
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
};

}  // namespace
std::unique_ptr<Page> make_bf6_page() { return std::make_unique<Bf6Page>(); }
}  // namespace fl::ui
