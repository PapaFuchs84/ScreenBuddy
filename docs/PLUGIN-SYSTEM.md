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

**Stand Phase 2b:** Parser, Grenzen, Store-Regel, Zonen-Geometrie, Block-Schnittstelle, Dichtestufen **und das Zeichnen nach Zonen** sind umgesetzt und getestet (`fl-plugin-zones-tests`, `fl-plugin-snapshot-tests`). Eine v2-Seite mit `layout` zeichnet die App nach Zonen; v1 und v2 ohne `layout` zeichnen pixelidentisch wie bisher im automatischen Raster (höchstens 6 Widgets). Manifest v1 bleibt unverändert. Ein fertiges Beispiel mit acht Blöcken: [offline-zonen.flplugin.json](../cpp/examples/plugins/offline-zonen.flplugin.json). **Phase 3:** [bedingte Anzeige](#bedingte-anzeige-show_if-phase-3) (`show_if`) und [Fehlerisolierung](#fehlerisolierung-phase-3) pro Block sind umgesetzt und getestet.

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

- **v2 erkennt der Parser nur an `schema_version: 2` als ganze Zahl.** Alle v1-Regeln gelten weiter (Vorlagen, Texte, Quellen, Einstellungen). Zusätzlich: jedes Widget hat eine **stabile Block-ID** `id` (Kleinbuchstabe am Anfang, dann Kleinbuchstaben/Ziffern/`_`, höchstens 40 Zeichen, eindeutig). Die ID bleibt über Versionen gleich und ist der Schlüssel für Tap-Ziele und Einstellungen.
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

### Bedingte Anzeige (`show_if`, Phase 3)

Ein v2-Widget kann `show_if` tragen (mit und ohne `layout`). Der Block erscheint nur, wenn die Bedingung erfüllt ist. Es gibt genau zwei Formen: **keine Ausdruckssprache**, keine Verknüpfung mit und/oder, höchstens eine Bedingung pro Block. Beispiel: [offline-bedingt.flplugin.json](../cpp/examples/plugins/offline-bedingt.flplugin.json).

```json
"show_if": {"path": "wetter.warnung"}
"show_if": {"path": "wetter.temperatur", "op": ">=", "value": 25}
```

- `path`: `<quelle>` (die ganze Antwort ist vorhanden), `<quelle>.<feld>[.<feld>…]` (Feldnamen und Listenindizes wie in Vorlagen) oder `settings.<schlüssel>` einer **nicht geheimen** Einstellung. Nicht erlaubt: `time`, `date`, `item`, unbekannte Quellen und Einstellungen sowie Einstellungen vom Typ `secret` (feste Meldung „Secret settings cannot appear in conditions.“). Höchstens 128 Zeichen und 8 Segmente.
- `op` (optional): `==`, `!=`, `<`, `<=`, `>`, `>=`. Ohne `op` wird nur geprüft, ob ein Wert vorhanden ist; `value` ist dann verboten. Mit `op` ist `value` Pflicht, und `path` muss ein Feld sein (nicht die ganze Quelle).
- `value`: Text (1–256 Zeichen, keine Steuer-, Bidi- oder Nullbreitenzeichen), endliche Zahl oder `true`/`false`. `null`, Listen und Objekte werden abgewiesen; `<`, `<=`, `>`, `>=` nur mit Zahlen.
- Ein ungültiges `show_if` lässt wie jeder andere Fehler das **ganze Manifest** scheitern. In Schema 1 ist `show_if` ein unbekanntes Feld („Unknown or misplaced field.“).

**Semantik (schlägt geschlossen fehl):** Zuerst muss der Wert *vorhanden* sein, erst dann wird verglichen. Es gibt keine Typumwandlung. „ja“ = Block sichtbar.

| Wert an `path` | ohne `op` (vorhanden) | `==` / `!=` | `<` `<=` `>` `>=` |
| --- | --- | --- | --- |
| fehlt (Pfad, Feld, Listenindex oder die ganze Quelle) | nein | nein | nein |
| `null` | nein | nein | nein |
| nicht endliche Zahl | nein | nein | nein |
| leer: `""`, `[]`, `{}` | nein | nein | nein |
| `false` | **ja** | nur gegen Wahrheitswerte: `== false` ja, `!= false` nein, `== true` nein, `!= true` ja | nein |
| `true` | ja | nur gegen Wahrheitswerte | nein |
| `0` | **ja** | nur gegen Zahlen: `== 0` ja, `!= 0` nein | normal (`>= 0` ja, `> 0` nein) |
| andere Zahl | ja | nur gegen Zahlen | normal |
| Text (auch `" "`, `"0"`, `"false"`) | ja | nur gegen Text, exakt (Groß-/Kleinschreibung zählt) | nein |
| nicht leere Liste oder Objekt | ja | nein | nein |

- „nein“ gilt auch für `!=`: Ein fehlender oder anders typisierter Wert ist nie „ungleich“. `"42"` ist nicht die Zahl `42`, `true` nicht `1`. Wer einen Block bei `false` oder `0` ausblenden will, schreibt `{"op": "==", "value": true}` bzw. `{"op": "!=", "value": 0}`.
- Zahlen werden als Gleitkommazahl verglichen (`-0.0 == 0`); ganze Zahlen über 2^53 verlieren Genauigkeit.
- Einstellungen: gespeicherter Wert, sonst Standardwert. Sind die gespeicherten Einstellungen ungültig, gelten alle als fehlend (und die Fußzeile meldet fehlende Daten).
- Ausgeblendete Blöcke fehlen auf der Seite. Im automatischen Raster rücken die übrigen nach; im Zonen-Layout teilen sich die übrigen Blöcke derselben Zone die Fläche. Eine Zone ohne sichtbaren Block bleibt leer, andere Zonen verschieben sich nicht.
- Vorlagen eines ausgeblendeten Blocks werden nicht ausgewertet und lösen deshalb kein „Plugin-Daten fehlen“ aus. Ausnahme: Hat eine Quelle live noch gar nichts geliefert, meldet die Fußzeile „Plugin-Daten fehlen“, auch wenn alle Blöcke dieser Quelle bedingt sind. Fehlt nur ein Feld in einer gelieferten Antwort, bleibt die Fußzeile normal.
- **Vorschau ohne Daten:** Die Vorschau (Import, Store) wertet Bedingungen gegen das `sample` der Quellen aus, live gegen die Antwort. Ohne oder mit leerem `sample` sind Blöcke mit Quellbedingung in der Vorschau ausgeblendet. Für den Store ist `sample` Pflicht; es sollte so gewählt sein, dass die Vorschau die bedingten Blöcke zeigt. Live ohne Antwort werden Beispieldaten nie verwendet (Bedingungen auf Quellen sind dann falsch).

### Fehlerisolierung (Phase 3)

- **Datenauswertung** (`render_page`): Jeder Block wird einzeln ausgewertet (Bedingung, Vorlagen, Liste, Balken). Wirft die Auswertung eines Blocks, fehlt nur dieser (`Page::failed`); die übrigen bleiben unverändert, die Fußzeile meldet „Plugin-Daten fehlen“. `render_page` wirft nicht: Ein Fehler außerhalb der Blöcke (etwa kein Speicher beim Kopieren von ID, Titel oder Layout) ergibt eine Seite ohne Blöcke mit „Plugin-Daten fehlen“; ID und Titel bleiben, wenn sie sich noch kopieren lassen.
- **Zeichnen** (`draw_page`): Jeder Block (Zonen-Layout) und jede Karte (automatisches Raster, Schema 1 und 2 ohne `layout`) wird einzeln abgesichert. Wirft er, auch halb fertig, wird seine Fläche wieder mit dem Seitenhintergrund gefüllt (der Block ist ausgeblendet). Der Clip-Stapel wird exakt auf den Stand vor dem Block zurückgesetzt, auch Clips, die der Block selbst offen ließ. Die übrigen Blöcke, Kopf- und Fußzeile werden normal gezeichnet. Auch im Raster liegt dafür ein Clip auf der Karte (plus 1 logisches Pixel für die Kantenglättung), damit nach einem Fehler nichts außerhalb übrig bleibt, auch kein Text, der bei kleiner Skalierung über die Karte hinausreichen würde; im Normalfall zeichnet das Raster pixelgleich wie bisher (alle Snapshot-Hashes unverändert). Auch das Messen eines Blocks ist abgesichert: ein Block, der dabei wirft, passt nie.
- **Log:** Warnung im vorhandenen App-Log (Bereich `plugins`), etwa „Plugin offline-zonen: Block dauer ausgeblendet (Fehler beim Zeichnen)“ oder „… (Fehler beim Auswerten der Daten)“. Sie enthält nur Plugin-ID, Block-ID (Schema 1: `#<Position>`) und die Stufe, nie Daten, Einstellungswerte, Secrets oder Ausnahmetexte. Ratenbegrenzt: höchstens ein Eintrag je Plugin, Block und Stufe pro 60 s, höchstens 64 verfolgte Schlüssel (darüber wird verworfen statt zu wachsen).
- Abgefangen werden C++-Ausnahmen (`/EHsc`). Zugriffsverletzungen (SEH) isoliert das nicht.
- Tests injizieren Fehler pro Block über Test-Hooks (`plugins_testing.h`, `plugin_render_testing.h`). Die App ruft `render_page` und `draw_page` immer ohne Hook auf.

### Store

Für den Store braucht ein v2-Manifest `min_app_version` **2.10.0 oder neuer**. So zeigt eine ältere App das Plugin als „braucht neueres ScreenBuddy“ an, bevor sie es herunterlädt; installieren könnte sie es ohnehin nicht (sie lehnt `schema_version: 2` ab).

## Eigenes Plugin erstellen

Das [Schritt-für-Schritt-Tutorial](PLUGIN-TUTORIAL.md) erklärt Import, Einstellungen, Host-Freigaben, Vorlagen und die Prüfung mit dem echten Parser. Beginne mit dem vollständig lokalen [Fokusboard](../cpp/examples/plugins/offline-focus.flplugin.json); die [Wettervariante](../cpp/examples/plugins/public-weather.flplugin.json) demonstriert optional eine öffentliche JSON-Quelle. Sample-Daten werden nie als Live-Ersatz verwendet.

## Wechselfeld pro Zone (Phase 4b, Entwicklung)

Nur `schema_version: 2` mit `layout` erlaubt an einer Zone `"cycle": {"mode": "rotate", "transition": "fade"}`. `mode` ist `rotate` oder `first_available`; `transition` ist `fade` (Standard) oder `none`. In `rotate`-Zonen darf jeder Block `seconds` als ganze Zahl von 3 bis 600 tragen (Standard 8). Ohne `cycle` bleibt die bisherige Stapelung unverändert; Schema 1 akzeptiert diese Schlüssel nicht. Store-Pakete mit Schema 2 brauchen `min_app_version` mindestens `2.10.0`.

`rotate` zeigt die verfügbaren Blöcke in Manifestreihenfolge; `first_available` zeigt den ersten verfügbaren Block und reagiert auf Datenänderungen. Verfügbar bedeutet sichtbar nach `show_if` und ohne fehlende Referenz im jeweiligen Block. Ohne verfügbare Blöcke bleibt die Zone leer. Beim Verlassen der sichtbaren Kacheln oder Manifestwechsel startet sie neu; Ansichten und unabhängige Geräte haben eigene Zeitpläne.

Das Manifest wählt keinen Effekt je Gerät: Monitor 4 ruhige Überblendungsschritte in 400 ms (bei hoher Renderzeit 2 oder Schnitt), Companion 2 Schritte in 400 ms bei verbundenen Clients, seriell ein Band-Vorhang nur bei ausreichend gemessener Rate (sonst Schnitt). Der serielle Takt berücksichtigt Übertragung, Renderzeit und 50 ms Ruhe; globale 8-Zeilen-Bänder werden der Reihe nach aufgedeckt. Reicht das Zyklusbudget nicht, wird geschnitten. Nur Companion verwirft verspätete Schritte; Monitor zeigt den erreichten Zwischenstand. Daten bleiben während des Übergangs eingefroren. Größenwechsel beendet den Übergang, Themewechsel erneuert den Bildcache. Die Mindestdauer steigt bei langsamer Übertragung auf das Vierfache der letzten Übertragungsdauer. Gleichzeitige Wechsel derselben Kachel warten höchstens 600 ms. Keine Töne oder aufdringlichen Effekte.

Beispiel: [offline-wechselfeld.flplugin.json](../cpp/examples/plugins/offline-wechselfeld.flplugin.json). Zum Ausprobieren lokal importieren; die zwei Texte wechseln nach 3 bzw. 5 Sekunden. Hardware-Abnahme und reale serielle Ratenmessung durch Basti stehen aus; bisher wurden ausschließlich simulierte Übertragungen geprüft. Tap-Aktionen sind in Phase 4c umgesetzt (Entwicklung, siehe unten).

## Tap-Ziele für Plugin-Blöcke (Phase 4c, Entwicklung für 2.10.0)

Nur Schema 2 erlaubt `links` und `tap`; Schema 1 und Seiten ohne `tap` behalten ihr bisheriges Verhalten. Store-Pakete mit Schema 2 benötigen weiterhin `min_app_version` mindestens `2.10.0`.

```json
"links": ["news.example.org"],
"tap": {"action": "open_link", "url": "https://news.example.org/article/{{feed.id}}"}
```

`links` steht auf oberster Manifest-Ebene, `tap` am Block. Dies ist ein Feldausschnitt, kein vollständiges Manifest. Höchstens vier unterschiedliche öffentliche DNS-Hosts sind erlaubt: keine IP-Literale, lokalen Namen, Ports oder Platzhalter in der Hostliste. Die URL beginnt exakt mit `https://`, enthält einen wörtlich deklarierten Host, höchstens Port 443, keine Steuerzeichen, Backslashes oder Fragmente und nach Expansion höchstens 2048 Bytes. Platzhalter sind nur in Pfad und Query erlaubt: Quellenfelder und nicht geheime Einstellungen, bei Listen zusätzlich `item.*` je angezeigter Zeile. Jeder Wert wird als UTF-8 nach RFC 3986 prozentkodiert (nur unreserved bleibt frei). Fehlende, leere oder strukturierte Werte ergeben kein Ziel; geheime Einstellungen lehnt schon der Parser ab.

Die geschlossene Aktionsliste ist `open_link` und `next`. `next` hat keine URL und ist nur in einer Zone mit `cycle` erlaubt: `"tap": {"action": "next"}`. Es fordert den nächsten Eintrag mit Daten an, auch wenn die Zone noch auf einen anderen Übergang wartet. Bei `first_available` bleibt die manuelle Auswahl bis zur nächsten Änderung der verfügbaren Einträge bestehen. Die Anfrageliste ist je Ansicht gesichert und auf acht unterschiedliche Plugin/Zonen-Schlüssel begrenzt; nur der Anzeige-Thread ändert den Zeitplan.

**Links öffnen ist standardmäßig aus.** Unter Einstellungen → Plugins zeigt der ausgewählte Eintrag die Link-Hosts und den eigenen Schalter **Links aus diesem Plugin am PC öffnen**. Ein Tipp auf dem Handy öffnet den Link am PC. Diese Zustimmung ist unabhängig von der Quellenfreigabe. Nur ein Update ausschließlich der Linkliste erhält Quellenfreigaben und Aktivzustand; die Linkfreigabe entfällt. Alle anderen Updates und identische Re-Imports widerrufen alle Freigaben. Pro Plugin wird höchstens ein Linkversuch in zwei Sekunden zugelassen, gemeinsam über alle Ansichten. Die Prüfung erfolgt beim Tipp erneut (aktiv, Generation, Zustimmung, HTTPS und Host). Das Log enthält ausschließlich feste Texte, keine URLs, Daten oder Einstellungswerte.

Ziele und Pixel entstehen im selben Zeichenaufruf, Listenzeilen teilen ihre Geometrie mit der Trefferprüfung. Ein Ziel muss nach Veröffentlichung unverändert ruhen: **1,0 s beim Companion**, **0,3 s beim Monitor**. Übergänge, unruhige Ziele und verweigerte Freigaben verschlucken den Tipp ohne Navigation. Flächen ohne Ziel verwenden weiter die Drittel-Regel. Android-TV hat in Phase 4c keine Touch-/D-Pad-Aktion. Das Companion-Protokoll bleibt v1 ohne Bildkennung; die Ruhezeit ist eine Verzugsbegrenzung, keine Bestätigung einer bestimmten JPEG-Version.

Automatisierte Tests verwenden synthetische Daten und einen injizierten Launcher; kein echter Browserstart. Prüfzahlen, Sandbox-Grenzen und offene Hardware-Abnahme: [Testprotokoll](../cpp/TESTPROTOKOLL.md).


Prüfung der Tap-URL beim Parsen und erneut unmittelbar vor dem Browserstart: Der feste Host muss klein geschrieben sein. Fester Pfad-/Query-Text erlaubt nur RFC-3986-Zeichen (unreserved, sub-delimiters, `:`, `@`, `/`, `?`); `%` benötigt genau zwei Hexziffern. Rohes Nicht-ASCII, Leerzeichen, Anführungszeichen, spitze und eckige Klammern, Backslash, Zirkumflex, Backtick, geschweifte Klammern und senkrechter Strich sind verboten. Geschweifte Klammern sind nur als geprüfte Platzhalter `{{referenz}}` erlaubt. Werte `.` und `..` ergeben kein Ziel; feste Punktsegmente im Pfad (auch mit prozentkodierten Punkten) werden abgelehnt. Unicode-Werte bleiben durch Prozentkodierung möglich.

Nur eine tatsächlich geänderte Linkliste bei **ansonsten vollständig identischem Manifest** erhält Quellenfreigaben und Aktivzustand; die Linkfreigabe entfällt. Alle anderen Updates, auch zusätzliche Änderungen an Widgets, Settings oder Tap-Vorlagen, sowie identische Re-Imports setzen alle Freigaben zurück (Quellenplugins werden deaktiviert). Kaputte Linkfreigabe-Dateien werden sicher abgewiesen; fehlgeschlagene Schreibvorgänge erteilen keine Freigabe. Bei Beschäftigung setzt sich der Linkschalter auf den vorigen Zustand zurück. Listenzeilen und Ziele bleiben auch auf flachen Flächen im Block; Ziele funktionieren auch im automatischen Raster ohne Layout.

## Seiten-Editor (P6-3 bis P6-5, Entwicklung für 2.10.0)

Unter **Einstellungen → Plugin-Seite anordnen** werden aktivierte lokale Plugins angeboten. Weitere Einstiege stehen unter **Plugins → Anordnen …** und **Seiten & Reihenfolge**. Manifest v1 zeigt nur „Anordnung braucht Manifest v2“. Eingebaute Seiten und Home Assistant gehören nicht zum Editor.

Der Editor speichert ausschließlich `<id>.layout.json`; die Manifestdatei bleibt unverändert. Rasteranteile (1–12), bis zu 6×6 Zellen, acht Zonen und zwölf Blöcke mit höchstens vier gezeigten Blöcken je Zone entsprechen dem Parser. Bearbeitbar sind Zonen-ID, Lage, Spannweite, Teilung, Wechselmodus/Übergang, Blockreihenfolge, Sichtbarkeit, Zonenzuordnung, Wechselzeit (3–600 Sekunden bei `rotate`) und `show_if`. Quellen, Inhalte, Links, Tap-Aktionen, Settings-Werte und Geheimnisse sind nicht bearbeitbar. Ein ausgeblendeter Block bleibt im Overlay erhalten.

Bedingungen wählen Quellenpfade aus den Beispieldaten bzw. bestehenden Bedingungen und nicht geheime Settings-Schlüssel. Es gibt kein freies JSON-Feld und keine Auswahl geheimer Settings. Der Vergleichswert hat einen ausdrücklichen Typ Text, Zahl oder `true`/`false`. Änderungen an Operator/Werttyp mit **Bedingung bestätigen**, Texte mit Enter oder Fokuswechsel abschließen. Unbestätigte oder ungültige Eingaben sperren **Übernehmen**. „Fehlerstelle auswählen“ fokussiert das betroffene Raster-/Blockfeld. Für Quellenfelder, die weder im Sample noch in bestehenden Bedingungen vorkommen, bietet dieser erste Editor keine freie Pfadeingabe.

Das Raster ist schematisch; die Live-Vorschau im vorhandenen Vorschaubereich nutzt ausschließlich `App::plugin_preview`, Parser, Plugin-Samples und denselben Renderer wie das Display. Querformat 480×320 und Hochformat 320×480 lassen sich umschalten. Uhr/Datum sind reproduzierbare Beispiele (12:34, 2026-01-01). Änderungen werden zusammengefasst; ein veralteter Vorschauauftrag darf kein neues Bild überschreiben. Bei ungültigem Entwurf bleibt das letzte gültige Bild stehen; beim Verlassen kehrt die normale Vorschau zurück.

**Tastatur:** Tab/Shift+Tab wechseln die Felder. Im Raster bewegen Pfeile den Zellfokus, Enter/Leertaste wählen eine Zone oder erzeugen eine freie 1×1-Zone, Shift+Pfeil verändert die Spannweite, Alt+Pfeil verschiebt die Zone. Entf entfernt die Zone und hängt ihre Blöcke in die erste verbleibende Zone um. Passt das nicht (insbesondere mehr als vier Blöcke), wird die Aktion mit Meldung abgewiesen; Blöcke zuerst manuell umhängen. In der Blockliste schaltet Leertaste die Sichtbarkeit und Alt+Auf/Ab die Reihenfolge. Strg+Z, Strg+Y bzw. Strg+Shift+Z wirken in Raster/Blockliste auf die Seitenhistorie, in Textfeldern bleibt die native Text-Rücknahme erhalten. Die Tasten Rückgängig/Wiederholen sind ebenfalls per Tab erreichbar. Die Historie hält 100 Schritte, fasst Pfeilserien am selben Element innerhalb 600 ms zusammen und beginnt nach Plugin-Wechsel/Übernehmen neu. Ein Neuaufbau der Widgets nach dem globalen Verwerfen erhält die abgeschlossene Editorhistorie.

Zellen sind mindestens 32 DIP groß; die Seite scrollt vertikal, ein schmales Raster folgt dem Zellfokus horizontal und clippt seinen Inhalt. MSAA meldet das Raster als Liste mit Zonen-ID, Spalten-/Zeilenbereich, Wechselmodus und Blockzahl; Auswahl hat Text und einen stärkeren Rahmen. Helle Themes werden auf Text ≥4,5:1 und Raster-/Fokuskontrast ≥3:1 geprüft. Der freigegebene globale helle FuchsLabs-Fokusring einschließlich Textfeldrahmen nutzt jetzt `accent_text` (Glut).

**Übernehmen** prüft nochmals über die Store-Schnittstelle, sichert den Vorgänger als `.layout.json.bak` und ersetzt die Datei atomar. Kein Neustart und kein Konfigurationsspeichern; Freigaben und Live-Daten bleiben erhalten. **Auf Manifest zurücksetzen** fragt nach und löscht Overlay und Backup. Ein Overlay mit veraltetem Manifest-Fingerabdruck wird ignoriert und mit Hinweis angeboten. Bei Speicherfehlern bleibt der bisherige wirksame Stand erhalten. Alle Editor-Widgets sind von „Speichern & anwenden“ ausgenommen.

Automatisierte native Tests (Tastatur, MSAA, Historie, Speicheranwendung), Kontrast und eigene Raster-Snapshots sowie Vorschauvergleiche sind vorhanden. USB-/Monitor-/Companion-Handabnahme und der sichtbare Rückfragedialog bleiben offen; Details und Abweichungen im [Testprotokoll](../cpp/TESTPROTOKOLL.md).

Vorschauaufträge lassen Raster, Eingabefelder und Undo/Redo bedienbar; nur Übernehmen, Reset und Pluginwahl sind während laufender Arbeit gesperrt. Ungültige Felder werden markiert, ohne Fokuswechsel und ohne wiederholte Meldung derselben Eingabe. Vor Pluginwechsel mit ungesicherten Änderungen folgt eine DE/EN-Rückfrage mit Standard **Nein**. Übernehmen/Reset synchronisieren vorhandene Widgets und erhalten einen bedienbaren Tastaturfokus. Während des Speicherauftrags bearbeitete Entwürfe werden von der Rückmeldung nicht überschrieben.
