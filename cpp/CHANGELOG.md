# Changelog

## 2.0.0 — 2026-10-22 (prerelease)

- Reimplemented the Windows display application in C++20.
- Added a single-EXE installer, updater, and uninstaller with Version 1.x configuration migration.
- Ported the system, friends, Battlefield 6, RSS, renderer, touch/monitor, tray, and local settings features.
- Integrated D++ for Discord and Windows DPAPI for saved tokens.
- Added a native Windows settings window using the Microsoft WebView2 SDK and static loader.
- Added a local test suite and static dependency build.

Hardware comparison on COM7 and the native settings window were validated on 2026-10-22. The first reboot test failed due to packaged-host installation redirection; the user confirmed the corrected normal-user installation starts automatically with its tray icon after a real Windows reboot. The EXE remains unsigned. See [test record](TESTPROTOKOLL.md).