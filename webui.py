# © 2026 FuchsLabs
"""Einstellungsoberfläche: kleine Webseite, die nur lokal (127.0.0.1) im Browser erreichbar ist.

Sie läuft in der Ereignisschleife der App, belegt also keinen eigenen Prozess und fast keine Ressourcen,
solange sie nicht geöffnet ist. Schutz gegen fremde Webseiten: nur Host 127.0.0.1/localhost, Schreibzugriffe
brauchen einen eigenen Header (löst im Browser eine Cross-Origin-Vorabprüfung aus, die wir nicht erlauben).
"""
from __future__ import annotations

import asyncio
import io
import logging
import sys
from pathlib import Path
from typing import TYPE_CHECKING

import aiohttp
from aiohttp import web

import appconfig
import i18n
import sysinfo
import winsetup
from bf6 import Bf6Stats
from i18n import tr
from rss import UA, RssFeeds

if TYPE_CHECKING:
    from fuchslabs_display import App

log = logging.getLogger("webui")
HTML = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent)) / "webui.html"
GUIDE = HTML.with_name("ANLEITUNG.html")
GUIDE_EN = HTML.with_name("GUIDE.html")
VERSION = "1.2"
_MESSAGES = {"error", "display_error", "discord", "detail"}   # Meldungstexte, die i18n übersetzt


def _tr_messages(data):
    if isinstance(data, dict):
        return {k: tr(v) if k in _MESSAGES else _tr_messages(v) for k, v in data.items()}
    return data


def _json(data, status: int = 200) -> web.Response:
    return web.json_response(_tr_messages(data), status=status, headers={"Cache-Control": "no-store"})


@web.middleware
async def guard(request: web.Request, handler):
    port = request.app["port"]
    if request.host not in (f"127.0.0.1:{port}", f"localhost:{port}"):
        return web.Response(status=403, text="Nur lokaler Zugriff")
    # Fremde Webseiten dürfen auch keine GET-Abrufe auslösen (z. B. <img src=…/api/check/rss?url=…>);
    # nur das Öffnen der Startseite per Link bleibt erlaubt.
    if request.headers.get("Sec-Fetch-Site", "same-origin") not in ("same-origin", "none") and not (
            request.path == "/" and request.headers.get("Sec-Fetch-Mode") == "navigate"):
        return web.Response(status=403, text="Anfrage abgelehnt")
    if request.method not in ("GET", "HEAD"):
        origin = request.headers.get("Origin")
        if request.headers.get("X-FL") != "1" or (origin and origin != f"http://{request.host}"):
            return web.Response(status=403, text="Anfrage abgelehnt")
    resp = await handler(request)
    # Nicht in fremde Seiten einbetten (Klick-Täuschung)
    resp.headers["X-Frame-Options"] = "DENY"
    resp.headers["Content-Security-Policy"] = "frame-ancestors 'none'"
    resp.headers["X-Content-Type-Options"] = "nosniff"
    return resp


class SettingsServer:
    def __init__(self, board: "App", cfg_path: Path):
        self.board = board
        self.cfg_path = cfg_path
        self.port = int(board.cfg["ui"]["port"])
        self.runner: web.AppRunner | None = None

    async def start(self) -> bool:
        app = web.Application(middlewares=[guard])
        app["port"] = self.port
        r = app.router
        r.add_get("/", self.index)
        r.add_get("/anleitung", self.guide)
        r.add_get("/api/config", self.get_config)
        r.add_post("/api/config", self.post_config)
        r.add_get("/api/preview.png", self.preview)
        r.add_post("/api/preview-theme.png", self.preview_theme)
        r.add_get("/api/display/options", self.display_options)
        r.add_get("/api/discord/members", self.members)
        r.add_get("/api/system/info", self.system_info)
        r.add_get("/api/check/bf6", self.check_bf6)
        r.add_get("/api/check/rss", self.check_rss)
        r.add_post("/api/check/friends/{platform:steam|xbox}", self.check_friends)
        r.add_get("/api/friends/accounts", self.friend_accounts)
        r.add_get("/api/connections", self.connections)
        r.add_get("/api/status", self.status)
        r.add_post("/api/autostart", self.autostart)
        r.add_post("/api/vendor/stop", self.vendor_stop)
        r.add_post("/api/vendor/disable-autostart", self.vendor_disable)
        r.add_post("/api/setup-done", self.setup_done)
        r.add_post("/api/restart", self.restart)
        r.add_post("/api/quit", self.quit)
        self.runner = web.AppRunner(app, access_log=None)
        await self.runner.setup()
        try:
            await web.TCPSite(self.runner, "127.0.0.1", self.port).start()
        except OSError as e:
            log.warning("Einstellungsoberfläche nicht gestartet (Port %d: %s)", self.port, e)
            return False
        log.info("Einstellungen: http://127.0.0.1:%d/", self.port)
        return True

    async def stop(self) -> None:
        if self.runner:
            await self.runner.cleanup()

    # ---------- Seiten ----------
    async def index(self, request: web.Request) -> web.Response:
        return web.Response(text=HTML.read_text(encoding="utf-8"), content_type="text/html",
                            headers={"Cache-Control": "no-store"})

    async def guide(self, request: web.Request) -> web.Response:
        en = request.query.get("lang", i18n.lang()) == "en" and GUIDE_EN.exists()
        return web.FileResponse(GUIDE_EN if en else GUIDE, headers={"Content-Type": "text/html; charset=utf-8"})

    async def get_config(self, request: web.Request) -> web.Response:
        b = self.board
        return _json({"config": appconfig.public(b.cfg), "version": VERSION,
                      "discord_ready": b.discord_ready, "screen": list(b.size), "first_run": not b.cfg["ui"]["setup_done"],
                      "active": b.active_pages(), "config_file": str(self.cfg_path)})

    async def post_config(self, request: web.Request) -> web.Response:
        try:
            body = await request.json()
            new = appconfig.merge(self.board.cfg, body.get("config", {}))
        except (ValueError, TypeError, AttributeError) as e:
            return _json({"ok": False, "error": str(e)}, 400)
        appconfig.save_config(self.cfg_path, new)
        log.info("Einstellungen gespeichert – starte neu")
        asyncio.get_running_loop().call_later(0.4, self.board.request_restart)
        return _json({"ok": True})

    async def preview(self, request: web.Request) -> web.Response:
        frame = self.board.last_frame
        if frame is None:
            return web.Response(status=404)
        buf = io.BytesIO()
        frame.save(buf, "PNG")
        return web.Response(body=buf.getvalue(), content_type="image/png", headers={"Cache-Control": "no-store"})

    async def preview_theme(self, request: web.Request) -> web.Response:
        """Aktuelles Displaybild mit den (noch nicht gespeicherten) Farben aus dem Formular."""
        try:
            body = await request.json()
            keys = {"theme", "color_base"} | {k for _, k in appconfig.COLORS}
            dp = {k: v for k, v in dict(body.get("display", {})).items() if k in keys}
            merged = appconfig.merge(self.board.cfg, {"display": dp})["display"]
        except (ValueError, TypeError, AttributeError) as e:
            return _json({"ok": False, "error": str(e)}, 400)
        frame = self.board.preview_theme(merged)
        if frame is None:
            return web.Response(status=404)
        buf = io.BytesIO()
        frame.save(buf, "PNG")
        return web.Response(body=buf.getvalue(), content_type="image/png", headers={"Cache-Control": "no-store"})

    async def display_options(self, request: web.Request) -> web.Response:
        """Angeschlossene Bildschirme, Touch-Fähigkeit und die Farben der Vorlagen."""
        import monitor_output
        import render
        monitors, touch = await asyncio.gather(asyncio.to_thread(monitor_output.list_monitors),
                                               asyncio.to_thread(monitor_output.touch_info))
        presets = {name: {k: render.hex_color(c) for k, c in p.items()} for name, p in render.PRESETS.items()}
        return _json({"monitors": monitors, "touch": touch, "presets": presets, "size": list(self.board.size)})

    async def members(self, request: web.Request) -> web.Response:
        return _json({"ready": self.board.discord_ready, "members": self.board.member_list()})

    async def system_info(self, request: web.Request) -> web.Response:
        interfaces, sensors = await asyncio.gather(asyncio.to_thread(sysinfo.list_interfaces),
                                                   asyncio.to_thread(sysinfo.hwinfo_sensors))
        return _json({"interfaces": interfaces, "sensors": sensors})

    # ---------- Prüfen ----------
    async def check_bf6(self, request: web.Request) -> web.Response:
        name = request.query.get("name", "").strip()
        if not name:
            return _json({"ok": False, "error": "Name fehlt"}, 400)
        async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=40),
                                         headers={"User-Agent": "FuchsLabs-USB-Display/1.2"}) as s:
            p = await Bf6Stats({"players": []})._fetch(s, name)
        if p.error:
            return _json({"ok": False, "error": "Spieler nicht gefunden"})
        return _json({"ok": True, "name": p.name, "rank": p.rank, "rank_name": p.rank_name,
                      "kd": p.kd, "win_pct": p.win_pct})

    async def check_rss(self, request: web.Request) -> web.Response:
        url = request.query.get("url", "").strip()
        if not url:
            return _json({"ok": False, "error": "Adresse fehlt"}, 400)
        feeds = RssFeeds({"feeds": []})
        async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=25), headers=UA) as s:
            posts = await feeds._feed(s, url)
        if posts is None:
            return _json({"ok": False, "error": "Kein Feed gefunden"})
        posts = feeds.select(posts)
        return _json({"ok": True, "feed": feeds._resolved.get(url, url),
                      "source": posts[0].source if posts else "", "latest": posts[0].title if posts else ""})

    async def check_friends(self, request: web.Request) -> web.Response:
        """Zugang zu Steam oder Xbox prüfen. Felder, die leer sind, werden aus den gespeicherten Einstellungen genommen,
        sodass man erst prüfen und dann speichern kann."""
        import friends as fr
        platform, body = request.match_info["platform"], await request.json()
        saved = self.board.cfg["friends"]
        key = str(body.get("key", "")).strip() or saved.get(f"{platform}_api_key", "")
        if not key:
            return _json({"ok": False, "error": "Bitte zuerst den Schlüssel eintragen"})
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=30), headers=fr.UA) as s:
                if platform == "steam":
                    sid = str(body.get("id", "")).strip() or saved.get("steam_id", "")
                    if not sid:
                        return _json({"ok": False, "error": "Bitte den Link zu deinem Steam-Profil eintragen"})
                    accounts = await fr.fetch_steam(s, key, sid)
                else:
                    accounts = await fr.fetch_xbox(s, key)
        except fr.FriendsError as e:
            return _json({"ok": False, "error": str(e)})
        online = sum(1 for a in accounts if a.status != "offline")
        return _json({"ok": True, "count": len(accounts), "online": online})

    async def connections(self, request: web.Request) -> web.Response:
        return _json(self.board.connections())

    async def friend_accounts(self, request: web.Request) -> web.Response:
        hub = self.board.friends
        accounts = await hub.all_accounts() if hub else []
        import friends as fr
        cfg = self.board.cfg["friends"]
        return _json({"accounts": accounts,
                      "suggestions": fr.suggest_links(accounts, list(cfg["links"]), bool(cfg["auto_merge"]))})

    # ---------- Status und Windows-Einstellungen ----------
    async def status(self, request: web.Request) -> web.Response:
        b = self.board
        vendor_running, vendor_task, auto = await asyncio.gather(
            asyncio.to_thread(winsetup.vendor_running), asyncio.to_thread(winsetup.vendor_autostart_enabled),
            asyncio.to_thread(winsetup.autostart_enabled))
        return _json({"display_ok": b.display_ok, "display_error": b.display_error,
                      "discord": b.discord.status if b.discord else None,
                      "vendor_running": vendor_running, "vendor_autostart": vendor_task, "autostart": auto})

    async def autostart(self, request: web.Request) -> web.Response:
        body = await request.json()
        await asyncio.to_thread(winsetup.set_autostart, bool(body.get("enabled")))
        return _json({"ok": True, "autostart": await asyncio.to_thread(winsetup.autostart_enabled)})

    async def vendor_stop(self, request: web.Request) -> web.Response:
        ok = await asyncio.to_thread(winsetup.vendor_stop)
        return _json({"ok": ok, "error": "" if ok else "Konnte die Hersteller-App nicht beenden"})

    async def vendor_disable(self, request: web.Request) -> web.Response:
        ok = await asyncio.to_thread(winsetup.vendor_disable_autostart)
        return _json({"ok": ok, "error": "" if ok else "Die Windows-Abfrage wurde abgelehnt"})

    async def setup_done(self, request: web.Request) -> web.Response:
        self.board.cfg["ui"]["setup_done"] = True
        appconfig.save_config(self.cfg_path, self.board.cfg)
        return _json({"ok": True})

    # ---------- Steuerung ----------
    async def restart(self, request: web.Request) -> web.Response:
        asyncio.get_running_loop().call_later(0.4, self.board.request_restart)
        return _json({"ok": True})

    async def quit(self, request: web.Request) -> web.Response:
        asyncio.get_running_loop().call_later(0.4, self.board.request_stop)
        return _json({"ok": True})
