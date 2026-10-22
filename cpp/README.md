# FuchsLabs USB-Display 2.0

Native Windows C++20 port of FuchsLabs USB-Display. The application and installer are distributed as one EXE; it requires no Python runtime or Visual C++ redistributable. The settings window is a native Windows window backed by Microsoft Edge WebView2. The WebView2 Evergreen Runtime is a Windows prerequisite and is not bundled in this EXE.

## Install and first start

1. Download the EXE from [v2.0.0](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.0), then start it from Downloads or another folder. This release is an unsigned prerelease.
2. Choose the options in the installer. The default location is `%LOCALAPPDATA%\FuchsLabs\USB-Display`; no administrator rights are required.
3. On first start, settings open in their own Windows window. Open them later from the fox icon in the notification area. Configure the display and optional Discord, Steam, Xbox, Battlefield 6, and RSS data there.

The installer can import the Version 1.x `config.toml`. Secrets use Windows DPAPI with the same application entropy as Version 1.x. The old folder is renamed with `-1.x-alt` so it can be restored. The app can be removed from Windows Apps & Features; the uninstall prompt offers a separate choice to remove settings and saved credentials.

## Features

- System statistics, friends from Discord/Steam/Xbox, Battlefield 6 profiles, and RSS/Atom feeds.
- USB serial display and full-screen monitor output, orientation, brightness, touch navigation, themes, and scalable multi-tile layouts.
- Native Windows settings window (WebView2), tray controls, current-user autostart, installer, updater, and uninstaller.
- Settings server binds only to loopback. Requests are checked for local Host, browser fetch metadata, and the `X-FL: 1` write header. RSS targets and redirects are checked against non-public IP ranges.

Discord uses D++ with the Members and Presence intents enabled. Enable both privileged intents in the Discord Developer Portal for the bot.

## Build

The checked-in `build.bat` expects Visual Studio Build Tools 2022, CMake/Ninja, and a vcpkg checkout at `%USERPROFILE%\FuchsLabs\vcpkg`. It installs the pinned dependencies using the repository's vcpkg manifest and overlay port, builds x64 Release, runs `fl-tests.exe`, and copies the installer/app to `dist\FuchsLabs-USB-Display.exe`.

```bat
build.bat
```

Dependencies are linked statically, including D++ and OpenSSL. OpenSSL on Windows loads trust roots from the Windows certificate store. The D++ vcpkg overlay records the corresponding build option and patches.

## Diagnostics

- The application log is `usb-display.log` next to the installed EXE.
- `--demo` renders sample pages; `--no-lcd` starts without a display output; `--preview <folder>` writes preview PNGs; `--log <file>` selects a log path; `-c <config.toml>` selects a config file.
- The test executable accepts `render <folder> [de|en]` to create renderer samples. It is a developer tool, not included in the one-file release.
- If the serial display cannot open its COM port, close the vendor monitor program and check that another app is not using that port.

## Validation status

Release build: 86 checks passed. The native settings window was opened and visibly verified on Windows. Version 1.x and C++ 2.0 both reported display_ok=true on COM7 using the same settings; the user confirmed correct orientation, colours, readability and page changes. The integrated installer migrated the settings and updated autostart. The user subsequently confirmed a successful Windows reboot with the tray icon appearing automatically. See [TESTPROTOKOLL.md](TESTPROTOKOLL.md). The EXE is unsigned and requires the WebView2 Evergreen Runtime.

## Installing from automation

Run the installer in the ordinary Windows user context. A packaged host can redirect LocalAppData and registry writes into its package sandbox; its own file/registry checks then do not establish that Windows sign-in sees those files. The first real reboot test exposed this on 22 October 2026. Reinstallation from an ordinary Windows process verified the actual EXE, hash and Run entry outside the package context; the user confirmed the final Windows reboot retest succeeded, with the tray icon appearing automatically.
