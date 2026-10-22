// © 2026 FuchsLabs
#include "tray.h"

#include <windows.h>
#include <shellapi.h>

#include "gfx.h"
#include "i18n.h"
#include "util.h"

namespace fl {

namespace {
constexpr UINT WM_TRAY = WM_USER + 20;
constexpr UINT ID_OPEN = 1001, ID_RESTART = 1002, ID_QUIT = 1003;
const wchar_t* CLASS_NAME = L"FuchsLabsUsbDisplayTray";

LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = (Tray*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE) {
        self = (Tray*)((CREATESTRUCTW*)lp)->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    }
    long long result = 0;
    if (self && self->handle(hwnd, msg, wp, lp, result)) return (LRESULT)result;
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

void* make_hicon(int size) {
    // FuchsLabs-Fuchs (Creme auf Orange), für jede Größe eigens gezeichnet, damit 16 px nicht verschwimmt
    Image img = fox_image(size, true);
    BITMAPV5HEADER bi{};
    bi.bV5Size = sizeof bi;
    bi.bV5Width = size;
    bi.bV5Height = -size;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;
    void* bits = nullptr;
    HDC dc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, dc);
    if (!color) return nullptr;
    // Symbole erwarten nicht vormultipliziertes Alpha
    auto* dst = (uint32_t*)bits;
    for (size_t i = 0; i < img.px.size(); ++i) {
        uint32_t p = img.px[i];
        uint32_t a = p >> 24;
        if (a && a < 255) {
            uint32_t r = std::min<uint32_t>(255, ((p >> 16) & 255) * 255 / a);
            uint32_t g = std::min<uint32_t>(255, ((p >> 8) & 255) * 255 / a);
            uint32_t b = std::min<uint32_t>(255, (p & 255) * 255 / a);
            p = a << 24 | r << 16 | g << 8 | b;
        }
        dst[i] = p;
    }
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO ii{TRUE, 0, 0, mask, color};
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}

Tray::Tray(std::string tip, std::function<void()> on_open, std::function<void()> on_restart, std::function<void()> on_quit)
    : tip_(std::move(tip)), on_open_(std::move(on_open)), on_restart_(std::move(on_restart)), on_quit_(std::move(on_quit)) {}

Tray::~Tray() { stop(); }

void Tray::start() {
    th_ = std::thread([this] { run(); });
}

void Tray::stop() {
    if (hwnd_) PostMessageW((HWND)hwnd_, WM_CLOSE, 0, 0);
    if (th_.joinable()) th_.join();
}

static void add_icon(HWND hwnd, HICON icon, const std::string& tip, bool& added) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof nid;
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = icon;
    wcsncpy_s(nid.szTip, widen(tip).c_str(), _TRUNCATE);
    added = Shell_NotifyIconW(NIM_ADD, &nid) != 0;
    if (!added) LOGW("tray", "Tray-Symbol konnte nicht angelegt werden");
}

void Tray::run() {
    HINSTANCE hinst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = hinst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    taskbar_created_ = RegisterWindowMessageW(L"TaskbarCreated");
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"FuchsLabs USB-Display", 0, 0, 0, 0, 0, nullptr, nullptr, hinst, this);
    if (!hwnd) return;
    hwnd_ = hwnd;
    int px = GetSystemMetrics(SM_CXSMICON);  // passende Größe bei hoher Skalierung
    icon_ = make_hicon(px ? px : 16);
    // Beim Windows-Start ist die Taskleiste manchmal noch nicht da: einige Male erneut versuchen
    for (int i = 0; i < 30 && !added_; ++i) {
        add_icon(hwnd, (HICON)icon_, tip_, added_);
        if (!added_) Sleep(1000);
    }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    hwnd_ = nullptr;
    if (icon_) DestroyIcon((HICON)icon_);
}

bool Tray::handle(void* hwnd_v, unsigned msg, unsigned long long, long long lp, long long& result) {
    HWND hwnd = (HWND)hwnd_v;
    if (msg == WM_TRAY) {
        if (lp == WM_LBUTTONDBLCLK) on_open_();
        else if (lp == WM_RBUTTONUP) menu(hwnd);
        result = 0;
        return true;
    }
    if (taskbar_created_ && msg == taskbar_created_) {  // Explorer neu gestartet: Symbol wieder anlegen
        add_icon(hwnd, (HICON)icon_, tip_, added_);
        return true;
    }
    if (msg == WM_CLOSE) {
        DestroyWindow(hwnd);
        result = 0;
        return true;
    }
    if (msg == WM_DESTROY) {
        if (added_) {
            NOTIFYICONDATAW nid{};
            nid.cbSize = sizeof nid;
            nid.hWnd = hwnd;
            nid.uID = 1;
            Shell_NotifyIconW(NIM_DELETE, &nid);
            added_ = false;
        }
        PostQuitMessage(0);
        result = 0;
        return true;
    }
    return false;
}

void Tray::menu(void* hwnd_v) {
    HWND hwnd = (HWND)hwnd_v;
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, ID_OPEN, widen(tr("Einstellungen öffnen")).c_str());
    AppendMenuW(m, MF_STRING, ID_RESTART, widen(tr("Neu starten")).c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, ID_QUIT, widen(tr("Beenden")).c_str());
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);  // sonst schließt das Menü nicht bei Klick daneben
    UINT cmd = (UINT)TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(m);
    if (cmd == ID_OPEN) on_open_();
    else if (cmd == ID_RESTART) on_restart_();
    else if (cmd == ID_QUIT) on_quit_();
}

}  // namespace fl
