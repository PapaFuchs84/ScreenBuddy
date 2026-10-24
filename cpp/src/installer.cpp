// © 2026 FuchsLabs
#include "installer.h"

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <tlhelp32.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>

#include "brand.h"
#include "config.h"
#include "gfx.h"
#include "http.h"
#include "i18n.h"
#include "resources.h"
#include "tray.h"
#include "util.h"
#include "winsetup.h"

namespace fs = std::filesystem;

namespace fl::installer {

namespace {

const wchar_t* EXE_NAME = L"FuchsLabs-USB-Display.exe";
const wchar_t* UNINSTALL_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\FuchsLabsUsbDisplay";
const wchar_t* SHORTCUT = L"FuchsLabs USB-Display.lnk";

fs::path installed_exe() { return install_dir() / EXE_NAME; }

fs::path start_menu_dir() {
    PWSTR p = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

bool same_file(const fs::path& a, const fs::path& b) {
    std::error_code ec;
    return fs::equivalent(a, b, ec);
}

int config_port(const fs::path& cfg) {
    try {
        std::error_code ec;
        if (fs::exists(cfg, ec)) return config::load(cfg)["ui"].value("port", 8765);
    } catch (...) {
    }
    return 8765;
}

bool app_running() {
    HANDLE h = OpenMutexW(SYNCHRONIZE, FALSE, mutex_name().c_str());
    if (!h) return false;
    CloseHandle(h);
    return true;
}

void kill_by_name(const wchar_t* name) {
    DWORD self = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe{sizeof pe};
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (pe.th32ProcessID == self || _wcsicmp(pe.szExeFile, name) != 0) continue;
        if (HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID)) {
            TerminateProcess(h, 0);
            WaitForSingleObject(h, 5000);
            CloseHandle(h);
        }
    }
    CloseHandle(snap);
}

bool make_shortcut(const fs::path& lnk, const fs::path& target) {
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&sl)))) return false;
    sl->SetPath(target.c_str());
    sl->SetWorkingDirectory(target.parent_path().c_str());
    sl->SetDescription(L"FuchsLabs USB-Display");
    sl->SetIconLocation(target.c_str(), 0);
    IPersistFile* pf = nullptr;
    bool ok = false;
    if (SUCCEEDED(sl->QueryInterface(IID_PPV_ARGS(&pf)))) {
        ok = SUCCEEDED(pf->Save(lnk.c_str(), TRUE));
        pf->Release();
    }
    sl->Release();
    return ok;
}

void reg_set(HKEY k, const wchar_t* name, const std::wstring& v) {
    RegSetValueExW(k, name, 0, REG_SZ, (const BYTE*)v.c_str(), (DWORD)((v.size() + 1) * sizeof(wchar_t)));
}
void reg_set(HKEY k, const wchar_t* name, DWORD v) { RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE*)&v, sizeof v); }

void write_uninstall_entry(const fs::path& exe) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, UNINSTALL_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    std::error_code ec;
    DWORD kb = (DWORD)(fs::file_size(exe, ec) / 1024);
    reg_set(k, L"DisplayName", L"FuchsLabs USB-Display");
    reg_set(k, L"DisplayVersion", widen(VERSION));
    reg_set(k, L"Publisher", L"FuchsLabs");
    reg_set(k, L"DisplayIcon", L"\"" + exe.wstring() + L"\",0");
    reg_set(k, L"InstallLocation", exe.parent_path().wstring());
    reg_set(k, L"UninstallString", L"\"" + exe.wstring() + L"\" --uninstall");
    reg_set(k, L"QuietUninstallString", L"\"" + exe.wstring() + L"\" --uninstall --silent");
    reg_set(k, L"EstimatedSize", kb);
    reg_set(k, L"NoModify", 1);
    reg_set(k, L"NoRepair", 1);
    RegCloseKey(k);
}

bool launch(const fs::path& exe, const std::wstring& args = L"") {
    std::wstring cmd = L"\"" + exe.wstring() + L"\"" + (args.empty() ? L"" : L" " + args);
    STARTUPINFOW si{sizeof si};
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr,
                        exe.parent_path().c_str(), &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

// ---------- Installation ----------
struct Options {
    bool autostart = true, shortcut = true, retire_old = true;
};

struct Result {
    bool ok = false;
    std::string error;
};

bool legacy_present() {
    std::error_code ec;
    return fs::exists(legacy_dir() / EXE_NAME, ec) && !same_file(legacy_dir(), install_dir());
}

Result install(const Options& o, const std::function<void(const std::string&)>& progress) {
    Result r;
    std::error_code ec;
    fs::path dir = install_dir(), exe = installed_exe(), self = exe_path();
    fs::path cfg = dir / L"config.toml";
    // Einstellungen von Version 1.x übernehmen (dort lagen sie neben der EXE oder in %APPDATA%)
    fs::path old_cfg = legacy_dir() / L"config.toml";
    if (!fs::exists(old_cfg, ec) && !test_instance()) {
        PWSTR p = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p))) old_cfg = fs::path(p) / L"FuchsLabs" / L"USB-Display" / L"config.toml";
        CoTaskMemFree(p);
    }
    auto prefer_newer_config = [&](const fs::path& candidate) {
        std::error_code exists_ec, current_ec, candidate_ec;
        if (candidate == cfg || !fs::exists(candidate, exists_ec)) return;
        auto current_time = fs::last_write_time(old_cfg, current_ec);
        auto candidate_time = fs::last_write_time(candidate, candidate_ec);
        if (!fs::exists(old_cfg, exists_ec) || (!candidate_ec && (current_ec || candidate_time > current_time)))
            old_cfg = candidate;
    };
    // Recover the newest usable local copy if the target config is missing.
    fs::path config_backup = cfg;
    config_backup += L".bak";
    prefer_newer_config(config_backup);
    // A portable EXE can keep its settings beside the downloaded file.
    fs::path portable_cfg = self.parent_path() / L"config.toml";
    if (!same_file(self, exe)) prefer_newer_config(portable_cfg);
    progress(tr("Laufende App wird beendet …"));
    int port = fs::exists(cfg, ec) ? config_port(cfg) : config_port(old_cfg);
    if (!stop_running_app(port)) {
        r.error = tr("Die laufende App ließ sich nicht beenden. Bitte über das Fuchs-Symbol beenden und erneut versuchen.");
        return r;
    }
    progress(tr("Dateien werden kopiert …"));
    fs::create_directories(dir, ec);
    if (ec) {
        r.error = tr("Installationsordner konnte nicht angelegt werden");
        return r;
    }
    if (!fs::exists(cfg, ec) && fs::exists(old_cfg, ec) && old_cfg != cfg) {
        fs::copy_file(old_cfg, cfg, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            r.error = tr("Vorhandene Einstellungen konnten nicht übernommen werden");
            return r;
        }
        LOGI("setup", "Vorhandene Einstellungen übernommen");
    }
    if (fs::exists(cfg, ec)) {
        fs::path backup = cfg;
        backup += L".install-backup";
        try {
            if (config::token_is_plaintext(cfg)) {
                // A plaintext token must never be duplicated into a backup. Serialize an encrypted snapshot instead.
                config::save(backup, config::load(cfg));
            } else {
                fs::copy_file(cfg, backup, fs::copy_options::overwrite_existing, ec);
                if (ec) throw std::runtime_error("copy failed");
            }
        } catch (...) {
            r.error = tr("Einstellungen konnten vor der Installation nicht gesichert werden");
            return r;
        }
        LOGI("setup", "Vorhandene Einstellungen vor der Installation gesichert");
    }
    if (!same_file(self, exe)) {
        if (fs::exists(exe, ec)) {
            fs::path old = exe;
            old += L".old";
            fs::remove(old, ec);
            fs::rename(exe, old, ec);  // falls die alte EXE noch gesperrt ist
        }
        if (!CopyFileW(self.c_str(), exe.c_str(), FALSE)) {
            r.error = tr("Kopieren fehlgeschlagen") + strprintf(" (%lu)", GetLastError());
            return r;
        }
    }
    // Lizenz und Drittanbieter-Hinweise neben die EXE legen (aus der EXE selbst, keine Zusatzdateien im Download)
    for (auto [name, file] : {std::pair{res::LICENSE, L"LICENSE.txt"}, std::pair{res::NOTICES, L"THIRD-PARTY-NOTICES.md"}}) {
        auto data = res::get(name);
        if (data.empty()) continue;
        std::ofstream f(dir / file, std::ios::binary | std::ios::trunc);
        f.write(data.data(), (std::streamsize)data.size());
    }
    progress(tr("Verknüpfungen werden angelegt …"));
    if (!test_instance()) {
        write_uninstall_entry(exe);
        fs::path lnk = start_menu_dir() / SHORTCUT;
        if (o.shortcut) make_shortcut(lnk, exe);
        else fs::remove(lnk, ec);
    }
    winsetup::set_autostart_for(exe.wstring(), o.autostart);
    if (o.retire_old && legacy_present()) {
        // Alte Version nicht löschen, sondern umbenennen: lässt sich jederzeit zurückholen
        fs::path target = legacy_dir();
        target += L"-1.x-alt";
        for (int i = 2; fs::exists(target, ec); ++i) {
            target = legacy_dir();
            target += L"-1.x-alt-" + std::to_wstring(i);
        }
        fs::rename(legacy_dir(), target, ec);
        if (ec) LOGW("setup", "Alte Version nicht umbenannt: %s", ec.message().c_str());
        else LOGI("setup", "Alte Version umbenannt nach %s", u8(target).c_str());
    }
    r.ok = true;
    return r;
}

// ---------- Fenster ----------
struct Ui {
    HWND hwnd = nullptr;
    float s = 1;
    Options opt;
    bool update = false, legacy = false;
    enum { Ask, Working, Done, Failed } state = Ask;
    std::string status;
    std::atomic<bool> busy{false};
    int hover = -1;
    struct Hit {
        float x0, y0, x1, y1;
        int id;
    };
    std::vector<Hit> hits;
};

constexpr float W = 500, H = 330;
constexpr UINT WM_PROGRESS = WM_APP + 10, WM_FINISHED = WM_APP + 11;
enum { ID_AUTOSTART = 1, ID_SHORTCUT = 2, ID_RETIRE = 3, ID_INSTALL = 10, ID_CANCEL = 11 };

Image paint(Ui& ui) {
    int pw = (int)std::lround(W * ui.s), ph = (int)std::lround(H * ui.s);
    Canvas d(pw, ph, ui.s, brand::CREME);
    auto F = [&](float size, bool bold = false) { return Font{std::round(size * ui.s), bold}; };
    ui.hits.clear();
    d.rectangle(0, 0, W, 79, brand::ORANGE);
    d.fox(24, 14, (int)std::lround(52 * ui.s), false);
    d.text(90, 32, "FuchsLabs USB-Display", F(24, true), brand::NAVY, "lm");
    d.text(90, 58, tr(ui.update ? "Update auf Version " : "Installation · Version ") + VERSION, F(13), brand::NAVY, "lm");
    float y = 104;
    auto line = [&](const std::string& t, Color c = brand::NAVY, float size = 13, bool bold = false) {
        d.text(24, y, t, F(size, bold), c, "lm");
        y += 22;
    };
    if (ui.state == Ui::Ask) {
        line(tr("Die App wird für dein Benutzerkonto installiert (keine Administratorrechte nötig):"));
        line(u8(install_dir()), brand::NEBEL, 12);
        y += 6;
        auto check = [&](int id, bool on, const std::string& label) {
            float x = 24;
            d.rounded_rectangle(x, y - 9, x + 18, y + 9, 4, ui.hover == id ? brand::GLUT : brand::NAVY);
            d.rounded_rectangle(x + 2, y - 7, x + 16, y + 7, 3, on ? brand::ORANGE : brand::CREME);
            if (on) d.line({{x + 5, y}, {x + 8, y + 4}, {x + 14, y - 4}}, brand::NAVY, 2);
            d.text(x + 28, y, label, F(13), brand::NAVY, "lm");
            ui.hits.push_back({x - 4, y - 12, x + 32 + d.textlength(label, F(13)), y + 12, id});
            y += 28;
        };
        check(ID_AUTOSTART, ui.opt.autostart, tr("Mit Windows starten (nur das Fuchs-Symbol im Infobereich)"));
        check(ID_SHORTCUT, ui.opt.shortcut, tr("Eintrag im Startmenü"));
        if (ui.legacy)
            check(ID_RETIRE, ui.opt.retire_old, tr("Alte Version 1.x stilllegen (Einstellungen werden übernommen)"));
    } else {
        y = 130;
        Color c = ui.state == Ui::Failed ? Color{215, 45, 50} : brand::NAVY;
        // Meldung bei Bedarf umbrechen
        std::string cur;
        for (auto& w : split(ui.status, " ")) {
            std::string test = trim(cur + " " + w);
            if (d.textlength(test, F(15, true)) > W - 48 && !cur.empty()) {
                line(cur, c, 15, true);
                cur = w;
            } else {
                cur = test;
            }
        }
        if (!cur.empty()) line(cur, c, 15, true);
    }
    // Knöpfe
    auto button = [&](int id, float x0, float x1, const std::string& label, bool primary) {
        float y0 = H - 58, y1 = H - 22;
        bool hov = ui.hover == id;
        if (primary) {
            d.rounded_rectangle(x0, y0, x1, y1, 8, hov ? brand::GLUT : brand::ORANGE);
            d.text((x0 + x1) / 2, (y0 + y1) / 2, label, F(15, true), hov ? brand::CREME : brand::NAVY, "mm");
        } else {
            d.rounded_rectangle(x0, y0, x1, y1, 8, hov ? brand::GLUT : brand::NAVY);
            d.rounded_rectangle(x0 + 2, y0 + 2, x1 - 2, y1 - 2, 6, brand::CREME);
            d.text((x0 + x1) / 2, (y0 + y1) / 2, label, F(15, true), brand::NAVY, "mm");
        }
        ui.hits.push_back({x0, y0, x1, y1, id});
    };
    if (ui.state == Ui::Ask) {
        button(ID_CANCEL, W - 290, W - 170, tr("Abbrechen"), false);
        button(ID_INSTALL, W - 154, W - 24, tr(ui.update ? "Aktualisieren" : "Installieren"), true);
    } else if (ui.state == Ui::Failed) {
        button(ID_CANCEL, W - 154, W - 24, tr("Schließen"), false);
    }
    d.text(24, H - 40, "© 2026 FuchsLabs", F(11), brand::NEBEL, "lm");
    return d.finish();
}

Ui* g_ui = nullptr;

LRESULT CALLBACK setup_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Ui& ui = *g_ui;
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Image img = paint(ui);
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof bmi.bmiHeader;
            bmi.bmiHeader.biWidth = img.w;
            bmi.bmiHeader.biHeight = -img.h;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            SetDIBitsToDevice(hdc, 0, 0, img.w, img.h, 0, 0, 0, img.h, img.px.data(), &bmi, DIB_RGB_COLORS);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_MOUSEMOVE: {
            float x = (short)LOWORD(lp) / ui.s, y = (short)HIWORD(lp) / ui.s;
            int h = -1;
            for (auto& t : ui.hits)
                if (x >= t.x0 && x <= t.x1 && y >= t.y0 && y <= t.y1) h = t.id;
            if (h != ui.hover) {
                ui.hover = h;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            SetCursor(LoadCursorW(nullptr, h >= 0 ? IDC_HAND : IDC_ARROW));
            TRACKMOUSEEVENT tme{sizeof tme, TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE: ui.hover = -1; InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT) return TRUE;
            break;
        case WM_LBUTTONUP: {
            if (ui.busy) return 0;
            switch (ui.hover) {
                case ID_AUTOSTART: ui.opt.autostart = !ui.opt.autostart; break;
                case ID_SHORTCUT: ui.opt.shortcut = !ui.opt.shortcut; break;
                case ID_RETIRE: ui.opt.retire_old = !ui.opt.retire_old; break;
                case ID_CANCEL: DestroyWindow(hwnd); return 0;
                case ID_INSTALL: {
                    ui.state = Ui::Working;
                    ui.busy = true;
                    ui.status = tr("Wird installiert …");
                    Options o = ui.opt;
                    std::thread([hwnd, o] {
                        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                        auto res = install(o, [hwnd](const std::string& s) {
                            PostMessageW(hwnd, WM_PROGRESS, 0, (LPARAM) new std::string(s));
                        });
                        PostMessageW(hwnd, WM_FINISHED, res.ok, (LPARAM) new std::string(res.error));
                        CoUninitialize();
                    }).detach();
                    break;
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE && !ui.busy) DestroyWindow(hwnd);
            if (wp == VK_RETURN && ui.state == Ui::Ask) {
                ui.hover = ID_INSTALL;
                SendMessageW(hwnd, WM_LBUTTONUP, 0, 0);
            }
            return 0;
        case WM_PROGRESS: {
            std::unique_ptr<std::string> s((std::string*)lp);
            ui.status = *s;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_FINISHED: {
            std::unique_ptr<std::string> err((std::string*)lp);
            ui.busy = false;
            if (wp) {
                ui.state = Ui::Done;
                ui.status = tr("Fertig! Die App startet jetzt.");
                InvalidateRect(hwnd, nullptr, FALSE);
                UpdateWindow(hwnd);
                launch(installed_exe());
                Sleep(1200);
                DestroyWindow(hwnd);
            } else {
                ui.state = Ui::Failed;
                ui.status = *err;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_CLOSE:
            if (!ui.busy) DestroyWindow(hwnd);
            return 0;
        case WM_DPICHANGED: {
            ui.s = HIWORD(wp) / 96.f;
            auto* r = (RECT*)lp;
            SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int show_setup_window() {
    Ui ui;
    g_ui = &ui;
    std::error_code ec;
    ui.update = fs::exists(installed_exe(), ec) || legacy_present();
    ui.legacy = legacy_present();
    HINSTANCE hinst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = setup_proc;
    wc.hInstance = hinst;
    wc.lpszClassName = L"FuchsLabsUsbDisplaySetup";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)make_hicon(32);
    RegisterClassW(&wc);
    DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    UINT dpi = GetDpiForSystem();
    ui.s = dpi / 96.f;
    RECT r{0, 0, (LONG)std::lround(W * ui.s), (LONG)std::lround(H * ui.s)};
    AdjustWindowRectExForDpi(&r, style, FALSE, 0, dpi);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    std::wstring title = widen("FuchsLabs USB-Display – " + tr(ui.update ? "Update" : "Installation"));
    ui.hwnd = CreateWindowExW(0, wc.lpszClassName, title.c_str(), style, work.left + (work.right - work.left - ww) / 2,
                              work.top + (work.bottom - work.top - wh) / 2, ww, wh, nullptr, nullptr, hinst, nullptr);
    if (!ui.hwnd) return 1;
    SendMessageW(ui.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)make_hicon(GetSystemMetrics(SM_CXSMICON)));
    // Größe an die DPI des tatsächlichen Bildschirms anpassen
    UINT wdpi = GetDpiForWindow(ui.hwnd);
    if (wdpi != dpi) {
        ui.s = wdpi / 96.f;
        RECT r2{0, 0, (LONG)std::lround(W * ui.s), (LONG)std::lround(H * ui.s)};
        AdjustWindowRectExForDpi(&r2, style, FALSE, 0, wdpi);
        SetWindowPos(ui.hwnd, nullptr, 0, 0, r2.right - r2.left, r2.bottom - r2.top, SWP_NOMOVE | SWP_NOZORDER);
    }
    ShowWindow(ui.hwnd, SW_SHOW);
    SetForegroundWindow(ui.hwnd);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    g_ui = nullptr;
    return ui.state == Ui::Done ? 0 : 1;
}

}  // namespace

bool running_installed() { return same_file(exe_path(), installed_exe()); }

std::string preview_png(bool update, bool legacy, int state, const std::string& status, float scale) {
    Ui ui;
    ui.update = update;
    ui.legacy = legacy;
    ui.state = (decltype(ui.state))state;
    ui.status = status;
    ui.s = scale;
    ui.hover = state == 0 ? ID_INSTALL : -1;
    return encode_png(paint(ui));
}

bool stop_running_app(int port) {
    if (!app_running()) return true;
    // Freundlich bitten (wie die Einstellungsseite), auch Version 1.x versteht das
    http::Request r;
    r.method = "POST";
    r.url = "http://127.0.0.1:" + std::to_string(port) + "/api/quit";
    r.headers = {{"X-FL", "1"}, {"Content-Type", "application/json"}};
    r.body = "{}";
    r.timeout_ms = 5000;
    http::fetch(r);
    for (int i = 0; i < 60 && app_running(); ++i) Sleep(250);
    if (!app_running()) {
        Sleep(500);  // Prozess beendet sich nach dem Freigeben des Mutex
        return true;
    }
    if (test_instance()) return false;  // im Test nie fremde Prozesse beenden
    kill_by_name(EXE_NAME);
    for (int i = 0; i < 20 && app_running(); ++i) Sleep(250);
    return !app_running();
}

int run_setup(bool silent, bool no_lcd) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (silent) {
        auto res = install(Options{}, [](const std::string& s) { LOGI("setup", "%s", s.c_str()); });
        if (res.ok) launch(installed_exe(), no_lcd ? L"--no-lcd" : L"");
        else LOGE("setup", "%s", res.error.c_str());
        return res.ok ? 0 : 1;
    }
    return show_setup_window();
}

int run_uninstall(bool silent) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool purge = false;
    if (!silent) {
        std::wstring title = L"FuchsLabs USB-Display";
        std::wstring main = widen(tr("FuchsLabs USB-Display entfernen?"));
        std::wstring content = widen(tr("Die App wird beendet und vom PC entfernt."));
        std::wstring verify = widen(tr("Auch Einstellungen und Zugangsdaten löschen"));
        TASKDIALOGCONFIG tc{sizeof tc};
        tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
        tc.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
        tc.pszWindowTitle = title.c_str();
        tc.pszMainInstruction = main.c_str();
        tc.pszContent = content.c_str();
        tc.pszVerificationText = verify.c_str();
        tc.nDefaultButton = IDNO;
        tc.hMainIcon = (HICON)make_hicon(32);
        tc.dwFlags |= TDF_USE_HICON_MAIN;
        int button = 0;
        BOOL verified = FALSE;
        if (FAILED(TaskDialogIndirect(&tc, &button, nullptr, &verified)) || button != IDYES) return 1;
        purge = verified;
    }
    fs::path dir = install_dir();
    stop_running_app(config_port(dir / L"config.toml"));
    std::error_code ec;
    if (!test_instance()) {
        winsetup::set_autostart_for(installed_exe().wstring(), false);
        fs::remove(start_menu_dir() / SHORTCUT, ec);
        RegDeleteTreeW(HKEY_CURRENT_USER, UNINSTALL_KEY);
    }
    // Die laufende EXE kann sich nicht selbst löschen: Kopie im Temp-Ordner räumt nach dem Beenden auf
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    fs::path helper = fs::path(tmp) / (L"fuchslabs-uninstall-" + std::to_wstring(GetCurrentProcessId()) + L".exe");
    if (!CopyFileW(exe_path().c_str(), helper.c_str(), FALSE)) return 1;
    std::wstring args = L"--uninstall-finish \"" + dir.wstring() + L"\" " + std::to_wstring(GetCurrentProcessId()) +
                        (purge ? L" --purge" : L"");
    return launch(helper, args) ? 0 : 1;
}

int finish_uninstall(const std::wstring& dir_s, unsigned long parent_pid) {
    if (HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, parent_pid)) {
        WaitForSingleObject(h, 15000);
        CloseHandle(h);
    }
    bool purge = false;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 0; i < argc; ++i) purge |= wcscmp(argv[i], L"--purge") == 0;
    LocalFree(argv);
    fs::path dir = dir_s;
    std::error_code ec;
    // Sicherheitsnetz: nur den eigenen Ordner leeren
    if (dir.filename() == L"USB-Display" || test_instance()) {
        for (int i = 0; i < 20; ++i) {
            fs::remove(dir / EXE_NAME, ec);
            if (!fs::exists(dir / EXE_NAME, ec)) break;
            Sleep(250);
        }
        fs::path old = dir / EXE_NAME;
        old += L".old";
        fs::remove(old, ec);
        fs::remove(dir / L"LICENSE.txt", ec);
        fs::remove(dir / L"THIRD-PARTY-NOTICES.md", ec);
        if (purge) {
            for (auto* f : {L"config.toml", L"config.toml.bak", L"config.toml.tmp", L"usb-display.log", L"usb-display.log.1"})
                fs::remove(dir / f, ec);
        }
        fs::remove(dir, ec);  // nur wenn leer
    }
    // sich selbst löschen, sobald dieser Prozess beendet ist
    std::wstring cmd = L"cmd.exe /c ping -n 3 127.0.0.1 >nul & del /f /q \"" + exe_path().wstring() + L"\"";
    STARTUPINFOW si{sizeof si};
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return 0;
}

}  // namespace fl::installer
