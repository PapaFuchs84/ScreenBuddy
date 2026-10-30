# ScreenBuddy 2.6 – Stand und Grenzen

**ScreenBuddy 2.6.0 – 30.10.2026.** Plugin-Store in der App mit signiertem Index und signierter Sperrliste, acht Start-Plugins und Companion-App 1.0 als separatem Download. Die Release-Notizen mit Prüfgrenzen stehen in [RELEASE-NOTES-2.6.md](RELEASE-NOTES-2.6.md).

## Neu gegenüber 2.5

- **Plugin-Store** als Einstellungsseite (neben Companion und Plugins): Suche, Kategorien, Karte mit Autor, Version, Lizenz und Status. Vor der Installation zeigt die Karte, an welche Hosts ein Plugin Daten senden würde.
- **Installieren und Aktualisieren** mit Bestätigung. Installierte Plugins bleiben **aus**; sie werden unter **Plugins** aktiviert. Jeder Host braucht eine Freigabe. Bei einem Update werden neue Hosts angezeigt und alle Freigaben zurückgesetzt.
- **Signierung:** Index und Sperrliste sind Ed25519-signiert; der öffentliche Schlüssel ist fest in der App. Die App prüft Signatur, Rollback-Schutz (`sequence`) und die Paketprüfsumme. Details: [Plugin-Signierung](PLUGIN-SIGNIERUNG.md).
- **Abgleich:** höchstens einmal täglich im Hintergrund, manuell über **Store aktualisieren**. Gesperrte Plugins werden automatisch ausgeschaltet. Ohne Netz gilt der zuletzt geprüfte Zwischenspeicher.
- **Schalter „Plugin-Store verwenden“** (`[ui] plugin_store`, Standard an). Aus bedeutet: kein Abruf.
- **Start-Plugins:** acht Plugins im Store-Repository `PapaFuchs84/ScreenBuddy-Plugins`.
- **Texte:** [Nutzungsbedingungen, Regeln und Datenschutz](PLUGIN-STORE-BEDINGUNGEN.md) (Entwurf, juristisch nicht geprüft) und [Autorenleitfaden](PLUGIN-AUTOREN.md) für Einreichungen.
- **Companion-App 1.0** als separater Download. Einrichtung und Grenzen: [COMPANION.md](COMPANION.md).

## Unverändert wichtig

- Einstellungen sind nativ (Direct2D/DirectWrite); WebView2 wird nicht benötigt.
- Plugins sind Datendateien ohne Programmcode. Import führt keinen Code aus.
- Online-Quellen brauchen die Zustimmung je Host. Zugangsdaten bleiben per DPAPI geschützt.
- Die Windows-EXE ist unsigniert. Prüfung über `SHA256SUMS.txt`.

## Grenzen

- Eine gültige Signatur zeigt nur, dass der Maintainer den Index veröffentlicht hat. Sie ist keine Garantie, dass ein Plugin ungefährlich ist. Die Freigabe bleibt eine menschliche Prüfung.
- Es gibt genau einen Signaturschlüssel. Eine Rotation mit mehreren Schlüsseln ist noch nicht umgesetzt.
- Eine alte, korrekt signierte Version bleibt wirksam, bis sie gesperrt wird.
- Die Store-Installation und das Store-Update in einer isolierten Instanz sind in den Release-Notizen gesondert ausgewiesen (Installation des Plugins „Wetter“ aus dem Live-Store bestanden (Plugin bleibt aus, kein Host zugestimmt); Schalter „Plugin-Store verwenden“ speichern und Neustart bestanden; Update nur als Simulation (Version lokal auf 0.9.0 gesetzt) bestanden, ein echtes Live-Update war mangels zweiter Version nicht prüfbar; Suchfeld nicht automatisiert prüfbar. Dabei gefundene Fehler (Plugin-Dateien unter GitHub Pages nicht ausgeliefert, Schalter nicht speicherbar) sind behoben).
- CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf für Online-Plugins sind nicht abgenommen.
- Offen bleiben Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus, DPI-/Screenreader-Prüfung und mehrere Android-Geräte.
- Die Companion-Geräteprüfung vom 27.10.2026 bezieht sich auf 0.2.1-preview. Eine eigene Geräteprüfung für 1.0 ist im Testprotokoll nicht dokumentiert.

## Prüfstand

Automatisierte Prüfzahlen: 1.312 Windows-Prüfungen und 2 Installer-Prüfungen bestanden, Android-Protokolltests bestanden. Die Gesamtübersicht steht im [Testprotokoll](../cpp/TESTPROTOKOLL.md). Historische Entwicklungsnotizen: [Archiv 2.5](DEVELOPMENT-NOTES-2.5.md).

## Dokumente

- [Release-Notizen 2.6.0](RELEASE-NOTES-2.6.md)
- [Plugin-Signierung](PLUGIN-SIGNIERUNG.md) · [Plugin-Signing (EN)](PLUGIN-SIGNING.md)
- [Plugin-Store: Bedingungen](PLUGIN-STORE-BEDINGUNGEN.md) · [Store terms (EN)](PLUGIN-STORE-TERMS.md)
- [Plugins einreichen](PLUGIN-AUTOREN.md) · [Author guide (EN)](PLUGIN-AUTHORS.md)
- [Plugin-System](PLUGIN-SYSTEM.md) · [Tutorial](PLUGIN-TUTORIAL.md)
