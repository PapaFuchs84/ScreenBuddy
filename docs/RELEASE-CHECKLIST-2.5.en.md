# ScreenBuddy 2.5.0 – Release checklist

27 October 2026.

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

- [x] Build and component, native UI, API, integration and installer checks passed.
- [x] Signed Android APK and 20 protocol tests passed; real Redmi 8 acceptance passed.
- [x] User confirmed Windows display/touch/native mouse and keyboard acceptance after the observation period.
- [x] Real target-system upgrade and rollback passed with verified executable/configuration/pairing backups.
- [x] 46 DE/EN guide views rendered and visually reviewed; 204 local documentation links checked.

## Final publication

- [x] Verify final installed executable hash and configuration after the documentation-resource build.
- [ ] Merge reviewed PR with passing CI; publish tag v2.5.0, Windows EXE, signed preview APK and SHA256SUMS.txt.
- [ ] Download published assets and verify their checksums.
- [ ] Refresh website release data and deploy the existing website.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

Revocation decision: chain and hostname verification remain enabled; CRL/OCSP is not implemented. Online plugins are explicitly experimental. This is a documented limit, not a claim of full plugin acceptance.

Keep the Android release key and its passwords securely backed up; future APK updates require the same certificate. For rollback stop the app and restore the private, verified 2.3.0 executable, config.toml, companion.json and app-specific startup/uninstall state. [Test record](../cpp/TESTPROTOKOLL.md).
