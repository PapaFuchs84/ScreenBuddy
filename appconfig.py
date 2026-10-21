# © 2026 FuchsLabs
"""Konfiguration: Standardwerte, Laden, Prüfen (Eingaben der Einstellungsoberfläche) und Schreiben."""
from __future__ import annotations

import os
import re
import tomllib
from pathlib import Path

import secure

PAGES = ["system", "friends", "discord", "bf6", "rss"]

DEFAULTS: dict[str, dict] = {
    "discord": {"enabled": True, "token": "", "guild_ids": [], "friend_ids": [], "ignore_ids": [],
                "show_offline": False},
    "display": {"port": "AUTO", "orientation": "landscape", "brightness": 50, "reset_on_start": True,
                "page_seconds": 15, "pages": list(PAGES), "title": "Discord-Freunde",
                "screen_off_on_exit": True, "theme": "hell",
                # Ausgabe: serial = USB-Display (3,5"), monitor = HDMI/USB-C-Bildschirm als Vollbildfenster
                "output": "serial", "monitor": "", "touch": False,
                # Eigene Farben (theme = "eigen"): Vorlage plus #RRGGBB-Werte; leer = Farbe der Vorlage
                "color_base": "hell", "color_bg": "", "color_card": "", "color_header": "", "color_accent": "",
                "color_text": "", "color_muted": ""},
    "system": {"enabled": True, "title": "System", "net_adapter": "", "net_unit": "bit", "disk": "C:",
               "cpu_temp_sensor": "", "cpu_fan_sensor": ""},
    "friends": {"enabled": False, "title": "Freunde", "include_discord": True, "auto_merge": True,
                "show_offline": False, "steam_api_key": "", "steam_id": "", "xbox_api_key": "", "links": []},
    "bf6": {"enabled": False, "players": [], "refresh_minutes": 15, "title": "Battlefield 6 · Rangliste"},
    "rss": {"enabled": False, "feeds": [], "refresh_minutes": 30, "max_items": 4, "max_per_feed": 1,
            "title": "Blog-Updates"},
    "ui": {"enabled": True, "port": 8765, "setup_done": False, "language": "de"},   # language: de | en (i18n.py)
}

# (min, max) für Zahlenwerte
LIMITS = {
    ("display", "brightness"): (0, 100), ("display", "page_seconds"): (3, 600),
    ("bf6", "refresh_minutes"): (5, 240),
    ("rss", "refresh_minutes"): (10, 720), ("rss", "max_items"): (1, 4), ("rss", "max_per_feed"): (1, 4),
    ("ui", "port"): (1024, 65535),
}
CHOICES = {
    ("display", "orientation"): ("landscape", "reverse_landscape", "portrait", "reverse_portrait"),
    ("system", "net_unit"): ("bit", "byte"),
    ("display", "theme"): ("hell", "dunkel", "eigen"),
    ("display", "color_base"): ("hell", "dunkel"),
    ("display", "output"): ("serial", "monitor"),
    ("ui", "language"): ("de", "en"),
}
COLORS = {("display", f"color_{k}") for k in ("bg", "card", "header", "accent", "text", "muted")}
ID_LISTS = {("discord", "guild_ids"), ("discord", "friend_ids"), ("discord", "ignore_ids")}
SECRETS = {("discord", "token"), ("friends", "steam_api_key"), ("friends", "xbox_api_key")}


def load_config(path: Path) -> dict:
    cfg = {k: {kk: (list(vv) if isinstance(vv, list) else vv) for kk, vv in v.items()} for k, v in DEFAULTS.items()}
    if path.exists():
        with path.open("rb") as fh:
            user = tomllib.load(fh)
        for sec, vals in user.items():
            cfg.setdefault(sec, {}).update(vals)
    for sec, key in SECRETS:
        cfg[sec][key] = secure.unprotect(str(cfg[sec].get(key, "")))
    # Seiten, die eine ältere Konfiguration noch nicht kennt (z. B. "friends"), hinten anhängen
    known = [p for p in cfg["display"].get("pages", []) if p in PAGES]
    cfg["display"]["pages"] = list(dict.fromkeys(known)) + [p for p in PAGES if p not in known]
    return cfg


def token_is_plaintext(path: Path) -> bool:
    """Alte Konfigurationen enthalten den Token im Klartext; sie werden beim Start verschlüsselt."""
    if not path.exists():
        return False
    with path.open("rb") as fh:
        data = tomllib.load(fh)
    return any(v and not secure.is_protected(v)
               for v in (str(data.get(sec, {}).get(key, "")) for sec, key in SECRETS))


def _coerce(sec: str, key: str, value, default):
    """Wert aus der Oberfläche auf den Typ des Standardwerts bringen; ungültig → ValueError."""
    if isinstance(default, bool):
        if isinstance(value, bool):
            return value
        raise ValueError("Ja/Nein erwartet")
    if isinstance(default, int):
        v = int(float(value))
    elif isinstance(default, float):
        v = float(value)
    elif isinstance(default, list):
        if not isinstance(value, list):
            raise ValueError("Liste erwartet")
        if (sec, key) in ID_LISTS:
            return sorted({int(str(x).strip()) for x in value if str(x).strip()})
        if (sec, key) == ("display", "pages"):
            out = [p for p in value if p in PAGES]
            return list(dict.fromkeys(out)) + [p for p in PAGES if p not in out]
        out = []
        for x in value:
            s = str(x).strip()[:200]
            if s and s not in out:
                out.append(s)
        return out[:50]
    else:
        v = str(value).strip()[:300]
        if (sec, key) in CHOICES and v not in CHOICES[(sec, key)]:
            raise ValueError(f"erlaubt: {', '.join(CHOICES[(sec, key)])}")
        if (sec, key) in COLORS and v:
            if not re.fullmatch(r"#[0-9a-fA-F]{6}", v):
                raise ValueError("Farbe als #RRGGBB erwartet")
            v = v.upper()
        return v
    if (sec, key) in LIMITS:
        lo, hi = LIMITS[(sec, key)]
        v = min(hi, max(lo, v))
    return v


def merge(cfg: dict, incoming: dict) -> dict:
    """Neue Konfiguration aus cfg und den Eingaben der Oberfläche. Leere Geheimnisse bleiben unverändert."""
    out = {k: dict(v) for k, v in cfg.items()}
    for sec, vals in incoming.items():
        if sec not in DEFAULTS or not isinstance(vals, dict):
            continue
        for key, value in vals.items():
            if key not in DEFAULTS[sec]:
                continue
            if (sec, key) in SECRETS and not str(value).strip():
                continue
            try:
                out[sec][key] = _coerce(sec, key, value, DEFAULTS[sec][key])
            except (ValueError, TypeError) as e:
                raise ValueError(f"{sec}.{key}: {e}") from e
    return out


def public(cfg: dict) -> dict:
    """Konfiguration für die Oberfläche – ohne Geheimnisse (Token wird nie ausgeliefert)."""
    out = {k: dict(v) for k, v in cfg.items() if k in DEFAULTS}
    for sec, key in SECRETS:
        out[sec][key] = ""
        out[sec][key + "_set"] = bool(str(cfg[sec].get(key, "")).strip())
    for sec, key in ID_LISTS:
        out[sec][key] = [str(x) for x in out[sec][key]]  # IDs als Text: JavaScript-Zahlen sind für 18 Stellen zu ungenau

    return out


def _toml_value(v) -> str:
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int, float)):
        return repr(v)
    if isinstance(v, list):
        return "[" + ", ".join(_toml_value(x) for x in v) + "]"
    s = str(v).replace("\\", "\\\\").replace('"', '\\"').replace("\n", " ")
    return f'"{s}"'


HEADERS = {
    "discord": "Discord-Freunde (Bot-Token, Server, Personen)",
    "display": "Display und Seitenwechsel (pages = Reihenfolge der Seiten)",
    "system": "Systemansicht",
    "friends": "Gemeinsame Freundesliste (Discord, Steam, Xbox); links = manuell verknüpfte Personen",
    "bf6": "Battlefield-6-Rangliste (EA-IDs, Konsole mit Suffix @psn / @xbox)",
    "rss": "Blog-Updates (RSS)",
    "ui": "Einstellungsoberfläche (läuft nur lokal auf diesem PC)",
}


def save_config(path: Path, cfg: dict) -> None:
    lines = ["# FuchsLabs USB-Display – wird von der Einstellungsoberfläche geschrieben.",
             "# Von Hand ändern ist möglich; die App danach neu starten.", ""]
    for sec in DEFAULTS:
        lines += [f"# {HEADERS[sec]}", f"[{sec}]"]
        for key in DEFAULTS[sec]:
            val = cfg.get(sec, {}).get(key, DEFAULTS[sec][key])
            if (sec, key) in SECRETS:
                val = secure.protect(str(val))
            lines.append(f"{key} = {_toml_value(val)}")
        lines.append("")
    tmp = path.with_suffix(".toml.tmp")
    tmp.write_text("\n".join(lines), encoding="utf-8")
    if path.exists():
        try:
            plain = token_is_plaintext(path)
        except (OSError, tomllib.TOMLDecodeError):
            plain = False
        # Eine Sicherung mit Klartext-Token wäre ein Leck: dann die neue (verschlüsselte) Fassung sichern
        path.with_suffix(".toml.bak").write_bytes(tmp.read_bytes() if plain else path.read_bytes())
    os.replace(tmp, path)
