// © 2026 FuchsLabs
#include "native_settings.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cmath>
#include <cwchar>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

#include "app.h"
#include "config.h"
#include "util.h"
#include "winsetup.h"

namespace fl {
namespace {
constexpr wchar_t WindowClass[] = L"ScreenBuddyNativeSettings";
constexpr UINT Show = WM_APP + 171, Stop = WM_APP + 172, Results = WM_APP + 173;
enum Id : int { Tabs = 100, OutputControl, Port, Monitor, OrientationControl, Theme, Brightness, Seconds,
    Touch, ResetOnStart, OffOnExit, Language, Updates, Save, Preview, PreviewTitle, Status,
    PluginList, PluginImport, PluginToggle, PluginRemove, PluginRefresh, PluginNote, GeneralNote, Advanced,
    Source, SourceNote, PageList, PageToggle, PageUp, PageDown, PageNote,
    CompanionEnabled, CompanionPort, CompanionSeconds, CompanionPages, CompanionToggle, CompanionUp, CompanionDown, CompanionNote,
    Autostart, ApplyAutostart, CheckUpdate, UpdateStatus, ApplicationNote, PluginConsent,
    PluginConfigure, PluginSettingsSave, PluginSettingsNote, PluginSettingsTitle };
enum Page : int { DisplayPage, SourcesPage, PagesPage, CompanionPage, ApplicationPage, PluginsPage, PluginSettingsPage };
constexpr const char* Outputs[] = {"serial", "monitor", "network"};
constexpr const char* Orientations[] = {"landscape", "reverse_landscape", "portrait", "reverse_portrait"};
constexpr const char* Themes[] = {"hell", "dunkel", "eigen"};
constexpr const char* Sources[] = {"system", "friends", "discord", "bf6", "rss", "homeassistant"};
constexpr const char* NetUnits[] = {"bit", "byte"};
enum class FieldKind { Text, Secret, List, Bool, Number, Unit };
struct SourceField {
    const char *section, *key;
    const wchar_t *de, *en;
    FieldKind kind = FieldKind::Text;
    int minimum = 0, maximum = 0;
    int id = 0, source = 0;
};
}

struct NativeSettings::Impl {
    App& app;
    NativePluginActions plugins;
    ojson initial; // Public snapshot: credentials are never copied into edit controls.
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
        enum Kind { Snapshot, Saved, PluginChanged, ApplicationChanged, Failure, PassiveFailure } kind = Snapshot;
        std::optional<Image> frame;
        std::wstring status;
        std::vector<NativePluginEntry> entries;
        ojson update;
        bool application_snapshot = false;
        std::optional<bool> autostart;
        std::wstring update_status;
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
    bool application_loaded = false;
    int source = 0;
    std::vector<SourceField> source_fields;
    struct PageItem { std::string key; bool enabled; };
    std::vector<PageItem> display_pages, companion_pages;
    std::string editing_plugin;
    std::vector<NativePluginSetting> plugin_settings;

    Impl(App& a, NativePluginActions p) : app(a), plugins(std::move(p)), initial(config::public_view(a.cfg())),
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
    const wchar_t* page_title(const std::string& key) const {
        if (key == "system") return L"System";
        if (key == "friends") return tr(L"Freunde", L"Friends");
        if (key == "discord") return L"Discord";
        if (key == "bf6") return L"Battlefield 6";
        if (key == "rss") return tr(L"Blog-Updates (RSS)", L"Blog updates (RSS)");
        return L"Home Assistant";
    }
    void source_field(SourceField field) {
        field.id = 200 + static_cast<int>(source_fields.size());
        for (int i = 0; i < int(std::size(Sources)); ++i) if (field.section == std::string(Sources[i])) field.source = i;
        const auto& entry = initial.at(field.section).at(field.key);
        if (field.kind == FieldKind::Bool) check(field.id, SourcesPage, field.de, field.en, entry.get<bool>());
        else {
            label(field.id, field.de, field.en, SourcesPage);
            if (field.kind == FieldKind::Unit) {
                combo(field.id, SourcesPage, {L"bit/s", L"Byte/s"}); select(field.id, NetUnits, entry.get<std::string>());
            } else if (field.kind == FieldKind::List) {
                std::string text;
                for (const auto& item : entry) {
                    if (!text.empty()) text += "\r\n";
                    text += item.is_string() ? item.get<std::string>() : item.dump();
                }
                add(L"EDIT", widen(text).c_str(), WS_BORDER | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                    field.id, SourcesPage);
                SendMessageW(child(field.id), EM_SETLIMITTEXT, 16384, 0);
            } else {
                const bool secret = field.kind == FieldKind::Secret;
                edit(field.id, SourcesPage, secret ? "" : (entry.is_string() ? entry.get<std::string>() : entry.dump()), field.kind == FieldKind::Number);
                SendMessageW(child(field.id), EM_SETLIMITTEXT, field.kind == FieldKind::Number ? 5 : 300, 0);
                if (secret) {
                    SendMessageW(child(field.id), EM_SETPASSWORDCHAR, L'\x25CF', 0);
                    const bool existing = initial.at(field.section).value(std::string(field.key) + "_set", false);
                    const wchar_t* hint = existing ? tr(L"Gespeichert – leer lassen zum Beibehalten", L"Saved – leave empty to keep")
                                                   : tr(L"Noch nicht eingerichtet", L"Not configured yet");
                    SendMessageW(child(field.id), EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(hint));
                }
            }
        }
        source_fields.push_back(field);
    }
    void initialize_sources() {
        label(Source, L"&Datenquelle", L"&Data source", SourcesPage);
        combo(Source, SourcesPage, {L"System", tr(L"Freunde (Steam / Xbox / Discord)", L"Friends (Steam / Xbox / Discord)"), L"Discord", L"Battlefield 6", L"RSS", L"Home Assistant"});
        SendMessageW(child(Source), CB_SETCURSEL, 0, 0);
        using K = FieldKind;
        for (SourceField field : {
            SourceField{"system", "enabled", L"System aktivieren", L"Enable system", K::Bool},
            {"system", "title", L"Titel", L"Title"},
            {"system", "net_adapter", L"Netzwerkadapter (leer = automatisch)", L"Network adapter (empty = automatic)"},
            {"system", "net_unit", L"Netzwerkeinheit", L"Network unit", K::Unit},
            {"system", "disk", L"Laufwerk", L"Disk"},
            {"system", "cpu_temp_sensor", L"CPU-Temperatursensor (leer = automatisch)", L"CPU temperature sensor (empty = automatic)"},
            {"system", "cpu_fan_sensor", L"CPU-Lüftersensor (leer = automatisch)", L"CPU fan sensor (empty = automatic)"},
            {"friends", "enabled", L"Freunde aktivieren", L"Enable friends", K::Bool},
            {"friends", "title", L"Titel", L"Title"},
            {"friends", "include_discord", L"Discord einbeziehen", L"Include Discord", K::Bool},
            {"friends", "auto_merge", L"Gleiche Namen automatisch verbinden", L"Automatically merge matching names", K::Bool},
            {"friends", "show_offline", L"Offline-Konten anzeigen", L"Show offline accounts", K::Bool},
            {"friends", "steam_api_key", L"Steam-API-Schlüssel (leer = beibehalten)", L"Steam API key (empty = keep)", K::Secret},
            {"friends", "steam_id", L"Steam-ID oder Profiladresse", L"Steam ID or profile URL"},
            {"friends", "xbox_api_key", L"Xbox-API-Schlüssel (leer = beibehalten)", L"Xbox API key (empty = keep)", K::Secret},
            {"friends", "links", L"Verknüpfungen: Name|discord:ID|steam:ID|xbox:Name", L"Links: Name|discord:ID|steam:ID|xbox:Name", K::List},
            {"discord", "enabled", L"Discord aktivieren", L"Enable Discord", K::Bool},
            {"display", "title", L"Titel der Discord-Seite", L"Discord page title"},
            {"discord", "token", L"Bot-Token (leer = beibehalten)", L"Bot token (empty = keep)", K::Secret},
            {"discord", "show_offline", L"Offline-Personen anzeigen", L"Show offline people", K::Bool},
            {"discord", "guild_ids", L"Server-IDs (eine pro Zeile)", L"Server IDs (one per line)", K::List},
            {"discord", "friend_ids", L"Personen-IDs (eine pro Zeile)", L"Person IDs (one per line)", K::List},
            {"discord", "ignore_ids", L"Ausgeblendete IDs (eine pro Zeile)", L"Ignored IDs (one per line)", K::List},
            {"bf6", "enabled", L"Battlefield 6 aktivieren", L"Enable Battlefield 6", K::Bool},
            {"bf6", "title", L"Titel", L"Title"},
            {"bf6", "players", L"EA-IDs (eine pro Zeile; Konsole: @psn / @xbox)", L"EA IDs (one per line; console: @psn / @xbox)", K::List},
            {"bf6", "refresh_minutes", L"Aktualisieren (5–240 Minuten)", L"Refresh (5–240 minutes)", K::Number, 5, 240},
            {"rss", "enabled", L"RSS aktivieren", L"Enable RSS", K::Bool},
            {"rss", "title", L"Titel", L"Title"},
            {"rss", "feeds", L"Feed-Adressen (eine pro Zeile)", L"Feed URLs (one per line)", K::List},
            {"rss", "refresh_minutes", L"Aktualisieren (10–720 Minuten)", L"Refresh (10–720 minutes)", K::Number, 10, 720},
            {"rss", "max_items", L"Einträge insgesamt (1–4)", L"Total items (1–4)", K::Number, 1, 4},
            {"rss", "max_per_feed", L"Einträge je Feed (1–4)", L"Items per feed (1–4)", K::Number, 1, 4},
            {"homeassistant", "enabled", L"Home Assistant aktivieren", L"Enable Home Assistant", K::Bool},
            {"homeassistant", "title", L"Titel", L"Title"},
            {"homeassistant", "url", L"Home-Assistant-Adresse", L"Home Assistant URL"},
            {"homeassistant", "token", L"Zugriffstoken (leer = beibehalten)", L"Access token (empty = keep)", K::Secret},
            {"homeassistant", "entities", L"Entitäts-IDs (höchstens 4; eine pro Zeile)", L"Entity IDs (up to 4; one per line)", K::List}
        }) {
            if (std::string(field.section) == "display") { source_field(field); source_fields.back().source = 2; }
            else source_field(field);
        }
        add(L"STATIC", tr(L"Listen: ein Eintrag pro Zeile. Leere Zugangsfelder behalten gespeicherte Schlüssel bei. Auswahlhilfen und eigene Farben sind weiterhin unter Erweiterte Einstellungen verfügbar.",
                           L"Lists: one entry per line. Empty credential fields keep saved keys. Selection helpers and custom colors remain available in Advanced settings."), SS_LEFT, SourceNote, SourcesPage);
    }
    void initialize_pages(int list, int toggle, int up, int down, int group, const ojson& order, const ojson& selected,
                          std::vector<PageItem>& items) {
        for (const auto& key : order) {
            if (!key.is_string() || std::find(config::PAGES.begin(), config::PAGES.end(), key.get<std::string>()) == config::PAGES.end()) continue;
            if (std::any_of(items.begin(), items.end(), [&](const auto& item) { return item.key == key.get<std::string>(); })) continue;
            items.push_back({key.get<std::string>(), std::find(selected.begin(), selected.end(), key) != selected.end()});
        }
        for (const auto& key : config::PAGES)
            if (std::none_of(items.begin(), items.end(), [&](const auto& item) { return item.key == key; })) items.push_back({key, false});
        add(L"LISTBOX", L"", WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, list, group);
        add(L"BUTTON", tr(L"Ein-/ausblenden", L"Show / hide"), BS_PUSHBUTTON | WS_TABSTOP, toggle, group);
        add(L"BUTTON", tr(L"Nach oben", L"Move up"), BS_PUSHBUTTON | WS_TABSTOP, up, group);
        add(L"BUTTON", tr(L"Nach unten", L"Move down"), BS_PUSHBUTTON | WS_TABSTOP, down, group);
        fill_pages(list, items, 0);
    }
    void fill_pages(int list, const std::vector<PageItem>& items, int selected) {
        SendMessageW(child(list), LB_RESETCONTENT, 0, 0);
        for (const auto& item : items) {
            std::wstring text = item.enabled ? L"[✓] " : L"[ ] "; text += page_title(item.key);
            SendMessageW(child(list), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }
        SendMessageW(child(list), LB_SETCURSEL, selected, 0);
    }
    void initialize() {
        dpi = GetDpiForWindow(hwnd);
        add(WC_TABCONTROLW, L"", WS_TABSTOP, Tabs, -1);
        for (auto title : {tr(L"Anzeige", L"Display"), tr(L"Quellen", L"Sources"), tr(L"Seiten", L"Pages"), L"Companion", tr(L"Anwendung", L"Application"), L"Plugins", tr(L"Plugin-Konfiguration", L"Plugin configuration")}) {
            TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = const_cast<wchar_t*>(title);
            TabCtrl_InsertItem(child(Tabs), TabCtrl_GetItemCount(child(Tabs)), &item);
        }
        const auto& dp = initial.at("display");
        label(OutputControl, L"&Ausgabe", L"&Output", 0);
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
        initialize_sources();
        initialize_pages(PageList, PageToggle, PageUp, PageDown, PagesPage, dp.at("pages"), dp.at("visible_pages"), display_pages);
        add(L"STATIC", tr(L"Die markierten Seiten erscheinen auf USB/HDMI in dieser Reihenfolge, wenn die zugehörige Quelle aktiv und eingerichtet ist. Bei reiner Netzwerk-Ausgabe gilt die Companion-Liste.",
                           L"Selected pages appear on USB/HDMI in this order when their source is enabled and configured. Network-only output uses the Companion list."), SS_LEFT, PageNote, PagesPage);
        const auto& companion = initial.at("companion");
        check(CompanionEnabled, CompanionPage, L"Companion zusätzlich aktivieren", L"Enable additional Companion output", companion.value("enabled", false));
        label(CompanionPort, L"TLS-Port (1024–65535; nie Port der Einstellungen)", L"TLS port (1024–65535; never the settings port)", CompanionPage);
        edit(CompanionPort, CompanionPage, std::to_string(companion.value("port", 8777)), true);
        SendMessageW(child(CompanionPort), EM_SETLIMITTEXT, 5, 0);
        label(CompanionSeconds, L"Seitenwechsel (3–600 Sekunden)", L"Page interval (3–600 seconds)", CompanionPage);
        edit(CompanionSeconds, CompanionPage, std::to_string(companion.value("page_seconds", 15)), true);
        initialize_pages(CompanionPages, CompanionToggle, CompanionUp, CompanionDown, CompanionPage, companion.at("pages"), companion.at("pages"), companion_pages);
        add(L"STATIC", tr(L"Companion verwendet eine eigene Seitenauswahl und Reihenfolge. Änderungen gelten nach Speichern und Neustart. Geräte koppeln/entfernen und QR-Code: Erweiterte Einstellungen → Companion.",
                           L"Companion uses its own page selection and order. Changes apply after saving and restarting. Pair/remove devices and show the QR code: Advanced settings → Companion."), SS_LEFT, CompanionNote, CompanionPage);
        label(Language, L"&Sprache (nach Neustart)", L"&Language (after restart)", ApplicationPage);
        combo(Language, ApplicationPage, {L"Deutsch", L"English"});
        SendMessageW(child(Language), CB_SETCURSEL, english ? 1 : 0, 0);
        check(Updates, ApplicationPage, L"Automatisch nach &Updates suchen", L"Automatically check for &updates", initial.at("ui").value("update_check", true));
        add(L"BUTTON", tr(L"Jetzt nach Updates suchen", L"Check for updates now"), BS_PUSHBUTTON | WS_TABSTOP, CheckUpdate, ApplicationPage);
        add(L"STATIC", L"", SS_LEFT, UpdateStatus, ApplicationPage);
        check(Autostart, ApplicationPage, L"Mit Windows starten (nur Infobereich)", L"Start with Windows (tray only)", false);
        add(L"BUTTON", tr(L"Windows-Start übernehmen", L"Apply Windows startup"), BS_PUSHBUTTON | WS_TABSTOP, ApplyAutostart, ApplicationPage);
        add(L"STATIC", tr(L"Windows-Start wird mit der eigenen Schaltfläche sofort übernommen. In Testinstanzen bleibt der echte Autostart gesperrt.\r\n\r\nSpeichern übernimmt die Einstellungen und startet ScreenBuddy neu. Leere Zugangsfelder und eigene Farben bleiben erhalten.",
                           L"Windows startup applies immediately using its own button. Real autostart changes are disabled in test instances.\r\n\r\nSave applies settings and restarts ScreenBuddy. Empty credential fields and custom colors are preserved."), SS_LEFT, GeneralNote, ApplicationPage);
        add(L"STATIC", tr(L"Live-Vorschau", L"Live preview"), SS_LEFT, PreviewTitle, -2);
        add(L"STATIC", tr(L"Aktuelles Displaybild", L"Current display frame"), SS_OWNERDRAW, Preview, -2);
        add(L"STATIC", tr(L"Status wird geladen …", L"Loading status …"), SS_LEFT, Status, -2);
        add(L"BUTTON", tr(L"&Speichern und neu starten", L"&Save and restart"), BS_DEFPUSHBUTTON | WS_TABSTOP, Save, -1);
        add(L"BUTTON", tr(L"Erweiterte Einstellungen …", L"Advanced settings …"), BS_PUSHBUTTON | WS_TABSTOP, Advanced, -1);
        add(L"LISTBOX", L"", WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, PluginList, PluginsPage);
        add(L"BUTTON", tr(L"Manifest &importieren …", L"&Import manifest …"), BS_PUSHBUTTON | WS_TABSTOP, PluginImport, PluginsPage);
        add(L"BUTTON", tr(L"&Aktivieren / deaktivieren", L"&Enable / disable"), BS_PUSHBUTTON | WS_TABSTOP, PluginToggle, PluginsPage);
        add(L"BUTTON", tr(L"&Entfernen", L"&Remove"), BS_PUSHBUTTON | WS_TABSTOP, PluginRemove, PluginsPage);
        add(L"BUTTON", tr(L"A&ktualisieren", L"Re&fresh"), BS_PUSHBUTTON | WS_TABSTOP, PluginRefresh, PluginsPage);
        add(L"BUTTON", tr(L"Freigaben widerrufen", L"Revoke host consent"), BS_PUSHBUTTON | WS_TABSTOP, PluginConsent, PluginsPage);
        add(L"BUTTON", tr(L"Plugin konfigurieren …", L"Configure plugin …"), BS_PUSHBUTTON | WS_TABSTOP, PluginConfigure, PluginsPage);
        add(L"STATIC", tr(L"Zuerst unter Plugins ein Plugin auswählen und konfigurieren.", L"Select a plugin and choose Configure plugin first."), SS_LEFT, PluginSettingsTitle, PluginSettingsPage);
        add(L"STATIC", tr(L"Leere Zugangsfelder behalten gespeicherte Werte. Zum Entfernen ausdrücklich Löschen markieren. Die Plugin-Konfiguration wird sofort gespeichert.",
                           L"Empty credential fields keep saved values. Select Clear explicitly to remove a credential. Plugin settings save immediately."), SS_LEFT, PluginSettingsNote, PluginSettingsPage);
        add(L"BUTTON", tr(L"Plugin-Konfiguration speichern", L"Save plugin settings"), BS_PUSHBUTTON | WS_TABSTOP, PluginSettingsSave, PluginSettingsPage);
        add(L"STATIC", plugins.list ? tr(L"Plugins werden lokal verwaltet. Ein Import führt keinen Plugin-Code aus.", L"Plugins are managed locally. Import does not execute plugin code.")
                                          : tr(L"Die Plugin-Verwaltung ist in diesem Build noch nicht verbunden.", L"Plugin management is not connected in this build yet."), SS_LEFT, PluginNote, PluginsPage);
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
        move(Updates, 24, 136, 320, 40);
        move(CheckUpdate, 24, 188, 320, 32); move(UpdateStatus, 24, 230, 320, 78);
        move(Autostart, 24, 318, 320, 34); move(ApplyAutostart, 24, 360, 320, 32);
        move(GeneralNote, 24, 408, 320, std::max(36, h - 480));
        move(Source + 1000, 24, 62, w - 48, 22); move(Source, 24, 88, std::min(440, w - 48), 190);
        const int field_width = (w - 72) / 2;
        int positions[2] = {138, 138}, column = 0;
        for (const auto& field : source_fields) {
            if (field.source != source) continue;
            int x = column == 0 ? 24 : 48 + field_width, y = positions[column];
            if (field.kind == FieldKind::Bool) {
                move(field.id, x, y, field_width, 30); positions[column] += 44;
            } else {
                move(field.id + 1000, x, y, field_width, 22);
                const int height = field.kind == FieldKind::List ? 84 : 28;
                move(field.id, x, y + 24, field_width, field.kind == FieldKind::Unit ? 150 : height);
                positions[column] += height + 38;
            }
            column = positions[0] <= positions[1] ? 0 : 1;
        }
        move(SourceNote, 24, h - 112, w - 48, 48);
        move(PageList, 24, 66, w - 300, h - 244);
        move(PageToggle, w - 252, 66, 228, 34); move(PageUp, w - 252, 112, 228, 34); move(PageDown, w - 252, 158, 228, 34);
        move(PageNote, 24, h - 160, w - 48, 70);
        move(CompanionEnabled, 24, 62, w - 48, 30);
        move(CompanionPort + 1000, 24, 108, field_width, 22); move(CompanionPort, 24, 134, field_width, 28);
        move(CompanionSeconds + 1000, 48 + field_width, 108, field_width, 22); move(CompanionSeconds, 48 + field_width, 134, field_width, 28);
        move(CompanionPages, 24, 184, w - 300, h - 362);
        move(CompanionToggle, w - 252, 184, 228, 34); move(CompanionUp, w - 252, 230, 228, 34); move(CompanionDown, w - 252, 276, 228, 34);
        move(CompanionNote, 24, h - 160, w - 48, 70);
        move(PreviewTitle, 376, 62, w - 400, 24);
        int available_w = std::max(1, w - 400), available_h = std::max(1, h - 320);
        int source_w = frame ? frame->w : 480, source_h = frame ? frame->h : 320;
        double scale = std::min(double(available_w) / std::max(1, source_w), double(available_h) / std::max(1, source_h));
        move(Preview, 376, 96, std::max(1, int(source_w * scale)), std::max(1, int(source_h * scale)));
        move(Status, 376, h - 205, w - 400, 140);
        move(Save, 24, h - 54, 320, 34);
        move(Advanced, 376, h - 54, 320, 34);
        move(PluginList, 24, 66, w - 48, h - 285);
        move(PluginImport, 24, h - 162, 230, 32); move(PluginToggle, 270, h - 162, 230, 32);
        move(PluginRemove, 516, h - 162, 160, 32); move(PluginRefresh, 692, h - 162, 160, 32);
        move(PluginConsent, 24, h - 204, 230, 32);
        move(PluginConfigure, 270, h - 204, 230, 32);
        move(PluginNote, 24, h - 112, w - 48, 50);
        move(PluginSettingsTitle, 24, 62, w - 48, 28);
        for (size_t i = 0; i < plugin_settings.size(); ++i) {
            int x = i % 2 ? 48 + field_width : 24, y = 104 + int(i / 2) * 70;
            move(1500 + int(i), x, y, field_width, 22);
            int input_width = plugin_settings[i].type == "secret" ? field_width - 100 : field_width;
            move(500 + int(i), x, y + 24, input_width, 28);
            if (plugin_settings[i].type == "secret") move(600 + int(i), x + input_width + 8, y + 24, 92, 28);
        }
        move(PluginSettingsNote, 24, h - 146, w - 48, 60);
        move(PluginSettingsSave, 24, h - 54, 320, 34);
    }
    void update_controls() {
        for (auto& c : controls) {
            bool visible = c.page == -1 || c.page == page || (c.page == -2 && (page == DisplayPage || page == ApplicationPage));
            if (c.page == SourcesPage && c.id != Source && c.id != Source + 1000 && c.id != SourceNote) {
                auto field = std::find_if(source_fields.begin(), source_fields.end(), [&](const auto& f) { return c.id == f.id || c.id == f.id + 1000; });
                visible = page == SourcesPage && field != source_fields.end() && field->source == source;
            }
            if (c.id == Save && (page == PluginsPage || page == PluginSettingsPage)) visible = false;
            ShowWindow(c.hwnd, visible ? SW_SHOW : SW_HIDE);
        }
        EnableWindow(child(Save), !action_pending && !saving);
        EnableWindow(child(Advanced), bool(plugins.open_advanced) && !saving);
        EnableWindow(child(ApplyAutostart), application_loaded && !test_instance() && !action_pending && !saving);
        EnableWindow(child(Autostart), application_loaded && !test_instance() && !action_pending && !saving);
        EnableWindow(child(CheckUpdate), initial.at("ui").value("update_check", true) && !action_pending && !saving);
        EnableWindow(child(PluginImport), !action_pending && !saving && bool(plugins.import_manifest));
        EnableWindow(child(PluginRefresh), !action_pending && !saving && bool(plugins.list));
        bool selected = SendMessageW(child(PluginList), LB_GETCURSEL, 0, 0) != LB_ERR;
        EnableWindow(child(PluginToggle), !action_pending && !saving && selected && bool(plugins.set_enabled));
        EnableWindow(child(PluginRemove), !action_pending && !saving && selected && bool(plugins.remove));
        EnableWindow(child(PluginConsent), !action_pending && !saving && selected && bool(plugins.set_host_consent));
        EnableWindow(child(PluginConfigure), !action_pending && !saving && selected && bool(plugins.save_settings));
        EnableWindow(child(PluginSettingsSave), !action_pending && !saving && !editing_plugin.empty() && bool(plugins.save_settings));
        for (int list : {PageList, CompanionPages}) {
            auto index = SendMessageW(child(list), LB_GETCURSEL, 0, 0);
            const auto& items = list == PageList ? display_pages : companion_pages;
            int toggle = list == PageList ? PageToggle : CompanionToggle;
            int up = list == PageList ? PageUp : CompanionUp;
            int down = list == PageList ? PageDown : CompanionDown;
            bool valid = index >= 0 && size_t(index) < items.size();
            EnableWindow(child(toggle), valid && !saving);
            EnableWindow(child(up), valid && index > 0 && !saving);
            EnableWindow(child(down), valid && size_t(index + 1) < items.size() && !saving);
        }
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
        for (const auto& field : source_fields) {
            auto& section = incoming[field.section];
            if (field.kind == FieldKind::Bool) section[field.key] = checked(field.id);
            else if (field.kind == FieldKind::Number) section[field.key] = number(field.id, field.minimum, field.maximum);
            else if (field.kind == FieldKind::Unit) section[field.key] = choice(field.id, NetUnits);
            else if (field.kind == FieldKind::List) {
                section[field.key] = ojson::array();
                std::istringstream lines(value(field.id));
                std::string line;
                while (std::getline(lines, line)) if (!trim(line).empty()) section[field.key].push_back(trim(line));
            } else {
                std::string text = trim(value(field.id));
                // Do not put a placeholder or a blank credential into the patch.
                if (field.kind != FieldKind::Secret || !text.empty()) section[field.key] = std::move(text);
            }
        }
        incoming["display"]["pages"] = ojson::array();
        incoming["display"]["visible_pages"] = ojson::array();
        for (const auto& item : display_pages) {
            incoming["display"]["pages"].push_back(item.key);
            if (item.enabled) incoming["display"]["visible_pages"].push_back(item.key);
        }
        incoming["companion"] = {{"enabled", checked(CompanionEnabled)}, {"port", number(CompanionPort, 1024, 65535)},
            {"page_seconds", number(CompanionSeconds, 3, 600)}, {"pages", ojson::array()}};
        for (const auto& item : companion_pages) if (item.enabled) incoming["companion"]["pages"].push_back(item.key);
        config::merge(initial, incoming); // Validate before placing a save job on the queue.
        return incoming;
    }

    void poll() {
        if (stopping || !shown || snapshot_pending.exchange(true)) return;
        const bool use_live_frame = !previewing_theme, load_application = !application_loaded;
        passive([this, use_live_frame, load_application] {
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
            result.update = app.update_info();
            result.application_snapshot = true;
            if (load_application) result.autostart = winsetup::autostart_enabled();
            deliver(std::move(result));
        });
    }
    void application_action(int id) {
        if (action_pending || saving) return;
        const bool desired = checked(Autostart);
        if (id == ApplyAutostart && (!application_loaded || test_instance())) return;
        action_pending = true; update_controls();
        enqueue([this, id, desired] {
            Result result; result.kind = Result::ApplicationChanged;
            if (id == ApplyAutostart) {
                if (test_instance()) throw std::runtime_error("Autostart is disabled in test instances.");
                winsetup::set_autostart(desired);
                bool actual = winsetup::autostart_enabled();
                if (actual != desired) throw std::runtime_error(english ? "Windows startup could not be changed." : "Windows-Start konnte nicht geändert werden.");
                result.autostart = actual;
                result.status = tr(L"Windows-Start übernommen.", L"Windows startup applied.");
            } else {
                bool started = app.update_check_now();
                result.update = app.update_info();
                result.update_status = started ? tr(L"Update-Prüfung angefordert …", L"Update check requested …")
                    : tr(L"Update-Prüfung aus oder bereits kürzlich angefordert (5 Minuten Pause).", L"Update checks are disabled or were requested recently (5-minute interval).");
            }
            deliver(std::move(result));
        });
    }
    void show_update(const ojson& info) {
        std::wstring text;
        if (info.is_null() || info.empty()) text = tr(L"Update-Prüfung deaktiviert. Zum Aktivieren speichern und neu starten.", L"Update checks are disabled. Save and restart to enable.");
        else {
            text = tr(L"Aktuell: ", L"Current: ") + widen(info.value("current", ""));
            const std::string error = info.value("error", "");
            if (!error.empty()) text += L"\r\n" + widen(error);
            else if (info.value("available", false)) text += tr(L"\r\nNeue Version: ", L"\r\nNew version: ") + widen(info.value("latest", ""));
            else if (info.value("checked_at", 0LL) > 0) text += tr(L"\r\nKeine neuere Version gefunden.", L"\r\nNo newer version found.");
            else text += tr(L"\r\nNoch nicht geprüft.", L"\r\nNot checked yet.");
        }
        SetWindowTextW(child(UpdateStatus), text.c_str());
    }
    void refresh_plugins() {
        if (!plugins.list || action_pending || saving) return;
        action_pending = true; update_controls();
        enqueue([this] { Result result; result.kind = Result::PluginChanged; result.entries = plugins.list(); deliver(std::move(result)); });
    }
    void configure_plugin(const NativePluginEntry& entry) {
        if (entry.settings.size() > 12) throw std::runtime_error("At most 12 plugin settings allowed.");
        for (auto it = controls.begin(); it != controls.end();) {
            if (it->page == PluginSettingsPage && ((it->id >= 500 && it->id < 612) || (it->id >= 1500 && it->id < 1512))) {
                DestroyWindow(it->hwnd); it = controls.erase(it);
            } else ++it;
        }
        editing_plugin = entry.id; plugin_settings = entry.settings;
        SetWindowTextW(child(PluginSettingsTitle), widen(entry.name + " (" + entry.id + ")").c_str());
        for (size_t i = 0; i < plugin_settings.size(); ++i) {
            auto& field = plugin_settings[i];
            int id = 500 + int(i);
            std::wstring title = widen(field.label + " (" + field.key + ")") + (field.required ? L" *" : L"");
            add(L"STATIC", title.c_str(), SS_LEFT, id + 1000, PluginSettingsPage);
            if (field.type == "secret") field.value.clear();
            edit(id, PluginSettingsPage, field.value);
            SendMessageW(child(id), EM_SETLIMITTEXT, 16384, 0);
            if (field.type == "secret") {
                SendMessageW(child(id), EM_SETPASSWORDCHAR, L'\x25CF', 0);
                const wchar_t* hint = field.secret_set ? tr(L"Gespeichert – leer lassen zum Beibehalten", L"Saved – leave empty to keep")
                                                      : tr(L"Noch nicht eingerichtet", L"Not configured yet");
                SendMessageW(child(id), EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(hint));
                check(600 + int(i), PluginSettingsPage, L"Löschen", L"Clear", false);
            }
        }
        fonts(); update_controls(); layout();
    }
    void save_plugin_settings() {
        if (saving || action_pending || editing_plugin.empty() || !plugins.save_settings) return;
        nlohmann::json patch = nlohmann::json::object();
        for (size_t i = 0; i < plugin_settings.size(); ++i) {
            const auto& field = plugin_settings[i];
            std::string text = value(500 + int(i));
            if (field.type == "secret") {
                if (checked(600 + int(i))) { patch[field.key] = nullptr; continue; }
                if (text.empty()) {
                    if (field.required && !field.secret_set) throw std::invalid_argument(english ? "Enter the required plugin credential." : "Bitte das erforderliche Plugin-Zugangsfeld ausfüllen.");
                    continue;
                }
            } else if (field.required && trim(text).empty()) {
                throw std::invalid_argument(english ? "Fill in every required plugin field." : "Bitte alle erforderlichen Plugin-Felder ausfüllen.");
            }
            if (field.type == "number") {
                auto number = nlohmann::json::parse(trim(text), nullptr, false);
                if (number.is_discarded() || !number.is_number() || !std::isfinite(number.get<double>()))
                    throw std::invalid_argument(english ? "Enter a finite plugin number." : "Bitte eine endliche Zahl eingeben.");
                patch[field.key] = std::move(number);
            } else patch[field.key] = std::move(text);
        }
        action_pending = true; update_controls();
        enqueue([this, id = editing_plugin, patch = std::move(patch)] {
            plugins.save_settings(id, patch);
            Result result; result.kind = Result::PluginChanged;
            result.status = tr(L"Plugin-Konfiguration gespeichert.", L"Plugin settings saved.");
            if (plugins.list) result.entries = plugins.list();
            deliver(std::move(result));
        });
    }
    void plugin_action(int id) {
        if (action_pending || saving) return;
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
            if (id == PluginToggle && !selected->enabled && !selected->hosts.empty()) {
                const bool missing = std::any_of(selected->settings.begin(), selected->settings.end(), [](const auto& field) {
                    return field.required && (field.type == "secret" ? !field.secret_set : trim(field.value).empty());
                });
                if (missing) {
                    MessageBoxW(hwnd, tr(L"Bitte zuerst die erforderlichen Plugin-Einstellungen unter „Plugin konfigurieren“ ausfüllen.",
                        L"Fill in the required plugin settings using Configure plugin first."), L"ScreenBuddy – Plugin", MB_OK | MB_ICONINFORMATION);
                    return;
                }
            }
            if (id == PluginToggle && !selected->enabled && !selected->hosts.empty()) {
                if (!plugins.set_host_consent) throw std::runtime_error(english ? "Host consent is not connected in this build." : "Host-Freigaben sind in diesem Build nicht angebunden.");
                std::wstring prompt = tr(L"Dieses Plugin darf die folgenden Hosts abrufen. Dabei kann es Werte der aufgeführten Plugin-Einstellungen an diese Hosts senden.\r\n\r\nHosts:\r\n",
                    L"This plugin may fetch from the following hosts. It may send values from the listed plugin settings to these hosts.\r\n\r\nHosts:\r\n");
                for (const auto& host : selected->hosts) {
                    bool consented = std::find(selected->consented_hosts.begin(), selected->consented_hosts.end(), host) != selected->consented_hosts.end();
                    prompt += L"• " + widen(host) + (consented ? tr(L" (bereits freigegeben)", L" (already allowed)") : tr(L" (neue Freigabe)", L" (new consent)")) + L"\r\n";
                }
                prompt += tr(L"\r\nEinstellungsschlüssel:\r\n", L"\r\nSetting keys:\r\n");
                if (selected->setting_keys.empty()) prompt += tr(L"(keine)\r\n", L"(none)\r\n");
                for (const auto& key : selected->setting_keys) {
                    const bool secret = std::any_of(selected->settings.begin(), selected->settings.end(),
                        [&](const auto& field) { return field.key == key && field.type == "secret"; });
                    prompt += L"• " + widen(key) + (secret ? tr(L" (Geheimnis – wird an die Hosts gesendet)", L" (secret – sent to the hosts)") : L"") + L"\r\n";
                }
                prompt += tr(L"\r\nDiese Hosts freigeben und das Plugin aktivieren?", L"\r\nAllow these hosts and enable the plugin?");
                if (MessageBoxW(hwnd, prompt.c_str(), L"ScreenBuddy – Plugin", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
            }
            if (id == PluginConsent && MessageBoxW(hwnd,
                    tr(L"Plugin deaktivieren und alle zugehörigen Host-Freigaben widerrufen?", L"Disable this plugin and revoke all its host grants?"),
                    L"ScreenBuddy – Plugin", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        }
        action_pending = true; update_controls();
        enqueue([this, id, path = std::move(path), selected] {
            if (id == PluginImport && plugins.import_manifest) plugins.import_manifest(path);
            else if (id == PluginToggle && plugins.set_enabled) {
                if (!selected->enabled && plugins.set_host_consent)
                    for (const auto& host : selected->hosts)
                        if (std::find(selected->consented_hosts.begin(), selected->consented_hosts.end(), host) == selected->consented_hosts.end())
                            plugins.set_host_consent(selected->id, host, true);
                plugins.set_enabled(selected->id, !selected->enabled);
            } else if (id == PluginConsent && plugins.set_host_consent) {
                // Stop fetching before revoking grants, including on a partial failure.
                if (plugins.set_enabled) plugins.set_enabled(selected->id, false);
                for (const auto& host : selected->consented_hosts) plugins.set_host_consent(selected->id, host, false);
            }
            else if (id == PluginRemove && plugins.remove) plugins.remove(selected->id);
            Result result; result.kind = Result::PluginChanged;
            if (plugins.list) result.entries = plugins.list();
            deliver(std::move(result));
        });
    }
    void command(WPARAM wp) {
        int id = LOWORD(wp), event = HIWORD(wp);
        if (id == PluginList && event == LBN_SELCHANGE) { update_controls(); return; }
        if ((id == PageList || id == CompanionPages) && event == LBN_SELCHANGE) { update_controls(); return; }
        if (id == Source && event == CBN_SELCHANGE) {
            LRESULT next = SendMessageW(child(Source), CB_GETCURSEL, 0, 0);
            if (next >= 0 && next < int(std::size(Sources))) source = int(next);
            update_controls(); layout(); return;
        }
        if (id == Theme && event == CBN_SELCHANGE) {
            std::string theme = choice(Theme, Themes);
            previewing_theme = true;
            passive([this, theme] { Result result; result.frame = app.preview_theme({{"theme", theme}}); deliver(std::move(result)); });
            return;
        }
        if (event != BN_CLICKED) return;
        if (id == PluginConfigure) {
            if (action_pending || saving) return;
            auto index = SendMessageW(child(PluginList), LB_GETCURSEL, 0, 0);
            if (index < 0 || size_t(index) >= plugin_entries.size()) return;
            configure_plugin(plugin_entries[size_t(index)]);
            page = PluginSettingsPage; TabCtrl_SetCurSel(child(Tabs), page); update_controls(); return;
        }
        if (id == PluginSettingsSave) { save_plugin_settings(); return; }
        if (id == PageToggle || id == PageUp || id == PageDown || id == CompanionToggle || id == CompanionUp || id == CompanionDown) {
            bool companion = id == CompanionToggle || id == CompanionUp || id == CompanionDown;
            int list = companion ? CompanionPages : PageList;
            auto& items = companion ? companion_pages : display_pages;
            LRESULT selected = SendMessageW(child(list), LB_GETCURSEL, 0, 0);
            if (saving || selected < 0 || size_t(selected) >= items.size()) return;
            size_t index = size_t(selected);
            if (id == PageToggle || id == CompanionToggle) items[index].enabled = !items[index].enabled;
            else if ((id == PageUp || id == CompanionUp) && index > 0) { std::swap(items[index], items[index - 1]); --index; }
            else if ((id == PageDown || id == CompanionDown) && index + 1 < items.size()) { std::swap(items[index], items[index + 1]); ++index; }
            fill_pages(list, items, int(index)); update_controls(); return;
        }
        if (id == ApplyAutostart || id == CheckUpdate) { application_action(id); return; }
        if (id == PluginRefresh) { refresh_plugins(); return; }
        if (id == Advanced && plugins.open_advanced) { passive([this] { plugins.open_advanced(); }); return; }
        if (id == PluginImport || id == PluginToggle || id == PluginRemove || id == PluginConsent) { plugin_action(id); return; }
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
                if (result.application_snapshot) show_update(result.update);
                if (result.autostart && !application_loaded) {
                    SendMessageW(child(Autostart), BM_SETCHECK, *result.autostart ? BST_CHECKED : BST_UNCHECKED, 0);
                    application_loaded = true; update_controls();
                }
                layout(); InvalidateRect(child(Preview), nullptr, TRUE);
            } else if (result.kind == Result::ApplicationChanged) {
                action_pending = false;
                if (result.autostart) SendMessageW(child(Autostart), BM_SETCHECK, *result.autostart ? BST_CHECKED : BST_UNCHECKED, 0);
                if (!result.status.empty()) SetWindowTextW(child(Status), result.status.c_str());
                if (!result.update_status.empty()) SetWindowTextW(child(UpdateStatus), result.update_status.c_str());
                update_controls();
            } else if (result.kind == Result::Saved) {
                SetWindowTextW(child(Status), tr(L"Gespeichert. ScreenBuddy startet neu …", L"Saved. ScreenBuddy is restarting …"));
            } else if (result.kind == Result::PluginChanged) {
                action_pending = false; plugin_entries = std::move(result.entries);
                SendMessageW(child(PluginList), LB_RESETCONTENT, 0, 0);
                for (const auto& entry : plugin_entries) {
                    std::wstring title = entry.enabled ? L"[✓] " : L"[ ] ";
                    title += widen(entry.name + "  " + entry.version + "  (" + entry.id + ")");
                    if (!entry.hosts.empty()) {
                        size_t granted = 0;
                        for (const auto& host : entry.hosts)
                            if (std::find(entry.consented_hosts.begin(), entry.consented_hosts.end(), host) != entry.consented_hosts.end()) ++granted;
                        title += tr(L" · Hosts freigegeben: ", L" · Hosts allowed: ") + std::to_wstring(granted) + L"/" + std::to_wstring(entry.hosts.size());
                    }
                    SendMessageW(child(PluginList), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
                }
                if (!result.status.empty()) {
                    auto entry = std::find_if(plugin_entries.begin(), plugin_entries.end(), [this](const auto& e) { return e.id == editing_plugin; });
                    if (entry != plugin_entries.end()) configure_plugin(*entry);
                    SetWindowTextW(child(PluginSettingsNote), result.status.c_str());
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
                    if (self->page == PluginsPage) self->refresh_plugins();
                }
                return 0;
            case WM_TIMER: self->poll(); return 0;
            case Results: self->consume(); return 0;
            case WM_DRAWITEM: self->paint(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
            case WM_GETMINMAXINFO:
                reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {self->px(960), self->px(700)}; return 0;
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
