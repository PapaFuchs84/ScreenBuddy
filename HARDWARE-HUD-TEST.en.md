# Hardware HUD

[Deutsch](HARDWARE-HUD-TEST.md)

Since ScreenBuddy 2.12.0 (14 November 2026), the Hardware HUD is an optional presentation of the System page: a dark neon display in cyan and magenta with original vector graphics for the PC, graphics card and processor. It is off by default; the regular system view remains available unchanged.

## Enabling

1. Open settings and enable the system page under **Pages → System**.
2. Turn on **Hardware HUD** there and save.

The System page then shows the HUD. Turn **Hardware HUD** off again to return to the regular view. Other enabled pages still appear through the normal page cycle; select only the system page for a continuous HUD. The HUD uses its own colour scheme.

## Pages

By default the HUD rotates automatically between three pages every eight seconds:

- **Overview:** the key CPU and GPU readings at a glance.
- **GPU:** graphics card readings with ring and fan graphics.
- **CPU:** load, clock, temperature, fan speed and package power in watts, where available.

Rings, fans and signal pulses are animated, as are page transitions. The motion is decorative: the drawings are schematic and not an independent measurement, for example of chassis fans.

There are separate layouts for wide, regular and portrait displays. Hardware names wrap using their measured text width so labels do not overlap.

## Colours

Under **Pages → System → Hardware HUD → Colours**:

- **Design** (default): the HUD uses the colours of the active design (skin): background, accent and header colour, text and warning colour. Light designs give a light HUD.
- **Neon**: the classic cyan and magenta look on dark blue.
- **Custom**: primary colour, secondary colour and background of your choice (colour picker or #RRGGBB). An empty field keeps the design value.

Colours with too little contrast are adjusted automatically for readability; text always stays legible. The warning colour from 85 °C comes from the design (red for Neon).

## Measurements and HWiNFO

- CPU load and clock come from Windows.
- CPU temperature, CPU fan speed and **CPU package power in watts** need a running HWiNFO with shared-memory output enabled and suitable sensors.
- CPU watts are accepted only from a power reading (type Power, unit `W`) in a CPU sensor group with a recognised original package/PPT name: `CPU Package Power`, `CPU Package Power (SMU)`, `CPU Package`, `CPU PPT`, `CPU Power`. Readings renamed in HWiNFO keep working through their original English label. Unknown sensor labels remain unavailable.
- NVIDIA GPU data use the existing NVML integration. Availability depends on the driver and model; other GPUs may provide fewer measurements.
- Missing or invalid measurements stay `--`. Nothing is estimated or invented.
- Additional individually selectable chassis fans are not included.

## Animation rate

Under **Settings → Display → Animations** the global animation rate can be set from **5 to 60** (`display.animation_fps`), default **30**. It applies to visible animations: the Hardware HUD, active AI pulses and row transitions, the playing Spotify page with level/progress tiles, animated custom views, the Game intro and plugin fades.

- The value is a target, not a guaranteed frame rate. Higher values cost more CPU and, with the Companion, more network traffic.
- Static views keep their own slow refresh.
- Sensors are not polled more often; only the graphics move more smoothly.
- A former local key `system.hardware_hud_fps` is migrated automatically; an explicitly configured global value takes precedence.

## Output devices

- **Windows monitor (HDMI/USB-C):** uses the normal monitor output.
- **Android Companion:** PC-paced animations get no second transport delay. Normal static Companion pages stay limited to 5 frames/s and are not redrawn merely because the main display animates quickly. The protocol and Android app are unchanged; Companion 1.1.0 remains current.
- **Serial USB display (Rev. A, 480 × 320):** transfers run in their own output thread and always hand over the latest frame. Transfer time depends on the image content: in the integrated 2.12.0 HUD test a HUD transfer took about 1.7 s (≈ 0.6 frames/s), while the normal system page reached about 1.9 updates/s; earlier partial-transfer measurements are in the [analysis page](HARDWARE-HUD-FPS-ANALYSE.en.md). The animation is therefore paced by the measured transfer time and does not reach the configured target rate on this display. A slow USB transfer no longer blocks Companion rendering.

## Limits and test status

**Test status 2.12.0 (integrated build):** 25 C++ test suites without failures, including 3556 core regression, 1877 layout UI, 4221 built-in snapshots with hit maps, 2101 plugin snapshots, 448 Companion (including the USB output thread), 152 Hardware HUD, 2144 custom-view, 845 plugin-scheduling and 290 game checks. Isolated integration: installer preservation (2), app Companion integration (40) and standalone distribution passed. In addition, ten Codex review rounds (Windows app, Companion app, documentation); all release-relevant findings are fixed.


**Physically tested:**

- A real Redmi 8 with Android 14 as Companion over a USB tunnel at targets 5/30/60: about 4.9/21.3/26.2 UI redraws/s with active AI, about 0.6/s on the normal system page. Portrait and landscape, swiping and automatic rotation without overlaps.
- A real serial Rev. A USB display on COM7: routing and transfer checks. No optical frame-rate measurement of the panel took place.

**Not tested:** a real Spotify account and playback control, real gameplay, Wi-Fi/LAN, background operation, restart, and long-term, battery and thermal behaviour.

**Joint USB display + Companion acceptance:**
Passed on 14 Nov 2026 with the same app (2.12.0 build), a real Rev. A USB display on COM7 and a real Redmi 8 as Companion over a USB tunnel, 10 seconds each: Companion with active AI at target 30/60 alone 21.4/22.9 UI redraws/s, together with the normal USB system page 19.9/25.5, and together with the Hardware HUD on the USB display 20.6/26.2. The USB display no longer slows the Companion down. The HUD itself only reaches about 0.6 frames/s on the serial panel because of its bandwidth (nearly full frames, about 1.7 s per transfer); the normal system page about 1.9 updates/s. Short foreground measurement, no optical panel measurement.

Measurements from the local HUD test builds before 2.12.0 are kept as a historical analysis in [Hardware HUD: frame-rate analysis](HARDWARE-HUD-FPS-ANALYSE.en.md).
