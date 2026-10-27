# ScreenBuddy 2.5.0

27 October 2026 · Windows 10/11 x64 · Companion 0.2.1-preview (Android 8+).

## Deutsch

Native Direct2D/DirectWrite-Einstellungen ohne WebView2, geführte Einrichtung, Display-/Touch-Auswahl und deklarative Plugins. Signierte Companion-Vorschau 0.2.1-preview separat enthalten. Windows-Sicht-/Touch-/Bedienprüfung, reales Upgrade 2.3.0 → 2.5.0 und Rückfall auf die gesicherte 2.3.0 bestanden; Konfiguration und Kopplungsdatei erhalten. Redmi 8: USB-Tethering, Quer-/Hochformat, Fingerbedienung, 5-Sekunden-Wechsel und Wiederverbindung ohne neue PIN bestanden. Automatisiert: 1.429 Windows-Prüfungen und 20 Android-Protokolltests.

Prüfgrenzen: Online-Plugins bleiben experimentell; CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen. DPI-/Screenreader- und vollständige Funktionsparität sind nicht vollständig geprüft. EN-Anleitung mit ausdrücklich gekennzeichneten deutschen nativen Beispielbildern. Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus und mehrere Android-Geräte bleiben offen.

Die Windows-EXE ist unsigniert; SHA256SUMS.txt mit dem Download vergleichen. Die Android-APK ist mit RSA-3072/APK-v2 signiert. Neues Zertifikat gegenüber früheren Debug-APKs: kein direktes Update; vor einer Deinstallation lokale Daten und Kopplungen sichern beziehungsweise neu koppeln. Vor Windows-Upgrade EXE, config.toml und companion.json rückspielbar sichern.

## English

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

The Windows EXE is unsigned; compare the download with SHA256SUMS.txt. Android uses RSA-3072/APK-v2 signing. Its new certificate differs from earlier debug APKs: no direct update; preserve local data before uninstalling and pair again after a fresh install. Back up the Windows executable, config.toml and companion.json before upgrading.
