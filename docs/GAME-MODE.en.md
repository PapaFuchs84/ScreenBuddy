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

**Additional games:** one complete game EXE path per line, for example `D:\Games\ExampleGame\Game.exe`. Do not enter commands, wildcards or directories. **Exclusions:** one EXE filename or complete EXE path per line. Browsers, Explorer and media players are excluded by default; add other launchers/helpers as needed.

Detection debounces changes: a game must remain detected for at least three seconds before Game Mode starts. Game Mode ends after five seconds without the previous game. Brief appearances or disappearances therefore do not immediately change the display. Playtime means the detected session, not total account playtime.

The states are `idle → starting → running → ending`. Missing read permissions and unmapped processes result in missing data. ScreenBuddy does not request elevated access to game processes.

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

ADLX is not integrated; AMD values are limited to ADL Overdrive 5/6 queries supported by the driver. DXGI `QueryVideoMemoryInfo` is deliberately not used as game VRAM consumption: that query would describe the calling ScreenBuddy process.

## Privacy and permitted interfaces

The implementation uses documented Windows/vendor interfaces only: registry, local launcher manifests, process/window information, PDH, ETW/DXGI, dynamically loaded NVML or AMD ADL, and ICMP. No hooking, DLL injection or reading foreign game-process memory. No libre-panel code is reused; the [clean-room rules](CLEAN-ROOM-LAYOUT.md) apply.

Steam CDN requests transmit the requested app ID and, like ordinary HTTPS requests, the IP address. Ping sends ICMP packets to the configured target. Local EXE paths and launcher manifests are read for detection and must be kept out of published screenshots or bug reports.

## Troubleshooting

- **No switch:** Check activation, view and target display; save changes and restart. Allow at least three seconds of stable detection. Check the EXE path and exclusions.
- **Wrong program detected:** Configure the actual game path rather than its launcher and exclude helper processes.
- **Missing logo:** Check internet/CDN availability; the EXE icon/name remain the fallback. A missing logo does not block the view.
- **One metric is `n/a`:** Check source support, driver and HWiNFO where applicable. Use a reachable IPv4 ping target. Never substitute invented measurements.
- **Missing view:** Save it in the editor and check its `custom:<id>` reference. Select another view if the original was deleted.

## Developer tests and validation limits

The app EXE includes its installer. Start it for tests only through an isolation script: `FUCHSLABS_INSTANCE`, a dedicated data directory, UI port **18766**, AI receiver **18765**, and no requests to **8765**. The template is `ScreenBuddy-2.10.0-test\Start-ScreenBuddy-Test.ps1`; update existing isolated configuration to the separate ports because a script parameter alone does not rewrite it. No audible test sounds.

Unit tests check the logic; a dummy-process simulation checks isolated app integration. Such checks do not establish real game detection across all launchers, physical display/touch output or hardware measurements. Executed checks, results, screenshots and pending acceptance are recorded only in the [current validation record](PLAN-GAME-MODE.md#p9-6-tests-und-doku).
