# Phase 6 – Entwurf: Seiten-Editor und Erststart

Stand 2026-11-06, Branch `feature/zonen-layout`. Phase 0–4c und der Phase-5-Plan sind committet, die P5-0-Baseline liegt noch uncommittet im Arbeitsbaum. Dieses Dokument ist nur ein Entwurf, es enthält keinen Produktivcode.
Grundlage: `PLAN-screenbuddy-zonen-layout.md` (Phase 6, Leitplanken, Clean-Room) und der ScreenBuddy-Code. libre-panel-Code wurde nicht gelesen.
Entscheidung Basti: Ein Editor ist erwünscht. Pfade sind relativ zu `screenbuddy-store/cpp/`.

Korrektur zum Hauptplan (Z. 75): `PagesPage` steht in `src/ui_page_general.cpp:318`, nicht in Z. 289.

## 1. Umfang in einem Satz

Der Editor ordnet die Blöcke **installierter Plugins mit Manifest v2** in Zonen an. Er ändert nie das Manifest selbst, sondern speichert eine getrennte **Anordnung**, die mit denselben Parser-Regeln geprüft und mit demselben Renderer gezeigt wird.

## 2. Was der Editor nicht darf

- **Kein Ändern der Manifestdatei.** Die Host-Freigaben hängen am SHA-256 von `manifest.definition` (`src/plugin_store.cpp:136–143`, Prüfung `read_consent` Z. 161). Ein umgeschriebenes Manifest würde alle Freigaben ungültig machen, und Store-Plugins würden nicht mehr zur Signatur passen.
- **Keine Inhalte.** Nicht editierbar sind Quellen, URLs, Header, `links`, `tap` (also auch kein neues `open_link`), Settings, Texte, Vorlagen, Widget-Typen, Block-IDs, `refresh_seconds` und Metadaten. Ein Feld für rohes JSON gibt es nicht.
- **Keine Secret-Settings.** In der `show_if`-Auswahl erscheinen nur Quellen und Settings, die nicht `secret` sind. Der Parser lehnt Secrets ohnehin ab (`src/plugins.cpp:268`).
- **Keine v1-Plugins.** Leitplanke „v1 unverändert“: Bei einem v1-Plugin erscheint nur der Hinweis „Anordnung braucht Manifest v2“.
- **Keine eingebauten Seiten.** Phase 5 nutzt feste Anordnungen und bewusst kein `zones::plan()` (PHASE5 §2.3). Eingebaute Seiten bleiben im Editor deshalb außen vor. Ihre Reihenfolge und Sichtbarkeit stellt weiter `PagesPage` ein.
- **Die HA-Seite bleibt nativ** (`HaRenderer`) und erscheint im Editor nicht.
- **Keine Grenzen über das Manifest hinaus.** Es gelten nur die `zones::`-Konstanten (`src/plugin_zones.h:23–29`): 6×6 Raster, Anteile 1–12, 8 Zonen, 4 Blöcke je Zone, 12 Blöcke.

## 3. Datenmodell: Anordnung als Overlay

Neue Datei im Plugin-Verzeichnis: `<id>.layout.json`. Sie passt nicht auf das Suchmuster `*.flplugin.json` in `plugin_store.cpp:208`.
```
{ "schema_version": 1,
  "manifest_sha256": "<fingerprint(definition)>",
  "layout": { "columns": [..], "rows": [..], "zones": [ {id,column,row,column_span,row_span,cycle?} ] },
  "blocks": [ {"id": "<block-id>", "zone": "<zone-id>", "shown": true, "seconds": 8?, "show_if": {path,op?,value?}?} ] }
```
- `blocks` enthält **jede** Block-ID des Basis-Manifests genau einmal. Die Reihenfolge gilt als Stapel- und Wechselreihenfolge. Mit `shown: false` wird ein Block aus der Seite genommen. Unbekannte, fehlende oder doppelte IDs machen das Overlay ungültig.
- **Zusammenführen** (rein): `definition` kopieren, `layout` ersetzen, `widgets` in Overlay-Reihenfolge neu aufbauen (nur die gezeigten Blöcke). Dabei werden genau `zone`, `seconds` und `show_if` ersetzt, alle anderen Felder bleiben byteweise gleich. Das Ergebnis geht mit `dump()` an `parse_manifest` (`src/plugins.cpp:398`). Damit gelten dieselben Regeln, auch die Querregeln: `seconds` nur bei `rotate` (Z. 545), `next` nur in einer Wechselzone (Z. 561), jede Zone braucht einen Block (Z. 569), keine Überlappung (Z. 318).
- **Gegenprüfung nach dem Parsen:** `source_fingerprint`, `link_fingerprint`, Settings, `tap` je Block und die Typ- und Textfelder müssen gleich der Basis sein. Weicht etwas ab, wird das Overlay abgelehnt.
- **Grenzen:** Das Overlay wird mit `parse_strict_json` und dem Limit 16 KiB gelesen, mit einer Liste erlaubter Felder wie im Parser.
- **Veraltet:** Passt `manifest_sha256` nicht mehr, etwa nach einem Plugin-Update, wird das Overlay ignoriert. Die Datei bleibt liegen und die UI zeigt „Anordnung passt nicht mehr zur Plugin-Version“ mit der Taste „Zurücksetzen“. Das Verhalten entspricht dem der Freigaben.
- **Startwert im Editor:** Hat das Manifest ein `layout`, übernimmt der Editor es 1:1. Ohne Layout (höchstens 6 Blöcke, `plugins.cpp:498`) schlägt er eine Spalte vor, mit einer Zeile und einer Zone je Block.

**Store:** `Installed` (`src/plugin_store.h:10–16`) bekommt zwei Felder:
- `std::optional<Manifest> arranged`: das geprüfte Ergebnis.
- `uint64_t layout_revision`: wird wie `generation` nicht gespeichert.

`Installed::manifest` bleibt die Basis, denn sie ist die Grundlage für Freigabe und Fingerabdruck. **Kein Hochzählen von `generation`:** `generation` sichert Daten und Freigaben (`app.cpp:541`, `544`, `1186`). Würde sie hochgezählt, verschwänden die Live-Daten bis zum nächsten Abruf. Deshalb führt `layout_revision` zu einem Reset des Wechselfelds: Der Vergleich in `app.cpp:568` vergleicht das Paar (generation, layout_revision). Die Alternative wäre, `generation` hochzuzählen. Das ist einfacher, zeigt aber kurz „keine Daten“. Entscheidung beim Koordinator.

**App:** In `src/app.cpp:531` wird `installed->arranged.value_or(installed->manifest)` gezeichnet. Das gilt für alle Ausgänge, also USB, Monitor, Netzwerk und Companion, weil alle `page_list` nutzen.

## 4. Editor-Seite (UI)

- Neue Seite `src/ui_page_layout.cpp`. Sie wird in `src/ui_pages.cpp:32` (nach `make_plugins_page`) und in `ui_pages.h` registriert, mit `id "layout"`, Gruppe `App` und dem Titel „Plugin-Seite anordnen“ / „Arrange plugin page“.
- Einstiege:
  - Taste „Anordnen …“ in `ui_page_plugins.cpp:44–48`
  - Verweis in der `PagesPage`-Notiz (`ui_page_general.cpp:333`)
- **Alle Editor-Widgets** setzen `track_dirty = false`. Sonst zählt `recount()` (`native_settings.cpp:397–404`) sie mit, und „Speichern & anwenden“ löst einen Neustart aus.

**Aufbau** (von oben nach unten, eine Spalte, passt in Material, Fluent und Cupertino):
1. `Combo` mit der Plugin-Auswahl. Angeboten werden nur aktivierte v2-Plugins, v1-Plugins mit Hinweis.
2. **Raster** (neues Widget `GridEditor`): Spalten- und Zeilenanteile als Zahlenfelder 1–12 mit den Tasten +/− für Spalte und Zeile. Die Zellen sind schematisch gezeichnet, Zonen erscheinen als beschriftete Rechtecke.
3. **Zone**: ID (Textfeld, Regel `identifier`), Wechsel (`Segment`: aus / rotate / first_available) und Übergang (fade / none).
4. **Blockliste**: `ItemList` (`ui_kit.h:453`) mit Haken = gezeigt und ▲/▼ bzw. Alt+Pfeil für die Reihenfolge. Pro Block gibt es dazu:
   - Zone (`Combo`)
   - `seconds` (nur bei rotate)
   - `show_if`: Quelle oder Setting (ohne Secrets), Operator und Wert
5. **Vorschau**: über `Host::preview` (`ui_kit.h:638`) im vorhandenen Vorschaubereich, mit `Segment` Quer / Hoch.
6. Meldungszeile mit dem Prüfergebnis und die Tasten „Rückgängig“, „Wiederholen“, „Übernehmen“ (Primary) sowie „Auf Manifest zurücksetzen“ (Danger, mit Rückfrage).

**Raster und Vorschau sind getrennt.** Zonenrahmen werden nicht in die Vorschau gezeichnet, denn dafür wären Geometriedaten aus `plugin_render.cpp` nötig, und diese Datei ist für Phase 5 gesperrt. Das Raster ist ein eigenes, schematisches Widget.

## 5. Live-Vorschau (kein zweiter Renderer)

- Weg: Modell → Zusammenführen (§3) → `App::plugin_preview` (`src/app.cpp:242–259`). Darin laufen `parse_manifest`, `sample_data` (Plugin-`sample`), `render_page` und `draw_page(..., sample=true)`. Prüfung und Bild kommen also aus **einem** Aufruf.
- Neu ist eine Überladung `plugin_preview(manifest, error, std::pair<int,int> size)` für den Quer-/Hochvergleich. Ohne Größenangabe gilt die heutige `size()`.
- Die Steuerung übernimmt das Muster aus `DesignPage::preview` (`ui_page_design.cpp:513–534`): Es läuft nie doppelt, Änderungen während eines Laufs lösen genau einen Folgeaufruf aus (`again_`), und es gibt eine Generationsprüfung. Bei einem ungültigen Modell bleibt das letzte Bild stehen. Beim Verlassen der Seite folgt `preview(std::nullopt)` (vgl. `native_settings.cpp:376`).
- Eingebaute Seiten werden nicht angeboten (§2). Sollten sie später dazukommen, kämen ihre Beispieldaten ausschließlich aus `render::samples` (`src/page_samples.h`) und gingen an dieselben `render()`-Methoden.

## 6. Rückgängig / Wiederholen

- Das Modell ist klein (höchstens 8 Zonen und 12 Blöcke). Deshalb werden **Schnappschüsse des ganzen Modells** gespeichert, nicht einzelne Befehle. Grenze: 100 Schritte. Die Historie gehört der Seite, nicht den Widgets, denn nach „Verwerfen“ werden die Widgets neu gebaut (`ui_kit.h:19–20`).
- Ein Schritt ist eine abgeschlossene Aktion:
  - Bei Textfeldern zählt der Fokuswechsel bzw. Enter.
  - Wiederholte Pfeiltasten am selben Element innerhalb von 600 ms werden zu einem Schritt zusammengefasst.
  - Ein Plugin-Wechsel und „Übernehmen“ beginnen eine neue Historie.
- Rein und testbar in `src/layout_edit.h/.cpp`: `class EditHistory { push, undo, redo, can_undo, can_redo }` und die Modelloperationen (Zone verschieben, Spannweite ändern, Zone teilen oder löschen, Block umhängen).
- Tasten: Strg+Z / Strg+Y bzw. Strg+Umschalt+Z, wenn `GridEditor` oder die Blockliste den Fokus hat. Die Nachrichtenschleife fängt nur Strg+S ab (`native_settings.cpp:944`). In einem EDIT-Feld wirkt Strg+Z also wie gewohnt auf den Text. Die Tasten auf der Seite sind immer erreichbar.

## 7. Tastatur und Barrierefreiheit

**`GridEditor`** (fokussierbar, eigene Datei `src/ui_layout_grid.h/.cpp`, abgeleitet von `ui::Widget`):

| Taste | Wirkung |
|---|---|
| Pfeile | Zellfokus bewegen |
| Enter / Leertaste | Zone der Zelle wählen bzw. auf einer freien Zelle eine neue 1×1-Zone anlegen |
| Umschalt+Pfeil | Spannweite der gewählten Zone ändern |
| Alt+Pfeil | Zone verschieben (wie `ItemList`, `ui_kit.cpp:943`) |
| Entf | Zone löschen, ihre Blöcke wandern in die erste Zone (Rückfrage, wenn das nicht passt) |

- **MSAA:** `role()` = `Role::List`. `value()` nennt die Zone mit Spalten- und Zeilenbereich, Wechselmodus und Blockzahl, etwa „Zone kopf, Spalte 1–2, Zeile 1, 2 Blöcke“. Bei jeder Änderung ruft das Widget `Surface::notify(EVENT_OBJECT_VALUECHANGE)` auf. Die gewählte Zone wird nicht nur farblich markiert, sondern auch mit ID-Text und einer dickeren Rahmenlinie (2 px).
- **Kontrast in hellen Themes:**
  - Text und Nebentext auf der Karte mindestens 4,5:1
  - Zonenrahmen, gewählte Zone und Fokusring mindestens 3:1 gegen Karte und Hintergrund (Nicht-Text)
- **Befund (gerechnet, nicht gemessen):** Der Fokusring (`ui_kit.cpp:1533`) nutzt `th.accent`. Im hellen FuchsLabs-Theme ist das `ORANGE` #F77F00 auf `CREME`/`SAND`, also etwa 2,4:1 bzw. 2,1:1 und damit **unter 3:1**. Vorschlag: Im hellen Theme `accent_text` (`GLUT` #B05200, etwa 4,2:1 auf Sand) für Ring und Auswahl im Editor verwenden. Den globalen Ring nur nach Freigabe durch Basti ändern, weil sich das im ganzen Fenster zeigt.
- Größe: Jede Rasterzelle ist mindestens 32×32 DIP groß. Bei 200 % DPI und schmaler Breite wird gescrollt, nichts überlappt.
- Texte DE/EN als `T(...)`. Die Parser-Meldungen sind feste englische Texte (`plugins.cpp`). Eine Zuordnungstabelle Meldung → DE/EN in `layout_edit.cpp` übersetzt sie, mit dem allgemeinen Rückfall „Anordnung ungültig“.

## 8. Validierung

- **Sofort:** Strukturregeln über `zones::valid(Layout)` (`plugin_zones.h:127`), dazu Zählgrenzen, damit der Nutzer schnell eine Rückmeldung bekommt.
- **Maßgeblich:** das Zusammenführen und `parse_manifest` (§3). Die Taste „Übernehmen“ ist nur aktiv, wenn beides gelingt.
- **App-seitig:** Dieselbe Prüfung läuft noch einmal in `Store::set_arrangement`. Der UI wird nicht vertraut.
- **Fehlerort:** Die Meldung nennt Zone oder Block, und der Fokus springt auf das betroffene Feld (Muster `ui::Invalid` mit Widget, `native_settings.cpp:448–451`).

## 9. „Übernehmen“: atomar, mit Backup

- Neue Rückrufe in `NativePluginActions` (`src/native_settings.h:47–62`): `arrangement_get(id)` → {Basis-Definition ohne Secrets, Overlay, Zustand ok|stale|none}, `arrangement_apply(id, overlay)` und `arrangement_reset(id)`. Sie werden in `app.cpp:1257–1275` nach demselben Muster verdrahtet, laufen im Job-Thread und melden Fehler per `throw std::runtime_error`.
- Ablauf in `Store::set_arrangement(id, json)`:
  1. Zusammenführen und prüfen.
  2. Die bisherige Datei als `<id>.layout.json.bak` sichern (`atomic_write` mit dem alten Inhalt).
  3. Die neue Datei per `atomic_write` schreiben (`plugin_store.cpp:90–127`: exklusive Temp-Datei, `FlushFileBuffers`, `MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH)`).
  4. `arranged` und `layout_revision` im Speicher setzen und `mark_dirty()` aufrufen.

  Ist Schritt 3 fehlgeschlagen, bleibt der alte Stand im Speicher.
- `arrangement_reset` löscht beide Dateien. `Store::remove` (`plugin_store.cpp:312–326`) entfernt auch `layout.json` und `.bak`.
- **Kein Neustart** und kein `config::save`. Die Anordnung ist eine sofort wirksame Plugin-Aktion wie „Aktivieren“.
- **Scan-Grenze:** In `Store::load` (Z. 205) gilt eine Grenze von 256 Einträgen. Pro Plugin liegen höchstens 6 Dateien im Verzeichnis, bei 32 Plugins also 192. Das passt.

## 10. Erststart: Bedarf und Umfang

- **Heute vorhanden:**
  - `ui.setup_done` (`config.cpp:65`)
  - Beim ersten Start öffnen sich die Einstellungen (`main.cpp:270–271`).
  - `setup_done` wird bei jedem Speichern gesetzt (`native_settings.cpp:461`).
  - Die Karte „Einrichtung in 3 Schritten“ (`ui_page_general.cpp:50–59`) navigiert nur, sie zeigt keinen Fortschritt.
- **Bewertung:** Ein eigener Assistent lohnt sich nicht. Die drei Schritte decken die Einrichtung ab. Es fehlt nur die Rückmeldung, was schon erledigt ist. Der Editor ist für den Erststart optional.
- **Umfang S:**
  1. Ein Status je Schritt aus vorhandenen Daten (`Status`, `config()`), z. B. „✓ USB verbunden“, „2 Quellen aktiv“, „5 Seiten“. Die Daten liegen schon in `OverviewPage::update` vor (Z. 71–91).
  2. Ein optionaler 4. Schritt „Plugin-Seite anordnen“, nur wenn ein aktiviertes v2-Plugin installiert ist.
  3. Solange `setup_done` false ist, steht die Karte oben auf der Übersicht.

  Keine neuen Konfigurationsschlüssel.

## 11. Schreibkonflikte mit Phase 5

- P5-0 hat uncommittet geändert: `CMakeLists.txt`, `build.bat`, `tests/run_release_checks.py`, `TESTPROTOKOLL.md`, `CHANGELOG*.md`, `docs/CLEAN-ROOM-LAYOUT.md`, `src/main.cpp`, `src/render.*`, `page_samples.*`, `tests/builtin*`, `snapshot_support.h` und `tests/plugin_snapshot_tests.cpp`. **Phase 6 fasst keine dieser Dateien an, solange P5-0 nicht committet ist.**
- Für die ganze Phase 6 gesperrt: `render.cpp/.h`, `page_frame.*`, `page_samples.*`, `plugin_render.cpp`, `main.cpp` und `HaRenderer`.
- **Ohne CMake-Änderung** (sofort möglich): P6-1 und P6-2. Sie arbeiten in vorhandenen Dateien und Zielen (`plugins.cpp`, `plugin_store.*`, `app.*`, `native_settings.h`, `tests/plugins_tests.cpp`, `tests/plugin_store_tests.cpp`).
- **Mit CMake-Änderung:** P6-3 bis P6-5 bringen neue Quellen in `CORE_SOURCES`. Sie starten erst nach dem Commit von P5-0 und nie gleichzeitig mit P5-1 (das `page_frame.cpp` einträgt). Pro Paket wird nur eine Zeile angehängt.
- Doku, Changelog, `TESTPROTOKOLL.md` und `CLEAN-ROOM-LAYOUT.md` werden gesammelt in P6-6 nach P5-7 bearbeitet. Den Clean-Room-Eintrag je Paket setzt der Koordinator.

## 12. Aufgabenpakete für Codex

Allgemein für jedes Paket:
- Nur ein Paket gleichzeitig, nur die genannten Dateien. **Nicht committen, nicht pushen.**
- Clean-Room: nur dieser Entwurf, der Hauptplan und ScreenBuddy-Code, kein libre-panel-Code.
- Build `cpp\build.bat nopause` bzw. `cmake --build build/store --target <ziel>`.
- Alle Suiten grün (Parser 223, Speicher 127, Transport 134, Settings 135). Plugin- und Built-in-Hashes bleiben unverändert.
- Kommentare deutsch. Bericht mit Dateien und Zeilen, Tests und Abweichungen.

**P6-1 Overlay-Modell, Zusammenführen, Prüfung** (rein, ohne UI, ohne CMake)
- Dateien: `src/plugins.h/.cpp` (neu `struct Arrangement`, `parse_arrangement`, `merge_arrangement`, `arrangement_from`, `to_json`), `tests/plugins_tests.cpp`.
- Akzeptanz:
  - Das Zusammenführen ruft `parse_manifest` auf und gibt dessen `ParseResult` zurück.
  - v1 wird abgelehnt.
  - Fehlende, doppelte oder unbekannte Block-IDs werden abgelehnt, ebenso jede Abweichung bei Quellen, Links, Settings, `tap` oder Text.
  - Secret in `show_if` wird abgelehnt.
  - Ohne Änderung ist das Ergebnis bei einem Manifest mit Layout gleich der Basis (`definition` gleich).
  - Overlay über 16 KiB oder mit doppelten Schlüsseln wird abgelehnt.
- Tests:
  - Tabelle mit mindestens 25 Fällen, darunter Grenzen 6×6, 8 Zonen, 4 Blöcke je Zone, Überlappung, `seconds` ohne rotate, `next` ohne Wechselzone, leere Zone und alle Blöcke `shown:false`.
  - Bestandstests unverändert grün.

**P6-2 Store und App: Speichern, Backup, Anwendung**
- Dateien: `src/plugin_store.h/.cpp` (`arranged`, `layout_revision`, `set_arrangement`, `clear_arrangement`, Laden mit Fingerabdruck, `remove`), `src/app.h/.cpp` (Z. 531 `effective`, Z. 565–569 Revisionspaar, Z. 1257–1275 Rückrufe, Überladung `plugin_preview` mit Größe), `src/native_settings.h` (drei Rückrufe), `tests/plugin_store_tests.cpp`.
- Akzeptanz:
  - Freigaben und Links bleiben nach `set_arrangement` gültig, `generation` bleibt gleich.
  - Ein veraltetes Overlay wird ignoriert und als `stale` gemeldet.
  - Nach einem simulierten Schreibfehler bleibt der alte Stand erhalten.
  - Die `.bak` enthält den Vorgänger.
  - `remove` entfernt beide Dateien.
  - Link- und Mehrfachsicherheit wie bei `atomic_write`.
- Tests: Speicher-Suite mindestens 127 plus neue Fälle (Laden mit, ohne und mit veraltetem Overlay, beschädigte Datei, Scan-Grenze, Plugin-Update ersetzt die Basis).

**P6-3 Bausteine Raster und Historie** (nach dem Commit von P5-0)
- Dateien:
  - neu `src/layout_edit.h/.cpp`: Modelloperationen, `EditHistory`, Meldungstabelle DE/EN
  - neu `src/ui_layout_grid.h/.cpp`: `GridEditor`
  - `CMakeLists.txt`: zwei Quellen in `CORE_SOURCES`, neues Ziel `fl-layout-edit-tests` mit `plugins.cpp`, `plugin_zones.cpp` und `layout_edit.cpp`
  - neu `tests/layout_edit_tests.cpp`
  - `build.bat` und `run_release_checks.py`: Aufruf der neuen Suite
- Akzeptanz:
  - Jede Modelloperation erhält `zones::valid` oder meldet einen Fehler.
  - Undo und Redo stellen das Modell bitgleich wieder her, über 100 Schritte gedeckelt.
  - Folgen von Pfeiltasten werden zu einem Schritt zusammengefasst.
  - `GridEditor` ist mit der Tastatur komplett bedienbar (Tabelle §7).
  - Der MSAA-Wert ist wie in §7 beschrieben.
- Tests: Modell und Historie (mindestens 30 Fälle), Tastenfolgen am Widget ohne Fenster, so weit `Surface` das zulässt, sonst in P6-5.

**P6-4 Editor-Seite mit Live-Vorschau**
- Dateien:
  - neu `src/ui_page_layout.cpp`
  - `src/ui_pages.h/.cpp`: Registrierung Z. 32
  - `src/ui_page_plugins.cpp`: Taste „Anordnen …“ Z. 44–48
  - `CMakeLists.txt`: eine Quelle
- Akzeptanz:
  - Die Vorschau kommt ausschließlich aus `plugin_preview`. Es gibt keinen eigenen Zeichencode und keinen Zugriff auf `plugin_render.cpp`.
  - Bei einem ungültigen Modell bleibt das letzte Bild stehen, eine Meldung erscheint und „Übernehmen“ ist inaktiv.
  - `track_dirty = false` überall: Der Zähler für „Speichern & anwenden“ bleibt bei 0.
  - „Übernehmen“ wirkt ohne Neustart auf dem Display.
  - „Zurücksetzen“ fragt vorher nach.
  - Ein v1-Plugin zeigt nur den Hinweis.
  - Kein Secret erscheint in der Auswahl oder im MSAA-Text.
- Tests: Handabnahme auf USB 480×320, hochkant, Monitor und Companion. `fl-tests` grün.

**P6-5 Barrierefreiheit, Kontrast, Vorschau gegen Snapshots**
- Dateien:
  - `tests/tests.cpp`: Kontrasttest über `ui::Theme` hell, Design-hell (alle Profile) und Windows-hell mit festen Einstellungen. Text und Nebentext auf Karte mindestens 4,5:1, Ring und Auswahl mindestens 3:1.
  - `tests/native_settings_smoke.py`: Editor per Tastatur, also Tab, Raster, Strg+Z/Y und Übernehmen, geprüft über MSAA.
  - `tests/plugin_snapshot_tests.cpp`: Die Vorschau eines unveränderten Overlays für jedes v2-Beispiel ergibt denselben Hash wie die Baseline.
  - optional `ui_kit.cpp:1533`: Ring im hellen Theme mit `accent_text`, nur nach Freigabe durch Basti
- Akzeptanz: Alle Kontrastpaare bestehen, oder der Befund ist dokumentiert und freigegeben. Der Smoke-Test ist grün, und die Plugin-`hashes.txt` ist unverändert.
- Abhängigkeit: Commit von P5-0 und P5-1, weil P5-1 `tests.cpp` berühren darf.

**P6-6 Erststart und Doku** (nach P5-7)
- Dateien: `src/ui_page_general.cpp` (Z. 50–59 Status je Schritt, optionaler 4. Schritt, Position), `docs/PLUGIN-SYSTEM.md` (+ `.en`, Abschnitt „Anordnung“), `docs/NAVIGATION.md` (+ `.en`), `docs/FUNKTIONEN.md`/`FEATURES.en.md`, `res/GUIDE.html`/`ANLEITUNG.html`, `CHANGELOG.md`/`.en.md`, `TESTPROTOKOLL.md`, `docs/CLEAN-ROOM-LAYOUT.md`.
- Akzeptanz:
  - Keine neuen Konfigurationsschlüssel.
  - Die Statustexte kommen nur aus `Status` und `config()`.
  - Der 4. Schritt fehlt ohne v2-Plugin.
  - Texte DE/EN vollständig.
  - Hardware-Abnahme durch Basti offen eingetragen.

## 13. Offene Entscheidungen für Basti

1. Den Fokusring im hellen Theme global auf `GLUT` umstellen (Befund §7) oder nur im Editor?
2. `layout_revision` (Vorschlag) oder `generation` hochzählen? Das zweite ist einfacher, zeigt aber kurz keine Daten.
3. `show_if` im Editor bearbeitbar machen (Vorschlag: ja, ohne Secrets) oder nur anzeigen?
4. Später auch eingebaute Seiten anordnen? Das ginge nicht vor dem Abschluss von Phase 5 und bräuchte eine eigene Baseline.
