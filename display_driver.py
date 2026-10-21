# © 2026 FuchsLabs
"""Treiber für das 3,5"-USB-Display (480 × 320, Hardware-Revision A).

Protokoll (öffentlich dokumentiert; Open-Source-Referenzimplementierungen nutzen dasselbe „Revision A“-Format):
  * USB-CDC-Serial, 115200 Baud
  * 6-Byte-Kommando: x/y/ex/ey als 10-Bit-Werte gepackt, Byte 5 = Befehl
  * Bilddaten als RGB565 little-endian direkt nach DISPLAY_BITMAP
"""
from __future__ import annotations

import logging
import threading
import time
from enum import IntEnum

import serial
from PIL import Image, ImageChops
from serial.tools.list_ports import comports

log = logging.getLogger("display")

NATIVE_W, NATIVE_H = 320, 480  # Panel ist nativ Hochformat
DEVICE_SERIAL = "USB35INCHIPSV2"
DEVICE_IDS = {(0x1A86, 0x5722), (0x1A86, 0xF111)}  # USB-Kennungen (VID, PID) dieser Displays


class Cmd(IntEnum):
    RESET = 101
    CLEAR = 102
    SCREEN_OFF = 108
    SCREEN_ON = 109
    SET_BRIGHTNESS = 110
    SET_ORIENTATION = 121
    DISPLAY_BITMAP = 197
    HELLO = 69


class Orientation(IntEnum):
    PORTRAIT = 0
    REVERSE_PORTRAIT = 1
    LANDSCAPE = 2
    REVERSE_LANDSCAPE = 3


def encode_command(cmd: int, x: int = 0, y: int = 0, ex: int = 0, ey: int = 0) -> bytes:
    return bytes((
        x >> 2,
        ((x & 3) << 6) + (y >> 4),
        ((y & 15) << 4) + (ex >> 6),
        ((ex & 63) << 2) + (ey >> 8),
        ey & 255,
        cmd,
    ))


# Nachschlagetabellen für RGB888 → RGB565 (rein mit Pillow, kein numpy nötig; ~2 ms pro Bild)
_T_R = [(v >> 3) << 3 for v in range(256)]
_T_GH = [v >> 5 for v in range(256)]
_T_GL = [((v >> 2) & 7) << 5 for v in range(256)]
_T_B = [v >> 3 for v in range(256)]


def to_rgb565le(img: Image.Image) -> bytes:
    r, g, b = img.convert("RGB").split()
    hi = ImageChops.add(r.point(_T_R), g.point(_T_GH))
    lo = ImageChops.add(g.point(_T_GL), b.point(_T_B))
    return Image.merge("LA", (lo, hi)).tobytes()  # je Pixel erst Low-, dann High-Byte = little-endian


def find_port() -> str | None:
    for p in comports():
        if p.serial_number == DEVICE_SERIAL or (p.vid, p.pid) in DEVICE_IDS:
            return p.device
    return None


class UsbDisplay:
    def __init__(self, port: str = "AUTO", orientation: Orientation = Orientation.LANDSCAPE,
                 brightness: int = 50, reset_on_start: bool = True, band_height: int = 8):
        self.port_cfg = port
        self.orientation = Orientation(orientation)
        self.brightness = brightness
        self.reset_on_start = reset_on_start
        self.band_height = band_height
        self.ser: serial.Serial | None = None
        self.last_frame: Image.Image | None = None
        self.lock = threading.Lock()

    @property
    def size(self) -> tuple[int, int]:
        if self.orientation in (Orientation.PORTRAIT, Orientation.REVERSE_PORTRAIT):
            return NATIVE_W, NATIVE_H
        return NATIVE_H, NATIVE_W

    # ---------- Verbindung ----------
    def _open(self, attempts: int = 15) -> None:
        for i in range(1, attempts + 1):
            port = find_port() if self.port_cfg.upper() == "AUTO" else self.port_cfg
            if port:
                try:
                    self.ser = serial.Serial(port, 115200, timeout=1, write_timeout=5, rtscts=True)
                    log.info("Display verbunden an %s", port)
                    return
                except serial.SerialException as e:
                    log.warning("Anschluss %s nicht zu öffnen (%s) – läuft noch die Hersteller-App des Displays? (%d/%d)",
                                port, e, i, attempts)
            else:
                log.warning("Display nicht gefunden (%d/%d)", i, attempts)
            time.sleep(1)
        raise RuntimeError("Display nicht erreichbar")

    def _close(self) -> None:
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    def _write(self, data: bytes) -> None:
        assert self.ser is not None
        self.ser.write(data)

    def connect(self) -> None:
        with self.lock:
            self._open()
            if self.reset_on_start:
                # Wie die Referenz: Reset, danach meldet sich das Gerät ggf. an neuem Port
                self._write(encode_command(Cmd.RESET))
                self._close()
                time.sleep(5)
                self._open()
            self._hello()
            self._write(encode_command(Cmd.SCREEN_ON))
            self._set_brightness(self.brightness)
            self._set_orientation()
            self.last_frame = None

    def _hello(self) -> None:
        # Dieses Display antwortet nicht; eine Antwort wird nur geloggt
        self._write(bytes([Cmd.HELLO] * 6))
        resp = self.ser.read(6)
        self.ser.reset_input_buffer()
        log.info("Display-Antwort: %s", resp.hex() or "(keine, normal für dieses Modell)")

    def _set_brightness(self, level: int) -> None:
        level = max(0, min(100, level))
        self._write(encode_command(Cmd.SET_BRIGHTNESS, int(255 - level / 100 * 255)))

    def _set_orientation(self) -> None:
        w, h = self.size
        buf = bytearray(16)
        buf[0:6] = encode_command(Cmd.SET_ORIENTATION)
        buf[6] = self.orientation + 100
        buf[7], buf[8] = w >> 8, w & 255
        buf[9], buf[10] = h >> 8, h & 255
        self._write(bytes(buf))

    def screen_off(self) -> None:
        with self.lock:
            if self.ser:
                self._write(encode_command(Cmd.SCREEN_OFF))

    def close(self) -> None:
        with self.lock:
            self._close()

    def invalidate(self) -> None:
        """Nächstes Bild komplett statt nur geänderter Bänder senden (heilt Übertragungsfehler)."""
        with self.lock:
            self.last_frame = None

    # ---------- Bilder ----------
    def _blit(self, img: Image.Image, x: int, y: int) -> None:
        w, h = img.size
        self._write(encode_command(Cmd.DISPLAY_BITMAP, x, y, x + w - 1, y + h - 1))
        data = to_rgb565le(img)
        step = self.size[0] * 8  # gleiche Blockgröße wie die Referenz
        for i in range(0, len(data), step):
            self._write(data[i:i + step])

    def show(self, frame: Image.Image) -> int:
        """Zeigt frame an; überträgt nur geänderte Zeilenbänder. Rückgabe: gesendete Zeilen."""
        frame = frame.convert("RGB")
        assert frame.size == self.size, f"Frame {frame.size} != Display {self.size}"
        with self.lock:
            if self.ser is None:
                self._open()
            try:
                return self._show(frame)
            except (serial.SerialException, OSError) as e:
                log.warning("Übertragung fehlgeschlagen (%s) – verbinde neu", e)
                self._close()
                self.last_frame = None
                self._open()
                self._set_orientation()
                return self._show(frame)

    def _show(self, frame: Image.Image) -> int:
        w, h = frame.size
        if self.last_frame is None:
            self._blit(frame, 0, 0)
            self.last_frame = frame
            return h
        diff = ImageChops.difference(frame, self.last_frame)
        bh = self.band_height
        changed = [y for y in range(0, h, bh)
                   if diff.crop((0, y, w, min(h, y + bh))).getbbox() is not None]
        sent = 0
        # Zusammenhängende Bänder zu einem Rechteck zusammenfassen
        i = 0
        while i < len(changed):
            start = changed[i]
            end = start + bh
            while i + 1 < len(changed) and changed[i + 1] == end:
                i += 1
                end += bh
            end = min(h, end)
            self._blit(frame.crop((0, start, w, end)), 0, start)
            sent += end - start
            i += 1
        self.last_frame = frame
        return sent
