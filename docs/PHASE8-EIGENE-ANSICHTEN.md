# Phase 8: Eigene Ansichten

Stand: 2026-11-07 · P8-2 Einzelkacheln und nativer Editor; lokale Entwicklung, keine Veröffentlichung.

Diese Phase ist ein lokaler Entwicklungsstand für das Zonen-Layout. Kein Release, keine neue EXE-Auslieferung und keine Versionsanhebung. Grundlage: eigener ScreenBuddy-Code und [Clean-Room-Regeln](CLEAN-ROOM-LAYOUT.md).

## P8-2: Bedienung und aktueller Stand

Unter **Einstellungen → App → Eigene Ansichten** öffnet sich eine eigene Einstellungsseite. Das Fenster wird bei Bedarf auf bis zu 1520 × 940 DIP innerhalb des verfügbaren Arbeitsbereichs vergrößert; im Material-Design entfällt hier der zusätzliche Inspector. Das aktive `theme::Design`/`ui_kit` liefert Farben, Schrift und Bedienelemente.

1. Links **+ Neue Ansicht** wählen. Der neue Entwurf beginnt mit einem Raster aus drei Spalten und zwei Zeilen. Name rechts ändern; **Umbenennen** fokussiert das Namensfeld. Erst **Übernehmen** speichert den Entwurf.
2. In der Palette eine Quelle auswählen oder Kacheln suchen. Klicken fügt an der ersten freien Stelle ein; Ziehen, Verschieben, Größenänderung und Papierkorb verwenden dieselben Rasterbausteine wie „Seite anordnen“. Freie Plätze sind gestrichelt; ungültige Ziele zeigen „passt nicht“. Pro Zone eine Kachel, höchstens 24. Mehrfach vorhandene Kacheln aus JSON bleiben erhalten; die Palette bietet jeden Quellblock einmal an.
3. Rechts Breite/Höhe in Zellen, **Anzeigen wenn** und Dichte bearbeiten. Bedingungen verwenden öffentliche `data.…`-Pfade; leer bedeutet immer. Vergleiche erwarten einen typisierten JSON-Wert, etwa `42`, `true` oder `"Text"`. **Bedingung bestätigen** übernimmt den Entwurf. Dichte ist eine Obergrenze (kompakt/normal/automatisch); zu kleine Flächen bleiben leer. Die Obergrenze gilt auch für HA-Entitäten und Plugin-Blöcke; HA hat kompakte und normale native Darstellung.
4. **Übernehmen** speichert mit dem vorhandenen atomaren Store/Backup. Die Ansicht erscheint als `custom:<id>` automatisch am Ende des normalen Seitenzyklus, sofern sie noch nicht ausdrücklich in der Seitenreihenfolge steht. Unter **Ansicht → Seiten** kann sie eingeordnet und ausgeblendet werden. Explizite Geräteseitenlisten bleiben verbindlich; dort muss die eigene Ansicht ausgewählt werden. Der allgemeine Companion-Seitenzyklus ergänzt eigene Ansichten automatisch; individuelle Companion-Gerätelisten bieten sie zur Auswahl an.

Tabs zeigen **Standard und ausschließlich aktive Geräte** aus der vorhandenen Ausgangs-/Companion-Erkennung; Auflösung/Nutzfläche stammen aus dem Geräteprofil. Es werden keine Geräte verbunden oder geweckt. Übernehmen im Geräte-Tab legt dessen eigenes Layout an. **Von Standard erben** speichert nach einer gegebenenfalls nötigen Verwerfen-Rückfrage sofort die Entfernung dieses Geräte-Layouts. **Löschen** entfernt nach Rückfrage die gesamte Ansicht einschließlich Geräte-Layouts und Backup. Offline-Layouts bleiben bei bloßem Trennen erhalten. Verschwindet das ausgewählte Gerät, fällt die Auswahl auf Standard zurück; ein gültiger ungespeicherter Geräteentwurf bleibt in der Arbeitssitzung, bis er übernommen oder verworfen wird. Auch „Seite anordnen“ filtert offline Geräte und fällt auf Standard zurück (dessen ungespeicherter Geräteentwurf wird dabei verworfen, gespeicherte Overlays bleiben erhalten).

**Tastatur:** F6/Strg+Tab wechseln Palette/Fläche/Papierkorb; Leertaste nimmt auf, Enter legt ab, Esc bricht ab. Pfeile bewegen das Ziel, Umschalt+Pfeile ändern die Größe, Entf entfernt; Strg+Z/Y im Raster macht Gesten rückgängig beziehungsweise wiederholt sie. Eigenschaften und Palette verwenden die vorhandenen MSAA-Rollen/Namen. Vor dem Geräte-/Ansichtswechsel fragt der Editor nach ungesicherten Änderungen.

### Quellen und unveränderte Standardseiten

- System: fünf bestehende Messwertkarten.
- Spotify: Cover, Titel/Interpret, Pegel, Fortschritt, Tasten und Status einzeln. Die Vorschau verwendet eingefrorene synthetische Pegel ohne Audiozugriff; im Live-Zyklus verwendet die Pegelkachel den bereits vorhandenen Pegeldienst einschließlich der gekennzeichneten Simulation bei Remote-Wiedergabe. Bei ausgeschalteter Amplifier-Anzeige bleiben Pegel leer.
- RSS, Streaming, BF6, Freunde und Discord: sowohl Inhalt/Liste als auch Status einzeln.
- Home Assistant: konfigurierte einzelne Entitäts-IDs (`block` ist beispielsweise `sensor.temperature`), gezeichnet über die **native `HaRenderer::cell`-Zeichenfunktion**. Fehlende HA-Daten lassen die Kachel leer. Die Palette enthält konfigurierte Entitäten; die Mini-Vorschau verwendet einen ausdrücklich synthetischen Sensorwert.
- Installierte aktivierte v2-Plugin-Blöcke: bestehender Plugin-Renderer und Bedingungen, Live-Daten weiterhin nur nach Quellenfreigabe und mit gültiger Generation/Einstellungsrevision. Vorschau ausschließlich mit Manifest-Beispieldaten und nicht geheimen Vorgaben.

`builtin::Overlay::single_tile` ist ein interner, nicht aus Seiten-JSON aktivierbarer Vertrag: genau ein bekannter sichtbarer Block und genau eine gültige Zone. Vollständige Seiten behalten Inventar- und Pflicht-Statusprüfung. Es gibt keine zweite Renderer-Kopie und keinen Ganzseiten-Ausschnitt als Ersatz für einen fehlenden Einzelrenderer. Die Komposition übernimmt nur die gezeichnete Zielzone, ohne Quellkopf/-fuß. Standard-Baselines werden nicht aktualisiert. Das reine Editor-Entwurfsmodell verwendet die vorhandenen Gesten bis 24 Zonen, erweitert aber weder Plugin-Manifest noch Plugin-Overlay-Grenzen.

Neu im Dateiformat: optionales ganzzahliges `density` pro Kachel (`0` kompakt, `1` normal, `2` automatisch, Standard `2`). `Store::list/erase` ergänzen Verwaltung; Links/Reparse-Punkte und ungültige IDs bleiben gesperrt. Kein neuer HTTP-Endpunkt.

### Grenzen

Interne Listenpagination, Wechselfelder, eigene Tap-/Schaltaktionen und eine Verwaltungs-HTTP-API bleiben offen. Spotify-Tasten und HA-Schalter sind in eigenen Ansichten derzeit nur gezeichnet. Quellen müssen wie bisher separat eingerichtet/aktiviert werden. Die Palette ist auf 256 Inventareinträge begrenzt; Ansichten behalten das 24-Kachel-Limit. Die Fläche zeigt gecachte Einzelkachel-Beispielbilder; die zusammengesetzte Vorschau darunter wird nach dem Ablegen aktualisiert. Beides ist keine Aufnahme laufender Geräte. Physische Mehrgeräte-, Touch- und Screenreader-Handabnahme bleiben offen.

## P8-2: English usage and implementation

Open **Settings → App → Custom views**. The dedicated page expands the settings window up to 1520 × 940 DIP within the monitor work area; Material hides the extra inspector on this page. It uses the active design tokens. Choose **+ New view**, change its name, select/search a source, then click or drag a sample tile onto the canvas. One tile per zone, up to 24; existing duplicate source tiles from JSON are preserved, while the palette offers each source block once. Adjust cell width/height, a public `data.…` presence/typed comparison and the maximum density. Confirm condition edits, then **Apply** to persist the draft. The cap also applies to HA entities (compact/regular native modes) and plugin blocks. Cached tile samples appear on the canvas, with a composed sample preview updated after dropping. No live-device screenshots or audio playback.

Applied views appear as `custom:<id>` at the end of the normal page cycle unless explicitly ordered already. **View → Pages** supports order/visibility. Explicit device page lists remain authoritative; select the view there. The general Companion cycle automatically includes custom views; individual Companion lists expose them for explicit selection. Device tabs show only active outputs/clients plus Standard; offline overlays stay stored. Apply in a device tab creates its own layout. **Inherit from standard** immediately persists removal of that device overlay after any required discard confirmation. **Delete** confirms removal of the view, device overlays and backup. A disappearing selected device returns to Standard; a valid custom-view device draft remains in memory until applied/discarded. The older Arrange page editor discards its unsaved device draft on disconnection while retaining saved overlays.

F6/Ctrl+Tab, Space/Enter/Esc, arrows, Shift+arrows, Delete and Ctrl+Z/Y use the existing grid interaction and MSAA roles. Changing views/devices confirms unsaved changes. All system, Spotify and list content/status blocks use the existing renderer through a separately validated single-tile contract; complete page validation still requires its status inventory. HA reuses the native entity-cell painter. Configured HA entities and enabled v2 plugin blocks are available; plugin preview uses manifest samples/non-secret defaults, live rendering keeps consent/generation checks. Optional integer `density` (`0`, `1`, `2`, default `2`) caps built-in density. `Store::list/erase` adds local management without an HTTP endpoint. The internal custom editor draft supports 24 zones without widening public plugin limits.

Still open: custom-view pagination/cycles/touch actions and management HTTP API; HA switches and Spotify buttons are visual only. Live Spotify tiles reuse the existing meter (including labelled remote simulation); disabling Amplifier leaves levels empty. The palette has a 256-entry bound. Physical device, touch and manual screenreader acceptance remain unverified. No release, version change, push or new public EXE.

## P8-2: Prüfstand / Validation (2026-11-07)

VS 2022 BuildTools, MSVC **14.44.35207**, Ninja Release in `cpp/build/store`: `ScreenBuddy` und alle **24 Testziele** gebaut und erfolgreich ausgeführt. Roboto aus den bereits vorhandenen Benutzerschriften mittels `AddFontResourceW`/`WM_FONTCHANGE` verfügbar gemacht. Keine Installation oder Auslieferung, kein Push. Vorhandene Versionswerte in CMake, `util.h` und `app.rc` bleiben 2.10.0 (Entwicklungsstand).

| Suite | Bestanden / passed |
| --- | ---: |
| fl-custom-view-tests | 499 |
| fl-tests | 3530 |
| fl-layout-ui-tests | 1671 |
| fl-plugin-tests | 380 |
| fl-plugin-zones-tests | 462, zusätzlich 8000 Fuzz-Manifeste / plus 8000 fuzz manifests |
| fl-layout-edit-tests | 163 |
| fl-plugin-example-tests | 124 |
| fl-plugin-sign-tests | 111 |
| fl-plugin-catalog-tests | 77 |
| fl-plugin-store-tests | 453 |
| fl-plugin-transport-tests | 140 |
| fl-plugin-settings-tests | 135 |
| fl-plugin-schedule-tests | 845 |
| fl-builtin-catalog-tests | 234 |
| fl-plugin-snapshot-tests | 2101 |
| fl-companion-tests | 386, vollständige lokale TLS-/Server-Integration / full local TLS/server integration |
| fl-device-profile-tests | 88 |
| fl-monitor-tests | 11, Gestenlogik ohne sichtbaren Fenstermodus / gesture logic without visible window mode |
| fl-tmdb-tests | 119 |
| fl-psn-tests | 37 |
| fl-amp-tests | 13, lautlos synthetisch / silent synthetic checks |
| fl-key-help-tests | OK, keine Anzahl ausgegeben / no count emitted |
| fl-distribution-tests | `unpackaged`: Exit 0; keine MSIX-Paketprüfung / no packaged MSIX probe |
| fl-builtin-snapshot-tests | 4221 Bilder + 4221 Trefferkarten, 0 Fehler / images + hit maps, zero errors |

Zusätzliche Built-in-Matrizen / additional built-in matrices: RSS 1024 Bilder/Trefferkarten (5952 Prüfungen), Streaming 1792 (9852), BF6 1024 (6439), Freunde 1024 (6360), Discord 1024 (6360), System 1536 (7900), Spotify 4096 (31404), Material Orange 768 Hashes. Jeweils **0 Fehler / zero errors**. **929 vorhandene Snapshot-Artefakte gegen `6289bfd` unverändert** (Git-Inhaltshashes), keine bestehenden Baselines neu aufgezeichnet.

Additiv: `cpp/tests/layout-snapshots/p8-custom-editor-hashes.txt` (DE/EN). Die Suite schreibt `cpp/build/store/p8-ui-snapshots/custom-editor.de.png` und `.en.png`: tatsächlicher Widgetbaum mit Palette, belegten/freien Zellen, Eigenschaften und zwei aktiven **synthetischen** Geräte-Tabs COM7/Monitor plus Standard. Native EDIT-Feldwerte werden für die fensterlose PNG-Aufnahme aus den tatsächlichen Feldern in das Testbild gezeichnet. Das ist keine Aufnahme eines sichtbaren Einstellungsfensters und kein Nachweis angeschlossener Hardware.

Neue Prüfungen: alle Inhaltsquellen über bestehende Renderer; native HA-Zelle pixelgleich zur Standardseite; Dichte inklusive HA/Plugin; unveränderte Pflicht-Statusprüfung ganzer Seiten; korrekte Vorschau-Bedingungsdaten; 24-Kachel-Adapter ohne Plugin-Limiterweiterung; Speichern/Löschen/Backup; Anlegen, Umbenennen, Geräte-Layout/Vererbung, Offline-Erhalt und Rückfall auf Standard; Eigenschaften, Undo/Redo, F6 sowie Aufnehmen/Abbrechen/Ablegen per Tastatur. Vorhandene Kontrastmatrizen prüfen Text ≥ 4,5:1 und Fokusring ≥ 3:1. / New coverage includes real source painters, native HA cell equivalence, density caps, strict complete-page validation, sample-condition consistency, bounded drafts, storage/management, device inheritance/disconnection and keyboard operations. Existing theme contrast matrices pass.

Nur lokale Testserver; kein Zugriff auf Port 8765, kein externes Quellnetzwerk, keine hörbaren Töne. Offen bleiben die physische USB-/Monitor-/Companion-/TV-Abnahme, sichtbare native Fenster-/Maus-/Touch-Abnahme und manuelle Screenreader-Abnahme. Keine Behauptung einer solchen Abnahme aus Simulationen oder PNGs. / Local test servers only, no port 8765, external source network or audible output; physical devices, visible native-window/mouse/touch acceptance and manual screenreader testing remain open.

## P8-1: Deutsch – historischer Implementierungsstand

Die folgenden Abschnitte dokumentieren P8-1 vor der Erweiterung durch P8-2; damalige offene Grenzen und Testergebnisse bleiben als Historie erhalten.



- Eine Ansicht liegt neben der Konfiguration in `views/<id>.view.json`. Sie wird über `custom:<id>` in `display.pages` und `display.visible_pages` beziehungsweise `companion.pages` oder der eigenen Geräteseitenliste gewählt. Fehlende, ungültige oder nicht zur Datei passende Ansichten fallen still aus dem Seitenzyklus. Die festen Standardseiten und ihre Reihenfolge bleiben gleich.
- `custom_view.h` ist die API: `Store::get`, `Store::save` (leerer Fehlertext bei Erfolg), `parse`, `to_json`, `View::effective` und `draw`. Die Zonen-ID identifiziert eine Kachel innerhalb ihres Layouts; es gibt kein zusätzliches `Tile::id`.
- Standardlayout mit gewichteten Spalten/Zeilen und bis zu 16 vollständigen Geräte-Layouts. Exakte Geräte-ID wie bei den bestehenden Display-Overlays; unbekannte Geräte erben Standard. Geräte-IDs sind reine JSON-Schlüssel, keine Dateinamen (druckbares ASCII, 1–512 Zeichen).
- Höchstens 24 Kacheln je Layout, 6 × 6 Zellen, Achsengewichte 1–12. Zellen und Spans müssen vollständig ins Raster passen; Überlappungen und doppelte Zonen-IDs werden abgewiesen.
- Dokument höchstens 256 KiB, höchstens 64 Ansichtsdateien pro Ordner. Ansichts-/Zonen-ID: 1–48 Zeichen aus `a-z`, `0-9`, `-`, `_`, erster Buchstabe/Ziffer; reservierte Windows-Gerätenamen ausgeschlossen. Name: 1–128 UTF-8-Bytes, keine ASCII-Steuerzeichen. Unbekannte Felder, doppelte JSON-Schlüssel, falsche Typen und übertiefe Dokumente werden abgewiesen.
- Speichern validiert zuerst, sichert die vorherige Datei als `.view.json.bak` und ersetzt die Zieldatei atomar. Reparse-Punkte/Links und nicht reguläre Dateiziele werden abgewiesen. Laden greift nicht automatisch auf Backups zurück; eine defekte Datei muss lokal korrigiert oder bewusst aus dem Backup wiederhergestellt werden.
- Kacheln verwenden die vorhandenen Quellrenderer. Nur der geplante Kachelausschnitt wird zusammengesetzt, ohne Quellkopf/-fuß. Plugin-Zonen werden auf das gemeinsame Raster abgebildet. Zu kleine Blöcke bleiben nach den bestehenden Dichteschwellen unsichtbar. Fehlende Daten/Quellen, nicht unterstützte Blöcke oder Ausnahmen lassen die betroffene Kachel leer.
- Plugin-v2-Blöcke mit stabiler ID nutzen den vorhandenen Plugin-Renderer und behalten ihre eigene Bedingung. Live-Daten werden nur mit aktueller Generation, Einstellungsrevision und Quellenfreigabe übernommen; kein Rückfall auf Beispieldaten. Öffentliche Plugin-Einstellungen gehen wie bisher an den Renderer, Geheimnisse nicht.

### Dateiformat (Beispieldaten)

```json
{
  "schema_version": 1,
  "id": "overview",
  "name": "Meine Übersicht",
  "layout": {
    "columns": [1],
    "rows": [1],
    "tiles": [{
      "source": "system",
      "block": "system.cpu",
      "zone": {"id": "cpu", "column": 0, "row": 0},
      "column_span": 1,
      "row_span": 1,
      "show_if": {"path": "data.cpu_pct", "op": ">=", "value": 0}
    }]
  },
  "devices": {}
}
```

`devices` ist optional, ein Eintrag enthält denselben Aufbau wie `layout`. `show_if` ist optional: Anwesenheit oder ein Vergleich (`==`, `!=`, `<`, `<=`, `>`, `>=`) mit booleschem Wert, endlicher Zahl oder nicht leerem Text bis 256 Bytes. Nur öffentliche Pfade unter `data` (bis 128 Zeichen/8 Segmente), kein Zugriff auf Einstellungen oder Geheimnisse. Fehlend/null/leer ist abwesend, 0 und false sind vorhanden; keine Typumwandlung. System bietet `cpu_pct`, `gpu_pct`, `ram_pct`, `disk_pct`, `net_down`, `net_up`; Spotify `playing`, `title`, `artist`; Listenquellen `count`, `ready`; Plugins `data.<source-id>.<field>` aus freigegebenen Quellantworten. Statusblöcke können einen fehlenden/verbindungsfehlerhaften Datenzustand anzeigen; Systemkarten ohne passenden Messwert und Plugin-Blöcke ohne Daten bleiben leer.

### Unterstützte Blöcke und offene Grenzen

| Quelle | Zeichnen in P8-1 |
| --- | --- |
| System | `system.cpu`, `system.gpu`, `system.ram`, `system.datentraeger`, `system.netz` |
| Plugin | `plugin:<id>` + vorhandene Widget-ID aus Manifest v2; statische Inhalte auch ohne Datenquelle |
| RSS / Streaming / BF6 | Nur `rss.hinweis`, `streaming.hinweis`, `bf6.hinweis` |
| Freunde / Discord | Nur `freunde.hinweis` |
| Spotify | Nur `spotify.hinweis` |
| Home Assistant | Modell akzeptiert die Quelle; Zeichnen offen, `HaRenderer` hat keinen Einzelblock-/Arrangement-Weg |

Die bestehenden Katalogvalidatoren verlangen für Listen und Spotify einen sichtbaren Pflicht-Statusblock. Deshalb sind Beiträge, Titel, Rangliste, Freundesliste sowie Spotify-Cover/Text/Pegel/Fortschritt/Tasten als **einzelne** Kacheln hier noch nicht verfügbar. Es wird kein Ganzseitenbild als Ersatz ausgeschnitten und keine Validierung umgangen. Plugin-v1-Blöcke haben keine stabile Widget-ID und werden nicht angeboten. Die Quellintegration muss konfiguriert/aktiviert sein; die Ansicht aktiviert keine Datenquelle selbst.

Eigene Ansichten haben derzeit eine Seite ohne internes Listenblättern, Wechselfelder oder Aktions-/Touch-Ziele. Anlegen und Bearbeiten im Editor sowie eine Verwaltungs-API kommen später; bis dahin lokales JSON beziehungsweise C++-Store. Die vorhandenen Standardseiten behalten ihre bisherigen Renderer und Aktionen. Physische USB-/Monitor-/Companion-/TV-Abnahme steht aus.

## P8-1: English – historical implementation and limits

P8-1 adds a bounded, versioned local view model and page-cycle integration; editor creation/editing remains P8-2/3. Store `views/<id>.view.json` beside the configuration and select `custom:<id>` in display order/visibility, Companion pages or a device page list. Missing or invalid documents disappear silently; built-in defaults/order are unchanged. The example above is the exact schema. The zone ID identifies a tile, with no separate tile ID.

`Store::get/save`, strict parsing and serialization, atomic replacement with previous-file backup, per-device selection and isolated composition are implemented. Limits: 256 KiB/document, 64 view files, 24 tiles/layout, 6 × 6 weighted grid (1–12), 16 device layouts, view/zone IDs 1–48 lowercase ASCII letters/digits/hyphen/underscore starting with a letter/digit (no Windows reserved names). Names allow 1–128 UTF-8 bytes without ASCII control characters. Device IDs are printable ASCII JSON keys (1–512 characters); unknown devices inherit the standard. Invalid types/fields, duplicate keys/zone IDs, overlaps, oversized documents, links/reparse points and non-regular file targets are rejected. Backups are not loaded automatically.

Optional `show_if` addresses public `data` only (128 characters, eight path segments): presence or one typed comparison with a boolean, finite number or nonempty string up to 256 bytes. Missing/null/empty values are absent; zero and false are present. Public fields: system CPU/GPU/RAM/disk percentages and network rates as listed above; Spotify playing/title/artist; list count/ready; plugins use `data.<source-id>.<field>`. Existing source renderers draw the selected block; only its zone is composed, without the source header/footer. Plugin zones map to the common grid. Missing/failed/unsupported tiles stay blank; status tiles may show a missing-data/connection state.

Supported: all five system cards, stable widget IDs from plugin manifest v2, and status blocks for RSS, streaming, BF6, friends, Discord and Spotify. Existing source enablement, host authorization, generation/settings checks and plugin conditions remain in force; no sample-data fallback or secret settings. Static plugin widgets need no live source.

Not implemented: Home Assistant block rendering (no arrangement API), list content and Spotify content/control blocks (existing validators require a separate visible status block), plugin v1 widgets, view editor/management API, internal pagination/cycles and custom-view touch actions. No renderer duplication or validator bypass. No release/version change, no physical device acceptance.

## P8-1: historischer Prüfstand / historical validation

VS 2022 BuildTools, MSVC 14.44, Ninja Release in `cpp/build/store`; `ScreenBuddy` und alle 24 Testziele gebaut. Roboto über `AddFontResourceW` plus `WM_FONTCHANGE` registriert. Keine Baselines neu erzeugt; 929 erfasste Baseline-Artefakte gegen `HEAD` unverändert.

| Suite | Bestanden / passed |
| --- | ---: |
| fl-custom-view-tests | 229 |
| fl-tests | 3530 |
| fl-layout-ui-tests | 1615 |
| fl-plugin-tests | 380 |
| fl-plugin-zones-tests | 462 (zusätzlich 8000 Fuzz-Manifeste / additional fuzz manifests) |
| fl-layout-edit-tests | 163 |
| fl-plugin-example-tests | 124 |
| fl-plugin-sign-tests | 111 |
| fl-plugin-catalog-tests | 77 |
| fl-plugin-store-tests | 453 |
| fl-plugin-transport-tests | 140 |
| fl-plugin-settings-tests | 135 |
| fl-plugin-schedule-tests | 845 |
| fl-builtin-catalog-tests | 234 |
| fl-plugin-snapshot-tests | 2101 |
| fl-companion-tests | 386, vollständig lokal inklusive TLS-/Server-Integration / full local integration |
| fl-device-profile-tests | 88 |
| fl-monitor-tests | 11, Gestenlogik ohne Fenster-/Hardwaremodus / gesture logic only |
| fl-tmdb-tests | 119 |
| fl-psn-tests | 37 |
| fl-amp-tests | 13, lautlose synthetische Prüfung / silent synthetic checks |
| fl-key-help-tests | OK (keine Anzahl ausgegeben / no count emitted) |
| fl-distribution-tests | unpackaged: Exit 0; keine MSIX-Paketprüfung / packaged probe not run |
| fl-builtin-snapshot-tests | 4221 Bilder + 4221 Trefferkarten; zusätzliche Matrizen unten / images + hit maps and matrices below |

Built-in-Zusatzmatrizen / additional matrices: RSS 1024 Bilder/Trefferkarten (5952 Prüfungen), Streaming 1792 (9852), BF6 1024 (6439), Freunde 1024 (6360), Discord 1024 (6360), System 1536 (7900), Spotify 4096 (31404); Material Orange 768 Hashes. Jeweils 0 Fehler / zero errors.

Neue Suite: Parser-/Grenzfälle, 24/25 Kacheln, 16/17 Geräte, 64/65 Ansichten, Backup/Fehlschreibschutz, ungültige Dateien/Dateiziele/Reparse-Punkte, Standard-/Geräte-Rückfall, Konfigurationsbereinigung und App-Seitenzyklus, vorhandener System- und Plugin-Renderer, fehlende Daten/Bedingungen/Fehlerisolierung. Rendernachweise sind synthetische Beispieldaten; keine physische Geräteabnahme. / New suite covers bounded parsing/storage, device inheritance, configuration/page-cycle integration and real source renderer composition using synthetic data; physical acceptance remains open.

Nur lokale Testserver, kein Zugriff auf Port 8765, kein externes Quellnetzwerk und kein Ton. MSIX-Ausführung, sichtbarer Monitor-Fenstertest und physische Mehrgeräte-Abnahme sind nicht Teil dieses Laufs. / Local test servers only, no access to port 8765, external source network or audible output; packaged MSIX, visible monitor window and physical multi-device acceptance not run.
