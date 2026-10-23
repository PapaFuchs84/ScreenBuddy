# FuchsLabs USB-Display – Dokumentation

Version 2.0.4 · Stand 22.10.2026 · © 2026 FuchsLabs

Die App steuert ein 3,5"-USB-Display (480 × 320 Pixel, „Revision A“) direkt an und ersetzt die Hersteller-App des Displays. Sie zeigt System, Freunde, Discord, Battlefield 6 und Blogs im Wechsel und verwendet ein eigenes Windows-Einstellungsfenster mit WebView2. Für den schnellen Einstieg gibt es die `ANLEITUNG.md`; dieses Dokument ist die ausführliche Referenz.

## Ansichten durchblättern (2.0.4)

Unter **Einstellungen → Übersicht** kannst du mit **Vorherige Ansicht** und **Nächste Ansicht** direkt durch die laufenden Display-Ansichten blättern. Die Position und die sichtbaren Seiten stehen unter der Live-Vorschau. Lange Listen blättern mit ihren Unterseiten, breite Displays wechseln wie bei der Touchbedienung um eine Kachelgruppe. Nach einem manuellen Wechsel beginnt die eingestellte Anzeigedauer neu; danach läuft der automatische Wechsel weiter. Bei keiner oder nur einer vollständig sichtbaren Ansicht sind die Knöpfe deaktiviert. Dafür ist kein Speichern oder Neustart nötig. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

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
- *Freunde (alle Plattformen):* Quellen sind Discord (Bot, siehe unten), Steam (offizielle Web-API von Valve) und Xbox (Dienst [OpenXBL](https://xbl.io)). Gleiche Personen werden zusammengefasst: manuell über **Freunde → Personen zusammenführen** oder automatisch bei gleichem Namen (abschaltbar; nie werden zwei Konten derselben Plattform vermischt). Online hat Vorrang, das Spiel kommt von dem Konto, das gerade spielt. Für **Steam** brauchst du einen kostenlosen Schlüssel von <https://steamcommunity.com/dev/apikey> und die Freundesliste im Steam-Profil muss öffentlich sein. Für **Xbox** brauchst du einen OpenXBL-Schlüssel aus dem Dashboard auf xbl.io (kostenlose Stufe, rund 150 Abrufe pro Stunde; die App fragt nur alle 3 Minuten). **PlayStation** und **EA** sind bewusst nicht eingebaut: Es gibt keine offiziellen Schnittstellen für Freundeslisten; der inoffizielle PlayStation-Zugang kann zur Kontosperre führen. Die Schlüssel werden wie der Discord-Token verschlüsselt gespeichert, und **Verbindung prüfen** zeigt sofort, ob sie funktionieren. Damit niemand doppelt erscheint, entfällt die reine Seite „Discord-Freunde“ automatisch, solange die gemeinsame Liste Discord einbezieht. Bei Konten mit ähnlichen, aber nicht gleichen Namen (z. B. „Basti“ und „Bastian“) schlägt die Einstellungsseite unter **Personen zusammenführen** eine Verknüpfung vor, die du mit einem Klick bestätigst oder ablehnst. Für Version 2.0 liegen Prüfungen unter `cpp/tests/tests.cpp` vor (u. a. gleichzeitig online auf allen Plattformen = genau eine Zeile mit allen Marken).
- *Battlefield 6:* sortiert nach Rang, bei Gleichstand nach K/D. Fällt die Datenquelle kurz aus, bleiben die letzten Werte stehen.
- *Blogs:* Titel laufen über höchstens zwei Zeilen, Rubrik-Präfixe wie „Anzeige:“ entfallen.

---

## 2. Installation und erster Start

Version 2.0: Windows 10/11 x64, angeschlossenes Display und installierte Microsoft Edge WebView2 Evergreen Runtime. Python und Visual-C++-Redistributable werden nicht benötigt. Die WebView2-Laufzeit ist nicht Teil der einzelnen EXE.

Die [Release-EXE](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/latest) enthält App, Installer, Updater und Deinstaller. Der Installer verwendet `%LOCALAPPDATA%\FuchsLabs\USB-Display`, legt Verknüpfung und Deinstallationseintrag an und kann Einstellungen aus dem bisherigen Benutzerordner oder dem alten Roaming-AppData-Ordner übernehmen. Eine bestehende Zielkonfiguration bleibt erhalten; der alte Versionsordner wird bei der Übernahme mit `-1.x-alt` umbenannt.

Das eigene Windows-Einstellungsfenster öffnet sich bei noch offener Ersteinrichtung. Später öffnet das Fuchs-Symbol oder ein erneuter Programmstart dieses Fenster. Schließen blendet es aus; **Beenden** im Infobereich stoppt die App. Beim Windows-Autostart bleibt das Fenster geschlossen.

Ausführliche Einrichtung der Datenquellen: [Schnellstart](ANLEITUNG.md).

## 3. Einstellungsseite

Sie erscheint in einem Win32-Fenster mit eingebettetem WebView2. Der interne Dienst läuft in der App und ist nur auf deinem PC erreichbar: `http://127.0.0.1:8765/` (Port einstellbar). Änderungen werden erst mit **Speichern & anwenden** übernommen; die Anzeige startet dann etwa 10 Sekunden neu.

| Bereich | Einstellungen |
|---|---|
| **Start** | Zustand (Display, Discord), Hersteller-App beenden/abschalten, Autostart, Ersteinrichtung |
| **Übersicht** | Live-Vorschau des Displays, Seiten ein/aus und Reihenfolge, Anzeigedauer, Helligkeit |
| **Freunde** | gemeinsame Liste: Seite an/aus, Discord einbeziehen, Steam- und Xbox-Zugang mit „Verbindung prüfen“ und Anleitung, Personen zusammenführen |
| **Discord** | Seite an/aus, Bot-Token, Offline-Freunde, Auswahl der Personen aus den Server-Mitgliedern, Server- und Ignorierlisten |
| **Battlefield 6** | Spieler hinzufügen (PC/PlayStation/Xbox), sofort **Prüfen**, Abrufintervall |
| **Blogs** | Blog-Adressen hinzufügen und mit **Prüfen** testen, Anzahl Beiträge, Intervall |
| **System** | Netzwerkadapter, Einheit (Mbit/s oder MB/s), Laufwerk, Sensoren für CPU-Temperatur und Lüfter |
| **Display** | Display-Typ, HDMI-/USB-C-Monitor, Touch-Steuerung und Touch-Zuordnung, Ausrichtung, COM-Port, Zurücksetzen beim Start, Ausschalten beim Beenden, Port der Einstellungsseite, Neu starten, Beenden |

**Verbindungsstatus:** In den Reitern Discord und Freunde zeigt eine Statuszeile dauerhaft, ob der Zugang funktioniert: grün mit Kontoname, Anzahl der Freunde und Zeitpunkt der letzten erfolgreichen Abfrage (Discord: live), rot mit der Fehlermeldung, wenn etwas nicht klappt (bei Steam/Xbox mit dem Zeitpunkt des letzten Erfolgs), grau, wenn nichts eingerichtet ist. Sie aktualisiert sich alle 5 Sekunden von selbst.

Der Bot-Token wird nie an den Browser zurückgegeben; das Feld zeigt nur, ob einer gespeichert ist. Leer lassen = behalten.

**HDMI/USB-C und Touch ab 2.0.3:** Unter **Display** den vorgesehenen Bildschirm auswählen; die Vollbildausgabe bleibt über der Taskleiste, ohne den Tastaturfokus zu übernehmen. Die Touch-Zuordnung zeigt, welchem Bildschirm Windows ein Touchgerät zugeordnet hat. Erscheinen Berührungen auf einem anderen Monitor, **Touch-Bildschirm zuordnen** wählen. Im Windows-Assistenten andere Bildschirme mit Enter überspringen, dann den gewünschten Touchscreen berühren; Esc bricht ab. Die Vollbildausgabe pausiert währenddessen und kehrt nach Ende des Assistenten zurück. Anschließend **Erneut prüfen** wählen und bei Bedarf die Touch-Steuerung aktivieren und speichern. HDMI überträgt nur das Bild; für Touch muss zusätzlich die USB-Verbindung des Geräts bestehen. Echte Touch-Hardware und der vollständige Assistentenablauf sind noch nicht physisch abgenommen; siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

---

## 4. Konfigurationsdatei

Bei regulärer Installation liegt `config.toml` unter `%LOCALAPPDATA%\FuchsLabs\USB-Display` neben der EXE und wird von der Einstellungsseite geschrieben (Sicherungskopie: `config.toml.bak`). Von Hand bearbeiten ist möglich; danach die App neu starten. Fehlende Einträge bekommen Standardwerte, ungültige Werte werden auf erlaubte Bereiche begrenzt.

| Abschnitt | Schlüssel (Standard) |
|---|---|
| `[discord]` | `enabled` (true), `token` (verschlüsselt), `guild_ids` ([]), `friend_ids` ([]), `ignore_ids` ([]), `show_offline` (false) |
| `[display]` | `port` ("AUTO"), `orientation` ("landscape"; auch `reverse_landscape`, `portrait`, `reverse_portrait`), `brightness` (50), `reset_on_start` (true), `page_seconds` (15), `pages` (["system","friends","discord","bf6","rss"]), `title`, `screen_off_on_exit` (true), `theme` ("hell"; oder "dunkel" – gilt für Display, Einstellungsseite und Anleitung; "eigen" = eigene Farben), `color_base` ("hell"/"dunkel", Vorlage für eigene Farben), `color_bg`, `color_card`, `color_header`, `color_accent`, `color_text`, `color_muted` ("#RRGGBB", leer = Vorlage), `output` ("serial" = 3,5"-USB-Display, "monitor" = HDMI/USB-C-Bildschirm als Vollbildfenster), `monitor` ("" = erster Nicht-Hauptbildschirm, sonst z. B. `\\.\DISPLAY2`), `touch` (false; Seitenwechsel per Wischen/Tippen, nur bei `output = "monitor"`) |
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

Die App speichert Discord-, Steam- und Xbox-Zugangsdaten per Windows-DPAPI verschlüsselt. Das schützt die Datei nicht vor Prozessen im selben Windows-Benutzerkonto. Niemals Konfigurationen mit echten Zugangsdaten veröffentlichen.

Der lokale Einstellungsdienst bindet an `127.0.0.1`, prüft Host und Fetch-Metadaten und verlangt für Schreibzugriffe `X-FL: 1`. RSS-Ziele und Weiterleitungen werden gegen nichtöffentliche IP-Bereiche geprüft. Die C++-Implementierung verwendet unter anderem WinHTTP, D++/OpenSSL und pugixml; Details stehen in `cpp/src/http.cpp`, `netguard.cpp`, `secure.cpp` und `webui.cpp`.

Abhängigkeiten sind über vcpkg-Manifest und D++-Overlay festgelegt; WebView2-SDK-Lizenzhinweise liegen unter `cpp/third_party/webview2`. Die WebView2 Evergreen Runtime wird separat bereitgestellt. Die EXE ist nicht signiert.

[SICHERHEITSREVIEW.md](SICHERHEITSREVIEW.md) dokumentiert den historischen Python-Stand und ist kein vollständiger C++-Audit. Sicherheitsmeldungen: [SECURITY.md](SECURITY.md).

## 7. Effizienz

Die C++-Version verwendet Windows-Grafik-APIs und statisch gelinkte Bibliotheken. WebView2 erzeugt für das Einstellungsfenster zusätzliche Laufzeitprozesse. Alte Python-Messwerte und ursprüngliche C++-Schätzungen sind kein belegter Ressourcenverbrauch der 2.0-Version. Einen reproduzierbaren CPU-/Speichervergleich gibt es bisher nicht.

Bestätigte Funktionsprüfungen: [cpp/TESTPROTOKOLL.md](cpp/TESTPROTOKOLL.md).

## 8. Fehlerbehebung

Das Protokoll steht in `usb-display.log` neben der EXE. Die meisten Probleme erklärt die Seite **Start** direkt.

| Symptom | Ursache und Lösung |
|---|---|
| „Die Hersteller-App des Displays läuft …“ | Auf **Start** → **Hersteller-App beenden**, danach deren Autostart **abschalten**. |
| „Display nicht gefunden“ | Kabel prüfen, anderen USB-Anschluss probieren; notfalls bei **Display** den COM-Port von Hand eintragen (Geräte-Manager → Anschlüsse). Die App versucht es alle 10 Sekunden erneut. |
| Bild steht auf dem Kopf | **Display → Ausrichtung** auf die gedrehte Variante stellen. |
| Taskleiste liegt über der HDMI-/USB-C-Anzeige | Auf 2.0.3 aktualisieren; die Vollbildausgabe hält die Taskleiste auf dem gewählten Monitor darunter. |
| Berührungen erscheinen auf einem anderen Bildschirm | **Display → Touch-Bildschirm zuordnen**; im Windows-Assistenten den vorgesehenen Touchscreen identifizieren. Danach **Erneut prüfen** wählen. |
| Kein Touchscreen erkannt | USB-Touchverbindung und Windows-Treiber prüfen, dann **Erneut prüfen**. Das HDMI-Bild allein stellt keinen Touch-Eingang bereit. |
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
| Einstellungsfenster öffnet nicht | WebView2 Evergreen Runtime prüfen; App-Prozess und Log kontrollieren. Bei belegtem Port UI-Port in der Konfiguration ändern. |
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

Beispiel ohne Hardware: `build\FuchsLabs-USB-Display.exe --run --no-lcd -c test.toml`. Für parallele Entwicklung ist zusätzlich ein eigener UI-Port und der nur für Tests gedachte Instanzname über `FUCHSLABS_INSTANCE` nötig.

## 10. Technischer Aufbau

Die aktuelle Implementierung ist C++20 unter `cpp/`.

| Datei unter `cpp/src/` | Aufgabe |
|---|---|
| `main.cpp`, `app.cpp` | Start, Einzelinstanz, Dienste und Seitenrotation |
| `settings_window.cpp`, `tray.cpp` | Win32/WebView2-Fenster und Infobereich |
| `webui.cpp`, `config.cpp`, `secure.cpp` | lokaler Einstellungsdienst, TOML und DPAPI |
| `serial_display.cpp`, `monitor_display.cpp` | USB-Display bzw. HDMI/USB-C und Touch |
| `gfx.cpp`, `render.cpp` | Grafik und Seitenlayout |
| `discord.cpp`, `friends.cpp`, `bf6.cpp`, `rss.cpp`, `sysinfo.cpp` | Datenquellen; Discord über D++ |
| `installer.cpp`, `winsetup.cpp` | Installation, Updates, Deinstallation und Autostart |

Einstellungen und Anleitungen unter `cpp/res/` sind als Ressourcen in die EXE eingebettet. Die Python-Dateien im Repository-Hauptordner bleiben der historische Stand 1.x.

## 11. Selbst bauen

Siehe [C++-Buildanleitung](cpp/README.md#build). Im Checkout **in den Ordner `cpp` wechseln**, dann `build.bat nopause` ausführen. Die `build.bat` im Hauptordner baut weiterhin den alten Python-Stand.

Der Build benötigt Visual Studio Build Tools 2022, Windows SDK, CMake/Ninja und vcpkg unter `%USERPROFILE%\FuchsLabs\vcpkg`. Ergebnis: `cpp\dist\FuchsLabs-USB-Display.exe`.

## 12. Deinstallation

In Windows **Einstellungen → Apps → FuchsLabs USB-Display → Deinstallieren**. Der integrierte Deinstaller entfernt Autostart und Verknüpfung. Der Dialog bietet zusätzlich an, Einstellungen und gespeicherte Zugangsdaten zu entfernen. Ein aufbewahrter `-1.x-alt`-Ordner gehört zur alten Version und ist ein eigener Rückfallstand.

## 13. Prüfstand und Windows-Neustart

**2.0.3:** Release-Build, 86 Kernprüfungen und 56 Monitor-/Touchprüfungen einschließlich 11 Basisprüfungen bestanden. Die 2.0.3-Installation wurde auf Datei-Version, Hash, API-Version, Autostart-Eintrag und `display_ok=true` geprüft. Touch wurde mit injizierten Windows-Eingaben getestet; echte Touch-Hardware, der vollständige Zuordnungsassistent und ein neuer Windows-Anmeldetest bleiben offen. Details im [Testprotokoll](cpp/TESTPROTOKOLL.md).

**Frühere Abnahme von 2.0:**

Hardware-Gegenvergleich am 22.10.2026 bestanden: 1.x und 2.0 meldeten an COM7 `display_ok=true`, der Nutzer bestätigte die physische Anzeige. Das native Einstellungsfenster wurde sichtbar geöffnet.

Der anschließende Windows-Neustarttest schlug fehl: kein Tray-Prozess, altes Displaybild blieb stehen. Eine Diagnose außerhalb des Codex-Paketkontexts zeigte, dass die neue Installation nur in dessen Windows-Paketumleitung sichtbar war. Windows selbst sah am vorgesehenen Ziel keine EXE und noch den alten Run-Eintrag. Installation und Autostart wurden deshalb außerhalb dieses Kontexts eingerichtet. Am 22.10.2026 bestätigte der Nutzer den erfolgreichen erneuten Windows-Neustart mit automatisch erscheinendem Tray-Symbol.
