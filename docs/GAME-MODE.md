# Game-Mode

**Deutsch** · [English](GAME-MODE.en.md)

Lokale Entwicklung auf `feature/game-mode`, noch nicht veröffentlicht. Der [Umsetzungsplan und Prüfstand](PLAN-GAME-MODE.md) nennt die tatsächlich umgesetzten Quellen, ausgeführten Tests und offenen Arbeiten. Die veröffentlichten Downloads und Versionsnummern ändern sich dadurch nicht.

Game-Mode erkennt ein laufendes Spiel und zeigt auf dem gewählten ScreenBuddy-Display eine eigene Ansicht. Ein optionales Intro zeigt das Spiellogo. Nach Spielende kehrt das Display zur vorherigen Seitenrotation zurück.

## Einrichten

1. Unter **App → Eigene Ansichten** eine Ansicht erstellen oder die Vorlage **Game-Mode** verwenden. Die gewünschten Spiel- und Systemkacheln hinzufügen, Größe und Geräte-Layout prüfen und die Ansicht speichern.
2. **Game-Mode** in den Einstellungen öffnen und aktivieren. Das Ziel-Display und die gespeicherte Ansicht (`custom:<id>`) wählen.
3. Intro nach Wunsch einschalten. **Animationen reduzieren** verwenden, wenn Skalierung und Glanzlauf stören.
4. Optional ein Ping-Ziel als IPv4-Adresse eintragen. Standard ist `1.1.1.1`; für eine Messung im Heimnetz die IPv4-Adresse des Routers verwenden. Ping misst die Erreichbarkeit dieses Ziels, nicht die Verzögerung zum Spielserver.
5. Zusätzliche Spiele und Ausschlüsse bei Bedarf ergänzen. Speichern und ScreenBuddy neu starten, damit die Hintergrundquellen die neuen Einstellungen übernehmen.

`primary` bezeichnet die Hauptausgabe von ScreenBuddy, also das konfigurierte USB-/Monitor-Display oder die Haupt-Netzwerkausgabe. Eine Companion-Geräte-ID bezeichnet genau dieses gekoppelte Gerät. Ein ausgeschaltetes oder getrenntes Ziel wird dadurch nicht eingeschaltet oder neu gekoppelt.

## Spielerkennung

Steam-Spiele werden über die Steam-Registry und lokale Bibliotheks-/App-Manifeste zugeordnet. Epic verwendet die lokalen Launcher-Manifeste. Xbox/Game-Pass-Spiele werden anhand der `XboxGames`-Installationspfade zugeordnet. Die allgemeine Erkennung prüft Vordergrundfenster/Vollbild und die Zugehörigkeit zu einer bekannten Spielebibliothek. Ein beliebiges Vollbildfenster allein gilt nicht als Spiel.

Zusätzlich werden folgende lokale HKLM-Registry-Einträge gelesen (jeweils `SOFTWARE\WOW6432Node` und die native `SOFTWARE`-Ansicht):

| Bibliothek | Quelle und Name |
| --- | --- |
| GOG Galaxy | `GOG.com\Games\<id>`: `gameName`, `path`, optional `exe`. |
| Ubisoft Connect | `Ubisoft\Launcher\Installs\<id>\InstallDir`; Name aus einem Uninstall-Eintrag mit identischem Installationspfad, sonst aus dem Ordnernamen. |
| EA App/Origin | `EA Games\<Name>` oder `Electronic Arts\<Name>`: `Install Dir`; außerdem Uninstall-Einträge mit Publisher `Electronic Arts`/`EA` (auch die unterstützten `Inc.`-Schreibweisen) und `InstallLocation`. |
| Battle.net/Blizzard | Uninstall-Einträge mit Publisher `Blizzard Entertainment` (auch `Inc.`) und `InstallLocation`. Zusätzlich `Blizzard Entertainment\Battle.net` beziehungsweise `Battle.net`, Wert `InstallPath`, oder der Battle.net-Uninstall-Pfad: nur fest benannte Spielordner darin und direkt daneben. |
| Rockstar | `Rockstar Games\<Name>\InstallFolder`. |

Uninstall bedeutet `Microsoft\Windows\CurrentVersion\Uninstall\<Eintrag>` unter den genannten HKLM-Ansichten. Für Battle.net umfasst die Ordnerliste World of Warcraft, Diablo III/IV, StarCraft/II, Hearthstone, Heroes of the Storm, Overwatch und Warcraft III/Warcraft III Reforged. Es gibt keine rekursive Laufwerkssuche und keine Abfrage von Launcher-Konten oder undokumentierten Datenbanken.

Bei allen neuen Bibliotheken muss zum Start ein Prozess unter dem registrierten Installationspfad laufen und dessen Fenster im Vordergrund Vollbild/Borderless belegen. Auch eine hinterlegte GOG-EXE umgeht diese Prüfung nicht; Epic-EXE-Treffer verwenden jetzt ebenfalls die Fensterprüfung. Steam behält den bisherigen `RunningAppID`-Sonderweg; manuell eingetragene EXEs gelten bei lesbarem Prozesspfad weiterhin als exakte Treffer ohne Fensterprüfung. Bei fehlendem Pfad gilt auch für sie der Dateinamen-Fallback mit Vordergrund/Vollbild. Ein bereits bestätigter, unverändert laufender Spielprozess bleibt beim Alt-Tab aktiv. Die Bibliotheken werden ungefähr alle 60 Sekunden neu eingelesen.

Grenzen: Fehlende/veraltete Registry-Einträge, reine Benutzerinstallationen ohne passenden HKLM-Eintrag, abweichende Publisher-Namen oder andere Battle.net-Spielordner können Erkennung verhindern. Dann den tatsächlichen Spiel-EXE-Pfad manuell ergänzen. Ein unbekanntes Hilfsprogramm im Spielordner kann als Spiel gelten, wenn es selbst im Vordergrund Vollbild läuft; bei Bedarf ausschließen. Pfadzugehörigkeit wird lexikalisch geprüft, ohne Junction-/Symlink-Auflösung. Nicht-Steam-Spiele verwenden das EXE-Icon plus Spielname; fehlt das Icon, bleibt der Name. Echte Abnahme dieser fünf Launcher steht aus.

**Zusätzliche Spiele:** je Zeile der vollständige Pfad zu einer Spiel-EXE, etwa `D:\Spiele\Beispielspiel\Spiel.exe`. Keine Befehlszeilen, Platzhalter oder Verzeichnisse eintragen. **Ausschlüsse:** je Zeile ein EXE-Dateiname oder vollständiger EXE-Pfad. Browser, Explorer, Medienplayer, bekannte Launcher, `Agent.exe`, Blizzard-Fehler-/Crashhandler und EXE-Namen mit `Launcher` werden grundsätzlich ausgeschlossen (ohne Beachtung der Groß-/Kleinschreibung). Das gilt auch für `EAAntiCheat.GameServiceLauncher.exe`, `BEService.exe` und `EasyAntiCheat*.exe`; weitere Hilfsprogramme bei Bedarf ergänzen.

Die Erkennung entprellt Wechsel: Ein Spiel muss mindestens drei Sekunden stabil erkannt werden, bevor Game-Mode startet. Nach fünf Sekunden ohne das bisherige Spiel endet Game-Mode. Kurzzeitiges Erkennen oder Verschwinden löst damit keinen sofortigen Seitenwechsel aus. Die Spielzeit zählt die erkannte Sitzung, keine vom Spielekonto gelieferte Gesamtspielzeit.

Die Zustände sind `idle → starting → running → ending`. Nicht zuordenbare Prozesse führen zu fehlenden Daten; bei fehlendem Image-Pfad kann der unten beschriebene Dateinamen-Fallback greifen. ScreenBuddy fordert keine erhöhten Spielprozessrechte an.

## Anti-Cheat-geschützte Spiele: Erkennung per Dateiname (P9-9)

Schutztreiber wie EA Anti-Cheat/Javelin können den Image-Pfad eines laufenden Spiels verbergen. Die Toolhelp-Prozessliste liefert weiterhin PID und EXE-Dateinamen, ohne einen Prozesshandle zu benötigen. Wenn der Pfad fehlt, vergleicht ScreenBuddy den Dateinamen mit dem lokalen Bibliothekscache und verlangt das Vordergrundfenster desselben Prozesses im Vollbild. Fenstertitel allein reichen nicht aus. Mehrdeutige Dateinamen in unterschiedlichen Installationen werden nicht automatisch zugeordnet.

Beim Bibliotheks-Refresh (alle 60 Sekunden) werden je Installationsordner höchstens 2.000 Einträge bis drei Unterordnerebenen gelesen und nur EXE-Dateien gecacht. Reparse Points/Junctions werden nicht verfolgt; manuelle Einträge liefern nur den ausdrücklich angegebenen EXE-Namen. Das gilt für Steam, Epic, Registry-Bibliotheken und lokale `XboxGames`-Ordner auf festen Laufwerken. Launcher, Anti-Cheat-, Installer-, Crash- und Hilfsprozesse bleiben ausgeschlossen, insbesondere `EAAntiCheat.GameService*.exe`. Dateinamen- und Pfadausschlüsse gelten auch beim Fallback.

Beispiel: Für **Battlefield 6**, Steam-App **2807960**, kann `bf6.exe` trotz leerem Image-Pfad und Steam-RunningAppID `0` (EA-App-Start) zugeordnet werden. Name und Steam-App-ID stammen weiterhin aus dem lokalen Appmanifest; das Logo verwendet die Steam-App-ID. Die Zuordnung bleibt über PID und EXE-Namen bei Alt-Tab erhalten. Die 3/5-Sekunden-Entprellung und PID-basierte ETW-Zuordnung bleiben bestehen; Erkennung garantiert keine verfügbaren FPS-Daten. Es werden keine Prozessrechte verändert, kein Prozessspeicher gelesen und keine erhöhten Rechte angefordert.

P9-9-Prüfstand: 26/26 Suiten bestanden, darunter 267 Game-Mode-Prüfungen mit simulierten Prozesslisten und lokalen Dateisystem-Fixtures. Eine Live-Abnahme mit Battlefield 6/Javelin sowie physische Display- und FPS-Prüfungen stehen aus. Ergebnisse: [P9-9-Prüfstand](PLAN-GAME-MODE.md#p9-9-prozesse-ohne-lesbaren-image-pfad).

## Logos und Intro

Für Steam-App-IDs werden Bilder ausschließlich von den vorgesehenen öffentlichen Steam-Asset-URLs geladen: zuerst `logo.png`, danach Header/Hero als Rückfall. Dafür ist weder ein Steam-Web-API-Key noch ein TMDB-Zugang erforderlich. Der lokale Cache liegt im ScreenBuddy-Datenverzeichnis; seine Gültigkeit beträgt sieben Tage. Ohne verfügbares Asset dienen das EXE-Icon und der Spielname als Rückfall.

Das normale Intro dauert ungefähr 2,5 Sekunden: abgedunkelter Hintergrund, Einblenden/Skalieren des Logos, Glanzlauf und Übergang zur gewählten Ansicht. Die reduzierte Variante verwendet nur einen etwa 0,35 Sekunden langen Übergang und vermeidet Skalierung und Glanzlauf. Das Intro lässt sich ganz abschalten.

## Messwerte und `n/a`

Die Game-Mode-Kacheln gehören im Katalog zur Quelle **System**; der Palettenfilter **Game-Mode** fasst die passenden Kacheln zusammen. CPU-Kernlast und Frametime-Verlauf benötigen genügend Kachelhöhe und eine andere Dichte als „Kompakt“.

Verfügbare Werte hängen von GPU, Treiber, Windows-Rechten und aktivierten Quellen ab. `n/a` bedeutet, dass kein gültiger Messwert vorliegt; es bedeutet nicht 0. Ein fehlender Sensor darf den übrigen Game-Mode nicht verhindern.

| Kachel/Quelle | Bedeutung und Grenze |
| --- | --- |
| Spielname, Logo, Spielzeit | Lokale Erkennung und erkannte Sitzung; keine Kontoabfrage. |
| GPU | NVIDIA: dynamisch aus System32 geladenes NVML, erster Adapter. AMD: dynamisch geladenes ADL mit Overdrive 5/6, nur bei Unterstützung durch den Treiber. PDH „GPU Engine“ liefert als Last-Rückfall die höchste Auslastung einer Engine. Temperatur, Takt, Lüfter und Leistung können einzeln fehlen. |
| VRAM | NVML meldet Belegung und Kapazität des ersten NVIDIA-Adapters. Sonst: PDH „GPU Adapter Memory\Dedicated Usage“ für die Belegung und über Hardwareadapter summierte DXGI-`DedicatedVideoMemory`-Kapazität. Adapterweite Werte, keine exklusive VRAM-Belegung des Spiels. |
| CPU | Last je logischem Prozessor über PDH „Processor Information“, Takt über PDH und `CallNtPowerInformation`. Temperatur benötigt wie bisher eine passende HWiNFO-Shared-Memory-Quelle. |
| Ping/Jitter | ICMP ungefähr jede Sekunde zum eingestellten IPv4-Ziel, mit 200 ms Zeitlimit. Jitter ist die absolute Differenz zweier aufeinanderfolgender erfolgreicher Antwortzeiten; nach einem Fehler braucht es wieder zwei erfolgreiche Antworten. Blockierte Echos, Zeitüberschreitungen und ungültige Ziele ergeben `n/a`. |
| RAM, Netzwerk | Bestehende Systemquellen; Netzwerkwerte gehören zum ausgewählten Adapter. |
| FPS, Frametime, 1 % Low, Verlauf | Eigene ETW-Sitzung für DXGI-`Present_Start`, einschließlich MPO, zugeordnet zur erkannten Prozess-ID und nach Swapchain getrennt. Gemessen werden DXGI-Present-Aufrufe, keine tatsächlich am Monitor ausgegebenen Bilder. Details und Grenzen unten. |

Die FPS-Anzeige verwendet die aktivste Swapchain und ignoriert `DXGI_PRESENT_TEST`-Aufrufe. Das Messfenster umfasst höchstens zehn Sekunden und 1.200 Intervalle. Frametime ist der mittlere Abstand der Present-Aufrufe; 1 % Low wird erst ab 100 Intervallen aus dem Mittel der langsamsten 1 % berechnet. Nach zwei Sekunden ohne neue Daten werden die Werte ungültig.

Das ist kein vollständiger PresentMon-/DxgKrnl-Sammler und keine Scanout-FPS-Messung. Vulkan, OpenGL und D3D9 ohne DXGI-Ereignisse bleiben `n/a`; fehlende Rechte, fehlende Ereignisse oder Ereignisverluste können ebenfalls `n/a` ergeben. ETW-Echtzeitsitzungen können Administratorrechte oder die Windows-Gruppe **Performance Log Users** erfordern. Erhöhte Rechte garantieren keine Messwerte. Keine Rechte an Spielprozessen ändern.

**FPS ohne Administrator:** `cpp/tools/Grant-PerfLogUsers.bat` einmalig per Rechtsklick „Als Administrator ausführen“ starten. Es trägt den aktuellen Benutzer in die Gruppe „Leistungsprotokollbenutzer“ ein (Rückgängig: `net localgroup "Performance Log Users" <Benutzer> /delete`); danach ab- und anmelden. Das Skript wird nie automatisch ausgeführt. Nicht garantiert sind außerdem UWP-/Store-Spiele, Streaming-/Cloud-Spiele und Prozesse, die nicht selbst präsentieren: dort bleibt die Anzeige `n/a`. Reste abgestürzter ScreenBuddy-Messsitzungen (`ScreenBuddy.Game.<PID>.<Tick>`) werden beim nächsten Start beendet. PresentMon-Code wird nicht verwendet; es wird nur dieselbe öffentliche ETW-Schnittstelle gelesen.

ADLX ist nicht eingebunden; AMD-Werte sind auf die vom Treiber angebotenen ADL-Overdrive-5/6-Abfragen beschränkt. DXGI `QueryVideoMemoryInfo` wird bewusst nicht als VRAM-Spielverbrauch verwendet: Diese Abfrage würde den aufrufenden ScreenBuddy-Prozess beschreiben.

## Datenschutz und erlaubte Schnittstellen

Die Umsetzung verwendet ausschließlich dokumentierte Windows-/Herstellerschnittstellen: Registry, lokale Launcher-Manifeste, Prozess-/Fensterinformationen, PDH, ETW/DXGI, dynamisch geladenes NVML beziehungsweise AMD ADL und ICMP. Kein Einhaken, keine DLL-Injektion und kein Lesen des Speichers fremder Spielprozesse. Kein Code aus libre-panel wird übernommen; es gelten die [Clean-Room-Regeln](CLEAN-ROOM-LAYOUT.md).

Steam-CDN-Anfragen übermitteln die angeforderte App-ID und wie gewöhnliche HTTPS-Anfragen die IP-Adresse. Ping sendet ICMP-Pakete an das konfigurierte Ziel. Lokale EXE-Pfade und Launcher-Manifeste werden für die Erkennung gelesen; sie gehören nicht in veröffentlichte Screenshots oder Fehlerberichte.

## Fehlerbehebung

- **Kein Wechsel:** Aktivierung, gewählte Ansicht und Ziel-Display prüfen; Änderungen speichern und neu starten. Auf mindestens drei Sekunden stabile Erkennung warten. EXE-Pfad und Ausschlüsse kontrollieren.
- **Geschütztes Spiel nicht erkannt:** Das Spielfenster im Vordergrund auf Vollbild/Borderless stellen, Bibliothekseintrag und Ausschlüsse prüfen und bis zum nächsten Bibliotheks-Refresh (etwa 60 Sekunden) warten. Bei mehreren Installationen mit derselben EXE bleibt die Zuordnung gegebenenfalls offen.
- **Falsches Programm erkannt:** Den tatsächlichen Spielpfad statt des Launchers hinterlegen und Hilfsprogramme ausschließen.
- **Logo fehlt:** Internet/CDN-Erreichbarkeit prüfen; EXE-Icon/Name bleiben Rückfall. Ein fehlendes Logo verhindert die Ansicht nicht.
- **Einzelner Messwert `n/a`:** Unterstützung der konkreten Quelle, Treiber und gegebenenfalls HWiNFO prüfen. Für Ping ein erreichbares IPv4-Ziel verwenden. Fehlende Messwerte nicht durch erfundene Werte ersetzen.
- **Ansicht fehlt:** Ansicht im Editor speichern und den `custom:<id>`-Verweis prüfen. Eine gelöschte Ansicht muss neu gewählt werden.

## Entwicklertests und Prüfgrenzen

Die App-EXE enthält den Installer. Sie für Tests ausschließlich über ein isolierendes Startskript starten: `FUCHSLABS_INSTANCE`, eigenes Datenverzeichnis, UI-Port **18766**, KI-Empfänger **18765**, keine Verbindung zu **8765**. Die Vorlage liegt unter `ScreenBuddy-2.10.0-test\Start-ScreenBuddy-Test.ps1`; vorhandene Testkonfigurationen auf die getrennten Ports anpassen, da ein Skriptparameter allein eine vorhandene Konfiguration nicht umschreibt. Keine hörbaren Testtöne.

Unit-Tests prüfen die Logik; eine Dummy-Prozess-Simulation prüft die isolierte App-Integration. Solche Prüfungen belegen keine reale Spieleerkennung über alle Launcher, keine physische Display-/Touch-Ausgabe und keine Hardwaremesswerte. Ausgeführte Prüfungen mit Ergebnis, Screenshot und noch offene Abnahmen stehen ausschließlich im [aktuellen Prüfstand](PLAN-GAME-MODE.md#p9-6-tests-und-doku).

Prüfung am 07.11.2026: 26 automatisierte Suiten und die isolierte Dummy-Simulation bestanden. Das Startskript liegt nun im Projekt unter `cpp/tests/Start-GameMode-Test.ps1`; Aufruf mit `python cpp/tests/game_mode_integration.py`. UI 18766; KI blieb wegen belegtem 18765 ausgeschaltet. Aktuelle Vorschaubilder liegen nur lokal unter `cpp/build/game-mode-test/`. Vollständiger Prüfstand und offene echte Spiel-/Hardwareabnahme im Plan.

P9-7: Die fünf zusätzlichen Registry-Quellen und Ausschlüsse sind durch synthetische Unit-Tests abgedeckt (196 Game-Mode-Prüfungen insgesamt); echte Launcher-/Spiel- und Hardwareabnahme wurde dafür nicht ausgeführt. Aktueller Gesamtlauf und Grenzen: [P9-7-Prüfstand](PLAN-GAME-MODE.md#p9-7-weitere-spielebibliotheken).

## P9-8

Solange Game-Mode aktiviert ist, ist die gewählte Ansicht auf allen Ausgaben aus normaler Rotation, Navigation, Companion-Seitenlisten und Seiten-Tap-Zielen reserviert. Sie erscheint nur im Game-Mode-Pfad. Bei deaktiviertem Game-Mode ist sie wieder eine normale Seite.

Nicht-Steam-Spiele (auch GOG/Ubisoft) verwenden das direkt aus der laufenden EXE extrahierte Icon mit bis zu 256 Pixeln. Bei passendem `Application Executable` im benachbarten `AppxManifest.xml` werden zuerst Paket-Logos einschließlich Größenvarianten gelesen; danach EXE-Icon, Shell-Icon und Name. Fehlende Rechte oder nicht auflösbare Paketressourcen bleiben Fallback-Fälle; das erweitert nicht die Spielerkennung.

Die neue Vorlage enthält oben Logo, Name und Spielzeit, darunter neun Metriken in drei Spalten. Bestehende gespeicherte Layouts werden nicht überschrieben; die Vorlage bei Bedarf neu anlegen. Spielkacheln einschließlich CPU/GPU verwenden gemeinsame Karten, Rundungen, orange Titel und größere Werte. FPS/Frametime zeigen bei ausreichender Höhe und Dichte oberhalb „Kompakt“ einen Verlauf aus bis zu 60 Sekundenwerten. Ungültige Messungen bleiben Lücken; ein Prozesswechsel leert den Verlauf. Das zehnsekündige ETW-Statistikfenster bleibt unverändert.

**Cockpit-Design:** Labels klein, grau und in Großbuchstaben, ein oranger Akzentstreifen links, Einheiten klein in Orange neben dem Wert. Eine FPS-Kachel ab etwa 100 Höhe wird zur Hero-Kachel (Orange-Rahmen, sehr große Zahl, Verlauf mit Fläche). CPU/GPU erscheinen in nicht zu breiten Kacheln als Ring statt Balken; Balken und Ringe wechseln ab 80 % auf Gelb und ab 90 % auf Rot (feste Signalfarben). Ein Hintergrundverlauf der Kachel ist wegen fehlender Alpha-Füllung im Renderer nicht umgesetzt.

Das Intro wird im normalen Ausgabeframe komponiert, in `last_frame()` gespeichert und ist über `/api/preview.png` sichtbar. Der isolierte Test auf Port 18766 prüft verschiedene Intro-Frames und den anschließenden Ansichtsframe. Das ist ein Software-Nachweis, keine physische Displayabnahme.

P9-8-Prüfstand: 26/26 Suiten (207 Game-Mode-Prüfungen) und isolierte Dummy-/Intro-Integration auf 18766 bestanden. Echte Launcher-Logos und physische Displays nicht getestet; Einzelheiten im [Plan](PLAN-GAME-MODE.md#p9-8-rotation-logos-und-spielkacheln).
