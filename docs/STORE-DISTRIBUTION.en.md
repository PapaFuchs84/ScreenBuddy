# Microsoft Store and Companion delivery

[Deutsch](STORE-DISTRIBUTION.md)

The Microsoft Store edition is being prepared as MSIX. Companion 1.1.0 is an unpublished, separate candidate prepared as a signed sideload APK. The scripts neither upload to a store nor create an account.

## Build and package Windows

Use Windows x64, Visual Studio 2022 C++/CMake, Windows SDK 19041 or newer, and the dependencies in `cpp/README.md`. `cpp/build.bat nopause` builds and tests the standalone edition and requires a trusted code-signing certificate before exporting public EXEs. Without a certificate, development binaries remain under `cpp/build/` and no public EXE is exported. CMake also produces `ScreenBuddyStartup.exe`; keep both EXEs together when packaging.

After developer verification and name reservation, use the exact product identity from Partner Center. For local validation only:

```powershell
./packaging/windows/Build-Msix.ps1 -IdentityName FuchsLabs.ScreenBuddy.Test -Publisher 'CN=ScreenBuddy Development' -Executable ./cpp/build/screenbuddy-release/ScreenBuddy.exe
```

The script checks EXE/package version agreement (default 2.9.6.0), stages only the two executables, logos and license notices, and validates with MakeAppx. Without a certificate the output is unsigned. Local installation requires a matching trusted test certificate; `-CertificateThumbprint` uses an existing certificate in the current user's certificate store. The script creates no certificate and installs no trust. Microsoft signs the final Store package. Do not submit the local test identity to the Store.

## Runtime behavior

Package identity is detected at runtime. Packaged apps bypass the standalone installer/uninstaller and GitHub update checks. The update button opens Microsoft's Store update page. Automatic updates and timing depend on Windows/Store settings. Configuration, pairing data, plugins and themes use the package family's `LocalState`. No private configuration, logs or signing keys are bundled.

Startup uses an initially disabled StartupTask and a separate launcher passing `--autostart`. Users enable it in ScreenBuddy; Windows policy or manual disabling may prevent this. The standalone Registry startup entry is not modified. Both editions share the same instance mutex, so close the running edition before switching.

## Import an existing installation

Close both editions. Find the installed package family using `Get-AppxPackage`. The explicit import script copies configuration, pairings, plugins and themes without overwriting destination data or altering the source. DPAPI-protected secrets remain tied to the original Windows user.

```powershell
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<package-family>' -WhatIf
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<package-family>'
```

Launching may already have created `config.toml`; back it up and move it aside before import. Check paths, pairing and integrations afterwards. To return to standalone, close the Store edition and launch the original installation. Its files remain unchanged.

## Companion APK

Use JDK 21, Android SDK 36, AGP 8.10.1 and Gradle 8.11.1. Application ID is `de.fuchslabs.display`, minSdk 26, targetSdk 36, version 1.1.0 / code 9. Setup uses insets for system bars, cutouts and the keyboard. Android 16 large-screen behavior can override orientation preferences; test rendering and touch on tablets/TVs. The new automatic mode defaults to off; permissions and pending device checks are in the [Companion guide](COMPANION.en.md#automatic-mode-companion-110-in-development).

Set `SCREENBUDDY_SIGNING_PROPERTIES` to the established private signing properties file outside the repository and run `./android/Build-Companion.ps1`. This runs protocol tests, release lint and signature verification, then writes `dist/android/ScreenBuddy-Companion-1.1.0.apk` and SHA-256. Updating existing release APKs requires the same signing key and application ID. Do not silently uninstall debug installations.

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

Copy `packaging/windows/store-identity.example.json` to a local JSON file, replace all three fields with the reserved product identity, then run:

```powershell
./packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/store
```

Placeholders and known local-test identities are rejected. The command creates the submission MSIX, listing draft and SHA-256, without uploading. Complete identity verification, product reservation, public privacy/support URLs, rating, hardware acceptance and Microsoft certification before publication.
