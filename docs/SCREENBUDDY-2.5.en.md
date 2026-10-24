# ScreenBuddy 2.5 – Development status

**Unpublished development build 2.5.0-dev.1.** Version 2.3.0 remains the latest published release.

## Implemented

- The executable, installer metadata and startup entry use ScreenBuddy. The existing uninstall registry key stays stable for upgrades; the old executable is renamed as a fallback copy.
- Native Win32 settings provide status, preview, basic options and local plugin management. **Advanced settings** keeps the existing WebView2 interface available for sources, Companion and more options.
- Declarative schema 1 plugin manifests are strictly validated. A local store imports, disables and removes them. Offline plugins without sources can appear as pages. Source plugins remain disabled; network transport, host consent and plugin secret management are still missing. Import does not execute code.

See [Plugin system](PLUGIN-SYSTEM.en.md) for an example and the provisional developer contract.

## Compatibility and open work

Plugin manifests replace neither config.toml nor DPAPI credentials or companion.json. A stable 2.5 release still needs complete native feature parity, secure source fetching, UI and upgrade acceptance, hardware testing and a performance comparison with 2.3.0. The repository and homepage have not been publicly renamed.

## Verification

The development build compiled locally on 25 October 2026. 236 core checks, 220 plugin checks, 11 monitor checks, 335 Companion protocol checks, 39 isolated app/Companion checks, 49 navigation checks and two installer checks passed. An isolated `--no-lcd` instance showed the native settings window. The installer test used a copy of the new build under the legacy filename. This does not prove a production upgrade, native usability across DPI settings or physical output. [Test record](../cpp/TESTPROTOKOLL.md).
