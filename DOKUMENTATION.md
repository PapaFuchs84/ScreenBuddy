# ScreenBuddy – Dokumentation

**Companion-Vorschau ab 2.3.0:** USB/HDMI und Android gleichzeitig mit eigenen Seiten, Wechselintervallen und Touch-Zielen; automatische Handy-Auflösung einschließlich Rotation. Zusätzliche Einstellungen nur bei aktivem Companion. [Einrichtung und Grenzen](docs/COMPANION.md).

Version 2.7.0 · Stand 31.10.2026 · © 2026 FuchsLabs

Die App steuert ein 3,5"-USB-Display (480 × 320 Pixel, Hardware-„Revision A“) direkt an und ersetzt die Hersteller-App des Displays. Sie zeigt System, Freunde, Discord, Battlefield 6 und Blogs im Wechsel und verwendet ein natives Windows-Einstellungsfenster (Direct2D/DirectWrite, ohne WebView2 seit 2.5). Ab 2.6 gibt es den Plugin-Store in der App (Abschnitt unten). Neu in 2.7.0: mehrere Companions gleichzeitig, sortierbare Plugin-Seiten, Store-Vorschaubilder und die Plugin-Werkstatt ([Release-Notizen 2.7](docs/RELEASE-NOTES-2.7.md)). Stand und Grenzen von 2.6: [docs/SCREENBUDDY-2.6.md](docs/SCREENBUDDY-2.6.md). Für den schnellen Einstieg gibt es die `ANLEITUNG.md`; dieses Dokument ist die ausführliche Referenz.

## Companion-App für Android (Companion-Vorschau ab 2.3.0)

Die PC-Seite ist eingebaut; die Android-App gibt es als **Companion-App 1.0** als separaten Download (nicht Teil der Windows-EXE). Ein Handy oder Tablet im selben WLAN zeigt dann das Display (automatische Auflösung nach Meldung der App) und meldet Gesten/Tipps zurück.

- **Einschalten:** **Einstellungen → Companion → Companion zusätzlich aktivieren** und speichern (läuft dann zusätzlich zum USB-Display) oder unter **Anzeige → Display-Typ** „Netzwerk“ wählen (nur das Handy, kein USB-Display). Standard: aus. Windows fragt beim ersten Start eventuell nach der Firewall: „Private Netzwerke“ erlauben.
- **Koppeln:** **Kopplungscode anzeigen** zeigt QR-Code, 6-stellige PIN und den Fingerabdruck des Zertifikats. In der App den QR-Code scannen (oder Adresse und PIN eintippen und den Fingerabdruck vergleichen). Der Code gilt 2 Minuten und nur einmal; nach 5 falschen Versuchen ist das Koppeln 5 Minuten gesperrt.
- **Netz:** nur private IPv4-Adressen des PCs (10.x, 172.16–31.x, 192.168.x), Port 8777 (`[companion] port`, nie 8765), Verbindung mit TLS. Mehrere Handys: `[companion] sync` (Standard `true` = alle zeigen dasselbe; `false` = jedes Handy unabhängig mit eigener Position, Auflösung und optional eigener Seitenliste, siehe [Companion](docs/COMPANION.md)). Der PC kündigt sich per mDNS als `_fuchslabs-display._tcp` an (Windows 10 1809 oder neuer); sonst die IP-Adresse in der App eintragen.
- **Gespeichert:** `companion.json` neben der `config.toml` (TLS-Zertifikat, DPAPI-verschlüsselter TLS-Schlüssel, je Gerät ein DPAPI-verschlüsselter Schlüssel). Unter **Gekoppelte Geräte** lässt sich jedes Gerät entfernen; seine Verbindung wird sofort getrennt. Wer `companion.json` löscht, bekommt ein neues Zertifikat und muss alle Geräte neu koppeln.
- **Last:** Bilder gehen nur bei Änderung als JPEG raus, höchstens 5 pro Sekunde.

Protokoll: [docs/COMPANION-PROTOKOLL.md](docs/COMPANION-PROTOKOLL.md). Prüfstand: siehe [Testprotokoll](cpp/TESTPROTOKOLL.md). Bestätigt auf Redmi 8 per USB-Tethering (Gerätetest vom 27.10.2026 mit dem Vorgängerstand 0.2.1-preview; für 1.0 ist im Testprotokoll kein eigener Gerätetest dokumentiert). Offen: physischer USB-Ausgang gemeinsam mit Companion in derselben neuen Instanz, WLAN, Android-Boot und mehrere Geräte.

## Plugin-Store (ab 2.6.0)

Auf der Einstellungsseite **Plugin-Store** (Gruppe **APP**, neben Companion und Plugins) findest und installierst du geprüfte Datenplugins aus dem Store-Repository `PapaFuchs84/ScreenBuddy-Plugins`. Plugins enthalten keinen Programmcode.

- **Schalter:** „Plugin-Store verwenden“ (`[ui] plugin_store`, Standard an). Aus bedeutet: kein Abruf. Die Änderung wirkt nach **Speichern & anwenden**.
- **Suche und Kategorien:** Alle, System, Spiele, Smart Home, Info, Sonstiges. Ein Klick auf ein Plugin zeigt Autor, Version, Lizenz, Status und die Hosts, an die es Daten sendet.
- **Installieren und Aktualisieren** mit Bestätigung. Das Plugin bleibt **aus**; unter **Plugins** aktivierst du es und erteilst die Zustimmung je Host. Bei einem Update erscheinen neue Hosts auf der Karte, alle Freigaben werden zurückgesetzt.
- **Gesperrte Plugins** werden beim Abgleich automatisch ausgeschaltet.
- **Abgleich:** höchstens einmal täglich im Hintergrund (erster Versuch 30 s nach dem Start), manuell über **Store aktualisieren** (frühestens nach 60 s). Ohne Netz gilt der zuletzt geprüfte Zwischenspeicher.
- **Meldungen:** „nicht verfügbar“ heißt, dass in dieser Version kein Signaturschlüssel eingetragen ist. „nicht erreichbar“ heißt, dass kein gültiger Index geladen werden konnte und keine geprüften Daten vorliegen.
- **Sicherheit:** Index und Sperrliste sind Ed25519-signiert. Die App prüft Signatur, Rollback-Schutz (`sequence`) und die Prüfsumme des Pakets. Eine gültige Signatur ersetzt nicht die Prüfung eines Plugins. Details: [Plugin-Signierung](docs/PLUGIN-SIGNIERUNG.md).
- **Texte:** [Nutzungsbedingungen, Regeln und Datenschutz](docs/PLUGIN-STORE-BEDINGUNGEN.md) (Entwurf, juristisch nicht geprüft), [Autorenleitfaden](docs/PLUGIN-AUTOREN.md).

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick darauf öffnet die Einstellungen), und **Einstellungen → Updates & Autostart** zeigt „Neue Version verfügbar: …“. **Release-Seite öffnen** führt zur Download-Seite, **Jetzt nach Updates suchen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Updates & Autostart → Automatisch nach Updates suchen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Touch-Ziele und Spielstunden (ab 2.0.8)

Entwickelt als 2.0.7; diese Version wurde nicht einzeln veröffentlicht, die Funktion steht ab 2.0.8 zur Verfügung. Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](docs/TOUCH-2.0.7.md). [Prüfstand](cpp/TESTPROTOKOLL.md).

## Spotify (ab 2.8.0)

Zeigt den aktuellen Titel mit Cover; mit Touch steuern Tasten Zurück, Wiedergabe/Pause und Weiter (Spotify Premium nötig). Einrichtung: [docs/SPOTIFY.md](docs/SPOTIFY.md).

## Home Assistant (ab 2.1.0)

Neue Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. http://homeassistant.local:8123), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in config.toml. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit http:// laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort https://, wenn Home Assistant es anbietet.

Schritt für Schritt mit Bildern: bebilderte Anleitung, Schritt „Home Assistant“.

## Ansichten durchblättern (2.0.4)

Seit 2.0.4 lässt sich die laufende Anzeige per Knopfdruck weiterschalten, ohne zu speichern oder neu zu starten. In der nativen Oberfläche (ab 2.5.0) stehen dafür **Vorige Seite zeigen** und **Nächste Seite zeigen** unter **Einstellungen → Companion** bereit; sie wirken auf die Companion-Ausgabe. Für die Hauptanzeige gibt es den Schritt nur über die lokale Schnittstelle (`POST /api/pages/step`). Die früheren Knöpfe „Vorherige Ansicht“/„Nächste Ansicht“ unter **Übersicht** sind in der nativen Oberfläche nicht vorhanden. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

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

Die Anzeige wechselt alle `page_seconds` Sekunden (Standard 15). Reihenfolge und Sichtbarkeit stellst du in der Einstellungsseite ein (**Seiten & Reihenfolge**; aktivierte Plugins stehen dort ebenfalls in der Liste und lassen sich sortieren und ausblenden).

| Seite | Inhalt | Aktualisierung |
|---|---|---|
| **System** | CPU (Last, Takt, Temperatur, Lüfter), GPU (Last, Temperatur, Lüfter, VRAM), RAM, Datenträger, Netzwerkraten mit Verlauf | etwa jede Sekunde, solange sichtbar |
| **Freunde (alle Plattformen)** | eine gemeinsame Liste aus Discord, Steam, Xbox und PlayStation; dieselbe Person nur einmal, mit Marken D/S/X/P für ihre Plattformen | Discord sofort, Steam jede Minute, PlayStation alle 2 Minuten, Xbox alle 3 Minuten |
| **Discord-Freunde** | nur die Discord-Freunde, mit Avatar, Status und Aktivität („Spielt …“) | sofort bei Änderungen |
| **Battlefield 6 · Rangliste** | Profile der eingetragenen Spieler nach Rang, mit Abzeichen, K/D, Siegquote | alle 15 Minuten |
| **Blog-Updates** | neuester Beitrag je eingetragenem Blog | alle 30 Minuten |
| **Streaming-Neustarts** | neue Filme und Serien der letzten 30 Tage bei den gewählten Anbietern (TMDB, Anbieterdaten JustWatch) | stündlich |
| **Home Assistant** | bis zu 4 Sensoren oder Schalter (Schalter nur mit Touch) | alle 15 Sekunden |

Ist keine Seite aktiv, zeigt das Display einen Willkommenshinweis mit der Adresse der Einstellungsseite. Fehler (z. B. ungültiger Discord-Token) erscheinen als lesbarer Satz direkt auf der betroffenen Seite.

**Details der Seiten**

- *System:* Bei aktivem Touch öffnet ein Tipp auf RAM den Task-Manager, auf Datenträger „Programme und Features“ und auf Netzwerk speedtest.net/de im Browser (ab 2.2.0). CPU-Last wie im Task-Manager („% Processor Utility“), Takt aus der Leistungsanzeige. Die GPU-Werte kommen über `nvidia-smi` (Nvidia-Treiber). Netzwerkraten stammen aus den 64-Bit-Zählern des Adapters, über den dein Internet läuft (automatisch gewählt), und stimmen mit Windows überein. Temperaturen ab 75 °C werden gelb, ab 85 °C rot.
- *Discord:* sortiert nach Status, dann Name. Farbiger Punkt am Avatar = Status, Handy-Symbol = am Smartphone. Emojis werden ausgeblendet (die Windows-Schriften zeigen sie sonst als Kästchen).
- *Freunde (alle Plattformen):* Quellen sind Discord (Bot, siehe unten), Steam (offizielle Web-API von Valve), Xbox (Dienst [OpenXBL](https://xbl.io)) und PlayStation (inoffizielle Sony-Schnittstelle, NPSSO-Code, nur lesend, Abruf alle 2 Minuten; Marke P). Gleiche Personen werden zusammengefasst: manuell über **Freunde → Personen zusammenführen** oder automatisch bei gleichem Namen (abschaltbar; nie werden zwei Konten derselben Plattform vermischt). Online hat Vorrang, das Spiel kommt von dem Konto, das gerade spielt. Unter jedem Zugangsfeld (auch Discord, TMDB, Spotify, Home Assistant) erklärt eine Karte „So bekommst du …“ die Schritte und öffnet per Knopf die offizielle Seite. Für **Steam** brauchst du einen kostenlosen Schlüssel von <https://steamcommunity.com/dev/apikey> und die Freundesliste im Steam-Profil muss öffentlich sein. Für **Xbox** brauchst du einen OpenXBL-Schlüssel aus dem Dashboard auf xbl.io (kostenlose Stufe, rund 150 Abrufe pro Stunde; die App fragt nur alle 3 Minuten). **PlayStation** ist optional und standardmäßig aus (ohne gespeicherten NPSSO-Code gibt es keine Anfragen): Es gibt keine offizielle Schnittstelle für Freundeslisten, der inoffizielle Zugang geschieht auf eigenes Risiko und kann zur Kontoeinschränkung führen. **EA** ist bewusst nicht eingebaut (keine offizielle Schnittstelle). Die Schlüssel werden wie der Discord-Token verschlüsselt gespeichert, und **Verbindung prüfen** zeigt sofort, ob sie funktionieren. Damit niemand doppelt erscheint, entfällt die reine Seite „Discord-Freunde“ automatisch, solange die gemeinsame Liste Discord einbezieht. Bei Konten mit ähnlichen, aber nicht gleichen Namen (z. B. „Basti“ und „Bastian“) schlägt die Einstellungsseite unter **Personen zusammenführen** eine Verknüpfung vor, die du mit einem Klick bestätigst oder ablehnst. Für Version 2.0 liegen Prüfungen unter `cpp/tests/tests.cpp` vor (u. a. gleichzeitig online auf allen Plattformen = genau eine Zeile mit allen Marken).
- *Battlefield 6:* sortiert nach Rang, bei Gleichstand nach K/D. Fällt die Datenquelle kurz aus, bleiben die letzten Werte stehen.
- *Streaming-Neustarts:* Details in [docs/FUNKTIONEN.md](docs/FUNKTIONEN.md#streaming-neustarts-noch-nicht-veröffentlicht). Ohne Key steht „Kein Key“, bei Fehler oder offline „Keine Daten“. This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.
- *Blogs:* Titel laufen über höchstens zwei Zeilen, Rubrik-Präfixe wie „Anzeige:“ entfallen.

---

## 2. Installation und erster Start

Ab Version 2.5: Windows 10/11 x64 und angeschlossenes Display. Python, Visual-C++-Redistributable und WebView2 werden nicht benötigt. Die frühere Version 2.3.0 braucht noch die Microsoft Edge WebView2 Evergreen Runtime, die nicht Teil der EXE ist.

Die [Release-EXE](https://github.com/PapaFuchs84/ScreenBuddy/releases/latest) enthält App, Installer, Updater und Deinstaller. Der Installer verwendet `%LOCALAPPDATA%\FuchsLabs\USB-Display`, legt Verknüpfung und Deinstallationseintrag an und kann Einstellungen aus dem bisherigen Benutzerordner oder dem alten Roaming-AppData-Ordner übernehmen. Eine bestehende Zielkonfiguration bleibt erhalten; der alte Versionsordner wird bei der Übernahme mit `-1.x-alt` umbenannt. Ab 2.0.6 legt der Installer außerdem `LICENSE.txt` (MIT) und `THIRD-PARTY-NOTICES.md` (Lizenzen der mitgelieferten Bibliotheken) in den Installationsordner.

Das eigene Windows-Einstellungsfenster öffnet sich bei noch offener Ersteinrichtung. Später öffnet das Fuchs-Symbol oder ein erneuter Programmstart dieses Fenster. Schließen blendet es aus; **Beenden** im Infobereich stoppt die App. Beim Windows-Autostart bleibt das Fenster geschlossen.

Ausführliche Einrichtung der Datenquellen: [Schnellstart](ANLEITUNG.md).

## 3. Einstellungsseite

Sie erscheint in einem nativen Win32-Fenster (Direct2D/DirectWrite, ohne WebView2 seit 2.5). Der interne Dienst läuft in der App und ist nur auf deinem PC erreichbar: `http://127.0.0.1:8765/` (Port einstellbar über `[ui] port` in der `config.toml`); die bebilderte Anleitung liegt unter `/anleitung`, die Plugin-Werkstatt unter `/plugin-werkstatt`. Änderungen werden erst mit **Speichern & anwenden** übernommen; die Anzeige startet dann kurz neu.

| Bereich | Einstellungen |
|---|---|
| **Updates & Autostart** | installierte Version mit Update-Prüfung (**Automatisch nach Updates suchen**, **Jetzt nach Updates suchen**, **Release-Seite öffnen**), **Mit Windows starten**, Hersteller-App beenden/Autostart abschalten, **Neu starten**, **Beenden**. Sprache und Hell/Dunkel des Fensters stehen oben rechts. |
| **Übersicht** | Kennzahlen (Display verbunden, Auflösung, aktive Quellen, Seiten), Verbindungsstatus von Discord, Steam, Xbox und Home Assistant, Schnellzugriff, „Einrichtung in 3 Schritten“ |
| **Seiten & Reihenfolge** | Seiten (auch aktivierte Plugins) ein-/ausblenden und mit ▲/▼ sortieren |
| **Farben & Design** | Hell, Dunkel, „An Windows anpassen“, eigene Themes (Editor, Import/Export) oder signierte Themes direkt aus dem Store (Karte „Themes aus dem Store“), mit Vorschau; siehe [docs/THEMES.md](docs/THEMES.md) |
| **Freunde** | gemeinsame Liste: Seite an/aus, Discord einbeziehen, Steam- und Xbox-Zugang mit „Verbindung prüfen“ und Anleitung, Personen zusammenführen |
| **Discord** | Seite an/aus, Bot-Token, Offline-Freunde, Auswahl der Personen aus den Server-Mitgliedern, Server- und Ignorierlisten |
| **Battlefield 6** | Spieler hinzufügen (PC/PlayStation/Xbox), sofort **Prüfen**, Abrufintervall |
| **Blogs** | Blog-Adressen hinzufügen und mit **Prüfen** testen, Anzahl Beiträge, Intervall |
| **Streaming-Neustarts** | Seite an/aus, TMDB-Key (maskiert), Region, Anbieter-Mehrfachauswahl, Anzahl Einträge |
| **System** | Netzwerkadapter, Einheit (Mbit/s oder MB/s), Laufwerk, Sensoren für CPU-Temperatur und Lüfter |
| **Companion, Plugins, Plugin-Store** | Companion (Kopplung, Geräte), Plugins und Plugin-Store (Schalter „Plugin-Store verwenden“, Suche, Installation, Abgleich) |
| **Anzeige** | Display-Typ (USB, Monitor, Netzwerk), Anzeigedauer pro Seite, Helligkeit, Ausrichtung, COM-Port, Bildschirm, Touch-Steuerung und Touch-Zuordnung, Zurücksetzen beim Start, Ausschalten beim Beenden |

**Verbindungsstatus:** Auf den Seiten Discord und Freunde zeigt eine Statuszeile dauerhaft, ob der Zugang funktioniert: grün mit Kontoname, Anzahl der Freunde und Zeitpunkt der letzten erfolgreichen Abfrage (Discord: live), rot mit der Fehlermeldung, wenn etwas nicht klappt (bei Steam/Xbox mit dem Zeitpunkt des letzten Erfolgs), grau, wenn nichts eingerichtet ist. Sie aktualisiert sich alle 5 Sekunden von selbst.

Der Bot-Token wird nie an den Browser zurückgegeben; das Feld zeigt nur, ob einer gespeichert ist. Leer lassen = behalten.

**HDMI/USB-C und Touch ab 2.0.3:** Unter **Anzeige** den vorgesehenen Bildschirm auswählen; die Vollbildausgabe bleibt über der Taskleiste, ohne den Tastaturfokus zu übernehmen. Die Touch-Zuordnung zeigt, welchem Bildschirm Windows ein Touchgerät zugeordnet hat. Erscheinen Berührungen auf einem anderen Monitor, **Touch-Bildschirm zuordnen** wählen. Im Windows-Assistenten andere Bildschirme mit Enter überspringen, dann den gewünschten Touchscreen berühren; Esc bricht ab. Die Vollbildausgabe pausiert währenddessen und kehrt nach Ende des Assistenten zurück. Anschließend **Erneut prüfen** wählen und bei Bedarf die Touch-Steuerung aktivieren und speichern. HDMI überträgt nur das Bild; für Touch muss zusätzlich die USB-Verbindung des Geräts bestehen. Echte Touch-Hardware und der vollständige Assistentenablauf sind noch nicht physisch abgenommen; siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

---

## 4. Konfigurationsdatei

Bei regulärer Installation liegt `config.toml` unter `%LOCALAPPDATA%\FuchsLabs\USB-Display` neben der EXE und wird von der Einstellungsseite geschrieben (Sicherungskopie: `config.toml.bak`). Von Hand bearbeiten ist möglich; danach die App neu starten. Fehlende Einträge bekommen Standardwerte, ungültige Werte werden auf erlaubte Bereiche begrenzt.

| Abschnitt | Schlüssel (Standard) |
|---|---|
| `[discord]` | `enabled` (true), `token` (verschlüsselt), `guild_ids` ([]), `friend_ids` ([]), `ignore_ids` ([]), `show_offline` (false) |
| `[display]` | `port` ("AUTO"), `orientation` ("landscape"; auch `reverse_landscape`, `portrait`, `reverse_portrait`), `brightness` (50), `reset_on_start` (true), `page_seconds` (15), `pages` (["system","friends","discord","bf6","rss","streaming","homeassistant"]), `title`, `screen_off_on_exit` (true), `theme` ("hell"; "dunkel", "windows" = Farben von Windows, "custom:<id>" = eigenes Theme aus `themes\<id>.json` im Datenordner; "eigen" = ältere eigene Farben; gilt für Display, Einstellungsseite und Anleitung), `color_base` ("hell"/"dunkel", Vorlage für eigene Farben), `color_bg`, `color_card`, `color_header`, `color_accent`, `color_text`, `color_muted` ("#RRGGBB", leer = Vorlage), `output` ("serial" = 3,5"-USB-Display, "monitor" = HDMI/USB-C-Bildschirm als Vollbildfenster), `monitor` ("" = erster Nicht-Hauptbildschirm, sonst z. B. `\\.\DISPLAY2`), `touch` (true; Seitenwechsel per Wischen/Tippen, Tippen öffnet Blogbeiträge, BF6-Profile oder Freundes-Chat/Profil gemäß der Anleitung für 2.0.7; nur bei `output = "monitor"`) |
| `[system]` | `enabled` (true), `net_adapter` ("" = automatisch), `net_unit` ("bit" oder "byte"), `disk` ("C:"), `cpu_temp_sensor`, `cpu_fan_sensor` ("" = automatisch), `title` |
| `[friends]` | `enabled` (false), `title` ("Freunde"), `include_discord` (true), `auto_merge` (true), `show_offline` (false), `steam_api_key` (verschlüsselt), `steam_id` (Profil-Link oder 17-stellige ID), `xbox_api_key` (verschlüsselt), `links` ([], Verknüpfungen als `"Name\|discord:ID\|steam:ID\|xbox:Gamertag"`) |
| `[bf6]` | `enabled` (false), `players` ([], PC-Namen; Konsole als `"Name@psn"` / `"Name@xbox"`), `refresh_minutes` (15), `title` |
| `[rss]` | `enabled` (false), `feeds` ([]), `refresh_minutes` (30), `max_items` (4), `max_per_feed` (1), `title` |
| `[streaming]` | `enabled` (false), `title` ("Neu bei Streaming"), `api_key` (verschlüsselt; v3-Key oder Read Access Token), `region` ("" = DE bei Deutsch, US bei Englisch; sonst zwei Buchstaben), `providers` (Zeilen `"ID\|Name"` mit TMDB-Provider-IDs, höchstens 30; Standard: Netflix 8, Prime Video 9 und 119, Disney+ 337, Apple TV+ 350, WOW 30, RTL+ 2750), `max_items` (5, 1–6), `refresh_minutes` (60, mindestens 60) |
| `[homeassistant]` | `enabled` (false), `title` ("Home Assistant"), `url` (z. B. "http://homeassistant.local:8123"), `token` (verschlüsselt), `entities` ([], höchstens 4, z. B. `"sensor.wohnzimmer_temperatur"`) |
| `[spotify]` | `enabled` (false), `title` ("Spotify"), `amp` (true; Amplifier-Balken aus der Systemausgabe im Querformat; Analyse bis 3 s nach der letzten sichtbaren Wiedergabeseite), `client_id` (32 Zeichen aus deiner Spotify-Developer-App; Anmeldung und Tokens siehe [docs/SPOTIFY.md](docs/SPOTIFY.md), Tokens liegen in `spotify.token`, nicht in der Konfiguration) |
| `[ui]` | `enabled` (true), `port` (8765), `setup_done` (false), `language` ("de" oder "en"; beim ersten Start wie die Windows-Sprache – gilt für Einstellungsseite, Display und Infobereich), `update_check` (true; höchstens einmal am Tag die GitHub-Releases abfragen und eine neuere Version melden, nie automatisch installieren), `plugin_store` (true; Plugin-Store höchstens einmal täglich abrufen, false = kein Abruf) |

---

## 5. HWiNFO für CPU-Temperatur und Lüfter

Windows stellt diese Werte nicht ohne Hilfsprogramm bereit. Die App liest sie aus dem Shared Memory des kostenlosen [HWiNFO64](https://www.hwinfo.com/). Ohne HWiNFO zeigt die Systemseite an diesen Stellen „–“; alles andere funktioniert.

`hwinfo-einrichten.bat` (Rechtsklick → **Als Administrator ausführen**) stellt HWiNFO sparsam ein (nur Sensormodul, Messung alle 2 Sekunden, minimiert, keine Update-Prüfung, Shared Memory an) und richtet die Aufgabe „FuchsLabs HWiNFO“ ein. Sie startet HWiNFO nach der Anmeldung unsichtbar und alle 11 Stunden neu, weil die kostenlose Version die Freigabe nach 12 Stunden abschaltet. Der Pfad `C:\Program Files\HWiNFO64\` ist in den Dateien festgelegt; bei abweichender Installation dort anpassen. Welche Sensoren verwendet werden, wählst du in der Einstellungsseite unter **System**.

---

## 6. Sicherheit und Datenschutz

Die App speichert Discord-, Steam-, Xbox-, PlayStation- und Home-Assistant-Zugangsdaten per Windows-DPAPI verschlüsselt. Das schützt die Datei nicht vor Prozessen im selben Windows-Benutzerkonto. Niemals Konfigurationen mit echten Zugangsdaten veröffentlichen.

Der lokale Einstellungsdienst bindet an `127.0.0.1`, prüft Host und Fetch-Metadaten und verlangt für Schreibzugriffe `X-FL: 1`. RSS-Ziele und Weiterleitungen werden gegen nichtöffentliche IP-Bereiche geprüft. Die C++-Implementierung verwendet unter anderem WinHTTP, D++/OpenSSL und pugixml; Details stehen in `cpp/src/http.cpp`, `netguard.cpp`, `secure.cpp` und `webui.cpp`.

Abhängigkeiten sind über vcpkg-Manifest und D++-Overlay festgelegt; WebView2-SDK-Lizenzhinweise liegen unter `cpp/third_party/webview2`. Seit 2.5 braucht die Einstellungsseite keine WebView2-Laufzeit mehr. Die Windows-EXE ist nicht signiert; der Plugin-Store prüft Plugin-Daten über eine Ed25519-Signatur (siehe oben).

[SICHERHEITSREVIEW.md](SICHERHEITSREVIEW.md) dokumentiert den historischen Python-Stand und ist kein vollständiger C++-Audit. Sicherheitsmeldungen: [SECURITY.md](SECURITY.md).

## 7. Effizienz

Die C++-Version verwendet Windows-Grafik-APIs und statisch gelinkte Bibliotheken. Bis 2.3.0 erzeugte WebView2 für das Einstellungsfenster zusätzliche Laufzeitprozesse; seit 2.5.0 entfällt das. Alte Python-Messwerte und ursprüngliche C++-Schätzungen sind kein belegter Ressourcenverbrauch der 2.0-Version. Einen reproduzierbaren CPU-/Speichervergleich gibt es bisher nicht.

Bestätigte Funktionsprüfungen: [cpp/TESTPROTOKOLL.md](cpp/TESTPROTOKOLL.md).

## 8. Fehlerbehebung

Das Protokoll steht in `usb-display.log` neben der EXE. Die meisten Probleme erklärt die Seite **Updates & Autostart** bzw. **Übersicht** direkt.

| Symptom | Ursache und Lösung |
|---|---|
| „Die Hersteller-App des Displays läuft …“ | Unter **Updates & Autostart** → **Hersteller-App beenden**, danach **Autostart der Hersteller-App abschalten**. |
| „Display nicht gefunden“ | Kabel prüfen, anderen USB-Anschluss probieren; notfalls unter **Anzeige** den COM-Port von Hand eintragen (Geräte-Manager → Anschlüsse). Die App versucht es alle 10 Sekunden erneut. |
| Bild steht auf dem Kopf | **Anzeige → Ausrichtung** auf die gedrehte Variante (Quer 180°) stellen. |
| Taskleiste liegt über der HDMI-/USB-C-Anzeige, Tippen unten öffnet das Startmenü | Auf 2.0.5 aktualisieren: Solange die Anzeige läuft, wird die Taskleiste auf diesem Bildschirm ausgeblendet und beim Beenden wieder gezeigt. |
| Wischen auf dem Touchscreen wechselt keine Seite | **Anzeige → Touch-Steuerung** einschalten und speichern. Ab 2.0.5 steht jede erkannte Geste im Protokoll `usb-display.log`; ein Hinweis erscheint dort auch, wenn Touch ausgeschaltet ist. |
| Berührungen erscheinen auf einem anderen Bildschirm | **Anzeige → Touch-Bildschirm zuordnen**; im Windows-Assistenten den vorgesehenen Touchscreen identifizieren. Danach **Erneut prüfen** wählen. |
| Kein Touchscreen erkannt | USB-Touchverbindung und Windows-Treiber prüfen, dann **Erneut prüfen**. Das HDMI-Bild allein stellt keinen Touch-Eingang bereit. |
| Display zu dunkel | **Anzeige → Helligkeit** erhöhen. |
| Dünner Streifen an fester Stelle, auch bei einfarbigen Bildern | Hardware- oder Kabelproblem des Displays; Kabel neu stecken, anderen Anschluss probieren. |
| Bildfehler, verschobene Zeilen | **Anzeige → Beim Start zurücksetzen** an lassen, Display einmal ab- und anstecken. |
| Discord: „Bot-Token ungültig“ | Token im Developer Portal neu erzeugen und eintragen. |
| Discord: „… beide Intents einschalten“ | Developer Portal → Bot → Presence Intent und Server Members Intent einschalten. |
| Discord: „Der Bot ist auf keinem Server“ | Bot per OAuth2-Adresse einladen. |
| Discord-Liste leer | Freunde sind nicht auf einem Server mit dem Bot, oder die Personenauswahl passt nicht. |
| Battlefield: „Spieler nicht gefunden“ | Schreibweise prüfen (Null statt O), Konsolenspieler mit Plattform auswählen. Mit **Prüfen** testen. |
| Streaming: „Kein Key“ / „Keine Daten“ | TMDB-Key unter **Streaming-Neustarts** eintragen und **Key prüfen & Anbieter laden** wählen; bei „Keine Daten“ Internet, Key und Anbieterauswahl prüfen. |
| Blogs: „Kein Feed gefunden“ | Statt der Blog-Adresse die Feed-Adresse eintragen (meist `…/feed/`). |
| Temperatur oder Lüfter „–“ | HWiNFO läuft nicht oder Shared Memory ist aus (Kapitel 5). |
| GPU „–“ | Keine Nvidia-Karte oder `nvidia-smi` fehlt (kommt mit dem Treiber). |
| Falscher Netzwerkadapter | **System → Netzwerkadapter** festlegen. |
| Einstellungsfenster öffnet nicht | App-Prozess und `usb-display.log` kontrollieren (ab 2.5 ist keine WebView2-Laufzeit nötig). Bei belegtem Port den Port in der Konfiguration ändern. |
| Plugin-Store „nicht erreichbar“ | Netzverbindung prüfen, dann **Store aktualisieren**. Ohne Netz zeigt die Seite den zuletzt geprüften Stand. |
| Plugin-Store „nicht verfügbar“ | Diese Version hat keinen Signaturschlüssel eingetragen. Der Store ist dann nicht nutzbar. |
| Installiertes Plugin erscheint nicht als aktiv | Es bleibt nach der Installation **aus**. Unter **Plugins** aktivieren und die Zustimmung je Host erteilen. |
| Nach Windows-Anmeldung kein Tray und nur altes Displaybild | Prüfen, ob der Autostart auf eine tatsächlich vorhandene EXE zeigt. Bei Installation aus einem paketierten Werkzeug Pfad und Run-Eintrag außerhalb dessen Paketkontexts prüfen; siehe Kapitel 13. |

---

## 9. Kommandozeile

| Option | Wirkung |
|---|---|
| `--install`, `--silent` | integrierten Installer starten, optional ohne Dialog |
| `--uninstall` | integrierten Deinstaller starten |
| `--run` | App statt Installer starten (für Entwicklung außerhalb des Installationsorts) |
| `-c DATEI`, `--config DATEI` | eigene Konfiguration |
| `--no-lcd` | ohne Displayausgabe |
| `--demo` | Beispieldaten rendern |
| `--preview ORDNER` | PNG-Vorschauen schreiben |
| `--log DATEI`, `-v` | Logpfad und ausführliche Protokollierung |
| `--autostart` | still im Infobereich starten |

Beispiel ohne Hardware: `build\ScreenBuddy.exe --run --no-lcd -c test.toml`. Für parallele Entwicklung ist zusätzlich ein eigener UI-Port und der nur für Tests gedachte Instanzname über `FUCHSLABS_INSTANCE` nötig.

## 10. Technischer Aufbau

Die aktuelle Implementierung ist C++20 unter `cpp/`.

| Datei unter `cpp/src/` | Aufgabe |
|---|---|
| `main.cpp`, `app.cpp` | Start, Einzelinstanz, Dienste und Seitenrotation |
| `native_settings.cpp`, `ui_page_*.cpp`, `tray.cpp` | natives Einstellungsfenster (Direct2D/DirectWrite) und Infobereich |
| `plugin_sign.cpp`, `plugin_store.cpp`, `plugin_catalog.cpp`, `ui_page_store.cpp` | Plugin-Signierung, Plugin-Speicher, Store-Logik und Store-Seite |
| `webui.cpp`, `config.cpp`, `secure.cpp` | lokaler Einstellungsdienst, TOML und DPAPI |
| `ha.cpp` | Home-Assistant-REST-Abfrage und Schalten |
| `serial_display.cpp`, `monitor_display.cpp`, `network_display.cpp` | USB-Display, HDMI/USB-C und Companion (Android) |
| `companion.cpp`, `companion_server.cpp` | Companion-Protokoll, TLS-Server, mDNS, QR/PIN-Kopplung |
| `gfx.cpp`, `render.cpp` | Grafik und Seitenlayout |
| `discord.cpp`, `friends.cpp`, `bf6.cpp`, `rss.cpp`, `tmdb.cpp` (+ `tmdb_logic.cpp`), `sysinfo.cpp` | Datenquellen; Discord über D++ |
| `installer.cpp`, `winsetup.cpp` | Installation, Updates, Deinstallation und Autostart |
| `update.cpp` | GitHub-Release-Abfrage und Update-Hinweise |

Einstellungen und Anleitungen unter `cpp/res/` sind als Ressourcen in die EXE eingebettet. Die Python-Dateien im Repository-Hauptordner bleiben der historische Stand 1.x.

## 11. Selbst bauen

Siehe [C++-Buildanleitung](cpp/README.md#build). Im Checkout **in den Ordner `cpp` wechseln**, dann `build.bat nopause` ausführen. Die `build.bat` im Hauptordner baut weiterhin den alten Python-Stand.

Der Build benötigt Visual Studio Build Tools 2022, Windows SDK, CMake/Ninja und vcpkg unter `%USERPROFILE%\FuchsLabs\vcpkg`. Ergebnis: `cpp\dist\ScreenBuddy.exe`.

## 12. Deinstallation

In Windows **Einstellungen → Apps → ScreenBuddy → Deinstallieren** (bis 2.3.0: FuchsLabs USB-Display). Der integrierte Deinstaller entfernt Autostart und Verknüpfung. Der Dialog bietet zusätzlich an, Einstellungen und gespeicherte Zugangsdaten zu entfernen. Ein aufbewahrter `-1.x-alt`-Ordner gehört zur alten Version und ist ein eigener Rückfallstand.

## 13. Prüfstand und Windows-Neustart

**2.6.0:** Plugin-Store mit signiertem Index, Sperrliste und acht Start-Plugins; Companion-App 1.0 als separater Download. Store-Installation und -Update in einer isolierten Instanz: Installation des Plugins „Wetter“ aus dem Live-Store bestanden (Plugin bleibt aus, kein Host zugestimmt); Schalter „Plugin-Store verwenden“ speichern und Neustart bestanden; Update nur als Simulation (Version lokal auf 0.9.0 gesetzt) bestanden, ein echtes Live-Update war mangels zweiter Version nicht prüfbar; Suchfeld nicht automatisiert prüfbar. Dabei gefundene Fehler (Plugin-Dateien unter GitHub Pages nicht ausgeliefert, Schalter nicht speicherbar) sind behoben.. Automatisierte Prüfungen: 1.312 Windows-Prüfungen und 2 Installer-Prüfungen bestanden, Android-Protokolltests bestanden. Details und Grenzen: [Release-Notizen 2.6.0](docs/RELEASE-NOTES-2.6.md).

**2.3.0:** Companion-Vorschau: unabhängige Anzeigen (USB/HDMI und Android gleichzeitig), automatische Auflösung und Drehung, Krypto/TLS-Härtung und Einstellungs-Migration/Sicherung. Release-Build, 236 Kernprüfungen, 11 Monitorprüfungen, 335 Companion-Prüfungen, 39 App-Integrationsprüfungen und 2 Installer-Sicherungsprüfungen bestanden. Bestätigt auf Redmi 8 per USB-Tethering. Offen: physischer USB-Ausgang gemeinsam mit Companion in derselben neuen Instanz, WLAN, Android-Boot und mehrere reale Geräte.

**2.2.0:** Release-Build, 225 Kernprüfungen und 11 Monitor-Basisprüfungen bestanden (neu: Trefferflächen der Systemseite, Versionsvergleich, Auswertung der GitHub-Antwort; die Trefferflächen wurden gegen das gerenderte Vorschaubild geprüft). Offen: Tippen auf echter Touch-Hardware, Sprechblase und Einstellungsanzeige mit einer tatsächlich neueren Veröffentlichung, Windows-Anmeldetest für 2.2.0.

**2.0.3:** Release-Build, 86 Kernprüfungen und 56 Monitor-/Touchprüfungen einschließlich 11 Basisprüfungen bestanden. Die 2.0.3-Installation wurde auf Datei-Version, Hash, API-Version, Autostart-Eintrag und `display_ok=true` geprüft. Touch wurde mit injizierten Windows-Eingaben getestet; echte Touch-Hardware, der vollständige Zuordnungsassistent und ein neuer Windows-Anmeldetest bleiben offen. Details im [Testprotokoll](cpp/TESTPROTOKOLL.md).

**Frühere Abnahme von 2.0:**

Hardware-Gegenvergleich am 22.10.2026 bestanden: 1.x und 2.0 meldeten an COM7 `display_ok=true`, der Nutzer bestätigte die physische Anzeige. Das native Einstellungsfenster wurde sichtbar geöffnet.

Der anschließende Windows-Neustarttest schlug fehl: kein Tray-Prozess, altes Displaybild blieb stehen. Eine Diagnose außerhalb des Codex-Paketkontexts zeigte, dass die neue Installation nur in dessen Windows-Paketumleitung sichtbar war. Windows selbst sah am vorgesehenen Ziel keine EXE und noch den alten Run-Eintrag. Installation und Autostart wurden deshalb außerhalb dieses Kontexts eingerichtet. Am 22.10.2026 bestätigte der Nutzer den erfolgreichen erneuten Windows-Neustart mit automatisch erscheinendem Tray-Symbol.
