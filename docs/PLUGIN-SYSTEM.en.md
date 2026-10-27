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

- Required: `schema_version: 1`, a unique lowercase-letter/digit/hyphen ID, name, version `X.Y.Z`, and 1–6 widgets.
- Widget types: `clock` (`time`, `date`, `datetime`), `text`, `metric`, `bar`, `list`. Text templates support validated references such as `{{time}}`, `{{date}}` and declared source values.
- Optional: up to 12 settings and four JSON sources with fixed HTTPS hosts. Bounded HTTPS fetching runs in its own worker. Host grants are stored locally; plugin settings are DPAPI-protected. One isolated public live fetch passed, but the full app flow with host consent has not been accepted. Sample source data is used only for explicit preview, never as live fallback.
- Limits: 128 KiB manifest, 32 installed plugins, JSON depth 32. Unknown fields and duplicate JSON keys are rejected.
- Security status: no plugin code is loaded. Fetches are restricted to declared HTTPS hosts and bounded responses; host grants are stored locally and plugin settings are protected with DPAPI. Certificate revocation checks (CRL/OCSP) are still missing. There is no public marketplace yet.

Checks passed: manifest parser (223), store (127), transport (134, including a local WinSock DNS regression check) and plugin settings (135). A direct, isolated HTTPS fetch from the public Open-Meteo example returned the expected weather fields. Production configuration and host grants were not changed. This does not accept the full app flow, certificate revocation, or hardware output; see the [test record](../cpp/TESTPROTOKOLL.md).

## Create your own plugin

The [step-by-step tutorial](PLUGIN-TUTORIAL.en.md) explains import, settings, host consent, templates and validation with the real parser. Start with the fully local [focus board](../cpp/examples/plugins/offline-focus.flplugin.json); the [weather variant](../cpp/examples/plugins/public-weather.flplugin.json) optionally demonstrates a public JSON source. Sample data is never used as a live fallback.
