# Phase 7 – Entwurf: Drag & Drop im Seiten-Editor, konfigurierbare eingebaute Seiten

Stand 2026-11-06, Branch `feature/zonen-layout`, Basis `0e363eb` (Phase 5 und 6 committet: `f49d559`, `cc2eca9`). Nur Entwurf, kein Produktivcode.
Pfade relativ zu `screenbuddy-store/cpp/`.

**Clean-Room:** Grundlage sind ausschließlich Bastis Beschreibung (unten), `docs/PHASE5-MIGRATIONSPLAN.md`, `docs/PHASE6-ENTWURF.md` und ScreenBuddy-Code. Kein libre-panel-Code, keine Webseite, kein GIF.
> Basti: Bedienung per Drag & Drop wie ein Modul-Editor. Bausteine mit der Maus aus einer Palette oder von einer Stelle auf die Seitenfläche ziehen, ablegen, verschieben, in der Größe ändern, entfernen. Live-Vorschau. Jede Seite komplett konfigurierbar, mit den Daten aus den bisherigen Datenquellen der Seite.

**Begriffe** (eigene): **Zone**, **Block**, **Bausteinleiste** (Palette), **Ablage** (Papierkorb), **Geist** (halbtransparente Einrastvorschau), **Griff** (Größenänderung), **Dichtestufe**, **Wechselfeld**, **Standardanordnung**, **Block-Katalog**.

**Änderung gegenüber Phase 5/6 (Basti, 2026-11-06):** Home Assistant gehört ebenfalls zum Editor („alles soll per Editor nutzbar sein“). Die Vorgabe „HA bleibt nativ“ ist aufgehoben. HA wird als **letzte** Seite migriert.

## 0. Ausgangslage (aus dem Code)

- **Editor (Phase 6):** `src/ui_page_layout.cpp` (Plugin-Auswahl, Raster, Zone, Blockliste, Vorschau, Undo/Redo, Übernehmen). `GridEditor` (`src/ui_layout_grid.*`) ist mit der Tastatur komplett bedienbar. Die Maus wählt heute nur eine Zelle per `release` (Z. 89–94). `press`/`drag` werden nicht genutzt.
- **Modell:** `plugins::Arrangement` (`plugins.h:80–83`). Die Operationen in `layout_edit.h:13–20` (`move_zone`, `resize_zone`, `add_zone`, `split_zone`, `delete_zone`, `assign_block`, …) sind transaktional. `EditHistory` arbeitet mit Schnappschüssen (100 Schritte) und Gruppen.
- **ui_kit:** Der Mausweg existiert bereits. `WM_LBUTTONDOWN` setzt `capture_` und ruft `press` auf, `WM_MOUSEMOVE` → `drag`, `WM_LBUTTONUP` → `release` (`ui_kit.cpp:1601–1684`). Es fehlen:
  - eine Mitteilung bei Abbruch: `WM_CAPTURECHANGED` (Z. 1679) setzt nur `pressed=false`.
  - ein Mauszeiger je Stelle: `WM_SETCURSOR` (Z. 1637) kennt nur Hand oder Pfeil.
- **Eingebaute Seiten:** Alle außer HA zeichnen über `draw_framed` (`page_frame.cpp`) mit **fester** Anordnung. Die Block-IDs stehen fest:
  - `rss.zeile.<i>`, `streaming.zeile.<i>`, `bf6.zeile.<i>`, `freunde.zeile.<i>`, jeweils mit `*.hinweis`
  - `system.cpu|gpu|ram|datentraeger|netz|gruppe` (`render.cpp:755`)
  - `spotify.cover|text|pegel|fortschritt|tasten|hinweis` (`render.cpp:1552`)

  HA zeichnet nativ (`render.cpp:1270–1351`, `cells()` mit höchstens 4 Kacheln). Die Treffer laufen über statische Geometrie (`touch.cpp:140–153`). Die Aktionen stehen in `app.cpp:621–702`, bei HA `request_toggle(id)` je Kachelindex (Z. 661–662).

## 1. Drag & Drop im bestehenden ui_kit

### 1.1 Ein Widget, keine fensterübergreifende Ablage
Bausteinleiste, Raster und Ablage liegen **im selben `GridEditor`**: links bzw. oben die Leiste, in der Mitte das Raster, unten rechts die Ablage. Damit genügt der vorhandene `capture_`-Weg, und ui_kit braucht kein allgemeines Drag-&-Drop-Protokoll. Alternative: siehe Entscheidung E1.

### 1.2 Interaktion (Maus)

| Geste | Wirkung | Modelloperation (ein Undo-Schritt) |
|---|---|---|
| Block aus der Leiste auf eine Zone ziehen | Block wird gezeigt und der Zone zugeordnet | `shown=true` + `assign_block` |
| Block aus der Leiste auf eine freie Zelle | neue 1×1-Zone mit diesem Block | `add_zone` + `assign_block` (neu: `place_block`) |
| Block-Chip in eine andere Zone ziehen | Block umhängen, Einfügeposition = Stapelposition | `assign_block` + Reihenfolge |
| Zone am Rumpf ziehen | Zone verschieben | `move_zone` (dx, dy in Zellen) |
| Griff (Ecke/Kante der gewählten Zone) ziehen | Spannweite ändern | `resize_zone` |
| Block oder Zone auf die Ablage, oder Entf | Block → `shown=false` (zurück in die Leiste). Eine Zone wird gelöscht, ihre Blöcke wandern in die Leiste. | neu: `remove_block`, `delete_zone` |
| Esc während des Ziehens, Fokusverlust, Capture verloren | Abbruch, Modell unverändert | – |

- **Ziehschwelle:** `GetSystemMetrics(SM_CXDRAG/SM_CYDRAG)`. Darunter gilt die Geste als Klick, wie heute (Auswahl).
- **Geist und Einrasten:** Während des Ziehens wird die Zielzelle bzw. das Zielrechteck aus dem Zellraster berechnet (dieselbe Rechnung wie `release`) und als Geist gezeichnet. Ein Ziel, das die Struktur verletzt, wird mit einer **Probe-Operation auf einer Kopie** geprüft (Transaktion wie `change`). Ein ungültiges Ziel bekommt einen gestrichelten Rahmen und den Text „passt nicht“, also **nicht nur Farbe**. Das Ablegen ist dann wirkungslos, es gibt einen Hinweis-Toast wie `reject()`.
- **Mauszeiger:** neuer virtueller `Widget::cursor(Point) → LPCWSTR`. Standard ist das heutige Verhalten. `WM_SETCURSOR` fragt das Widget unter der Maus, beim Ziehen `capture_`. Zeiger: Griff = `IDC_SIZEWE/NS/NWSE/NESW`, Rumpf = `IDC_SIZEALL`, ungültig = `IDC_NO`.
- **Abbruch:** neuer virtueller `Widget::cancel(Surface&)`. Er wird in `WM_CAPTURECHANGED` aufgerufen, wenn `capture_` noch gesetzt ist, und bei `VK_ESCAPE` im Widget.
- **Bildlauf:** Liegt der Zeiger beim Ziehen am Rand, wird der Raster-Ausschnitt verschoben (`first_column`) bzw. die Seite gescrollt (`Surface::scroll_to`). Begrenzt auf eine Zelle je 150 ms.
- **DPI:** Alle Treffer in DIP. Griff mindestens 8 DIP, Zelle wie bisher mindestens 32 DIP.

### 1.3 Tastatur bleibt vollwertig
Die Tabelle aus PHASE6 §7 bleibt. Neu dazu kommt **Aufnehmen und Ablegen** als Gegenstück zum Ziehen:

| Taste | Wirkung |
|---|---|
| F6 bzw. Strg+Tab innerhalb des Widgets | Bereich wechseln: Leiste → Raster → Ablage |
| Leertaste auf Block-Chip oder Zone | aufnehmen (Geist erscheint an der Fokuszelle) |
| Pfeile (aufgenommen) | Ziel bewegen, mit Geist und Gültigkeitsprüfung |
| Enter | ablegen (ein Undo-Schritt) |
| Esc | abbrechen |
| Entf | Block in die Leiste zurück bzw. Zone löschen |

- **MSAA:**
  - `EVENT_SYSTEM_DRAGDROPSTART/END` beim Aufnehmen und Ablegen bzw. Abbrechen.
  - `value()` nennt den Zustand, z. B. „Block spotify.cover aufgenommen, Ziel Zone kopf, gültig“ bzw. „… passt nicht“.
  - Die Leiste erscheint als Liste mit `Role::List`.
- **Kontrast:** Für den Geist, die Griffe und die Ablage gelten die Regeln aus PHASE6 §7 (Nicht-Text mindestens 3:1). Der Fokusring ist seit `cc2eca9` im hellen Theme abgedunkelt.

### 1.4 Undo/Redo, Live-Vorschau
- Ein Ziehvorgang ergibt **genau einen** `EditHistory::push`, und zwar beim Ablegen. Zwischenstände landen nie in der Historie. Die Gruppenbildung (600 ms) gilt weiter nur für Pfeilserien.
- Die Vorschau startet nach dem Ablegen über den vorhandenen Weg (`pending_` → `preview()` → `plugin_preview`). Während des Ziehens gibt es **keine** Vorschau-Aufträge, nur den Geist im Raster. Grund: Ein Vorschaulauf dauert zu lange für ein Mausereignis.
- Testbarkeit: `GridEditor::pointer(Kind{Down,Move,Up,Cancel}, Point)` ist der gemeinsame Weg für UI und fensterlose Tests. Das entspricht `input()` für Tasten.

## 2. Eingebaute Seiten konfigurierbar machen

### 2.1 Grundsatz: Standard bleibt der heutige Weg
- **Ohne Overlay oder mit gelöschtem Overlay** zeichnet jede Seite **unverändert** über ihren festen Weg (`regions()`, `layout()`, Zeilenraster, HA `cells()`). Die Baseline (`tests/builtin-snapshots/hashes.txt`) bleibt deshalb **per Konstruktion** bitgleich. Sie ist weiter der Beweis.
- **Mit Overlay** geht die Seite über `zones::plan()` und dieselben Block-Klassen mit `draw_framed`, also mit demselben Kopf und Fuß.
- Begründung: PHASE5 §2.3 hat gezeigt, dass `plan()` die Material-Anordnung und die `floordiv`-Kanten nicht exakt nachbildet. Eine „pixelgleiche Standardanordnung aus dem Raster“ ist deshalb nicht erreichbar, ein zweiter fester Weg schon.
- **Folge, ehrlich benannt:** Der Editor zeigt die Standardanordnung als **schematisches** Raster aus dem Katalog. Die erste Änderung schaltet auf den Rasterweg um, Abstände und Kanten können sich dabei um einige Pixel verschieben. Die UI sagt dazu: „Eigene Anordnung – Abstände folgen dem Raster“. „Auf Standard zurücksetzen“ löscht das Overlay und führt zurück auf den bitgleichen Weg.

### 2.2 Block-Katalog je Seite
Neue reine Datei `src/builtin_catalog.h/.cpp`. Pro Seitenschlüssel gibt es eine Liste von `BlockSpec {id, Text label, Dichte-Schwellen, max. Anzahl, Pflicht, Aktion}` und eine schematische Standardanordnung (Quer und Hoch). Die Daten kommen **nur** aus den heutigen Quellen.

| Seite | Blöcke (Katalog) | Datenquelle | Touch-Aktion | Pflicht |
|---|---|---|---|---|
| RSS `rss` | `rss.liste` (Zeilen aus der Fläche, höchstens 4) | `rss_->posts()` | Zeile → Beitrag | `rss.hinweis` |
| Streaming `streaming` | `streaming.liste` (Titel, Datum, Art, Logos/Kürzel) | `streaming_->snapshot()` | Zeile → Titel | `streaming.hinweis`. Fußzeile TMDB/JustWatch bleibt im Rahmen (Namensnennung) |
| BF6 `bf6` | `bf6.rangliste` (Zeilen, Blättern) | `bf6_->players()` | Zeile → Spieler | `bf6.hinweis` |
| Freunde/Discord | `freunde.liste` (Avatar, Name, Status, Aktivität, Plattformmarken), Blättern | `friends_`/`discord_` | Zeile → Profil | `freunde.hinweis` |
| Spotify | `spotify.cover`, `.text`, `.pegel`, `.fortschritt`, `.tasten` | `spotify_->snapshot()`, `meter_` | Cover → App, Tasten → Befehl | `spotify.hinweis` (Zustände) |
| System | `system.cpu`, `.gpu`, `.ram`, `.datentraeger`, `.netz` (inkl. Verlauf) | `sys_->snapshot()` | RAM/Datenträger/Netz → Werkzeug | – |
| Home Assistant | `ha.entitaet.<n>` je eingetragener Entität (Wert, Einheit oder Schalter) | `ha_->entities()` | Schalter → `request_toggle(entity_id)` | `ha.hinweis` |

- Listenseiten bieten **einen** Listenblock an statt `*.zeile.<i>`. Er verwendet die vorhandenen Zeilenblöcke intern weiter. Die Zeilenzahl ergibt sich aus der Fläche, `rows_per_page`/`pages(n)` aus der Anordnung.
- **Pflichtblöcke** (Hinweis, Zustandsmeldungen) lassen sich verschieben, aber nicht entfernen. Sonst bliebe die Seite bei „Lädt …“ oder „Nicht eingerichtet“ leer.
- Kopf und Fuß gehören zum Rahmen und sind nicht editierbar (E3).

### 2.3 Overlay-Speicherung pro Seite (analog Plugin)
- Datei `layouts/builtin.<seite>.layout.json` im App-Datenverzeichnis, mit derselben Form wie das Plugin-Overlay:
  `{schema_version, catalog_version, layout{columns,rows,zones}, portrait?{…}, blocks[{id,zone,shown}]}`
- Statt `manifest_sha256` gilt `catalog_version`. Bei einer abweichenden Version wird das Overlay ignoriert und als „veraltet“ gemeldet, mit der Taste „Zurücksetzen“, wie in PHASE6 §3.
- Lesen über `parse_strict_json` mit 16 KiB. Speichern per `atomic_write` mit `.bak`. Eine Datei, die nicht passt, führt auf die Standardanordnung und erzeugt einen Logeintrag. Ein Absturzpfad entsteht nicht.
- **Overlay gleich Standard:** Ein Overlay, das dem Katalog-Standard entspricht, wird nicht gespeichert (bzw. gelöscht). Die Seite bleibt auf dem festen, bitgleichen Weg. „Übernehmen“ ohne Änderung erzeugt also keinen Sprung.
- **Freunde und Discord:** ein gemeinsamer Katalog, aber eine Datei je Seitenschlüssel (`builtin.friends…`, `builtin.discord…`), weil beide Seiten gleichzeitig aktiv sein können (`app.cpp:630/637`).
- **Nicht in `config.json`:** kein `track_dirty`, kein Neustart. Wirkung sofort über `layout_revision` je Seitenschlüssel. Das Wechselfeld wird zurückgesetzt, wie in PHASE6 §3.
- Alle Ausgänge (USB, Monitor, Netzwerk, Companion) bekommen die Änderung automatisch, weil sie `page_list` nutzen.

### 2.4 Touch-Trefferpfade
- Standardweg: unverändert (`row_at`, `zone_at`, `button_at`, `cell_at`). Die Trefferkarten-Hashes bleiben gleich.
- Rasterweg: `render_arranged` liefert wie `plugins::DrawOut` eine Liste `targets {Rect, Aktion}` aus **demselben** `Plan`, mit dem gezeichnet wurde. `touch::Tile` nutzt dann diese Ziele statt der statischen Funktion, analog zu `tile.plugin` (`app.cpp:729`). Es gibt eine Quelle für Zeichnen und Treffer.
- **HA:** Die Aktion wird an die **Entitäts-ID** gebunden, nicht an den Kachelindex. Ein Schalter, der wegen zu wenig Fläche ausgeblendet ist, hat kein Ziel. So schaltet ein Tipp nie eine falsche Entität. Ein Test prüft das für jede Ziel-Entität.
- Spotify-Tasten behalten die Mindesttrefferfläche von 44 Punkten (`render.cpp:1400`).

### 2.5 Dichtestufen
- Im Standardweg gilt weiter nur `regular`.
- Im Rasterweg bekommt jeder Katalogblock Schwellen für `compact` und `regular`. `detailed` folgt erst später (E4). Ohne passende Fläche wird ein Block ausgeblendet; bei Pflichtblöcken lehnt der Editor die Anordnung ab.
- Beispiele für `compact`:
  - System-Karte nur mit Wert und Balken
  - Spotify-Text einzeilig
  - HA-Kachel nach der heutigen `compact`-Variante (`render.cpp:1345`)
- Im Rasterweg werden Clip und Fehlerisolierung je Block **neu** eingeführt (wie bei Plugins). Den Standardweg betrifft das nicht.

### 2.6 Editor und Vorschau
- `LayoutPage` bekommt eine Auswahl mit Plugins und eingebauten Seiten (Gruppen „Plugins“ / „Eingebaut“). Das Modell bleibt `Arrangement`. Statt `merge_arrangement` prüft `builtin::validate(catalog, model)`.
- Vorschau: `App::page_preview(key, overlay, size)` mit `render::samples` (`page_samples.h`). Dieselbe `render()`- bzw. `render_arranged()`-Methode wie auf dem Display, kein zweiter Renderer. Für HA kommt `samples::ha()` dazu (heute `demo_ha`).

### 2.7 Reihenfolge der Seiten (einfachste zuerst)
RSS → Streaming → BF6 → Freunde + Discord → System → Spotify → **HA zuletzt**.
- System kommt vor Spotify: Die Karten sind unabhängig, Spotify hat 9 Zustände und Abhängigkeiten zwischen Text, Fortschritt und Tasten.
- HA zuletzt, weil es das größte Risiko hat:
  - echte Schaltaktionen auf Geräte
  - bisher kein Block-Code und keine Phase-5-Migration
  - das Standardlayout muss zuerst bitgleich auf `draw_framed` umziehen (eigenes Paket P7-13, mit Baseline-Nachweis)

### 2.8 Rückrollbarkeit
- **Laufzeit:** Overlay löschen bzw. „Auf Standard zurücksetzen“ ⇒ bitgleicher fester Weg. Ein beschädigtes oder veraltetes Overlay ⇒ Standard.
- **Code:** ein Commit je Paket bzw. Seite, Revert mit `git revert`. Ohne Katalogeintrag erscheint die Seite nicht im Editor. Den festen Weg gibt es weiterhin, er ist der Standard und kein Schalter.
- Baseline-Updates bekommen nie einen gemeinsamen Commit mit Code (PHASE5 §6).

### 2.9 Aufwand und Risiken (ehrlich)
| Teil | Aufwand (Codex-Pakete, inkl. Review) | Hauptrisiko |
|---|---|---|
| Phase A: Drag & Drop Plugins (P7-1…P7-4) | 4 Pakete, ca. 4–6 Arbeitstage | Capture-Abbruch und Fokus in ui_kit (ganzes Fenster betroffen), Barrierefreiheit beim Aufnehmen und Ablegen |
| Phase B Infrastruktur (P7-5…P7-6) | 2 Pakete, ca. 3–4 Tage | Doppelte Wahrheit Standard/Raster, Revision und Wechselfeld |
| RSS, Streaming, BF6 | je 1 Paket, ca. 1–2 Tage | Blättern/`pages(n)` aus der Anordnung, Namensnennung TMDB |
| Freunde + Discord | 1 Paket, ca. 2 Tage | zwei Schlüssel, eine Basisklasse |
| System | 1 Paket, ca. 2–3 Tage | 4 Stile (Cupertino-Gruppe je Zone), Werkzeug-Aktionen |
| Spotify | 1 Paket, ca. 3 Tage | Zustände, Pegel, Tasten-Trefferflächen, Hoch/Quer |
| HA (2 Pakete) | ca. 4–5 Tage | Schalten falscher Entitäten, bitgleiche Migration des Standards |
| Abschluss (Doku) | 1 Paket, ca. 1 Tag | – |
| **Summe** | **15 Pakete, ca. 5–6 Wochen** inkl. Hardware-Abnahme | Sichtbarer „Sprung“ beim Wechsel von Standard auf Raster (§2.1) |

Weitere Risiken:
- Compact-Varianten sind neuer Zeichencode und brauchen neue Snapshots (`tests/layout-snapshots/`).
- Schriften stammen weiter aus dem Konstruktor (PHASE5 §1). Das gilt auch für den Rasterweg.

## 3. Aufgabenpakete für Codex

Gilt für jedes Paket:
- Immer nur **ein** Schreibender gleichzeitig, nur die genannten Dateien. Nicht committen, nicht pushen.
- Clean-Room: nur dieses Dokument, PHASE5/6 und ScreenBuddy-Code. Den Eintrag in `docs/CLEAN-ROOM-LAYOUT.md` setzt der Koordinator.
- Build mit `cpp\build.bat nopause`. Alle Suiten grün. **Built-in- und Plugin-`hashes.txt` sowie die Trefferkarten-Hashes bleiben unverändert**, sofern das Paket nichts anderes sagt.
- Kommentare deutsch. Bericht mit Dateien und Zeilen, Tests und Abweichungen.

**Phase A – Drag & Drop für Plugin-Seiten**

**P7-1 ui_kit: Zeiger und Abbruch**
- Dateien: `src/ui_kit.h` (Widget: `cursor(Point)`, `cancel(Surface&)`), `src/ui_kit.cpp` (`WM_SETCURSOR` Z. 1637, `WM_CAPTURECHANGED` Z. 1679, Esc während Capture), `tests/layout_ui_tests.cpp`.
- Akzeptanz: Standardwerte ändern kein bestehendes Widget. `cancel` kommt genau einmal bei Capture-Verlust, nicht nach einem regulären `release`. `native_mouse_smoke.py` ist grün.
- Tests: Folgen aus Down, Move, CaptureChanged und Down, Up ohne Fenster.

**P7-2 Modell: zusammengesetzte Operationen und Probe**
- Dateien: `src/layout_edit.h/.cpp` (`place_block`, `remove_block`, `move_block(block, zone, index)`, `probe(model, op) → bool`), `tests/layout_edit_tests.cpp`.
- Akzeptanz: jede Operation transaktional (bitgleiches Original bei einem Fehler). `probe` verändert nie das Modell.
- Tests: mindestens 25 neue Fälle (Grenzen 8 Zonen, 4 Blöcke je Zone, 12 Blöcke, Überlappung, Pflichtfelder).

**P7-3 GridEditor: Leiste, Raster, Ablage, Ziehen, Griffe, Geist**
- Dateien: `src/ui_layout_grid.h/.cpp` (`pointer()`, `press/drag/release/cancel/cursor`, Block-Chips je Zone, Leiste, Ablage, Aufnehmen und Ablegen per Tastatur, MSAA), `tests/layout_ui_tests.cpp`.
- Akzeptanz:
  - Alle Gesten aus §1.2 und die Tasten aus §1.3 funktionieren.
  - Ein Ziehvorgang ergibt genau einen `on_edit`, Esc oder Abbruch keinen.
  - Ungültige Ziele sind ohne Farbe erkennbar.
  - Bedienbar bei 200 % DPI.
  - `value()` wie in §1.3.
- Tests: Zeigerfolgen und Tastenfolgen fensterlos, Undo nach dem Ziehen stellt das Modell bitgleich wieder her.

**P7-4 Editor-Seite einbinden und Abnahme**
- Dateien: `src/ui_page_layout.cpp` (Hinweistext, Vorschau nach dem Ablegen, Fokus nach dem Ablegen), `tests/native_settings_smoke.py` (Aufnehmen und Ablegen per Tastatur, über MSAA geprüft).
- Akzeptanz:
  - Die Vorschau läuft nur nach dem Ablegen, nie während des Ziehens.
  - `track_dirty=false` überall.
  - Handabnahme mit Maus, Touchpad und Touch-Bildschirm (Maus-Emulation).

**Phase B – Eingebaute Seiten**

**P7-5 Katalog, Overlay, Prüfung** (rein)
- Dateien: neu `src/builtin_catalog.h/.cpp`, neu `tests/builtin_catalog_tests.cpp`, `CMakeLists.txt` (eine Quelle plus Testziel), `build.bat`, `tests/run_release_checks.py`.
- Akzeptanz: `validate` lehnt unbekannte Blöcke, entfernte Pflichtblöcke und eine falsche `catalog_version` ab. Die Standardanordnungen aller Seiten sind gültig.

**P7-6 Store, App, Vorschau, Touch-Ziele**
- Dateien: `src/app.h/.cpp` (Overlay laden, `layout_revision` je Schlüssel, `page_preview`, Ziele aus der Anordnung), `src/touch.h/.cpp` (Ziele aus der Anordnung bevorzugen), `src/native_settings.h` (Rückrufe `builtin_layout_get/apply/reset`), `src/page_frame.*` (Clip und Isolierung nur im Rasterweg), Tests.
- Akzeptanz:
  - Ohne Overlay sind alle Hashes und Trefferkarten unverändert.
  - Atomar mit `.bak`.
  - Ein veraltetes oder beschädigtes Overlay führt auf den Standard.

**P7-7 … P7-12 Seiten** (je ein Paket, Reihenfolge §2.7: P7-7 RSS, P7-8 Streaming, P7-9 BF6, P7-10 Freunde + Discord, P7-11 System, P7-12 Spotify)
- Dateien: `src/render.h/.cpp` (nur der Abschnitt der Seite: `render_arranged`, compact-Variante), `src/builtin_catalog.cpp` (Seite freischalten), `tests/builtin_snapshot_tests.cpp` (neue Fälle unter `tests/layout-snapshots/`).
- Akzeptanz:
  - Overlay gleich Standard ⇒ kein Overlay, fester Weg, Hashes bitgleich. Jede echte Änderung ⇒ Raster.
  - Die Trefferziele entsprechen den gezeichneten Rechtecken (abgetastet im 4-px-Raster).
  - Blättern und `pages(n)` stimmen.
  - Pflichtblöcke bleiben sichtbar.

**P7-13 HA: Standard auf den gemeinsamen Rahmen** (bitgleich, noch kein Editor)
- Dateien: `src/render.h/.cpp` (HA-Abschnitt, Blöcke `ha.entitaet.<n>` und `ha.hinweis` über `draw_framed`, `cell_at` bleibt auf `cells()`).
- Vorher: Der Fußtext wird heute während des Zeichnens bestimmt (`render.cpp:1348`). Er wird zuerst eine reine Funktion `ha_footer(...)`, wie `spotify_footer` in P5-5.
- HA ist danach kein Kontrollbild mehr für `header/footer/message/fit` (PHASE5 §4). Diese Rolle übernimmt nur noch Willkommen, deren Hashes unverändert bleiben müssen.
- Akzeptanz: HA-Hashes und die Trefferkarte `cell_at` bitgleich, `tests.cpp`-HA-Fälle grün.

**P7-14 HA im Editor**
- Dateien: `src/render.*` (HA `render_arranged`), `src/builtin_catalog.cpp`, `src/app.cpp` (Aktion je Entitäts-ID, Z. 653–663), Tests.
- Akzeptanz:
  - Ein Tipp schaltet ausschließlich die gezeichnete Entität.
  - Eine ausgeblendete Kachel hat kein Ziel.
  - Entitäten ohne Schalter haben kein Ziel.
  - Hardware-Abnahme mit echtem HA durch Basti.

**P7-15 Abschluss:** Doku (PLUGIN-SYSTEM, NAVIGATION, FUNKTIONEN DE/EN, ANLEITUNG), CHANGELOG, TESTPROTOKOLL, CLEAN-ROOM-LAYOUT, Hardware-Abnahme offen eingetragen.

**Schreibkonflikte:**
- `ui_kit.*` nur in P7-1.
- `render.cpp` immer nur durch ein Seitenpaket gleichzeitig.
- `app.cpp` in P7-6 und P7-14.
- `CMakeLists.txt` nur in P7-5.

## 4. Offene Entscheidungen für Basti

1. **E1 Bausteinleiste:** im Raster-Widget (Vorschlag, kein neues ui_kit-Protokoll) oder als eigene Liste mit allgemeinem Drag & Drop zwischen Widgets (ca. +1 Paket, betrifft das ganze Fenster)?
2. **E2 „Sprung“ beim ersten Bearbeiten** (§2.1) akzeptieren, oder eingebaute Seiten nur mit fester Standardanordnung plus Ein-/Ausblenden und Reihenfolge erlauben (kleiner, ohne Sprung, aber nicht „komplett konfigurierbar“)?
3. **E3 Kopf und Fuß** fest lassen (Vorschlag) oder auch als Blöcke (Uhr, Titel, Stand)? Die Fußzeile von Streaming muss wegen der Namensnennung TMDB/JustWatch bleiben.
4. **E4 Dichtestufen:** in Phase 7 nur `compact` und `regular` (Vorschlag), `detailed` später?
5. **E5 Hochformat:** eigenes `portrait`-Layout im Overlay (Vorschlag), oder dasselbe Raster für beide Ausrichtungen?
6. **E6 Wechselfeld auf eingebauten Seiten** schon in Phase 7 erlauben oder erst danach?
7. **E7 HA:** Sollen Schalter-Kacheln mehr als die heutigen 4 Entitäten zeigen dürfen (Katalog bis 8)? Muss vor dem Schalten eine Bestätigung erscheinen, wenn die Kachel kleiner als 44 Punkte ist?
8. **E8 Ablageort** `layouts/` im App-Datenverzeichnis (Vorschlag) und Export/Import der Anordnungen erst später?
