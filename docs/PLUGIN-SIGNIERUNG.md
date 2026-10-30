# Plugin-Signierung (Phase 2)

© 2026 FuchsLabs · **Stand:** Bibliothek, Werkzeug und Tests sind umgesetzt. **Noch nicht vorhanden:** der Produktionsschlüssel (muss Basti offline erzeugen), das Store-Repository mit Index-Build per Actions, das Herunterladen in der App und die Store-Oberfläche (Phase 3). Die App verifiziert bisher nur in Tests; es gibt noch keinen aktiven Store.

## Idee

Der Store besteht aus Daten, die ein Maintainer prüft und mit einem **Ed25519**-Schlüssel signiert. Die App kennt nur den öffentlichen Schlüssel (fest im Code, `store_public_key_hex`) und installiert nichts, was nicht passt. Ein gehackter Server oder GitHub-Account kann damit keine unsignierten Plugins unterschieben.

## Dateien im Store

| Datei | Inhalt |
| --- | --- |
| `index.json` | Liste aller Plugins: ID, Name, Version, Autor, Lizenz, Kategorie, `min_app_version`, Pfad, **SHA-256**, Größe, **Hosts**; dazu `sequence` und `generated`. |
| `index.json.sig` | Abgetrennte Signatur (Base64, 88 Zeichen) über die exakten Bytes von `index.json`. |
| `revoked.json` + `.sig` | Sperrliste: `{"id", "versions": "all" oder ["1.0.0"], "reason"}`, ebenfalls signiert. |
| `plugins/<id>/<id>.flplugin.json` | Das Plugin selbst (eine Datei). |

Beim Signieren wird ein Domänenpräfix vorangestellt (`…Plugin-Index-v1` bzw. `…Plugin-Revoked-v1`). Eine Indexsignatur gilt daher nie als Sperrlistensignatur und umgekehrt.

## Prüfreihenfolge in der App

1. **Signatur zuerst.** Ungültig oder fehlend: Dokument wird nicht weiter ausgewertet.
2. Strenges Parsen (Größe höchstens 1 MiB, unbekannte Felder und doppelte Schlüssel abgewiesen, Pfade nur `plugins/<id>/<id>.flplugin.json`).
3. **Rollback-Schutz:** `sequence` darf nicht kleiner sein als die zuletzt akzeptierte (`sequence_acceptable`).
4. Nach dem Download `package_matches`: Größe, SHA-256, gültiges Manifest, und ID, Version, Lizenz, Autor, Kategorie, `min_app_version` und **Hosts** stimmen mit dem signierten Indexeintrag überein.
5. Gesperrte Plugins (`is_revoked`) werden beim nächsten Abgleich deaktiviert.

Die bisherigen Schutzmechanismen bleiben: Aktivierung standardmäßig aus, Host-Zustimmung je Host, Neu-Zustimmung bei geänderten Quellen.

## Werkzeug `fl-plugin-sign`

```text
fl-plugin-sign keygen <private.pem>
fl-plugin-sign build-index <plugins-root> <out-dir> <sequence>
fl-plugin-sign sign   index|revoked <datei> <private.pem>
fl-plugin-sign verify index|revoked <datei> <public-hex>
```

`build-index` nimmt nur Plugins, die `fl-plugin-validate --store` bestehen, und verlangt die Ordnerstruktur `<root>/<id>/<id>.flplugin.json`. `keygen` überschreibt nie eine vorhandene Schlüsseldatei.

## Schlüsselverwaltung (Vorgabe)

- Den Produktionsschlüssel erzeugt der Maintainer **offline** mit `keygen`; die PEM-Datei gehört nie ins Repository oder in Chats. Für CI später als GitHub-Secret hinterlegen und nur zur Laufzeit in eine temporäre Datei schreiben.
- Der öffentliche Schlüssel (Hex) wird in `plugin_sign.h` eingetragen und mit einem App-Release ausgeliefert.
- **Rotation:** neuen Schlüssel erzeugen, Release mit beiden Schlüsseln ausliefern, Index mit dem neuen signieren, alten Schlüssel nach zwei Releases entfernen. Dieser Mehrschlüssel-Ablauf ist **noch nicht implementiert**; derzeit gibt es genau einen Schlüssel. Bei Verlust oder Kompromittierung hilft nur ein neuer App-Release.
- Die Sperrliste verhindert nicht, dass eine **alte, korrekt signierte Version** weiter im Umlauf ist; deshalb Rollback-Schutz und Sperrliste gemeinsam nutzen.

## Grenzen

- Die Signatur beweist, dass der Maintainer den Index veröffentlicht hat, nicht dass ein Plugin ungefährlich ist. Die Freigabe bleibt eine menschliche Prüfung.
- `generated` ist nur informativ, es gibt keine Ablaufzeit. Ein Angreifer kann einen alten, gültigen Index erneut ausliefern, solange die App seine `sequence` noch nicht kennt (Erststart).
- Getestet: 60 Prüfungen (Schlüssel, Signatur, Manipulation, Domänentrennung, Index, Paket, Sperrliste) und ein Ende-zu-Ende-Lauf des Werkzeugs unter Linux. Windows/MSVC siehe PR.
