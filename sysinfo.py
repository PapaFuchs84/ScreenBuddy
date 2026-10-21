# © 2026 FuchsLabs
"""Systemwerte für die Systemansicht – ohne Zusatzpakete (Windows-APIs per ctypes).

Netzwerk: 64-Bit-Zähler (GetIfEntry2) des Adapters, über den die Standardroute läuft –
nicht der Loopback-Adapter, den manche Hersteller-Apps fälschlich messen.
CPU-Temperatur und Lüfter kommen aus dem Shared Memory von HWiNFO, sofern es läuft
(HWiNFO → Einstellungen → „Shared Memory Support“).
"""
from __future__ import annotations

import copy
import ctypes
import ctypes.wintypes as wt
import logging
import mmap
import re
import socket
import struct
import subprocess
import threading
import time
from collections import deque
from dataclasses import dataclass, field

log = logging.getLogger("sysinfo")

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
iphlpapi = ctypes.WinDLL("iphlpapi")
pdh = ctypes.WinDLL("pdh")


# ---------- Netzwerk ----------
class GUID(ctypes.Structure):
    _fields_ = [("d1", wt.DWORD), ("d2", wt.WORD), ("d3", wt.WORD), ("d4", ctypes.c_ubyte * 8)]


class MIB_IF_ROW2(ctypes.Structure):
    _fields_ = [
        ("InterfaceLuid", ctypes.c_uint64), ("InterfaceIndex", wt.ULONG), ("InterfaceGuid", GUID),
        ("Alias", ctypes.c_wchar * 257), ("Description", ctypes.c_wchar * 257),
        ("PhysicalAddressLength", wt.ULONG), ("PhysicalAddress", ctypes.c_ubyte * 32),
        ("PermanentPhysicalAddress", ctypes.c_ubyte * 32), ("Mtu", wt.ULONG), ("Type", wt.ULONG),
        ("TunnelType", ctypes.c_int), ("MediaType", ctypes.c_int), ("PhysicalMediumType", ctypes.c_int),
        ("AccessType", ctypes.c_int), ("DirectionType", ctypes.c_int), ("Flags", ctypes.c_ubyte),
        ("OperStatus", ctypes.c_int), ("AdminStatus", ctypes.c_int), ("MediaConnectState", ctypes.c_int),
        ("NetworkGuid", GUID), ("ConnectionType", ctypes.c_int),
        ("TransmitLinkSpeed", ctypes.c_uint64), ("ReceiveLinkSpeed", ctypes.c_uint64),
        ("InOctets", ctypes.c_uint64), ("InUcastPkts", ctypes.c_uint64), ("InNUcastPkts", ctypes.c_uint64),
        ("InDiscards", ctypes.c_uint64), ("InErrors", ctypes.c_uint64), ("InUnknownProtos", ctypes.c_uint64),
        ("InUcastOctets", ctypes.c_uint64), ("InMulticastOctets", ctypes.c_uint64),
        ("InBroadcastOctets", ctypes.c_uint64), ("OutOctets", ctypes.c_uint64),
        ("OutUcastPkts", ctypes.c_uint64), ("OutNUcastPkts", ctypes.c_uint64), ("OutDiscards", ctypes.c_uint64),
        ("OutErrors", ctypes.c_uint64), ("OutUcastOctets", ctypes.c_uint64),
        ("OutMulticastOctets", ctypes.c_uint64), ("OutBroadcastOctets", ctypes.c_uint64),
        ("OutQLen", ctypes.c_uint64),
    ]


assert ctypes.sizeof(MIB_IF_ROW2) == 1352


def default_interface() -> int | None:
    """ifIndex des Adapters, über den Internet-Verkehr läuft (wie Windows' eigene Routenwahl)."""
    idx = wt.DWORD()
    dest = struct.unpack("<I", socket.inet_aton("1.1.1.1"))[0]
    return idx.value if iphlpapi.GetBestInterface(wt.DWORD(dest), ctypes.byref(idx)) == 0 else None


def interface_index(name: str) -> int | None:
    """ifIndex zu einem Adapternamen (z. B. "Skynet") oder einem Teil der Beschreibung."""
    for i in range(1, 512):
        row = MIB_IF_ROW2(InterfaceIndex=i)
        if iphlpapi.GetIfEntry2(ctypes.byref(row)) == 0:
            if name.casefold() in (row.Alias.casefold(), ) or name.casefold() in row.Description.casefold():
                return i
    return None


def if_row(index: int) -> MIB_IF_ROW2 | None:
    row = MIB_IF_ROW2(InterfaceIndex=index)
    return row if iphlpapi.GetIfEntry2(ctypes.byref(row)) == 0 else None


# ---------- CPU / RAM / Datenträger ----------
class MEMORYSTATUSEX(ctypes.Structure):
    _fields_ = [("dwLength", wt.DWORD), ("dwMemoryLoad", wt.DWORD), ("ullTotalPhys", ctypes.c_uint64),
                ("ullAvailPhys", ctypes.c_uint64), ("ullTotalPageFile", ctypes.c_uint64),
                ("ullAvailPageFile", ctypes.c_uint64), ("ullTotalVirtual", ctypes.c_uint64),
                ("ullAvailVirtual", ctypes.c_uint64), ("ullAvailExtendedVirtual", ctypes.c_uint64)]


class PDH_FMT_COUNTERVALUE(ctypes.Structure):
    _fields_ = [("CStatus", wt.DWORD), ("doubleValue", ctypes.c_double)]


PDH_FMT_DOUBLE = 0x00000200
PDH_FMT_NOCAP100 = 0x00008000


class Pdh:
    """Leistungsindikatoren wie im Task-Manager ("% Processor Utility" statt Leerlaufzeit)."""

    def __init__(self, paths: dict[str, str]):
        self.q = wt.HANDLE()
        self.counters: dict[str, wt.HANDLE] = {}
        if pdh.PdhOpenQueryW(None, None, ctypes.byref(self.q)) != 0:
            return
        for key, path in paths.items():
            h = wt.HANDLE()
            if pdh.PdhAddEnglishCounterW(self.q, path, None, ctypes.byref(h)) == 0:
                self.counters[key] = h
        pdh.PdhCollectQueryData(self.q)

    def read(self) -> dict[str, float]:
        out: dict[str, float] = {}
        if not self.counters or pdh.PdhCollectQueryData(self.q) != 0:
            return out
        for key, h in self.counters.items():
            v = PDH_FMT_COUNTERVALUE()
            if pdh.PdhGetFormattedCounterValue(h, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, None, ctypes.byref(v)) == 0:
                out[key] = v.doubleValue
        return out


def cpu_name() -> tuple[str, int]:
    """("Ryzen 7 5700X", Basistakt in MHz) aus der Registry."""
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as k:
            name = winreg.QueryValueEx(k, "ProcessorNameString")[0]
            mhz = winreg.QueryValueEx(k, "~MHz")[0]
    except OSError:
        return "CPU", 0
    name = re.sub(r"\(R\)|\(TM\)|^AMD |^Intel |\s+\d+-Core.*$|\s+Processor.*$|\s+with .*$|\s+CPU\s+@.*$", "", name.strip())
    return " ".join(name.split()), int(mhz)


# ---------- HWiNFO ----------
HWINFO_SM = "Global\\HWiNFO_SENS_SM2"
FILE_MAP_READ = 0x0004
kernel32.OpenFileMappingW.restype = wt.HANDLE
kernel32.OpenFileMappingW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
kernel32.MapViewOfFile.restype = ctypes.c_void_p
kernel32.MapViewOfFile.argtypes = [wt.HANDLE, wt.DWORD, wt.DWORD, wt.DWORD, ctypes.c_size_t]
kernel32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
kernel32.CloseHandle.argtypes = [wt.HANDLE]
SENSOR_TEMP, SENSOR_FAN = 1, 3


def hwinfo_readings() -> list[tuple[int, str, str, str, float]] | None:
    """[(Typ, Sensorname, Label, Einheit, Wert)] aus dem HWiNFO-Shared-Memory oder None."""
    # Nur öffnen, nie anlegen (Pythons mmap würde CreateFileMapping benutzen)
    h = kernel32.OpenFileMappingW(FILE_MAP_READ, False, HWINFO_SM)
    if not h:
        return None
    try:
        view = kernel32.MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0)
        if not view:
            return None
        try:
            hdr = ctypes.string_at(view, 44)
            sig, _ver, _rev, _poll, s_off, s_size, s_num, r_off, r_size, r_num = struct.unpack("<3Iq6I", hdr)
            if sig != 0x53695748:  # "HWiS"; "DEAD" = HWiNFO beendet
                return None
            buf = ctypes.string_at(view, r_off + r_size * r_num)
        finally:
            kernel32.UnmapViewOfFile(ctypes.c_void_p(view))
    finally:
        kernel32.CloseHandle(h)

    def cstr(b: bytes) -> str:
        return b.split(b"\0", 1)[0].decode("latin-1")

    sensors = [cstr(buf[s_off + i * s_size + 136: s_off + i * s_size + 264]) or
               cstr(buf[s_off + i * s_size + 8: s_off + i * s_size + 136]) for i in range(s_num)]
    out = []
    for i in range(r_num):
        o = r_off + i * r_size
        typ, sidx, _rid = struct.unpack_from("<3I", buf, o)
        label = cstr(buf[o + 140:o + 268]) or cstr(buf[o + 12:o + 140])
        unit = cstr(buf[o + 268:o + 284])
        value = struct.unpack_from("<d", buf, o + 284)[0]
        out.append((typ, sensors[sidx] if sidx < len(sensors) else "", label, unit, value))
    return out


def _pick(readings, typ: int, wanted: list[str], sensor_hint: str = "") -> float | None:
    wanted = [w for w in wanted if w]
    cands = [r for r in readings if r[0] == typ and (not sensor_hint or sensor_hint.casefold() in r[1].casefold())]
    for w in wanted:
        for r in cands:
            if r[2].casefold() == w.casefold():
                return r[4]
    for w in wanted:
        for r in cands:
            if w.casefold() in r[2].casefold():
                return r[4]
    return None


# ---------- Sammler ----------
@dataclass
class SysSnapshot:
    cpu_name: str = "CPU"
    cpu_pct: float | None = None
    cpu_ghz: float | None = None
    cpu_temp: float | None = None
    cpu_fan: float | None = None          # RPM
    gpu_name: str = "GPU"
    gpu_pct: float | None = None
    gpu_temp: float | None = None
    gpu_fan: float | None = None          # %
    gpu_mem_used: float | None = None     # MiB
    gpu_mem_total: float | None = None
    ram_pct: float | None = None
    ram_used: float | None = None         # GiB
    ram_total: float | None = None
    disk_label: str = "C:"
    disk_pct: float | None = None
    disk_temp: float | None = None
    net_name: str = ""
    net_down: float | None = None         # Bytes/s
    net_up: float | None = None
    net_link: int | None = None           # bit/s
    down_hist: deque = field(default_factory=lambda: deque(maxlen=60))
    up_hist: deque = field(default_factory=lambda: deque(maxlen=60))
    hwinfo: bool = False


class SysMonitor:
    def __init__(self, cfg: dict):
        self.cfg = cfg
        self.snap = SysSnapshot()
        self.snap.cpu_name, self._base_mhz = cpu_name()
        self.snap.disk_label = str(cfg.get("disk", "C:")).rstrip("\\")
        self._published = copy.copy(self.snap)
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._net_prev: tuple[float, int, int, int] | None = None
        self._net_idx: int | None = None
        self._gpu_next = 0.0
        self._pdh: Pdh | None = None

    def start(self) -> None:
        threading.Thread(target=self._run, name="sysinfo", daemon=True).start()

    def stop(self) -> None:
        self._stop.set()

    def snapshot(self) -> SysSnapshot:
        """Kopie für den Zeichen-Thread (die Messung läuft parallel weiter)."""
        with self._lock:
            return self._published

    def _run(self) -> None:
        while not self._stop.is_set():
            t = time.monotonic()
            try:
                self.sample()
                snap = copy.copy(self.snap)
                snap.down_hist, snap.up_hist = deque(self.snap.down_hist), deque(self.snap.up_hist)
                with self._lock:
                    self._published = snap
            except Exception:
                log.exception("Systemwerte konnten nicht gelesen werden")
            self._stop.wait(max(0.1, 1.0 - (time.monotonic() - t)))

    def sample(self) -> None:
        s = self.snap
        if self._pdh is None:  # beim ersten Aufruf anlegen; Werte gibt es ab dem zweiten Aufruf
            self._pdh = Pdh({"cpu": r"\Processor Information(_Total)\% Processor Utility",
                             "perf": r"\Processor Information(_Total)\% Processor Performance"})
        p = self._pdh.read()
        if "cpu" in p:
            s.cpu_pct = min(100.0, p["cpu"])
        if "perf" in p and self._base_mhz:
            s.cpu_ghz = self._base_mhz * p["perf"] / 100 / 1000

        ms = MEMORYSTATUSEX(dwLength=ctypes.sizeof(MEMORYSTATUSEX))
        if kernel32.GlobalMemoryStatusEx(ctypes.byref(ms)):
            s.ram_total = ms.ullTotalPhys / 2**30
            s.ram_used = (ms.ullTotalPhys - ms.ullAvailPhys) / 2**30
            s.ram_pct = s.ram_used / s.ram_total * 100

        free, total = ctypes.c_uint64(), ctypes.c_uint64()
        if kernel32.GetDiskFreeSpaceExW(s.disk_label + "\\", None, ctypes.byref(total), ctypes.byref(free)):
            s.disk_pct = (1 - free.value / total.value) * 100 if total.value else None

        self._sample_net(s)
        if time.monotonic() >= self._gpu_next:
            self._gpu_next = time.monotonic() + 2
            self._sample_gpu(s)
            self._sample_hwinfo(s)

    def _sample_net(self, s: SysSnapshot) -> None:
        name = str(self.cfg.get("net_adapter", "")).strip()
        if name and self._net_idx is None:
            self._net_idx = interface_index(name)
        idx = self._net_idx if name else default_interface()
        row = if_row(idx) if idx else None
        if row is None:
            s.net_name, s.net_down, s.net_up, self._net_prev = "kein Netzwerk", None, None, None
            return
        now = time.monotonic()
        prev = self._net_prev
        self._net_prev = (now, idx, row.InOctets, row.OutOctets)
        s.net_name = row.Alias
        s.net_link = row.ReceiveLinkSpeed or None
        if prev and prev[1] == idx and now > prev[0]:
            dt = now - prev[0]
            # Zähler können beim Adapter-Reset zurückspringen → negativ = 0
            s.net_down = max(0, row.InOctets - prev[2]) / dt
            s.net_up = max(0, row.OutOctets - prev[3]) / dt
            s.down_hist.append(s.net_down)
            s.up_hist.append(s.net_up)

    def _sample_gpu(self, s: SysSnapshot) -> None:
        try:
            out = subprocess.run(
                ["nvidia-smi", "--query-gpu=name,utilization.gpu,temperature.gpu,fan.speed,memory.used,memory.total",
                 "--format=csv,noheader,nounits"],
                capture_output=True, text=True, timeout=5, creationflags=subprocess.CREATE_NO_WINDOW).stdout
        except (OSError, subprocess.TimeoutExpired):
            return
        parts = [x.strip() for x in out.splitlines()[0].split(",")] if out.strip() else []
        if len(parts) < 6:
            return

        def num(x: str) -> float | None:
            try:
                return float(x)
            except ValueError:
                return None
        s.gpu_name = parts[0].replace("NVIDIA ", "").replace("GeForce ", "")
        s.gpu_pct, s.gpu_temp, s.gpu_fan, s.gpu_mem_used, s.gpu_mem_total = map(num, parts[1:6])

    def _sample_hwinfo(self, s: SysSnapshot) -> None:
        r = hwinfo_readings()
        s.hwinfo = r is not None
        if not r:
            return
        s.cpu_temp = _pick(r, SENSOR_TEMP, [self.cfg.get("cpu_temp_sensor", ""), "CPU (Tctl/Tdie)",
                                            "CPU Package", "CPU Die (average)", "CPU"])
        s.cpu_fan = _pick(r, SENSOR_FAN, [self.cfg.get("cpu_fan_sensor", ""), "CPU", "CPU_OPT", "AIO Pump", "AIO Pumpe",
                                          "Pumpe", "Chassis1"])
        disk = [x for x in r if "S.M.A.R.T" in x[1] and f"{s.disk_label}" in x[1].split("[")[-1]]
        s.disk_temp = _pick(disk, SENSOR_TEMP, ["Drive Temperature", "Laufwerkstemperatur"])


# ---------- Hilfen für die Einstellungsoberfläche ----------
def list_interfaces() -> list[dict]:
    """Aktive Netzwerkadapter (ohne Loopback): [{"name": Alias, "description": ...}]."""
    out = []
    for i in range(1, 512):
        row = if_row(i)
        if row and row.OperStatus == 1 and row.Type != 24 and row.Alias and not re.search(
                r"WAN Miniport|Filter|Scheduler|Extension Adapter|Loopback|Pseudo", row.Description + " " + row.Alias):
            out.append({"name": row.Alias, "description": row.Description})
    return out


def hwinfo_sensors() -> dict:
    """Namen der Temperatur- und Lüftersensoren aus HWiNFO (leer, wenn HWiNFO nicht läuft)."""
    r = hwinfo_readings()
    if r is None:
        return {"available": False, "temps": [], "fans": []}

    def names(typ: int) -> list[str]:
        return list(dict.fromkeys(x[2] for x in r if x[0] == typ))
    return {"available": True, "temps": names(SENSOR_TEMP), "fans": names(SENSOR_FAN)}
