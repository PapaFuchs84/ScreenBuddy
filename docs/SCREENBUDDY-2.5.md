# ScreenBuddy 2.5 – Entwicklungsstand

**Unveröffentlichter Entwicklungsbuild 2.5.0-dev.2.** Die letzte veröffentlichte Version ist 2.3.0.

## Implementiert

- EXE, Installer-Metadaten und Autostart-Eintrag verwenden ScreenBuddy. Der alte Registry-Uninstall-Schlüssel bleibt für Upgrades bestehen; die alte EXE wird als Rückfallkopie umbenannt.
- Das native Win32-Einstellungsfenster bietet sieben Reiter, darunter Quellen-Schema und -Editor, Reihenfolge der Display-/Companion-Seiten sowie App-Updates und Autostart. **Erweiterte Einstellungen** öffnet die WebView2-Abläufe für geführte Konto-/Auswahl-Workflows, benutzerdefinierte Farben sowie Companion-Kopplung und QR-Code.
- Deklarative Plugin-Manifeste (Schema 1) werden streng validiert. Der lokale Speicher kann Plugins importieren, aktivieren, deaktivieren und entfernen. Quellenbasierte Plugins lassen sich nach Eingabe ihrer Pflicht-Einstellungen und Zustimmung zu jedem deklarierten Host aktivieren. Der begrenzte HTTPS-Abruf läuft in einem eigenen Worker; Plugin-Einstellungen werden mit DPAPI geschützt. Import führt keinen Code aus.
- Datenabrufe laufen im eigenen Worker; das Rendern bleibt im Display-Thread. Es wurden keine externen HTTPS-Liveabfragen geprüft.

## Architektur und Rollout

Die native Win32-Hülle nutzt die vorhandene C++20-Implementierung für Rendering, USB und Companion weiter und vermeidet eine zusätzliche Electron-Laufzeit. WebView2 bleibt für geführte Workflows erhalten. In dev.2 wurde weder auf Flutter noch Tauri umgeschrieben. Vor einer Freigabe sind UI-, Upgrade-, Hardware- und externe-Dienst-Abnahmen erforderlich. Der Installer sichert eine Vorgänger-EXE als Rückfallkopie; ein echtes Rollback wurde noch nicht abgenommen. Öffentliche Repository- und Homepage-Umbenennung bleibt bis nach der Freigabe zurückgestellt.

Ein Beispiel und die vorläufige Entwickler-Schnittstelle stehen unter [Plugin-System](PLUGIN-SYSTEM.md).

## Kompatibilität und offene Arbeit

Plugin-Manifeste ersetzen weder config.toml noch vorhandene DPAPI-Zugangsdaten oder companion.json. Für eine stabile 2.5-Freigabe fehlen vollständige native Funktionsparität, Abnahme externer HTTPS-Abfragen, visuelle und interaktive UI-Abnahme, echtes Upgrade von 2.3.0, Hardwaretest sowie ein Performancevergleich mit 2.3.0. Repository und Homepage sind noch nicht öffentlich umbenannt.

## Prüfstatus

Am 25.10.2026 wurde der Entwicklungsbuild lokal kompiliert. Bestanden: 236 Kern-, 220 Manifest-Parser-, 127 Plugin-Speicher-, 133 Plugin-Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Protokollprüfungen sowie 39 isolierte App-, 49 Navigations- und zwei Installer-Prüfungen. Eine isolierte Instanz zeigte das native Fenster. Die Tests belegen keine externen HTTPS-Aufrufe, echte Display-Hardware, ein Upgrade von 2.3.0 oder visuelle/interaktive UI-Abnahme. [Testprotokoll](../cpp/TESTPROTOKOLL.md).
