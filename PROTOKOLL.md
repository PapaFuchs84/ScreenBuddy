# Projektprotokoll – FuchsLabs USB-Display

Kurzer Verlauf, wie die App entstanden ist, und welche Entscheidungen dabei gefallen sind. Die technischen Änderungen stehen in `CHANGELOG.md`.

## 21.10.2026

| Zeit | Schritt |
|---|---|
| vormittags | Start aus einer Übergabedatei: Python-Skript, das Discord-Freunde auf dem 3,5"-USB-Display zeigt. Demo am Display getestet, Ausrichtung gedreht, eigener Discord-Bot angelegt, Autostart eingerichtet, Autostart der Hersteller-App abgeschaltet. |
| vormittags | Weitere Seiten: Battlefield-6-Rangliste der EA-Freunde, Systemansicht (statt Hersteller-App, deren Netzwerkwerte falsch waren), CPU-Temperatur und Lüfter über HWiNFO, Blog-Updates per RSS. |
| vormittags | Dokumentation und erste EXE (Version 1.0). Streifen auf dem Display mit Testbildern als Hardware- bzw. Kabelfehler eingegrenzt. |
| mittags | Version 1.1: Einstellungsseite im Browser, Herstellername überall entfernt, Token verschlüsselt, schlanker Build, Fassung für Freunde ohne Vorkenntnisse. |
| mittags | Gemeinsame Freundesliste aus Discord, Steam und Xbox. Unabhängiges Sicherheits- und Performance-Review mit Fixes. Kindgerechte, bebilderte Anleitung und Hilfen an jedem Schlüsselfeld. |
| nachmittags | Verbindungsstatus je Plattform, FuchsLabs-CI mit Fuchs-Logo, Hell/Dunkel-Umschalter. |
| nachmittags | Quellcode als Git-Repository, CHANGELOG, dieses Protokoll, Vorlagen für Rückmeldungen in der Testphase. |

## Entscheidungen

- **Discord nur über einen eigenen Bot.** Freundeslisten mit dem persönlichen Zugang wären ein „Self-Bot“ und können zur Kontosperre führen. Der Bot sieht die Mitglieder der Server, auf die er eingeladen ist.
- **Kein PlayStation, kein EA-Freundesabruf.** Es gibt keine offiziellen Schnittstellen; der inoffizielle PlayStation-Weg kann zur Sperre führen. EA-Namen werden deshalb von Hand eingetragen.
- **Steam über die offizielle Web-API, Xbox über OpenXBL.** Beide brauchen einen kostenlosen Schlüssel; bei Steam muss die Freundesliste öffentlich sein.
- **Kein Ab- und Aufblenden.** Das Display liest Helligkeitsbefehle während einer Bildübertragung als Bilddaten.
- **HWiNFO für Temperatur und Lüfter.** Windows liefert diese Werte nicht ohne Hilfsprogramm. Die kostenlose Version schaltet die Freigabe nach 12 Stunden ab, daher der Neustart alle 11 Stunden.
- **Keine Administratorrechte.** Autostart im eigenen Benutzerkonto; nur das Abschalten der Hersteller-Aufgabe fragt einmal per UAC.
- **CI:** Fuchs-Orange und Creme als Hauptfarben (Entscheidung Basti), Navy für Schrift, Windows-Schriften statt Web-Fonts, Hell als Standard.
- **EXE nicht signiert**, weil ein Code-Signing-Zertifikat kostet. Daher die SmartScreen-Warnung beim ersten Start.

## Offene Punkte

- Steam-Schlüssel bzw. öffentliche Freundesliste fehlt noch (Status rot).
- Steam- und Xbox-Abruf bisher nur mit Bastis Konten getestet.
- Alter Ordner der Skript-Vorversion und die alte Aufgabe „FuchsLabs Discord-Display“ können entfernt werden, wenn nichts mehr fehlt.
- Zwei Anleitungs-Screenshots zeigen echte Namen (`anleitung/bilder/uebersicht.png` mit EA-Namen, `anleitung/bilder/freunde.png` und damit `ANLEITUNG.html` mit dem eigenen Xbox-Namen). Sie liegen deshalb nicht im Repository und sollten vor dem Weitergeben neu aufgenommen werden.
