# Menus and themes in ScreenBuddy 2.9.5

Page names remain the same across themes; the design determines how you reach them.

| Group | Settings |
| --- | --- |
| View | Overview, Display, Pages & order, Colors & design |
| Pages | System, Friends, Discord, Battlefield 6, Blogs (RSS), New on streaming, Home Assistant, Spotify |
| App | Companion, Plugins, Plugin store, Custom views with the page editor (from 2.10.0, in preparation; the former "Arrange page" entry is gone), Updates & startup |

## Navigation by theme

- **FuchsLabs:** full page list on the left; live preview and devices on the right.
- **Cupertino:** **☰** toggles the sidebar; **Preview** opens the device inspector. A hidden preview is not a connection failure.
- **Fluent:** filter page titles in the search field and open a result; **‹** returns to the previous page. Clear the search to see all pages again.
- **Material:** **View → Colors & design**. Five destinations: Overview, View, Pages, Extensions and App. Plugins and Plugin store have their own **Extensions** destination; Companion and updates are under **App**. Choose subpages below the heading; a dropdown replaces them when space is limited. **◧** toggles the inspector. Floating save actions appear when changes are pending.

## Install and apply a theme

1. Open **View → Colors & design** (Fluent can search for the page title).
2. Under **Themes from the store**, use **Refresh store**, select an entry and choose **Install theme**.
3. The installed theme is selected automatically. **Save & apply** keeps the selection and restarts ScreenBuddy.
4. Check the selection and preview after restart. Theme files are saved immediately; **Discard** resets a pending selection without uninstalling the file.

Themes are on Colors & design; plugins remain in Plugin store. **App → Plugin store → Use the plugin store** still controls requests for both signed catalogs. Enable it there and save if disabled. A store theme can contain colors only; it does not necessarily enable the similarly named design profile. **Design system** in the theme editor determines the layout. Light/Dark at the top affects the window; the color scheme on Colors & design affects display output. **Windows** can replace the palette with system colors.

## Images

New menu images were captured during release testing and anonymized afterwards. Covered accounts, identifiers, addresses and preview content are not real sample accounts. Raw images remain private. Display demos are renderer exports with synthetic data, not USB-panel photographs. [Light/dark theme gallery](screenshots/themes/README.md). The captured UI uses German labels; this guide provides the English page names.

## Device profile and keepout (Phase 4a, development)

**Display → Keepout** provides four numeric fields reachable with Tab: top/right/bottom/left (pixels, physical edge). USB uses native portrait 320 × 480 as its basis; the border follows orientation. Monitors follow Windows without software rotation. **Save & apply** (Ctrl+S) applies the values; the refreshed preview includes the border in the theme background colour.

**Companion → Keepout: shared default** applies in shared mode and supplies the default. For the selected paired device, save an individual keepout immediately or reset it to the shared default; this only applies with synchronisation off. Use it for TV overscan. Device previews include the border.

Only integer pixels 0–1000 are accepted; invalid input produces a message. Rendering limits each edge to one third of the smaller frame dimension. If less than 160 × 120 usable pixels remain, the entire keepout is ignored (rate-limited warning). Border taps do nothing; navigation thirds use the usable content width. Stale Companion frame sizes are rejected.

## Page editor inside Custom views (development for 2.10.0)

The former "Arrange page" entry is gone. Under **App → Custom views** pick a "Standard / plugin" entry in the page list and arrange the blocks of an enabled plugin with manifest v2 on a grid: define zones, assign blocks, set conditions (`show_if`) and check the preview in landscape and portrait. Undo and redo (100 steps) and keyboard operation are available. **Apply** saves only the arrangement, with a backup copy; plugin content, link consents and the manifest stay unchanged. Manifest v1 plugins only show "Arrangement needs manifest v2" there. RSS, Streaming, BF6, Friends, Discord, System and Spotify are included as built-in pages. Standard Home Assistant remains native; individual entities are available in the separate Custom views editor. Other entry point: the "Custom views …" button under **Pages & order**; the "Arrange …" button under Plugins is removed. Blocks can also be dragged with the mouse (block palette, handles, trash); keyboard: F6 switches the area, Space picks up, Enter drops, Esc cancels; Ctrl+Up/Down selects block chips in a zone (Space picks up, Delete returns to the palette). Releasing outside the grid cancels silently. Details: [Plugin system](PLUGIN-SYSTEM.en.md#page-editor-p6-3-to-p7-10-development-for-2100). Mouse, touchpad and touch operation are not yet accepted.

## Plugin taps (Phase 4c, development)

Schema 2 plugins can offer HTTPS links and the next cycling-zone entry. Links open on the PC, including taps from a phone. The separate grant in Settings → Plugins defaults to off and displays the link hosts. Targets require 1.0 s stability on Companion or 0.3 s on a monitor and are blocked during transitions; denied taps do not navigate. Areas without a target retain thirds navigation. Android TV gains no tap/D-pad action. See the [plugin system](PLUGIN-SYSTEM.en.md#tap-targets-for-plugin-blocks-phase-4c-development-for-2100). Physical acceptance is pending.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Pure built-in catalog and strict versioned overlays; standard keeps the fixed renderer; HA deferred. 14 suites passed (Companion offline: 250); built-in 4,221 images/hit maps unchanged, plugin 2,093 checks passed; physical acceptance pending.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomic per-page layouts with backup/revision, sample preview, arranged touch targets and isolated clipping; fixed rendering unchanged. 14 suites passed (editor 835; Companion offline 250); built-in 4,221 images/hit maps and plugin baselines unchanged; hardware pending.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS editor with Posts/required Status, separate landscape/portrait layouts in `layouts/builtin.rss.layout.json`; opening or applying Standard keeps the pixel-identical fixed path, real changes use the grid (spacing may shift, E2). Reset restores Standard; rows, paging and touch share one plan. Header/footer fixed; built-in cycles and HA deferred. 14 suites passed (catalog 217, editor 874, core 3,470; Companion offline 250); existing baselines unchanged, plus 1,024 RSS images/hit maps and 5,952 checks without errors; hardware pending.

P7-8 (2026-11-06, Agent Codex CLI 0.166.0): Streaming in the page selector; Titles/required Status from the existing snapshot, per-page landscape/portrait overlay, paging and touch from one grid plan. Standard/reset keep the fixed path; TMDB / JustWatch stays in the footer; compact rows omit the date. 14 suites passed (Companion offline); existing baselines bit-identical, 1,792 new grid images/hit maps added, 6,267 checks/0; hardware pending.

P7-9 (2026-11-06, Agent Codex CLI 0.166.0): BF6 in the page selector; Ranking/required Status, sorted snapshot, per-page landscape/portrait overlay. Grid capacity controls paging and player links; compact rows show name, K/D, rank or error. Standard/reset remain pixel-identical. 14 suites passed (Companion offline); existing baselines bit-identical, 1,024 new grid images/hit maps added, 4,390 checks/0; hardware pending.

P7-10 (2026-11-06, Agent Codex CLI 0.166.0): Friends and Discord in the page selector; shared Friends/required Status catalog and sorted snapshots, separate `builtin.friends.layout.json` / `builtin.discord.layout.json` overlays, backups and revisions. Grid rows, paging and profile targets share one plan; narrow compact rows omit avatars/badges. Standard/reset stay pixel-identical (E2 accepted). 14 suites passed (Companion offline); existing baselines bit-identical, 1,024 grid images/hit maps added per page, 8,622 checks/0 combined; hardware pending.

P7-11 (2026-11-06, Agent Codex CLI 0.166.0): System editor blocks use the existing CPU/GPU/RAM/disk/network snapshot; temperatures retain threshold colours, regular network includes history, compact shows values/bars or rates. Per-page landscape/portrait overlay, Cupertino groups per zone and tool targets from the drawing plan. Standard/reset remain pixel-identical; E2 confirmed. 14 suites passed (Companion offline); existing baselines unchanged, 1,536 additive system images/hit maps. Physical acceptance pending.

P7-12 (2026-11-06, Agent Codex CLI 0.166.0): Spotify editor blocks use the existing snapshot/meter for cover, title/artist/album, progress, controls, levels and required status; last played keeps progress/controls hidden. Per-page landscape/portrait overlay, compact single-line text, three level colour modes and separate 48-point button targets from the drawing arrangement. Standard/reset remain pixel-identical; 14 suites passed (Companion offline); existing baselines unchanged, 4,096 additive Spotify images/hit maps. Physical acceptance pending.

P7b-0 (2026-11-06, Claude Sonnet 5.5 on behalf): New default dark Material with orange accent (`material-orange`; text/surface and accent/surface at least 4.5:1, bar at least 3:1, contrast tests in fl-tests and fl-layout-ui-tests). New installs start with it; existing installs get a one-time DE/EN dialog "Set the new default?" (default No, answer stored in `ui.standard_theme_choice`, closing changes nothing). Cupertino, Fluent, Light, Dark, Windows and custom themes stay selectable. Existing built-in/plugin hash files unchanged, new baseline is additive in `p7b0-material-orange-hashes.txt` (768 hashes).

P7b-6 (2026-11-06, Agent Codex CLI 0.166.0): Per-device built-in/plugin overlays inheriting the standard (up to 16 per page, 16 KiB per overlay, 256 KiB total), atomic persistence/backup and shared device selection for images/touch; display tabs with size/orientation, offline profiles and discard confirmation. Mark Android TV on the PC (protocol v1 has no device type) to enforce landscape. 14 suites passed (Companion offline), eleven existing hash files unchanged; physical multi-device acceptance pending.

P7b-1 (2026-11-06, Agent Claude Sonnet 5.5): Editor window layout: header bar (page selector, device-size chip, undo/redo/apply/reset), left rail with Tiles/Layers/Pages, canvas and properties as a three-column shell (wide from 900 DIP; medium: properties below the canvas; narrow: stacked, palette collapsed, section choice survives page switches); display tab bar unchanged. New `ui_editor.*`. 14 suites green, baselines bit-identical; hand acceptance open.

P7b-2 (2026-11-06, Agent Claude Sonnet 5.5): Tile palette with search, filter chips (All/System/Time/Media/Games/Other) and a grid of live mini previews (same page renderer with sample data: a crop of the standard page, or a single zone for plugins; cached per page/tile/size/display theme, loaded only on page switch, tiles without data marked "no data"). Drag onto the canvas, click inserts at the first free spot (one undo step), keyboard (arrows, Space, Enter, Delete, F6), MSAA "Tile palette". New `ui_tile_palette.*`. 14 suites green, baselines bit-identical.

P7b-3 (2026-11-06, Agent Claude Sonnet 5.5): Canvas in the aspect ratio of the display tab: device-size frame, subtle cell grid, dashed drop slots "drop here", snapping ghost (invalid: dashed red plus text "does not fit"), handles, trash, zoom -/+/fit. Opt-in `GridEditor::canvas`; the previous grid stays bit-identical. Editor snapshots additive (`p7b-editor-hashes.txt`), text contrast >= 4.5:1 and focus ring/border >= 3:1 verified in all themes. 14 suites green.

P7b-4 (2026-11-06, Agent Claude Sonnet 5.5): Layer list (order, visibility; selection works in list and canvas), page list (switching asks about unsaved changes) and starter card "In three steps" (dismiss once, remembered in `layouts/editor-intro.done`). 14 suites green (fl-layout-ui-tests 1,615), baselines bit-identical; hand acceptance open.

## Custom views (P8-2, development)

Use **App → Custom views** to create, rename, delete and arrange source tiles per active display. **Apply** saves; the view appears as `custom:<id>` in the page cycle. Only active devices get tabs; offline layouts remain stored. Trash, grid sizes, conditions, density and undo/redo: [Usage and limits](PHASE8-EIGENE-ANSICHTEN.md).

The properties panel has a **Touch target** per tile: **Default** (the source behaves as before) or **Custom** with **Switch to page …** (any page in the current page cycle: standard, `custom:` or plugin page), **Open link on PC** and **Next item**. Link and Next item apply to plugin tiles only (links need the "Open links" consent and a declared host). Invalid, incomplete or disallowed targets and pages outside the cycle fall back to Default. The target is stored per layout, i.e. separately for Standard and each display overlay.
