// Regressionen für HDMI-Vollbild und native Touch-Gesten. Ohne Argument keine sichtbare Ausgabe.
// "window" zeigt kurz ein neutrales Testbild auf einem Nebenmonitor und injiziert dort Testberührungen.
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include "output.h"
#include "util.h"

using namespace fl;
static int passed = 0, failed = 0;
#define CHECK(x) do { if (x) ++passed; else { ++failed; std::printf("FAIL line %d: %s (Win32=%lu)\n", __LINE__, #x, GetLastError()); } } while (0)

static void gestures() {
    MonitorDisplay::Gesture g{1920, 720, {}};
    CHECK(g.end(50, 100) == 0);
    g.down(1500, 100); CHECK(g.end(1500, 100) == 1);
    CHECK(g.end(1500, 100) == 0);  // dieselbe Berührung zählt nur einmal
    g.down(100, 100); CHECK(g.end(100, 100) == -1);
    g.down(800, 200); CHECK(g.end(400, 200) == 1);
    g.down(400, 200); CHECK(g.end(800, 200) == -1);
    g.down(400, 100); CHECK(g.end(410, 500) == 0);
    g.down(400, 100); g.start.reset(); CHECK(g.end(800, 100) == 0);
    auto info = touch_info();
    CHECK(info["available"].is_boolean());
    CHECK(info["mapping_known"].is_boolean());
    CHECK(info["devices"].is_array());
    for (auto& device : info["devices"]) {
        CHECK(device["monitor"].is_string());
        CHECK(device["points"].is_number());
    }
}

static HWND own_window() {
    HWND result = nullptr;
    EnumWindows([](HWND w, LPARAM ptr) -> BOOL {
        DWORD pid = 0;
        GetWindowThreadProcessId(w, &pid);
        wchar_t name[128]{};
        GetClassNameW(w, name, 128);
        if (pid == GetCurrentProcessId() && wcscmp(name, L"FuchsLabsUsbDisplayScreen") == 0) {
            *(HWND*)ptr = w;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&result);
    return result;
}

static bool contact(DWORD flags, int x, int y, UINT32 id = 1) {
    POINTER_TOUCH_INFO p{};
    p.pointerInfo.pointerType = PT_TOUCH;
    p.pointerInfo.pointerId = id;
    p.pointerInfo.pointerFlags = flags;
    p.pointerInfo.ptPixelLocation = {x, y};
    p.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    p.rcContact = {x - 2, y - 2, x + 2, y + 2};
    p.orientation = 90;
    p.pressure = 32000;
    Sleep(25);
    return InjectTouchInput(1, &p) != FALSE;
}

static void window_tests() {
    dpi_aware();
    auto monitors = list_monitors();
    auto it = std::find_if(monitors.begin(), monitors.end(), [](const auto& m) { return !m.primary; });
    if (it == monitors.end()) throw std::runtime_error("Für den sichtbaren Test wird ein Nebenmonitor benötigt");
    auto m = *it;
    HWND foreground = GetForegroundWindow();
    std::atomic<int> count{0}, direction{0};
    auto test = [&](bool touch) {
        MonitorDisplay display(m.device, touch);
        display.on_step = [&](int d) { direction = d; ++count; };
        display.connect();
        display.show(Image(m.w, m.h, {248, 240, 226}));
        HWND hwnd = own_window();
        CHECK(hwnd != nullptr);
        CHECK((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & (WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW)) ==
              (WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW));
        RECT rect{}; GetWindowRect(hwnd, &rect);
        CHECK(rect.left == m.x && rect.top == m.y && rect.right == m.x + m.w && rect.bottom == m.y + m.h);
        CHECK(GetForegroundWindow() == foreground);
        for (POINT p : {POINT{m.x + m.w / 2, m.y + m.h - 2}, POINT{m.x + m.w / 2, m.y + 2},
                        POINT{m.x + 2, m.y + m.h / 2}, POINT{m.x + m.w - 2, m.y + m.h / 2}})
            CHECK(WindowFromPoint(p) == hwnd);
        auto mouse = MAKELPARAM(100, 100);
        SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, mouse);
        SendMessageW(hwnd, WM_LBUTTONUP, 0, mouse);
        CHECK(count == (touch ? 1 : 0));  // echte Mausbedienung bleibt erhalten
        count = 0;
        int x = m.x + m.w / 4, y = m.y + m.h / 2;
        CHECK(WindowFromPoint({x, y}) == hwnd); // niemals Testeingaben an fremde Fenster senden
        if (WindowFromPoint({x, y}) != hwnd) throw std::runtime_error("Testfenster liegt nicht über dem Touch-Testpunkt");
        CHECK(contact(POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, x, y));
        CHECK(contact(POINTER_FLAG_UP, x, y));
        Sleep(100);
        CHECK(count == (touch ? 1 : 0));
        if (touch) {
            CHECK(direction == -1);
            int right = m.x + m.w * 3 / 4;
            CHECK(contact(POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, right, y));
            CHECK(contact(POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, x, y));
            CHECK(contact(POINTER_FLAG_UP, x, y));
            Sleep(100);
            CHECK(count == 2 && direction == 1);
            CHECK(contact(POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, x, y));
            CHECK(contact(POINTER_FLAG_UP | POINTER_FLAG_CANCELED, x, y));
            Sleep(100);
            CHECK(count == 2);
        }
        display.screen_off();
        Sleep(1200); // auch der Vollbild-Timer darf die ausgeschaltete Ausgabe nicht erneut zeigen
        CHECK(!IsWindowVisible(hwnd));
        display.connect();
        Sleep(100);
        CHECK(IsWindowVisible(hwnd));
        CHECK(WindowFromPoint({m.x + m.w / 2, m.y + m.h - 2}) == hwnd);
        CHECK(GetForegroundWindow() == foreground);
        display.close();
        CHECK(!IsWindow(hwnd));
    };
    CHECK(InitializeTouchInjection(2, TOUCH_FEEDBACK_NONE));
    test(true);
    count = 0;
    test(false);
}

int main(int argc, char** argv) {
    try {
        gestures();
        if (argc > 1 && std::string(argv[1]) == "window") window_tests();
    } catch (const std::exception& e) { ++failed; std::printf("ERROR: %s\n", e.what()); }
    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
