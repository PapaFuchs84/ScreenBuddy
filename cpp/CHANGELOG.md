# Changelog

## Unreleased

- Documentation follow-up for 2.0.3: current download links, matching German/English HDMI/touch steps in the guide templates and HTML resources, validation status, and permanent documentation rules. Updated embedded guides will be included in the next app build; the existing 2.0.3 EXE is unchanged.

## 2.0.7 – local, not published

- Long page titles are shortened in narrow headers so the player count and clock remain readable.
- Tapping a BF6 player opens their GameTools profile in the browser; the leaderboard also shows hours played.
- Tapping friends prefers online accounts, using Discord → Steam → Xbox order within each group; checks registered apps and falls through on launch failures. Steam opens the chat; Discord/Xbox open the profile with access to Message (user-approved variant). Window/tab choice belongs to the platform app/browser.
- Selection uses the displayed snapshot, sorting and subpage, including wide tiles. Blog targets also use the rendered snapshot.
- German/English usage, troubleshooting and embedded guides updated. Automated checks open no real chats. Physical touch/platform acceptance remains open; see [test record](TESTPROTOKOLL.md).

## 2.0.6 — 2026-10-23

- Tapping a blog post on a touch display opens the article in the default browser (http/https links only).
- Fixed: after saving, the settings window no longer disappears into the tray; the restarted app reopens it (`--settings`).
- Settings server binds its port with SO_EXCLUSIVEADDRUSE (httplib set SO_REUSEADDR, so a second instance could bind the same port).
- LICENSE and THIRD-PARTY-NOTICES.md are embedded as resources and written to the installation folder; removed on uninstall.
- Published as v2.0.6.

## 2.0.5 — 2026-10-23

- HDMI/USB-C output hides the Windows taskbar on the display's screen while the app shows there (restored when the output is hidden or the app quits). Before, a touch near the bottom edge opened the Start menu instead of reaching the app.
- Touch on: new configurations start with touch enabled; a touch while it is switched off writes a hint to the log.
- Every touch gesture is logged (distance and result), so problems on a touch display can be diagnosed from the log.
- Handed to a tester only (touch confirmed on real hardware); included in 2.0.6.

## 2.0.4 — 2026-10-22

- Overview adds Previous view / Next view, live page position, and disabled controls when no further view is available. Uses the existing touch navigation, including list subpages and wide tile groups. Automatic cycling resumes with a full page duration.
- Added loopback-only GET /api/pages and validated POST /api/pages/step (delta -1 or 1), protected by the existing request checks. No config write or restart is needed.
- German/English documentation and embedded guides updated. Installed locally with matching version/hash, unchanged settings, preserved autostart and display_ok=true. Published as v2.0.4 on 2026-10-23.

## 2.0.3 — 2026-10-22

- HDMI/USB-C output now stays above the taskbar without taking keyboard focus, including after hiding/restoring the display and Explorer/taskbar changes.
- Touch uses native Windows pointer events instead of mouse emulation, with one gesture per contact and cancellation handling.
- Display settings show the Windows touch-to-monitor assignment and warn about another or unknown target monitor.
- Added a Windows touch setup button. The fullscreen output pauses while the assignment wizard is open and resumes afterwards. Physical assignment requires touching the intended screen once in the wizard.

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
