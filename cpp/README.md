# FuchsLabs USB-Display 2.0.7 (local development build)

Native Windows C++20 port of FuchsLabs USB-Display. The application and installer are distributed as one EXE; it requires no Python runtime or Visual C++ redistributable. The settings window is a native Windows window backed by Microsoft Edge WebView2. The WebView2 Evergreen Runtime is a Windows prerequisite and is not bundled in this EXE.

## Install and first start

1. Download the EXE from the [latest release](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/latest) (currently 2.0.6), then start it from Downloads or another folder. The EXE is unsigned; verify it using the supplied `SHA256SUMS.txt`.
2. Choose the options in the installer. The default location is `%LOCALAPPDATA%\FuchsLabs\USB-Display`; no administrator rights are required.
3. On first start, settings open in their own Windows window. Open them later from the fox icon in the notification area. Configure the display and optional Discord, Steam, Xbox, Battlefield 6, and RSS data there.

The installer can import the Version 1.x `config.toml`. Secrets use Windows DPAPI with the same application entropy as Version 1.x. The old folder is renamed with `-1.x-alt` so it can be restored. The app can be removed from Windows Apps & Features; the uninstall prompt offers a separate choice to remove settings and saved credentials.

## Features

- System statistics, friends from Discord/Steam/Xbox, Battlefield 6 profiles, and RSS/Atom feeds.
- USB serial display and full-screen monitor output, orientation, brightness, touch navigation, themes, and scalable multi-tile layouts.
- Native Windows settings window (WebView2), tray controls, current-user autostart, installer, updater, and uninstaller.
- Settings server binds only to loopback. Requests are checked for local Host, browser fetch metadata, and the `X-FL: 1` write header. RSS targets and redirects are checked against non-public IP ranges.

Discord uses D++ with the Members and Presence intents enabled. Enable both privileged intents in the Discord Developer Portal for the bot.

## Touch targets and playtime (2.0.7, local)

2.0.7 is a local development build; the published download remains 2.0.6. With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Since 2.0.8, tapping a friend opens the Steam chat with that person; friends without a Steam account, and Discord and Xbox, open nothing for now (no chat link available). Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](../docs/TOUCH-2.0.7.en.md). [Validation](TESTPROTOKOLL.md).

## Browse views (2.0.4)

In **Settings → Overview**, use **Previous view** and **Next view** to cycle through the running display views directly. The position and visible pages appear below the live preview. Long lists include their subpages; wide displays advance by one tile group, as with touch navigation. Each manual change restarts the configured page duration, then automatic cycling continues. Buttons are disabled when there are no views or all views already fit on screen. No save or restart is needed. Changes to page selection and order take effect after **Save & apply**.

## HDMI/USB-C and touch

Choose **Screen over HDMI or USB-C** and the intended monitor under **Display**; set Windows to **Extend**. Fullscreen covers the taskbar without taking keyboard focus. A touchscreen also requires its USB touch connection.

If taps affect another display, select **Assign touch display**. The fullscreen output pauses while the Windows wizard is open. Press Enter to skip other screens, then touch the intended touchscreen; Esc cancels. When finished, select **Check again**, enable **Touch control** if needed, and save. Assignment is a Windows setting. Physical touch hardware and the full wizard workflow remain untested.

## Build

The checked-in `build.bat` expects Visual Studio Build Tools 2022, CMake/Ninja, and a vcpkg checkout at `%USERPROFILE%\FuchsLabs\vcpkg`. It installs the pinned dependencies using the repository's vcpkg manifest and overlay port, builds x64 Release, runs `fl-tests.exe` plus `fl-monitor-tests.exe`, and copies the installer/app to `dist\FuchsLabs-USB-Display.exe`.

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

Version 2.0.4: release build, 86 core checks, 11 monitor basics and 49 navigation/API checks passed. The new controls and page changes were also checked in the German and English browser UI, using an isolated instance with no display output. Subsequently installed in the ordinary Windows user context: file/API version and hash match, settings are unchanged, autostart is preserved and display_ok=true. No new Windows sign-in or physical visual/touch check was performed. Run `python tests/test_page_navigation.py build/FuchsLabs-USB-Display.exe` to repeat the navigation checks on the reserved test port 8766.

Version 2.0.3: release build, 86 core checks and 56 monitor/touch checks with injected Windows input passed (the 56 include 11 basic checks). Installation, actual file version/hash, API version, autostart entry and `display_ok=true` were verified outside package redirection. Physical touch hardware, the full assignment wizard and a new Windows sign-in test for 2.0.3 remain untested. Earlier 2.0 validation included the user-confirmed COM7 display comparison and Windows reboot. See [TESTPROTOKOLL.md](TESTPROTOKOLL.md). The EXE is unsigned and requires WebView2 Evergreen Runtime.

## Installing from automation

Run the installer in the ordinary Windows user context. A packaged host can redirect LocalAppData and registry writes into its package sandbox; its own file/registry checks then do not establish that Windows sign-in sees those files. The first real reboot test exposed this on 22 October 2026. Reinstallation from an ordinary Windows process verified the actual EXE, hash and Run entry outside the package context; the user confirmed the final Windows reboot retest succeeded, with the tray icon appearing automatically.
