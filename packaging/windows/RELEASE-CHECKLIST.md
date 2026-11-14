# Store preparation – 11 November 2026

Current worktree base: Windows ScreenBuddy 2.11.0 at 7b8d0c1, branch feature/msstore-2.11. Upstream was refreshed on 10 November to 4b39e04 (Companion 1.1.0 changes; Windows source unchanged before this Store work). Historical validation remains dated below.

On 10 November the live Partner Center Windows program was verified registered/active. The initially empty product table now contains the reserved draft; Microsoft Developer Agreement 8.11 was explicitly approved by the owner and its acceptance was verified on 10 November. ScreenBuddy by FuchsLabs is reserved (Store ID 9P03461RLGG0), with exact official identity recorded in STORE-LISTING.md. The selected account type is Individual developer (private hobby project), as requested by the user. The 9 November disabled-account observation and unsent reactivation draft are historical, not current blockers.

## Current validation evidence — 10 November

- Final Windows binary: version 2.11.0.0, SHA-256 `b78e2dc576beea84c169b888349ce1cb80ca594d9403dea43f0dff8eeab2a466`. It includes packaged Help & privacy links and the corrected pre-initialization navigation behavior.
- Final targeted Companion startup integration: 40 checks passed. Fresh local developer-package MSIX startup/import/WhatIf/overwrite/synthetic-update/LocalState/uninstall checks passed: `cpp/build/msix-checks-final-20261110/results.json`. This is not signed Store servicing.
- Font test prerequisite corrected by registering the six already-present per-user Roboto font files; focused layout/builtin renders passed without changing goldens. The final original 25-suite runner passed 25/25 with no failed suites on a temporary private Windows desktop (no visible desktop switch), preserving all assertions and golden references. Extra Game Mode 290 and custom-view 2,144 checks also passed. Final core 3,556; layout 1,877; builtin images and hit maps 4,221 each; Companion integration 40; installer preservation 2. Sanitized record: `packaging/windows/validation-summary-20261110.json`; raw run evidence stays in ignored build output.
- Packaging DisplayName/asset/checksum/six-document/identity guard regression passed: `cpp/build/packaging-displayname-final-20261110.txt`. The reserved title is passed into product/app/startup manifest fields with XML escaping; legacy identities without DisplayName retain ScreenBuddy.
- Official-identity MSIX built and independently unpacked/verified: `dist/store-submission-20261110/package-verification.json`, package SHA-256 `d050177351a490e56d84ff4b61235466e7ba1bf111d2f2e3774a046165d7f6f7`.
- Actual official-identity WACK completed: OVERALL_RESULT=PASS, PARTIAL_RUN=FALSE, reset/test exit 0, all 13 required tests passed; 10 optional tests passed and optional Blocked executables reported an informational warning. Microsoft documents Desktop Bridge optional tests as informational (Windows S-mode compatibility), not Store-onboarding pass/fail criteria. No claim of 24/24 or complete S-mode support is made. Sanitized evidence: `cpp/build/wack-official-20261110/sanitized-summary.json`; raw report/transcript remain private in ignored build output. Windows UAC was approved, the user manually quit the installed app, the controller verified its restoration with --autostart, and only our developer-registered official test package was removed afterward. No user process was killed and no certificate trust was installed.
- Four current DE/EN native-rendered 1600 × 1200 settings/editor samples passed visual review; manifest truthfully records developer-package context, sample data and no physical-device/account evidence.
- Public bilingual app policy/support pages were verified HTTPS 200 and exact hashes. The Companion 1.1 automation/permissions amendment was explicitly approved by the owner, deployed only to the two existing app routes with known-old-hash guards and backups, and verified over HTTPS against the new exact hashes. The earlier automatic transfer rejection is resolved; no pending approval remains for this update.

## Earlier 9 November diagnostics (historical)

- Additional current results: core 3,556 checks, custom-view 2,144 checks, Companion integration 39 checks and two installer-preservation cases passed. WACK interactive validation was deferred while the desktop was in use. Initial native-window capture could not activate the intended window and saved no image. The later successful 10 November renderer examples are documented separately above.
- Fresh current-source Windows x64 Release build passed (189 targets), followed by the incremental UI rebuild. The native release checks attempted all 25 suites using continuation logs: 23 passed. Evidence: `cpp/build/release-checks-20261109/results.json`, `cpp/build/release-checks-remaining-20261109/results.json`, `cpp/build/release-checks-tail-20261109/results.json`. Layout UI reported 1,864 passing checks and 10 golden mismatches. A comparison using pristine 7b8d0c1 production-source variants reproduced all ten mismatches (1,858 passes / 16 failures, including six additional focus/keyboard assertions under the active desktop). All 77 rendered PNG files were SHA-256 byte-identical between the Store build and the pristine comparison. The ten golden mismatches therefore predate these Store changes; the suite remains failing, and no golden references have been changed. Evidence: `cpp/build/layout-base-audit/layout-base-result.txt` and `comparison.json`. Builtin snapshots exited 2 because sandbox DirectWrite could not see the installed per-user Roboto font (the documented test-environment condition); no product/font change was made. The separate Game Mode suite initially failed because its icon fixture used a hardcoded build path. After resolving ScreenBuddy.exe beside the test executable, its targeted rebuild and rerun passed all 290 checks. Evidence: `cpp/build/game-tests-20261109-rerun.txt`. No production game code was changed.
- Current local-identity MSIX validation passed: MakeAppx accepted 2.11.0.0; developer-package startup enable/restore, import WhatIf/overwrite protection, synthetic 2.11.1.0 update with LocalState retention and test-package removal completed. The packaged probe reports `fps_elevation_blocked=true` and `vendor_elevation_blocked=true`; no real Windows group membership or scheduler task was changed. Evidence: `cpp/build/msix-checks-20261109/results.json`, `before-update.json`, `after-update.json`. This unsigned local-test/developer-registration route does not prove signed Store servicing or certification.
- Packaging fixture regression passed using PowerShell 5 with process-only execution policy (machine policy unchanged): only the newly built MSIX is hashed despite two stale packages; nested screenshot/logo assets and provenance are copied; reruns refuse existing asset directories without altering the manifest; placeholder, whitespace, numeric and array identity fields are rejected. Fixture: `cpp/build/test-packaging-fixture.ps1`; isolated outputs: `cpp/build/packaging-fixture-*/output`. This fixture mocks the package build and does not establish MakeAppx validation or Store certification.

## Portal state — 11 November

Store ID `9P03461RLGG0`; active submission draft `1152921505702369346`. Properties is verified Complete: primary Utilities + tools; misleading Backup/manage subcategory removed; immersive/MR-PC, OneDrive backup, game recording and alternate-drive declarations are off. Pricing is Complete at 0 EUR. Localized listings, live IARC and upload are being completed in the portal. The local official MSIX is verified. The 8.83 MB file was transferred and the portal shows Validating…; accepted-package validation and certification submission are not yet confirmed.

## Current submission gates

- [x] Partner Center Windows program verified registered/active on 10 November.
- [x] Developer Agreement 8.11 accepted after explicit owner approval; live acceptance verified on 10 November.
- [x] ScreenBuddy by FuchsLabs reserved; exact identity/name/publisher/display name/PFN verified against the portal.
- [x] App-specific bilingual privacy/support pages published and verified HTTPS 200 with exact reviewed hashes; public contact verified. Evidence: `packaging/windows/website-pages/deployment-verification.json`.
- [ ] Complete category and live IARC rating; source-justified form preparation is in `STORE-FORMS-DRAFT.md`.
- [x] Four current 2.11.0 DE/EN native-rendered settings/editor examples (1600 × 1200), correct-language/sample-data visual review; manifest distinguishes developer package context from desktop capture or hardware/account evidence.
- [x] Final current-source Windows build and exact official-identity MSIX verified.
- [x] Actual Windows App Certification Kit overall PASS; all required tests passed, one optional informational S-mode heuristic documented.
- [x] Final original native runner 25/25 plus both extra suites passed, with sanitized current evidence recorded.
- [ ] Signed MSIX installation/update and Store servicing using official identity.
- [ ] Agree and complete current physical acceptance scope before publication.
- [ ] Microsoft certification.

Prepare-Store copies listing text, Screenshots (including provenance) and ListingAssets. SHA256SUMS.txt identifies only the package created by that invocation, excluding older MSIX files. Existing Screenshots/ListingAssets destinations are rejected before building to prevent stale or private asset merges. Use a fresh output directory. An external trusted signing certificate is required for public standalone EXEs, not for MSIX submitted to the Store for Microsoft signing.

---

# Release preparation – 1 November 2026

Status: prepared locally; not published. Microsoft account reactivation is pending. Windows 2.9.6 / Companion 1.0.4 (8). Source base: origin/main 1cf069f. Branch: feature/store-distribution.

## Completed

- [x] Windows x64 release build and version consistency.
- [x] Runtime MSIX detection; package-managed installation/update path and LocalState.
- [x] StartupTask launcher plus enable/disable roundtrip under local package identity.
- [x] Explicit source-preserving import, WhatIf, overwrite refusal and encrypted plugin-settings copy.
- [x] Synthetic 2.9.6.0 → 2.9.7.0 developer-package update with data retention, followed by uninstall.
- [x] 17 Windows suites: 3,327 core assertions; remaining focused suites; two installer-preservation cases; 39 isolated app/Companion integration checks.
- [x] Signed APK: target/compile API 36, min API 26, same application ID and release certificate as 1.0.3.
- [x] 22 Android protocol tests; release lint without errors (existing warnings remain).
- [x] Four DE/EN sample screenshots, native PNGs at 3440 × 1392; no private accounts/configuration included.
- [x] Store listing draft, product identity template and packaging commands.
- [x] Public EXE gate rejects unsigned delivery; Store identity gate rejects placeholders.
- [x] Local test packages/processes removed; existing production installation preserved.

## Still required before public release

- [ ] Microsoft reactivates the developer account; complete identity/profile verification and agreements.
- [ ] Reserve ScreenBuddy; obtain exact package name, publisher identity and publisher display name.
- [ ] Verify public support/privacy pages, publisher contact, category and IARC questionnaire in Partner Center.
- [ ] Supply a publicly trusted Windows code-signing certificate or configure a managed signer. No code-signing certificate was present; no public EXE was exported.
- [ ] Build the submission MSIX with official identity; Microsoft signs it after certification.
- [ ] Test signed MSIX installation/servicing and Store-delivered updates. Developer registration is not equivalent to these paths.
- [ ] Agree final physical test scope: USB/COM panel, monitor, phone/tablet/TV, tethering/WLAN, pairing, reconnect, touch, rotation and boot/startup. No new device firmware or APK installation was performed in this preparation.
- [ ] Exercise target API 36 UI on Android 15/16 devices/emulator; no new physical Android acceptance is claimed.
- [ ] Final review/CI, release approval and Microsoft certification.

Play Store publication is explicitly deferred. The prepared APK can be sideloaded after device acceptance.

## Resume commands

Build development Windows binaries using the documented CMake configuration, then run:

```powershell
python cpp/tests/run_release_checks.py --build cpp/build/store --output cpp/build/release-checks-new
./packaging/windows/Test-Msix.ps1 -BuildDirectory ./cpp/build/store -OutputDirectory ./cpp/build/msix-checks-new
./packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/store
./packaging/windows/Export-WindowsRelease.ps1 -BuildDirectory ./cpp/build/store
./android/Build-Companion.ps1
```

The last two commands require external signing configuration. Never put private keys/passwords in the repository or commands. The export script computes hashes after signing. Do not upload the local-test identity or an unsigned standalone EXE.

## Rollback

Before migrating, back up the user's source configuration and pairing data locally. Import preserves source data and refuses existing destination files. To return to standalone, close Store ScreenBuddy and start the original installation. For a faulty Store release, stop rollout, preserve LocalState and submit a higher-version fixed package; do not assume Microsoft Store supports an in-place downgrade. APK rollback normally requires a higher versionCode rebuild or a user-approved uninstall; do not remove pairings silently.

## Evidence

Local evidence remains in ignored build folders: cpp/build/final-checks/results.json; cpp/build/msix-validation-dev-complete/results.json; Android lint/test reports. Packaged screenshots have their own manifest. MakeAppx validates package structure, not Store policy compliance. Windows App Certification Kit and signed servicing are still pending.
