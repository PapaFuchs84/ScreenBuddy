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

**Status phase 2b:** parser, limits, store rule, zone geometry, block interface, density levels **and drawing by zones** are implemented and tested (`fl-plugin-zones-tests`, `fl-plugin-snapshot-tests`). The app draws a v2 page with a `layout` by zones; v1 and v2 without `layout` draw pixel-identically to before in the automatic grid (at most 6 widgets). Manifest v1 is unchanged. A complete example with eight blocks: [offline-zonen.flplugin.json](../cpp/examples/plugins/offline-zonen.flplugin.json).

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

- **The parser recognizes v2 only by `schema_version: 2` as an integer.** All v1 rules still apply (templates, texts, sources, settings). In addition every widget has a **stable block ID** `id` (lowercase letter first, then lowercase letters/digits/`_`, at most 40 characters, unique). The ID stays the same across versions and later keys tap targets and settings.
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

### Store

For the store a v2 manifest needs `min_app_version` **2.10.0 or newer**. An older app then shows the plugin as "needs a newer ScreenBuddy" before downloading it; it could not install it anyway (it rejects `schema_version: 2`).

## Create your own plugin

The [step-by-step tutorial](PLUGIN-TUTORIAL.en.md) explains import, settings, host consent, templates and validation with the real parser. Start with the fully local [focus board](../cpp/examples/plugins/offline-focus.flplugin.json); the [weather variant](../cpp/examples/plugins/public-weather.flplugin.json) optionally demonstrates a public JSON source. Sample data is never used as a live fallback.
