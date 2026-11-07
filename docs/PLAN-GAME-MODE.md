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

## Umsetzungsstand

P9-1: Erkennung und reine Parser implementiert; automatisierte Build-/Testprüfung läuft im Arbeitsgang. Keine Live-Spiel-/Hardwarebestätigung.

P9-1 geprüft: Release-Build und 25 Offline-Prüfungen bestanden. P9-2: Logo-Cache implementiert; feste Steam-URLs, 7 Tage Ablauf, Größenlimit, Reparse-Punkt-Abweisung, EXE-Icon/Name als Fallback.

P9-2 geprüft: 38 Offline-Prüfungen bestanden. P9-3: Intro-Renderer implementiert (2,5 s; reduziert 0,35 s), bestehender `FadeCache`/`blend_rect`. App-Anbindung in P9-5.

P9-3 geprüft: 47 Offline-Prüfungen bestanden. P9-4 implementiert: 16 additive System-Kacheln, DXGI-ETW mit PID-/Swapchain-Filter, TEST-Ausschluss, begrenztem Verlauf, 1%-Low erst ab 100 Intervallen und n/a nach 2 s ohne Present. NVML/AMD ADL dynamisch aus System32; PDH-Fallback, CPU-Kernlast/Takt, ICMP (200 ms Timeout, IPv4, Jitter = absolute RTT-Differenz), VRAM via NVML bzw. PDH/DXGI-Kapazität. Standardseiten unverändert.

## Nicht umgesetzt

- Vollständige PresentMon-/DxgKrnl-Present-Auswertung für Scanout-/Display-FPS, D3D9/OpenGL/Vulkan ohne DXGI: der vorhandene Sammler misst ausdrücklich **DXGI-Present-Aufrufrate**, keine angezeigten Frames. Nicht unterstützte Ereignisse ergeben n/a. Ohne passende Rechte bleibt ETW n/a; keine automatische Rechteerhöhung. Fehlende Performance-Log-Users-Rechte ggf. administrativ einrichten, dann neu starten.
- ADLX ist nicht zusätzlich integriert: AMD wird über dokumentierte ADL-OD5-/OD6-Leseaufrufe unterstützt, soweit vom Treiber angeboten. Nicht unterstützte einzelne Sensoren bleiben n/a. Kein Treiber-/Hardwareversprechen ohne physische Prüfung.
- `QueryVideoMemoryInfo` wird bewusst nicht als globaler VRAM-Verbrauch verwendet: die API meldet Prozessbudget/-verbrauch des Aufrufers. Ersatz: NVML für ersten NVIDIA-Adapter, sonst PDH Dedicated Usage plus DXGI DedicatedVideoMemory (über Adapter aggregiert). Keine Zuordnung dieser Werte zum Spielprozess oder automatische Auswahl der vom Spiel benutzten GPU.

## Schnittstellenbelege

- [Windows Prozessrechte](https://learn.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights), [Vollbildstatus](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shqueryusernotificationstate).
- [NVML Device Queries](https://docs.nvidia.com/deploy/nvml-api/group__nvmlDeviceQueries.html), [AMD ADL OD5](https://gpuopen-librariesandsdks.github.io/adl/group__OVERDRIVE5API.html), [ADL AdapterInfo](https://gpuopen-librariesandsdks.github.io/adl/structAdapterInfo.html).
- [DXGI-Eventmetadaten, PresentMon/MIT](https://github.com/GameTechDev/PresentMon/blob/main/PresentData/ETW/Microsoft_Windows_DXGI.h), [ICMP](https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho). Keine libre-panel-Quellen eingesehen/verwendet.


P9-4 geprüft: 105 Game-Prüfungen und 313 Katalogprüfungen bestanden. P9-5: Einstellungen/Validierung, Editor-Vorlage und Filter, asynchroner Logo-Worker, gezielter Display-Override samt Sperrzonen/Touch-Sperre während Intro und Wiederaufnahme der vorherigen Rotation implementiert. Keine Änderung der gespeicherten normalen Seitenliste. Zusätzliche Spiele/Ausschlüsse sind begrenzte mehrzeilige Textfelder, exakte EXE-Pfade. Einstellungen gelten nach Speichern/Neustart. DE/EN-Bedienung: [Deutsch](GAME-MODE.md), [English](GAME-MODE.en.md); Anleitungsvorlagen und eingebettete Ressourcen synchron.

P9-5 geprüft: Release-Entwicklungsbuild, 114 Game-Unit-Prüfungen und 2.101 Custom-View-Prüfungen bestanden. Native Einstellungen und Game-Palette mit Beispieldaten DE/EN im vorhandenen UI-Testweg gerendert; keine physische Abnahme. Die alten P8-Referenzdateien bleiben unverändert, die beabsichtigten Editoränderungen haben eigene P9-Hashes. Isolierte Dummy-Prozess-Simulation über `cpp/tests/Start-GameMode-Test.ps1`: 3-s-Start/5-s-Ende-Entprellung, Intro/EXE-Icon und Rückkehr zur normalen Rotation bestanden. UI 18766; KI-Empfänger bleibt ausgeschaltet, da 18765 bereits belegt ist. Hauptinstanz nicht verändert. Abschließender Gesamttest folgt in P9-6.
