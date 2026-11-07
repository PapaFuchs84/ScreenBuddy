# Phase 5 – Migrationsplan: eingebaute Seiten auf Block-Schnittstelle

Stand 2026-11-06, Branch `feature/zonen-layout` (Phase 0–4c committet). Nur Plan, kein Produktivcode.
Grundlage: `PLAN-screenbuddy-zonen-layout.md` (Phase 5, Leitplanken, Clean-Room) und ScreenBuddy-Code. Kein libre-panel-Code gelesen.
Entscheidung Basti: eingebaute Seiten werden migriert. **`HaRenderer` bleibt nativ und wird nicht angefasst.**
Pfade relativ zu `screenbuddy-store/cpp/`.

Korrektur der Zeilen im Hauptplan (Z. 68): `tile_layout` steht in `src/render.cpp:202` (nicht 196). `Region` steht in `src/render.h:104`, `regions()` in `render.cpp:548`. `Layout` (Spotify) steht in `render.h:187`, `layout()` in `render.cpp:1031`.

## 1. Inventur

Alle Seiten zeichnen über `Renderer` (`render.h:56–85`). Gemeinsam genutzt werden `header` (`render.cpp:326–340`, 34 px), `footer` (342–348, 18 px), `message` (279–297), `fit` (265–277) und `font` (245–247). Die Seitenliste baut `App::page_list` (`src/app.cpp:523–696`), die Zeichner hält `RendererSet` (`app.cpp:38–56`). Die Trefferprüfung läuft über statische Funktionen in `src/touch.cpp:140–153`.

| Seite (Schlüssel) | Zeichencode `render.cpp` | Zeilen | App-Anbindung `app.cpp` | Touch | Risiko |
|---|---|---|---|---|---|
| Blogs (`rss`) | 788–857 (`wrap`, `short_title`, `row_at`, `render`) | ~70 | 688–692 | `RssRenderer::row_at` (Zeile → Beitrag) | **niedrig**: 4 feste Zeilen, 3 Zustände. Achtung: `ago(p.published)` (Z. 846) liest die Systemuhr. |
| Streaming (`streaming`) | 860–929 | ~70 | 684–687 | `StreamRenderer::row_at` | **niedrig–mittel**: 6 Zustände, Logos/Kürzel/„+n“, Pflicht-Fußzeile TMDB/JustWatch |
| Battlefield 6 (`bf6`) | 452–532 (`sort_players`, `render`, `bf_row`) | ~80 | 636–643 | `Renderer::row_at`, `rows_per_page` | **mittel**: Blättern, Zeilenhöhe je Stil (`row_height`, Z. 27), Rang-Icon über `thumbnail` |
| Freunde + Discord (`friends`, `discord`) | 350–449 (`sort_friends`, `render_friends`, `badges`, `avatar`, `row`) | ~100 | 621–635 | `Renderer::row_at`, `touch::friend_rows` | **mittel–hoch**: eine Methode der Basisklasse für 2 Seitenschlüssel. Avatar über `round_image`, Statussymbole, Blättern. |
| Spotify (`spotify`) | 1025–1185 (`layout`, `button_at`, `render`) | ~160 | 655–683 | `SpotifyRenderer::button_at` (3 Tasten + Cover) | **hoch**: 9 Zustände, Pegelbalken in 3 Farbmodi plus „simuliert“, Hoch- und Querformat. Kopfzeile mit `header(d, 0, "")` (Z. 1082) zeigt die Systemuhr. Die Fußzeile wird während des Zeichnens bestimmt. |
| System (`system`) | 535–785 (`regions`, `zone_at`, `render`, `grouped_system`, `expressive_system`, Karten) | ~250 | 612–620 | `SysRenderer::zone_at` (RAM/Datenträger/Netz öffnen Werkzeuge) | **hoch**: 3 verschiedene Anordnungen (FuchsLabs/Fluent, Cupertino, Material). `regions()` wird gleichzeitig für Zeichnen und Treffer genutzt. Netz-Verlaufskurve. |
| Home Assistant (`homeassistant`) | 932–1023 | ~90 | 644–654 | `HaRenderer::cell_at` | **nicht migriert**. Bleibt nur als Kontrollbild in der Baseline. |
| Willkommen (keine Seite) | 299–324 | ~25 | 732–737 | – | nicht migriert, Kontrollbild |

Befunde, die Phase 5 **nicht** ändert (Verhalten bleibt gleich, für später notiert):
- `sets_` wird nur ab mehr als 16 Größen geleert (`app.cpp:858`), nicht beim Wechsel des Designs. Die Schriftfamilie aus `font()` wird im Konstruktor festgelegt und bleibt nach einem Stilwechsel veraltet, bis die App neu startet. Migrierte Blöcke übernehmen die Schriften deshalb weiter aus dem Konstruktor und berechnen sie nicht beim Zeichnen neu. Eine Neuberechnung beim Zeichnen würde das Verhalten unbemerkt ändern.
- Um `p.fn` und `p.plugin_draw` gibt es kein `try/catch` (`app.cpp:698–730`, `render_core` 822–908). Migrierte eingebaute Seiten bekommen deshalb **keine** Fehlerisolierung und keinen Clip.

## 2. Zielarchitektur (eigene Begriffe)

Begriffe: **Seitenrahmen** (Kopf, Körper, Fuß), **Block** (`plugins::zones::Block`, `src/plugin_zones.h:71–80`), **feste Anordnung**, **Vorschau-Eingabe**.

1. **Ein gemeinsamer Einstieg, zwei Rahmenvarianten.** Neue Datei `src/page_frame.h/.cpp` mit der Funktion `render::draw_framed(const Rahmen&, const std::vector<Platzierung>&)`:
   - Leinwand anlegen.
   - `header(now, info)` zeichnen.
   - die Blöcke in der festen Reihenfolge zeichnen.
   - `footer(text)` zeichnen und `finish()` aufrufen.
   - Die Reihenfolge Kopf → Körper → Fuß bleibt wie heute, weil die Kantenglättung von der Zeichenreihenfolge abhängt.

   Die Variante „Eingebaut“ nutzt den heutigen Kopf (34 px, Fuchs, Uhr) und Fuß (18 px). Die Plugin-Seiten (`plugin_render.cpp:441–467`, Band 32 px, Ränder 39/19) bleiben in Phase 5 **unverändert**. Kopf und Fuß werden optisch nicht vereinheitlicht.
2. **Blöcke in der Körnung der heutigen Zeichenfunktionen**: je Seite eine Zeile, eine Karte, ein Cover usw. Jeder Block implementiert `zones::Block`:
   - stabile ID, z. B. `rss.zeile.0`, `system.cpu`, `spotify.tasten`.
   - `min_size(normal)` = die Fläche bei der Basisgröße 480×320 bzw. 320×480.
   - `compact` und `detailed` geben unendlich zurück, werden also nie gewählt. Damit gibt es **eine** Dichtestufe und kein neues Verhalten.
   - Die Blöcke verwenden die Schriften und das Theme ihres `Renderer` (Verweis im Block, die Schriften kommen aus dem Konstruktor).
3. **Feste Anordnung statt `zones::plan()`.** Die Geometrie liefern die vorhandenen Funktionen:
   - System: `regions()`
   - Spotify: `layout()`
   - Listen-Seiten: Zeilenraster aus `HEADER_H + 2`, `row_height()` bzw. `floordiv(bot - top, MAX_ROWS)`

   Diese Funktionen werden zu Anbietern, die `zones::PlacedBlock`-Rechtecke liefern (gleicher Typ wie bei Plugins). Damit können Tap-Ziele, Wechselfeld und der spätere Editor dieselbe Datenform verwenden. Das Raster mit Bruchteilen (`plan()`) wird **nicht** benutzt: die Material-Anordnung (`min(94, body*.38)`) und die `floordiv`-Kanten lassen sich mit ganzzahligen Anteilen nicht exakt nachbilden.
4. **Rechenregel für Bitgleichheit.** Ein Block rechnet `area.x/area.y + derselbe Versatz wie heute`. Er bildet keine neuen Ausdrücke. Beispiel: RSS heute `y + 12` mit `y = top + i*rh`, danach `area.y + 12` mit `area.y = top + i*rh`. Die rechten Kanten bleiben `w - 10`, deshalb sind die Zeilenblöcke vollbreit (`x = 0`, `width = w`). Lässt sich ein Teil so nicht bitgleich halten, wird er für diese Seite ein einzelner Block über den ganzen Körper. Das ist dokumentierter Rückfall und kein Fehlschlag.
5. **Treffer aus derselben Anordnung.** `row_at`, `zone_at` und `button_at` behalten ihre Signaturen, weil `touch.cpp` und `tests.cpp` sie aufrufen. Intern fragen sie den Anordnungs-Anbieter. Es gibt eine Quelle für Zeichnen und Treffer, wie heute bei `regions()`.
6. **Daten sind rein, Zeichnen ist getrennt.**
   - Die Fußzeile ist eine reine Funktion der Eingabe, z. B. `spotify_footer(snap)`. Sie wird nicht mehr während des Zeichnens bestimmt.
   - Der Seitenindex beim Blättern wird beim Aufbau der Blockliste gebunden.
   - Sortierung (`sort_friends`, `sort_players`) bleibt vor der Platzierung.
7. **`app.cpp` bleibt fast unverändert.** `PageEntry.fn` ruft weiter `rs.<seite>.render(...)` mit denselben Argumenten auf. `render()` ruft intern `draw_framed`. `tile_layout`, `compose`, `RendererSet`, `active_pages` und die Seitenschlüssel bleiben gleich.
8. **Nicht in Phase 5** (jeweils Folgeschritt mit eigener Baseline-Aktualisierung):
   - Clip und Fehlerisolierung für eingebaute Blöcke
   - echte Dichtestufen
   - Zonen-Raster per Manifest oder Editor
   - Wechselfeld auf eingebauten Seiten
   - einheitlicher Kopf für Plugins und eingebaute Seiten
   - Hinweis „Beispieldaten“ in der Vorschau

## 3. Baseline-Strategie

Neues Testziel **`fl-builtin-snapshot-tests`** (`tests/builtin_snapshot_tests.cpp`, Baseline unter `tests/builtin-snapshots/`). Aufbau wie `fl-plugin-snapshot-tests` (`CMakeLists.txt:113–118`): nur die Testdatei, `flcore` PRIVATE, Modi `prüfen` (Standard), `update` und `export <ordner>`.

**Determinismus (Pflicht, sonst ist keine Baseline gültig):**
- **Feste Wanduhr.** P5-0 fügt zwei Nahtstellen ein. Im Betrieb haben beide den Standardwert, das Verhalten bleibt gleich.
  - `RssRenderer::render` reicht `now` an `ago(p.published, now)` weiter (`render.cpp:846`).
  - `SpotifyRenderer::render` erhält zusätzlich `std::time_t clock = 0` für `header(d, clock, "")` (`render.cpp:1082`). Der vorhandene Parameter `now` ist die monotone Zeit des Fortschritts. Der Test setzt `fetched_at` und `now` fest.
- **Zeitzone.** `clock_hhmm` nutzt `localtime_s` (`util.cpp:194`). Der Test setzt in `main` `_putenv_s("TZ","UTC0")` und ruft `_tzset()` auf. Feste Zeit 1792606500.
- **Sprache.** `i18n::set_lang("de")` ist Standard. Ein Lauf mit `en` läuft für die Designs `dunkel` und `custom-material` in den Größen 480×320 und 320×480.
- **Reihenfolge.** Erst `apply_theme`, dann den Zeichner bauen, für jeden Fall neu, weil `font()` den Stil im Konstruktor festhält.
- **Kein Netz, kein Port 8765, kein Audio, keine Registry.** Logos und Cover sind künstliche Bilder (`demo_stream_logos`, ein einfarbiges Cover- und Avatarbild aus `Canvas`). Das Design `windows` wird inline aus festen `WindowsSettings` gebaut, wie in `plugin_snapshot_tests.cpp:57–73`.
- **Schriften.** Der Test prüft beim Start, ob Roboto und Segoe UI Variable installiert sind (DirectWrite-Systemschriften). Fehlt eine, bricht der Test mit einer Meldung ab und vergleicht nichts. Eine Baseline gilt nur auf dem Rechner, der sie erzeugt hat.

**Matrix:**
- **Designs:** `theme_cases()` aus dem Plugin-Test: dunkel (Referenz), hell, windows, custom (alle Stil FuchsLabs), custom-cupertino, custom-fluent, custom-material. `theme_cases()`, `hash_image()` und `write_png()` wandern in den gemeinsamen Kopf `tests/snapshot_support.h`. Der Plugin-Test bindet ihn ein, seine `hashes.txt` muss **unverändert** bleiben.
- **Größen:** 480×320, 320×480, 240×160, 160×240, 960×640 (Skalierung 2), die Kachel aus `tile_layout(1920,720)` (der Test berechnet sie, nach Rechnung 958×720), 480×300 (Gerät mit Sperrzone oben 20) sowie Handy 1080×2400 und 2400×1080.
- **Zustände je Seite** (Eingaben aus §5 plus Randfälle im Test):
  - RSS: Daten (Umbruch, Präfix `heise+ |`, Beitrag ohne Datum), lädt, leer
  - Streaming: Ok mit Logos, Ok ohne Logos (Kürzel), mehr als `MAX_LOGOS` („+n“), NoKey, Loading, Error, leer
  - BF6: Seite 1, Seite 2, Spieler mit Fehler, lädt, leer, Rang ohne Icon, mit künstlichem Icon
  - Freunde: Seite 1, Seite 2, getrennt mit Meldung, leer, mit Avatarbild, offline, mobil, Plattformmarken; zusätzlich die Discord-Titelvariante
  - Spotify: Playing mit Pegeln in Farbmodus 0/1/2, simuliert, Paused, controls aus, last_played, Idle leer, NotSetUp, LoggedOut, Connecting, Error, mit Cover, Hoch- und Querformat
  - System: voll; ohne HWiNFO; alle Werte `nullopt`; Temperatur 75/85; Auslastung ≥ 90; Verlauf mit weniger als 2 Werten
  - HA (5 Zustände wie `tests.cpp:830–834`) und Willkommen
- **Trefferkarte:** Je Seite, Design, Größe und Zustand werden `row_at`, `zone_at`, `button_at` und `cell_at` auf einem 4-px-Raster abgetastet und als Hash gespeichert, dazu `rows_per_page()` und `pages(n)`.
- **Ablage:** `hashes.txt` (FNV-1a 64 wie im Plugin-Test) für alle Fälle. PNGs werden nur für die Referenz eingecheckt (dunkel, de, alle Größen, alle Zustände). Alle übrigen Bilder erzeugt `export` bei Bedarf. Abweichende Bilder schreibt der Test nach `%TEMP%\fl-builtin-snapshot-actual`.
- **Toleranz: bitgleich.** Ausnahmen gibt es nur mit schriftlicher Begründung im Paketbericht, Freigabe durch den Koordinator und einem eigenen Baseline-Commit. Ein Baseline-Update gehört **nie** in denselben Commit wie eine Migration.
- **Vergleich vor/nach:** Auf demselben Rechner `export` auf dem Basis-Commit und auf dem Kandidaten laufen lassen, dann die Hashes vergleichen. So spielen Schriftunterschiede zwischen Rechnern keine Rolle. `run_release_checks.py` prüft gegen die eingecheckte `hashes.txt`.

## 4. Reihenfolge

P5-0 Baseline-Gerüst → **RSS** → **Streaming** → **BF6** → **Freunde + Discord** (ein Paket, zwei Schlüssel, derselbe Code) → **Spotify** → **System** → P5-7 Abschluss.
HA wird nie migriert. In **jedem** Schritt müssen die HA- und Willkommen-Hashes unverändert sein. Das ist der Nachweis, dass gemeinsame Teile (`header`, `footer`, `message`, `fit`) nicht verändert wurden.
BF6 kommt vor Freunde: damit steht das Muster „Zeilenliste mit Blättern“ an einer Seite mit nur einem Schlüssel, bevor die Basisklassen-Methode für zwei Seiten umgebaut wird.

## 5. Vorschau-Eingaben (kein zweiter Renderer)

- Neue Datei `src/page_samples.h/.cpp` (Namensraum `render::samples`):
  - `demo_*` aus `render.cpp:1187–1269` wandert hierher, die alten Namen bleiben als Weiterleitung, bis keine Aufrufer mehr da sind.
  - Neu kommen `system()` (der `SysSnapshot` aus `tests.cpp:812–821`) und `posts(lang)` (`tests.cpp:798–804`) dazu.
- Diese Eingaben nutzen der Baseline-Test, `run_demo` (`src/main.cpp:146–176`) und später der Editor (Phase 6). Alle geben sie an **dieselben** `render()`-Methoden. Es gibt keinen eigenen Vorschauzeichner.
- Die Beispieldaten sind eingefroren. Eine Änderung erfordert einen eigenen Commit mit Baseline-Update. Kein Hinweis „Beispieldaten“ in Phase 5.
- Die HA-Beispieldaten (`demo_ha`) ziehen mit um. Das ist reine Verschiebung, `HaRenderer` bleibt unverändert.

## 6. Rückroll-Mechanismus

**Entscheidung: getrennter Commit je Seite, kein Feature-Schalter.** Ein Schalter hielte zwei Zeichenwege für dieselbe Seite am Leben (ein zweiter Renderer) und bräuchte einen Konfigurationsschlüssel.
- Ein Paket ist eine Seite, ersetzt den alten Zeichenweg und ergibt einen Commit (`Phase 5-n Zonen-Layout: <Seite> auf Blöcke`). Den Commit setzt der Koordinator nach dem Review, nicht Codex.
- Rückrollen geht mit `git revert <commit>`. Jeder Commit ist für sich baubar und grün, weil die Baseline unverändert bleibt.
- Baseline-Commits (P5-0, begründete Ausnahmen) sind von Migrations-Commits getrennt.
- Hardware-Abnahme durch Basti nach jeder Seite oder gesammelt vor dem Release. Ein Befund führt zum Revert des betroffenen Commits.

## 7. Aufgabenpakete für Codex

Allgemein für **jedes** Paket:
- Nur ein Paket gleichzeitig. Nur die genannten Dateien ändern. **Nicht committen, nicht pushen.**
- Clean-Room: nur dieser Plan, der Hauptplan und ScreenBuddy-Code, kein libre-panel-Code. Zeile in `docs/CLEAN-ROOM-LAYOUT.md` §6 ergänzen.
- Build `cpp\build.bat nopause` oder `cmake --build build/store --target <ziel>`.
- Alle Suiten grün (Parser 223, Speicher 127, Transport 134, Settings 135). **Plugin-Hashes und Built-in-Hashes unverändert.** Die Trefferkarten-Hashes bleiben ebenfalls unverändert.
- `HaRenderer` (`render.cpp:932–1023`, `render.h:155–171`), `plugin_render.cpp`, `tile_layout` und `compose` nicht ändern.
- Kommentare deutsch, Stil wie die Umgebung. Bericht mit geänderten Dateien und Zeilen, Testergebnissen und jeder Abweichung.

**P5-0 Baseline-Gerüst** (vor jeder Migration, enthält die einzigen Produktivänderungen ohne Bildwirkung)
- Dateien:
  - `src/render.h/.cpp`: Nahtstellen `ago(..., now)` in RSS (Z. 846) und `clock` für Spotify (Z. 1078/1082)
  - neu `src/page_samples.h/.cpp` (in `CORE_SOURCES`, `CMakeLists.txt:28–36`)
  - `src/main.cpp` (`run_demo` auf `samples`)
  - neu `tests/snapshot_support.h`, `tests/plugin_snapshot_tests.cpp` (nur auf den Kopf umstellen)
  - neu `tests/builtin_snapshot_tests.cpp` und `tests/builtin-snapshots/`
  - `CMakeLists.txt` (Ziel `fl-builtin-snapshot-tests` wie 113–118), `build.bat` (Aufruf neben Z. 35), `tests/run_release_checks.py` (`suites`)
  - `TESTPROTOKOLL.md` (Abschnitt „Built-in-Snapshots“)
- Akzeptanz:
  - Plugin-`hashes.txt` byteweise unverändert.
  - Ein zweiter Lauf von `prüfen` direkt nach `update` ist grün.
  - Zwei Läufe hintereinander liefern identische Hashes. Das gilt auch, wenn vorher in der Shell eine andere Zeitzone gesetzt ist, z. B. `set TZ=NZST-12NZDT`. Der Test setzt `TZ` selbst, das Ergebnis muss gleich bleiben.
  - Matrix wie §3 vollständig. Fehlende Schrift bricht mit Meldung ab.
  - `--demo --preview` erzeugt dieselben Bilder wie vorher (Hash-Vergleich der `demo-*.png` vor/nach).
  - `fl-tests` grün.

**P5-1 Seitenrahmen + RSS**
- Dateien: neu `src/page_frame.h/.cpp` (in `CORE_SOURCES`), `src/render.h` (RssRenderer 129–140), `src/render.cpp` (788–857), `tests/tests.cpp` (nur bei Bedarf Aufrufe).
- Umsetzung:
  - `draw_framed` mit der Variante „Eingebaut“.
  - Blöcke `rss.zeile.<i>` (0–3) und `rss.hinweis` (zentrierter Einzeiler für „Lade Feeds …“ und „Keine Beiträge gefunden“).
  - Anordnung mit `top = HEADER_H + 2`, `rh = floordiv(bot - top, MAX_ROWS)`. `row_at` nutzt dieselbe Anordnung.
- Akzeptanz: alle Built-in- und Plugin-Hashes plus Trefferkarten bitgleich; `fl-tests` (Touch-Zeilen `tests.cpp:765–774`) grün; kein Aufruf von `zones::plan`.

**P5-2 Streaming**
- Dateien: `src/render.h` (142–153), `src/render.cpp` (860–929).
- Umsetzung:
  - Blöcke `streaming.zeile.<i>` (bis `MAX_ROWS`) und `streaming.hinweis` (ein- oder zweizeilig, NoKey).
  - Die Fußzeile ist eine reine Funktion aus `state` und `updated`.
  - Logos und Kürzel werden unverändert gezeichnet.
- Akzeptanz wie P5-1, zusätzlich `tests.cpp:738–743` (row_at) grün.

**P5-3 Battlefield 6**
- Dateien: `src/render.h` (87–95), `src/render.cpp` (452–532).
- Umsetzung:
  - Blöcke `bf6.zeile.<i>` mit gebundenem Seitenindex.
  - Platzierung `body_top + i*row_height()`.
  - Fußtext rein („Seite x/y · Stand“).
  - Hinweisblock für „Lade …“ und „Keine Spieler …“.
- Akzeptanz wie P5-1 in allen Stilen, weil die Zeilenhöhe vom Stil abhängt.

**P5-4 Freunde + Discord**
- Dateien: `src/render.h` (56–85), `src/render.cpp` (350–449).
- Umsetzung:
  - Blöcke `freunde.zeile.<i>`, mehrzeiliger Hinweisblock (`message`).
  - `row`, `avatar` und `badges` werden Teile des Zeilenblocks.
  - `welcome`, `header`, `footer`, `message`, `fit` und `rows_per_page`/`row_at` bleiben in der Signatur unverändert.
  - Beide Schlüssel (`app.cpp:621–635`) laufen ohne Änderung in `app.cpp`.
- Akzeptanz wie P5-1, zusätzlich: Hashes von Willkommen und HA unverändert, `tests.cpp:701–711` grün.

**P5-5 Spotify**
- Dateien: `src/render.h` (173–194), `src/render.cpp` (1025–1185).
- Umsetzung:
  - Blöcke `spotify.cover`, `spotify.text` (Titel, Künstler, Album), `spotify.pegel`, `spotify.fortschritt`, `spotify.tasten`, `spotify.hinweis`.
  - Anordnung aus `layout()`.
  - `button_at` bleibt auf `layout()`.
  - `spotify_footer(snap)` ist rein.
  - Die Zeichenreihenfolge entspricht genau Z. 1107–1178.
- Akzeptanz wie P5-1 für alle Spotify-Zustände; `tests.cpp:443–519` (Tasten, Cover-Tipp, Pegel) grün.

**P5-6 System**
- Dateien: `src/render.h` (97–127), `src/render.cpp` (535–785).
- Umsetzung:
  - Blöcke `system.cpu`, `system.gpu`, `system.ram`, `system.datentraeger`, `system.netz`, je Stil gleiche IDs.
  - Cupertino zusätzlich `system.gruppe` (gemeinsamer Hintergrund, wird zuerst gezeichnet). Die Trennlinien gehören zum jeweiligen Zeilenblock.
  - Anordnung = `regions()` (unverändert). `zone_at` bleibt auf `regions()`.
  - Die Zonen-Nummern (`ZONE_RAM/DISK/NET`) und die Aktionen in `app.cpp:613–619` bleiben gleich.
- Akzeptanz wie P5-1 in allen 4 Stilen; `test_themes` (`tests.cpp:852–884`) grün.

**P5-7 Abschluss**
- Dateien: `src/render.cpp` (Weiterleitungen `demo_*` entfernen, wenn ohne Aufrufer), `docs/ARCHITEKTUR.md`, `docs/PLUGIN-SYSTEM.md` (Abschnitt „Eingebaute Seiten nutzen die Block-Schnittstelle“), `CHANGELOG.md`/`CHANGELOG.en.md` („keine sichtbare Änderung“), `docs/CLEAN-ROOM-LAYOUT.md` (Zeile Phase 5), `TESTPROTOKOLL.md`.
- Akzeptanz:
  - Alle Hashes unverändert gegenüber P5-0.
  - `grep` findet keinen alten Zeichenweg der migrierten Seiten mehr.
  - Hardware-Abnahme durch Basti offen eingetragen: USB 480×320 und hochkant, Monitor 1920×720, Netzwerk-Display, Companion, Android-TV, Touch auf allen Tasten und Zeilen.

## 8. Ausentschiedene heikle Punkte (Kurzliste)

1. Feste Anordnung aus den vorhandenen Geometriefunktionen, `zones::plan()` wird nicht benutzt. Grund: Bitgleichheit.
2. Eine Dichtestufe (normal). `compact` und `detailed` werden nie gewählt.
3. Kein Clip und keine Fehlerisolierung für eingebaute Blöcke in Phase 5, weil es heute keinen `catch` gibt.
4. Kopf und Fuß von Plugin- und eingebauten Seiten bleiben getrennt. Der Einstieg ist gemeinsam, es gibt zwei Rahmenvarianten.
5. Schriften kommen weiter aus dem Konstruktor. Die veralteten Schriften nach einem Stilwechsel (`sets_`) sind ein bekannter Befund und werden nicht in Phase 5 behoben.
6. Rückrollen per getrenntem Commit je Seite, kein Feature-Schalter.
7. Toleranz bitgleich. Die Baseline gilt pro Rechner, der Vergleich vor/nach läuft auf demselben Rechner.
8. Freunde und Discord sind ein Paket. HA und Willkommen sind Kontrollbilder.
9. Vorschau-Eingaben = eingefrorene `render::samples`, dieselben `render()`-Methoden, kein Hinweis „Beispieldaten“.
