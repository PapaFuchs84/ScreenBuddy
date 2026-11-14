# Privacy for ScreenBuddy and ScreenBuddy Companion

Updated 10 November 2026. This notice covers ScreenBuddy for Windows and the separately available Android Companion app. Visits to our website are covered by the [website privacy notice](https://fuchslabs.duckdns.org/datenschutz/).

## Controller and contact

FuchsLabs, Sebastian Herbst. The address and other publisher details are in the [legal notice](https://fuchslabs.duckdns.org/impressum/). Privacy and support requests: [fuchslabs2026@gmail.com](mailto:fuchslabs2026@gmail.com).

## Local operation and stored data

ScreenBuddy does not require a FuchsLabs account. Settings, page layouts, themes, plugins, pairings and diagnostic logs are stored on your device. The Store edition uses the Windows package's `LocalState`; the standalone edition uses its own local data directory. Credentials in the designated settings are protected with Windows DPAPI and bound to the Windows account. This does not replace protecting your signed-in Windows account.

System displays read values such as CPU, GPU, memory, temperatures, disk and network measurements. The optional level/spectrum display processes current Windows audio output locally using loopback to calculate levels and frequencies; it is not a microphone or voice-recording feature. ScreenBuddy operates no central analytics, advertising or automatic diagnostic-upload service. This does not mean every feature works offline.

## Connections and optional services

When a network connection is made, the destination provider can receive your IP address, request time, requested content and any identifiers or credentials required for that service. Its own terms and privacy notice also apply; its processing locations may be outside the EU.

- **Updates and catalog:** Microsoft distributes Store-edition updates and processes data under its own terms. The Store edition does not query GitHub app releases. The standalone edition can contact GitHub when update checks are enabled. The plugin/theme catalog downloads catalogs and packages from configured HTTPS hosts and can be disabled in app settings.
- **Friends and Discord:** Configured integrations can query account/player identifiers, display names, avatars, friend lists, presence and game information from Discord, Steam, the Xbox service through OpenXBL (`xbl.io`), and PlayStation Network. Supplied keys or tokens are used for the selected connection. These services are unavailable without their required setup.
- **Music, streaming and RSS:** Spotify processes the configured authorization and playback-status requests; artwork is loaded from Spotify's image service. Streaming releases use TMDB with your API key, region and selected providers. RSS requests the feed addresses you enter; content and links can come from third parties.
- **Smart home:** The optional Home Assistant connection sends requests with your token to your configured instance and displays selected entity states. You control its HTTP/HTTPS address and security.
- **Game Mode:** Local detection reads installed-game launcher files/registry entries and running processes (names, paths and process IDs). It does not modify game processes, inject code or read game-process memory. ETW or HWiNFO supply performance values. Game names and foreground executable names can appear in local diagnostics. Game logos can be downloaded using the Steam app ID from `shared.fastly.steamstatic.com` and cached locally. A configured ping measurement sends network packets to the selected target.
- **Battlefield statistics:** Configured player names or identifiers are sent to `api.gametools.network` to request statistics.
- **Plugins and links:** Installed plugins can use their declared network sources after your permission. Approved link actions open a browser. Review permissions and destination addresses before enabling them.

## AI Status and usage displays

AI Status is optional. After you confirm setup, local Claude Code/Codex hooks send session identifier, event, tool name and model when present to an authenticated receiver on `127.0.0.1`. The standard adapter does not forward prompts, tool arguments or transcripts. Status lives in memory; hook files, adapter, encrypted key and backups remain locally until removed.

Separate real-account switches are off by default and require confirmation. Only then can ScreenBuddy read existing local Claude or Codex sign-in files and send access tokens over HTTPS exclusively to the relevant provider: Anthropic (`api.anthropic.com`) or OpenAI (`chatgpt.com`). These are unofficial usage interfaces and can change. ScreenBuddy does not make an additional persistent copy of these tokens, write the sign-in files or refresh tokens. With account access disabled, real-account sign-in files are not used for these queries. Disabling access stops further queries; requests already in flight can finish within their time limits.

## Companion and your displays

After explicit pairing, the PC sends rendered display pages over TLS to the selected Companion device. Pages can contain information from your enabled integrations. Touch/navigation commands and device dimensions return to the PC. Consider who can see the connected displays.

The Android app stores the PC address, port, fingerprint, device name and pairing key in app-private storage; the key is protected through Android Keystore. Optional camera access decodes setup QR codes locally. “Forget PC” removes the saved connection and key. The app manifest disables Android backup; this is not a claim about every manufacturer's device-transfer implementation.

## Optional Companion automation (from 1.1.0)

If your separately installed Companion version provides this feature, it is initially off and requires an existing pairing. It discovers the paired PC using local mDNS and can check the saved Companion port in the private network by opening a TCP connection; that availability probe sends no pairing code or display data. Enabled standby can run as an Android foreground service with a notification and restart after device boot. Discovery can use additional battery power.

The app can offer optional permission to display over other apps for background opening. On supported phones, separately granted Android device-administrator permission allows it to lock the screen. It does not bypass an existing PIN/biometric lock. Android TV does not offer this administrator-based screen lock; system wake behavior can trigger HDMI-CEC depending on the device. Notification, overlay and device-administrator permissions are controlled in Android. Disable automation in the app and revoke system permissions there if desired; device-administrator permission may need to be disabled before uninstalling. Manual use remains possible without automation.

## Control, retention and deletion

You can disable integrations, remove credentials, revoke pairings, uninstall plugins and delete local app data. Settings, caches, backups and local logs have no central FuchsLabs deletion schedule; depending on the feature and operating system, they remain locally until replaced or deleted. Remove AI hooks through the preview/removal function before deleting their files or app data. Disabling a display alone does not delete existing files. Importing standalone data explicitly preserves the source files.

When requesting support, you send the information you select to FuchsLabs. Do not send passwords, tokens, pairing codes or unredacted configuration. We use the supplied information to answer your request and retain it only as necessary for that purpose or legal obligations. We do not sell this support data.

## Legal bases and rights

Where FuchsLabs processes personal information to provide a requested service or answer a request, the basis is GDPR Article 6(1)(b), or the legitimate interest in support and secure operation under Article 6(1)(f). Additional voluntary processing can be based on consent under Article 6(1)(a), which can be withdrawn for the future. Configured third-party providers also have their own legal bases.

Subject to the GDPR, rights include access, rectification, erasure, restriction, portability and objection, and the right to complain to a supervisory authority. Contact FuchsLabs above for requests concerning FuchsLabs; you may need to contact third-party providers directly about data they hold.
