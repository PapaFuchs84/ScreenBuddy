# ScreenBuddy 2.8.0

31.10.2026 · Windows 10/11 x64 · Companion 1.0.1 (Android 8+)

## Deutsch

- **Spotify-Seite:** aktueller Titel mit Cover, Künstler, Fortschritt und Touch-Tasten für Zurück, Wiedergabe/Pause und Weiter (Premium nötig). Anmeldung per PKCE mit eigener Spotify-Developer-App, Token DPAPI-verschlüsselt. Anleitung: [SPOTIFY.md](SPOTIFY.md).
- **Streaming-Neustarts:** neue Seite mit Filmen und Serien der letzten 30 Tage bei gewählten Anbietern (TMDB, Anbieterdaten JustWatch). Key als v3-Key oder Read Access Token. RTL+ nutzt die TMDB-ID 2750 (Region DE).
- **Einstellungen:** Schnellzugriff mit Symbolen sowie Anleitung (DE/EN) und Plugin-Werkstatt; die linke Navigation passt sich der Fensterhöhe an; die rechte Seite zeigt jedes verbundene Gerät (USB-Display, jede Companion-App) als anwählbare Karte, die Live-Vorschau zeigt das gewählte Gerät.
- **Companion 1.0.1:** randloser Hintergrund, Inhalt bleibt im sicheren Bereich vor abgerundeten Ecken und Kamera-Ausschnitt.
- **Fehlerkorrektur:** `--uninstall` bricht in Testinstanzen ohne eigenen Installordner ab, statt die echte Installation zu löschen.
- Neue Seiten erscheinen bei bestehenden Installationen erst, wenn sie unter **Seiten & Reihenfolge** (Display und Companion-Liste) angehakt werden.

### Prüfung

- Release-Build: 330 Kern-, 77 TMDB-, 251 Parser-, 41 Beispiel-, 127 Store-, 140 Transport-, 135 Einstellungs- und 361 Companion-Prüfungen bestanden (1.462 mit Zähler), Monitor-Suite und 2 Installer-Erhaltungsprüfungen ohne Fehler.
- Echtsystem (Meanmachine): Update von 2.7.0 auf 2.8.0 über den Installer, `config.toml` und Companion-Kopplungen erhalten; USB-Display (COM7, 480 × 320) verbunden. Streaming mit echtem TMDB-Key: Titel, Datum, Film/Serie, Anbieter auf Display und beiden Handys; Anbieter-IDs für WOW (30) und RTL+ (2750) gegen die TMDB-Anbieterliste DE geprüft. Spotify mit echtem Konto: Anmeldung, Titel, Cover und Fortschritt auf dem Redmi 8 (Companion 1.0.1) beobachtet; die Befehle Pause und Wiedergabe sind im Protokoll belegt. Companion: Pixel 10 Pro (Rundungen 157 px, Kamera-Ausschnitt) und Redmi 8 geprüft.

### Grenzen

Das offizielle TMDB-Logo ist noch nicht eingebunden (nur der Pflichttext). Kein echtes Store-Update in dieser Version geprüft; Plugin-Store unverändert zu 2.7.0. Spotify auf dem Pixel 10 Pro, am USB-Display (physischer Touch) sowie Weiter/Zurück nicht einzeln beobachtet. Keine Amplifier-/Pegelanzeige in 2.8.0. Der Codex-Review der Änderungen wurde gestartet, aber nicht abgeschlossen. DPI-/Screenreader-, Windows-Anmelde-, WLAN- und Boot-Abnahme stehen weiter aus.

Vor dem Upgrade EXE, `config.toml` und `companion.json` sichern. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen. Die signierte Companion-APK 1.0.1 (versionCode 5, gleicher Schlüssel wie 1.0.0) wird separat mitgeliefert und aktualisiert 1.0.0 ohne neue Kopplung.

## English

- **Spotify page:** current track with cover, artist, progress and touch buttons for previous, play/pause and next (Premium required). Sign-in via PKCE with your own Spotify developer app, DPAPI-encrypted token. Guide: [SPOTIFY.en.md](SPOTIFY.en.md).
- **New on streaming:** new page with movies and series of the last 30 days on your chosen providers (TMDB, provider data JustWatch). Key as v3 key or Read Access Token. RTL+ uses TMDB ID 2750 (region DE).
- **Settings:** shortcuts with icons plus guide (DE/EN) and Plugin workshop; the left navigation adapts to the window height; the right-hand panel shows every connected device (USB display, each Companion app) as a selectable card and the live preview shows the selected device.
- **Companion 1.0.1:** edge-to-edge background, content stays inside the safe area clear of rounded corners and camera cut-out.
- **Fix:** `--uninstall` aborts in test instances without their own install folder instead of deleting the real installation.
- On existing installations new pages only appear once ticked under **Pages & order** (display and Companion list).

### Validation

Release build: 330 core, 77 TMDB, 251 parser, 41 examples, 127 store, 140 transport, 135 settings and 361 Companion checks passed (1,462 counted); the monitor suite and 2 installer preservation checks reported no failures.

Real system (Meanmachine): update from 2.7.0 to 2.8.0 through the installer, `config.toml` and Companion pairings kept; USB display (COM7, 480 × 320) connected. Streaming with a real TMDB key: titles, date, movie/series, provider on the display and both phones; provider IDs for WOW (30) and RTL+ (2750) checked against the TMDB DE provider list. Spotify with a real account: sign-in, track, cover and progress observed on the Redmi 8 (Companion 1.0.1); pause and play commands are recorded in the log. Companion checked on Pixel 10 Pro (corner radius 157 px, camera cut-out) and Redmi 8.

### Limits

The official TMDB logo is not included yet (attribution text only). No real store update checked in this version; the plugin store is unchanged from 2.7.0. Spotify on the Pixel 10 Pro, on the USB display (physical touch) and next/previous were not individually observed. No amplifier/level meter in 2.8.0. The Codex review was started but not completed. DPI/screen-reader, Windows sign-in, Wi-Fi and boot acceptance remain open.

Back up the EXE, `config.toml` and `companion.json` before upgrading. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`. The signed Companion APK 1.0.1 (versionCode 5, same key as 1.0.0) ships separately and updates 1.0.0 without re-pairing.