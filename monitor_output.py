# © 2026 FuchsLabs
"""Ausgabe auf einen normalen Bildschirm (HDMI/USB-C, z. B. Leisten mit 1920 × 720): randloses Vollbildfenster.

Gleiche Schnittstelle wie display_driver.UsbDisplay (size, connect, show, invalidate, screen_off, close), nur mit
Windows-API per ctypes – kein Zusatzpaket. Touch: Windows meldet Fingertipps an Fenster ohne eigene Touch-Behandlung
zusätzlich als Mausklicks; daraus werden Wischen (links/rechts) und Tippen auf den linken/rechten Rand.
"""
from __future__ import annotations

import ctypes
import ctypes.wintypes as wt
import logging
import threading
import time
from typing import Callable

from PIL import Image

log = logging.getLogger("monitor")

user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

WM_DESTROY, WM_PAINT, WM_CLOSE, WM_ERASEBKGND, WM_SETCURSOR = 0x0002, 0x000F, 0x0010, 0x0014, 0x0020
WM_MOUSEACTIVATE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_APP = 0x0021, 0x0201, 0x0202, 0x8000
WM_FRAME, WM_QUIT_WINDOW = WM_APP + 1, WM_APP + 2
WS_POPUP, WS_VISIBLE = 0x80000000, 0x10000000
WS_EX_TOOLWINDOW, WS_EX_NOACTIVATE = 0x00000080, 0x08000000
MA_NOACTIVATE, SW_HIDE, SW_SHOWNOACTIVATE = 3, 0, 4
SM_DIGITIZER, SM_MAXIMUMTOUCHES = 94, 95
NID_INTEGRATED_TOUCH, NID_EXTERNAL_TOUCH, NID_READY = 0x01, 0x02, 0x80
MONITORINFOF_PRIMARY = 1

LRESULT = ctypes.c_ssize_t
WNDPROC = ctypes.WINFUNCTYPE(LRESULT, wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM)
MONITORENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HMONITOR, wt.HDC, ctypes.POINTER(wt.RECT), wt.LPARAM)


class WNDCLASSW(ctypes.Structure):
    _fields_ = [("style", ctypes.c_uint), ("lpfnWndProc", WNDPROC), ("cbClsExtra", ctypes.c_int),
                ("cbWndExtra", ctypes.c_int), ("hInstance", wt.HINSTANCE), ("hIcon", wt.HICON),
                ("hCursor", wt.HANDLE), ("hbrBackground", wt.HBRUSH), ("lpszMenuName", wt.LPCWSTR),
                ("lpszClassName", wt.LPCWSTR)]


class MONITORINFOEXW(ctypes.Structure):
    _fields_ = [("cbSize", wt.DWORD), ("rcMonitor", wt.RECT), ("rcWork", wt.RECT), ("dwFlags", wt.DWORD),
                ("szDevice", ctypes.c_wchar * 32)]


class DISPLAY_DEVICEW(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("DeviceName", ctypes.c_wchar * 32), ("DeviceString", ctypes.c_wchar * 128),
                ("StateFlags", wt.DWORD), ("DeviceID", ctypes.c_wchar * 128), ("DeviceKey", ctypes.c_wchar * 128)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD),
                ("biBitCount", wt.WORD), ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]


class PAINTSTRUCT(ctypes.Structure):
    _fields_ = [("hdc", wt.HDC), ("fErase", wt.BOOL), ("rcPaint", wt.RECT), ("fRestore", wt.BOOL),
                ("fIncUpdate", wt.BOOL), ("rgbReserved", ctypes.c_byte * 32)]


user32.DefWindowProcW.restype = LRESULT
user32.DefWindowProcW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
user32.CreateWindowExW.restype = wt.HWND
user32.CreateWindowExW.argtypes = [wt.DWORD, wt.LPCWSTR, wt.LPCWSTR, wt.DWORD, ctypes.c_int, ctypes.c_int,
                                   ctypes.c_int, ctypes.c_int, wt.HWND, wt.HMENU, wt.HINSTANCE, wt.LPVOID]
user32.RegisterClassW.argtypes = [ctypes.POINTER(WNDCLASSW)]
user32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
user32.InvalidateRect.argtypes = [wt.HWND, wt.LPVOID, wt.BOOL]
user32.BeginPaint.restype = wt.HDC
user32.BeginPaint.argtypes = [wt.HWND, ctypes.POINTER(PAINTSTRUCT)]
user32.EndPaint.argtypes = [wt.HWND, ctypes.POINTER(PAINTSTRUCT)]
user32.ShowWindow.argtypes = [wt.HWND, ctypes.c_int]
user32.DestroyWindow.argtypes = [wt.HWND]
user32.SetCursor.argtypes = [wt.HANDLE]
user32.EnumDisplayMonitors.argtypes = [wt.HDC, wt.LPVOID, MONITORENUMPROC, wt.LPARAM]
user32.GetMonitorInfoW.argtypes = [wt.HMONITOR, ctypes.POINTER(MONITORINFOEXW)]
user32.EnumDisplayDevicesW.argtypes = [wt.LPCWSTR, wt.DWORD, ctypes.POINTER(DISPLAY_DEVICEW), wt.DWORD]
gdi32.SetDIBitsToDevice.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, wt.DWORD, wt.DWORD, ctypes.c_int,
                                    ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p,
                                    ctypes.POINTER(BITMAPINFOHEADER), ctypes.c_uint]
kernel32.GetModuleHandleW.restype = wt.HMODULE


def dpi_aware() -> None:
    """Echte Pixel statt Windows-Skalierung (sonst wird 1920 × 720 bei 125 % als 1536 × 576 gemeldet).
    Muss vor dem ersten Fenster des Prozesses laufen."""
    try:
        user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))  # Per-Monitor v2 (ab Windows 10 1703)
    except (AttributeError, OSError):
        try:
            ctypes.WinDLL("shcore").SetProcessDpiAwareness(2)
        except (AttributeError, OSError):
            pass


def list_monitors() -> list[dict]:
    """Alle angeschlossenen Bildschirme: device (z. B. \\\\.\\DISPLAY2), Lage/Größe in Pixeln, primary, name."""
    out: list[dict] = []

    def cb(hmon, _hdc, _rect, _data):
        mi = MONITORINFOEXW()
        mi.cbSize = ctypes.sizeof(MONITORINFOEXW)
        if user32.GetMonitorInfoW(hmon, ctypes.byref(mi)):
            r = mi.rcMonitor
            dd = DISPLAY_DEVICEW()
            dd.cb = ctypes.sizeof(DISPLAY_DEVICEW)
            name = dd.DeviceString if user32.EnumDisplayDevicesW(mi.szDevice, 0, ctypes.byref(dd), 0) else ""
            out.append({"device": mi.szDevice, "name": name, "x": r.left, "y": r.top,
                        "w": r.right - r.left, "h": r.bottom - r.top,
                        "primary": bool(mi.dwFlags & MONITORINFOF_PRIMARY)})
        return True

    user32.EnumDisplayMonitors(None, None, MONITORENUMPROC(cb), 0)
    return out


def pick_monitor(device: str = "") -> dict | None:
    """Gewählter Bildschirm; ohne Wahl der erste, der nicht Hauptbildschirm ist (dort läuft ja Windows)."""
    mons = list_monitors()
    if device:
        return next((m for m in mons if m["device"].casefold() == device.casefold()), None)
    return next((m for m in mons if not m["primary"]), None)


def touch_info() -> dict:
    """Meldet Windows einen Touchscreen? (Gilt für den ganzen PC; welcher Bildschirm, weiß Windows hier nicht.)"""
    v = user32.GetSystemMetrics(SM_DIGITIZER)
    ok = bool(v & NID_READY) and bool(v & (NID_INTEGRATED_TOUCH | NID_EXTERNAL_TOUCH))
    return {"available": ok, "points": user32.GetSystemMetrics(SM_MAXIMUMTOUCHES) if ok else 0}


class Gesture:
    """Wischen nach links = nächste Seite, nach rechts = vorige; Tippen links (unteres Drittel der Breite) = vorige,
    sonst nächste. Rückgabe von end(): +1, -1 oder 0."""

    def __init__(self, width: int, height: int):
        self.w, self.h = width, height
        self.start: tuple[int, int, float] | None = None

    def down(self, x: int, y: int) -> None:
        self.start = (x, y, time.monotonic())

    def end(self, x: int, y: int) -> int:
        if self.start is None:
            return 0
        x0, y0, t0 = self.start
        self.start = None
        dx, dy = x - x0, y - y0
        thr = max(30, self.w * 0.05)
        if abs(dx) >= thr and abs(dx) > abs(dy):
            return 1 if dx < 0 else -1
        if abs(dx) < thr and abs(dy) < thr and time.monotonic() - t0 < 1.0:
            return -1 if x < self.w / 3 else 1
        return 0


class MonitorDisplay:
    def __init__(self, device: str = "", touch: bool = False, on_step: Callable[[int], None] | None = None,
                 rect: tuple[int, int, int, int] | None = None):
        """rect = (x, y, w, h) statt eines ganzen Bildschirms (nur für Tests)."""
        self.device, self.touch, self.on_step = device, touch, on_step
        if rect:
            self.mon = {"device": "Test", "x": rect[0], "y": rect[1], "w": rect[2], "h": rect[3], "primary": False}
        else:
            self.mon = pick_monitor(device)
        self.hwnd = None
        self._bits: bytes | None = None
        self._lock = threading.Lock()
        self._ready = threading.Event()
        self._thread: threading.Thread | None = None
        self._proc = WNDPROC(self._wndproc)  # Referenz halten, sonst räumt der GC den Callback ab
        self._gesture = Gesture(*self.size)
        bmi = BITMAPINFOHEADER()
        bmi.biSize, bmi.biPlanes, bmi.biBitCount = ctypes.sizeof(BITMAPINFOHEADER), 1, 32
        bmi.biWidth, bmi.biHeight = self.size[0], -self.size[1]  # negativ = Zeilen von oben nach unten
        self._bmi = bmi

    @property
    def size(self) -> tuple[int, int]:
        return (self.mon["w"], self.mon["h"]) if self.mon else (480, 320)

    # ---------- Fenster ----------
    def connect(self) -> None:
        if self.mon is None:
            raise RuntimeError("Bildschirm nicht gefunden" if self.device else "Kein zweiter Bildschirm angeschlossen")
        if self._thread and self._thread.is_alive():
            user32.ShowWindow(self.hwnd, SW_SHOWNOACTIVATE)
            return
        self._ready.clear()
        self._thread = threading.Thread(target=self._run, name="monitor", daemon=True)
        self._thread.start()
        if not self._ready.wait(5) or not self.hwnd:
            raise RuntimeError("Anzeigefenster konnte nicht geöffnet werden")
        m = self.mon
        log.info("Ausgabe auf Bildschirm %s (%d × %d bei %d/%d)", m["device"], m["w"], m["h"], m["x"], m["y"])

    def _run(self) -> None:
        try:
            hinst = kernel32.GetModuleHandleW(None)
            wc = WNDCLASSW()
            wc.lpfnWndProc, wc.hInstance, wc.lpszClassName = self._proc, hinst, "FuchsLabsUsbDisplayScreen"
            user32.RegisterClassW(ctypes.byref(wc))
            m = self.mon
            # Randlos, ohne Taskleisten-Eintrag und ohne Fokus zu stehlen (Tippen aktiviert das Fenster nicht)
            self.hwnd = user32.CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName,
                                               "FuchsLabs USB-Display", WS_POPUP, m["x"], m["y"], m["w"], m["h"],
                                               None, None, hinst, None)
            if not self.hwnd:
                log.warning("Fenster nicht erstellt (Fehler %d)", ctypes.get_last_error())
                return
            user32.ShowWindow(self.hwnd, SW_SHOWNOACTIVATE)
            self._ready.set()
            msg = wt.MSG()
            while user32.GetMessageW(ctypes.byref(msg), None, 0, 0) > 0:
                user32.TranslateMessage(ctypes.byref(msg))
                user32.DispatchMessageW(ctypes.byref(msg))
        except Exception:
            log.exception("Anzeigefenster abgestürzt")
        finally:
            self.hwnd = None
            self._ready.set()

    def _wndproc(self, hwnd, msg, wparam, lparam):
        try:
            if msg == WM_PAINT:
                ps = PAINTSTRUCT()
                hdc = user32.BeginPaint(hwnd, ctypes.byref(ps))
                with self._lock:
                    if self._bits:
                        w, h = self.size
                        gdi32.SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, self._bits, ctypes.byref(self._bmi), 0)
                user32.EndPaint(hwnd, ctypes.byref(ps))
                return 0
            if msg == WM_ERASEBKGND:
                return 1  # kein weißes Aufblitzen zwischen den Bildern
            if msg == WM_SETCURSOR:
                user32.SetCursor(None)  # Mauszeiger über der Anzeige ausblenden
                return 1
            if msg == WM_MOUSEACTIVATE:
                return MA_NOACTIVATE
            if msg in (WM_LBUTTONDOWN, WM_LBUTTONUP) and self.touch:
                x, y = ctypes.c_short(lparam & 0xFFFF).value, ctypes.c_short((lparam >> 16) & 0xFFFF).value
                if msg == WM_LBUTTONDOWN:
                    self._gesture.down(x, y)
                else:
                    step = self._gesture.end(x, y)
                    if step and self.on_step:
                        self.on_step(step)
                return 0
            if msg == WM_FRAME:
                user32.InvalidateRect(hwnd, None, False)
                return 0
            if msg == WM_CLOSE:
                return 0  # Alt+F4 schließt die Anzeige nicht; Beenden geht über das Tray-Symbol
            if msg == WM_QUIT_WINDOW:
                user32.DestroyWindow(hwnd)
                return 0
            if msg == WM_DESTROY:
                user32.PostQuitMessage(0)
                return 0
        except Exception:
            log.exception("Fehler im Anzeigefenster")
        return user32.DefWindowProcW(hwnd, msg, wparam, lparam)

    # ---------- Bilder ----------
    def show(self, frame: Image.Image) -> int:
        frame = frame.convert("RGB")
        assert frame.size == self.size, f"Frame {frame.size} != Bildschirm {self.size}"
        if not self.hwnd:
            raise RuntimeError("Anzeigefenster ist geschlossen")
        bits = frame.tobytes("raw", "BGRX")
        with self._lock:
            self._bits = bits
        user32.PostMessageW(self.hwnd, WM_FRAME, 0, 0)
        return frame.height

    def invalidate(self) -> None:
        pass  # jedes Bild wird ohnehin komplett gezeichnet

    def screen_off(self) -> None:
        if self.hwnd:
            user32.ShowWindow(self.hwnd, SW_HIDE)

    def close(self) -> None:
        if self.hwnd:
            user32.PostMessageW(self.hwnd, WM_QUIT_WINDOW, 0, 0)
            if self._thread:
                self._thread.join(2)
