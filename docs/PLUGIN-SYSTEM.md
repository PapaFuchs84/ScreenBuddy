# Plugin-System im ScreenBuddy-2.5-Entwicklungsbuild

Der aktuelle Vertrag ist **vorläufig**. Er besteht aus JSON-Daten; native Bibliotheken, JavaScript und Shell-Befehle werden nicht geladen. Ein Plugin ohne Quellen kann lokal importiert und als Displayseite angezeigt werden. Ein Plugin mit `sources` wird beim Import deaktiviert und kann derzeit nicht aktiviert werden.

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
- Optional: höchstens 12 Einstellungen und vier JSON-Quellen mit fester HTTPS-DNS-Domain. Die Schnittstelle validiert sie, ruft sie aber noch nicht ab. Vorschau-Daten einer Quelle werden nur für eine explizite Vorschau benutzt, nie als Live-Ersatz.
- Grenzen: Manifest 128 KiB, maximal 32 installierte Plugins, JSON-Tiefe 32. Unbekannte Felder und doppelte JSON-Schlüssel werden abgewiesen.
- Sicherheitszustand: Kein Plugin-Code, kein Quellen-Netzwerkverkehr, keine Secrets-Speicherung, keine Berechtigungsfreigabe und noch kein öffentlicher Marketplace.

Tests: `fl-plugin-tests` (129) und `fl-plugin-store-tests` (91) bestanden am 24.10.2026 unter Windows. Die sichtbare UI und Hardwareausgabe sind noch nicht abgenommen.
