# Your first ScreenBuddy plugin

[Deutsch](PLUGIN-TUTORIAL.md) · [System overview](PLUGIN-SYSTEM.en.md) · [Examples](../cpp/examples/plugins/README.md) · [Plugin Workshop (interactive)](../cpp/res/PLUGIN-WERKSTATT.html)

For the ScreenBuddy 2.5 development build. Schema 1 is provisional. A plugin is a UTF-8 JSON file describing one display page with up to six cards. No compiler or executable plugin code is needed.

## 1. Start offline

1. Save [offline-focus.flplugin.json](../cpp/examples/plugins/offline-focus.flplugin.json) with that exact extension. On GitHub, download the raw file, without HTML or Markdown fences.
2. Open **Settings → Plugins → Import manifest …**, select the file, then select **Fokus / Focus** in the list.
3. Edit **My focus**, **Planned minutes** and **Progress** below. Use short text, a number such as `25`, and progress from `0` to `100`. Use a decimal point for fractional numbers. Click **Save plugin settings**; changes apply immediately.
4. Click **Enable / disable** if the entry is disabled. This example has `sources: []` and requires no host consent.
5. Inspect the plugin page in the running display output or its live preview: local time, planned minutes, your focus text and progress bar should appear. Allow the page rotation to reach it if several pages are active. `25` is a planned duration, not a countdown. Update progress manually in settings.
6. Disable the entry when finished; **Remove** removes it from local management.

The Plugins tab currently has no dedicated plugin preview button. A live preview shows the running state. The developer function `App::plugin_preview` instead uses manifest defaults and `sample`, without network requests. A sample preview does not prove live availability. The tests below validate data and templates, not physical display output.

## 2. Create a personal variant

Copy the focus example, change `id` to `my-focus`, change `name` to your page title and edit the defaults. The ID is the local key: a different ID installs another page; importing the same ID replaces its manifest. Check configuration and activation again afterwards. Previously saved settings can override changed defaults.

This card reads a declared setting:

```json
{"type":"metric","label":"Focus session","value":"{{settings.minutes}}","unit":"min"}
```

Declare `settings.minutes` first. Templates are paths only: no arithmetic, conditions, filters, HTML, JavaScript, shell commands or timers. Spaces make `{{ settings.minutes }}` invalid. A bar needs a value containing only a readable number; put the unit in a separate field. Values outside `min`/`max` are clamped for the bar, while the displayed number remains unchanged.

## 3. Optional: public weather source and consent

[public-weather.flplugin.json](../cpp/examples/plugins/public-weather.flplugin.json) demonstrates a fixed public HTTPS host without a private server or API credential. Its endpoint and fields follow the [official Open-Meteo documentation](https://open-meteo.com/en/docs). Review the provider's current terms and any commercial requirements. Values are weather model data; the included `sample` values are fictional.

1. Import the file and select **Wetter / Weather**.
2. Set display name, latitude (`-90` to `90`) and longitude (`-180` to `180`), for example the default approximate Berlin coordinates `52.52`/`13.41`. The schema does not enforce geographic ranges; enter valid coordinates. Save plugin settings.
3. Click **Enable / disable**. Read the consent dialog: the only expected host is `api.open-meteo.com`. Setting keys sent to it are `latitude` and `longitude`. `place` stays local. The provider also sees your public IP address and request. Prefer approximate city coordinates when precise private coordinates are unnecessary.
4. Approve only the intended request. Network requests require an enabled plugin and host consent. This example uses `refresh_seconds: 900`, an interval of 15 minutes.
5. Check temperature, humidity and **Data time** in running output. Missing data produces missing values; the fictional `18.5`/`62` values are never substituted in live mode. A failed request is no reason to approve additional hosts.
6. **Revoke host consent** disables the plugin and revokes its host grants. Enabling again requires consent.

Your own source must return a JSON object, for example `{"current":{"temperature_2m":18.5}}`. The corresponding reference is `{{weather.current.temperature_2m}}`. Give `sample` the same structure with public, fictional data. For a list, a response such as `{"entries":[{"title":"First entry"}]}` can use:

```json
{"type":"list","label":"Entries","items":"feed.entries","item_template":"{{item.title}}","max_items":3}
```

Declare source ID `feed` first. `items` is a path without braces; `item_template` uses braces. Array indices work as `{{feed.entries.0.title}}`.

## Schema 1 reference

The authority is [plugins.h](../cpp/src/plugins.h) and [plugins.cpp](../cpp/src/plugins.cpp), with `plugin_store.cpp`, `plugin_settings.cpp` and `plugin_transport.cpp` in the same folder covering management and protection. Unknown or misplaced fields, duplicate JSON keys and wrong types are rejected. JSON has no comments or trailing commas.

| Area | Fields and limits |
| --- | --- |
| Root | Required: `schema_version: 1`, `id`, `name`, `version`, `widgets`. Optional: `description`, `refresh_seconds`, `settings`, `sources` and the store fields below. |
| Identity | `id`: 1–48 ASCII characters, starts with `a-z`, then lowercase letters/digits/hyphens; no final or double hyphen, no Windows device names such as `con`, `com1`. `name`: 1–80 characters; `description`: up to 500. |
| Store fields | All optional for local plugins, required for the plugin store (see [package format](PLUGIN-PACKAGE-FORMAT.md)): `license` (only `MIT` or `CC0-1.0`), `author` (1–80 characters), `category` (`system`, `games`, `smart-home`, `info`, `other`), `min_app_version` (`X.Y.Z`, earliest compatible ScreenBuddy version). |
| Version / refresh | `version`: three numeric groups `X.Y.Z`, 1–4 digits per group. `refresh_seconds`: integer 15–3600, default 60. |
| Settings | At most 12 objects: `key`, `label`, `type`, optional `default`, `required` (Boolean). Unique key: 1–40 characters, starts lowercase, followed by `a-z`, `0-9`, `_`. Label: 1–80 characters. Type `text`, `number` or `secret`. Numbers must be finite; text defaults at most 4096 UTF-8 bytes without control characters. Omitted defaults become `0` or `""`. Secret defaults must be `""`. |
| Sources | At most four: `id`, `url`, optional `format: "json"`, `headers`, `sample`. ID follows setting-key rules; cannot be `settings`, `time`, `date`, `item`. `sample` must be an object. |
| URL / headers | Fixed `https://` public DNS host, default port or 443 only; no IP literals, local hosts, fragments or dynamic hosts. URL/header templates may only reference settings. Up to 16 headers: `Accept`, `Authorization`, `X-*` (case insensitive), values without control characters. URL substitutions are component-encoded; header substitutions are not. |
| Widgets | 1–6. All have `type`, optional `label`. `text`: `text`; `metric`: `value`, optional `unit`; `bar`: `value`, numeric `min`/`max` with `max > min`, optional `unit`; `list`: `items`, `item_template`, integer `max_items` 1–5; `clock`: `format` = `time`, `date` or `datetime`. No custom colors, positions or click actions. |
| Templates | `{{time}}`, `{{date}}`, `{{settings.key}}`, `{{source.path.0.field}}`; `{{item.field}}` only inside list entries. Path segments: letter/underscore followed by letters/digits/underscores, or a numeric index. Secret settings are forbidden in display templates. |
| Resources | Manifest at most 128 KiB, JSON depth 32; source response at most 512 KiB with object root; display text bounded to 2048 UTF-8 bytes, smaller cards can truncate earlier. At most 32 installed plugins. Network uses public address validation, no redirects or proxy, and a five-second timeout. |

Missing/null values render as `—`; objects/arrays embedded in text as `[Daten]`. Reference scalar fields for readable output. ScreenBuddy supplies local clock time; API timestamp timezones are determined by the provider.

## Schema 2: zone layout (from 2.10.0)

With `schema_version: 2` you arrange up to 12 widgets (blocks) in **zones** of a grid. The complete, fully local example is [offline-zonen.flplugin.json](../cpp/examples/plugins/offline-zonen.flplugin.json) (clock, date, motto, focus, focus session, progress and two counters). The rules are in the [manifest v2](PLUGIN-SYSTEM.en.md#manifest-v2-zone-layout-2100-in-preparation) section.

1. Set `schema_version` to `2` and give every widget a stable `id` (lowercase letter first, then `a-z`, `0-9`, `_`).
2. Add a `layout`: `columns` and `rows` are integer shares 1–12 (`[1, 1]` = two equally wide columns, `[3, 2, 2]` = three rows in the ratio 3 : 2 : 2). Each zone has `id`, `column`, `row` and optionally `column_span`/`row_span`. Zones must not overlap and contain no further zones.
3. Give every widget the `zone` it belongs in. Up to four blocks of a zone stack in manifest order. Every zone needs at least one block.
4. For the store set `min_app_version: "2.10.0"` and check with `fl-plugin-validate --store`.

On a small area a block shows fewer details (compact, regular, detailed). If a block does not fit at all, ScreenBuddy hides it instead of overlapping or cutting it off, so put the most important blocks into large zones. Without a `layout` a v2 page stays in the automatic grid with at most 6 widgets, and `zone` is not allowed.

### Conditional display with `show_if`

Should a block only appear when there is something to show? Give it a `show_if` in schema 2. The complete local example is [offline-bedingt.flplugin.json](../cpp/examples/plugins/offline-bedingt.flplugin.json): the note appears only if the setting is not empty, "Breaks" only with more than 0 breaks, and the blocks change with mode and progress.

```json
{"type": "text", "id": "hinweis", "zone": "hinweis", "text": "{{settings.note}}", "show_if": {"path": "settings.note"}}
{"type": "metric", "id": "pausen", "zone": "pausen", "value": "{{settings.breaks}}", "show_if": {"path": "settings.breaks", "op": ">", "value": 0}}
```

- Without `op`, `show_if` means "value present": missing, `null`, `""`, `[]` and `{}` hide the block; `false` and `0` count as present.
- With `op` (`==`, `!=`, `<`, `<=`, `>`, `>=`) the value is compared with exactly one fixed literal, without type conversion: `"42"` is not `42`. If the value is missing or has another type, the block stays hidden, also for `!=`.
- Secret settings (`secret`) are forbidden in conditions; the validator then rejects the manifest.
- The preview uses the `sample` data: choose it so the conditional blocks are visible there.

The complete value table is in [conditional display](PLUGIN-SYSTEM.en.md#conditional-display-show_if-phase-3). If a block throws while being evaluated or drawn, ScreenBuddy hides only that block and writes a short log entry without data.

## Credentials and safe sharing

Enter credentials only into a declared `secret` field in local plugin settings. On Windows, stored plugin values use DPAPI protection; values explicitly referenced in requests can still be sent to approved hosts. Prefer `Authorization` over tokens in URLs. An empty credential input preserves its saved value; enable **Clear saved value** and save to remove it.

Before sharing, inspect manifest, `sample`, URL and headers for embedded credentials. Never publish real tokens as defaults, sample data or fixed headers. Never send configuration, decrypted values, logs or private screenshots to an AI assistant. Source changes can require renewed consent and re-entering stored credentials; check explicitly afterwards.

## Reusable AI assistant prompt

```text
Create a declarative ScreenBuddy 2.5 plugin (provisional schema 1).
Goal: [what should appear?]
Desired cards (at most 6): [content]
Prefer offline. Optional public JSON API: [documentation link or none].
Sanitized example object without personal data: [example or none].

Read docs/PLUGIN-TUTORIAL.en.md and cpp/src/plugins.h and plugins.cpp,
or ask me for their contents. Do not invent schema fields or API endpoints.
Deliver UTF-8 JSON as .flplugin.json, brief import/configuration steps,
expected output, all hosts and the setting keys sent to each host.
Do not assume executable scripts, HTML, calculations or private APIs.
Secrets must be secret settings with empty defaults, never embedded in manifests.
Never ask me to paste real tokens, passwords, configurations or private logs.
Use only fictional sample data and label it as such.
Check unknown fields, unique IDs, limits, paths, required fields,
secret-display prohibitions, and host/header rules. Validate the completed file
with the real parse_manifest and render_page when source code is available.
State which checks actually ran and what remains unverified.
A JSON syntax check alone does not validate schema, network or hardware behavior.
```

## Developer checks and troubleshooting

The `fl-plugin-example-tests` target reads every shipped `*.flplugin.json` from `cpp/examples/plugins` with the real parser, renders default/sample values, and checks focus configuration plus weather data flow. It works offline without installation, transport or hardware. From `cpp`, after CMake configuration:

```powershell
cmake --build build/screenbuddy-release --target fl-plugin-example-tests
./build/screenbuddy-release/fl-plugin-example-tests.exe
```

### Validator for your own plugins

`fl-plugin-validate` checks single files or folders with the same parser as the app, fully offline. `--store` adds the store rules, `--app-version X.Y.Z` checks `min_app_version`. For every plugin the output lists the hosts it would send data to. Exit code 0 = everything valid.

```powershell
cmake --build build/screenbuddy-release --target fl-plugin-validate
./build/screenbuddy-release/fl-plugin-validate.exe --store --app-version 2.6.0 my-plugins/
```

On Linux (also for CI): `sh cpp/tests/build-validator-linux.sh` builds parser tests, example tests and the validator with g++ and nlohmann/json.

For personal files, pass another example directory as the first argument. Technical acceptance status is recorded in the [test log](../cpp/TESTPROTOKOLL.md). External weather requests and physical display output need separate validation.

| Symptom | Check |
| --- | --- |
| Import rejected | Raw UTF-8 JSON, schema 1, no additional fields/commas/comments, size limit. |
| Unknown reference | Declare source/setting keys, check spelling, omit spaces inside `{{…}}`. |
| Enabling refused | Save required fields, approve intended hosts. |
| `—` / missing data | JSON path, valid source response, connectivity and consent; sample is never a live fallback. |
| Missing bar | `value` must contain only the number, `max > min`; put units in label/`unit`. |
| Text truncated | Use short labels and values; card dimensions limit available space. |

## Cycling zone (Phase 4b, development)

Only `schema_version: 2` with `layout` permits a zone to carry `"cycle": {"mode": "rotate", "transition": "fade"}`. `mode` is `rotate` or `first_available`; `transition` is `fade` (default) or `none`. Blocks in `rotate` zones may specify integer `seconds` from 3 to 600 (default 8). Without `cycle`, existing stacking is unchanged; schema 1 rejects these keys. Schema 2 Store packages require `min_app_version` of at least `2.10.0`.

`rotate` shows available blocks in manifest order; `first_available` shows the first available block and reacts to data changes. Available means visible under `show_if` and without missing references in that block. If none are available, the zone stays empty. Leaving the visible tiles or updating the manifest resets its schedule. Views and independent devices keep separate schedules.

The output determines the quiet transition: monitor 4 fade steps over 400 ms (2 steps or a cut at high rendering cost), Companion 2 steps over 400 ms with connected clients, serial a band reveal only with a sufficiently fast measured rate, otherwise a cut. Data is frozen during transitions. Resizing ends the transition; theme changes rebuild its image cache. Slow outputs stretch the entry duration to at least four times the last transfer duration. Simultaneous changes within a tile wait at most 600 ms. No sounds or intrusive effects.

Example: [offline-wechselfeld.flplugin.json](../cpp/examples/plugins/offline-wechselfeld.flplugin.json). Import locally to try two texts with durations of 3 and 5 seconds. Physical acceptance and actual serial rate measurement by Basti remain pending; only simulated transfers have been checked. Tap actions are implemented in Phase 4c (development, see below).

Serial reveal timing includes transfer, rendering and a 50 ms rest. Global eight-row bands are revealed in order; an insufficient cycle budget uses a cut. Only Companion discards late steps; monitors show the intermediate state reached.

## Tap actions (schema 2, Phase 4c development)

Add top-level `"links": ["news.example.org"]`, then add `"tap": {"action": "open_link", "url": "https://news.example.org/{{settings.city}}"}` to a block with a declared non-secret `city` setting. For a list, use source values or `item.*` for each row. Values are percent-encoded; missing values produce no target. Only HTTPS with a literal listed public host is accepted (at most four hosts, 2048 bytes after expansion, no fragments or secret settings). Schema 1 rejects both keys. `"tap": {"action": "next"}` is the only other action and requires a `cycle` zone. Schema 2 store packages require `min_app_version` ≥ `2.10.0`.

Select the plugin in Settings → Plugins and explicitly enable **Open links from this plugin on the PC** (off by default). Phone taps open on the PC. This grant is separate from source consent; a link-list-only change resets link consent while retaining source grants and activation only if every other manifest field is identical. Wait 1.0 s after a new Companion target or 0.3 s on a monitor; targets are blocked during transitions. Denied or unstable taps are consumed, with no page change. Launches are limited to one per plugin every two seconds. Pages without targets retain existing navigation. See the [full contract](PLUGIN-SYSTEM.en.md#tap-targets-for-plugin-blocks-phase-4c-development-for-2100) and [test record](../cpp/TESTPROTOKOLL.md); physical acceptance remains pending.


Tap URL validation (parser and again immediately before browser launch): the literal host must be lowercase. Fixed path/query text permits only RFC 3986 characters (unreserved, sub-delimiters, `:`, `@`, `/`, `?`); `%` requires exactly two hexadecimal digits. Raw non-ASCII, spaces, quotes, angle brackets, backslashes, square brackets, carets, backticks, braces and vertical bars are rejected. Braces are allowed only as validated `{{reference}}` placeholders. Values `.` and `..` produce no target; fixed dot path segments (including percent-encoded dots) are rejected. Unicode values remain supported through percent-encoding.

Only an actual link-list change with **every other manifest field identical** retains source consent and activation; link consent is reset. Other updates, including widgets, settings or tap templates changed alongside links, and identical reimports reset all grants (source plugins become disabled). Corrupt link-consent files fail closed; failed consent writes never grant permission. While busy, the link toggle restores its previous state. List rows and targets stay within their block even on shallow surfaces; targets also work in the automatic grid without a layout.

## Editor: arrange a plugin page

1. Install and enable a manifest-v2 plugin locally. Open **Settings → App → Custom views** and select the plugin's "Standard / plugin" entry in the page list. v1 only shows a notice.
2. Set grid columns/rows and weights 1–12. Arrows choose a cell; Enter creates a free zone. Shift+arrow resizes, Alt+arrow moves and Delete removes a zone if its blocks fit the first remaining zone.
3. In the block list, Space shows/hides blocks and Alt+Up/Down reorders. Choose the zone, duration for `rotate`, and an optional condition. New zones need at least one shown block. Conditions offer sample/existing source paths and non-secret settings. Use **Confirm condition** after operator/type changes; commit text with Enter or focus change.
4. Check landscape/portrait preview using plugin samples, the same output renderer and fixed sample time/date. Invalid drafts keep the last valid image and disable **Apply**. Use Undo/Redo buttons or Ctrl+Z/Y in the grid/list.
5. **Apply** saves only the validated arrangement, atomically with a predecessor backup and no restart. The manifest, content, sources, links, tap actions and secrets remain unchanged. **Reset to manifest** confirms before deleting overlay and backup. Built-in pages and native HA are excluded.

You can keep editing during preview. Invalid input is marked without pulling focus back; the same invalid input reports an error only once. Switching plugins asks before discarding unsaved changes (default **No**). Apply/Reset keep the existing fields; focus stays in place or moves from the disabled action button to the grid. Edits made during a save remain in the draft.

Limits, MSAA/keyboard details and stale overlays: [Page editor](PLUGIN-SYSTEM.en.md#page-editor-p6-3-to-p6-5-development-for-2100). Automated evidence and open physical acceptance: [test record](../cpp/TESTPROTOKOLL.md).
