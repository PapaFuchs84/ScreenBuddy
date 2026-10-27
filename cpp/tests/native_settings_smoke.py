"""Smoke-test the native ScreenBuddy settings window in a test instance.

The app uses --no-lcd, a temporary config and a free localhost port.
No hardware or production installation is touched.

Checks (via window classes, MSAA/IAccessible and posted keys – no mouse, no foreground needed):
  * window and content area appear, every navigation entry has an accessible name
  * navigation, keyboard focus and slider keys; dirty counter on the save button
  * "Verwerfen" restores the value; "Speichern & anwenden" writes config.toml and restarts the test instance
  * plugin list loads a seeded example plugin; enabling/disabling it works without a consent prompt (no hosts)

Build directory: argument 1 or SCREENBUDDY_BUILD (default build/screenbuddy-release).
"""
import ctypes
import ctypes.wintypes as wt
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]
BUILD = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(os.environ.get("SCREENBUDDY_BUILD", ROOT / "build" / "screenbuddy-release"))
USER32 = ctypes.WinDLL("user32", use_last_error=True)
OLEACC = ctypes.WinDLL("oleacc")
OLEAUT = ctypes.WinDLL("oleaut32")
OLE32 = ctypes.WinDLL("ole32")
CALLBACK = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_ssize_t)
USER32.EnumWindows.argtypes = [CALLBACK, ctypes.c_ssize_t]
USER32.EnumChildWindows.argtypes = [ctypes.c_void_p, CALLBACK, ctypes.c_ssize_t]
USER32.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong)]
USER32.GetClassNameW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_int]
USER32.IsWindowVisible.argtypes = [ctypes.c_void_p]
USER32.GetWindowTextW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_int]
USER32.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_size_t, ctypes.c_ssize_t]
OLEAUT.SysFreeString.argtypes = [ctypes.c_void_p]

ROLE_SLIDER, ROLE_LIST, ROLE_PAGETAB, ROLE_PUSHBUTTON, ROLE_COMBOBOX = 0x33, 0x21, 0x25, 0x2B, 0x2E
NAV = ["Übersicht", "Anzeige", "Seiten & Reihenfolge", "System", "Freunde", "Discord", "Battlefield 6", "Blogs (RSS)",
       "Home Assistant", "Companion", "Plugins", "Farben & Design", "Updates & Autostart"]


class GUID(ctypes.Structure):
    _fields_ = [("d1", ctypes.c_uint32), ("d2", ctypes.c_uint16), ("d3", ctypes.c_uint16), ("d4", ctypes.c_ubyte * 8)]


class VARIANT(ctypes.Structure):
    _fields_ = [("vt", ctypes.c_uint16), ("r1", ctypes.c_uint16), ("r2", ctypes.c_uint16), ("r3", ctypes.c_uint16),
                ("val", ctypes.c_int64), ("pad", ctypes.c_int64)]


def child(i):
    v = VARIANT(); v.vt = 3; v.val = i  # VT_I4
    return v


IID_IAccessible = GUID(0x618736E0, 0x3C3D, 0x11CF, (ctypes.c_ubyte * 8)(0x81, 0x0C, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71))


class Acc:
    """Minimal IAccessible client (vtable calls)."""
    def __init__(self, hwnd):
        self.ptr = ctypes.c_void_p()
        hr = OLEACC.AccessibleObjectFromWindow(ctypes.c_void_p(hwnd), ctypes.c_long(-4), ctypes.byref(IID_IAccessible), ctypes.byref(self.ptr))
        assert hr == 0 and self.ptr.value, f"AccessibleObjectFromWindow failed: {hr:#x}"

    def _m(self, index, *argtypes):
        vtbl = ctypes.cast(self.ptr, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
        return ctypes.WINFUNCTYPE(ctypes.c_long, ctypes.c_void_p, *argtypes)(vtbl[index])

    def count(self):
        n = ctypes.c_long()
        self._m(8, ctypes.POINTER(ctypes.c_long))(self.ptr, ctypes.byref(n))
        return n.value

    def _bstr(self, index, i):
        b = ctypes.c_void_p()
        self._m(index, VARIANT, ctypes.POINTER(ctypes.c_void_p))(self.ptr, child(i), ctypes.byref(b))
        if not b.value:
            return ""
        s = ctypes.wstring_at(b.value)
        OLEAUT.SysFreeString(b)
        return s

    def name(self, i): return self._bstr(10, i)
    def value(self, i): return self._bstr(11, i)

    def role(self, i):
        v = VARIANT()
        self._m(13, VARIANT, ctypes.POINTER(VARIANT))(self.ptr, child(i), ctypes.byref(v))
        return v.val

    def focus(self, i): return self._m(21, ctypes.c_long, VARIANT)(self.ptr, 1, child(i))  # SELFLAG_TAKEFOCUS
    def do(self, i): return self._m(25, VARIANT)(self.ptr, child(i))

    def items(self):
        return [(i, self.name(i), self.role(i)) for i in range(1, self.count() + 1)]

    def find(self, text, role=None, exact=False):
        for i, n, r in self.items():
            if (n == text if exact else text in n) and (role is None or r == role):
                return i
        raise AssertionError(f"accessible element not found: {text!r} in {[n for _, n, _ in self.items()]}")


def windows(pid, cls):
    found = []
    @CALLBACK
    def visit(hwnd, _):
        owner = ctypes.c_ulong()
        USER32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid:
            name = ctypes.create_unicode_buffer(128)
            USER32.GetClassNameW(hwnd, name, len(name))
            if name.value == cls:
                found.append((hwnd, bool(USER32.IsWindowVisible(hwnd))))
        return True
    ctypes.set_last_error(0)
    if not USER32.EnumWindows(visit, 0):
        err = ctypes.get_last_error()
        if err != 0:
            raise ctypes.WinError(err)
    return found


def content_window(frame):
    found = []
    @CALLBACK
    def visit(hwnd, _):
        name = ctypes.create_unicode_buffer(128)
        USER32.GetClassNameW(hwnd, name, len(name))
        if name.value == "ScreenBuddySettingsContent":
            found.append(hwnd)
        return True
    USER32.EnumChildWindows(frame, visit, 0)
    return found[0] if found else None


class GTI(ctypes.Structure):
    _fields_ = [("cbSize", ctypes.c_uint32), ("flags", ctypes.c_uint32), ("hwndActive", ctypes.c_void_p), ("hwndFocus", ctypes.c_void_p),
                ("hwndCapture", ctypes.c_void_p), ("hwndMenuOwner", ctypes.c_void_p), ("hwndMoveSize", ctypes.c_void_p),
                ("hwndCaret", ctypes.c_void_p), ("rcCaret", wt.RECT)]


def key(frame, vk):
    """Post a key to the window that has keyboard focus in the app's UI thread."""
    tid = USER32.GetWindowThreadProcessId(frame, None)
    info = GTI(); info.cbSize = ctypes.sizeof(GTI)
    USER32.GetGUIThreadInfo.argtypes = [ctypes.c_uint32, ctypes.POINTER(GTI)]
    target = info.hwndFocus if USER32.GetGUIThreadInfo(tid, ctypes.byref(info)) and info.hwndFocus else frame
    USER32.PostMessageW(target, 0x100, vk, 0)
    USER32.PostMessageW(target, 0x101, vk, 0)
    time.sleep(0.4)


def wait(predicate, seconds, message):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            if predicate():
                return
        except (AssertionError, OSError):
            pass
        time.sleep(0.2)
    raise AssertionError(message)


def quit_instance(port):
    request = Request(f"http://127.0.0.1:{port}/api/quit", data=b"{}", method="POST",
                      headers={"X-FL": "1", "Content-Type": "application/json"})
    try:
        with urlopen(request, timeout=2):
            pass
    except OSError:
        pass


def port_open(port):
    with socket.socket() as s:
        s.settimeout(0.3)
        return s.connect_ex(("127.0.0.1", port)) == 0


def main():
    exe = BUILD / "ScreenBuddy.exe"
    if not exe.is_file():
        raise SystemExit(f"Missing build: {exe}")
    OLE32.CoInitializeEx(None, 2)
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    checks = 0
    with tempfile.TemporaryDirectory(prefix="native-smoke-", dir=BUILD, ignore_cleanup_errors=True) as folder:
        config = Path(folder) / "config.toml"
        config.write_text(f'[display]\noutput = "serial"\nbrightness = 80\n[ui]\nport = {port}\nupdate_check = false\n'
                          f'setup_done = false\nlanguage = "de"\nsettings_theme = "hell"\n', encoding="utf-8")
        (Path(folder) / "plugins").mkdir()
        shutil.copy(ROOT / "examples" / "plugins" / "offline-clock.flplugin.json", Path(folder) / "plugins")
        env = dict(os.environ, FUCHSLABS_INSTANCE=f"-native-smoke-{port}")
        process = subprocess.Popen(
            [str(exe), "--run", "--no-lcd", "--settings", "--config", str(config)],
            env=env, creationflags=subprocess.CREATE_NO_WINDOW)
        restarted = False
        try:
            deadline = time.monotonic() + 15
            frame = None
            while time.monotonic() < deadline and not frame:
                found = [h for h, visible in windows(process.pid, "ScreenBuddyNativeSettings") if visible]
                frame = found[0] if found else None
                if process.poll() is not None:
                    break
                time.sleep(0.1)
            assert frame, "Native settings window did not become visible"
            content = content_window(frame)
            assert content, "Content area (ScreenBuddySettingsContent) missing"
            checks += 2
            time.sleep(2.0)

            nav = Acc(frame)
            tabs = [n for _, n, r in nav.items() if r == ROLE_PAGETAB]
            assert tabs == NAV, f"Navigation entries differ: {tabs}"
            save_id = nav.find("Speichern & anwenden", ROLE_PUSHBUTTON)
            assert "(0 " in nav.name(save_id), nav.name(save_id)
            checks += 2

            # Plugins: seeded example appears, enabling works (no hosts -> no consent prompt)
            nav.do(nav.find("Plugins", ROLE_PAGETAB, exact=True))
            def select_first_plugin():
                body = Acc(content)
                plugin_list = body.find("Installierte Plugins", ROLE_LIST)
                body.focus(plugin_list)
                key(frame, 0x28)  # Down: select the first plugin
                return "Uhr / Clock" in body.value(plugin_list)
            wait(select_first_plugin, 8, "plugin list not loaded")
            body = Acc(content)
            plugin_list = body.find("Installierte Plugins", ROLE_LIST)
            value = body.value(plugin_list)
            assert "Uhr / Clock" in value, value
            was_enabled = "· aktiv" in value
            expected = "· aus" if was_enabled else "· aktiv"
            body.do(body.find("Aktivieren / deaktivieren", ROLE_PUSHBUTTON))
            wait(lambda: expected in Acc(content).value(Acc(content).find("Installierte Plugins", ROLE_LIST)), 8, "plugin state did not change")
            checks += 2

            # Phase 2: the former placeholder pages are native now (controls exist, no browser hand-over)
            for page, control, role in (("Discord", "Mitglieder neu laden", ROLE_PUSHBUTTON),
                                        ("Freunde", "Verknüpfen", ROLE_PUSHBUTTON),
                                        ("Home Assistant", "Verbindung prüfen & Entitäten laden", ROLE_PUSHBUTTON),
                                        ("Companion", "Kopplungscode anzeigen", ROLE_PUSHBUTTON),
                                        ("Farben & Design", "Zurücksetzen", ROLE_PUSHBUTTON),
                                        ("Updates & Autostart", "Hersteller-App beenden", ROLE_PUSHBUTTON)):
                nav.do(nav.find(page, ROLE_PAGETAB, exact=True))
                def present():
                    try:
                        Acc(content).find(control, role)
                        return True
                    except AssertionError:
                        return False
                wait(present, 6, f"{page}: '{control}' missing")
                assert "Vorerst im Browser-Fenster" not in " ".join(n for _, n, _r in Acc(content).items()), page
            checks += 6

            # Native first-run links and touch-assignment entry. Never launch the Windows wizard in this test.
            nav.do(nav.find("Übersicht", ROLE_PAGETAB, exact=True))
            body = Acc(content)
            for step in ("1 · Ausgabe wählen", "2 · Datenquellen einrichten", "3 · Seiten wählen"):
                body.find(step, ROLE_PUSHBUTTON)
            checks += 3

            nav.do(nav.find("Anzeige", ROLE_PAGETAB, exact=True))
            body = Acc(content)
            output = body.find("Display-Typ", ROLE_COMBOBOX)
            body.focus(output)
            key(frame, 0x27)  # USB -> monitor, only in this isolated --no-lcd configuration
            body = Acc(content)
            monitor = body.find("Bildschirm", ROLE_COMBOBOX, exact=True)
            assert "Automatisch" in body.value(monitor), body.value(monitor)
            body.do(body.find("Bildschirme neu suchen", ROLE_PUSHBUTTON))  # Enumerate only; do not select or open a display.
            body = Acc(content)
            assert body.find("Bildschirm", ROLE_COMBOBOX, exact=True)
            body.find("Windows-Assistent öffnen", ROLE_PUSHBUTTON)
            body.find("Erneut prüfen", ROLE_PUSHBUTTON)
            nav.do(nav.find("Verwerfen", ROLE_PUSHBUTTON))
            time.sleep(0.6)
            checks += 4

            # Display page: slider via keyboard, dirty counter, discard
            nav.do(nav.find("Anzeige", ROLE_PAGETAB, exact=True))
            time.sleep(0.6)
            body = Acc(content)
            slider = body.find("Helligkeit", ROLE_SLIDER)
            assert body.value(slider) == "80 %", body.value(slider)
            body.focus(slider)
            key(frame, 0x27)  # Right
            assert body.value(slider) == "85 %", body.value(slider)
            assert "(1 Änderung)" in nav.name(save_id), nav.name(save_id)
            nav.do(nav.find("Verwerfen", ROLE_PUSHBUTTON))
            time.sleep(0.6)
            body = Acc(content)
            slider = body.find("Helligkeit", ROLE_SLIDER)
            assert body.value(slider) == "80 %", "discard did not restore the value"
            assert "(0 " in nav.name(save_id), nav.name(save_id)
            checks += 3

            # Live language switch in the header (DE -> EN), counted as a change, discarded again
            lang = nav.find("Sprache", exact=True)
            nav.focus(lang)
            key(frame, 0x27)  # Right: EN
            title = ctypes.create_unicode_buffer(128)
            USER32.GetWindowTextW(ctypes.c_void_p(frame), title, 128)
            assert title.value == "ScreenBuddy – Settings", title.value
            assert "Display" in [n for _, n, r in nav.items() if r == ROLE_PAGETAB], "navigation not translated"
            assert "(1 change)" in nav.name(save_id), nav.name(save_id)
            nav.do(nav.find("Discard", ROLE_PUSHBUTTON))
            time.sleep(0.6)
            USER32.GetWindowTextW(ctypes.c_void_p(frame), title, 128)
            assert title.value == "ScreenBuddy – Einstellungen", title.value
            checks += 2

            # Save & apply: config written, test instance restarts with the window open
            body.focus(slider)
            key(frame, 0x27)
            assert "(1 Änderung)" in nav.name(save_id)
            nav.do(save_id)
            wait(lambda: "brightness = 85" in config.read_text(encoding="utf-8"), 10, "config.toml not written")
            restarted = True
            text = config.read_text(encoding="utf-8")
            assert 'settings_theme = "hell"' in text and "setup_done = true" in text, "ui section not saved"
            process.wait(timeout=15)
            wait(lambda: port_open(port), 20, "restarted test instance did not come back")
            checks += 3
            print(f"native settings: {checks} checks passed (window, MSAA names, keyboard, dirty/discard, language, save+restart, plugins)")
        finally:
            quit_instance(port)
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=5)
            if restarted:  # the restarted instance is detached; it quits through its own API
                end = time.monotonic() + 15
                while port_open(port) and time.monotonic() < end:
                    quit_instance(port)
                    time.sleep(0.5)
                time.sleep(1.0)


if __name__ == "__main__":
    main()
