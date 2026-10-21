# © 2026 FuchsLabs
"""Tray-Symbol (Infobereich) ohne Zusatzpakete: Windows-API per ctypes.

Doppelklick oder Menü „Einstellungen“ öffnet die Einstellungsoberfläche; Rechtsklick zeigt das Menü.
"""
from __future__ import annotations

import ctypes
import ctypes.wintypes as wt
import logging
import tempfile
import threading
from pathlib import Path
from typing import Callable

import brand
from i18n import tr

log = logging.getLogger("tray")

user32 = ctypes.WinDLL("user32", use_last_error=True)
shell32 = ctypes.WinDLL("shell32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

WM_DESTROY, WM_COMMAND, WM_USER, WM_NULL = 0x0002, 0x0111, 0x0400, 0x0000
WM_LBUTTONDBLCLK, WM_RBUTTONUP = 0x0203, 0x0205
WM_TRAY = WM_USER + 20
NIM_ADD, NIM_DELETE, NIM_MODIFY = 0, 2, 1
NIF_MESSAGE, NIF_ICON, NIF_TIP = 1, 2, 4
MF_STRING, MF_SEPARATOR = 0, 0x800
TPM_RIGHTBUTTON, TPM_RETURNCMD = 0x0002, 0x0100
IMAGE_ICON, LR_LOADFROMFILE = 1, 0x0010
ID_OPEN, ID_RESTART, ID_QUIT = 1001, 1002, 1003

LRESULT = ctypes.c_ssize_t
WNDPROC = ctypes.WINFUNCTYPE(LRESULT, wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM)


class WNDCLASSW(ctypes.Structure):
    _fields_ = [("style", ctypes.c_uint), ("lpfnWndProc", WNDPROC), ("cbClsExtra", ctypes.c_int),
                ("cbWndExtra", ctypes.c_int), ("hInstance", wt.HINSTANCE), ("hIcon", wt.HICON),
                ("hCursor", wt.HANDLE), ("hbrBackground", wt.HBRUSH), ("lpszMenuName", wt.LPCWSTR),
                ("lpszClassName", wt.LPCWSTR)]


class NOTIFYICONDATAW(ctypes.Structure):
    _fields_ = [("cbSize", wt.DWORD), ("hWnd", wt.HWND), ("uID", ctypes.c_uint), ("uFlags", ctypes.c_uint),
                ("uCallbackMessage", ctypes.c_uint), ("hIcon", wt.HICON), ("szTip", ctypes.c_wchar * 128),
                ("dwState", wt.DWORD), ("dwStateMask", wt.DWORD), ("szInfo", ctypes.c_wchar * 256),
                ("uVersion", ctypes.c_uint), ("szInfoTitle", ctypes.c_wchar * 64), ("dwInfoFlags", wt.DWORD),
                ("guidItem", ctypes.c_byte * 16), ("hBalloonIcon", wt.HICON)]


user32.DefWindowProcW.restype = LRESULT
user32.DefWindowProcW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
user32.CreateWindowExW.restype = wt.HWND
user32.CreateWindowExW.argtypes = [wt.DWORD, wt.LPCWSTR, wt.LPCWSTR, wt.DWORD, ctypes.c_int, ctypes.c_int,
                                   ctypes.c_int, ctypes.c_int, wt.HWND, wt.HMENU, wt.HINSTANCE, wt.LPVOID]
user32.RegisterClassW.argtypes = [ctypes.POINTER(WNDCLASSW)]
user32.LoadImageW.restype = wt.HANDLE
user32.LoadImageW.argtypes = [wt.HINSTANCE, wt.LPCWSTR, ctypes.c_uint, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
user32.CreatePopupMenu.restype = wt.HMENU
user32.AppendMenuW.argtypes = [wt.HMENU, ctypes.c_uint, ctypes.c_size_t, wt.LPCWSTR]
user32.TrackPopupMenu.argtypes = [wt.HMENU, ctypes.c_uint, ctypes.c_int, ctypes.c_int, ctypes.c_int, wt.HWND,
                                  wt.LPVOID]
user32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
user32.DestroyMenu.argtypes = [wt.HMENU]
user32.SetForegroundWindow.argtypes = [wt.HWND]
shell32.Shell_NotifyIconW.argtypes = [wt.DWORD, ctypes.POINTER(NOTIFYICONDATAW)]
kernel32.GetModuleHandleW.restype = wt.HMODULE


def _make_icon() -> str:
    """FuchsLabs-Fuchs (Creme auf Orange) als .ico in den Temp-Ordner schreiben."""
    path = Path(tempfile.gettempdir()) / "fuchslabs-usb-display.ico"
    brand.save_ico(path)
    return str(path)


class Tray:
    def __init__(self, tip: str, on_open: Callable[[], None], on_restart: Callable[[], None],
                 on_quit: Callable[[], None]):
        self.tip, self.on_open, self.on_restart, self.on_quit = tip, on_open, on_restart, on_quit
        self.hwnd = None
        self._proc = WNDPROC(self._wndproc)  # Referenz halten, sonst räumt der GC den Callback ab
        self._nid: NOTIFYICONDATAW | None = None
        self._thread = threading.Thread(target=self._run, name="tray", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        if self.hwnd:
            user32.PostMessageW(self.hwnd, WM_DESTROY, 0, 0)

    def _wndproc(self, hwnd, msg, wparam, lparam):
        try:
            if msg == WM_TRAY:
                if lparam == WM_LBUTTONDBLCLK:
                    self.on_open()
                elif lparam == WM_RBUTTONUP:
                    self._menu(hwnd)
                return 0
            if msg == WM_DESTROY:
                if self._nid:
                    shell32.Shell_NotifyIconW(NIM_DELETE, ctypes.byref(self._nid))
                    self._nid = None
                user32.PostQuitMessage(0)
                return 0
        except Exception:
            log.exception("Tray-Fehler")
        return user32.DefWindowProcW(hwnd, msg, wparam, lparam)

    def _menu(self, hwnd) -> None:
        menu = user32.CreatePopupMenu()
        user32.AppendMenuW(menu, MF_STRING, ID_OPEN, tr("Einstellungen öffnen"))
        user32.AppendMenuW(menu, MF_STRING, ID_RESTART, tr("Neu starten"))
        user32.AppendMenuW(menu, MF_SEPARATOR, 0, None)
        user32.AppendMenuW(menu, MF_STRING, ID_QUIT, tr("Beenden"))
        pt = wt.POINT()
        user32.GetCursorPos(ctypes.byref(pt))
        user32.SetForegroundWindow(hwnd)  # sonst schließt das Menü nicht bei Klick daneben
        cmd = user32.TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, None)
        user32.PostMessageW(hwnd, WM_NULL, 0, 0)
        user32.DestroyMenu(menu)
        {ID_OPEN: self.on_open, ID_RESTART: self.on_restart, ID_QUIT: self.on_quit}.get(cmd, lambda: None)()

    def _run(self) -> None:
        try:
            hinst = kernel32.GetModuleHandleW(None)
            wc = WNDCLASSW()
            wc.lpfnWndProc, wc.hInstance, wc.lpszClassName = self._proc, hinst, "FuchsLabsUsbDisplayTray"
            user32.RegisterClassW(ctypes.byref(wc))
            self.hwnd = user32.CreateWindowExW(0, wc.lpszClassName, "FuchsLabs USB-Display", 0, 0, 0, 0, 0,
                                               None, None, hinst, None)
            px = user32.GetSystemMetrics(49) or 16  # SM_CXSMICON: passende Größe bei hoher Skalierung
            icon = user32.LoadImageW(None, _make_icon(), IMAGE_ICON, px, px, LR_LOADFROMFILE)
            nid = NOTIFYICONDATAW()
            nid.cbSize = ctypes.sizeof(NOTIFYICONDATAW)
            nid.hWnd, nid.uID, nid.uFlags = self.hwnd, 1, NIF_MESSAGE | NIF_ICON | NIF_TIP
            nid.uCallbackMessage, nid.hIcon, nid.szTip = WM_TRAY, icon, self.tip[:127]
            if not shell32.Shell_NotifyIconW(NIM_ADD, ctypes.byref(nid)):
                log.warning("Tray-Symbol konnte nicht angelegt werden")
                return
            self._nid = nid
            msg = wt.MSG()
            while user32.GetMessageW(ctypes.byref(msg), None, 0, 0) > 0:
                user32.TranslateMessage(ctypes.byref(msg))
                user32.DispatchMessageW(ctypes.byref(msg))
        except Exception:
            log.exception("Tray konnte nicht gestartet werden")
