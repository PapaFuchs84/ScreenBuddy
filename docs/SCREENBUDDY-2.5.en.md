# ScreenBuddy 2.5 – Development status

**ScreenBuddy 2.5.0 – 27 October 2026.** Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

See the [release checklist](RELEASE-CHECKLIST-2.5.en.md) for the remaining acceptance gates.

Signed Companion 0.2.1-preview is prepared separately; its new certificate differs from the previous debug APK. Redmi 8 acceptance over USB tethering passed; limits are recorded in the test record. [Signing and migration](../android/README.md).

## Implemented

- The executable, installer metadata and startup entry use ScreenBuddy. FuchsLabs remains credited as the publisher. The existing uninstall registry key stays stable for upgrades; the old executable is renamed as a fallback copy.
- The settings window is native only and self-drawn (Direct2D/DirectWrite), with grouped navigation, live preview, light/dark and DE/EN switching. Phase 2 adds native pages for Discord, combined friends, Home Assistant, Companion pairing/devices, and Colors & design. Display, page ordering, updates, autostart and plugin management are also available. Changes can be saved with **Save & apply** or reverted with **Discard**. A startup failure does not fall back to WebView2; the app logs it and displays an error when settings are opened. Full feature parity with the former HTML interface has not yet been accepted.
- The native overview links three setup steps: output, data sources and page selection. Under **Display**, the native UI shows the Windows touch assignment, opens the Windows wizard directly and can refresh the assignment status. The full wizard has not yet been accepted on physical touch hardware for this build.
- Under **Display**, the native monitor selector offers Automatic, detected screens with resolution, and a refresh action. A saved but currently disconnected monitor ID remains selectable; Automatic remains available when no monitor is detected. The selected output changes only after **Save & apply**.
- Under **Updates & startup**, **Open release page** opens the verified GitHub page of a reported newer release in the default browser. The button stays disabled without a valid release URL; ScreenBuddy does not download or install anything automatically.
- Declarative schema 1 plugin manifests are strictly validated. The local store can import, enable, disable and remove plugins. Source plugins can be enabled after their required settings are supplied and the user consents to every declared host. Bounded HTTPS fetching runs in its own worker; plugin settings are protected with DPAPI. Importing a manifest does not execute code.
- Data fetching runs in its own worker; rendering remains on the display thread. One public HTTPS example fetch passed earlier in isolation; the full in-app flow for import, settings and host consent is still open.
- The [clock](../cpp/examples/plugins/offline-clock.flplugin.json), [focus board](../cpp/examples/plugins/offline-focus.flplugin.json) and [weather source](../cpp/examples/plugins/public-weather.flplugin.json) examples show offline configuration and an optional public HTTPS source. The [example index](../cpp/examples/plugins/README.md) and [English tutorial](PLUGIN-TUTORIAL.en.md) explain import and limitations. Example checks cover manifest validation and rendering only; they make no live requests.

## Architecture and rollout

The native Win32 shell reuses the existing C++20 implementation for rendering, USB and Companion. The 2.5 candidate rebuilt on 27 October 2026 no longer links WebView2; its runtime is not an app prerequisite for this build. The loopback HTTP service remains for Companion/API and the embedded guide; the former HTML settings page is no longer embedded or served at `/`. Its source file remains for historical tests. The dev.2 build was not rewritten in Flutter or Tauri. UI, upgrade, hardware and external-service validation are required before release. The installer saves the previous executable as a fallback; real target-system upgrade and rollback to saved 2.3.0 have passed. The public repository and homepage rename remains deferred until after release.

See [Plugin system](PLUGIN-SYSTEM.en.md) for the provisional contract and [Your first ScreenBuddy plugin](PLUGIN-TUTORIAL.en.md) for a step-by-step tutorial.

## Compatibility and open work

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

## Earlier development verification

On 26 October 2026, the rebuilt development state passed 272 core, 223 manifest-parser, 127 plugin-store, 134 plugin-transport, 135 plugin-settings, 41 plugin-example, 11 monitor and 335 Companion-protocol checks, plus 27 native settings, 16 mouse, 16 Companion rebuild, 39 isolated app, 49 navigation and two installer checks. The app tests used `--no-lcd`, separate configurations and local test ports. They do not establish physical output, the full external plugin flow, a target-system upgrade or full visual/interactive UI acceptance. One external HTTPS example fetch passed separately in an earlier isolated run. [Test record](../cpp/TESTPROTOKOLL.md).
