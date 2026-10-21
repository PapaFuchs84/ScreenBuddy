<div align="center">

# 🦊 FuchsLabs USB-Display

[Deutsch](README.md) · **English**

**Your second screen shows what matters to you: system stats, friends, game rankings and news.**

![Version](https://img.shields.io/badge/version-1.2-F77F00)
![Platform](https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-14213D)
![Python](https://img.shields.io/badge/python-3.11%2B-3776AB?logo=python&logoColor=white)
![Status](https://img.shields.io/badge/status-testing-F77F00)
![Languages](https://img.shields.io/badge/languages-Deutsch%20%7C%20English-14213D)
![Licence](https://img.shields.io/badge/licence-all%20rights%20reserved-lightgrey)

[![Buy me a coffee](https://img.shields.io/badge/☕_Buy_me_a_coffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/en/display-system.png" width="360" alt="System page, light theme">&nbsp;
<img src="docs/screenshots/en/display-friends.png" width="360" alt="Friends list, light theme">

</div>

A Windows app for 3.5" USB displays (480 × 320) and second screens over HDMI/USB-C at any resolution (e.g. 10.25" bars at 1920 × 720, touch included). It replaces the vendor software, runs quietly in the background and is set up entirely through a settings page in your browser. No Python or prior knowledge needed.

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

- **Settings page** at `http://127.0.0.1:8765/` with live preview, page order, connection status per platform and fold-out help next to every key field
- **First-run setup without prior knowledge**: detects the display, closes and disables the vendor app if you want, autostart without admin rights
- **German or English**: the settings page, display and tray menu can be switched; on first start the language follows Windows
- **Design**: FuchsLabs style in light or dark, or **custom colours** via colour pickers with a contrast warning
- **Any resolution**: pages are rendered sharply at the real size; wide bars show several pages side by side
- **HDMI/USB-C screens** as borderless full screen, optionally with **touch** (swipe or tap to change pages)
- **Illustrated guide** to click through, easy enough for kids, in English (`GUIDE.html`) and German (`ANLEITUNG.html`)
- **Lightweight**: about 1% of one CPU core and around 90 MB of RAM

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

All pictures show sample data.

## Quick start

1. **Extract the ZIP** and put the folder somewhere permanent, e.g. `Documents\FuchsLabs-USB-Display`.
2. **Plug in the display** (USB) or connect the second screen via HDMI/USB-C.
3. **Start `FuchsLabs-USB-Display.exe`.** If Windows warns you (“Windows protected your PC”), click **More info → Run anyway**. The EXE is not code-signed.
4. The **settings page** opens in your browser. Under **Sprache · Language**, choose **English** and click **Save & apply**. **Start** shows whether the display was found; this is also where you close the vendor app if needed and turn on **Start with Windows**.
5. Turn on the pages you want under **Overview** and enter your logins. Every field has fold-out help.
6. **Save & apply.**

Later you open the settings by double-clicking the fox icon in the notification area. Illustrated, step by step: `GUIDE.html` in the ZIP.

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

Everything can be set on the settings page. It writes `config.toml` next to the EXE (or to `%APPDATA%\FuchsLabs\USB-Display` if the folder is read-only). An excerpt:

```toml
[display]
output = "serial"          # "serial" = 3.5" USB display, "monitor" = HDMI/USB-C screen
orientation = "landscape"  # also reverse_landscape, portrait, reverse_portrait
brightness = 90
page_seconds = 15
pages = ["system", "friends", "discord", "bf6", "rss"]   # order
theme = "hell"             # "hell" (light), "dunkel" (dark) or "eigen" (custom)
touch = false              # only with output = "monitor"

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

Requirements: Windows 10/11 and Python 3.11+ (64-bit, from [python.org](https://www.python.org/)).

```bat
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display
build.bat
```

`build.bat` creates its own `.venv`, installs the pinned packages from `requirements.txt` and `requirements-build.txt` and builds with PyInstaller into `dist\FuchsLabs-USB-Display\`. Always pass on the whole folder (the EXE needs `_internal`).

Developing without the EXE:

```bat
.venv\Scripts\python fuchslabs_display.py --no-lcd        :: app without display, settings page runs
.venv\Scripts\python fuchslabs_display.py --demo --no-lcd --preview preview   :: sample pictures of all pages
.venv\Scripts\python test_friends.py                      :: tests for merging the friends lists
```

The illustrated guides are built with `anleitung/make_images.py`: `ANLEITUNG.html` from `anleitung/anleitung.template.html` and the screenshots in `anleitung/bilder`, `GUIDE.html` from `anleitung/anleitung.template.en.html` with `make_images.py <english screenshots> en`. `anleitung/screenshots.mjs` takes the screenshots (`FL_LANG=en` for English) and replaces private names with samples. `anleitung/bilder/freunde.png` is kept out of the repository as a precaution and is captured locally before building.

Translations: app and display texts in `i18n.py`, settings page texts in the `EN` table in `webui.html`. The German texts in the code are the source.

How to contribute: [`CONTRIBUTING.en.md`](CONTRIBUTING.en.md).

## Project layout

```
fuchslabs_display.py   main program: start, single instance, page rotation, services, restart
display_driver.py      driver for the serial 3.5" display (RGB565, partial updates in row bands)
monitor_output.py      output to an HDMI/USB-C screen as a full-screen window, touch
render.py              draws all pages (logically 480 × 320, scaled to any resolution)
brand.py               FuchsLabs design: colours and fox logo
sysinfo.py             system stats: Windows APIs, nvidia-smi, HWiNFO
discord_feed.py        Discord bot (only loaded when needed)
friends.py             Steam, Xbox and merging the friends lists
bf6.py, rss.py         Battlefield 6 and blogs
i18n.py                language: German/English for display, messages and tray menu
appconfig.py           read, validate and write the configuration
secure.py              DPAPI encryption of credentials
webui.py, webui.html   settings page (local web server, a single HTML file)
tray.py, winsetup.py   tray icon, autostart, vendor app
anleitung/             sources of the illustrated guides
docs/screenshots/      pictures for the READMEs
```

One process with one asyncio loop for display, data fetching, Discord and web server; system stats and the tray icon run in their own threads. Details in [`DOKUMENTATION.md`](DOKUMENTATION.md#10-technischer-aufbau) (German).

## Security and privacy

- Runs **without admin rights**, no telemetry, no account.
- Credentials are stored encrypted with **Windows DPAPI**, never logged and never sent to the browser.
- The settings page is **only reachable on `127.0.0.1`** and protected against DNS rebinding, CSRF and framing by other sites.
- All connections use HTTPS with certificate checks; packages are pinned to exact versions.

Review report: [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) (German). Report a vulnerability as described in [`SECURITY.en.md`](SECURITY.en.md), please not as a public issue.

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

No open-source licence: © 2026 FuchsLabs, all rights reserved. The source code is only visible to invited contributors and testers and may not be passed on without permission.

Battlefield, Discord, Steam and Xbox are trademarks of their respective owners. FuchsLabs is not affiliated with them.
