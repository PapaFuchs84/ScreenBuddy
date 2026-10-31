// © 2026 FuchsLabs
// App: Updates & Autostart (Farben & Design: ui_page_design.cpp).
#include "ui_pages.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <string_view>

#include <windows.h>
#include <shellapi.h>

#include "app.h"
#include "render.h"
#include "ui_logic.h"
#include "update.h"
#include "util.h"
#include "winsetup.h"

namespace fl::ui {
namespace {

bool release_page_url(std::string_view url) {
    constexpr std::string_view latest = "releases/latest";
    constexpr std::string_view tag = "releases/tag/";
    const std::string_view base = update::RELEASES_PAGE_PREFIX;
    if (!url.starts_with(base)) return false;
    url.remove_prefix(base.size());
    if (url == latest) return true;
    if (!url.starts_with(tag)) return false;
    url.remove_prefix(tag.size());
    return !url.empty() && std::all_of(url.begin(), url.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
    });
}

class AppPage : public Page {
public:
    PageInfo info() const override {
        return {"app", Group::App, T(L"Updates & Autostart", L"Updates & startup"),
                T(L"Programmversion, Update-Prüfung und Start mit Windows", L"Program version, update check and starting with Windows")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "ui", "update_check"); }
    void build(Host& host, Column& root) override {
        host_ = &host;
        form_ = Form(host);
        autostart_loaded_ = manual_ = false;
        Card* upd = root.add<Card>(T(L"Updates", L"Updates"));
        upd->row<Label>(T(L"Installierte Version", L"Installed version"), Text(), Text(widen(VERSION)), Font::Strong);
        form_.toggle(*upd, "ui", "update_check", T(L"Automatisch nach Updates suchen", L"Automatically check for updates"),
                     T(L"Wirkt nach „Speichern & anwenden“", L"Takes effect after “Save & apply”"));
        update_label_ = upd->row<Label>(T(L"Stand", L"State"), Text(), T(L"Wird geladen …", L"Loading …"), Font::Small, Label::Muted);
        auto row = upd->add<ButtonRow>();
        check_ = row->add(T(L"Jetzt nach Updates suchen", L"Check for updates now"), Button::Secondary, [this] { check_now(); });
        release_ = row->add(T(L"Release-Seite öffnen", L"Open release page"), Button::Secondary, [this] { open_release(); });
        release_->enabled = false;
        Card* start = root.add<Card>(T(L"Start mit Windows", L"Start with Windows"));
        autostart_ = start->row<Toggle>(T(L"Mit Windows starten", L"Start with Windows"),
            test_instance() ? T(L"In Testinstanzen gesperrt", L"Disabled in test instances")
                            : T(L"Nur im Infobereich; wird sofort übernommen", L"Tray only; applied immediately"), false);
        autostart_->track_dirty = false;
        autostart_->enabled = false;
        autostart_->on_change = [this] { apply_autostart(); };
        Card* vendor = root.add<Card>(T(L"Hersteller-Software des Displays", L"Display vendor software"),
            T(L"Die mitgelieferte Software des Displays belegt sonst den Anschluss. Beenden und Autostart abschalten wirken sofort.",
              L"The display's bundled software would otherwise occupy the port. Stopping it and disabling its autostart apply immediately."));
        vendor_state_ = vendor->row<Label>(T(L"Status", L"Status"), Text(), T(L"Wird geladen …", L"Loading …"), Font::Small, Label::Muted);
        auto vendor_row = vendor->add<ButtonRow>();
        vendor_stop_ = vendor_row->add(T(L"Hersteller-App beenden", L"Stop vendor app"), Button::Secondary, [this] { vendor_action(true); });
        vendor_off_ = vendor_row->add(T(L"Autostart der Hersteller-App abschalten", L"Disable vendor app autostart"), Button::Secondary,
                                      [this] { vendor_action(false); });
        vendor_stop_->enabled = vendor_off_->enabled = false;
        Card* credits = root.add<Card>(T(L"Nachweise", L"Credits"),
            T(L"Datenquellen der Seite „Streaming-Neustarts“", L"Data sources of the “New on streaming” page"));
        credits->add<Note>(Text(L"This product uses the TMDB API but is not endorsed or certified by TMDB."));
        credits->add<Note>(T(L"Anbieterdaten: JustWatch", L"Provider data: JustWatch"));
        credits->add<Note>(T(L"Anbieter-Logos: Marken der jeweiligen Anbieter", L"Provider logos: trademarks of the respective providers"));
        Card* control = root.add<Card>(T(L"ScreenBuddy steuern", L"Control ScreenBuddy"),
            T(L"Neu starten lädt alle Einstellungen neu. Beenden macht das Display dunkel.", L"Restart reloads all settings. Quit turns the display dark."));
        auto control_row = control->add<ButtonRow>();
        control_row->add(T(L"Neu starten", L"Restart"), Button::Secondary, [this] { control_app(false); });
        control_row->add(T(L"Beenden", L"Quit"), Button::Danger, [this] { control_app(true); });
        root.add<Note>(T(L"Sprache und Darstellung dieses Fensters stellst du oben rechts um (DE | EN, Hell | Dunkel). "
                         L"Beides wird mit „Speichern & anwenden“ dauerhaft übernommen.",
                         L"Switch the language and appearance of this window at the top right (DE | EN, Light | Dark). "
                         L"Both are stored permanently with “Save & apply”."));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void shown(Host&) override { refresh_vendor(); }
    void tick(Host&, const Status& s) override {
        if (!s.valid) return;
        if (manual_ && (s.update != update_before_ || std::chrono::steady_clock::now() >= manual_until_))
            manual_ = false;
        if (!manual_) show_update(s.update);
        if (s.autostart && !autostart_loaded_) {
            autostart_->on = autostart_->original = *s.autostart;
            autostart_loaded_ = true;
        }
        autostart_->enabled = autostart_loaded_ && !test_instance() && !host_->busy();
        check_->enabled = !host_->busy() && host_->config().at("ui").value("update_check", true);
        release_->enabled = !host_->busy() && !release_url_.empty();
    }
private:
    Form form_;
    Host* host_ = nullptr;
    Label* update_label_ = nullptr;
    Button *check_ = nullptr, *release_ = nullptr;
    std::wstring release_url_;
    Toggle* autostart_ = nullptr;
    bool autostart_loaded_ = false, manual_ = false;
    ojson update_before_;
    std::chrono::steady_clock::time_point manual_until_{};
    Label* vendor_state_ = nullptr;
    Button *vendor_stop_ = nullptr, *vendor_off_ = nullptr;
    bool vendor_running_ = false, vendor_autostart_ = false;

    void refresh_vendor() {
        if (!vendor_state_) return;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            const bool running = winsetup::vendor_running(), autostart = winsetup::vendor_autostart_enabled();
            return [this, running, autostart] {
                vendor_running_ = running;
                vendor_autostart_ = autostart;
                const bool locked = test_instance();
                vendor_state_->text = running || autostart
                    ? Text(std::wstring(running ? L"Läuft gerade. " : L"Läuft nicht. ") + (autostart ? L"Startet mit Windows." : L"Kein Autostart."),
                           std::wstring(running ? L"Running now. " : L"Not running. ") + (autostart ? L"Starts with Windows." : L"No autostart."))
                    : T(L"Nicht aktiv, kein Autostart – alles in Ordnung.", L"Not running, no autostart – all fine.");
                vendor_state_->tone = running || autostart ? Label::Danger : Label::Success;
                vendor_stop_->enabled = running && !locked;
                vendor_off_->enabled = autostart && !locked;
                host_->relayout();
            };
        });
    }
    void vendor_action(bool stop) {
        Host* h = host_;
        h->run([this, h, stop]() -> std::function<void()> {
            const bool ok = stop ? winsetup::vendor_stop() : winsetup::vendor_disable_autostart();
            return [this, h, ok, stop] {
                if (ok) h->toast(stop ? T(L"Hersteller-App beendet.", L"Vendor app stopped.") : T(L"Autostart der Hersteller-App abgeschaltet.", L"Vendor autostart disabled."), ToastKind::Success);
                else h->toast(stop ? T(L"Konnte die Hersteller-App nicht beenden.", L"Could not stop the vendor app.")
                                   : T(L"Die Windows-Abfrage wurde abgelehnt.", L"The Windows prompt was declined."), ToastKind::Error);
                refresh_vendor();
            };
        });
    }
    void control_app(bool quit) {
        const bool en = host_->english();
        const wchar_t* text = quit ? (en ? L"Quit ScreenBuddy? The display goes dark." : L"ScreenBuddy beenden? Das Display wird dunkel.")
                                   : (en ? L"Restart ScreenBuddy? Unsaved changes in this window are lost." : L"ScreenBuddy neu starten? Nicht gespeicherte Änderungen in diesem Fenster gehen verloren.");
        if (MessageBoxW(host_->window(), text, L"ScreenBuddy", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        Host* h = host_;
        h->run([h, quit]() -> std::function<void()> {
            if (quit) h->app().request_stop(0.4);
            else h->app().request_restart(0.4);
            return {};
        });
    }

    void show_update(const ojson& info) {
        release_url_.clear();
        if (info.is_object() && info.value("available", false)) {
            const std::string url = info.value("url", std::string());
            if (release_page_url(url)) release_url_ = widen(url);
        }
        if (info.is_null() || info.empty()) {
            update_label_->text = T(L"Update-Prüfung ist aus. Zum Einschalten oben aktivieren und speichern.",
                                    L"Update checks are off. Enable above and save to turn them on.");
            return;
        }
        std::wstring cur = widen(info.value("current", ""));
        const std::string error = info.value("error", "");
        if (!error.empty()) update_label_->text = Text(L"Aktuell " + cur + L" · " + widen(error), L"Current " + cur + L" · " + widen(error));
        else if (info.value("available", false))
            update_label_->text = Text(L"Neue Version verfügbar: " + widen(info.value("latest", "")), L"New version available: " + widen(info.value("latest", "")));
        else if (info.value("checked_at", 0LL) > 0) update_label_->text = T(L"Keine neuere Version gefunden.", L"No newer version found.");
        else update_label_->text = T(L"Noch nicht geprüft.", L"Not checked yet.");
    }
    void open_release() {
        if (release_url_.empty() || host_->busy()) return;
        const HINSTANCE result = ShellExecuteW(host_->window(), L"open", release_url_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32)
            host_->toast(T(L"Die Release-Seite konnte nicht geöffnet werden.", L"Could not open the release page."), ToastKind::Error);
    }
    void check_now() {
        if (host_->busy()) return;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            const ojson before = h->app().update_info();
            bool started = h->app().update_check_now();
            return [this, started, before] {
                manual_ = true;
                update_before_ = before;
                manual_until_ = std::chrono::steady_clock::now() + std::chrono::seconds(started ? 20 : 3);
                update_label_->text = started ? T(L"Update-Prüfung angefordert …", L"Update check requested …")
                                              : T(L"Update-Prüfung aus oder kürzlich angefordert (5 Minuten Pause).",
                                                  L"Update checks are off or were requested recently (5-minute interval).");
                host_->relayout();
            };
        });
    }
    void apply_autostart() {
        if (!autostart_loaded_ || test_instance()) return;
        const bool desired = autostart_->on;
        Host* h = host_;
        h->run([this, h, desired]() -> std::function<void()> {
            std::string error;
            bool actual = !desired;
            try {
                winsetup::set_autostart(desired);
                actual = winsetup::autostart_enabled();
                if (actual != desired) error = h->english() ? "Windows startup could not be changed." : "Windows-Start konnte nicht geändert werden.";
            } catch (const std::exception& e) { error = e.what(); }
            return [this, h, actual, error] {
                autostart_->on = autostart_->original = actual;
                if (error.empty()) h->toast(T(L"Windows-Start übernommen.", L"Windows startup applied."), ToastKind::Success);
                else h->toast(Text(widen(error)), ToastKind::Error);
            };
        });
    }
};

}  // namespace
std::unique_ptr<Page> make_app_page() { return std::make_unique<AppPage>(); }
}  // namespace fl::ui
