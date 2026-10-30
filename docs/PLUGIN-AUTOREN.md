# Plugins für ScreenBuddy einreichen

Kurzanleitung für Autoren. Englisch: [PLUGIN-AUTHORS.md](PLUGIN-AUTHORS.md). Format: [Tutorial](PLUGIN-TUTORIAL.md) und [Paketformat](PLUGIN-PAKETFORMAT.md). Regeln: [Store-Bedingungen](PLUGIN-STORE-BEDINGUNGEN.md).

1. **Plugin schreiben** nach dem Tutorial (eine Datei `<id>.flplugin.json`, kein Code).
2. **Pflichtfelder für den Store:** `description` (mindestens 10 Zeichen), `author`, `license` (`MIT` oder `CC0-1.0`), `category` (`system`, `games`, `smart-home`, `info`, `other`), `min_app_version`. Plugins mit Quellen: `refresh_seconds` mindestens 60 und ein `sample` je Quelle.
3. **Prüfen:** `fl-plugin-validate --store --app-version 2.6.0 deine-datei.flplugin.json`. Die Ausgabe nennt die Hosts, an die Daten gehen würden; sie müssen zum Zweck passen.
4. **Einreichen:** Pull Request im Repository `ScreenBuddy-Plugins` unter `plugins/<id>/<id>.flplugin.json` (das Repository ist öffentlich). Die CI wiederholt die Prüfung; ein Maintainer prüft Zweck, Hosts und Lizenz.
5. **Updates:** neue Version (`X.Y.Z` erhöhen). Fügt sie Hosts hinzu, müssen Nutzer erneut zustimmen. Nicht erlaubt: Zugangsdaten, Tokens oder Schlüssel im Manifest.
