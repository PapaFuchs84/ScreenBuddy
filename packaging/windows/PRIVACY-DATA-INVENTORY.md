# Privacy data inventory / Datenschutz-Datenübersicht

Technical publication preparation, updated 9 November 2026 for Windows 2.11.0. This inventory is not the public policy. App-specific bilingual policy/support pages were published and verified on 10 November at https://fuchslabs.duckdns.org/screenbuddy/privacy/ and /screenbuddy/support/; the separate /datenschutz/ page remains website-only. See website-pages/deployment-verification.json. Do not claim that the desktop app only contacts GitHub after a link click: optional catalog and standalone update checks make network requests.

| Component | Data / storage | Transmission / control |
|---|---|---|
| Windows settings | Local configuration, optional integration account/API data; sensitive values protected with Windows DPAPI | Optional integrations contact their respective configured providers. No credentials in package/listing/screenshots. |
| Store Windows build | Package-family LocalState, pairing state, plugin settings, themes and plugins | App updates are handled by Microsoft Store; the app does not run GitHub release checks in package context. Microsoft Store's own account/diagnostic processing is separate. |
| Standalone update check | Local last-check/version state | Public GitHub release endpoint when enabled; no app account token is needed for that query. |
| Plugin/theme catalog | Local signed catalog/cache and installed packages | Configured catalog/package hosts receive normal network request metadata when enabled. Plugins' network hosts require declared permissions/consent. |
| Companion connection | Desktop pairing state; on Android host, port, fingerprint, device name and encrypted pairing key | TLS connection to the selected PC. Rendered pages can include information from configured integrations; touch/navigation and dimensions travel back to the PC. |
| Optional Companion automation (1.1.0 upstream 4b39e04) | Existing local PC pairing, mDNS discovery/private-network TCP availability probes, standby service/notification and optional overlay/device-admin screen lock | Initially off; no PIN or display data in availability-only TCP probe. System permissions are controlled in Android, including revocation before uninstall where required. No Windows package permission is implied. |
| Android QR scan | Camera access for setup | Local QR decoding; describe this purpose in the privacy text. No Google Play Services dependency is used for scanning. |
| Android storage | App-private preferences; pairing key encrypted via Android Keystore-backed KeyVault; allowBackup=false | Forgetting the PC clears preferences/key and disables optional launcher mode. Test modern device-transfer behavior before making broader backup claims. |
| Local logs | Windows diagnostic files | No automatic support upload was introduced. Users may choose to share diagnostics; remove secrets and private data first. |
| Optional Game Mode | Locally reads installed-game launcher/registry manifests and running-game names, executable paths/PIDs; foreground filename/game name can appear in local detection diagnostics. ETW/HWiNFO provide performance values; no game-process memory access, injection or hooks are used for detection. | When enabled, Steam logo requests send the game app ID and ordinary network request metadata to shared.fastly.steamstatic.com; logos are cached locally, with an executable-icon fallback. Packaged FPS setup does not change Windows group membership. |
| Optional audio visualizer | Processes Windows output audio through loopback for local level/spectrum calculations | No microphone/voice recording feature or automatic audio upload is introduced. |
| Optional AI Status hooks | Local authenticated loopback events: session ID, event, tool and model; status in memory, adapter/key/backups local | Standard adapter does not forward prompt/tool arguments/transcripts. Hook setup/removal requires explicit preview and confirmation. |
| Optional real-account usage | Separately confirmed opt-in reads existing Claude/Codex login access tokens in memory, not refresh tokens | HTTPS only to api.anthropic.com or chatgpt.com for the corresponding provider's usage query; no login-file writes, extra token persistence or token refresh. In-flight queries can finish after opt-out within time limits. |
| Optional external integrations | Friends, game, music, streaming, RSS, smart-home or plugin data according to enabled features | The final policy should name relevant providers/categories, explain the user's control and distinguish local presentation from provider-side processing. |

## Before publication / Vor Veröffentlichung

- Confirm responsible publisher/contact, actual support/privacy URLs and retention/deletion descriptions.
- Provide German and English app-specific privacy information as appropriate.
- Concrete app-policy/support sources are APP-PRIVACY.de/en.md and APP-SUPPORT.de/en.md. Build-AppPages.py generates two self-contained bilingual HTML pages under website-pages/. Deployment and live URL verification are separate, described in WEBSITE-ADDITIVE-DEPLOYMENT.md.
- Review all bundled third-party dependencies and actual data flows; do not simply declare “no collection” based on the absence of advertising.
- Keep access credentials, pairing codes, personal logs and private configuration out of submissions.
- Google Play publication is deferred; no Data safety form has been submitted.

Source inspection: cpp/src/game_detect.cpp, game_logo.cpp, perflog.cpp, secure.cpp, update.cpp, distribution.cpp, plugin_catalog_net.cpp, plugin_transport.cpp; android/app/src/main/AndroidManifest.xml and Store.kt, KeyVault.kt, Pinning.kt. This document does not replace verification of the final public policy or Store forms.
