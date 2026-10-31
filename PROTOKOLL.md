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
| nachmittags | Quellcode als Git-Repository, CHANGELOG, dieses Protokoll, Vorlagen für Rückmeldungen in der Testphase. Anleitungs-Screenshots ohne echte Namen neu aufgenommen. |
| abends | Version 1.2: eigene Farben, beliebige Auflösungen, HDMI/USB-C-Bildschirme mit optionalem Touch. Doku im Repository nach üblichem Standard ausgebaut (README, CONTRIBUTING, SECURITY). |

## 22.10.2026

| Zeit | Schritt |
|---|---|
| nachmittags | C++20-Neuimplementierung für Windows mit D++ für Discord und einem Ein-Datei-Installer fortgesetzt. Das Einstellungsfenster wird nun als eigenes Win32-Fenster mit eingebettetem Microsoft WebView2 geöffnet; Tray und Mehrfachstart fordern dieses Fenster bei der laufenden Instanz an. |
| nachmittags | WebView2-SDK-Header und statischer x64-Loader eingebunden, Callback-Lebensdauer und externe Link-Schemata abgesichert. Release-EXE erfolgreich gebaut; die eingebauten 86 Prüfungen bestanden. |
| nachmittags | Native Fensterdarstellung noch nicht zur Laufzeit geprüft. Die WebView2 Evergreen Runtime bleibt eine Windows-Voraussetzung und ist nicht in der EXE enthalten. COM7 wurde nicht angesprochen; Hardwarebetrieb bleibt offen. |

## Entscheidungen

- **Dokumentation immer mitpflegen.** Bei Änderungen und Releases die betroffene Doku im selben Arbeitsgang aktualisieren, einschließlich vorhandener Sprachfassungen, Versions-/Downloadlinks und Prüfstatus. Dauerhafte Projektregel in `AGENTS.md` und der PR-Vorlage.

- **Discord nur über einen eigenen Bot.** Freundeslisten mit dem persönlichen Zugang wären ein „Self-Bot“ und können zur Kontosperre führen. Der Bot sieht die Mitglieder der Server, auf die er eingeladen ist.
- **Kein EA-Freundesabruf.** Es gibt keine offizielle Schnittstelle. EA-Namen werden deshalb von Hand eingetragen.
- **PlayStation (31.10., Basti):** doch eingebaut, aber nur lesend und ausdrücklich inoffiziell (NPSSO-Code, DPAPI-verschlüsselt, optional). Vorbild: die Endpunkte des MIT-lizenzierten Projekts `tmrp/psn-mcp`; der Code ist nativ in C++ neu geschrieben, es wird kein Node-Code ausgeliefert.
- **Steam über die offizielle Web-API, Xbox über OpenXBL.** Beide brauchen einen kostenlosen Schlüssel; bei Steam muss die Freundesliste öffentlich sein.
- **Kein Ab- und Aufblenden.** Das Display liest Helligkeitsbefehle während einer Bildübertragung als Bilddaten.
- **HWiNFO für Temperatur und Lüfter.** Windows liefert diese Werte nicht ohne Hilfsprogramm. Die kostenlose Version schaltet die Freigabe nach 12 Stunden ab, daher der Neustart alle 11 Stunden.
- **Keine Administratorrechte.** Autostart im eigenen Benutzerkonto; nur das Abschalten der Hersteller-Aufgabe fragt einmal per UAC.
- **CI:** Fuchs-Orange und Creme als Hauptfarben (Entscheidung Basti), Navy für Schrift, Windows-Schriften statt Web-Fonts, Hell als Standard.
- **EXE nicht signiert**, weil ein Code-Signing-Zertifikat kostet. Daher die SmartScreen-Warnung beim ersten Start.

## Offene Punkte

- Steam-Schlüssel bzw. öffentliche Freundesliste fehlt noch (Status rot).
- Steam- und Xbox-Abruf bisher nur mit Bastis Konten getestet.
- C++-Version 2.0 liegt unter `cpp/`, einschließlich Testprotokoll.
- Windows-Autostart nach Korrektur durch echten Neustart mit Tray-Symbol am 22.10.2026 vom Nutzer bestätigt.
- Alter Ordner der Skript-Vorversion und die alte Aufgabe „FuchsLabs Discord-Display“ können entfernt werden, wenn nichts mehr fehlt.
- Zwei Anleitungs-Screenshots zeigen echte Namen (`anleitung/bilder/uebersicht.png` mit EA-Namen, `anleitung/bilder/freunde.png` und damit `ANLEITUNG.html` mit dem eigenen Xbox-Namen). Sie liegen deshalb nicht im Repository und sollten vor dem Weitergeben neu aufgenommen werden.

## Abschluss 22.10.2026

C++ 2.0 und bisherige App nacheinander an COM7 getestet: beide display_ok=true, physische Anzeige durch den Nutzer bestätigt. Natives Einstellungsfenster sichtbar geprüft. PC auf 2.0 umgestellt, Einstellungen übernommen und Autostart aktualisiert. Details: `cpp/TESTPROTOKOLL.md`.

## Nachtrag: echter Windows-Neustarttest

Der erste Neustarttest scheiterte. Installation und Registryeintrag waren im Windows-Paketkontext von Codex umgeleitet; außerhalb dieses Kontexts fehlte die 2.0-EXE am vorgesehenen Ziel. Installer anschließend als normaler Windows-Prozess ausgeführt, Einstellungen übernommen und tatsächlichen Pfad/Hash/Autostarteintrag sowie display_ok=true geprüft. Erneuter realer Windows-Neustart am 22.10.2026 vom Nutzer bestätigt: Tray-Symbol erschien automatisch.
