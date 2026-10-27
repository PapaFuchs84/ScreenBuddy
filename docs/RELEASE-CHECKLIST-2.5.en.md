# ScreenBuddy 2.5.0 – Release checklist

Status: 27 October 2026. This list tracks the **stable** release. Version 2.3.0 remains the published download; the earlier local build was `2.5.0-dev.2`.

Current continuation: local Windows candidate `2.5.0` (hash `17A3DB7C…`) and signed Companion `0.2.1-preview` (versionCode 3) prepared. Component, native UI, API, isolated real-2.3.0 upgrade and 20 Android protocol tests passed. Headless Edge checked 23 steps per guide for JS errors, broken images and horizontal overflow. Full visual/device acceptance remains pending; no publication. See the latest [test record](../cpp/TESTPROTOKOLL.md) and [Android signing/migration](../android/README.md). Back up signing material securely off this machine before publication.


## Before release

- [x] Built the Windows development executable and passed core, plugin, monitor, Companion and isolated installer checks. The current executable hash is in the [test record](../cpp/TESTPROTOKOLL.md).
- [x] Checked native settings and navigation in isolated instances; built all 15 local website staging pages.
- [x] Passed an isolated upgrade from the 2.3.0 executable verified byte-for-byte against the GitHub release to the local 2.5.0-dev.2 candidate: settings and Companion file preserved, old executable backed up, test registry unchanged. This was not a target-system test.
- [ ] Document clean visual acceptance of the **final** candidate with its build hash. The user reported the earlier image fault resolved on 26 October 2026; the previous failed run remains in [hardware acceptance](HARDWARE-ACCEPTANCE-2.5.en.md). No further root-cause work on that fault.
- [ ] Test the upgrade on the target system from its actually installed predecessor and a return to the saved state. The isolated test does not replace that acceptance.
- [ ] Check plugin import, required settings and host consent in the UI; test one approved external HTTPS source live and record the decision on certificate revocation (CRL/OCSP).
- [ ] Visually and interactively accept the native UI with mouse, keyboard, DPI and screen reader. Check changed DE/EN guide images for anonymization and consistency with the UI.
- [x] Regenerated the English RSS/Home Assistant demo images from localized fixtures and checked the matching `docs/screenshots/en/` copies and embedded English guide. `en/page_bf6.png` already correctly shows “Rank”; image manifest and hashes checked.
- [x] Updated the embedded DE/EN guide with complete anonymous captures of the native 2.5 interface and revised its generator and steps. JavaScript syntax and 17 embedded images per language passed checks. The English guide explicitly identifies its currently German-labelled native captures.
- [ ] Visually inspect all 23 steps per language of the generated HTML guides in a browser and add native English captures. All 46 DE/EN steps were rendered in Headless Edge after the fade-in and visually checked in overview sheets; native English captures remain a documented localization limit.
- [ ] Review and integrate the working-tree changes; rebuild and retest the exact commit to publish. Set `cpp/CMakeLists.txt`, `cpp/src/util.h`, `cpp/src/main.cpp` and `cpp/res/app.rc` consistently to `2.5.0`; recheck DE/EN documentation, embedded help and changelogs afterward.
- [x] Prepared a new Android APK as requested: 0.2.1-preview with valid release signing.
- [x] Accepted the signed APK on Redmi 8 over USB tethering: landscape/portrait, real finger gestures, timer and reconnect without a new PIN. Certificate migration tested through an explicitly approved fresh installation.
- Recommendation: also back up signing material securely off this machine.

## Publication and rollback

- [ ] Record final executable hash and size; create checksum and release notes from the **final** build. Current 2.5 source no longer links WebView2; distinguish its remaining limits from the WebView2 requirement of the published 2.3.0 download.
- [ ] Publish `v2.5.0` and the release only after the gates pass; compare the published download and checksum with the final artifact. Keep 2.3.0 as the current download until then.
- [ ] Refresh website data from GitHub after publication, verify generated DE/EN pages and download links, then deploy.
- [ ] Before a production upgrade, back up the executable, `config.toml` and `companion.json` so they can be restored. After installation, verify executable path, file and API version, one running process, autostart, display image, touch and Companion. On image corruption, startup failure or data loss, stop the app and restore the saved 2.3.0 state.

A green build or an earlier visual check does not override an open fault in the final candidate.
