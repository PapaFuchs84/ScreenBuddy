"""Exercise native mouse capture/release through actual settings window messages.

No MSAA default actions are used to click widgets. MSAA only supplies geometry
and observes state. Mouse messages target the isolated --no-lcd test HWND; the
mouse helpers do not move the desktop cursor or foreground window. No hardware
or installation is used.
"""
import ctypes
import os
from pathlib import Path
import socket
import subprocess
import tempfile

import native_settings_smoke as ui

U = ui.USER32
U.SendMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_size_t, ctypes.c_ssize_t]
U.SendMessageW.restype = ctypes.c_ssize_t
U.ScreenToClient.argtypes = [ctypes.c_void_p, ctypes.POINTER(ui.wt.POINT)]
U.GetClientRect.argtypes = [ctypes.c_void_p, ctypes.POINTER(ui.wt.RECT)]
U.GetDpiForWindow.argtypes = [ctypes.c_void_p]
U.GetGUIThreadInfo.argtypes = [ctypes.c_uint32, ctypes.POINTER(ui.GTI)]
DOWN, UP, MOVE, CANCEL = 0x201, 0x202, 0x200, 0x1F


def location(acc, item):
    coords = [ctypes.c_long() for _ in range(4)]
    hr = acc._m(22, *([ctypes.POINTER(ctypes.c_long)] * 4), ui.VARIANT)(
        acc.ptr, *(ctypes.byref(c) for c in coords), ui.child(item))
    assert hr == 0, "Could not read test widget geometry"
    return tuple(c.value for c in coords)


def center(hwnd, acc, item):
    x, y, w, h = location(acc, item)
    point = ui.wt.POINT(x + w // 2, y + h // 2)
    assert U.ScreenToClient(hwnd, ctypes.byref(point)), "Could not map test widget geometry"
    return point.x, point.y


def mouse(hwnd, message, point, buttons=None):
    x, y = point
    if buttons is None:
        buttons = 1 if message in (DOWN, MOVE) else 0
    U.SendMessageW(hwnd, message, buttons,
                   ((y & 0xFFFF) << 16) | (x & 0xFFFF))


def capture(hwnd):
    info = ui.GTI()
    info.cbSize = ctypes.sizeof(info)
    tid = U.GetWindowThreadProcessId(hwnd, None)
    assert U.GetGUIThreadInfo(tid, ctypes.byref(info)), "Could not read test capture state"
    return info.hwndCapture


def state(acc, item):
    value = ui.VARIANT()
    acc._m(14, ui.VARIANT, ctypes.POINTER(ui.VARIANT))(acc.ptr, ui.child(item), ctypes.byref(value))
    return value.val


def main():
    build = ui.BUILD.resolve()
    exe = build / "ScreenBuddy.exe"
    assert exe.is_file(), "ScreenBuddy test build missing"
    ui.OLE32.CoInitializeEx(None, 2)
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    checks = 0
    with tempfile.TemporaryDirectory(prefix="native-mouse-", dir=build, ignore_cleanup_errors=True) as folder:
        config = Path(folder) / "config.toml"
        config.write_text('[display]\noutput = "serial"\nbrightness = 80\n'
                          f'[ui]\nport = {port}\nupdate_check = false\nsetup_done = false\n'
                          'language = "de"\nsettings_theme = "hell"\n', encoding="utf-8")
        process = subprocess.Popen(
            [str(exe), "--run", "--no-lcd", "--settings", "--config", str(config),
             "--log", str(Path(folder) / "test.log")],
            env=dict(os.environ, FUCHSLABS_INSTANCE=f"-native-mouse-{port}"),
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            frames = []

            def ready():
                frames[:] = [h for h, shown in ui.windows(process.pid, "ScreenBuddyNativeSettings") if shown]
                return bool(frames)

            ui.wait(ready, 15, "Isolated native mouse test did not start")
            frame = frames[0]
            content = ui.content_window(frame)
            assert content, "Native content window missing"
            nav = ui.Acc(frame)

            def selected(name):
                return bool(state(nav, nav.find(name, ui.ROLE_PAGETAB, exact=True)) & 2)

            def nav_point(name):
                return center(frame, nav, nav.find(name, ui.ROLE_PAGETAB, exact=True))

            def click(hwnd, point):
                mouse(hwnd, DOWN, point)
                mouse(hwnd, UP, point)

            assert selected("Übersicht"), "Unexpected initial test page"
            point = nav_point("Anzeige")
            mouse(frame, DOWN, point)
            assert capture(frame) == frame, "Mouse down did not capture the native frame"
            assert selected("Übersicht"), "Navigation activated before mouse release"
            mouse(frame, UP, point)
            ui.wait(lambda: selected("Anzeige"), 3, "Mouse release did not activate navigation")
            assert not capture(frame), "Navigation retained mouse capture"
            checks += 5

            # Releasing outside or losing capture cancels the pending navigation.
            point = nav_point("Seiten & Reihenfolge")
            mouse(frame, DOWN, point)
            mouse(frame, UP, (-1, -1))
            assert selected("Anzeige"), "Release outside activated navigation"
            mouse(frame, DOWN, point)
            U.SendMessageW(frame, CANCEL, 0, 0)
            assert not capture(frame), "Cancel mode did not release capture"
            mouse(frame, UP, point)
            assert selected("Anzeige"), "Lost capture activated navigation"
            checks += 3

            # A real scrollbar thumb drag stops on mouse up and permits later clicks.
            click(frame, nav_point("Companion"))
            assert selected("Companion"), "Second mouse navigation failed"
            body = ui.Acc(content)
            toggle = body.find("Companion zusätzlich aktivieren", exact=True)
            before_y = location(body, toggle)[1]
            rect = ui.wt.RECT()
            U.GetClientRect(content, ctypes.byref(rect))
            scale = U.GetDpiForWindow(content) / 96
            thumb = (rect.right - round(6 * scale), round(8 * scale))
            mouse(content, DOWN, thumb)
            assert capture(content) == content, "Scrollbar did not capture the native content"
            mouse(content, MOVE, (thumb[0], thumb[1] + round(80 * scale)))
            dragged_y = location(body, toggle)[1]
            assert dragged_y < before_y, "Scrollbar thumb did not scroll the page"
            mouse(content, UP, (thumb[0], thumb[1] + round(80 * scale)))
            assert not capture(content), "Scrollbar retained capture after release"
            mouse(content, MOVE, (thumb[0], thumb[1] + round(160 * scale)), buttons=0)
            assert location(body, toggle)[1] == dragged_y, "Scrollbar continued dragging after release"
            checks += 5

            body.focus(toggle)  # Reveal the control without changing its value.
            click(content, center(content, body, toggle))
            save = nav.find("Speichern & anwenden", ui.ROLE_PUSHBUTTON)
            ui.wait(lambda: "(1 Änderung)" in nav.name(save), 3, "Content mouse click did not change the form")
            discard = nav.find("Verwerfen", ui.ROLE_PUSHBUTTON)
            click(frame, center(frame, nav, discard))
            ui.wait(lambda: "(0 " in nav.name(save), 3, "Mouse release did not activate the discard button")
            assert not state(ui.Acc(content), ui.Acc(content).find("Companion zusätzlich aktivieren", exact=True)) & 16, \
                "Mouse discard did not restore the toggle"
            checks += 3
            print(f"native mouse: {checks} checks passed (navigation, release outside, capture loss, thumb drag, discard)")
        finally:
            ui.quit_instance(port)
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=5)


if __name__ == "__main__":
    main()
