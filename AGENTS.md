# Projektregeln

## Dokumentation gehört zu jeder Änderung

- Bei Funktionsänderungen, Fehlerkorrekturen und Releases die betroffene Dokumentation im selben Arbeitsgang aktualisieren, ohne zusätzliche Aufforderung. Das ist eine ausdrückliche Nutzerpräferenz.
- Vor Commit und Release README/Schnellstart, Referenz, Versionsangaben, Downloadlinks, Laufzeitvoraussetzungen, Bedienung, Fehlerbehebung und Prüfstatus auf Konsistenz prüfen. Vorhandene deutsche und englische Fassungen gemeinsam pflegen.
- Beide Haupt-Changelogs und bei C++-Änderungen `cpp/CHANGELOG.md` pflegen. Historische Release-Einträge und belegte frühere Tests nicht nachträglich auf eine neue Version umschreiben.
- Bei Änderungen an der bebilderten Anleitung die Vorlagen unter `anleitung/` und die zugehörigen HTML-Ressourcen unter `cpp/res/` synchron halten. Bereits veröffentlichte EXE-Dateien nicht allein für Dokumentationskorrekturen stillschweigend ersetzen; geänderte Ressourcen fließen in den nächsten App-Build ein.
- Vor Veröffentlichung lokale Links, Versions-/Release-Verweise und gegebenenfalls die HTML-/JavaScript-Syntax prüfen. Bestätigte Tests, Simulationen und offene physische Tests klar unterscheiden.
- Nie Konfigurationen, Zugangsdaten, Logs oder private Screenshots veröffentlichen. Anleitungsbilder nur mit Beispieldaten; bei reinen Textkorrekturen vorhandene geprüfte Bilder unverändert lassen.

Weitere Abläufe stehen in `CONTRIBUTING.md` und `CONTRIBUTING.en.md`.

## Windows-Auslieferungen signieren

- Ausdrückliche Nutzeranweisung: Öffentliche Windows-EXE-Auslieferungen ab jetzt immer mit vertrauenswürdiger Authenticode-Codesignierung und RFC-3161-Zeitstempel erstellen. Keine unsignierte EXE freigeben und kein selbstsigniertes Zertifikat als öffentliche Lösung verwenden.
- `packaging/windows/Export-WindowsRelease.ps1` signiert und prüft vor Kopie und SHA-256. Fehlt ein geeignetes Zertifikat/Signierdienst, bleibt die öffentliche EXE-Auslieferung gesperrt; lokale Entwicklungsbuilds sind weiterhin möglich.
- MSIX für den Microsoft Store wird nach Zertifizierung von Microsoft signiert. Lokale Testidentitäten niemals als Store-Einreichung ausgeben. SmartScreen-Warnungsfreiheit nicht versprechen: Reputation wird separat bewertet.
