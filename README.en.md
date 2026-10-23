<div align="center">

<img src="docs/logo/fuchslabs-logo.png" alt="FuchsLabs" width="96"><br>

# FuchsLabs USB-Display

[Deutsch](README.md) · **English**

**Your second screen shows what matters to you: system stats, friends, game rankings and news.**

![Version](https://img.shields.io/badge/version-2.0.8-F77F00)
![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-14213D)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-testing-F77F00)
![Languages](https://img.shields.io/badge/languages-Deutsch%20%7C%20English-14213D)
![Licence](https://img.shields.io/badge/licence-MIT-green)

[![Buy me a coffee](https://img.shields.io/badge/☕_Buy_me_a_coffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/en/display-system.png" width="360" alt="System page, light theme">&nbsp;
<img src="docs/screenshots/en/display-friends.png" width="360" alt="Friends list, light theme">

</div>

A Windows app for 3.5" USB displays (480 × 320) and second screens over HDMI/USB-C at any resolution (e.g. 10.25" bars at 1920 × 720, touch included). It replaces the vendor software, runs quietly in the background and uses its own Windows settings window, powered by WebView2. Version 2.0 is a C++20 app with its installer in one EXE. Python is not required; the Microsoft Edge WebView2 Evergreen Runtime must be installed separately.

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

Plus:

- **Own Windows settings window** (WebView2; internal local service at `http://127.0.0.1:8765/`) with live preview, page order, connection status per platform and fold-out help next to every key field
- **First-run setup without prior knowledge**: detects the display, closes and disables the vendor app if you want, autostart without admin rights
- **German or English**: the settings page, display and tray menu can be switched; on first start the language follows Windows
- **Design**: FuchsLabs style in light or dark, or **custom colours** via colour pickers with a contrast warning
- **Any resolution**: pages are rendered sharply at the real size; wide bars show several pages side by side
- **HDMI/USB-C screens** as borderless fullscreen above the taskbar, optionally with **touch** (swipe or tap to change pages, tap a blog post to open it in the browser); Windows touch assignment checks and setup under **Display**
- **Illustrated guide** to click through, easy enough for kids, embedded in the EXE in English and German
- **Single EXE** for installation, updates and uninstallation; no Python or Visual C++ redistributable required. WebView2 Runtime remains a prerequisite.

## Touch targets and playtime (from 2.0.7)

2.0.9 is a local development build; the published download remains 2.0.8. With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Tapping a friend preferably opens the Steam chat with that person (Steam app required). Without a Steam account, their Discord profile opens (choose **Message** there), otherwise their Xbox profile on xbox.com in the browser. If the target's app is not installed, the next target is used. Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](docs/TOUCH-2.0.7.en.md). [Validation](cpp/TESTPROTOKOLL.md).

## Browse views (2.0.4)

Version 2.0.8 is the current version.

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

1. Download **FuchsLabs-USB-Display.exe** from the [latest release](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/latest) (current version: **2.0.8**). The EXE is unsigned. `SHA256SUMS.txt` is provided alongside it.
2. Use Windows 10/11 x64 with Microsoft Edge WebView2 Evergreen Runtime installed. The runtime is not bundled and the installer does not install it automatically.
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
pages = ["system", "friends", "discord", "bf6", "rss"]   # order
theme = "hell"             # "hell" (light), "dunkel" (dark) or "eigen" (custom)
touch = true               # only with output = "monitor"

[bf6]
enabled = true
players = ["FuchsKumpel", "Lena_Sniper@psn"]

[rss]
enabled = true
feeds = ["https://www.theverge.com/"]

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
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display\cpp
build.bat nopause
```

The C++ build installs dependencies from the pinned vcpkg manifest/overlay, builds Release, runs `fl-tests.exe` and writes `cpp\dist\FuchsLabs-USB-Display.exe`. The root `build.bat` belongs to the historical Python 1.x version. See [C++ build notes](cpp/README.md#build) and [contribution guide](CONTRIBUTING.en.md).

The WebView2 SDK header/static loader and their licence notices are under `cpp/third_party/webview2`; the runtime itself is external. App translations are in `cpp/src/i18n.cpp`, settings translations in `cpp/res/webui.html`.

## Project layout

| Path | Purpose |
|---|---|
| `cpp/src/` | C++ application, display output, data sources, native settings host and installer |
| `cpp/res/` | Embedded settings, guides, icon and version resources |
| `cpp/tests/` | C++ checks |
| `cpp/overlay-ports/`, `cpp/vcpkg.json` | Dependency versions and D++ build settings |
| `cpp/TESTPROTOKOLL.md` | Local Windows/display validation record |
| Root `*.py`, root `build.bat`, `anleitung/` | Historical Python 1.x sources and guide generation |

## Security and privacy

- Runs **without admin rights**, no telemetry, no account.
- Credentials are stored encrypted with **Windows DPAPI**, never logged and never sent to the browser.
- The settings page is **only reachable on `127.0.0.1`** and protected against DNS rebinding, CSRF and framing by other sites.
- All connections use HTTPS with certificate checks; packages are pinned to exact versions.

Historical Python 1.x review: [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) (German); it is not a complete C++ 2.0 audit. Current validation: [test record](cpp/TESTPROTOKOLL.md). Report a vulnerability as described in [`SECURITY.en.md`](SECURITY.en.md), please not as a public issue.

## Testing and feedback

The app is in testing. Please report bugs and ideas via **Issues → New issue** using the “Bug report” or “Idea or feedback” forms. Where to find the log `usb-display.log` and what must never be sent: [`TESTPHASE.en.md`](TESTPHASE.en.md).

## Documents

| File | Content |
|---|---|
| [`README.md`](README.md) | this README in German |
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

## Validation of 2.0.3

Release build, 86 core checks and 56 monitor/touch checks with injected Windows input passed (the 56 include 11 basic checks). Installed 2.0.3: actual file version, hash, API version, autostart entry and `display_ok=true` verified outside package redirection. Physical touch hardware, the complete Windows assignment wizard and a new Windows sign-in test for 2.0.3 remain untested. See [test record](cpp/TESTPROTOKOLL.md).

The earlier 2.0 validation included comparison with 1.x on COM7 and user confirmation of the physical display and a successful Windows reboot with the tray icon.

The first real reboot test failed due to packaged-host installation redirection. Installation and the actual Windows Run entry were subsequently corrected outside that context; the user subsequently confirmed a successful Windows reboot with the tray icon appearing automatically.
