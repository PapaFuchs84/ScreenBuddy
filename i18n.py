# © 2026 FuchsLabs
"""Sprache der App: Deutsch (Quelltexte) oder Englisch.

Alle Texte im Code sind deutsch. tr() übersetzt sie beim Zeichnen bzw. vor der Ausgabe an die
Einstellungsseite, solange Englisch eingestellt ist: erst als ganzer Satz, sonst über Muster
(z. B. „Spielt X“ → „Playing X“). Unbekannte Texte (Namen, Blog-Titel) bleiben unverändert.
"""
from __future__ import annotations

import re

LANGS = ("de", "en")
_lang = "de"


def set_lang(lang: str) -> None:
    global _lang
    _lang = lang if lang in LANGS else "de"


def lang() -> str:
    return _lang


EN = {
    # Display: Titel, Kopf- und Fußzeilen, Hinweise
    "Discord-Freunde": "Discord friends",
    "Freunde": "Friends",
    "Battlefield 6 · Rangliste": "Battlefield 6 · Leaderboard",
    "Blog-Updates": "Blog updates",
    "Datenträger": "Drive",
    "Netzwerk": "Network",
    "Takt": "Clock",
    "Lüfter": "Fan",
    "belegt": "used",
    "ohne HWiNFO": "without HWiNFO",
    "Online": "Online",
    "Abwesend": "Away",
    "Bitte nicht stören": "Do not disturb",
    "Offline": "Offline",
    "Gerade ist niemand online": "Nobody is online right now",
    "Keine Verbindung zu Discord …": "No connection to Discord …",
    "Keine Spieler in config.toml": "No players set up",
    "Lade Battlefield-Profile …": "Loading Battlefield profiles …",
    "Rang unbekannt": "Rank unknown",
    "nicht gefunden": "not found",
    "Lade Feeds …": "Loading feeds …",
    "Keine Beiträge gefunden": "No posts found",
    "gerade eben": "just now",
    "Willkommen!": "Welcome!",
    # Meldungen (Display und Einstellungsseite)
    "Noch keine Konten verbunden – bitte in den Einstellungen einrichten":
        "No accounts connected yet – please set them up in the settings",
    "Verbindung zum Display verloren – wird erneut versucht": "Lost connection to the display – retrying",
    "Die Hersteller-App des Displays läuft und belegt den Anschluss":
        "The display's vendor app is running and blocking the port",
    "Display nicht gefunden – USB-Kabel und Anschluss prüfen": "Display not found – check the USB cable and port",
    "Display nicht erreichbar": "Display not reachable",
    "Bildschirm nicht gefunden": "Screen not found",
    "Kein zweiter Bildschirm angeschlossen": "No second screen connected",
    "Anzeigefenster konnte nicht geöffnet werden": "Could not open the display window",
    "Anzeigefenster ist geschlossen": "The display window is closed",
    "Discord verweigert den Zugriff: im Developer Portal beide „Intents“ einschalten":
        "Discord denied access: turn on both “Intents” in the Developer Portal",
    "Bot-Token ungültig – bitte in den Einstellungen neu eintragen": "Invalid bot token – please enter it again in the settings",
    "Keine Verbindung zu Discord": "No connection to Discord",
    "Steam: Schlüssel ungültig oder die Freundesliste im Profil ist nicht öffentlich":
        "Steam: invalid key, or the friends list in your profile is not public",
    "Steam: zu viele Abrufe, ich versuche es später wieder": "Steam: too many requests, trying again later",
    "Steam ist nicht erreichbar": "Steam is not reachable",
    "Steam: Profil nicht gefunden – bitte den Link aus der Adresszeile deines Profils einfügen":
        "Steam: profile not found – please paste the link from your profile's address bar",
    "Xbox: OpenXBL-Schlüssel ungültig": "Xbox: invalid OpenXBL key",
    "Xbox: Abruflimit von OpenXBL erreicht, ich versuche es später wieder": "Xbox: OpenXBL rate limit reached, trying again later",
    "OpenXBL ist nicht erreichbar": "OpenXBL is not reachable",
    "Name fehlt": "Name missing",
    "Spieler nicht gefunden": "Player not found",
    "Adresse fehlt": "Address missing",
    "Kein Feed gefunden": "No feed found",
    "Ungültige oder unsichere Adresse": "Invalid or unsafe address",
    "Bitte zuerst den Schlüssel eintragen": "Please enter the key first",
    "Bitte den Link zu deinem Steam-Profil eintragen": "Please enter the link to your Steam profile",
    "Konnte die Hersteller-App nicht beenden": "Could not close the vendor app",
    "Die Windows-Abfrage wurde abgelehnt": "The Windows prompt was declined",
    "Ja/Nein erwartet": "yes/no expected",
    "Liste erwartet": "list expected",
    "Farbe als #RRGGBB erwartet": "colour expected as #RRGGBB",
    # Beispieldaten (--demo, Vorschaubilder)
    "gleich zurück": "back soon",
    "Tippfehler": "Typo",
    "Beispiel-Blog": "Example blog",
    "Noch ein Blog": "Another blog",
    "Ein kurzer Titel für die Vorschau": "A short title for the preview",
    "Zweiter Beitrag mit etwas längerem Titel, der umbricht": "Second post with a slightly longer title that wraps",
    # Infobereich
    "Einstellungen öffnen": "Open settings",
    "Neu starten": "Restart",
    "Beenden": "Quit",
}

# Muster für zusammengesetzte Texte; Reihenfolge zählt
_PATTERNS = [(re.compile(p), r) for p, r in (
    (r"^Spielt (.+)$", r"Playing \1"),
    (r"^Streamt (.+)$", r"Streaming \1"),
    (r"^Hört (.+)$", r"Listening to \1"),
    (r"^Schaut (.+)$", r"Watching \1"),
    (r"^Seite (\d+)/(\d+)(.*)$", r"Page \1/\2\3"),
    (r"^Stand (\d+:\d+)$", r"Updated \1"),
    (r"^Rang (\S+)(.*)$", r"Rank \1\2"),
    (r"^(\d+) % Siege$", r"\1 % wins"),
    (r"^(\d+) Spieler$", r"\1 players"),
    (r"^vor (\d+) min$", r"\1 min ago"),
    (r"^vor (\d+) h$", r"\1 h ago"),
    (r"^vor (\d+) Tagen$", r"\1 days ago"),
    (r"^(\d+) Server$", r"\1 servers"),
    (r"^(\d+) Freunde, (\d+) online$", r"\1 friends, \2 online"),
    (r"^Steam antwortet mit Fehler (\d+)$", r"Steam replied with error \1"),
    (r"^OpenXBL antwortet mit Fehler (\d+)$", r"OpenXBL replied with error \1"),
    (r"^erlaubt: (.+)$", r"allowed: \1"),
    (r"^(.+) – Bildschirm in den Einstellungen prüfen$", r"\1 – check the screen in the settings"),
    (r"^Öffne die Einstellungen über das Fuchs-Symbol im Infobereich oder im Browser: (.+)$",
     r"Open the settings via the fox icon in the notification area or in your browser: \1"),
)]


def tr(text):
    """Text in der eingestellten Sprache; Nicht-Texte und Unbekanntes unverändert."""
    if _lang == "de" or not isinstance(text, str) or not text:
        return text
    hit = EN.get(text)
    if hit is not None:
        return hit
    if " · " in text:   # z. B. „Spielt X · 42 min“, „Fehler · Fehler“
        parts = text.split(" · ")
        out = [tr(p) for p in parts]
        if out != parts:
            return " · ".join(out)
    for rx, rep in _PATTERNS:
        if rx.match(text):
            return rx.sub(rep, text)
    if ": " in text:    # „display.port: erlaubt: …“
        head, rest = text.split(": ", 1)
        if "." in head and " " not in head:
            return f"{head}: {tr(rest)}"
    return text
