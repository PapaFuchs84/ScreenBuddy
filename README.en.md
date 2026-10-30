<div align="center">

**ScreenBuddy 2.6.0 – 30 October 2026:** plugin store with a signed index and Companion app 1.0 as a separate download. [Release notes](docs/RELEASE-NOTES-2.6.md).


<img src="docs/logo/fuchslabs-logo.png" alt="FuchsLabs" width="96"><br>

# ScreenBuddy

**New in 2.6:** plugin store inside the app (settings page **Plugin store**, next to Companion and Plugins) with a signed index, a signed revocation list and eight starter plugins. Installed plugins stay off until you enable them; every host needs your consent. Companion app 1.0 is a separate download. [Release notes](docs/RELEASE-NOTES-2.6.md) · [Status and limits](docs/SCREENBUDDY-2.6.en.md) · [Plugin guide](docs/PLUGIN-SYSTEM.en.md) · [Store terms](docs/PLUGIN-STORE-TERMS.md) · [Examples](cpp/examples/plugins/README.md).

[Deutsch](README.md) · **English**

**Your second screen shows what matters to you: system stats, friends, game rankings and news.**

![Version](https://img.shields.io/badge/version-2.6.0-F77F00)
![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-14213D)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-testing-F77F00)
![Languages](https://img.shields.io/badge/languages-Deutsch%20%7C%20English-14213D)
![Licence](https://img.shields.io/badge/licence-MIT-green)

[![Buy me a coffee](https://img.shields.io/badge/☕_Buy_me_a_coffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/en/display-system.png" width="360" alt="System page, light theme">&nbsp;
<img src="docs/screenshots/en/display-friends.png" width="360" alt="Friends list, light theme">

</div>

A Windows app for 3.5-inch USB displays (480 × 320) and HDMI/USB-C secondary screens, including touch. It runs in the background and provides native Windows settings. The C++20 app and installer are one EXE; no Python, WebView2 or Visual C++ redistributable is required. Earlier version 2.3.0 still required WebView2.

## Contents

- [Features](#features)
- [Pictures](#pictures)
- [Quick start](#quick-start)
- [Configuration](#configuration)
- [Supported hardware](#supported-hardware)
- [Building and development](#building-and-development)
- [Project layout](#project-layout)
- [Security and privacy](#security-and-privacy)
- [Testing and feedback](#testing-and-feedback)
- [Documents](#documents)
- [Licence](#licence)

## Features

| Page | Content | Source | Refresh |
|---|---|---|---|
| **System** | CPU (load, clock, temperature, fan), GPU (load, temperature, fan, VRAM), RAM, drive, network with history | Windows APIs, `nvidia-smi`, HWiNFO shared memory | every second |
| **Friends** | Discord, Steam and Xbox in one list; each person appears only once | your own Discord bot, Steam Web API, [OpenXBL](https://xbl.io) | instant / 1 min / 3 min |
| **Discord friends** | status, activity, avatar | your own Discord bot | instant |
| **Battlefield 6 · Leaderboard** | the EA friends you add, ranked, with badge, K/D and win rate | [gametools.network](https://api.gametools.network) | 15 min |
| **Blog updates** | newest post per blog; the app finds the feed itself | RSS/Atom | 30 min |
| **Home Assistant** | up to 4 sensors (value, unit) or switches (flippable by touch) | Home Assistant REST API | 15 s |

Plus:

- **Native Windows settings window** (Direct2D/DirectWrite, no WebView2; the local service at `http://127.0.0.1:8765/` serves Companion and the guide at `/anleitung`) with live preview, page order, connection status per platform and fold-out help next to every key field
- **First-run setup without prior knowledge**: detects the display, closes and disables the vendor app if you want, autostart without admin rights
- **German or English**: the settings page, display and tray menu can be switched; on first start the language follows Windows
- **Plugin store** (from 2.6.0, settings page, can be switched off): search, check and install reviewed data plugins from the signed store `PapaFuchs84/ScreenBuddy-Plugins`. Before installation the card shows which hosts a plugin would send data to. Installed plugins stay off until you enable them under **Plugins**. See the [2.6.0 release notes](docs/RELEASE-NOTES-2.6.md).
- **Design**: FuchsLabs style in light or dark, or **custom colours** via colour pickers with a contrast warning
- **Any resolution**: pages are rendered sharply at the real size; wide bars show several pages side by side
- **HDMI/USB-C screens** as borderless fullscreen above the taskbar, optionally with **touch** (swipe or tap to change pages, tap a blog post to open it in the browser, tap RAM, Drive or Network on the system page to open Task Manager, “Programs and Features” or a speed test); Windows touch assignment checks and setup under **Display**
- **Illustrated guide** to click through, easy enough for kids, embedded in the EXE in English and German
- **Update notice** (can be turned off): reports a new GitHub version at most once a day in the tray and in the settings, but never installs by itself
- **Single EXE** for installation, updates and uninstallation; no Python or Visual C++ redistributable required. WebView2 is not required.

## System page touch and update notice (from 2.2.0)

**Touch on the system page:** With touch active (screen output only), tapping the **RAM** card opens Task Manager, **Drive** opens “Programs and Features” (`appwiz.cpl`), and **Network** opens the speed test <https://www.speedtest.net/de> in the default browser. The CPU and GPU cards do not react. Windows opens the windows, usually on the main screen.

**Update notice:** At most once a day (at the earliest 45 seconds after start) the app queries the public GitHub releases, without a key or account. If a newer published version exists, a balloon appears at the fox icon (clicking it opens the settings) and **Settings → Start** shows “New version … available” with a link to the download page. **Check now** queries outside the schedule (at most every 5 minutes). Nothing is ever downloaded or installed; to update you download the new EXE yourself and start it. Turn it off under **Settings → Start → Notify about new versions** or with `update_check = false` in `[ui]`. Only an ordinary request with the program name and version in the user agent is sent; as with any request, GitHub sees your IP address. The time of the last check is stored in `update-check.json` next to `config.toml`.

## Touch targets and playtime (from 2.0.7)

With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Tapping a friend preferably opens the Steam chat with that person (Steam app required). Without a Steam account, their Discord profile opens (choose **Message** there), otherwise their Xbox profile on xbox.com in the browser. If the target's app is not installed, the next target is used. Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](docs/TOUCH-2.0.7.en.md). [Validation](cpp/TESTPROTOKOLL.md).

## Home Assistant (from 2.1.0)

New **Home Assistant** page with up to 4 tiles. **Sensors** (including binary sensors) show value and unit. **Switches** (switch, light, fan, input boolean) show On/Off and can be flipped by tapping when touch is active. Without touch only sensors can be chosen in the settings. Set it up under **Settings → Home Assistant**: address (e.g. `http://homeassistant.local:8123`), a long-lived access token (profile in Home Assistant → Security) and the entities from the list that **Check connection** loads straight from Home Assistant. Like the other keys, the token is stored DPAPI-encrypted in `config.toml`. The app polls the REST API every 15 seconds; tapping a switch takes effect immediately. Locks, covers and alarm panels are deliberately not switchable. With an `http://` address the token and values cross your home network unencrypted; use `https://` if Home Assistant offers it.

## Android Companion App (from 2.3.0)

Use an Android phone or tablet as an independent secondary display alongside your USB/HDMI screen. The Companion app is version 1.0 as a separate download; the device test of 27 Oct 2026 refers to its predecessor 0.2.1-preview.
- **Independent pages & rotation timers:** Configure separate page lists and intervals for the smartphone (under **Companion → Independent Companion display**); data sources and theme remain shared.
- **Pairing via QR code or PIN:** Click **Pair device** under the **Companion** tab. The Android app scans the QR code or connects manually using address, port (8777) and a 6-digit PIN; a SHA-256 certificate fingerprint secures the TLS connection.
- **Touch & gestures:** Swipes navigate pages only on the phone. Interactive tiles (smart home, friends, system actions) respond directly to finger taps.
- **Native resolution & rotation:** The phone automatically reports orientation (portrait/landscape) and resolution; the PC renders matching frames at full clarity.
- [Setup and limits](docs/COMPANION.en.md) · [Companion protocol details](docs/COMPANION-PROTOKOLL.md)

## Browse views (2.0.4)


In **Settings → Overview**, use **Previous view** and **Next view** to cycle through the running display views directly. The position and visible pages appear below the live preview. Long lists include their subpages; wide displays advance by one tile group, as with touch navigation. Each manual change restarts the configured page duration, then automatic cycling continues. Buttons are disabled when there are no views or all views already fit on screen. No save or restart is needed. Changes to page selection and order take effect after **Save & apply**.

## Pictures

| Display | |
|---|---|
| <img src="docs/screenshots/en/display-bf6-dark.png" width="380" alt="Battlefield 6 leaderboard, dark theme"> | <img src="docs/screenshots/en/display-blogs-dark.png" width="380" alt="Blog updates, dark theme"> |
| Battlefield 6 leaderboard (dark) | Blog updates (dark) |

| Settings page | |
|---|---|
| <img src="docs/screenshots/en/settings-start.png" width="380" alt="Start page of the settings"> | <img src="docs/screenshots/en/settings-overview.png" width="380" alt="Overview with live preview"> |
| Start with first-run setup | Overview with live preview |
| <img src="docs/screenshots/en/settings-discord.png" width="380" alt="Discord settings with fold-out help"> | <img src="docs/screenshots/en/settings-bf6.png" width="380" alt="Adding Battlefield 6 players"> |
| Discord with fold-out help | Add and check Battlefield 6 players |

All pictures show sample data; some screenshots are from the previous 1.x UI.

## Quick start

1. Download **ScreenBuddy.exe** from [release 2.6.0](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.6.0). The Windows EXE is unsigned; verify it against `SHA256SUMS.txt`. The Companion app 1.0 for Android is a separate download.
2. Use Windows 10/11 x64. From version 2.5 onwards **no** Microsoft Edge WebView2 runtime is required; the earlier version 2.3.0 still needs it.
3. Connect the display, run the EXE and complete the installer. Default location: `%LOCALAPPDATA%\FuchsLabs\USB-Display`, without administrator rights.
4. On first setup, settings open in their own Windows window. Choose **English** under **Sprache · Language**, then **Save & apply**.
5. Configure pages and data sources. Reopen settings from the fox tray icon or by starting the installed app again. Closing the settings window leaves the display running; use **Quit** in the tray to stop it.

The installer can import Version 1.x settings and retains the old folder with a `-1.x-alt` suffix. Updates keep an existing destination configuration. The illustrated guide opens in your default browser from the settings window or **Open guide** in the tray and is embedded in the EXE; no ZIP or `_internal` folder is needed.

For an HDMI/USB-C touchscreen, choose the intended monitor under **Display**. If taps appear on another screen, select **Assign touch display**, press Enter to skip other screens, and touch the intended touchscreen. Then select **Check again**. A USB touch connection is required. See [HDMI/USB-C and touch](cpp/README.md#hdmiusb-c-and-touch).

### Logins for the data sources

| Source | What you need | Cost |
|---|---|---|
| Discord | your own bot in the [Developer Portal](https://discord.com/developers/applications) with *Presence Intent* and *Server Members Intent*, invited to your server | free |
| Steam | a [Steam Web API key](https://steamcommunity.com/dev/apikey) and your profile link; friends list set to “Public” | free |
| Xbox | a key from [OpenXBL](https://xbl.io) | free tier |
| Battlefield 6 | just your friends' EA IDs | free |
| CPU temperature and fan | [HWiNFO64](https://www.hwinfo.com/), set up with `hwinfo-einrichten.bat` | free |

PlayStation and automatic EA friend lists are deliberately not included: there are no official APIs for them, and the unofficial PlayStation route can get accounts banned.

## Configuration

Everything can be set on the settings page. The regular installation stores `config.toml` and `usb-display.log` in `%LOCALAPPDATA%\FuchsLabs\USB-Display`. Custom `-c` and `--log` paths override this. An excerpt:

```toml
[display]
output = "serial"          # "serial" = 3.5" USB display, "monitor" = HDMI/USB-C screen
orientation = "landscape"  # also reverse_landscape, portrait, reverse_portrait
brightness = 90
page_seconds = 15
pages = ["system", "friends", "discord", "bf6", "rss", "homeassistant"]   # order
theme = "hell"             # "hell" (light), "dunkel" (dark) or "eigen" (custom)
touch = true               # only with output = "monitor"

[bf6]
enabled = true
players = ["FuchsKumpel", "Lena_Sniper@psn"]

[rss]
enabled = true
feeds = ["https://www.theverge.com/"]

[homeassistant]
enabled = true
url = "http://homeassistant.local:8123"
entities = ["sensor.living_room_temperature", "light.floor_lamp"]

[ui]
language = "en"            # "de" or "en"
```

Tokens and keys are only stored encrypted (`dpapi:…`). All keys with their defaults are described in [`DOKUMENTATION.md`](DOKUMENTATION.md#4-konfigurationsdatei) (German).

### Command line

| Option | Effect |
|---|---|
| `-c FILE` | use a different configuration file |
| `--demo` | show all pages with sample data |
| `--no-lcd` | start without a display (for testing the settings page) |
| `--preview FOLDER` | also save every frame as PNG |
| `--log FILE`, `-v` | log to a file, verbose |

## Supported hardware

| Device | Connection | Note |
|---|---|---|
| 3.5" USB display 480 × 320 (“revision A”) | USB (serial, 115200 baud) | found automatically; COM port can be set by hand |
| Second screen of any size, e.g. 10.25" 1920 × 720 | HDMI or USB-C (DisplayPort) | borderless full screen on the chosen screen, optional touch |
| GPU stats | Nvidia with driver (`nvidia-smi`) | other GPUs show “–” |

## Building and development

Requirements: Windows x64, Visual Studio Build Tools 2022 with C++/Windows SDK and CMake/Ninja, plus a bootstrapped vcpkg checkout at `%USERPROFILE%\FuchsLabs\vcpkg`.

```bat
git clone https://github.com/PapaFuchs84/ScreenBuddy.git
cd ScreenBuddy\cpp
build.bat nopause
```

The C++ build installs dependencies from the pinned vcpkg manifest/overlay, builds Release, runs core, plugin, monitor, Companion and isolated installer checks, and writes `cpp\dist\ScreenBuddy.exe` locally. The root `build.bat` belongs to the historical Python 1.x version. See [C++ build notes](cpp/README.md#build) and [contribution guide](CONTRIBUTING.en.md).

The WebView2 SDK files and licence notices remain under `cpp/third_party/webview2`, but current 2.5 and 2.6 sources no longer link them. The loopback HTTP service and embedded guide remain available. App translations are in `cpp/src/i18n.cpp`, native page text under `cpp/src/ui_page_*.cpp`; the former HTML source remains only for historical tests under `cpp/res/webui.html` and is not embedded in the 2.5 or 2.6 executables. `/` no longer serves HTML settings.

## Project layout

| Path | Purpose |
|---|---|
| `cpp/src/` | C++ application, display output, data sources, native settings host and installer |
| `cpp/res/` | Embedded guides, icon and version resources |
| `cpp/tests/` | C++ checks |
| `cpp/overlay-ports/`, `cpp/vcpkg.json` | Dependency versions and D++ build settings |
| `cpp/TESTPROTOKOLL.md` | Local Windows/display validation record |
| Root `*.py`, root `build.bat`, `anleitung/` | Historical Python 1.x sources and generation of the current DE/EN guide |

## Security and privacy

- Runs **without admin rights**, no telemetry, no account. The only request of its own besides your data sources is the update notice to the public GitHub releases (can be turned off; at most once a day, no key).
- Credentials are stored encrypted with **Windows DPAPI**, never logged and never sent to the browser.
- The local service is **only reachable on `127.0.0.1`** and protected against DNS rebinding, CSRF and framing by other sites.
- All connections use HTTPS with certificate checks; packages are pinned to exact versions.

Historical Python 1.x review: [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) (German); it is not a complete C++ 2.0 audit. Current validation: [test record](cpp/TESTPROTOKOLL.md). Report a vulnerability as described in [`SECURITY.en.md`](SECURITY.en.md), please not as a public issue.

## Testing and feedback

The app is in testing. Please report bugs and ideas via **Issues → New issue** using the “Bug report” or “Idea or feedback” forms. Where to find the log `usb-display.log` and what must never be sent: [`TESTPHASE.en.md`](TESTPHASE.en.md).

## Documents

| File | Content |
|---|---|
| [`README.md`](README.md) | this README in German |
| [`docs/RELEASE-NOTES-2.6.md`](docs/RELEASE-NOTES-2.6.md) | release notes 2.6.0: plugin store, signing, starter plugins, Companion 1.0, test limits (DE/EN) |
| [`docs/SCREENBUDDY-2.6.en.md`](docs/SCREENBUDDY-2.6.en.md) | status and limits 2.6 ([Deutsch](docs/SCREENBUDDY-2.6.md)) |
| [`CHANGELOG.en.md`](CHANGELOG.en.md) | changes per version ([Deutsch](CHANGELOG.md)) |
| [`CONTRIBUTING.en.md`](CONTRIBUTING.en.md) | contributing: workflow, style, rules ([Deutsch](CONTRIBUTING.md)) |
| [`SECURITY.en.md`](SECURITY.en.md) | reporting vulnerabilities ([Deutsch](SECURITY.md)) |
| [`TESTPHASE.en.md`](TESTPHASE.en.md) | feedback during testing ([Deutsch](TESTPHASE.md)) |
| [`DOKUMENTATION.md`](DOKUMENTATION.md) | full reference: pages, settings, `config.toml`, HWiNFO, troubleshooting (German) |
| [`ANLEITUNG.md`](ANLEITUNG.md) | quick start for users (German) |
| [`PROTOKOLL.md`](PROTOKOLL.md) | project history and decisions (German) |
| [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) | security and performance review (German) |

## Support

Like the app? FuchsLabs appreciates a coffee: [☕ paypal.me/SHerbst84](https://www.paypal.me/SHerbst84)

## Licence

The code is released under the [MIT License](LICENSE), © 2026 FuchsLabs. The name and fox-head logo are excluded ([TRADEMARKS.md](TRADEMARKS.md)). Third-party components retain their own licences ([THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)).

Battlefield, Discord, Steam and Xbox are trademarks of their respective owners. FuchsLabs is not affiliated with them.

## Validation of 2.6.0

Test limits and automated check counts are in the [2.6.0 release notes](docs/RELEASE-NOTES-2.6.md#test-limits). The store installation in an isolated instance is stated separately there.

## Validation of 2.3.0

Release build: 236 core, 11 monitor and 335 Companion/TLS checks passed. Also passed: 39 isolated app-integration checks, 2 installer config-preservation checks, 22 browser checks (DE/EN), and 20 Android protocol tests. On the Redmi 8, native dimensions, portrait/landscape rotation, automatic page changes, taps, swipes and reconnect were confirmed; the user also confirmed a sharp image and finger operation. **Open:** physical USB output and Redmi together in the same new PC instance, Wi-Fi, Android launch at boot and multiple real devices. See the [test record](cpp/TESTPROTOKOLL.md).

## Validation of 2.2.0 (historical)

Release build, 225 core checks (new: hit areas of the system page, version comparison, parsing of the GitHub response) and 11 monitor basics passed; the hit areas match the rendered system page (preview image). **Still open:** tapping on real touch hardware (Task Manager, “Programs and Features”, speed test), the balloon and the settings entry with a genuinely newer release, and a Windows sign-in test for 2.2.0. See the [test record](cpp/TESTPROTOKOLL.md).

## Validation of 2.0.3

Release build, 86 core checks and 56 monitor/touch checks with injected Windows input passed (the 56 include 11 basic checks). Installed 2.0.3: actual file version, hash, API version, autostart entry and `display_ok=true` verified outside package redirection. Physical touch hardware, the complete Windows assignment wizard and a new Windows sign-in test for 2.0.3 remain untested. See [test record](cpp/TESTPROTOKOLL.md).

The earlier 2.0 validation included comparison with 1.x on COM7 and user confirmation of the physical display and a successful Windows reboot with the tray icon.

The first real reboot test failed due to packaged-host installation redirection. Installation and the actual Windows Run entry were subsequently corrected outside that context; the user subsequently confirmed a successful Windows reboot with the tray icon appearing automatically.
