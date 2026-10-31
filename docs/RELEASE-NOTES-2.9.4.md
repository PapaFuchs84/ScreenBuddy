# ScreenBuddy 2.9.4

2026-10-31 · Windows 10/11 x64 · Companion 1.0.3

## Deutsch

- **Streaming per Touch:** Einen Film oder eine Serie auf „Streaming-Neustarts“ antippen öffnet die passende TMDB-Infoseite am PC. Die Ziele gehören zum angezeigten Datenstand. Neue Tabs richten sich nach den Einstellungen des Standardbrowsers; mit Chrome auf Meanmachine als neue Tabs bestätigt.
- **Spotify-Cover:** Das geladene Cover antippen öffnet die Spotify-App am PC, auch wenn sie minimiert ist. Kein Premium für das Öffnen nötig. Die Wiedergabetasten bleiben getrennt; diese benötigen weiterhin Premium. Ohne Cover ist die Aktion inaktiv; bei nicht registrierter Spotify-App erfolgt kein Start.
- **TMDB-Einrichtung:** Deutsche und englische Hilfe erklären die Beantragung mit Beispielen für Nutzungsart, App-Name, Projekt-URL und Beschreibung sowie die Wahl des API-Zugangswerts. Persönliche Pflichtfelder mit eigenen Angaben ausfüllen; das Formular kann sich ändern.

Companion 1.0.3 liegt bei (bereits separat veröffentlicht, modernes Login-/Kopplungsdesign in Hell und Dunkel). Für diese Touchaktionen ist kein neues Android-Protokoll erforderlich.

### Prüfung

Release-Build, 3327 Kernprüfungen, 119 TMDB, Key-Hilfe, 37 PSN, 13 Amplifier, 251 Plugins, 41 Plugin-Beispiele, 127 Store, 140 Transport, 135 Settings, 111 Signatur, 77 Katalog, 11 Monitor, 361 Companion sowie zwei isolierte Installerprüfungen bestanden. App-Integration: 39 Prüfungen; Mehrgeräte-Integration: 17 Prüfungen.

Meanmachine von 2.9.3 aktualisiert, EXE/Konfiguration/Kopplungen vorher gesichert. Konfiguration nach der Abnahme bytegleich wiederhergestellt; USB-Display verbunden. Mit ausdrücklich neu gekoppeltem Redmi 8: Film „Auf Kollisionskurs“ und Serie „Nocturne“ öffnen die richtigen TMDB-Seiten in separaten PC-Browser-Tabs. Geladenes Spotify-Cover holt die zuvor minimierte Desktop-App nach vorn. TMDB-Hilfe auf Deutsch und Englisch in der installierten Oberfläche visuell geprüft. Testwiedergabe anschließend pausiert.

Keine physische Fingerabnahme; Redmi-Tipps wurden über ADB injiziert. Pixel/TV, Screenreader und weitere Standardbrowser wurden für diese Version nicht geprüft. Die öffentliche TMDB-Dokumentation bestätigt Beantragung und Zugangswerte; das persönliche Antragsformular wurde nicht ausgefüllt.

Vor einem Upgrade EXE, Konfiguration und Companion-Kopplungen sichern. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen.

## English

- **Streaming touch:** Tap a movie or series on “New on streaming” to open its matching TMDB details on the PC. Targets belong to the rendered snapshot. New tabs follow default browser settings; verified as separate Chrome tabs on Meanmachine.
- **Spotify cover art:** Tap loaded cover art to open the Spotify desktop app, including when minimized. Opening does not require Premium. Playback buttons remain separate and still require Premium. No cover means no action; an unregistered Spotify app is not launched.
- **TMDB setup:** German and English help explain application fields with examples for use type, application name, project URL and summary, and identify the supported API credentials. Use your own details for required personal fields; the form may change.

Companion 1.0.3 is included (already published separately, modern light/dark pairing design). These actions require no new Android protocol.

### Validation

Release build, 3327 core checks, 119 TMDB, key help, 37 PSN, 13 amplifier, 251 plugins, 41 examples, 127 store, 140 transport, 135 settings, 111 signature, 77 catalog, 11 monitor, 361 Companion and two isolated installer checks passed. App integration: 39 checks; multiple devices: 17 checks.

Meanmachine upgraded from 2.9.3 after backing up the EXE, configuration and pairings. Configuration restored byte-identically after acceptance; USB display connected. With the explicitly newly paired Redmi 8, the displayed movie and series opened their correct TMDB details in separate PC browser tabs. Tapping loaded Spotify cover art restored the minimized desktop app. German/English TMDB help visually checked in the installed app. Test playback paused afterwards.

Redmi taps were injected through ADB; physical finger input was not verified. Pixel/TV, screen readers and other default browsers were not tested for this version. Public TMDB documentation confirms registration and credentials; no personal application form was submitted.

Back up the EXE, configuration and Companion pairings before upgrading. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`.

TMDB references: [Getting started](https://developer.themoviedb.org/docs/getting-started), [Application authentication](https://developer.themoviedb.org/docs/authentication-application).
