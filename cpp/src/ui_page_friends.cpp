// © 2026 FuchsLabs
// Datenquelle Freunde (Steam, Xbox, Discord gemeinsam).
#include "ui_pages.h"

#include <algorithm>

#include "app.h"
#include "friends.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

using logic::Account;

std::wstring platform_name(const std::string& p) {
    if (p == "steam") return L"Steam";
    if (p == "xbox") return L"Xbox";
    if (p == "discord") return L"Discord";
    return widen(p);
}
std::wstring account_title(const Account& a) { return widen(a.name) + L" (" + platform_name(a.platform) + L")"; }

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
        steam_key_ = form_.secret(*steam, "friends", "steam_api_key", T(L"Steam-API-Schlüssel", L"Steam API key"),
                                  T(L"Leer lassen = gespeicherten Schlüssel behalten", L"Leave empty to keep the saved key"));
        steam_id_ = form_.text(*steam, "friends", "steam_id", T(L"Steam-ID oder Profiladresse", L"Steam ID or profile URL"));
        steam_result_ = steam->row<Label>(T(L"Prüfung", L"Check"), Text(), Text(L"–"), Font::Small, Label::Muted);
        steam_button_ = steam->add<ButtonRow>()->add(T(L"Verbindung prüfen", L"Check connection"), Button::Secondary,
                                                    [this] { check("steam"); });
        Card* xbox = root.add<Card>(Text(L"Xbox"), T(L"Über den kostenlosen Dienst OpenXBL (xbl.io), nicht direkt von Microsoft.",
                                                     L"Via the free OpenXBL service (xbl.io), not directly from Microsoft."));
        xbox_state_ = xbox->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        xbox_key_ = form_.secret(*xbox, "friends", "xbox_api_key", T(L"Xbox-API-Schlüssel", L"Xbox API key"),
                                 T(L"Leer lassen = gespeicherten Schlüssel behalten", L"Leave empty to keep the saved key"));
        xbox_result_ = xbox->row<Label>(T(L"Prüfung", L"Check"), Text(), Text(L"–"), Font::Small, Label::Muted);
        xbox_button_ = xbox->add<ButtonRow>()->add(T(L"Verbindung prüfen", L"Check connection"), Button::Secondary,
                                                  [this] { check("xbox"); });
        root.add<Note>(T(L"PlayStation: Es gibt keine offizielle Schnittstelle. Der inoffizielle Weg kann zur Sperre des Kontos führen und ist deshalb nicht eingebaut.",
                         L"PlayStation: there is no official interface. The unofficial route can get the account banned and is therefore not included."));

        Card* merge = root.add<Card>(T(L"Personen zusammenführen", L"Merge people"),
                                     T(L"Wer auf zwei Plattformen verschieden heißt: beide Konten wählen und „Verknüpfen“. "
                                       L"Die Konten stammen aus den laufenden Verbindungen.",
                                       L"If someone has different names on two platforms: pick both accounts and press “Link”. "
                                       L"Accounts come from the running connections."));
        account_info_ = merge->row<Label>(T(L"Konten", L"Accounts"), Text(), T(L"Noch nicht geladen.", L"Not loaded yet."), Font::Small, Label::Muted);
        a_ = merge->row<Combo>(T(L"Konto 1", L"Account 1"), Text(), std::vector<Text>{T(L"– nicht geladen –", L"– not loaded –")}, 0);
        b_ = merge->row<Combo>(T(L"Konto 2", L"Account 2"), Text(), std::vector<Text>{T(L"– nicht geladen –", L"– not loaded –")}, 0);
        for (Combo* c : {a_, b_}) { c->width = 320; c->track_dirty = false; }
        a_->name = T(L"Konto 1", L"Account 1");
        b_->name = T(L"Konto 2", L"Account 2");
        auto buttons = merge->add<ButtonRow>();
        link_ = buttons->add(T(L"Verknüpfen", L"Link"), Button::Primary, [this] { link_selected(); });
        buttons->add(T(L"Konten neu laden", L"Reload accounts"), Button::Secondary, [this] { load_accounts(); });
        suggestions_list_ = merge->add<ItemList>(std::vector<ItemList::Item>{});
        suggestions_list_->name = T(L"Vorschläge", L"Suggestions");
        suggestions_list_->checkable = false;
        suggestions_list_->reorder = false;
        suggestions_list_->track_dirty = false;
        suggestions_list_->empty_text = T(L"Keine Vorschläge.", L"No suggestions.");
        auto sug = merge->add<ButtonRow>();
        accept_ = sug->add(T(L"Vorschlag übernehmen", L"Accept suggestion"), Button::Secondary, [this] { accept(true); });
        reject_ = sug->add(T(L"Vorschlag verwerfen", L"Dismiss suggestion"), Button::Ghost, [this] { accept(false); });
        links_ = form_.list(*merge, "friends", "links", T(L"Verknüpfungen", L"Links"),
                            T(L"Eine Person pro Zeile: Name|discord:ID|steam:ID|xbox:Name – hier lassen sich Einträge auch löschen",
                              L"One person per line: Name|discord:ID|steam:ID|xbox:Name – entries can also be deleted here"));
        fill_accounts();
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void shown(Host&) override { if (!loading_) load_accounts(); }
    void tick(Host&, const Status& s) override {
        set(steam_state_, s, "steam");
        set(xbox_state_, s, "xbox");
        const bool idle = !host_->busy();
        for (Button* b : {steam_button_, xbox_button_}) if (b) b->enabled = idle;
        if (suggestions_list_) {
            const bool any = !suggestions_.empty();
            if (accept_) accept_->enabled = any && idle;
            if (reject_) reject_->enabled = any && idle;
        }
    }

private:
    Form form_;
    Host* host_ = nullptr;
    Label *steam_state_ = nullptr, *xbox_state_ = nullptr, *steam_result_ = nullptr, *xbox_result_ = nullptr, *account_info_ = nullptr;
    TextField *steam_key_ = nullptr, *steam_id_ = nullptr, *xbox_key_ = nullptr, *links_ = nullptr;
    Button *steam_button_ = nullptr, *xbox_button_ = nullptr, *link_ = nullptr, *accept_ = nullptr, *reject_ = nullptr;
    Combo *a_ = nullptr, *b_ = nullptr;
    ItemList* suggestions_list_ = nullptr;
    std::vector<Account> accounts_;
    std::vector<std::pair<Account, Account>> suggestions_;
    bool loading_ = false;

    static void set(Label* l, const Status& s, const char* key) {
        if (!l || !s.valid) return;
        Label::Tone tone = Label::Muted;
        l->text = connection_text(s.connections.contains(key) ? s.connections.at(key) : ojson(), tone);
        l->tone = tone;
    }

    void check(const std::string& platform) {
        // Eingegebene Werte; leere Felder ersetzt der Arbeits-Thread durch die gespeicherten
        std::string key = trim(narrow((platform == "steam" ? steam_key_ : xbox_key_)->text()));
        std::string id = trim(narrow(steam_id_->text()));
        Label* result = platform == "steam" ? steam_result_ : xbox_result_;
        result->text = T(L"Prüfe …", L"Checking …");
        result->tone = Label::Muted;
        Host* h = host_;
        h->run([this, h, platform, key, id]() mutable -> std::function<void()> {
            const ojson saved = h->app().cfg()["friends"];
            auto str = [&](const char* k) { return saved.contains(k) && saved[k].is_string() ? trim(saved[k].get<std::string>()) : std::string(); };
            if (key.empty()) key = str((platform + "_api_key").c_str());
            if (id.empty()) id = str("steam_id");
            Text message;
            Label::Tone tone = Label::Danger;
            if (key.empty()) message = T(L"Bitte zuerst den Schlüssel eintragen.", L"Please enter the key first.");
            else if (platform == "steam" && id.empty()) message = T(L"Bitte den Link zu deinem Steam-Profil eintragen.", L"Please enter the link to your Steam profile.");
            else {
                try {
                    auto accounts = platform == "steam" ? friends::fetch_steam(key, id) : friends::fetch_xbox(key);
                    int online = int(std::count_if(accounts.begin(), accounts.end(), [](auto& a) { return a.status != "offline"; }));
                    message = Text(L"Verbunden: " + std::to_wstring(accounts.size()) + L" Freunde, " + std::to_wstring(online) + L" gerade online",
                                   L"Connected: " + std::to_wstring(accounts.size()) + L" friends, " + std::to_wstring(online) + L" online now");
                    tone = Label::Success;
                } catch (const friends::FriendsError& e) {
                    message = Text(widen(e.what()));
                }
            }
            key.clear();  // Geheimnis nicht länger halten als nötig
            return [this, platform, message, tone] {
                Label* r = platform == "steam" ? steam_result_ : xbox_result_;
                if (!r) return;
                r->text = message;
                r->tone = tone;
                host_->relayout();
            };
        });
    }

    void load_accounts() {
        if (loading_) return;
        loading_ = true;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            ojson data;
            try { data = h->app().friend_accounts(); }
            catch (...) { return [this] { loading_ = false; }; }
            auto parse = [](const ojson& j) {
                auto s = [&](const char* k) { return j.is_object() && j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); };
                return Account{s("platform"), s("id"), s("name")};
            };
            std::vector<Account> accounts;
            std::vector<std::pair<Account, Account>> suggestions;
            if (data.contains("accounts") && data["accounts"].is_array())
                for (const auto& a : data["accounts"]) accounts.push_back(parse(a));
            if (data.contains("suggestions") && data["suggestions"].is_array())
                for (const auto& s : data["suggestions"])
                    if (s.is_object() && s.contains("a") && s.contains("b")) suggestions.push_back({parse(s["a"]), parse(s["b"])});
            return [this, accounts = std::move(accounts), suggestions = std::move(suggestions)]() mutable {
                loading_ = false;
                accounts_ = std::move(accounts);
                suggestions_ = std::move(suggestions);
                fill_accounts();
                host_->relayout();
            };
        });
    }

    void fill_accounts() {
        if (!a_) return;
        std::vector<Text> names;
        for (const auto& a : accounts_) names.push_back(Text(account_title(a)));
        const bool have = !names.empty();
        if (!have) names.push_back(T(L"– nicht geladen –", L"– not loaded –"));
        const int n = int(names.size());
        a_->options = names;
        b_->options = names;
        a_->index = a_->original = std::clamp(a_->index, 0, n - 1);
        b_->index = std::clamp(b_->index, 0, n - 1);
        if (n > 1 && b_->index == a_->index) b_->index = (a_->index + 1) % n;
        b_->original = b_->index;
        a_->enabled = b_->enabled = link_->enabled = have;
        account_info_->text = have ? Text(std::to_wstring(accounts_.size()) + L" Konten geladen.", std::to_wstring(accounts_.size()) + L" accounts loaded.")
                                   : T(L"Sobald Konten verbunden sind (Schlüssel oben, Speichern & anwenden), kannst du sie hier zusammenführen.",
                                       L"As soon as accounts are connected (keys above, Save & apply) you can merge them here.");
        std::vector<ItemList::Item> items;
        for (const auto& [x, y] : suggestions_)
            items.push_back({"", Text(account_title(x) + L" = " + account_title(y) + L"?", account_title(x) + L" = " + account_title(y) + L"?"),
                             T(L"Ähnliche Namen – dieselbe Person?", L"Similar names – the same person?"), false});
        suggestions_list_->items = items;
        suggestions_list_->original = items;
        suggestions_list_->selected = items.empty() ? -1 : std::min(std::max(suggestions_list_->selected, 0), int(items.size()) - 1);
    }

    void apply_link(const Account& x, const Account& y) {
        auto lines = logic::link_accounts(logic::lines_of(narrow(links_->text())), x, y);
        links_->set_text(widen(logic::lines_join(lines)));
        suggestions_.erase(std::remove_if(suggestions_.begin(), suggestions_.end(), [&](const auto& s) {
            return (s.first.platform == x.platform && s.first.id == x.id && s.second.platform == y.platform && s.second.id == y.id) ||
                   (s.first.platform == y.platform && s.first.id == y.id && s.second.platform == x.platform && s.second.id == x.id);
        }), suggestions_.end());
        fill_accounts();
        host_->toast(T(L"Verknüpfung eingetragen – mit „Speichern & anwenden“ übernehmen.", L"Link added – apply it with “Save & apply”."), ToastKind::Success);
        host_->relayout();
    }
    void link_selected() {
        if (accounts_.empty()) return;
        const auto& x = accounts_[size_t(std::clamp(a_->index, 0, int(accounts_.size()) - 1))];
        const auto& y = accounts_[size_t(std::clamp(b_->index, 0, int(accounts_.size()) - 1))];
        if (x.platform == y.platform && x.id == y.id) {
            host_->toast(T(L"Bitte zwei verschiedene Konten wählen.", L"Please pick two different accounts."), ToastKind::Error);
            return;
        }
        apply_link(x, y);
    }
    void accept(bool yes) {
        const int i = suggestions_list_->selected;
        if (i < 0 || i >= int(suggestions_.size())) return;
        auto pair = suggestions_[size_t(i)];
        if (yes) { apply_link(pair.first, pair.second); return; }
        suggestions_.erase(suggestions_.begin() + i);
        fill_accounts();
        host_->relayout();
    }
};

}  // namespace
std::unique_ptr<Page> make_friends_page() { return std::make_unique<FriendsPage>(); }
}  // namespace fl::ui
