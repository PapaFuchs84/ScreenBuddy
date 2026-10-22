# Projektregeln

## Dokumentation gehört zu jeder Änderung

- Bei Funktionsänderungen, Fehlerkorrekturen und Releases die betroffene Dokumentation im selben Arbeitsgang aktualisieren, ohne zusätzliche Aufforderung. Das ist eine ausdrückliche Nutzerpräferenz.
- Vor Commit und Release README/Schnellstart, Referenz, Versionsangaben, Downloadlinks, Laufzeitvoraussetzungen, Bedienung, Fehlerbehebung und Prüfstatus auf Konsistenz prüfen. Vorhandene deutsche und englische Fassungen gemeinsam pflegen.
- Beide Haupt-Changelogs und bei C++-Änderungen `cpp/CHANGELOG.md` pflegen. Historische Release-Einträge und belegte frühere Tests nicht nachträglich auf eine neue Version umschreiben.
- Bei Änderungen an der bebilderten Anleitung die Vorlagen unter `anleitung/` und die zugehörigen HTML-Ressourcen unter `cpp/res/` synchron halten. Bereits veröffentlichte EXE-Dateien nicht allein für Dokumentationskorrekturen stillschweigend ersetzen; geänderte Ressourcen fließen in den nächsten App-Build ein.
- Vor Veröffentlichung lokale Links, Versions-/Release-Verweise und gegebenenfalls die HTML-/JavaScript-Syntax prüfen. Bestätigte Tests, Simulationen und offene physische Tests klar unterscheiden.
- Nie Konfigurationen, Zugangsdaten, Logs oder private Screenshots veröffentlichen. Anleitungsbilder nur mit Beispieldaten; bei reinen Textkorrekturen vorhandene geprüfte Bilder unverändert lassen.

Weitere Abläufe stehen in `CONTRIBUTING.md` und `CONTRIBUTING.en.md`.
