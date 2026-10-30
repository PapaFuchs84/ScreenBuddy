// © 2026 FuchsLabs
// Verbindung: Plugin-Store. Zeigt den signierten Index (plugin_catalog), installiert geprüfte Plugins und zeigt vor der
// Installation, an welche Hosts ein Plugin Daten senden würde. Ein Plugin bleibt nach der Installation ausgeschaltet.
// Alle Store-Aufrufe laufen im Arbeits-Thread (Host::run), nie im UI-Thread.
#include "ui_pages.h"

#include <algorithm>
#include <map>
#include <set>

#include "gfx.h"
#include "native_settings.h"
#include "util.h"

namespace fl::ui {
namespace {

class StorePage : public Page {
public:
    PageInfo info() const override {
        return {"store", Group::Connection, Text(L"Plugin-Store"),
                T(L"Geprüfte Plugins aus dem signierten Store finden und installieren", L"Find and install reviewed plugins from the signed store")};
    }
    bool active(const ojson&) const override { return false; }
    void build(Host& host, Column& root) override {
        host_ = &host;
        form_ = Form(host);
        Card* top = root.add<Card>(T(L"Plugin-Store", L"Plugin store"),
            T(L"Der Store besteht nur aus Daten: Plugins enthalten keinen Programmcode. Index und Sperrliste sind digital signiert, jedes Plugin wird vor der Installation gegen die Signatur geprüft. Es werden keine Nutzungsdaten gesendet.",
              L"The store contains data only: plugins contain no program code. Index and revocation list are digitally signed and every plugin is checked against the signature before installation. No usage data is sent."));
        form_.toggle(*top, "ui", "plugin_store", T(L"Plugin-Store verwenden", L"Use the plugin store"),
                     T(L"Aus: kein Abruf, nur lokale Plugins. Wirkt nach „Speichern & anwenden“", L"Off: no requests, local plugins only. Takes effect after “Save & apply”"));
        status_ = top->row<Label>(T(L"Stand", L"State"), Text(), T(L"Wird geladen …", L"Loading …"), Font::Small, Label::Muted);

        Card* browse = root.add<Card>(T(L"Verfügbare Plugins", L"Available plugins"));
        search_ = browse->row<TextField>(T(L"Suche", L"Search"), T(L"Name, Beschreibung oder Autor", L"Name, description or author"), TextField::Single, L"");
        search_->track_dirty = false;
        search_->limit = 80;
        auto cats = browse->add<ButtonRow>();
        add_category(cats, "", T(L"Alle", L"All"));
        add_category(cats, "system", T(L"System", L"System"));
        add_category(cats, "games", T(L"Spiele", L"Games"));
        add_category(cats, "smart-home", T(L"Smart Home", L"Smart home"));
        add_category(cats, "info", T(L"Info", L"Info"));
        add_category(cats, "other", T(L"Sonstiges", L"Other"));
        list_ = browse->add<ItemList>(std::vector<ItemList::Item>{});
        list_->checkable = false;
        list_->reorder = false;
        list_->track_dirty = false;
        list_->name = T(L"Verfügbare Plugins", L"Available plugins");
        list_->empty_text = T(L"Noch nichts geladen.", L"Nothing loaded yet.");
        list_->on_select = [this](int i) {
            selected_id_ = i >= 0 && i < int(state_.cards.size()) ? state_.cards[size_t(i)].id : "";
            show_details();
            update_buttons();
        };
        auto row = browse->add<ButtonRow>();
        search_btn_ = row->add(T(L"Suchen", L"Search"), Button::Secondary, [this] { reload(false); });
        refresh_ = row->add(T(L"Store aktualisieren", L"Refresh store"), Button::Secondary, [this] { reload(true); });
        install_ = row->add(T(L"Installieren", L"Install"), Button::Primary, [this] { install(); });

        details_ = root.add<Card>(T(L"Details", L"Details"), T(L"Oben ein Plugin auswählen.", L"Select a plugin above first."));
        if (loaded_) fill();
        update_buttons();
    }
    void shown(Host&) override { reload(false); }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status&) override {
        update_buttons();
        if (const auto* c = selected(); c && c->has_preview && !previews_.count(c->id)) request_preview(c->id);
    }

private:
    Host* host_ = nullptr;
    Form form_;
    Label* status_ = nullptr;
    TextField* search_ = nullptr;
    ItemList* list_ = nullptr;
    Card* details_ = nullptr;
    Button *search_btn_ = nullptr, *refresh_ = nullptr, *install_ = nullptr;
    NativeStoreState state_;
    std::map<std::string, Image> previews_;    // decoded, verified preview images by plugin id
    std::set<std::string> preview_failed_;     // no retry loop within one window session
    std::string preview_pending_;
    bool loaded_ = false;
    std::string selected_id_, category_;

    bool en() const { return host_->english(); }
    void add_category(ButtonRow* row, const std::string& slug, Text label) {
        row->add(std::move(label), Button::Ghost, [this, slug] { category_ = slug; reload(false); });
    }
    const NativeStoreCard* selected() const {
        auto it = std::find_if(state_.cards.begin(), state_.cards.end(), [this](auto& c) { return c.id == selected_id_; });
        return it == state_.cards.end() ? nullptr : &*it;
    }
    static bool installable(const NativeStoreCard& c) { return c.status == "not_installed" || c.status == "update_available"; }
    void update_buttons() {
        if (!install_) return;
        auto& a = host_->plugins();
        const bool idle = !host_->busy();
        const bool live = state_.state == "available" || state_.state == "invalid" || state_.state == "offline";
        const auto* sel = selected();
        search_btn_->enabled = idle && bool(a.store_list);
        refresh_->enabled = idle && bool(a.store_refresh) && state_.state != "unavailable" && state_.state != "disabled";
        install_->enabled = idle && live && sel && installable(*sel) && bool(a.store_install);
        install_->text = sel && sel->status == "update_available" ? T(L"Aktualisieren", L"Update") : T(L"Installieren", L"Install");
        host_->surface().invalidate();
    }
    void reload(bool force) {
        auto& a = host_->plugins();
        if (host_->busy() || !a.store_list) return;
        Host* h = host_;
        const std::string search = search_ ? narrow(search_->text()) : "";
        const std::string category = category_;
        h->run([this, h, force, search, category]() -> std::function<void()> {
            auto& actions = h->plugins();
            if (force && actions.store_refresh) actions.store_refresh(true);
            else if (actions.store_refresh) actions.store_refresh(false);  // limited to once a day inside the callback
            NativeStoreState result = actions.store_list(search, category);
            return [this, result = std::move(result)] { apply(result); };
        });
        update_buttons();
    }
    void apply(const NativeStoreState& result) {
        state_ = result;
        loaded_ = true;
        fill();
    }
    Text state_text() const {
        if (state_.state == "disabled")
            return T(L"Der Store ist ausgeschaltet. Oben einschalten und „Speichern & anwenden“.", L"The store is switched off. Switch it on above and “Save & apply”.");
        if (state_.state == "unavailable")
            return T(L"Der Store ist in dieser Version noch nicht verfügbar (kein Signaturschlüssel hinterlegt).", L"The store is not available in this version yet (no signing key pinned).");
        if (state_.state == "invalid")
            return T(L"Der Store lieferte ungültige oder veraltete Daten. Es werden die zuletzt geprüften Daten gezeigt.",
                     L"The store returned invalid or outdated data. The last verified data is shown.");
        if (state_.state == "offline")
            return T(L"Der Store ist nicht erreichbar und es gibt noch keine geprüften Daten.", L"The store is not reachable and there is no verified data yet.");
        return state_.from_cache ? T(L"Geprüfte Daten aus dem Zwischenspeicher (Store zuletzt nicht erreichbar oder noch nicht abgefragt).",
                                     L"Verified data from the cache (store not reachable or not queried yet).")
                                 : T(L"Verbunden. Index und Sperrliste sind geprüft.", L"Connected. Index and revocation list verified.");
    }
    static Text status_text(const std::string& s) {
        if (s == "installed") return T(L"installiert", L"installed");
        if (s == "update_available") return T(L"Update verfügbar", L"update available");
        if (s == "unsupported") return T(L"braucht neuere ScreenBuddy-Version", L"needs a newer ScreenBuddy");
        if (s == "revoked") return T(L"gesperrt", L"revoked");
        return T(L"nicht installiert", L"not installed");
    }
    void fill() {
        if (!list_) return;
        if (status_) status_->text = state_text();
        std::vector<ItemList::Item> items;
        for (const auto& c : state_.cards) {
            Text title(widen(c.name + "  " + c.version));
            Text detail(widen(c.author + " · " + c.category + " · " + c.license) + L" · " + status_text(c.status).get(false),
                        widen(c.author + " · " + c.category + " · " + c.license) + L" · " + status_text(c.status).get(true));
            items.push_back({c.id, title, detail, c.status == "installed"});
        }
        list_->items = items;
        list_->original = items;
        list_->empty_text = state_.state == "available" ? T(L"Keine passenden Plugins.", L"No matching plugins.") : state_text();
        list_->selected = -1;
        for (size_t i = 0; i < state_.cards.size(); ++i) if (state_.cards[i].id == selected_id_) list_->selected = int(i);
        if (list_->selected < 0) selected_id_.clear();
        show_details();
        update_buttons();
    }
    void show_details() {
        if (!details_) return;
        details_->clear();
        const auto* c = selected();
        if (!c) {
            details_->title = T(L"Details", L"Details");
            details_->desc = T(L"Oben ein Plugin auswählen.", L"Select a plugin above first.");
            host_->relayout();
            return;
        }
        details_->title = Text(widen(c->name), widen(c->name));
        details_->desc = Text(widen(c->description), widen(c->description));
        if (c->has_preview) {
            auto have = previews_.find(c->id);
            if (have != previews_.end()) {
                auto* view = details_->add<ImageView>();
                view->image = have->second;
                view->version = 1;
                view->aspect = float(have->second.h) / float(std::max(1, have->second.w));
                view->name = Text(widen(c->name)) ;
            } else {
                request_preview(c->id);
            }
        }
        details_->row<Label>(T(L"Autor", L"Author"), Text(), Text(widen(c->author)), Font::Small, Label::Muted);
        details_->row<Label>(T(L"Version", L"Version"), Text(), Text(widen(c->version) + (c->installed_version.empty() ? L"" : L" (installiert: " + widen(c->installed_version) + L")"),
                                                                    widen(c->version) + (c->installed_version.empty() ? L"" : L" (installed: " + widen(c->installed_version) + L")")), Font::Small, Label::Muted);
        details_->row<Label>(T(L"Lizenz", L"License"), Text(), Text(widen(c->license)), Font::Small, Label::Muted);
        details_->row<Label>(T(L"Status", L"Status"), Text(), status_text(c->status), Font::Small, Label::Muted);
        if (c->hosts.empty()) {
            details_->add<Note>(T(L"Dieses Plugin sendet keine Daten ins Internet.", L"This plugin does not send data to the internet."), Note::Info);
        } else {
            std::wstring list;
            for (const auto& h : c->hosts) list += L"• " + widen(h) + L"\n";
            details_->add<Note>(Text(L"Dieses Plugin sendet Daten an:\n" + list + L"Es bleibt nach der Installation ausgeschaltet; jeder Host braucht Ihre Freigabe.",
                                     L"This plugin sends data to:\n" + list + L"It stays off after installation; every host needs your consent."), Note::Warn);
        }
        if (!c->new_hosts.empty()) {
            std::wstring list;
            for (const auto& h : c->new_hosts) list += L"• " + widen(h) + L"\n";
            details_->add<Note>(Text(L"Das Update fügt neue Hosts hinzu:\n" + list + L"Alle Freigaben werden beim Update zurückgesetzt.",
                                     L"The update adds new hosts:\n" + list + L"All consents are reset on update."), Note::Warn);
        }
        host_->relayout();
    }
    void request_preview(const std::string& id) {
        if (!host_->plugins().store_preview || preview_failed_.count(id) || preview_pending_ == id || host_->busy()) return;
        preview_pending_ = id;
        Host* h = host_;
        h->run([this, h, id]() -> std::function<void()> {
            std::string bytes = h->plugins().store_preview(id);
            return [this, id, bytes = std::move(bytes)] {
                preview_pending_.clear();
                std::optional<Image> img = bytes.empty() ? std::nullopt : decode_image(bytes);
                if (img && !img->empty()) previews_[id] = std::move(*img);
                else preview_failed_.insert(id);
                if (selected_id_ == id) show_details();
            };
        });
    }
    void install() {
        const auto* sel = selected();
        if (host_->busy() || !sel || !installable(*sel) || !host_->plugins().store_install) return;
        const NativeStoreCard c = *sel;
        std::wstring prompt = en() ? L"Install “" : L"„";
        prompt += widen(c.name) + (en() ? L"” " : L"“ ") + widen(c.version) + L"?\r\n\r\n";
        if (c.hosts.empty()) prompt += en() ? L"It sends no data to the internet.\r\n" : L"Es sendet keine Daten ins Internet.\r\n";
        else {
            prompt += en() ? L"It may send data to these hosts (after your consent):\r\n" : L"Es darf (nach Ihrer Freigabe) Daten an diese Hosts senden:\r\n";
            for (const auto& h : c.hosts) prompt += L"• " + widen(h) + L"\r\n";
        }
        prompt += en() ? L"\r\nThe plugin stays off until you enable it." : L"\r\nDas Plugin bleibt ausgeschaltet, bis Sie es aktivieren.";
        if (MessageBoxW(host_->window(), prompt.c_str(), L"ScreenBuddy – Plugin-Store", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        Host* h = host_;
        h->run([this, h, id = c.id]() -> std::function<void()> {
            std::string error;
            try { h->plugins().store_install(id); } catch (const std::exception& e) { error = e.what(); }
            auto& actions = h->plugins();
            NativeStoreState result = actions.store_list("", "");
            return [this, h, error, result = std::move(result)] {
                apply(result);
                if (error.empty()) h->toast(T(L"Plugin installiert. Unter „Plugins“ aktivieren.", L"Plugin installed. Enable it under “Plugins”."), ToastKind::Success);
                else h->toast(Text(widen(error)), ToastKind::Error);
            };
        });
        update_buttons();
    }
};

}  // namespace
std::unique_ptr<Page> make_store_page() { return std::make_unique<StorePage>(); }
}  // namespace fl::ui
