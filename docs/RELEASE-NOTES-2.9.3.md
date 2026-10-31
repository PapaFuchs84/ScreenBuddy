# ScreenBuddy 2.9.3

2026-10-31 · Windows 10/11 x64 · Companion 1.0.2 (unverändert).

## Deutsch

- **PlayStation-Freunde:** Die gemeinsame Freundesliste zeigt auch PlayStation-Freunde (Marke „P“, Online-Status, gespieltes Spiel). Das geht nur über Sonys inoffizielle Schnittstelle mit dem NPSSO-Code, auf eigenes Risiko; Sony kann sie jederzeit ändern. Der Code wird mit DPAPI verschlüsselt gespeichert, die Funktion ist optional und standardmäßig aus. Mit Hilfe zum Code, Verbindungsprüfung und Verknüpfung per `psn:Name`.
- **Korrektur:** Sonys Autorisierungscode mit Punkt (`v3.…`) wird akzeptiert; die Diagnose der Anmeldung steht im Log.

Companion/Android-Code unverändert; kein neues APK (1.0.2 liegt zur Bequemlichkeit bei).

### Prüfstand und Grenzen

Release-Build und alle automatisierten Suiten (1357 Kern, 119 TMDB, 251 Plugins, 41 Beispiele, 127 Store, 140 Transport, 135 Settings, 11 Monitor, 361 Companion) sowie beide Installerprüfungen bestanden. Meanmachine von 2.9.2 aktualisiert, Konfiguration byte-identisch erhalten, COM7 verbunden, Redmi und TV zeigen Seiten der neuen Version (Screenshots). PlayStation live: mit frischem Code lieferte die Freundesliste ein PlayStation-Konto ohne Fehler. **Nicht geprüft:** Online-Status/Spiel von PlayStation-Freunden auf den Geräten, Pixel, physische Finger-/Fernbedienungsabnahme, Store-Installation/-Update über die Oberfläche.

Vor einem Upgrade EXE, Konfiguration und Companion-Kopplungen sichern. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen.

## English

- **PlayStation friends:** The combined friends list now also shows PlayStation friends (badge “P”, online status, current game). This only works through Sony's unofficial interface with the NPSSO code, at your own risk; Sony can change it at any time. The code is stored DPAPI-encrypted; the feature is optional and off by default. Includes a how-to for the code, a connection check and linking via `psn:Name`.
- **Fix:** Sony authorization codes containing a dot (`v3.…`) are accepted; sign-in diagnostics are written to the log.

Companion/Android code unchanged; no new APK (1.0.2 is attached for convenience).

### Validation and limits

Release build and all automated suites (1357 core, 119 TMDB, 251 plugins, 41 examples, 127 store, 140 transport, 135 settings, 11 monitor, 361 companion) and both installer checks passed. Meanmachine upgraded from 2.9.2, configuration preserved byte-identically, COM7 connected, Redmi and TV show pages from the new version (screenshots). PlayStation live: with a fresh code the friends list returned one PlayStation account without errors. **Not verified:** online status/game of PlayStation friends on the devices, Pixel, physical finger/remote acceptance, store install/update through the UI.

Before upgrading, back up the EXE, configuration and Companion pairings. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`.