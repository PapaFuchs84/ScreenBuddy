## Historische Entwicklungsnotizen vor 2.5.0

Die folgenden datierten Notizen beschreiben ihren damaligen Prüfstand; die obige Release-Abnahme ersetzt frühere offene Punkte nur im ausdrücklich geprüften Umfang.

- 27.10.2026: Companion 0.2.1-preview (versionCode 3) mit externer privater Release-Signierung vorbereitet; gültige APK-v2-Signatur und 20 Protokolltests geprüft. Andere Signatur als die bisherige Debug-APK; Geräteabnahme und Veröffentlichung offen.

- 27.10.2026: Die frühere HTML-Einstellungsseite wird weder eingebettet noch unter `/` ausgeliefert. Die bebilderte DE/EN-Anleitung zeigt jetzt vollständige anonymisierte native Aufnahmen; die englische Fassung kennzeichnet deren deutsche Beschriftung. Neuer Build und isolierte Prüfungen einschließlich 51 API-/Navigationschecks bestanden. Die visuelle Browser-Abnahme der Anleitung bleibt offen.

### Architektur
- Native Win32-Hülle nutzt den vorhandenen C++20-Kern für Rendering, USB und Companion; dadurch ist keine zusätzliche Electron-Laufzeit nötig. Die aktuelle Quellfassung bindet WebView2 nicht mehr ein; der lokale HTTP-Dienst für Companion/API und die eingebettete Anleitung bleibt erhalten. Kein Flutter-/Tauri-Umbau in dev.2. Release-Gates sind UI-, Upgrade-, Hardware- und externe-Dienst-Abnahme. Der Installer sichert eine Vorgänger-EXE; ein echtes Rollback ist noch nicht abgenommen. Öffentliche Umbenennung von Repository und Homepage ist bis nach der Freigabe aufgeschoben.

### Implementiert im unveröffentlichten Entwicklungsbuild
- ScreenBuddy-Branding für EXE, Installer und Autostart; bestehender Uninstall-Schlüssel für Upgrades erhalten.
- Selbst gezeichnetes natives Einstellungsfenster (Direct2D/DirectWrite) mit gruppierter Navigation, Live-Vorschau, Hell/Dunkel, DE/EN-Wechsel, Tastaturbedienung, Sprachausgabenamen, Änderungszähler, „Verwerfen“ und „Speichern & anwenden“. Phase 2 ergänzt native Seiten für Discord, Freunde, Home Assistant, Companion-Kopplung/-Geräte und Farben & Design. Weitere vorhandene Seiten und Plugin-Verwaltung bleiben verfügbar; vollständige Funktionsparität zur bisherigen WebView2-Oberfläche ist nicht behauptet. Neue Einstellung `ui.settings_theme` (auto/hell/dunkel).
- Native Übersicht mit drei direkten Einrichtungsschritten; native Display-Seite mit Windows-Touch-Zuordnungsstatus, Assistentenstart und erneuter Prüfung. Physische Touch-Abnahme steht aus.
- Einstellungen öffnen ausschließlich nativ; der aktive WebView2-Fallback und seine Linker-Abhängigkeit wurden entfernt. Ein Startfehler des nativen Fensters wird protokolliert und beim Öffnungsversuch angezeigt. Die frühere HTML-Einstellungsseite wird nicht mehr in die EXE eingebettet oder unter `/` ausgeliefert; ihre Quelldatei bleibt für historische Tests erhalten. Neuer Build und nativer Smoke-Test bestanden am 26.10.2026.
- Native Monitorauswahl mit automatischer Wahl, erkannten Bildschirmen und erneuter Suche; gespeicherte, getrennte Monitor-IDs bleiben erhalten.
- Die native Update-Seite kann bei gemeldetem neueren Release dessen geprüfte GitHub-Seite im Standardbrowser öffnen; ohne gültige Adresse ist der Knopf deaktiviert. Kein automatischer Download und keine Installation.
- Streng validierte Plugin-Manifeste, lokale Verwaltung, begrenzter HTTPS-Abruf im Worker, gespeicherte Host-Zustimmungen, DPAPI-geschützte Plugin-Einstellungen und Live-Rendering im Display-Thread. Quellen-Plugins sind nach Pflicht-Einstellungen und Einzelzustimmung pro Host aktivierbar; ein öffentlicher HTTPS-Abruf bestand isoliert, der vollständige App-Ablauf bleibt offen.
- Plugin-Beispiele: Offline-Uhr, konfigurierbares Fokus-Board und optionale öffentliche Wetterquelle. Deutsche und englische Anleitung erklären Import, Einstellungen, Host-Freigabe, Beispieldaten und Grenzen. 41 Beispielprüfungen bestanden; sie verwenden nur Beispieldaten und prüfen keine Hardware. Die öffentliche Wetterquelle wurde danach separat einmal live abgerufen. [Beispiele](../cpp/examples/plugins/README.md) · [Anleitung](../docs/PLUGIN-TUTORIAL.md) · [English tutorial](../docs/PLUGIN-TUTORIAL.en.md).
- Details zum Entwicklungsstand und zu den Kompatibilitätsgrenzen: [ScreenBuddy 2.5](../docs/SCREENBUDDY-2.5.md).

### Behoben im Entwicklungsbuild
- Die Plugin-DNS-Auflösung über `GetAddrInfoExW` nutzt den Abschluss-Callback jetzt mit separatem Warteereignis. Zuvor führte ein gesetztes `OVERLAPPED.hEvent` zusammen mit dem Callback auf Windows zu `WSAEINVAL` und verhinderte Live-HTTPS-Abfragen. Ein lokaler WinSock-Regressionstest und ein isolierter Abruf der öffentlichen Open-Meteo-Beispielquelle bestanden; der spätere Gesamtbuild und 134 Transportprüfungen bestanden ebenfalls.
- Der mDNS-Status meldet Erfolg erst nach erfolgreicher Registrierung. Registrierung und Abmeldung werden getrennt protokolliert. Der vollständige Build und die Tests bestanden nach dem Fix; ein eigener Mock-Test für den Fehler-Callback fehlt noch.

### Sicherheitshärtung nach Review
- Gespeicherte Plugin-Secrets werden verworfen, wenn ein Manifest-Update Quelle (Host, URL oder Header) ändert.
- Windows-Gerätenamen wie `con`, `nul`, `com1`–`com9` und `lpt1`–`lpt9` werden als Plugin-ID abgelehnt.
- Plugin-Secrets nutzen eine eigene DPAPI-Entropie; vorhandene Werte werden automatisch migriert.
- Bidi- und Zero-Width-Zeichen in Manifesttexten werden abgelehnt.
- `.` und `..` werden als Einstellungswert im URL-Pfad abgelehnt.
- Der DNS-Abschluss ist gegen Doppelfreigabe abgesichert.
- Offen: Zertifikats-Sperrprüfung (CRL/OCSP) fehlt noch; vollständiger App-Ablauf mit Host-Freigabe und externen HTTPS-Quellen ist nicht abgenommen.

### Status
- Kein 2.5-Release oder Download; 2.3.0 bleibt veröffentlicht. Der Build vom 26.10.2026 bestand 272 Kern-, 223 Parser-, 127 Speicher-, 134 Transport-, 135 Plugin-Einstellungs-, 41 Beispiel-, 11 Monitor- und 335 Companion-Prüfungen sowie 27 native UI-, 16 Maus-, 16 Companion-Rebuild-, 39 App-, 49 Navigations- und zwei Installer-Prüfungen. Englische Demobilder sind lokalisiert. Visuelle/interaktive UI-Abnahme, finales Zielsystem-Upgrade und Abnahme des finalen Builds bleiben offen; der Nutzer meldete den früheren Bildfehler als behoben.
