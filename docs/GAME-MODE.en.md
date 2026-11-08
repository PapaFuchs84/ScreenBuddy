# Game Mode

[Deutsch](GAME-MODE.md) · **English**

Local development on `feature/game-mode`, not released. The [implementation plan and validation record](PLAN-GAME-MODE.md) identifies implemented sources, executed checks and remaining work. Published downloads and release versions are unchanged.

Game Mode detects a running game and shows a custom view on the selected ScreenBuddy display. An optional intro shows the game logo. When the game ends, the display returns to its previous page rotation.

## Setup

1. In **App → Custom views**, create a view or use the **Game Mode** template. Add game and system tiles, check their size and device layout, then save the view.
2. Open **Game Mode** in settings and enable it. Select the target display and saved view (`custom:<id>`).
3. Enable the intro if wanted. Use **Reduce animations** if scaling and the sheen effect are distracting.
4. Optionally enter a ping target as an IPv4 address. The default is `1.1.1.1`; use your router's IPv4 address to measure the local network. Ping measures reachability of that target, not latency to the game server.
5. Add extra games and exclusions as needed. Save and restart ScreenBuddy so background sources use the updated settings.

`primary` means ScreenBuddy's main output: the configured USB/monitor display or main network output. A Companion device ID identifies that paired device. Selecting an offline device does not turn it on or pair it again.

## Game detection

Steam games are mapped using the Steam registry and local library/app manifests. Epic uses local launcher manifests. Xbox/Game Pass games are mapped by their `XboxGames` installation paths. Generic detection checks the foreground/fullscreen window and membership in a known game library. An arbitrary fullscreen window alone does not count as a game.

The following local HKLM registry entries are also read (both `SOFTWARE\WOW6432Node` and native `SOFTWARE`):

| Library | Source and name |
| --- | --- |
| GOG Galaxy | `GOG.com\Games\<id>`: `gameName`, `path`, optional `exe`. |
| Ubisoft Connect | `Ubisoft\Launcher\Installs\<id>\InstallDir`; name from an uninstall entry with the same installation path, otherwise the folder name. |
| EA App/Origin | `EA Games\<Name>` or `Electronic Arts\<Name>`: `Install Dir`; also uninstall entries published by `Electronic Arts`/`EA` (including supported `Inc.` spellings) with `InstallLocation`. |
| Battle.net/Blizzard | Uninstall entries published by `Blizzard Entertainment` (including `Inc.`) with `InstallLocation`. Also `Blizzard Entertainment\Battle.net` or `Battle.net`, value `InstallPath`, or the Battle.net uninstall location: only named game folders inside and directly alongside that location. |
| Rockstar | `Rockstar Games\<Name>\InstallFolder`. |

Uninstall means `Microsoft\Windows\CurrentVersion\Uninstall\<entry>` in those HKLM views. Battle.net's folder list covers World of Warcraft, Diablo III/IV, StarCraft/II, Hearthstone, Heroes of the Storm, Overwatch and Warcraft III/Warcraft III Reforged. There is no recursive drive search, launcher account query or undocumented database access.

Starting detection for each new library requires a process beneath its registered installation path whose foreground window is fullscreen/borderless. Even an explicit GOG EXE cannot bypass this gate; Epic EXE matches now also use the window check. Steam keeps its existing `RunningAppID` shortcut; manually entered EXEs still match exactly without a window check when the process path is readable. When that path is unavailable, they also require the basename fallback with foreground/fullscreen evidence. A previously confirmed, unchanged running game process stays active through Alt-Tab. Libraries are refreshed approximately every 60 seconds.

Limits: missing/stale registry entries, per-user installations without a matching HKLM entry, other publisher names or different Battle.net game folders may prevent detection. Add the actual game EXE manually in that case. An unknown helper inside a game folder can count as a game if it has its own fullscreen foreground window; exclude it if needed. Path membership is lexical, without resolving junctions/symlinks. Non-Steam games use the EXE icon plus game name; if the icon is unavailable, the name remains. Real acceptance tests for these five launchers remain open.

**Additional games:** one complete game EXE path per line, for example `D:\Games\ExampleGame\Game.exe`. Do not enter commands, wildcards or directories. **Exclusions:** one EXE filename or complete EXE path per line. Browsers, Explorer, media players, known launchers, `Agent.exe`, Blizzard error/crash handlers and EXE names containing `Launcher` are excluded by default (case-insensitive). This also covers `EAAntiCheat.GameServiceLauncher.exe`, `BEService.exe` and `EasyAntiCheat*.exe`; add other helpers as needed.

Detection debounces changes: a game must remain detected for at least three seconds before Game Mode starts. Game Mode ends after five seconds without the previous game. Brief appearances or disappearances therefore do not immediately change the display. Playtime means the detected session, not total account playtime.

The states are `idle → starting → running → ending`. Unmapped processes result in missing data; an unavailable image path may be handled by the filename fallback below. ScreenBuddy does not request elevated access to game processes.

## Anti-cheat-protected games: detection by filename (P9-9)

Protection drivers such as EA Anti-Cheat/Javelin may hide a running game's image path. The Toolhelp process list still provides its PID and EXE basename without requiring a process handle. When the path is unavailable, ScreenBuddy matches that basename against the local library cache and requires the same process to own the foreground fullscreen window. A window title alone is insufficient. Ambiguous basenames across different installations are not assigned automatically.

Each library refresh (every 60 seconds) reads at most 2,000 entries per installation, through three nested directory levels, caching EXE files only. Reparse points/junctions are not followed; manual entries supply only the explicitly configured executable basename. This covers Steam, Epic, registry libraries and local `XboxGames` folders on fixed drives. Launchers, anti-cheat, installer, crash and helper processes stay excluded, especially `EAAntiCheat.GameService*.exe`. Both basename and full-path exclusions also apply to the fallback.

Example: **Battlefield 6**, Steam app **2807960**, can match `bf6.exe` with an unavailable image path and Steam RunningAppID `0` when started through the EA app. Its name and Steam app ID still come from the local appmanifest; the logo uses that Steam app ID. PID and basename retain the confirmed association through Alt-Tab. The 3/5-second debounce and PID-based ETW attribution remain in place; detection does not guarantee FPS data. No game-process permissions are changed, no process memory is read and no elevation is requested.

P9-9 validation: 26/26 suites passed, including 267 Game-Mode checks with simulated process lists and local filesystem fixtures. Live Battlefield 6/Javelin acceptance, physical display output and FPS validation remain pending. Results: [P9-9 validation](PLAN-GAME-MODE.md#p9-9-prozesse-ohne-lesbaren-image-pfad).

## Logos and intro

Steam app IDs use only the designated public Steam asset URLs: `logo.png` first, then header/hero as fallbacks. Neither a Steam Web API key nor a TMDB account is required. The local cache is inside the ScreenBuddy data directory and expires after seven days. When an asset is unavailable, the EXE icon and game name provide the fallback.

The normal intro lasts approximately 2.5 seconds: dimmed background, logo fade/scale, sheen and transition to the selected view. Reduced animation uses only an approximately 0.35-second transition, without scaling or sheen. The intro can also be disabled completely.

## Measurements and `n/a`

Game Mode tiles belong to the **System** catalog source; the **Game Mode** palette filter groups relevant tiles. Per-core load and frametime history need enough tile height and a density other than Compact.

Available values depend on GPU, driver, Windows permissions and enabled sources. `n/a` means there is no valid measurement; it does not mean zero. One unavailable sensor must not prevent the rest of Game Mode from working.

| Tile/source | Meaning and limitation |
| --- | --- |
| Game name, logo, playtime | Local detection and detected session; no account lookup. |
| GPU | NVIDIA: NVML dynamically loaded from System32, first adapter. AMD: dynamically loaded ADL Overdrive 5/6, only where supported by the driver. PDH GPU Engine provides the busiest engine as the load fallback. Temperature, clock, fan and power may be unavailable individually. |
| VRAM | NVML provides usage and capacity for the first NVIDIA adapter. Otherwise: PDH GPU Adapter Memory Dedicated Usage for usage, with DXGI `DedicatedVideoMemory` capacity summed across hardware adapters. Adapter-wide values, not exclusive game VRAM usage. |
| CPU | Per-logical-processor load through PDH Processor Information, clock through PDH and `CallNtPowerInformation`. Temperature requires a matching HWiNFO shared-memory source as before. |
| Ping/jitter | ICMP approximately once a second to the configured IPv4 target, with a 200 ms timeout. Jitter is the absolute difference between two consecutive successful round-trip times; after a failure, two successful replies are needed again. Blocked echoes, timeouts and invalid targets produce `n/a`. |
| RAM, network | Existing system sources; network values belong to the selected adapter. |
| FPS, frametime, 1% low, history | A dedicated ETW session for DXGI `Present_Start`, including MPO, attributed to the detected process ID and separated by swapchain. Measures DXGI Present calls, not frames actually displayed on the monitor. Details and limits below. |

FPS uses the most active swapchain and ignores `DXGI_PRESENT_TEST` calls. The measurement window is limited to ten seconds and 1,200 intervals. Frametime is the average interval between Present calls; 1% low requires at least 100 intervals and uses the average of the slowest 1%. Values become invalid after two seconds without new data.

This is not a complete PresentMon/DxgKrnl collector or a scanout FPS measurement. Vulkan, OpenGL and D3D9 without DXGI events remain `n/a`; missing permissions, missing events or lost events can also result in `n/a`. Real-time ETW sessions may require administrator privileges or the Windows **Performance Log Users** group. Elevated privileges do not guarantee measurements. Do not change game-process permissions.

**FPS without administrator:** click “Enable FPS measurement” in the Game-Mode settings. Windows shows one User Account Control (UAC) prompt; a short helper call of ScreenBuddy's own EXE (`--grant-perflog`) adds the current user to the “Performance Log Users” group, and ScreenBuddy never keeps running as administrator. Then **sign out and back in once** (Windows only applies new group memberships at a new sign-in) and restart ScreenBuddy. The card shows the status: active, inactive or “please sign out/in”. Undo: `net localgroup "Performance Log Users" <user> /delete`. The status is based on a real test (creating a trace session), not just on the group. **Known limit:** on some Windows builds (tested: Windows 11 build 26300) Windows refuses ETW sessions even when the group is in the token (`logman` fails too). The card then shows “Windows refuses the measurement”. **BF6 / EA anti-cheat (verified live):** the anti-cheat refuses creating new ETW sessions while the game runs (even as administrator). ScreenBuddy therefore creates the session at start-up when Game-Mode is on; **start ScreenBuddy before the game** and FPS/frametime are measured (confirmed live with BF6). As a fallback, if ETW fails and HWiNFO is running, ScreenBuddy reads HWiNFO's “PresentMon” sensor group (Framerate Presented, 1% low, Frame Time), read-only from HWiNFO's shared memory; only values of the “PresentMon [<game>.exe]” group of the detected game process are used. **Limit (verified live):** HWiNFO tracks a single process; in our BF6 test it followed “PresentMon [chrome.exe]”, so the display correctly stayed `n/a` instead of showing wrong values. The fallback therefore only helps when HWiNFO tracks exactly that game. `cpp/tools/Grant-PerfLogUsers.bat` remains as a fallback without the app dialog (right-click “Run as administrator”). Also not guaranteed: UWP/Store games, streaming/cloud games and processes that do not present themselves; those stay `n/a`. Leftovers of crashed ScreenBuddy sessions (`ScreenBuddy.Game.<PID>.<tick>`) are stopped on the next start. No PresentMon code is used; only the same public ETW interface is read.

ADLX is not integrated; AMD values are limited to ADL Overdrive 5/6 queries supported by the driver. DXGI `QueryVideoMemoryInfo` is deliberately not used as game VRAM consumption: that query would describe the calling ScreenBuddy process.

## Privacy and permitted interfaces

The implementation uses documented Windows/vendor interfaces only: registry, local launcher manifests, process/window information, PDH, ETW/DXGI, dynamically loaded NVML or AMD ADL, and ICMP. No hooking, DLL injection or reading foreign game-process memory. No libre-panel code is reused; the [clean-room rules](CLEAN-ROOM-LAYOUT.md) apply.

Steam CDN requests transmit the requested app ID and, like ordinary HTTPS requests, the IP address. Ping sends ICMP packets to the configured target. Local EXE paths and launcher manifests are read for detection and must be kept out of published screenshots or bug reports.

## Troubleshooting

- **No switch:** Check activation, view and target display; save changes and restart. Allow at least three seconds of stable detection. Check the EXE path and exclusions.
- **Protected game not detected:** Bring the game to the foreground in fullscreen/borderless mode, check its library entry and exclusions, and allow the next library refresh (approximately 60 seconds). Multiple installations sharing an EXE basename may remain ambiguous.
- **Wrong program detected:** Configure the actual game path rather than its launcher and exclude helper processes.
- **Missing logo:** Check internet/CDN availability; the EXE icon/name remain the fallback. A missing logo does not block the view.
- **One metric is `n/a`:** Check source support, driver and HWiNFO where applicable. Use a reachable IPv4 ping target. Never substitute invented measurements.
- **Missing view:** Save it in the editor and check its `custom:<id>` reference. Select another view if the original was deleted.

## Developer tests and validation limits

The app EXE includes its installer. Start it for tests only through an isolation script: `FUCHSLABS_INSTANCE`, a dedicated data directory, UI port **18766**, AI receiver **18765**, and no requests to **8765**. The template is `ScreenBuddy-2.10.0-test\Start-ScreenBuddy-Test.ps1`; update existing isolated configuration to the separate ports because a script parameter alone does not rewrite it. No audible test sounds.

Unit tests check the logic; a dummy-process simulation checks isolated app integration. Such checks do not establish real game detection across all launchers, physical display/touch output or hardware measurements. Executed checks, results, screenshots and pending acceptance are recorded only in the [current validation record](PLAN-GAME-MODE.md#p9-6-tests-und-doku).

Validation on 2026-11-07: 26 automated suites and the isolated dummy simulation passed. The project launcher is `cpp/tests/Start-GameMode-Test.ps1`; run `python cpp/tests/game_mode_integration.py`. UI 18766; AI stayed disabled because 18765 was occupied. Preview images remain local under `cpp/build/game-mode-test/`. The plan records full validation and pending real-game/hardware acceptance.

P9-7: The five additional registry sources and exclusions are covered by synthetic unit tests (196 Game-Mode checks in total); no real launcher/game or hardware acceptance was performed for this change. Current full-suite result and limits: [P9-7 validation](PLAN-GAME-MODE.md#p9-7-weitere-spielebibliotheken).

## P9-8

While Game-Mode is enabled, its selected view is reserved on all outputs: normal rotation, navigation, Companion page lists and page-tap destinations exclude it. Only the Game-Mode path displays it. Disabling Game-Mode makes it a normal page again.

Non-Steam games (including GOG/Ubisoft) use the running executable's directly extracted icon at up to 256 pixels. A matching `Application Executable` in a nearby `AppxManifest.xml` first supplies package logos, including size variants; fallback order is EXE icon, shell icon and name. Missing permissions or unresolved package resources remain fallback cases; this does not extend game detection.

The new template has logo, name and session time above nine metrics in three columns. Existing saved layouts are preserved; create a new view from the template to adopt it. Game tiles including CPU/GPU share cards, corner radii, orange titles and larger values. FPS/frametime show up to 60 one-second samples when tile height and density above Compact allow it. Invalid samples leave gaps; process changes clear the history. The ten-second ETW statistics window is unchanged.

**Cockpit design:** small, grey, upper-case labels, an orange accent stripe on the left, units small in orange next to the value. An FPS tile of roughly 100 height or more becomes the hero tile (orange frame, very large number, filled history). CPU/GPU appear as a ring instead of a bar in tiles that are not too wide; bars and rings switch to yellow from 80% and red from 90% (fixed signal colours). A tile background gradient is not implemented because the renderer lacks alpha fills.

The intro is composed into the regular output frame, stored in `last_frame()` and visible through `/api/preview.png`. The isolated port-18766 test checks distinct intro frames and the subsequent view frame. This proves the software path, not physical display output.

P9-8 validation: 26/26 suites (207 Game-Mode checks) and isolated dummy/intro integration on 18766 passed. Real launcher logos and physical displays were not tested; see the [validation record](PLAN-GAME-MODE.md#p9-8-rotation-logos-und-spielkacheln).
