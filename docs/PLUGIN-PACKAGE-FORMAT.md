# Plugin package format and validator

© 2026 FuchsLabs · Phase 1 of the plugin platform (groundwork for the future plugin store). **Status:** manifest fields and validator are implemented; the store, signing and the in-app store UI come in later phases and do not exist yet.

## A plugin is one file

A plugin stays **one** UTF-8 JSON file `<id>.flplugin.json` (schema 1, at most 128 KiB) with no executable code. There are no archives, scripts or extra files. For the planned plugin collection: one folder per plugin, `plugins/<id>/<id>.flplugin.json`.

## New manifest fields (all optional, schema stays 1)

| Field | Rule |
| --- | --- |
| `license` | Only `MIT` or `CC0-1.0` (exact spelling). |
| `author` | 1–80 characters, no control or invisible format characters. |
| `category` | `system`, `games`, `smart-home`, `info` or `other`. |
| `min_app_version` | `X.Y.Z`. Older ScreenBuddy versions will later show the plugin as unsupported (UI follows with the store). |

Existing manifests without these fields stay valid and importable locally. An invalid value in one of these fields makes the import fail.

## Additional store rules

To be accepted into the store, these also apply (`fl-plugin-validate --store`):

- `description` (at least 10 characters), `author`, `license`, `category`, `min_app_version` present.
- Plugins with sources: `refresh_seconds` of at least 60 and a `sample` for every source (basis of the preview).
- File name `<id>.flplugin.json`, unique IDs.

These rules are an **automatic pre-check**. Approval remains a human review (purpose, hosts, license).

## Validator

```text
fl-plugin-validate [--store] [--app-version X.Y.Z] <file-or-folder>...
```

It uses the app's `plugins.cpp` (same parser), only reads files and makes no network requests. For each plugin it prints `OK` with ID, version and the hosts data would be sent to, or `FAIL` with fixed error messages. Exit code: 0 valid, 1 problems, 2 usage error. Tests run in GitHub Actions (`Plugin validator`) and locally with `sh cpp/tests/build-validator-linux.sh`.

## Open (later phases)

Signing is implemented as a library and tool, see [Plugin signing](PLUGIN-SIGNING.md); production key, index build via Actions and download are open. In-app store UI (phase 3), moderation and terms of use (phase 4). See the plan in PR [#21](https://github.com/PapaFuchs84/ScreenBuddy/pull/21).
