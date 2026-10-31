# Sicherheit

**Deutsch** · [English](SECURITY.en.md)

## Unterstützte Versionen

| Version | Sicherheitskorrekturen |
|---|---|
| 2.9.2 | aktueller Release |
| 2.9.1 | auf 2.9.2 aktualisieren |
| 2.9.0 | auf 2.9.2 aktualisieren |
| 2.8.0 | auf 2.9.2 aktualisieren |
| 2.7.0 | auf 2.9.2 aktualisieren |
| 2.0.0–2.6.0 | auf 2.9.2 aktualisieren |
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
- Discord-Token, Steam- und Xbox-Schlüssel, der PlayStation-NPSSO-Code (inoffiziell, wirkt wie ein Passwort; nur an Sony gesendet) sowie das Home-Assistant-Token liegen nur DPAPI-verschlüsselt in `config.toml`, erscheinen nie im Log und werden nie an den Browser ausgeliefert.
- Daten aus dem Netz (Feeds, Avatare, Spielernamen) werden größenbegrenzt verarbeitet und nur als Text gezeichnet.
- Companion-App (standardmäßig aus): lauscht nur an privaten IPv4-Adressen des Heimnetzes (Port 8777, nie 8765), nimmt nur Verbindungen aus privaten Adressbereichen an und spricht nur TLS (selbstsigniertes Zertifikat; die App prüft dessen Fingerabdruck). Koppeln nur mit der PIN, die die Einstellungsseite anzeigt (2 Minuten, einmalig, nach 5 Fehlversuchen 5 Minuten gesperrt). Geräteschlüssel (32 Byte) und der TLS-Schlüssel liegen DPAPI-verschlüsselt in `companion.json`. Ein gekoppeltes Gerät kann dieselben Touch-Ziele auslösen wie ein Touchscreen (Browser/Chat öffnen, Home-Assistant-Schalter).
- Plugins sind reine JSON-Daten ohne Programmcode. Quellen brauchen HTTPS, öffentliche Adressen und die Zustimmung je Host; Zugangsdaten bleiben DPAPI-geschützt. Der Plugin-Store (falls aktiv) prüft Ed25519-Signatur, SHA-256 und Sperrliste vor jeder Installation, siehe [Plugin-Signierung](docs/PLUGIN-SIGNIERUNG.md).
- Die App braucht keine Administratorrechte.

## Ein Plugin melden

Ein Plugin, das Daten an unerwartete Hosts sendet, täuscht, Rechte oder Marken verletzt oder Inhalte Dritter ohne Berechtigung nutzt, meldest du über **Issues → „Plugin melden“** (Vorlage). Bitte nenne Plugin-ID, Version und was auffällt. Ist ein Plugin **akut gefährlich** (zum Beispiel Datenabfluss), melde es wie eine Sicherheitslücke **nicht öffentlich**, siehe oben. Meldungen werden angestrebt innerhalb von 72 Stunden beantwortet; gefährliche Plugins werden über die signierte Sperrliste gesperrt.

**Nicht im Schutzbereich:** Wer Zugriff auf dein Windows-Benutzerkonto hat, kann gespeicherte Zugangsdaten mit denselben Mitteln lesen wie die App. Andere Programme auf demselben PC können die Einstellungsseite aufrufen, erhalten aber keine Zugangsdaten.

Der letzte Prüfbericht steht in [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md).

> Der verlinkte Prüfbericht betrifft den historischen Python-Stand 1.x. Er ist kein vollständiger C++-Audit; siehe [aktuelles Testprotokoll](cpp/TESTPROTOKOLL.md). Seit 2.5.0 wird keine WebView2-Runtime mehr benötigt (die frühere Version 2.3.0 braucht sie noch).
