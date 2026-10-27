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

- Pflicht: `schema_version: 1`, eindeutige ID aus Kleinbuchstaben/Ziffern/Bindestrichen, Name, Version `X.Y.Z` und 1–6 Widgets.
- Widgettypen: `clock` (`time`, `date`, `datetime`), `text`, `metric`, `bar`, `list`. Textvorlagen unterstützen geprüfte Referenzen wie `{{time}}`, `{{date}}` und Werte aus deklarierten Quellen.
- Optional: höchstens 12 Einstellungen und vier JSON-Quellen mit festgelegtem HTTPS-Host. Der begrenzte HTTPS-Abruf läuft in einem eigenen Worker. Host-Zustimmungen werden lokal gespeichert; Plugin-Einstellungen sind DPAPI-geschützt. Ein isolierter öffentlicher Live-Abruf bestand, aber der vollständige App-Ablauf mit Host-Freigabe ist noch nicht abgenommen. Vorschau-Daten einer Quelle werden nur für eine explizite Vorschau benutzt, nie als Live-Ersatz.
- Grenzen: Manifest 128 KiB, maximal 32 installierte Plugins, JSON-Tiefe 32. Unbekannte Felder und doppelte JSON-Schlüssel werden abgewiesen.
- Sicherheitszustand: Kein Plugin-Code wird geladen. Der Abruf ist auf deklarierte HTTPS-Hosts und begrenzte Antworten beschränkt; Host-Freigaben werden lokal gespeichert, Plugin-Einstellungen mit DPAPI geschützt. Zertifikats-Sperrprüfungen (CRL/OCSP) fehlen noch. Es gibt noch keinen öffentlichen Marketplace.

Prüfungen: Manifest-Parser (223), Speicher (127), Transport (134, einschließlich lokalem WinSock-DNS-Regressionstest) und Plugin-Einstellungen (135) bestanden. Ein direkter, isolierter HTTPS-Abruf der öffentlichen Open-Meteo-Beispielquelle lieferte die erwarteten Wetterfelder. Produktive Konfiguration und Host-Freigaben wurden nicht verändert. Vollständiger App-Ablauf, Zertifikats-Sperrprüfung und Hardwareausgabe sind damit nicht abgenommen; siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md).

## Eigenes Plugin erstellen

Das [Schritt-für-Schritt-Tutorial](PLUGIN-TUTORIAL.md) erklärt Import, Einstellungen, Host-Freigaben, Vorlagen und die Prüfung mit dem echten Parser. Beginne mit dem vollständig lokalen [Fokusboard](../cpp/examples/plugins/offline-focus.flplugin.json); die [Wettervariante](../cpp/examples/plugins/public-weather.flplugin.json) demonstriert optional eine öffentliche JSON-Quelle. Sample-Daten werden nie als Live-Ersatz verwendet.
