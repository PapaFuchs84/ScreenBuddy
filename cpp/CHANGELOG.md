# Changelog

## Unreleased

- Plugin platform phase 1: `Manifest` gains `license`, `author`, `category`, `min_app_version` (optional; invalid values are rejected, schema 1 unchanged); new `store_problems()` and `app_compatible()` in `plugins.cpp`; new tool `tools/plugin_validate.cpp` (CMake target `fl-plugin-validate`, Linux script `tests/build-validator-linux.sh`, workflow `Plugin validator`). `fl-plugin-tests`: 249 checks passed (was 223); 41 example checks passed. Verified on Linux with g++ only; the Windows/MSVC build of the new target was not run here. See [package format](../docs/PLUGIN-PACKAGE-FORMAT.md).

- GitHub repository renamed to `PapaFuchs84/ScreenBuddy` (formerly `fuchslabs-usb-display`). Update check, release links, guides and clone commands use the new address. Installed versions still find updates through GitHub's redirect.

## 2.5.0 – 27 October 2026

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

### Historical development notes (earlier state)

- 27 October 2026: validated the existing 2.5.0 candidate with component, native UI, API, integration and isolated real-2.3.0 upgrade checks; synchronized dist with the tested executable. No production installation or publication. Prepared signed Android 0.2.1-preview separately; hardware acceptance remains pending.

- 27 October 2026: removed the legacy HTML settings endpoint and embedded resource while retaining Companion/API and `/anleitung`. Regenerated the DE/EN illustrated guides from full anonymous native captures. Rebuilt ScreenBuddy and passed the component, isolated UI, installer and 51 navigation/API checks; browser layout acceptance remains open.

- Native Updates & Autostart: a reported newer release can open its validated GitHub release page in the default browser. The action is disabled when the URL is absent or invalid; it never downloads or installs automatically.
- Native settings, phase 2: the self-drawn Direct2D/DirectWrite UI now includes native pages for Discord, Friends, Home Assistant, Companion and Colors & design, in addition to the existing display, page-order, app and plugin pages. The current native settings smoke test passes 27 checks in an isolated `--no-lcd` instance; this checks selected controls and workflows, not complete visual/interaction acceptance or full feature parity.
- Native setup links on Overview and touch assignment on Display: status from Windows, direct wizard launch and refresh. The wizard still requires physical touch acceptance.
- Settings now launch only the native Direct2D/DirectWrite window. The active WebView2 fallback and static loader dependency were removed; native startup failures are logged and shown on request. The loopback API, Companion endpoints and embedded guide remain; the former HTML settings page is no longer embedded or served at `/`. Its source file remains for historical tests. Build and smoke verification passed on 26 October 2026.
- Native monitor chooser on Display lists detected screens and resolutions, supports refresh, and preserves a configured monitor that is currently disconnected.
- mDNS status fix: report success only after successful registration, and log registration and deregistration separately. Full build and tests passed after the fix; the error callback still has no dedicated mock test.
- Plugin examples and tutorial: `examples/plugins/` now includes the offline clock, configurable offline focus board and optional public Open-Meteo weather source. The paired German/English tutorials explain importing, configuring, host consent, sample data and limits. `fl-plugin-example-tests` parsed and rendered all three manifests: 41 checks passed, 0 failed. These checks do not make live HTTPS requests or verify hardware output. See [examples](examples/plugins/README.md), [DE tutorial](../docs/PLUGIN-TUTORIAL.md) and [EN tutorial](../docs/PLUGIN-TUTORIAL.en.md).
- Development line: ScreenBuddy executable and installer name; validated JSON manifests and source fetching with host consent. Earlier dev.2 builds included a WebView2 settings window; current source uses only the native window. One public HTTPS source was fetched in isolation; the full app consent flow has not been accepted.
- Plugin DNS fix: `GetAddrInfoExW` completion now uses a separate wait event while `OVERLAPPED.hEvent` stays null, as required with a completion callback. The previous combination returned `WSAEINVAL` on Windows and blocked live HTTPS. A local numeric-address WinSock regression test, an isolated public Open-Meteo fetch and the later full build with 134 transport checks passed.
- Current 26 October 2026 build: 272 core, 223 plugin parser, 127 store, 134 transport, 135 plugin-settings, 41 plugin-example, 11 monitor, 335 Companion-protocol, 27 native UI, 16 mouse, 16 Companion rebuild, 39 isolated app, 49 navigation and 2 installer checks passed. Real target-system upgrade and full UI/final-build acceptance remain open; the user reports the earlier image fault resolved.
- Security hardening after review: stored plugin secrets are discarded when a manifest update changes the source (host, URL or headers); Windows device names (`con`, `nul`, `com1`–`com9`, `lpt1`–`lpt9`) are rejected as plugin IDs; plugin secrets use their own DPAPI entropy with automatic migration; bidi and zero-width characters are rejected in manifest text; `.` and `..` are rejected as setting values in URL paths; DNS completion is protected against double approval. Open: certificate revocation checks (CRL/OCSP) and full external-source acceptance in the app.
- No 2.5 release or download; 2.3.0 remains the latest published version. See [development status](../docs/SCREENBUDDY-2.5.md).

## 2.3.0 – 2026-10-24

- Independent output state, page filters and timers; shared data sources/theme. Native resolution from authenticated `hello`/`resize`, bounded to 2560 pixels per edge and 4,194,304 pixels, first connected reporting device owns sizing. Each cached JPEG carries dimensions from the same snapshot; `ready` updates precede the corresponding JPEG. Touch coordinates from older dimensions are dropped.
- `/api/companion/pages`, `/api/companion/pages/step`, `/api/companion/preview.png` provide independent local controls. Extra DE/EN settings appear only when Companion is active. Companion disabled preserves the existing main page order/visibility.
- Installer now migrates settings beside a portable EXE when creating a local installation. Existing settings are snapshotted before updates; plaintext credentials are re-serialized with DPAPI encryption in the backup.
- Supports `swipe.dir` strings left/right and the tested numeric quick fix. Swipe/touch share the 10-per-second limit; resize is authenticated and limited to 4 per second.
- Fixed: Tray notification icon initialization uses non-blocking `WM_TIMER` retries instead of synchronous `Sleep`, preventing up to 30 seconds shutdown delays on `POST /api/quit` in headless/test environments.
- Fixed: Uninstaller with `--purge` now removes `config.toml.install-backup`, `companion.json` (with temporary/backup files), and `update-check.json`.
- Fixed: Installer copies existing `companion.json` when migrating settings alongside a portable EXE.
- Fixed: Guide templates (`anleitung.template.*`) and documentation synchronized for 2.3.0 release consistency.
- Performance & Cleanup: Optimized multi-tile frame pasting using row-level `std::memcpy` in `paste()`, added stack-buffer fast path to `strprintf()` eliminating dynamic allocations for common messages, replaced regex and substring conversions in `parse_color()` with fast zero-allocation hex parsing, added inline flat cache for Direct2D brushes in `Canvas::Impl`, avoided string formatting overhead when debug logging is disabled via relaxed atomic `is_verbose()`, enabled compiler string pooling (`/GF`) and linker security hardening (`/DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA`), and resolved all compiler conversion and deprecation warnings in `render.cpp` and `sysinfo.cpp`.
- Current Windows and Android checks: see TESTPROTOKOLL.md. Historical pre-Windows prototype statements below describe the earlier implementation.

### Companion foundation

- Companion output (`NetworkDisplay`, `output.h`/`network_display.cpp`): `display.output = "network"` (480 × 320) or `[companion] enabled = true` as a mirror next to the serial/monitor output. Platform-neutral core in `companion.cpp` (RFC 6455 handshake/framing, JSON messages, `Pairing` with 6-digit PIN, 120 s validity, single use, lock for 300 s after 5 failures, `FrameGate` ≤ 5 fps, `Store` for `companion.json`, pairing URI and QR SVG) and `companion_server.cpp` (OpenSSL TLS ≥ 1.2, ECDSA P-256 self-signed certificate generated once, listeners only on private IPv4 addresses, peer must be private too, max. 4 connections, `Origin` header rejected, ping every 5 s, 15 s silence closes). Windows part: LAN addresses via `GetAdaptersAddresses`, mDNS via `DnsServiceRegister` (loaded at runtime), JPEG via WIC (`encode_jpeg`), keys via DPAPI, taps on an STA thread into `App::tap_at` / page step. Settings: tab **Companion** (DE/EN) with QR code, PIN countdown, fingerprint, connected and paired devices; endpoints `/api/companion/status|pair|pair/cancel|devices/remove`. Config `[companion] enabled=false, port=8777` (rejected: 8765 or the settings port while in use).
- Companion hardening (review findings): bounded input before and after login (read budget per loop, at most 64 KiB unparsed per connection, no busy loop on a partial frame); one deadline of 10 s from `accept()` for TLS handshake, HTTP upgrade and `hello` (non-blocking, `stop()` no longer waits for slow clients); no exception leaves a worker thread (storage errors during pairing answer `storage`, non-`std` exceptions from hooks are caught); a device removed while it is logging in is rejected (device id is attached before the store is re-checked, so `drop_device` or the check always catches it); after a server (re)start the current image is sent as initial frame even if it did not change; `poll()` instead of `select()` on POSIX (descriptors ≥ `FD_SETSIZE`), listener count capped at `FD_SETSIZE` on Windows; WebSocket lengths must be minimally encoded, close payloads need a valid code and UTF-8 reason. mDNS (Windows): callbacks may arrive after the 2 s waits, so the registration context is reference-counted per pending request and `dnsapi.dll` stays loaded; `mdns_` is atomic.
- New dependency: Nayuki QR Code generator 1.8.0 (MIT), vendored in `third_party/qrcodegen`. OpenSSL is now listed directly in `vcpkg.json` (was already linked through D++).
- Tests: the earlier platform-neutral `fl-companion-tests` and Linux ASan/UBSan/TSan run covered the TLS protocol foundation. The later Windows integration, TLS tests and Redmi preview checks are recorded under 2.3.0 in [TESTPROTOKOLL.md](TESTPROTOKOLL.md).

## 2.2.0 – 2026-10-24

- System page touch: `SysRenderer::zone_at` maps taps to the RAM, drive and network cards (same geometry as `render()`); `touch::open_system` opens Task Manager, `control.exe appwiz.cpl` or https://www.speedtest.net/de. Only the system page with touch active reacts.
- New `update.cpp`/`update.h`: `UpdateChecker` queries `releases/latest` of the public repository at most once a day (first check 45 s after start, retry after 6 h on failure, manual check at most every 5 min), keeps the state in `update-check.json`, and reports only non-draft, non-prerelease versions with a github.com project URL. Tray balloon (`Tray::notify`, click opens settings), `/api/status` field `update`, `POST /api/update/check`, setting `[ui] update_check` (default true). Skipped in test instances and preview runs; nothing is downloaded or installed.
- Tests: hit areas of the system page, version comparison, parsing of the GitHub response.

- Documentation follow-up for 2.0.3: current download links, matching German/English HDMI/touch steps in the guide templates and HTML resources, validation status, and permanent documentation rules. Updated embedded guides will be included in the next app build; the existing 2.0.3 EXE is unchanged.

## 2.1.0 – 2026-10-23

Combined state of the two latest changes; 2.0.9 is not released on its own.

- New Home Assistant page: up to 4 entities. Sensors show value and unit, switches/lights/fans/input booleans show On/Off and can be flipped by tap when touch is active (only sensors can be chosen without touch). REST API with a DPAPI-protected long-lived token, 15 s polling, entity picker in the settings. Locks, covers and alarms are not switchable.

- Tapping a friend now follows a fixed order: Steam chat, otherwise the Discord profile (`discord://-/users/<id>`, choose Message there), otherwise the Xbox profile on xbox.com in the browser. Missing apps fall through to the next target; online status is no longer considered.

## 2.0.8 – 2026-10-23

- Tapping a BF6 player now opens the GameTools profile with the website's platform slug (`pc`, `xboxone`, `ps4`) instead of `ea`, which produced a blank page. Players GameTools does not know for PC still show "not found" on the website although the API returns them.
- Tapping a friend now only opens the Steam chat; Discord and Xbox open nothing for now (no chat link available), the 2.0.7 profile variant is removed.

## 2.0.7 – not published on its own, included in 2.0.8

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
