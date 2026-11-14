# Hardware HUD – local test build v3

[Deutsch](HARDWARE-HUD-TEST.md)

Local development dated 14 November 2026, based on ScreenBuddy 2.11.0. No publication to GitHub or the Microsoft Store; no new public version. A local development EXE may be unsigned.

## Changes in v3

The local package is named `ScreenBuddy-Hardware-HUD-Test-v3.zip`; the display is marked `TEST3`. The executable keeps the filename `ScreenBuddy-Hardware-HUD-Test.exe`, so the separate start/stop helpers continue to work. Extract v3 into its own folder and use the launcher in that folder.

Hardware names wrap using measured text widths inside bounded areas. The 1200 × 360 pixel preview is the reference format. Rings and fans move more visibly, signal pulses travel along the circuit lines and page transitions are animated. This motion remains decorative and does not replace sensor readings.

Animation rate is now global under **Display → Animations**, configurable from **5 to 60** (`display.animation_fps`), default **30**. It applies to visible motion on monitors and Companion: Hardware HUD, active AI states and row transitions, playing Spotify progress/meters, and visible native, custom-view, Game Intro and plugin transitions. Static views retain normal source/display cadence and the normal Companion 5 frames/s transport cap. The old `system.hardware_hud_fps` key is a migration alias only; an explicitly configured `display.animation_fps` takes precedence.

The value is a target/resource hint, not guaranteed delivery. Latest-frame storage, version deduplication, synchronous backpressure and bounded shutdown remain. Android app, wire format and sensor polling do not change. Global app/device acceptance and mixed fast/static-output checks remain pending. Before the added separation, static Companion pages could be redrawn unnecessarily with a fast primary output; the new guard against that still needs validation.

The previous 5 frames/s cap is not an Android/protocol requirement. The earlier HUD-only transport candidate measured 16.06/21.31/31.38 UI redraws/s on the physical Redmi at targets 20/30/60. 30 is the chosen balance; 60 was the smoothest measured setting but used about 26.2 rather than 17.5 Mbit/s TLS traffic and more CPU. This does not establish achieved 60 frames/s or a hardware maximum. [Full measurements and limits](HARDWARE-HUD-FPS-ANALYSE.en.md).

## Usage

For the prepared local test, double-click `local-test/Start-Hardware-HUD.cmd`. The launcher uses a fresh `local-test/config.toml`, its own data directory and a separate `HardwareHudTest` instance. The HUD is already enabled and opens in a 1200 × 360 pixel preview window. Installation is not required.

To exit, double-click `local-test/Stop-Hardware-HUD.cmd`. Before sending the local shutdown request, the helper checks that the listener on port 18769 belongs to the exact `ScreenBuddy-Hardware-HUD-Test.exe` path in this test folder. A different executable is never stopped. If the managed test environment does not expose port ownership, the helper directly terminates only processes whose executable path exactly matches this test EXE; that fallback is not a graceful application shutdown. Alternatively choose **Exit** in this test instance's tray menu.

The following setting applies when configuring this test EXE manually:

In settings under **Pages → System**, enable the system page and **Hardware HUD**, then save. The system view uses a dark cyan/magenta display with original vector graphics. Overview, GPU and CPU cycle every eight seconds by default. Other enabled pages may appear through the normal page cycle; select only the system page for a continuous HUD.

The view adapts to wide, regular and portrait output sizes. Disable **Hardware HUD** to restore the regular system view. The HUD uses its own colour scheme.

The prepared launcher limits output to the preview window. For a physical monitor, choose the appropriate output in a separate test configuration and remove `FUCHSLABS_TEST_RECT` from the launcher. Existing personal ScreenBuddy configuration is not copied.

## Measurements

- CPU load and clock come from Windows. CPU temperature, CPU fan speed and CPU package power need running HWiNFO with shared-memory output enabled and suitable sensors.
- CPU watts are accepted only from a power reading in a CPU sensor group, with unit `W` and a recognised original package/PPT name. User-renamed HWiNFO readings retain support through their original English labels. Supported original names: `CPU Package Power`, `CPU Package Power (SMU)`, `CPU Package`, `CPU PPT`, `CPU Power`. Unknown sensor labels remain unavailable.
- NVIDIA GPU data use the existing NVML integration. Availability depends on the driver and model. Other GPUs may provide fewer measurements.
- Missing or invalid measurements stay `--`, with no invented replacements. Vector drawings are schematic. Animated graphics are decorative, not an independent measurement of chassis fan speeds.
- Hardware measurements retain the existing sampling cadence; graphics animate separately. Additional individually selectable chassis fans are outside this test build.

## Output and validation limits

**Current global scope:** app/device checks for all visible animation, mixed static outputs and the revised native USB output remain pending. The passing HUD checks below belong to older artifacts and do not establish global acceptance.

The physical native Rev-A USB display (480 × 320) has so far accepted controller writes only, without optical readback: source target 5 yielded about 1.65 show calls/s, while 30 and 60 each yielded about 2.27/s; mean USB show time was 428–540 ms. A partial-transfer rate was approximately 170 kB/s. Full-frame 30/60 frames/s is not established on this path. The additional fixed HUD one-second pause is being removed; native animation follows actual transfer time with a short idle pause. Actual global app cases and plugin transition budgets still need testing. During the isolated raw-transfer test, the normal Windows instance was briefly paused, then restored with confirmed display status.

Synthetic renderer/transport examples for Spotify, games and other pages do not establish real Spotify-account/control or gameplay acceptance.

An HDMI/USB-C display recognised as a Windows monitor uses the existing monitor output. Supported serial displays and Companion outputs use their existing output paths. The reference video's display model and protocol are unknown; support for that specific device has not been established.

The previous HUD-only v3 build with default 30 passed 88 HUD and 3556 regression checks. All 434 Companion checks passed on the same transport code, including unchanged normal-page pacing at 5 frames/s and bounded shutdown with a non-reading client (0.44 seconds in the test). The final physical Redmi smoke test of that exact build passed: 36.41 seconds at 1520 × 720 landscape and 35.80 seconds at 720 × 1520 portrait, twelve device captures each plus an additional 34-second device video. All three pages showed live values, automatic cycling and visible movement without observed overlaps; the connection stayed active. The test configuration deliberately omitted `hardware_hud_fps` and the API confirmed the compiled default of 30. This is functional acceptance, not a new frame-rate measurement. Afterwards the host and forwarding were stopped, preferences restored and normal Companion reopened with pairing preserved.

Local device report: [final smoke test](redmi-validation/test3-final-30fps-smoke/FINAL-DEVICE-RESULT.md).

The following older validation belongs to the previous v2 test build and is not a current v3 monitor-rate measurement. The local Windows v2 test build completed successfully. All 79 targeted HUD checks and 3556 existing regression checks passed; the AI subset contains 300 passing checks. HUD checks include 1200 × 360, 1520 × 720 and 720 × 1520 pixel outputs. The German preview was inspected across all three HUD pages at 1200 × 360 and in landscape/portrait formats without visible text collisions.

The actual isolated 1200 × 360 monitor run recorded 83 rendered frames over 5.113 seconds, corresponding to 16.04 frames/s. All twelve additional sampled preview frames differed. This is a short run on the test PC, not proof of sustained 20 frames/s. Graceful shutdown through the local API and subsequent process exit were confirmed.

Physical Redmi 8 testing on Android 14 passed in the foreground with the screen awake: all three pages at 1520 × 720 landscape (nine captures over 31.7 seconds plus a 26-second device video) and 720 × 1520 portrait (eight captures over approximately 28 seconds). Live PC values, automatic eight-second page cycling and moving rings, fans and circuit pulses were visible; no overlaps were observed. Missing GPU watts remained `--`.

The test used a separate debug app and USB ADB reverse with a temporary local relay. Production host access policy was unchanged. This does not establish a passing LAN connection. The original v2 test did not measure phone frame rate. A fresh Redmi retest without screen recording recorded 49 UI redraws over 12.093 seconds, equivalent to 4.05/s, and exposed the previous global 5 frames/s transport cap. This does not establish an achieved 10 frames/s. Background, reboot and battery behaviour were not accepted. Afterwards the test app, host and forwarding were stopped and the normal Companion app reopened; its pairing and data were preserved.

Full physical acceptance across all display sizes, sensors and connected devices remains outstanding. Personal configurations, credentials and logs must not be included in the test package. Package validation is also recorded in `local-test/PRUEFSTATUS.txt`; the local device report is [redmi-validation/REDMI-TESTRESULT.md](redmi-validation/REDMI-TESTRESULT.md).
