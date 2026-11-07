# Plugin system in the ScreenBuddy 2.5 development build

The current contract is **provisional**. It consists of JSON data; native libraries, JavaScript and shell commands are not loaded. A plugin without sources can be imported locally and displayed as a page. A source plugin can be enabled once its required settings are supplied and every declared host is approved individually. An isolated public weather fetch succeeded; full acceptance of external HTTPS services and the app's consent flow remains open.

## Minimal example

Save these bytes as `offline-clock.flplugin.json` or use the [ready example file](../cpp/examples/plugins/offline-clock.flplugin.json). Select **Settings → Plugins → Import manifest** in ScreenBuddy:

```json
{
  "schema_version": 1,
  "id": "offline-clock",
  "name": "Uhr / Clock",
  "version": "1.0.0",
  "widgets": [
    {"type": "clock", "format": "time"}
  ]
}
```

The plugin appears as its own page. Enable, disable and remove it in the same tab. Manifests live in a `plugins` directory beside the per-user `config.toml`.

## Contract and limits

- Required (schema 1; for schema 2 see [manifest v2](#manifest-v2-zone-layout-2100-in-preparation)): `schema_version: 1`, a unique lowercase-letter/digit/hyphen ID, name, version `X.Y.Z`, and 1–6 widgets.
- Widget types: `clock` (`time`, `date`, `datetime`), `text`, `metric`, `bar`, `list`. Text templates support validated references such as `{{time}}`, `{{date}}` and declared source values.
- Optional: store fields `license`, `author`, `category`, `min_app_version` (see [package format](PLUGIN-PACKAGE-FORMAT.md)); up to 12 settings and four JSON sources with fixed HTTPS hosts. Bounded HTTPS fetching runs in its own worker. Host grants are stored locally; plugin settings are DPAPI-protected. One isolated public live fetch passed, but the full app flow with host consent has not been accepted. Sample source data is used only for explicit preview, never as live fallback.
- Limits: 128 KiB manifest, 32 installed plugins, JSON depth 32. Unknown fields and duplicate JSON keys are rejected.
- Security status: no plugin code is loaded. Fetches are restricted to declared HTTPS hosts and bounded responses; host grants are stored locally and plugin settings are protected with DPAPI. Certificate revocation checks (CRL/OCSP) are still missing. There is no public marketplace yet.

Checks passed: manifest parser (223), store (127), transport (134, including a local WinSock DNS regression check) and plugin settings (135). A direct, isolated HTTPS fetch from the public Open-Meteo example returned the expected weather fields. Production configuration and host grants were not changed. This does not accept the full app flow, certificate revocation, or hardware output; see the [test record](../cpp/TESTPROTOKOLL.md).

## Manifest v2: zone layout (2.10.0, in preparation)

**Status phase 2b:** parser, limits, store rule, zone geometry, block interface, density levels **and drawing by zones** are implemented and tested (`fl-plugin-zones-tests`, `fl-plugin-snapshot-tests`). The app draws a v2 page with a `layout` by zones; v1 and v2 without `layout` draw pixel-identically to before in the automatic grid (at most 6 widgets). Manifest v1 is unchanged. A complete example with eight blocks: [offline-zonen.flplugin.json](../cpp/examples/plugins/offline-zonen.flplugin.json). **Phase 3:** [conditional display](#conditional-display-show_if-phase-3) (`show_if`) and per-block [fault isolation](#fault-isolation-phase-3) are implemented and tested.

```json
{
  "schema_version": 2,
  "id": "zones-demo",
  "name": "Zones",
  "version": "1.0.0",
  "layout": {
    "columns": [2, 1],
    "rows": [1, 1],
    "zones": [
      {"id": "main", "column": 0, "row": 0, "row_span": 2},
      {"id": "top", "column": 1, "row": 0},
      {"id": "bottom", "column": 1, "row": 1}
    ]
  },
  "widgets": [
    {"type": "clock", "id": "time", "zone": "main", "format": "time"},
    {"type": "clock", "id": "date", "zone": "top", "format": "date"},
    {"type": "text", "id": "hello", "zone": "bottom", "text": "Hello"}
  ]
}
```

Terms: **zone** = rectangular area of a flat grid; **block** = one widget in a zone; **density level** = compact, regular or detailed.

- **The parser recognizes v2 only by `schema_version: 2` as an integer.** All v1 rules still apply (templates, texts, sources, settings). In addition every widget has a **stable block ID** `id` (lowercase letter first, then lowercase letters/digits/`_`, at most 40 characters, unique). The ID stays the same across versions and keys tap targets and settings.
- **`layout` is optional.** Without `layout` a v2 page uses the automatic grid and `zone` is not allowed. With `layout` every widget needs an existing `zone`, and every zone needs at least one block.
- **Grid in fractions:** `columns` and `rows` are integer shares 1–12 (e.g. `[2, 1]` = two thirds / one third). Decimals, `2.0`, `1e0`, strings, booleans and negative numbers are rejected.
- **Zones:** `id`, `column`, `row` (0-based), optional `column_span`/`row_span` (default 1). A zone must lie entirely inside the grid and must not overlap another one. Empty cells are allowed. No nesting: zones do not contain zones.
- **Limits:** 1–6 columns, 1–6 rows, 1–8 zones, at most 4 blocks per zone, 1–12 widgets per page with a `layout`; without `layout` (v1 and v2) it stays at 1–6. Unknown fields and duplicate keys are rejected, as in v1.
- **Arrangement:** blocks in a zone are stacked in manifest order and share the zone height equally. Column/row edges are rounded from the cumulative shares to whole logical pixels; neighbours share exactly one edge (plus gap), so there are neither gaps nor overlaps.
- **Too little space:** if a block does not fit even in compact form, it is **hidden instead of overlapping**. The last non-fitting block of the zone is hidden first; the freed space goes to the others. Invalid areas (non-finite, ≤ 0, larger than 16384) produce a page without visible blocks.
- **Clipping:** a block draws only inside its rectangle; the renderer sets a clip to that rectangle before every block (`Canvas::clip_push`/`clip_pop`). Texts are additionally shortened with an ellipsis so nothing looks cut off. Frame style (card radius, spacing) and colors come from the theme, as in v1.

### Density levels

Each block type (`text`, `metric`, `bar`, `list`, `clock`) has three minimum areas in logical pixels: compact (value only), regular (label + value), detailed (extra details). The highest level whose minimum (width **and** height) fits is chosen. Logical pixels: the page is referenced to 480×320 (landscape) or 320×480 (portrait) and scaled; 240×160 and 160×240 give the same logical areas as 480×320 and 320×480. The font is not scaled along (fixed pixel sizes), so at scale 0.5 a text line is twice as tall in logical pixels. The table values therefore hold for scale ≥ 1 and are multiplied by 1/s at a smaller scale s (thresholds as well as label row, margins, line pitch and bar sizes); blocks that then no longer fit are hidden instead of overlapped. The thresholds are derived from the drawing geometry (label row 22, top/bottom margin 6, line pitch 18) and were checked with the example snapshots in all four sizes and all theme styles. The thresholds are **central in the app** (`zones::DensityTable`), adjustable per block type, and are not part of the manifest. Only finite, positive, ascending thresholds are accepted; invalid values are rejected and the previous value stays.

| Type | compact | regular | detailed |
| --- | --- | --- | --- |
| `text` | 56×22 | 80×42 | 120×64 |
| `metric` | 48×28 | 72×50 | 110×66 |
| `bar` | 56×22 | 80×58 | 120×72 |
| `list` | 60×22 | 96×46 | 130×82 |
| `clock` | 52×28 | 72×50 | 110×66 |

What each level draws:

| Type | compact | regular | detailed |
| --- | --- | --- | --- |
| `text` | value, one line | label + value, one line | label + value, wrapped (as many lines as fit) |
| `metric`, `clock` | value; label at the left of the same row if there is room | label + value (with unit) | larger type (26 down to 20, whatever fits the width), small unit after the value |
| `bar` | only the bar (without a number: the value as text) | label, value, bar | plus large number and percentage of the range in the header row |
| `list` | first entry | label + at most 2 entries | up to 5 entries, "+N" in the header row for more (without a label in the first entry's row, which is shortened to make room) |

### Block interface (for developers)

`cpp/src/plugin_zones.h`: a `zones::Block` provides its stable ID, `min_size(density)` and `draw(Canvas, area, density)`. `draw` is never called with an area below the minimum size. `TableBlock` takes its minimum sizes from a `DensityTable` and keeps only its address: the table must outlive the block, and **temporary tables cannot be passed** (deleted constructor). `plugin_render.cpp` has one subclass per widget type (text, value for `metric`/`clock`, bar, list) and one central static table. `zones::plan(area, layout, blocks, gap)` is a pure function without graphics and returns the rectangles of all zones and blocks with their density level or "hidden".

### Conditional display (`show_if`, phase 3)

A v2 widget may carry `show_if` (with and without `layout`). The block appears only if the condition holds. There are exactly two forms: **no expression language**, no and/or combination, at most one condition per block. Example: [offline-bedingt.flplugin.json](../cpp/examples/plugins/offline-bedingt.flplugin.json).

```json
"show_if": {"path": "weather.warning"}
"show_if": {"path": "weather.temperature", "op": ">=", "value": 25}
```

- `path`: `<source>` (the whole response is present), `<source>.<field>[.<field>…]` (field names and list indices as in templates) or `settings.<key>` of a **non-secret** setting. Not allowed: `time`, `date`, `item`, unknown sources and settings, and settings of type `secret` (fixed message "Secret settings cannot appear in conditions."). At most 128 characters and 8 segments.
- `op` (optional): `==`, `!=`, `<`, `<=`, `>`, `>=`. Without `op` only presence is tested and `value` is forbidden. With `op`, `value` is required and `path` must be a field (not the whole source).
- `value`: text (1–256 characters, no control, bidi or zero-width characters), a finite number or `true`/`false`. `null`, arrays and objects are rejected; `<`, `<=`, `>`, `>=` only with numbers.
- An invalid `show_if` fails the **whole manifest**, like any other error. In schema 1 `show_if` is an unknown field ("Unknown or misplaced field.").

**Semantics (fail closed):** the value must first be *present*; only then is it compared. There is no type conversion. "yes" = block shown.

| Value at `path` | without `op` (present) | `==` / `!=` | `<` `<=` `>` `>=` |
| --- | --- | --- | --- |
| missing (path, field, list index or the whole source) | no | no | no |
| `null` | no | no | no |
| non-finite number | no | no | no |
| empty: `""`, `[]`, `{}` | no | no | no |
| `false` | **yes** | only against booleans: `== false` yes, `!= false` no, `== true` no, `!= true` yes | no |
| `true` | yes | only against booleans | no |
| `0` | **yes** | only against numbers: `== 0` yes, `!= 0` no | normal (`>= 0` yes, `> 0` no) |
| other number | yes | only against numbers | normal |
| text (also `" "`, `"0"`, `"false"`) | yes | only against text, exact (case-sensitive) | no |
| non-empty array or object | yes | no | no |

- "no" also applies to `!=`: a missing or differently typed value is never "unequal". `"42"` is not the number `42`, `true` is not `1`. To hide a block on `false` or `0`, write `{"op": "==", "value": true}` or `{"op": "!=", "value": 0}`.
- Numbers compare as floating point (`-0.0 == 0`); integers above 2^53 lose precision.
- Settings: stored value, otherwise the default. If the stored settings are invalid, all settings count as missing (and the footer reports missing data).
- Hidden blocks are left out of the page. In the automatic grid the others move up; in a zone layout the remaining blocks of the same zone share its area. A zone without a visible block stays empty; other zones do not move.
- Templates of a hidden block are not evaluated, so they do not trigger "Plugin-Daten fehlen". Exception: if a source has delivered nothing at all in live mode, the footer reports missing data even when every block of that source is conditional. If only a field is missing from a delivered response, the footer stays normal.
- **Preview without data:** the preview (import, store) evaluates conditions against the sources' `sample`, live mode against the response. Without a `sample` (or with an empty one), blocks with a source condition are hidden in the preview. The store requires `sample`; choose it so the preview shows the conditional blocks. Live mode without a response never uses sample data (source conditions are then false).

### Fault isolation (phase 3)

- **Data evaluation** (`render_page`): every block is evaluated separately (condition, templates, list, bar). If evaluating a block throws, only that block is left out (`Page::failed`); the others are unchanged and the footer reports missing data. `render_page` does not throw: a failure outside the blocks (e.g. out of memory while copying ID, title or layout) gives a page without blocks that reports missing data; ID and title are kept if they can still be copied.
- **Drawing** (`draw_page`): every block (zone layout) and every card (automatic grid, schema 1 and 2 without `layout`) is guarded separately. If it throws, even half-way, its area is filled with the page background again (the block is hidden). The clip stack is restored exactly to its depth before the block, including clips the block itself left open. The other blocks, header and footer are drawn normally. In the grid, too, a clip lies on the card (plus 1 logical pixel for anti-aliasing), so nothing is left outside after a failure, not even text that would reach beyond the card at a small scale; in the normal case the grid draws pixel-identically to before (all snapshot hashes unchanged). Measuring a block is guarded too: a block that throws there never fits.
- **Log:** a warning in the existing app log (area `plugins`), e.g. "Plugin offline-zonen: Block dauer ausgeblendet (Fehler beim Zeichnen)" or "… (Fehler beim Auswerten der Daten)". It contains only the plugin ID, the block ID (schema 1: `#<position>`) and the stage, never data, setting values, secrets or exception texts. Rate-limited: at most one entry per plugin, block and stage per 60 s, at most 64 tracked keys (beyond that entries are dropped instead of growing).
- Only C++ exceptions are caught (`/EHsc`). Access violations (SEH) are not isolated.
- Tests inject faults per block through test hooks (`plugins_testing.h`, `plugin_render_testing.h`). The app always calls `render_page` and `draw_page` without a hook.

### Store

For the store a v2 manifest needs `min_app_version` **2.10.0 or newer**. An older app then shows the plugin as "needs a newer ScreenBuddy" before downloading it; it could not install it anyway (it rejects `schema_version: 2`).

## Create your own plugin

The [step-by-step tutorial](PLUGIN-TUTORIAL.en.md) explains import, settings, host consent, templates and validation with the real parser. Start with the fully local [focus board](../cpp/examples/plugins/offline-focus.flplugin.json); the [weather variant](../cpp/examples/plugins/public-weather.flplugin.json) optionally demonstrates a public JSON source. Sample data is never used as a live fallback.

## Cycling zone (Phase 4b, development)

Only `schema_version: 2` with `layout` permits a zone to carry `"cycle": {"mode": "rotate", "transition": "fade"}`. `mode` is `rotate` or `first_available`; `transition` is `fade` (default) or `none`. Blocks in `rotate` zones may specify integer `seconds` from 3 to 600 (default 8). Without `cycle`, existing stacking is unchanged; schema 1 rejects these keys. Schema 2 Store packages require `min_app_version` of at least `2.10.0`.

`rotate` shows available blocks in manifest order; `first_available` shows the first available block and reacts to data changes. Available means visible under `show_if` and without missing references in that block. If none are available, the zone stays empty. Leaving the visible tiles or updating the manifest resets its schedule. Views and independent devices keep separate schedules.

The output determines the quiet transition: monitor 4 fade steps over 400 ms (2 steps or a cut at high rendering cost), Companion 2 steps over 400 ms with connected clients, serial a band reveal only with a sufficiently fast measured rate, otherwise a cut. Data is frozen during transitions. Resizing ends the transition; theme changes rebuild its image cache. Slow outputs stretch the entry duration to at least four times the last transfer duration. Simultaneous changes within a tile wait at most 600 ms. No sounds or intrusive effects.

Example: [offline-wechselfeld.flplugin.json](../cpp/examples/plugins/offline-wechselfeld.flplugin.json). Import locally to try two texts with durations of 3 and 5 seconds. Physical acceptance and actual serial rate measurement by Basti remain pending; only simulated transfers have been checked. Tap actions are implemented in Phase 4c (development, see below).

Serial reveal timing includes transfer, rendering and a 50 ms rest. Global eight-row bands are revealed in order; an insufficient cycle budget uses a cut. Only Companion discards late steps; monitors show the intermediate state reached.

## Tap targets for plugin blocks (Phase 4c, development for 2.10.0)

Only schema 2 accepts `links` and `tap`; schema 1 and pages without `tap` keep their existing behaviour. Schema 2 store packages still require `min_app_version` at least `2.10.0`.

```json
"links": ["news.example.org"],
"tap": {"action": "open_link", "url": "https://news.example.org/article/{{feed.id}}"}
```

`links` is a top-level manifest field; `tap` belongs to a block. This is a field excerpt, not a complete manifest. Up to four distinct public DNS hosts are allowed: no IP literals, local names, ports or placeholders in the host list. URLs start with literal `https://` and a declared literal host, allow only port 443, forbid control characters, backslashes and fragments, and are limited to 2048 bytes after expansion. Placeholders are allowed only in path and query: source fields and non-secret settings, plus `item.*` per displayed list row. Each UTF-8 value is percent-encoded using RFC 3986 unreserved characters. Missing, empty or structured values produce no target; secret settings are rejected by the parser.

The closed action list is `open_link` and `next`. `next` has no URL and requires a zone with `cycle`: `"tap": {"action": "next"}`. It requests the next entry with data, retaining the request while another transition blocks the zone. In `first_available`, manual selection remains until the set of available entries changes. Each view has a protected queue of at most eight distinct plugin/zone keys; only the display thread changes the schedule.

**Opening links defaults to off.** Settings → Plugins shows the selected plugin's link hosts and a separate **Open links from this plugin on the PC** switch. A phone tap opens the link on the PC. This consent is independent of source consent. Only a link-list-only update retains source grants and activation; link consent is reset. All other updates and identical reimports revoke every grant. At most one launch attempt per plugin every two seconds is allowed across all views. Each tap rechecks enabled state, generation, consent, HTTPS and host. Logs use fixed messages only, without URLs, data or setting values.

Targets and pixels are created in the same drawing call; list drawing and hit testing share row geometry. A published target must remain stable for **1.0 s on Companion** or **0.3 s on a monitor**. Transitions, unstable targets and denied consent consume the tap without navigation. Areas without a target retain the existing thirds navigation. Android TV has no Phase 4c touch/D-pad action. Companion protocol v1 still has no frame identifier; stability limits latency risk without acknowledging a particular JPEG version.

Automated tests use synthetic data and an injected launcher; no real browser is opened. Counts, sandbox limits and pending hardware acceptance: [test record](../cpp/TESTPROTOKOLL.md).


Tap URL validation (parser and again immediately before browser launch): the literal host must be lowercase. Fixed path/query text permits only RFC 3986 characters (unreserved, sub-delimiters, `:`, `@`, `/`, `?`); `%` requires exactly two hexadecimal digits. Raw non-ASCII, spaces, quotes, angle brackets, backslashes, square brackets, carets, backticks, braces and vertical bars are rejected. Braces are allowed only as validated `{{reference}}` placeholders. Values `.` and `..` produce no target; fixed dot path segments (including percent-encoded dots) are rejected. Unicode values remain supported through percent-encoding.

Only an actual link-list change with **every other manifest field identical** retains source consent and activation; link consent is reset. Other updates, including widgets, settings or tap templates changed alongside links, and identical reimports reset all grants (source plugins become disabled). Corrupt link-consent files fail closed; failed consent writes never grant permission. While busy, the link toggle restores its previous state. List rows and targets stay within their block even on shallow surfaces; targets also work in the automatic grid without a layout.

## Page editor (P6-3 to P7-10, development for 2.10.0)

Open **Settings → App → Custom views** (page list, "Standard / plugin" entries) for enabled local plugins, RSS, Streaming, BF6, Friends and Discord (Plugins/Built-in groups), or use the "Custom views …" button under **Pages & order**; "Arrange page" and "Arrange …" no longer exist. Manifest v1 only displays “Arrangement requires manifest v2”. RSS, Streaming, BF6, Friends and Discord are offered as built-in pages; other pages and Home Assistant follow later.

For plugins, only `<id>.layout.json` is saved; the manifest file remains unchanged. Grid weights (1–12), up to 6×6 cells, eight zones and twelve blocks with at most four shown blocks per zone follow the parser. Editable properties are zone ID, position, span, splitting, cycle/transition, block order, visibility, zone assignment, duration (3–600 seconds for `rotate`) and `show_if`. Sources, content, links, tap actions, setting values and secrets cannot be edited. Hidden blocks remain in the overlay.

Conditions offer source paths from sample data/existing conditions and non-secret setting keys. There is no raw JSON field or secret-setting selection. Comparison values have an explicit text, number or `true`/`false` type. Commit operator/type changes with **Confirm condition**, and text with Enter or focus change. Uncommitted/invalid input disables **Apply**. “Select invalid field” focuses the affected grid/block field. This first editor does not offer free path entry for source fields absent from both samples and existing conditions.

The grid is schematic. The existing preview area uses only `App::plugin_preview`, the parser, plugin samples and the same renderer as display output. Choose landscape 480×320 or portrait 320×480. Clock/date are reproducible examples (12:34, 2026-01-01). Changes are coalesced; an outdated preview job cannot replace a newer result. Invalid drafts retain the last valid image. Leaving the page restores the normal preview.

**Mouse (drag & drop):** Drag blocks from the block palette onto the grid, move blocks between zones, move zones by their title and resize them with the handles. Dragging onto the trash removes them. One drag gesture creates exactly one undo step; Esc or an aborted mouse capture leaves arrangement and selection unchanged; releasing outside the grid cancels silently (no message). A target that would leave a zone without a shown block appears as "does not fit" and is rejected with a message. Dragging from a free cell creates nothing (a plain click still creates a 1×1 zone). At the grid/page edge a timer keeps scrolling while the mouse rests. The preview starts only after dropping, not while dragging. The mouse pointer shows handles and move.

**Keyboard:** Tab/Shift+Tab move between fields. F6 or Ctrl+Tab switches the area: block palette, grid, trash. Arrows move the target, Space picks up, Enter drops (in the grid, Enter selects a zone or creates a free 1×1 zone), Esc cancels. Block chips in the grid: Ctrl+Up/Down enters the chips of the zone under the cell focus, Up/Down steps on, Left/Right leaves the chips; Space picks up the chip, Delete returns the block to the palette (not if the zone would be left without a shown block); the MSAA value names the selected chip. While carrying: Ctrl+Up/Down changes the stack position, Shift+arrow resizes, Alt+arrow moves, Delete removes. Delete on a zone moves its blocks to the first remaining zone and hides them. If they do not fit (especially more than four blocks), a message rejects the action; reassign blocks manually first. In the block list, Space toggles visibility and Alt+Up/Down reorders. Ctrl+Z, Ctrl+Y or Ctrl+Shift+Z act on page history in the grid/list; text fields retain native text undo. Undo/Redo buttons are reachable with Tab. History holds 100 steps, coalesces arrow repeats on the same element within 600 ms, and resets after plugin changes/application. Rebuilding widgets after global Discard preserves committed editor history.

Cells are at least 32 DIP. The page scrolls vertically; narrow grids follow cell focus horizontally and clip their contents. MSAA exposes a list with zone ID, column/row range, cycle mode and block count. Selection has text and a thicker outline. Light themes are checked for text ≥4.5:1 and grid/focus contrast ≥3:1. The approved global light FuchsLabs focus ring, including text-field frames, now uses `accent_text` (Glut).

**Apply** validates again through the store callback, backs up the previous overlay as `.layout.json.bak` and atomically replaces the file. No restart/configuration save; consent and live data are preserved. **Reset to manifest** asks for confirmation and deletes overlay and backup. An outdated manifest fingerprint causes an overlay to be ignored and a reset hint to appear. Write failures retain the previous effective state. All editor widgets are excluded from Save & apply.

Native automated tests cover keyboard, MSAA, history and store application, plus contrast, dedicated grid snapshots and preview comparisons. Physical USB/monitor/Companion acceptance, mouse/touchpad/touch acceptance and the visible confirmation dialog remain open. The native mouse and settings smoke tests (`native_mouse_smoke.py`, `native_settings_smoke.py`) need a running app and did not run in the 06.11.2026 state. See the [test record](../cpp/TESTPROTOKOLL.md) for evidence and deviations.

Preview jobs keep the grid, fields and Undo/Redo usable; only Apply, Reset and plugin selection are disabled during work. Invalid fields are marked without moving focus or repeating the same input error. Switching plugins with unsaved edits asks for confirmation in DE/EN with **No** as the default. Apply/Reset synchronize existing widgets and retain usable keyboard focus. Draft edits made during a save are not overwritten by its completion.

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
