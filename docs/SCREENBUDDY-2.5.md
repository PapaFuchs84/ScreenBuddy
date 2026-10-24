# ScreenBuddy 2.5 – Entwicklungsstand

**Unveröffentlichter Entwicklungsbuild 2.5.0-dev.1.** Die letzte veröffentlichte Version ist 2.3.0.

## Implementiert

- EXE, Installer-Metadaten und Autostart-Eintrag verwenden ScreenBuddy. Der alte Registry-Uninstall-Schlüssel bleibt für Upgrades bestehen; die alte EXE wird als Rückfallkopie umbenannt.
- Native Win32-Einstellungen bieten Status, Vorschau, Basisoptionen und lokale Plugin-Verwaltung. Über **Erweiterte Einstellungen** bleibt die bisherige WebView2-Oberfläche für Datenquellen, Companion und weitere Optionen erreichbar.
- Deklarative Plugin-Manifeste (Schema 1) werden streng validiert. Ein lokaler Speicher importiert, deaktiviert und entfernt sie. Offline-Plugins ohne Quellen können als Seiten erscheinen. Quellenbasierte Plugins bleiben deaktiviert; Netzwerk-Transport, Host-Zustimmung und Plugin-Secrets-Verwaltung fehlen noch. Import führt keinen Code aus.

Ein Beispiel und die vorläufige Entwickler-Schnittstelle stehen unter [Plugin-System](PLUGIN-SYSTEM.md).

## Kompatibilität und offene Arbeit

Plugin-Manifeste ersetzen weder config.toml noch DPAPI-Zugangsdaten oder companion.json. Für eine stabile 2.5-Freigabe fehlen vollständige native Funktionsparität, sichere Quellenabfrage, UI- und Upgrade-Abnahme, Hardwaretest sowie ein Performancevergleich mit 2.3.0. Repository und Homepage sind noch nicht öffentlich umbenannt.

## Prüfstatus

Am 25.10.2026 wurde der Entwicklungsbuild lokal kompiliert. 236 Kernprüfungen, 220 Plugin-Prüfungen, 11 Monitorprüfungen, 335 Companion-Protokollprüfungen, 39 isolierte App-/Companion- und 49 Navigationsprüfungen sowie zwei Installer-Prüfungen bestanden. Eine isolierte `--no-lcd`-Instanz zeigte das native Einstellungsfenster. Der Installer-Test verwendete für den alten Namen eine Testkopie des neuen Builds. Ein produktives Upgrade, die native Bedienbarkeit bei verschiedenen DPI-Stufen und physische Ausgabe sind dadurch nicht belegt. [Testprotokoll](../cpp/TESTPROTOKOLL.md).
