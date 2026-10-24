# Plugin system in the ScreenBuddy 2.5 development build

The current contract is **provisional**. It consists of JSON data; native libraries, JavaScript and shell commands are not loaded. A plugin without sources can be imported locally and displayed as a page. A plugin with `sources` is disabled on import and cannot currently be enabled.

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

- Required: `schema_version: 1`, a unique lowercase-letter/digit/hyphen ID, name, version `X.Y.Z`, and 1–6 widgets.
- Widget types: `clock` (`time`, `date`, `datetime`), `text`, `metric`, `bar`, `list`. Text templates support validated references such as `{{time}}`, `{{date}}` and declared source values.
- Optional: up to 12 settings and four JSON sources with fixed HTTPS DNS hosts. The interface validates them but does not fetch them yet. Sample source data is used only for explicit preview, never as live fallback.
- Limits: 128 KiB manifest, 32 installed plugins, JSON depth 32. Unknown fields and duplicate JSON keys are rejected.
- Security status: no plugin code, no source network traffic, no secret storage, no permission consent and no public marketplace yet.

Tests: `fl-plugin-tests` (129) and `fl-plugin-store-tests` (91) passed on Windows on 24 October 2026. The visible UI and hardware output are not yet accepted.
