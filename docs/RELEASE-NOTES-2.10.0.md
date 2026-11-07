# ScreenBuddy 2.10.0 – veröffentlicht am 07.11.2026

**Status:** veröffentlicht. Release **2.10.0** (07.11.2026); die Companion-App bleibt **1.0.3** (keine neue APK). Änderungen im Detail: [CHANGELOG.md](../CHANGELOG.md).

## Neu: Plugin-Seiten

- **Zonen-Layout (Manifest v2):** `schema_version: 2` beschreibt ein Raster mit Zonen und Blöcken mit stabilen IDs. Grenzen: 6 × 6 Raster, 8 Zonen, 4 Blöcke je Zone, 12 Widgets mit Layout. Manifest v1 bleibt unverändert. Beschreibung und Beispiel: [Plugin-System](PLUGIN-SYSTEM.md#manifest-v2-zonen-layout-2100-in-vorbereitung).
- **Dichtestufen:** kompakt, normal und ausführlich je Bausteintyp. Passt ein Block nicht, wird er ausgeblendet statt überlappt.
- **Bedingte Anzeige (`show_if`)** und **Fehlerisolierung:** Ein Block erscheint nur bei vorhandenem Wert oder einfachem Vergleich; schlägt ein Block fehl, fehlt nur dieser Block.
- **Wechselfeld:** Eine Zone wechselt zwischen Blöcken (Modus `rotate` mit 3–600 s oder `first_available`).
- **Tap-Ziele:** Blöcke können HTTPS-Links am PC öffnen. Die Freigabe je Plugin ist standardmäßig aus.
- **Seiten-Editor:** Kachelpalette, Raster, Blockliste, Bedingungen, Vorschau in Quer- und Hochformat, Rückgängig/Wiederholen, Tastaturbedienung. Gespeichert wird nur die Anordnung, mit Sicherungskopie. Der frühere Eintrag „Seite anordnen“ entfällt; Plugin- und Standardseiten werden über die Seitenliste von **Eigene Ansichten** bearbeitet.
- **Rahmenstil:** Plugin-Karten übernehmen Radius und Abstände aus dem Theme.

## Neu: Eigene Ansichten (P8-2)

Unter **App → Eigene Ansichten** Quellen-Kacheln kombinieren, mit Palette/Raster/Eigenschaften bearbeiten und je aktivem Display speichern. HA-Entitäten bleiben nativ gezeichnet; Listen und Spotify-Inhalte sind einzeln verfügbar. Offline-Tabs entfallen, gespeicherte Geräte-Layouts bleiben erhalten. Je Kachel lässt sich ein Touch-Ziel festlegen (Seite wechseln, Link nur mit Freigabe, nächster Eintrag bei Plugin-Wechselfeldern). KI-Status-Kacheln sind enthalten. [Bedienung, Grenzen und aktueller Prüfstand](PHASE8-EIGENE-ANSICHTEN.md).

## Neu: Game-Mode

- **Spielerkennung** (lesend): Steam, Epic, Xbox, GOG Galaxy, Ubisoft Connect, EA App/Origin, Battle.net und Rockstar; Vollbildprüfung, Anti-Cheat-geschützte Spiele über PID/EXE-Namen, zusätzliche Spiele und Ausschlüsse in den Einstellungen.
- **Intro und Ansicht:** Beim Spielstart ein Logo-Intro von 2,5 Sekunden (reduzierte Variante bei „Animationen reduzieren“), danach eine eigene Game-Ansicht auf dem gewählten Display. Die normale Rotation pausiert und läuft nach Spielende weiter. Bei aktivem Game-Mode ist die Game-Ansicht aus Rotation, Companion-Navigation und Tap-Zielen reserviert.
- **Messwerte:** 16 System-Einzelkacheln (u. a. FPS, Frametime, 1%-Low, GPU, VRAM, Ping) und 60-Sekunden-Sparklines. Fehlende Quellen zeigen `n/a`.
- Einrichtung und Grenzen: [Plan und Prüfstand](PLAN-GAME-MODE.md).

## Neu: KI-Status

- Karten für Claude Code und Codex mit Status- und Sitzungsanimationen, lokale Hook-Ereignisse (Loopback, Port 18765 mit Fallback konfigurierbar) mit Vorschau, einmaliger Sicherung und selektiver Entfernung, lokale `ai.*`-Kacheldaten.
- Limits über die vorhandene Konto-Einwilligung aus den Windows-Login-Dateien (inoffizielle HTTPS-Schnittstellen, Abfrage im 3-/15-Minuten-Takt, kein Token-Refresh). Ein abgelaufenes Token wird alle 60 Sekunden neu geprüft. Werte ohne Messwert bleiben `n/a`.
- Details und offene Arbeiten: [KI-Status-Prüfstand](../cpp/ai-status-report.md).

## Neu: Spotify, Theme, BF6

- **Spotify:** Läuft nichts, zeigt die Seite den zuletzt gespielten Titel mit Cover (gedimmt, ohne Tasten). Ein Tipp auf das Cover öffnet die Spotify-App **am PC**, auch bei einem Tipp von einem Companion-Gerät. Einen Start der Spotify-App am Handy gibt es nicht.
- **Standard-Theme `material-orange`:** neuer Standard für Neuinstallationen; bestehende Installationen werden einmal gefragt (Voreinstellung Nein). Andere Themes bleiben wählbar.
- **BF6-Rangliste:** Eintrag per ID (`Name#ID`); bei gestörter Quelle (HTTP 5xx) erscheint „Quelle gestört“ statt „nicht gefunden“, zuletzt gute Werte bleiben.
## Neu: Geräte und Darstellung

- **Sperrzone pro Gerät:** Ränder in ganzen Pixeln für USB-Display und Monitor (unter **Anzeige**). Companion mit gemeinsamer Vorgabe und eigener Sperrzone je gekoppeltem Gerät im unabhängigen Betrieb. Details: [Navigation](NAVIGATION.md#geräteprofil-und-sperrzone-phase-4a-entwicklung) · [Companion](COMPANION.md).
- **Android-TV** läuft immer im Querformat; die Ausrichtungswahl ist dort ausgeblendet.
- **Eingebaute Seiten** (System, Freunde, Discord, Battlefield 6, Blogs, Streaming, Spotify) zeichnen ihre Karten über den gemeinsamen Rahmen. Home Assistant bleibt nativ.
- **Kontrast:** Warn- und Kopfzeilenfarben in drei Paletten angehoben; Textkontrast ≥ 4,5:1 wird in einer Themenmatrix geprüft.

## Behoben

- **BF6-Rangliste (Plattform):** PC-Konten werden mit `platform=pc` abgefragt, mit Rückfall auf `ea` und den kleingeschriebenen Namen.
- **Spotify:** Läuft nichts, zeigt die Seite den zuletzt gespielten Titel. Dafür einmal unter **Einstellungen › Spotify** neu anmelden, damit der Zugriff „zuletzt gespielt“ erteilt wird.

## Sicherheit und Kompatibilität

- Tap-URLs nur über HTTPS, strikt geprüft und beim Tipp erneut kontrolliert; Link-Freigabe bei jeder anderen Plugin-Änderung widerrufen (nur eine reine Linklisten-Änderung behält sie); höchstens ein Linkversuch je Plugin alle zwei Sekunden.
- Geheime Einstellungen sind in Bedingungen verboten; ein ungültiges `show_if` lehnt das Manifest ab.
- Schema 2 braucht im Store `min_app_version` 2.10.0. Ältere Apps zeigen das Plugin als „braucht neueres ScreenBuddy“ und lehnen es ab.
- Companion-Protokoll v1 bleibt unverändert; ältere Companion-Versionen funktionieren ohne Anpassung.

## Prüfung und Grenzen

- **Automatische Prüfungen: grün** (Zonen, Bedingungen, Beispiele, Snapshots, Tap-Ziele, Wechselfeld-Planung) laufen gegen feste Daten. Prüfzahlen und Umgebungsgrenzen stehen in [`cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md).
- **Hardware- und Geräteabnahme nur teilweise live** (Redmi 8, Android-TV, Pixel 10 Pro): Game-Mode-Intro am Redmi 8, BF6-Erkennung am PC, Rangliste am Android-TV, KI-Status. **Nicht abgenommen:** weitere echte Spiele und Launcher, USB-Display (auch Hochformat-Gehäuse), Monitor, gemessene serielle Bildrate für das Wechselfeld, Companion-Prozesstests.
- **Spotify am Handy:** Ein Companion-Gerätestart von Spotify gibt es nicht; der Cover-Tipp öffnet Spotify am PC.
- Companion 1.0.4 ist ein Entwicklungsstand ohne APK-Auslieferung; ausgeliefert bleibt 1.0.3.
- **Eingebaute Seiten:** Die Basislinie ist rechnergebunden (Roboto als Benutzerschrift nötig). Auf dem Referenzrechner sind alle migrierten Seiten bitgleich (0 Abweichungen); die Hardware-Abnahme steht aus.
- Die Anleitungen in der EXE (`cpp/res/GUIDE.html`, `ANLEITUNG.html`) sind noch nicht auf 2.10.0 abgeglichen; sie werden nur zusammen mit dem Build-Hash-Mechanismus geändert.

## English

**Status: released.** Release **2.10.0** (2026-11-07); the Companion app remains **1.0.3** (no new APK).

- **Plugin pages:** manifest v2 with a zone grid (up to 8 zones, 4 blocks per zone, 12 widgets with a layout), density levels, conditional display (`show_if`), fault isolation per block, cycling zones (`rotate`, 3–600 s, or `first_available`), and tap targets that open HTTPS links on the PC. Link consent is off by default. The page editor has a tile palette; the old "Arrange page" entry is gone, pages are edited via the page list of Custom views. Manifest v1 is unchanged. See [Plugin system](PLUGIN-SYSTEM.en.md).
- **Custom views (P8-2):** dedicated editor under App, with source palette, grid, properties and layouts per active display; native HA entity/list/Spotify tiles reuse existing painters. Offline tabs are hidden while saved layouts remain. Custom touch actions are pending. [Usage and validation](PHASE8-EIGENE-ANSICHTEN.md).
- **Game-Mode:** read-only game detection (Steam, Epic, Xbox, GOG Galaxy, Ubisoft Connect, EA App/Origin, Battle.net, Rockstar), a 2.5-second logo intro, then a dedicated game view with 16 system tiles and 60-second sparklines; normal rotation pauses and resumes after the game. [Plan](PLAN-GAME-MODE.md).
- **AI Status:** Claude Code and Codex cards, local hooks, limits via the existing account consent (3/15-minute polling; an expired token is re-checked every 60 seconds). [Report](../cpp/ai-status-report.md).
- **Spotify and theme:** the cover tap opens Spotify on the PC (also when tapped from a Companion device); new default theme `material-orange` (existing installations are asked once); BF6 players can be entered by ID and a failing source shows "source unavailable".
- **Devices:** keepout in whole pixels 0–1000 for the USB display and monitor, plus a shared default and an individual keepout per paired Companion device in independent mode. Android TV always runs in landscape.
- **Built-in pages** draw their cards through the shared frame; Home Assistant stays native.
- **Fixed:** BF6 leaderboard requests use `platform=pc` with fallbacks; Spotify shows the last played track when nothing is playing (re-sign-in required for the new permission).
- **Security:** strict HTTPS tap URLs checked again at tap time; link consent revoked on any other plugin change (a pure link-list change keeps it); secrets are forbidden in conditions.
- **Validation:** automated checks are green. Hardware and device acceptance was only partly done live (Redmi 8, Android TV, Pixel 10 Pro): Game-Mode intro on the Redmi 8, BF6 detection on the PC, leaderboard on Android TV, AI Status. **Not accepted:** further real games and launchers, USB display (including the portrait enclosure), monitor, measured serial frame rate for cycling zones, Companion process tests. There is no Companion-side launch of Spotify on the phone; the cover tap opens Spotify on the PC. The built-in baseline is machine-bound (Roboto as a user font); on the reference machine all migrated pages compare bit-exact (0 differences). Details: [`cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md).
