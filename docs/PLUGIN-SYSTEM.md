# Plugin-System im ScreenBuddy-2.5-Entwicklungsbuild

Der aktuelle Vertrag ist **vorläufig**. Er besteht aus JSON-Daten; native Bibliotheken, JavaScript und Shell-Befehle werden nicht geladen. Ein Plugin ohne Quellen kann lokal importiert und als Displayseite angezeigt werden. Ein Quellen-Plugin lässt sich aktivieren, sobald seine Pflicht-Einstellungen gesetzt und alle deklarierten Hosts einzeln freigegeben sind. Ein isolierter Abruf öffentlicher Wetterdaten gelang; die vollständige Abnahme externer HTTPS-Dienste und des Freigabeablaufs in der App steht noch aus.

## Minimales Beispiel

Speichere diese Daten als `offline-clock.flplugin.json` oder verwende die [fertige Beispieldatei](../cpp/examples/plugins/offline-clock.flplugin.json). Wähle in ScreenBuddy **Einstellungen → Plugins → Manifest importieren**:

```json
{
  "schema_version": 1,
  "id": "offline-clock",
  "name": "Uhr / Clock",
  "version": "1.0.0",
  "widgets": [
    {"type": "clock", "format": "time"}
  ]
}
```

Das Plugin erscheint als eigene Seite. Aktivieren, Deaktivieren und Entfernen erfolgt im selben Tab. Manifeste liegen im `plugins`-Ordner neben der per Benutzer gespeicherten `config.toml`.

## Vertrag und Grenzen

- Pflicht (Schema 1; Schema 2 siehe [Manifest v2](#manifest-v2-zonen-layout-2100-in-vorbereitung)): `schema_version: 1`, eindeutige ID aus Kleinbuchstaben/Ziffern/Bindestrichen, Name, Version `X.Y.Z` und 1–6 Widgets.
- Widgettypen: `clock` (`time`, `date`, `datetime`), `text`, `metric`, `bar`, `list`. Textvorlagen unterstützen geprüfte Referenzen wie `{{time}}`, `{{date}}` und Werte aus deklarierten Quellen.
- Optional: Store-Felder `license`, `author`, `category`, `min_app_version` (siehe [Paketformat](PLUGIN-PAKETFORMAT.md)); höchstens 12 Einstellungen und vier JSON-Quellen mit festgelegtem HTTPS-Host. Der begrenzte HTTPS-Abruf läuft in einem eigenen Worker. Host-Zustimmungen werden lokal gespeichert; Plugin-Einstellungen sind DPAPI-geschützt. Ein isolierter öffentlicher Live-Abruf bestand, aber der vollständige App-Ablauf mit Host-Freigabe ist noch nicht abgenommen. Vorschau-Daten einer Quelle werden nur für eine explizite Vorschau benutzt, nie als Live-Ersatz.
- Grenzen: Manifest 128 KiB, maximal 32 installierte Plugins, JSON-Tiefe 32. Unbekannte Felder und doppelte JSON-Schlüssel werden abgewiesen.
- Sicherheitszustand: Kein Plugin-Code wird geladen. Der Abruf ist auf deklarierte HTTPS-Hosts und begrenzte Antworten beschränkt; Host-Freigaben werden lokal gespeichert, Plugin-Einstellungen mit DPAPI geschützt. Zertifikats-Sperrprüfungen (CRL/OCSP) fehlen noch. Es gibt noch keinen öffentlichen Marketplace.

Prüfungen: Manifest-Parser (223), Speicher (127), Transport (134, einschließlich lokalem WinSock-DNS-Regressionstest) und Plugin-Einstellungen (135) bestanden. Ein direkter, isolierter HTTPS-Abruf der öffentlichen Open-Meteo-Beispielquelle lieferte die erwarteten Wetterfelder. Produktive Konfiguration und Host-Freigaben wurden nicht verändert. Vollständiger App-Ablauf, Zertifikats-Sperrprüfung und Hardwareausgabe sind damit nicht abgenommen; siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md).

## Manifest v2: Zonen-Layout (2.10.0, in Vorbereitung)

**Stand Phase 2b:** Parser, Grenzen, Store-Regel, Zonen-Geometrie, Block-Schnittstelle, Dichtestufen **und das Zeichnen nach Zonen** sind umgesetzt und getestet (`fl-plugin-zones-tests`, `fl-plugin-snapshot-tests`). Eine v2-Seite mit `layout` zeichnet die App nach Zonen; v1 und v2 ohne `layout` zeichnen pixelidentisch wie bisher im automatischen Raster (höchstens 6 Widgets). Manifest v1 bleibt unverändert. Ein fertiges Beispiel mit acht Blöcken: [offline-zonen.flplugin.json](../cpp/examples/plugins/offline-zonen.flplugin.json).

```json
{
  "schema_version": 2,
  "id": "zonen-demo",
  "name": "Zonen",
  "version": "1.0.0",
  "layout": {
    "columns": [2, 1],
    "rows": [1, 1],
    "zones": [
      {"id": "haupt", "column": 0, "row": 0, "row_span": 2},
      {"id": "oben", "column": 1, "row": 0},
      {"id": "unten", "column": 1, "row": 1}
    ]
  },
  "widgets": [
    {"type": "clock", "id": "zeit", "zone": "haupt", "format": "time"},
    {"type": "clock", "id": "datum", "zone": "oben", "format": "date"},
    {"type": "text", "id": "gruss", "zone": "unten", "text": "Hallo"}
  ]
}
```

Begriffe: **Zone** = rechteckiger Bereich eines flachen Rasters; **Block** = ein Widget in einer Zone; **Dichtestufe** = kompakt, normal oder ausführlich.

- **v2 erkennt der Parser nur an `schema_version: 2` als ganze Zahl.** Alle v1-Regeln gelten weiter (Vorlagen, Texte, Quellen, Einstellungen). Zusätzlich: jedes Widget hat eine **stabile Block-ID** `id` (Kleinbuchstabe am Anfang, dann Kleinbuchstaben/Ziffern/`_`, höchstens 40 Zeichen, eindeutig). Die ID bleibt über Versionen gleich und ist später der Schlüssel für Tap-Ziele und Einstellungen.
- **`layout` ist optional.** Ohne `layout` nutzt eine v2-Seite das automatische Raster; `zone` ist dann nicht erlaubt. Mit `layout` braucht jedes Widget eine `zone`, die es gibt, und jede Zone mindestens einen Block.
- **Raster in Bruchteilen:** `columns` und `rows` sind ganze Anteile 1–12 (z. B. `[2, 1]` = zwei Drittel / ein Drittel). Kommazahlen, `2.0`, `1e0`, Texte, Wahrheitswerte und negative Zahlen werden abgewiesen.
- **Zonen:** `id`, `column`, `row` (0-basiert), optional `column_span`/`row_span` (Standard 1). Eine Zone muss vollständig im Raster liegen und darf keine andere überlappen. Leere Zellen sind erlaubt. Keine Verschachtelung: Zonen enthalten keine Zonen.
- **Grenzen:** 1–6 Spalten, 1–6 Zeilen, 1–8 Zonen, höchstens 4 Blöcke pro Zone, 1–12 Widgets pro Seite mit `layout`; ohne `layout` (v1 und v2) bleibt es bei 1–6. Unbekannte Felder und doppelte Schlüssel werden abgewiesen, wie in v1.
- **Anordnung:** Blöcke einer Zone stehen in Manifest-Reihenfolge untereinander und teilen die Zonenhöhe gleichmäßig. Spalten-/Zeilenkanten werden aus den kumulierten Anteilen auf ganze logische Pixel gerundet; Nachbarn teilen sich exakt eine Kante (plus Abstand), es entstehen weder Lücken noch Überlappungen.
- **Zu wenig Fläche:** Passt ein Block auch kompakt nicht in seine Fläche, wird er **ausgeblendet statt zu überlappen**. Ausgeblendet wird zuerst der letzte nicht passende Block der Zone; der frei gewordene Platz geht an die übrigen. Ungültige Flächen (nicht endlich, ≤ 0, größer als 16384) ergeben eine Seite ohne sichtbare Blöcke.
- **Clipping:** Ein Block zeichnet nur innerhalb seines Rechtecks; der Renderer setzt vor jedem Block einen Clip auf dieses Rechteck (`Canvas::clip_push`/`clip_pop`). Texte werden zusätzlich mit „…“ gekürzt, damit nichts abgeschnitten aussieht. Rahmenstil (Kartenradius, Abstände) und Farben kommen wie bei v1 aus dem Theme.

### Dichtestufen

Jeder Bausteintyp (`text`, `metric`, `bar`, `list`, `clock`) hat drei Mindestflächen in logischen Pixeln: kompakt (nur Wert), normal (Bezeichnung + Wert), ausführlich (zusätzliche Details). Gewählt wird die höchste Stufe, deren Mindestfläche (Breite **und** Höhe) passt. Logische Pixel: die Seite wird auf 480×320 (Querformat) bzw. 320×480 (Hochformat) bezogen und skaliert; 240×160 und 160×240 ergeben dieselben logischen Flächen wie 480×320 und 320×480. Die Schrift wird dabei nicht mitskaliert (feste Bildpunkte); bei Skalierung 0,5 ist eine Textzeile daher in logischen Pixeln doppelt so hoch. Deshalb gelten die Tabellenwerte für Skalierung ≥ 1 und werden bei kleinerer Skalierung s mit 1/s multipliziert (Schwellen sowie Bezeichnungszeile, Ränder, Zeilenabstand und Balkenmaße); Blöcke, die dann nicht mehr passen, werden ausgeblendet statt überlappt. Die Schwellen sind aus der Zeichengeometrie abgeleitet (Bezeichnungszeile 22, Rand oben/unten 6, Zeilenabstand 18) und mit den Snapshots des Beispiels in allen vier Größen und allen Theme-Stilen geprüft. Die Schwellen sind **zentral in der App** (`zones::DensityTable`) je Bausteintyp einstellbar und gehören nicht ins Manifest. Gültig sind nur endliche, positive, aufsteigende Schwellen; ungültige Werte werden abgelehnt und der alte Wert bleibt.

| Typ | kompakt | normal | ausführlich |
| --- | --- | --- | --- |
| `text` | 56×22 | 80×42 | 120×64 |
| `metric` | 48×28 | 72×50 | 110×66 |
| `bar` | 56×22 | 80×58 | 120×72 |
| `list` | 60×22 | 96×46 | 130×82 |
| `clock` | 52×28 | 72×50 | 110×66 |

Was jede Stufe zeichnet:

| Typ | kompakt | normal | ausführlich |
| --- | --- | --- | --- |
| `text` | Wert, eine Zeile | Bezeichnung + Wert, eine Zeile | Bezeichnung + Wert, umbrochen (so viele Zeilen wie passen) |
| `metric`, `clock` | Wert; Bezeichnung links in derselben Zeile, wenn Platz ist | Bezeichnung + Wert (mit Einheit) | größere Schrift (26 bis 20, passend zur Breite), Einheit klein dahinter |
| `bar` | nur der Balken (ohne Zahl: der Wert als Text) | Bezeichnung, Wert, Balken | zusätzlich große Zahl und Prozent des Bereichs in der Kopfzeile |
| `list` | erster Eintrag | Bezeichnung + höchstens 2 Einträge | bis 5 Einträge, bei weiteren „+N“ in der Kopfzeile (ohne Bezeichnung in der Zeile des ersten Eintrags, der dafür gekürzt wird) |

### Block-Schnittstelle (für Entwickler)

`cpp/src/plugin_zones.h`: Ein `zones::Block` liefert seine stabile ID, `min_size(Dichtestufe)` und `draw(Canvas, Fläche, Dichtestufe)`. `draw` wird nie mit einer Fläche unter der Mindestgröße aufgerufen. `TableBlock` holt die Mindestgrößen aus einer `DensityTable` und hält nur deren Adresse: Die Tabelle muss den Block überleben, **temporäre Tabellen lassen sich nicht übergeben** (gelöschter Konstruktor). `plugin_render.cpp` hat je Widget-Typ eine Unterklasse (Text, Wert für `metric`/`clock`, Balken, Liste) und eine zentrale statische Tabelle. `zones::plan(Fläche, Layout, Blöcke, Abstand)` ist eine reine Funktion ohne Grafik und liefert die Rechtecke aller Zonen und Blöcke samt Dichtestufe oder „ausgeblendet“.

### Store

Für den Store braucht ein v2-Manifest `min_app_version` **2.10.0 oder neuer**. So zeigt eine ältere App das Plugin als „braucht neueres ScreenBuddy“ an, bevor sie es herunterlädt; installieren könnte sie es ohnehin nicht (sie lehnt `schema_version: 2` ab).

## Eigenes Plugin erstellen

Das [Schritt-für-Schritt-Tutorial](PLUGIN-TUTORIAL.md) erklärt Import, Einstellungen, Host-Freigaben, Vorlagen und die Prüfung mit dem echten Parser. Beginne mit dem vollständig lokalen [Fokusboard](../cpp/examples/plugins/offline-focus.flplugin.json); die [Wettervariante](../cpp/examples/plugins/public-weather.flplugin.json) demonstriert optional eine öffentliche JSON-Quelle. Sample-Daten werden nie als Live-Ersatz verwendet.
