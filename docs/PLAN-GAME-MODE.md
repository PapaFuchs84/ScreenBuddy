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

## Umsetzungsstand (2026-11-07)

| Phase | Ergebnis |
|---|---|
| P9-1 | `game_detect.*`: begrenzte Steam-VDF/ACF- und Epic-Parser (einschließlich leerer VDF-Felder), Bibliotheks-/EXE-Zuordnung, XboxGames auf festen Laufwerken, Vollbild-/Fensterprüfung, Ausschlüsse, 3-s-Start/5-s-Ende-Zustandsautomat. Nur `PROCESS_QUERY_LIMITED_INFORMATION`; Alt-Tab beendet ein bestätigtes, weiter laufendes Spiel nicht. |
| P9-2 | `game_logo.*`: feste Steam-CDN-URLs, PNG/Header/Hero-Fallback, begrenzte Antworten/Bildgrößen, lokaler Cache mit 7-Tage-Ablauf und Reparse-Abweisung, lokales EXE-Icon plus Name. Laden asynchron, sofortiger lokaler Icon-Ersatz. Kein Scraping/TMDB. |
| P9-3 | `game_intro.*` und `app.cpp`: 2,5 s mit Abdunklung/Easing/Glanz und vorhandenem `FadeCache`/`blend_rect`; reduziert 0,35 s ohne Skalierung/Glanz. Displaysperrzonen berücksichtigt, Touch-Ziele während des Intros gesperrt. Größenwechsel beendet die laufende Animation. |
| P9-4 | 16 additive System-Einzelkacheln, bestehende RAM-/Netz-/GPU-/CPU-Karten bleiben verfügbar. Eigene DXGI-ETW-Sitzung mit PID-/Swapchain-Filter und TEST-Ausschluss, aktivste Kette nach ungekapptem Intervallzähler, 10-s-/1.200-Intervall-Fenster, 1%-Low erst ab 100 Intervallen, n/a nach 2 s ohne Present. NVML und AMD-ADL aus System32, PDH-Fallback, CPU-Kernlast/Takt, ICMP/VRAM. Standardseiten unverändert. |
| P9-5 | `game_runtime.*`, `config.cpp`, native Game-Mode-Seite und Editor: Aktivierung, Ziel, `custom:<id>`, Intro/reduziert, IPv4-Ping, mehrzeilige zusätzliche EXE-Pfade/Ausschlüsse. Vorlage **+ Game-Mode** und Palettenfilter. Worker und Logos blockieren den Render-Thread nicht. Display-Override erhält normale Seitenliste/Position/Restlaufzeit, Rückkehr beim Spielende; fehlende Ansicht lässt normale Rotation bestehen. |
| P9-6 | Neue Unit-/UI-Prüfungen, wiederholbare isolierte Start-/Testskripte, Dummy-Prozess-Simulation und Vorschaubilder; DE/EN-Dokumentation, Referenzlinks, Changelogs und Anleitungsvorlagen/Ressourcen synchronisiert. |

Bedienung und Quellenbeschreibung: [Deutsch](GAME-MODE.md), [English](GAME-MODE.en.md). Einstellungen gelten nach Speichern/Neustart. Zusätzliche Spiele/Ausschlüsse: maximal 128 Zeilen/32 KiB; zusätzliche Spiele verlangen absolute EXE-Pfade, keine Befehle/Platzhalter. Cacheablauf betrifft die Wiederverwendung von Dateien, keine automatische Komplettbereinigung des Cacheverzeichnisses.

## Nicht umgesetzt

- Vollständige PresentMon-/DxgKrnl-Auswertung für Scanout-/Display-FPS sowie D3D9/OpenGL/Vulkan ohne DXGI: der Sammler misst ausdrücklich **DXGI-Present-Aufrufrate**, keine angezeigten Frames. Nicht unterstützte Ereignisse bleiben n/a. Ohne passende ETW-Rechte ebenfalls n/a; keine automatische Rechteerhöhung. Gegebenenfalls Mitgliedschaft in Performance Log Users administrativ einrichten, dann neu starten.
- ADLX ist nicht zusätzlich integriert: AMD verwendet dokumentierte ADL-OD5-/OD6-Leseaufrufe, soweit der Treiber diese anbietet. Nicht unterstützte Sensoren bleiben n/a. Keine automatische Auswahl der vom Spiel benutzten GPU; NVML nutzt den ersten verfügbaren NVIDIA-Adapter, AMD den ersten aktiven ADL-Adapter. PDH-Last ist der höchste gemeldete Engine-Wert.
- `QueryVideoMemoryInfo` wird nicht als globaler VRAM-Verbrauch ausgegeben: sie meldet Prozessbudget/-verbrauch des Aufrufers. Ersatz: NVML für ersten NVIDIA-Adapter, sonst PDH Dedicated Usage plus DXGI DedicatedVideoMemory über Adapter aggregiert. Keine Zuordnung dieser VRAM-Werte zum Spielprozess.
- Kein Nachweis realer Steam-/Epic-/Xbox-Spielstarts, geschützter Spielprozesse oder vollständiger Hersteller-/Treiberabdeckung. Kein physischer Display-/Touch-/Mehrgeräte-Test und keine reale ETW-FPS-/GPU-Sensor-Abnahme. Fehlende Bibliotheks-/Prozessrechte können Erkennung verhindern; zusätzliche EXE-Pfade sind der manuelle Rückfall.

## P9-6 Prüfstand (2026-11-07)

| Prüfung | Ergebnis und Grenze |
|---|---|
| CMake/vcpkg, MSVC x64 Release | Gesamter vorhandener CMake-Baum gebaut, inklusive ScreenBuddy und Testprogrammen. Bestehender vcpkg-Abhängigkeitscache nur lesend verwendet. Lokaler Entwicklungsbuild, keine Auslieferung/Signierung, kein Push. |
| Automatisierte Suiten | **26/26 bestanden** über `python cpp/tests/game_mode_checks.py`: 24 C++-Prüfprogramme einschließlich Game-/Custom-View-/Katalog-/UI-/Companion-/Snapshot-Prüfungen, Standalone-Distribution und synthetischer KI-Adapter. `fl-game-tests`: **114**; Custom views **2.101**; Katalog **313**; Layout-UI **1.866**; `fl-tests` **3.531**, jeweils ohne Fehler. |
| Bildreferenzen | Bestehende Standardseiten-/Plugin-Snapshots grün. Historische P8-Hashdateien unverändert, beabsichtigte Editoränderungen und neue Game-Mode-Form/Palette als eigene `p9-*-hashes.txt`. DE/EN-Widgetbilder mit Beispieldaten; keine reale Desktop-/Hardware-Abnahme. |
| Isolierte App-Simulation | `python cpp/tests/game_mode_integration.py`: App ausschließlich über `cpp/tests/Start-GameMode-Test.ps1`, `FUCHSLABS_INSTANCE`, eigenes Datenverzeichnis, `--run --no-lcd`, UI **18766**. Stummer eigener Dummy-Prozess: Start-/Ende-Entprellung, Logo-Intro/EXE-Icon, Game-Ansicht und Rückkehr zur vorherigen Rotation bestanden. |
| Portgrenze | Kein Testzugriff auf **8765**. KI-Port **18765** war belegt; der für Game-Mode nicht benötigte KI-Empfänger blieb in der App-Simulation ausgeschaltet. Keine fremde Instanz beendet oder umkonfiguriert. Das Startskript erlaubt KI nur mit explizitem `-AiReceiver` und freiem 18765. |
| Dokumentation | Lokale Links der betroffenen READMEs/Referenzen/Bedienungsdokumente geprüft; JavaScript-Syntax aller vier DE/EN-Anleitungsvorlagen/Ressourcen mit `node --check` bestanden. Geprüfte eingebettete Bilder unverändert. `git diff --check` sauber. |
| Nicht ausgeführte Abläufe | Bestehende Installer-/App-Skripte, die die EXE direkt starten oder installieren, wurden gemäß Aufgabenregel nicht ausgeführt. Keine Veröffentlichung, kein Store-/Installer-/Upgrade-Test; physische Prüfungen siehe oben. |

Lokale, nicht eingecheckte Artefakte: `cpp/build/game-mode-checks/results.json` und einzelne Prüfausgaben; `cpp/build/game-mode-test/result.json`, `intro.png`, `active.png`, `restored.png`; synthetische native Widgetbilder unter `cpp/build/screenbuddy-release/p9-ui-snapshots/`. Konfigurationen, DPAPI-Daten und Logs bleiben lokal/ignoriert. Keine hörbaren Testtöne.

## Schnittstellenbelege

- [Windows Prozessrechte](https://learn.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights), [Vollbildstatus](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shqueryusernotificationstate).
- [NVML Device Queries](https://docs.nvidia.com/deploy/nvml-api/group__nvmlDeviceQueries.html), [AMD ADL OD5](https://gpuopen-librariesandsdks.github.io/adl/group__OVERDRIVE5API.html), [ADL AdapterInfo](https://gpuopen-librariesandsdks.github.io/adl/structAdapterInfo.html).
- [DXGI-Eventmetadaten, PresentMon/MIT](https://github.com/GameTechDev/PresentMon/blob/main/PresentData/ETW/Microsoft_Windows_DXGI.h), [ICMP](https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho). Keine libre-panel-Quellen eingesehen/verwendet.
