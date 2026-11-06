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

Ein Plugin bleibt auch mit `schema_version: 2` eine einzelne JSON-Datei. Neu sind die Block-ID `id` an jedem Widget (Pflicht), das optionale `layout` (Raster aus ganzen Anteilen, Zonen mit Spalte/Zeile/Spannweite) und `zone` an jedem Widget, sobald ein `layout` vorhanden ist; mit `layout` außerdem bis zu 12 statt 6 Widgets. Grenzen und Verhalten: [Plugin-System, Manifest v2](PLUGIN-SYSTEM.md#manifest-v2-zonen-layout-2100-in-vorbereitung). Manifeste mit Schema 1 bleiben unverändert gültig; `layout`, `id`, `zone` und `show_if` sind dort unbekannte Felder und werden abgewiesen. ScreenBuddy bis 2.9.x lehnt Schema 2 beim Import ab.

Regeln für v2 (der Parser erzwingt sie, `fl-plugin-validate` meldet Verstöße):

- `layout` enthält genau `columns`, `rows` (je 1–6 Anteile, jeder Anteil eine ganze Zahl 1–12) und `zones` (1–8 Zonen).
- Zone: `id` (Kleinbuchstabe am Anfang, dann Kleinbuchstaben, Ziffern, `_`; höchstens 40 Zeichen; eindeutig), `column`, `row` (0-basiert), optional `column_span`, `row_span` (mindestens 1, innerhalb des Rasters). Zonen dürfen sich nicht überlappen; keine Verschachtelung.
- Block-ID `id` an jedem Widget: gleiche Zeichenregeln, plugin-weit eindeutig. `zone` muss auf eine vorhandene Zone verweisen; höchstens 4 Widgets pro Zone, jede Zone braucht mindestens eines; mit `layout` 1–12 Widgets.
- Ohne `layout` (Schema 2 ohne Raster) gelten die Schema-1-Regeln: höchstens 6 Widgets, kein `zone`; die Seite wird pixelidentisch wie Schema 1 gezeichnet.
- Kein Pixel- oder Prozentmaß im Manifest: Die App wählt je Block die Dichtestufe (kompakt, normal, ausführlich) aus der Zonenfläche und blendet Blöcke aus, die nicht passen. Auf kleinen Displays (Skalierung unter 1, etwa 240×160) sind die Mindestflächen entsprechend größer, weil die Schrift nicht mitskaliert; dort entfallen eher Blöcke. Plugins sollten den wichtigsten Block an den Anfang der Zone setzen (der letzte nicht passende Block entfällt zuerst).
- Optional `show_if` an jedem Widget (mit und ohne `layout`): `{"path": "<quelle>[.<feld>…]"}` oder `{"path": "settings.<schlüssel>"}` (Wert vorhanden) bzw. mit `"op"` (`==`, `!=`, `<`, `<=`, `>`, `>=`) und `"value"` (Text 1–256 Zeichen, endliche Zahl oder Wahrheitswert; `<`/`>` nur Zahlen). Keine Ausdruckssprache, eine Bedingung pro Block, `path` höchstens 128 Zeichen und 8 Segmente. Einstellungen vom Typ `secret`, `time`, `date` und `item` sind verboten; jeder Fehler lehnt das ganze Manifest ab. Semantik (fehlend, `null`, leer, `false`, `0`): [Bedingte Anzeige](PLUGIN-SYSTEM.md#bedingte-anzeige-show_if-phase-3).

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

## Wechselfeld pro Zone (Phase 4b, Entwicklung)

Nur `schema_version: 2` mit `layout` erlaubt an einer Zone `"cycle": {"mode": "rotate", "transition": "fade"}`. `mode` ist `rotate` oder `first_available`; `transition` ist `fade` (Standard) oder `none`. In `rotate`-Zonen darf jeder Block `seconds` als ganze Zahl von 3 bis 600 tragen (Standard 8). Ohne `cycle` bleibt die bisherige Stapelung unverändert; Schema 1 akzeptiert diese Schlüssel nicht. Store-Pakete mit Schema 2 brauchen `min_app_version` mindestens `2.10.0`.

`rotate` zeigt die verfügbaren Blöcke in Manifestreihenfolge; `first_available` zeigt den ersten verfügbaren Block und reagiert auf Datenänderungen. Verfügbar bedeutet sichtbar nach `show_if` und ohne fehlende Referenz im jeweiligen Block. Ohne verfügbare Blöcke bleibt die Zone leer. Beim Verlassen der sichtbaren Kacheln oder Manifestwechsel startet sie neu; Ansichten und unabhängige Geräte haben eigene Zeitpläne.

Das Manifest wählt keinen Effekt je Gerät: Monitor 4 ruhige Überblendungsschritte in 400 ms (bei hoher Renderzeit 2 oder Schnitt), Companion 2 Schritte in 400 ms bei verbundenen Clients, seriell ein Band-Vorhang nur bei ausreichend gemessener Rate (sonst Schnitt). Der serielle Takt berücksichtigt Übertragung, Renderzeit und 50 ms Ruhe; globale 8-Zeilen-Bänder werden der Reihe nach aufgedeckt. Reicht das Zyklusbudget nicht, wird geschnitten. Nur Companion verwirft verspätete Schritte; Monitor zeigt den erreichten Zwischenstand. Daten bleiben während des Übergangs eingefroren. Größenwechsel beendet den Übergang, Themewechsel erneuert den Bildcache. Die Mindestdauer steigt bei langsamer Übertragung auf das Vierfache der letzten Übertragungsdauer. Gleichzeitige Wechsel derselben Kachel warten höchstens 600 ms. Keine Töne oder aufdringlichen Effekte.

Beispiel: [offline-wechselfeld.flplugin.json](../cpp/examples/plugins/offline-wechselfeld.flplugin.json). Zum Ausprobieren lokal importieren; die zwei Texte wechseln nach 3 bzw. 5 Sekunden. Hardware-Abnahme und reale serielle Ratenmessung durch Basti stehen aus; bisher wurden ausschließlich simulierte Übertragungen geprüft. Tap-Aktionen gehören zur späteren Phase 4c.
