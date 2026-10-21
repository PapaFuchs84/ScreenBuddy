# Changelog – FuchsLabs USB-Display

Alle nennenswerten Änderungen an der App. Format angelehnt an [Keep a Changelog](https://keepachangelog.com/de/1.1.0/), Versionen nach [SemVer](https://semver.org/lang/de/).

## [Unveröffentlicht]

### Hinzugefügt
- Kaffee-Knopf (PayPal) in der Einstellungsseite links unten, im README und als Sponsor-Knopf auf GitHub (`.github/FUNDING.yml`).

### Dokumentation
- README nach üblichem Aufbau: Badges, Funktionen, Bilder mit Beispieldaten (`docs/screenshots/`), Schnellstart, Zugänge, Konfiguration, Hardware, Bauen, Projektaufbau, Sicherheit, Lizenzhinweis.
- Neu: `CONTRIBUTING.md` (Ablauf, Code-Regeln, Umgang mit Geheimnissen), `SECURITY.md` (Lücken melden, Sicherheitsmodell) und eine Vorlage für Pull Requests.

## [1.2] – 2026-10-21

### Hinzugefügt
- Git-Repository mit README, CHANGELOG, Projektprotokoll (`PROTOKOLL.md`) und `.gitignore`.
- Vorlagen für Fehlerberichte und Rückmeldungen in der Testphase (`.github/ISSUE_TEMPLATE/`), Hinweise dazu in `TESTPHASE.md`.
- **Eigene Farben** (`display.theme = "eigen"`): Hintergrund, Karten, Kopfzeile & Balken, Akzentschrift, Schrift und Nebentext per Farbwähler über der Vorlage Hell oder Dunkel (`display.color_*`, leer = Vorlage). Live-Vorschau der aktuellen Seite (`POST /api/preview-theme.png`), „Zurücksetzen“ und Warnung bei zu wenig Kontrast (WCAG). Die Kopfzeilenschrift wählt Navy oder Weiß selbst.
- **Beliebige Auflösungen**: Alle Seiten zeichnen in logischen 480 × 320-Koordinaten und werden scharf auf die echte Größe gerendert (`render.ScaledDraw`). Beim 3,5"-Display bleibt das Bild pixelgleich. Breite Leisten zeigen mehrere Seiten nebeneinander (1920 × 720: zwei, 1920 × 480: drei).
- **Bildschirm über HDMI/USB-C** (`display.output = "monitor"`, `monitor_output.py`): randloses Vollbildfenster auf dem gewählten Bildschirm (`display.monitor`, leer = erster Nicht-Hauptbildschirm), ohne Taskleisteneintrag und ohne Fokus zu stehlen, Mauszeiger ausgeblendet. Die App ist jetzt DPI-bewusst (echte Pixelgrößen).
- **Touch-Seitenwechsel** (`display.touch`, nur bei Bildschirmausgabe und wenn Windows einen Touchscreen meldet): nach links wischen = nächste Seite, nach rechts = vorige, Tippen ins linke Drittel = zurück, sonst weiter.

## [1.1] – 2026-10-21

### Hinzugefügt
- **Einstellungsseite** im Browser unter `http://127.0.0.1:8765/`, geöffnet über das neue Symbol im Infobereich (Doppelklick) oder einen zweiten Programmstart. Bereiche: Start, Übersicht (Live-Vorschau, Seiten an/aus und Reihenfolge, Anzeigedauer, Helligkeit), Freunde, Discord, Battlefield 6, Blogs, System, Display.
- **Ersteinrichtung** für Freunde ohne Vorkenntnisse: Zustand von Display und Discord, Knöpfe zum Beenden und Abschalten der Hersteller-App, Autostart über `HKCU\…\Run` ohne Administratorrechte.
- **Gemeinsame Freundesliste** (`friends.py`): Discord, Steam (offizielle Web-API) und Xbox (OpenXBL) in einer Liste, dieselbe Person nur einmal (automatisch bei gleichem Namen oder per „Personen zusammenführen“, mit Vorschlägen bei ähnlichen Namen), Marken D/S/X je Plattform. PlayStation und EA bewusst nicht eingebaut (keine offizielle Schnittstelle, Sperrrisiko). Tests in `test_friends.py`.
- **Verbindungsstatus** in den Reitern Discord und Freunde: grün mit Kontoname, Anzahl und letzter Abfrage, rot mit Fehlermeldung, grau wenn nicht eingerichtet.
- **Hilfen zum Aufklappen** neben jedem Token- und Schlüsselfeld (Discord, Steam, Profil-Link, Xbox, EA-ID, Blogs) mit Knöpfen, die direkt die richtige Seite öffnen.
- **Bebilderte, interaktive Anleitung** `ANLEITUNG.html` (kindgerecht, „Schritt X von N“, Auswahl der gewünschten Seiten am Anfang, „Klappt nicht?“-Hilfe je Schritt), erreichbar unter `/anleitung` und aus der Einstellungsseite. Quellen in `anleitung/`.
- Screenshots in der Anleitung mit Beispielnamen erneuert; private Xbox-Namen werden in der Aufnahme anonymisiert.
- **FuchsLabs-CI** (`brand.py`): Fuchs-Orange `#F77F00` und Creme `#FFF4E6` als Hauptfarben, Navy `#14213D` für Schrift, geometrisches Fuchs-Logo für Display, Infobereich und EXE-Symbol. Schriften Bahnschrift (Titel, Zahlen) und Segoe UI (Text), keine Web-Fonts.
- **Hell/Dunkel-Umschalter** (`display.theme`), gilt für Display, Einstellungsseite und Anleitung. Hell ist voreingestellt.
- Willkommensbild auf dem Display, solange keine Seite eingerichtet ist.

### Geändert
- Neuer Name **FuchsLabs USB-Display**, alle Hinweise auf den Display-Hersteller entfernt (Code, Texte, EXE).
- Als eigenständige EXE gebaut (PyInstaller, `build.bat`), Python wird beim Anwender nicht mehr gebraucht. Programmgröße von 66 MB auf rund 36 MB gesenkt.
- Discord wird nur geladen, wenn eine Discord-Seite aktiv ist.
- Die reine Seite „Discord-Freunde“ entfällt automatisch, solange die gemeinsame Liste Discord einbezieht.

### Sicherheit
- Discord-Token, Steam- und Xbox-Schlüssel werden mit Windows-DPAPI verschlüsselt gespeichert (`secure.py`) und nie an den Browser oder ins Log gegeben. Alte Klartext-Tokens werden beim Start verschlüsselt.
- `save_config` legt keine Klartext-Sicherung mehr an (Token stand vorher in `.bak`-Dateien).
- Pillow von 11.3.0 (45 bekannte Lücken) auf 13.0.0 angehoben; fremde Bilder nur als PNG/JPEG/WebP/GIF und höchstens 2 MB.
- Einstellungsseite: nur `127.0.0.1`, Host-Prüfung gegen DNS-Rebinding, eigener Header für Änderungen (CSRF), Ablehnung von `Sec-Fetch-Site: cross-site/same-site`, `X-Frame-Options: DENY`, `frame-ancestors 'none'`, `nosniff`.
- Feeds mit `<!ENTITY` werden verworfen, nur http/https, Antworten auf 3 MB begrenzt. AVIF-Modul aus dem Build ausgeschlossen.
- Details in `SICHERHEITSREVIEW.md`.

### Behoben
- Ungültige JSON-Eingaben an `/api/config` führen zu 400 statt 500.
- Im Reiter „Freunde“ stand „null“.

## [1.0] – 2026-10-21

Erste Fassung als Python-Skript, danach als EXE `FuchsLabs-USB-Display-1.0.zip`.

### Hinzugefügt
- Eigener Treiber für das 3,5"-USB-Display (480 × 320, serielles Protokoll über USB-CDC, RGB565, Teil-Updates in Zeilenbändern), ersetzt die Hersteller-App.
- Seite **Discord-Freunde** über einen eigenen Bot (Presence und Server Members Intent), Status, Aktivität, Avatare.
- Seite **Battlefield 6 · Rangliste** über `api.gametools.network`: Rang mit Abzeichen, K/D, Siegquote, Spieler in der Konfiguration.
- Seite **System**: CPU, GPU (`nvidia-smi`), RAM, Datenträger, Netzwerk über den Adapter der Internetverbindung; CPU-Temperatur und Lüfter aus dem HWiNFO-Shared-Memory. `hwinfo-einrichten.bat` stellt HWiNFO sparsam ein und startet es alle 11 Stunden neu.
- Seite **Blog-Updates** (RSS/Atom, Feed wird zur Blog-Adresse gesucht), neuester Beitrag je Blog.
- Seitenrotation alle 15 Sekunden, Autostart, `DOKUMENTATION.md`.

### Behoben
- Bild stand auf dem Kopf: Ausrichtung `reverse_landscape`.
- Netzwerkwerte falsch, weil die Hersteller-App den Loopback-Adapter maß.
- Ab- und Aufblenden beim Seitenwechsel entfernt: Helligkeitsbefehle zwischen Bilddaten erzeugten Bildfehler.
- Helligkeit von 50 auf 90 angehoben.
- Ganzes Bild wird bei jedem Seitenwechsel und alle 5 Minuten neu gesendet. Ein Streifen an fester Stelle bei einfarbigen Testbildern liegt an Display oder Kabel, nicht an der App.

[Unveröffentlicht]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v1.2...HEAD
[1.2]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v1.1...v1.2
[1.1]: https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v1.1
