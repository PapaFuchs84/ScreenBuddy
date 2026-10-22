# Changelog

## 2.0.2 — 2026-10-22

- Illustrated guide (DE/EN) updated for 2.0: new texts and new screenshots of the current settings page (previously still from 1.1).
- The guide opens in the default browser, no longer inside the settings window; new tray menu entry *Open guide*.

## 2.0.1 — 2026-10-22

- Fixed: Discord stayed on "Connecting …" forever if there was no internet when the app started; it now retries after 90 seconds.
- Fixed: services answering with `Content-Encoding: deflate` (e.g. Open-Meteo) failed to load; only gzip is requested now.

## 2.0.0 — 2026-10-22 (prerelease)

- Reimplemented the Windows display application in C++20.
- Added a single-EXE installer, updater, and uninstaller with Version 1.x configuration migration.
- Ported the system, friends, Battlefield 6, RSS, renderer, touch/monitor, tray, and local settings features.
- Integrated D++ for Discord and Windows DPAPI for saved tokens.
- Added a native Windows settings window using the Microsoft WebView2 SDK and static loader.
- Added a local test suite and static dependency build.

Hardware comparison on COM7 and the native settings window were validated on 2026-10-22. The first reboot test failed due to packaged-host installation redirection; the user confirmed the corrected normal-user installation starts automatically with its tray icon after a real Windows reboot. The EXE remains unsigned. See [test record](TESTPROTOKOLL.md).