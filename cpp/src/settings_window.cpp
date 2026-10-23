// © 2026 FuchsLabs
#include "settings_window.h"

#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <wrl.h>
#include <WebView2.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "util.h"

namespace fl {
namespace {
constexpr wchar_t WINDOW_CLASS[] = L"FuchsLabsUsbDisplaySettingsWindow";
constexpr UINT WM_SHOW_SETTINGS = WM_APP + 31;
constexpr UINT WM_STOP_SETTINGS = WM_APP + 32;

std::wstring settings_url(int port) {
    return L"http://127.0.0.1:" + std::to_wstring(port) + L"/";
}

LRESULT CALLBACK settings_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
}  // namespace

struct SettingsWindow::Impl : std::enable_shared_from_this<SettingsWindow::Impl> {
    explicit Impl(int p) : port(p), url(settings_url(p)) {}
    void start() {
        worker = std::thread([this] { run(); });
        std::unique_lock lk(ready_mu);
        ready_cv.wait(lk, [this] { return ready; });
    }
    ~Impl() { stop(); }

    int port;
    std::wstring url;
    HWND hwnd = nullptr;
    std::thread worker;
    DWORD thread_id = 0;
    std::mutex ready_mu;
    std::condition_variable ready_cv;
    bool ready = false;
    std::atomic<bool> stopping{false};
    bool webview_started = false;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview;

    void signal_ready() {
        { std::lock_guard lk(ready_mu); ready = true; }
        ready_cv.notify_all();
    }

    void show() {
        HWND w = hwnd;
        if (w) PostMessageW(w, WM_SHOW_SETTINGS, 0, 0);
    }

    void stop() {
        stopping = true;
        HWND w = hwnd;
        if (w) PostMessageW(w, WM_STOP_SETTINGS, 0, 0);
        else if (thread_id) PostThreadMessageW(thread_id, WM_QUIT, 0, 0);
        if (worker.joinable() && worker.get_id() != std::this_thread::get_id()) worker.join();
    }

    void resize_webview() {
        if (!controller || !hwnd) return;
        RECT r{};
        GetClientRect(hwnd, &r);
        controller->put_Bounds(r);
    }

    void runtime_error(HRESULT hr) {
        wchar_t msg[512];
        swprintf_s(msg, L"Das Einstellungsfenster konnte nicht geladen werden.\n\n"
                         L"Bitte installiere die Microsoft Edge WebView2-Laufzeit und starte die App erneut.\n"
                         L"Fehlercode: 0x%08X", (unsigned)hr);
        MessageBoxW(hwnd, msg, L"FuchsLabs USB-Display", MB_OK | MB_ICONERROR);
    }

    void initialize_webview() {
        if (webview_started || stopping) return;
        webview_started = true;
        std::weak_ptr<Impl> weak = weak_from_this();
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr, nullptr, nullptr,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [weak](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    auto self = weak.lock();
                    if (!self || self->stopping) return S_OK;
                    if (FAILED(result) || !env) {
                        self->runtime_error(FAILED(result) ? result : E_FAIL);
                        return S_OK;
                    }
                    Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment = env;
                    HRESULT create_hr = environment->CreateCoreWebView2Controller(
                        self->hwnd,
                        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [weak, environment](HRESULT controller_result, ICoreWebView2Controller* ctl) -> HRESULT {
                                auto self = weak.lock();
                                if (!self || self->stopping) return S_OK;
                                if (FAILED(controller_result) || !ctl) {
                                    self->runtime_error(FAILED(controller_result) ? controller_result : E_FAIL);
                                    return S_OK;
                                }
                                self->controller = ctl;
                                self->controller->get_CoreWebView2(&self->webview);
                                self->resize_webview();
                                self->webview->Navigate(self->url.c_str());
                                self->add_navigation_handlers();
                                return S_OK;
                            }).Get());
                    if (FAILED(create_hr)) self->runtime_error(create_hr);
                    return S_OK;
                }).Get());
        if (FAILED(hr)) runtime_error(hr);
    }

    void add_navigation_handlers() {
        std::weak_ptr<Impl> weak = weak_from_this();
        webview->add_NavigationStarting(
            Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                [weak](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                    auto self = weak.lock();
                    if (!self || self->stopping) return S_OK;
                    LPWSTR raw = nullptr;
                    if (SUCCEEDED(args->get_Uri(&raw)) && raw) {
                        std::wstring target(raw);
                        CoTaskMemFree(raw);
                        if (target.rfind(self->url, 0) != 0) {
                            args->put_Cancel(TRUE);
                            if (target.rfind(L"https://", 0) == 0 || target.rfind(L"http://", 0) == 0)
                                ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        }
                    }
                    return S_OK;
                }).Get(), &navigation_token);
        webview->add_NewWindowRequested(
            Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                [weak](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                    auto self = weak.lock();
                    if (!self || self->stopping) { args->put_Handled(TRUE); return S_OK; }
                    LPWSTR raw = nullptr;
                    if (SUCCEEDED(args->get_Uri(&raw)) && raw) {
                        std::wstring target(raw);
                        CoTaskMemFree(raw);
                        // Neue Fenster (z. B. die bebilderte Anleitung) öffnen im Standardbrowser, nicht im
                        // Einstellungsfenster: so bleiben Einstellungen und Anleitung nebeneinander offen
                        if (target.rfind(L"https://", 0) == 0 || target.rfind(L"http://", 0) == 0)
                            ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    }
                    args->put_Handled(TRUE);
                    return S_OK;
                }).Get(), &new_window_token);
    }

    void run() {
        thread_id = GetCurrentThreadId();
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSEXW wc{sizeof wc};
        wc.lpfnWndProc = settings_proc;
        wc.hInstance = instance;
        wc.hIcon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        wc.hIconSm = wc.hIcon;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = WINDOW_CLASS;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            signal_ready();
            if (SUCCEEDED(com)) CoUninitialize();
            return;
        }
        hwnd = CreateWindowExW(0, WINDOW_CLASS, L"FuchsLabs USB-Display – Einstellungen",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1120, 780,
            nullptr, nullptr, instance, this);
        if (!hwnd) {
            signal_ready();
            if (SUCCEEDED(com)) CoUninitialize();
            return;
        }
        signal_ready();
        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        hwnd = nullptr;
        if (SUCCEEDED(com)) CoUninitialize();
    }

    EventRegistrationToken navigation_token{};
    EventRegistrationToken new_window_token{};
};

namespace {
LRESULT CALLBACK settings_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<SettingsWindow::Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<SettingsWindow::Impl*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd = hwnd;
    }
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
    case WM_SHOW_SETTINGS:
        ShowWindow(hwnd, SW_SHOW);
        if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        self->initialize_webview();
        return 0;
    case WM_STOP_SETTINGS:
        self->stopping = true;
        DestroyWindow(hwnd);
        return 0;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);  // Schließen blendet ins Tray aus; beendet nicht die Anzeige.
        return 0;
    case WM_SIZE:
        self->resize_webview();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* m = reinterpret_cast<MINMAXINFO*>(lp);
        m->ptMinTrackSize = {720, 500};
        return 0;
    }
    case WM_DESTROY:
        self->webview.Reset();
        if (self->controller) {
            self->controller->Close();
            self->controller.Reset();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

SettingsWindow::SettingsWindow(int port) : d_(std::make_shared<Impl>(port)) { d_->start(); }
SettingsWindow::~SettingsWindow() = default;
void SettingsWindow::show() { d_->show(); }

bool SettingsWindow::visible() const {
    HWND w = d_ ? d_->hwnd : nullptr;
    return w && IsWindowVisible(w) && !IsIconic(w);
}

}  // namespace fl