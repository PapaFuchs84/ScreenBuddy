"""Smoke-test the native ScreenBuddy settings window in a test instance.

The app uses --no-lcd, a temporary config and a free localhost port.
No hardware or production installation is touched.
"""
import ctypes
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import time
from urllib.request import Request, urlopen

BUILD = Path(__file__).resolve().parents[1] / "build"
USER32 = ctypes.WinDLL("user32", use_last_error=True)
CALLBACK = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_ssize_t)
USER32.EnumWindows.argtypes = [CALLBACK, ctypes.c_ssize_t]
USER32.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong)]
USER32.GetClassNameW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_int]
USER32.IsWindowVisible.argtypes = [ctypes.c_void_p]

def native_window(pid):
    found = []
    @CALLBACK
    def visit(hwnd, _):
        owner = ctypes.c_ulong()
        USER32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid:
            name = ctypes.create_unicode_buffer(128)
            USER32.GetClassNameW(hwnd, name, len(name))
            if name.value == "ScreenBuddyNativeSettings":
                found.append(bool(USER32.IsWindowVisible(hwnd)))
        return True
    if not USER32.EnumWindows(visit, 0):
        raise ctypes.WinError(ctypes.get_last_error())
    return found

def main():
    exe = BUILD / "ScreenBuddy.exe"
    if not exe.is_file():
        raise SystemExit(f"Missing build: {exe}")
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="native-smoke-", dir=BUILD) as folder:
        config = Path(folder) / "config.toml"
        config.write_text(f'[display]\noutput = "serial"\n[ui]\nport = {port}\nupdate_check = false\nsetup_done = false\n', encoding="utf-8")
        env = dict(os.environ, FUCHSLABS_INSTANCE=f"-native-smoke-{port}")
        process = subprocess.Popen(
            [str(exe), "--run", "--no-lcd", "--settings", "--config", str(config)],
            env=env, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 12
            visible = False
            while time.monotonic() < deadline:
                windows = native_window(process.pid)
                if windows and any(windows):
                    visible = True
                    break
                if process.poll() is not None:
                    break
                time.sleep(0.1)
            assert visible, "Native settings window did not become visible"
            print("native settings window shown in an isolated no-lcd app")
        finally:
            request = Request(f"http://127.0.0.1:{port}/api/quit", data=b"{}", method="POST",
                              headers={"X-FL": "1", "Content-Type": "application/json"})
            try:
                with urlopen(request, timeout=2):
                    pass
            except OSError:
                pass
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=5)

if __name__ == "__main__":
    main()
