# Plugin-Paketformat und Validator

© 2026 FuchsLabs · Phase 1 der Plugin-Plattform (Grundlage für den späteren Pluginstore). **Stand:** Manifest-Felder und Validator sind umgesetzt; Store, Signierung und Store-Oberfläche folgen in späteren Phasen und existieren noch nicht.

## Ein Plugin ist eine Datei

Ein Plugin bleibt **eine** UTF-8-JSON-Datei `<id>.flplugin.json` (Schema 1 oder 2, höchstens 128 KiB) ohne ausführbaren Code. Es gibt keine Archive, Skripte oder Zusatzdateien. Für die geplante Plugin-Sammlung gilt: ein Ordner pro Plugin, `plugins/<id>/<id>.flplugin.json`.

## Neue Manifest-Felder (alle optional, gültig in Schema 1 und 2)

| Feld | Regel |
| --- | --- |
| `license` | Nur `MIT` oder `CC0-1.0` (exakte Schreibweise). |
| `author` | 1–80 Zeichen, ohne Steuer- und unsichtbare Formatzeichen. |
| `category` | `system`, `games`, `smart-home`, `info` oder `other`. |
| `min_app_version` | `X.Y.Z`. Ältere ScreenBuddy-Versionen zeigen das Plugin später als nicht unterstützt an (Oberfläche folgt mit dem Store). |

Bestehende Manifeste ohne diese Felder bleiben unverändert gültig und lokal importierbar. Ein ungültiger Wert in einem dieser Felder lässt den Import dagegen scheitern.

## Schema 2: Zonen-Layout (ab 2.10.0, in Vorbereitung)

Ein Plugin bleibt auch mit `schema_version: 2` eine einzelne JSON-Datei. Neu sind die Block-ID `id` an jedem Widget (Pflicht), das optionale `layout` (Raster aus ganzen Anteilen, Zonen mit Spalte/Zeile/Spannweite) und `zone` an jedem Widget, sobald ein `layout` vorhanden ist; mit `layout` außerdem bis zu 12 statt 6 Widgets. Grenzen und Verhalten: [Plugin-System, Manifest v2](PLUGIN-SYSTEM.md#manifest-v2-zonen-layout-2100-in-vorbereitung). Manifeste mit Schema 1 bleiben unverändert gültig; `layout`, `id` und `zone` sind dort unbekannte Felder und werden abgewiesen. ScreenBuddy bis 2.9.x lehnt Schema 2 beim Import ab.

Regeln für v2 (der Parser erzwingt sie, `fl-plugin-validate` meldet Verstöße):

- `layout` enthält genau `columns`, `rows` (je 1–6 Anteile, jeder Anteil eine ganze Zahl 1–12) und `zones` (1–8 Zonen).
- Zone: `id` (Kleinbuchstabe am Anfang, dann Kleinbuchstaben, Ziffern, `_`; höchstens 40 Zeichen; eindeutig), `column`, `row` (0-basiert), optional `column_span`, `row_span` (mindestens 1, innerhalb des Rasters). Zonen dürfen sich nicht überlappen; keine Verschachtelung.
- Block-ID `id` an jedem Widget: gleiche Zeichenregeln, plugin-weit eindeutig. `zone` muss auf eine vorhandene Zone verweisen; höchstens 4 Widgets pro Zone, jede Zone braucht mindestens eines; mit `layout` 1–12 Widgets.
- Ohne `layout` (Schema 2 ohne Raster) gelten die Schema-1-Regeln: höchstens 6 Widgets, kein `zone`; die Seite wird pixelidentisch wie Schema 1 gezeichnet.
- Kein Pixel- oder Prozentmaß im Manifest: Die App wählt je Block die Dichtestufe (kompakt, normal, ausführlich) aus der Zonenfläche und blendet Blöcke aus, die nicht passen. Auf kleinen Displays (Skalierung unter 1, etwa 240×160) sind die Mindestflächen entsprechend größer, weil die Schrift nicht mitskaliert; dort entfallen eher Blöcke. Plugins sollten den wichtigsten Block an den Anfang der Zone setzen (der letzte nicht passende Block entfällt zuerst).

## Zusätzliche Store-Regeln

Für die Aufnahme in den Store müssen zusätzlich gelten (`fl-plugin-validate --store`):

- `description` (mindestens 10 Zeichen), `author`, `license`, `category`, `min_app_version` vorhanden.
- Plugins mit Quellen: `refresh_seconds` mindestens 60 und für jede Quelle ein `sample` (Grundlage der Vorschau).
- Dateiname `<id>.flplugin.json`, IDs eindeutig.
- Schema 2: `min_app_version` mindestens `2.10.0`.

Diese Regeln sind eine **automatische Vorprüfung**. Die Freigabe bleibt eine menschliche Prüfung (Zweck, Hosts, Lizenz).

## Validator

```text
fl-plugin-validate [--store] [--app-version X.Y.Z] <datei-oder-ordner>...
```

Er nutzt `plugins.cpp` der App (gleicher Parser), liest nur Dateien und macht keine Netzwerkabfragen. Pro Plugin erscheint `OK` mit ID, Version und den Hosts, an die Daten gesendet würden, oder `FAIL` mit festen Fehlermeldungen. Rückgabewert: 0 gültig, 1 Probleme, 2 falscher Aufruf. Die Tests laufen in GitHub Actions (`Plugin validator`) und lokal mit `sh cpp/tests/build-validator-linux.sh`.

## Offen (spätere Phasen)

Signierung ist als Bibliothek und Werkzeug umgesetzt, siehe [Plugin-Signierung](PLUGIN-SIGNIERUNG.md); offen sind Produktionsschlüssel, Index-Build per Actions und Download. Store-Oberfläche in der App (Phase 3), Moderation und Nutzungsbedingungen (Phase 4). Siehe Plan in PR [#21](https://github.com/PapaFuchs84/ScreenBuddy/pull/21).
