# Changelog – FuchsLabs USB-Display

[Deutsch](CHANGELOG.md) · **English**

All notable changes to the app. Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), versions follow [SemVer](https://semver.org/).

## [Unreleased] – ScreenBuddy 2.5 in development

### Implemented in the unpublished development build
- ScreenBuddy branding for the executable, installer and startup entry; existing uninstall key preserved for upgrades.
- Native Win32 basic settings with preview and plugin management; advanced WebView2 settings remain available.
- Strictly validated JSON manifest, offline plugin store and offline pages. Network plugins remain disabled. The contract is not yet stable.
- Development status and compatibility limits: [ScreenBuddy 2.5](docs/SCREENBUDDY-2.5.en.md).

### Status
- No 2.5 release or download. Version 2.3.0 remains the latest published version. Core/plugin tests: 236 + 220; isolated app/navigation checks: 39 + 49; two installer checks passed. The native window was shown in isolation; layout, a real upgrade and hardware acceptance remain open.

## [2.3.0] – 2026-10-24

### Added
- **Android Companion preview:** USB/HDMI and phone now have separate page lists, positions, rotation intervals and touch targets. Companion-specific settings appear only while Companion is enabled; data sources and theme remain shared.
- Android reports native dimensions and rotation. The PC sends matching frames and drops stale touch coordinates. Page selection, timing and preview are independently controlled. See [setup and limits](docs/COMPANION.en.md).
- Companion preview APK: `FuchsLabs-Companion-0.2.0-preview.apk`.

### Changed
- Windows version bumped to 2.3.0; German/English guides, test record and embedded help synchronized.
- Performance & cleanup: Row-level `std::memcpy` in `paste()`, stack-buffer fast-path in `strprintf()` eliminating heap allocations for common formatting, fast allocation-free hex color parsing without regex, inline flat brush cache in `Canvas::Impl`, eliminated formatting overhead when debug logging is inactive via `is_verbose()`, string pooling (`/GF`), and resolved compiler conversion/deprecation warnings in `render.cpp` and `sysinfo.cpp`.
- Local installation imports settings beside a portable EXE. Updates create a local settings snapshot first; plaintext credentials are encrypted in that snapshot.

### Fixed
- App shutdown and `POST /api/quit` no longer block for up to 30 seconds in the notification area (`Tray`) when the taskbar is unavailable (non-blocking `WM_TIMER` instead of synchronous `Sleep`).
- Uninstallation with purge (`--purge`) now cleans up `config.toml.install-backup`, `companion.json` (including backups/temporary files) and `update-check.json`.
- Local installation migrates existing Companion pairings (`companion.json`) alongside a portable EXE.
- Illustrated guide templates (`anleitung.template.html`, `anleitung.template.en.html`) synchronized with embedded resources; documentation and version links aligned with 2.3.0.

### Validation
- Redmi 8 confirmed: automatic dimensions, portrait/landscape rotation, page changes, finger swipes and taps, pairing and reconnect. Physical USB output alongside Companion, Wi-Fi, Android boot launch and multiple real devices remain open.

## [2.2.0] – 2026-10-24

### Added
- System page by touch: tapping **RAM** opens Task Manager, **Drive** opens “Programs and Features” (`appwiz.cpl`), **Network** opens the speed test at speedtest.net/de in the default browser. Only with touch active.
- Update notice: at most once a day (45 s after start) the public GitHub releases are queried, without a key. A newer published version shows as a balloon at the tray icon and under **Settings → Start** (link to the download page, **Check now**). Nothing is ever downloaded or installed. Can be turned off under **Start → Notify about new versions** (`[ui] update_check`). The time and reported version are stored in `update-check.json` next to `config.toml`; test instances never query.

### Changed
- Illustrated guide (DE/EN) extended with the system page by touch and the update notice; documentation aligned with 2.2.0.

### Changed
- Documentation aligned with 2.1.0: README, docs, changelogs and download links updated; illustrated guide (DE/EN) with a new “Home Assistant” step (address, token, check connection, choose tiles). The already published EXE still embeds the old guide; the new one ships with the next build.
- Documentation aligned with 2.0.3: README/quick start, download links, HDMI/touch guide, validation status and security version overview; existing German/English versions updated together. Project rules and the PR template now require documentation updates with every change.
- Illustrated guide (ANLEITUNG.html / GUIDE.html) updated to C++ 2.0: download of the single EXE instead of a ZIP, built-in installer (no administrator rights, 1.x migration), dedicated settings window (WebView2), tray autostart, uninstall via Windows Apps.
- When starting with Windows, the app starts quietly in the notification area without opening the settings page (autostart entry with `--autostart`; older entries are switched over on the next start). No extra browser tab opens after a restart from the settings either.

### Security
- CodeQL scanning on GitHub. Protection against requests into the home network (SSRF) now covers every blog fetch and every redirect (`rss.public_url`).

### Fixed
- Percent values on the settings page (brightness showed “90 90” after an automatic fix).

## [2.1.0] – 2026-10-23

Combined state of the two latest changes; 2.0.9 is not released on its own.

### Added
- **Home Assistant page**: up to 4 entities from Home Assistant on the display. Sensors (and binary sensors) show value and unit; switches (switch, light, fan, input boolean) show On/Off and can be flipped by tapping when touch is active. Set up under Settings → Home Assistant: address, long-lived access token (DPAPI-encrypted) and an entity picker loaded straight from Home Assistant. Without touch only sensors can be chosen, and switches already chosen are not shown. Access uses the REST API, polled every 15 seconds. Locks, covers and alarm panels are deliberately not switchable.

### Changed

- Tapping a friend now follows a fixed order: Steam chat, otherwise the Discord profile (choose “Message” there), otherwise the Xbox profile on xbox.com in the browser. If the app for a target (Steam, Discord) is missing, the next target is used. Online status no longer matters; in 2.0.8 only Steam opened anything.

## [2.0.8] – 2026-10-23

- Tapping a BF6 player now opens the GameTools profile with the website's platform slug (`pc`, `xboxone`, `ps4`) instead of `ea`, which produced a blank page. Players GameTools does not know for PC still show "not found" on the website although the API returns them.
- Tapping a friend now only opens the Steam chat; Discord and Xbox open nothing for now (no chat link available), the 2.0.7 profile variant is removed.

## [2.0.7] – not published on its own, included in 2.0.8

- Long page titles are shortened in narrow headers so the player count and clock remain readable.
- Tapping a BF6 player opens their GameTools profile in the browser; the leaderboard also shows hours played.
- Tapping friends prefers online accounts, using Discord → Steam → Xbox order within each group; checks registered apps and falls through on launch failures. Steam opens the chat; Discord/Xbox open the profile with access to Message (user-approved variant). Window/tab choice belongs to the platform app/browser.
- Selection uses the displayed snapshot, sorting and subpage, including wide tiles. Blog targets also use the rendered snapshot.
- German/English usage, troubleshooting and embedded guides updated. Automated checks open no real chats. Physical touch/platform acceptance remains open; see [test record](cpp/TESTPROTOKOLL.md).

## [2.0.6] – 2026-10-23

- Touch: tapping a blog post opens it in the default browser on the PC; tapping elsewhere and swiping change pages as before.
- Fixed: after “Save & apply” the settings window disappeared into the notification area. It now reopens after the restart.
- Fixed: a second instance (e.g. a test build) could share the settings page port; the port is now bound exclusively.
- `LICENSE.txt` and `THIRD-PARTY-NOTICES.md` are embedded in the EXE; the installer places them in the installation folder.
- Download and checksums: [v2.0.6](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.6). See [test record](cpp/TESTPROTOKOLL.md) for validation status.

## [2.0.5] – 2026-10-23

- HDMI/USB-C: while the display is shown, the Windows taskbar on that screen is hidden (a tap near the bottom used to open the Start menu); it comes back when the app quits.
- Every touch gesture is logged, with a hint when touch is switched off in the settings. New configurations start with touch enabled.
- Only handed to a tester (touch confirmed on real hardware), not published separately; included in 2.0.6.

## [2.0.4] – 2026-10-22

- Overview: Previous view and Next view control the running display, with its current position below the live preview. Includes list subpages and wide tile groups; automatic cycling continues.
- German/English controls are disabled when there are no views or all views fit on screen. No save or restart needed.
- Download and checksums: [v2.0.4](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.4). See [test record](cpp/TESTPROTOKOLL.md) for build and validation status.

## [2.0.3] – 2026-10-22

- HDMI/USB-C fullscreen stays above the taskbar without taking keyboard focus; hiding and restoring the output are covered.
- Native Windows touch events replace mouse emulation, handling cancelled gestures and duplicate page changes. Real mouse navigation is preserved.
- Display settings check the Windows assignment of touch devices to the selected monitor. “Assign touch display” opens the Windows wizard and pauses fullscreen output during setup.
- Release build and 86 core checks passed; 56 monitor/touch checks with injected Windows input passed. Physical touch hardware and the complete assignment wizard remain untested. [Test record](cpp/TESTPROTOKOLL.md).

## [2.0.0] – 2026-10-22 (prerelease)

- Reimplemented in C++20 under `cpp/`, using D++ for Discord and a native Win32/WebView2 settings window.
- One EXE contains app, installer, updater and uninstaller; Version 1.x settings can be imported. No Python distribution folder is required.
- WebView2 Evergreen Runtime remains external; EXE unsigned.
- 86 checks passed; native window visibly tested and COM7 comparison confirmed by the user. The first real reboot test failed because installation/autostart were redirected into the Codex package context; after correcting the installation, the user confirmed a successful Windows reboot with the tray icon appearing automatically.
- Download and checksums: [v2.0.0](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.0). [Test record](cpp/TESTPROTOKOLL.md).

## [1.2] – 2026-10-21

### Added
- **Switchable English interface** (`ui.language = "de" | "en"`): settings page, display pages, messages and tray menu. The switch is under **Start** and **Display › App**; on first start the language follows the Windows language. Translations live in `i18n.py` (app) and `webui.html` (settings page); German texts remain the source.
- **English guide** `GUIDE.html` (built from `anleitung/anleitung.template.en.html`, pictures with sample data); both guides link to each other, `/anleitung?lang=en` serves the English one.
- Coffee button (PayPal) at the bottom left of the settings page, in the README and as a sponsor button on GitHub (`.github/FUNDING.yml`).
- **Custom colours** (`display.theme = "eigen"`): background, cards, header & bars, accent text, text and secondary text via colour pickers on top of the light or dark preset (`display.color_*`, empty = preset). Live preview of the current page (`POST /api/preview-theme.png`), “Reset” and a warning for low contrast (WCAG). The header text picks navy or white by itself.
- **Any resolution**: all pages draw in logical 480 × 320 coordinates and are rendered sharply at the real size (`render.ScaledDraw`). On the 3.5" display the picture stays pixel-identical. Wide bars show several pages side by side (1920 × 720: two, 1920 × 480: three).
- **Screen via HDMI/USB-C** (`display.output = "monitor"`, `monitor_output.py`): borderless full-screen window on the chosen screen (`display.monitor`, empty = first non-primary screen), no taskbar entry, does not steal focus, mouse pointer hidden. The app is now DPI-aware (real pixel sizes).
- **Touch page switching** (`display.touch`, only with screen output and when Windows reports a touchscreen): swipe left = next page, right = previous, tap in the left third = back, elsewhere = next.

### Documentation
- Bilingual repository: README, CHANGELOG, CONTRIBUTING, SECURITY and TESTPHASE also in English (`*.en.md`); issue forms and PR template are bilingual.
- README in the usual structure: badges, features, pictures with sample data (`docs/screenshots/`), quick start, logins, configuration, hardware, building, project layout, security, licence note.
- New: `CONTRIBUTING.md` (workflow, code rules, handling secrets), `SECURITY.md` (reporting vulnerabilities, security model) and a pull request template.

## [1.1] – 2026-10-21

### Added
- **Settings page** in the browser at `http://127.0.0.1:8765/`, opened via the new tray icon (double-click) or by starting the program a second time. Sections: Start, Overview (live preview, pages on/off and order, time per page, brightness), Friends, Discord, Battlefield 6, Blogs, System, Display.
- **First-run setup** for friends without prior knowledge: status of display and Discord, buttons to close and disable the vendor app, autostart via `HKCU\…\Run` without admin rights.
- **Combined friends list** (`friends.py`): Discord, Steam (official Web API) and Xbox (OpenXBL) in one list, each person only once (automatically for identical names or via “Merge people”, with suggestions for similar names), D/S/X badges per platform. PlayStation and EA deliberately not included (no official API, risk of bans). Tests in `test_friends.py`.
- **Connection status** in the Discord and Friends tabs: green with account name, count and last check, red with the error, grey when not set up.
- **Fold-out help** next to every token and key field (Discord, Steam, profile link, Xbox, EA ID, blogs) with buttons that open the right page directly.
- **Illustrated, interactive guide** `ANLEITUNG.html` (kid-friendly, “step X of N”, choose the wanted pages at the start, “Not working?” help per step), available at `/anleitung` and from the settings page. Sources in `anleitung/`.
- Guide screenshots renewed with sample names; private Xbox names are anonymised while capturing.
- **FuchsLabs design** (`brand.py`): fox orange `#F77F00` and cream `#FFF4E6` as main colours, navy `#14213D` for text, geometric fox logo for display, tray and EXE icon. Fonts Bahnschrift (headings, numbers) and Segoe UI (text), no web fonts.
- **Light/dark switch** (`display.theme`), applies to display, settings page and guide. Light is the default.
- Welcome picture on the display while no page is set up.

### Changed
- New name **FuchsLabs USB-Display**, all references to the display vendor removed (code, texts, EXE).
- Built as a standalone EXE (PyInstaller, `build.bat`); users no longer need Python. Program size reduced from 66 MB to about 36 MB.
- Discord is only loaded when a Discord page is active.
- The plain “Discord friends” page is skipped automatically while the combined list includes Discord.

### Security
- Discord token, Steam and Xbox keys are stored encrypted with Windows DPAPI (`secure.py`) and never sent to the browser or written to the log. Old plain-text tokens are encrypted on start.
- `save_config` no longer creates a plain-text backup (the token used to end up in `.bak` files).
- Pillow raised from 11.3.0 (45 known vulnerabilities) to 13.0.0; external images only as PNG/JPEG/WebP/GIF and at most 2 MB.
- Settings page: `127.0.0.1` only, host check against DNS rebinding, custom header for changes (CSRF), rejects `Sec-Fetch-Site: cross-site/same-site`, `X-Frame-Options: DENY`, `frame-ancestors 'none'`, `nosniff`.
- Feeds containing `<!ENTITY` are discarded, http/https only, responses limited to 3 MB. AVIF module excluded from the build.
- Details in `SICHERHEITSREVIEW.md` (German).

### Fixed
- Invalid JSON sent to `/api/config` now returns 400 instead of 500.
- The “Friends” tab showed “null”.

## [1.0] – 2026-10-21

First version as a Python script, then as the EXE `FuchsLabs-USB-Display-1.0.zip`.

### Added
- Own driver for the 3.5" USB display (480 × 320, serial protocol over USB CDC, RGB565, partial updates in row bands), replacing the vendor app.
- **Discord friends** page via your own bot (Presence and Server Members intents): status, activity, avatars.
- **Battlefield 6 · Leaderboard** page via `api.gametools.network`: rank with badge, K/D, win rate, players from the configuration.
- **System** page: CPU, GPU (`nvidia-smi`), RAM, drive, network via the adapter of the internet connection; CPU temperature and fan from HWiNFO shared memory. `hwinfo-einrichten.bat` sets HWiNFO up to be lightweight and restarts it every 11 hours.
- **Blog updates** page (RSS/Atom, the feed is found from the blog address), newest post per blog.
- Page rotation every 15 seconds, autostart, `DOKUMENTATION.md`.

### Fixed
- Picture was upside down: orientation `reverse_landscape`.
- Wrong network values because the vendor app measured the loopback adapter.
- Fading between pages removed: brightness commands between image data caused picture errors.
- Brightness raised from 50 to 90.
- The whole picture is resent on every page change and every 5 minutes. A stripe at a fixed spot on single-colour test pictures comes from the display or cable, not the app.

[Unreleased]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v2.3.0...HEAD
[2.3.0]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v2.2.0...v2.3.0
[1.2]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v1.1...v1.2
[1.1]: https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v1.1
