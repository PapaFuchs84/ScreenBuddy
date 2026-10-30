# ScreenBuddy 2.7.0

31.10.2026 · Windows 10/11 x64 · Companion 1.0.0 (Android 8+, unverändert)

## Deutsch

- Mehrere Companions gleichzeitig: synchron oder unabhängig, mit eigener Auflösung, Wischbedienung und optional eigener Seitenliste pro Gerät.
- Aktivierte Plugins in **Seiten & Reihenfolge** sortieren und ein-/ausblenden.
- Signierte Vorschaubilder im Plugin-Store und interaktive **Plugin-Werkstatt** in Deutsch/Englisch.
- Fehlerkorrektur: Store-Installationen und Updates bleiben jetzt auch ohne Netzwerkquellen ausgeschaltet, bis das Plugin ausdrücklich aktiviert wird. Zuvor wurde „Drei Aufgaben“ trotz gegenteiliger Dialogmeldung sofort aktiv. Der lokale direkte Manifestimport behält sein bisheriges Verhalten.

### Prüfung

- Release-Build: 279 Kern-, 251 Parser-, 41 Beispiel-, 83 Signatur-, 65 Katalog-, 127 Store-, 140 Transport-, 135 Einstellungs-, 11 Monitor- und 361 Companion-Prüfungen bestanden (1.493 insgesamt).
- Zusätzlich: 2 isolierte Installer-Erhaltungsprüfungen; 39 App-/Companion-, 17 simulierte Mehrgeräte-, 51 Navigations-/API-, 30 native Einstellungs-, 16 Maus- und 16 Companion-Neuaufbauprüfungen bestanden.
- Echtsystem: Pixel 10 Pro und Redmi 8 mit Companion 1.0.0 / Code 4. Unterschiedliche eigene Seitenlisten, getrennte injizierte Wischgesten und automatischer Wechsel über zwei Intervalle geprüft. Hochformat, Querformat und Wiederverbindung ohne neue Kopplung beobachtet. Sync-Navigation von beiden Geräten geprüft. Store-Vorschaubild und Plugin-Seite in der laufenden App sichtbar; Sortieren und Ausblenden geprüft.
- USB-Display: App meldet verbunden, 480 × 320. Der Nutzer bestätigt sauberes physisches Bild und Seitenwechsel; keine eigene Kamerabeobachtung.

### Grenzen und Upgrade

Die automatisierten Mehrgeräteprüfungen sind Simulationen; adb-Gesten ersetzen keine physische Fingerprüfung. Keine vollständige DPI-/Screenreader-, Windows-Anmelde-, WLAN- oder Launcher-/Boot-Abnahme. Kein echtes Store-Update zwischen zwei veröffentlichten Plugin-Versionen verfügbar. Online-Plugins und CRL/OCSP-Prüfungen bleiben wie in 2.6 experimentell. Home Assistant wurde nicht umkonfiguriert und es wurden keine Schaltaktionen ausgelöst.

Vor dem Upgrade EXE, `config.toml` und `companion.json` lokal sichern. Besonders bei aus paketierten Werkzeugen gestarteten Apps den tatsächlich verwendeten Dateikontext prüfen: die Windows-Paketumleitung kann einen anderen Einstellungsstand sichtbar machen. Der isolierte Installer-Erhaltungstest besteht; der Echtsystem-Wechsel erforderte den Abgleich der Anzeige-/Companion-Werte. Keine pauschale Zusicherung eines verlustfreien Wechsels zwischen diesen Kontexten.

Die Windows-EXE ist unsigniert. Downloads mit `SHA256SUMS.txt` abgleichen. Die unveränderte signierte Companion-APK 1.0.0 wird separat mitgeliefert. Rückweg: gesicherte EXE und zugehörige Einstellungen gemeinsam wiederherstellen. Niemals automatisch die Android-App deinstallieren oder Kopplungen löschen.

## English

- Multiple Companions at once, synchronised or independent, with device-specific resolution, swipes and optional page lists.
- Sort enabled plugins in **Pages & order** and control their visibility.
- Signed store preview images and the interactive German/English **Plugin Workshop**.
- Fix: store installations and updates now keep offline plugins disabled until explicitly enabled. Previously Three tasks became active immediately despite the confirmation dialog. Direct local manifest imports retain their previous behavior.

### Validation

Release build: 1,493 checks passed (279 core, 251 parser, 41 examples, 83 signing, 65 catalog, 127 store, 140 transport, 135 settings, 11 monitor, 361 Companion). Additional checks: 2 isolated installer preservation, 39 app/Companion, 17 simulated multi-device, 51 navigation/API, 30 native settings, 16 mouse and 16 Companion rebuild checks passed.

Pixel 10 Pro and Redmi 8, Companion 1.0.0 / code 4: different device lists, independent injected swipes, automatic switching over two intervals, portrait/landscape and reconnection without pairing were observed. Shared navigation was exercised from both devices. In-app previews, plugin output, ordering and visibility were checked. The user confirmed clean physical USB-display output; this was not a camera observation.

### Limits and upgrade

Simulated tests and adb gestures do not replace physical finger acceptance. Full DPI/screen-reader, Windows sign-in, Wi-Fi and launcher/boot acceptance remain outside this run. A real store update between two published versions was unavailable. Online plugins and CRL/OCSP checks retain the 2.6 experimental status. No Home Assistant settings or switch actions were changed.

Back up the EXE, `config.toml` and `companion.json`. Windows package filesystem redirection can expose a different settings context when an app is launched from packaged tools. The isolated installer tests pass; this real-system transition required reconciling display/Companion settings. Preservation across these contexts is not generally guaranteed.

The Windows EXE is unsigned: verify downloads against `SHA256SUMS.txt`. The unchanged signed Companion 1.0.0 APK remains a separate asset. Roll back the saved EXE and corresponding settings together. Never automatically uninstall Android or discard pairings.
