# Dein erstes ScreenBuddy-Plugin

[English](PLUGIN-TUTORIAL.en.md) · [Systemübersicht](PLUGIN-SYSTEM.md) · [Beispiele](../cpp/examples/plugins/README.md)

Für den ScreenBuddy-2.5-Entwicklungsbuild. Schema 1 ist vorläufig. Ein Plugin ist eine UTF-8-JSON-Datei mit einer Displayseite und bis zu sechs Karten. Du brauchst keinen Compiler und installierst keinen ausführbaren Plugin-Code.

## 1. Offline starten

1. Speichere [offline-focus.flplugin.json](../cpp/examples/plugins/offline-focus.flplugin.json) als Datei mit genau dieser Endung. Bei einem Download aus GitHub die Rohdatei verwenden; keine HTML-Seite oder Markdown-Codezäune mitspeichern.
2. Öffne **Einstellungen → Plugins → Manifest importieren …** und wähle die Datei. Wähle anschließend **Fokus / Focus** in der Liste aus.
3. Ändere unten **Mein Fokus**, **Geplante Minuten** und **Fortschritt**. Verwende kurze Texte, eine Zahl wie `25` und einen Fortschritt von `0` bis `100`. Dezimalzahlen mit Punkt eingeben. Wähle **Plugin-Konfiguration speichern**; diese Änderung gilt sofort.
4. Wähle **Aktivieren / deaktivieren**, falls der Eintrag noch „aus“ anzeigt. Das Beispiel hat `sources: []` und braucht keine Host-Freigabe.
5. Prüfe die Pluginseite in der laufenden Displayausgabe bzw. deren Live-Vorschau: Ortszeit, geplante Minuten, dein Fokus und der Fortschrittsbalken müssen erscheinen. Bei mehreren aktiven Seiten den Seitenwechsel abwarten. `25` ist die geplante Dauer, kein Countdown. Den Fortschritt änderst du selbst in den Einstellungen.
6. Zum Beenden den Eintrag deaktivieren; **Entfernen** entfernt ihn aus der lokalen Verwaltung.

Es gibt derzeit keinen eigenen „Plugin-Vorschau“-Knopf im Plugins-Tab. Eine Live-Vorschau zeigt den laufenden Zustand. Die Entwicklerfunktion `App::plugin_preview` verwendet dagegen Manifest-Standardwerte und `sample`; sie führt keine Netzabfrage aus. Sie ist kein Nachweis für funktionierende Live-Daten. Die folgenden Tests prüfen Daten und Vorlagen, keine physische Displayausgabe.

## 2. Eine eigene Variante erstellen

Kopiere das Fokusbeispiel, ändere `id` zu `mein-fokus`, `name` zu deinem Seitentitel und die Standardwerte. Die ID ist der lokale Schlüssel: Eine andere ID installiert eine zusätzliche Seite; ein erneuter Import derselben ID ersetzt das Manifest. Danach Konfiguration und Aktivierung erneut prüfen. Bereits gespeicherte Einstellungen können Vorrang vor geänderten Standardwerten haben.

So greift eine Karte auf ein Feld zu:

```json
{"type":"metric","label":"Fokusblock","value":"{{settings.minutes}}","unit":"min"}
```

`settings.minutes` muss vorher unter `settings` deklariert sein. Vorlagen sind reine Pfade: keine Berechnungen, Bedingungen, Filter, HTML, JavaScript, Shell-Aufrufe oder Timer. `{{ settings.minutes }}` ist wegen der Leerzeichen ungültig. Ein Balken braucht einen numerisch lesbaren Wert; schreibe die Einheit separat und nicht in `value`. Werte außerhalb von `min`/`max` werden für den Balken begrenzt, die Zahl selbst bleibt unverändert.

## 3. Optional: öffentliche Wetterquelle und Freigabe

[public-weather.flplugin.json](../cpp/examples/plugins/public-weather.flplugin.json) demonstriert einen festen öffentlichen HTTPS-Host ohne privaten Server oder API-Schlüssel. Endpunkt und Felder richten sich nach der [offiziellen Open-Meteo-Dokumentation](https://open-meteo.com/en/docs). Dort die aktuellen Nutzungsbedingungen und gegebenenfalls kommerziellen Voraussetzungen prüfen. Die Werte sind Wettermodelldaten; die enthaltenen `sample`-Werte sind frei erfunden.

1. Importiere die Datei und wähle **Wetter / Weather** aus.
2. Setze Anzeigename, Breitengrad (`-90` bis `90`) und Längengrad (`-180` bis `180`), zum Beispiel die voreingestellten groben Berliner Koordinaten `52.52`/`13.41`. Das Schema erzwingt keine geografischen Zahlenbereiche; verwende gültige Koordinaten. Speichere die Plugin-Konfiguration.
3. Wähle **Aktivieren / deaktivieren**. Lies den Freigabedialog: Der erwartete Host ist ausschließlich `api.open-meteo.com`. Die übertragenen Einstellungsschlüssel sind `latitude` und `longitude`. `place` wird nur lokal angezeigt. Zusätzlich sieht der Dienst deine öffentliche IP-Adresse und die Anfrage. Verwende keine genaue Privatadresse, wenn dir grobe Ortsdaten genügen.
4. Bestätige die Freigabe nur für diesen gewünschten Abruf. Erst ein aktiviertes und freigegebenes Plugin kann Netzabfragen starten. Das Beispiel fragt mit `refresh_seconds: 900` im Abstand von 15 Minuten ab.
5. Prüfe Temperatur, Feuchte und **Datenstand** in der laufenden Ausgabe. Bei ausbleibenden Daten erscheinen Fehlwerte; die fiktiven `18.5`/`62` werden niemals als Live-Ersatz eingespielt. Ein leerer bzw. fehlgeschlagener Abruf ist kein Grund, zusätzliche Hosts freizugeben.
6. **Freigaben widerrufen** deaktiviert das Plugin und widerruft seine Host-Freigaben. Zum erneuten Aktivieren ist die Zustimmung wieder erforderlich.

Für eine eigene API braucht die Quelle eine JSON-Antwort als Objekt, etwa `{"current":{"temperature_2m":18.5}}`. Die Referenz lautet dann `{{weather.current.temperature_2m}}`. `sample` muss dieselbe Struktur mit öffentlichen Beispieldaten haben. Für eine Liste kann ein Objekt beispielsweise `{"entries":[{"title":"Erster Eintrag"}]}` enthalten; dazu passt:

```json
{"type":"list","label":"Einträge","items":"feed.entries","item_template":"{{item.title}}","max_items":3}
```

Hier muss eine Quelle mit ID `feed` existieren. `items` ist ein Pfad ohne Klammern; `item_template` hat Vorlagenklammern. Arrays lassen sich mit `{{feed.entries.0.title}}` adressieren.

## Schema 1: Referenz

Maßgeblich sind [plugins.h](../cpp/src/plugins.h) und [plugins.cpp](../cpp/src/plugins.cpp), für Verwaltung und Schutz zusätzlich `plugin_store.cpp`, `plugin_settings.cpp` und `plugin_transport.cpp` im selben Quellordner. Unbekannte oder falsch platzierte Felder, doppelte JSON-Schlüssel und falsche Typen werden abgewiesen. JSON erlaubt keine Kommentare oder nachgestellten Kommas.

| Bereich | Felder und Grenzen |
| --- | --- |
| Wurzel | Pflicht: `schema_version: 1`, `id`, `name`, `version`, `widgets`. Optional: `description`, `refresh_seconds`, `settings`, `sources`. |
| Identität | `id`: 1–48 ASCII-Zeichen, beginnt mit `a`–`z`, danach Kleinbuchstaben/Ziffern/Bindestriche; kein abschließender oder doppelter Bindestrich, keine Windows-Gerätenamen wie `con`, `com1`. `name`: 1–80 Zeichen, `description`: bis 500. |
| Version / Abruf | `version`: drei Zahlengruppen `X.Y.Z`, jeweils 1–4 Ziffern. `refresh_seconds`: ganze Zahl 15–3600, Standard 60. |
| Einstellungen | Höchstens 12 Objekte: `key`, `label`, `type`, optional `default`, `required` (Boolean). Schlüssel: 1–40 Zeichen, beginnt mit Kleinbuchstabe, danach `a-z`, `0-9`, `_`; eindeutig. Label 1–80 Zeichen. Typ `text`, `number` oder `secret`. Zahl muss endlich sein; Textstandard maximal 4096 UTF-8-Bytes ohne Steuerzeichen. Standard ohne Angabe: `0` bzw. `""`. Secret-Standard ausschließlich `""`. |
| Quellen | Höchstens vier: `id`, `url`, optional `format: "json"`, `headers`, `sample`. ID nach Einstellungsschlüssel-Regeln, nicht `settings`, `time`, `date`, `item`. `sample` ist ein Objekt. |
| URL / Header | Festes `https://` mit öffentlichem DNS-Host, Port nur Standard/443; keine IP-Literale, lokalen Hosts, Fragmente oder dynamischen Hosts. URL-/Header-Vorlagen dürfen nur `settings` referenzieren. Bis 16 Header: `Accept`, `Authorization`, `X-*` (Groß-/Kleinschreibung unerheblich); Werte ohne Steuerzeichen. URL-Werte werden als Komponenten kodiert, Header-Werte nicht. |
| Widgets | 1–6. Alle haben `type`, optional `label`. `text`: `text`; `metric`: `value`, optional `unit`; `bar`: `value`, numerische `min`/`max` mit `max > min`, optional `unit`; `list`: `items`, `item_template`, ganze `max_items` 1–5; `clock`: `format` = `time`, `date` oder `datetime`. Keine frei definierbaren Farben, Positionen oder Klickaktionen. |
| Vorlagen | `{{time}}`, `{{date}}`, `{{settings.key}}`, `{{source.path.0.field}}`; `{{item.field}}` nur in Listeneinträgen. Pfadsegmente: Buchstabe/Unterstrich gefolgt von Buchstaben/Ziffern/Unterstrichen oder reiner Zahlenindex. Secret-Einstellungen sind in Anzeigevorlagen verboten. |
| Ressourcen | Manifest maximal 128 KiB, JSON-Tiefe 32; Quellantwort maximal 512 KiB und Objekt als Wurzel; Anzeigetext auf 2048 UTF-8-Bytes begrenzt, kleine Karten können früher kürzen. Höchstens 32 installierte Plugins. Netzwerk ohne Umleitungen oder Proxy, mit öffentlicher Adressprüfung und 5-Sekunden-Timeout. |

Fehlende/null-Werte erscheinen als `—`; Objekt-/Arraywerte in Text als `[Daten]`. Für eine sinnvolle Anzeige auf konkrete skalare Felder zugreifen. Die lokale Uhrzeit wird von ScreenBuddy geliefert; die Zeitzone eines API-Zeitstempels bestimmt der Anbieter.

## Zugangsdaten und sichere Weitergabe

Geheimnisse ausschließlich in einem deklarierten Feld vom Typ `secret` über die lokale Plugin-Konfiguration eingeben. Unter Windows werden gespeicherte Pluginwerte mit DPAPI geschützt; das verhindert nicht, dass ausdrücklich referenzierte Werte an freigegebene Hosts gesendet werden. Tokens möglichst in `Authorization` statt URL-Parametern verwenden. Ein leeres Zugangsfeld behält den gespeicherten Wert; zum Entfernen **Gespeicherten Wert löschen** verwenden und speichern.

Vor Weitergabe Manifest, `sample`, URL und Header auf eingebettete Zugangsdaten prüfen. Niemals echte Tokens als `default`, Beispieldaten oder feste Header veröffentlichen. Keine Konfiguration, entschlüsselten Werte, Logs oder privaten Screenshots an KI-Assistenten senden. Änderungen an Quellen können eine neue Freigabe und erneute Eingabe gespeicherter Zugangsdaten erfordern; das anschließend ausdrücklich prüfen.

## Wiederverwendbarer Prompt für einen KI-Assistenten

```text
Erstelle ein deklaratives ScreenBuddy-2.5-Plugin (vorläufiges Schema 1).
Ziel: [was soll angezeigt werden?]
Gewünschte Karten (max. 6): [Inhalte]
Offline bevorzugt. Optionale öffentliche JSON-API: [Dokumentationslink oder keine].
Bereinigtes Beispielobjekt ohne personenbezogene Daten: [Beispiel oder keines].

Lies docs/PLUGIN-TUTORIAL.md sowie cpp/src/plugins.h und plugins.cpp
oder fordere diese Inhalte an. Erfinde keine Schemafelder oder API-Endpunkte.
Liefere UTF-8-JSON als .flplugin.json, kurze Import-/Konfigurationsschritte,
erwartete Anzeige und alle Hosts sowie die an sie übertragenen Einstellungsschlüssel.
Keine ausführbaren Skripte, HTML, Berechnungen oder privaten APIs voraussetzen.
Secrets nur als secret-Einstellung mit leerem Standard; niemals im Manifest speichern.
Bitte mich niemals, echte Tokens, Passwörter, Konfigurationen oder private Logs einzufügen.
Nutze ausschließlich fiktive sample-Daten und markiere sie als solche.
Prüfe unbekannte Felder, eindeutige IDs, Limits, Pfade, Pflichtfelder,
Secret-Anzeigeverbote sowie Host-/Header-Regeln. Validiere die fertige Datei mit
dem echten parse_manifest und render_page, falls der Quellcode verfügbar ist.
Sage klar, welche Prüfung tatsächlich ausgeführt wurde und was ungeprüft bleibt.
Ein JSON-Syntaxcheck allein ist keine Schema-, Netzwerk- oder Hardwareprüfung.
```

## Entwicklerprüfung und Fehlerhilfe

Das Ziel `fl-plugin-example-tests` liest jede mitgelieferte `*.flplugin.json` aus `cpp/examples/plugins` mit dem echten Parser, rendert Standard-/Samplewerte und prüft die Fokus-Konfiguration sowie den Datenfluss der Wetterquelle. Es arbeitet offline, ohne Installation, Transport oder Hardware. Aus `cpp` nach CMake-Konfiguration bauen und starten:

```powershell
cmake --build build/screenbuddy-release --target fl-plugin-example-tests
./build/screenbuddy-release/fl-plugin-example-tests.exe
```

Für eigene Dateien akzeptiert das Testprogramm einen alternativen Beispielordner als erstes Argument. Der technische Abnahmestatus steht im [Testprotokoll](../cpp/TESTPROTOKOLL.md). Die externe Wetterabfrage und physische Ausgabe müssen separat geprüft werden.

| Symptom | Prüfen |
| --- | --- |
| Import abgelehnt | Rohes UTF-8-JSON, Schema 1, keine zusätzlichen Felder/Kommas/Kommentare, Größenlimit. |
| Unbekannte Referenz | Quellen-/Einstellungsschlüssel deklarieren, Schreibweise prüfen, keine Leerzeichen in `{{…}}`. |
| Aktivierung verweigert | Pflichtfelder speichern, gewünschte Hosts freigeben. |
| `—` / Daten fehlen | JSON-Pfad, gültige Quellantwort, Verbindung und Zustimmung prüfen; Sample ist kein Live-Fallback. |
| Balken fehlt | `value` muss nur die Zahl ergeben, `max > min`; Einheit in Label/`unit`. |
| Lange Texte abgeschnitten | Kurze Labels und Texte verwenden; die Kartengröße begrenzt die Darstellung. |
