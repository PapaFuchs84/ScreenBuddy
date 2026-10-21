# © 2026 FuchsLabs
"""Windows-Integration ohne Administratorrechte: Autostart (Registry, nur dieses Benutzerkonto)
und der Umgang mit der Hersteller-App des Displays, die den Anschluss sonst belegt."""
from __future__ import annotations

import ctypes
import subprocess
import sys
import winreg
from pathlib import Path

RUN_KEY = r"Software\Microsoft\Windows\CurrentVersion\Run"
RUN_NAME = "FuchsLabs USB-Display"
# Technische Kennungen der Hersteller-App (Prozess und Autostart-Aufgabe), nötig zum Beenden/Abschalten
VENDOR_PROCESS = "UsbMonitor.exe"
VENDOR_TASK = "UsbMonitor"
NO_WINDOW = subprocess.CREATE_NO_WINDOW


def launch_command() -> str:
    """Befehl, mit dem diese App gestartet wird (EXE oder Python-Skript)."""
    if getattr(sys, "frozen", False):
        return f'"{sys.executable}"'
    pyw = Path(sys.executable).with_name("pythonw.exe")
    return f'"{pyw if pyw.exists() else sys.executable}" "{Path(sys.argv[0]).resolve()}"'


# ---------- Autostart ----------
def autostart_enabled() -> bool:
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, RUN_KEY) as k:
            return winreg.QueryValueEx(k, RUN_NAME)[0] == launch_command()
    except OSError:
        return False


def set_autostart(enabled: bool) -> None:
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, RUN_KEY, 0, winreg.KEY_SET_VALUE) as k:
        if enabled:
            winreg.SetValueEx(k, RUN_NAME, 0, winreg.REG_SZ, launch_command())
        else:
            try:
                winreg.DeleteValue(k, RUN_NAME)
            except FileNotFoundError:
                pass


# ---------- Hersteller-App ----------
def _run(args: list[str]) -> subprocess.CompletedProcess:
    """Befehl ausführen; Ausgabe in der Windows-Codepage lesen (sonst scheitert die Dekodierung bei Umlauten)."""
    r = subprocess.run(args, capture_output=True, timeout=20, creationflags=NO_WINDOW)
    r.stdout = (r.stdout or b"").decode("mbcs", errors="replace")
    return r


def vendor_running() -> bool:
    out = _run(["tasklist", "/FI", f"IMAGENAME eq {VENDOR_PROCESS}", "/NH"]).stdout
    return VENDOR_PROCESS.lower() in out.lower()


def vendor_stop() -> bool:
    _run(["taskkill", "/IM", VENDOR_PROCESS, "/F"])
    return not vendor_running()


def vendor_autostart_enabled() -> bool:
    r = _run(["schtasks", "/Query", "/TN", VENDOR_TASK, "/FO", "LIST"])
    return r.returncode == 0 and "Disabled" not in r.stdout and "Deaktiviert" not in r.stdout


def vendor_disable_autostart() -> bool:
    """Schaltet die Autostart-Aufgabe der Hersteller-App ab. Braucht Administratorrechte, deshalb mit
    Windows-Abfrage (UAC); der Nutzer bestätigt sie selbst."""
    rc = ctypes.windll.shell32.ShellExecuteW(None, "runas", "schtasks.exe",
                                             f'/Change /TN "{VENDOR_TASK}" /DISABLE', None, 0)
    return rc > 32
