// © 2026 FuchsLabs
// Datenquelle Battlefield 6 (Rangliste).
#include "ui_pages.h"

#include "app.h"
#include "bf6.h"
#include "util.h"

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
        Card* check = root.add<Card>(T(L"Name prüfen", L"Check a name"),
                                     T(L"Zeigt sofort, ob die EA-ID gefunden wird (Netzabfrage, ohne zu speichern).",
                                       L"Shows right away whether the EA ID is found (network request, nothing is saved)."));
        name_ = check->row<TextField>(T(L"EA-ID", L"EA ID"), Text(), TextField::Single, L"");
        name_->track_dirty = false;
        name_->hint = Text(L"P4p4F0x");
        result_ = check->row<Label>(T(L"Ergebnis", L"Result"), Text(), Text(L"–"), Font::Small, Label::Muted);
        result_->width = 340;
        button_ = check->add<ButtonRow>()->add(T(L"Prüfen", L"Check"), Button::Secondary, [this, h = &host] { run_check(*h); });
    }
    void tick(Host& host, const Status&) override { if (button_) button_->enabled = !host.busy(); }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
    TextField* name_ = nullptr;
    Label* result_ = nullptr;
    Button* button_ = nullptr;
    void run_check(Host& host) {
        const std::string name = trim(narrow(name_->text()));
        if (name.empty()) {
            result_->text = T(L"Bitte einen Namen eintragen.", L"Please enter a name.");
            result_->tone = Label::Danger;
            return;
        }
        result_->text = T(L"Prüfe …", L"Checking …");
        result_->tone = Label::Muted;
        Host* h = &host;
        h->run([this, name]() -> std::function<void()> {
            bf6::Bf6Stats stats(ojson{{"players", ojson::array()}});
            Bf6Player p = stats.fetch(name);
            Text message;
            Label::Tone tone = Label::Danger;
            if (!p.error.empty()) message = T(L"Spieler nicht gefunden.", L"Player not found.");
            else {
                std::wstring kd = p.kd ? L", K/D " + widen(strprintf("%.2f", *p.kd)) : L"";
                message = Text(widen(p.name) + (p.rank_name.empty() ? L"" : L" · " + widen(p.rank_name)) + kd);
                tone = Label::Success;
            }
            return [this, message, tone] { result_->text = message; result_->tone = tone; };
        });
    }
};

}  // namespace
std::unique_ptr<Page> make_bf6_page() { return std::make_unique<Bf6Page>(); }
}  // namespace fl::ui
