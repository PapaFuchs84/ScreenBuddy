# Plugin package format and validator

© 2026 FuchsLabs · Phase 1 of the plugin platform (groundwork for the future plugin store). **Status:** manifest fields and validator are implemented; the store, signing and the in-app store UI come in later phases and do not exist yet.

## A plugin is one file

A plugin stays **one** UTF-8 JSON file `<id>.flplugin.json` (schema 1 or 2, at most 128 KiB) with no executable code. There are no archives, scripts or extra files. For the planned plugin collection: one folder per plugin, `plugins/<id>/<id>.flplugin.json`.

## New manifest fields (all optional, valid in schema 1 and 2)

| Field | Rule |
| --- | --- |
| `license` | Only `MIT` or `CC0-1.0` (exact spelling). |
| `author` | 1–80 characters, no control or invisible format characters. |
| `category` | `system`, `games`, `smart-home`, `info` or `other`. |
| `min_app_version` | `X.Y.Z`. Older ScreenBuddy versions will later show the plugin as unsupported (UI follows with the store). |

Existing manifests without these fields stay valid and importable locally. An invalid value in one of these fields makes the import fail.

## Schema 2: zone layout (from 2.10.0, in preparation)

With `schema_version: 2` a plugin is still a single JSON file. New are the block ID `id` on every widget (required), the optional `layout` (grid of integer shares, zones with column/row/span) and `zone` on every widget as soon as a `layout` exists; with a `layout` also up to 12 instead of 6 widgets. Limits and behaviour: [plugin system, manifest v2](PLUGIN-SYSTEM.en.md#manifest-v2-zone-layout-2100-in-preparation). Schema 1 manifests stay valid unchanged; `layout`, `id`, `zone` and `show_if` are unknown fields there and are rejected. ScreenBuddy up to 2.9.x rejects schema 2 on import.

Rules for v2 (enforced by the parser; `fl-plugin-validate` reports violations):

- `layout` contains exactly `columns`, `rows` (1–6 shares each, every share an integer 1–12) and `zones` (1–8 zones).
- Zone: `id` (starts with a lowercase letter, then lowercase letters, digits, `_`; at most 40 characters; unique), `column`, `row` (0-based), optional `column_span`, `row_span` (at least 1, inside the grid). Zones must not overlap; no nesting.
- Block ID `id` on every widget: same character rules, unique across the plugin. `zone` must reference an existing zone; at most 4 widgets per zone, every zone needs at least one; with a `layout` 1–12 widgets.
- Without a `layout` (schema 2 without a grid) the schema 1 rules apply: at most 6 widgets, no `zone`; the page is drawn pixel-identical to schema 1.
- No pixel or percentage sizes in the manifest: the app picks each block's density level (compact, regular, detailed) from the zone area and hides blocks that do not fit. On small displays (scale below 1, e.g. 240×160) the minimum areas are correspondingly larger because the font is not scaled along, so more blocks drop out there. Put the most important block first in a zone (the last block that does not fit is dropped first).
- Optional `show_if` on every widget (with and without `layout`): `{"path": "<source>[.<field>…]"}` or `{"path": "settings.<key>"}` (value present), or with `"op"` (`==`, `!=`, `<`, `<=`, `>`, `>=`) and `"value"` (text of 1–256 characters, a finite number or a boolean; `<`/`>` numbers only). No expression language, one condition per block, `path` at most 128 characters and 8 segments. Settings of type `secret`, `time`, `date` and `item` are forbidden; any error rejects the whole manifest. Semantics (missing, `null`, empty, `false`, `0`): [conditional display](PLUGIN-SYSTEM.en.md#conditional-display-show_if-phase-3).

## Additional store rules

To be accepted into the store, these also apply (`fl-plugin-validate --store`):

- `description` (at least 10 characters), `author`, `license`, `category`, `min_app_version` present.
- Plugins with sources: `refresh_seconds` of at least 60 and a `sample` for every source (basis of the preview).
- File name `<id>.flplugin.json`, unique IDs.
- Schema 2: `min_app_version` at least `2.10.0`.

These rules are an **automatic pre-check**. Approval remains a human review (purpose, hosts, license).

## Validator

```text
fl-plugin-validate [--store] [--app-version X.Y.Z] <file-or-folder>...
```

It uses the app's `plugins.cpp` (same parser), only reads files and makes no network requests. For each plugin it prints `OK` with ID, version and the hosts data would be sent to, or `FAIL` with fixed error messages. Exit code: 0 valid, 1 problems, 2 usage error. Tests run in GitHub Actions (`Plugin validator`) and locally with `sh cpp/tests/build-validator-linux.sh`.

## Open (later phases)

Signing is implemented as a library and tool, see [Plugin signing](PLUGIN-SIGNING.md); production key, index build via Actions and download are open. In-app store UI (phase 3), moderation and terms of use (phase 4). See the plan in PR [#21](https://github.com/PapaFuchs84/ScreenBuddy/pull/21).

## Cycling zone (Phase 4b, development)

Only `schema_version: 2` with `layout` permits a zone to carry `"cycle": {"mode": "rotate", "transition": "fade"}`. `mode` is `rotate` or `first_available`; `transition` is `fade` (default) or `none`. Blocks in `rotate` zones may specify integer `seconds` from 3 to 600 (default 8). Without `cycle`, existing stacking is unchanged; schema 1 rejects these keys. Schema 2 Store packages require `min_app_version` of at least `2.10.0`.

`rotate` shows available blocks in manifest order; `first_available` shows the first available block and reacts to data changes. Available means visible under `show_if` and without missing references in that block. If none are available, the zone stays empty. Leaving the visible tiles or updating the manifest resets its schedule. Views and independent devices keep separate schedules.

The output determines the quiet transition: monitor 4 fade steps over 400 ms (2 steps or a cut at high rendering cost), Companion 2 steps over 400 ms with connected clients, serial a band reveal only with a sufficiently fast measured rate, otherwise a cut. Data is frozen during transitions. Resizing ends the transition; theme changes rebuild its image cache. Slow outputs stretch the entry duration to at least four times the last transfer duration. Simultaneous changes within a tile wait at most 600 ms. No sounds or intrusive effects.

Example: [offline-wechselfeld.flplugin.json](../cpp/examples/plugins/offline-wechselfeld.flplugin.json). Import locally to try two texts with durations of 3 and 5 seconds. Physical acceptance and actual serial rate measurement by Basti remain pending; only simulated transfers have been checked. Tap actions belong to the later Phase 4c.

Serial reveal timing includes transfer, rendering and a 50 ms rest. Global eight-row bands are revealed in order; an insufficient cycle budget uses a cut. Only Companion discards late steps; monitors show the intermediate state reached.
