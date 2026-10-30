# Plugin-Paketformat und Validator

© 2026 FuchsLabs · Phase 1 der Plugin-Plattform (Grundlage für den späteren Pluginstore). **Stand:** Manifest-Felder und Validator sind umgesetzt; Store, Signierung und Store-Oberfläche folgen in späteren Phasen und existieren noch nicht.

## Ein Plugin ist eine Datei

Ein Plugin bleibt **eine** UTF-8-JSON-Datei `<id>.flplugin.json` (Schema 1, höchstens 128 KiB) ohne ausführbaren Code. Es gibt keine Archive, Skripte oder Zusatzdateien. Für die geplante Plugin-Sammlung gilt: ein Ordner pro Plugin, `plugins/<id>/<id>.flplugin.json`.

## Neue Manifest-Felder (alle optional, Schema bleibt 1)

| Feld | Regel |
| --- | --- |
| `license` | Nur `MIT` oder `CC0-1.0` (exakte Schreibweise). |
| `author` | 1–80 Zeichen, ohne Steuer- und unsichtbare Formatzeichen. |
| `category` | `system`, `games`, `smart-home`, `info` oder `other`. |
| `min_app_version` | `X.Y.Z`. Ältere ScreenBuddy-Versionen zeigen das Plugin später als nicht unterstützt an (Oberfläche folgt mit dem Store). |

Bestehende Manifeste ohne diese Felder bleiben unverändert gültig und lokal importierbar. Ein ungültiger Wert in einem dieser Felder lässt den Import dagegen scheitern.

## Zusätzliche Store-Regeln

Für die Aufnahme in den Store müssen zusätzlich gelten (`fl-plugin-validate --store`):

- `description` (mindestens 10 Zeichen), `author`, `license`, `category`, `min_app_version` vorhanden.
- Plugins mit Quellen: `refresh_seconds` mindestens 60 und für jede Quelle ein `sample` (Grundlage der Vorschau).
- Dateiname `<id>.flplugin.json`, IDs eindeutig.

Diese Regeln sind eine **automatische Vorprüfung**. Die Freigabe bleibt eine menschliche Prüfung (Zweck, Hosts, Lizenz).

## Validator

```text
fl-plugin-validate [--store] [--app-version X.Y.Z] <datei-oder-ordner>...
```

Er nutzt `plugins.cpp` der App (gleicher Parser), liest nur Dateien und macht keine Netzwerkabfragen. Pro Plugin erscheint `OK` mit ID, Version und den Hosts, an die Daten gesendet würden, oder `FAIL` mit festen Fehlermeldungen. Rückgabewert: 0 gültig, 1 Probleme, 2 falscher Aufruf. Die Tests laufen in GitHub Actions (`Plugin validator`) und lokal mit `sh cpp/tests/build-validator-linux.sh`.

## Offen (spätere Phasen)

Signierung ist als Bibliothek und Werkzeug umgesetzt, siehe [Plugin-Signierung](PLUGIN-SIGNIERUNG.md); offen sind Produktionsschlüssel, Index-Build per Actions und Download. Store-Oberfläche in der App (Phase 3), Moderation und Nutzungsbedingungen (Phase 4). Siehe Plan in PR [#21](https://github.com/PapaFuchs84/ScreenBuddy/pull/21).
