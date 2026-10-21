# © 2026 FuchsLabs
"""Neueste Beiträge aus RSS-/Atom-Feeds für die Blog-Seite.

In config.toml unter [rss] feeds stehen Feed- oder Blog-Adressen; bei einer Blog-Adresse
wird der Feed automatisch gesucht (<link rel="alternate"> bzw. /feed, /rss, /atom.xml).
"""
from __future__ import annotations

import asyncio
import html
import logging
import re
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from datetime import datetime, timezone
from email.utils import parsedate_to_datetime
from typing import Callable
from urllib.parse import urljoin, urlparse

import aiohttp

log = logging.getLogger("rss")

UA = {"User-Agent": "Mozilla/5.0 (FuchsLabs-USB-Display RSS)", "Accept": "application/rss+xml, application/atom+xml, "
      "application/xml;q=0.9, text/html;q=0.8, */*;q=0.5"}
ATOM = "{http://www.w3.org/2005/Atom}"
# Seiten, die ihren Feed nicht im HTML verlinken
KNOWN = {"heise.de": "https://www.heise.de/rss/heise-atom.xml"}


@dataclass
class Post:
    source: str
    title: str
    published: datetime | None
    link: str = ""


def _date(text: str | None) -> datetime | None:
    if not text:
        return None
    text = text.strip()
    try:
        d = parsedate_to_datetime(text)
    except (TypeError, ValueError):
        try:
            d = datetime.fromisoformat(text.replace("Z", "+00:00"))
        except ValueError:
            return None
    return d if d.tzinfo else d.replace(tzinfo=timezone.utc)


def _clean(text: str | None) -> str:
    return " ".join(html.unescape(re.sub(r"<[^>]+>", "", text or "")).split())


def parse_feed(data: bytes, fallback_name: str) -> tuple[str, list[Post]] | None:
    if b"<!ENTITY" in data:  # echte Feeds deklarieren keine Entitäten; schützt vor Entitäten-Bomben
        return None
    try:
        root = ET.fromstring(data)
    except ET.ParseError:
        return None
    tag = root.tag.lower()
    posts: list[Post] = []
    if tag.endswith("rss") or tag.endswith("rdf"):
        ch = root.find("channel")
        name = _clean(ch.findtext("title")) if ch is not None else ""
        items = root.iter("item") if tag.endswith("rss") else root.iter("{http://purl.org/rss/1.0/}item")
        for it in items:
            title = _clean(it.findtext("title") or it.findtext("{http://purl.org/rss/1.0/}title"))
            when = _date(it.findtext("pubDate") or it.findtext("{http://purl.org/dc/elements/1.1/}date"))
            link = (it.findtext("link") or "").strip()
            if title:
                posts.append(Post(name or fallback_name, title, when, link))
    elif root.tag == f"{ATOM}feed":
        name = _clean(root.findtext(f"{ATOM}title"))
        for e in root.iter(f"{ATOM}entry"):
            title = _clean(e.findtext(f"{ATOM}title"))
            when = _date(e.findtext(f"{ATOM}published") or e.findtext(f"{ATOM}updated"))
            le = e.find(f"{ATOM}link")
            if title:
                posts.append(Post(name or fallback_name, title, when, le.get("href", "") if le is not None else ""))
    else:
        return None
    return name or fallback_name, posts


def _discover(page: str, base: str) -> list[str]:
    links = []
    for m in re.finditer(r"<link\b[^>]*>", page, re.I):
        tag = m.group(0)
        if re.search(r'rel=["\']?alternate', tag, re.I) and re.search(r"application/(rss|atom)\+xml", tag, re.I):
            href = re.search(r'href=["\']([^"\']+)', tag, re.I)
            if href:
                links.append(urljoin(base, html.unescape(href.group(1))))
    root = f"{urlparse(base).scheme}://{urlparse(base).netloc}"
    return links + [urljoin(root, p) for p in ("/feed", "/rss", "/feed.xml", "/atom.xml", "/rss.xml", "/index.xml")]


class RssFeeds:
    def __init__(self, cfg: dict):
        self.entries = [str(e).strip() for e in cfg.get("feeds", []) if str(e).strip()]
        self.refresh = max(10, int(cfg.get("refresh_minutes", 30))) * 60
        self.max_items = int(cfg.get("max_items", 4))
        self.max_per_feed = int(cfg.get("max_per_feed", 1))
        self.posts: list[Post] = []
        self.updated: datetime | None = None
        self.failed: list[str] = []
        self._resolved: dict[str, str] = {}   # Eintrag → gefundene Feed-URL
        self._cache: dict[str, list[Post]] = {}

    async def run(self, on_update: Callable[[], None]) -> None:
        timeout = aiohttp.ClientTimeout(total=25)
        async with aiohttp.ClientSession(timeout=timeout, headers=UA) as s:
            while True:
                t = time.monotonic()
                results = await asyncio.gather(*(self._feed(s, e) for e in self.entries))
                self.failed = [e for e, r in zip(self.entries, results) if r is None]
                for e, r in zip(self.entries, results):
                    if r is not None:
                        self._cache[e] = r
                self.posts = self.select([p for e in self.entries for p in self._cache.get(e, [])])
                self.updated = datetime.now()
                log.info("RSS: %d/%d Feeds, %d Beiträge (%.1f s)", len(self.entries) - len(self.failed),
                         len(self.entries), len(self.posts), time.monotonic() - t)
                if self.failed:
                    log.warning("RSS: kein Feed gefunden für %s", ", ".join(self.failed))
                on_update()
                await asyncio.sleep(self.refresh)

    def select(self, posts: list[Post]) -> list[Post]:
        """Neueste zuerst, höchstens max_per_feed je Quelle, damit ein fleißiger Blog nicht alles füllt."""
        old = datetime(1970, 1, 1, tzinfo=timezone.utc)
        out, per = [], {}
        for p in sorted(posts, key=lambda p: p.published or old, reverse=True):
            if per.get(p.source, 0) < self.max_per_feed:
                per[p.source] = per.get(p.source, 0) + 1
                out.append(p)
        return out[:self.max_items]

    async def _get(self, s: aiohttp.ClientSession, url: str) -> tuple[bytes, str] | None:
        try:
            async with s.get(url, allow_redirects=True) as r:
                if r.status != 200:
                    return None
                data = bytearray()
                async for chunk in r.content.iter_chunked(65536):
                    data += chunk
                    if len(data) > 3_000_000:  # Schutz vor riesigen Seiten
                        break
                return bytes(data), str(r.url)
        except (aiohttp.ClientError, asyncio.TimeoutError, ValueError) as e:
            log.debug("RSS %s → %s", url, e)
            return None

    async def _feed(self, s: aiohttp.ClientSession, entry: str) -> list[Post] | None:
        url = entry if "://" in entry else f"https://{entry}"
        if urlparse(url).scheme not in ("http", "https"):
            return None
        name = urlparse(url).netloc.removeprefix("www.")
        if name in KNOWN and urlparse(url).path in ("", "/"):
            self._resolved.setdefault(entry, KNOWN[name])
        if entry in self._resolved:
            got = await self._get(s, self._resolved[entry])
            parsed = parse_feed(got[0], name) if got else None
            return parsed[1] if parsed else None
        got = await self._get(s, url)
        if not got:
            return None
        parsed = parse_feed(got[0], name)
        if parsed:
            self._resolved[entry] = url
            return parsed[1]
        page = got[0].decode("utf-8", "replace")
        for cand in _discover(page, got[1]):
            g = await self._get(s, cand)
            parsed = parse_feed(g[0], name) if g else None
            if parsed:
                log.info("RSS: Feed für %s gefunden: %s", entry, cand)
                self._resolved[entry] = cand
                return parsed[1]
        return None


def ago(d: datetime | None, now: datetime | None = None) -> str:
    if d is None:
        return ""
    now = now or datetime.now(timezone.utc)
    mins = int((now - d).total_seconds() // 60)
    if mins < 1:
        return "gerade eben"
    if mins < 60:
        return f"vor {mins} min"
    if mins < 48 * 60:
        return f"vor {mins // 60} h"
    return f"vor {mins // 1440} Tagen"


async def _check(entries: list[str], out: str | None) -> None:
    """Einmal abrufen und ausgeben: python rss.py blog1.de https://blog2.com/feed"""
    r = RssFeeds({"feeds": entries})

    async def once() -> None:
        task = asyncio.current_task()
        await r.run(lambda: task.cancel())
    try:
        await once()
    except asyncio.CancelledError:
        pass
    for e in entries:
        print(f"{e:40} → {r._resolved.get(e, 'KEIN FEED')}")
    for p in r.posts:
        print(f"  {ago(p.published):14} {p.source[:20]:20} {p.title[:70]}")
    if out:
        from render import RssRenderer
        RssRenderer((480, 320)).render(r.posts, updated=r.updated).save(out)
        print("Vorschau:", out)


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="RSS-Feeds testweise abrufen")
    ap.add_argument("feeds", nargs="+")
    ap.add_argument("--preview", metavar="PNG")
    a = ap.parse_args()
    asyncio.run(_check(a.feeds, a.preview))
