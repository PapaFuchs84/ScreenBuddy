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
# Plattform-Kürzel in der Config → Parameter der Stats-API (PC-Konten laufen über "ea")
STATS_PLATFORM = {"pc": "ea", "ea": "ea", "steam": "ea", "epic": "ea",
                  "psn": "psn", "ps": "psn", "xbox": "xboxseries", "xbl": "xboxseries"}


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


def parse_entry(entry: str) -> tuple[str, str]:
    """"Name" oder "Name@psn" → (Name, Plattform)."""
    name, _, plat = entry.strip().rpartition("@")
    if not name or plat.lower() not in STATS_PLATFORM:
        return entry.strip(), "pc"
    return name, plat.lower()


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

    async def run(self, on_update: Callable[[], None]) -> None:
        timeout = aiohttp.ClientTimeout(total=45)
        async with aiohttp.ClientSession(timeout=timeout, headers={"User-Agent": "FuchsLabs-USB-Display/1.1"}) as s:
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
        name, plat = parse_entry(entry)
        p = Bf6Player(entry, name)
        stats = None
        for _ in range(2):  # die API antwortet gelegentlich sporadisch mit 404/500
            stats = await self._get(s, "stats", name=name, platform=STATS_PLATFORM[plat],
                                    format_values="false", categories="multiplayer")
            if stats:
                break
            await asyncio.sleep(2)
        if stats and not stats.get("hasResults", True):
            stats = None
        if stats:
            p.kd = stats.get("killDeath")
            p.win_pct = stats.get("winPercent")
            p.kpm = stats.get("killsPerMinute")
            secs = stats.get("secondsPlayed")
            p.hours = secs / 3600 if secs else None
        pp = None
        tries = ([{"playerid": stats["id"]}] if stats and stats.get("id") else []) + [{"name": name}]
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
