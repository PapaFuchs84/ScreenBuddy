// © 2026 FuchsLabs
// Ausgabe auf einen normalen Bildschirm (HDMI/USB-C): randloses Vollbildfenster ohne Taskleisten-Eintrag.
// Touch: Windows meldet Fingertipps an Fenster ohne eigene Touch-Behandlung zusätzlich als Mausklicks; daraus werden
// Wischen (links/rechts) und Tippen auf den linken/rechten Rand.
#include <windows.h>
#include <shellscalingapi.h>

#include <stdexcept>

#include "output.h"
#include "util.h"

namespace fl {

namespace {
constexpr UINT WM_FRAME = WM_APP + 1, WM_QUIT_WINDOW = WM_APP + 2;
const wchar_t* CLASS_NAME = L"FuchsLabsUsbDisplayScreen";

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
    // Meldet Windows einen Touchscreen? (Gilt für den ganzen PC)
    int v = GetSystemMetrics(SM_DIGITIZER);
    bool ok = (v & NID_READY) && (v & (NID_INTEGRATED_TOUCH | NID_EXTERNAL_TOUCH));
    return {{"available", ok}, {"points", ok ? GetSystemMetrics(SM_MAXIMUMTOUCHES) : 0}};
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
    if (!start) return 0;
    auto [x0, y0, t0] = *start;
    start.reset();
    int dx = x - x0, dy = y - y0;
    double thr = std::max(30.0, w * 0.05);
    if (std::abs(dx) >= thr && std::abs(dx) > std::abs(dy)) return dx < 0 ? 1 : -1;
    if (std::abs(dx) < thr && std::abs(dy) < thr && mono() - t0 < 1.0) return x < w / 3.0 ? -1 : 1;
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
}

MonitorDisplay::~MonitorDisplay() { close(); }

std::pair<int, int> MonitorDisplay::size() const {
    return mon_ ? std::make_pair(mon_->w, mon_->h) : std::make_pair(480, 320);
}

void MonitorDisplay::connect() {
    if (!mon_) throw std::runtime_error(device_.empty() ? "Kein zweiter Bildschirm angeschlossen" : "Bildschirm nicht gefunden");
    if (thread_.joinable() && hwnd_) {
        ShowWindow((HWND)hwnd_, SW_SHOWNOACTIVATE);
        return;
    }
    if (thread_.joinable()) thread_.join();
    ready_ = false;
    thread_ = std::thread([this] { run(); });
    for (int i = 0; i < 100 && !ready_; ++i) Sleep(50);
    if (!ready_ || !hwnd_) throw std::runtime_error("Anzeigefenster konnte nicht geöffnet werden");
    LOGI("monitor", "Ausgabe auf Bildschirm %s (%d × %d bei %d/%d)", mon_->device.c_str(), mon_->w, mon_->h, mon_->x, mon_->y);
}

void MonitorDisplay::run() {
    HINSTANCE hinst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = hinst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    // Randlos, ohne Taskleisten-Eintrag und ohne Fokus zu stehlen (Tippen aktiviert das Fenster nicht)
    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, CLASS_NAME, L"FuchsLabs USB-Display", WS_POPUP,
                                mon_->x, mon_->y, mon_->w, mon_->h, nullptr, nullptr, hinst, this);
    if (!hwnd) {
        LOGW("monitor", "Fenster nicht erstellt (Fehler %lu)", GetLastError());
        ready_ = true;
        return;
    }
    hwnd_ = hwnd;
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    ready_ = true;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    hwnd_ = nullptr;
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
        case WM_LBUTTONUP:
            if (!touch_) return false;
            {
                int x = (short)(lp & 0xFFFF), y = (short)((lp >> 16) & 0xFFFF);
                if (msg == WM_LBUTTONDOWN) gesture_.down(x, y);
                else if (int step = gesture_.end(x, y); step && on_step) on_step(step);
            }
            result = 0;
            return true;
        case WM_FRAME: InvalidateRect(hwnd, nullptr, FALSE); result = 0; return true;
        case WM_CLOSE: result = 0; return true;  // Alt+F4 schließt die Anzeige nicht; Beenden geht über das Tray-Symbol
        case WM_QUIT_WINDOW: DestroyWindow(hwnd); result = 0; return true;
        case WM_DESTROY: PostQuitMessage(0); result = 0; return true;
    }
    (void)wp;
    return false;
}

int MonitorDisplay::show(const Image& frame) {
    auto [w, h] = size();
    if (frame.w != w || frame.h != h) throw std::runtime_error("Bildgröße passt nicht zum Bildschirm");
    if (!hwnd_) throw std::runtime_error("Anzeigefenster ist geschlossen");
    {
        std::lock_guard lk(mu_);
        bits_ = frame.px;  // BGRA wie der Bildschirm
    }
    PostMessageW((HWND)hwnd_, WM_FRAME, 0, 0);
    return frame.h;
}

void MonitorDisplay::screen_off() {
    if (hwnd_) ShowWindow((HWND)hwnd_, SW_HIDE);
}

void MonitorDisplay::close() {
    if (hwnd_) PostMessageW((HWND)hwnd_, WM_QUIT_WINDOW, 0, 0);
    if (thread_.joinable()) thread_.join();
}

}  // namespace fl
