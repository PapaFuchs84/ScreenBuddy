> P8-4 integration (2026-11-07): current Custom views tiles, complete suite counts and the test-UI port collision are documented in [Phase 8](../docs/PHASE8-EIGENE-ANSICHTEN.md). The report below records the earlier KI-Status branch work. / Aktueller Integrationsprüfstand in Phase 8; nachfolgend historischer Branchbericht.

# KI-Status – Abschlussbericht

Stand: 2026-11-07. Lokaler Entwicklungsstand; kein Commit, Push oder Release.

Implementiert: optionale DE/EN-Seite `ai` für 480 × 320 mit zwei Statuskarten, Zustandsdauer, Tool/Modell, Kontext/Tokens, Limitbalken und Reset-Countdown, gemeldeter Claude-Kostenschätzung sowie lokalem Datenalter. Sitzungen werden waiting > working > idle sortiert; Puls, weiches Blinken, Ein-/Ausblendung und Positionswechsel laufen im vorhandenen Render-Takt. Der dunkle Orange-Standard wird unterstützt; unbekannte Werte bleiben n/a.

Der eigene Empfänger bindet ausschließlich an 127.0.0.1:18765, verwendet einen zufälligen 256-Bit-Schlüssel mit vorhandener DPAPI-Speicherung und verweigert den Produktionsstart bei Secret-Fehlern. Kein Klartext-Fallback, keine Schlüssel in Befehlen/Logs, kein CORS. Hook-Einrichtung/-Entfernung bietet schreibgeschützte Vorher-/Nachher-Vorschau ausschließlich der eigenen Hook-Einträge (keine env-Werte, fremden Befehle oder sonstigen Einstellungen), einmalige .bak-Sicherung, Konfliktprüfung und atomare Ersetzung je Datei. Fremde Hook-Befehle bleiben erhalten.

Die reservierte lokale Quelle `ai.claude.*` / `ai.codex.*` liefert state, tool, model, state_seconds, context, five_hour, week, five_hour_reset, week_reset, cost_today, tokens_today und s1..s3 an deklarative Plugin-/Zonen-Templates. Unbekannte Messwerte sind null; direkte AI-Referenzen in Links sind gesperrt. Eigenständiger Code ohne libre-panel-/GPL-Implementierung.

## Dateien

Neu:

- `src/ai_status.h/.cpp`: Modell, Parser, Hooks, DPAPI-Adapter und HTTP.
- `src/render_ai.cpp`, `src/ui_page_ai.cpp`: Display und native Einstellungen.
- `tests/ai_status_tests.cpp`: Prüfungen im bestehenden fl-tests-Target.
- `AI-STATUS.md`, `AI-STATUS.en.md`, dieser Bericht.

Angepasst:

- `CMakeLists.txt`, `src/render.h`, `src/config.cpp`, `src/app.h/.cpp`, `src/builtin_catalog.cpp`: Build, Registrierung, Lebenszyklus und Kachelquelle.
- `src/ui_pages.h/.cpp`, `src/ui_kit.cpp`, `src/i18n.cpp`, `src/plugins.cpp`: Seitenauswahl, DE/EN und reservierte lokale Datenquelle.
- `tests/tests.cpp`: Testaufruf und Seitenanzahlprüfungen.
- C++-README/Changelog sowie beide Haupt-READMEs und Haupt-Changelogs: Entwicklungsstatus und DE/EN-Dokumentationsverweise.

Unverändert: `ui_editor.*`, `layout_edit.*`, `ui_layout_grid.*`, `ui_tile_palette.*`, `ui_page_layout.*`, vorhandene Screenshot-Baselines, Guide-Ressourcen, Versionsnummern und veröffentlichte EXE-Dateien. Die zuvor vorhandene unversionierte Datei `../codex-ai-status-prompt.md` blieb unangetastet.

## Korrekturlauf vom 2026-11-07

- Seitenzahltests erwarten fest 9 Standardseiten bzw. 10 mit zusätzlicher Plugin-Seite.
- `ai` ist nicht im Builtin-/Layout-Katalog enthalten, solange ein Builtin-Renderpfad für Anordnungen fehlt. Die feste KI-Seite und die Template-Datenquelle bleiben vorhanden. Katalogtest prüft die Abwesenheit explizit.
- Temp-Dateien werden mit `symlink_status` einschließlich dangling Symlinks geprüft. Alte reguläre `.screenbuddy-ai.tmp` werden entfernt; Links und andere Dateitypen werden abgewiesen. `CREATE_NEW` verhindert das Schreiben durch zwischenzeitlich angelegte Dateien/Links. Schreib-, Flush- und Ersetzungsfehler räumen die selbst angelegte Temp-Datei auf.
- Ausblendende Zeilen behalten ihre letzte Nummer, beginnend mit 1. Kein Rückfall auf Ordinal 0.
- Die Hook-Vorschau enthält ausschließlich bekannte Ereignisnamen und exakt eigene Hook-Befehle vor/nach der Änderung. Vollständige Originalbytes dienen intern weiterhin der Erhaltung und Konfliktprüfung; Vorschau bei erneutem Laden zunächst geleert.
- KI-Tests enthalten keine ausgeschriebene gesperrte Portnummer und keinen festen HTTP-Testport mehr. Ablehnung wird über die Produktionskonstante geprüft; der Empfänger wählt einen freien Port aus 49152–65535. Exklusive Windows-Sockets verhindern die gemeinsame Belegung eines Ports. Testverzeichnisse bleiben über eine reservierte Datei bis zum Ende des Laufs eindeutig.

## Ausgeführte Prüfungen (aktueller Korrekturstand)

| Prüfung | Ergebnis |
| --- | --- |
| Release-Entwicklungsbuild: ScreenBuddy, fl-tests, fl-plugin-tests, fl-builtin-catalog-tests | erfolgreich; vorhandene C4996-Warnung zu `getenv` in tests.cpp beim Neubau |
| `fl-tests ai-status` | 242 bestanden, 0 fehlgeschlagen; positiver DPAPI-Roundtrip explizit SKIP |
| `fl-tests ai-status cpp/build/ai-previews` vom Repository-Root | 244 bestanden, 0 fehlgeschlagen; zwei zusätzliche Beispieldaten-Ingests, DE/EN- und Ausblendbilder erzeugt |
| Parallelbetrieb beider vorgenannten KI-Läufe | beide erfolgreich; getrennte freie HTTP-Ports und reservierte Temp-Verzeichnisse |
| HTTP mit synthetischem Testschlüssel, dynamischer Port | 403 ohne/falscher Schlüssel oder mit Origin; 204 gültig; 400 ungültiges JSON; 413 übergroßer Body; kein erfolgreicher OPTIONS-Preflight |
| Neue Hook-Regressionsprüfungen | synthetischer env-Secret und fremder Befehl fehlen in Vorschau; eigene Befehle enthalten; alte reguläre Temp-Datei ersetzt; gesperrte Zieldatei führt zu Fehler, Temp-Aufräumung und erfolgreichem erneuten Apply; dangling Symlink abgewiesen und Ziel unangetastet (kein SKIP) |
| Renderprüfung | Animation und vollständiges Entfernen ausblendender Zeilen automatisch geprüft; `fading.png` visuell geprüft: Codex 1, Claude 2, Claude 3, keine Nummer 0 |
| fl-plugin-tests / fl-builtin-catalog-tests | 380 / 235 bestanden, jeweils 0 fehlgeschlagen; weniger Katalogprüfungen durch entfernten AI-Katalogeintrag |
| vollständiges fl-tests | KI-Teil 242/0, danach weiterhin Abbruch im bestehenden Konfigurationstest: „Verschlüsselung fehlgeschlagen“; Prozesscode -1073740791; kein erfolgreicher Gesamtlauf |
| git diff --check / geänderte Dokumentationslinks | erfolgreich / 48 lokale Links geprüft, 0 defekt |

Zwischenläufe deckten zwei Fehler in neuen Test-Fixtures auf (JSON-Escaping bei der Befehlsprüfung und ungültiges JSON vor der Symlink-Prüfung); beide korrigiert. Der erste parallele Lauf zeigte außerdem eine gemeinsame Portbelegung durch Windows-Socket-Wiederverwendung, ein weiterer eine Kollision beim Anlegen der Testverzeichnisse. Exklusive Sockets, prozessabhängige Portwahl und durchgehend reservierte Temp-Namen beheben dies; der abschließende parallele Wiederholungslauf bestand vollständig.

Windows-DPAPI kann in dieser Ausführungsumgebung keine Daten schützen. Der positive Roundtrip ist deshalb kein bestätigter Test. Geprüft wurden abgewiesene Produktionsstarts sowie abgewiesene Klartext-Schlüsseldateien. HTTP-Authentifizierung wurde separat mit einem flüchtigen synthetischen Schlüssel getestet. Die bestehende Gesamtsuite wurde nicht abgeschwächt.

Alle KI-Dateitests verwenden eindeutige temporäre Verzeichnisse. Keine echten Claude-/Codex-Hook-Dateien wurden geschrieben, kein Empfänger am gesperrten Einstellungsport gebunden, keine echte App-Konfiguration gestartet und keine Hardware/Kopplung verändert. Der Ablehnungstest verwendet die Produktionskonstante und wird vor Bindung abgewiesen. Der lokale Build ist keine signierte öffentliche Auslieferung. Kein Commit oder Push.

## Nicht umgesetzt / offen

- Keine automatische Konto-/Limit-, Kontext- oder Tokenabfrage aus privaten APIs/Transkripten; keine Preislisten-basierte Kostenberechnung. Der Standardadapter überträgt nur Ereignis, Sitzung, Toolname und gegebenenfalls Modell. Weitere Messwerte können autorisierte lokale Produzenten melden; sonst n/a.
- Keine neue Editor-Palette oder freie Anordnung der KI-Standardseite. Die Template-Datenquelle ist vorhanden; Editor-Dateien blieben ausdrücklich unverändert.
- Ein tatsächlicher WriteFile-/FlushFileBuffers-Fehler (z. B. voller Datenträger) wurde nicht injiziert; der gemeinsame Cleanup-Pfad ist über die fehlgeschlagene Ersetzung geprüft. Native Einstellungsoberfläche nicht interaktiv abgenommen.
- Positiver DPAPI-/PowerShell-End-to-End-Test, echte Claude-Code-/Codex-Hooks, Client-Vertrauen/Ausführungsrichtlinie, physisches Display und Mehrgeräteabnahme offen.
- Keine Sitzungspersistenz oder erfundene Crash-/Heartbeat-Erkennung. Ohne End-Hook bleibt der letzte Zustand mit seinem Alter sichtbar.
- Hook-Dateien sind einzeln atomar, nicht gemeinsam transaktional. Adapter, verschlüsselter Schlüssel, Backups und gegebenenfalls leere Ereignislisten bleiben nach Entfernung lokal erhalten.

Anleitung und Protokoll: [DE](AI-STATUS.md), [EN](AI-STATUS.en.md). Herstellerreferenzen: [Anthropic Hooks](https://code.claude.com/docs/en/hooks), [OpenAI Docs Hooks](https://learn.chatgpt.com/docs/hooks).

## Phase 2 (Stand 07.11.2026, Codex-Guthaben leer, von Claude nachgezogen)
- Umgesetzt (Code): Nutzungsabfrage `ai_usage.cpp`, Anzeige/Schalter in `ui_page_ai.cpp`, Adapter-Skript `tests/ai_adapter_tests.ps1`.
- Tests (Lauf im vorhandenen Build, ninja "no work to do", also keine frische Neukompilation belegt): fl-tests ai-status 284/0, ai-cli-test 11/0, Katalog 279/0, Plugins 380/0, Adapter 15/0.
- Offen/ungeprüft: echte CLI (`ai-cli-probe`), echte Hooks, Hardware, Gesamtsuite (DPAPI-Fehler), Editor-Kachel (Editor-Dateien gehören anderem Thread; nur Datenseite vorbereitet).
- Die Doku (AI-STATUS*.md) wurde in Phase 2 nicht aktualisiert.
