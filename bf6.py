# © 2026 FuchsLabs
"""Battlefield-6-Rangliste der Freunde – Daten von der Community-API api.gametools.network.

EA bietet keine öffentliche Freundesliste an; die Spielernamen (EA-ID bzw. PSN/Xbox-Name)
stehen deshalb in config.toml unter [bf6] players.
"""
from __future__ import annotations

import asyncio
import io
import logging
import time
from dataclasses import dataclass, field
from datetime import datetime
from typing import Callable

import aiohttp
from PIL import Image

log = logging.getLogger("bf6")

API = "https://api.gametools.network/bf6"
# Plattform-Kürzel in der Config → Werte der Stats-API in Reihenfolge der Versuche (wie cpp/src/bf6.cpp).
# Seit Nov. 2026 antwortet die API für PC-Konten nur noch mit "pc" ("ea" liefert 404); "ea" bleibt als Rückfall.
_PC, _PSN, _XBOX = ["pc", "ea"], ["psn", "ps5", "ps4"], ["xboxseries", "xboxone", "xbox"]
STATS_PLATFORM = {"pc": _PC, "ea": _PC, "steam": _PC, "epic": _PC,
                  "psn": _PSN, "ps": _PSN, "xbox": _XBOX, "xbl": _XBOX}

@dataclass
class Bf6Player:
    query: str                          # Eintrag aus der Config
    name: str
    rank: int | None = None
    rank_name: str = ""
    kd: float | None = None
    win_pct: float | None = None
    kpm: float | None = None
    hours: float | None = None
    rank_icon: Image.Image | None = field(default=None, repr=False)
    error: str = ""                     # z. B. "nicht gefunden"
    id: str = ""                        # gefundene Spieler-ID (zum Übernehmen als "Name#id")


def parse_entry_full(entry: str) -> tuple[str, str, str]:
    """"Name", "Name@psn", "Name#id", "Name@psn#id" → (Name, Plattform, ID).

    Als ID gelten nur reine Ziffern ab 6 Stellen (Gamertag-Zahlen wie "Fox#1234" bleiben im Namen)."""
    e, pid = entry.strip(), ""
    head, sep, tail = e.rpartition("#")
    if sep and head and len(tail) >= 6 and tail.isascii() and tail.isdigit():
        e, pid = head.strip(), tail
    name, _, plat = e.rpartition("@")
    if not name or plat.lower() not in STATS_PLATFORM:
        return e, "pc", pid
    return name, plat.lower(), pid


def parse_entry(entry: str) -> tuple[str, str]:
    """"Name" oder "Name@psn" (jeweils auch mit "#id") → (Name, Plattform)."""
    name, plat, _ = parse_entry_full(entry)
    return name, plat


def platform_candidates(plat: str) -> list[str]:
    return STATS_PLATFORM.get(plat.lower(), _PC)


def name_candidates(name: str) -> list[str]:
    """Original, danach klein geschrieben (die API findet manche Konten nur so)."""
    return [name] if name.lower() == name else [name, name.lower()]


def json_id(v) -> str:
    """ID aus Zahl oder Text; leer bei None/0/False."""
    if isinstance(v, bool) or v is None:
        return ""
    s = str(v).strip()
    return "" if s in ("0", "false") else s


def pick_search_id(resp, name: str) -> str:
    """Treffer der Spielersuche (/bf6/player) → ID; nur Treffer mit gleichem Namen (exakt vor ohne Groß/Klein)."""
    items = resp.get("results") if isinstance(resp, dict) else resp
    if not isinstance(items, list):
        return ""
    ci = ""
    for it in items:
        if not isinstance(it, dict):
            continue
        got = next((it[k] for k in ("name", "userName", "displayName", "platformUserHandle") if it.get(k) is not None), None)
        pid = json_id(next((it[k] for k in ("id", "playerId", "personaId", "nucleusId") if it.get(k) is not None), None))
        if not isinstance(got, str) or not pid:
            continue
        if got == name:
            return pid
        if not ci and got.lower() == name.lower():
            ci = pid
    return ci

def _profile(data: dict | None) -> dict | None:
    """Spielerprofil aus /profile – die API liefert zwei Formate (mit und ohne "other")."""
    try:
        if "other" in data:
            data = data["other"][0]
        pp = data["playerProfiles"][0]
    except (TypeError, KeyError, IndexError):
        return None
    card = pp.get("playerCard") or pp
    return pp if card.get("rank") is not None else None


def sort_players(players: list[Bf6Player]) -> list[Bf6Player]:
    return sorted(players, key=lambda p: (p.rank is None, -(p.rank or 0), -(p.kd or 0), p.name.casefold()))


class Bf6Stats:
    def __init__(self, cfg: dict):
        self.entries = [str(e) for e in cfg.get("players", []) if str(e).strip()]
        self.refresh = max(5, int(cfg.get("refresh_minutes", 15))) * 60
        self.players: list[Bf6Player] = [Bf6Player(e, parse_entry(e)[0]) for e in self.entries]
        self.updated: datetime | None = None
        self._icons: dict[str, Image.Image] = {}
        self._ids: dict[str, str] = {}      # Eintrag → zuletzt gefundene ID (Auto-Heilung im Speicher)
        self.on_heal: Callable[[str, str], None] | None = None  # (Eintrag, Eintrag#id) – Aufrufer darf in die Config schreiben

    async def run(self, on_update: Callable[[], None]) -> None:
        timeout = aiohttp.ClientTimeout(total=45)
        async with aiohttp.ClientSession(timeout=timeout, headers={"User-Agent": "FuchsLabs-USB-Display/1.2"}) as s:
            while True:
                t = time.monotonic()
                fresh = []
                for entry in self.entries:
                    fresh.append(await self._fetch(s, entry))
                    await asyncio.sleep(1)  # API schonen
                self.players = fresh
                self.updated = datetime.now()
                ok = sum(1 for p in fresh if not p.error)
                log.info("BF6: %d/%d Profile aktualisiert (%.1f s)", ok, len(fresh), time.monotonic() - t)
                on_update()
                await asyncio.sleep(self.refresh)

    async def _get(self, s: aiohttp.ClientSession, path: str, **params) -> dict | None:
        try:
            async with s.get(f"{API}/{path}/", params=params) as r:
                if r.status != 200:
                    log.debug("BF6 %s %s → HTTP %s", path, params, r.status)
                    return None
                return await r.json(content_type=None)
        except (aiohttp.ClientError, asyncio.TimeoutError, ValueError) as e:
            log.debug("BF6 %s %s → %s", path, params, e)
            return None

    async def _fetch(self, s: aiohttp.ClientSession, entry: str) -> Bf6Player:
        name, plat, pinned = parse_entry_full(entry)
        p = Bf6Player(entry, name)
        pid = pinned or self._ids.get(entry, "")   # mit ID wird per playerid abgefragt, der Name dient nur der Anzeige
        stats, used_plat, used_name, by_name = None, platform_candidates(plat)[0], name, False
        common = dict(format_values="false", categories="multiplayer")

        async def by_id(sid: str, pls: list[str]) -> bool:
            nonlocal stats, used_plat
            for pl in pls:
                r = await self._get(s, "stats", playerid=sid, platform=pl, **common)
                if r and r.get("hasResults", True):
                    stats, used_plat = r, pl
                    return True
            return False

        for i in range(2):  # die API antwortet gelegentlich sporadisch mit 404/500
            pls, nms = platform_candidates(plat), name_candidates(name)
            if i:  # Wiederholung nur mit dem ersten Paar, damit die API nicht überlastet wird
                pls, nms = pls[:1], nms[:1]
            if pid and await by_id(pid, pls):
                break
            for pl in pls:
                for nm in nms:
                    r = await self._get(s, "stats", name=nm, platform=pl, **common)
                    if r and r.get("hasResults", True):
                        stats, used_plat, used_name, by_name = r, pl, nm, True
                        break
                if stats:
                    break
            if stats:
                break
            if not i:
                await asyncio.sleep(2)
        if not stats:  # Letzter Ausweg: Spielersuche, nur Treffer mit gleichem Namen
            for nm in name_candidates(name):
                sid = pick_search_id(await self._get(s, "player", name=nm, limit=10), name)
                if sid and sid != pid and await by_id(sid, platform_candidates(plat)):
                    by_name = True
                    break
        if stats:
            p.kd = stats.get("killDeath")
            p.win_pct = stats.get("winPercent")
            p.kpm = stats.get("killsPerMinute")
            secs = stats.get("secondsPlayed")
            p.hours = secs / 3600 if secs else None
            p.id = json_id(stats.get("id")) or pid
            # Auto-Heilung: gefundene ID merken und einmalig melden, sofern der Name zur Antwort passt
            got = stats.get("userName") or stats.get("name")
            same = not isinstance(got, str) or got.lower() == name.lower()
            if by_name and not pinned and p.id and same:
                first = entry not in self._ids
                self._ids[entry] = p.id
                if first and self.on_heal:
                    self.on_heal(entry, f"{entry.strip()}#{p.id}")
        pp = None
        tries = ([{"playerid": p.id, "platform": used_plat}] if p.id else []) + [{"name": used_name, "platform": used_plat}]
        for params in tries:
            pp = _profile(await self._get(s, "profile", **params))
            if pp:
                break
        if pp:
            card = pp.get("playerCard") or pp
            p.rank = card.get("rank")
            p.rank_name = pp.get("rankName") or card.get("rankName") or ""
            p.rank_icon = await self._icon(s, (card.get("rankImage") or {}).get("small"))
        if not stats and not pp:
            p.error = "nicht gefunden"
            # Bei Netzwerkfehlern lieber die alten Werte behalten
            old = next((o for o in self.players if o.query == entry and (o.rank or o.kd) is not None), None)
            if old:
                return old
        return p

    async def _icon(self, s: aiohttp.ClientSession, url: str | None) -> Image.Image | None:
        if not url:
            return None
        if url not in self._icons:
            try:
                async with s.get(url) as r:
                    r.raise_for_status()
                    # nur gängige Webformate dekodieren (kleinere Angriffsfläche für fremde Bilddaten)
                    self._icons[url] = Image.open(io.BytesIO(await r.read()),
                                                  formats=["PNG", "JPEG", "WEBP", "GIF"]).convert("RGBA")
            except Exception as e:
                log.debug("Rang-Icon %s nicht geladen: %s", url, e)
                return None
        return self._icons[url]


async def _check(entries: list[str], out: str | None) -> None:
    """Einmal abrufen und ausgeben – zum Prüfen der Namen: python bf6.py Name1 Name2@psn"""
    st = Bf6Stats({"players": entries})

    async def once() -> None:
        task = asyncio.current_task()
        await st.run(lambda: task.cancel())
    try:
        await once()
    except asyncio.CancelledError:
        pass
    for p in sort_players(st.players):
        print(f"{p.query:24} → {p.error or f'Rang {p.rank} ({p.rank_name}), K/D {p.kd}, Siege {p.win_pct} %'}")
    if out:
        from render import Bf6Renderer
        Bf6Renderer((480, 320)).render(st.players, updated=st.updated).save(out)
        print("Vorschau:", out)


def demo_players() -> list[Bf6Player]:
    return [
        Bf6Player("Fuchs", "Fuchs", 174, "First Lieutenant III", 3.02, 51.6, 0.74, 189.9),
        Bf6Player("Basti", "Basti", 121, "Captain I", 1.48, 54.2, 0.61, 96.3),
        Bf6Player("Lena", "Lena", 88, "Sergeant Major II", 1.12, 49.0, 0.52, 61.0),
        Bf6Player("Kevin", "Kevin", 57, "Staff Sergeant III", 0.94, 47.5, 0.44, 40.2),
        Bf6Player("Mara", "Mara", 33, "Corporal II", 0.81, 50.3, 0.39, 18.7),
        Bf6Player("Tippfehler", "Tippfehler", error="nicht gefunden"),
    ]


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="BF6-Profile testweise abrufen")
    ap.add_argument("players", nargs="+")
    ap.add_argument("--preview", metavar="PNG")
    a = ap.parse_args()
    asyncio.run(_check(a.players, a.preview))
