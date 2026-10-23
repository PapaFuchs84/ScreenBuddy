// © 2026 FuchsLabs
// Ausgabe auf einen normalen Bildschirm (HDMI/USB-C): randloses Vollbildfenster ohne Taskleisten-Eintrag.
// Touch: native WM_POINTER-Ereignisse. Die Zuordnung Touchgerät -> Monitor wird von Windows verwaltet.
#include <windows.h>
#include <shellscalingapi.h>
#include <shobjidl.h>

#include <algorithm>
#include <stdexcept>

#include "output.h"
#include "util.h"

namespace fl {

namespace {
constexpr UINT WM_FRAME = WM_APP + 1, WM_QUIT_WINDOW = WM_APP + 2,
               WM_VISIBLE = WM_APP + 3, WM_SETUP_TOUCH = WM_APP + 4;
constexpr UINT_PTR MONITOR_TIMER = 1;

// Taskleisten auf dem Anzeigebildschirm: Unser Fenster darf keinen Fokus nehmen (NOACTIVATE), deshalb schiebt Windows
// die Taskleiste dieses Bildschirms bei jeder Berührung wieder nach vorn – ein Tipp unten öffnet dann das Startmenü.
// Solange die Anzeige läuft, wird sie auf diesem Bildschirm ausgeblendet und beim Verlassen wieder gezeigt.
std::vector<HWND> taskbars_on(HMONITOR mon) {
    std::vector<HWND> out;
    for (const wchar_t* cls : {L"Shell_SecondaryTrayWnd", L"Shell_TrayWnd"}) {
        for (HWND h = FindWindowExW(nullptr, nullptr, cls, nullptr); h; h = FindWindowExW(nullptr, h, cls, nullptr))
            if (MonitorFromWindow(h, MONITOR_DEFAULTTONULL) == mon) out.push_back(h);
    }
    return out;
}
const wchar_t* CLASS_NAME = L"FuchsLabsUsbDisplayScreen";

HANDLE launch_touch_setup() {
    wchar_t dir[MAX_PATH];
    UINT n = GetSystemDirectoryW(dir, MAX_PATH);
    if (!n || n >= MAX_PATH) return nullptr;
    std::wstring exe = std::wstring(dir) + L"\\multidigimon.exe";
    std::wstring command = L"\"" + exe + L"\" -touch";
    STARTUPINFOW si{sizeof si};
    PROCESS_INFORMATION pi{};
    // Absoluter Windows-Pfad und feste Argumente; keine Kommandozeile aus der Web-Anfrage.
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, dir, &si, &pi)) return nullptr;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = (MonitorDisplay*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        self = (MonitorDisplay*)cs->lpCreateParams;
    }
    long long result = 0;
    if (self && self->handle(hwnd, msg, wp, lp, result)) return (LRESULT)result;
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

void dpi_aware() {
    // Echte Pixel statt Windows-Skalierung (sonst wird 1920 × 720 bei 125 % als 1536 × 576 gemeldet)
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) SetProcessDPIAware();
}

std::vector<MonitorInfo> list_monitors() {
    std::vector<MonitorInfo> out;
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR hm, HDC, LPRECT, LPARAM data) -> BOOL {
            auto* v = (std::vector<MonitorInfo>*)data;
            MONITORINFOEXW mi{};
            mi.cbSize = sizeof mi;
            if (GetMonitorInfoW(hm, &mi)) {
                DISPLAY_DEVICEW dd{};
                dd.cb = sizeof dd;
                MonitorInfo m;
                m.device = narrow(mi.szDevice);
                m.name = EnumDisplayDevicesW(mi.szDevice, 0, &dd, 0) ? narrow(dd.DeviceString) : "";
                m.x = mi.rcMonitor.left;
                m.y = mi.rcMonitor.top;
                m.w = mi.rcMonitor.right - mi.rcMonitor.left;
                m.h = mi.rcMonitor.bottom - mi.rcMonitor.top;
                m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
                v->push_back(m);
            }
            return TRUE;
        },
        (LPARAM)&out);
    return out;
}

ojson monitors_json() {
    ojson arr = ojson::array();
    for (auto& m : list_monitors())
        arr.push_back({{"device", m.device}, {"name", m.name}, {"x", m.x}, {"y", m.y}, {"w", m.w}, {"h", m.h}, {"primary", m.primary}});
    return arr;
}

ojson touch_info() {
    int v = GetSystemMetrics(SM_DIGITIZER);
    bool ok = (v & NID_READY) && (v & (NID_INTEGRATED_TOUCH | NID_EXTERNAL_TOUCH));
    ojson devices = ojson::array();
    bool known = false;
    // Beim An-/Abstecken kann sich die Anzahl zwischen den beiden Aufrufen ändern.
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 count = 0;
        if (!GetPointerDevices(&count, nullptr) || count > 256) break;
        if (!count) { known = !ok; break; }
        std::vector<POINTER_DEVICE_INFO> pointers(count);
        if (!GetPointerDevices(&count, pointers.data())) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) continue;
            break;
        }
        known = true;
        for (UINT32 i = 0; i < count; ++i) {
            const auto& p = pointers[i];
            if (p.pointerDeviceType != POINTER_DEVICE_TYPE_TOUCH) continue;
            MONITORINFOEXW mi{};
            mi.cbSize = sizeof mi;
            bool mapped = p.monitor && GetMonitorInfoW(p.monitor, &mi);
            devices.push_back({{"name", narrow(p.productString)}, {"monitor", mapped ? narrow(mi.szDevice) : ""},
                               {"points", p.maxActiveContacts}});
            if (!mapped) known = false;
        }
        // Manche Treiber melden nur die globale Touch-Fähigkeit, aber keine Gerätezuordnung.
        if (ok && devices.empty()) known = false;
        break;
    }
    return {{"available", ok || !devices.empty()}, {"points", GetSystemMetrics(SM_MAXIMUMTOUCHES)},
            {"mapping_known", known}, {"devices", devices}};
}

bool open_touch_setup() {
    HANDLE process = launch_touch_setup();
    if (!process) return false;
    CloseHandle(process);
    return true;
}

static std::optional<MonitorInfo> pick_monitor(const std::string& device) {
    // Gewählter Bildschirm; ohne Wahl der erste, der nicht Hauptbildschirm ist (dort läuft ja Windows)
    for (auto& m : list_monitors()) {
        if (!device.empty() ? casefold(m.device) == casefold(device) : !m.primary) return m;
    }
    return std::nullopt;
}

void MonitorDisplay::Gesture::down(int x, int y) { start = std::make_tuple(x, y, mono()); }

int MonitorDisplay::Gesture::end(int x, int y) {
    tapped = false;
    if (!start) return 0;
    auto [x0, y0, t0] = *start;
    start.reset();
    int dx = x - x0, dy = y - y0;
    double thr = std::max(30.0, w * 0.05);
    if (std::abs(dx) >= thr && std::abs(dx) > std::abs(dy)) return dx < 0 ? 1 : -1;
    if (std::abs(dx) < thr && std::abs(dy) < thr && mono() - t0 < 1.0) {
        tapped = true;
        return x < w / 3.0 ? -1 : 1;
    }
    return 0;
}

MonitorDisplay::MonitorDisplay(std::string device, bool touch) : device_(std::move(device)), touch_(touch) {
    mon_ = pick_monitor(device_);
    // Nur für Tests: FUCHSLABS_TEST_RECT="x,y,b,h" statt eines ganzen Bildschirms
    wchar_t buf[64];
    if (test_instance() && GetEnvironmentVariableW(L"FUCHSLABS_TEST_RECT", buf, 64)) {
        MonitorInfo m;
        if (swscanf_s(buf, L"%d,%d,%d,%d", &m.x, &m.y, &m.w, &m.h) == 4) {
            m.device = "Test";
            mon_ = m;
        }
    }
    auto [w, h] = size();
    gesture_ = Gesture{w, h, {}};
    mouse_gesture_ = Gesture{w, h, {}};
}

MonitorDisplay::~MonitorDisplay() { close(); }

std::pair<int, int> MonitorDisplay::size() const {
    return mon_ ? std::make_pair(mon_->w, mon_->h) : std::make_pair(480, 320);
}

void MonitorDisplay::connect() {
    if (!mon_) throw std::runtime_error(device_.empty() ? "Kein zweiter Bildschirm angeschlossen" : "Bildschirm nicht gefunden");
    if (thread_.joinable() && hwnd_.load()) {
        PostMessageW((HWND)hwnd_.load(), WM_VISIBLE, TRUE, 0);
        return;
    }
    if (thread_.joinable()) thread_.join();
    ready_ = false;
    thread_ = std::thread([this] { run(); });
    for (int i = 0; i < 100 && !ready_; ++i) Sleep(50);
    if (!ready_ || !hwnd_.load()) throw std::runtime_error("Anzeigefenster konnte nicht geöffnet werden");
    LOGI("monitor", "Ausgabe auf Bildschirm %s (%d × %d bei %d/%d)", mon_->device.c_str(), mon_->w, mon_->h, mon_->x, mon_->y);
}

bool MonitorDisplay::setup_touch() {
    HWND hwnd = (HWND)hwnd_.load();
    return hwnd && SendMessageW(hwnd, WM_SETUP_TOUCH, 0, 0) != 0;
}

void MonitorDisplay::fullscreen(bool show) {
    HWND hwnd = (HWND)hwnd_.load();
    if (!hwnd) return;
    if (taskbar_) ((ITaskbarList2*)taskbar_)->MarkFullscreenWindow(hwnd, show);
    if (show) {
        // NOACTIVATE erhält den Fokus auf dem Arbeitsbildschirm. TOPMOST hält die sekundäre Taskleiste darunter.
        SetWindowPos(hwnd, HWND_TOPMOST, mon_->x, mon_->y, mon_->w, mon_->h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    } else ShowWindow(hwnd, SW_HIDE);
    hide_taskbars(show);
}

void MonitorDisplay::hide_taskbars(bool hide) {
    HWND hwnd = (HWND)hwnd_.load();
    if (hide && hwnd) {
        HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if (!mon) return;
        for (HWND t : taskbars_on(mon)) {
            if (!IsWindowVisible(t)) continue;
            ShowWindow(t, SW_HIDE);
            if (std::find(hidden_taskbars_.begin(), hidden_taskbars_.end(), (void*)t) == hidden_taskbars_.end()) {
                hidden_taskbars_.push_back(t);
                LOGI("monitor", "Taskleiste auf dem Anzeigebildschirm ausgeblendet");
            }
        }
    } else {
        for (void* t : hidden_taskbars_)
            if (IsWindow((HWND)t)) ShowWindow((HWND)t, SW_SHOWNA);
        hidden_taskbars_.clear();
    }
}

void MonitorDisplay::run() {
    visible_ = true;
    pointer_id_ = 0;
    gesture_.start.reset();
    mouse_gesture_.start.reset();
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HINSTANCE hinst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = hinst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    // Randlos, ohne Taskleisten-Eintrag und ohne Fokus zu stehlen (Tippen aktiviert das Fenster nicht)
    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST, CLASS_NAME, L"FuchsLabs USB-Display", WS_POPUP,
                                mon_->x, mon_->y, mon_->w, mon_->h, nullptr, nullptr, hinst, this);
    if (!hwnd) {
        LOGW("monitor", "Fenster nicht erstellt (Fehler %lu)", GetLastError());
        ready_ = true;
        if (SUCCEEDED(com)) CoUninitialize();
        return;
    }
    hwnd_ = hwnd;
    ITaskbarList2* taskbar = nullptr;
    if (SUCCEEDED(com) && SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar)))) {
        if (SUCCEEDED(taskbar->HrInit())) taskbar_ = taskbar;
        else { taskbar->Release(); taskbar = nullptr; }
    }
    fullscreen(true);
    SetTimer(hwnd, MONITOR_TIMER, 1000, nullptr);
    ready_ = true;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    hwnd_ = nullptr;
    taskbar_ = nullptr;
    if (taskbar) taskbar->Release();
    if (SUCCEEDED(com)) CoUninitialize();
}

bool MonitorDisplay::handle(void* hwnd_v, unsigned msg, unsigned long long wp, long long lp, long long& result) {
    HWND hwnd = (HWND)hwnd_v;
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            {
                std::lock_guard lk(mu_);
                auto [w, h] = size();
                if (!bits_.empty()) {
                    BITMAPINFO bmi{};
                    bmi.bmiHeader.biSize = sizeof bmi.bmiHeader;
                    bmi.bmiHeader.biWidth = w;
                    bmi.bmiHeader.biHeight = -h;  // negativ = Zeilen von oben nach unten
                    bmi.bmiHeader.biPlanes = 1;
                    bmi.bmiHeader.biBitCount = 32;
                    SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, bits_.data(), &bmi, DIB_RGB_COLORS);
                }
            }
            EndPaint(hwnd, &ps);
            result = 0;
            return true;
        }
        case WM_ERASEBKGND: result = 1; return true;  // kein weißes Aufblitzen zwischen den Bildern
        case WM_SETCURSOR: SetCursor(nullptr); result = 1; return true;  // Mauszeiger über der Anzeige ausblenden
        case WM_MOUSEACTIVATE: result = MA_NOACTIVATE; return true;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: {
            // Echte Mausbedienung erhalten; aus Touch/Stift erzeugte Mausnachrichten nicht doppelt auswerten.
            INPUT_MESSAGE_SOURCE source{};
            bool pointer_mouse = GetCurrentInputMessageSource(&source) &&
                                 (source.deviceType == IMDT_TOUCH || source.deviceType == IMDT_PEN);
            if (touch_ && !setup_process_ && !pointer_id_ && !pointer_mouse &&
                ((ULONG_PTR)GetMessageExtraInfo() & 0xFFFFFF00) != 0xFF515700) {
                int x = (short)(lp & 0xFFFF), y = (short)((lp >> 16) & 0xFFFF);
                if (msg == WM_LBUTTONDOWN) mouse_gesture_.down(x, y);
                else if (int step = mouse_gesture_.end(x, y); step && !(mouse_gesture_.tapped && on_tap && on_tap(x, y)) && on_step)
                    on_step(step);
            }
            result = 0; return true;
        }
        case WM_POINTERACTIVATE: result = PA_NOACTIVATE; return true;
        case WM_POINTERDOWN:
        case WM_POINTERUPDATE:
        case WM_POINTERUP: {
            POINTER_INFO info{};
            unsigned id = GET_POINTERID_WPARAM(wp);
            if (!GetPointerInfo(id, &info) || info.pointerType != PT_TOUCH) return false;
            // Auch deaktivierten Touch konsumieren: kein zusätzlicher, synthetischer Mausklick.
            if (msg == WM_POINTERDOWN && !touch_ && !touch_off_logged_) {
                touch_off_logged_ = true;
                LOGW("monitor", "Berührung auf dem Anzeigebildschirm, aber Touch ist in den Einstellungen aus (Display → Touch)");
            }
            if (touch_ && !setup_process_) {
                POINT pt = info.ptPixelLocation;
                ScreenToClient(hwnd, &pt);
                if (info.pointerFlags & POINTER_FLAG_CANCELED) {
                    if (pointer_id_ == id) { pointer_id_ = 0; gesture_.start.reset(); }
                } else if (msg == WM_POINTERDOWN && !pointer_id_) {
                    mouse_gesture_.start.reset();
                    pointer_id_ = id;
                    gesture_.down(pt.x, pt.y);
                } else if (msg == WM_POINTERUP && pointer_id_ == id) {
                    pointer_id_ = 0;
                    int x0 = gesture_.start ? std::get<0>(*gesture_.start) : pt.x;
                    int step = gesture_.end(pt.x, pt.y);
                    // Protokoll: zeigt, ob Berührungen ankommen und wie sie gedeutet werden
                    bool opened = gesture_.tapped && on_tap && on_tap(pt.x, pt.y);
                    LOGI("monitor", "Touch: %d px waagerecht → %s", pt.x - x0,
                         opened ? "Ziel ausgewählt" : step > 0 ? "nächste Seite" : step < 0 ? "vorige Seite" : "keine Geste");
                    if (step && !opened && on_step) on_step(step);
                }
            }
            result = 0;
            return true;
        }
        case WM_POINTERCAPTURECHANGED:
            if (pointer_id_ == GET_POINTERID_WPARAM(wp)) { pointer_id_ = 0; gesture_.start.reset(); }
            result = 0; return true;
        case WM_VISIBLE:
            visible_ = wp != 0;
            if (!visible_) { pointer_id_ = 0; gesture_.start.reset(); mouse_gesture_.start.reset(); }
            fullscreen(visible_ && !setup_process_);
            result = 0; return true;
        case WM_SETUP_TOUCH:
            if (setup_process_) { result = 1; return true; }
            pointer_id_ = 0;
            gesture_.start.reset();
            mouse_gesture_.start.reset();
            fullscreen(false);
            setup_process_ = launch_touch_setup();
            if (!setup_process_) fullscreen(visible_);
            result = setup_process_ ? 1 : 0;
            return true;
        case WM_TIMER:
            if (wp != MONITOR_TIMER) return false;
            if (setup_process_) {
                if (WaitForSingleObject((HANDLE)setup_process_, 0) == WAIT_OBJECT_0) {
                    CloseHandle((HANDLE)setup_process_);
                    setup_process_ = nullptr;
                    fullscreen(visible_);
                }
            } else if (visible_) {
                // Explorer-Neustart, Taskleisten-Aktivierung und Fokuswechsel dürfen die Leiste nicht freilegen.
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                hide_taskbars(true);  // Explorer zeigt sie nach Neustart oder Einstellungsänderung wieder
            }
            result = 0; return true;
        case WM_FRAME: InvalidateRect(hwnd, nullptr, FALSE); result = 0; return true;
        case WM_CLOSE: result = 0; return true;  // Alt+F4 schließt die Anzeige nicht; Beenden geht über das Tray-Symbol
        case WM_QUIT_WINDOW: DestroyWindow(hwnd); result = 0; return true;
        case WM_DESTROY:
            KillTimer(hwnd, MONITOR_TIMER);
            fullscreen(false);
            if (setup_process_) { CloseHandle((HANDLE)setup_process_); setup_process_ = nullptr; }
            PostQuitMessage(0); result = 0; return true;
    }
    (void)wp;
    return false;
}

int MonitorDisplay::show(const Image& frame) {
    auto [w, h] = size();
    if (frame.w != w || frame.h != h) throw std::runtime_error("Bildgröße passt nicht zum Bildschirm");
    HWND hwnd = (HWND)hwnd_.load();
    if (!hwnd) throw std::runtime_error("Anzeigefenster ist geschlossen");
    {
        std::lock_guard lk(mu_);
        bits_ = frame.px;  // BGRA wie der Bildschirm
    }
    PostMessageW(hwnd, WM_FRAME, 0, 0);
    return frame.h;
}

void MonitorDisplay::screen_off() {
    if (auto hwnd = hwnd_.load()) PostMessageW((HWND)hwnd, WM_VISIBLE, FALSE, 0);
}

void MonitorDisplay::close() {
    if (auto hwnd = hwnd_.load()) PostMessageW((HWND)hwnd, WM_QUIT_WINDOW, 0, 0);
    if (thread_.joinable()) thread_.join();
}

}  // namespace fl
