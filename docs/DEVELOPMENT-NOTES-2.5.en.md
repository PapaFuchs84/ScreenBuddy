## Historical development notes before 2.5.0

The following dated notes describe their earlier test state; the release acceptance above supersedes earlier open items only where explicitly tested.

- 27 October 2026: prepared Companion 0.2.1-preview (versionCode 3) with external private release signing; APK v2 signature and 20 protocol tests verified. Its certificate differs from the previous debug APK; device acceptance and publication remain pending.

- 27 October 2026: The former HTML settings page is neither embedded nor served at `/`. The illustrated DE/EN guide now uses complete anonymous native captures; the English guide identifies their German labels. A new build and isolated checks, including 51 navigation/API checks, passed. Visual browser acceptance of the guide remains open.

### Architecture
- The native Win32 shell reuses the existing C++20 core for rendering, USB and Companion, avoiding an additional Electron runtime. Current source no longer links WebView2; the loopback HTTP service for Companion/API and the embedded guide remain. No Flutter/Tauri rewrite was made in dev.2. Release gates are UI, upgrade, hardware and external-service acceptance. The installer saves the previous executable; a real rollback has not been accepted. Public repository and homepage renaming is deferred until after release.

### Implemented in the unpublished development build
- ScreenBuddy branding for the executable, installer and startup entry; existing uninstall key preserved for upgrades.
- Self-drawn native settings window (Direct2D/DirectWrite) with grouped navigation, live preview, light/dark, DE/EN switching, keyboard operation, accessible names, dirty counter, “Discard” and “Save & apply”. Phase 2 adds native pages for Discord, friends, Home Assistant, Companion pairing/devices, and Colors & design. Other existing pages and plugin management remain available; full feature parity with the previous WebView2 interface is not claimed. New setting `ui.settings_theme` (auto/hell/dunkel).
- Native overview with three direct setup steps; native Display page with Windows touch-assignment status, wizard launch and refresh. Physical touch acceptance remains open.
- Settings now open only in the native window; the active WebView2 fallback and linker dependency were removed. Native startup failures are logged and shown when settings are requested. The former HTML settings page is no longer embedded in the executable or served at `/`; its source file remains for historical tests. A fresh build and native smoke test passed on 26 October 2026.
- Native monitor selection offers Automatic, detected screens and refresh; saved disconnected monitor IDs remain available.
- The native update page can open a reported newer release's verified GitHub page in the default browser; its button stays disabled without a valid URL. No automatic download or installation.
- Strictly validated plugin manifests, local management, bounded HTTPS fetching in a worker, persisted host consent, DPAPI-protected plugin settings and live rendering on the display thread. Source plugins can be enabled after required settings and per-host consent; one public HTTPS fetch passed in isolation, while the full app flow remains open.
- Plugin examples: offline clock, configurable offline focus board and optional public weather source. German and English tutorials cover import, settings, host consent, sample data and limits. All 41 example checks passed with sample data and without hardware output. The public weather source was subsequently fetched live once in a separate isolated test. [Examples](../cpp/examples/plugins/README.md) · [German tutorial](../docs/PLUGIN-TUTORIAL.md) · [English tutorial](../docs/PLUGIN-TUTORIAL.en.md).
- Development status and compatibility limits: [ScreenBuddy 2.5](../docs/SCREENBUDDY-2.5.en.md).

### Fixed in the development build
- Plugin DNS resolution through `GetAddrInfoExW` now uses a completion callback with a separate wait event. Previously, setting `OVERLAPPED.hEvent` together with the callback caused `WSAEINVAL` on Windows and prevented live HTTPS fetches. A local WinSock regression test and an isolated fetch from the public Open-Meteo example passed; the later full build and 134 transport checks passed too.
- mDNS status reports success only after successful registration. Registration and deregistration are logged separately. The full build and tests passed after the fix; a dedicated error-callback mock test is still missing.

### Security hardening after review
- Stored plugin secrets are discarded when a manifest update changes the source (host, URL or headers).
- Windows device names such as `con`, `nul`, `com1`–`com9` and `lpt1`–`lpt9` are rejected as plugin IDs.
- Plugin secrets use their own DPAPI entropy; existing values are migrated automatically.
- Bidi and zero-width characters are rejected in manifest text.
- `.` and `..` are rejected as setting values in URL paths.
- DNS completion is protected against double approval.
- Open: certificate revocation checks (CRL/OCSP) are still missing; the full app flow with host consent and external HTTPS sources has not been accepted.

### Status
- No 2.5 release or download; 2.3.0 remains published. The 26 October 2026 build passed 272 core, 223 parser, 127 store, 134 transport, 135 plugin-settings, 41 example, 11 monitor and 335 Companion checks, plus 27 native UI, 16 mouse, 16 Companion rebuild, 39 app, 49 navigation and two installer checks. English demo images are localized. Visual/interactive UI acceptance, target-system upgrade and final-build acceptance remain open; the user reports the earlier image fault resolved.
