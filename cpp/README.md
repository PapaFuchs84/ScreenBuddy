# ScreenBuddy 2.7.0

Companion: preview since 2.3.0 (independent USB/HDMI and Android views, automatic phone dimensions and rotation), Companion app 1.0.0 as a separate download since 2.6.0, several devices at once since 2.7.0. See [DE](../docs/COMPANION.md) / [EN](../docs/COMPANION.en.md). Tests include `python tests/app_companion_integration.py` on Windows (isolated processes with `--no-lcd`, no physical display).

Native Windows C++20 application formerly released as FuchsLabs USB-Display. The application and installer are built as one EXE; it requires no Python runtime or Visual C++ redistributable. The current source (2.7.0) opens settings only in the self-drawn native Win32 interface and no longer links WebView2. The loopback API and embedded guide remain; `/` no longer serves the former HTML settings page. ScreenBuddy 2.5.0, 2.6.0 and 2.7.0 require no WebView2; earlier 2.3.0 still does.

## ScreenBuddy 2.7.0

Store previews, plugin page ordering, Plugin Workshop and independent/synchronised Companions. Store installs, including offline plugins, remain disabled until explicitly enabled. See [2.7.0 release notes](../docs/RELEASE-NOTES-2.7.md).

## Previous release 2.6.0

Plugin store on the settings page **Plugin store** (next to Companion and Plugins): signed index and revocation list (Ed25519, pinned public key), packages checked by SHA-256 and host declarations, daily background sync and the switch `[ui] plugin_store`. Installed plugins stay off until enabled and approved per host. The Companion app 1.0 is a separate download. Test limits and automated check counts: [release notes 2.6.0](../docs/RELEASE-NOTES-2.6.md).

## ScreenBuddy 2.5.0

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

The sample manifests and tutorials are in [examples/plugins](examples/plugins/README.md), [German](../docs/PLUGIN-TUTORIAL.md) and [English](../docs/PLUGIN-TUTORIAL.en.md). They include an offline clock, an offline focus board and an optional public weather source. The 41 example checks validate and render manifests; they do not contact external services or prove hardware output.

## Install and first start

1. Download the EXE from the [latest release](https://github.com/PapaFuchs84/ScreenBuddy/releases/latest) (2.7.0: `ScreenBuddy.exe`), then start it from Downloads or another folder. The EXE is unsigned; verify it using the supplied `SHA256SUMS.txt`.
2. Choose the options in the installer. The default location is `%LOCALAPPDATA%\FuchsLabs\USB-Display`; no administrator rights are required.
3. On first start, settings open in their own Windows window. Open them later from the fox icon in the notification area. Configure the display and optional Discord, Steam, Xbox, Battlefield 6, and RSS data there.

The installer can import the Version 1.x `config.toml`. Secrets use Windows DPAPI with the same application entropy as Version 1.x. The old folder is renamed with `-1.x-alt` so it can be restored. The app can be removed from Windows Apps & Features; the uninstall prompt offers a separate choice to remove settings and saved credentials.

## Features

- System statistics, friends from Discord/Steam/Xbox, Battlefield 6 profiles, RSS/Atom feeds, and new releases on streaming services (TMDB API, provider data from JustWatch; unreleased). This product uses the TMDB API but is not endorsed or certified by TMDB.
- USB serial display and full-screen monitor output, orientation, brightness, touch navigation, themes, and scalable multi-tile layouts.
- Tray controls, current-user autostart, installer, updater and uninstaller. ScreenBuddy 2.5 has native Win32 settings; earlier 2.3.0 used WebView2. The loopback API and embedded guide remain available.
- Settings server binds only to loopback. Requests are checked for local Host, browser fetch metadata, and the `X-FL: 1` write header. RSS targets and redirects are checked against non-public IP ranges.

Discord uses D++ with the Members and Presence intents enabled. Enable both privileged intents in the Discord Developer Portal for the bot.

## System page touch and update notice (from 2.2.0)

**Touch on the system page:** With touch active (screen output only), tapping the **RAM** card opens Task Manager, **Drive** opens “Programs and Features” (`appwiz.cpl`), and **Network** opens the speed test <https://www.speedtest.net/de> in the default browser. The CPU and GPU cards do not react. Windows opens the windows, usually on the main screen.

**Update notice:** At most once a day (at the earliest 45 seconds after start) the app queries the public GitHub releases, without a key or account. If a newer published version exists, a balloon appears at the fox icon (clicking it opens the settings) and **Settings → Updates & startup** shows “New version available: …”. **Open release page** leads to the download page, **Check for updates now** queries outside the schedule (at most every 5 minutes). Nothing is ever downloaded or installed; to update you download the new EXE yourself and start it. Turn it off under **Settings → Updates & startup → Automatically check for updates** or with `update_check = false` in `[ui]`. Only an ordinary request with the program name and version in the user agent is sent; as with any request, GitHub sees your IP address. The time of the last check is stored in `update-check.json` next to `config.toml`.

## Touch targets and playtime (from 2.0.8)

Developed as 2.0.7; that version was not released separately, so the feature is available from 2.0.8. With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Tapping a friend preferably opens the Steam chat with that person (Steam app required). Without a Steam account, their Discord profile opens (choose **Message** there), otherwise their Xbox profile on xbox.com in the browser. If the target's app is not installed, the next target is used. Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](../docs/TOUCH-2.0.7.en.md). [Validation](TESTPROTOKOLL.md).

## Home Assistant (from 2.1.0)

New **Home Assistant** page with up to 4 tiles. **Sensors** (including binary sensors) show value and unit. **Switches** (switch, light, fan, input boolean) show On/Off and can be flipped by tapping when touch is active. Without touch only sensors can be chosen in the settings. Set it up under **Settings → Home Assistant**: address (e.g. `http://homeassistant.local:8123`), a long-lived access token (profile in Home Assistant → Security) and the entities from the list that **Check connection** loads straight from Home Assistant. Like the other keys, the token is stored DPAPI-encrypted in `config.toml`. The app polls the REST API every 15 seconds; tapping a switch takes effect immediately. Locks, covers and alarm panels are deliberately not switchable. With an `http://` address the token and values cross your home network unencrypted; use `https://` if Home Assistant offers it.

## Browse views (since 2.0.4)

Since 2.0.4 the running display can be advanced with a button press, without saving or restarting. In the native interface (from 2.5.0) **Show previous page** and **Show next page** under **Settings → Companion** do this; they act on the Companion output. For the main display the step is available only through the local interface (`POST /api/pages/step`). The former “Previous view”/“Next view” buttons under **Overview** do not exist in the native interface. Changes to page selection and order take effect after **Save & apply**.

## Companion app (Android preview, PC side)

`[companion] enabled = true` (settings page **Companion**) mirrors the display to a paired Android device; `display.output = "network"` uses the device as the only output (480 × 320). TLS WebSocket on port 8777, private IPv4 addresses only, mDNS `_fuchslabs-display._tcp`, QR/PIN pairing, keys DPAPI-protected in `companion.json`. Protocol: [../docs/COMPANION-PROTOKOLL.md](../docs/COMPANION-PROTOKOLL.md). The platform-neutral part (`companion.cpp`, `companion_server.cpp`) builds and tests on Linux: `tests/build-linux.sh` (needs g++, OpenSSL dev files, `NLOHMANN_INCLUDE` pointing to a folder with `nlohmann/json.hpp`).

## HDMI/USB-C and touch

Choose the display type **Monitor** and the intended screen under **Display**; set Windows to **Extend**. Fullscreen covers the taskbar without taking keyboard focus. A touchscreen also requires its USB touch connection.

If taps affect another display, open **Assign touchscreen** and choose **Open Windows wizard**. The fullscreen output pauses while the Windows wizard is open. Press Enter to skip other screens, then touch the intended touchscreen; Esc cancels. When finished, select **Check again**, enable **Touch control** if needed, and save. Assignment is a Windows setting. Physical touch hardware and the full wizard workflow remain untested.

## Build

The checked-in `build.bat` expects Visual Studio Build Tools 2022, CMake/Ninja, and a vcpkg checkout at `%USERPROFILE%\FuchsLabs\vcpkg`. It installs pinned dependencies using the repository's vcpkg manifest and overlay port, builds x64 Release in `build\screenbuddy-release`, runs core, plugin, monitor, Companion and isolated installer tests, and copies the installer/app to `dist\ScreenBuddy.exe`.

```bat
build.bat
```

Dependencies are linked statically, including D++ and OpenSSL. OpenSSL on Windows loads trust roots from the Windows certificate store. The D++ vcpkg overlay records the corresponding build option and patches.

Run `build\fl-monitor-tests.exe window` only for a controlled visible secondary-monitor test; it injects touch events and closes its test windows afterwards. This does not validate physical touch hardware.

## Diagnostics

- The application log is `usb-display.log` next to the installed EXE.
- `--demo` renders sample pages; `--no-lcd` starts without a display output; `--preview <folder>` writes preview PNGs; `--log <file>` selects a log path; `-c <config.toml>` selects a config file.
- The test executable accepts `render <folder> [de|en]` to create renderer samples. It is a developer tool, not included in the one-file release.
- If the serial display cannot open its COM port, close the vendor monitor program and check that another app is not using that port.

## Validation status

Version 2.3.0: release build, 236 core checks, 11 monitor checks and 335 Companion/TLS checks; 39 isolated app-integration checks; 2 installer config-preservation checks; 22 DE/EN browser checks; Android has 20 protocol tests. The 2.3.0 settings page rendered on Windows with the Redmi 8 connected. Redmi preview validation confirms native portrait/landscape sizing, automatic page changes, touch/swipe and reconnect. The companion and physical USB display have not yet been tested together in the same new PC process; Wi-Fi, launcher/boot and multiple real devices remain open.

Development checks before release (27 October 2026): 272 core, 223 parser, 127 plugin-store, 134 plugin-transport, 135 plugin-settings, 41 plugin-example, 11 monitor and 335 Companion checks; 27 native UI, 16 mouse, 16 Companion rebuild, 39 isolated app, 51 navigation/API and two installer checks passed. One isolated public HTTPS fetch passed; the full in-app consent flow, hardware, real upgrade and full interactive visual acceptance of the native UI remain open. See [development status](../docs/SCREENBUDDY-2.5.en.md) and the [test record](TESTPROTOKOLL.md).

Version 2.2.0: release build, 225 core checks (new: system-page hit areas, version comparison, GitHub response parsing) and 11 monitor basics passed. Open: tapping on real touch hardware, balloon/settings entry with a genuinely newer release, Windows sign-in test.

Version 2.0.4: release build, 86 core checks, 11 monitor basics and 49 navigation/API checks passed. The new controls and page changes were also checked in the German and English browser UI, using an isolated instance with no display output. Subsequently installed in the ordinary Windows user context: file/API version and hash match, settings are unchanged, autostart is preserved and display_ok=true. No new Windows sign-in or physical visual/touch check was performed. Run `python tests/test_page_navigation.py build/FuchsLabs-USB-Display.exe` to repeat the navigation checks on the reserved test port 8766.

Version 2.0.3: release build, 86 core checks and 56 monitor/touch checks with injected Windows input passed (the 56 include 11 basic checks). Installation, actual file version/hash, API version, autostart entry and `display_ok=true` were verified outside package redirection. Physical touch hardware, the full assignment wizard and a new Windows sign-in test for 2.0.3 remain untested. Earlier 2.0 validation included the user-confirmed COM7 display comparison and Windows reboot. See [TESTPROTOKOLL.md](TESTPROTOKOLL.md). The EXE is unsigned and requires WebView2 Evergreen Runtime.

## Installing from automation

Run the installer in the ordinary Windows user context. A packaged host can redirect LocalAppData and registry writes into its package sandbox; its own file/registry checks then do not establish that Windows sign-in sees those files. The first real reboot test exposed this on 22 October 2026. Reinstallation from an ordinary Windows process verified the actual EXE, hash and Run entry outside the package context; the user confirmed the final Windows reboot retest succeeded, with the tray icon appearing automatically.
