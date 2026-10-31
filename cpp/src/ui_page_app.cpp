// © 2026 FuchsLabs
// App: Farben & Design (Display-Farbschema) sowie Updates & Autostart.
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

class DesignPage : public Page {
public:
    PageInfo info() const override {
        return {"design", Group::App, T(L"Farben & Design", L"Colors & design"),
                T(L"Farbschema des Displays – die Vorschau rechts zeigt die Wahl sofort", L"Color scheme of the display – the preview shows your choice right away")};
    }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        previewing_ = again_ = false;
        visible_ = false;
        ++preview_generation_;
        Card* card = root.add<Card>();
        theme_ = form_.segment(*card, "display", "theme", T(L"Farbschema", L"Color scheme"),
            T(L"Gilt für USB-Display, Bildschirm und Companion", L"Applies to USB display, monitor and Companion"),
            {"hell", "dunkel", "eigen"}, {T(L"Hell", L"Light"), T(L"Dunkel", L"Dark"), T(L"Eigene Farben", L"Custom colors")});
        theme_->on_change = [this] { preview(); };

        Card* colors = root.add<Card>(T(L"Eigene Farben", L"Custom colors"),
            T(L"Gilt für das Display, sobald oben „Eigene Farben“ gewählt ist. Leere Farben kommen von der Vorlage. "
              L"Die Vorschau rechts zeigt die Änderung sofort, übernommen wird sie erst mit „Speichern & anwenden“.",
              L"Applies to the display once “Custom colors” is selected above. Empty colors come from the base. "
              L"The preview on the right shows changes right away; they are only kept with “Save & apply”."));
        base_ = form_.segment(*colors, "display", "color_base", T(L"Ausgangsfarben", L"Base colors"),
            T(L"Leere Farben kommen von dieser Vorlage", L"Empty colors come from this base"),
            {"hell", "dunkel"}, {T(L"Hell (Creme)", L"Light (cream)"), T(L"Dunkel (Navy)", L"Dark (navy)")});
        base_->on_change = [this] { refresh_fallbacks(); changed(); };
        const ojson& d = host.config().at("display");
        struct Def { const char* key; Text label; };
        const Def defs[] = {{"bg", T(L"Hintergrund", L"Background")}, {"card", T(L"Karten", L"Cards")},
                            {"header", T(L"Kopfzeile & Balken", L"Header & bars")}, {"accent", T(L"Akzentschrift", L"Accent text")},
                            {"text", T(L"Schrift", L"Text")}, {"muted", T(L"Nebentext", L"Secondary text")}};
        fields_.clear();
        for (const auto& def : defs) {
            std::string value = d.value(std::string("color_") + def.key, "");
            ColorField* f = colors->row<ColorField>(def.label, Text(), value);
            f->name = def.label;
            f->field.on_change = [this] { changed(); };
            fields_.push_back({def.key, f});
        }
        refresh_fallbacks(false);
        auto reset = colors->add<ButtonRow>();
        reset->add(T(L"Zurücksetzen", L"Reset"), Button::Secondary, [this] {
            for (auto& [key, f] : fields_) f->set_hex("");
            changed();
        });
        readable_ = root.add<Note>(Text(), Note::Info);
        update_readable(false);
    }
    void collect(Host&, ojson& patch) override {
        form_.collect(patch);
        for (auto& [key, f] : fields_) {
            if (!f->valid()) throw Invalid(host_->english() ? "Enter a color as #RRGGBB or leave it empty." : "Farbe als #RRGGBB eingeben oder leer lassen.", &f->field);
            patch["display"]["color_" + key] = f->hex();
        }
    }
    void shown(Host&) override { visible_ = true; }
    void hidden(Host&) override {
        visible_ = false;
        again_ = false;
        ++preview_generation_;
    }
private:
    Form form_;
    Host* host_ = nullptr;
    Segment *theme_ = nullptr, *base_ = nullptr;
    Note* readable_ = nullptr;
    std::vector<std::pair<std::string, ColorField*>> fields_;
    bool previewing_ = false, again_ = false;
    bool visible_ = false;
    uint64_t preview_generation_ = 0;

    std::string base_name() const { return base_ && base_->index == 1 ? "dunkel" : "hell"; }
    void refresh_fallbacks(bool layout = true) {
        const ojson presets = render::presets_json();
        const std::string base = base_name();
        for (auto& [key, f] : fields_) {
            f->fallback = presets.contains(base) ? presets[base].value(key, "") : "";
            f->field.hint = Text(widen(f->fallback));
            f->field.restyle(host_->surface());
        }
        if (layout) host_->surface().invalidate();
    }
    void changed() {
        update_readable();
        preview();
    }
    void update_readable(bool layout = true) {
        if (!readable_) return;
        std::map<std::string, std::string> colors;
        for (auto& [key, f] : fields_) colors[key] = f->effective();
        const auto warn = logic::color_warnings(colors);
        auto name = [](const std::string& id) {
            if (id == "text_bg") return T(L"Schrift auf Hintergrund", L"text on background");
            if (id == "text_card") return T(L"Schrift auf Karten", L"text on cards");
            if (id == "muted_bg") return T(L"Nebentext auf Hintergrund", L"secondary text on background");
            return T(L"Akzentschrift auf Hintergrund", L"accent text on background");
        };
        if (warn.empty()) {
            readable_->kind = Note::Info;
            readable_->text = T(L"Alles gut lesbar. Die Schrift auf der Kopfzeile wählt die App selbst (Navy oder Weiß).",
                                L"Everything is easy to read. The app picks the header text colour itself (navy or white).");
        } else {
            std::wstring de, en;
            for (const auto& id : warn) { de += (de.empty() ? L"" : L", ") + name(id).de; en += (en.empty() ? L"" : L", ") + name(id).en; }
            readable_->kind = Note::Warn;
            readable_->text = Text(L"Schwer lesbar: " + de + L". Wähle dort mehr Unterschied zwischen hell und dunkel.",
                                   L"Hard to read: " + en + L". Choose more difference between light and dark there.");
        }
        if (layout) host_->relayout();
    }
    // Live-Vorschau im Vorschaubereich; läuft nie doppelt, Änderungen währenddessen lösen genau einen Folgeaufruf aus.
    void preview() {
        if (!visible_) return;
        for (auto& [key, f] : fields_) if (!f->valid()) return;
        if (previewing_) { again_ = true; return; }
        previewing_ = true;
        again_ = false;
        static const char* themes[] = {"hell", "dunkel", "eigen"};
        ojson display = {{"theme", themes[std::clamp(theme_->index, 0, 2)]}, {"color_base", base_name()}};
        for (auto& [key, f] : fields_) display["color_" + key] = f->hex();
        Host* h = host_;
        const auto generation = preview_generation_;
        h->run([this, h, display, generation]() -> std::function<void()> {
            auto frame = h->app().preview_theme(display);
            return [this, h, frame, generation] {
                previewing_ = false;
                if (visible_ && generation == preview_generation_ && frame) h->preview(frame);
                if (visible_ && again_) preview();
            };
        });
    }
};
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
std::unique_ptr<Page> make_design_page() { return std::make_unique<DesignPage>(); }
std::unique_ptr<Page> make_app_page() { return std::make_unique<AppPage>(); }
}  // namespace fl::ui
