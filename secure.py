# © 2026 FuchsLabs
"""Geheimnisse (Bot-Token) mit Windows-DPAPI verschlüsseln: nur dasselbe Windows-Benutzerkonto auf
demselben PC kann sie wieder lesen. Die Konfigurationsdatei enthält dadurch keinen Klartext-Token."""
from __future__ import annotations

import base64
import ctypes
import ctypes.wintypes as wt

PREFIX = "dpapi:"
_ENTROPY = b"FuchsLabs.USB-Display.v1"  # zusätzliche App-Bindung: andere Programme entschlüsseln nicht ohne Weiteres


class _Blob(ctypes.Structure):
    _fields_ = [("cbData", wt.DWORD), ("pbData", ctypes.POINTER(ctypes.c_char))]


_crypt = ctypes.WinDLL("crypt32", use_last_error=True)
_kernel = ctypes.WinDLL("kernel32", use_last_error=True)
_crypt.CryptProtectData.argtypes = [ctypes.POINTER(_Blob), wt.LPCWSTR, ctypes.POINTER(_Blob), wt.LPVOID,
                                    wt.LPVOID, wt.DWORD, ctypes.POINTER(_Blob)]
_crypt.CryptUnprotectData.argtypes = [ctypes.POINTER(_Blob), ctypes.POINTER(wt.LPWSTR), ctypes.POINTER(_Blob),
                                      wt.LPVOID, wt.LPVOID, wt.DWORD, ctypes.POINTER(_Blob)]
_kernel.LocalFree.argtypes = [wt.HLOCAL]


def _blob(data: bytes) -> tuple[_Blob, ctypes.Array]:
    buf = ctypes.create_string_buffer(data, len(data))
    return _Blob(len(data), ctypes.cast(buf, ctypes.POINTER(ctypes.c_char))), buf


def _take(out: _Blob) -> bytes:
    data = ctypes.string_at(out.pbData, out.cbData)
    _kernel.LocalFree(ctypes.cast(out.pbData, wt.HLOCAL))
    return data


def protect(text: str) -> str:
    if not text:
        return ""
    src, keep1 = _blob(text.encode("utf-8"))
    ent, keep2 = _blob(_ENTROPY)
    out = _Blob()
    if not _crypt.CryptProtectData(ctypes.byref(src), "FuchsLabs", ctypes.byref(ent), None, None, 0, ctypes.byref(out)):
        raise OSError(ctypes.get_last_error(), "Verschlüsselung fehlgeschlagen")
    return PREFIX + base64.b64encode(_take(out)).decode("ascii")


def unprotect(value: str) -> str:
    """Entschlüsselt "dpapi:…"; Klartext (alte Konfigurationen) wird unverändert zurückgegeben.
    Nicht entschlüsselbar (anderer Benutzer/PC) → leerer Text."""
    if not value.startswith(PREFIX):
        return value
    try:
        raw = base64.b64decode(value[len(PREFIX):])
    except ValueError:
        return ""
    src, keep1 = _blob(raw)
    ent, keep2 = _blob(_ENTROPY)
    out = _Blob()
    if not _crypt.CryptUnprotectData(ctypes.byref(src), None, ctypes.byref(ent), None, None, 0, ctypes.byref(out)):
        return ""
    return _take(out).decode("utf-8")


def is_protected(value: str) -> bool:
    return value.startswith(PREFIX)
