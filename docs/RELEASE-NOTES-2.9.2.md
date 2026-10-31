# ScreenBuddy 2.9.2

2026-10-31 · Windows 10/11 x64 · Companion 1.0.2 (unverändert).

## Deutsch

- **Designsysteme:** Cupertino, Fluent und Material sind eigenständige Profile für Formen, Typografie, Abstände und Rückmeldung, jeweils mit eigener Hell- und Dunkel-Palette. Der native Hell/Dunkel-Schalter bleibt wirksam; sechs Beispieldateien sind enthalten.
- **Bedienwege:** Cupertino mit einblendbarer Sidebar und optionalem Inspector, Fluent mit Seitensuche und Zurück-Verlauf, Material mit fünf Hauptzielen, Unterseiten und schwebender Speicheraktion bei Änderungen. Die Systemanzeige hat je Profil einen eigenen Aufbau; Rendering und Touchziele nutzen dieselben Regionen.
- **Themes:** optionales, streng validiertes `style`-Feld, Auswahl im Theme-Editor; vorhandene Themes werden anhand ihrer Namen zugeordnet.

Companion-/Android-Code unverändert; kein neues APK.

### Prüfstand und Grenzen

Build und alle automatisierten Suiten sowie zwei Installerprüfungen bestanden; zwölf DE/EN-Demos und sechs native Hell/Dunkel-Varianten visuell geprüft. Details in [TESTPROTOKOLL.md](../cpp/TESTPROTOKOLL.md). **Nicht geprüft:** Anzeige der neuen Profile auf Redmi, Pixel, Android-TV und am seriellen Display (Geräte- und Relay-Tests wurden nicht durchgeführt).

Vor einem Upgrade EXE, Konfiguration und Companion-Kopplungen sichern. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` abgleichen.

## English

- **Design systems:** Cupertino, Fluent and Material are distinct profiles for shapes, typography, spacing and feedback, each with its own light and dark palette. The native appearance switch keeps working; six example files are included.
- **Navigation:** Cupertino with collapsible sidebar and optional inspector, Fluent with page search and Back history, Material with five destinations, subpages and floating save actions for pending changes. System displays use a distinct composition per profile; rendering and touch targets share the same regions.
- **Themes:** optional strictly validated `style` field, selectable in the theme editor; existing themes are recognized by name.

Companion/Android code unchanged; no new APK.

### Validation and limits

Build, all automated suites and two installer checks passed; twelve DE/EN demos and six native light/dark variants checked visually. See [TESTPROTOKOLL.md](../cpp/TESTPROTOKOLL.md). **Not verified:** display of the new profiles on Redmi, Pixel, Android TV and the serial display (no device or relay tests were run).

Before upgrading, back up the EXE, configuration and Companion pairings. The Windows EXE is unsigned; compare it with `SHA256SUMS.txt`.