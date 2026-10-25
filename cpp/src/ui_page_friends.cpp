// © 2026 FuchsLabs
// Datenquelle Freunde (Steam, Xbox, Discord gemeinsam).
#include "ui_pages.h"

namespace fl::ui {
namespace {

class FriendsPage : public Page {
public:
    PageInfo info() const override {
        return {"friends", Group::Sources, T(L"Freunde", L"Friends"),
                T(L"Gemeinsame Freundesliste aus Steam, Xbox und Discord", L"Combined friends list from Steam, Xbox and Discord")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "friends", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "friends", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "friends", "title", T(L"Titel", L"Title"));
        form_.toggle(*page, "friends", "include_discord", T(L"Discord einbeziehen", L"Include Discord"));
        form_.toggle(*page, "friends", "auto_merge", T(L"Gleiche Namen automatisch verbinden", L"Automatically merge matching names"));
        form_.toggle(*page, "friends", "show_offline", T(L"Offline-Konten anzeigen", L"Show offline accounts"));
        Card* steam = root.add<Card>(Text(L"Steam"), T(L"Kostenlos und offiziell von Valve. Die Freundesliste im Steam-Profil muss öffentlich sein.",
                                                       L"Free and official from Valve. The friends list in your Steam profile must be public."));
        steam_state_ = steam->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        form_.secret(*steam, "friends", "steam_api_key", T(L"Steam-API-Schlüssel", L"Steam API key"),
                     T(L"Leer lassen = gespeicherten Schlüssel behalten", L"Leave empty to keep the saved key"));
        form_.text(*steam, "friends", "steam_id", T(L"Steam-ID oder Profiladresse", L"Steam ID or profile URL"));
        Card* xbox = root.add<Card>(Text(L"Xbox"), T(L"Über den kostenlosen Dienst OpenXBL (xbl.io), nicht direkt von Microsoft.",
                                                     L"Via the free OpenXBL service (xbl.io), not directly from Microsoft."));
        xbox_state_ = xbox->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        form_.secret(*xbox, "friends", "xbox_api_key", T(L"Xbox-API-Schlüssel", L"Xbox API key"),
                     T(L"Leer lassen = gespeicherten Schlüssel behalten", L"Leave empty to keep the saved key"));
        Card* links = root.add<Card>(T(L"Personen zusammenführen", L"Merge people"));
        form_.list(*links, "friends", "links", T(L"Verknüpfungen", L"Links"),
                   T(L"Eine Person pro Zeile: Name|discord:ID|steam:ID|xbox:Name", L"One person per line: Name|discord:ID|steam:ID|xbox:Name"));
        placeholder(host, root, T(L"Konten auswählen und verknüpfen", L"Pick and link accounts"),
                    T(L"Geführte Auswahl der Steam-/Xbox-/Discord-Konten mit Suche und „Verknüpfen“.",
                      L"Guided selection of Steam/Xbox/Discord accounts with search and “Link”."));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        set(steam_state_, s, "steam");
        set(xbox_state_, s, "xbox");
    }
private:
    Form form_;
    Host* host_ = nullptr;
    Label* steam_state_ = nullptr;
    Label* xbox_state_ = nullptr;
    static void set(Label* l, const Status& s, const char* key) {
        if (!l || !s.valid) return;
        Label::Tone tone = Label::Muted;
        l->text = connection_text(s.connections.contains(key) ? s.connections.at(key) : ojson(), tone);
        l->tone = tone;
    }
};

}  // namespace
std::unique_ptr<Page> make_friends_page() { return std::make_unique<FriendsPage>(); }
}  // namespace fl::ui
