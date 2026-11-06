# Testprotokoll – Stand 06.11.2026

## P7-5 bis P7-7 – Katalog, Store und RSS-Editor (06.11.2026)

Branch `feature/zonen-layout`, Hauptcheckout auf `12ca4ed`, uncommittiert. Kein Netzwerk/Port, kein Audio, keine laufende App oder Hardware. Companion ausschließlich `--offline`; TLS-/Serverintegration ausgelassen.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Reiner Built-in-Katalog und strikt versionierte Overlays; Standard behält festen Renderweg, HA später. 14 Suiten grün (Companion offline: 250); Built-in 4.221 Bilder/Trefferkarten: 0 Fehler, Plugin 2.093 Prüfungen: 0 Fehler; physische Abnahme offen.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomare Seitenlayouts mit Backup/Revision, Beispieldatenvorschau, Raster-Touch-Zielen und isoliertem Clip; fester Renderweg unverändert. 14 Suiten grün (Editor 835; Companion offline 250); Built-in 4.221 Bilder/Trefferkarten und Plugin-Baselines unverändert; Hardware offen.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS-Editor mit Beiträge/Pflicht-Zustand, getrennten Quer-/Hochlayouts in `layouts/builtin.rss.layout.json`; Öffnen/Übernehmen von Standard behält den pixelgleichen festen Weg, echte Änderungen nutzen das Raster (Abstände können sich ändern, E2). Zurücksetzen stellt Standard her; Zeilen, Blättern und Touch teilen einen Plan. Kopf/Fuß fest; Built-in-Wechselfelder und HA später. 14 Suiten grün (Katalog 217, Editor 874, Kern 3.470; Companion offline 250); bestehende Baselines bitgleich, zusätzlich 1.024 RSS-Bilder/Trefferkarten und 5.952 Prüfungen ohne Fehler; Hardware offen.

Abschluss P7-7: `fl-tests` 3470/0; layout-edit 163/0; layout-ui 874/0; plugins 380/0; store 422/0; zones 462/0; example 124/0; settings 135/0; snapshots 2093/0; builtin 4221 Bilder + 4221 Trefferkarten/0; companion `--offline` 250/0; device-profile 85/0; schedule 845/0; neuer Katalog 217/0. Neue Datei `tests/layout-snapshots/p7-rss-hashes.txt`: 1024 Rasterbilder/Trefferkarten, 5952 Prüfungen/0. Keine bestehenden Baselines aktualisiert. Abweichungen vom Dateiplan: punktierte Built-in-IDs nur im Overlay-Parser zugelassen (`plugins.cpp`); Editor und Einstiege für RSS ergänzt. Built-in-Wechselfelder bleiben deaktiviert (E6 offen). Keine Release-Prüfung, kein Commit/Push/Branchwechsel; öffentliche Auslieferung nicht erstellt.

## P7-1 bis P7-4 – Drag & Drop im Seiten-Editor (06.11.2026)

Branch `feature/zonen-layout`, uncommittete Änderungen auf HEAD `ccf8eab`. Code gebaut und getestet; kein Commit, kein Push. Umfang: P7-1 `ui_kit` (Mauszeiger je Stelle, Abbruch bei Capture-Verlust und Esc), P7-2 Modell (`place_block`, `remove_block`, `move_block`, `probe`), P7-3 `GridEditor` (Bausteinleiste, Ziehen, Griffe, Ablage, Aufnehmen und Ablegen per Tastatur, MSAA), P7-4 Editor-Seite (Hinweistext, Vorschau nach dem Ablegen).

| Suite | Bestanden / Fehler |
|---|---|
| `fl-layout-edit-tests` | 145 / 0 |
| `fl-layout-ui-tests` | 624 / 0 |
| `fl-builtin-snapshot-tests` | 4.221 Bilder / 0 Fehler |
| `fl-plugin-snapshot-tests` | 2.093 / 0 |
| `fl-tests` | 3.470 / 0 |

**Nicht gelaufen / offen:**
- `native_mouse_smoke.py` und `native_settings_smoke.py` benötigen eine laufende App; in diesem Lauf nicht ausgeführt.
- Hardware- und Sichtabnahme durch Basti (Maus, Touchpad, Touch-Bildschirm) steht aus.

## P6-Reviewkorrekturen (06.11.2026)

Im Hauptcheckout `feature/zonen-layout`, auf den uncommitteten P6-3/4/5-Änderungen. Alle sieben Befunde bearbeitet: Body/Undo/Redo/Fehlerwahl bleiben bei busy bedienbar; verzögerte Vorschaujobs und verworfene ältere Bilder geprüft. Zone/Sekunden/Vergleichswert werden bei Fehlern markiert, ohne Fokusentzug aus `on_commit`/WM_KILLFOCUS und ohne wiederholten Toast derselben Eingabe. Pluginwechsel fragt vor dem Verwerfen mit Standard Nein (DE/EN); Nein und Ja automatisiert geprüft. Übernehmen/Reset synchronisieren vorhandene Widgets, setzen den Fokus vom gesperrten Aktionsknopf ins Raster und überschreiben keine während des Jobs entstandenen Änderungen. Null Blöcke sind abgesichert. App-Anordnungsfehler laufen markiert über die vorhandene DE/EN-Meldungstabelle im UI-Thread; bekannte Store-/Verfügbarkeitsmeldungen getestet. Beide Test-Tempordner werden nach App-Abbau entfernt, auch über RAII bei frühem Ende; erfolgreiche Entfernung geprüft.

Build: zusätzlich zu den beiden Editorzielen die unveränderte Vorlage `cpp/build/store/p5-0-validation/build.cmd` ausgeführt, Exit 0. Bekannte `vswhere.exe`-Meldung ohne Abbruch; erster Neuaufbau mit vorhandener `getenv`-Warnung. Endstand der erneut ausgeführten geforderten Suiten:

| Suite | Bestanden / Fehler |
|---|---|
| `fl-tests` | 3.470 / 0 |
| `fl-layout-edit-tests` | 82 / 0 |
| `fl-layout-ui-tests` (inkl. 11 Editor-Snapshots) | 310 / 0 |
| `fl-plugin-tests` | 380 / 0 |
| `fl-plugin-store-tests` | 422 / 0 |
| `fl-plugin-zones-tests` | 462 / 0 |
| `fl-plugin-example-tests` | 124 / 0 |
| `fl-plugin-settings-tests` | 135 / 0 |
| `fl-plugin-snapshot-tests` (inkl. 21 Temp-Cleanup-Prüfungen) | 2.093 / 0 |
| `fl-builtin-snapshot-tests` | 4.221 Bilder + 4.221 Trefferkarten / 0 Fehler |

Ohne Sandbox, Roboto über den Schrift-Preflight sichtbar. Built-in-/Plugin-Baselines unverändert; **kein `update`, keine neuen Editor-Hashes** (Editor-Hashdatei SHA-256 weiterhin `B900DCB300DCE3019632AA02C1E8E3D4894DF15E1A38F15A2E353EFC93AED594`). Phase-5-Dateien `render.cpp/.h`, `page_frame.*`, `page_samples.*` unverändert. Kein Netzwerk/Port 8765, keine Töne, kein Fremdcode, Commit, Push, Branchwechsel, Installations- oder Release-Schritt. DE/EN-Referenz/Tutorial und alle drei Changelogs ergänzt; lokale Links und Diff geprüft. Rückfragen über den überschriebenen Host-Dialogweg automatisiert geprüft; sichtbare Dialog-/physische Geräteabnahme bleibt offen.

## P6-3 / P6-4 / P6-5 – nativer Seiten-Editor (06.11.2026)

Hauptcheckout `feature/zonen-layout`, Basis/HEAD weiterhin `f49d559`. Pakete nacheinander umgesetzt; keine Commits, Pushes oder Branchwechsel. Clean-Room: nur Phase-6-Entwurf und ScreenBuddy-Code. Kein Netz (auch kein Loopback-Server), keine Töne, keine Hardware-/Installations-/Release-Aktion und kein Zugriff auf Port 8765. `tests/run_release_checks.py` nur um Suiten ergänzt und syntaktisch geparst, **nicht gestartet**.

- **P6-3 fertig:** `layout_edit.*` mit transaktionalen Rasteroperationen, Teilung/Löschung/Blockzuordnung, DE/EN-Diagnosen und 100-Schritt-Historie. Ganze Modelle, serialisierungsgetreue Gleichheit auch bei `1` gegenüber `1.0`, Undo/Redo und 600-ms-Pfeilgruppen. `ui_layout_grid.*` mit allen Tasten aus §7, getrennter Zell-/Zonenauswahl, MSAA-Liste/Werteänderungen, ID und 2-DIP-Auswahlrahmen. Zellen ≥32 DIP, horizontaler Ausschnitt folgt dem Zellfokus und clippt, vertikaler Bildlauf über Surface.
- **P6-4 fertig:** `ui_page_layout.cpp`, Registrierung und Einstiege aus Plugins/PagesPage. Aktivierte Plugins, v1 nur Hinweis, keine Built-ins/HA. Rasteranteile, Zonen-ID/Modus/Übergang, Blockliste/Sichtbarkeit/Reihenfolge/Zuordnung, Sekunden und typisierte Bedingungen ohne Secret-Settings. Validierung mit Zone/Block, Taste zur Fehlerstelle, ungültige/unbestätigte Entwürfe sperren Übernehmen. Alle Widgets `track_dirty=false`; Historie überlebt vollständig neu gebaute Widgets. Anwendung/Reset ausschließlich über die P6-2-Job-/Store-Schnittstellen, atomar mit Backup, ohne Konfigurationsspeichern oder Neustart. Reset-Rückfrage im Code, sichtbare Dialogabnahme offen.
- **P6-5 fertig (automatischer Umfang):** Vorschau ausschließlich `App::plugin_preview` mit demselben Parser, Plugin-Samples und Renderer; 480×320 / 320×480, maximal ein laufender Auftrag, Generation und nachgeholte letzte Änderung. Ungültige Entwürfe behalten das letzte Bild; Verlassen setzt den Override zurück. Nur der neue Editoraufruf wählt feste Beispielzeit/-datum (12:34 / 2026-01-01), bisherige Vorschauaufrufe bleiben unverändert. Freigegebener globaler heller Fokusring samt Textfeldrahmen auf `accent_text` (Glut) abgedunkelt. Kontrastreiche Blockauswahl bleibt lokal im Editor. Native Tastatur-/MSAA-/Storetests, 11 eigene Editor-Raster-Snapshots und Vergleich echter App-Vorschauen mit allen v2-Plugin-Snapshots ergänzt.

Build: VS Build Tools 2022, `vcvars64.bat`, dortiges CMake/Ninja Release, vorhandene lokale vcpkg-Abhängigkeiten mit Manifestmodus aus; Ordner `cpp/build/store`, Vorlage `p5-0-validation/build.cmd`. ScreenBuddy und alle nachstehenden Ziele erfolgreich gebaut. Bekannte `vswhere.exe`-Meldung und vorhandene `getenv`-Warnung verhindern den Build nicht. Kein öffentlicher EXE-Export.

| Suite | Endgültiger automatischer Stand |
|---|---|
| `fl-tests` | **3.470 / 0**, davon 80 neue Editor-Kontrastprüfungen |
| `fl-layout-edit-tests` | **62 / 0** |
| `fl-layout-ui-tests` | **246 / 0**, einschließlich 11 neuer Raster-Snapshotvergleiche |
| `native_settings_smoke.py <build> --editor-offline` | **246 / 0**, ruft die isolierte native Editor-Suite auf |
| `fl-plugin-tests` | **380 / 0** |
| `fl-plugin-zones-tests` | **462 / 0** |
| `fl-plugin-example-tests` | **124 / 0** |
| `fl-plugin-store-tests` | **422 / 0** |
| `fl-plugin-transport-tests` | **140 / 0**, Testtransport/numerische lokale DNS-Vertragsprüfung ohne Socketverkehr |
| `fl-plugin-settings-tests` | **135 / 0** |
| `fl-plugin-sign-tests` | **111 / 0** |
| `fl-plugin-catalog-tests` | **77 / 0** |
| `fl-plugin-schedule-tests` | **845 / 0** |
| `fl-device-profile-tests` | **85 / 0** |
| `fl-companion-tests --offline` | **250 / 0**, reine Protokoll-/Speicherfälle; Serverfälle ausdrücklich nicht gelaufen |
| `fl-plugin-snapshot-tests` | **2.072 / 0**, davon 171 zusätzliche Prüfungen für unveränderte v2-Overlays/echte App-Vorschau, sieben Themes, vier Größen |
| `fl-builtin-snapshot-tests` | **4.221 Bilder + 4.221 Trefferkarten / 0 Fehler** |

Kontrastprüfung am tatsächlichen `ui::Theme` für FuchsLabs hell, Cupertino/Fluent/Material hell (über Inline-Profil) und Windows hell mit festen Einstellungen: Text/Nebentext auf Karte/Hintergrund/Eingabe ≥4,5:1, Fokusring auf diesen Flächen ≥3:1, Rasterrahmen/Auswahl ≥3:1, Primary-/Danger-Texte und ausgewählte Blocklisten-/Hinweisflächen ≥4,5:1. Keine bekannten Kontrastausnahmen. Neue synthetische Rasterbilder (FuchsLabs quer und Material hoch) zusätzlich angesehen; keine echte Geräte-/vollständige sichtbare App-Abnahme daraus abgeleitet.

Native Suite: echte Surface-/MSAA-Abfrage (`ROLE_SYSTEM_LIST`, Name und Wert), WM_KEYDOWN/WM_SYSKEYDOWN mit rein threadlokalen Modifikatoren, Alt+Links/Rechts, Strg+Z und Strg+Shift+Z, Größenänderung, Anlegen/Löschen, Blockreihenfolge/Sichtbarkeit, Validierung/Fehlerfokus, ID-Umbenennung, Tabfolge bei 200 % DPI, zerstörte/neue Widgets mit erhaltener Historie, echte Store-Anwendung und Vorgänger-Backup, unveränderte Manifestdatei, Dirty-Zähler 0, fehlende Secret-Auswahl, v1-Hinweis und verspätete Vorschau nach hidden(). App wird nur mit synthetischer Konfiguration und abgeschalteten Quellen/Updates/Companion konstruiert; keine App-/Server-Schleife.

**Abweichungen / Grenzen des kleinsten Umfangs:**

- Rasteranteile sind begrenzte Zahlen-Schieberegler (Pfeiltasten/Home/End) statt freier Zahlenfelder. Spalten/Zeilen haben +/−. Schmale Raster scrollen automatisch mit dem Zellfokus, ohne eigene horizontale Scrollleiste.
- Quellenpfade für `show_if` kommen aus Sample-Daten und bestehenden Bedingungen (begrenzte Auswahl), dazu nicht geheime Settings. Kein freies Pfadfeld für sonstige Quellfelder. Operator/Werttyp werden mit „Bedingung bestätigen“ abgeschlossen; Text mit Enter/Fokuswechsel. Kein rohes JSON.
- Passt der Inhalt einer gelöschten Zone nicht in die erste verbleibende Zone, wird die Löschung mit Meldung abgewiesen statt einen ungültigen Transfer nach einer Rückfrage zu erzwingen. Blöcke zuerst manuell umhängen.
- Der bestehende allgemeine Python-Smoke-Test startet eine App mit lokalem HTTP-Port. Wegen „kein Netzwerk“ wurde stattdessen dessen neuer `--editor-offline`-Weg mit echter nativer Surface und MSAA verwendet. Kein kompletter sichtbarer Einstellungsfenster-Smoke-Test.
- Die vollständige Companion-Suite benötigt Loopback-TLS-Server. Der neue `--offline`-Modus meldet die ausgelassenen Server-, Härtungs-, Resize/Swipe- und unabhängigen Serverfälle ausdrücklich. Daher **keine Behauptung, die komplette Companion-Netzwerksuite sei gelaufen**.
- Physische Handabnahme auf USB 480×320/hochkant, Monitor und Companion sowie der sichtbare Reset-Rückfragedialog bleiben offen. Keine Erststart-Erweiterung (P6-6).

`render.cpp/.h`, `page_frame.*`, `page_samples.*`, `plugin_render.cpp`, `main.cpp` und alle vorhandenen Plugin-/Built-in-Baselines einschließlich beider `hashes.txt` laut Git-Diff unverändert. Nur die neue Datei `tests/layout-snapshots/hashes.txt` wurde als eigene Editor-Baseline angelegt. Neue Suiten in `build.bat` und `tests/run_release_checks.py` eingetragen. DE/EN-Systemreferenz, Tutorial und beide Haupt-Changelogs sowie C++-Changelog/Clean-Room-Nachweis aktualisiert; keine Versions-/Download-/Laufzeitvoraussetzungen geändert. Lokale neue Dokumentationslinks, Python-Syntax und `git diff --check` geprüft.

## P6-1/P6-2 – Overlay, Store und App (06.11.2026)

Basis `ba22173`, Hauptcheckout `feature/zonen-layout`; nur die Paketdateien und die ausdrücklich angeforderten fünf Dokumentationsdateien geändert. Clean-Room: ausschließlich Entwurf und ScreenBuddy-Code, kein libre-panel-Code. Kein Commit/Push/Branchwechsel, Netz, Portzugriff auf 8765, Audio oder `run_release_checks.py`.

Build: vorhandene Vorlage `build/store/p5-0-validation/build.cmd`, VS Build Tools/vcvars64 und CMake/Ninja unter `build/store`; alle Ziele der Vorlage einschließlich ScreenBuddy erfolgreich (Exit 0; bekannte `vswhere.exe`-Meldung ohne Abbruch). Kein Installations-/Release-Artefakt exportiert.

| Einzeln gestartete Suite | Bestanden / Fehler |
|---|---|
| Parser `fl-plugin-tests` | 380 / 0; neue Tabelle mit 40 Overlay-Fällen plus ergänzende Prüfungen |
| Speicher `fl-plugin-store-tests` | 422 / 0 |
| Transport `fl-plugin-transport-tests` | 140 / 0; Abbruch-/Validierungsfälle und Testtransport, kein Netz |
| Settings `fl-plugin-settings-tests` | 135 / 0 |
| Zonen `fl-plugin-zones-tests` | 462 / 0; zusätzlich 2 × 4.000 feste Fuzz-Fälle |
| Beispiele `fl-plugin-example-tests` | 124 / 0 |
| Wechselfeld `fl-plugin-schedule-tests` | 845 / 0 |
| `fl-tests plugin-touch` | 2.093 / 0 |
| Plugin-Snapshots | 1.901 / 0 |
| Built-in-Snapshots | 4.221 Bilder + 4.221 Trefferkarten / 0 Fehler |

Prüfungen ohne Sandbox, Roboto sichtbar. Snapshot-Dateien und beide `hashes.txt` unverändert. Neue Fälle belegen unter anderem unveränderte Manifest-/Freigabedateien und `generation`, gültige Host-/Linkfreigaben, eigene Layoutrevision, Laden ohne/mit/veraltetem/beschädigtem Overlay, Basiswechsel nach Plugin-Update, Vorgänger-Backup, Reset/Remove, 32 Plugins mit je sechs Dateien sowie die 256-Einträge-Grenze. Ein offener Windows-Dateihandle erlaubt das Backup-Lesen, verhindert aber den atomaren Austausch: alte Datei, wirksame Anordnung und Revision bleiben erhalten. Zusätzlich Backupfehler, Hard-/Symlinks an aktiver Datei und Backup sowie vier gleichzeitige Store-Aufrufe geprüft; keine übersprungenen Linkfälle.

Entscheidungen: `layout_revision` statt Erhöhen von `generation`; auch ausgeblendete Blöcke müssen gültige Zonen/Bedingungen/Wechselzeiten haben. Beschädigte Overlays erhalten zusätzlich den Zustand `invalid` und führen zu einer Teil-Lademeldung, während die Basis nutzbar bleibt; `stale` bleibt separat und die Datei erhalten. Reset entfernt zuerst das Backup und dann die aktive Datei; ein fehlgeschlagenes Schreiben ändert den Speicherstand nicht. Reine `arrangement_from`-Entwürfe ohne übergebenen SHA tragen einen Null-Fingerabdruck; `Store::arrangement_get` liefert das an die aktuelle Basis gebundene Startmodell. Vorschaugrößen werden vor dem Rendern auf 1..4096 je Achse begrenzt.

App-Anbindung und Vorschauüberladung erfolgreich gebaut; Editoroberfläche (P6-3 ff.), tatsächliche Job-/UI-Bedienung und physische Abnahme auf USB/Monitor/Companion bleiben offen. Kein vollständiger `fl-tests`-Lauf und keine Hardwaretests.
- P5-5 (06.11.2026, Branch `p5/spotify`, Basis `ba22173`): Nur Spotify mechanisch auf `draw_framed` mit `spotify.cover`, `.text`, `.pegel`, `.fortschritt`, `.tasten`, `.hinweis` umgestellt; feste Vollflächen erhalten die bisherigen absoluten `layout()`-Ausdrücke/Überlappungen. Reiner Fußtext aus Snapshot, `layout()`/`button_at()` unverändert. VS Build Tools/vcvars64, CMake/Ninja Release, lokale vcpkg-Installation und `x64-windows-static`, nur drei angeforderte Ziele unter `build/store` gebaut (Exit 0; bekannte vswhere-/getenv-Warnungen). Aus diesem `cpp`-Ordner ohne Sandbox: Built-in **4.221 Bilder + 4.221 Trefferkarten, 0 Fehler**, Plugin-Snapshots **1.901 bestanden, 0 Fehler**, vollständiger Standardlauf `fl-tests` **3.390 bestanden, 0 fehlgeschlagen**. Erster Built-in-Lauf verwendete eine während des Builds noch unvollständige Tasten-Zwischenfassung (469 Bildabweichungen); nach abgeschlossenem Neuaufbau obiger vollständiger Vergleich grün. Beide Baselines unverändert; Quellvergleich vor/nach Spotify einschließlich HA/Willkommen und aller anderen Seiten identisch. Kein Netz/Portzugriff, keine Töne, kein Release-Prüfskript, keine physischen Tests, kein Push.

- P5-2 (06.11.2026, Branch `p5/streaming`, Basis `b1d9c59`): Nur Streaming mechanisch nach RSS-Muster migriert; gemeinsame feste Anordnung für Zeichnen und `row_at`, Zeilen-/Hinweisblöcke, reine Fußzeilenfunktion, unveränderter Logo-/Kürzel-Zeichenkörper. Release mit VS Build Tools/vcvars64, CMake/Ninja, lokaler vcpkg-Installation und `x64-windows-static` in diesem Worktree unter `build/store` konfiguriert; nur ScreenBuddy, fl-builtin-snapshot-tests, fl-plugin-snapshot-tests und fl-tests gebaut (Exit 0; bekannte vswhere-/getenv-Warnungen). Aus dem `cpp`-Ordner ohne Sandbox, Roboto sichtbar: Built-in **4.221 Bilder + 4.221 Trefferkarten, 0 Fehler**, Plugin-Snapshots **1.901 bestanden, 0 Fehler**, `fl-tests plugin-touch` **2.093 bestanden, 0 Fehler** (einschließlich Streaming-`row_at`). Beide Baselines unverändert; Quellvergleich bestätigt unveränderten Code vor/nach Streaming einschließlich HA. Kein vollständiger fl-tests-Lauf, keine physischen Tests, kein Netz, Port 8765, Audio, Release-Prüfskript, Veröffentlichung oder Push.

- P5-1 (06.11.2026): Alle Ziele mit `vcvars64.bat`/CMake in `build/store` vollständig neu gebaut (`--clean-first`, 149 Schritte, Exit 0; bekannte vswhere-/getenv-Warnungen). Einzeln grün: Plugin-Snapshots 1.901/0, Parser 316/0, `fl-tests plugin-touch` 2.093/0, Zonen 462/0 + 2 × 4.000 Fuzz-Fälle, Beispiele 124/0. Quellvergleich: außerhalb RSS unveränderter Zeichencode; RSS-Kopf → Alter/Quelle/Titel/Trennlinie je Zeile → Fuß → finish, unveränderte Schriften, `floordiv`, `top + i * rh`, Versätze und Trefferdivision; kein `zones::plan`, Clip oder Catch. Beide Snapshot-Baselines unverändert. Built-in-Test wegen in der Sandbox unsichtbarem Roboto ausdrücklich nicht gestartet: sämtliche Built-in-Bild-/Trefferkarten-Hashes und physische Abnahme extern offen. Kein Netzwerk, Port 8765, Audio, Release-Prüfskript, Commit, Push oder Branchwechsel.

## P5-0 – Built-in-Snapshots (06.11.2026)

**Baseline inzwischen erzeugt; Fehlerursache, Korrektur und TZ-/Exportabnahme noch offen.** Branch `feature/zonen-layout`; keine Migration, kein Commit/Push/Branchwechsel. `fl-builtin-snapshot-tests` verwendet ausschließlich die bestehenden Renderer, feste `render::samples`, Wanduhr 1792606500, Spotify-Monotonzeit/Fetched-at 1000 und setzt selbst `TZ=UTC0`/`_tzset()`. 63 Zustände, 7 Designs und 9 Größen DE sowie dunkel/custom-material bei 480×320/320×480 EN ergeben **4.221 Bilder + 4.221 Trefferkarten**. Die Karte hasht die statischen Trefferfunktionen auf 4-px-Raster einschließlich `rows_per_page()`/`pages(n)`; sie belegt keine ausgeführte App-Aktion. HA/Willkommen ausschließlich Kontrollbilder. Details/Modi: [Baseline-Verzeichnis](tests/builtin-snapshots/README.md).

Der folgende ursprüngliche Prüfstand entstand vor der Roboto-Benutzerinstallation; den aktuellen Nachlauf siehe weiter unten.

Release-Build lokal über `vcvars64.bat` und CMake/Ninja in `cpp/build/store` erfolgreich, einschließlich App, neuem Testziel und `fl-tests`. Bekannte `vswhere.exe`-Warnung ohne Buildabbruch und bestehende `getenv`-Compilerwarnung in `tests.cpp`. Die vorhandenen Prüfzahlen sind gegenüber dem älteren Plan gestiegen; unten stehen die tatsächlich ausgeführten aktuellen Suiten, keine übernommenen historischen Zahlen.

| Einzeln gestartete Suite | Ergebnis |
|---|---|
| `fl-device-profile-tests` | 85 bestanden |
| `fl-plugin-tests` (Parser) | 316 bestanden |
| `fl-plugin-zones-tests` | 462 bestanden; Ausgabe zusätzlich 2 × 4.000 feste Fuzz-Fälle |
| `fl-plugin-schedule-tests` | 845 bestanden |
| `fl-plugin-example-tests` | 124 bestanden |
| `fl-plugin-sign-tests` | 111 bestanden |
| `fl-plugin-catalog-tests` | 77 bestanden |
| `fl-plugin-store-tests` (Speicher) | 193 bestanden |
| `fl-plugin-transport-tests` | 140 bestanden; Ablehnungs-/Mockpfade, numerische Loopback-Adressauflösung ohne Netzverkehr |
| `fl-plugin-snapshot-tests` | 1.901 bestanden; Baseline nicht aktualisiert |
| `fl-tmdb-tests` | 119 bestanden |
| `fl-psn-tests` | 37 bestanden |
| `fl-key-help-tests` | OK |
| `fl-amp-tests` | 13 bestanden; reine Logik, keine Töne |
| `fl-monitor-tests` (ohne `window`) | 11 bestanden |
| `fl-tests plugin-touch` | 2.093 bestanden (Teilmodus, kein vollständiger Kernlauf) |
| `fl-plugin-settings-tests` | **Nicht bestanden/DPAPI blockiert**, Exit 2 beim initialen Speichern: `Plugin settings could not be encrypted or saved.` |
| `fl-builtin-snapshot-tests update` | **Nicht gelaufen bis zum Bildvergleich**, Exit 2 beim vorgeschalteten Schriftcheck: `Schrift fehlt: Roboto.` |
| `fl-builtin-snapshot-tests` mit äußerem `TZ=NZST-12NZDT` | Ebenfalls vorgeschalteter Schriftabbruch; **kein** Nachweis gleicher Bild-/Trefferkarten-Hashes |

Plugin-`hashes.txt` bleibt byteweise unverändert: SHA-256 `3039AF0D01635BC21F06FE458523014B8929ABC814A9338AD53E67915963ACA2` vor/nach. Mechanischer Quellvergleich bestätigt identische verschobene `demo_*`-Funktionskörper und Snapshot-Hilfen sowie unveränderte `tile_layout`, `compose`, Willkommen, Streaming, HA und Spotify-Treffergeometrie. Das ist Quellnachweis, kein Ersatz für die neue Bildbaseline.

**Damals offen:** Roboto auf dem Referenzrechner verfügbar machen und Baseline erzeugen. Das ist laut Nutzer inzwischen erfolgt (siehe Nachlauf); die drei regulären TZ-Vergleiche und der Export-/Hashvergleich bleiben offen. Keine Ersatzschrift-Baseline erzeugt. Der vollständige `fl-tests`-Lauf wurde wegen bekannter DPAPI-Sandboxgrenze **nicht ausgeführt**. Companion-/App-/Hardwaretests wurden wegen „kein Netzwerk“ nicht gestartet. `--demo --preview` vor/nach wurde nicht ausgeführt: Der vorhandene Modus verwendet Live-Systemmessungen und aktuelle Uhrzeit; zwei reguläre Läufe sind keine deterministische Bitgleichheitsprüfung. `run_demo` behält Live-Systemmessungen, Uhrzeit und die bisherigen zwei Blogbeiträge bei; nur die bestehenden `demo_*`-Aufrufe wechseln auf `samples`, damit das Demo-Verhalten nicht verändert wird. Die neuen eingefrorenen `system()`/`posts(lang)` werden vom Baseline-Test genutzt. Die Umstellung der unveränderten Demo-Daten ist mechanisch belegt, die verlangte Demo-Bildabnahme bleibt offen. Keine Bildgleichheit behauptet, solange dieser Nachweis fehlt.

Keine Töne, Geräteausgabe, Netzwerkverbindungen, Port 8765, Registry-Leseoperationen des neuen Tests, `run_release_checks.py`, Signierung oder Auslieferung. Das Release-Prüfskript und `build.bat` wurden nur um das neue Ziel ergänzt, nicht ausgeführt. Physische/UI-Abnahme bleibt offen. **P5-0 darf noch nicht als vollständig grün oder als Basis für P5-1 abgenommen werden.**

### P5-0-Nachlauf – Baseline vorhanden, Sandbox-Fontgrenze (06.11.2026)

Laut Nutzer mit installierter Benutzerschrift Roboto erzeugt: `cd cpp; $env:TZ='UTC0'; .\build\store\fl-builtin-snapshot-tests.exe update`. Lokal vorhanden und nachgezählt: **4.221 Bildschlüssel + 4.221 Trefferkarten und 567 DE/dunkel-PNGs**. SHA-256 von `hashes.txt`: `1DF980F2623294A34E25B370E02D99C4B1A227BD8580C46E951DF14A357073FA`. Diese Baseline ist an Rechner, Windows-/Schriftstand und DirectWrite-Fallback gebunden, kein portabler Nachweis.

Der gemeldete NZST-Lauf hinterließ 33 Bildabweichungen in `%TEMP%/fl-builtin-snapshot-actual`: **31 System, 2 Streaming; alle 33 Trefferkarten stimmen überein**. Pixelvergleich der verfügbaren dunklen Referenzen: `landscape.system.full`, `landscape.system.null` und `portrait.system.null` jeweils nur ein abweichendes Textpixel; `landscape.streaming.abbreviations` zwei. Kein verschobener Uhrtext in diesen Bildern. Das grenzt die Untersuchung auf Text-Rasterisierung ein, beweist aber noch keine Ursache.

Aktueller regulärer Testprozess sieht Roboto nicht; Lesen von `%LOCALAPPDATA%/Microsoft/Windows/Fonts` wird verweigert. Erneute Aufrufe `update` mit UTC sowie Vergleich mit äußerem UTC, NZST und entfernter TZ endeten jeweils **Exit 2 am Schriftcheck**. Baseline unverändert; kein neuer erfolgreicher Update-, Vergleichs- oder Exportlauf. Die Schriftsperre wurde nicht umgangen.

Isolierte Diagnoseprogramme nur unter `build/store/p5-0-validation`, ohne Baseline-Schreibzugriff: drei dunkle Fälle liefern in allen äußeren TZ-Varianten dieselben Baseline-Hashes; wiederholte Zeichenrunden im selben Prozess bleiben stabil. Eine größere DE-Diagnose ohne Roboto/Material zeigt 69 Abweichungen gegen die vorhandene Baseline. Eine isolierte DirectWrite-Factory erzeugt unter UTC und NZST bytegleiche Diagnoseausgaben, weiterhin 69 Abweichungen. PNG-Kodierung sowie frühes/spätes Einlesen der Soll-Hashes beseitigen diese Unterschiede nicht. **Teiluntersuchungen, keine vollständige 4.221-Bilder-Abnahme und kein bestätigter Fix.** Renderer und regulärer Testcode unverändert.

Build über `cpp/build/store/p5-0-validation/build.cmd` erneut erfolgreich (`vcvars64.bat`, CMake/Ninja, Ordner `cpp/build/store`; bekannte `vswhere.exe`-Warnung, kein Buildfehler). Erneut regulär grün: `fl-plugin-snapshot-tests` **1.901/0**, `fl-tests plugin-touch` **2.093/0**. Plugin-Hashdatei vor/nach unverändert: `3039AF0D01635BC21F06FE458523014B8929ABC814A9338AD53E67915963ACA2`.

**Weiter erforderlich:** Roboto im regulären ausführenden Prozess verfügbar machen; die 33 Abweichungen mit vollständiger Matrix reproduzieren, Ursache beheben, Baseline explizit erneuern, dann UTC/NZST/ohne TZ jeweils 0 Fehler sowie bytegleiche Export-Hashkarten nachweisen. Aufruf mit Wiederherstellung der äußeren TZ: [Snapshot-README](tests/builtin-snapshots/README.md). Kein Netzwerk, Port 8765, Audio, Hardware, Release-Prüfskript, Commit, Push oder Branchwechsel im Nachlauf. Bestehende DPAPI-/physische/UI-Grenzen bleiben offen.

## Phase-4c-Sicherheitsreview M1/M2/N1–N4 (06.11.2026)

Neuer Korrekturlauf auf `feature/zonen-layout`, bestehende uncommittete Phase-4c-Arbeit fortgeführt. Tap-URLs prüfen feste RFC-3986-Zeichen, vollständige Prozentsequenzen und kleine feste Hosts; Prüfung beim Parsen und über dieselbe Funktion bei Touch-Entscheidung und unmittelbar vor dem Browserstart. Punktwerte sowie feste (auch prozentkodierte) Punktpfadsegmente ergeben kein Ziel. Quellen-URLs behalten ihre bisherige Hostnormalisierung. Quellenfreigabe/Aktivzustand bleiben ausschließlich bei geänderter Linkliste und vollständig gleichem übrigen Manifest erhalten; identische Re-Imports und zusätzliche Widget-/Settings-/Tap-Änderungen widerrufen Freigaben. Listen ohne Platz erhalten keine Zeilen; gesammelte Ziele werden auf die Blockfläche begrenzt. Busy-Linkschalter setzt seinen lokalen Zustand zurück.

Release-Build über `vcvars64.bat` und CMake/Ninja in `cpp/build/store` erfolgreich (bekannte `vswhere.exe`-Warnung ohne Abbruch). Einzeln gestartete Suiten mit **0 Fehlern**:

| Suite | Prüfungen |
|---|---:|
| `fl-plugin-tests` | 316 |
| `fl-plugin-store-tests` | 193 |
| `fl-plugin-snapshot-tests` | 1.901 |
| `fl-tests plugin-touch` | 2.093 |
| `fl-plugin-zones-tests` | 462 + zweimal 4.000 feste Fuzz-Fälle |
| `fl-plugin-schedule-tests` | 845 |
| `fl-plugin-example-tests` | 124 |
| `fl-plugin-transport-tests` | 140 |
| `fl-plugin-sign-tests` | 111 |
| `fl-plugin-catalog-tests` | 77 |
| `fl-device-profile-tests` | 85 |
| `fl-tmdb-tests` | 119 |
| `fl-psn-tests` | 37 |
| `fl-key-help-tests` | OK |
| `fl-amp-tests` | 13, synthetisch ohne Wiedergabe |
| `fl-monitor-tests` | 11, ohne Fenstermodus |
| `fl-companion-tests` | 377, isolierter lokaler Testserver |

Neue Regressionen: ungültige feste URL-Zeichen/Prozentsequenzen/Platzhalter und Großschreibung, Punktwerte und -segmente, Update nur der Linkliste versus Links plus Widgets/Settings/Tap sowie identischer Re-Import (auch neu geladen), kaputte `links.json` beim Laden, tatsächlicher `write_links`-Fehler über ein Verzeichnis am Dateipfad (Widerruf, Erteilen und Import), Rasterziele ohne Layout, flache Listenfläche und Rate bei 1,9 s gesperrt / 2,0 s erlaubt. Browserstarts werden ausschließlich durch injizierte Funktionen geprüft. Die zusätzliche Rasterprüfung trennt die größere Löschfläche für Kantenglättung von der eigentlichen Tap-Fläche; vorhandene Snapshot-Vergleiche sind grün.

**Umgebungsblocker:** `fl-plugin-settings-tests` beendet sich beim ersten Speichern mit „Plugin settings could not be encrypted or saved“ (Exit 2). `fl-tests` vollständig bricht im Abschnitt `config` mit „Verschlüsselung fehlgeschlagen“ ab (Exit -1073740791). DPAPI wird nicht ersetzt oder umgangen; beide Suiten sind nicht grün. Prozess-/Installer-/Distributionstests und reale Geräte/UI-Abnahme wurden in diesem Korrekturlauf nicht ausgeführt. Native Busy-Schalter-Sichtprüfung bleibt offen.

Clean-Room: nur eigener Repo-Code und lokale Arbeitsanweisungen, kein Fremdcode gelesen. Keine Töne, keine Hardware/COM-Zugriffe, keine echten Links, kein Port 8765 gestartet, kein `run_release_checks.py`, kein EXE-Export und kein Commit/Push/Branchwechsel. DE/EN-System, Paketformat, Tutorial und drei Changelogs aktualisiert. `git diff --check` und lokale Dateilinks der sechs betroffenen Plugin-Dokumente geprüft. Bestehende Snapshot-Basis unverändert: `cpp/tests/plugin-snapshots/hashes.txt`, SHA-256 `3039AF0D01635BC21F06FE458523014B8929ABC814A9338AD53E67915963ACA2`. Die folgenden Abschnitte dokumentieren frühere Läufe mit ihren damaligen Prüfzahlen.


## Phase 4c – P4c-1 bis P4c-4 (06.11.2026)

Implementiert in Paket-Reihenfolge: strenge Schema-2-Felder `links`/`tap` und kodierte Expansion; Ziele im selben Zeichenlauf mit gemeinsamer Listenzeilen-Geometrie; persistente eigene Linkfreigabe (Standard aus) mit DE/EN-Schalter und Hostliste; Snapshot-Treffer, Ruhezeit (Companion 1,0 s / Monitor 0,3 s), Generations-/HTTPS-/Host-Prüfung beim Tipp und unmittelbar vor dem Browserstart, global je Plugin höchstens ein Linkversuch alle zwei Sekunden. Nur `open_link` und `next`; keine geheimen Einstellungen in Linkvorlagen. Feste Logtexte ohne URLs/Daten. Kachel- und Sperrzonenversatz werden über die Kachelbox in die lokale Zielgeometrie eingerechnet.

`next` verwendet die auf acht unterschiedliche Schlüssel begrenzte, per Mutex gesicherte Liste je Ansicht. Nur der Anzeige-Thread übernimmt die Wünsche. Zusätzlich zu den P4c-Dateien wurde `plugin_schedule.h/.cpp` für den ausdrücklich beauftragten 4b-Reviewpunkt geändert: ein wartendes Wechselfeld behält `pending_next` bis zur Ausführung. `first_available` behält eine manuelle Auswahl bis zur nächsten Änderung der verfügbaren Einträge; ohne Tap bleibt die automatische Auswahl erhalten, auch wenn ein anderer Übergang sie verzögert. Reine Regressionen decken diese Fälle ab.

Build: VS Build Tools 2022, `vcvars64.bat`, CMake/Ninja Release in **`cpp/build/store`** mit bestehender vcpkg-Konfiguration. Bekannte `vswhere.exe`-Warnung ohne Abbruch, PATH-Anführungszeichen wie in `build.bat` bereinigt. Ein zwischenzeitlicher Linkversuch traf eine noch laufende Snapshot-Test-EXE (LNK1104); nach ihrem Ende erfolgreich neu gebaut. Kein neuer Testtarget, deshalb keine Änderung an `CMakeLists.txt`, `build.bat` oder `run_release_checks.py`; letzteres wurde **nicht gestartet**.

Einzeln gestartete Suiten, jeweils **0 Fehler**:

| Suite | Prüfungen |
|---|---:|
| `fl-plugin-tests` | 292 |
| `fl-plugin-zones-tests` | 462 + 2 feste Fuzz-Läufe à 4.000 |
| `fl-plugin-schedule-tests` | 845 |
| `fl-plugin-example-tests` | 124 |
| `fl-plugin-store-tests` | 146 |
| `fl-plugin-transport-tests` | 140 |
| `fl-plugin-sign-tests` | 111 |
| `fl-plugin-catalog-tests` | 77 |
| `fl-plugin-snapshot-tests` | 1.859 |
| `fl-device-profile-tests` | 85 |
| `fl-tmdb-tests` | 119 |
| `fl-psn-tests` | 37 |
| `fl-key-help-tests` | OK (keine Anzahl ausgegeben) |
| `fl-amp-tests` | 13 (rein synthetisch, keine Wiedergabe) |
| `fl-monitor-tests` | 11 (ohne Fenstermodus) |
| `fl-companion-tests` | 377 (lokaler Testserver, keine Geräte) |
| `fl-tests plugin-touch` | 2.083 (davon 29 neue Plugin-Touch-Prüfungen) |
| `installer_preserves_config.py` | 2 isolierte Prüfungen |
| `fl-distribution-tests standalone` | Exit 0 |
| `fl-plugin-validate --store cpp/examples/plugins` | 6 Manifeste gültig |

Die sechs Beispielmanifeste und ein aus dem Doku-Ausschnitt vervollständigtes synthetisches Tap-Manifest sind offline gültig. Python-Syntax, lokale Links der betroffenen Doku und `git diff --check` geprüft. Die Codeänderungen benötigen kein neues Testziel.

Die Vollsuiten **`fl-tests` und `fl-plugin-settings-tests` sind nicht gelaufen** (bekannte DPAPI-Sandbox-Grenze). Der neue Modus `fl-tests plugin-touch` führt ausschließlich die vorhandenen Touch-/Systemziel-Regressionen und die neuen reinen Plugin-Touch-Tests aus, ohne DPAPI, Netzabruf oder echte Linkstarts. Er ersetzt die Vollsuite nicht.

Mehrgeräte-Prozessprüfung `app_companion_multi.py cpp/build/store/ScreenBuddy.exe --tap-only`: **blockiert vor Kopplung**, Companion meldet im isolierten Testlog „Verschlüsselung fehlgeschlagen“, anschließend Status-Timeout; Prozess sauber beendet. Damit sind die neuen Prozess-Akzeptanzprüfungen (verweigerter Link ohne Navigation auf A/B, normale Navigation außerhalb der Zielrechtecke) hier **nicht ausgeführt**. `app_companion_integration.py`: Fall mit ausgeschaltetem Companion durchlaufen, aktivierter Companion am gleichen Start-Timeout blockiert; gesamte Suite nicht grün. Die reinen Mehransicht-Tests und die Companion-Suite sind separate Nachweise, kein Ersatz dafür.

Keine Hardware/COM-Ports, keine echten Browserlinks und keine Töne. Linkstarts in Tests ausschließlich injiziert; die Prozessprüfung besitzt keine Linkfreigabe. Alle Prozessprüfungen verwenden isolierte Daten/Instanzen und freie Ports mit Ausschluss von 8765. Installerprüfung ebenfalls nur in isolierten Testordnern mit `--no-lcd`, ohne echten Autostart/Startmenüeingriff. Manifest v1 und alle bestehenden Snapshot-Hashes unverändert: `hashes.txt` SHA-256 **3039AF0D01635BC21F06FE458523014B8929ABC814A9338AD53E67915963ACA2**. Keine neue Snapshot-Basis geschrieben.

Offen: native UI-Sichtprüfung und physische 4c-H1-Abnahme durch Basti (Monitor/Handy, Freigabe, Ruhezeit). Die übrigen bekannten 4b-Punkte (mehrfache Zonengeometrie, Warnung bei absichtlich leerem Eintrag, Seriell-Lockverschachtelung) bleiben offen. Keine Veröffentlichung, kein EXE-Export, Commit, Push oder Branchwechsel; Branch bleibt `feature/zonen-layout`.


## Phase-4b-Reviewkorrekturen (06.11.2026)

Serieller Vorhang: Budget umfasst Bandübertragung, 50 ms Ruhe und gemessene Renderzeit; maximal ein Band-Schritt je Aufruf, bei Verspätung wird die Zeitbasis verschoben. Nur Companion darf verspätete Zwischenschritte verwerfen. Monitor bleibt beim erreichten Zwischenbild. Rotation überspringt einen datenlos gewordenen Eintrag vorwärts. Globale Bandaufteilung auch bei Kachelversatz. Reine Tests für first_available-Überblendung, Generations-/Größenreset, Parsergrenzen und altes Skip-Prädikat ergänzt. Der Prozessprüfer verwirft Start-/Größenbilder über eine neue bestätigte Auflösung, nutzt das erste Plugin-JPEG nur als Basis, prüft Navigation während der gesamten Beobachtung und führt zusätzlich ein statisches Plugin ohne cycle als Negativkontrolle aus.

Prüfergebnisse dieses Korrekturlaufs (jeweils eigene Suite, 0 Fehler): Zeitplan **836**, Parser **271**, Snapshots **1.811**, Zonen **462** (plus zwei feste Fuzz-Läufe à 4.000), Beispiele **124**, Speicher **127**, Transport **140**. Release-Build mit `vcvars64.bat`/CMake in `cpp/build/store` erfolgreich; bekannte `vswhere.exe`-Warnung ohne Abbruch nach Bereinigung der PATH-Anführungszeichen wie in `build.bat`. Python-Syntax und `git diff --check` grün.

Prozessprüfung `app_companion_multi.py … --cycle-only`: **blockiert vor Kopplung**, isolierter Companion startet nicht; synthetisches Testlog meldet „Verschlüsselung fehlgeschlagen“ (bestehende DPAPI-Sandbox-Grenze), danach Status-Timeout. Prozess beendet sich sauber. Weder positive JPEG-Prüfung noch Negativkontrolle sind damit hier abgenommen. Reine Suiten ersetzen diesen Prozessnachweis und die physische Abnahme nicht.

`hashes.txt` in diesem Korrekturlauf nicht geändert (SHA-256 `3039AF0D01635BC21F06FE458523014B8929ABC814A9338AD53E67915963ACA2`); alle vorhandenen Snapshot-Vergleiche grün. Keine Töne, keine Hardware, kein Port 8765, kein `run_release_checks.py`, kein Export/Release und kein Commit/Push/Branchwechsel.

## Phase-4b-Review: bekannte offene Punkte (06.11.2026)

Diese Punkte sind dokumentiert und in diesem Korrekturlauf bewusst unverändert:
- Zonengeometrie wird dreifach berechnet (`app.cpp`: `zone_rows`, sowie Geometrie-/Zeichenpfade in `plugin_render.cpp`). Eine gemeinsame Berechnung bleibt offen.
- Die Seitenwarnung „Plugin-Daten fehlen“ erscheint auch bei einem absichtlich leeren Eintrag.
- Ein `next`-Wunsch konnte in einer wartenden Zone verloren gehen; **in Phase 4c behoben**, siehe aktueller Abschnitt oben.
- Lock-Verschachtelung: `render::mutex` → `SerialDisplay::mu_`; separater Seriell-Thread bleibt außerhalb von Phase 4.

## Phase 4b – P4b-0 bis P4b-4 (06.11.2026)

Implementiert: Bandstatistik und rein simulierte Messstrecke, Schema-2-Parser `cycle`/`seconds` und Fehlend-Kennzeichen je Block, reines `plugin_schedule` mit injizierter Uhr, Auswahl/Überblendung/Band-Vorhang mit Bildcache, Fristen und Zustände je sichtbarer Ansicht. Clean-Room-Zeilen für alle fünf Pakete ergänzt. Ohne Wechselfeld prüft eine feste Referenzliste über 60 simulierte Sekunden die bisherige Frame-/Wartefolge; keine Änderung an Manifest v1 oder den **180 bestehenden Snapshot-Hashes**, **168 neue Snapshots**.

Build: VS Build Tools 2022 + `vcvars64.bat`, CMake/Ninja Release in `cpp/build/store` mit vorhandener vcpkg-Konfiguration. `vswhere.exe`-Warnung ohne Buildabbruch. `build.bat` und `run_release_checks.py` nur um das neue Ziel ergänzt, **nicht gestartet**; kein Export, Signieren, Commit, Push oder Branchwechsel.

| Einzeln ausgeführte Suite | Ergebnis |
|---|---|
| `fl-tests transfer-sim` (nur dieser Teilmodus) | **10 grün**, simulierte Strecke 1.000.000 Byte/s; EWMA, unbekannte Rate, keine Änderung, benachbarte Bänder und verkürztes Schlussband. Kein `Output::connect`, kein COM-Port. |
| `fl-plugin-schedule-tests` | **566 grün**: Rotation, Datenlage, Quantisierung, Verspätung, Idempotenz einschließlich wartender Zone, getrennte Geräte, 600-ms-Wartegrenze, Mindestdauer, Budgets und Frame-/Wartefolge. |
| Parser / Zonen / Beispiele / Speicher / Transport / Signatur / Katalog | **269 / 462 / 124 / 127 / 140 / 111 / 77 grün**, jeweils 0 Fehler. Zonen zusätzlich zwei feste Fuzz-Läufe à 4.000 Manifesten. Schema 2 verlangt im Store weiterhin mindestens 2.10.0. |
| `fl-plugin-snapshot-tests` | **1.808 grün**, 0 Fehler; alle vorhandenen Hashes vor Anhängen der neuen Einträge verglichen. Alle Themes, Quer/Hoch, normale/kleine Größen, beide Einträge, leer, fade 1/2, reveal 2/4; reine Pixel-/Bandkostenprüfung. Warnungen stammen aus absichtlich injizierten Zeichenfehlern der bestehenden Isolationstests. |
| Geräteprofil / Companion / TMDB / PSN / Amplifier / Monitor | **85 / 377 / 119 / 37 / 13 / 11 grün**; Amplifier ausschließlich reine Logik, Monitor ohne Fenstermodus. |
| Schlüsselhilfe / Distribution (unpackaged) | **OK / Exit 0**, kein Store-Pakettest. |
| Installer / Navigation-API | **2 / 51 grün**, isolierte temporäre Instanzen mit `--no-lcd`. |
| `fl-tests` vollständig / `fl-plugin-settings-tests` | **Nicht gelaufen**, bekannte DPAPI-Grenze dieser Sandbox. Teilmodus `transfer-sim` ist kein vollständiger Kernprüflauf. |
| Neuer Zweigeräte-Prozesstest / bestehende Companion-Integration | **Nicht abgenommen**: isolierte App startet und beendet sich, Companion scheitert vor Kopplung mit „Verschlüsselung fehlgeschlagen“ (DPAPI). Der neue Test prüft zwei geänderte JPEGs binnen `seconds + 2 s` und unveränderte Navigation; diese Kriterien sind hier nicht durchgelaufen. Aufruf: `python cpp/tests/app_companion_multi.py cpp/build/store/ScreenBuddy.exe` (optional `--cycle-only`). |
| Native Settings Smoke | **Nicht grün**: vorhandene Navigationsliste enthält zusätzliche Einträge gegenüber der Testkonstante `NAV`; Abbruch vor weiteren UI-Prüfungen. Native Mouse-/Pairing-Smokes nicht ausgeführt; keine native Sichtabnahme. |

Release-Messung, ausschließlich reine Zeichnung (nicht der vollständige `render_core`): 480×320 0,47 ms, 1920×720 2,33 ms, 2400×1080 4,24 ms. Übergangsschritt aus Cache (Mittel aus 10): **0,51 / 5,24 / 9,73 ms**. Das erfüllt die Cache-Budgets auf diesem PC; die App-Gesamtzeit und physische Darstellung bleiben wegen der blockierten Prozessabnahme offen. Timing wird berichtet, nicht als harter Debug-Test gewertet.

### Messung serielle Rate (offen, Basti)

**Keine reale serielle Rate gemessen. Kein echtes Display angesprochen, kein COM-Port geöffnet.** Nur simulierte Bandstrecke und Kostenfunktionen geprüft. Die laufende serielle Ausgabe erfasst künftig Payload-Byte/s als EWMA für Teilupdates ab acht Zeilen und die letzte tatsächliche Übertragungsdauer. Leere Diffs überschreiben diese Dauer nicht. Diagnoseformat: „Zeilen, ms, kB/s“ ohne Nutzdaten. Ohne gemessene Rate bleibt der Übergang ein Schnitt.

Basti misst vor Release am echten Gerät die Rate R und trägt sie hier ein; außerdem Monitor, Companion und seriellen Vorhang/Schnitt auf ruhige Darstellung, Flackern und Streifen prüfen. Die Simulation ersetzt diese Abnahme nicht. Keine Töne, keine Hardwarekontakte, keine Tests gegen Port 8765. Physische Tests bleiben offen.


## Phase-4a-Reviewkorrekturen (06.11.2026, separater Prüflauf)

Alle fünf Review-Befunde bearbeitet: Teilpatch erhält aktuelle Ränder (beide Konfigurationsabschnitte); DE/EN-Gerätekarte erklärt bei Synchronisierung die unwirksamen eigenen Ränder und die gemeinsame Vorgabe; `tap_at` führt `step_page` nach Freigabe von `tap_mu_` aus; Android-KDoc und Importe bereinigt. `place`/`compose_view` begrenzen auch übergroßen Inhalt auf die Nutzfläche.

Build erneut erfolgreich: `vcvars64.bat`, bereinigter PATH wie `cpp/build.bat`, CMake/Ninja Release in `cpp/build/store`, vorhandene vcpkg-Konfiguration, `VCPKG_MANIFEST_MODE=OFF`. Die `vswhere.exe`-Warnung verhindert den erfolgreichen Build nicht. Kein Export/Signieren gestartet.

| Suite einzeln gestartet | Ergebnis dieses Review-Laufs |
|---|---|
| `fl-device-profile-tests` | **85 grün**: Teilpatch/leeres Patch, gemeinsamer Betrieb wählt `companion.keepout` trotz eigenem Gerätewert (produktiver reiner Auswahlhelfer), Orientation 4/255, physisch drehendes Hochformat, h=119 und w=159 getrennt, Kantenbegrenzung, kleinerer/größerer Inhalt mit vollständigem Pixelvergleich. |
| `fl-companion-tests` | **377 grün**, 0 Fehler; zusätzlich echtes Laden synthetischer `companion.json` mit Keepout als String, Array, negativer Zahl und beschädigten Objekt-Rändern (ignoriert/geklammert). |
| `fl-plugin-snapshot-tests` | **1.186 grün**, 0 Fehler; unveränderte bestehende Snapshot-Hashes, keine Aktualisierung der Baseline. |
| Plugin Parser / Zonen / Beispiele / Store / Transport / Signatur / Katalog | **251 / 462 / 119 / 127 / 140 / 111 / 77 grün**, jeweils 0 Fehler; Zonen zusätzlich beide festen Fuzz-Läufe mit je 4.000 Manifesten. |
| `fl-plugin-settings-tests` | **Nicht grün**, unverändert DPAPI: `initial_save`, „Plugin settings could not be encrypted or saved.“ |
| `fl-tests` | **Nicht grün**, unverändert DPAPI: Abbruch in `config`, „Verschlüsselung fehlgeschlagen“. |
| `app_companion_multi.py` | **Nicht abgenommen**: isolierte App startet und beendet sich sauber; Companion scheitert vor der Kopplung an DPAPI. Die neuen Kontrollen (A Inhalt bis zum Rand bei 0, B Versatz 48 ±2 JPEG-Pixel und gleichförmige Streifen ±8, separate Randtipps links/oben/rechts/unten) sind implementiert, aber nicht als App-Test durchgelaufen. |

Die JPEG-Auswertung separat mit zwei synthetischen JPEGs (0/48 Pixel Rand) geprüft: beide Kontrollen grün, erkannter Inhaltsbeginn 0/47 (zulässige JPEG-Toleranz ±2); dies ist **kein** Ersatz für den blockierten App-Prozesstest. Python-Syntax und `git diff --check` erfolgreich geprüft. Kein neuer Android-Build für die rein kosmetischen Kotlin-Änderungen; frühere Android-Prüfstände unten bleiben historisch.

Grenzen eingehalten: Clean-Room, keine Töne/Hardware, kein Port 8765 benutzt, `run_release_checks.py` nicht gestartet; kein Commit, Push oder Branchwechsel. Native UI-Sichtprüfung und physische Geräteabnahme bleiben offen. Die DPAPI-Blocker werden weder umgangen noch als grün ausgewiesen.

## Phase 4a – Geräteprofile (06.11.2026, lokaler Entwicklungsstand)

P4a-1 → P4a-2 → P4a-3 → P4a-4 nacheinander implementiert auf `feature/zonen-layout`; kein Commit, Push oder Branchwechsel. Kein Fremdcode eingesehen oder in die Implementierung übernommen. Keine Hardwareausgabe, Installation, Signierung, Veröffentlichung oder Tonerzeugung. `run_release_checks.py` wurde **nicht gestartet**, nur die neue Suite in dessen Liste registriert. Tests verwenden keine Verbindung zu Port 8765 (vorhandene reine Zahlen-/URL-Validierungen bleiben bestehen).

Build: VS Build Tools 2022, `vcvars64.bat`, CMake aus `Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`, Ninja Release in `cpp/build/store`. `ScreenBuddy` und alle unten genannten C++-Testziele erfolgreich gebaut. Vorhandener vcpkg-Cache des Build-Verzeichnisses verwendet. Die Meldung zu fehlendem `vswhere.exe` verhinderte weder Compilerinitialisierung noch Build.

| Suite (einzeln gestartet) | Ergebnis dieses Laufs |
|---|---|
| `fl-device-profile-tests` | **69 grün**; vier asymmetrische Drehtabellen, Begrenzung/Rückfall, Innen-/Außenkanten, Inhalt-Koordinaten, Drittel-Regel, drei Geräteprofile, Zielversatz, Nullmaske und Konfigurationsvalidierung beider Abschnitte, TOML-Speichern/Laden und alte Konfigurationen ohne Sperrzone. Eine Prüfung vergleicht alle **153.600 Pixel** eines komponierten Bildes. |
| `fl-companion-tests` | **369 grün**, 0 Fehler; eigener Sperrzonen-Store, ungültige Werte/ID, Persistenz, Rücksetzen und ältere Geräte ohne Feld eingeschlossen. TLS-Prozess-/Mehrgeräte-Simulation der vorhandenen Suite; keine echten Geräte. |
| `fl-tests` | **Nicht grün**: Abbruch in `config` mit „Verschlüsselung fehlgeschlagen“ (Windows-DPAPI in dieser eingeschränkten Umgebung). Keine vollständige Prüfzahl behauptet. |
| `fl-plugin-tests` | **251 grün**, 0 Fehler |
| `fl-plugin-zones-tests` | **462 grün**, 0 Fehler; zusätzlich Ausgabe der beiden festen Fuzz-Läufe mit je 4.000 Manifesten |
| `fl-plugin-example-tests` | **119 grün**, 0 Fehler |
| `fl-plugin-store-tests` | **127 grün**, 0 Fehler |
| `fl-plugin-transport-tests` | **140 grün**, 0 Fehler |
| `fl-plugin-settings-tests` | **Nicht grün**: `initial_save`, „Plugin settings could not be encrypted or saved.“; gleicher DPAPI-Blocker. |
| `fl-plugin-sign-tests` | **111 grün**, 0 Fehler; nur kryptografische Testdaten, keine Auslieferung signiert |
| `fl-plugin-catalog-tests` | **77 grün**, 0 Fehler |
| `fl-plugin-snapshot-tests` | **1.186 grün**, 0 Fehler; bestehende Hashdatei bytegleich zu `HEAD`, keine Baseline neu geschrieben |
| Android `:protocol:test` / `ProtocolTest` | **23 grün**, 0 Fehler/übersprungen; TV ignoriert alle gespeicherten Ausrichtungen, Handy reicht sie unverändert weiter. Gradle 8.11.1 offline, lokaler beschreibbarer Cache, `--configure-on-demand`; Kotlin-Daemon fiel auf Kompilierung ohne Daemon zurück. |

**Mehrgeräte-App-Prozesstest nicht abgenommen:** `python cpp/tests/app_companion_multi.py cpp/build/store/ScreenBuddy.exe` wurde mit zufälligen, explizit ausgeschlossenen Standardports, `FUCHSLABS_INSTANCE` und `--no-lcd` gestartet. Die isolierte App beendet sich sauber, der Companion-Start scheitert aber vor der ersten Kopplung an DPAPI („Verschlüsselung fehlgeschlagen“). Daher sind die neu ergänzten 2400 × 1080 / 1920 × 1080-, JPEG-Rand- (±8) und Randtipp-Prüfungen **noch nicht erfolgreich durchlaufen**. Das Skript nutzt frische Testverzeichnisse und Windows-Bilddekodierung ohne zusätzliche Python-Pakete. Keine App-Test-Hooks oder Änderungen an der Verschlüsselung eingebaut.

**Android-Build nicht abgenommen:** Companion bleibt bei der bereits vorhandenen Entwicklungsversion **1.0.4 / Code 8**. `Build-Companion.ps1 -SigningProperties ''` verweigert bestimmungsgemäß ohne private Signierdaten den Start; dieser Auslieferungsweg lässt sich mit der Paketvorgabe „keine Signierung“ nicht erfolgreich verwenden. Als unsignierter Entwicklungscheck wurden `:app:lintRelease :app:assembleRelease` direkt offline versucht. Der vollständige Build scheitert am nicht zugänglichen Android-SDK (`C:/Users/admin/AppData/Local/Android`: Zugriff verweigert; Gradle meldet SDK location not found). Kein neues APK als gebaut, signiert oder installiert ausgegeben. JVM-Protokolltests konnten separat erfolgreich laufen.

**Noch offen:** vollständige grüne Läufe von `fl-tests`, `fl-plugin-settings-tests` und dem App-Mehrgerätetest in einer Umgebung mit funktionsfähigem DPAPI; Android-Lint/APK-Build mit zugänglichem SDK; native UI-Sicht-/Tastaturprüfung einschließlich aktualisierter Vorschau. Physische Abnahme durch Basti: 4a-H1 (Gehäuserand in allen vier Ausrichtungen, Landscape/Gegenrichtung bestätigen) und 4a-H2 (TV bleibt quer, Overscan passt). Automatisierte Geometrie-/Pixelprüfungen ersetzen diese Abnahme nicht.

DE/EN-Dokumentation: Navigation und Companion-Anleitung, Android-Buildhinweise, beide Haupt-Changelogs und C++-Changelog ergänzt; Clean-Room-Tabelle enthält eine Zeile je Paket (Agent `Codex CLI 0.166.0`). Lokale Dateilinks, Python-Syntax und `git diff --check` geprüft. Manifest v1/Plugin-Parser, Protokollschema, Versions-/Downloadlinks und historische Release-/Testeinträge wurden nicht umgeschrieben. Keine Änderung an bebilderten HTML-Anleitungen in diesem Paket.

### English validation status

P4a-1 through P4a-4 are implemented in order, with no commit, push, branch switch, device output, audible sound, signing or distribution. C++ Release builds pass in `cpp/build/store`; the table above records each separately invoked suite. Device profile: **69 checks** (including a full 153,600-pixel comparison), Companion: **369**, Android ProtocolTest: **23**. Eight of the ten requested existing suites pass. Core and plugin settings tests are blocked by Windows-DPAPI encryption failures, as is the application-level multi-device test before pairing. Existing snapshot hashes remain byte-identical. The APK/lint build is blocked by Android SDK access; the delivery script requires signing credentials and was not used to sign anything. Native UI visual/keyboard acceptance and physical orientation/TV overscan checks remain open. This is not a fully accepted release.

## Render-Snapshots (2.10.0)

Testgerüst für das Zonen-Layout (Phase 0): `fl-plugin-snapshot-tests` rendert alle Beispiel-Plugins aus `cpp/examples/plugins` mit `plugins::draw_page` (feste Beispieldaten, Uhr 12:34, Datum 2026-01-01, Theme `dunkel` als Referenz in Querformat 480x320 und Hochformat 320x480 sowie (Phase 1) die Theme-Matrix hell, windows (feste Einstellungen, inline), custom, custom-cupertino, custom-fluent, custom-material nur im Querformat mit Präfix `<theme>.`; bei Plugins mit Quellen zusätzlich die Variante „Daten fehlen“). Offline, ohne Netzwerk, ohne Port 8765, ohne Ton. Basis: `cpp/tests/plugin-snapshots/` (`hashes.txt` = FNV-1a-64 über Größe und Pixel, PNGs zur Sichtprüfung).

- Vergleichen (Standard): `build/store/fl-plugin-snapshot-tests.exe` (Exit 1 bei Abweichung; Ist-Bilder landen in `%TEMP%\fl-plugin-snapshot-actual`).
- Basis neu schreiben: `build/store/fl-plugin-snapshot-tests.exe update` (danach Diff der PNGs prüfen und bewusst committen).
- Bauen: `cmake --build build/store --target fl-plugin-snapshot-tests`; ist in `build.bat` und `tests/run_release_checks.py` eingebunden.
- Stand (Phase 1, Rahmenstil aus `theme::Design`): 32 Snapshots, 185 Prüfungen bestanden, 0 fehlgeschlagen. Die acht dunkel-Hashes wurden bewusst neu erzeugt (Kartenradius 9 → 8). Die Hashes beruhen auf Direct2D/DirectWrite; andere Schriftarten, Treiber oder Skalierung können sie ändern, dann bewusst mit `update` neu erzeugen.

## Phase 2b Zonen-Layout: Zeichnen nach Zonen (2.10.0, Entwicklungsstand, nicht committet)

- Umfang: `draw_page` zeichnet v2-Seiten mit `layout` nach Zonen (Dichtestufen, Clip pro Block, Blöcke ohne Platz entfallen). v1 und v2 ohne `layout` zeichnen pixelidentisch wie bisher: alle bestehenden v1-Snapshot-Hashes unverändert (nur `offline-zonen` ist neu, 28 neue Zeilen in `hashes.txt`; keine bestehende Zeile geändert).
- Zonen-Snapshots: das Beispiel `offline-zonen` in vier Größen (480×320, 320×480, 240×160, 160×240) und allen sieben Themes (dunkel, hell, windows, custom, custom-cupertino/-fluent/-material) = 28 Bilder; Snapshot-Basis insgesamt 140 Bilder (vorher 112 mit den Phase-1-Beispielen). Die 14 kleinen Bilder (240×160, 160×240) wurden nach den Codex-Korrekturen neu erzeugt (`update`): Bei Skalierung 0,5 wird die Schrift nicht mitskaliert, deshalb werden Schwellen und Zeilenmaße mit 1/Skalierung multipliziert (vorher überlappten/schnitten Zeilen), die 14 großen Bilder blieben bytegleich.
- Neue Suiten und Prüfungen: `fl-plugin-zones-tests` (Geometrie, Parser, Dichtestufen, Fuzz mit 4000 Manifesten) und Erweiterungen in `fl-plugin-snapshot-tests`: v2-ohne-layout gegen v1 pixelgleich in allen Themes/Größen, Stress-Seite (12 Blöcke, 300-Zeichen-Texte) ohne Pixel in den Seitenrändern, `Canvas::clip_push/clip_pop`, umgekehrte Clip-Grenzen vor der Rundung (Skalierung 0,5/1/2, nur x/nur y umgekehrt, gültiger Clip bleibt sichtbar), Ausnahme-Unwinding der RAII-Klammer, Liste ausführlich ohne Bezeichnung mit 5 langen Einträgen in einer 136×82-Zone („+N“ überlagert den ersten Eintrag nicht; die Prüfung schlägt ohne Korrektur fehl, wurde so gegengeprüft), Ausblenden statt Überlappen bei Skalierung 0,5, Zeilenabstand bei 240×160.
- Stand einzeln gebaut (cmake/vcvars64, `cpp/build/store`) und gelaufen: `fl-tests` 3327, `fl-plugin-tests` 251, `fl-plugin-zones-tests` 256, `fl-plugin-example-tests` 85, `fl-plugin-store-tests` 127, `fl-plugin-transport-tests` 140, `fl-plugin-settings-tests` 135, `fl-plugin-sign-tests` 111, `fl-plugin-catalog-tests` 77, `fl-plugin-snapshot-tests` 914 Prüfungen; jeweils 0 fehlgeschlagen. Nicht ausgeführt: `run_release_checks.py`, Installer-/Release-Prüfungen, TMDB/Monitor/Companion/Amp/PSN/Key-Hilfe (nicht berührt), Port 8765, Töne.
- Grenzen: Die Hashes beruhen auf Direct2D/DirectWrite dieses Rechners; andere Schriftarten oder Treiber können sie ändern. Keine physische Display-Abnahme; die kleinen Bilder wurden nur am Bildschirm (zwei PNGs) gesichtet.
## Phase 3 Zonen-Layout: Bedingte Anzeige und Fehlerisolierung (2.10.0, Entwicklungsstand, nicht committet)

- Umfang: `show_if` an v2-Widgets (Vorhanden-Test oder ein Vergleich gegen ein festes Literal, keine Ausdruckssprache, Secrets verboten), Auswertung pro Block abgesichert (`render_page`), Zeichnen pro Block/Karte abgesichert (`draw_page`, Fläche wird wieder Hintergrund, Clip-Stapel exakt wiederhergestellt), ratenbegrenzter Log-Eintrag ohne Daten. Schema 1 unverändert.
- Wertzustands-Tabelle (`fl-plugin-zones-tests`, `test_condition_table`): 20 Wertzustände (fehlend, `null`, nicht endlich, `""`, `[]`, `{}`, `false`, `true`, `0`, `-0.0`, 2.5, −1, 2^64−1, `"0"`, `"abc"`, `"ABC"`, `" "`, `"true"`, `[1]`, `{a:1}`) × 11 Bedingungen; dieselbe Tabelle steht in `docs/PLUGIN-SYSTEM.md`.
- Parser: 10 gültige Formen an den Grenzen (128 Zeichen, 8 Segmente, 256-Zeichen-Literal), 44 abgelehnte Formen jeweils mit exakter fester Meldung, Secrets (3 Formen, eigene Meldung), v1 mit `show_if` abgelehnt, doppelte Schlüssel und `1e999` in `show_if`, ein ungültiges `show_if` lehnt das ganze Manifest ab. Zweiter Fuzz (fester Seed 20261107, 4000 Manifeste, 345 angenommen, 1120 bedingte Blöcke) mit zufälligen Quell- und Einstellungsdaten: keine Ausnahme, kein angenommenes Secret, keine Ausdrücke; der bestehende Fuzz ist unverändert.
- Vorschau ohne Daten: Beispieldaten (Vorschau), keine Antwort (live, „Plugin-Daten fehlen“), Antwort ohne Felder (ausgeblendet, Fußzeile normal), Quelle ohne `sample`, ganze Quelle als Bedingung, ungültige Einstellungen; Zone ohne sichtbaren Block bleibt leer. Snapshots `bedingt-quelle.sample/missing/partial` in vier Größen.
- Fehlerinjektion (nur über Test-Hooks `plugins_testing.h`/`plugin_render_testing.h`, App ohne Hook): Daten – jeder Block einzeln, `std::exception`, Nicht-std-Ausnahme, `bad_alloc` für alle Blöcke, v1-Schlüssel `#0`; Zeichnen – vier Blöcke von `offline-zonen` (erster, gestapelter, einzelner, letzter) und alle vier Karten des v1-Rasters `offline-focus`: außerhalb der Blockfläche pixelgleich zur Referenz, innerhalb nur Hintergrund; Block mit offen gelassenen Clips plus Ausnahme = gleiches Bild wie ohne Clips; offen gelassener Clip ohne Ausnahme stört keinen anderen Block; alle Blöcke werfen = Seite ohne Blöcke; Hook ohne Auslösung = pixelgleich. Log: genau ein Eintrag je Block und Stufe bei drei Zeichnungen, kein Ausnahmetext, keine Daten.
- Codex-Review Phase 3 eingearbeitet: Clip auch im v1-Raster (Karte plus 1 logisches Pixel; alle Hashes unverändert), Fehlerinjektion zusätzlich mit Wurf nach dem gesamten Inhalt (`after_content`) bei Skalierung 0,5, 1 und 2 für alle vier Rasterkarten (beide Zeitpunkte) und alle sichtbaren Zonenblöcke; äußere Fehlergrenze von `render_page` (Fehler außerhalb der Blöcke, `bad_alloc` und Nicht-std-Ausnahme: Seite ohne Blöcke, Daten fehlen, ID/Titel bleiben). Hinweis: Dass der Test ohne Raster-Clip fehlschlägt, wurde nicht per Mutation gegengeprüft (die Beispieltexte ragen bei 0,5 nicht sichtbar über die Karte).
- Snapshots: alle 140 bestehenden Hashes unverändert (`git diff`: nur 40 hinzugefügte Zeilen in `hashes.txt`, keine bestehende PNG geändert); neu `offline-bedingt` (7 Themes × 4 Größen) und `bedingt-quelle` (3 Varianten × 4 Größen, dunkel).
- Stand einzeln gebaut (cmake/vcvars64, `cpp/build/store`) und gelaufen: `fl-tests` 3327, `fl-plugin-tests` 251, `fl-plugin-zones-tests` 462, `fl-plugin-example-tests` 119, `fl-plugin-store-tests` 127, `fl-plugin-transport-tests` 140, `fl-plugin-settings-tests` 135, `fl-plugin-sign-tests` 111, `fl-plugin-catalog-tests` 77, `fl-plugin-snapshot-tests` 1186 Prüfungen; jeweils 0 fehlgeschlagen. `fl-plugin-validate --store` für `offline-bedingt`: OK. Nicht ausgeführt: `run_release_checks.py`, Installer-/Release-Prüfungen, TMDB/Monitor/Companion/Amp/PSN/Key-Hilfe (nicht berührt), Port 8765, Töne, Hardware.
- Grenzen: nur C++-Ausnahmen werden isoliert (keine SEH-Zugriffsverletzungen); keine physische Display-Abnahme; Hashes rechnerabhängig (Direct2D/DirectWrite).
## Release 2.9.5 – 01.11.2026

- Basis main nach PR #50; Nutzerkorrektur Farben & Design nach Ansicht in Seitenliste und Material-Zuordnung eingearbeitet. Theme-Store lädt asynchron; sichtbare Überlagerung der Liste mit den Knöpfen reproduziert und durch relayout nach fill_store behoben. Codeprüfung der Änderungen durchgeführt; Signatur-/Hash-/Schema-Schutz unverändert.
- Alle 14 Suiten bestanden: 3327 Kern, 119 TMDB, 251 Plugins, 41 Beispiele, 127 Store, 140 Transport, 135 Settings, 11 Monitor, 361 Companion, 111 Signatur, 77 Katalog, 13 Amp, 37 PSN und Key-Hilfe OK. Zwei Installerchecks; App-Integration 39 und Mehrgeräte 17 grün.
- Meanmachine im normalen Windows-Kontext nach expliziter Freigabe installiert; EXE-Hash identisch und Konfiguration beim Upgrade erhalten. Vorherige Installation privat gesichert. Temporäre Theme-/Sprachänderungen zurückgestellt.
- Pixel, Redmi und TV gleichzeitig verbunden. Beide Handys per injiziertem Wischen, TV per Pfeil rechts jeweils genau einen Seitenwechsel geprüft. Erste Pixel-Prüfung wartete nur 350 ms auf die Metadaten und scheiterte; nach dem auch im Integrationstest verwendeten Warten auf tatsächliche Änderung alle drei grün. Nutzer bestätigt Pixel-Wischen zusätzlich physisch. Gespeicherte Pixel-Kopplung konnte wiederverwendet werden; keine Altgeräte gelöscht.
- Cupertino/Fluent/Material nativ Hell/Dunkel angesehen, echte Companion-Ausgaben aufgenommen; Navigation DE/EN geprüft. Kontenfreie eigene Store-Instanz: Cupertino installiert, automatische Auswahl und Vorschauwechsel, Speichern/Neustart und persistente Auswahl bestätigt. Testinstanz beendet.
- 16 native Menü-/Profilbilder während Tests aufgenommen, danach opak anonymisiert; Rohbilder nur im ignorierten Buildordner. Erste Bildserie wegen veraltetem Screenshot-Bezug verworfen und neu aufgenommen; Kontaktübersicht danach auf richtige Seite und private Angaben geprüft. Bildmanifest unter docs/screenshots/menu-2.9.5-manifest.json. Display-Demos aus aktuellem Renderer mit Beispieldaten. Vorlagen/Ressourcen synchron, JavaScript-Syntax und 219 lokale Dokumentationslinks geprüft, neuer Abschnitt DE/EN im Browser geprüft.
- Grenzen: keine neue Kamera-/Fingerprüfung des seriellen Panels, keine serielle Bildrate, keine physische TV-Fernbedienung oder Screenreader-Abnahme. Android unverändert, APK 1.0.3 wiederverwendet. Keine privaten Rohbilder, Configs, Schlüssel, Kopplungsdaten oder Logs im Commit.


## Release 2.9.4 – 31.10.2026

- Release-Build erfolgreich: 3327 Kernprüfungen, 119 TMDB, Key-Hilfe OK, 37 PSN, 13 Amp, 251 Plugins, 41 Beispiele, 127 Store, 140 Transport, 135 Settings, 111 Signatur, 77 Katalog, 11 Monitor, 361 Companion; zwei isolierte Installerprüfungen, 39 App-Integrations- und 17 Mehrgeräteprüfungen bestanden.
- Meanmachine von 2.9.3 auf 2.9.4 aktualisiert; EXE/Config/Kopplungen vorher lokal gesichert. Konfiguration beim Update erhalten und nach temporären Testseiten bytegenau zurückgestellt; USB-Display verbunden. Spotify-Token wurde regulär aktualisiert; Kopplungsdatei blieb beim Update unverändert.
- Redmi 8 hatte einen alten Testserver gespeichert. Nach ausdrücklicher Zustimmung des Nutzers neu mit Meanmachine gekoppelt, Fingerprint abgeglichen. Reale ADB-Tipps auf den angezeigten Film und die Serie öffneten jeweils die richtige TMDB-Seite in einem neuen PC-Chrome-Tab. Cover-Tipp öffnete die vorher minimierte Spotify-App. Wiedergabe danach pausiert. Native erweiterte TMDB-Hilfe in DE/EN visuell geprüft, Sprache ohne Speichern zurückgestellt.
- Keine physische Fingerabnahme, kein Pixel-/TV-Test, kein Screenreader-/weiterer Browsertest. API-Antrag nicht mit persönlichen Daten abgesendet. Standardbrowser entscheidet über Tab/Fenster. Alle Testbilder, Logs, Tokens und Konfigurationen bleiben lokal.

## Release 2.9.3 – 31.10.2026

- Basis main 909e34d (PR #46, PlayStation-Freunde inkl. Punkt-Code-Fix). Release-Build per build.bat grün: 1357 Kern, 119 TMDB, 251 Plugins, 41 Beispiele, 127 Store, 140 Transport, 135 Settings, 11 Monitor, 361 Companion, zwei Installerchecks. EXE meldet 2.9.3.
- Meanmachine von 2.9.2 ersetzt (Sicherung EXE/Config/Kopplungen vorher), Config byte-identisch, COM7 verbunden, Companion-Server und mDNS gestartet. Redmi und TV zeigen Seiten (adb-Screenshots). Pixel nicht per adb vorhanden.
- PlayStation live: der Testcode der Testinstanz war abgelaufen (login_required); mit frischem Code in der installierten 2.9.3 lieferte die Freundesliste ein PlayStation-Konto ohne Fehler im Log. Online-Status/Spiel auf den Geräten nicht gesichtet. Store-Installation/-Update über die Oberfläche und physische Abnahme nicht durchgeführt.

## Designsysteme nach 2.9.1 – 31.10.2026 (Entwicklung, nicht veröffentlicht)

- Branch `feature/theme-design-systems`, PR #45; Basis ist die veröffentlichte 2.9.1. Nach Nutzerkritik an zu ähnlichen Themes wurden eigenständige Fensteraufbauten implementiert: Cupertino mit einblendbarer Sidebar, Werkzeugleiste und optionalem Inspector; Fluent mit echter Seitensuche, Zurück-Verlauf und Breadcrumb; Material mit fünf Hauptzielen, kontextbezogenen Unterseiten, Hauptkennzahl und schwebender Speichergruppe nur bei Änderungen. Bei engem Platz wechseln Material-Unterseiten in eine Auswahl. Alle Seiten bleiben erreichbar.
- Systemdisplay: Cupertino gruppierte Liste aus fünf Zeilen; Fluent kompaktes Dashboard; Material CPU-Hauptkarte über GPU und drei Statuskarten. RAM-/Datenträger-/Netzwerkaktionen verwenden dieselben Regionen wie das Rendering. Das FuchsLabs-Layout bleibt erhalten. Die englischen CPU/GPU-Detailbeschriftungen der neuen Liste wurden beim Sichttest korrigiert.
- Build cpp/build/design-systems erfolgreich. Alle 13 Suiten grün: 1357 Kernprüfungen, 119 TMDB, 251 Plugins, 41 Beispiele, 127 Store, 140 Transport, 135 Settings, 11 Monitor, 361 Companion, 111 Signatur, 77 Catalog, 13 Amp und Key-Hilfe OK. Zwei isolierte Installerchecks bestanden. Neue Regressionen prüfen Material-Geometrie für mehrere Größen/Inspector-/Änderungszustände sowie alle fünf Systemregionen, Überlappungsfreiheit und Touchzuordnung bei 1x/2x für alle Profile.
- Zwölf DE/EN-Demokonfigurationen neu gerendert, die sechs Hell-/Dunkel-Systemansichten visuell verglichen. Native Cupertino-/Fluent-/Material-Oberfläche in beiden Darstellungen geprüft. Interaktiv bestätigt: Sidebar/Inspector, Fluent-Suche nach Spotify mit Öffnen und Zurück, Material-Ziele/Unterseiten, Unterseitenmenü bei 980 Pixel Breite mit Inspector, Erscheinen der Speichergruppe nach Darstellungswechsel und Verschwinden nach Verwerfen. Lokale Galerie `verification/DESIGN-VORSCHAU.html`; Bilder und Testkonfigurationen bleiben uncommittet.
- Sechs Profilpaletten, strenge Schema-/Roundtrip-/Kontrasttests weiterhin grün. Fluent fordert einen DWM-Mica-Fenstertitel an; Inhaltsflächen sind deckend. Cupertino-Werkzeugleiste ist eine eigene Windows-Interpretation, kein Liquid Glass. Keine Plattformanimationen oder vollständigen OS-Frameworks. Native Auswahlmenüs folgen Windows. Keine neue Companion-APK notwendig.
- Keine weitere Geräteabnahme der neuen Profile: Nutzer zog die zunächst erteilte LAN-Relay-Freigabe mit der jüngsten Antwort zurück. Der kurz gestartete Relay hatte keine Verbindung geliefert (Timeout Port 8792); Relay und Testserver wurden beendet, keine Firewalländerung. Frühere Loopback-Relay/ADB-Reverse-Versuche ebenfalls entfernt. Produktionskopplungen für Redmi und TV wiederhergestellt, zwei Clients bestätigt. Pixel seit der ausdrücklichen Einschränkung nicht bedient.
- Kein Store-Katalog geändert, signiert oder veröffentlicht. Die veröffentlichte 2.9.1 und ihre Assets bleiben unverändert. Keine Konfigurationen, Kopplungen, Logs oder privaten Screenshots im Commit.


## Abschlussprüfung 2.9.1 – 31.10.2026

- Nach ausdrücklichem Nutzerauftrag echte Meanmachine-Installation auf 2.9.1 aktualisiert; vorher EXE, Konfiguration, Kopplungen und Themes privat gesichert. Companion-Apps auf Redmi, Pixel und Android TV mit SHA-256-geprüftem vorhandenem APK 1.0.2 aktualisiert, Daten erhalten. TV neu gekoppelt; drei echte Clients gleichzeitig verbunden, COM7 erfolgreich geöffnet.
- Fehlende Amplifier-Anzeige reproduziert: Spotify war deaktiviert, nicht sichtbar und ohne Client-ID konfiguriert. Bestehende Client-ID ergänzt, Anmeldung wiederverwendet und Seite aktiviert. Zusätzlicher Codefehler: bei stiller Ausgabe erlosch die 3-s-Aktivität vor dem 4-s-Simulationsstart; lange serielle Übertragungen verlängerten dies. Sichtbare Wiedergabe wird jetzt auch ohne Pegeländerung aktualisiert; ein RAII-Guard hält die Aktivität während einer sichtbaren seriellen Wiedergabeübertragung und erneuert danach den normalen Nachlauf, ohne den Titel-Seed zu verlieren.
- Neue Regression: sichtbare Übertragung bleibt nach 3,1 s aktiv, Freigabe ohne Erneuerung wird inaktiv, erneutes touch aktiviert wieder. Kernsuite jetzt 482 Prüfungen. Vollständiger Release-Build mit allen eingebauten Suiten und beiden Installerchecks grün. Zusätzliche Signatur-/Catalog-/Amp-/Key-Hilfe-Suiten vorher ebenfalls grün.
- Reale Amplifier-Abnahme: isoliert ohne LCD und anschließend mit COM7 zwei aufeinanderfolgende Spotify-Bilder mit unterschiedlichen Balkenhöhen und Kennzeichnung „simuliert“ gesehen. Keine Testtöne, keine Audioaufnahme, kein Standard-Audiogerätewechsel. Private Screenshots bleiben lokal.
- Grenzen: keine physische Finger-/Fernbedienungsabnahme oder Sichtprüfung des seriellen Panels per Kamera; keine gemessene serielle Bildrate. Vollständige EN-Key-Hilfe- und Browserabnahme der Anleitungen bleibt offen. Vorheriger Abbruchstand unten bleibt als Historie erhalten.


## Release-Kandidat 2.9.1 – 31.10.2026 (Abnahme unterbrochen)

- Basis: Branch `release/2.9.1`, Versionscommit `307de16`, main `1c943ff`. Android unverändert gegenüber Tag `v2.9.0`; keine neue APK gebaut. Installierte Geräte melden Companion 1.0.1, Quellfassung 1.0.2.
- MSVC-Release-Build erfolgreich; EXE-Ressourcen melden FileVersion 2.9.1.0 und ProductVersion 2.9.1. Kern 479, TMDB 119, Plugins 251, Beispiele 41, Store 127, Transport 140, Settings 135, Monitor 11, Companion 361: jeweils null Fehler. Beide isolierten Installerprüfungen bestanden. Zusätzliche Suiten: Signatur 111, Catalog 77, Amp 13, Key-Hilfe OK.
- Companion-Integration mit simulierten Geräten: 39 Prüfungen; zwei unabhängige simulierte Geräte: 17 Prüfungen bestanden. Keine physische Geräteabnahme daraus abgeleitet.
- Isolierte native Oberfläche (Funktionsstand vor Versionsbuild): Fluent- und Material-Stil aus signiertem Store installiert; Auswahl Fluent-Stil gespeichert, automatischer Neustart, Auswahl und Vorschau erhalten. Store-Themekarte und Status-Tooltip im dunklen Einstellungsfenster lesbar. Die 2.9.1-EXE übernimmt dieselbe Theme-Auswahl ebenfalls.
- Isolierter echter Installer-Updatepfad mit vorhandener 2.9.0-EXE auf 2.9.1 bestanden: installierte EXE stimmt bytegenau mit Kandidat überein; Fluent-Auswahl und Theme-Dateien erhalten; Konfigurationssicherung bytegenau. Ein erster Durchlauf des lokalen Testskripts hatte die API-Hülle `config` übersehen; nach Korrektur der Prüfroutine bestanden, kein App-Befund.
- Key-Hilfen DE: Schritte für alle sechs Dienste im nativen Fenster gelesen. Browserziele Steam, OpenXBL, TMDB und Spotify durch tatsächliche Klicks bestätigt. Discord-Knopf zusätzlich betätigt; abschließende Zielkontrolle sowie Home-Assistant-Linkkontrolle und EN-Sichtprüfung noch offen. Keine Anmeldung oder Schlüsselerstellung durchgeführt.
- Anleitung DE/EN aus Vorlagen mit unveränderten vorhandenen eingebetteten Bildern synchronisiert; vier JavaScript-Blöcke mit `node --check` geprüft. `make_images.py` konnte mit dem vorhandenen Python mangels Pillow nicht laufen; daher bestehende geprüfte Bilddaten unverändert übernommen. Browserabnahme Hell/Dunkel und lokale Linkprüfung noch offen.
- Reale Hardwareabnahme nach ausdrücklicher Nutzerfreigabe mit vorübergehend gestoppter echter App und isolierter Kopie ihrer Daten: Kandidat 2.9.1 öffnet COM7 erfolgreich; Companion-Server startet, zwei Clients werden gemeldet. Keine physische Sichtabnahme des seriellen Panels behauptet.
- **Abbruchbefund:** Verbindung des Android-TV wird vom isolierten Server mit unbekanntem Geräteschlüssel abgelehnt. Nach Rückkehr zur Original-App zeigt auch der TV „unauthorized: Gerät ist nicht (mehr) gekoppelt“. Damit ebenfalls am Originalbetrieb beobachtet; Ursache ungeklärt, kein bestätigter 2.9.1-Codefehler. Gemäß Handover weitere Abnahme gestoppt, kein Release-Paket, Commit, Push oder PR erstellt. Keine Kopplungen geändert. Reale Seitenwechsel, Theme-/Amplifier-Sichtabnahme und TV-Pfeiltastenprüfung bleiben offen.
- Kandidat beendet und ursprüngliche installierte EXE wieder gestartet; EXE und Konfiguration der echten Installation bytegenau unverändert geprüft. Private Testbilder und Daten bleiben außerhalb der Versionskontrolle. Keine Testtöne, keine Änderung des Standard-Audiogeräts.


## Amplifier-Korrektur nach PR #32 (31.10.2026, noch unveröffentlicht)

- GitHub-Status geprüft: PR #32 wurde als `12fd87a` gemergt; sein Baum stimmt mit der lokalen Ausgangsbasis `d4a1c48` überein. Separater Worktree `screenbuddy-amplifier-fix`, Branch `fix/amplifier-visibility-decay`.
- Code-Review: `page_list` aktivierte den Pegelmesser bereits beim Auflisten aller Seiten. Aktivierung in den Zeichen-Callback der tatsächlich ausgewählten Spotify-Kachel verschoben, nur bei Wiedergabe. Nachlauf bleibt höchstens 3 s. Die WASAPI-Verbindung bleibt offen; nur die FFT-Berechnung pausiert. Dies ist eine Codepfadprüfung, keine neue physische Sichtabnahme.
- Code-Review: Änderungsschwelle von 0,012 unterdrückte letzte Abkling-Schritte einschließlich null. Gemeinsame Glättungs-/Änderungsfunktion publiziert jetzt jeden Schritt. Regression prüft das tatsächlich publizierte Ende bei Stille, Ruhezustand und Anstieg.
- Isolierter MSVC-Release-Build erfolgreich. 356 Kern-, 111 TMDB-, 251 Plugin-, 41 Beispiel-, 127 Store-, 140 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 361 Companion-Prüfungen bestanden: **1.533**, dazu **2 Installer-Prüfungen** in eigenen Installationsordnern. Test-Temp separat; kein Test gegen Port 8765. CMake blieb in der Sandbox bei der Compilerprüfung hängen; außerhalb der Sandbox lief der Build erfolgreich.
- README DE/EN, Referenz, Spotify-Doku DE/EN, Changelogs und Anleitungen aktualisiert. 157 lokale Markdown-Linkziele und 8 JavaScript-Blöcke bestanden; Spotify-Schritte zwischen Vorlagen und Ressourcen identisch.
- Offen: erneute physische Abnahme auf Redmi 8, Pixel und USB-Display, gemessene serielle Bildrate, Wechsel des Windows-Standard-Audiogeräts. Keine Installation auf dem Echtsystem, kein Release/Tag, kein Wiki-Push. Themes-Auftrag bleibt beim Koordinator; kein neuer Chat angelegt.


## Veröffentlichung ScreenBuddy 2.8.0 + Companion 1.0.1 (31.10.2026)

- Windows-Build: 330 Kern-, 111 TMDB-, 251 Plugin-, 41 Beispiel-, 127 Store-, 140 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 361 Companion-Prüfungen bestanden (1.507 insgesamt) sowie 2 Installer-Prüfungen, nach Integration des Branches `feature/streaming-logos` (Anbieter-Logos). Netflix-Logos auf dem Redmi 8 beobachtet, Prime-Logos laut Streaming-Thread live geladen, übrige Anbieter und Fehlerfall ohne Netz nicht am Gerät geprüft. Gefunden und behoben: Merge-Namenskonflikt in `tests.cpp`, Seitenzahlprüfungen (neue Seite), RTL+-Anbieter-ID (298 → 2750 laut TMDB-Anbieterliste DE).
- Echtsystem auf der Meanmachine: Update 2.7.0 → 2.8.0 über den Installer, `config.toml` und `companion.json` erhalten (Sicherung vor dem Test), USB-Display COM7 (480 × 320) verbunden. TMDB mit echtem Read-Access-Token: Titel auf Testinstanz (Port 8791) und im echten Betrieb auf Display-Vorschau und Redmi 8 sichtbar. Spotify mit echtem Konto: Anmeldung, Titel, Cover und Fortschritt auf dem Redmi 8 sichtbar; Befehle Pause/Wiedergabe im Protokoll. Neue Seiten erscheinen erst nach Anhaken unter „Seiten & Reihenfolge“; dies wurde so in der Config freigeschaltet und dokumentiert.
- Companion 1.0.1 (versionCode 5, gleicher Release-Schlüssel, Zertifikat-SHA-256 `2DFCD3F3…1088`): Update über 1.0.0 auf Pixel 10 Pro und Redmi 8 ohne neue Kopplung. Erste Variante (Inhalt mit Rahmen) vom Nutzer abgelehnt; zweite Variante (randloser Hintergrund, Inhalt im sicheren Bereich) auf beiden Geräten per Bildschirmfoto geprüft. Nach App-Start musste auf beiden Handys einmal „Verbinden“ getippt werden.
- Einstellungen: linke Navigation passt in die Fensterhöhe, rechte Geräteliste mit anwählbaren Karten (USB-Display, Redmi 8, Pixel 10 Pro) und Live-Vorschau, Schnellzugriff mit Symbolen und Anleitung/Werkstatt-Knöpfen; per Bildschirmfoto im echten Fenster geprüft, Klick auf eine Karte schaltete die Vorschau um.
- Nicht geprüft: echtes Store-Update (die isolierte Store-Instanz lieferte in dieser Sitzung kein Protokoll), TMDB-Logo (nicht eingebunden), Spotify auf dem Pixel und per physischem Touch am USB-Display, Codex-Review (gestartet, nicht abgeschlossen), DPI-/Screenreader-Abnahme.
## Veröffentlichung ScreenBuddy 2.6.0 + Companion 1.0.0 (30.10.2026)

- Windows-Build mit allen Prüfungen bestanden: 272 Kern-, 251 Plugin-, 41 Beispiel-, 127 Store-, 140 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Prüfungen (zusammen 1.312) sowie 2 Installer-Prüfungen.
- Plugin-Store in isolierter Instanz (Port 8791, nie Port 8765): Index und 8 Start-Plugins geladen, Installation „Wetter“ aus dem Live-Store bestanden (Plugin bleibt aus, kein Host zugestimmt), Schalter „Plugin-Store verwenden“ speichern und Neustart bestanden, Offline-Zwischenspeicher vorhanden. Update nur als Simulation (lokale Version 0.9.0) bestanden; ein echtes Live-Update war mangels zweiter Version nicht prüfbar. Suchfeld nicht automatisiert prüfbar.
- Gefunden und behoben: Plugin-Dateien wurden von GitHub Pages nicht ausgeliefert (404); Schalter wurde nicht gespeichert (`collect()` fehlte).
- Companion 1.0.0: Android-Protokolltests und signierter Release-Build bestanden, Zertifikat identisch zu 0.2.1-preview. Kein neuer Gerätetest.
- Codex-Review der Release-Änderungen durchgeführt; Befunde zu Formulierungen eingearbeitet.
## Veröffentlichung ScreenBuddy 2.5.0 (27.10.2026)

- PR #17 und Dokumentations-PR #18 mit bestandenen Python-/JavaScript-CodeQL-Prüfungen integriert. Tag `v2.5.0` zeigt auf Merge-Commit `0351234707f7bbfb92e8754b6991535d97a0b499`. [Release](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.5.0).
- Drei öffentliche Downloads erneut abgerufen und bytegleich verifiziert: `ScreenBuddy.exe` (14.970.368 Bytes, SHA-256 `BF63FB91118FF1E16DB718D9EE9448301CCFDAA477A857A37F375A144AA2AA0C`), signierte Companion-APK (3.265.144 Bytes, `E06F57AAF7CC51906C5E6626BE1BE44C119034F1AD04B2FE529282520A76EB47`) und SHA256SUMS.txt. GitHub markiert v2.5.0 als neuesten stabilen Release; Android bleibt ausdrücklich Vorschau.
- Finale unabhängige Windows-Prüfung: Datei/API 2.5.0, exakter veröffentlichter EXE-Hash, ein installierter Prozess und korrekter Autostart; config.toml bytegleich zum Ausgangsstand. Kopplungsdatei unmittelbar nach Installation bytegleich, später im Betrieb abweichende Geräteliste. Lesender C++-Gegentest lädt alle gesicherten Alt-Kopplungen korrekt; Zertifikat und privater Serverschlüssel sind unverändert. Ursache der später geänderten Liste nicht abschließend zugeordnet. Produktions-Kopplungen nicht zusammengeführt oder zurückgesetzt; eine zusätzliche Schlüssel-Wiederherstellung wurde von der automatischen Freigabeprüfung mangels ausdrücklicher Zustimmung abgelehnt. Sicherung bleibt privat vorhanden. Dies ist kein Nachweis dauerhafter Bytegleichheit einer laufend veränderten Kopplungsdatei.
- Bestehender Website-Update-Dienst erfolgreich; gecachter Release v2.5.0, neue atomare Veröffentlichung aktiv. Öffentliche DE/EN-Seiten HTTP 200 und v2.5.0-Downloadlink bestätigt. Keine Änderung an Firewall, Proxy oder Containerkonfiguration.
- Eigene isolierte Testinstanzen beendet; eigene einmalige Prüftasks entfernt. Nicht zum Release gehörende Marketing-Entwürfe bleiben unberührt. Schlüssel, Konfigurationen, Logs und lokale Prüfprogramme werden nicht veröffentlicht.

## Release-Abnahme ScreenBuddy 2.5.0 (27.10.2026)

- **Finale Installationsprüfung bestanden:** Ressourcen-Build `BF63FB91…` im echten Windows-Kontext installiert, Installer-Exit 0, API 2.5.0, finaler EXE-Hash geprüft, config.toml und companion.json bytegleich erhalten. 51 API-/Navigations- und zwei Installerprüfungen gegen exakt diesen Build bestanden; 272 Kernprüfungen mit eigenem Test-Temp erneut bestanden. Keine erneute physische Langzeitabnahme dieses Hashes behauptet; Änderungen gegenüber dem abgenommenen Programmcode betreffen ausschließlich Anleitungsressourcen und Build-Metadaten.

- Nutzer bestätigte nach der angeforderten Beobachtungszeit: „Sicht-/Touch- und Bedienprüfung bestanden“. Umfang: USB-Display ohne Streifen/Flackern/Aussetzer, vorhandenes Touch und native Einstellungen mit Maus/Tastatur auf Übersicht, Anzeige, Datenquellen, Companion und Design. Beobachteter Kandidat: SHA-256 `17A3DB7C517608068B2DB2059A4E582EDC6D0EEDD31C580B3E01D03BFBB54B22`.
- Echter Rückfall auf die gesicherte 2.3.0 bestanden: Datei und API 2.3.0, veröffentlichter Hash `3A1ECBD20269AFC78B3E5B63C129FA3460860C5720E195F2C1191104B0792A69`, ein Prozess, EXE/config.toml/companion.json bytegleich wiederhergestellt. Appbezogene Autostart-/Uninstall-Registrierung wiederhergestellt; Standard-Startmenüverknüpfung rekonstruiert. Der Test ist keine erneute physische Sichtabnahme der alten Version.
- Erneutes Upgrade auf den nach Integration geprüften Build `5706E50E…` bestanden: Installer-Exit 0, API 2.5.0 und Konfiguration/Kopplungsdatei erhalten. Dieser Build unterscheidet sich vom physisch beobachteten Kandidaten nur durch PE-/Debug-Metadaten.
- Abschließend nur Release-Texte, Downloadname und Companion-Migrationshinweis in DE/EN-Anleitungsressourcen angepasst. Finaler Build: SHA-256 `BF63FB91118FF1E16DB718D9EE9448301CCFDAA477A857A37F375A144AA2AA0C`, 14.970.368 Bytes. Programmcode unverändert; neue Anleitungen mit Headless Edge in allen 46 Ansichten geprüft. Der finale Ressourcen-Build erhält noch eine eigene Installations-/API-/Hash-Prüfung; die physische Langzeitabnahme wird nicht fälschlich diesem abweichenden Hash zugeordnet.
- Android bleibt bytegleich zum physisch abgenommenen APK-Hash `E06F57AAF7CC51906C5E6626BE1BE44C119034F1AD04B2FE529282520A76EB47`. Grenzen: Online-Plugins experimentell; vollständiger UI-Consent-Ablauf, CRL/OCSP, vollständige UI-Parität/DPI/Screenreader, Windows-Anmeldung, WLAN/Boot/mehrere Android-Geräte nicht vollständig geprüft. Details in Release-Notizen und Checkliste.

## ScreenBuddy 2.5.0 candidate + Companion 0.2.1-preview (27.10.2026): Fortsetzung

### Integrierter Commit 8492c54 – Wiederholungsbuild und Tests

- Nach Integration gebaut: Windows 2.5.0, 14.971.392 Bytes, SHA-256 `5706E50E0C3F41054E715C2DF1012D5F48F366583BD258B33E6E10CD899E64F0`. Build, dist und lokales Kandidatenpaket synchronisiert. Android-APK bleibt bytegleich `E06F57AAF7CC51906C5E6626BE1BE44C119034F1AD04B2FE529282520A76EB47`.
- Erneut bestanden auf dem Commit: 272 Kern-, 223 Parser-, 41 Beispiel-, 127 Speicher-, 134 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Prüfungen; 27 native Einstellungs-, 16 Maus-, 16 Companion-Rebuild-, 51 Navigations/API-, 39 Integrations- und zwei Installerprüfungen. Android-Protokolltest und signierter Release-Build bestätigt; 204 lokale Markdown-Linkziele ohne fehlendes Ziel geprüft.
- Die aktuell am Zielsystem beobachtete EXE hat weiterhin den separat dokumentierten Hash `17A3DB7C…`. PE-Vergleich: kompletter Neubuild bytegleich nach Maskierung ausschließlich von PE-Zeitstempel, PE-Prüfsumme und Debug-Metadaten; insbesondere .text, .data und .rsrc unverändert. Die Hashes werden nicht gleichgesetzt. Finales Installations-/Sichttestergebnis und Rückfallprüfung bleiben offen.
- Kein Tag oder Release veröffentlicht, Website unverändert. Lokaler Kandidaten-Commit enthält kein Signiermaterial, keine privaten Konfigurationen/Logs und keine App-Binärdateien. Nicht zum Release gehörende Marketing-Entwürfe bleiben unangetastet im Arbeitsbaum.

### Windows-Zielsystem: Upgrade und Sicherung verifiziert; Sichtprüfung läuft

- Codex-Dateizugriffe wurden nachweislich auf eine alte Paketkopie (2.0) umgeleitet. Eine einmalige Aufgabe mit normalen Benutzerrechten außerhalb des Paketkontexts bestätigte die echte Ausgangsinstallation: API und Datei 2.3.0, SHA-256 `3A1ECBD20269AFC78B3E5B63C129FA3460860C5720E195F2C1191104B0792A69`. EXE, config.toml und companion.json bytegleich privat gesichert; Registry-Zustand für Rückfall lokal erfasst.
- Zielsystem auf Kandidat 2.5.0 aktualisiert. Unabhängige Nachprüfung außerhalb der Paketumleitung: Datei- und API-Version 2.5.0, EXE-Hash `17A3DB7C517608068B2DB2059A4E582EDC6D0EEDD31C580B3E01D03BFBB54B22`, genau ein installierter Prozess, Autostart auf die neue EXE. config.toml und companion.json bytegleich erhalten; Installer-Sicherung der Vorgänger-EXE entspricht dem veröffentlichten 2.3.0-Hash.
- Native Einstellungen für den Nutzer geöffnet; etwa zehnminütige Sicht-/Touch-/Bedienprüfung angefordert. Rückfall auf 2.3.0 ist konkret vorbereitet, aber noch nicht ausgeführt. Kein vollständiger Upgrade-/Rollback-Pass allein aus dem erfolgreichen Upgrade.
- Der erste Installationshelfer wartet mit PowerShell Start-Process -Wait auch auf gestartete Nachfolgeprozesse; sein noch ausstehender Abschluss ist kein laufender Installerfehler. Der Erfolg des Upgrades wurde deshalb separat am tatsächlich laufenden Prozess und an den echten Dateien geprüft.
- Beide HTML-Ressourcen wurden nach Korrektur des reproduzierbaren anonymisierten Freunde-Bildpfads neu erzeugt und byteweise mit den Ressourcen der getesteten EXE verglichen: identisch. Alle 46 DE/EN-Schritte wurden nach Ende ihrer Einblendanimation erneut gerendert und in Übersichtsaufnahmen visuell geprüft; kein defektes Bild oder horizontaler Überlauf. Deutsche native Beispielaufnahmen in EN bleiben ausdrücklich gekennzeichnet. Frühere Capture-Provenienz im Bildmanifest bleibt getrennt vom Vergleich mit dem aktuellen Kandidaten.
- Öffentliche Open-Meteo-Beispielabfrage erneut direkt mit dem Plugin-Transport erfolgreich. Keine privaten Koordinaten oder Produktionsfreigaben verwendet. UI-Import-/Consent-Ablauf und fehlende CRL/OCSP-Prüfungen bleiben davon getrennt.

### Redmi 8 – signierter Kandidat 0.2.1-preview: physischer Test bestanden

- Nutzer hat Austausch der gesicherten alten 0.1.0-proto-App ausdrücklich freigegeben. Signierte APK erfolgreich installiert und Version 0.2.1-preview / versionCode 3 verifiziert. Die APK entspricht SHA-256 `E06F57AAF7CC51906C5E6626BE1BE44C119034F1AD04B2FE529282520A76EB47`.
- Isolierte Windows-2.5.0-Testinstanz mit eigener Konfiguration, Ports 18958/18959 und `--no-lcd`: zwei synthetische Seiten (System/Freunde), keine Windows-Installation, kein COM7-Zugriff. Kopplung manuell über USB-Tethering mit lokal angezeigten Daten; Nutzer bestätigte Kopplung und sichtbares Systembild. Genau ein gespeichertes Gerät geprüft.
- Querformat 1520 × 720 und Hochformat 720 × 1520 durch den tatsächlich gerenderten PNG-Header verifiziert. Nutzer bestätigte scharfe, vollständige Darstellung in beiden Formaten sowie die abschließende Rückkehr zum Querformat.
- Echter Finger-Wisch links per Companion-Seitenindex bestätigt, Hauptanzeige unverändert. Nutzer bestätigte beide Wischrichtungen und Kopfzeilen-Tipp; abschließender Tippzustand per API geprüft. Automatischer 5-Sekunden-Wechsel per API und vom Nutzer sichtbar bestätigt.
- Eigene PC-Testinstanz beendet und neu gestartet: gleiches Serverzertifikat, gespeicherte Kopplung erhalten, Redmi ohne neue PIN wieder verbunden. Nutzer bestätigte saubere Rückkehr des Bildes. Rückkehr zur Einrichtung und erneute Verbindung nach Ausrichtungswechsel ebenfalls ohne neue PIN bestätigt.
- Genau ein falscher PIN mit einem lokalen TLS-Testclient wurde als `pin_wrong` abgelehnt; kein zusätzliches Gerät angelegt. Dies war kein falscher-PIN-Test über die Android-Oberfläche.
- Nach Aktivierung des USB-Tetherings verschwand das Gerät aus ADB. Windows erkannte RNDIS und eine WinUSB-ADB-Schnittstelle; Neustart/anderes ADB-Backend und erneutes Aktivieren von USB-Debugging änderten dies nicht. Keine Treiberänderungen. Alle anschließenden Gerätebedienungen erfolgten manuell durch den Nutzer; Serverzustände wurden unabhängig per API geprüft.
- Grenzen: USB-Tethering und diese Redmi-/Kandidatenkombination bestanden. WLAN, Launcher-/Boot-Modus, weitere Geräte, vollständige Wiederherstellung alter Android-Keystore-Daten und gleichzeitiger physischer USB-Display-Betrieb bleiben ungetestet. Windows-Release-Gates bleiben separat offen. App bleibt installiert; eigene PC-Testinstanz läuft derzeit zur Kontrolle weiter.

### Redmi-8-Gerätetest: Installation und Ausgangsstand

- Redmi 8 über ADB erkannt. Tatsächlich installiert war `0.1.0-proto` (versionCode 1), nicht die zuvor vermutete 0.2.0-Vorschau. Alte APK und private App-Daten vor dem Austausch außerhalb des Repositorys gesichert; TAR-Lesbarkeit und Prüfsummen geprüft. Die Sicherung ist kein Nachweis einer vollständigen Android-Keystore-Wiederherstellung.
- Nach ausdrücklicher Zustimmung des Nutzers alte App deinstalliert und den signierten Kandidaten installiert: beide ADB-Schritte erfolgreich, Paket meldet `0.2.1-preview`, versionCode 3. MainActivity gestartet. Keine Windows-Installation und kein Zugriff auf COM7.
- Kopplungs-/Ausgabetest noch ausstehend: das Gerät hatte bei der Vorprüfung keine aktive IPv4-Route. USB-Tethering beim Nutzer angefordert. Installation allein ist keine Geräteabnahme.


- **Windows:** vorhandener MSVC-Release-Baum erneut geprüft (`ninja: no work to do`). Build-EXE und `dist/ScreenBuddy.exe` jetzt bytegleich: 14.971.392 Bytes, SHA-256 `17A3DB7C517608068B2DB2059A4E582EDC6D0EEDD31C580B3E01D03BFBB54B22`, Produktversion `2.5.0`. Die vorherige dist-Datei war noch dev.2; historische Einträge bleiben erhalten.
- **Erneut bestanden:** 272 Kern-, 223 Parser-, 41 Beispiel-, 127 Speicher-, 134 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Prüfungen; 27 native Einstellungen, 16 Maus, 16 Companion-Rebuild, 51 Navigation/API, 39 App-Integration und 2 Installerprüfungen. Companion und UI/API benötigten Loopback-Zugriff außerhalb der Sandbox. Kein serieller Zugriff durch diese Tests.
- **Isoliertes echtes Upgrade:** veröffentlichte, per Hash geprüfte 2.3.0-EXE → 2.5.0 bestanden: API-Version, alter Prozess beendet, neue EXE und alte EXE-Sicherung, Konfiguration/Snapshot, Companion-Datei und unveränderte Test-Registry geprüft. Kein produktives Upgrade oder Rollback.
- **Android:** 20 Protokolltests erneut bestanden, Release-Build und Lint erfolgreich. `0.2.1-preview`, versionCode 3, 3.265.144 Bytes, SHA-256 `E06F57AAF7CC51906C5E6626BE1BE44C119034F1AD04B2FE529282520A76EB47`. APK-v2-Signatur mit RSA-3072 geprüft; Zertifikat-SHA-256 `2dfcd3f3153cd9e1371d8f19b9e175793d44f544a45764cbd5f43c74a1ad1088`. Neuer privater Release-Schlüssel außerhalb des Repositorys, externer Signierpfad dokumentiert. Andere Signatur als die frühere Debug-APK: kein direktes Update darüber. Keine Deinstallation oder Installation am Gerät; ADB meldete kein Gerät. Schlüssel und Passwörter vor Veröffentlichung zusätzlich extern sichern.
- **Anleitung:** Headless Edge hat mit allen aktivierten Optionen 23 Schritte je Sprache (46 gesamt) auf JavaScript-Fehler, fehlende Bilder und horizontalen Überlauf geprüft; alle bestanden. Prüfbilder liegen im ignorierten Build-Ordner. Dies ist keine vollständige visuelle Abnahme, keine EN-Native-Capture- oder DPI-/Screenreader-Abnahme.
- **Freigabe:** Nutzerauftrag zur Testung und zum Release einschließlich neuer APK liegt vor. Hardware-/Touch-/Geräteabnahme, produktives Upgrade/Rollback, vollständige UI-/Guide-Abnahme sowie Integration und Wiederholungsbuild auf dem finalen Commit bleiben offen. Lokales Kandidatenpaket ohne Geheimnisse vorbereitet; kein Tag, Upload oder Website-Deployment. GitHub `releases/latest` ergab weiterhin `v2.3.0`.

## ScreenBuddy 2.5.0-dev.2 (27.10.2026): aktueller unveröffentlichter Arbeitsbaum

- **Build:** Der vorhandene Release-CMake-Baum wurde mit initialisierter MSVC-Umgebung erfolgreich neu gebaut. `cpp/dist/ScreenBuddy.exe` und `cpp/build/screenbuddy-release/ScreenBuddy.exe` sind bytegleich: 14.971.392 Bytes, SHA-256 `21DFD0BC46A91C4B0ACC208E389DCA21DF321FE715E44983A2F39E1CA4878351`. Produktversion `2.5.0-dev.2`; keine Installation oder Veröffentlichung.
- **Komponenten:** 272 Kern-, 223 Plugin-Parser-, 41 Plugin-Beispiel-, 127 Plugin-Speicher-, 134 Plugin-Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Protokollprüfungen bestanden. Companion lief mit lokalem Loopback-Zugriff; in der eingeschränkten Netzwerksandbox beendete er sich ohne Testergebnis.
- **Isolierte App/UI:** 27 native Einstellungs-, 16 Maus-, 16 Companion-Rebuild-, 39 App-/Companion-Integrations-, 51 Navigations-/API- und zwei Installer-Konfigurationsprüfungen bestanden. UI und Installer mussten nach Sandbox-Fehlern mit lokalem Loopback-Zugriff wiederholt werden. Alle Instanzen nutzten `--no-lcd`, eigene Konfigurationen und Testports; kein serieller Zugriff.
- **Änderungen:** Die alte HTML-Einstellungsseite wird unter `/` nicht mehr ausgeliefert (HTTP 410) und nicht mehr eingebettet. `/anleitung` und `/api/config` bleiben erreichbar; der neue API-Test prüft diese Pfade. Die eingebettete DE/EN-Anleitung nutzt vollständige anonymisierte Native-Captures statt alter Zuschnitte; JavaScript-Syntax und 17 eingebettete Bilder je Sprache wurden geprüft. Die EN-Fassung kennzeichnet ihre derzeit deutsch beschrifteten Aufnahmen.
- **Abgrenzung:** Die Anleitungen wurden in Headless Edge außerhalb der Sandbox für die Schritte 1 und 7 in DE/EN visuell geprüft; die vollständige Sichtabnahme aller 21 Schritte bleibt offen. Vollständige native UI-/DPI-/Screenreader-Abnahme, Plugin-Consent in der App, Zielsystem-Upgrade/Rollback und Sichtabnahme dieses EXE-Hashes auf dem physischen Display bleiben offen. Der Nutzer meldete den früheren Bildfehler als behoben und wünscht keine weitere Ursachenanalyse. 2.3.0 bleibt der veröffentlichte Download.


## ScreenBuddy 2.5.0-dev.2 (26.10.2026): aktueller unveröffentlichter Arbeitsbaum

- **Build:** Der normale Aufruf `cpp/build.bat nopause` scheiterte vor dem Kompilieren beim vcpkg-Entpacken von 7zip (`CreateFileW stdin: Access denied`). Der vorhandene Release-CMake-Baum wurde danach mit initialisierter MSVC-Umgebung vollständig gebaut. `cpp/dist/ScreenBuddy.exe` und `cpp/build/screenbuddy-release/ScreenBuddy.exe` sind bytegleich: 14.815.232 Bytes, SHA-256 `61DC9D0E607284667AFD1646F58474B59F545F17F2CBEAE97F215D7F2470815A`. Produktversion weiterhin `2.5.0-dev.2`; keine Installation oder Veröffentlichung.
- **Kern- und Komponententests:** 272 Kern-, 223 Plugin-Parser-, 41 Plugin-Beispiel-, 127 Plugin-Speicher-, 134 Plugin-Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Protokollprüfungen bestanden. Der erste Kernlauf scheiterte an einem nicht beschreibbaren Sandbox-Temp-Ordner; mit eigenem Temp-Ordner unter `cpp/build` bestanden. Companion und die folgenden lokalen Netzwerk-/App-Prüfungen wurden außerhalb der Netzwerksandbox mit isolierten Testports wiederholt.
- **Isolierte App- und UI-Prüfungen:** 27 native Einstellungs-, 16 Maus-, 16 Companion-Rebuild-, 39 App-/Companion-Integrations-, 49 Seiten-Navigations-/API- und zwei Installer-Konfigurationsprüfungen bestanden. Alle App-Läufe verwendeten `--no-lcd`, eigene Konfigurationen und lokale Testports; der Windows-Touch-Assistent wurde nicht gestartet.
- **Änderungen dieses Stands:** Die Einstellungen starten ausschließlich nativ und linken WebView2 nicht mehr. Native Übersicht mit drei Einrichtungsschritten, Touch-Assistenten-Einstieg und Monitorauswahl unter Anzeige sowie ein geprüfter Release-Link unter Updates sind ergänzt. Der lokale HTTP-Dienst für Companion/API und die Anleitung bleibt. Englische RSS-/Home-Assistant-Demobilder und die eingebettete englische Anleitung wurden synchronisiert; das Bildmanifest referenziert die Hashes dieses Builds.
- **Abgrenzung:** Der Nutzer meldete am 26.10.2026 den zuvor beobachteten Bildfehler als behoben und wünscht keine weitere Diagnose. Dieser Build wurde nicht physisch auf COM7 abgenommen. Zielsystem-Upgrade/Rollback, vollständige visuelle Native-UI-Abnahme einschließlich DPI/Screenreader sowie die neue bebilderte DE/EN-Anleitung bleiben offen. 2.3.0 bleibt der veröffentlichte Download.

## ScreenBuddy 2.5.0-dev.2 (25.10.2026): unveröffentlichter Entwicklungsbuild

- **Build:** `cpp/build.bat nopause` mit MSVC/CMake/Ninja nach der WebView2-Brandingkorrektur erneut vollständig erfolgreich; `dist/ScreenBuddy.exe` (14.844.928 Bytes, SHA-256 `1DFA772B703C52EADDDE1F77286FE0063D9E7BC32C313DF8A359DA49825D14A4`) wurde lokal erzeugt. Produktversion `2.5.0-dev.2`, Dateiversion `2.5.0.0`. Kein Release, keine produktive Installation.
- **Unit-Tests:** 271 Kernprüfungen, 223 Plugin-Parser-, 127 Plugin-Speicher-, 133 Plugin-Transport-, 135 Plugin-Einstellungs-, 41 Plugin-Beispiel-, 11 Monitor- und 335 Companion-Protokollprüfungen bestanden.
- **Isolierte App:** 39 Companion-/App-Integrationsprüfungen und 49 Navigations-/API-Prüfungen bestanden; `--no-lcd`, eigene Konfigurationsdateien und lokale Testports, keine physische Displayausgabe.
- **Isolierter Installer:** Zwei Prüfungen bestanden. Vorhandene Einstellungen und deren Snapshot bleiben bytegleich; eine alte EXE wird als `.pre-screenbuddy` gesichert. Für die Namensmigration wurde eine Testkopie des neuen Builds unter altem Dateinamen verwendet, keine installierte 2.3.0-EXE.
- **Native UI, Phase 2:** Das selbst gezeichnete Direct2D/DirectWrite-Fenster enthält Seiten für Discord, Freunde, Home Assistant, Companion und Farben & Design. 20 Smoke-Prüfungen in einer isolierten `--no-lcd`-Instanz prüfen Fenster und Inhalte, 13 Navigationsnamen per MSAA, Tastatur, Plugin-Aktivierung, Phase-2-Steuerelemente ohne Browser-Übergabe, Verwerfen, Sprachwechsel sowie Speichern und Neustart (`native_settings_smoke.py`). 16 Maus-Prüfungen (`native_mouse_smoke.py`) und 16 Companion-Rebuild-Prüfungen (`native_companion_rebuild_smoke.py`) bestanden.
- **Android:** `:protocol:test` (20 Tests bestanden, 0 Fehler), `:app:assembleDebug` und `:app:assembleRelease` erfolgreich mit Gradle 8.9/JDK 21. `app-debug.apk` (4.149.316 Bytes, SHA-256 `5008C2E40CBD8191939CAA08098C9E8C727D301FCA3D5C660019BC6442D481E1`, Debug-Key) und `app-release-unsigned.apk` (3.256.948 Bytes, SHA-256 `320FDAD783985D852D2FC1A7DDCE97D3FA65185180241A63D7856A40C0494D9C`, unsigniert) erzeugt. Keine physischen Geräte über ADB verbunden.
- **Plugin-Beispiele:** `fl-plugin-example-tests` hat die drei Manifeste `offline-clock`, `offline-focus` und `public-weather` geparst und gerendert: 41 bestanden, 0 fehlgeschlagen. Wiederholung mit `build/nui/fl-plugin-example-tests.exe`. Das prüft Beispieldaten und Vorlagen; es stellt keine Live-HTTPS-Anfrage und belegt keine Display-Hardware. [Beispiele](examples/plugins/README.md) · [Anleitung DE](../docs/PLUGIN-TUTORIAL.md) · [EN](../docs/PLUGIN-TUTORIAL.en.md).
- **Plugin-Funktionen:** Begrenzter HTTPS-Abruf im eigenen Worker, gespeicherte Host-Zustimmungen, DPAPI-geschützte Einstellungen und Live-Rendering im Display-Thread sind implementiert. Quellen-Plugins können nach Pflicht-Einstellungen und Host-Zustimmung aktiviert werden. Die Transportprüfungen belegen keine Live-Abfrage externer Dienste.
- **Physische Sichtkontrollen:** Ein früherer `2.5.0-dev.2`-Echtlauf von etwa vier Minuten wurde mit gutem Bild auf beiden Displays und funktionierendem Touch bestätigt (separates lokales Handabnahmeprotokoll vom 25.10.2026). Ein späterer COM7-Demolauf zeigte Streifen und Flackern. Der frühere Pass bleibt dokumentiert; der spätere Fail ist für eine Freigabe ungeklärt. Der hier neu gebaute Hash wurde nicht physisch abgenommen.
- **Offen / Blocker:** Der jüngste COM7-Sichttest ist fehlgeschlagen ([Hardware-Abnahme](../docs/HARDWARE-ACCEPTANCE-2.5.md)); externe HTTPS-Liveabfragen, echtes Upgrade von 2.3.0, Screenreader-/DPI- und vollständige Nutzerabnahme sowie Performancevergleich sind offen. Eine neue Android-Release-APK wäre derzeit unsigniert; der Android-Quellstand ist gegenüber 2.3.0 unverändert. Kein Release oder Download für 2.5; aktueller Download bleibt 2.3.0. Details: [Codex-Übergabe](../UEBERGABE-CODEX-2.5.md).

## Version 2.3.0 (24.10.2026): Companion-Vorschau

- **Windows:** Release-Build erfolgreich (Produktversion 2.3.0, 14.037.504 Bytes, SHA-256 `3a1ecbd20269afc78b3e5b63c129fa3460860c5720e195f2c1191104b0792a69`). 236 Kernprüfungen, 11 Monitorprüfungen und 335 Companion-Protokoll-/TLS-Prüfungen bestanden. Neue TLS-Prüfungen: Anmeldung vor Größen-/Wischmeldungen, native Größen und Rotation, `ready` vor dem passenden JPEG, numerische Schnellfix-Kompatibilität und Schutz vor Größenänderungen durch einen zweiten Client.
- **Android:** `:protocol:test` (20 Tests, keine Fehler) und `:app:assembleDebug` erfolgreich mit Gradle 8.9/JDK 21. Die aktuelle Debug-APK wurde mit ausdrücklicher Freigabe auf dem Redmi 8 installiert.
- **Isolierte App:** 39 Prüfungen mit echten PC-App-Prozessen, eigener Konfiguration und freien Ports: Start ohne Companion, unabhängige Seitenwahl, Navigation in beiden Richtungen, eigener Timer, native Größe, Rotation, Nur-Companion-Modus und sauberes Beenden. Hauptausgang mit `--no-lcd` simuliert; kein serieller Zugriff.
- **Installation:** Zwei Windows-Integrationstests decken Upgrade mit vorhandener `config.toml` samt lokaler Sicherung und Neuinstallation aus einer portablen EXE mit Einstellungen neben der EXE ab. Alle Testpfade liegen in `cpp/build`; `--no-lcd`, eigene Ports und Testinstanz verhindern Hardware- oder Benutzerprofilzugriffe.
- **Oberfläche:** 22 Headless-Edge-Prüfungen mit ausschließlich synthetischen Daten (DE/EN, Zusatzoptionen bei Companion aus verborgen, getrennte Auswahllisten, keine JavaScript-Fehler, Syntax beider Anleitungs-Vorlagen und eingebetteter HTML-Dateien). Beispielansicht manuell visuell kontrolliert; keine echten Accountbilder im Repository.
- **Echtes Redmi 8, USB-Tethering:** Verbindung zum eigenen PC-Testserver, TLS-Fingerabdruck vor Kopplung verglichen. Native Ausgabe und tatsächlicher Screenshot 1520 × 720. Automatischer Wechsel nach 5 Sekunden, Wischen in beide Richtungen und Tipp auf die Kopfzeile über Android-Eingabeinjektion; dabei bleibt die Hauptansicht des Testprozesses unverändert. Neustart der Android-App verbindet mit gespeichertem Schlüssel ohne neue PIN. Ausrichtungswahl in der App: Hochformat 720 × 1520, anschließend Querformat 1520 × 720 wiederhergestellt.
- **Manuelle Abnahme durch den Nutzer:** Auf die ausdrückliche Prüfung von scharfem Bild, langsamem/normalem Wischen und Antippen am Redmi antwortete der Nutzer: „Alles funktioniert mit dem Finger“. Diese Bestätigung gilt dem aktuellen Gesamtstand.
- **Abgrenzung:** Die automatischen Gesten wurden per ADB injiziert; die manuelle Bestätigung ist separat belegt. Die produktive USB-App wurde nicht ersetzt und ihr COM-Port nicht geöffnet. Ein gemeinsamer physischer USB-/Redmi-Test innerhalb derselben neuen PC-Instanz, WLAN, Launcher-/Boot-Verhalten und mehrere reale Android-Geräte wurden nicht geprüft.

Reproduzieren: `cpp/build.bat nopause` (inklusive `installer_preserves_config.py`); `python cpp/tests/app_companion_integration.py`; anschließend `node cpp/tests/app_companion_ui.cjs` mit installiertem Playwright und Edge. Android: `android/gradlew.bat :protocol:test :app:assembleDebug`. Lokale Konfigurationen, Testkopplungen, Logs und Gerätescreenshots bleiben unter ignorierten Build-Pfaden.


## Version 2.2.0 (24.10.2026): Systemseite per Touch, Update-Hinweis

- Neu: Tipp auf RAM/Datenträger/Netzwerk der Systemseite (Task-Manager, „Programme und Features“, speedtest.net/de) und Update-Hinweis über die öffentlichen GitHub-Releases (höchstens 1× täglich, Sprechblase und Anzeige in den Einstellungen, Schalter `[ui] update_check`). Release-Build erfolgreich, **225 Kernprüfungen und 11 Monitor-Prüfungen bestanden**. Neue Prüfungen: Trefferflächen der Systemseite (inkl. skaliertem Display, Kopf-/Fußzeile, CPU-Karte, Aktionsaufruf je Zone), Versionsvergleich (Vorabversionen, ungültige Tags), Auswertung der GitHub-Antwort (Entwurf/Vorabversion verworfen, fremde Adresse ersetzt).
- Trefferflächen gegen das mit `--demo --preview` gerenderte Systembild geprüft; das Format der echten Antwort von `releases/latest` (Tag `v2.1.0`) abgerufen und entspricht dem Auswerter. Keine Installation, kein Test gegen Port 8765; die Prüfungen starten weder Task-Manager noch Browser.
- **Nicht getestet:** Tippen auf echter Touch-Hardware (Fenster erscheinen ggf. auf dem Hauptbildschirm statt auf dem Touch-Monitor), die Sprechblase und die Einstellungsanzeige mit einer tatsächlich neueren Veröffentlichung (Testinstanzen fragen bewusst nie ab), Windows-Anmeldetest.

## Version 2.1.0 (23.10.2026): Home-Assistant-Seite, enthält den Freunde-Tipp aus 2.0.9 – veröffentlicht als v2.1.0

- Neue Seite Home Assistant (bis zu 4 Entitäten, REST-API, DPAPI-Token, Auswahlliste in den Einstellungen, Schalter nur mit Touch). Release-Build erfolgreich (Produktversion 2.1.0, 13258240 Bytes, SHA-256 `711F76DF3F8D5DF343E2C1FA6EE5944EB8D012EC7D776F35990D26C27ED5D53A`), **212 Kernprüfungen und 11 Monitor-Prüfungen bestanden**, dazu 49 Seitennavigations-Prüfungen. Neue Prüfungen: Entitäts-Syntax, Schalter-Bereiche (Schlösser/Rollläden ausgeschlossen), Adressprüfung, Wertformat, Kachel-Treffer inkl. skaliertem Display, Konfiguration (höchstens 4, Token verborgen), Zustandsauswertung.
- Gegen einen lokalen Testserver (Python, Port 18123, kein echtes Home Assistant): Abfrage, falsches Token (Fehlermeldung), unbekannte Entität, nicht erreichbar, Umlegen eines Schalters (genau ein Aufruf; Schloss und Sensor werden nie gesendet), Entitätsliste ohne Touch ohne Schalter/Schlösser. Die Seite im Browser (DE und EN) und das Displaybild (Hell/Dunkel, 1–4 Kacheln, Fehlerfall) von Hand geprüft. Testinstanz auf Port 18770, ohne Display; Bastis laufende App und Port 8765 blieben unberührt.
- **Nicht getestet:** Schalten per echtem Fingertipp auf dem Touchscreen und gegen ein echtes Home Assistant (nur die Trefferberechnung und der Serveraufruf einzeln).
## Version 2.0.9 (23.10.2026): Freunde-Tipp Steam → Discord → Xbox – veröffentlicht als v2.1.0

- Auf Wunsch des Nutzers feste Reihenfolge: Steam-Chat, sonst Discord-Profil, sonst Xbox-Profil im Browser; fehlende Apps führen zum nächsten Ziel. Release-Build erfolgreich (Produktversion 2.0.9, SHA-256 `60771E3FED6B69564B731E6C14D3B818CF0E807A7E1D0493303B3730802E592C`), **164 Kernprüfungen und 11 Monitor-Prüfungen bestanden**, darunter neue Prüfungen für Reihenfolge, Fallback bei fehlender Discord-/Steam-App, Xbox-Profil-URL und ungültige IDs. Die Startfunktion wird dabei ersetzt: keine echten Chats oder Profile geöffnet. Keine Installation, kein Test gegen Port 8765.
- Offen: physische Abnahme am Display (Tipp auf Freund mit nur Discord bzw. nur Xbox) in angemeldeten Clients.

## Version 2.0.8 (23.10.2026): BF6-Profil-Link, nur Steam-Absprung – veröffentlicht als v2.1.0

- Ursache der leeren Seite: Die Webseite kennt die Plattform `ea` nicht (Route bleibt leer); mit `pc` lädt z. B. P4p4F0x im Browser korrekt. Für `Schmieselbeehee` liefert die API bei `platform=ea` Daten, bei `pc` (das die Webseite verwendet) 404; die Webseite zeigt dort „Playername not found“. Der Name selbst wird nicht verändert (Konfiguration und API stimmen überein).
- Freunde-Tipp auf Wunsch des Nutzers: nur Steam-Chat, Discord/Xbox ohne Absprung. Release-Build erfolgreich, 158 Prüfungen und 11 Monitor-Prüfungen bestanden. Keine Installation, kein Test gegen Port 8765.
- Offen: Discord-Direktnachricht (der Bot kennt die DM-Kanal-ID zwischen Nutzer und Freund nicht) und Xbox-Chat (kein offizieller Deep-Link, Xbox-App nicht installiert).

## Version 2.0.7 (23.10.2026): Touch-Ziele und Spielstunden – lokal, noch nicht installiert/veröffentlicht

- Übernahme von Claudes begonnenem Branch `codex/2.0.7`, Basis `3ef0ef3` (veröffentlichtes 2.0.6). Die begonnenen Plattform-ID-Ergänzungen wurden vervollständigt; die `Account`-Struktur bleibt mit bisherigen Aggregat-Initialisierungen kompatibel.
- Release-Build mit MSVC/CMake/Ninja erfolgreich. Datei-Version `2.0.7.0`, Produktversion `2.0.7`, Größe 13.150.208 Bytes. SHA-256: `0A55662586E7C9C41CE9F944328CA3DCD5311D9CEF72A9AEA14EB82B11D50396`.
- **164 Kernprüfungen bestanden**, einschließlich 78 neuer Prüfungen für Chat-/Profilziele, URL-Kodierung, Online-Priorität, fehlende Desktop-Apps, Startfehler/Fallback, Listen-Sortierung, Unterseiten und Umlauf, leere Zeilen, Kachelgrenzen und vier Displaygrößen. Die Startfunktion wird in diesen Prüfungen ersetzt: keine echten Chats oder Browser-Tabs geöffnet und keine Nachrichten gesendet.
- **11 Monitor-Basisprüfungen bestanden.** Kein sichtbarer Monitor-/Touch-Injektionstest für 2.0.7 durchgeführt.
- **49 API-/Navigationsprüfungen gegen die echte EXE bestanden**, einschließlich API-Version 2.0.7, manueller/automatischer Seitenwechsel, leerer/einzelner Ansichten, Eingabeprüfung, Host-/Origin-/Fetch-Metadata-/X-FL-Schutz und unveränderter neutraler Testkonfiguration. Eigene Testinstanz auf Port 8766, `--no-lcd`, ohne Zugangsdaten. Das Testskript unterstützt nun eine erwartete Version als Argument und beendet seine eigene Instanz auch bei Fehlern der lokalen Verbindung.
- Der erste Kernprüfungsversuch konnte im umgeleiteten Sandbox-Temp-Ordner keine Testkonfiguration schreiben; Wiederholung mit eigenem Temp-Ordner unter `cpp/build` erfolgreich. Der erste API-Versuch wurde durch die Netzwerk-Sandbox (Windows 10013) blockiert; Wiederholung mit erweiterter Ausführungsberechtigung erfolgreich. Beides waren Testumgebungsprobleme, keine bestandenen Tests.
- Deutsche/englische Beispielbilder in Querformat (480 × 320) und Hochformat (320 × 480) visuell geprüft: Spielstunden, K/D und Siegquote sichtbar. Überlappung von Titel und Spielerzahl in schmalen Kopfzeilen behoben. Weitere Bilder für 1920 × 720 und 637 × 720 erzeugt; keine physische Anzeigeabnahme daraus abgeleitet.
- **88 lokale Markdown-Linkziele und 10 eingebettete JavaScript-Blöcke geprüft**; neue Anleitungstexte in Vorlagen und HTML-Ressourcen stimmen überein. README/Schnellstart, Referenz, beide Haupt-Changelogs und C++-Doku gemeinsam aktualisiert. Download bleibt v2.0.6, 2.0.7 ist ausdrücklich lokal gekennzeichnet.
- Auswahlziele werden zusammen mit dem gezeichneten Datenstand erzeugt, einschließlich Sortierung und Unterseiten. Eine spätere Datenaktualisierung ändert das Ziel einer bereits angezeigten Zeile nicht. Tippen bei deaktivierter Touch-Steuerung öffnet kein Ziel; Wischen bleibt im bisherigen Gestenpfad.
- Vom Nutzer bestätigte Plattformvariante: Steam öffnet den Chat direkt, Discord/Xbox öffnen das Profil mit dem Weg zu „Nachricht“. Online erreichbare Konten zuerst, innerhalb beider Gruppen Discord → Steam → Xbox; fehlende Apps oder Startfehler führen zum nächsten Ziel. Separates Fenster/Tab bestimmt die Plattform-App bzw. der Browser.
- **Offen:** physische Touch-Abnahme der neuen Ziele und Öffnen in angemeldeten Discord-/Steam-/Xbox-Clients, Anmeldung, Chatberechtigungen und deren Fensterverhalten. Protokollzuordnung und Startübergabe sind kein Nachweis für einen erfolgreich geöffneten Chat. Installer, echte Installation, Autostart, COM-Port und laufende produktive App wurden für diesen Stand nicht verändert oder getestet. Installation und Veröffentlichung benötigen wie vereinbart die abschließende Nutzerfreigabe.

Prüfungen wiederholen (Entwicklerkonsole, eigener beschreibbarer Temp-Ordner; API-Test benötigt lokalen Netzwerkzugriff):

```bat
build\fl-tests.exe
build\fl-monitor-tests.exe
python tests/test_page_navigation.py build/FuchsLabs-USB-Display.exe 2.0.7
build\fl-tests.exe render build/preview-2.0.7-de de
build\fl-tests.exe render build/preview-2.0.7-en en
```

## Version 2.0.6 (23.10.2026): Blogbeiträge per Touch, Einstellungsfenster, Lizenzdateien (enthält 2.0.5: Touch, Taskleiste)

- Basis: veröffentlichter Stand 2.0.4 plus Lizenzdateien (main nach PR #12/#13).
- Release-Build erfolgreich; Datei-/Produktversion 2.0.6.0 / 2.0.6. 86 Kernprüfungen und 11 Monitor-Basisprüfungen bestanden; sichtbarer Monitor-/Touchtest mit injizierten Windows-Berührungen: 56 bestanden.
- Isolierte Instanz (eigene Instanzkennung, Port 18767, neutrale Konfiguration, Ausgabe auf einem Nebenmonitor): Taskleiste dieses Monitors wird ausgeblendet und nach dem Beenden wieder gezeigt; injiziertes Tippen auf einen Blogbeitrag öffnete den Artikel im Standardbrowser, Tippen außerhalb blätterte; beides im Protokoll vermerkt.
- Einstellungsfenster geöffnet, dann „Speichern & anwenden“ per API ausgelöst: Die neu gestartete Instanz öffnete das Fenster wieder.
- Touch (Stand 2.0.5: Wischen, Taskleiste) laut Tester auf echter Touch-Hardware einwandfrei. Blog-Tippen und Fenster-Fix nur simuliert bzw. per API geprüft. Veröffentlicht als v2.0.6.

## Version 2.0.4: Ansichten in der Übersicht durchblättern

- Basis: veröffentlichter Quellstand 2.0.3 (Commit 9e32bd3); separate 2.5-Plugin-Änderungen sind nicht enthalten.
- Release-Build erfolgreich; Datei-/Produktversion 2.0.4.0 / 2.0.4.
- 86 bestehende Kernprüfungen und 11 Monitor-Basisprüfungen bestanden. Keine sichtbaren Monitor-/Touchtests für diesen Stand durchgeführt.
- 49 neue Prüfungen gegen die echte EXE in isolierten Instanzen auf Port 8766 mit neutralen Konfigurationen, eigener Instanzkennung und --no-lcd bestanden: vor/zurück, Umlauf in beide Richtungen, keine/eine Ansicht, automatische Fortsetzung und neue Anzeigedauer nach manuellem Wechsel.
- Eingabeprüfung (einschließlich ungültigem JSON, falschen Typen und übergroßen Zahlen) sowie Host-/Origin-/Fetch-Metadata-/X-FL-Schutz geprüft. Unveränderter Prozess und unveränderte Konfigurationsdatei nach Navigation bestätigt.
- Browserprüfung DE/EN bestanden: neue Knöpfe, Positionsanzeige und tatsächlicher Seitenwechsel geprüft; Sprachänderung nur im Formular ausprobiert und verworfen.
- JavaScript-Syntax der Oberfläche und der eingebetteten Anleitungen sowie lokale Links der geänderten Dokumentation geprüft.
- Unterseiten und breite Kachelgruppen verwenden den vorhandenen Touch-Wechselpfad; kein zusätzlicher Laufzeittest mit gefüllten Listen oder breitem Display durchgeführt.
- Bei den isolierten Prüfungen keine Installation, Veröffentlichung, Autostartänderung oder physische Hardwareprüfung durchgeführt.

### Anschließende Installation von 2.0.4

Nach ausdrücklicher Nutzerfreigabe am 22.10.2026 im normalen Windows-Benutzerkontext außerhalb der Paketumleitung installiert. Bestehende EXE und Konfiguration zuvor lokal gesichert. Installer-Exitcode 0; installierte Datei-Version 2.0.4.0 und API-Version 2.0.4. SHA-256 stimmt mit dem geprüften Build überein: `EF7186C5ABF810DD1CC4D9F88E49883FC755AA10C65665D98FA61A39C54A177F`. Einstellungsdatei bytegleich erhalten, serielle Ausgabe und Autostart erhalten, genau ein Prozess aus dem regulären Installationsordner, `display_ok=true`. Neue Ansichten-Schnittstelle und Bedienelemente erreichbar. Unabhängige Abschlussprüfung erneut im normalen Windows-Kontext bestanden. Kein neuer Windows-Anmeldetest und keine visuelle/physische Touch-Abnahme durchgeführt. Am 23.10.2026 unverändert (gleicher Hash) als v2.0.4 auf GitHub veröffentlicht.

## Version 2.0.3: HDMI-Vollbild und Touch

- Basis: aktueller C++-Quellstand 2.0.2 im Veröffentlichungs-Repository. Die älteren Änderungen an Discord, gzip und Anleitung bleiben enthalten.
- Release-Build erfolgreich; Datei- und Produktversion `2.0.3.0` / `2.0.3`.
- Bestehende Kernprüfungen: 86 bestanden. Zusätzliche Gesten-/Touch-Metadatenprüfungen: 11 bestanden.
- Kontrollierter Windows-Test auf einem Nebenmonitor: 56 Prüfungen einschließlich der 11 Basisprüfungen bestanden. Geprüft wurden vollständige Monitorgeometrie, Abdeckung an allen vier Bildschirmrändern, Topmost/NoActivate/ToolWindow, Erhalt des Tastaturfokus, native injizierte Touch-Eingaben, genau ein Seitenwechsel je Tipp/Wischgeste, Abbruch, deaktivierter Touch, Mausbedienung und Ausblenden/Wiederherstellen. Alle eigenen Testfenster danach geschlossen.
- Isolierte Instanz auf Port 8766 mit neutraler Konfiguration und `--no-lcd`: API meldet 2.0.3; neue Touch-Metadaten verfügbar. Deutsche Einstellungsseite im Browser visuell geprüft, Touch-Zuordnung und Aktualisieren-Button funktionieren; ohne Touchgerät ist der Einrichtungsbutton deaktiviert.
- Neuer Einrichtungs-Endpunkt: GET 404, POST ohne Schutzheader 403, fremder Origin 403, Cross-Site 403, kein Touchgerät 400. Es wurde kein Windows-Assistent und keine echte Gerätezuordnung ausgelöst.
- Auf diesem PC ist kein Touchscreen angeschlossen. Echte Touch-Hardware, die Gerätezuordnung im Windows-Assistenten sowie dessen vollständiger Ausblenden/Rückkehr-Ablauf bleiben als physische Abnahme offen. Windows kann die physische Identität eines separat per USB angeschlossenen Touchgeräts nicht allein aus dem HDMI-Bildsignal ableiten; die Zuordnung erfolgt im Assistenten durch einen Fingertipp auf den vorgesehenen Bildschirm.
- Produktive App während der Tests unverändert: API 2.0.2, serielle Ausgabe, `display_ok=true`. Keine Installation, kein Autostart-/Registry-Eingriff und keine Veröffentlichung vorgenommen.

### Anschließende Installation von 2.0.3

Nach ausdrücklicher Nutzerfreigabe am 22.10.2026 über einen normalen Windows-Prozess außerhalb der Codex-Paketumleitung installiert. Unabhängige Abschlussprüfung: Datei-Version `2.0.3.0`, API-Version `2.0.3`, genau ein App-Prozess aus dem regulären LocalAppData-Installationsordner, serielle Ausgabe und `display_ok=true`. Installierter SHA-256 stimmt mit dem getesteten Build überein: `ADED00C0219623D75E4F15C485392C2CBCB0D9D0A443EB0E5A79BBABB914DD84`. Bestehende Einstellungsdatei vorhanden; HKCU-Run-Eintrag zeigt auf die installierte EXE mit `--autostart`. Ein neuer Windows-Anmeldetest und die physische HDMI-/Touch-Abnahme wurden nicht durchgeführt.

## Version 2.0.0

- Release-Build: 86 Prüfungen bestanden.
- Eigenes Win32/WebView2-Fenster sichtbar geöffnet und korrekt gerendert.
- Version 1.x und 2.0 nacheinander an COM7 mit derselben Konfiguration betrieben: beide display_ok=true. Nutzer bestätigte Ausrichtung, Farben, Lesbarkeit und Seitenwechsel für beide.
- Integrierter Installer: Einstellungen übernommen, PC auf 2.0 umgestellt, Autostart aktualisiert. Windows-Neustart mit automatisch erscheinendem Tray-Symbol vom Nutzer am 22.10.2026 bestätigt.
- Installierte und getestete EXE: SHA-256 1068EA18C4BA6C2B7ACDD464A33C3FE499A885734CCA84FC5F8CEFFB9A160998.
- WebView2 Evergreen Runtime erforderlich. EXE nicht signiert.
- Veröffentlichungskontrolle: alle 10 eingebetteten JPEG-Bilder der deutschen und alle 10 der englischen Anleitung einzeln sichtbar geprüft. Plattformbilder zeigen explizite Beispielkonten (FuchsKumpel, Beispielkonto/sample account); Discord zeigt Musternamen Lena/Max/Tom, Battlefield zeigt FuchsKumpel/Lena_Sniper/MaxPower99/TomTheTank/Mia2015. Kein echter Kontoschlüssel oder Bot-Token sichtbar; Felder sind maskiert. Guide-Ressourcen sind die vorliegenden anonymisierten Anleitungen und nicht die früher ausgeschlossenen Originalbilder.

## Windows-Neustarttest und Korrektur

Der erste reale Neustarttest scheiterte: Die App erschien nicht im Infobereich und das letzte Displaybild blieb stehen. Eine rein lesende Diagnose über einen Windows-Prozess außerhalb des Codex-Paketkontexts zeigte `ExeExists=false` für das Installationsziel und noch den alten Run-Eintrag. Die vorherigen Prüfungen waren durch die Paketumleitung des Hosts beeinflusst.

Korrektur: Einstellungen aus dem umgeleiteten Ordner übernommen und Installer aus einem normalen Windows-Prozess erneut ausgeführt. Dort bestätigt: InstallerExit=0, ExeExists=true, EXE-Hash identisch zum Release, Run-Eintrag auf die reguläre 2.0-EXE mit --autostart, API display_ok=true und autostart=true. Der Nutzer bestätigte am 22.10.2026 den erfolgreichen zweiten Windows-Neustart: Die App startete automatisch mit Tray-Symbol.
