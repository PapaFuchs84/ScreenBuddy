# © 2026 FuchsLabs
"""FuchsLabs USB-Display: zeigt System, Discord-Freunde, Battlefield-6-Rangliste und Blog-Updates
im Wechsel auf dem 3,5"-USB-Display. Einstellungen: Symbol im Infobereich → Einstellungsseite."""
from __future__ import annotations

import argparse
import asyncio
import ctypes
import logging
import logging.handlers
import os
import subprocess
import sys
import time
import webbrowser
from datetime import datetime
from pathlib import Path

from PIL import Image

import appconfig
import winsetup
from bf6 import Bf6Stats, demo_players
from display_driver import Orientation, UsbDisplay
from render import Bf6Renderer, Renderer, RssRenderer, SysRenderer, demo_friends, set_theme
from rss import RssFeeds
from sysinfo import SysMonitor

FROZEN = getattr(sys, "frozen", False)
HERE = Path(sys.executable).resolve().parent if FROZEN else Path(__file__).resolve().parent
# Liegt das Programm in einem Ordner ohne Schreibrecht (z. B. Programme), wandern Einstellungen und Protokoll
# ins Benutzerprofil.
DATA = HERE if os.access(HERE, os.W_OK) else Path(os.environ.get("APPDATA", HERE)) / "FuchsLabs" / "USB-Display"
log = logging.getLogger("app")

ORIENT = {"portrait": Orientation.PORTRAIT, "landscape": Orientation.LANDSCAPE,
          "reverse_portrait": Orientation.REVERSE_PORTRAIT, "reverse_landscape": Orientation.REVERSE_LANDSCAPE}
MUTEX_NAME = "Local\\FuchsLabsUsbDisplay" + os.environ.get("FUCHSLABS_INSTANCE", "")  # Zusatz nur für Tests neben der echten App


class App:
    def __init__(self, cfg: dict, cfg_path: Path, lcd: UsbDisplay | None, preview_dir: Path | None):
        self.cfg, self.cfg_path, self.lcd, self.preview_dir = cfg, cfg_path, lcd, preview_dir
        self.dp = cfg["display"]
        self.size = lcd.size if lcd else (480, 320) if "landscape" in self.dp["orientation"] else (320, 480)
        self.dirty = asyncio.Event()
        self.stop_event = asyncio.Event()
        self.restart = False
        self.last_frame: Image.Image | None = None
        self.display_ok = lcd is None
        self.display_error = ""
        self.page, self.page_switched, self._last_full = 0, time.monotonic(), time.monotonic()
        self._current = ""  # Name der gerade gezeigten Seite
        w = self.size
        set_theme(self.dp.get("theme", "hell"))
        self.renderers = {"discord": Renderer(w, title=self.dp["title"]),
                          "friends": Renderer(w, title=cfg["friends"]["title"]),
                          "bf6": Bf6Renderer(w, title=cfg["bf6"]["title"]),
                          "rss": RssRenderer(w, title=cfg["rss"]["title"]),
                          "system": SysRenderer(w, title=cfg["system"]["title"], net_unit=cfg["system"]["net_unit"])}
        bf, rs, sy, dc, fr = cfg["bf6"], cfg["rss"], cfg["system"], cfg["discord"], cfg["friends"]
        self.bf6 = Bf6Stats(bf) if bf["enabled"] and bf["players"] else None
        self.rss = RssFeeds(rs) if rs["enabled"] and rs["feeds"] else None
        self.sys = SysMonitor(sy) if sy["enabled"] else None
        self.discord = None
        # Discord wird gebraucht, wenn seine Seite an ist oder die gemeinsame Freundesliste es einbezieht
        want_discord = dc["enabled"] or (fr["enabled"] and fr["include_discord"])
        if want_discord and str(dc.get("token", "")).strip():  # ohne Token keine Discord-Daten
            from discord_feed import DiscordFeed  # erst hier laden: spart Speicher und Startzeit, wenn aus
            self.discord = DiscordFeed(dc, self.dirty.set)
        self.friends = None
        if fr["enabled"]:
            from friends import FriendsHub
            self.friends = FriendsHub(fr, self.discord if fr["include_discord"] else None, self.dirty.set)

    # ---------- für die Einstellungsseite ----------
    @property
    def discord_ready(self) -> bool:
        return bool(self.discord and self.discord.ready)

    def member_list(self) -> list[dict]:
        return self.discord.member_list() if self.discord else []

    def active_pages(self) -> list[str]:
        # Die reine Discord-Seite entfällt, solange die gemeinsame Freundesliste Discord schon enthält
        # (sonst stünden dieselben Leute auf zwei Seiten).
        fr = self.cfg["friends"]
        discord_page = self.cfg["discord"]["enabled"] and not (self.friends and fr["include_discord"])
        have = {"system": self.sys, "discord": self.discord if discord_page else None,
                "friends": self.friends, "bf6": self.bf6, "rss": self.rss}
        return [p for p in self.dp["pages"] if have.get(p)]

    def connections(self) -> dict:
        """Dauerhafter Verbindungsstatus für Discord, Steam und Xbox (Einstellungsseite).
        state: ok = verbunden · bad = Fehler · idle = Zugang gespeichert, aber nicht in Benutzung · off = nicht eingerichtet"""
        dc, fr = self.cfg["discord"], self.cfg["friends"]
        out: dict[str, dict] = {}
        if self.discord:
            d = self.discord
            if not d.status:
                out["discord"] = {"state": "ok", "name": d.bot_name, "detail": f"{d.server_count} Server", "live": True}
            else:  # "Verbinde …" ist kein Fehler, alles andere schon
                out["discord"] = {"state": "idle" if d.status.startswith("Verbinde") else "bad", "error": d.status}
        else:
            out["discord"] = {"state": "idle" if dc.get("token") else "off"}
        for plat, key_set in (("steam", fr.get("steam_api_key") and fr.get("steam_id")), ("xbox", fr.get("xbox_api_key"))):
            info = self.friends.info.get(plat) if self.friends else None
            if info:
                out[plat] = {"state": "ok" if info.get("ok") else "bad", "name": info.get("name", ""),
                             "detail": f"{info.get('count', 0)} Freunde, {info.get('online', 0)} online",
                             "at": info.get("at"), "error": info.get("error", "")}
            else:
                out[plat] = {"state": "idle" if key_set else "off"}
        return out

    def request_stop(self) -> None:
        self.stop_event.set()

    def request_restart(self) -> None:
        self.restart = True
        self.stop_event.set()

    # ---------- Hauptschleife ----------
    async def run(self) -> None:
        tasks = []
        if self.sys:
            self.sys.start()
        if self.bf6:
            tasks.append(asyncio.create_task(self.bf6.run(self.dirty.set)))
        if self.rss:
            tasks.append(asyncio.create_task(self.rss.run(self.dirty.set)))
        if self.discord:
            tasks.append(asyncio.create_task(self.discord.run()))
        if self.friends:
            tasks.append(asyncio.create_task(self.friends.run()))
        tasks.append(asyncio.create_task(self._display_loop()))
        server = None
        if self.cfg["ui"]["enabled"]:
            from webui import SettingsServer
            server = SettingsServer(self, self.cfg_path)
            await server.start()
        tray = self._start_tray()
        try:
            await self.stop_event.wait()
        finally:
            if tray:
                tray.stop()
            for t in tasks:
                t.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)
            if self.discord:
                await self.discord.close()
            if server:
                await server.stop()
            if self.sys:
                self.sys.stop()

    def _start_tray(self):
        try:
            from tray import Tray
        except Exception:
            return None
        loop = asyncio.get_running_loop()
        port = self.cfg["ui"]["port"]
        tray = Tray("FuchsLabs USB-Display",
                    on_open=lambda: webbrowser.open(f"http://127.0.0.1:{port}/"),
                    on_restart=lambda: loop.call_soon_threadsafe(self.request_restart),
                    on_quit=lambda: loop.call_soon_threadsafe(self.request_stop))
        tray.start()
        return tray

    # ---------- Anzeige ----------
    def _friends_message(self) -> str:
        """Hinweis für die Freunde-Seite, solange keine Daten da sind oder etwas nicht klappt."""
        fr = self.cfg["friends"]
        problems = [self.friends.status] if self.friends.status else []
        if self.discord and fr["include_discord"] and self.discord.status:
            problems.append(self.discord.status)
        has_source = bool(self.discord and fr["include_discord"]) or fr["steam_api_key"] or fr["xbox_api_key"]
        if not has_source:
            return "Noch keine Konten verbunden – bitte in den Einstellungen einrichten"
        return " · ".join(problems)

    def _page_list(self, friends, people) -> list[tuple[str, int, object]]:
        """[(Name, Seitenanzahl, Zeichenfunktion(seite))] in der eingestellten Reihenfolge."""
        r, out = self.renderers, []
        for key in self.active_pages():
            if key == "system":
                out.append((key, 1, lambda i: r["system"].render(self.sys.snapshot())))
            elif key == "discord":
                st = self.discord.status
                out.append((key, r["discord"].pages(friends),
                            lambda i, st=st: r["discord"].render(friends, page=i, connected=not st, message=st)))
            elif key == "friends":
                msg = self._friends_message()
                bad = bool(msg) and not people
                out.append((key, r["friends"].pages(people),
                            lambda i, msg=msg, bad=bad: r["friends"].render(people, page=i, connected=not bad, message=msg)))
            elif key == "bf6":
                out.append((key, r["bf6"].pages(self.bf6.players),
                            lambda i: r["bf6"].render(self.bf6.players, page=i, updated=self.bf6.updated)))
            elif key == "rss":
                out.append((key, 1, lambda i: r["rss"].render(self.rss.posts, updated=self.rss.updated)))
        return out

    def _welcome(self) -> Image.Image:
        port = self.cfg["ui"]["port"]
        return self.renderers["discord"].welcome(
            "Willkommen!", f"Öffne die Einstellungen über das Fuchs-Symbol im Infobereich "
            f"oder im Browser: 127.0.0.1:{port}")

    async def _connect_display(self) -> bool:
        try:
            await asyncio.to_thread(self.lcd.connect)
        except Exception as e:
            vendor = await asyncio.to_thread(winsetup.vendor_running)
            self.display_error = ("Die Hersteller-App des Displays läuft und belegt den Anschluss"
                                  if vendor else "Display nicht gefunden – USB-Kabel und Anschluss prüfen")
            log.warning("Display: %s (%s)", self.display_error, e)
            return False
        self.display_ok, self.display_error = True, ""
        return True

    async def _display_loop(self) -> None:
        last_minute, next_try = None, 0.0
        if self.lcd:
            await self._connect_display()
        while not self.stop_event.is_set():
            try:
                await asyncio.wait_for(self.dirty.wait(), timeout=1.0)
            except asyncio.TimeoutError:
                pass
            if self.lcd and not self.display_ok and time.monotonic() >= next_try:
                if not await self._connect_display():
                    next_try = time.monotonic() + 10
            minute = datetime.now().strftime("%H:%M")
            page_due = time.monotonic() - self.page_switched >= self.dp["page_seconds"]
            # Die Systemansicht wird jede Sekunde neu gezeichnet, die anderen nur bei Änderungen
            live = self._current == "system"
            if not (self.dirty.is_set() or page_due or live or minute != last_minute):
                continue
            self.dirty.clear()
            last_minute = minute
            active = self.active_pages()
            friends = await self.discord.friends() if self.discord and "discord" in active else []
            people = await self.friends.people() if self.friends else []
            pages = self._page_list(friends, people)
            total = sum(n for _, n, _ in pages)
            if not total:
                frame, switched, self._current = self._welcome(), False, ""
            else:
                switched = False
                if page_due:
                    switched = total > 1
                    self.page = (self.page + 1) % total
                    self.page_switched = time.monotonic()
                self.page %= total
                idx = self.page
                for key, n, fn in pages:
                    if idx < n:
                        frame, self._current = fn(idx), key
                        break
                    idx -= n
            await self._output(frame, transition=switched)
            if switched:
                self.page_switched = time.monotonic()  # Anzeigedauer ab Ende der Übertragung zählen
            await asyncio.sleep(0.5)  # max. ~2 Bilder/s, damit Status-Stürme das Display nicht fluten

    async def _output(self, frame: Image.Image, transition: bool = False) -> None:
        self.last_frame = frame
        if self.preview_dir:
            frame.save(self.preview_dir / "live.png")
        if not (self.lcd and self.display_ok):
            return
        # Teil-Updates übertragen nur geänderte Bänder; ein verlorenes Datenpaket bliebe sonst als Streifen
        # stehen. Deshalb bei jedem Seitenwechsel und alle 5 Minuten ein Vollbild.
        now = time.monotonic()
        if transition or now - self._last_full > 300:
            await asyncio.to_thread(self.lcd.invalidate)
            self._last_full = now
        try:
            rows = await asyncio.to_thread(self.lcd.show, frame)
            log.debug("%d Zeilen übertragen", rows)
        except Exception as e:
            self.display_ok = False
            self.display_error = "Verbindung zum Display verloren – wird erneut versucht"
            log.warning("Display: %s", e)


# ---------- Start ----------
def run_demo(lcd: UsbDisplay | None, cfg: dict, preview_dir: Path | None) -> None:
    size = lcd.size if lcd else (480, 320)
    mon = SysMonitor(cfg["system"])
    mon.sample()
    time.sleep(1)
    mon.sample()  # zweite Messung, damit Netzwerkraten vorhanden sind
    set_theme(cfg["display"].get("theme", "hell"))
    r = Renderer(size, title=cfg["display"]["title"])
    br, rr = Bf6Renderer(size, title=cfg["bf6"]["title"]), RssRenderer(size, title=cfg["rss"]["title"])
    from rss import Post
    posts = [Post("Beispiel-Blog", "Ein kurzer Titel für die Vorschau", datetime.now().astimezone(), ""),
             Post("Noch ein Blog", "Zweiter Beitrag mit etwas längerem Titel, der umbricht", datetime.now().astimezone(), "")]
    sr = SysRenderer(size, title=cfg["system"]["title"], net_unit=cfg["system"]["net_unit"])
    frames = [sr.render(mon.snap)]
    friends, players = demo_friends(), demo_players()
    frames += [r.render(friends, page=p) for p in range(r.pages(friends))]
    frames += [br.render(players, page=p, updated=datetime.now()) for p in range(br.pages(players))]
    frames += [rr.render(posts, updated=datetime.now())]
    if lcd:
        lcd.connect()
    for p, frame in enumerate(frames):
        if preview_dir:
            frame.save(preview_dir / f"demo-{p + 1}.png")
        if lcd:
            lcd.show(frame)
            time.sleep(cfg["display"]["page_seconds"])


def _single_instance(wait: bool):
    """Nur eine laufende App. Rückgabe: Mutex-Handle oder None, wenn schon eine läuft."""
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    k.CreateMutexW.restype = ctypes.c_void_p
    k.CreateMutexW.argtypes = [ctypes.c_void_p, ctypes.c_bool, ctypes.c_wchar_p]
    k.CloseHandle.argtypes = [ctypes.c_void_p]
    for _ in range(40 if wait else 1):
        h = k.CreateMutexW(None, False, MUTEX_NAME)
        if ctypes.get_last_error() != 183:  # 183 = ERROR_ALREADY_EXISTS
            return h
        k.CloseHandle(h)
        time.sleep(0.5)
    return None


def _setup_logging(args) -> None:
    level = logging.DEBUG if args.verbose else logging.INFO
    fmt = "%(asctime)s %(levelname)s %(name)s: %(message)s"
    log_file = args.log or (str(DATA / "usb-display.log") if sys.stderr is None else None)
    if log_file:
        h = logging.handlers.RotatingFileHandler(log_file, maxBytes=500_000, backupCount=1, encoding="utf-8")
        logging.basicConfig(level=level, format=fmt, handlers=[h])
    else:
        logging.basicConfig(level=level, format=fmt)
    for noisy in ("discord", "aiohttp", "PIL", "asyncio"):
        logging.getLogger(noisy).setLevel(logging.WARNING)


def _relaunch() -> None:
    cmd = [sys.executable] + ([] if FROZEN else [sys.argv[0]]) + [a for a in sys.argv[1:] if a != "--restarted"]
    subprocess.Popen(cmd + ["--restarted"], creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NO_WINDOW,
                     close_fds=True)


def main() -> int:
    ap = argparse.ArgumentParser(description="FuchsLabs USB-Display")
    ap.add_argument("-c", "--config", default=str(DATA / "config.toml"))
    ap.add_argument("--demo", action="store_true", help="Beispieldaten aller Seiten zeigen")
    ap.add_argument("--no-lcd", action="store_true", help="Display nicht ansteuern (nur Vorschau)")
    ap.add_argument("--preview", metavar="ORDNER", help="Bilder zusätzlich als PNG speichern")
    ap.add_argument("--log", metavar="DATEI", help="Protokoll in diese Datei schreiben")
    ap.add_argument("--restarted", action="store_true", help=argparse.SUPPRESS)
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    _setup_logging(args)

    cfg_path = Path(args.config)
    first_run = not cfg_path.exists()
    cfg = appconfig.load_config(cfg_path)
    if not args.demo:
        mutex = _single_instance(args.restarted)
        if mutex is None:  # läuft schon: nur die Einstellungen öffnen
            webbrowser.open(f"http://127.0.0.1:{cfg['ui']['port']}/")
            return 0
        if appconfig.token_is_plaintext(cfg_path):
            appconfig.save_config(cfg_path, cfg)  # alten Klartext-Token verschlüsseln
            log.info("Bot-Token wird jetzt verschlüsselt gespeichert")

    dp = cfg["display"]
    preview = Path(args.preview) if args.preview else None
    if preview:
        preview.mkdir(parents=True, exist_ok=True)
    lcd = None if args.no_lcd else UsbDisplay(
        port=str(dp["port"]), orientation=ORIENT[dp["orientation"]],
        brightness=int(dp["brightness"]), reset_on_start=bool(dp["reset_on_start"]))
    app = None
    try:
        if args.demo:
            run_demo(lcd, cfg, preview)
            return 0
        app = App(cfg, cfg_path, lcd, preview)
        if first_run or not cfg["ui"]["setup_done"]:
            webbrowser.open(f"http://127.0.0.1:{cfg['ui']['port']}/")
        asyncio.run(app.run())
        return 0
    finally:
        restart = bool(app and app.restart)
        if lcd:
            if dp.get("screen_off_on_exit") and not restart:
                try:
                    lcd.screen_off()
                except Exception:
                    pass
            lcd.close()
        if restart:
            _relaunch()


if __name__ == "__main__":
    sys.exit(main())
