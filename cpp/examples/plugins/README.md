# ScreenBuddy plugin examples

These UTF-8 JSON manifests use the provisional ScreenBuddy 2.5 schema 1. They contain data, never executable plugin code.

| File | Purpose | Network |
| --- | --- | --- |
| [offline-clock.flplugin.json](offline-clock.flplugin.json) | Smallest starting point | None |
| [offline-focus.flplugin.json](offline-focus.flplugin.json) | Configurable four-card board with clock, text, metric and manual progress | None |
| [public-weather.flplugin.json](public-weather.flplugin.json) | Optional public JSON source with coordinate settings, host consent and fictional preview data | `api.open-meteo.com`, only after enabling with consent |

The examples carry the store metadata fields (`license`, `author`, `category`, `min_app_version`) and pass `fl-plugin-validate --store`; see the [package format](../../../docs/PLUGIN-PACKAGE-FORMAT.md).

Start with the **[German tutorial](../../../docs/PLUGIN-TUTORIAL.md)** or **[English tutorial](../../../docs/PLUGIN-TUTORIAL.en.md)**. The weather example is optional and follows the [Open-Meteo API documentation](https://open-meteo.com/en/docs); check the provider's current terms for your use. No API credential is included or required by this example. A successful sample preview does not establish live service availability or hardware output.
