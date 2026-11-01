# ScreenBuddy 2.9.5 – 1. November 2026

## Neues Menü

- **Ansicht:** Übersicht, Anzeige, Seiten & Reihenfolge und **Farben & Design**.
- **Seiten:** alle Datenquellen. **App:** Companion, Plugins, Plugin-Store, Updates & Autostart.
- Material behält fünf Hauptziele: Übersicht, Ansicht, Seiten, Erweiterungen, App; Farben & Design ist eine Unterseite von Ansicht. Plugins sind im eigenen Ziel Erweiterungen.

## Themes und Anleitung

Signierte Themes direkt unter **Ansicht → Farben & Design → Themes aus dem Store** installieren. Das neue Theme ist danach ausgewählt; **Speichern & anwenden** übernimmt es. Der gemeinsame Abrufschalter bleibt im Plugin-Store. Ein im Sichttest entdeckter Layoutfehler ist behoben: asynchron geladene Theme-Zeilen überdecken die Installationsknöpfe nicht mehr.

Die DE/EN-Anleitung enthält aktuelle, nach der Aufnahme anonymisierte Menübilder sowie Cupertino, Fluent und Material in Hell/Dunkel. Die englische Anleitung kennzeichnet die deutschen UI-Bilder. [Menü-Hilfe](NAVIGATION.md) · [English](NAVIGATION.en.md).

## Prüfung und Grenzen

- Release-Build, 14 Testsuiten (einschließlich 3327 Kernprüfungen), zwei Installerchecks; 39 App- und 17 Mehrgeräte-Integrationsprüfungen bestanden.
- Meanmachine-Upgrade mit privater Sicherung, Hashvergleich der EXE und erhaltener Konfiguration geprüft.
- Pixel 10 Pro, Redmi 8 und BeyondTV gleichzeitig verbunden, Bilder geprüft; Wischen auf beiden Handys und TV-Pfeiltaste wechseln jeweils genau eine Seite. Der Nutzer bestätigt die physische Wischbedienung am Pixel zusätzlich. Die zunächst zu frühe automatische Abfrage wurde durch Warten auf Navigationsmetadaten korrigiert.
- Neue Menügruppen DE/EN, sechs native Profil-/Darstellungsvarianten, direkte Store-Installation mit automatischer Auswahl und Persistenz nach Speichern/Neustart geprüft. HTML/JavaScript und lokale Dokumentationslinks geprüft; neuen Menüabschnitt DE/EN im Browser angesehen.
- USB-Display wird verbunden gemeldet und liefert Vorschau; keine neue Kamera-/Fingerabnahme des seriellen Panels, keine serielle Bildratenmessung. Keine physische TV-Fernbedienungs- oder Screenreader-Abnahme.
- Companion bleibt **1.0.3**, Android-Quellcode unverändert; vorhandene geprüfte APK beigefügt. Private Rohbilder, Zugangsdaten, Kopplungen und Logs sind nicht Teil des Releases.

## English

View now contains Colors & design. Pages groups data sources; App holds Companion, plugins, the plugin store and updates. Material keeps its separate Extensions destination. Install signed themes directly on Colors & design; installation selects the theme automatically, then Save & apply keeps it. Fixes overlapping theme-store rows after asynchronous loading. Updated bilingual illustrated guides include anonymized release-test captures and all three light/dark design profiles. Companion remains 1.0.3. Automated suites and three-device navigation passed; physical USB-panel, TV-remote and screen-reader acceptance remains limited as described above.
