# ScreenBuddy 2.9.1

2026-10-31 · Windows 10/11 x64 · Companion 1.0.2 (unverändert zu 2.9.0).

## Deutsch

- **Store-Themes:** signierter Katalog `themes.json`, Installation von Cupertino-, Fluent- und Material-Stil im Plugin-Store. Die App prüft Signatur, Größe, SHA-256 und Schema. Themes enthalten nur Farben; unter **Farben & Design** auswählen und **Speichern & anwenden**.
- **Navigation:** Tooltips erklären die Statuspunkte der Seitenleiste; die Timer-Verarbeitung des Einstellungsfensters ist korrigiert.
- **Key-Hilfe:** Karten mit Schritten und festen Links für Discord, Steam, Xbox/OpenXBL, TMDB, Spotify und Home Assistant, auf Deutsch und Englisch.
- **Amplifier:** erkennt Änderungen des Standard-Audiogeräts. Bei Remote-Wiedergabe und vier Sekunden stiller Systemausgabe erscheint eine klar als **simuliert** bezeichnete Animation. Balkenfarben: Pegel, Designverlauf oder einfarbig. Keine Aufnahme, kein Mikrofon, keine Spotify-Analyse-API.
- **Amplifier bei Stille:** sichtbare Wiedergabe hält die Pegelberechnung aktiv, damit die simulierte Animation nach vier Sekunden starten kann.
- **Korrekturen:** Kontrastregeln für Hintergrund und Karte werden gemeinsam erfüllt; Absturz im Pinsel-Cache behoben.

Der Companion-/Android-Code bleibt gegenüber 2.9.0 unverändert; kein neues APK für dieses Release.

### Prüfstand und Grenzen

Build und automatisierte Suiten bestanden. Meanmachine aktualisiert, COM7 geöffnet, drei Companion-Geräte mit 1.0.2 verbunden und TV neu gekoppelt. Animierte simulierte Balken auf der realen Spotify-Seite bestätigt. Details in [TESTPROTOKOLL.md](../cpp/TESTPROTOKOLL.md). Noch offene Prüfungen sind keine bestätigten Ergebnisse. Keine physische Finger-/Fernbedienungsabnahme oder gemessene serielle Bildrate. Vollständige EN-Key-Hilfe- und Browserabnahme der Anleitungen bleibt offen.

Vor einem späteren Upgrade EXE, Konfiguration und Companion-Kopplungen sichern. Die Windows-EXE ist unsigniert; vor Verwendung mit `SHA256SUMS.txt` abgleichen. Ein Rückwechsel erfolgt mit der gesicherten EXE und den zugehörigen gesicherten Daten.

## English

- **Store themes:** signed `themes.json` catalog with Cupertino, Fluent and Material styles. Signature, size, SHA-256 and schema are checked. Themes contain colors only; select one in **Colors & design**, then **Save & apply**.
- **Navigation:** tooltips explain sidebar status dots; settings-window timer handling is fixed.
- **Key help:** German and English instructions with fixed links for Discord, Steam, Xbox/OpenXBL, TMDB, Spotify and Home Assistant.
- **Amplifier:** detects default audio device changes. Remote playback with four seconds of silent system output shows an animation clearly labelled **simulated**. Bar colors: level, theme gradient or single color. No recording, microphone or Spotify analysis API.
- **Silent amplifier:** visible playback keeps the level meter active so the simulated animation can start after four seconds.
- **Fixes:** background and card contrast rules are satisfied together; brush-cache crash fixed.

Companion/Android code is unchanged from 2.9.0; no new APK for this release.

### Validation and limits

Build and automated suites passed. Meanmachine updated, COM7 opened, three Companion 1.0.2 devices connected and TV paired again. Animated simulated bars confirmed on the live Spotify page. See [TESTPROTOKOLL.md](../cpp/TESTPROTOKOLL.md) for actual validation evidence. Pending checks are not confirmed results. Physical finger/remote acceptance and measured serial frame rate remain open, as do full English key-help and browser-guide acceptance.

Before a later upgrade, back up the EXE, configuration and Companion pairings. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`. Roll back using the backed-up EXE and its matching backed-up data.
