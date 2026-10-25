# ScreenBuddy 2.5 – Development status

**Unpublished development build 2.5.0-dev.2.** Version 2.3.0 remains the latest published release.

## Implemented

- The executable, installer metadata and startup entry use ScreenBuddy. The existing uninstall registry key stays stable for upgrades; the old executable is renamed as a fallback copy.
- The native Win32 settings window provides seven tabs, including the source schema/editor, display and Companion page ordering, app updates and autostart. **Advanced settings** opens the WebView2 workflows for guided account/selection tasks, custom colors, and Companion pairing/QR.
- Declarative schema 1 plugin manifests are strictly validated. The local store can import, enable, disable and remove plugins. Source plugins can be enabled after their required settings are supplied and the user consents to every declared host. Bounded HTTPS fetching runs in its own worker; plugin settings are protected with DPAPI. Importing a manifest does not execute code.
- Data fetching runs in its own worker; rendering remains on the display thread. No live requests to external HTTPS services have been tested.

## Architecture and rollout

The native Win32 shell reuses the existing C++20 implementation for rendering, USB and Companion, avoiding an additional Electron runtime. WebView2 remains for guided workflows. The dev.2 build was not rewritten in Flutter or Tauri. UI, upgrade, hardware and external-service validation are required before release. The installer saves the previous executable as a fallback; a real rollback has not been tested. The public repository and homepage rename remains deferred until after release.

See [Plugin system](PLUGIN-SYSTEM.en.md) for an example and the provisional developer contract.

## Compatibility and open work

Plugin manifests replace neither config.toml nor existing DPAPI credentials or companion.json. A stable 2.5 release still needs complete native feature parity, acceptance of external HTTPS requests, visual and interaction acceptance of the UI, a real upgrade from 2.3.0, hardware testing and a performance comparison with 2.3.0. The repository and homepage have not been publicly renamed.

## Verification

The development build compiled locally on 25 October 2026. Passed: 236 core, 220 manifest-parser, 127 plugin-store, 133 plugin-transport, 135 plugin-settings, 11 monitor and 335 Companion-protocol checks; 39 isolated app, 49 navigation and two installer checks also passed. An isolated instance showed the native window. These checks do not establish live external HTTPS requests, physical display hardware, an upgrade from 2.3.0 or visual/interaction acceptance of the UI. [Test record](../cpp/TESTPROTOKOLL.md).
