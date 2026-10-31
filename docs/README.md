<div align="center">
<img src="logo/fuchslabs-logo.png" alt="FuchsLabs" width="72">

# ScreenBuddy · Dokumentationsübersicht

</div>

Stand: ScreenBuddy 2.9.4. Diese Seite ist der Einstieg in alle Dokumente. Sie ist nach Zielgruppe geordnet; jedes Dokument hat genau einen Zweck.

## Aufbau und Prinzipien

Die Struktur folgt den Vorgaben des Doku-Skills [document-specialist-skill](https://github.com/SpillwaveSolutions/document-specialist-skill) (übernommen wurden nur Methodik und Gliederung, kein Code):

- **Docs as Code:** Dokumentation wird im selben Arbeitsgang wie die Änderung aktualisiert (siehe [`AGENTS.md`](../AGENTS.md)).
- **Klein und aktuell statt groß und veraltet:** Das README bleibt ein Schaufenster; Details stehen in je einem Dokument pro Thema.
- **Zielgruppe bestimmt den Inhalt:** Anwender, Plugin-Autoren, Entwickler, Betreiber/Release.
- **Architektur nach arc42 (kompakt)**, Diagramme als Mermaid.
- **Sprachen:** README, Anleitung und Changelog zweisprachig (DE/EN); Fachdoku nur Deutsch.
- **Prüfstände ehrlich trennen:** automatisiert, simuliert, am Gerät beobachtet, offen.

## Für Anwender

| Dokument | Zweck | Status |
|---|---|---|
| [`README.md`](../README.md) · [English](../README.en.md) | Überblick, Schnellstart, Konfiguration | aktuell |
| [`ANLEITUNG.md`](../ANLEITUNG.md) | Schnellstart; die bebilderte Anleitung steckt in der EXE | Version, Menünamen und PlayStation-Angaben am 31.10.2026 gegen den Code geprüft (Bilder der bebilderten Anleitung teils 2.5) |
| [`FUNKTIONEN.md`](FUNKTIONEN.md) · [English](FEATURES.en.md) | Funktionen im Detail: Touch, Update-Hinweis, Home Assistant, Streaming, Spotify, PlayStation-Freunde, Farben & Design, Companion, Plugin-Store, Ansichten | aktuell (2.9.4) |
| [`COMPANION.md`](COMPANION.md) · [English](COMPANION.en.md) | Android-Companion einrichten, mehrere Geräte | aktuell |
| [`TOUCH-2.0.7.md`](TOUCH-2.0.7.md) · [English](TOUCH-2.0.7.en.md) | Touch-Ziele und Spielstunden | aktuell |
| [`../DOKUMENTATION.md`](../DOKUMENTATION.md) | Referenz: Seiten, `config.toml`, HWiNFO, Fehlerbehebung | Version und Menünamen am 31.10.2026 korrigiert; Prüfstandskapitel historisch |
| [`../TESTPHASE.md`](../TESTPHASE.md) · [English](../TESTPHASE.en.md) | Rückmeldungen in der Testphase | aktuell |

## Für Plugin-Autoren

| Dokument | Zweck |
|---|---|
| [`PLUGIN-SYSTEM.md`](PLUGIN-SYSTEM.md) · [English](PLUGIN-SYSTEM.en.md) | Konzept: deklarative Plugins |
| [`PLUGIN-TUTORIAL.md`](PLUGIN-TUTORIAL.md) · [English](PLUGIN-TUTORIAL.en.md) | Schritt für Schritt zum eigenen Plugin (die interaktive Plugin-Werkstatt steckt in der App) |
| [`PLUGIN-PAKETFORMAT.md`](PLUGIN-PAKETFORMAT.md) · [English](PLUGIN-PACKAGE-FORMAT.md) | Paketformat |
| [`PLUGIN-SIGNIERUNG.md`](PLUGIN-SIGNIERUNG.md) · [English](PLUGIN-SIGNING.md) | Signierung, Index, Sperrliste |
| [`PLUGIN-AUTOREN.md`](PLUGIN-AUTOREN.md) · [English](PLUGIN-AUTHORS.md) | Einreichen im Store |
| [`PLUGIN-STORE-BEDINGUNGEN.md`](PLUGIN-STORE-BEDINGUNGEN.md) · [English](PLUGIN-STORE-TERMS.md) | Store-Bedingungen (juristische Prüfung steht noch aus) |
| [`../cpp/examples/plugins/README.md`](../cpp/examples/plugins/README.md) | Beispielmanifeste |

## Für Entwickler

| Dokument | Zweck |
|---|---|
| [`ARCHITEKTUR.md`](ARCHITEKTUR.md) | Bausteine, Laufzeitablauf, Entscheidungen, Risiken (arc42 kompakt) |
| [`../cpp/README.md`](../cpp/README.md) | Build, Diagnose, C++-Details |
| [`COMPANION-PROTOKOLL.md`](COMPANION-PROTOKOLL.md) | Companion-Protokoll |
| [`../CONTRIBUTING.md`](../CONTRIBUTING.md) · [English](../CONTRIBUTING.en.md) | Mitarbeiten |
| [`../SECURITY.md`](../SECURITY.md) · [English](../SECURITY.en.md) | Sicherheitslücken melden |
| [`../AGENTS.md`](../AGENTS.md) | Projektregeln für Doku und Releases |
| [`../TRADEMARKS.md`](../TRADEMARKS.md), [`../THIRD-PARTY-NOTICES.md`](../THIRD-PARTY-NOTICES.md), [`../LICENSE`](../LICENSE) | Marke und Lizenzen |

## Für Release und Betrieb

| Dokument | Zweck | Status |
|---|---|---|
| [`../CHANGELOG.md`](../CHANGELOG.md) · [English](../CHANGELOG.en.md) · [`../cpp/CHANGELOG.md`](../cpp/CHANGELOG.md) | Änderungen je Version | aktuell |
| [`RELEASE-NOTES-2.9.4.md`](RELEASE-NOTES-2.9.4.md) | Release-Notizen der aktuellen Version | aktuell |
| [`RELEASE-NOTES-2.9.2.md`](RELEASE-NOTES-2.9.2.md), [`RELEASE-NOTES-2.9.1.md`](RELEASE-NOTES-2.9.1.md), [`RELEASE-NOTES-2.9.md`](RELEASE-NOTES-2.9.md) | Release-Notizen 2.9.x | historisch |
| [`THEMES.md`](THEMES.md) | Farbdesigns: Windows-Farben, Designsysteme, eigene Themes (DE/EN) | aktuell |
| [`RELEASE-NOTES-2.8.md`](RELEASE-NOTES-2.8.md) | Release-Notizen 2.8 | historisch |
| [`RELEASE-NOTES-2.7.md`](RELEASE-NOTES-2.7.md) | Release-Notizen 2.7 | historisch |
| [`../cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md) | Testprotokoll | aktuell, jüngster Eintrag „Release 2.9.4“ (31.10.2026) |
| [`PRUEFSTAND-HISTORIE.md`](PRUEFSTAND-HISTORIE.md) | Prüfstände 2.0.3 bis 2.6.0 (aus dem README ausgelagert) | historisch |
| [`RELEASE-NOTES-2.6.md`](RELEASE-NOTES-2.6.md), [`RELEASE-NOTES-2.5.md`](RELEASE-NOTES-2.5.md) | frühere Release-Notizen | historisch |
| [`RELEASE-CHECKLIST-2.5.md`](RELEASE-CHECKLIST-2.5.md), [`HARDWARE-ACCEPTANCE-2.5.md`](HARDWARE-ACCEPTANCE-2.5.md) | Checklisten (Stand 2.5; bei neuen Releases anlegen statt umschreiben) | historisch |
| [`SCREENBUDDY-2.6.md`](SCREENBUDDY-2.6.md), [`SCREENBUDDY-2.5.md`](SCREENBUDDY-2.5.md), [`DEVELOPMENT-NOTES-2.5.md`](DEVELOPMENT-NOTES-2.5.md) | Stand und Grenzen früherer Versionen | historisch |
| [`PLUGIN-PLATTFORM-PLAN.md`](PLUGIN-PLATTFORM-PLAN.md), [`../PROTOKOLL.md`](../PROTOKOLL.md), [`../UEBERGABE-CODEX-2.5.md`](../UEBERGABE-CODEX-2.5.md), [`../SICHERHEITSREVIEW.md`](../SICHERHEITSREVIEW.md) | Planung, Verlauf, Übergabe, Python-1.x-Review | historisch |

## Pflegeregeln

1. Neue Funktion oder Fehlerkorrektur: betroffenes Fachdokument, README (DE/EN), Changelogs, ggf. Anleitung im selben Commit aktualisieren.
2. Neues Release: neue Release-Notizen anlegen, ältere nie auf die neue Version umschreiben; Prüfstand des Vorgängers nach [`PRUEFSTAND-HISTORIE.md`](PRUEFSTAND-HISTORIE.md) verschieben.
3. Prüfaussagen immer in vier Klassen einordnen: automatisiert · simuliert · am Gerät beobachtet · offen.
4. Keine Konfigurationen, Zugangsdaten, Logs oder privaten Screenshots; Bilder nur mit Beispieldaten.
5. Das Wiki wird erst nach ausdrücklicher Freigabe von Basti angepasst.

## Bekannte Lücken dieser Neuordnung

- [`../cpp/README.md`](../cpp/README.md) enthält noch gewachsene, teils englische Abschnitte (2.0.x bis 2.7) und Altstände; die Versions-, Menü- und WebView2-Angaben wurden am 31.10.2026 korrigiert, historische Abschnitte sind als solche gekennzeichnet.
- `DOKUMENTATION.md` wurde nicht umgeschrieben; am 31.10.2026 wurden nur Version, Menünamen und WebView2-Angaben korrigiert. Die Prüfstandskapitel bleiben historisch.
- Offen nach der Prüfung vom 31.10.2026: `TESTPHASE.md`/`TESTPHASE.en.md` nennen nur „Version 2.0“ für den Logpfad. Die Unterstützungstabelle in `SECURITY.md`/`SECURITY.en.md` nennt 2.9.3 als aktuell; dass ältere 2.x „auf 2.9.3 aktualisieren“ sollen, ist eine Festlegung des Maintainers. Nicht gegen den Code belegt sind „Statuszeile aktualisiert sich alle 5 Sekunden“ (`DOKUMENTATION.md`, Kapitel 3) und der frühere Deinstallationsname „FuchsLabs USB-Display“. `cpp/TESTPROTOKOLL.md` hat keinen Eintrag für 2.7.0.
- Die bebilderte Anleitung (`anleitung/`, `cpp/res/`): Texte am 31.10.2026 gegen die Beschriftungen im Code geprüft und korrigiert (inkl. PlayStation); die Bilder blieben unverändert, ein Teil stammt noch von 2.5 und zeigt die PlayStation-Karte nicht.
- Das GitHub-Wiki (`Home.md`) steht auf 2.7.0; seine Links auf diese Übersicht funktionieren erst nach dem Merge dieses Branches nach `main`.
