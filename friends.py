# © 2026 FuchsLabs
"""Gemeinsame Freundesliste: führt Discord, Steam und Xbox zu einer Liste zusammen.

* Steam: offizielle Web-API (kostenloser Schlüssel, Freundesliste im Profil muss öffentlich sein).
* Xbox: über den Dienst OpenXBL (kostenloser Schlüssel).
* Discord: kommt vom Bot (discord_feed.py).
Dieselbe Person auf mehreren Plattformen erscheint nur einmal – automatisch bei gleichem Namen
oder manuell über „Verknüpfungen“ (Einstellungsseite).
"""
from __future__ import annotations

import asyncio
import io
import logging
import re
import time
import unicodedata
from difflib import SequenceMatcher
from dataclasses import dataclass, field
from typing import Callable
from urllib.parse import urlparse

import aiohttp
from PIL import Image

from render import Friend, STATUS_ORDER, clean_text

log = logging.getLogger("friends")

STEAM_API = "https://api.steampowered.com"
XBOX_API = "https://xbl.io/api/v2"
STEAM_EVERY, XBOX_EVERY = 60, 180   # Sekunden zwischen Abrufen (schont die Limits der Schnittstellen)
UA = {"User-Agent": "FuchsLabs-USB-Display/1.2"}
MAX_AVATAR_BYTES = 2_000_000
STEAM_STATE = {0: "offline", 1: "online", 2: "dnd", 3: "idle", 4: "idle", 5: "online", 6: "online"}
PLATFORMS = ("discord", "steam", "xbox")


@dataclass
class Account:
    platform: str
    uid: str
    name: str
    status: str = "offline"       # online | idle | dnd | offline
    activity: str = ""
    mobile: bool = False
    avatar_url: str = ""
    avatar: Image.Image | None = field(default=None, repr=False)


class FriendsError(Exception):
    """Fehler mit einer Meldung, die der Nutzer versteht."""


# ---------- Steam ----------
async def _steam_get(s: aiohttp.ClientSession, path: str, **params) -> dict:
    try:
        async with s.get(f"{STEAM_API}/{path}", params=params) as r:
            if r.status in (401, 403):
                raise FriendsError("Steam: Schlüssel ungültig oder die Freundesliste im Profil ist nicht öffentlich")
            if r.status == 429:
                raise FriendsError("Steam: zu viele Abrufe, ich versuche es später wieder")
            if r.status != 200:
                raise FriendsError(f"Steam antwortet mit Fehler {r.status}")
            return await r.json(content_type=None)
    except (aiohttp.ClientError, asyncio.TimeoutError) as e:
        raise FriendsError("Steam ist nicht erreichbar") from e


async def resolve_steam_id(s: aiohttp.ClientSession, key: str, text: str) -> str:
    """17-stellige Steam-ID, Profil-Link (…/profiles/ID oder …/id/Name/) oder Profilname → Steam-ID."""
    text = text.strip().rstrip("/")
    if re.fullmatch(r"\d{17}", text):
        return text
    path = urlparse(text if "://" in text else f"https://x/{text}").path.strip("/").split("/")
    if len(path) >= 2 and path[0] == "profiles" and re.fullmatch(r"\d{17}", path[1]):
        return path[1]
    vanity = path[1] if len(path) >= 2 and path[0] == "id" else path[-1]
    data = await _steam_get(s, "ISteamUser/ResolveVanityURL/v1/", key=key, vanityurl=vanity)
    resp = data.get("response", {})
    if resp.get("success") != 1:
        raise FriendsError("Steam: Profil nicht gefunden – bitte den Link aus der Adresszeile deines Profils einfügen")
    return str(resp["steamid"])


def parse_steam_players(data: dict) -> list[Account]:
    out = []
    for p in data.get("response", {}).get("players", []):
        game = clean_text(p.get("gameextrainfo", ""))
        out.append(Account("steam", str(p["steamid"]), clean_text(p.get("personaname", "")) or "?",
                           STEAM_STATE.get(p.get("personastate", 0), "offline"),
                           f"Spielt {game}" if game else "", avatar_url=p.get("avatarmedium", "")))
    return out


async def fetch_steam(s: aiohttp.ClientSession, key: str, steam_id: str) -> list[Account]:
    sid = await resolve_steam_id(s, key, steam_id)
    data = await _steam_get(s, "ISteamUser/GetFriendList/v1/", key=key, steamid=sid, relationship="friend")
    ids = [str(f["steamid"]) for f in data.get("friendslist", {}).get("friends", [])]
    out: list[Account] = []
    for i in range(0, len(ids), 100):  # höchstens 100 pro Abruf
        part = await _steam_get(s, "ISteamUser/GetPlayerSummaries/v2/", key=key, steamids=",".join(ids[i:i + 100]))
        out += parse_steam_players(part)
    return out


# ---------- Xbox (OpenXBL) ----------
def parse_xbox_people(data: dict) -> list[Account]:
    out = []
    body = data.get("content") if isinstance(data.get("content"), dict) else data  # v2 verpackt die Antwort in "content"
    for p in body.get("people", []):
        online = str(p.get("presenceState", "")).lower() == "online"
        text = clean_text(str(p.get("presenceText", "") or ""))
        activity = ""
        if online and text and text.lower() not in ("online", "home", "startseite"):
            activity = re.sub(r"^Playing\s+", "Spielt ", text)
            if not activity.startswith(("Spielt", "Streamt")):
                activity = "Spielt " + activity
        out.append(Account("xbox", str(p.get("xuid") or p.get("gamertag", "")),
                           clean_text(p.get("gamertag") or p.get("displayName") or "") or "?",
                           "online" if online else "offline", activity,
                           avatar_url=str(p.get("displayPicRaw", "") or "")))
    return out


async def fetch_xbox(s: aiohttp.ClientSession, key: str) -> list[Account]:
    try:
        async with s.get(f"{XBOX_API}/friends", headers={"X-Authorization": key, "Accept": "application/json"}) as r:
            if r.status in (401, 403):
                raise FriendsError("Xbox: OpenXBL-Schlüssel ungültig")
            if r.status == 429:
                raise FriendsError("Xbox: Abruflimit von OpenXBL erreicht, ich versuche es später wieder")
            if r.status != 200:
                raise FriendsError(f"OpenXBL antwortet mit Fehler {r.status}")
            return parse_xbox_people(await r.json(content_type=None))
    except (aiohttp.ClientError, asyncio.TimeoutError) as e:
        raise FriendsError("OpenXBL ist nicht erreichbar") from e


# ---------- Zusammenführen ----------
def _norm(name: str) -> str:
    plain = unicodedata.normalize("NFKD", name.casefold())  # Akzente und Sonderformen vereinheitlichen
    return re.sub(r"[^0-9a-z]", "", plain)


def parse_links(links: list[str]) -> list[tuple[str, list[tuple[str, str]]]]:
    """"Name|discord:123|steam:765…|xbox:Gamertag" → [(Name, [(plattform, id), …])]"""
    out = []
    for entry in links:
        name, *members = [x.strip() for x in entry.split("|")]
        pairs = [tuple(m.split(":", 1)) for m in members if ":" in m]
        if name and pairs:
            out.append((name, [(p.lower(), i) for p, i in pairs]))
    return out


def _best(accs: list[Account]) -> Account:
    return min(accs, key=lambda a: (STATUS_ORDER.get(a.status, 9), not a.activity, not a.avatar))


def merge_accounts(accounts: list[Account], links: list[str], auto_merge: bool) -> list[Friend]:
    """Eine Zeile je Person. Reihenfolge der Regeln: manuelle Verknüpfungen, dann gleicher Name."""
    groups: list[tuple[str, list[Account]]] = []
    used: set[int] = set()
    index = {(a.platform, a.uid.casefold()): a for a in accounts}
    index.update({(a.platform, a.name.casefold()): a for a in accounts})
    for name, members in parse_links(links):
        found = []
        for plat, ident in members:
            a = index.get((plat, ident.casefold()))
            if a and id(a) not in used and all(f.platform != a.platform for f in found):
                found.append(a)
                used.add(id(a))
        if found:
            groups.append((name, found))
    if auto_merge:
        by_name: dict[str, list[Account]] = {}
        for a in accounts:
            if id(a) not in used:
                by_name.setdefault(_norm(a.name), []).append(a)
        for key, accs in by_name.items():
            # nie zwei Konten derselben Plattform zusammenfassen
            seen, merged, rest = set(), [], []
            for a in accs:
                (rest if a.platform in seen else merged).append(a)
                seen.add(a.platform)
            if key and len(merged) > 1:
                groups.append((merged[0].name, merged))
                used.update(id(a) for a in merged)
                for a in rest:
                    groups.append((a.name, [a]))
                    used.add(id(a))
    for a in accounts:
        if id(a) not in used:
            groups.append((a.name, [a]))
    out = []
    for name, accs in groups:
        # Online hat Vorrang; die Aktivität stammt vom Konto, das gerade spielt
        top = _best(accs)
        playing = next((a for a in accs if a.activity.startswith(("Spielt", "Streamt")) and a.status != "offline"), None)
        out.append(Friend(user_id=abs(hash((name.casefold(), accs[0].platform, accs[0].uid))) & 0xFFFFFFFF, name=name,
                          status=top.status, activity=(playing or top).activity, mobile=top.mobile,
                          avatar=next((a.avatar for a in [top, *accs] if a.avatar), None),
                          platforms=tuple(sorted({a.platform for a in accs}, key=PLATFORMS.index))))
    return out


def suggest_links(accounts: list[dict], links: list[str], auto_merge: bool, limit: int = 12) -> list[dict]:
    """Vorschläge für Konten, die vermutlich dieselbe Person sind (ähnlicher, aber nicht gleicher Name),
    damit der Nutzer sie mit einem Klick verknüpfen kann. Bereits verknüpfte Paare entfallen."""
    linked: set[frozenset] = set()
    for _, members in parse_links(links):
        keys = [f"{p}:{i.casefold()}" for p, i in members]
        linked |= {frozenset((a, b)) for a in keys for b in keys if a != b}
    out = []
    for n, a in enumerate(accounts):
        for b in accounts[n + 1:]:
            if a["platform"] == b["platform"]:
                continue
            ka, kb = _norm(a["name"]), _norm(b["name"])
            if not ka or not kb:
                continue
            if frozenset((f"{a['platform']}:{a['id'].casefold()}", f"{b['platform']}:{b['id'].casefold()}")) in linked:
                continue
            if ka == kb:
                if auto_merge:
                    continue  # wird ohnehin automatisch zusammengefasst
                score = 1.0
            else:
                short, long_ = sorted((ka, kb), key=len)
                score = SequenceMatcher(None, ka, kb).ratio()
                if len(short) >= 4 and short in long_:
                    score = max(score, 0.85)
                if score < 0.8:
                    continue
            out.append({"a": a, "b": b, "score": round(score, 2)})
    return sorted(out, key=lambda x: -x["score"])[:limit]


async def steam_self_name(s: aiohttp.ClientSession, key: str, steam_id: str) -> str:
    """Name des eigenen Steam-Kontos (für die Statusanzeige); Fehler sind hier egal."""
    try:
        sid = await resolve_steam_id(s, key, steam_id)
        data = await _steam_get(s, "ISteamUser/GetPlayerSummaries/v2/", key=key, steamids=sid)
        return clean_text((data.get("response", {}).get("players") or [{}])[0].get("personaname", ""))
    except FriendsError:
        return ""


async def xbox_self_name(s: aiohttp.ClientSession, key: str) -> str:
    """Gamertag des eigenen Xbox-Kontos (für die Statusanzeige); Fehler sind hier egal."""
    try:
        async with s.get(f"{XBOX_API}/account", headers={"X-Authorization": key, "Accept": "application/json"}) as r:
            if r.status != 200:
                return ""
            body = await r.json(content_type=None)
        users = (body.get("content") or body).get("profileUsers") or [{}]
        return clean_text(next((x.get("value", "") for x in users[0].get("settings", []) if x.get("id") == "Gamertag"), ""))
    except (aiohttp.ClientError, asyncio.TimeoutError, ValueError):
        return ""


# ---------- Dienst ----------
class FriendsHub:
    def __init__(self, cfg: dict, discord_feed, on_change: Callable[[], None]):
        self.cfg, self.discord, self.on_change = cfg, discord_feed, on_change
        self.accounts: dict[str, list[Account]] = {"steam": [], "xbox": []}
        self.errors: dict[str, str] = {}
        self.info: dict[str, dict] = {}     # je Plattform: ok, name, count, online, at, error (für die Statusanzeige)
        self.updated: float | None = None
        self._avatars: dict[str, Image.Image | None] = {}

    @property
    def status(self) -> str:
        return " · ".join(self.errors.values())

    async def run(self) -> None:
        timeout = aiohttp.ClientTimeout(total=30)
        async with aiohttp.ClientSession(timeout=timeout, headers=UA) as s:
            jobs = []
            if self.cfg.get("steam_api_key") and self.cfg.get("steam_id"):
                jobs.append(self._loop(s, "steam", STEAM_EVERY,
                                       lambda: fetch_steam(s, self.cfg["steam_api_key"], self.cfg["steam_id"]),
                                       lambda: steam_self_name(s, self.cfg["steam_api_key"], self.cfg["steam_id"])))
            if self.cfg.get("xbox_api_key"):
                jobs.append(self._loop(s, "xbox", XBOX_EVERY, lambda: fetch_xbox(s, self.cfg["xbox_api_key"]),
                                       lambda: xbox_self_name(s, self.cfg["xbox_api_key"])))
            if not jobs:
                self.updated = time.time()
                self.on_change()
                return
            await asyncio.gather(*jobs)

    async def _loop(self, s, platform: str, every: int, fetch, who=None) -> None:
        while True:
            try:
                accs = await fetch()
                await self._load_avatars(s, accs)
                self.accounts[platform] = accs
                self.errors.pop(platform, None)
                info = self.info.setdefault(platform, {})
                if who and not info.get("name"):
                    info["name"] = await who()   # Kontoname nur einmal holen (schont die Abruflimits)
                info.update(ok=True, error="", count=len(accs), online=sum(1 for a in accs if a.status != "offline"),
                            at=time.time())
            except FriendsError as e:
                self.errors[platform] = str(e)  # alte Daten bleiben stehen
                self.info.setdefault(platform, {}).update(ok=False, error=str(e))
                log.warning("%s", e)
            except Exception:
                self.errors[platform] = f"{platform.capitalize()}: unerwarteter Fehler"
                self.info.setdefault(platform, {}).update(ok=False, error=self.errors[platform])
                log.exception("Freunde (%s)", platform)
            self.updated = time.time()
            self.on_change()
            await asyncio.sleep(every)

    async def _load_avatars(self, s: aiohttp.ClientSession, accs: list[Account]) -> None:
        async def one(a: Account) -> None:
            if not a.avatar_url:
                return
            if a.avatar_url not in self._avatars:
                try:
                    data = bytearray()
                    async with s.get(a.avatar_url) as r:
                        if r.status == 200:
                            async for chunk in r.content.iter_chunked(65536):
                                data += chunk
                                if len(data) > MAX_AVATAR_BYTES:  # fremde Bilder: Größe begrenzen
                                    raise ValueError("Avatar zu groß")
                    # nur Standardformate öffnen (fremde Daten!), nie exotische Plugins
                    img = Image.open(io.BytesIO(bytes(data)), formats=["PNG", "JPEG", "WEBP"]).convert("RGB")
                    img.thumbnail((64, 64))
                    self._avatars[a.avatar_url] = img
                except Exception:
                    self._avatars[a.avatar_url] = None
            a.avatar = self._avatars[a.avatar_url]
        await asyncio.gather(*(one(a) for a in accs))

    async def people(self) -> list[Friend]:
        accounts: list[Account] = []
        if self.cfg.get("include_discord", True) and self.discord:
            for f in await self.discord.friends():
                accounts.append(Account("discord", str(f.user_id), f.name, f.status, f.activity, f.mobile, avatar=f.avatar))
        for plat in ("steam", "xbox"):
            accounts += self.accounts[plat]
        people = merge_accounts(accounts, list(self.cfg.get("links", [])), bool(self.cfg.get("auto_merge", True)))
        if not self.cfg.get("show_offline"):
            people = [p for p in people if p.status != "offline"]
        return people

    async def all_accounts(self) -> list[dict]:
        """Alle bekannten Konten (für den Verknüpfungs-Editor der Einstellungsseite)."""
        out = [{"platform": a.platform, "id": a.uid, "name": a.name} for p in ("steam", "xbox") for a in self.accounts[p]]
        if self.discord:
            out += [{"platform": "discord", "id": m["id"], "name": m["name"]} for m in self.discord.member_list()]
        return sorted(out, key=lambda x: (x["name"].casefold(), x["platform"]))
