// © 2026 FuchsLabs
// App: Farben & Design (Display-Farbschema) sowie Updates & Autostart.
#include "ui_pages.h"

#include "app.h"
#include "util.h"
#include "winsetup.h"

namespace fl::ui {
namespace {

class DesignPage : public Page {
public:
    PageInfo info() const override {
        return {"design", Group::App, T(L"Farben & Design", L"Colors & design"),
                T(L"Farbschema des Displays – die Vorschau rechts zeigt die Wahl sofort", L"Color scheme of the display – the preview shows your choice right away")};
    }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* card = root.add<Card>();
        theme_ = form_.segment(*card, "display", "theme", T(L"Farbschema", L"Color scheme"),
            T(L"Gilt für USB-Display, Bildschirm und Companion", L"Applies to USB display, monitor and Companion"),
            {"hell", "dunkel", "eigen"}, {T(L"Hell", L"Light"), T(L"Dunkel", L"Dark"), T(L"Eigene Farben", L"Custom colors")});
        Host* h = &host;
        theme_->on_change = [this, h] {
            static const char* values[] = {"hell", "dunkel", "eigen"};
            std::string theme = values[theme_->index];
            h->run([h, theme]() -> std::function<void()> {
                auto frame = h->app().preview_theme({{"theme", theme}});
                return [h, frame] { if (frame) h->preview(frame); };
            });
        };
        root.add<Note>(T(L"„Eigene Farben“ verwendet die gespeicherten Farbwerte. Sie bleiben beim Speichern unverändert erhalten.",
                         L"“Custom colors” uses the saved color values. They are kept unchanged when saving."));
        placeholder(host, root, T(L"Eigene Farben bearbeiten", L"Edit custom colors"),
                    T(L"Vorlage (hell/dunkel) und sechs Farbwerte (#RRGGBB) mit Farbwähler und Live-Vorschau.",
                      L"Base (light/dark) and six color values (#RRGGBB) with color picker and live preview."));
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
    Segment* theme_ = nullptr;
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
        Card* start = root.add<Card>(T(L"Start mit Windows", L"Start with Windows"));
        autostart_ = start->row<Toggle>(T(L"Mit Windows starten", L"Start with Windows"),
            test_instance() ? T(L"In Testinstanzen gesperrt", L"Disabled in test instances")
                            : T(L"Nur im Infobereich; wird sofort übernommen", L"Tray only; applied immediately"), false);
        autostart_->track_dirty = false;
        autostart_->enabled = false;
        autostart_->on_change = [this] { apply_autostart(); };
        root.add<Note>(T(L"Sprache und Darstellung dieses Fensters stellst du oben rechts um (DE | EN, Hell | Dunkel). "
                         L"Beides wird mit „Speichern & anwenden“ dauerhaft übernommen.",
                         L"Switch the language and appearance of this window at the top right (DE | EN, Light | Dark). "
                         L"Both are stored permanently with “Save & apply”."));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        if (!s.valid) return;
        if (!manual_) show_update(s.update);
        if (s.autostart && !autostart_loaded_) {
            autostart_->on = autostart_->original = *s.autostart;
            autostart_loaded_ = true;
        }
        autostart_->enabled = autostart_loaded_ && !test_instance() && !host_->busy();
        check_->enabled = !host_->busy() && host_->config().at("ui").value("update_check", true);
    }
private:
    Form form_;
    Host* host_ = nullptr;
    Label* update_label_ = nullptr;
    Button* check_ = nullptr;
    Toggle* autostart_ = nullptr;
    bool autostart_loaded_ = false, manual_ = false;

    void show_update(const ojson& info) {
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
    void check_now() {
        if (host_->busy()) return;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            bool started = h->app().update_check_now();
            return [this, started] {
                manual_ = true;
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
