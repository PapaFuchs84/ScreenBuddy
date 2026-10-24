// © 2026 FuchsLabs
#include "native_settings.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cwchar>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "app.h"
#include "config.h"
#include "util.h"

namespace fl {
namespace {
constexpr wchar_t WindowClass[] = L"ScreenBuddyNativeSettings";
constexpr UINT Show = WM_APP + 171, Stop = WM_APP + 172, Results = WM_APP + 173;
enum Id : int { Tabs = 100, OutputControl, Port, Monitor, OrientationControl, Theme, Brightness, Seconds,
    Touch, ResetOnStart, OffOnExit, Language, Updates, Save, Preview, PreviewTitle, Status,
    PluginList, PluginImport, PluginToggle, PluginRemove, PluginRefresh, PluginNote, GeneralNote, Advanced };
constexpr const char* Outputs[] = {"serial", "monitor", "network"};
constexpr const char* Orientations[] = {"landscape", "reverse_landscape", "portrait", "reverse_portrait"};
constexpr const char* Themes[] = {"hell", "dunkel", "eigen"};
}

struct NativeSettings::Impl {
    App& app;
    NativePluginActions plugins;
    ojson initial;
    bool english = false;
    std::thread ui_thread, job_thread;
    std::atomic<HWND> window{nullptr};
    std::atomic<bool> stopping{false}, shown{false}, snapshot_pending{false};
    std::mutex ready_mutex, jobs_mutex, results_mutex;
    std::condition_variable ready_cv, jobs_cv;
    bool ready = false;
    std::exception_ptr startup_error;
    std::deque<std::function<void()>> jobs;
    struct Result {
        enum Kind { Snapshot, Saved, PluginChanged, Failure, PassiveFailure } kind = Snapshot;
        std::optional<Image> frame;
        std::wstring status;
        std::vector<NativePluginEntry> entries;
    };
    std::deque<Result> results;

    // UI-thread-owned state below.
    HWND hwnd = nullptr;
    UINT dpi = 96;
    HFONT font = nullptr;
    struct Control { HWND hwnd; int page, id; };
    std::vector<Control> controls;
    std::vector<NativePluginEntry> plugin_entries;
    std::optional<Image> frame;
    int page = 0;
    bool action_pending = false, saving = false;
    bool previewing_theme = false;

    Impl(App& a, NativePluginActions p) : app(a), plugins(std::move(p)), initial(a.cfg()),
        english(initial.at("ui").value("language", "de") == "en") {}
    ~Impl() { stop(); }
    const wchar_t* tr(const wchar_t* de, const wchar_t* en) const { return english ? en : de; }
    int px(int value) const { return MulDiv(value, dpi, 96); }
    HWND child(int id) const { return GetDlgItem(hwnd, id); }

    void start() {
        job_thread = std::thread([this] { work(); });
        try { ui_thread = std::thread([this] { run(); }); }
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
        if (HWND w = window.load()) PostMessageW(w, Stop, 0, 0);
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
    void passive(std::function<void()> action) {
        enqueue([this, action = std::move(action)] {
            try { action(); }
            catch (const std::exception& error) {
                Result result; result.kind = Result::PassiveFailure; result.status = widen(error.what()); deliver(std::move(result));
            } catch (...) {
                Result result; result.kind = Result::PassiveFailure;
                result.status = tr(L"Vorschau/Status konnte nicht aktualisiert werden.", L"Could not refresh preview/status."); deliver(std::move(result));
            }
        });
    }
    void deliver(Result result) {
        { std::lock_guard lock(results_mutex); if (stopping) return; results.push_back(std::move(result)); }
        if (HWND w = window.load()) PostMessageW(w, Results, 0, 0);
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
            try { action(); }
            catch (const std::exception& error) {
                Result result; result.kind = Result::Failure; result.status = widen(error.what()); deliver(std::move(result));
            } catch (...) {
                Result result; result.kind = Result::Failure;
                result.status = tr(L"Die Aktion ist fehlgeschlagen.", L"The action failed."); deliver(std::move(result));
            }
        }
    }

    HWND add(const wchar_t* type, const wchar_t* text, DWORD style, int id, int group) {
        HWND c = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                                  hwnd, reinterpret_cast<HMENU>(INT_PTR(id)), GetModuleHandleW(nullptr), nullptr);
        if (!c) throw std::runtime_error("Native settings: CreateWindowExW");
        controls.push_back({c, group, id});
        return c;
    }
    void label(int target, const wchar_t* de, const wchar_t* en, int group) {
        add(L"STATIC", tr(de, en), SS_LEFT, target + 1000, group);
    }
    void combo(int id, int group, std::initializer_list<const wchar_t*> items) {
        HWND c = add(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, id, group);
        for (auto item : items) SendMessageW(c, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    }
    void edit(int id, int group, const std::string& value, bool number = false) {
        add(L"EDIT", widen(value).c_str(), WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | (number ? ES_NUMBER : 0), id, group);
        SendMessageW(child(id), EM_SETLIMITTEXT, number ? 4 : 1024, 0);
    }
    void check(int id, int group, const wchar_t* de, const wchar_t* en, bool value) {
        add(L"BUTTON", tr(de, en), BS_AUTOCHECKBOX | WS_TABSTOP, id, group);
        SendMessageW(child(id), BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    template<size_t N> void select(int id, const char* const (&values)[N], const std::string& value) {
        size_t index = 0;
        for (size_t i = 0; i < N; ++i) if (value == values[i]) index = i;
        SendMessageW(child(id), CB_SETCURSEL, index, 0);
    }
    void initialize() {
        dpi = GetDpiForWindow(hwnd);
        add(WC_TABCONTROLW, L"", WS_TABSTOP, Tabs, -1);
        for (auto title : {tr(L"Anzeige", L"Display"), tr(L"Anwendung", L"Application"), L"Plugins"}) {
            TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = const_cast<wchar_t*>(title);
            TabCtrl_InsertItem(child(Tabs), TabCtrl_GetItemCount(child(Tabs)), &item);
        }
        const auto& dp = initial.at("display");
        label(OutputControl, L"&Ausgabe", L"&OutputControl", 0);
        combo(OutputControl, 0, {tr(L"USB-Display (seriell)", L"USB display (serial)"), tr(L"Windows-Bildschirm", L"Windows monitor"), tr(L"Android / Netzwerk", L"Android / network")});
        select(OutputControl, Outputs, dp.value("output", "serial"));
        label(Port, L"&Port (AUTO oder COM…)", L"&Port (AUTO or COM…)", 0);
        edit(Port, 0, dp.value("port", "AUTO"));
        label(Monitor, L"&Monitor-ID (leer = automatisch)", L"&Monitor ID (empty = automatic)", 0);
        edit(Monitor, 0, dp.value("monitor", ""));
        label(OrientationControl, L"A&usrichtung", L"O&rientation", 0);
        combo(OrientationControl, 0, {tr(L"Querformat", L"Landscape"), tr(L"Querformat gedreht", L"Reverse landscape"), tr(L"Hochformat", L"Portrait"), tr(L"Hochformat gedreht", L"Reverse portrait")});
        select(OrientationControl, Orientations, dp.value("orientation", "landscape"));
        label(Theme, L"&Design", L"&Theme", 0);
        combo(Theme, 0, {tr(L"Hell", L"Light"), tr(L"Dunkel", L"Dark"), tr(L"Eigene Farben (beibehalten)", L"Custom colors (preserved)")});
        select(Theme, Themes, dp.value("theme", "hell"));
        label(Brightness, L"&Helligkeit (0–100)", L"&Brightness (0–100)", 0);
        edit(Brightness, 0, std::to_string(dp.value("brightness", 50)), true);
        label(Seconds, L"&Seitenwechsel (3–600 Sekunden)", L"Page &interval (3–600 seconds)", 0);
        edit(Seconds, 0, std::to_string(dp.value("page_seconds", 15)), true);
        check(Touch, 0, L"&Touch aktivieren", L"Enable t&ouch", dp.value("touch", true));
        check(ResetOnStart, 0, L"Display beim Start &zurücksetzen", L"&Reset display on startup", dp.value("reset_on_start", true));
        check(OffOnExit, 0, L"Display beim Beenden ausschalten", L"Turn display off on exit", dp.value("screen_off_on_exit", true));
        label(Language, L"&Sprache (nach Neustart)", L"&Language (after restart)", 1);
        combo(Language, 1, {L"Deutsch", L"English"});
        SendMessageW(child(Language), CB_SETCURSEL, english ? 1 : 0, 0);
        check(Updates, 1, L"Automatisch nach &Updates suchen", L"Automatically check for &updates", initial.at("ui").value("update_check", true));
        add(L"STATIC", tr(L"Speichern übernimmt Anzeige- und Anwendungseinstellungen und startet ScreenBuddy neu. Bestehende Quellen, Zugangsdaten und eigene Farben bleiben erhalten.\r\n\r\nWeitere Quelleneinstellungen und die Seitenreihenfolge sind in dieser Oberfläche noch nicht verfügbar.",
                           L"Saving applies display/application settings and restarts ScreenBuddy. Existing sources, credentials and custom colors are preserved.\r\n\r\nSource settings and page order are not available in this interface yet."), SS_LEFT, GeneralNote, 1);
        add(L"STATIC", tr(L"Live-Vorschau", L"Live preview"), SS_LEFT, PreviewTitle, -2);
        add(L"STATIC", tr(L"Aktuelles Displaybild", L"Current display frame"), SS_OWNERDRAW, Preview, -2);
        add(L"STATIC", tr(L"Status wird geladen …", L"Loading status …"), SS_LEFT, Status, -2);
        add(L"BUTTON", tr(L"&Speichern und neu starten", L"&Save and restart"), BS_DEFPUSHBUTTON | WS_TABSTOP, Save, -1);
        add(L"BUTTON", tr(L"Erweiterte Einstellungen …", L"Advanced settings …"), BS_PUSHBUTTON | WS_TABSTOP, Advanced, 1);
        add(L"LISTBOX", L"", WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, PluginList, 2);
        add(L"BUTTON", tr(L"Manifest &importieren …", L"&Import manifest …"), BS_PUSHBUTTON | WS_TABSTOP, PluginImport, 2);
        add(L"BUTTON", tr(L"&Aktivieren / deaktivieren", L"&Enable / disable"), BS_PUSHBUTTON | WS_TABSTOP, PluginToggle, 2);
        add(L"BUTTON", tr(L"&Entfernen", L"&Remove"), BS_PUSHBUTTON | WS_TABSTOP, PluginRemove, 2);
        add(L"BUTTON", tr(L"A&ktualisieren", L"Re&fresh"), BS_PUSHBUTTON | WS_TABSTOP, PluginRefresh, 2);
        add(L"STATIC", plugins.list ? tr(L"Plugins werden lokal verwaltet. Ein Import führt keinen Plugin-Code aus.", L"Plugins are managed locally. Import does not execute plugin code.")
                                          : tr(L"Die Plugin-Verwaltung ist in diesem Build noch nicht verbunden.", L"Plugin management is not connected in this build yet."), SS_LEFT, PluginNote, 2);
        fonts();
        update_controls();
        SetTimer(hwnd, 1, 1000, nullptr);
    }

    void fonts() {
        HFONT next = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        if (!next) throw std::runtime_error("Native settings: CreateFontW");
        for (auto& c : controls) SendMessageW(c.hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(next), TRUE);
        if (font) DeleteObject(font);
        font = next;
    }
    void move(int id, int x, int y, int w, int h) { MoveWindow(child(id), px(x), px(y), px(w), px(h), TRUE); }
    void layout() {
        RECT r{}; GetClientRect(hwnd, &r);
        int w = MulDiv(r.right, 96, dpi), h = MulDiv(r.bottom, 96, dpi);
        move(Tabs, 16, 12, w - 32, 34);
        const int fields[] = {OutputControl, Port, Monitor, OrientationControl, Theme, Brightness, Seconds};
        for (int i = 0; i < int(std::size(fields)); ++i) {
            move(fields[i] + 1000, 24, 62 + i * 58, 320, 20);
            bool combo_box = fields[i] == OutputControl || fields[i] == OrientationControl || fields[i] == Theme;
            move(fields[i], 24, 84 + i * 58, 320, combo_box ? 180 : 28);
        }
        move(Touch, 24, 478, 320, 26); move(ResetOnStart, 24, 510, 320, 26); move(OffOnExit, 24, 542, 320, 26);
        move(Language + 1000, 24, 62, 320, 22); move(Language, 24, 88, 320, 160);
        move(Updates, 24, 136, 320, 40); move(GeneralNote, 24, 200, 320, h - 280);
        move(PreviewTitle, 376, 62, w - 400, 24);
        int available_w = std::max(1, w - 400), available_h = std::max(1, h - 320);
        int source_w = frame ? frame->w : 480, source_h = frame ? frame->h : 320;
        double scale = std::min(double(available_w) / std::max(1, source_w), double(available_h) / std::max(1, source_h));
        move(Preview, 376, 96, std::max(1, int(source_w * scale)), std::max(1, int(source_h * scale)));
        move(Status, 376, h - 205, w - 400, 140);
        move(Save, 24, h - 54, 320, 34);
        move(Advanced, 24, h - 108, 320, 34);
        move(PluginList, 24, 66, w - 48, h - 245);
        move(PluginImport, 24, h - 162, 230, 32); move(PluginToggle, 270, h - 162, 230, 32);
        move(PluginRemove, 516, h - 162, 160, 32); move(PluginRefresh, 692, h - 162, 160, 32);
        move(PluginNote, 24, h - 112, w - 48, 50);
    }
    void update_controls() {
        for (auto& c : controls) {
            bool visible = c.page == -1 || c.page == page || (c.page == -2 && page != 2);
            if (c.id == Save && page == 2) visible = false;
            ShowWindow(c.hwnd, visible ? SW_SHOW : SW_HIDE);
        }
        EnableWindow(child(Save), !action_pending && !saving);
        EnableWindow(child(PluginImport), !action_pending && bool(plugins.import_manifest));
        EnableWindow(child(PluginRefresh), !action_pending && bool(plugins.list));
        bool selected = SendMessageW(child(PluginList), LB_GETCURSEL, 0, 0) != LB_ERR;
        EnableWindow(child(PluginToggle), !action_pending && selected && bool(plugins.set_enabled));
        EnableWindow(child(PluginRemove), !action_pending && selected && bool(plugins.remove));
    }
    std::string value(int id) const {
        int count = GetWindowTextLengthW(child(id));
        std::wstring text(size_t(count) + 1, L'\0');
        GetWindowTextW(child(id), text.data(), count + 1); text.resize(count);
        return narrow(text);
    }
    bool checked(int id) const { return SendMessageW(child(id), BM_GETCHECK, 0, 0) == BST_CHECKED; }
    template<size_t N> const char* choice(int id, const char* const (&values)[N]) const {
        LRESULT index = SendMessageW(child(id), CB_GETCURSEL, 0, 0);
        if (index < 0 || size_t(index) >= N) throw std::invalid_argument("Invalid selection");
        return values[index];
    }
    int number(int id, int minimum, int maximum) const {
        std::string text = value(id); size_t end = 0;
        int n;
        try { n = std::stoi(text, &end); }
        catch (...) { throw std::invalid_argument(english ? "Enter a valid number." : "Bitte eine gültige Zahl eingeben."); }
        if (end != text.size() || n < minimum || n > maximum)
            throw std::invalid_argument(english ? "Number is outside the permitted range." : "Die Zahl liegt außerhalb des erlaubten Bereichs.");
        return n;
    }
    ojson patch() const {
        ojson incoming = {{"display", {{"output", choice(OutputControl, Outputs)}, {"port", trim(value(Port))},
            {"monitor", trim(value(Monitor))}, {"orientation", choice(OrientationControl, Orientations)}, {"theme", choice(Theme, Themes)},
            {"brightness", number(Brightness, 0, 100)}, {"page_seconds", number(Seconds, 3, 600)},
            {"touch", checked(Touch)}, {"reset_on_start", checked(ResetOnStart)}, {"screen_off_on_exit", checked(OffOnExit)}}},
            {"ui", {{"language", SendMessageW(child(Language), CB_GETCURSEL, 0, 0) == 1 ? "en" : "de"},
                     {"update_check", checked(Updates)}, {"setup_done", true}}}};
        config::merge(initial, incoming); // Validate before placing a save job on the queue.
        return incoming;
    }

    void poll() {
        if (stopping || !shown || snapshot_pending.exchange(true)) return;
        const bool use_live_frame = !previewing_theme;
        passive([this, use_live_frame] {
            struct Reset { std::atomic<bool>& value; ~Reset() { value = false; } } reset{snapshot_pending};
            Result result;
            if (use_live_frame) result.frame = app.last_frame();
            auto [width, height] = app.size();
            result.status = tr(L"Display: ", L"Display: ");
            result.status += app.display_ok() ? tr(L"verbunden", L"connected") : tr(L"nicht verbunden", L"not connected");
            result.status += L" · " + std::to_wstring(width) + L" × " + std::to_wstring(height);
            std::string error = app.display_error();
            if (!error.empty()) result.status += L"\r\n" + widen(error);
            ojson connections = app.connections();
            for (auto name : {"discord", "steam", "xbox", "homeassistant"})
                if (connections.contains(name)) result.status += L"\r\n" + widen(name) + L": " + widen(connections[name].value("state", "off"));
            deliver(std::move(result));
        });
    }
    void refresh_plugins() {
        if (!plugins.list || action_pending) return;
        action_pending = true; update_controls();
        enqueue([this] { Result result; result.kind = Result::PluginChanged; result.entries = plugins.list(); deliver(std::move(result)); });
    }
    void plugin_action(int id) {
        if (action_pending) return;
        std::filesystem::path path;
        std::optional<NativePluginEntry> selected;
        if (id == PluginImport) {
            if (!plugins.import_manifest) return;
            wchar_t filename[32768]{};
            OPENFILENAMEW picker{sizeof(picker)};
            picker.hwndOwner = hwnd; picker.lpstrFile = filename; picker.nMaxFile = DWORD(std::size(filename));
            picker.lpstrFilter = L"JSON manifest (*.json)\0*.json\0\0";
            picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
            if (!GetOpenFileNameW(&picker)) {
                if (CommDlgExtendedError()) throw std::runtime_error(english ? "Cannot open the file picker." : "Dateiauswahl konnte nicht geöffnet werden.");
                return;
            }
            path = filename;
            std::error_code ec;
            auto size = std::filesystem::file_size(path, ec);
            if (ec || size > 256 * 1024) throw std::runtime_error(english ? "Manifest must be a readable file of at most 256 KiB." : "Das Manifest muss eine lesbare Datei mit höchstens 256 KiB sein.");
        } else {
            LRESULT index = SendMessageW(child(PluginList), LB_GETCURSEL, 0, 0);
            if (index < 0 || size_t(index) >= plugin_entries.size()) return;
            selected = plugin_entries[size_t(index)];
            if (id == PluginRemove && MessageBoxW(hwnd, tr(L"Dieses Plugin aus der lokalen Verwaltung entfernen?", L"Remove this plugin from local management?"),
                    L"ScreenBuddy", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        }
        action_pending = true; update_controls();
        enqueue([this, id, path = std::move(path), selected] {
            if (id == PluginImport && plugins.import_manifest) plugins.import_manifest(path);
            else if (id == PluginToggle && plugins.set_enabled) plugins.set_enabled(selected->id, !selected->enabled);
            else if (id == PluginRemove && plugins.remove) plugins.remove(selected->id);
            Result result; result.kind = Result::PluginChanged;
            if (plugins.list) result.entries = plugins.list();
            deliver(std::move(result));
        });
    }
    void command(WPARAM wp) {
        int id = LOWORD(wp), event = HIWORD(wp);
        if (id == PluginList && event == LBN_SELCHANGE) { update_controls(); return; }
        if (id == Theme && event == CBN_SELCHANGE) {
            std::string theme = choice(Theme, Themes);
            previewing_theme = true;
            passive([this, theme] { Result result; result.frame = app.preview_theme({{"theme", theme}}); deliver(std::move(result)); });
            return;
        }
        if (event != BN_CLICKED) return;
        if (id == PluginRefresh) { refresh_plugins(); return; }
        if (id == Advanced && plugins.open_advanced) { plugins.open_advanced(); return; }
        if (id == PluginImport || id == PluginToggle || id == PluginRemove) { plugin_action(id); return; }
        if (id != Save || saving || action_pending) return;
        ojson incoming = patch();
        saving = true; update_controls();
        enqueue([this, incoming = std::move(incoming)] {
            auto merged = config::merge(app.cfg(), incoming); // Preserve all existing secrets and unrelated fields.
            config::save(app.cfg_path(), merged); // Existing DPAPI + atomic replace implementation.
            Result result; result.kind = Result::Saved; deliver(std::move(result));
            if (!stopping) app.request_restart(0.4);
        });
    }
    void consume() {
        std::deque<Result> pending;
        { std::lock_guard lock(results_mutex); pending.swap(results); }
        for (auto& result : pending) {
            if (result.kind == Result::Snapshot) {
                if (result.frame) frame = std::move(result.frame);
                if (!result.status.empty()) SetWindowTextW(child(Status), result.status.c_str());
                layout(); InvalidateRect(child(Preview), nullptr, TRUE);
            } else if (result.kind == Result::Saved) {
                SetWindowTextW(child(Status), tr(L"Gespeichert. ScreenBuddy startet neu …", L"Saved. ScreenBuddy is restarting …"));
            } else if (result.kind == Result::PluginChanged) {
                action_pending = false; plugin_entries = std::move(result.entries);
                SendMessageW(child(PluginList), LB_RESETCONTENT, 0, 0);
                for (const auto& entry : plugin_entries) {
                    std::wstring title = entry.enabled ? L"[✓] " : L"[ ] ";
                    title += widen(entry.name + "  " + entry.version + "  (" + entry.id + ")");
                    SendMessageW(child(PluginList), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
                }
                update_controls();
            } else if (result.kind == Result::PassiveFailure) {
                SetWindowTextW(child(Status), result.status.c_str());
            } else {
                saving = false; action_pending = false; update_controls();
                MessageBoxW(hwnd, result.status.c_str(), tr(L"ScreenBuddy – Fehler", L"ScreenBuddy – Error"), MB_OK | MB_ICONERROR);
            }
        }
    }
    void paint(const DRAWITEMSTRUCT& item) {
        FillRect(item.hDC, &item.rcItem, GetSysColorBrush(COLOR_WINDOW));
        if (!frame || frame->empty()) {
            SetBkMode(item.hDC, TRANSPARENT); SetTextColor(item.hDC, GetSysColor(COLOR_WINDOWTEXT));
            RECT r = item.rcItem;
            DrawTextW(item.hDC, tr(L"Noch kein Displaybild verfügbar.", L"No display frame available yet."), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return;
        }
        BITMAPINFO bitmap{}; bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = frame->w; bitmap.bmiHeader.biHeight = -frame->h;
        bitmap.bmiHeader.biPlanes = 1; bitmap.bmiHeader.biBitCount = 32; bitmap.bmiHeader.biCompression = BI_RGB;
        SetStretchBltMode(item.hDC, HALFTONE); SetBrushOrgEx(item.hDC, 0, 0, nullptr);
        StretchDIBits(item.hDC, 0, 0, item.rcItem.right, item.rcItem.bottom, 0, 0,
                      frame->w, frame->h, frame->px.data(), &bitmap, DIB_RGB_COLORS, SRCCOPY);
    }

    static LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            self = static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            self->hwnd = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
        try {
            switch (msg) {
            case WM_CREATE: self->initialize(); return 0;
            case Show:
                ShowWindow(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOW); SetForegroundWindow(hwnd);
                self->shown = true; self->poll(); return 0;
            case Stop: DestroyWindow(hwnd); return 0;
            case WM_CLOSE: ShowWindow(hwnd, SW_HIDE); self->shown = false; return 0;
            case WM_SIZE:
                self->shown = IsWindowVisible(hwnd) && wp != SIZE_MINIMIZED;
                self->layout(); return 0;
            case WM_COMMAND: self->command(wp); return 0;
            case WM_NOTIFY:
                if (reinterpret_cast<NMHDR*>(lp)->idFrom == Tabs && reinterpret_cast<NMHDR*>(lp)->code == TCN_SELCHANGE) {
                    self->page = TabCtrl_GetCurSel(self->child(Tabs)); self->update_controls();
                    if (self->page == 2) self->refresh_plugins();
                }
                return 0;
            case WM_TIMER: self->poll(); return 0;
            case Results: self->consume(); return 0;
            case WM_DRAWITEM: self->paint(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
            case WM_GETMINMAXINFO:
                reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {self->px(960), self->px(650)}; return 0;
            case WM_DPICHANGED: {
                self->dpi = HIWORD(wp); self->fonts(); auto r = reinterpret_cast<RECT*>(lp);
                SetWindowPos(hwnd, nullptr, r->left, r->top, r->right-r->left, r->bottom-r->top, SWP_NOACTIVATE | SWP_NOZORDER);
                self->layout(); return 0;
            }
            case WM_DESTROY:
                KillTimer(hwnd, 1); self->shown = false; self->window = nullptr; PostQuitMessage(0); return 0;
            case WM_NCDESTROY: SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0); break;
            }
        } catch (...) {
            if (msg == WM_CREATE) { self->startup_error = std::current_exception(); return -1; }
            try { throw; }
            catch (const std::exception& error) { MessageBoxW(hwnd, widen(error.what()).c_str(), L"ScreenBuddy", MB_OK | MB_ICONERROR); }
            catch (...) { MessageBoxW(hwnd, L"Native settings error", L"ScreenBuddy", MB_OK | MB_ICONERROR); }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    void run() {
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool signaled = false;
        try {
            INITCOMMONCONTROLSEX init{sizeof(init), ICC_TAB_CLASSES | ICC_STANDARD_CLASSES}; InitCommonControlsEx(&init);
            HINSTANCE instance = GetModuleHandleW(nullptr);
            WNDCLASSEXW klass{sizeof(klass)}; klass.lpfnWndProc = procedure; klass.hInstance = instance;
            klass.hCursor = LoadCursorW(nullptr, IDC_ARROW); klass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
            klass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1)); klass.lpszClassName = WindowClass;
            if (!RegisterClassExW(&klass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                throw std::runtime_error("Native settings: RegisterClassExW");
            HWND created = CreateWindowExW(WS_EX_CONTROLPARENT, WindowClass, tr(L"ScreenBuddy – Einstellungen", L"ScreenBuddy – Settings"),
                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1120, 840, nullptr, nullptr, instance, this);
            if (!created) {
                if (startup_error) std::rethrow_exception(startup_error);
                throw std::runtime_error("Native settings: CreateWindowExW");
            }
            RECT desired{0, 0, 1040, 680};
            AdjustWindowRectExForDpi(&desired, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT, dpi);
            MONITORINFO monitor{sizeof monitor};
            if (GetMonitorInfoW(MonitorFromWindow(created, MONITOR_DEFAULTTONEAREST), &monitor)) {
                const int available_w = monitor.rcWork.right - monitor.rcWork.left;
                const int available_h = monitor.rcWork.bottom - monitor.rcWork.top;
                const int outer_w = std::min(static_cast<int>(desired.right - desired.left), available_w);
                const int outer_h = std::min(static_cast<int>(desired.bottom - desired.top), available_h);
                const int x = monitor.rcWork.left + std::max(0, (available_w - outer_w) / 2);
                const int y = monitor.rcWork.top + std::max(0, (available_h - outer_h) / 2);
                SetWindowPos(created, nullptr, x, y, outer_w, outer_h, SWP_NOACTIVATE | SWP_NOZORDER);
            }
            window = created;
            signal_ready(); signaled = true;
            MSG msg{};
            while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                if (!IsDialogMessageW(created, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
            }
        } catch (...) {
            if (!signaled) signal_ready(std::current_exception());
        }
        if (hwnd && IsWindow(hwnd)) DestroyWindow(hwnd);
        window = nullptr; shown = false;
        if (font) { DeleteObject(font); font = nullptr; }
        if (SUCCEEDED(com)) CoUninitialize();
    }
};

NativeSettings::NativeSettings(App& app, NativePluginActions plugins) : d_(std::make_unique<Impl>(app, std::move(plugins))) { d_->start(); }
NativeSettings::~NativeSettings() = default;
void NativeSettings::show() { if (HWND hwnd = d_->window.load()) PostMessageW(hwnd, Show, 0, 0); }
bool NativeSettings::visible() const { return d_->shown; }
}  // namespace fl
