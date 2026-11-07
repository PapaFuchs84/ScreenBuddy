> P8-4 integration (2026-11-07): current Custom views tiles, complete suite counts and the test-UI port collision are documented in [Phase 8](../docs/PHASE8-EIGENE-ANSICHTEN.md). The report below records the earlier KI-Status branch work. / Aktueller Integrationsprüfstand in Phase 8; nachfolgend historischer Branchbericht.

# KI-Status – Abschlussbericht

Stand: 2026-11-07. Lokaler Entwicklungsstand; kein Commit, Push oder Release. Der aktuelle Datei-/HTTPS-Prüfstand steht im letzten Abschnitt; frühere Abschnitte dokumentieren die damaligen Phasen.

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

## Nicht umgesetzt / offen (historischer Phase-1-Stand)

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

## Echtes Konto lesend nutzen (historischer CLI-Arbeitsgang 2026-11-07)

Implementiert, Clean-Room ohne libre-panel-Code:

- Getrennte DE/EN-Einstellungen `ai.codex_real_account` und `ai.claude_real_account`, Standard AUS. Einschaltversuch zeigt die Anleitung und lässt den Schalter aus; erst die separate Bestätigung setzt ihn. Anschließend speichern/neustarten. Unbestätigte Vorschau aktiviert nichts. Ausschalten stoppt den gesamten CLI-Worker bis zum Neustart; die Hook-Einrichtung bleibt separat.
- Codex erhält nach Opt-in die tatsächlichen Benutzerpfade einschließlich eines gesetzten `CODEX_HOME`; ohne Opt-in bleiben die bisherigen isolierten Homes. Ausschließlich Initialisierung, `account/read` (`refreshToken:false`) und `account/rateLimits/read`, keine Modellnachricht und keine Login-/Logout-/Tokenmethoden. 300/10080 Minuten bestimmen 5h/Woche; abgemeldet erscheint als eigener DE/EN-Status.
- Umgebungsaufbau liest nur einzeln benannte System-/Home-Pfade. Kein Auslesen des gesamten Umgebungsblocks, keine API-Schlüsselübernahme, keine Credential-Dateien oder Protokollierung von CLI-Ausgaben. Der Anbieter-Client verwaltet seine Anmeldung selbst; eigene interne Schreibvorgänge des Clients sind nicht als gesperrt zugesichert. Die Nutzungsabfrage kann den Anbieter über das Netzwerk kontaktieren; angezeigte Daten bleiben lokal.
- Claude-Suche zusätzlich anhand von Metadaten unter `.local/bin`, `APPDATA/npm` und `LOCALAPPDATA/Programs/{Claude,claude-code}`. Nur native EXE; keine Skriptinhalte gelesen oder npm-Wrapper als Shellbefehl ausgeführt. Die installierte echte `.local/bin/claude.exe --help` lief mit ausschließlich temporären Home-/Konfigurationspfaden und Exitcode 0. Kein belegter Nutzungsbefehl ohne Modellnachricht in dieser Hilfe: **Nutzung nur über Statuszeile**. Konto-Opt-in erlaubt bei Claude nur den Hilfeaufruf mit Benutzerpfaden; keine automatische Kontonutzungsabfrage, keine undokumentierten APIs.
- Bounded pipes, Prozesslimit 12 Sekunden, Kill-on-close-Job, ein Programm gleichzeitig und Fehlerpause bleiben erhalten. Ein fehlgeschlagener Claude-Hilfeaufruf verhindert die anschließende unabhängige Codex-Abfrage nicht; der nächste Durchlauf pausiert weiterhin 15 Minuten.
- DE/EN-Anleitung, Haupt-READMEs, C++-README und alle drei Changelogs aktualisiert. Keine Versions-/Release-/Downloadänderung; keine Änderung an den ausgeschlossenen Editor-Dateien.

### Tatsächlich ausgeführte aktuelle Prüfungen

| Prüfung | Ergebnis |
| --- | --- |
| `cpp/build/ai-build.ps1`: `cmake --build cpp/build/testversion` mit Targets ScreenBuddy, fl-tests, fl-plugin-tests, fl-builtin-catalog-tests | erfolgreich; frische Kompilation der geänderten C++-Quellen und Linken von ScreenBuddy/fl-tests belegt; vorhandene C4996-Warnung in tests.cpp beim ersten Neubau |
| `fl-tests ai-status` | **294 bestanden, 0 fehlgeschlagen**; positiver DPAPI-Roundtrip ausdrücklich SKIP |
| `fl-tests ai-cli-test` | **31 bestanden, 0 fehlgeschlagen**, ausschließlich synthetische Programme/Antworten und Temp-Pfade |
| `fl-builtin-catalog-tests` | **279 bestanden, 0 fehlgeschlagen** |
| `fl-plugin-tests` | **380 bestanden, 0 fehlgeschlagen** |
| `tests/ai_adapter_tests.ps1` | **15 bestanden, 0 fehlgeschlagen**; AST-Syntax geprüft, kein DPAPI/HTTP-End-to-End |
| Dokumentationslinks / `git diff --check` / ausgeschlossene Quelldateien | **203 lokale Links, 0 defekt** / erfolgreich / **0 Änderungen** |

Die neuen Prüfungen decken Standard-AUS und Konfigurationsmerge, isolierte vs. freigegebene Benutzerpfade, beide Opt-ins gemeinsam und jeweils unabhängig, isolierte Codex-Hilfe, beide Limitfenster, Abmeldung ohne Limits und ohne nachfolgende Limitabfrage sowie vier Claude-Installationspfade außerhalb PATH ab. Bestehende Fehlerpausen-/Abbruchprüfungen laufen weiter. Das synthetische Programm akzeptiert ausschließlich die erwarteten Protokollmethoden und prüft `refreshToken:false`; es schreibt nur harmlose Marker in sein Temp-Arbeitsverzeichnis. Keine Credentials werden erzeugt oder geprüft.

Ein Zwischenlauf fand eine Namensüberschattung des Opt-in-Flags durch die Kontoantwort im neuen Abmeldestatus. Korrigiert; der abschließende Lauf bestand. Testzahlen oben stammen vom abschließenden Stand.

Offen: echte angemeldete Codex-Limitabfrage, echte Hooks/Claude-Statuszeile, interaktive DE/EN-Einwilligungsoberfläche, positiver DPAPI-End-to-End-Test und physische Mehrgeräteabnahme. Keine vollständige fl-tests-Gesamtsuite in diesem Arbeitsgang ausgeführt; deren historischer DPAPI-Fehler bleibt separat dokumentiert. Die installierte Claude-Hilfe ist kein Nachweis einer echten Kontonutzungsabfrage.

ScreenBuddy.exe ausschließlich gebaut, **nie gestartet**. Keine Schreibvorgänge in echte `.claude`-/`.codex`-Verzeichnisse, keine Credential-Dateien gelesen, keine Auth-Tokens kopiert/gespeichert/angezeigt, kein Empfänger am reservierten Einstellungsport. Kein Commit, Push, Installieren oder Release. Der lokale Entwicklungsbuild ist keine öffentliche EXE-Auslieferung.

## Login-Dateien und inoffizielle HTTPS-Nutzung (aktueller Arbeitsgang 2026-11-07)

Dieser Abschnitt ersetzt den historischen CLI-Kontoabruf als aktuellen Implementierungsstand; frühere Prüfergebnisse bleiben historische Nachweise.

- Vorhandene getrennte Konto-Schalter erweitert, keine doppelten Einstellungen. Standard AUS und separate DE/EN-Einwilligungsbestätigung erhalten. Einwilligung nennt inoffizielle Schnittstellen, Änderungsrisiko, unklare Anbieter-Duldung, ausschließlich lesenden Zugriff und Tokenübermittlung nur an den jeweiligen Anbieter.
- Claude liest ausschließlich die Windows-Datei unter USERPROFILE/.claude/.credentials.json, Codex USERPROFILE/.codex/auth.json. Nur Zugriffstoken und optional Codex-Konto-ID werden für die fest vorgegebenen HTTPS-GETs verwendet. Kein Refresh, kein Schreiben von Login-Dateien, kein Anbieterprozess im Konto-Modus. Eigene Datei-/Parser-/Headerpuffer werden beim Freigeben überschrieben; auch kurze Strings, Parserfehler und Reallokationen sind berücksichtigt. Der Credential-Parser verwendet feste Fehlermeldungen ohne Eingabefragmente. Interne Windows-/WinHTTP-Kopien liegen außerhalb der eigenen Pufferverwaltung.
- Feste Hosts/Pfade, TLS mit unveränderter Windows-Zertifikats-/Hostprüfung; alle Redirects, Proxy, Cookies und automatische HTTP-Anmeldung deaktiviert. 3-Sekunden-Schritt-Timeouts, 12-Sekunden-Body-Lesefrist, maximal 64 KiB Antwort und 256 KiB Login-Datei. Anbieterfehlertexte werden nicht übernommen. Die HTTPS-Transportregeln sind durch Quellprüfung belegt, nicht durch einen echten Anbieter-/TLS-Aufruf.
- Claude-ISO-8601-Reset sowie Codex-Fenster anhand 18000/604800 Sekunden; vertauschte Fenster, null-secondary, Begrenzung auf 0–100 und unbekannte Fenster berücksichtigt. Modellfenster/extra_usage werden toleriert, aber nicht als Gesamtlimit oder Tageskosten erfunden. Fehlende/ungültige Anmeldung, Ablaufdatum/JWT-exp und 401/403/429 erhalten lesbare feste Gründe; direkte Limits werden nach Fehlern n/a.
- Unabhängige Anbieterzeitpunkte: Erfolg frühestens nach 180 Sekunden, jeder Fehler nach 900 Sekunden. Aus-/Einschalten umgeht bestehende Pausen nicht. Ein Anbieterfehler verhindert den anderen Anbieter nicht. Ausschalten verwirft die Ergebnisse laufender Abfragen und pausiert den gesamten Worker bis Neustart; ein laufender WinHTTP-Aufruf endet innerhalb seiner Zeitgrenzen.
- ai.port in Config und KI-Einstellungen, Standard 18765. Belegter Wunschport führt zu freiem dynamischem Port. Exklusive Sockets erhalten; nach fehlgeschlagener Bindung muss httplib vor dem nächsten Versuch zurückgesetzt werden. Tatsächlicher Port wird im Service gehalten, in der Einstellungsstatuszeile angezeigt und beim Generieren des gemeinsamen Hook-/Statuszeilenadapters eingesetzt.
- DE/EN-Anleitung, READMEs, Beitragsregeln und alle drei Changelogs aktualisiert. Veraltete Aussagen zur Katalogabwesenheit in der Anleitung mit dem vorhandenen Builtin-Katalog/Renderpfad abgeglichen. Keine Versions-/Download-/Releaseänderung und keine Änderung an ausgeschlossenen Editor-Dateien.

### Tatsächlich ausgeführte abschließende Prüfungen

| Prüfung | Ergebnis |
| --- | --- |
| cpp/build/ai-build.ps1: ScreenBuddy, fl-tests, fl-plugin-tests, fl-builtin-catalog-tests | **erfolgreich**, geänderte Quellen frisch kompiliert und ScreenBuddy/fl-tests gelinkt; beim umfangreichen Neubau vorhandene C4996-getenv-Warnung in tests.cpp |
| fl-tests ai-status | **298 bestanden, 0 fehlgeschlagen**; positiver DPAPI-Roundtrip ausdrücklich SKIP |
| fl-tests ai-cli-test | **147 bestanden, 0 fehlgeschlagen**; ausschließlich synthetische Programme, Login-/Antwortdaten und Temp-Pfade; injizierter Anbietertransport |
| fl-builtin-catalog-tests | **279 bestanden, 0 fehlgeschlagen** |
| fl-plugin-tests | **380 bestanden, 0 fehlgeschlagen** |
| tests/ai_adapter_tests.ps1 | **15 bestanden, 0 fehlgeschlagen**; AST-Syntaxprüfung, kein DPAPI-/HTTP-End-to-End |
| git diff --check / ausgeschlossene Quelldateien | erfolgreich / **0 Änderungen** |

Dokumentationsabschluss: **215 lokale Markdown-Links geprüft, 0 defekt** (geänderte Dokumente einschließlich beider Beitragsregeln); git diff --check nach dem Dokumentationsabgleich erfolgreich.

Die synthetischen Prüfungen decken beide Antwortformate, ISO-Reset/Zeitzone, ungültige Zeiten/Typen, vertauschte Codex-Fenster, null-secondary, unbekannte Fenster, Clamp, fehlende/fehlerhafte/übergroße Login-Dateien, doppelte Schlüssel, JSON-Escapes/Unicode, abgelaufene Token ohne Transportaufruf, optionale Konto-ID, 401/403/429/Redirect/Server-/Transportfehler, ungültige/übergroße Antworten, unveränderte Login-Dateibytes, n/a nach Fehler und Wiederherstellung nach Erfolg ab. Anbieter-Schalter werden einzeln und gemeinsam mit injizierter Abfrage geprüft, einschließlich Fehlerisolation und Umschalten innerhalb der Pause. Die 3-/15-Minuten-Werte sind geprüft; kein Echtzeit-Warten über volle Intervalle. Kein Heap-/Speicherforensik-Nachweis der Löschung; dies ist durch Pufferverwaltung im Quellcode belegt.

Portprüfung: Zweiter synthetischer Empfänger fordert den bereits belegten dynamischen Port des ersten an, erhält einen anderen Port; der erzeugte Adapter enthält exakt diesen tatsächlichen Port. Synthetische authentifizierte Loopback-HTTP-Tests bleiben erhalten. Ein Zwischenlauf scheiterte beim Fallback wegen der httplib-Decommission-Markierung; Reset ergänzt, abschließender Lauf fehlerfrei.

Offen: echte angemeldete Anbieterantworten, Zertifikats-/Redirect-/Timeout-Verhalten gegen echte Dienste, interaktive DE/EN-Einwilligung und Portanzeige, positiver DPAPI-/Adapter-End-to-End-Test, echte Hooks und physische Mehrgeräte-Abnahme. Kein vollständiger fl-tests-Gesamtlauf in diesem Arbeitsgang; der historische DPAPI-Fehler bleibt separat dokumentiert. **Inoffizielle Endpunkte können sich ändern oder vom Anbieter verweigert werden; Anbieter-Duldung ist unklar.**

ScreenBuddy.exe **nie gestartet**. Keine echten Login-/Credential-Dateien gelesen oder geschrieben, keine echten Anbieter-Netzwerkaufrufe, kein Testempfänger am reservierten Einstellungsport; HTTP nur synthetisch auf dynamischen Loopback-Ports. Kein Commit/Push, keine Installation oder Veröffentlichung. Dieser lokale Entwicklungsbuild ist keine signierte öffentliche EXE-Auslieferung.
