# ScreenBuddy 2.9.0

Entwurf bis zur Veröffentlichung (Datum eintragen) · Windows 10/11 x64 · Companion-App: Version laut Release-Seite (Android 8+)

## Deutsch

- **Farben & Design:** neue Option **An Windows anpassen** (Hell/Dunkel und Akzentfarbe von Windows, wirkt live; im Windows-Kontrastdesign nutzt das Display die Systemfarben; Text und Balken werden automatisch kontrastkorrigiert). **Eigene Themes** als JSON-Dateien mit Editor, Live-Vorschau, Kontrastwarnung, Duplizieren, Löschen, Import und Export; „Eigene Farben“ werden beim ersten Start automatisch migriert. Drei Beispiel-Themes (Cupertino-, Material- und Fluent-Stil) unter `cpp/examples/themes`. Das Einstellungsfenster bietet „Windows“ als Darstellung. Details: [THEMES.md](THEMES.md).
- **Navigation:** einheitliche Statuspunkte nur bei Datenquellen und Verbindungen (grün = eingerichtet und eingeschaltet, grau = nicht eingerichtet oder aus), mit Legende.
- **Streaming-Neustarts:** Die Seite zeigte fast nur Netflix. Jetzt fragt die App je Anbieter getrennt ab (bis zu 8 Gruppen) und wählt reihum den neuesten Titel je Anbieter. Das **offizielle TMDB-Logo** steht in der Quellenangabe (README, `THIRD-PARTY-NOTICES.md`).
- **Spotify-Amplifier:** 16 Balken mit dem Pegel der Windows-Systemausgabe (WASAPI-Loopback, lokal, nichts wird aufgezeichnet); Analyse nur, solange die Spotify-Seite sichtbar ist, nach dem Seitenwechsel endet sie nach höchstens drei Sekunden, Restbalken klingen auf null ab. Schalter: Einstellungen → Spotify → *Amplifier-Anzeige*.
- **Companion / Android TV:** eigener Eintrag im TV-Launcher (mit Banner); Fernbedienung: Pfeil links/rechts wechselt die Seite, OK ohne Aktion, Zurück wie bisher; ohne Kamera kein QR-Scan und keine Kameraberechtigung (manuelle Kopplung bleibt). Der Bildschirm bleibt auch in der sichtbaren Einrichtung an.

### Prüfung

*Release-Build: 465 Kern-, 119 TMDB-, 361 Companion- und die übrigen Suiten bestanden, 2 Installer-Erhaltungsprüfungen bestanden. Echtsystem (Meanmachine): Update über den Installer; Migration der vorhandenen „Eigenen Farben“ in ein Theme (Optik unverändert); USB-Display COM7 verbunden; Redmi 8, Pixel 10 Pro und Android-TV (BeyondTV) gekoppelt und mit Live-Bild beobachtet; Streaming mit echtem TMDB-Key (Netflix, Prime Video, Disney+, Apple TV+ gemischt, mit Logo). Nicht geprüft: Store-Update, Themes-Editor am echten Display per Hand, physische TV-Fernbedienung. Codex-Review: Befunde 1 und 2 (Migration) behoben; weitere Befunde folgen in 2.9.1.*

### Grenzen

**Nicht in 2.9.0:** Teilen von Themes über den Plugin-Store (Typ „Theme“), Maus-Tooltip für die Statuspunkte und der Amplifier bei Remote-Wiedergabe (Spotify auf Handy oder Lautsprecher). Die Bebilderte Anleitung beschreibt Farben & Design und Statuspunkte nur im Text (keine neuen Bildschirmfotos).

Vor dem Upgrade EXE, `config.toml` und `companion.json` sichern. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen.

## English

- **Colors & design:** new **Match Windows** option (Windows light/dark and accent color, applied live; in Windows high-contrast mode the display uses the system colors; text and bars are contrast-corrected automatically). **Custom themes** as JSON files with editor, live preview, contrast warning, duplicate, delete, import and export; "Custom colors" migrate automatically on first start. Three example themes (Cupertino, Material and Fluent style) in `cpp/examples/themes`. The settings window offers "Windows" as appearance. Details: [THEMES.md](THEMES.md) (bilingual).
- **Navigation:** uniform status dots only for data sources and connections (green = set up and on, grey = not set up or off), with a legend.
- **New on streaming:** the page showed almost only Netflix. The app now queries each provider separately (up to 8 groups) and picks the newest title per provider in turn. The **official TMDB logo** appears in the attribution (README, `THIRD-PARTY-NOTICES.md`).
- **Spotify amplifier:** 16 bars with the level of the Windows system output (WASAPI loopback, local, nothing is recorded); the analysis runs only while the Spotify page is visible, ends within three seconds after leaving it, and leftover bars decay to zero. Switch: Settings → Spotify → *Amplifier display*.
- **Companion / Android TV:** own entry in the TV launcher (with banner); remote: left/right switches the page, OK has no action, Back as before; without a camera there is no QR scan and no camera permission (manual pairing remains). The screen also stays on during the visible setup.

### Validation

*Release build: 465 core, 119 TMDB, 361 Companion and the remaining suites passed, 2 installer-preservation checks passed. Real system (Meanmachine): installer update; migration of existing custom colors into a theme (look unchanged); USB display COM7 connected; Redmi 8, Pixel 10 Pro and Android TV (BeyondTV) paired and observed with live frames; streaming with a real TMDB key (Netflix, Prime Video, Disney+, Apple TV+ mixed, with logos). Not checked: store update, theme editor by hand on the real display, physical TV remote. Codex review: findings 1 and 2 (migration) fixed; further findings follow in 2.9.1.*

### Limits

**Not in 2.9.0:** sharing themes through the plugin store (type "theme"), a mouse tooltip for the status dots and the amplifier for remote playback (Spotify on a phone or speaker). The illustrated guide describes colors & design and status dots in text only (no new screenshots).

Back up the EXE, `config.toml` and `companion.json` before upgrading. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`.
