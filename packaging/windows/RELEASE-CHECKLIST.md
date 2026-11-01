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
