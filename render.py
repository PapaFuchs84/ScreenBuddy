# © 2026 FuchsLabs
"""Zeichnet die Freundesliste als Pillow-Bild für das 3,5"-Display."""
from __future__ import annotations

import os
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

import brand

HERE = Path(__file__).resolve().parent

# FuchsLabs-CI (brand.py): Creme-Grund, Fuchs-Orange als Fläche, Navy als Schrift
BG = brand.CREME
CARD = brand.SAND
LINE = brand.LINE
ORANGE = brand.ORANGE      # nur für Flächen (Balken, Kopfzeile, Avatare)
ACCENT = brand.GLUT        # Orange als Schrift – auf Creme besser lesbar
TEXT = brand.NAVY
MUTED = brand.NEBEL
STATUS_COLOR = {           # Discord-Statusfarben, für hellen Grund abgedunkelt
    "online": (22, 140, 72),
    "idle": (200, 140, 0),
    "dnd": (215, 45, 50),
    "offline": (140, 140, 140),
}


def set_theme(name: str) -> None:
    """Hell (Creme-Grund) oder dunkel (Navy-Grund); Kopfzeile bleibt in beiden Fuchs-Orange."""
    global BG, CARD, LINE, ACCENT, TEXT, MUTED, WARN, HOT, DOWN
    if name == "dunkel":
        BG, CARD, LINE = brand.NAVY_TIEF, brand.NAVY, brand.NAVY_LINIE
        ACCENT, TEXT, MUTED = brand.ORANGE, brand.CREME, (150, 160, 180)
        status = {"online": (35, 165, 90), "idle": (240, 178, 50), "dnd": (242, 63, 67), "offline": (128, 132, 142)}
        WARN, HOT, DOWN = (240, 178, 50), (242, 63, 67), (35, 165, 90)
    else:
        BG, CARD, LINE = brand.CREME, brand.SAND, brand.LINE
        ACCENT, TEXT, MUTED = brand.GLUT, brand.NAVY, brand.NEBEL
        status = {"online": (22, 140, 72), "idle": (200, 140, 0), "dnd": (215, 45, 50), "offline": (140, 140, 140)}
        WARN, HOT, DOWN = (190, 120, 0), (215, 45, 50), (22, 140, 72)
    STATUS_COLOR.update(status)


STATUS_ORDER = {"online": 0, "idle": 1, "dnd": 2, "offline": 3}
STATUS_LABEL = {"online": "Online", "idle": "Abwesend", "dnd": "Bitte nicht stören", "offline": "Offline"}

HEADER_H = 34
FOOTER_H = 18
ROW_H = 44
AVATAR = 32


@dataclass
class Friend:
    user_id: int
    name: str
    status: str                 # online | idle | dnd | offline
    activity: str = ""          # z. B. "Spielt Battlefield 6 · 23 min"
    mobile: bool = False
    avatar: Image.Image | None = field(default=None, repr=False)
    platforms: tuple = ()       # z. B. ("discord", "steam") – kleine Plattform-Marken rechts in der Zeile


def _font_candidates(bold: bool) -> list[str]:
    win = Path(os.environ.get("WINDIR", r"C:\Windows")) / "Fonts"
    if bold:
        return [str(win / "segoeuib.ttf"), str(win / "arialbd.ttf"),
                "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"]
    return [str(win / "segoeui.ttf"), str(win / "arial.ttf"),
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"]


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    if bold:
        # CI-Schrift für Titel, Namen, Zahlen: Bahnschrift SemiBold (ab Windows 10 dabei, schmal und klar)
        bahn = Path(os.environ.get("WINDIR", r"C:\Windows")) / "Fonts" / "bahnschrift.ttf"
        if bahn.exists():
            try:
                f = ImageFont.truetype(str(bahn), size + 1)
                f.set_variation_by_name("SemiBold")
                return f
            except OSError:
                pass
    for path in _font_candidates(bold):
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size)


def clean_text(text: str) -> str:
    """Entfernt Emoji/Symbole, die Systemschriften auf dem Display als Kästchen zeigen würden."""
    out = []
    for ch in text:
        o = ord(ch)
        if o >= 0x1F000 or 0x2600 <= o <= 0x27BF or 0xFE00 <= o <= 0xFE0F or o == 0x200D:
            continue
        out.append(ch)
    return " ".join("".join(out).split())


def _fit(draw: ImageDraw.ImageDraw, text: str, font, max_w: int) -> str:
    text = clean_text(text)
    if draw.textlength(text, font=font) <= max_w:
        return text
    while text and draw.textlength(text + "…", font=font) > max_w:
        text = text[:-1]
    return text + "…"


def _round_avatar(img: Image.Image | None, name: str, size: int) -> Image.Image:
    if img is None:
        # Platzhalter: Initiale auf Orange
        img = Image.new("RGB", (size, size), ORANGE)
        d = ImageDraw.Draw(img)
        f = load_font(int(size * 0.55), bold=True)
        ch = (name[:1] or "?").upper()
        d.text((size / 2, size / 2), ch, font=f, fill=brand.NAVY, anchor="mm")
    img = img.convert("RGB").resize((size, size), Image.LANCZOS)
    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, size * 4 - 1, size * 4 - 1), fill=255)
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.paste(img, (0, 0), mask.resize((size, size), Image.LANCZOS))
    return out


class Renderer:
    def __init__(self, size: tuple[int, int], title: str = "Discord-Freunde"):
        self.w, self.h = size
        self.title = title
        self.f_title = load_font(17, bold=True)
        self.f_small = load_font(12)
        self.f_name = load_font(16, bold=True)
        self.f_act = load_font(13)
        self.f_big = load_font(18, bold=True)
        self.icon = brand.fox(26, badge=False)
        self._avatar_cache: dict[tuple[int, int], Image.Image] = {}

    @property
    def rows_per_page(self) -> int:
        return max(1, (self.h - HEADER_H - FOOTER_H) // ROW_H)

    def pages(self, friends: list[Friend]) -> int:
        return max(1, -(-len(friends) // self.rows_per_page))

    @staticmethod
    def sort(friends: list[Friend]) -> list[Friend]:
        return sorted(friends, key=lambda f: (STATUS_ORDER.get(f.status, 9), f.name.casefold()))

    def _avatar(self, f: Friend) -> Image.Image:
        key = (f.user_id, id(f.avatar))
        if key not in self._avatar_cache:
            self._avatar_cache[key] = _round_avatar(f.avatar, f.name, AVATAR)
        return self._avatar_cache[key]

    def _message(self, d: ImageDraw.ImageDraw, text: str) -> None:
        """Mehrzeilige, zentrierte Hinweis- oder Fehlermeldung (bis zu 4 Zeilen)."""
        words, lines, cur = clean_text(text).split(), [], ""
        for w in words:
            test = f"{cur} {w}".strip()
            if d.textlength(test, font=self.f_big) <= self.w - 40:
                cur = test
            else:
                lines.append(cur)
                cur = w
        lines = (lines + [cur])[:4]
        y = self.h / 2 - (len(lines) - 1) * 14
        for ln in lines:
            d.text((self.w / 2, y), ln, font=self.f_big, fill=MUTED, anchor="mm")
            y += 28

    def welcome(self, title: str, text: str) -> Image.Image:
        """Startbild im CI: großer Fuchs auf Orange, Wortmarke, darunter der Hinweis."""
        img = Image.new("RGB", (self.w, self.h), BG)
        d = ImageDraw.Draw(img)
        band = int(self.h * 0.42)
        d.rectangle((0, 0, self.w, band), fill=ORANGE)
        fox = brand.fox(int(band * 0.62), badge=False)
        d.text((self.w / 2 + fox.width / 2 + 4, band / 2), "FuchsLabs", font=load_font(30, bold=True),
               fill=brand.NAVY, anchor="mm")
        tw = d.textlength("FuchsLabs", font=load_font(30, bold=True))
        img.paste(fox, (int(self.w / 2 - tw / 2 - fox.width / 2 - 4), (band - fox.height) // 2), fox)
        words, lines, cur = clean_text(text).split(), [], ""
        for w in words:
            test = f"{cur} {w}".strip()
            if d.textlength(test, font=self.f_act) <= self.w - 40:
                cur = test
            else:
                lines.append(cur)
                cur = w
        lines = (lines + [cur])[:4]
        y = band + (self.h - band) / 2 - len(lines) * 10 - 4
        d.text((self.w / 2, y), title, font=self.f_big, fill=TEXT, anchor="mm")
        for i, ln in enumerate(lines):
            d.text((self.w / 2, y + 30 + i * 20), ln, font=self.f_act, fill=MUTED, anchor="mm")
        return img

    def render(self, friends: list[Friend], page: int = 0, connected: bool = True, message: str = "",
               now: datetime | None = None) -> Image.Image:
        now = now or datetime.now()
        friends = self.sort(friends)
        img = Image.new("RGB", (self.w, self.h), BG)
        d = ImageDraw.Draw(img)
        online = sum(1 for f in friends if f.status != "offline")
        self._header(img, d, now, f"{online} online", STATUS_COLOR["online"])

        body_top = HEADER_H + 2
        if not connected:
            self._message(d, message or "Keine Verbindung zu Discord …")
        elif not friends:
            self._message(d, "Gerade ist niemand online")
        else:
            per = self.rows_per_page
            page %= self.pages(friends)
            for i, f in enumerate(friends[page * per:(page + 1) * per]):
                self._row(img, d, f, body_top + i * ROW_H)

        n = self.pages(friends)
        self._footer(d, f"Seite {page % n + 1}/{n}" if n > 1 else "")
        return img

    def _header(self, img: Image.Image, d: ImageDraw.ImageDraw, now: datetime, info: str, info_col) -> None:
        # Orange Kopfzeile mit Creme-Fuchs; Schrift in Navy (info_col bleibt aus Kompatibilität, Kontrast geht vor)
        d.rectangle((0, 0, self.w, HEADER_H - 1), fill=ORANGE)
        x = 8
        if self.icon:
            img.paste(self.icon, (x, (HEADER_H - self.icon.height) // 2), self.icon)
            x += self.icon.width + 6
        d.text((x, HEADER_H // 2), self.title, font=self.f_title, fill=brand.NAVY, anchor="lm")
        clock = now.strftime("%H:%M")
        d.text((self.w - 8, HEADER_H // 2), clock, font=self.f_title, fill=brand.NAVY, anchor="rm")
        cx = self.w - 8 - d.textlength(clock, font=self.f_title) - 12
        d.text((cx, HEADER_H // 2), info, font=self.f_small, fill=brand.NAVY, anchor="rm")

    def _footer(self, d: ImageDraw.ImageDraw, left: str) -> None:
        fy = self.h - FOOTER_H
        d.line((0, fy, self.w, fy), fill=LINE, width=1)
        if left:
            d.text((8, fy + FOOTER_H // 2), left, font=self.f_small, fill=MUTED, anchor="lm")
        d.text((self.w - 8, fy + FOOTER_H // 2), "FuchsLabs", font=self.f_small, fill=ACCENT, anchor="rm")

    BADGE = {"discord": ("D", (88, 101, 242)), "steam": ("S", (35, 98, 148)), "xbox": ("X", (16, 124, 16))}

    def _badges(self, d: ImageDraw.ImageDraw, platforms: tuple, right: int, cy: int) -> int:
        """Kleine Plattform-Marken (D/S/X) rechts oben in der Zeile; Rückgabe: belegte Breite."""
        x = right
        for p in reversed(platforms):
            letter, col = self.BADGE.get(p, ("?", MUTED))
            d.rounded_rectangle((x - 16, cy - 8, x, cy + 8), radius=4, fill=col)
            d.text((x - 8, cy), letter, font=self.f_small, fill=(255, 255, 255), anchor="mm")
            x -= 20
        return right - x + 8

    def _row(self, img: Image.Image, d: ImageDraw.ImageDraw, f: Friend, y: int) -> None:
        ax, ay = 10, y + (ROW_H - AVATAR) // 2
        av = self._avatar(f)
        if f.status == "offline":
            av = Image.blend(Image.new("RGBA", av.size, (*BG, 0)), av, 0.45)
        img.paste(av, (ax, ay), av)
        # Statuspunkt unten rechts am Avatar, wie in Discord
        r = 6
        cx, cy = ax + AVATAR - 4, ay + AVATAR - 4
        d.ellipse((cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2), fill=BG)
        col = STATUS_COLOR.get(f.status, STATUS_COLOR["offline"])
        if f.mobile and f.status != "offline":
            # Handy-Symbol statt Punkt
            d.rounded_rectangle((cx - 4, cy - r, cx + 4, cy + r), radius=2, fill=col)
            d.rectangle((cx - 2, cy - r + 2, cx + 2, cy + r - 4), fill=BG)
        else:
            d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=col)
            if f.status == "idle":
                d.ellipse((cx - r - 1, cy - r - 1, cx + 1, cy + 1), fill=BG)
            elif f.status == "dnd":
                d.rectangle((cx - 4, cy - 1, cx + 4, cy + 1), fill=BG)
            elif f.status == "offline":
                d.ellipse((cx - 3, cy - 3, cx + 3, cy + 3), fill=BG)

        tx = ax + AVATAR + 12
        max_w = self.w - tx - 10
        if len(f.platforms) > 1 or (f.platforms and f.platforms != ("discord",)):
            max_w -= self._badges(d, f.platforms, self.w - 10, y + 13)
        name_col = TEXT if f.status != "offline" else MUTED
        line2 = f.activity or STATUS_LABEL.get(f.status, "")
        if line2:
            d.text((tx, y + 13), _fit(d, f.name, self.f_name, max_w), font=self.f_name, fill=name_col, anchor="lm")
            act_col = ACCENT if f.activity.startswith(("Spielt", "Streamt")) else MUTED
            d.text((tx, y + 31), _fit(d, line2, self.f_act, max_w), font=self.f_act, fill=act_col, anchor="lm")
        else:
            d.text((tx, y + ROW_H // 2), _fit(d, f.name, self.f_name, max_w), font=self.f_name,
                   fill=name_col, anchor="lm")
        d.line((tx, y + ROW_H - 1, self.w - 10, y + ROW_H - 1), fill=LINE)


class Bf6Renderer(Renderer):
    """Rangliste der Battlefield-6-Profile (Daten aus bf6.py)."""

    def __init__(self, size: tuple[int, int], title: str = "Battlefield 6 · Rangliste"):
        super().__init__(size, title=title)
        self.f_stat = load_font(15, bold=True)
        self.f_pos = load_font(14, bold=True)
        self._icon_cache: dict[int, Image.Image] = {}

    def pages(self, players) -> int:
        return max(1, -(-len(players) // self.rows_per_page))

    def render(self, players, page: int = 0, updated: datetime | None = None,
               now: datetime | None = None) -> Image.Image:
        from bf6 import sort_players
        now = now or datetime.now()
        players = sort_players(players)
        img = Image.new("RGB", (self.w, self.h), BG)
        d = ImageDraw.Draw(img)
        found = sum(1 for p in players if not p.error)
        self._header(img, d, now, f"{found} Spieler", ORANGE)

        body_top = HEADER_H + 2
        if not players:
            d.text((self.w / 2, self.h / 2), "Keine Spieler in config.toml", font=self.f_big, fill=MUTED, anchor="mm")
        elif updated is None:
            d.text((self.w / 2, self.h / 2), "Lade Battlefield-Profile …", font=self.f_big, fill=MUTED, anchor="mm")
        else:
            per = self.rows_per_page
            page %= self.pages(players)
            for i, p in enumerate(players[page * per:(page + 1) * per]):
                self._bf_row(img, d, p, page * per + i + 1, body_top + i * ROW_H)

        n = self.pages(players)
        left = f"Stand {updated:%H:%M}" if updated else ""
        if n > 1:
            left = f"Seite {page % n + 1}/{n}" + (f" · {left}" if left else "")
        self._footer(d, left)
        return img

    def _rank_icon(self, icon: Image.Image | None) -> Image.Image | None:
        if icon is None:
            return None
        key = id(icon)
        if key not in self._icon_cache:
            ic = icon.copy()
            ic.thumbnail((AVATAR + 4, AVATAR + 4), Image.LANCZOS)
            self._icon_cache[key] = ic
        return self._icon_cache[key]

    def _bf_row(self, img: Image.Image, d: ImageDraw.ImageDraw, p, pos: int, y: int) -> None:
        cy = y + ROW_H // 2
        pos_col = ACCENT if pos <= 3 and not p.error else MUTED
        d.text((24, cy), f"{pos}.", font=self.f_pos, fill=pos_col, anchor="rm")
        ix = 30
        icon = self._rank_icon(p.rank_icon)
        if icon:
            img.paste(icon, (ix + (AVATAR + 4 - icon.width) // 2, cy - icon.height // 2), icon)
        elif p.rank is not None:
            d.ellipse((ix + 2, cy - 16, ix + 34, cy + 16), outline=ORANGE, width=2)
            d.text((ix + 18, cy), str(p.rank), font=self.f_small, fill=TEXT, anchor="mm")

        tx = ix + AVATAR + 12
        # Rechte Spalte: K/D groß, Siegquote klein
        stat_w = 92
        sx = self.w - 10
        if p.kd is not None:
            d.text((sx, y + 13), f"K/D {p.kd:.2f}", font=self.f_stat, fill=TEXT, anchor="rm")
        if p.win_pct is not None:
            d.text((sx, y + 31), f"{p.win_pct:.0f} % Siege", font=self.f_act, fill=MUTED, anchor="rm")
        max_w = sx - stat_w - tx

        name_col = MUTED if p.error else TEXT
        d.text((tx, y + 13), _fit(d, p.name, self.f_name, max_w), font=self.f_name, fill=name_col, anchor="lm")
        if p.error:
            line2, col = p.error, STATUS_COLOR["dnd"]
        else:
            line2 = f"Rang {p.rank}" if p.rank is not None else "Rang unbekannt"
            if p.rank_name:
                line2 += f" · {p.rank_name}"
            col = ACCENT
        d.text((tx, y + 31), _fit(d, line2, self.f_act, max_w), font=self.f_act, fill=col, anchor="lm")
        d.line((tx, y + ROW_H - 1, self.w - 10, y + ROW_H - 1), fill=LINE)


WARN = (190, 120, 0)
HOT = (215, 45, 50)
DOWN = (22, 140, 72)


def _num(v: float | None, fmt: str = "{:.0f}", unit: str = "") -> str:
    if v is None:
        return "–"
    return fmt.format(v).replace(".", ",") + (f" {unit}" if unit else "")


def fmt_rate(bytes_per_s: float | None, unit: str = "bit") -> str:
    if bytes_per_s is None:
        return "–"
    if unit == "byte":
        steps, v = (("B/s", 1), ("KB/s", 1e3), ("MB/s", 1e6), ("GB/s", 1e9)), bytes_per_s
    else:
        steps, v = (("bit/s", 1), ("kbit/s", 1e3), ("Mbit/s", 1e6), ("Gbit/s", 1e9)), bytes_per_s * 8
    name, div = steps[0]
    for name, div in steps:
        if v < div * 1000:
            break
    x = v / div
    return f"{x:.0f} {name}" if x >= 100 or div == 1 else f"{x:.1f} {name}".replace(".", ",")


class SysRenderer(Renderer):
    """Systemansicht: CPU, GPU, RAM, Datenträger, Netzwerk (Daten aus sysinfo.py)."""

    def __init__(self, size: tuple[int, int], title: str = "System", net_unit: str = "bit"):
        super().__init__(size, title=title)
        self.net_unit = net_unit
        self.f_card = load_font(14, bold=True)
        self.f_huge = load_font(30, bold=True)
        self.f_val = load_font(15, bold=True)
        self.f_lbl = load_font(12)

    def pages(self, _=None) -> int:
        return 1

    def render(self, s, now: datetime | None = None) -> Image.Image:
        now = now or datetime.now()
        img = Image.new("RGB", (self.w, self.h), BG)
        d = ImageDraw.Draw(img)
        self._header(img, d, now, "" if s.hwinfo else "ohne HWiNFO", MUTED)
        top, bot, gap = HEADER_H + 4, self.h - FOOTER_H - 4, 6
        lw = (self.w - 3 * gap) // 2
        lx, rx = gap, gap * 2 + lw
        rw = self.w - rx - gap
        half = (bot - top - gap) // 2
        self._usage_card(d, lx, top, lw, half, "CPU", s.cpu_name, s.cpu_pct, [
            ("Temp", _num(s.cpu_temp, unit="°C"), self._temp_col(s.cpu_temp)),
            ("Takt", _num(s.cpu_ghz, "{:.2f}", "GHz"), TEXT),
            ("Lüfter", _num(s.cpu_fan, unit="RPM"), TEXT)])
        vram = (f"{_num(s.gpu_mem_used / 1024, '{:.1f}')}/{_num(s.gpu_mem_total / 1024, '{:.0f}')} GB"
                if s.gpu_mem_used is not None and s.gpu_mem_total else "–")
        self._usage_card(d, lx, top + half + gap, lw, bot - top - half - gap, "GPU", s.gpu_name, s.gpu_pct, [
            ("Temp", _num(s.gpu_temp, unit="°C"), self._temp_col(s.gpu_temp)),
            ("Lüfter", _num(s.gpu_fan, unit="%"), TEXT),
            ("VRAM", vram, TEXT)])

        h_small = 62
        ram = (f"{_num(s.ram_used, '{:.1f}')} / {_num(s.ram_total, '{:.0f}')} GB" if s.ram_used is not None else "–")
        self._bar_card(d, rx, top, rw, h_small, "RAM", ram, s.ram_pct)
        disk = f"{s.disk_label} " + (_num(s.disk_temp, unit="°C") if s.disk_temp is not None else "belegt")
        self._bar_card(d, rx, top + h_small + gap, rw, h_small, "Datenträger", disk, s.disk_pct)
        ny = top + 2 * (h_small + gap)
        self._net_card(d, rx, ny, rw, bot - ny, s)
        self._footer(d, "")
        return img

    @staticmethod
    def _temp_col(t: float | None):
        return HOT if t is not None and t >= 85 else WARN if t is not None and t >= 75 else TEXT

    def _card(self, d, x, y, w, h, title: str, sub: str = "") -> None:
        d.rounded_rectangle((x, y, x + w, y + h), radius=8, fill=CARD)
        d.text((x + 10, y + 13), title, font=self.f_card, fill=ACCENT, anchor="lm")
        if sub:
            tx = x + 16 + d.textlength(title, font=self.f_card)
            d.text((tx, y + 13), _fit(d, sub, self.f_lbl, x + w - 10 - tx), font=self.f_lbl, fill=MUTED, anchor="lm")

    def _bar(self, d, x, y, w, pct: float | None, h: int = 6) -> None:
        d.rounded_rectangle((x, y, x + w, y + h), radius=h // 2, fill=BG)
        if pct:
            col = HOT if pct >= 90 else ORANGE
            d.rounded_rectangle((x, y, x + max(h, int(w * min(pct, 100) / 100)), y + h), radius=h // 2, fill=col)

    def _usage_card(self, d, x, y, w, h, title, sub, pct, rows) -> None:
        self._card(d, x, y, w, h, title, sub)
        d.text((x + 10, y + 50), _num(pct, unit="%"), font=self.f_huge, fill=TEXT, anchor="lm")
        self._bar(d, x + 10, y + 74, 84, pct)
        cx, step = x + 104, (h - 30) // len(rows)
        for i, (lbl, val, col) in enumerate(rows):
            ry = y + 30 + step * i + step // 2
            d.text((cx, ry), lbl, font=self.f_lbl, fill=MUTED, anchor="lm")
            d.text((x + w - 10, ry), val, font=self.f_val, fill=col, anchor="rm")

    def _bar_card(self, d, x, y, w, h, title, detail, pct) -> None:
        self._card(d, x, y, w, h, title)
        d.text((x + w - 10, y + 13), _num(pct, unit="%"), font=self.f_val, fill=TEXT, anchor="rm")
        d.text((x + 10, y + 36), detail, font=self.f_lbl, fill=MUTED, anchor="lm")
        self._bar(d, x + 10, y + h - 16, w - 20, pct)

    @staticmethod
    def _arrow(d, cx, cy, down: bool, col) -> None:
        s = 1 if down else -1  # Schaft zeigt von der Spitze weg, Spitze in Pfeilrichtung
        d.line((cx, cy - 6 * s, cx, cy), fill=col, width=3)
        d.polygon([(cx - 6, cy), (cx + 6, cy), (cx, cy + 7 * s)], fill=col)

    def _net_card(self, d, x, y, w, h, s) -> None:
        self._card(d, x, y, w, h, "Netzwerk", s.net_name)
        # Pfeile als Formen: die CI-Schrift hat keine Pfeilzeichen
        self._arrow(d, x + 16, y + 36, down=True, col=DOWN)
        d.text((x + w - 10, y + 36), fmt_rate(s.net_down, self.net_unit), font=self.f_val, fill=TEXT, anchor="rm")
        self._arrow(d, x + 16, y + 56, down=False, col=ORANGE)
        d.text((x + w - 10, y + 56), fmt_rate(s.net_up, self.net_unit), font=self.f_val, fill=TEXT, anchor="rm")
        # Verlauf der letzten 60 s (Download grün, Upload orange)
        gx0, gx1, gy0, gy1 = x + 10, x + w - 10, y + 70, y + h - 8
        if gy1 - gy0 < 10:
            return
        hist_d, hist_u = list(s.down_hist), list(s.up_hist)
        peak = max(hist_d + hist_u + [1.0])
        for hist, col in ((hist_d, DOWN), (hist_u, ORANGE)):
            if len(hist) < 2:
                continue
            n = hist.__len__()
            pts = [(gx1 - (n - 1 - i) * (gx1 - gx0) / 59, gy1 - v / peak * (gy1 - gy0)) for i, v in enumerate(hist)]
            d.line(pts, fill=col, width=2)


class RssRenderer(Renderer):
    """Blog-Updates: neueste Beiträge aus den RSS-Feeds (Daten aus rss.py)."""

    MAX_ROWS = 4

    def __init__(self, size: tuple[int, int], title: str = "Blog-Updates"):
        super().__init__(size, title=title)
        self.f_post = load_font(17, bold=True)
        self.f_src = load_font(13, bold=True)

    def pages(self, posts) -> int:
        return 1  # bewusst nur eine Seite: wenige Beiträge, große Schrift

    @staticmethod
    def _short(title: str) -> str:
        # Werbe-/Rubrik-Präfixe wie "heise+ |" oder "Anzeige:" weglassen
        return re.sub(r"^\s*(\S{1,12}\s*\|\s*|Anzeige:\s*|Deal:\s*)", "", title)

    def _wrap(self, d, text: str, font, max_w: int, lines: int = 2) -> list[str]:
        words, out, cur = clean_text(text).split(), [], ""
        for i, w in enumerate(words):
            test = f"{cur} {w}".strip()
            if d.textlength(test, font=font) <= max_w:
                cur = test
                continue
            out.append(cur)
            cur = w
            if len(out) == lines - 1:
                return out + [_fit(d, " ".join([cur] + words[i + 1:]), font, max_w)]
        return out + [cur] if cur else out

    def render(self, posts, page: int = 0, updated: datetime | None = None,
               now: datetime | None = None) -> Image.Image:
        from rss import ago
        now = now or datetime.now()
        img = Image.new("RGB", (self.w, self.h), BG)
        d = ImageDraw.Draw(img)
        self._header(img, d, now, "", ORANGE)
        top, bot = HEADER_H + 2, self.h - FOOTER_H
        if updated is None:
            d.text((self.w / 2, self.h / 2), "Lade Feeds …", font=self.f_big, fill=MUTED, anchor="mm")
        elif not posts:
            d.text((self.w / 2, self.h / 2), "Keine Beiträge gefunden", font=self.f_big, fill=MUTED, anchor="mm")
        else:
            rows = posts[:self.MAX_ROWS]
            rh = (bot - top) // self.MAX_ROWS
            for i, p in enumerate(rows):
                y = top + i * rh
                age = ago(p.published)
                d.text((self.w - 10, y + 12), age, font=self.f_act, fill=MUTED, anchor="rm")
                src_w = self.w - 30 - d.textlength(age, font=self.f_act)
                d.text((10, y + 12), _fit(d, p.source, self.f_src, src_w), font=self.f_src, fill=ACCENT, anchor="lm")
                for j, line in enumerate(self._wrap(d, self._short(p.title), self.f_post, self.w - 20)):
                    d.text((10, y + 32 + j * 20), line, font=self.f_post, fill=TEXT, anchor="lm")
                if i < len(rows) - 1:
                    d.line((10, y + rh - 1, self.w - 10, y + rh - 1), fill=LINE)
        self._footer(d, f"Stand {updated:%H:%M}" if updated else "")
        return img


def demo_friends() -> list[Friend]:
    return [
        Friend(1, "Fuchs", "online", "Spielt Battlefield 6 · 42 min"),
        Friend(2, "Basti", "dnd", "Streamt Battlefield 6"),
        Friend(3, "Lena", "idle", "🌙 gleich zurück"),
        Friend(4, "Kevin", "online", "Hört Numb – Linkin Park", mobile=True),
        Friend(5, "Mara", "online", ""),
        Friend(6, "Tom", "offline"),
        Friend(7, "Jonas", "online", "Spielt Counter-Strike 2 · 1 h 05 min"),
        Friend(8, "Sven", "idle", ""),
    ]


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "preview.png"
    r = Renderer((480, 320))
    fr = demo_friends()
    for p in range(r.pages(fr)):
        name = out if p == 0 else out.replace(".png", f"-{p + 1}.png")
        r.render(fr, page=p, now=datetime(2026, 10, 21, 20, 15)).save(name)
        print("geschrieben:", name)
