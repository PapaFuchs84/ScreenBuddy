// © 2026 FuchsLabs
// Isolated browser-free Win32 spike. Uses only defaults/sample data and never constructs App/Output.
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>
#include <cwchar>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "i18n.h"
#include "render.h"
#include "util.h"

namespace {
constexpr wchar_t ClassName[] = L"ScreenBuddyNativeSettingsSpike";
enum : int { Theme = 101, Brightness, Touch, Apply, Reset, Title, Note, State, PreviewCaption, BrightnessCaption,
             ThemeCaption, Preview, StatusCaption, TouchCaption };

struct Window {
    HWND hwnd = nullptr;
    HFONT font = nullptr;
    UINT dpi = 96;
    bool english = false;
    fl::ojson committed = fl::config::defaults();
    fl::Image preview;
    std::vector<HWND> children;

    ~Window() { if (font) DeleteObject(font); }
    const wchar_t* text(const wchar_t* de, const wchar_t* en) const { return english ? en : de; }
    int px(int logical) const { return MulDiv(logical, dpi, 96); }
    HWND child(int id) const { return GetDlgItem(hwnd, id); }

    HWND control(const wchar_t* type, const wchar_t* label, DWORD style, int id) {
        HWND result = CreateWindowExW(0, type, label, WS_CHILD | WS_VISIBLE | style,
                                      0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(INT_PTR(id)),
                                      GetModuleHandleW(nullptr), nullptr);
        if (!result) throw std::runtime_error("CreateWindowExW: native control");
        children.push_back(result);
        return result;
    }

    void fonts() {
        HFONT next = CreateFontW(-px(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH, L"Segoe UI");
        if (!next) throw std::runtime_error("CreateFontW");
        for (HWND c : children) SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(next), TRUE);
        if (font) DeleteObject(font);
        font = next;
    }

    void initialize() {
        control(L"STATIC", L"ScreenBuddy 2.5 · Native UI Spike", SS_LEFT, Title);
        control(L"STATIC", text(L"Isolierte Vorschau mit Beispieldaten. Keine Geräteverbindung, keine Speicherung.",
                                L"Isolated sample preview. No device connection, no saved configuration."), SS_LEFT, Note);
        control(L"STATIC", text(L"&Design", L"&Theme"), SS_LEFT, ThemeCaption);
        HWND theme = control(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, Theme);
        SendMessageW(theme, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text(L"Hell", L"Light")));
        SendMessageW(theme, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text(L"Dunkel", L"Dark")));
        control(L"STATIC", text(L"&Helligkeit (0–100)", L"&Brightness (0–100)"), SS_LEFT, BrightnessCaption);
        control(L"EDIT", L"50", WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, Brightness);
        SendMessageW(child(Brightness), EM_SETLIMITTEXT, 3, 0);
        control(L"BUTTON", text(L"&Touch aktivieren", L"Enable &touch"), BS_AUTOCHECKBOX | WS_TABSTOP, Touch);
        control(L"STATIC", text(L"Helligkeit und Touch werden nur im Demo-Modell geändert.",
                                L"Brightness and touch change only the demo model."), SS_LEFT, TouchCaption);
        control(L"BUTTON", text(L"&Übernehmen (nur Demo)", L"&Apply (demo only)"), BS_DEFPUSHBUTTON | WS_TABSTOP, Apply);
        control(L"BUTTON", text(L"&Zurücksetzen", L"&Reset"), BS_PUSHBUTTON | WS_TABSTOP, Reset);
        control(L"STATIC", text(L"Status", L"Status"), SS_LEFT, StatusCaption);
        control(L"STATIC", text(L"Offline-Demo · keine Hardware geöffnet", L"Offline demo · no hardware opened"), SS_LEFT, State);
        control(L"STATIC", text(L"Display-Vorschau · Original-C++-Renderer", L"Display preview · original C++ renderer"), SS_LEFT, PreviewCaption);
        // SS_OWNERDRAW keeps the preview in an accessible native child window.
        control(L"STATIC", text(L"Display-Vorschau mit Beispieldaten", L"Display preview using sample data"), SS_OWNERDRAW, Preview);
        dpi = GetDpiForWindow(hwnd);
        fonts();
        restore();
    }

    void move(int id, int x, int y, int width, int height) {
        MoveWindow(child(id), px(x), px(y), px(width), px(height), TRUE);
    }
    void layout() {
        RECT r{};
        GetClientRect(hwnd, &r);
        int width = MulDiv(r.right, 96, dpi), height = MulDiv(r.bottom, 96, dpi);
        move(Title, 24, 20, width - 48, 26);
        move(Note, 24, 56, width - 48, 40);
        move(ThemeCaption, 24, 114, 252, 22);
        move(Theme, 24, 140, 252, 160);
        move(BrightnessCaption, 24, 188, 252, 22);
        move(Brightness, 24, 214, 252, 30);
        move(Touch, 24, 264, 252, 30);
        move(TouchCaption, 24, 306, 252, 62);
        move(Apply, 24, 390, 252, 34);
        move(Reset, 24, 436, 252, 34);
        move(StatusCaption, 24, height - 110, width - 48, 22);
        move(State, 24, height - 80, width - 48, 60);
        move(PreviewCaption, 316, 114, width - 340, 26);
        int available_w = std::max(1, width - 340), available_h = std::max(1, height - 290);
        int w = std::min(available_w, available_h * 3 / 2), h = w * 2 / 3;
        move(Preview, 316, 154, w, h);
    }

    void render(const fl::ojson& cfg) {
        // Same config validation + renderer as production; no App, data sources, Output or serial port.
        std::lock_guard lock(fl::render::mutex());
        fl::render::apply_theme(cfg.at("display"));
        fl::render::Renderer renderer(480, 320, "ScreenBuddy · Demo");
        preview = renderer.render_friends(fl::render::demo_friends(), 0, true,
                                          english ? "Sample data" : "Beispieldaten", 1792836000);
        InvalidateRect(child(Preview), nullptr, FALSE);
    }

    fl::ojson draft() const {
        wchar_t value[16]{};
        GetWindowTextW(child(Brightness), value, int(std::size(value)));
        wchar_t* end = nullptr;
        long brightness = wcstol(value, &end, 10);
        if (end == value || *end != L'\0' || brightness < 0 || brightness > 100)
            throw std::invalid_argument(english ? "Brightness must be 0–100." : "Helligkeit muss 0–100 sein.");
        LRESULT theme = SendMessageW(child(Theme), CB_GETCURSEL, 0, 0);
        return fl::config::merge(committed, {{"display", {
            {"brightness", brightness}, {"theme", theme == 1 ? "dunkel" : "hell"},
            {"touch", SendMessageW(child(Touch), BM_GETCHECK, 0, 0) == BST_CHECKED}
        }}});
    }

    void restore() {
        SendMessageW(child(Theme), CB_SETCURSEL, 0, 0);
        SetWindowTextW(child(Brightness), L"50");
        SendMessageW(child(Touch), BM_SETCHECK, BST_CHECKED, 0);
        committed = fl::config::defaults();
        render(committed);
        SetWindowTextW(child(State), text(L"Offline-Demo · keine Hardware geöffnet", L"Offline demo · no hardware opened"));
    }

    void command(WPARAM command) {
        int id = LOWORD(command), event = HIWORD(command);
        if (id == Reset && event == BN_CLICKED) { restore(); return; }
        if (id == Theme && event == CBN_SELCHANGE) { render(draft()); return; }
        if (id != Apply || event != BN_CLICKED) return;
        fl::ojson next = draft();
        render(next);
        committed = std::move(next);  // Deliberately memory-only: never config::load/save.
        std::wstring status = text(L"Demo übernommen · Helligkeit ", L"Demo applied · brightness ");
        status += std::to_wstring(committed["display"]["brightness"].get<int>());
        status += committed["display"]["touch"].get<bool>() ? text(L" · Touch an", L" · touch on") : text(L" · Touch aus", L" · touch off");
        status += text(L"\r\nKeine Datei geschrieben, keine Geräteaktion.", L"\r\nNo file written, no device action.");
        SetWindowTextW(child(State), status.c_str());
    }

    void paint(const DRAWITEMSTRUCT& item) const {
        FillRect(item.hDC, &item.rcItem, GetSysColorBrush(COLOR_WINDOW));
        if (preview.empty()) return;
        BITMAPINFO bitmap{};
        bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = preview.w;
        bitmap.bmiHeader.biHeight = -preview.h;
        bitmap.bmiHeader.biPlanes = 1;
        bitmap.bmiHeader.biBitCount = 32;
        bitmap.bmiHeader.biCompression = BI_RGB;
        SetStretchBltMode(item.hDC, HALFTONE);
        SetBrushOrgEx(item.hDC, 0, 0, nullptr);
        StretchDIBits(item.hDC, 0, 0, item.rcItem.right, item.rcItem.bottom, 0, 0,
                      preview.w, preview.h, preview.px.data(), &bitmap, DIB_RGB_COLORS, SRCCOPY);
    }
};

LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        window = static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        window->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (!window) return DefWindowProcW(hwnd, msg, wp, lp);
    try {
        switch (msg) {
        case WM_CREATE: window->initialize(); return 0;
        case WM_SIZE: window->layout(); return 0;
        case WM_COMMAND: window->command(wp); return 0;
        case WM_DRAWITEM: window->paint(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
        case WM_GETMINMAXINFO: {
            auto info = reinterpret_cast<MINMAXINFO*>(lp);
            info->ptMinTrackSize = {window->px(960), window->px(720)};
            return 0;
        }
        case WM_DPICHANGED: {
            window->dpi = HIWORD(wp);
            window->fonts();
            auto rect = reinterpret_cast<RECT*>(lp);
            SetWindowPos(hwnd, nullptr, rect->left, rect->top, rect->right - rect->left,
                          rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            window->layout();
            return 0;
        }
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_NCDESTROY: SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0); break;
        }
    } catch (const std::exception& error) {
        MessageBoxW(hwnd, fl::widen(error.what()).c_str(), L"ScreenBuddy Native UI Spike", MB_OK | MB_ICONERROR);
        if (msg == WM_CREATE) return -1;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    try {
        Window window;
        int count = 0;
        LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
        if (args) {
            for (int i = 1; i < count; ++i) if (wcscmp(args[i], L"--en") == 0) window.english = true;
            LocalFree(args);
        }
        fl::i18n::set_lang(window.english ? "en" : "de");
        fl::gfx_init();
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&common);
        WNDCLASSEXW klass{sizeof(klass)};
        klass.lpfnWndProc = procedure;
        klass.hInstance = instance;
        klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        klass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        klass.lpszClassName = ClassName;
        if (!RegisterClassExW(&klass)) throw std::runtime_error("RegisterClassExW");
        HWND hwnd = CreateWindowExW(WS_EX_CONTROLPARENT, ClassName, L"ScreenBuddy · Native Settings Spike",
                                     WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                     1040, 760, nullptr, nullptr, instance, &window);
        if (!hwnd) return 1;
        ShowWindow(hwnd, show);
        MSG message{};
        BOOL result = 0;
        while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            if (!IsDialogMessageW(hwnd, &message)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        return result < 0 ? 1 : int(message.wParam);
    } catch (const std::exception& error) {
        MessageBoxW(nullptr, fl::widen(error.what()).c_str(), L"ScreenBuddy Native UI Spike", MB_OK | MB_ICONERROR);
        return 1;
    }
}
