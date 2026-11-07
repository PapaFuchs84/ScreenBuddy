# Phase 8: Eigene Ansichten

Stand: 2026-11-07 · P8-2 Quellen-Fundament; Editor im anschließenden lokalen Commit.

Diese Phase ist ein lokaler Entwicklungsstand für das Zonen-Layout. Kein Release, keine neue EXE-Auslieferung und keine Versionsanhebung. Grundlage: eigener ScreenBuddy-Code und [Clean-Room-Regeln](CLEAN-ROOM-LAYOUT.md).

## P8-2: Quellen-Fundament / source foundation

Einzelkacheln für alle Katalogblöcke (System, Listen, Spotify) verwenden den vorhandenen Renderer über einen eigenen, strikt geprüften Einzelkachel-Vertrag. Vollständige Seiten behalten Inventar und Pflicht-Statusblock. HA zeichnet einzelne Entitäten über die native Zellfunktion. Optionale Dichte-Obergrenzen gelten auch für HA und Plugins; Live-Spotify nutzt den vorhandenen Pegeldienst. Standard-Renderpfade bleiben unverändert. Vorschau-Bedingungen prüfen die tatsächlich gezeichneten Beispieldaten.

`Store::list/erase` ergänzen lokale Verwaltung. Gespeicherte Ansichten erscheinen automatisch im regulären Seitenzyklus; explizite Gerätelisten bleiben verbindlich. Der interne Editor-Adapter unterstützt 24 Kacheln, ohne Plugin-Parserlimits zu erweitern. Editor-Oberfläche und aktive Tabs folgen im anschließenden Commit. Kein Release oder Push, keine Töne/Port 8765, physische Abnahme offen.

Individual catalog blocks and native HA entity cells reuse existing painters through a strictly validated single-tile contract; full-page requirements remain unchanged. Density caps apply to built-ins, HA and plugins. Spotify reuses the existing meter; previews evaluate conditions against their rendered sample values. Local list/delete management, automatic cycle discovery and a 24-tile internal editor adapter are added without widening plugin document limits. Editor UI follows in the next commit. No release/push, audible output or port 8765; physical acceptance is pending.

## P8-1: historischer Stand / historical state

## Deutsch: umgesetzt

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

## English: implementation and limits

P8-1 adds a bounded, versioned local view model and page-cycle integration; editor creation/editing remains P8-2/3. Store `views/<id>.view.json` beside the configuration and select `custom:<id>` in display order/visibility, Companion pages or a device page list. Missing or invalid documents disappear silently; built-in defaults/order are unchanged. The example above is the exact schema. The zone ID identifies a tile, with no separate tile ID.

`Store::get/save`, strict parsing and serialization, atomic replacement with previous-file backup, per-device selection and isolated composition are implemented. Limits: 256 KiB/document, 64 view files, 24 tiles/layout, 6 × 6 weighted grid (1–12), 16 device layouts, view/zone IDs 1–48 lowercase ASCII letters/digits/hyphen/underscore starting with a letter/digit (no Windows reserved names). Names allow 1–128 UTF-8 bytes without ASCII control characters. Device IDs are printable ASCII JSON keys (1–512 characters); unknown devices inherit the standard. Invalid types/fields, duplicate keys/zone IDs, overlaps, oversized documents, links/reparse points and non-regular file targets are rejected. Backups are not loaded automatically.

Optional `show_if` addresses public `data` only (128 characters, eight path segments): presence or one typed comparison with a boolean, finite number or nonempty string up to 256 bytes. Missing/null/empty values are absent; zero and false are present. Public fields: system CPU/GPU/RAM/disk percentages and network rates as listed above; Spotify playing/title/artist; list count/ready; plugins use `data.<source-id>.<field>`. Existing source renderers draw the selected block; only its zone is composed, without the source header/footer. Plugin zones map to the common grid. Missing/failed/unsupported tiles stay blank; status tiles may show a missing-data/connection state.

Supported: all five system cards, stable widget IDs from plugin manifest v2, and status blocks for RSS, streaming, BF6, friends, Discord and Spotify. Existing source enablement, host authorization, generation/settings checks and plugin conditions remain in force; no sample-data fallback or secret settings. Static plugin widgets need no live source.

Not implemented: Home Assistant block rendering (no arrangement API), list content and Spotify content/control blocks (existing validators require a separate visible status block), plugin v1 widgets, view editor/management API, internal pagination/cycles and custom-view touch actions. No renderer duplication or validator bypass. No release/version change, no physical device acceptance.

## Prüfstand / Validation

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
