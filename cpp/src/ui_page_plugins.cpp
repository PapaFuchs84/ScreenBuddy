// © 2026 FuchsLabs
// Verbindung: Plugins verwalten (Import, Aktivieren mit Host-Freigabe, Entfernen) und konfigurieren.
// Die Plugin-Logik selbst bleibt in App (NativePluginActions); alle Aufrufe laufen im Arbeits-Thread.
#include "ui_pages.h"

#include <commdlg.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

#include "native_settings.h"
#include "util.h"

namespace fl::ui {
namespace {

class PluginsPage : public Page {
public:
    PageInfo info() const override {
        return {"plugins", Group::Connection, Text(L"Plugins"),
                T(L"Lokale, deklarative Plugins verwalten und konfigurieren", L"Manage and configure local, declarative plugins")};
    }
    bool active(const ojson&) const override { return false; }
    void build(Host& host, Column& root) override {
        host_ = &host;
        auto& actions = host.plugins();
        Card* card = root.add<Card>(T(L"Installierte Plugins", L"Installed plugins"),
            actions.list ? T(L"Plugins werden lokal verwaltet. Ein Import führt keinen Plugin-Code aus. Änderungen gelten sofort.",
                             L"Plugins are managed locally. Import does not execute plugin code. Changes apply immediately.")
                         : T(L"Die Plugin-Verwaltung ist in diesem Build nicht verbunden.", L"Plugin management is not connected in this build."));
        list_ = card->add<ItemList>(std::vector<ItemList::Item>{});
        list_->checkable = false;
        list_->reorder = false;
        list_->track_dirty = false;
        list_->name = T(L"Installierte Plugins", L"Installed plugins");
        list_->empty_text = T(L"Wird geladen …", L"Loading …");
        list_->on_select = [this](int i) {
            selected_id_ = i >= 0 && i < int(entries_.size()) ? entries_[size_t(i)].id : "";
            show_config();
            update_buttons();
        };
        auto row = card->add<ButtonRow>();
        import_ = row->add(T(L"Manifest importieren …", L"Import manifest …"), Button::Primary, [this] { import_manifest(); });
        toggle_ = row->add(T(L"Aktivieren / deaktivieren", L"Enable / disable"), Button::Secondary, [this] { toggle(); });
        revoke_ = row->add(T(L"Freigaben widerrufen", L"Revoke host consent"), Button::Secondary, [this] { revoke(); });
        remove_ = row->add(T(L"Entfernen", L"Remove"), Button::Danger, [this] { remove(); });
        refresh_ = row->add(T(L"Aktualisieren", L"Refresh"), Button::Ghost, [this] { refresh(); });
        config_ = root.add<Card>(T(L"Plugin-Konfiguration", L"Plugin settings"),
                                 T(L"Zuerst oben ein Plugin auswählen.", L"Select a plugin above first."));
        if (loaded_) fill();
        update_buttons();
    }
    void shown(Host&) override { refresh(); }
    void tick(Host&, const Status&) override { update_buttons(); }

private:
    Host* host_ = nullptr;
    ItemList* list_ = nullptr;
    Card* config_ = nullptr;
    Button *import_ = nullptr, *toggle_ = nullptr, *revoke_ = nullptr, *remove_ = nullptr, *refresh_ = nullptr, *save_ = nullptr;
    std::vector<NativePluginEntry> entries_;
    bool loaded_ = false;
    std::string selected_id_;
    std::vector<NativePluginSetting> fields_;
    std::vector<TextField*> inputs_;
    std::vector<Toggle*> clears_;
    std::string editing_;

    bool en() const { return host_->english(); }
    const NativePluginEntry* selected() const {
        auto it = std::find_if(entries_.begin(), entries_.end(), [this](auto& e) { return e.id == selected_id_; });
        return it == entries_.end() ? nullptr : &*it;
    }
    void update_buttons() {
        auto& a = host_->plugins();
        const bool idle = !host_->busy();
        const bool sel = selected() != nullptr;
        if (!import_) return;
        import_->enabled = idle && bool(a.import_manifest);
        refresh_->enabled = idle && bool(a.list);
        toggle_->enabled = idle && sel && bool(a.set_enabled);
        remove_->enabled = idle && sel && bool(a.remove);
        revoke_->enabled = idle && sel && bool(a.set_host_consent);
        if (save_) save_->enabled = idle && !editing_.empty() && bool(a.save_settings);
        host_->surface().invalidate();
    }
    void reload_after(std::function<void()> action, Text done = {}) {
        Host* h = host_;
        h->run([this, h, action = std::move(action), done]() -> std::function<void()> {
            if (action) action();
            auto list = h->plugins().list ? h->plugins().list() : std::vector<NativePluginEntry>{};
            return [this, h, list = std::move(list), done] {
                entries_ = list;
                loaded_ = true;
                fill();
                if (!done.empty()) h->toast(done, ToastKind::Success);
            };
        });
        update_buttons();
    }
    void refresh() {
        if (!host_->plugins().list || host_->busy()) return;
        reload_after({});
    }
    void fill() {
        if (!list_) return;
        std::vector<ItemList::Item> items;
        for (const auto& e : entries_) {
            size_t granted = 0;
            for (const auto& h : e.hosts)
                if (std::find(e.consented_hosts.begin(), e.consented_hosts.end(), h) != e.consented_hosts.end()) ++granted;
            std::wstring id = widen(e.id);
            std::wstring hosts = e.hosts.empty() ? L"" : L" · " + std::to_wstring(granted) + L"/" + std::to_wstring(e.hosts.size());
            Text detail(id + (e.enabled ? L" · aktiv" : L" · aus") + (e.hosts.empty() ? L"" : L" · Hosts freigegeben" + hosts),
                        id + (e.enabled ? L" · enabled" : L" · disabled") + (e.hosts.empty() ? L"" : L" · hosts allowed" + hosts));
            items.push_back({e.id, Text(widen(e.name + "  " + e.version)), detail, e.enabled});
        }
        list_->items = items;
        list_->original = items;
        list_->empty_text = T(L"Keine Plugins installiert.", L"No plugins installed.");
        list_->selected = -1;
        for (size_t i = 0; i < entries_.size(); ++i) if (entries_[i].id == selected_id_) list_->selected = int(i);
        if (list_->selected < 0) selected_id_.clear();
        show_config();
        update_buttons();
    }
    void show_config() {
        if (!config_) return;
        config_->clear();
        inputs_.clear();
        clears_.clear();
        save_ = nullptr;
        fields_.clear();
        editing_.clear();
        const NativePluginEntry* e = selected();
        if (!e) {
            config_->title = T(L"Plugin-Konfiguration", L"Plugin settings");
            config_->desc = T(L"Zuerst oben ein Plugin auswählen.", L"Select a plugin above first.");
            host_->relayout();
            return;
        }
        config_->title = Text(L"Konfiguration: " + widen(e->name), L"Settings: " + widen(e->name));
        config_->desc = T(L"Leere Zugangsfelder behalten gespeicherte Werte; zum Entfernen „Löschen“ einschalten. Wird sofort gespeichert.",
                          L"Empty credential fields keep saved values; switch on “Clear” to remove one. Saved immediately.");
        if (e->settings.size() > 12) {
            config_->add<Note>(T(L"Dieses Plugin hat mehr als 12 Einstellungen und kann hier nicht bearbeitet werden.",
                                 L"This plugin has more than 12 settings and cannot be edited here."), Note::Warn);
            host_->relayout();
            return;
        }
        if (e->settings.empty()) config_->add<Label>(T(L"Dieses Plugin hat keine Einstellungen.", L"This plugin has no settings."), Font::Small, Label::Muted);
        editing_ = e->id;
        fields_ = e->settings;
        for (const auto& f : fields_) {
            Text label(widen(f.label + " (" + f.key + ")") + (f.required ? L" *" : L""));
            const bool secret = f.type == "secret";
            auto input = config_->row<TextField>(label, Text(), secret ? TextField::Secret : TextField::Single, secret ? L"" : widen(f.value));
            input->track_dirty = false;
            input->secret_set = f.secret_set;
            input->limit = 16384;
            inputs_.push_back(input);
            if (secret) {
                auto clear = config_->row<Toggle>(T(L"Gespeicherten Wert löschen", L"Clear saved value"), Text(widen(f.key)), false);
                clear->track_dirty = false;
                clears_.push_back(clear);
            } else clears_.push_back(nullptr);
        }
        if (!fields_.empty()) {
            auto row = config_->add<ButtonRow>();
            save_ = row->add(T(L"Plugin-Konfiguration speichern", L"Save plugin settings"), Button::Primary, [this] { save_settings(); });
        }
        host_->relayout();
    }
    void save_settings() {
        if (host_->busy() || editing_.empty() || !host_->plugins().save_settings) return;
        nlohmann::json patch = nlohmann::json::object();
        try {
            for (size_t i = 0; i < fields_.size(); ++i) {
                const auto& f = fields_[i];
                std::string text = narrow(inputs_[i]->text());
                if (f.type == "secret") {
                    if (clears_[i] && clears_[i]->on) { patch[f.key] = nullptr; continue; }
                    if (text.empty()) {
                        if (f.required && !f.secret_set)
                            throw Invalid(en() ? "Enter the required plugin credential." : "Bitte das erforderliche Plugin-Zugangsfeld ausfüllen.", inputs_[i]);
                        continue;
                    }
                } else if (f.required && trim(text).empty()) {
                    throw Invalid(en() ? "Fill in every required plugin field." : "Bitte alle erforderlichen Plugin-Felder ausfüllen.", inputs_[i]);
                }
                if (f.type == "number") {
                    auto number = nlohmann::json::parse(trim(text), nullptr, false);
                    if (number.is_discarded() || !number.is_number() || !std::isfinite(number.get<double>()))
                        throw Invalid(en() ? "Enter a finite plugin number." : "Bitte eine endliche Zahl eingeben.", inputs_[i]);
                    patch[f.key] = std::move(number);
                } else patch[f.key] = std::move(text);
            }
        } catch (const Invalid& e) {
            if (e.widget) host_->surface().focus(e.widget, true);
            host_->toast(Text(widen(e.what())), ToastKind::Error);
            return;
        }
        Host* h = host_;
        std::string id = editing_;
        reload_after([h, id, patch] { h->plugins().save_settings(id, patch); },
                     T(L"Plugin-Konfiguration gespeichert.", L"Plugin settings saved."));
    }
    void import_manifest() {
        if (host_->busy() || !host_->plugins().import_manifest) return;
        wchar_t filename[32768]{};
        OPENFILENAMEW picker{sizeof(picker)};
        picker.hwndOwner = host_->window();
        picker.lpstrFile = filename;
        picker.nMaxFile = DWORD(std::size(filename));
        picker.lpstrFilter = L"JSON manifest (*.json)\0*.json\0\0";
        picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
        if (!GetOpenFileNameW(&picker)) {
            if (CommDlgExtendedError()) host_->toast(T(L"Dateiauswahl konnte nicht geöffnet werden.", L"Cannot open the file picker."), ToastKind::Error);
            return;
        }
        std::filesystem::path path = filename;
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec || size > 256 * 1024) {
            host_->toast(T(L"Das Manifest muss eine lesbare Datei mit höchstens 256 KiB sein.", L"Manifest must be a readable file of at most 256 KiB."),
                         ToastKind::Error);
            return;
        }
        Host* h = host_;
        reload_after([h, path] { h->plugins().import_manifest(path); }, T(L"Plugin importiert.", L"Plugin imported."));
    }
    void toggle() {
        const NativePluginEntry* sel = selected();
        if (host_->busy() || !sel || !host_->plugins().set_enabled) return;
        NativePluginEntry e = *sel;
        HWND owner = host_->window();
        if (!e.enabled && !e.hosts.empty()) {
            const bool missing = std::any_of(e.settings.begin(), e.settings.end(), [](const auto& f) {
                return f.required && (f.type == "secret" ? !f.secret_set : trim(f.value).empty());
            });
            if (missing) {
                MessageBoxW(owner, en() ? L"Fill in the required plugin settings below first."
                                        : L"Bitte zuerst unten die erforderlichen Plugin-Einstellungen ausfüllen.",
                            L"ScreenBuddy – Plugin", MB_OK | MB_ICONINFORMATION);
                return;
            }
            if (!host_->plugins().set_host_consent) {
                host_->toast(T(L"Host-Freigaben sind in diesem Build nicht angebunden.", L"Host consent is not connected in this build."), ToastKind::Error);
                return;
            }
            std::wstring prompt = en() ? L"This plugin may fetch from the following hosts. It may send values from the listed plugin settings to these hosts.\r\n\r\nHosts:\r\n"
                                       : L"Dieses Plugin darf die folgenden Hosts abrufen. Dabei kann es Werte der aufgeführten Plugin-Einstellungen an diese Hosts senden.\r\n\r\nHosts:\r\n";
            for (const auto& host : e.hosts) {
                bool consented = std::find(e.consented_hosts.begin(), e.consented_hosts.end(), host) != e.consented_hosts.end();
                prompt += L"• " + widen(host) + (consented ? (en() ? L" (already allowed)" : L" (bereits freigegeben)")
                                                           : (en() ? L" (new consent)" : L" (neue Freigabe)")) + L"\r\n";
            }
            prompt += en() ? L"\r\nSetting keys:\r\n" : L"\r\nEinstellungsschlüssel:\r\n";
            if (e.setting_keys.empty()) prompt += en() ? L"(none)\r\n" : L"(keine)\r\n";
            for (const auto& key : e.setting_keys) {
                const bool secret = std::any_of(e.settings.begin(), e.settings.end(), [&](const auto& f) { return f.key == key && f.type == "secret"; });
                prompt += L"• " + widen(key) + (secret ? (en() ? L" (secret – sent to the hosts)" : L" (Geheimnis – wird an die Hosts gesendet)") : L"") + L"\r\n";
            }
            prompt += en() ? L"\r\nAllow these hosts and enable the plugin?" : L"\r\nDiese Hosts freigeben und das Plugin aktivieren?";
            if (MessageBoxW(owner, prompt.c_str(), L"ScreenBuddy – Plugin", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        }
        Host* h = host_;
        reload_after([h, e] {
            auto& a = h->plugins();
            if (!e.enabled && a.set_host_consent)
                for (const auto& host : e.hosts)
                    if (std::find(e.consented_hosts.begin(), e.consented_hosts.end(), host) == e.consented_hosts.end())
                        a.set_host_consent(e.id, host, true);
            a.set_enabled(e.id, !e.enabled);
        }, e.enabled ? T(L"Plugin deaktiviert.", L"Plugin disabled.") : T(L"Plugin aktiviert.", L"Plugin enabled."));
    }
    void revoke() {
        const NativePluginEntry* sel = selected();
        if (host_->busy() || !sel || !host_->plugins().set_host_consent) return;
        if (MessageBoxW(host_->window(), en() ? L"Disable this plugin and revoke all its host grants?"
                                              : L"Plugin deaktivieren und alle zugehörigen Host-Freigaben widerrufen?",
                        L"ScreenBuddy – Plugin", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        Host* h = host_;
        NativePluginEntry e = *sel;
        reload_after([h, e] {
            auto& a = h->plugins();
            if (a.set_enabled) a.set_enabled(e.id, false);  // erst abschalten, dann widerrufen (auch bei Teilfehlern)
            for (const auto& host : e.consented_hosts) a.set_host_consent(e.id, host, false);
        }, T(L"Freigaben widerrufen.", L"Host consent revoked."));
    }
    void remove() {
        const NativePluginEntry* sel = selected();
        if (host_->busy() || !sel || !host_->plugins().remove) return;
        if (MessageBoxW(host_->window(), en() ? L"Remove this plugin from local management?" : L"Dieses Plugin aus der lokalen Verwaltung entfernen?",
                        L"ScreenBuddy", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        Host* h = host_;
        std::string id = sel->id;
        reload_after([h, id] { h->plugins().remove(id); }, T(L"Plugin entfernt.", L"Plugin removed."));
    }
};

}  // namespace
std::unique_ptr<Page> make_plugins_page() { return std::make_unique<PluginsPage>(); }
}  // namespace fl::ui
