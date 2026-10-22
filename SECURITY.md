# Sicherheit

**Deutsch** · [English](SECURITY.en.md)

## Unterstützte Versionen

| Version | Sicherheitskorrekturen |
|---|---|
| 2.0.3 | aktueller Release |
| 2.0.0–2.0.2 | auf 2.0.3 aktualisieren |
| 1.2 | historisch |
| 1.1 | nur nach Rücksprache |
| 1.0 | ❌ |

## Eine Lücke melden

Bitte **kein öffentliches Issue** anlegen. Stattdessen:

- Basti (FuchsLabs) direkt per Nachricht informieren.
- Falls im Repository aktiviert, geht es auch über **Security → Report a vulnerability** auf GitHub.

Hilfreich sind: betroffene Version, was ein Angreifer tun kann, Schritte zum Nachstellen, und falls vorhanden ein Vorschlag zur Behebung. Schicke **keine echten Tokens oder Schlüssel** mit, auch nicht deine eigenen.

FuchsLabs bestätigt den Eingang so bald wie möglich, hält dich über die Behebung auf dem Laufenden und nennt dich auf Wunsch im Changelog.

## Was zum Sicherheitsmodell gehört

- Die Einstellungsseite lauscht nur auf `127.0.0.1` und darf aus dem Netzwerk und von fremden Webseiten aus nicht bedienbar sein (Host-Prüfung, eigener Header für Änderungen, `Sec-Fetch-Site`-Prüfung, kein Einbetten).
- Discord-Token, Steam- und Xbox-Schlüssel liegen nur DPAPI-verschlüsselt in `config.toml`, erscheinen nie im Log und werden nie an den Browser ausgeliefert.
- Daten aus dem Netz (Feeds, Avatare, Spielernamen) werden größenbegrenzt verarbeitet und nur als Text gezeichnet.
- Die App braucht keine Administratorrechte.

**Nicht im Schutzbereich:** Wer Zugriff auf dein Windows-Benutzerkonto hat, kann gespeicherte Zugangsdaten mit denselben Mitteln lesen wie die App. Andere Programme auf demselben PC können die Einstellungsseite aufrufen, erhalten aber keine Zugangsdaten.

Der letzte Prüfbericht steht in [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md).

> Der verlinkte Prüfbericht betrifft den historischen Python-Stand 1.x. Er ist kein vollständiger C++-Audit; siehe [aktuelles Testprotokoll](cpp/TESTPROTOKOLL.md). WebView2 Runtime ist eine separate Voraussetzung.
