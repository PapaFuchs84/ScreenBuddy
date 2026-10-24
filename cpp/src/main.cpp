// © 2026 FuchsLabs
// FuchsLabs USB-Display 2.0: zeigt System, Freunde (Discord/Steam/Xbox), Battlefield-6-Rangliste und Blog-Updates
// im Wechsel auf dem USB-Display. Einstellungen: Fuchs-Symbol im Infobereich → Einstellungsseite.
// Dieselbe EXE ist auch Installer, Updater und Deinstaller.
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <filesystem>
#include <fstream>

#include "app.h"
#include "config.h"
#include "gfx.h"
#include "i18n.h"
#include "installer.h"
#include "output.h"
#include "render.h"
#include "sysinfo.h"
#include "util.h"
#include "winsetup.h"

namespace fs = std::filesystem;
using namespace fl;

namespace {

struct Args {
    std::optional<fs::path> config, preview, log;
    bool reopen_settings = false;  // --settings: Einstellungsfenster gleich öffnen (nach Neustart aus dem Fenster)
    bool demo = false, no_lcd = false, restarted = false, autostart = false, verbose = false;
    bool install = false, silent = false, uninstall = false, run = false;
    std::optional<std::wstring> finish_dir;
    unsigned long finish_pid = 0;
};

Args parse(int argc, wchar_t** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::wstring s = argv[i];
        auto next = [&]() -> std::wstring { return i + 1 < argc ? argv[++i] : L""; };
        if (s == L"-c" || s == L"--config") a.config = next();
        else if (s == L"--demo") a.demo = true;
        else if (s == L"--no-lcd") a.no_lcd = true;
        else if (s == L"--preview") a.preview = next();
        else if (s == L"--log") a.log = next();
        else if (s == L"--restarted") a.restarted = true;
        else if (s == L"--settings") a.reopen_settings = true;
        else if (s == widen(winsetup::AUTOSTART_FLAG)) a.autostart = true;
        else if (s == L"-v" || s == L"--verbose") a.verbose = true;
        else if (s == L"--install") a.install = true;
        else if (s == L"--silent") a.silent = true;
        else if (s == L"--uninstall") a.uninstall = true;
        else if (s == L"--run") a.run = true;
        else if (s == L"--uninstall-finish") {
            a.finish_dir = next();
            a.finish_pid = std::wcstoul(next().c_str(), nullptr, 10);
        }
    }
    return a;
}

fs::path data_dir() {
    // Liegt das Programm in einem Ordner ohne Schreibrecht, wandern Einstellungen und Protokoll ins Benutzerprofil
    fs::path here = exe_path().parent_path();
    fs::path probe = here / L".fl-write-test";
    HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return here;
    }
    return install_dir();
}

void open_settings(int port) {
    std::wstring host = L"127.0.0.1";
    std::wstring path = L"/api/open-window";
    INTERNET_PORT p = static_cast<INTERNET_PORT>(port);
    HINTERNET session = WinHttpOpen(L"FuchsLabs-USB-Display/2.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
    if (!session) return;
    WinHttpSetTimeouts(session, 300, 300, 500, 500);
    for (int attempt = 0; attempt < 12; ++attempt) {
        HINTERNET connection = WinHttpConnect(session, host.c_str(), p, 0);
        if (connection) {
            HINTERNET request = WinHttpOpenRequest(connection, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            if (request) {
                const wchar_t* headers = L"X-FL: 1\r\n";
                BOOL sent = WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                if (sent && WinHttpReceiveResponse(request, nullptr)) {
                    WinHttpCloseHandle(request);
                    WinHttpCloseHandle(connection);
                    break;
                }
                WinHttpCloseHandle(request);
            }
            WinHttpCloseHandle(connection);
        }
        Sleep(150);
    }
    WinHttpCloseHandle(session);
}

HANDLE single_instance(bool wait) {
    // Nur eine laufende App. Rückgabe: Mutex-Handle oder nullptr, wenn schon eine läuft
    for (int i = 0; i < (wait ? 40 : 1); ++i) {
        HANDLE h = CreateMutexW(nullptr, FALSE, mutex_name().c_str());
        if (GetLastError() != ERROR_ALREADY_EXISTS) return h;
        CloseHandle(h);
        Sleep(500);
    }
    return nullptr;
}

void relaunch(int argc, wchar_t** argv, bool reopen_settings) {
    std::wstring cmd = L"\"" + exe_path().wstring() + L"\"";
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--restarted") == 0 || wcscmp(argv[i], L"--settings") == 0) continue;
        cmd += L" \"" + std::wstring(argv[i]) + L"\"";
    }
    cmd += L" --restarted";
    if (reopen_settings) cmd += L" --settings";
    STARTUPINFOW si{sizeof si};
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

Orientation orientation(const std::string& s) {
    if (s == "portrait") return Orientation::Portrait;
    if (s == "reverse_portrait") return Orientation::ReversePortrait;
    if (s == "reverse_landscape") return Orientation::ReverseLandscape;
    return Orientation::Landscape;
}

void save_png(const fs::path& p, const Image& img) {
    std::ofstream f(p, std::ios::binary);
    std::string png = encode_png(img);
    f.write(png.data(), (std::streamsize)png.size());
}

void run_demo(Output* lcd, const ojson& cfg, const std::optional<fs::path>& preview) {
    auto [w, h] = lcd ? lcd->size() : std::make_pair(480, 320);
    sysinfo::SysMonitor mon(cfg["system"]);
    mon.sample();
    Sleep(1000);
    mon.sample();  // zweite Messung, damit Netzwerkraten vorhanden sind
    std::lock_guard rl(render::mutex());
    render::apply_theme(cfg["display"]);
    render::Renderer r(w, h, cfg["display"].value("title", "Discord-Freunde"));
    render::Bf6Renderer br(w, h, cfg["bf6"].value("title", "Battlefield 6 · Rangliste"));
    render::RssRenderer rr(w, h, cfg["rss"].value("title", "Blog-Updates"));
    render::SysRenderer sr(w, h, cfg["system"].value("title", "System"), cfg["system"].value("net_unit", "bit"));
    std::time_t now = std::time(nullptr);
    std::vector<Post> posts = {{tr("Beispiel-Blog"), tr("Ein kurzer Titel für die Vorschau"), now, ""},
                               {tr("Noch ein Blog"), tr("Zweiter Beitrag mit etwas längerem Titel, der umbricht"), now, ""}};
    std::vector<Image> frames = {sr.render(mon.raw())};
    auto friends = render::demo_friends();
    auto players = render::demo_players();
    for (int p = 0; p < r.pages(friends.size()); ++p) frames.push_back(r.render_friends(friends, p));
    for (int p = 0; p < br.pages(players.size()); ++p) frames.push_back(br.render(players, p, now));
    frames.push_back(rr.render(posts, now));
    if (lcd) lcd->connect();
    double secs = cfg["display"].value("page_seconds", 15);
    for (size_t i = 0; i < frames.size(); ++i) {
        if (preview) save_png(*preview / ("demo-" + std::to_string(i + 1) + ".png"), frames[i]);
        if (lcd) {
            lcd->show(frames[i]);
            Sleep((DWORD)(secs * 1000));
        }
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    Args args = parse(argc, argv);
    dpi_aware();  // vor dem ersten Fenster: Bildschirmgrößen in echten Pixeln
    gfx_init();

    fs::path data = data_dir();
    if (args.log) log_init(*args.log, args.verbose);
    else log_init((args.install || args.uninstall || args.finish_dir ? install_dir() : data) / L"usb-display.log", args.verbose);
    // Sprache für Installer-Fenster: wie Windows (Deutsch, sonst Englisch)
    i18n::set_lang((GetUserDefaultUILanguage() & 0x3FF) == 0x07 ? "de" : "en");

    // ---------- Installer / Deinstaller ----------
    if (args.finish_dir) return installer::finish_uninstall(*args.finish_dir, args.finish_pid);
    if (args.uninstall) return installer::run_uninstall(args.silent);
    bool from_elsewhere = !installer::running_installed() && !args.run && !args.demo && !args.config && !test_instance();
    if (args.install || from_elsewhere) {
        std::error_code ec;
        fs::create_directories(install_dir(), ec);
        log_init(install_dir() / L"usb-display.log", args.verbose);
        return installer::run_setup(args.silent, args.no_lcd);
    }

    // ---------- App ----------
    fs::path cfg_path = args.config ? *args.config : data / L"config.toml";
    std::error_code ec;
    bool first_run = !fs::exists(cfg_path, ec);
    ojson cfg;
    try {
        cfg = config::load(cfg_path);
    } catch (const std::exception& e) {
        LOGE("app", "Konfiguration %s nicht lesbar: %s", u8(cfg_path).c_str(), e.what());
        std::wstring msg = widen("config.toml ist fehlerhaft:\n" + std::string(e.what()));
        MessageBoxW(nullptr, msg.c_str(), L"FuchsLabs USB-Display", MB_ICONERROR);
        return 1;
    }
    if (first_run) cfg["ui"]["language"] = (GetUserDefaultUILanguage() & 0x3FF) == 0x07 ? "de" : "en";
    i18n::set_lang(cfg["ui"].value("language", "de"));
    int port = cfg["ui"].value("port", 8765);
    LOGI("app", "FuchsLabs USB-Display %s startet (%s)", VERSION, u8(cfg_path).c_str());

    HANDLE mutex = nullptr;
    if (!args.demo) {
        mutex = single_instance(args.restarted);
        if (!mutex) {  // läuft schon: nur die Einstellungen öffnen (beim Autostart still beenden)
            if (!args.autostart) open_settings(port);
            return 0;
        }
        if (installer::running_installed() && !test_instance()) winsetup::upgrade_autostart();
        try {
            if (config::token_is_plaintext(cfg_path)) {
                config::save(cfg_path, cfg);  // alten Klartext-Token verschlüsseln
                LOGI("app", "Zugangsdaten werden jetzt verschlüsselt gespeichert");
            }
        } catch (const std::exception& e) {
            LOGW("app", "Verschlüsseln fehlgeschlagen: %s", e.what());
        }
    }

    const ojson& dp = cfg["display"];
    if (args.preview) fs::create_directories(*args.preview, ec);
    std::unique_ptr<Output> lcd;
    if (!args.no_lcd) {
        if (dp.value("output", "serial") == "monitor")  // HDMI/USB-C-Bildschirm: Vollbildfenster statt serieller Übertragung
            lcd = std::make_unique<MonitorDisplay>(dp.value("monitor", ""), dp.value("touch", false));
        else if (dp.value("output", "serial") == "network")  // Startfläche; das Handy meldet seine native Größe nach der Anmeldung
            lcd = std::make_unique<NetworkDisplay>(cfg["companion"].value("port", 8777), port, cfg_path.parent_path() / L"companion.json",
                                                   480, 320);
        else
            lcd = std::make_unique<SerialDisplay>(dp.value("port", "AUTO"), orientation(dp.value("orientation", "landscape")),
                                                  dp.value("brightness", 50), dp.value("reset_on_start", true));
    }
    bool restart = false, reopen = false;
    int rc = 0;
    try {
        if (args.demo) {
            run_demo(lcd.get(), cfg, args.preview);
        } else {
            bool setup_done = cfg["ui"].value("setup_done", false);
            bool show_settings = ((first_run || !setup_done) && !(args.autostart || args.restarted)) || args.reopen_settings;
            App app(cfg, cfg_path, std::move(lcd), args.preview, show_settings);
            app.run();
            restart = app.restart_requested();
            reopen = app.settings_open_at_exit();
            lcd = nullptr;
        }
    } catch (const std::exception& e) {
        LOGE("app", "Abbruch: %s", e.what());
        rc = 1;
    }
    if (restart) {
        if (mutex) CloseHandle(mutex);
        mutex = nullptr;
        relaunch(argc, argv, reopen);
    }
    if (mutex) CloseHandle(mutex);
    LOGI("app", "beendet%s", restart ? " (Neustart)" : "");
    return rc;
}
