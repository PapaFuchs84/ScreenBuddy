# Plugin-Signierung (Phase 2)

© 2026 FuchsLabs · **Stand:** Bibliothek, Werkzeug und Tests sind umgesetzt. Der Produktionsschlüssel ist erzeugt (privat nur auf dem Rechner des Maintainers, öffentlicher Teil in `plugin_sign.h`), die App enthält Abruf und Store-Oberfläche. **Stand 30.10.2026:** Das Store-Repository `ScreenBuddy-Plugins` ist live; der signierte Index (sequence 2, 8 Plugins) liegt unter `base_url`. Ohne erreichbaren Index meldet die App „nicht erreichbar“.

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

## Store-Logik (Phase 3, Teil 1)

`plugin_catalog` ist der Store-Client ohne Oberfläche. Der Netzwerkabruf wird als Funktion eingespeist (nur HTTPS, öffentliche Adressen, keine Umleitung, Byte-Limit), die Logik ist damit offline testbar.

- **Aktualisieren:** `index.json`, `revoked.json` und beide `.sig` laden, Signatur und Rollback prüfen. Bei Fehlern bleiben die zuletzt akzeptierten Daten unverändert. Ohne Netz wird der Cache genutzt, der beim Laden erneut verifiziert wird (manipulierter Cache wird ignoriert).
- **Liste:** Suche (ID, Name, Beschreibung, Autor), Kategorie, Status `installiert`, `Update`, `nicht unterstützt` (zu alte App), `gesperrt`. Bei Updates stehen **neue Hosts** in der Karte.
- **Installieren:** nur wenn nicht gesperrt, App kompatibel und `package_matches` besteht. Das Plugin bleibt ausgeschaltet, Host-Zustimmungen werden bei jedem Update zurückgesetzt.
- **Sperren:** `enforce_revocations` schaltet installierte, gesperrte Plugins ab.
- Ist kein gültiger Schlüssel eingetragen, ist der Store „nicht verfügbar“ und es erfolgt kein Netzwerkzugriff.

## Store in der App (Phase 3, Teil 2)

- **Rohbyte-Abruf:** `fetch_public_bytes` nutzt dieselben Netzwerkregeln wie Plugin-Quellen (nur HTTPS, öffentliche Adressen, keine Umleitung, kein Proxy, Zeitlimit, Byte-Limit), gibt aber die **unveränderten** Bytes zurück, weil die Signatur genau diese Bytes prüft. `public_https_fetcher` erlaubt nur feste relative Pfade unterhalb von `base_url`.
- **Seite „Plugin-Store“** (Einstellungen → Verbindung, nativ, FuchsLabs-Look, DE/EN): Suche, Kategorien, Karte mit Autor, Version, Lizenz, Status und **„Dieses Plugin sendet Daten an: …“**, Installieren/Aktualisieren mit Bestätigung. Installierte Plugins bleiben **ausgeschaltet**, Host-Zustimmungen werden bei jeder Installation und jedem Update zurückgesetzt; gespeicherte Zugangsdaten werden wie beim Import verworfen, wenn sich Quellen ändern.
- **Abgleich:** höchstens einmal pro Tag im Hintergrund (erster Versuch 30 s nach Start) und manuell über „Store aktualisieren“ (frühestens nach 60 s). Danach werden gesperrte Plugins ausgeschaltet, auch aus dem Zwischenspeicher.
- **Abschaltbar:** Schalter „Plugin-Store verwenden“ (`ui.plugin_store`, Standard an). Aus = keinerlei Abruf. Keine Telemetrie.
- **Ist `store_public_key_hex` leer oder ungültig**, zeigt die Seite „nicht verfügbar“ und es gibt keinen Netzwerkzugriff. Solange unter `base_url` (GitHub Pages von `ScreenBuddy-Plugins`) kein signierter Index liegt, zeigt die Seite „nicht erreichbar“.
- **Veröffentlichen (Maintainer):** Plugins im Store-Repository ändern, `fl-plugin-sign build-index plugins docs <sequence>` ausführen, `fl-plugin-sign sign index docs/index.json <Schlüssel.pem>` (analog `revoked.json`) und committen. Die CI des Store-Repositorys führt `fl-plugin-sign check-index` aus und lehnt jede Änderung ab, bei der Plugins und signierter Index nicht übereinstimmen. Der private Schlüssel liegt nie in GitHub.

## Vorschaubilder (ab 2.6.1)

Die Store-Oberfläche kann zu jedem Plugin ein Vorschaubild zeigen. Die Bilder stehen **nicht** im `index.json` (dessen Schema ist für ältere Apps fest), sondern in einer eigenen signierten Datei `previews.json` mit eigener Signaturdomäne (`FuchsLabs-ScreenBuddy-Plugin-Previews-v1`):

- Bild: `plugins/<id>/preview.png`, PNG, höchstens 200 KiB, 16–1024 Pixel je Seite. Gerendert wird mit `fl-plugin-preview` (Beispieldaten, fester Zeitstempel, keine Live-Daten).
- `previews.json`: `schema_version`, `sequence`, je Bild `id`, `file`, `sha256`, `size`. Erzeugen mit `fl-plugin-sign build-previews <ordner> <ausgabe> <sequenz>`, signieren mit `fl-plugin-sign sign previews …`, in der CI prüfen mit `check-previews`.
- Die App lädt die Datei best effort nach dem Index, prüft Signatur, Rollback-Schutz, Größe, SHA-256 und PNG-Kopf und zeigt das Bild erst danach (Zwischenspeicher mit Hash im Namen). Fehlt die Datei oder ist sie ungültig, bleibt der Store ohne Bilder voll nutzbar.


## Grenzen

- Die Signatur beweist, dass der Maintainer den Index veröffentlicht hat, nicht dass ein Plugin ungefährlich ist. Die Freigabe bleibt eine menschliche Prüfung.
- `generated` ist nur informativ, es gibt keine Ablaufzeit. Ein Angreifer kann einen alten, gültigen Index erneut ausliefern, solange die App seine `sequence` noch nicht kennt (Erststart).
- Getestet: 60 Prüfungen (Schlüssel, Signatur, Manipulation, Domänentrennung, Index, Paket, Sperrliste) und ein Ende-zu-Ende-Lauf des Werkzeugs unter Linux. Windows/MSVC siehe PR.
