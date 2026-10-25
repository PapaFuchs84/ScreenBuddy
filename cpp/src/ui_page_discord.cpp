// © 2026 FuchsLabs
// Datenquelle Discord (Bot, Server, Personen).
#include "ui_pages.h"

namespace fl::ui {
namespace {

class DiscordPage : public Page {
public:
    PageInfo info() const override {
        return {"discord", Group::Sources, Text(L"Discord"),
                T(L"Bot-Zugang, Server und Personen der Discord-Seite", L"Bot access, servers and people of the Discord page")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "discord", "enabled") && flag(cfg, "discord", "token_set"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "discord", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "display", "title", T(L"Titel der Discord-Seite", L"Discord page title"));
        form_.toggle(*page, "discord", "show_offline", T(L"Offline-Personen anzeigen", L"Show offline people"));
        Card* conn = root.add<Card>(T(L"Verbindung", L"Connection"));
        state_ = conn->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        form_.secret(*conn, "discord", "token", T(L"Bot-Token", L"Bot token"),
                     T(L"Leer lassen = gespeichertes Token behalten", L"Leave empty to keep the saved token"));
        Card* ids = root.add<Card>(T(L"Server und Personen", L"Servers and people"),
                                   T(L"Zahlen-IDs, eine pro Zeile.", L"Numeric IDs, one per line."));
        form_.list(*ids, "discord", "guild_ids", T(L"Server-IDs", L"Server IDs"), {}, 3);
        form_.list(*ids, "discord", "friend_ids", T(L"Personen-IDs", L"Person IDs"), {}, 4);
        form_.list(*ids, "discord", "ignore_ids", T(L"Ausgeblendete IDs", L"Hidden IDs"), {}, 3);
        placeholder(host, root, T(L"Mitglieder auswählen", L"Pick members"),
                    T(L"Mitgliederliste des Servers mit Suche und Häkchen statt IDs von Hand.",
                      L"Server member list with search and checkboxes instead of typing IDs."));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        if (!state_ || !s.valid) return;
        Label::Tone tone = Label::Muted;
        state_->text = connection_text(s.connections.contains("discord") ? s.connections.at("discord") : ojson(), tone);
        state_->tone = tone;
    }
private:
    Form form_;
    Label* state_ = nullptr;
};

}  // namespace
std::unique_ptr<Page> make_discord_page() { return std::make_unique<DiscordPage>(); }
}  // namespace fl::ui
