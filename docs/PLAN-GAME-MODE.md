# Plan Game-Mode (feature/game-mode, Basis d0c279f)

Ziel: Spielstart erkennen, Spiellogo animiert auf dem gewählten Display zeigen, dann in die Game-Mode-Ansicht (`custom:<id>`, per Editor eingerichtet) wechseln, nach Spielende zurück zur normalen Rotation.

## Regeln
- Nur legale, dokumentierte Wege. Kein Einhaken in Spielprozesse, keine DLL-Injection, kein Speicherlesen fremder Prozesse (Anti-Cheat).
- libre-panel nur Ideenquelle (Clean-Room, siehe docs/CLEAN-ROOM-LAYOUT.md).
- Tests nie gegen Port 8765; nur isolierte Testinstanz (UI 18766, KI 18765, eigenes Datenverzeichnis). Keine hörbaren Töne.

## P9-1 Spielerkennung (`game_detect.h/.cpp`)
- Steam: `HKCU\Software\Valve\Steam\RunningAppID`, appmanifest_*.acf aus libraryfolders.vdf (Name, appid).
- Epic: `%ProgramData%\Epic\EpicGamesLauncher\Data\Manifests\*.item` (Install-Pfad, Name, AppName). Xbox/Game Pass: Install-Pfade `XboxGames`, Prozess unter diesem Pfad.
- Generisch: Vordergrundfenster im Vollbild/Borderless (`SHQueryUserNotificationState` = D3D-Fullscreen, Fenster = Monitorgröße) plus Prozess aus bekannter Bibliothek; Ausschlussliste (Browser, Explorer, Player). Benutzerliste „zusätzliche Spiele“ in den Einstellungen.
- Entprellung (Start nach 3 s stabil, Ende nach 5 s), Zustandsautomat idle → starting → running → ending.
- Nur Prozessliste/Fensterinfo/Registry/Dateien lesen.

## P9-2 Logo (`game_logo.h/.cpp`)
- Steam: öffentliches CDN (`shared.fastly.steamstatic.com/store_item_assets/steam/apps/<appid>/logo.png`, Fallback `header.jpg`/`library_hero`). Lokaler Cache im Datenverzeichnis. Fallback: TMDB-Zugang nicht nötig; sonst Exe-Icon + Name als Text.
- Kein Scraping, nur offizielle Asset-URLs; Cache mit Ablauf.

## P9-3 Animation (`game_intro.h/.cpp`)
- Intro ~2,5 s auf dem gewählten Display: Hintergrund abdunkeln, Logo skaliert/blendet ein mit Glanzlauf (Easing), Fade in die Game-Mode-Ansicht. Reduzierte Variante bei „Animationen reduzieren“. Nutzt vorhandenes Fade/FadeCache aus app.cpp.

## P9-4 Neue Systemquellen (Tiles, in Katalog und Palette)
Nur dokumentierte APIs:
| Kachel | Quelle |
|---|---|
| FPS, Frametime (1 % Low, Verlauf) | ETW `Microsoft-Windows-DxgKrnl`/`DXGI` Present-Events (wie PresentMon, MIT); Prozess = erkanntes Spiel; Rechtefall (Gruppe „Performance Log Users“/Admin) erklären, bei Fehlen Kachel „n/a“ |
| GPU-Last, Temperatur, Takt, Lüfter, Leistung | NVML (`nvml.dll` dynamisch geladen), AMD ADLX/ADL dynamisch, Fallback PDH „GPU Engine“ |
| VRAM belegt/gesamt | DXGI `QueryVideoMemoryInfo`, PDH „GPU Adapter Memory“ |
| CPU-Takt, CPU-Last pro Kern, CPU-Temp | PDH „Processor Information“, `CallNtPowerInformation`; Temp wie bisher über HWiNFO |
| Ping | ICMP `IcmpSendEcho` auf konfigurierbares Ziel (Standard Router/1.1.1.1), Jitter |
| Spielzeit, Spielname, Logo | aus P9-1/P9-2 |
| RAM, Netz | vorhanden |

## P9-5 Einstellungen und Editor
- Neue Seite „Game-Mode“: Aktiv, Ziel-Display, Ansicht (`custom:<id>`), Intro an/aus, Ping-Ziel, zusätzliche Spiele, Ausschlüsse.
- Kachel-Palette um neue Kacheln erweitern, Vorlage „Game-Mode“ im Editor.
- Rückkehr zur vorherigen Rotation beim Spielende.

## P9-6 Tests und Doku
- Unit-Tests: Zustandsautomat, ACF/Epic-Parser, Entprellung, Logo-Cache, Kachel-Katalog.
- Isolierter Testlauf über `ScreenBuddy-2.10.0-test\Start-ScreenBuddy-Test.ps1` (separates Datenverzeichnis, Port 18766), Screenshot; Spielstart per Dummy-Prozess simulieren.
- Doku DE/EN (document-specialist).
