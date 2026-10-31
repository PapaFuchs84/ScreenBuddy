// © 2026 FuchsLabs
// Datenquelle Discord (Bot, Server, Personen).
#include "ui_pages.h"

#include <algorithm>

#include "app.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

class DiscordPage : public Page {
public:
    PageInfo info() const override {
        return {"discord", Group::Sources, Text(L"Discord"),
                T(L"Bot-Zugang, Server und Personen der Discord-Seite", L"Bot access, servers and people of the Discord page")};
    }
    bool shows_status() const override { return true; }
    bool active(const ojson& cfg) const override { return flag(cfg, "discord", "enabled") && flag(cfg, "discord", "token_set"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "discord", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "display", "title", T(L"Titel der Discord-Seite", L"Discord page title"));
        form_.toggle(*page, "discord", "show_offline", T(L"Offline-Personen anzeigen", L"Show offline people"));
        Card* conn = root.add<Card>(T(L"Verbindung", L"Connection"));
        state_ = conn->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        form_.secret(*conn, "discord", "token", T(L"Bot-Token", L"Bot token"),
                     T(L"Leer lassen = gespeichertes Token behalten", L"Leave empty to keep the saved token"));
        add_key_help(root, host, "discord");

        Card* pick = root.add<Card>(T(L"Personen auswählen", L"Pick people"),
                                    T(L"Ohne Auswahl werden alle Mitglieder angezeigt, mit Auswahl nur die angehakten.",
                                      L"Without a selection everybody is shown, with a selection only the checked people."));
        info_ = pick->row<Label>(T(L"Mitglieder", L"Members"), Text(), T(L"Noch nicht geladen.", L"Not loaded yet."), Font::Small, Label::Muted);
        search_ = pick->row<TextField>(T(L"Suche", L"Search"), Text(), TextField::Single, L"");
        search_->width = 300;
        search_->track_dirty = false;
        search_->hint = T(L"Name eingeben …", L"Type a name …");
        search_->name = T(L"Mitglieder suchen", L"Search members");
        search_->on_change = [this] { refill(); };
        list_ = pick->add<ItemList>(std::vector<ItemList::Item>{});
        list_->name = T(L"Mitglieder", L"Members");
        list_->reorder = false;
        list_->track_dirty = false;
        list_->empty_text = T(L"Keine Mitglieder gefunden.", L"No members found.");
        list_->on_change = [this] { sync_from_list(); };
        auto buttons = pick->add<ButtonRow>();
        reload_ = buttons->add(T(L"Mitglieder neu laden", L"Reload members"), Button::Secondary, [this] { load(); });

        Card* ids = root.add<Card>(T(L"Server und Personen (IDs)", L"Servers and people (IDs)"),
                                   T(L"Zahlen-IDs, eine pro Zeile. Die Auswahl oben trägt die Personen-IDs hier ein.",
                                     L"Numeric IDs, one per line. The selection above fills in the person IDs here."));
        form_.list(*ids, "discord", "guild_ids", T(L"Server-IDs", L"Server IDs"), {}, 3);
        people_ = form_.list(*ids, "discord", "friend_ids", T(L"Personen-IDs", L"Person IDs"), {}, 4);
        people_->on_change = [this] { if (!syncing_) refill(); };
        form_.list(*ids, "discord", "ignore_ids", T(L"Ausgeblendete IDs", L"Hidden IDs"), {}, 3);
        refill(false);
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void shown(Host&) override { if (!loading_) load(); }
    void tick(Host&, const Status& s) override {
        if (!state_ || !s.valid) return;
        Label::Tone tone = Label::Muted;
        const ojson conn = s.connections.contains("discord") ? s.connections.at("discord") : ojson();
        state_->text = connection_text(conn, tone);
        state_->tone = tone;
        // Bot wurde nach dem Öffnen verbunden: Liste jetzt nachladen
        if (!ready_ && !loading_ && conn.is_object() && conn.value("state", "") == "ok") load();
    }

private:
    struct Member { std::string id, name, guild; };
    Form form_;
    Host* host_ = nullptr;
    Label *state_ = nullptr, *info_ = nullptr;
    TextField *search_ = nullptr, *people_ = nullptr;
    ItemList* list_ = nullptr;
    Button* reload_ = nullptr;
    std::vector<Member> members_;
    bool ready_ = false, loading_ = false, syncing_ = false;

    void load() {
        if (loading_) return;
        loading_ = true;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            std::vector<Member> members;
            bool ready = false;
            try {
                ready = h->app().discord_ready();
                for (const auto& m : h->app().member_list())
                    members.push_back({m.value("id", ""), m.value("name", ""), m.value("guild", "")});
            } catch (...) {
                return [this] { loading_ = false; };
            }
            return [this, members = std::move(members), ready]() mutable {
                loading_ = false;
                ready_ = ready;
                members_ = std::move(members);
                refill();
            };
        });
    }
    // Liste neu aufbauen: Mitglieder + bereits gewählte, aber unbekannte IDs, gefiltert nach der Suche.
    void refill(bool layout = true) {
        if (!list_ || !people_ || !search_) return;
        const auto chosen = logic::id_list(narrow(people_->text()));
        std::vector<Member> all = members_;
        for (const auto& id : chosen)
            if (std::none_of(all.begin(), all.end(), [&](const Member& m) { return m.id == id; }))
                all.push_back({id, "?", ""});
        std::vector<std::string> names;
        std::vector<bool> picked;
        for (const auto& m : all) {
            names.push_back(m.name == "?" ? "ID " + m.id : m.name);
            picked.push_back(std::find(chosen.begin(), chosen.end(), m.id) != chosen.end());
        }
        constexpr size_t cap = 40;
        const auto order = logic::filter_indices(names, picked, narrow(search_->text()), cap);
        std::vector<ItemList::Item> items;
        for (size_t i : order) {
            const Member& m = all[i];
            Text title = m.name == "?" ? Text(L"Unbekannte ID " + widen(m.id), L"Unknown ID " + widen(m.id)) : Text(widen(m.name));
            items.push_back({m.id, title, Text(widen(m.guild)), picked[i]});
        }
        const size_t total = names.size();
        list_->items = items;
        list_->original = items;
        if (list_->selected >= int(items.size())) list_->selected = items.empty() ? -1 : 0;
        if (list_->selected < 0 && !items.empty()) list_->selected = 0;
        if (!ready_ && members_.empty())
            info_->text = T(L"Der Bot ist gerade nicht verbunden. Trage zuerst einen gültigen Token ein und speichere.",
                            L"The bot is not connected right now. Enter a valid token first and save.");
        else if (total > items.size())
            info_->text = Text(std::to_wstring(total) + L" Einträge, " + std::to_wstring(items.size()) + L" angezeigt – Suche nutzen.",
                               std::to_wstring(total) + L" entries, " + std::to_wstring(items.size()) + L" shown – use the search.");
        else
            info_->text = Text(std::to_wstring(total) + L" Mitglieder.", std::to_wstring(total) + L" members.");
        if (layout) host_->relayout();
    }
    // Haken geändert: Personen-IDs im Textfeld nachführen (das Feld ist die einzige gespeicherte Wahrheit).
    void sync_from_list() {
        std::string text = narrow(people_->text());
        for (const auto& item : list_->items) text = logic::set_id(text, item.key, item.enabled);
        syncing_ = true;
        people_->set_text(widen(text));
        syncing_ = false;
    }
};

}  // namespace
std::unique_ptr<Page> make_discord_page() { return std::make_unique<DiscordPage>(); }
}  // namespace fl::ui
