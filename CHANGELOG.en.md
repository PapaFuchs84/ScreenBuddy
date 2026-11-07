# Changelog – ScreenBuddy

[Deutsch](CHANGELOG.md) · **English**

All notable changes to the app. Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), versions follow [SemVer](https://semver.org/).

## [Unreleased]

BF6 leaderboard: players can now be entered by ID (`Name#ID`, `Name@psn#ID`; existing entries keep working). Lookups use `playerid`, the name is display-only. After a successful name lookup the app remembers the ID and writes it to the configuration; on failure it falls back to the player search and lowercase names. "Check" shows the ID found (web UI with "Use ID" and an optional ID field); the web link points to the ID page when an ID is known. The Python variant now uses the `pc, ea` platform order.

P9-9: Detect anti-cheat-protected games with unavailable image paths using Toolhelp PID/basename, a bounded library cache and foreground/fullscreen evidence; Battlefield 6 uses its Steam appmanifest even with RunningAppID 0. EA anti-cheat/helper exclusions, stable PID/basename identity and synthetic regression tests. Live game/hardware acceptance pending; no release.

P9-8: Reserve the selected Game-Mode view from normal rotation, Companion navigation and tap destinations while enabled. Direct EXE icons/package logos, consistent game cards, larger values, updated template and 60-second FPS/frametime sparklines. Isolated rotation/intro checks and synthetic regression tests; real games and physical displays remain pending.

P9-7: Read-only library discovery for GOG Galaxy, Ubisoft Connect, EA App/Origin, Battle.net/Blizzard and Rockstar. Replaceable registry access, synthetic parser/filter tests, fullscreen gate also for GOG/Epic EXE matches; extended launcher/anti-cheat exclusions. DE/EN docs and help updated; validation and pending real launcher acceptance in the [plan](docs/PLAN-GAME-MODE.md).

BF6 leaderboard: when the source (gametools) answers with HTTP 5xx, the app now shows "source unavailable" instead of "not found" (leaderboard, "Check", web UI); the last good values and remembered IDs are kept. Network errors and 404 behave as before. A player's touch/web link with an ID now opens `tracker.gg/bf6/profile/<ID>/overview` (same ID as gametools).

P9-6: 26 automated suites passed, including 114 Game-Mode checks; isolated dummy simulation on UI 18766 produced intro/active/restored previews and passed. AI receiver disabled because 18765 was occupied. PresentMon/hardware limits and unexecuted installer tests in the [plan](docs/PLAN-GAME-MODE.md).

P9-5: Game-Mode settings, display/view selection, additional games/exclusions, editor template and palette. Background discovery with asynchronous logos, intro and paused normal rotation; rotation resumes after the game ends. DE/EN usage and embedded guides synchronized. Local development, no release.

P9-4: 16 additional single system tiles for Game-Mode, DXGI Present-call cadence/frametime/1% low, NVML/AMD ADL/PDH, per-core CPU load/clock, VRAM and ICMP ping/jitter. Missing sources show n/a; standard pages unchanged.

P9-3: 2.5-second intro with dimming, eased logo, shine and existing FadeCache; reduced transition without scaling/shine.

P9-2: Steam logos from fixed public asset URLs, bounded local cache (7 days), executable icon fallback; retrieval reserved for background work.

P9-1: Read-only game discovery with Steam/Epic parsers, Xbox/full-screen paths and 3/5-second debounce. New offline unit tests; integration follows.

P8-4 (2026-11-07): Merge AI Status; Claude/Codex/limits tiles in Custom views, existing renderer/frame schedule, public conditions and page touch targets, additive DE/EN editor snapshots. Standard page baselines unchanged. Port 18765 collision documented; no port change or release. [P8-4](docs/PHASE8-EIGENE-ANSICHTEN.md).

P8-2 (2026-11-07): Individual list, Spotify and native HA entity tiles reuse existing painters. Dedicated view editor with management, palette, grid, properties, undo/redo and device inheritance. Only active display tabs, fallback to Standard on disconnect, saved offline layouts retained. Complete-page contracts unchanged; no release/push. [Usage and validation](docs/PHASE8-EIGENE-ANSICHTEN.md).

P8-1 (2026-11-07): Custom views with a bounded JSON model, backup/atomic persistence, device inheritance and `custom:` page-cycle integration. System cards, plugin v2 blocks and source status use existing renderers; HA, list/Spotify content tiles, editor and touch actions remain open. Scope and validation: [Phase 8](docs/PHASE8-EIGENE-ANSICHTEN.md).

- AI Status: existing account consent now reads Windows login files for unofficial Claude/Codex HTTPS usage; wiped token buffers, no refresh, independent 3/15-minute scheduling, configurable receiver port with fallback. Offline synthetic validation; no release.
- AI Status (`ai`, C++): Claude Code/Codex cards, frame-driven status/session animations, loopback events on 18765 with a dedicated DPAPI key, hook preview with one-time backup and selective removal, and local `ai.*` tile data. Limits/cost/context/tokens remain `n/a` without explicit measurements. DE/EN guide and new fl-tests checks; executed validation and pending live/hardware acceptance are recorded in `cpp/ai-status-report.md`. No release.

P7b-0 (2026-11-06, Claude Sonnet 5.5 on behalf): New default dark Material with orange accent (`material-orange`; text/surface and accent/surface at least 4.5:1, bar at least 3:1, contrast tests in fl-tests and fl-layout-ui-tests). New installs start with it; existing installs get a one-time DE/EN dialog "Set the new default?" (default No, answer stored in `ui.standard_theme_choice`, closing changes nothing). Cupertino, Fluent, Light, Dark, Windows and custom themes stay selectable. Existing built-in/plugin hash files unchanged, new baseline is additive in `p7b0-material-orange-hashes.txt` (768 hashes).

P7b-6 (2026-11-06, Agent Codex CLI 0.166.0): Per-device built-in/plugin overlays inheriting the standard (up to 16 per page, 16 KiB per overlay, 256 KiB total), atomic persistence/backup and shared device selection for images/touch; display tabs with size/orientation, offline profiles and discard confirmation. Mark Android TV on the PC (protocol v1 has no device type) to enforce landscape. 14 suites passed (Companion offline), eleven existing hash files unchanged; physical multi-device acceptance pending.

P7b-1 (2026-11-06, Agent Claude Sonnet 5.5): Editor window layout: header bar (page selector, device-size chip, undo/redo/apply/reset), left rail with Tiles/Layers/Pages, canvas and properties as a three-column shell (wide from 900 DIP; medium: properties below the canvas; narrow: stacked, palette collapsed, section choice survives page switches); display tab bar unchanged. New `ui_editor.*`. 14 suites green, baselines bit-identical; hand acceptance open.

P7b-2 (2026-11-06, Agent Claude Sonnet 5.5): Tile palette with search, filter chips (All/System/Time/Media/Games/Other) and a grid of live mini previews (same page renderer with sample data: a crop of the standard page, or a single zone for plugins; cached per page/tile/size/display theme, loaded only on page switch, tiles without data marked "no data"). Drag onto the canvas, click inserts at the first free spot (one undo step), keyboard (arrows, Space, Enter, Delete, F6), MSAA "Tile palette". New `ui_tile_palette.*`. 14 suites green, baselines bit-identical.

P7b-3 (2026-11-06, Agent Claude Sonnet 5.5): Canvas in the aspect ratio of the display tab: device-size frame, subtle cell grid, dashed drop slots "drop here", snapping ghost (invalid: dashed red plus text "does not fit"), handles, trash, zoom -/+/fit. Opt-in `GridEditor::canvas`; the previous grid stays bit-identical. Editor snapshots additive (`p7b-editor-hashes.txt`), text contrast >= 4.5:1 and focus ring/border >= 3:1 verified in all themes. 14 suites green.

P7b-4 (2026-11-06, Agent Claude Sonnet 5.5): Layer list (order, visibility; selection works in list and canvas), page list (switching asks about unsaved changes) and starter card "In three steps" (dismiss once, remembered in `layouts/editor-intro.done`). 14 suites green (fl-layout-ui-tests 1,615), baselines bit-identical; hand acceptance open.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Pure built-in catalog and strict versioned overlays; standard keeps the fixed renderer; HA deferred. 14 suites passed (Companion offline: 250); built-in 4,221 images/hit maps unchanged, plugin 2,093 checks passed; physical acceptance pending.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomic per-page layouts with backup/revision, sample preview, arranged touch targets and isolated clipping; fixed rendering unchanged. 14 suites passed (editor 835; Companion offline 250); built-in 4,221 images/hit maps and plugin baselines unchanged; hardware pending.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS editor with Posts/required Status, separate landscape/portrait layouts in `layouts/builtin.rss.layout.json`; opening or applying Standard keeps the pixel-identical fixed path, real changes use the grid (spacing may shift, E2). Reset restores Standard; rows, paging and touch share one plan. Header/footer fixed; built-in cycles and HA deferred. 14 suites passed (catalog 217, editor 874, core 3,470; Companion offline 250); existing baselines unchanged, plus 1,024 RSS images/hit maps and 5,952 checks without errors; hardware pending.

P7-8 (2026-11-06, Agent Codex CLI 0.166.0): Streaming in the page selector; Titles/required Status from the existing snapshot, per-page landscape/portrait overlay, paging and touch from one grid plan. Standard/reset keep the fixed path; TMDB / JustWatch stays in the footer; compact rows omit the date. 14 suites passed (Companion offline); existing baselines bit-identical, 1,792 new grid images/hit maps added, 6,267 checks/0; hardware pending.

P7-9 (2026-11-06, Agent Codex CLI 0.166.0): BF6 in the page selector; Ranking/required Status, sorted snapshot, per-page landscape/portrait overlay. Grid capacity controls paging and player links; compact rows show name, K/D, rank or error. Standard/reset remain pixel-identical. 14 suites passed (Companion offline); existing baselines bit-identical, 1,024 new grid images/hit maps added, 4,390 checks/0; hardware pending.

P7-10 (2026-11-06, Agent Codex CLI 0.166.0): Friends and Discord in the page selector; shared Friends/required Status catalog and sorted snapshots, separate `builtin.friends.layout.json` / `builtin.discord.layout.json` overlays, backups and revisions. Grid rows, paging and profile targets share one plan; narrow compact rows omit avatars/badges. Standard/reset stay pixel-identical (E2 accepted). 14 suites passed (Companion offline); existing baselines bit-identical, 1,024 grid images/hit maps added per page, 8,622 checks/0 combined; hardware pending.

P7-11 (2026-11-06, Agent Codex CLI 0.166.0): System editor blocks use the existing CPU/GPU/RAM/disk/network snapshot; temperatures retain threshold colours, regular network includes history, compact shows values/bars or rates. Per-page landscape/portrait overlay, Cupertino groups per zone and tool targets from the drawing plan. Standard/reset remain pixel-identical; E2 confirmed. 14 suites passed (Companion offline); existing baselines unchanged, 1,536 additive system images/hit maps. Physical acceptance pending.

P7-12 (2026-11-06, Agent Codex CLI 0.166.0): Spotify editor blocks use the existing snapshot/meter for cover, title/artist/album, progress, controls, levels and required status; last played keeps progress/controls hidden. Per-page landscape/portrait overlay, compact single-line text, three level colour modes and separate 48-point button targets from the drawing arrangement. Standard/reset remain pixel-identical; 14 suites passed (Companion offline); existing baselines unchanged, 4,096 additive Spotify images/hit maps. Physical acceptance pending.


### 2.10.0 – in preparation (unreleased)

No release yet, no tag, no EXE distribution. Hardware, native UI and Companion acceptance and the APK build are still pending; see "Development status" below.

#### Highlights
- **Phase 8 P8-3 (2026-11-07):** The "Arrange page" entry is gone; standard and plugin pages are edited through the page list of **Custom views**. New: **touch target per tile** (Default or Custom: switch page, link only with consent, next item for plugin cycles only), stored per display overlay, falling back to Default when invalid.

- **Zone layout for plugin pages (manifest v2):** `schema_version: 2` describes a flat grid with zones and blocks that have stable block IDs. Manifest v1 is unchanged. Limits: 6 × 6 grid, 8 zones, at most 4 blocks per zone, up to 12 widgets with a layout.
- **Density levels:** Each block type (text, metric, clock, bar, list) has compact, regular and detailed drawing. If a block does not fit its area, it is hidden instead of overlapping.
- **Conditional display (`show_if`):** A block appears only when a value is present or a simple comparison holds. No expression language, no type conversion.
- **Fault isolation:** If evaluating or drawing one block fails, only that block is hidden; a rate-limited log entry without data is written.
- **Device profile and keepout per device:** Whole-pixel edges for the USB display and monitor (under **Display**), plus a shared default and an individual keepout per paired Companion device (in independent mode). Android TV always runs in landscape.
- **Cycling zones with a calm transition:** A zone can alternate between blocks (`rotate` mode with a 3–600 s interval, or `first_available`). Scheduling avoids needless redraws. The real serial frame rate has not been measured yet.
- **Tap targets with link consent:** Blocks can open HTTPS links (on the PC, including taps from a phone). Consent per plugin is **off** by default and shows the hosts.
- **Frame style from the theme:** Plugin cards take corner radius and spacing from the theme (FuchsLabs radius 9 → 8).
- **Contrast fixes:** Warning colour in the light theme, Windows high-contrast design and the Cupertino dark-profile header adjusted; the focus ring in the light FuchsLabs theme is darkened to Glut. Snapshot tests check text contrast ≥ 4.5:1 across a theme matrix.
- **Built-in pages on the shared frame:** System, Friends, Discord, Battlefield 6, Blogs (RSS), Streaming and Spotify draw their cards through the same frame (`draw_framed`). Home Assistant stays native. Bit-exact against the baselines (4221 images and hit maps, 0 differences on the reference machine).
- **Page editor:** Under **App → Arrange plugin page**, arrange blocks on a grid, set conditions, preview in landscape and portrait, undo/redo (100 steps), keyboard operation. Only the arrangement is saved, with a backup copy; the manifest stays unchanged.
- **Page editor with drag & drop:** Block palette; drag, drop and move blocks and zones; resize handles; trash area for removal; keyboard alternative pick up/drop (Space, Enter, Esc). One undo step per drag gesture. Cancelling restores the selection, releasing outside the grid cancels silently, targets that leave a zone without a shown block count as "does not fit", block chips work by keyboard (Ctrl+Up/Down, Space, Delete). Still an overlay, manifest unchanged. Mouse, touchpad and touch acceptance are still pending.

#### Fixed

- **BF6 leaderboard:** The community API (gametools) now rejects `platform=ea` and only finds players with `pc`. ScreenBuddy queries PC accounts with `pc` and falls back to `ea` and the lower-case name; PSN and Xbox try several platform values. The rank profile uses the same platform.
- **Spotify:** When nothing is playing, the page shows the **last played track** with cover ("Last played", dimmed, no buttons) instead of "Nothing is playing". This needs the extra Spotify permission "recently played": sign in once more under Settings › Spotify. Without it the previous message stays.

#### Security

- **Tap URLs:** HTTPS only, strict characters and percent escapes, lowercase hosts, no dot segments. The address is checked again at tap time before the browser starts.
- **Link consent:** Off by default; any other change to a plugin revokes its consent; only a pure change of the link list keeps it. At most one link attempt per plugin every two seconds. Denied taps do not change the page.
- **`show_if`:** Secret settings are forbidden in conditions; an invalid condition rejects the whole manifest.
- **Logging:** Warnings name only the plugin ID, block ID and stage, never data, setting values or exception text.

#### Compatibility

- Manifest v1, protocol v1 and the existing snapshot hashes are unchanged. Without `layout`, v1 and v2 draw as before in the automatic grid.
- Schema 2 requires `min_app_version` 2.10.0 or newer in the store; older apps reject it.
- Keepout: whole pixels 0–1000; each edge at most one third of the smaller frame edge. If fewer than 160 × 120 usable pixels remain, the keepout is ignored.
- Android TV always runs in landscape. A stored portrait choice is kept but ignored.
- Home Assistant stays native and is not part of the zone system.
- The published Companion app remains 1.0.3. Version 1.0.4 (versionCode 8) is a development state without signing or distribution.

#### Development status (open)

- Not released: no tag, no EXE, no Companion APK distribution.
- Open: physical acceptance on the USB display (including the portrait enclosure), monitor, Companion and Android TV; native visual check of the UI; measured serial frame rate for cycling zones; Companion process tests; APK build.
- Built-in pages: the baseline is machine-bound (Roboto as a user font is required). On the reference machine all migrated pages are bit-exact (0 differences); hardware acceptance is still pending.
- Phases in brief (details, test figures and limits: `cpp/TESTPROTOKOLL.md`):
  - Phase 2a/2b: manifest v2, zone geometry, zone drawing, density levels.
  - Phase 3: conditional display and fault isolation.
  - Phase 4a: device profiles, keepout, Android TV landscape.
  - Phase 4b: cycling zones.
  - Phase 4c: tap targets with link consent.
  - P5-0 to P5-6: baseline and migration of built-in pages to the shared frame.
  - P6-1 to P6-5 and review: arrangement overlay, page editor, focus ring.

### 2.9.6 – in preparation

Public Windows EXE releases now require trusted, timestamped Authenticode signing; without a certificate the export is blocked.

Microsoft Store preparation: MSIX packaging, separate user data, Store updates and package-stable autostart. Companion 1.0.4 (code 8): Android API 36 and setup-window insets. No Store publication has taken place.

## [2.9.5] – 2026-11-01

### Changed

- **Settings navigation:** "General" is now **View**, "Data sources" is now **Pages**. The "Connection" group is gone: Companion, Plugins and Plugin store sit under **App** (next to Updates & startup). **Colors & design** belongs to **View**.
- **Store themes** are browsed and installed directly under **Colors & design** (card "Themes from the store"); the installed theme is selected afterwards. The theme card was removed from the Plugin store page. Signed index, signature checks and the "Use the plugin store" switch are unchanged.

- Recalculate layout after asynchronous store-theme loading so rows do not cover install buttons. Refresh the DE/EN illustrated guides with anonymized menu captures.

## [2.9.4] – 2026-10-31

### Added

- Touching a streaming title opens its TMDB details in the default browser on the PC.
- Touching Spotify cover art opens the Spotify desktop app, without requiring Premium.
- TMDB key application explained with field examples in German/English help.

### Changed

- **Companion app 1.0.3** (versionCode 7): modern login/pairing design in light and dark following the system setting, contrast at least 4.5:1, rounded buttons and fields, visible focus ring for remote control. Protocol and app ID unchanged.

## [2.9.3] – 2026-10-31

### Added

- **PlayStation friends** in the combined friends list (badge “P”, online status and current game). Unofficial Sony interface via the NPSSO code, DPAPI-encrypted, optional and off by default; includes a how-to for the code, a connection check and linking via `psn:Name`.

### Fixed

- Sony authorization codes containing a dot (`v3.…`) are accepted during PlayStation sign-in; connection diagnostics are written to the log.

## [2.9.2] – 2026-10-31

### Added

- Distinct window compositions and navigation: Cupertino: collapsible sidebar and optional inspector; Fluent: page search and Back history; Material: five destinations, subpages and floating save actions for pending changes. System displays also use distinct compositions.

- Design systems include paired light/dark palettes, a working native appearance switch and six example files.

- Design systems: Cupertino grouped surfaces and spacious rows, Fluent compact geometry and selection indicator, Material tonal surfaces, pill shapes and state feedback. Optional strictly validated theme `style`, editor selection, native settings and display rendering; existing starter themes are recognized by exact names.

### Fixed

- Design systems: overview tile captions (“Pages on the display”) wrap instead of being clipped; slider track in light Material/Cupertino/Fluent is tinted from the accent instead of beige; Material and Cupertino system displays keep 8 px above the footer.
- Contrast of light design systems: sample themes and built-in palettes reach at least 4.5:1 for text, muted text, accent, header text, success/danger and 3:1 for dividers, status dots and bars; warning and offline colors darkened on the light display.
- Web API: `POST /api/config` without `{"config": …}` now returns an error (400) instead of a no-op `ok:true`.

## [2.9.1] – 2026-10-31

### Added

- **Amplifier for remote playback**: if Spotify plays on a phone, speaker or Connect and the system output stays silent for 4 s, the amplifier shows an animation derived from the track and a tempo, clearly labelled “simulated” (no measurement, no microphone, no Spotify analysis API). New “Bar colour” setting: level (green/yellow/red, default), theme gradient, or single colour.
- **Plugin store: themes**: color designs can be shared and installed through the signed store ("Themes" card in the plugin store). Separate signed list `themes.json`, colors only, no code; signature, size, SHA-256 and schema are checked, revoked themes are hidden, a theme with the same name is never overwritten. Three starter themes in `store-seed/themes`. Maintenance tool: `fl-plugin-sign build-themes|check-themes`.
- **Navigation**: mouse tooltip on the sidebar status dots.
- Help for API keys and tokens: under the credential fields for Discord, Steam, Xbox, TMDB, Spotify and Home Assistant a card “How to get …” explains in short steps (DE/EN) where the key is created and opens the official page with a button. Only fixed https addresses are opened; keys are never read or logged. Guide and docs updated.

### Fixed

- Amplifier: keep visible Spotify playback active during silence so the simulated animation can start after four seconds.

- Amplifier: default audio device changes are detected and the WASAPI connection is rebuilt.
- Crash fix in the brush cache of `gfx.cpp` (from #37, 3d92295); possibly affected 2.9.0 as well.
- Contrast correction: adjustments against background and card no longer cancel each other (all rules of one color are satisfied together).

## [2.9.0] – 2026-10-31

### Added

- **Colors & design**: new "Match Windows" option (Windows light/dark and accent color, follows changes live, high contrast uses the system colors; text and bars are contrast-corrected automatically). Custom themes as JSON files with editor, live preview, contrast warning, duplicate, delete, import and export; "Custom colors" migrate automatically. Three example themes (Cupertino, Material, Fluent style) in `cpp/examples/themes`. The settings window offers "Windows" as appearance.
- **Navigation**: status dots only for data sources and connections (green = set up and on, grey = not set up or off), with a legend.
- **Companion / Android TV:** own Android TV launcher entry (leanback, with banner); remote: left/right switches the page (one press, one switch), OK has no action, Back unchanged; short hint on connect. Without a camera the QR scan is not offered and no camera permission is requested; manual pairing remains. Checked on a BeyondTV (Android 11) with ADB key input; physical remote, standby/CEC and long-term runs are not tested.
- **Spotify: amplifier display**: while music plays, 16 bars show the level of the Windows system output (spectrum, about 8 frames per second, landscape only). The system output is read locally via WASAPI loopback; nothing is recorded or stored, and the analysis only runs while the Spotify page is visible. Switch: Settings → Spotify → *Amplifier display* (spotify.amp, on by default). Applies to the display, monitor and Companion; the actual frame rate depends on the output device.
- **New on streaming: official TMDB logo** in the attribution (README, `THIRD-PARTY-NOTICES.md`) next to the required notice; file `docs/logo/tmdb.svg`.

### Fixed

- Streaming relaunches: the page showed almost only Netflix because all chosen providers were queried together and sorted by date. The app now queries each provider separately (up to 8 groups) and picks the newest title per provider in turn, so every chosen provider appears with its logo.
- Spotify amplifier: activate analysis when drawing the visible playback page; analysis expires within three seconds after leaving it. Residual bars decay fully to zero.
- **Companion / Android TV:** Visible setup now keeps the screen on and prevents the screensaver. The full-screen display retains its existing protection; normal TV settings apply after leaving the app.

### Not included

- Sharing themes through the plugin store (type "theme"), a mouse tooltip for the status dots and the amplifier for remote playback (Spotify on a phone or speaker) are **not** part of 2.9.0.

## [2.8.0] – 2026-10-31

### Added

- **Settings**: the Overview shortcuts now have icons plus new buttons for the guide (German/English) and the Plugin workshop. The right-hand “Device” panel shows every connected device (USB display or monitor, each Companion app with name, IP address and page position) as its own selectable card; the live preview shows the selected device.
- **Spotify page**: current track with cover art, progress and – with touch – buttons for previous, play/pause and next (Spotify Web API). Sign-in uses PKCE without a client secret and your own Spotify developer app; tokens are DPAPI-encrypted in `spotify.token`. Settings → Spotify, docs: [docs/SPOTIFY.en.md](docs/SPOTIFY.en.md). Covered by automated checks (PKCE test vector, reply parsing, buttons, rendering); the real Spotify login has not yet been tested with an account.
- **New on streaming**: new display page with movies and series of the last 30 days on the chosen streaming providers (title, date, movie/series, provider abbreviation). Source: TMDB API v3 (GET only over HTTPS, 1 h cache), provider data from JustWatch. Settings: page on/off, title, TMDB key (masked, DPAPI), region (automatic from the language, overridable), provider multi-select from the TMDB list (preset: Netflix, Prime Video, Disney+, Apple TV+, WOW, RTL+) and number of entries. Without a key the page shows “No key”, on errors or offline “No data”. Required credit “This product uses the TMDB API but is not endorsed or certified by TMDB. Provider data: JustWatch.” in the settings, the guide and `THIRD-PARTY-NOTICES.md`; footer “TMDB / JustWatch” on the page. New config section `[streaming]`, new page key `streaming`. The official TMDB logo is not included yet. Only the logic was tested automatically on Linux; the Windows UI and display have not been built or tested on a device yet.
- **New on streaming: provider logos** instead of abbreviations. Logos come from TMDB/JustWatch (provider `logo_path`, image from `image.tmdb.org`, w92), are fetched via HTTPS GET only without redirects (size limit, PNG/JPEG only), cached once under `cache/tmdb-logos/<provider id>.png` in the data folder and scaled to row height with rounded corners. At most 3 logos per title, the rest as "+n". If a logo is missing the abbreviation is shown. Loading happens in the background thread. The logos are trademarks of the respective providers. Only the logic was tested on Linux; the Windows part and the on-device look are not yet verified.

### Fixed

- Settings window: the left navigation adapts to the window height and no longer runs under the footer (no overlap with “No unsaved changes”).
- **Companion app 1.0.1**: the background now runs edge to edge, while content stays inside the safe area clear of rounded display corners (Android 12+, radius from the system) and the camera cut-out; reported resolution and touch coordinates refer to the safe area. Verified on the Pixel 10 Pro (corners 157 px, cut-out at the top); Redmi 8 without rounding is unchanged.
- TMDB: the RTL+ preset uses TMDB ID 2750 (region DE) instead of 298.
- `--uninstall` aborts in test instances (`FUCHSLABS_INSTANCE`) without their own `FUCHSLABS_INSTALL_DIR` instead of deleting the real installation (EXE) while leaving the autostart entry and programs-list entry behind.

## [2.7.0] – 2026-10-31

### Added

- **Plugin Workshop**: kid-friendly interactive guide (German/English) for building your own plugins with a builder, live preview, bug detective and file export. In the app at `/plugin-werkstatt` (linked from the illustrated guide), as the file `cpp/res/PLUGIN-WERKSTATT.html`.
- Plugin store: preview images. Signed `previews.json` (separate from the index, older apps ignore it), shown in the store details, tools `fl-plugin-sign build-previews|check-previews` and `fl-plugin-preview` (renders sample images). Images for the 8 starter plugins in `store-seed/previews/`.
- Companion: several phones at once, independently controllable (`[companion] sync = false`) or synchronised (default). In independent mode every device has its own page position, resolution, swipe/tap handling and optionally its own page list (Settings → Companion, stored in `companion.json`); switch “Show the same on all devices”. Protocol unchanged. Tested with simulated devices; a test with several real phones is pending.
- Pages & order: enabled plugins are now listed, can be reordered with ▲/▼ and shown/hidden with the checkbox (config key `plugin:<id>` in `display.pages` / `display.visible_pages`). Newly enabled plugins appear last at first.

### Fixed

- The checkboxes in Pages & order now also apply without a running Companion output (previously `display.visible_pages` was only evaluated with Companion).

- Store installations and updates now keep offline plugins disabled until explicitly enabled. Reproduced with Three tasks on the real system; regression coverage includes fresh installation, reimport and reload.

## [2.6.0] – 2026-10-30

### Added

- Plugin platform, key: production key generated, public key entered in `plugin_sign.h`; tool `fl-plugin-sign check-index` for the store CI.
- Plugin platform, phase 4 (texts and starter plugins): [terms of use, rules and privacy](docs/PLUGIN-STORE-TERMS.md) (draft, not legally reviewed), [author guide](docs/PLUGIN-AUTHORS.md), "Report a plugin" issue template, plugin reports added to `SECURITY.en.md` and 8 starter plugins in `store-seed/plugins/` (validator and example tests pass). The store repository `PapaFuchs84/ScreenBuddy-Plugins` is live (index with 8 starter plugins); the signing key is generated and pinned in the app.

- Plugin platform, phase 3 (part 2): raw-byte HTTPS fetch, native "Plugin store" page (search, categories, hosts before install, install/update, plugins stay off), daily background sync, switch "Use the plugin store". Without a signing key the page reports "not available". See [Plugin signing](docs/PLUGIN-SIGNING.md#store-in-the-app-phase-3-part-2).
- Plugin platform, phase 3 (part 1, store logic): `plugin_catalog` loads the index and revocation list through an injected HTTPS fetch, verifies signature and rollback protection, keeps an offline cache that is verified again on load, lists with search/category/status (installed, update, unsupported, revoked), shows hosts an update would add, installs only signed and matching packages (plugin stays off, hosts without consent) and disables revoked plugins. Without a pinned production key the store reports "unavailable". **Still open:** native store page in the app and the real HTTPS fetch (Windows). Docs: [Plugin signing](docs/PLUGIN-SIGNING.md#store-logic-phase-3-part-1).
- Plugin platform, phase 2: Ed25519 signing for index and revocation list (`plugin_sign`), SHA-256 package check including host comparison, rollback protection and the `fl-plugin-sign` tool. The production key is still missing (maintainer generates it offline); the app does not download anything yet. Docs: [Plugin signing](docs/PLUGIN-SIGNING.md).
- Plugin platform, phase 1: manifest fields `license` (`MIT`/`CC0-1.0`), `author`, `category` and `min_app_version` (optional, schema stays 1), the `store_problems` and `app_compatible` checks and the offline validator `fl-plugin-validate` (`--store`, `--app-version`) with a GitHub Actions workflow. Example plugins carry the new fields. Docs: [package format](docs/PLUGIN-PACKAGE-FORMAT.md). There is no plugin store, signing or store UI yet.

### Fixed

- Plugin store: the “Use the plugin store” switch was not saved (the store page had no `collect()` method).
- Plugin store repository: plugin files are now also served from `docs/plugins/` because GitHub Pages only serves `docs/`; before, installation failed with “Plugin download failed”.
- Note: the phase entries above describe the development history; wording such as “no store yet” or “key missing” no longer applies in 2.6.0.

### Changed

- GitHub repository renamed to `PapaFuchs84/ScreenBuddy` (formerly `fuchslabs-usb-display`). Update check, release links, guides and clone commands use the new address. Installed versions still find updates through GitHub's redirect.

## [2.5.0] – 2026-10-27

### Added

- Native Direct2D/DirectWrite settings replace WebView2, with guided setup and display/touch selection.
- Local declarative plugins and optional experimental HTTPS sources with per-host consent.
- Signed Companion 0.2.1-preview with documented debug-to-release certificate migration.

### Validation

- Windows display/touch/native operation and real 2.3.0 upgrade/rollback passed; settings preserved.
- Redmi 8 landscape/portrait, real gestures, timer and saved-key reconnect passed over USB tethering.
- 1,429 Windows checks, 20 Android protocol tests and all 46 DE/EN guide views checked.

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

The Windows EXE is unsigned. Android uses a new release certificate; an old debug APK cannot be updated directly. [Release notes](docs/RELEASE-NOTES-2.5.md).

Historical development notes and earlier test states: [archive](docs/DEVELOPMENT-NOTES-2.5.en.md).

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
- Download and checksums: [v2.0.6](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.6). See [test record](cpp/TESTPROTOKOLL.md) for validation status.

## [2.0.5] – 2026-10-23

- HDMI/USB-C: while the display is shown, the Windows taskbar on that screen is hidden (a tap near the bottom used to open the Start menu); it comes back when the app quits.
- Every touch gesture is logged, with a hint when touch is switched off in the settings. New configurations start with touch enabled.
- Only handed to a tester (touch confirmed on real hardware), not published separately; included in 2.0.6.

## [2.0.4] – 2026-10-22

- Overview: Previous view and Next view control the running display, with its current position below the live preview. Includes list subpages and wide tile groups; automatic cycling continues.
- German/English controls are disabled when there are no views or all views fit on screen. No save or restart needed.
- Download and checksums: [v2.0.4](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.4). See [test record](cpp/TESTPROTOKOLL.md) for build and validation status.

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
- Download and checksums: [v2.0.0](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.0). [Test record](cpp/TESTPROTOKOLL.md).

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

[Unreleased]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.6.0...HEAD
[2.6.0]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.5.0...v2.6.0
[2.3.0]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.2.0...v2.3.0
[1.2]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v1.1...v1.2
[1.1]: https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v1.1
