# © 2026 FuchsLabs
"""Discord-Freunde (Online-Status + Aktivität) über einen eigenen Bot.

Discord wird erst hier importiert – ist die Discord-Seite aus, kostet es weder Speicher noch Startzeit.
Freundeslisten selbst sind per Bot-API nicht abrufbar: Der Bot sieht die Mitglieder der Server, auf die er
eingeladen ist (Intents „Presence“ + „Server Members“).
"""
from __future__ import annotations

import asyncio
import io
import logging
from datetime import datetime, timezone
from typing import Callable

from PIL import Image

from render import Friend, clean_text as _clean

log = logging.getLogger("discord")


def _duration(start: datetime | None) -> str:
    if not start:
        return ""
    mins = int((datetime.now(timezone.utc) - start).total_seconds() // 60)
    if mins < 1:
        return ""
    return f" · {mins // 60} h {mins % 60:02d} min" if mins >= 60 else f" · {mins} min"


class DiscordFeed:
    """status: "" = verbunden · sonst eine verständliche Meldung für Anzeige und Einstellungsseite."""

    def __init__(self, cfg: dict, on_change: Callable[[], None]):
        self.cfg = cfg
        self.on_change = on_change
        self.status = "Verbinde mit Discord …"
        self.bot_name, self.server_count = "", 0
        self.client = None
        self.avatars: dict[int, tuple[str, Image.Image]] = {}
        self.friend_ids = {int(x) for x in cfg.get("friend_ids", [])}
        self.ignore_ids = {int(x) for x in cfg.get("ignore_ids", [])}
        self.guild_ids = {int(x) for x in cfg.get("guild_ids", [])}

    @property
    def ready(self) -> bool:
        return self.client is not None and self.client.is_ready()

    # ---------- Verbindung ----------
    async def run(self) -> None:
        token = str(self.cfg.get("token", "")).strip()
        if not token:
            self.status = "Kein Bot-Token – bitte in den Einstellungen eintragen"
            self.on_change()
            return
        import discord
        discord.VoiceClient.warn_nacl = False  # Voice wird nicht gebraucht
        self._d = discord
        feed = self

        class Client(discord.Client):
            async def on_ready(self) -> None:
                guilds = [g for g in self.guilds if not feed.guild_ids or g.id in feed.guild_ids]
                feed.status = "" if guilds else "Der Bot ist auf keinem Server – bitte einladen (siehe Anleitung)"
                feed.bot_name, feed.server_count = str(self.user.name), len(guilds)  # für die Statusanzeige
                log.info("Discord verbunden – beobachte %d Server", len(guilds))
                feed.on_change()

            async def on_disconnect(self) -> None:
                feed.on_change()

            async def on_resumed(self) -> None:
                feed.on_change()

            async def on_presence_update(self, before, after) -> None:
                if feed._watched(after):
                    feed.on_change()

            async def on_member_update(self, before, after) -> None:
                if feed._watched(after):
                    feed.on_change()

            async def on_member_join(self, member) -> None:
                feed.on_change()

            async def on_member_remove(self, member) -> None:
                feed.on_change()

        intents = discord.Intents.none()
        intents.guilds = True
        intents.members = True     # privilegiert – im Developer Portal aktivieren
        intents.presences = True   # privilegiert – im Developer Portal aktivieren
        self.client = Client(intents=intents, chunk_guilds_at_startup=True)
        try:
            await self.client.start(token)
        except discord.PrivilegedIntentsRequired:
            self.status = "Discord verweigert den Zugriff: im Developer Portal beide „Intents“ einschalten"
        except discord.LoginFailure:
            self.status = "Bot-Token ungültig – bitte in den Einstellungen neu eintragen"
        except Exception as e:
            self.status = "Keine Verbindung zu Discord"
            log.warning("Discord: %s", e)
        else:
            return
        log.error("Discord: %s", self.status)
        self.on_change()

    async def close(self) -> None:
        if self.client is not None:
            await self.client.close()

    # ---------- Daten ----------
    def _watched(self, m) -> bool:
        c = self.client
        if m.bot or m.id in self.ignore_ids or (c.user and m.id == c.user.id):
            return False
        if self.guild_ids and m.guild.id not in self.guild_ids:
            return False
        return not self.friend_ids or m.id in self.friend_ids

    def _status_of(self, member) -> str:
        d, s = self._d, member.status
        if s == d.Status.online:
            return "online"
        if s == d.Status.idle:
            return "idle"
        if s in (d.Status.dnd, d.Status.do_not_disturb):
            return "dnd"
        return "offline"

    def _describe(self, member) -> str:
        d, custom = self._d, ""
        for act in member.activities:
            if isinstance(act, d.Streaming):
                return _clean(f"Streamt {act.game or act.name or ''}".strip())
            if isinstance(act, d.Spotify):
                return _clean(f"Hört {act.title} – {act.artist}")
            if isinstance(act, d.CustomActivity):
                custom = _clean(act.name or "")
                continue
            if act.type == d.ActivityType.playing:
                return _clean(f"Spielt {act.name}") + _duration(getattr(act, "start", None))
            if act.type == d.ActivityType.listening:
                return _clean(f"Hört {act.name}")
            if act.type == d.ActivityType.watching:
                return _clean(f"Schaut {act.name}")
            if act.type == d.ActivityType.competing:
                return _clean(f"Tritt an in {act.name}")
        return custom

    def _collect(self) -> list:
        seen: dict[int, object] = {}
        for g in self.client.guilds:
            for m in g.members:
                if not self._watched(m):
                    continue
                prev = seen.get(m.id)
                # Bei mehreren gemeinsamen Servern das Mitglied mit Aktivität bevorzugen
                if prev is None or (not prev.activities and m.activities):
                    seen[m.id] = m
        members = list(seen.values())
        if not self.cfg.get("show_offline"):
            members = [m for m in members if self._status_of(m) != "offline"]
        return members

    async def _avatar(self, m) -> Image.Image | None:
        asset = m.display_avatar.replace(size=64, format="png")
        cached = self.avatars.get(m.id)
        if cached and cached[0] == asset.key:
            return cached[1]
        try:
            img = Image.open(io.BytesIO(await asset.read()), formats=["PNG"]).convert("RGB")  # nur das angeforderte Format dekodieren
        except Exception as e:  # Netzwerkfehler o. Ä. → Initialen-Platzhalter
            log.debug("Avatar nicht geladen: %s", e)
            return cached[1] if cached else None
        self.avatars[m.id] = (asset.key, img)
        return img

    async def friends(self) -> list[Friend]:
        if not self.ready:
            return []
        out = []
        for m in self._collect():
            out.append(Friend(user_id=m.id, name=_clean(m.display_name) or m.name, status=self._status_of(m),
                              activity=self._describe(m), mobile=m.is_on_mobile(), avatar=await self._avatar(m)))
        return out

    def member_list(self) -> list[dict]:
        """Alle Mitglieder der beobachteten Server (für die Auswahl in den Einstellungen)."""
        if not self.ready:
            return []
        seen: dict[int, dict] = {}
        for g in self.client.guilds:
            if self.guild_ids and g.id not in self.guild_ids:
                continue
            for m in g.members:
                if not m.bot and m.id != self.client.user.id and m.id not in seen:
                    seen[m.id] = {"id": str(m.id), "name": _clean(m.display_name) or m.name, "guild": g.name}
        return sorted(seen.values(), key=lambda x: x["name"].casefold())
