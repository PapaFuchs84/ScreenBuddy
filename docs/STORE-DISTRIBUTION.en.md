# Microsoft Store and Companion delivery

[Deutsch](STORE-DISTRIBUTION.md)

This Store preparation is based on ScreenBuddy 2.11.0. MSIX publication remains pending; the Companion app has a separate delivery path. The APK 1.0.4 below is historical preparation. Upstream `origin/main` now contains Companion 1.1.0 (`4b39e04`, checked 10 November); this Windows Store work does not build or publish Android releases. The scripts neither upload to a store nor create an account.

**Current, 10 November:** The “ScreenBuddy by FuchsLabs” Store draft is reserved and the official-identity package is built. The complete original native runner passed **25/25 suites** (plus Game Mode 290 and custom-view 2,144 checks). WACK reports overall PASS and 13/13 required tests; one optional S-mode warning remains documented. Sanitized evidence: `packaging/windows/validation-summary-20261110.json`. This does not claim Microsoft certification, signed Store delivery or fresh physical-device acceptance.

## Build and package Windows

Use Windows x64, Visual Studio 2022 C++/CMake, Windows SDK 19041 or newer, and the dependencies in `cpp/README.md`. `cpp/build.bat nopause` builds and tests the standalone edition and requires a trusted code-signing certificate before exporting public EXEs. Without a certificate, development binaries remain under `cpp/build/` and no public EXE is exported. CMake also produces `ScreenBuddyStartup.exe`; keep both EXEs together when packaging.

After developer verification and name reservation, use the exact product identity from Partner Center. For local validation only:

```powershell
./packaging/windows/Build-Msix.ps1 -IdentityName FuchsLabs.ScreenBuddy.Test -Publisher 'CN=ScreenBuddy Development' -Executable ./cpp/build/screenbuddy-release/ScreenBuddy.exe
```

The script checks EXE/package version agreement (derived from the EXE automatically; 2.11.0 uses 2.11.0.0), stages only the two executables, logos and license notices, and validates with MakeAppx. Without a certificate the output is unsigned. Local installation requires a matching trusted test certificate; `-CertificateThumbprint` uses an existing certificate in the current user's certificate store. The script creates no certificate and installs no trust. Microsoft signs the final Store package. Do not submit the local test identity to the Store.

## Runtime behavior

Package identity is detected at runtime. Packaged apps bypass the standalone installer/uninstaller and GitHub update checks. The update button opens Microsoft's Store update page. Automatic updates and timing depend on Windows/Store settings. Configuration, pairing data, plugins and themes use the package family's `LocalState`. No private configuration, logs or signing keys are bundled.

Startup uses an initially disabled StartupTask and a separate launcher passing `--autostart`. Users enable it in ScreenBuddy; Windows policy or manual disabling may prevent this. The standalone Registry startup entry is not modified. Both editions share the same instance mutex, so close the running edition before switching.

### FPS permission in the Store package

The Store package does not elevate for FPS setup and refuses the `--grant-perflog` helper. “Check FPS permission” explains the manual path: an administrator adds the Windows user to the “Performance Log Users” group; then sign out and in once. Existing ETW permissions and HWiNFO FPS remain usable. The manifest does not request the additional `allowElevation` capability.

The Store package also does not elevate to disable vendor-app autostart. That button is disabled; the app directs users to Windows Task Scheduler and an administrator if needed. Stopping a running vendor process with existing user privileges remains available.

## Import an existing installation

Close both editions. Find the installed package family using `Get-AppxPackage`. The explicit import script copies configuration, pairings, plugins and themes without overwriting destination data or altering the source. DPAPI-protected secrets remain tied to the original Windows user.

```powershell
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<package-family>' -WhatIf
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<package-family>'
```

Launching may already have created `config.toml`; back it up and move it aside before import. Check paths, pairing and integrations afterwards. To return to standalone, close the Store edition and launch the original installation. Its files remain unchanged.

## Companion APK (historical preparation from 1 November)

Use the [current releases](https://github.com/PapaFuchs84/ScreenBuddy/releases) and [Companion guide](https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.en.md) for current downloads and optional automation from 1.1.0. The build details below record the earlier 1.0.4 preparation; they do not recommend installing an older APK over a newer one.

Use JDK 21, Android SDK 36, AGP 8.10.1 and Gradle 8.11.1. Application ID is `de.fuchslabs.display`, minSdk 26, targetSdk 36, version 1.0.4 / code 8. Setup uses insets for system bars, cutouts and the keyboard. Android 16 large-screen behavior can override orientation preferences; test rendering and touch on tablets/TVs.

Set `SCREENBUDDY_SIGNING_PROPERTIES` to the established private signing properties file outside the repository and run `./android/Build-Companion.ps1`. This runs protocol tests, release lint and signature verification, then writes `dist/android/ScreenBuddy-Companion-1.0.4.apk` and SHA-256. Updating existing release APKs requires the same signing key and application ID. Do not silently uninstall debug installations.

Play publication is deferred. A future AAB can be built with `:app:bundleRelease`; Play App Signing, privacy/data safety declarations and any required closed testing need separate preparation before upload.

## Submission and acceptance

Still required: developer account and verification, reserved app name, official package identity, public privacy/support pages, DE/EN descriptions, screenshots with sample data and age rating. `runFullTrust` is required for this classic desktop app's device access and local integrations. No Windows driver is installed by the MSIX.

Before submission test installation/removal, an update between two MSIX versions, persistence, startup, USB/COM, monitor output, LAN/tethering, Companion pairing/reconnect/touch, plugins and themes. Automated tests and MakeAppx validation are not hardware acceptance or Store certification. Windows/APK delivery can occur at different times; protocol and application ID remain unchanged.

Sources: [MSIX preparation](https://learn.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-prepare), [Google target API](https://support.google.com/googleplay/android-developer/answer/11926878), [AGP 8.10](https://developer.android.com/build/releases/agp-8-10-0-release-notes), [Android 16 behavior](https://developer.android.com/about/versions/16/behavior-changes-16).

## Codesignierung / Code signing

`SCREENBUDDY_CODESIGN_THUMBPRINT` selects a publicly trusted Code Signing certificate with an accessible private key in `Cert:/CurrentUser/My`. `Sign-Windows.ps1` signs both EXEs with SHA-256 and an RFC-3161 timestamp, then verifies chain, timestamp and publisher before `Export-WindowsRelease.ps1` copies or hashes public artifacts. No private key is exported. Self-signed certificates are refused for public EXE releases. A suitable certificate or managed signing service is still required; none was present on the preparation machine. Store-package signing by Microsoft is separate from standalone EXE signing. A valid signature does not guarantee immediate SmartScreen reputation.

[Microsoft signing options](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options) · [SmartScreen reputation](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation)

## Wiederholbare Prüfungen / Repeatable validation

```powershell
python cpp/tests/run_release_checks.py --build cpp/build/store --output cpp/build/release-checks-new
./packaging/windows/Test-Msix.ps1 -BuildDirectory ./cpp/build/store -OutputDirectory ./cpp/build/msix-checks-new
```

The second command requires Windows developer mode to already be enabled. It uses an isolated test identity and sample data, checks StartupTask enable/disable, import protection, a synthetic package-version update and LocalState persistence, then removes its own package. It neither installs certificate trust nor proves signed MSIX/Store servicing. Unsigned `Add-AppxPackage -AllowUnsigned` cannot install this app's executable startup activation. Test signed installation and Store updates once official identity/signing are available.

## Nach Kontoaktivierung / After account activation

Copy `packaging/windows/store-identity.example.json` to a local JSON file, replace all identity fields with the reserved product identity, then run:

```powershell
./packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/store
```

Placeholders and known local-test identities are rejected. The command creates the submission MSIX, listing draft, screenshots with provenance manifest, ListingAssets and SHA-256, without uploading. The checksum file names only the MSIX just built; older MSIX files left in the output directory are not part of this submission. Existing destination directories named `Screenshots` or `ListingAssets` are rejected before building to prevent merging obsolete or private images; use a fresh directory via `-OutputDirectory`. Four current DE/EN sample images in `Screenshots/` use the actual native Page/TreeCanvas renderer under a temporary developer package identity (1600 × 1200), with visual/language review and provenance. They show Game Mode and the editor with sample data, not desktop captures or device/account acceptance. Complete identity verification, product reservation, public privacy/support URLs, rating, hardware acceptance and Microsoft certification before publication.

## Account status on 9 November 2026

Personal Microsoft account sign-in works. After selecting Individual developer, the portal blocks setup because the account is disabled and directs activation requests to `reportap@microsoft.com`. No new developer account or product reservation was created and no official identity was obtained. The local email draft `packaging/windows/ACCOUNT-REACTIVATION-DRAFT.txt` has not been sent.

## Current validation on 9 November 2026

The current Windows x64 Release build and subsequent UI rebuild passed. Local MSIX validation with a test identity passed MakeAppx, startup enable/restore, import WhatIf/overwrite protection and a synthetic update from 2.11.0.0 to 2.11.1.0 retaining LocalState; the test package was removed. The packaged probe confirms both FPS and vendor-autostart elevation blocks without changing Windows groups or tasks. Evidence: `cpp/build/msix-checks-20261109/results.json`, `before-update.json`, `after-update.json`. All 25 native release suites were attempted using continuation logs: 23 passed. Layout UI reported 1,864 passing checks and ten golden mismatches. A comparison using unchanged production-source variants from 7b8d0c1 reproduced all ten mismatches; all 77 rendered PNG files were SHA-256 byte-identical to the Store build. The baseline comparison also had six additional focus/keyboard failures under the active desktop (1,858 passes / 16 failures). The ten golden mismatches therefore predate these Store changes; the suite remains failing and reference images have not been changed. Evidence: `cpp/build/layout-base-audit/layout-base-result.txt` and `comparison.json`. Builtin snapshots exited 2 because sandbox DirectWrite cannot see the installed per-user Roboto font (a documented test-environment condition); no product/font change was made. Additional passes: core checks (3,556), custom views (2,144), Companion integration (39) and two installer-preservation cases. Interactive WACK validation was deferred while the desktop was in use. The initial native-window capture attempt failed without saving an image. Four current DE/EN samples were then generated with the actual native renderer under a developer package and visually checked; manifest and captions distinguish these UI samples from desktop captures and hardware evidence. The separate Game Mode suite initially failed on a hardcoded test-build path. After correcting that fixture, all 290 checks passed (`cpp/build/game-tests-20261109-rerun.txt`); no production Game Mode code was changed. This does not replace signed Store servicing, physical acceptance or Microsoft certification.

## Current account and public pages on 10 November 2026

Partner Center now shows the Windows developer program as registered/active; the owner-selected Store title `ScreenBuddy by FuchsLabs` is reserved (Store ID `9P03461RLGG0`); exact official values are recorded in `packaging/windows/STORE-LISTING.md`. The disabled-account observation from 9 November is historical. The bilingual app-specific [privacy](https://fuchslabs.duckdns.org/screenbuddy/privacy/) and [support](https://fuchslabs.duckdns.org/screenbuddy/support/) pages are public: HTTPS 200 and exact SHA-256 matches to reviewed files were confirmed on 10 November (`packaging/windows/website-pages/deployment-verification.json`). The packaged app provides matching Help & privacy buttons. Actual agreement, IARC and certification steps must be completed in Partner Center; prepared form data/reviewer notes are in `packaging/windows/STORE-FORMS-DRAFT.md` and `CERTIFICATION-NOTES.md`.

`DisplayName` in the identity file must match the reserved Store title exactly (`ScreenBuddy by FuchsLabs`). `PublisherDisplayName` remains the exact Microsoft value `Herbst`. Older identity files without `DisplayName` retain `ScreenBuddy` for legacy local validation; set this field explicitly for the current submission. The build script XML-escapes the title into product properties, app display name and StartupTask.

## Official package and WACK on 10 November

The official `ScreenBuddy-2.11.0.0-x64.msix` has SHA-256 `d050177351a490e56d84ff4b61235466e7ba1bf111d2f2e3774a046165d7f6f7` and contains the verified Windows executable `b78e2dc5…`. Package fields match the reserved identity and title “ScreenBuddy by FuchsLabs”; Microsoft signs during Store certification. WACK actually ran under this official developer-package identity: overall PASS, no partial run, all 13 required tests passed. Ten optional tests passed; Blocked executables reported process-launch/CLI-name references. Microsoft describes this Desktop Bridge test as optional Windows S-mode information rather than a Store-onboarding criterion. Neither 24/24 nor full S-mode support is claimed. Sanitized evidence: `cpp/build/wack-official-20261110/sanitized-summary.json`. Raw XML containing system details stays local. The installed app was restarted with `--autostart` after manual Quit and verified running; the artificial developer test package was removed. No certificate trust or user configuration was changed.

[Microsoft: Desktop Bridge required/optional tests](https://learn.microsoft.com/en-us/windows/uwp/debug-test-perf/windows-desktop-bridge-app-tests).

## Portal state on 11 November 2026

The active draft is `1152921505702369346` for Store ID `9P03461RLGG0`. Microsoft Developer Agreement 8.11 was accepted after explicit owner approval and verified in the portal. Properties is Complete: Utilities + tools, no misleading subcategory and no MR/OneDrive-backup/game-recording/alternate-drive declarations. Both app pages now include the approved Companion 1.1 automation/permission disclosures, with deployment and HTTPS hashes verified. Pricing is Complete at 0 EUR. Listings, live IARC and package upload are still being handled in the portal. Successful upload, certification submission or public Store publication will be reported only after the portal confirms them.
