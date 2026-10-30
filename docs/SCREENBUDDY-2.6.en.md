# ScreenBuddy 2.6 – Status and limits

**ScreenBuddy 2.6.0 – 30 October 2026.** Plugin store inside the app with a signed index and signed revocation list, eight starter plugins and Companion app 1.0 as a separate download. The release notes with test limits are in [RELEASE-NOTES-2.6.md](RELEASE-NOTES-2.6.md).

## New compared with 2.5

- **Plugin store** as a settings page (next to Companion and Plugins): search, categories, cards with author, version, licence and status. Before installation the card shows which hosts a plugin would send data to.
- **Install and update** with confirmation. Installed plugins stay **off**; enable them under **Plugins**. Every host needs consent. On update, new hosts are shown and all consents are reset.
- **Signing:** index and revocation list are Ed25519-signed; the public key is fixed in the app. The app checks the signature, rollback protection (`sequence`) and the package checksum. Details: [Plugin signing](PLUGIN-SIGNING.md).
- **Sync:** at most once a day in the background, manually via **Refresh store**. Revoked plugins are switched off automatically. Without a network connection the last verified cache is used.
- **Switch "Use the plugin store"** (`[ui] plugin_store`, default on). Off means no requests at all.
- **Starter plugins:** eight plugins in the store repository `PapaFuchs84/ScreenBuddy-Plugins`.
- **Texts:** [Terms of use, rules and privacy](PLUGIN-STORE-TERMS.md) (draft, not legally reviewed) and the [author guide](PLUGIN-AUTHORS.md) for submissions.
- **Companion app 1.0** as a separate download. Setup and limits: [COMPANION.en.md](COMPANION.en.md).

## Still true

- Settings are native (Direct2D/DirectWrite); WebView2 is not required.
- Plugins are data files without program code. Import does not execute code.
- Online sources need consent per host. Credentials stay protected with DPAPI.
- The Windows EXE is unsigned. Verify it with `SHA256SUMS.txt`.

## Limits

- A valid signature only shows that the maintainer published the index. It is not a guarantee that a plugin is harmless. Approval remains a human review.
- There is exactly one signing key. Rotation with several keys is not implemented yet.
- An old, correctly signed version remains effective until it is revoked.
- Store installation and update in an isolated instance are stated separately in the release notes (installing the plugin “Weather” from the live store passed (plugin stays off, no host consented); saving the “Use the plugin store” switch and restarting passed; update passed only as a simulation (version set to 0.9.0 locally), a real live update could not be tested because no second version exists; the search field was not testable by automation. Defects found on the way (plugin files not served by GitHub Pages, switch not saved) are fixed).
- CRL/OCSP revocation checks and the complete UI import/consent workflow for online plugins are not accepted.
- Still open: Windows sign-in test, Wi-Fi, launcher/boot mode, DPI/screen-reader checks and several Android devices.
- The Companion device test of 27 Oct 2026 refers to 0.2.1-preview. A separate device test for 1.0 is not documented in the test record.

## Test record

Automated check counts: 1,312 Windows checks and 2 installer checks passed, Android protocol tests passed. The overall status is in the [test record](../cpp/TESTPROTOKOLL.md). Historical development notes: [archive 2.5](DEVELOPMENT-NOTES-2.5.en.md).

## Documents

- [Release notes 2.6.0](RELEASE-NOTES-2.6.md)
- [Plugin signing](PLUGIN-SIGNING.md) · [Plugin-Signierung (DE)](PLUGIN-SIGNIERUNG.md)
- [Plugin store: terms](PLUGIN-STORE-TERMS.md) · [Bedingungen (DE)](PLUGIN-STORE-BEDINGUNGEN.md)
- [Author guide](PLUGIN-AUTHORS.md) · [Plugins einreichen (DE)](PLUGIN-AUTOREN.md)
- [Plugin system](PLUGIN-SYSTEM.en.md) · [Tutorial](PLUGIN-TUTORIAL.en.md)
