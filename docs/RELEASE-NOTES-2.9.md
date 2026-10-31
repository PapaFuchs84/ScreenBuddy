# ScreenBuddy 2.9.0

31.10.2026 · Windows 10/11 x64 · Companion 1.0.2 (Android 8+, auch Android TV)

## Deutsch

- **Farben & Design:** „An Windows anpassen“ (Hell/Dunkel und Akzentfarbe von Windows, live, automatische Kontrastkorrektur, Windows-Kontrastdesign = Systemfarben) und **eigene Themes** (JSON, Editor mit Live-Vorschau und Kontrastwarnung, Duplizieren, Löschen, Import, Export). „Eigene Farben“ werden automatisch in ein Theme übernommen. Drei Beispiel-Themes (Cupertino-, Material-, Fluent-Stil) unter `cpp/examples/themes`. Das Einstellungsfenster kennt „Windows“ als Darstellung. Siehe [THEMES.md](THEMES.md).
- **Navigation:** Statuspunkte nur noch bei Datenquellen und Verbindungen (grün = eingerichtet und eingeschaltet, grau = nicht eingerichtet oder aus), mit Legende.
- **Streaming-Neustarts:** alle gewählten Anbieter werden fair gemischt (vorher fast nur Netflix).
- **Spotify-Amplifier** (Balken zur Musik) und Korrektur: Analyse nur bei sichtbarer Wiedergabeseite, Balken klingen vollständig ab.
- **Companion 1.0.2:** Android-TV-Einstieg mit D-Pad-Bedienung; QR-Scan nur mit Kamera.
- **TMDB:** offizielles Logo in README und Third-Party-Notices neben dem Pflichttext.

### Prüfung

- Release-Build: Kern 465, TMDB 119, Companion 361 und die übrigen Suiten bestanden, 2 Installer-Erhaltungsprüfungen bestanden.
- Echtsystem (Meanmachine): Update über den Installer, Themes-Migration der vorhandenen „Eigenen Farben“; USB-Display COM7 verbunden; Redmi 8, Pixel 10 Pro und Android-TV (BeyondTV) gekoppelt und mit Live-Bild beobachtet. Streaming mit echtem TMDB-Key: Netflix, Prime Video, Disney+ und Apple TV+ gemischt mit Logo.

### Grenzen

Noch nicht enthalten (geplant für 2.9.1): Plugin-Store-Typ „Theme“ und Maus-Tooltip der Statuspunkte. Kein echtes Store-Update geprüft. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen. Das TV-Companion-APK ist dasselbe signierte APK 1.0.2. Vor dem Upgrade EXE, `config.toml` und `companion.json` sichern. Amplifier: Pegel am seriellen Display und Standard-Audiogerätewechsel nicht gesondert abgenommen.

## English

- **Colors & design:** "Match Windows" (light/dark and accent from Windows, live, automatic contrast correction, Windows high contrast = system colors) and **custom themes** (JSON, editor with live preview and contrast warning, duplicate, delete, import, export). "Custom colors" migrate automatically. Three example themes (Cupertino, Material, Fluent style) in `cpp/examples/themes`. The settings window offers "Windows" as appearance. See [THEMES.md](THEMES.md).
- **Navigation:** status dots only for data sources and connections (green = set up and on, grey = not set up or off), with a legend.
- **New on streaming:** all chosen providers are mixed fairly (previously almost only Netflix).
- **Spotify amplifier** (bars to the music) and fix: analysis only while the playback page is visible, bars decay fully.
- **Companion 1.0.2:** Android TV entry with D-pad control; QR scan only with a camera.
- **TMDB:** official logo in README and third-party notices next to the required notice.

### Verification

Release build: core 465, TMDB 119, Companion 361 and the remaining suites passed; 2 installer-preservation checks passed. Real system: installer update, migration of existing custom colors, USB display COM7 connected; Redmi 8, Pixel 10 Pro and Android TV (BeyondTV) paired and observed with live frames. Streaming with a real TMDB key: Netflix, Prime Video, Disney+ and Apple TV+ mixed with logos.

### Limits

Not included yet (planned for 2.9.1): plugin store type "theme" and mouse tooltip for status dots. No real store update was tested. The Windows EXE is unsigned; compare with `SHA256SUMS.txt`. Back up EXE, `config.toml` and `companion.json` before upgrading. Amplifier: levels on the serial display and default audio device changes not individually verified.
