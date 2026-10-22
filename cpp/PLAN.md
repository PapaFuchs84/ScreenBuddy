> **Historischer Umsetzungsplan.** Architekturvorschläge und Verbrauchsschätzungen unten sind keine aktuellen Messergebnisse. Aktueller Stand: [README](README.md) und [Testprotokoll](TESTPROTOKOLL.md). Discord wurde mit D++ umgesetzt; das Einstellungsfenster verwendet WebView2.

# FuchsLabs USB-Display 2.0 – Neubau in C++ (Plan)

Stand: 22.10.2026 · Grundlage: Python-Version 1.2 (`usb-display-src`, ca. 4.800 Zeilen Python + 770 Zeilen webui.html)

## Ziel

Eine einzige Datei `FuchsLabs-USB-Display.exe` (geschätzt 3–5 MB statt heute 5 MB EXE + `_internal`-Ordner).
Doppelklick = Installer, danach ist dieselbe Datei die App und auch der Deinstaller.
Alle Funktionen von 1.2 bleiben, die Einstellungsseite sieht genauso aus wie heute.

## Was schon da ist (auf Meanmachine geprüft)

- Visual Studio Build Tools 2022 (MSVC 14.44), Windows SDK 10.0.26100
- CMake und vcpkg liegen bei den Build Tools mit dabei
- Es muss nichts als Admin installiert werden

## Bibliotheken

So viel wie möglich kommt aus Windows selbst, dadurch bleibt die EXE klein und es gibt keine OpenSSL-Updates zu pflegen.

| Aufgabe | Heute (Python) | Neu (C++) |
|---|---|---|
| Zeichnen, Schrift, Farben | Pillow | **Direct2D + DirectWrite** (Windows), Bahnschrift/Segoe UI wie bisher, farbige Emoji gratis |
| Bilder laden (Avatare, Logos) | Pillow | **WIC** (Windows), PNG/JPEG/GIF, WebP über die installierte Windows-Erweiterung |
| Internet (Steam, Xbox, BF6, RSS, Discord-Avatare) | aiohttp | **WinHTTP** (Windows), TLS über Windows selbst |
| Discord-Bot (Live-Status) | discord.py | **eigener schlanker Gateway-Client** auf WinHTTP-WebSocket (Identify, Heartbeat, Resume, Presence/Member-Events, Member-Chunking) |
| Einstellungsseite 127.0.0.1:8765 | aiohttp.web | **cpp-httplib** (nur Header, ohne TLS), nur an 127.0.0.1 gebunden |
| JSON | eingebaut | **nlohmann/json** (nur Header) |
| config.toml | tomllib + eigener Schreiber | **toml++** (nur Header) |
| RSS/Atom | ElementTree | **pugixml** (klein, keine externen Entities) |
| USB-Seriell (COM7) | pyserial | Win32 `CreateFile`/`WriteFile` direkt |
| Tray, Monitor-Fenster, Touch, DPAPI, Autostart, HWiNFO-Speicher, Leistungszähler | ctypes | Win32 direkt (1:1 übertragbar) |

Alle Fremdbibliotheken werden über vcpkg mit festen Versionen statisch eingebunden (Triplet `x64-windows-static`, keine DLLs).

## Integrierter Installer (in derselben EXE)

- Start aus Downloads o. ä.: kleines Fenster im CI-Design „Installieren“ (DE/EN) → kopiert sich nach `%LOCALAPPDATA%\FuchsLabs\USB-Display` (Vorschlag, ohne Admin; alternativ wie heute `%USERPROFILE%\FuchsLabs\USB-Display`, siehe Entscheidung 2)
- Startmenü-Verknüpfung, Autostart (HKCU\Run mit `--tray`), Eintrag unter „Apps & Features“ zum Deinstallieren (HKCU, kein Admin)
- Update: neue EXE einfach starten → beendet die laufende Version sauber (wie heute `/api/quit`), ersetzt die Datei, startet neu, `config.toml` bleibt
- Übernahme von 1.2: vorhandene `config.toml` wird gelesen, DPAPI-Tokens bleiben gültig (gleiche Entropie `FuchsLabs.USB-Display.v1`, gleiches Format `dpapi:` + Base64), alte Python-Dateien und der Ordner `_internal` werden nach Rückfrage entfernt
- webui.html, ANLEITUNG.html, GUIDE.html und Icon stecken als Ressourcen in der EXE

## Was gleich bleibt

- Seiten System, Freunde (Discord + Steam + Xbox, Zusammenführen nach Name/Hand), BF6, Blogs/RSS, Rotation, Kacheln auf breiten Bildschirmen
- Einstellungsseite mit allen 21 API-Endpunkten, Vorschau, Assistent beim ersten Start, Prüf-Knöpfe, Herstellerprogramm stoppen
- Eigene Farben, Hell/Dunkel/Eigen, beliebige Auflösungen (Serial oder Monitor-Fenster), Touch (Wischen/Tippen), DE/EN, Tray beim Autostart
- Heimnetz-Schutz für RSS (private Adressen blockiert, auch nach Weiterleitung), Sec-Fetch-Site-Prüfung, `X-Frame-Options: DENY`, Header `X-FL: 1`
- Kaffee-Knopf, Fuchs-Logo, Fuchs-Orange/Creme/Navy

Die webui.html wird unverändert übernommen, nur die Antworten des Servers müssen gleich aussehen. Dadurch bleibt auch die Anleitung gültig.

## Ablauf und Aufwand

Gebaut wird in `%USERPROFILE%\FuchsLabs\usb-display-cpp` (eigenes Git-Repo, Python-Ordner bleibt unberührt). Zum Testen läuft die neue App auf **Port 8766** und mit eigenem Mutex-Namen, nie gegen deine laufende 1.2 auf 8765 und nie auf COM7, solange 1.2 läuft (Bildschirm-Test über das Monitor-Fenster).

| Schritt | Inhalt | Aufwand (Sitzungen) |
|---|---|---|
| 1 | Gerüst: CMake, vcpkg, Ressourcen, Logging, Config lesen/schreiben, DPAPI, Mutex | 1 |
| 2 | Zeichnen: Direct2D-Renderer, alle 4 Seiten, Themes, Skalierung, Kacheln; Vergleichsbilder gegen 1.2 | 2–3 |
| 3 | Ausgabe: Serial-Treiber (Bildstreaming, Ausrichtung, Helligkeit), Monitor-Fenster, Touch | 1 |
| 4 | Daten: Systemwerte (CPU/RAM/Netz/HWiNFO), BF6, RSS mit Heimnetz-Schutz, Steam, Xbox | 1–2 |
| 5 | Discord-Gateway-Client | 1–2 |
| 6 | Einstellungsserver mit allen Endpunkten, Vorschau-PNG | 1 |
| 7 | Tray, Autostart, Installer/Updater/Deinstaller, i18n | 1 |
| 8 | Gesamttest, Sicherheits-Check, Leistungsvergleich, Doku/CHANGELOG, Version 2.0 | 1 |

Grob 9–12 Arbeitssitzungen. Nach Schritt 2 bekommst du Vergleichsbilder alt/neu zum Abnicken.

## Risiken

1. **Discord-Gateway selbst bauen** ist das größte Risiko: Reconnects, Rate-Limits und Discord-Änderungen fängt heute discord.py ab. Abhilfe: nur die wenigen Events, die wir brauchen, sauber mit Resume und Backoff. Ausweichlösung wäre die Bibliothek D++ (zieht aber OpenSSL mit rein, EXE wird ca. 4 MB größer).
2. **Bilder sehen minimal anders aus**: DirectWrite glättet Schrift anders als Pillow. Pixelgleich geht nicht, Layout und Farben werden aber 1:1 übernommen.
3. **Virenscanner**: Eine unsignierte EXE, die sich selbst kopiert und in Autostart einträgt, wird von Defender/SmartScreen eher beäugt als heute. Abhilfe: Signieren (kostet Geld) oder bei Microsoft zur Prüfung einreichen.
4. **Speicherfehler**: C++ verzeiht weniger als Python. Abhilfe: moderne C++20-Typen, kein manuelles Speichermanagement, Tests für Parser (RSS, JSON, Freunde-Zusammenführung) und ein Durchlauf mit AddressSanitizer.
5. **Parallel-Pflege**: Änderungen an 1.2 während des Umbaus müssen nachgezogen werden. Vorschlag: 1.2 nur noch für Fehler anfassen.

## Gewinn

Schnellerer Start, weniger RAM (geschätzt 20–40 MB statt 97 MB), eine Datei zum Weitergeben, kein Python/PyInstaller mehr.

## Entscheidungen und aktueller Stand (22.10.2026)

1. Umsetzung bestätigt.
2. Installerziel: `%LOCALAPPDATA%\FuchsLabs\USB-Display` (Windows-Standard, ohne Adminrechte).
3. Discord: D++ (statisch gelinkt).

- C++20 x64 Release baut als eine EXE; die Bibliotheken sind statisch eingebunden.
- 86 lokale Prüfungen bestanden; DPAPI-Kompatibilität mit Version 1.x wurde separat geprüft.
- Einstellungen, Browseroberfläche, lokale API-Schutzregeln, Discord, RSS, Monitor-Ausgabe und Touch wurden lokal erprobt.
- Installerkopie und Konfigurationsübernahme liefen im Scratch-Test. Ein Problem im Teststart wurde behoben: `--no-lcd` wird nun durch den stillen Installerstart weitergegeben; danach antwortete die Testkopie auf Port 8766 und zeigte keine COM7-Zugriffsversuche.
- COM7-Gegenvergleich inzwischen bestanden; Anzeige vom Nutzer bestätigt. Der erste reale Windows-Neustarttest schlug wegen Paketumleitung fehl. Installation außerhalb dieses Kontexts korrigiert; erneuter Neustarttest offen.
- Offen: kompletter Install/Update/Deinstallationsdurchlauf, abschließende Sicherheitsprüfung und Release-Signierung. Der Build ist daher noch kein freigegebener 2.0-Release.