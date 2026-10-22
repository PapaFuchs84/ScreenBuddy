# FuchsLabs USB-Display – Dokumentation

Version 1.2 · Stand 21.10.2026 · © 2026 FuchsLabs

Die App steuert ein 3,5"-USB-Display (480 × 320 Pixel, „Revision A“) direkt an und ersetzt die Hersteller-App des Displays. Sie zeigt im Wechsel vier Seiten und wird komplett über eine Einstellungsseite im Browser bedient. Für den schnellen Einstieg gibt es die `ANLEITUNG.md`; dieses Dokument ist die ausführliche Referenz.

## Inhalt

1. [Seiten](#1-seiten)
2. [Installation und erster Start](#2-installation-und-erster-start)
3. [Einstellungsseite](#3-einstellungsseite)
4. [Konfigurationsdatei](#4-konfigurationsdatei)
5. [HWiNFO für CPU-Temperatur und Lüfter](#5-hwinfo-für-cpu-temperatur-und-lüfter)
6. [Sicherheit und Datenschutz](#6-sicherheit-und-datenschutz)
7. [Effizienz](#7-effizienz)
8. [Fehlerbehebung](#8-fehlerbehebung)
9. [Kommandozeile](#9-kommandozeile)
10. [Technischer Aufbau](#10-technischer-aufbau)
11. [Selbst bauen](#11-selbst-bauen)
12. [Deinstallation](#12-deinstallation)

---

## 1. Seiten

Die Anzeige wechselt alle `page_seconds` Sekunden (Standard 15). Reihenfolge und Sichtbarkeit stellst du in der Einstellungsseite ein.

| Seite | Inhalt | Aktualisierung |
|---|---|---|
| **System** | CPU (Last, Takt, Temperatur, Lüfter), GPU (Last, Temperatur, Lüfter, VRAM), RAM, Datenträger, Netzwerkraten mit Verlauf | etwa jede Sekunde, solange sichtbar |
| **Freunde (alle Plattformen)** | eine gemeinsame Liste aus Discord, Steam und Xbox; dieselbe Person nur einmal, mit Marken D/S/X für ihre Plattformen | Discord sofort, Steam jede Minute, Xbox alle 3 Minuten |
| **Discord-Freunde** | nur die Discord-Freunde, mit Avatar, Status und Aktivität („Spielt …“) | sofort bei Änderungen |
| **Battlefield 6 · Rangliste** | Profile der eingetragenen Spieler nach Rang, mit Abzeichen, K/D, Siegquote | alle 15 Minuten |
| **Blog-Updates** | neuester Beitrag je eingetragenem Blog | alle 30 Minuten |

Ist keine Seite aktiv, zeigt das Display einen Willkommenshinweis mit der Adresse der Einstellungsseite. Fehler (z. B. ungültiger Discord-Token) erscheinen als lesbarer Satz direkt auf der betroffenen Seite.

**Details der Seiten**

- *System:* CPU-Last wie im Task-Manager („% Processor Utility“), Takt aus der Leistungsanzeige. Die GPU-Werte kommen über `nvidia-smi` (Nvidia-Treiber). Netzwerkraten stammen aus den 64-Bit-Zählern des Adapters, über den dein Internet läuft (automatisch gewählt), und stimmen mit Windows überein. Temperaturen ab 75 °C werden gelb, ab 85 °C rot.
- *Discord:* sortiert nach Status, dann Name. Farbiger Punkt am Avatar = Status, Handy-Symbol = am Smartphone. Emojis werden ausgeblendet (die Windows-Schriften zeigen sie sonst als Kästchen).
- *Freunde (alle Plattformen):* Quellen sind Discord (Bot, siehe unten), Steam (offizielle Web-API von Valve) und Xbox (Dienst [OpenXBL](https://xbl.io)). Gleiche Personen werden zusammengefasst: manuell über **Freunde → Personen zusammenführen** oder automatisch bei gleichem Namen (abschaltbar; nie werden zwei Konten derselben Plattform vermischt). Online hat Vorrang, das Spiel kommt von dem Konto, das gerade spielt. Für **Steam** brauchst du einen kostenlosen Schlüssel von <https://steamcommunity.com/dev/apikey> und die Freundesliste im Steam-Profil muss öffentlich sein. Für **Xbox** brauchst du einen OpenXBL-Schlüssel aus dem Dashboard auf xbl.io (kostenlose Stufe, rund 150 Abrufe pro Stunde; die App fragt nur alle 3 Minuten). **PlayStation** und **EA** sind bewusst nicht eingebaut: Es gibt keine offiziellen Schnittstellen für Freundeslisten; der inoffizielle PlayStation-Zugang kann zur Kontosperre führen. Die Schlüssel werden wie der Discord-Token verschlüsselt gespeichert, und **Verbindung prüfen** zeigt sofort, ob sie funktionieren. Damit niemand doppelt erscheint, entfällt die reine Seite „Discord-Freunde“ automatisch, solange die gemeinsame Liste Discord einbezieht. Bei Konten mit ähnlichen, aber nicht gleichen Namen (z. B. „Basti“ und „Bastian“) schlägt die Einstellungsseite unter **Personen zusammenführen** eine Verknüpfung vor, die du mit einem Klick bestätigst oder ablehnst. Das Zusammenführen ist mit `test_friends.py` abgesichert (u. a. gleichzeitig online auf allen Plattformen = genau eine Zeile mit allen Marken).
- *Battlefield 6:* sortiert nach Rang, bei Gleichstand nach K/D. Fällt die Datenquelle kurz aus, bleiben die letzten Werte stehen.
- *Blogs:* Titel laufen über höchstens zwei Zeilen, Rubrik-Präfixe wie „Anzeige:“ entfallen.

---

## 2. Installation und erster Start

Voraussetzungen: Windows 10/11 (64 Bit), das Display per USB. Python wird **nicht** benötigt.

1. Ordner `FuchsLabs-USB-Display` entpacken und an einen festen Ort legen (z. B. `Dokumente`). Liegt er in einem Ordner ohne Schreibrecht wie „Programme“, speichert die App Einstellungen und Protokoll stattdessen unter `%APPDATA%\FuchsLabs\USB-Display`.
2. `FuchsLabs-USB-Display.exe` doppelklicken. Windows warnt, weil die Datei nicht mit einem kostenpflichtigen Zertifikat signiert ist: **Weitere Informationen → Trotzdem ausführen**.
3. Die Einstellungsseite öffnet sich im Browser. Auf **Start** siehst du, ob das Display erkannt wurde.
4. Bei Bedarf: **Hersteller-App beenden** und deren Autostart **abschalten** (beides per Klick auf der Seite Start; für das Abschalten fragt Windows einmal nach der Administratorbestätigung).
5. **Mit Windows starten** einschalten, wenn die App immer laufen soll.

Die App läuft ohne Fenster im Hintergrund. Das Symbol im Infobereich öffnet per Doppelklick die Einstellungen, per Rechtsklick: Einstellungen öffnen · Neu starten · Beenden. Ein zweiter Programmstart öffnet nur die Einstellungsseite der laufenden App (es läuft immer nur eine Instanz).

### Discord-Bot (nur für die Discord-Seite)

Freundeslisten gibt Discord nur über den persönlichen Zugang heraus. Das wäre ein „Self-Bot“, verstößt gegen die Nutzungsbedingungen und kann zur Kontosperre führen. Deshalb nutzt die App einen eigenen Bot, der die Mitglieder der Server sieht, auf die er eingeladen ist.

1. <https://discord.com/developers/applications> → **New Application**.
2. **Bot** → **Reset Token** → Token kopieren. Unter *Privileged Gateway Intents* **Presence Intent** und **Server Members Intent** einschalten, *Public Bot* ausschalten.
3. **OAuth2 → URL Generator** → Haken bei **bot** → Adresse öffnen und den Server wählen.
4. In der Einstellungsseite bei **Discord** den Token einfügen und speichern. Unter **Personen** wählst du dann aus, wen du sehen willst.

Der Name des Bots ist der Name der „Application“ und lässt sich im Developer Portal jederzeit ändern.

---

## 3. Einstellungsseite

Sie läuft in der App selbst und ist nur auf deinem PC erreichbar: `http://127.0.0.1:8765/` (Port einstellbar). Änderungen werden erst mit **Speichern & anwenden** übernommen; die Anzeige startet dann etwa 10 Sekunden neu.

| Bereich | Einstellungen |
|---|---|
| **Start** | Zustand (Display, Discord), Hersteller-App beenden/abschalten, Autostart, Ersteinrichtung |
| **Übersicht** | Live-Vorschau des Displays, Seiten ein/aus und Reihenfolge, Anzeigedauer, Helligkeit |
| **Freunde** | gemeinsame Liste: Seite an/aus, Discord einbeziehen, Steam- und Xbox-Zugang mit „Verbindung prüfen“ und Anleitung, Personen zusammenführen |
| **Discord** | Seite an/aus, Bot-Token, Offline-Freunde, Auswahl der Personen aus den Server-Mitgliedern, Server- und Ignorierlisten |
| **Battlefield 6** | Spieler hinzufügen (PC/PlayStation/Xbox), sofort **Prüfen**, Abrufintervall |
| **Blogs** | Blog-Adressen hinzufügen und mit **Prüfen** testen, Anzahl Beiträge, Intervall |
| **System** | Netzwerkadapter, Einheit (Mbit/s oder MB/s), Laufwerk, Sensoren für CPU-Temperatur und Lüfter |
| **Display** | Ausrichtung, COM-Port, Zurücksetzen beim Start, Ausschalten beim Beenden, Port der Einstellungsseite, Neu starten, Beenden |

**Verbindungsstatus:** In den Reitern Discord und Freunde zeigt eine Statuszeile dauerhaft, ob der Zugang funktioniert: grün mit Kontoname, Anzahl der Freunde und Zeitpunkt der letzten erfolgreichen Abfrage (Discord: live), rot mit der Fehlermeldung, wenn etwas nicht klappt (bei Steam/Xbox mit dem Zeitpunkt des letzten Erfolgs), grau, wenn nichts eingerichtet ist. Sie aktualisiert sich alle 5 Sekunden von selbst.

Der Bot-Token wird nie an den Browser zurückgegeben; das Feld zeigt nur, ob einer gespeichert ist. Leer lassen = behalten.

---

## 4. Konfigurationsdatei

`config.toml` liegt neben der EXE und wird von der Einstellungsseite geschrieben (Sicherungskopie: `config.toml.bak`). Von Hand bearbeiten ist möglich; danach die App neu starten. Fehlende Einträge bekommen Standardwerte, ungültige Werte werden auf erlaubte Bereiche begrenzt.

| Abschnitt | Schlüssel (Standard) |
|---|---|
| `[discord]` | `enabled` (true), `token` (verschlüsselt), `guild_ids` ([]), `friend_ids` ([]), `ignore_ids` ([]), `show_offline` (false) |
| `[display]` | `port` ("AUTO"), `orientation` ("landscape"; auch `reverse_landscape`, `portrait`, `reverse_portrait`), `brightness` (50), `reset_on_start` (true), `page_seconds` (15), `pages` (["system","discord","bf6","rss"]), `title`, `screen_off_on_exit` (true), `theme` ("hell"; oder "dunkel" – gilt für Display, Einstellungsseite und Anleitung; "eigen" = eigene Farben), `color_base` ("hell"/"dunkel", Vorlage für eigene Farben), `color_bg`, `color_card`, `color_header`, `color_accent`, `color_text`, `color_muted` ("#RRGGBB", leer = Vorlage), `output` ("serial" = 3,5"-USB-Display, "monitor" = HDMI/USB-C-Bildschirm als Vollbildfenster), `monitor` ("" = erster Nicht-Hauptbildschirm, sonst z. B. `\\.\DISPLAY2`), `touch` (false; Seitenwechsel per Wischen/Tippen, nur bei `output = "monitor"`) |
| `[system]` | `enabled` (true), `net_adapter` ("" = automatisch), `net_unit` ("bit" oder "byte"), `disk` ("C:"), `cpu_temp_sensor`, `cpu_fan_sensor` ("" = automatisch), `title` |
| `[friends]` | `enabled` (false), `title` ("Freunde"), `include_discord` (true), `auto_merge` (true), `show_offline` (false), `steam_api_key` (verschlüsselt), `steam_id` (Profil-Link oder 17-stellige ID), `xbox_api_key` (verschlüsselt), `links` ([], Verknüpfungen als `"Name\|discord:ID\|steam:ID\|xbox:Gamertag"`) |
| `[bf6]` | `enabled` (false), `players` ([], PC-Namen; Konsole als `"Name@psn"` / `"Name@xbox"`), `refresh_minutes` (15), `title` |
| `[rss]` | `enabled` (false), `feeds` ([]), `refresh_minutes` (30), `max_items` (4), `max_per_feed` (1), `title` |
| `[ui]` | `enabled` (true), `port` (8765), `setup_done` (false), `language` ("de" oder "en"; beim ersten Start wie die Windows-Sprache – gilt für Einstellungsseite, Display und Infobereich) |

---

## 5. HWiNFO für CPU-Temperatur und Lüfter

Windows stellt diese Werte nicht ohne Hilfsprogramm bereit. Die App liest sie aus dem Shared Memory des kostenlosen [HWiNFO64](https://www.hwinfo.com/). Ohne HWiNFO zeigt die Systemseite an diesen Stellen „–“; alles andere funktioniert.

`hwinfo-einrichten.bat` (Rechtsklick → **Als Administrator ausführen**) stellt HWiNFO sparsam ein (nur Sensormodul, Messung alle 2 Sekunden, minimiert, keine Update-Prüfung, Shared Memory an) und richtet die Aufgabe „FuchsLabs HWiNFO“ ein. Sie startet HWiNFO nach der Anmeldung unsichtbar und alle 11 Stunden neu, weil die kostenlose Version die Freigabe nach 12 Stunden abschaltet. Der Pfad `C:\Program Files\HWiNFO64\` ist in den Dateien festgelegt; bei abweichender Installation dort anpassen. Welche Sensoren verwendet werden, wählst du in der Einstellungsseite unter **System**.

---

## 6. Sicherheit und Datenschutz

**Was die App tut**

- Läuft **ohne Administratorrechte** (Standardbenutzer reicht). Nur das Abschalten der Autostart-Aufgabe der Hersteller-App braucht einmalig die Windows-Bestätigung, und die gibst du selbst.
- Der Autostart wird im eigenen Benutzerkonto eingetragen (`HKCU\...\Run`), nicht systemweit.
- Keine Telemetrie, kein Konto, keine Werbung. Die App liest weder Browserverlauf noch Passwörter.

**Schutz von Zugangsdaten**

- Der Discord-Bot-Token wird mit der Windows-Datenschutz-API (**DPAPI**) verschlüsselt gespeichert (`dpapi:…` in `config.toml`). Entschlüsseln kann ihn nur dasselbe Windows-Benutzerkonto auf demselben PC; eine kopierte Datei ist auf anderen PCs wertlos. Alte Klartext-Tokens werden beim Start automatisch verschlüsselt.
- Der Token wird nie protokolliert und nie an den Browser ausgeliefert. Das Protokoll (`usb-display.log`, höchstens 2 × 0,5 MB) enthält keine Zugangsdaten.

**Netzwerk**

- Fremde Avatar-Bilder (Steam, Xbox) werden nur als PNG, JPEG oder WebP geöffnet und auf 2 MB begrenzt. Steam- und Xbox-Schlüssel werden wie der Discord-Token verschlüsselt gespeichert und nur an die jeweilige Schnittstelle gesendet.
- Alle Verbindungen laufen über HTTPS mit Zertifikatsprüfung (Standard von aiohttp und discord.py; die Prüfung wird nirgends abgeschaltet).
- Die Einstellungsseite lauscht **nur auf `127.0.0.1`**, ist also aus dem Netzwerk nicht erreichbar. Zusätzlich lehnt sie Anfragen mit fremdem Host-Namen ab (Schutz vor DNS-Rebinding) und verlangt für jede Änderung einen eigenen Header, den fremde Webseiten im Browser nicht senden dürfen (Schutz vor Angriffen über präparierte Webseiten).
- Eingaben der Einstellungsseite werden auf Typ, erlaubte Werte und Wertebereiche geprüft und in der Länge begrenzt. Texte aus dem Netz (Blog-Titel, Spielernamen) werden nur als Text gezeichnet bzw. im Browser per `textContent` eingesetzt, nie als HTML.
- Blog-Antworten werden auf 3 MB begrenzt; Feed-XML liest der Standardparser von Python.

**Lieferkette und Build**

- Alle Pakete sind in `requirements.txt` auf genaue Versionen festgelegt. Der Build läuft in einer frischen virtuellen Umgebung (`build.bat`); die EXE enthält Firmenname und Version.
- Die EXE ist **nicht digital signiert**, weil dafür ein kostenpflichtiges Code-Signing-Zertifikat nötig ist. Daher die SmartScreen-Warnung beim ersten Start. Wer die Datei signieren möchte, kann das nach dem Build mit `signtool` tun.

**Grenzen:** Wer Zugriff auf dein Windows-Benutzerkonto hat, kann den Token mit denselben Mitteln lesen wie die App. Die Einstellungsseite ist für jedes Programm auf deinem PC erreichbar (nicht für andere Geräte); den Token gibt sie aber nicht heraus.

---

## 7. Effizienz

Gemessen auf Windows 11 mit allen vier Seiten, Discord verbunden:

| | Wert |
|---|---|
| Programmgröße | 36 MB (vorher 66 MB; numpy und ungenutzte Module entfernt) |
| Arbeitsspeicher | rund 85–90 MB |
| Start bis Einstellungsseite erreichbar | etwa 1,2 Sekunden (die Display-Verbindung läuft parallel) |
| CPU | im Leerlauf nahe 0 %; wenige Prozent eines Kerns, solange die Systemseite sichtbar ist |
| Bildumwandlung | etwa 2 ms pro Bild; übertragen werden nur geänderte Zeilenbänder |

Wie das gelingt: Discord wird nur geladen, wenn die Seite aktiv ist. Datenabrufe laufen asynchron in einem einzigen Prozess. Die Einstellungsseite ist eine kleine Datei ohne externe Bibliotheken und belegt nichts, solange sie nicht geöffnet ist. Datenquellen werden nur in großen Abständen abgefragt (15/30 Minuten). Systemwerte liest ein eigener Thread einmal pro Sekunde. Das ganze Bild wird nur bei Seitenwechseln und alle 5 Minuten vollständig gesendet (heilt Übertragungsfehler), sonst nur Änderungen.

---

## 8. Fehlerbehebung

Das Protokoll steht in `usb-display.log` neben der EXE. Die meisten Probleme erklärt die Seite **Start** direkt.

| Symptom | Ursache und Lösung |
|---|---|
| „Die Hersteller-App des Displays läuft …“ | Auf **Start** → **Hersteller-App beenden**, danach deren Autostart **abschalten**. |
| „Display nicht gefunden“ | Kabel prüfen, anderen USB-Anschluss probieren; notfalls bei **Display** den COM-Port von Hand eintragen (Geräte-Manager → Anschlüsse). Die App versucht es alle 10 Sekunden erneut. |
| Bild steht auf dem Kopf | **Display → Ausrichtung** auf die gedrehte Variante stellen. |
| Display zu dunkel | **Übersicht → Helligkeit** erhöhen. |
| Dünner Streifen an fester Stelle, auch bei einfarbigen Bildern | Hardware- oder Kabelproblem des Displays; Kabel neu stecken, anderen Anschluss probieren. |
| Bildfehler, verschobene Zeilen | **Display → Beim Start zurücksetzen** an lassen, Display einmal ab- und anstecken. |
| Discord: „Bot-Token ungültig“ | Token im Developer Portal neu erzeugen und eintragen. |
| Discord: „… beide Intents einschalten“ | Developer Portal → Bot → Presence Intent und Server Members Intent einschalten. |
| Discord: „Der Bot ist auf keinem Server“ | Bot per OAuth2-Adresse einladen. |
| Discord-Liste leer | Freunde sind nicht auf einem Server mit dem Bot, oder die Personenauswahl passt nicht. |
| Battlefield: „Spieler nicht gefunden“ | Schreibweise prüfen (Null statt O), Konsolenspieler mit Plattform auswählen. Mit **Prüfen** testen. |
| Blogs: „Kein Feed gefunden“ | Statt der Blog-Adresse die Feed-Adresse eintragen (meist `…/feed/`). |
| Temperatur oder Lüfter „–“ | HWiNFO läuft nicht oder Shared Memory ist aus (Kapitel 5). |
| GPU „–“ | Keine Nvidia-Karte oder `nvidia-smi` fehlt (kommt mit dem Treiber). |
| Falscher Netzwerkadapter | **System → Netzwerkadapter** festlegen. |
| Einstellungsseite öffnet nicht | Läuft die App? Symbol im Infobereich prüfen; Port bei **Display → Programm** ändern, falls er belegt ist. |

---

## 9. Kommandozeile

`FuchsLabs-USB-Display.exe [Optionen]`

| Option | Wirkung |
|---|---|
| `-c DATEI` | andere Konfigurationsdatei verwenden |
| `--demo` | Beispieldaten aller Seiten zeigen (beendet sich danach) |
| `--no-lcd` | Display nicht ansteuern (z. B. zum Testen der Einstellungsseite) |
| `--preview ORDNER` | jedes Bild zusätzlich als `live.png` speichern |
| `--log DATEI` | Protokoll in eine bestimmte Datei schreiben |
| `-v` | ausführliches Protokoll |
| `--autostart` | still im Infobereich starten, keine Einstellungsseite öffnen (so trägt **Mit Windows starten** die App ein) |

---

## 10. Technischer Aufbau

| Datei | Aufgabe |
|---|---|
| `fuchslabs_display.py` | Hauptprogramm: Start, Einzelinstanz, Seitenrotation, Dienste, Neustart |
| `display_driver.py` | Display-Treiber (serieller Port, 6-Byte-Befehle, RGB565, Teil-Updates in 8-Zeilen-Bändern) |
| `render.py` | Zeichnet alle Seiten mit Pillow (Hell- und Dunkelmodus) |
| `brand.py` | FuchsLabs-CI: Farben (Fuchs-Orange #F77F00, Creme #FFF4E6, Navy #14213D als Schrift) und das Fuchs-Logo für Display, Infobereich und EXE. Schriften: Bahnschrift für Titel und Zahlen, Segoe UI für Text (beide bei Windows dabei) |
| `sysinfo.py` | Systemwerte: Windows-APIs per ctypes, `nvidia-smi`, HWiNFO-Shared-Memory |
| `discord_feed.py` | Discord-Bot (wird nur bei aktiver Seite geladen) |
| `bf6.py`, `rss.py` | Datenabruf Battlefield 6 und RSS/Atom |
| `friends.py` | Gemeinsame Freundesliste: Steam, Xbox (OpenXBL), Zusammenführen mit Discord |
| `appconfig.py`, `secure.py` | Konfiguration (Prüfung, Schreiben) und DPAPI-Verschlüsselung |
| `webui.py`, `webui.html` | Einstellungsseite (kleiner lokaler Webserver und eine HTML-Datei) |
| `tray.py`, `winsetup.py` | Symbol im Infobereich; Autostart und Umgang mit der Hersteller-App |

Ein Prozess, eine asyncio-Schleife: Anzeigeschleife, Datenabrufe, Discord und Webserver laufen darin; die Systemwerte misst ein Thread, das Symbol im Infobereich ein weiterer. **Neustart** nach dem Speichern: Die App beendet sich sauber (Anschluss frei), startet sich selbst neu und wartet dabei auf die Freigabe der Einzelinstanz.

Display-Protokoll: USB-CDC (seriell, 115200 Baud), Befehle mit gepackten 10-Bit-Koordinaten, Bilddaten als RGB565 little-endian; ein volles Bild braucht rund eine Sekunde. Helligkeitsbefehle während einer Bildübertragung liest das Display fälschlich als Bilddaten, deshalb gibt es kein Ab- und Aufblenden.

---

## 11. Selbst bauen

Voraussetzung: Python 3.11+ (64 Bit). Im Quellordner `build.bat` ausführen: legt eine eigene `.venv` an, installiert die festgelegten Pakete (`requirements.txt`, `requirements-build.txt`), erzeugt das Symbol und baut mit PyInstaller nach `dist\FuchsLabs-USB-Display\`. Der Build dauert etwa eine Minute. Den ganzen Ordner weitergeben (die EXE braucht `_internal`).

Zum Entwickeln ohne EXE: `.venv\Scripts\python fuchslabs_display.py --no-lcd` startet die App ohne Display.

---

## 12. Deinstallation

Autostart auf der Seite **Start** ausschalten, die App über das Symbol im Infobereich beenden, den Ordner löschen. Weitere Spuren gibt es nicht (das Symbol liegt temporär in `%TEMP%`). Die Hersteller-App lässt sich bei Bedarf in der Aufgabenplanung („UsbMonitor“) wieder aktivieren.
