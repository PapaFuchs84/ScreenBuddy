# Privacy data inventory / Datenschutz-Datenübersicht

Technical publication preparation, 1 November 2026. Not a published privacy policy. The existing website's privacy page primarily describes website access/contact; confirm and extend it for the apps before using its URL in a Store listing. Do not claim that the desktop app only contacts GitHub after a link click: optional catalog and standalone update checks make network requests.

| Component | Data / storage | Transmission / control |
|---|---|---|
| Windows settings | Local configuration, optional integration account/API data; sensitive values protected with Windows DPAPI | Optional integrations contact their respective configured providers. No credentials in package/listing/screenshots. |
| Store Windows build | Package-family LocalState, pairing state, plugin settings, themes and plugins | App updates are handled by Microsoft Store; the app does not run GitHub release checks in package context. Microsoft Store's own account/diagnostic processing is separate. |
| Standalone update check | Local last-check/version state | Public GitHub release endpoint when enabled; no app account token is needed for that query. |
| Plugin/theme catalog | Local signed catalog/cache and installed packages | Configured catalog/package hosts receive normal network request metadata when enabled. Plugins' network hosts require declared permissions/consent. |
| Companion connection | Desktop pairing state; on Android host, port, fingerprint, device name and encrypted pairing key | TLS connection to the selected PC. Rendered pages can include information from configured integrations; touch/navigation and dimensions travel back to the PC. |
| Android QR scan | Camera access for setup | Local QR decoding; describe this purpose in the privacy text. No Google Play Services dependency is used for scanning. |
| Android storage | App-private preferences; pairing key encrypted via Android Keystore-backed KeyVault; allowBackup=false | Forgetting the PC clears preferences/key and disables optional launcher mode. Test modern device-transfer behavior before making broader backup claims. |
| Local logs | Windows diagnostic files | No automatic support upload was introduced. Users may choose to share diagnostics; remove secrets and private data first. |
| Optional external integrations | Friends, game, music, streaming, RSS, smart-home or plugin data according to enabled features | The final policy should name relevant providers/categories, explain the user's control and distinguish local presentation from provider-side processing. |

## Before publication / Vor Veröffentlichung

- Confirm responsible publisher/contact, actual support/privacy URLs and retention/deletion descriptions.
- Provide German and English app-specific privacy information as appropriate.
- Review all bundled third-party dependencies and actual data flows; do not simply declare “no collection” based on the absence of advertising.
- Keep access credentials, pairing codes, personal logs and private configuration out of submissions.
- Google Play publication is deferred; no Data safety form has been submitted.

Source inspection: cpp/src/secure.cpp, update.cpp, distribution.cpp, plugin_catalog_net.cpp, plugin_transport.cpp; android/app/src/main/AndroidManifest.xml and Store.kt, KeyVault.kt, Pinning.kt. This document does not replace verification of the final public policy or Store forms.
