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

**Zusätzliche Spiele:** je Zeile der vollständige Pfad zu einer Spiel-EXE, etwa `D:\Spiele\Beispielspiel\Spiel.exe`. Keine Befehlszeilen, Platzhalter oder Verzeichnisse eintragen. **Ausschlüsse:** je Zeile ein EXE-Dateiname oder vollständiger EXE-Pfad. Browser, Explorer und Medienplayer werden grundsätzlich ausgeschlossen; weitere Launcher-/Hilfsprogramme bei Bedarf ergänzen.

Die Erkennung entprellt Wechsel: Ein Spiel muss mindestens drei Sekunden stabil erkannt werden, bevor Game-Mode startet. Nach fünf Sekunden ohne das bisherige Spiel endet Game-Mode. Kurzzeitiges Erkennen oder Verschwinden löst damit keinen sofortigen Seitenwechsel aus. Die Spielzeit zählt die erkannte Sitzung, keine vom Spielekonto gelieferte Gesamtspielzeit.

Die Zustände sind `idle → starting → running → ending`. Fehlende Leserechte und nicht zuordenbare Prozesse führen zu fehlenden Daten. ScreenBuddy fordert keine erhöhten Spielprozessrechte an.

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

ADLX ist nicht eingebunden; AMD-Werte sind auf die vom Treiber angebotenen ADL-Overdrive-5/6-Abfragen beschränkt. DXGI `QueryVideoMemoryInfo` wird bewusst nicht als VRAM-Spielverbrauch verwendet: Diese Abfrage würde den aufrufenden ScreenBuddy-Prozess beschreiben.

## Datenschutz und erlaubte Schnittstellen

Die Umsetzung verwendet ausschließlich dokumentierte Windows-/Herstellerschnittstellen: Registry, lokale Launcher-Manifeste, Prozess-/Fensterinformationen, PDH, ETW/DXGI, dynamisch geladenes NVML beziehungsweise AMD ADL und ICMP. Kein Einhaken, keine DLL-Injektion und kein Lesen des Speichers fremder Spielprozesse. Kein Code aus libre-panel wird übernommen; es gelten die [Clean-Room-Regeln](CLEAN-ROOM-LAYOUT.md).

Steam-CDN-Anfragen übermitteln die angeforderte App-ID und wie gewöhnliche HTTPS-Anfragen die IP-Adresse. Ping sendet ICMP-Pakete an das konfigurierte Ziel. Lokale EXE-Pfade und Launcher-Manifeste werden für die Erkennung gelesen; sie gehören nicht in veröffentlichte Screenshots oder Fehlerberichte.

## Fehlerbehebung

- **Kein Wechsel:** Aktivierung, gewählte Ansicht und Ziel-Display prüfen; Änderungen speichern und neu starten. Auf mindestens drei Sekunden stabile Erkennung warten. EXE-Pfad und Ausschlüsse kontrollieren.
- **Falsches Programm erkannt:** Den tatsächlichen Spielpfad statt des Launchers hinterlegen und Hilfsprogramme ausschließen.
- **Logo fehlt:** Internet/CDN-Erreichbarkeit prüfen; EXE-Icon/Name bleiben Rückfall. Ein fehlendes Logo verhindert die Ansicht nicht.
- **Einzelner Messwert `n/a`:** Unterstützung der konkreten Quelle, Treiber und gegebenenfalls HWiNFO prüfen. Für Ping ein erreichbares IPv4-Ziel verwenden. Fehlende Messwerte nicht durch erfundene Werte ersetzen.
- **Ansicht fehlt:** Ansicht im Editor speichern und den `custom:<id>`-Verweis prüfen. Eine gelöschte Ansicht muss neu gewählt werden.

## Entwicklertests und Prüfgrenzen

Die App-EXE enthält den Installer. Sie für Tests ausschließlich über ein isolierendes Startskript starten: `FUCHSLABS_INSTANCE`, eigenes Datenverzeichnis, UI-Port **18766**, KI-Empfänger **18765**, keine Verbindung zu **8765**. Die Vorlage liegt unter `ScreenBuddy-2.10.0-test\Start-ScreenBuddy-Test.ps1`; vorhandene Testkonfigurationen auf die getrennten Ports anpassen, da ein Skriptparameter allein eine vorhandene Konfiguration nicht umschreibt. Keine hörbaren Testtöne.

Unit-Tests prüfen die Logik; eine Dummy-Prozess-Simulation prüft die isolierte App-Integration. Solche Prüfungen belegen keine reale Spieleerkennung über alle Launcher, keine physische Display-/Touch-Ausgabe und keine Hardwaremesswerte. Ausgeführte Prüfungen mit Ergebnis, Screenshot und noch offene Abnahmen stehen ausschließlich im [aktuellen Prüfstand](PLAN-GAME-MODE.md#p9-6-tests-und-doku).
