// © 2026 FuchsLabs
// Natives Einstellungsfenster (Shell): Kopfzeile, Navigation, Inhaltsbereich, Live-Vorschau und Fußleiste.
// Die Seiten selbst liegen in ui_page_*.cpp (Schnittstelle: ui_kit.h), das UI-Kit in ui_kit.cpp.
#include "native_settings.h"
#include "ui_shell_material.h"

#include <windows.h>
#include <dwmapi.h>
#include <objbase.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include "app.h"
#include "brand.h"
#include "config.h"
#include "ui_kit.h"
#include "ui_pages.h"
#include "util.h"
#include "winsetup.h"

namespace fl {
namespace {
constexpr UINT ShowMsg = WM_APP + 171, StopMsg = WM_APP + 172, ResultsMsg = WM_APP + 173;
constexpr UINT_PTR PollTimer = 1, ToastTimer = 2;
constexpr float HeaderH = 56, FooterH = 64, NavW = 232, PreviewW = 300;

// Rechte Seite „Gerät“: jedes verbundene Gerät als eigene anwählbare Karte; die Auswahl bestimmt die Live-Vorschau.
class DeviceList : public ui::Widget {
public:
    struct Entry { std::string id; ui::Text title, detail; bool ok = true; };
    std::vector<Entry> entries;
    std::string selected = "main";
    std::function<void(const std::string&)> on_pick;
    static constexpr float RowH = 54, Gap = 8;
    float height(ui::Ctx&, float) override { return entries.empty() ? 0 : entries.size() * (RowH + Gap) - Gap; }
    bool clickable() const override { return true; }
    ui::Role role() const override { return ui::Role::List; }
    void paint(ui::Ctx& c) override {
        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];
            const float top = rc.top + float(i) * (RowH + Gap);
            ui::Rect r{rc.left, top, rc.right, top + RowH};
            const bool sel = e.id == selected;
            c.fill(r, sel ? c.th.nav_active : (c.th.dark ? c.th.card : c.th.bg), 12);
            c.frame(r, sel ? c.th.accent : c.th.line, 12, sel ? 2.f : 1.f);
            c.circle({r.right - 16, r.top + RowH / 2}, 4, e.ok ? c.th.success : c.th.danger);
            c.text(c.s(e.title), {r.left + 14, r.top + 6, r.right - 30, r.top + 28}, ui::Font::Strong, c.th.text);
            c.text(c.s(e.detail), {r.left + 14, r.top + 28, r.right - 30, r.bottom - 4}, ui::Font::Small, c.th.muted);
        }
    }
    void release(ui::Point p, ui::Surface& s) override {
        if (p.x < rc.left || p.x > rc.right || p.y < rc.top) return;
        const size_t i = size_t((p.y - rc.top) / (RowH + Gap));
        if (i >= entries.size() || selected == entries[i].id) return;
        selected = entries[i].id;
        if (on_pick) on_pick(selected);
        s.invalidate();
    }
    void activate(ui::Surface&) override {}
};

// Rahmen-Widgets in Tab-Reihenfolge; die Lage berechnet die Shell.
class FrameRoot : public ui::Widget {
public:
    std::vector<ui::Widget*> order;
    void children(std::vector<ui::Widget*>& out) override { out.insert(out.end(), order.begin(), order.end()); }
    ui::Role role() const override { return ui::Role::Group; }
};
}  // namespace

struct NativeSettings::Impl : ui::Host {
    App& app_;
    NativePluginActions plugins_;
    ojson initial_;  // öffentliche Sicht: Zugangsdaten kommen nie in Eingabefelder
    std::thread ui_thread, job_thread;
    std::atomic<HWND> live_window{nullptr};
    std::atomic<bool> stopping{false}, shown{false}, poll_pending{false};
    std::mutex ready_mutex, jobs_mutex, results_mutex;
    std::condition_variable ready_cv, jobs_cv;
    bool ready = false;
    std::exception_ptr startup_error;
    std::deque<std::function<void()>> jobs;
    std::deque<std::function<void()>> results;

    // Ab hier nur im UI-Thread
    std::unique_ptr<ui::Kit> kit;
    std::unique_ptr<ui::Surface> frame, content;
    struct Slot {
        std::unique_ptr<ui::Page> page;
        ui::PageInfo info;
        ui::WidgetPtr root;
        ui::NavItem* nav = nullptr;
    };
    std::vector<Slot> slots;
    int current = -1;
    FrameRoot frame_root;
    std::vector<ui::WidgetPtr> frame_widgets;
    ui::Segment* lang = nullptr;
    ui::Segment* theme = nullptr;
    ui::Portal* portal = nullptr;
    ui::ImageView* preview_view = nullptr;
    DeviceList* device_list = nullptr;
    std::string preview_device = "main";   // Gerät, dessen Bild die Live-Vorschau zeigt (UI-Thread)
    ui::Button* discard_button = nullptr;
    ui::Button* save_button = nullptr;
    ui::Button *inspector_button = nullptr, *sidebar_button = nullptr;
    ui::TextField* nav_search = nullptr;
    ui::Segment* material_tabs = nullptr;
    ui::Combo* material_page_menu = nullptr;
    std::array<ui::NavItem*, 5> destinations{};
    std::vector<int> material_tab_slots;
    ui::shell::MaterialShell material_layout;
    bool inspector_open = true, sidebar_collapsed = false;
    std::vector<int> page_history;
    bool navigating_back = false;
    ui::Status status_;
    std::optional<Image> override_frame;
    bool preview_visible = true;
    int dirty_count = 0, busy_count = 0;
    bool saving = false;
    ui::Text toast_text;
    ui::ToastKind toast_kind = ui::ToastKind::Info;

    Impl(App& a, NativePluginActions p) : app_(a), plugins_(std::move(p)), initial_(config::public_view(a.cfg())) {}
    ~Impl() override { stop(); }

    // ---------------- Host
    App& app() override { return app_; }
    const ojson& config() const override { return initial_; }
    NativePluginActions& plugins() override { return plugins_; }
    bool english() const override { return kit && kit->english; }
    bool busy() const override { return busy_count > 0 || saving; }
    HWND window_handle() const { return frame ? frame->hwnd() : nullptr; }
    HWND window() const override { return window_handle(); }
    const ui::Status& status() const override { return status_; }
    ui::Surface& surface() override { return *content; }
    void relayout() override { if (content) content->relayout(); recount(); }
    void toast(const ui::Text& text, ui::ToastKind kind = ui::ToastKind::Info) override {
        toast_text = text; toast_kind = kind;
        if (frame) {
            SetTimer(frame->hwnd(), ToastTimer, kind == ui::ToastKind::Error ? 9000 : 4500, nullptr);
            frame->invalidate();
            // Sprachausgabe: Hinweis als Namensänderung des Fensters ankündigen
            NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, frame->hwnd(), OBJID_CLIENT, CHILDID_SELF);
        }
    }
    void run(std::function<std::function<void()>()> job) override {
        ++busy_count;
        refresh_footer();
        enqueue([this, job = std::move(job)] {
            std::function<void()> next;
            std::wstring error;
            try { next = job(); }
            catch (const std::exception& e) { error = widen(e.what()); }
            catch (...) { error = L"?"; }
            deliver([this, next = std::move(next), error] {
                --busy_count;
                if (!error.empty()) toast(ui::Text(error == L"?" ? std::wstring(english() ? L"The action failed." : L"Die Aktion ist fehlgeschlagen.") : error),
                                          ui::ToastKind::Error);
                else if (next) next();
                recount();
                refresh_footer();
                if (content) content->invalidate();
            });
        });
    }
    void navigate(const std::string& id) override {
        for (size_t i = 0; i < slots.size(); ++i) if (slots[i].info.id == id) { open(int(i), false); return; }
    }
    void preview(std::optional<Image> f) override {
        override_frame = std::move(f);
        show_frame(override_frame ? override_frame : app_frame);
    }
    std::optional<Image> app_frame;

    // ---------------- Threads (wie bisher: UI-Thread + Arbeits-Thread)
    void start() {
        job_thread = std::thread([this] { work(); });
        try { ui_thread = std::thread([this] { run_ui(); }); }
        catch (...) { stop(); throw; }
        std::unique_lock lock(ready_mutex);
        ready_cv.wait(lock, [this] { return ready; });
        if (startup_error) {
            auto error = startup_error;
            lock.unlock();
            stop();
            std::rethrow_exception(error);
        }
    }
    void stop() {
        stopping = true;
        jobs_cv.notify_all();
        if (HWND w = live_window.load()) PostMessageW(w, StopMsg, 0, 0);
        if (ui_thread.joinable()) ui_thread.join();
        if (job_thread.joinable()) job_thread.join();
    }
    void signal_ready(std::exception_ptr error = {}) {
        { std::lock_guard lock(ready_mutex); startup_error = error; ready = true; }
        ready_cv.notify_all();
    }
    void enqueue(std::function<void()> action) {
        { std::lock_guard lock(jobs_mutex); if (stopping) return; jobs.push_back(std::move(action)); }
        jobs_cv.notify_one();
    }
    void deliver(std::function<void()> next) {
        { std::lock_guard lock(results_mutex); if (stopping) return; results.push_back(std::move(next)); }
        if (HWND w = live_window.load()) PostMessageW(w, ResultsMsg, 0, 0);
    }
    void work() {
        for (;;) {
            std::function<void()> action;
            {
                std::unique_lock lock(jobs_mutex);
                jobs_cv.wait(lock, [this] { return stopping || !jobs.empty(); });
                if (stopping) { jobs.clear(); return; }
                action = std::move(jobs.front()); jobs.pop_front();
            }
            try { action(); } catch (...) {}
        }
    }
    void consume() {
        std::deque<std::function<void()>> pending;
        { std::lock_guard lock(results_mutex); pending.swap(results); }
        for (auto& next : pending) {
            try { next(); }
            catch (const std::exception& e) { toast(ui::Text(widen(e.what())), ui::ToastKind::Error); }
        }
    }

    // ---------------- Status (einmal pro Sekunde, im Arbeits-Thread)
    void poll() {
        if (stopping || !shown || poll_pending.exchange(true)) return;
        const bool need_autostart = !status_.autostart.has_value();
        const std::string output = initial_.at("display").value("output", "serial");
        enqueue([this, need_autostart, output, device = preview_device] {
            struct Reset { std::atomic<bool>& v; ~Reset() { v = false; } } reset{poll_pending};
            ui::Status s;
            std::optional<Image> f;
            try {
                f = device == "main" ? app_.last_frame() : app_.device_frame(device);
                s.size = app_.size();
                s.display_ok = app_.display_ok();
                s.display_error = app_.display_error();
                s.connections = app_.connections();
                s.update = app_.update_info();
                s.navigation = app_.page_navigation();
                s.output = output;
                if (auto* companion = app_.companion()) s.companion = companion->status();
                if (need_autostart) s.autostart = winsetup::autostart_enabled();
                s.valid = true;
            } catch (...) {}
            deliver([this, s = std::move(s), f = std::move(f)]() mutable {
                auto autostart = status_.autostart;
                status_ = std::move(s);
                if (!status_.autostart) status_.autostart = autostart;
                if (f) app_frame = std::move(f);
                if (!override_frame) show_frame(app_frame);
                update_device();
                if (current >= 0) slots[size_t(current)].page->tick(*this, status_);
                frame->invalidate();
                content->invalidate();
            });
        });
    }
    void show_frame(const std::optional<Image>& f) {
        if (!preview_view) return;
        bool had = preview_view->image.has_value();
        preview_view->image = f;
        ++preview_view->version;
        if (had != f.has_value()) frame->relayout();
        else frame->invalidate();
    }
    std::wstring output_name(bool en) const {
        const std::string o = initial_.at("display").value("output", "serial");
        if (o == "monitor") return en ? L"Monitor" : L"Bildschirm";
        if (o == "network") return en ? L"Companion / network" : L"Companion / Netzwerk";
        return L"USB-Display";
    }
    void update_device() {
        if (!device_list) return;
        // Jedes verbundene Gerät eine eigene Karte: Hauptdisplay (USB/Monitor) und jede verbundene Companion-App.
        std::vector<DeviceList::Entry> list;
        const std::string o = initial_.at("display").value("output", "serial");
        const bool independent = status_.companion.is_object() && status_.companion.value("independent", false);
        if (o != "network") {
            std::wstring size = status_.size.first > 0 ? std::to_wstring(status_.size.first) + L" × " + std::to_wstring(status_.size.second) : L"–";
            DeviceList::Entry e{"main", ui::Text(output_name(false), output_name(true)), {}, status_.display_ok};
            std::wstring de = size + (status_.display_ok ? L" · verbunden" : L" · nicht verbunden");
            std::wstring en = size + (status_.display_ok ? L" · connected" : L" · not connected");
            if (!status_.display_error.empty() && !status_.display_ok) { de = widen(status_.display_error); en = de; }
            e.detail = ui::Text(de, en);
            list.push_back(std::move(e));
        }
        const ojson& cc = status_.companion;
        if (cc.is_object() && cc.contains("clients") && cc["clients"].is_array()) {
            for (const auto& x : cc["clients"]) {
                const std::string id = x.value("id", std::string());
                std::wstring ip = widen(x.value("ip", std::string()));
                std::wstring de = L"Companion · " + ip, en = de;
                if (x.contains("navigation") && x["navigation"].is_object() && x["navigation"].value("total", 0) > 0) {
                    std::wstring pos = L" · " + std::to_wstring(x["navigation"].value("index", 0) + 1) + L"/" + std::to_wstring(x["navigation"].value("total", 0));
                    de += pos; en += pos;
                }
                list.push_back({id, ui::Text(widen(x.value("name", std::string("Companion")))), ui::Text(de, en), true});
            }
        }
        (void)independent;
        if (list.empty()) list.push_back({"none", ui::Text(L"Kein Gerät verbunden", L"No device connected"), {}, false});
        // Auswahl beibehalten, sonst das erste Gerät
        bool found = false;
        for (auto& e : list) if (e.id == device_list->selected) found = true;
        if (!found) { device_list->selected = list.front().id; preview_device = device_list->selected == "none" ? "main" : device_list->selected; }
        device_list->entries = std::move(list);
        frame->relayout();
    }
    // ---------------- Seiten
    bool material() const { return kit->design.style == fl::theme::Style::Material; }
    bool cupertino() const { return kit->design.style == fl::theme::Style::Cupertino; }
    bool fluent() const { return kit->design.style == fl::theme::Style::Fluent; }
    int destination(const Slot& slot) const {
        const auto& id = slot.info.id;
        if (id == "overview") return 0;
        if (id == "display" || id == "pages") return 1;
        if (id == "plugins" || id == "store") return 3;
        if (id == "companion" || id == "design" || id == "app") return 4;
        return 2;
    }
    bool nav_matches(const Slot& slot) const {
        if (!fluent() || !nav_search || nav_search->text().empty()) return true;
        const auto query = nav_search->text(), title = slot.info.title.get(kit->english);
        for (size_t i = 0; i + query.size() <= title.size(); ++i)
            if (CompareStringOrdinal(title.data() + i, int(query.size()), query.data(), int(query.size()), TRUE) == CSTR_EQUAL) return true;
        return false;
    }
    void update_material_tabs() {
        if (!material_tabs || current < 0) return;
        const int group = destination(slots[size_t(current)]);
        material_tab_slots.clear(); material_tabs->options.clear();
        for (size_t i = 0; i < slots.size(); ++i) if (destination(slots[i]) == group) {
            if (int(i) == current) material_tabs->index = int(material_tab_slots.size());
            material_tab_slots.push_back(int(i)); material_tabs->options.push_back(slots[i].info.title);
        }
        material_page_menu->options = material_tabs->options;
        material_page_menu->index = material_tabs->index;
        for (int i = 0; i < 5; ++i) if (destinations[size_t(i)]) destinations[size_t(i)]->active = i == group;
    }
    ui::Widget* material_navigation() {
        if (material_page_menu->visible) return material_page_menu;
        if (material_tabs->visible) return material_tabs;
        return destinations[size_t(destination(slots[size_t(current)]))];
    }
    void build(Slot& slot) {
        auto root = std::make_unique<ui::Column>();
        root->pad_x = cupertino() ? 32.f : material() ? 8.f : 26.f;
        root->pad_y = material() ? 8.f : 22.f; root->gap = cupertino() || material() ? 24.f : 14.f;
        if (!material()) root->add<ui::Heading>(slot.info.title, slot.info.subtitle);
        slot.page->build(*this, *root);
        slot.root = std::move(root);
    }
    void open(int index, bool keyboard) {
        if (index < 0 || index >= int(slots.size())) return;
        if (current >= 0 && current != index && !navigating_back) page_history.push_back(current);
        auto& slot = slots[size_t(index)];
        if (!slot.root) build(slot);
        if (current >= 0 && current != index) {
            slots[size_t(current)].page->hidden(*this);
            slots[size_t(current)].nav->active = false;
            if (override_frame) preview(std::nullopt);
        }
        current = index;
        update_material_tabs();
        slot.nav->active = true;
        content->set_root(slot.root.get());
        slot.page->shown(*this);
        if (status_.valid) slot.page->tick(*this, status_);
        frame->notify(slot.nav, EVENT_OBJECT_SELECTION);
        frame->relayout();
        frame->focus(material() ? material_navigation() : sidebar_collapsed ? static_cast<ui::Widget*>(sidebar_button) : slot.nav, keyboard);
        frame->invalidate();
        recount();
    }
    void step(int delta) {
        int next = std::clamp(current + delta, 0, int(slots.size()) - 1);
        while (next >= 0 && next < int(slots.size()) && !nav_matches(slots[size_t(next)])) next += delta;
        open(next, true);
    }

    // ---------------- Änderungen, Speichern, Verwerfen
    void recount() {
        int n = 0;
        for (auto& s : slots) n += ui::count_dirty(s.root.get());
        if (lang && lang->dirty()) ++n;
        if (theme && theme->dirty()) ++n;
        dirty_count = n;
        refresh_footer();
    }
    void refresh_footer() {
        if (!save_button) return;
        save_button->enabled = dirty_count > 0 && !busy();
        discard_button->enabled = dirty_count > 0 && !busy();
        save_button->name = english() ? ui::Text(L"Save & apply (" + std::to_wstring(dirty_count) + (dirty_count == 1 ? L" change)" : L" changes)"))
                                      : ui::Text(L"Speichern & anwenden (" + std::to_wstring(dirty_count) + (dirty_count == 1 ? L" Änderung)" : L" Änderungen)"));
        if (material()) frame->relayout();
        frame->invalidate();
    }
    void apply_language() {
        kit->english = lang->index == 1;
        SetWindowTextW(frame->hwnd(), kit->english ? L"ScreenBuddy – Settings" : L"ScreenBuddy – Einstellungen");
        frame->restyle();
        content->restyle();
        update_device();
    }
    void apply_theme() {
        kit->use_design(initial_["display"]);
        kit->use_windows(theme->index == 2);
        if (theme->index != 2) kit->dark = theme->index == 1;
        BOOL dark = kit->dark;
        DwmSetWindowAttribute(frame->hwnd(), 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
        apply_caption();
        frame->restyle();
        content->restyle();
    }
    void apply_caption() {
        const auto& th = kit->theme();
        auto color = [](ui::Rgba c) { return RGB(int(c.r * 255 + .5f), int(c.g * 255 + .5f), int(c.b * 255 + .5f)); };
        COLORREF caption = color(th.header), text = color(th.header_text);
        const DWORD backdrop = fluent() ? 2u /* DWMSBT_MAINWINDOW */ : 1u /* DWMSBT_NONE */;
        const HRESULT material_result = DwmSetWindowAttribute(frame->hwnd(), 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &backdrop, sizeof(backdrop));
        if (fluent() && SUCCEEDED(material_result)) caption = text = 0xFFFFFFFFu; // let DWM draw the supported Mica caption
        DwmSetWindowAttribute(frame->hwnd(), 35, &caption, sizeof(caption));
        DwmSetWindowAttribute(frame->hwnd(), 36, &text, sizeof(text));
    }
    void save() {
        if (busy() || dirty_count == 0) return;
        ojson patch = ojson::object();
        for (size_t i = 0; i < slots.size(); ++i) {
            auto& s = slots[i];
            if (!s.root) continue;
            try { s.page->collect(*this, patch); }
            catch (const ui::Invalid& e) {
                open(int(i), false);
                if (e.widget) content->focus(e.widget, true);
                toast(ui::Text(widen(e.what())), ui::ToastKind::Error);
                return;
            } catch (const std::exception& e) {
                open(int(i), false);
                toast(ui::Text(widen(e.what())), ui::ToastKind::Error);
                return;
            }
        }
        patch["ui"]["language"] = lang->index == 1 ? "en" : "de";
        if (theme->dirty()) patch["ui"]["settings_theme"] = theme->index == 2 ? "windows" : theme->index == 1 ? "dunkel" : "hell";
        patch["ui"]["setup_done"] = true;
        try { config::merge(initial_, patch); }  // vor dem Speichern prüfen
        catch (const std::exception& e) { toast(ui::Text(widen(e.what())), ui::ToastKind::Error); return; }
        saving = true;
        refresh_footer();
        toast(ui::T(L"Wird gespeichert …", L"Saving …"));
        enqueue([this, patch = std::move(patch)] {
            try {
                auto merged = config::merge(app_.cfg(), patch);  // vorhandene Geheimnisse und fremde Felder bleiben
                config::save(app_.cfg_path(), merged);           // DPAPI + atomares Ersetzen wie bisher
            } catch (const std::exception& e) {
                std::wstring error = widen(e.what());
                deliver([this, error] { saving = false; refresh_footer(); toast(ui::Text(error), ui::ToastKind::Error); });
                return;
            }
            deliver([this] { toast(ui::T(L"Gespeichert. ScreenBuddy startet neu …", L"Saved. ScreenBuddy is restarting …"), ui::ToastKind::Success); });
            if (!stopping) app_.request_restart(0.4);
        });
    }
    void discard() {
        if (busy() || dirty_count == 0) return;
        if (current >= 0) slots[size_t(current)].page->hidden(*this);
        preview(std::nullopt);
        for (auto& s : slots) {
            if (!s.root) continue;
            std::vector<ui::WidgetPtr> old;
            old.push_back(std::move(s.root));
            ui::retire(old);
        }
        if (lang->dirty()) { lang->index = lang->original; apply_language(); }
        if (theme->dirty()) { theme->index = theme->original; apply_theme(); }
        int keep = current;
        current = -1;
        content->set_root(nullptr);
        open(keep < 0 ? 0 : keep, false);
        toast(ui::T(L"Änderungen verworfen.", L"Changes discarded."));
    }
    bool confirm_close() {
        if (dirty_count == 0 || saving) return true;
        int answer = MessageBoxW(frame->hwnd(),
            english() ? L"There are unsaved changes. Save and apply them now?\n\nNo = discard, Cancel = keep editing."
                      : L"Es gibt ungespeicherte Änderungen. Jetzt speichern und anwenden?\n\nNein = verwerfen, Abbrechen = weiter bearbeiten.",
            L"ScreenBuddy", MB_YESNOCANCEL | MB_ICONQUESTION);
        if (answer == IDYES) { save(); return false; }
        if (answer == IDNO) { discard(); return true; }
        return false;
    }

    // ---------------- Rahmen: Aufbau, Layout, Zeichnen
    template<class W, class... A> W* own(A&&... a) {
        auto w = std::make_unique<W>(std::forward<A>(a)...);
        W* raw = w.get(); frame_widgets.push_back(std::move(w)); return raw;
    }
    void build_frame() {
        auto logo = own<ui::ImageView>();
        logo->image = fox_image(56, false);
        logo->framed = false;
        logo->name = ui::Text(L"ScreenBuddy");
        const std::string settings_theme = initial_.at("ui").value("settings_theme", "auto");
        const bool windows = settings_theme == "windows";
        kit->use_design(initial_["display"]);
        kit->use_windows(windows);
        inspector_open = !cupertino() && !material();
        const bool dark = windows ? kit->dark : (settings_theme == "dunkel" || (settings_theme != "hell" && ui::system_prefers_dark()));
        kit->dark = dark;
        kit->english = initial_.at("ui").value("language", "de") == "en";
        lang = own<ui::Segment>(std::vector<ui::Text>{ui::Text(L"DE"), ui::Text(L"EN")}, kit->english ? 1 : 0);
        lang->header_style = true;
        lang->name = ui::T(L"Sprache", L"Language");
        lang->on_change = [this] { apply_language(); };
        theme = own<ui::Segment>(std::vector<ui::Text>{ui::T(L"☀ Hell", L"☀ Light"), ui::T(L"☾ Dunkel", L"☾ Dark"), ui::Text(L"Windows")}, windows ? 2 : dark ? 1 : 0);
        theme->header_style = true;
        theme->name = ui::T(L"Darstellung des Fensters", L"Window appearance");
        theme->on_change = [this] { apply_theme(); };
        frame_root.order = {lang, theme};
        inspector_button = own<ui::Button>(material() ? ui::Text(L"◧") : ui::T(L"Vorschau", L"Preview"), ui::Button::Ghost,
            [this] { inspector_open = !inspector_open; frame->relayout(); frame->invalidate(); });
        inspector_button->name = ui::T(L"Geräte-Inspector ein-/ausblenden", L"Toggle device inspector");
        sidebar_button = own<ui::Button>(ui::Text(fluent() ? L"‹" : L"☰"), ui::Button::Ghost,
            [this] {
                if (fluent()) {
                    if (page_history.empty()) return;
                    const int previous = page_history.back(); page_history.pop_back();
                    navigating_back = true; open(previous, true); navigating_back = false;
                } else { sidebar_collapsed = !sidebar_collapsed; frame->relayout(); frame->focus(sidebar_button, true); frame->invalidate(); }
            });
        sidebar_button->name = fluent() ? ui::T(L"Zurück zur vorherigen Seite", L"Back to previous page") : ui::T(L"Seitenleiste ein-/ausblenden", L"Toggle sidebar");
        nav_search = own<ui::TextField>(ui::TextField::Single, std::wstring());
        nav_search->name = ui::T(L"Einstellungsseiten suchen", L"Find settings pages");
        nav_search->hint = ui::T(L"Einstellungen suchen", L"Find settings");
        nav_search->track_dirty = false; nav_search->limit = 80;
        nav_search->on_change = [this] { frame->relayout(); frame->invalidate(); };
        frame_root.order.insert(frame_root.order.end(), {sidebar_button, inspector_button, nav_search});
        for (auto& page : ui::create_pages()) {
            Slot slot;
            slot.info = page->info();
            slot.page = std::move(page);
            slots.push_back(std::move(slot));
        }
        std::stable_sort(slots.begin(), slots.end(), [](const Slot& a, const Slot& b) { return int(a.info.group) < int(b.info.group); });
        for (size_t i = 0; i < slots.size(); ++i) {
            auto nav = own<ui::NavItem>(slots[i].info.title, slots[i].info.id);
            nav->has_dot = slots[i].page->shows_status();
            nav->dot_on = nav->has_dot && slots[i].page->active(initial_);
            if (nav->has_dot)
                nav->dot_hint = nav->dot_on ? ui::T(L"Eingerichtet und eingeschaltet", L"Set up and switched on")
                                            : ui::T(L"Nicht eingerichtet oder ausgeschaltet", L"Not set up or switched off");
            nav->on_open = [this, i] { open(int(i), false); };
            nav->on_step = [this](int d) { step(d); };
            slots[i].nav = nav;
            frame_root.order.push_back(nav);
        }
        const std::array<ui::Text,5> areas = {ui::T(L"Übersicht", L"Overview"), ui::T(L"Display", L"Display"),
            ui::T(L"Quellen", L"Sources"), ui::T(L"Erweiterungen", L"Extensions"), ui::T(L"Einstellungen", L"Settings")};
        for (int group = 0; group < 5; ++group) {
            auto item = own<ui::NavItem>(areas[size_t(group)], "destination-" + std::to_string(group));
            item->rail = true; item->icon = group; item->has_dot = false;
            item->on_open = [this, group] {
                for (size_t i = 0; i < slots.size(); ++i) if (destination(slots[i]) == group) { open(int(i), false); break; }
            };
            item->on_step = [this, group](int delta) {
                const int next = std::clamp(group + delta, 0, 4);
                for (size_t i = 0; i < slots.size(); ++i) if (destination(slots[i]) == next) { open(int(i), true); frame->focus(destinations[size_t(next)], true); break; }
            };
            destinations[size_t(group)] = item; frame_root.order.push_back(item);
        }
        material_tabs = own<ui::Segment>(std::vector<ui::Text>{}, 0);
        material_tabs->name = ui::T(L"Seiten im gewählten Bereich", L"Pages in selected destination");
        material_tabs->track_dirty = false;
        material_tabs->on_change = [this] {
            const int index = material_tabs->index;
            if (index >= 0 && index < int(material_tab_slots.size())) open(material_tab_slots[size_t(index)], false);
        };
        frame_root.order.push_back(material_tabs);
        material_page_menu = own<ui::Combo>(std::vector<ui::Text>{}, 0);
        material_page_menu->name = material_tabs->name;
        material_page_menu->track_dirty = false;
        material_page_menu->on_change = [this] {
            const int index = material_page_menu->index;
            if (index >= 0 && index < int(material_tab_slots.size())) open(material_tab_slots[size_t(index)], false);
        };
        frame_root.order.push_back(material_page_menu);
        portal = own<ui::Portal>();
        portal->target = content.get();
        frame_root.order.push_back(portal);
        preview_view = own<ui::ImageView>();
        preview_view->name = ui::T(L"Live-Vorschau des Displays", L"Live display preview");
        preview_view->empty_text = ui::T(L"Noch kein Displaybild verfügbar.", L"No display frame available yet.");
        device_list = own<DeviceList>();
        device_list->name = ui::T(L"Verbundene Geräte", L"Connected devices");
        device_list->on_pick = [this](const std::string& id) {
            preview_device = id == "none" ? "main" : id;
            app_frame.reset();
            show_frame(std::nullopt);
            poll();
        };

        discard_button = own<ui::Button>(ui::T(L"Verwerfen", L"Discard"), ui::Button::Secondary, [this] { discard(); });
        save_button = own<ui::Button>(material() ? ui::T(L"Speichern", L"Save") : ui::T(L"Speichern & anwenden", L"Save & apply"), ui::Button::Primary, [this] { save(); });
        frame_root.order.insert(frame_root.order.end(), {discard_button, save_button, preview_view, device_list, logo});
        logo_ = logo;
        frame->set_root(&frame_root);
        frame->on_layout = [this](ui::Ctx& c, D2D1_SIZE_F sz) { return layout_frame(c, sz); };
        frame->on_background = [this](ui::Ctx& c, D2D1_SIZE_F sz) { paint_frame(c, sz); };
        frame->on_overlay = [this](ui::Ctx& c, D2D1_SIZE_F sz) { paint_toast(c, sz); };
        content->on_tab_out = [this](bool forward) {
            frame->keyboard_cues = true;
            SetFocus(frame->hwnd());
            frame->mark(portal);
            frame->tab(forward);
        };
        content->on_changed = [this](ui::Widget*) { recount(); };
        frame->on_changed = [this](ui::Widget*) { recount(); };
        content->on_focus_in = [this] { frame->invalidate(); };
        update_device();
    }
    ui::ImageView* logo_ = nullptr;
    float nav_y0 = 0;

    float nav_width() const { return cupertino() ? (sidebar_collapsed ? 0.f : 208.f) : fluent() ? 256.f : NavW; }
    float header_height() const { return fluent() ? 104.f : HeaderH; }
    float footer_height() const { return cupertino() ? 52.f : fluent() ? 56.f : FooterH; }
    void place(ui::Ctx& c, ui::Widget* widget, ui::Rect r) {
        widget->visible = r.right > r.left && r.bottom > r.top;
        if (widget->visible) { widget->layout(c, r.left, r.top, r.right - r.left); widget->rc = r; }
    }
    void place_content(ui::Rect r) {
        SetWindowPos(content->hwnd(), nullptr, frame->px(r.left), frame->px(r.top),
            std::max(1, frame->px(r.right - r.left)), std::max(1, frame->px(r.bottom - r.top)), SWP_NOZORDER | SWP_NOACTIVATE);
    }
    void material_frame(ui::Ctx& c, D2D1_SIZE_F sz) {
        material_layout = ui::shell::layout_material(sz.width, sz.height, inspector_open, dirty_count > 0,
            theme->pref_width(c), lang->pref_width(c));
        if (material_tab_slots.size() <= 1) { material_layout.tabs = {}; material_layout.body.top = 152; }
        const auto& m = material_layout;
        for (auto& slot : slots) slot.nav->visible = false;
        sidebar_button->visible = nav_search->visible = false;
        for (size_t i = 0; i < destinations.size(); ++i) place(c, destinations[i], m.destinations[i]);
        const bool compact_tabs = material_tabs->pref_width(c) > m.tabs.right - m.tabs.left;
        place(c, material_tabs, compact_tabs ? ui::Rect{} : m.tabs);
        place(c, material_page_menu, compact_tabs ? m.tabs : ui::Rect{});
        place(c, theme, m.appearance); place(c, lang, m.language);
        place(c, inspector_button, m.preview_toggle); place(c, save_button, m.save); place(c, discard_button, m.discard);
        logo_->visible = true; logo_->layout(c, 32, 28, 32);
        preview_visible = m.inspector_visible;
        preview_view->visible = device_list->visible = preview_visible;
        if (preview_visible) {
            const float x = m.inspector.left + 16, width = m.inspector.right - x - 16;
            const float height = preview_view->layout(c, x, m.inspector.top + 36, width);
            preview_view->rc.bottom = preview_view->rc.top + std::min(height, 260.f);
            const float y = preview_view->rc.bottom + 60;
            const float dh = device_list->layout(c, x, y, width);
            device_card_ = {x, y, x + width, y + dh};
        }
        place_content(m.body);
    }

    float layout_frame(ui::Ctx& c, D2D1_SIZE_F sz) {
        const float w = sz.width, h = sz.height;
        if (material()) { material_frame(c, sz); return h; }
        const float NavW = nav_width(), HeaderH = header_height(), FooterH = footer_height();
        for (auto* item : destinations) item->visible = false;
        material_tabs->visible = false;
        material_page_menu->visible = false;
        inspector_button->visible = cupertino() || fluent();
        sidebar_button->visible = cupertino() || fluent(); sidebar_button->enabled = !fluent() || !page_history.empty();
        nav_search->visible = fluent();
        preview_visible = kit->design.style == fl::theme::Style::FuchsLabs ? w >= 1060 : inspector_open;
        logo_->visible = NavW > 0;
        logo_->layout(c, 22, fluent() ? 13 : (HeaderH - 26) / 2, 26);
        float tw = theme->pref_width(c), lw = lang->pref_width(c);
        const float controls_y = fluent() ? 12.f : (HeaderH - 28) / 2;
        theme->layout(c, w - 18 - tw, controls_y, tw);
        lang->layout(c, w - 18 - tw - 10 - lw, controls_y, lw);
        if (inspector_button->visible) inspector_button->layout(c, lang->rc.left - 106, controls_y - 4, 96);
        if (sidebar_button->visible) sidebar_button->layout(c, NavW + 10, controls_y - 4, 38);
        if (fluent()) nav_search->layout(c, 16, 60, NavW - 32);
        // Navigation in Gruppen
        // Passt die Navigation nicht in die Höhe, werden Einträge und Gruppenabstände enger (statt unter die Fußleiste zu laufen).
        int groups = 0, last_group = -1;
        for (auto& s : slots) if (int(s.info.group) != last_group) { last_group = int(s.info.group); ++groups; }
        const float avail = h - FooterH - (HeaderH + 12) - 6 - 24;   // 24: Legende der Statuspunkte
        float gap0 = 18, gapN = 26, item_h = 36;
        const bool filtered = fluent() && !nav_search->text().empty();
        size_t visible_items = 0;
        for (auto& slot : slots) if (nav_matches(slot)) ++visible_items;
        if (cupertino() || fluent()) { gap0 = 16; gapN = 26; item_h = 34; }   // gapN >= 26: Gruppenüberschrift (22 über dem Eintrag) überlappt den vorherigen Eintrag nicht
        if (filtered) gap0 = gapN = 0;
        float need = visible_items * item_h + gap0 + gapN * std::max(0, groups - 1);
        if (need > avail) {
            if (cupertino() || fluent()) item_h = std::max(20.f, (avail - gap0 - gapN * std::max(0, groups - 1)) / std::max(1.f, float(visible_items)));
            else {
                float k = avail / need;
                item_h = std::max(26.f, item_h * k);
                gap0 = std::max(16.f, gap0 * k); gapN = std::max(22.f, gapN * k);
            }
        }
        float y = HeaderH + 12;
        int group = -1;
        bool first_result = true;
        for (auto& s : slots) {
            s.nav->visible = NavW > 0 && nav_matches(s);
            s.nav->search_result = filtered && first_result && s.nav->visible;
            if (!s.nav->visible) continue;
            first_result = false;
            if (int(s.info.group) != group) { group = int(s.info.group); if (!filtered) y += group == 0 ? gap0 : gapN; }
            s.nav->layout(c, 10, y, NavW - 20);
            s.nav->rc.bottom = s.nav->rc.top + std::min(34.f, item_h - 2);
            y += item_h;
        }
        nav_y0 = HeaderH + 12;
        // Fußleiste
        float sw = save_button->pref_width(c) + 8, dw = discard_button->pref_width(c);
        float by = h - FooterH + (FooterH - 36) / 2;
        save_button->layout(c, w - 18 - sw, by, sw);
        discard_button->layout(c, w - 18 - sw - 12 - dw, by, dw);
        // Vorschau
        for (ui::Widget* x : {static_cast<ui::Widget*>(preview_view), static_cast<ui::Widget*>(device_list)})
            x->visible = preview_visible;
        if (preview_visible) {
            float px = w - PreviewW + 18, pw = PreviewW - 36;
            float ph = preview_view->layout(c, px, HeaderH + 44, pw);
            ph = std::min(ph, 300.f);
            preview_view->rc.bottom = preview_view->rc.top + ph;
            float dy = preview_view->rc.bottom + 62;
            const float dh = device_list->layout(c, px, dy, pw);
            device_card_ = {px, dy, px + pw, dy + dh};
        }
        // Inhaltsbereich als Kindfenster
        RECT r{frame->px(NavW), frame->px(HeaderH), frame->px(preview_visible ? w - PreviewW : w), frame->px(h - FooterH)};
        SetWindowPos(content->hwnd(), nullptr, r.left, r.top, std::max(1L, r.right - r.left), std::max(1L, r.bottom - r.top),
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return h;
    }
    D2D1_RECT_F device_card_{};

    void paint_frame(ui::Ctx& c, D2D1_SIZE_F sz) {
        const auto& th = c.th;
        const float w = sz.width, h = sz.height;
        if (material()) {
            const ui::Text title = current >= 0 ? slots[size_t(current)].info.title : ui::T(L"Übersicht", L"Overview");
            const ui::Text subtitle = current >= 0 ? slots[size_t(current)].info.subtitle : ui::Text();
            ui::shell::paint_material(c, material_layout, title, subtitle);
            if (preview_visible) {
                const auto& p = material_layout.inspector;
                c.text(kit->english ? L"DEVICE INSPECTOR" : L"GERÄTE-INSPECTOR", {p.left + 16,p.top + 8,p.right - 16,p.top + 30}, ui::Font::Section, th.muted);
                c.text(kit->english ? L"DEVICE" : L"GERÄT", {p.left + 16,device_card_.top - 26,p.right - 16,device_card_.top - 6}, ui::Font::Section, th.muted);
            }
            return;
        }
        const float NavW = nav_width(), HeaderH = header_height(), FooterH = footer_height();
        c.fill({0, HeaderH, NavW, h - FooterH}, th.side);
        if (preview_visible) {
            c.fill({w - PreviewW, HeaderH, w, h - FooterH}, th.side);
            c.line({w - PreviewW, HeaderH}, {w - PreviewW, h - FooterH}, th.dark ? th.line : ui::from(brand::GLUT, .35f));
        }
        if (NavW > 0) {   // Legende der Punkte in der Navigation
            const float ly = h - FooterH - 20;
            c.circle({26, ly}, 4, th.success);
            c.text(kit->english ? L"set up & on" : L"eingerichtet & an", {36, ly - 9, 118, ly + 9}, ui::Font::Small, th.side_muted);
            ui::Rgba grey = th.side_muted;
            grey.a *= .45f;
            c.circle({126, ly}, 4, grey);
            c.text(kit->english ? L"not set up / off" : L"nicht eingerichtet / aus", {136, ly - 9, NavW - 6, ly + 9}, ui::Font::Small, th.side_muted);
        }
        c.fill({0, 0, w, HeaderH}, th.header);
        if (cupertino()) {
            const ui::Rect toolbar{NavW + 6, 6, w - 6, HeaderH - 4};
            auto shadow = th.text; shadow.a = .035f;
            c.fill({toolbar.left,toolbar.top + 3,toolbar.right,toolbar.bottom + 3},shadow,16);
            auto surface = th.card; surface.a = .88f;
            c.fill(toolbar,surface,16); c.frame(toolbar,th.line,16,.75f);
        }
        if (logo_->visible) c.circle({35, fluent() ? 26.f : HeaderH / 2}, 18, ui::from(brand::NAVY));
        c.fill({0, h - FooterH, w, h}, th.bg);
        c.line({0, h - FooterH}, {w, h - FooterH}, th.line);
        // Kopfzeile
        if (cupertino() || fluent()) {
            if (NavW > 0) c.text(L"ScreenBuddy", {62, 0, NavW - 12, 52}, ui::Font::Strong, th.header_text);
            const std::wstring page = current >= 0 ? slots[size_t(current)].info.title.get(kit->english) : L"";
            if (cupertino()) c.text(page, {NavW + 60, 0, inspector_button->rc.left - 12, HeaderH}, ui::Font::Strong, th.text, DWRITE_TEXT_ALIGNMENT_CENTER);
            else c.text((kit->english ? L"Settings  /  " : L"Einstellungen  /  ") + page,
                {NavW + 26, 64, w - (preview_visible ? PreviewW : 0) - 26, 96}, ui::Font::Body, th.muted);
        } else {
        c.text(L"ScreenBuddy", {62, 0, 220, HeaderH}, ui::Font::Brand, th.header_text);
        float bw = c.text_width(L"ScreenBuddy", ui::Font::Brand);
        c.text(L"v" + widen(VERSION), {62 + bw + 8, 2, 62 + bw + 160, HeaderH}, ui::Font::Body,
               kit->design.style == fl::theme::Style::FuchsLabs ? ui::from(brand::NAVY, .7f) : th.muted);
        std::wstring pill = output_name(kit->english) + L" " + (status_.display_ok ? (kit->english ? L"connected" : L"verbunden")
                                                                                 : (kit->english ? L"not connected" : L"nicht verbunden"));
        if (!status_.valid) pill = kit->english ? L"Status loading …" : L"Status wird geladen …";
        float pw = c.text_width(pill, ui::Font::Label) + 34;
        float right = lang->rc.left - 10;
        D2D1_RECT_F p{right - pw, (HeaderH - 28) / 2, right, (HeaderH + 28) / 2};
        c.fill(p, status_.display_ok ? ui::from(brand::NAVY) : ui::from(brand::NAVY, .13f), 14);
        c.circle({p.left + 14, (p.top + p.bottom) / 2}, 4, status_.display_ok ? ui::rgb(46, 204, 113) : ui::rgb(178, 34, 34));
        c.text(pill, {p.left + 24, p.top, p.right - 8, p.bottom}, ui::Font::Label,
               status_.display_ok ? ui::from(brand::CREME) : kit->design.style == fl::theme::Style::FuchsLabs ? ui::from(brand::NAVY) : th.text);
        }
        // Gruppenüberschriften
        static const wchar_t* group_de[] = {L"ANSICHT", L"SEITEN", L"APP"};
        static const wchar_t* group_en[] = {L"VIEW", L"PAGES", L"APP"};
        int group = -1;
        for (auto& s : slots) {
            if (!s.nav->visible || (fluent() && !nav_search->text().empty())) continue;
            if (int(s.info.group) == group) continue;
            group = int(s.info.group);
            float y = s.nav->rc.top - 22;
            c.text(kit->english ? group_en[group] : group_de[group], {20, y, NavW - 10, y + 18}, ui::Font::Section, th.side_muted);
        }
        if (fluent() && !nav_search->text().empty() && std::none_of(slots.begin(), slots.end(), [this](const Slot& s) { return nav_matches(s); }))
            c.text(kit->english ? L"No matching pages" : L"Keine passenden Seiten", {20, HeaderH + 20, NavW - 20, HeaderH + 70}, ui::Font::Small, th.muted, DWRITE_TEXT_ALIGNMENT_LEADING, true);
        // Vorschau
        if (preview_visible) {
            float x = w - PreviewW + 18;
            c.text(kit->english ? L"LIVE PREVIEW" : L"LIVE-VORSCHAU", {x, HeaderH + 16, w - 18, HeaderH + 36}, ui::Font::Section, th.side_muted);
            const ojson& nav = status_.navigation;
            int total = std::min(12, nav.is_object() ? nav.value("total", 0) : 0), index = nav.is_object() ? nav.value("index", 0) : 0;
            float dots_y = preview_view->rc.bottom + 14, dots_w = total * 24.f - 6;
            float dx = (w - PreviewW / 2) - dots_w / 2;
            for (int i = 0; i < total; ++i)
                c.fill({dx + i * 24.f, dots_y, dx + i * 24.f + 18, dots_y + 5}, i == index ? th.accent : ui::from(brand::NAVY, th.dark ? .6f : .27f), 2.5f);
            c.text(kit->english ? L"DEVICE" : L"GERÄT", {x, device_card_.top - 26, w - 18, device_card_.top - 6}, ui::Font::Section, th.side_muted);
        }
        // Fußleiste: ungespeicherte Änderungen
        std::wstring dirty_text;
        if (saving) dirty_text = kit->english ? L"Saving …" : L"Wird gespeichert …";
        else if (dirty_count == 0) dirty_text = kit->english ? L"No unsaved changes" : L"Keine ungespeicherten Änderungen";
        else if (kit->english) dirty_text = L"● " + std::to_wstring(dirty_count) + (dirty_count == 1 ? L" unsaved change" : L" unsaved changes");
        else dirty_text = L"● " + std::to_wstring(dirty_count) + (dirty_count == 1 ? L" ungespeicherte Änderung" : L" ungespeicherte Änderungen");
        c.text(dirty_text, {18, h - FooterH, 320, h}, ui::Font::Strong, dirty_count ? th.accent_text : th.muted);
    }
    void paint_toast(ui::Ctx& c, D2D1_SIZE_F sz) {
        if (toast_text.empty()) return;
        const auto& th = c.th;
        const std::wstring& t = c.s(toast_text);
        const float FooterH = footer_height();
        // Material: unten im Inhalt (Snackbar), links neben der Speicheraktion; nie über Kopfbereich und Steuerungen
        const auto& ml = material_layout;
        float left = material() ? ml.body.left : 330.f;
        float right = material() ? (dirty_count > 0 ? ml.discard.left - 16.f : ml.body.right) : discard_button->rc.left - 16;
        if (right - left < 120) return;
        float tw = std::min(right - left, c.text_width(t, ui::Font::Body) + 44);
        float th_ = std::min(48.f, c.text_height(t, tw - 44, ui::Font::Body) + 16);
        const float cy = material() ? sz.height - 56.f : sz.height - FooterH / 2;
        D2D1_RECT_F r{left, cy - th_ / 2, left + tw, cy + th_ / 2};
        ui::Rgba accent = toast_kind == ui::ToastKind::Error ? th.danger : toast_kind == ui::ToastKind::Success ? th.success : th.accent;
        c.fill(r, th.dark ? th.card : th.input, 10);
        c.frame(r, accent, 10, 1.5f);
        c.circle({r.left + 16, (r.top + r.bottom) / 2}, 5, accent);
        c.text(t, {r.left + 30, r.top + 4, r.right - 10, r.bottom - 4}, ui::Font::Body, th.text, DWRITE_TEXT_ALIGNMENT_LEADING, true, true);
    }

    // ---------------- Fenster und Nachrichten
    bool message(UINT msg, WPARAM wp, LPARAM, LRESULT&) {
        switch (msg) {
        case ShowMsg:
            ShowWindow(frame->hwnd(), IsIconic(frame->hwnd()) ? SW_RESTORE : SW_SHOW);
            SetForegroundWindow(frame->hwnd());
            shown = true;
            if (current >= 0) frame->focus(material() ? material_navigation() : sidebar_collapsed ? static_cast<ui::Widget*>(sidebar_button) : slots[size_t(current)].nav, false);
            poll();
            return true;
        case StopMsg: DestroyWindow(frame->hwnd()); return true;
        case WM_CLOSE:
            if (confirm_close()) { ShowWindow(frame->hwnd(), SW_HIDE); shown = false; }
            return true;
        case WM_SIZE:
            shown = IsWindowVisible(frame->hwnd()) && wp != SIZE_MINIMIZED;
            return false;
        case WM_TIMER:
            if (wp == PollTimer) { poll(); return true; }
            if (wp == ToastTimer) { KillTimer(frame->hwnd(), ToastTimer); toast_text = {}; frame->invalidate(); return true; }
            return false;   // übrige Timer (Tooltip-Verzögerung) gehören der Surface
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
            if (theme && theme->index == 2) apply_theme();   // Windows-Farben live nachziehen
            return false;
        case ResultsMsg: consume(); return true;
        case WM_DESTROY:
            KillTimer(frame->hwnd(), PollTimer); shown = false; live_window = nullptr; PostQuitMessage(0);
            return true;
        }
        return false;
    }

    void run_ui() {
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool signaled = false;
        try {
            kit = std::make_unique<ui::Kit>();
            frame = std::make_unique<ui::Surface>(*kit, nullptr, L"ScreenBuddy – Einstellungen", false);
            content = std::make_unique<ui::Surface>(*kit, frame->hwnd(), L"Inhalt", true);
            frame->on_message = [this](UINT m, WPARAM wp, LPARAM lp, LRESULT& r) {
                if (m == WM_GETMINMAXINFO) {
                    reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {frame->px(980), frame->px(680)};
                    return true;
                }
                return message(m, wp, lp, r);
            };
            build_frame();
            if (kit->english) SetWindowTextW(frame->hwnd(), L"ScreenBuddy – Settings");
            apply_caption();
            BOOL dark = kit->dark;
            DwmSetWindowAttribute(frame->hwnd(), 20, &dark, sizeof(dark));
            const UINT dpi = GetDpiForWindow(frame->hwnd());
            RECT desired{0, 0, MulDiv(1180, int(dpi), 96), MulDiv(760, int(dpi), 96)};
            AdjustWindowRectExForDpi(&desired, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
            MONITORINFO monitor{sizeof monitor};
            if (GetMonitorInfoW(MonitorFromWindow(frame->hwnd(), MONITOR_DEFAULTTONEAREST), &monitor)) {
                const int aw = monitor.rcWork.right - monitor.rcWork.left, ah = monitor.rcWork.bottom - monitor.rcWork.top;
                const int ow = std::min(int(desired.right - desired.left), aw), oh = std::min(int(desired.bottom - desired.top), ah);
                SetWindowPos(frame->hwnd(), nullptr, monitor.rcWork.left + std::max(0, (aw - ow) / 2),
                             monitor.rcWork.top + std::max(0, (ah - oh) / 2), ow, oh, SWP_NOACTIVATE | SWP_NOZORDER);
            }
            frame->set_dpi(GetDpiForWindow(frame->hwnd()));
            content->set_dpi(GetDpiForWindow(content->hwnd()));
            open(0, false);
            SetTimer(frame->hwnd(), PollTimer, 1000, nullptr);
            live_window = frame->hwnd();
            signal_ready(); signaled = true;
            MSG msg{};
            while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                if (msg.message == WM_KEYDOWN && msg.wParam == 'S' && GetKeyState(VK_CONTROL) < 0) { save(); continue; }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                ui::flush_retired();
            }
        } catch (...) {
            if (!signaled) signal_ready(std::current_exception());
        }
        live_window = nullptr; shown = false;
        // Seiten (mit EDIT-Fenstern) vor den Fenstern abbauen, danach Kit und COM
        for (auto& s : slots) s.root.reset();
        ui::flush_retired();
        content.reset();
        frame.reset();
        frame_widgets.clear();
        slots.clear();
        kit.reset();
        if (SUCCEEDED(com)) CoUninitialize();
    }
};

NativeSettings::NativeSettings(App& app, NativePluginActions plugins) : d_(std::make_unique<Impl>(app, std::move(plugins))) { d_->start(); }
NativeSettings::~NativeSettings() = default;
void NativeSettings::show() { if (HWND hwnd = d_->live_window.load()) PostMessageW(hwnd, ShowMsg, 0, 0); }
bool NativeSettings::visible() const { return d_->shown; }
}  // namespace fl
