# Companion Android build

[Deutsch](../docs/COMPANION.md) · [English](../docs/COMPANION.en.md)

Companion 1.1.0 is the current version with versionName 1.1.0 and versionCode 9: dark default design and opt-in automatic mode using the paired PC's mDNS announcement. It includes the earlier API 36/setup-insets changes and keeps the protocol and application ID. `app/build.gradle.kts` declares the same values. Permissions, phone locking, TV limits and test limits: [automatic-mode guide](../docs/COMPANION.en.md#automatic-mode-companion-110). The Redmi 8 acceptance over USB tethering on 27 October 2026 was for predecessor 0.2.1-preview (versionCode 3), not this version.

Use JDK 21 and Android SDK 36. Run `gradlew.bat :protocol:test :app:assembleDebug` for development. To sign a release, set `SCREENBUDDY_SIGNING_PROPERTIES` to an absolute properties-file path outside the checkout, then run `gradlew.bat :app:assembleRelease`. The properties must contain `storeFile` (absolute path, forward slashes), `storePassword`, `keyAlias` and `keyPassword`. Never put secrets in command arguments or the repository. Without this environment variable, release builds remain unsigned and must not be distributed.

Verify `app/build/outputs/apk/release/app-release.apk` using SDK Build Tools `apksigner verify --verbose --print-certs`, and record its SHA-256 and certificate fingerprint. Back up the private keystore and its passwords securely outside this machine before publication; future APK updates require the same key. No signing material is included here.

The new release certificate differs from the old debug certificate. Android cannot install it as an update over that debug APK. Do not uninstall automatically: this removes local app data and pairings. Keep the old APK available, agree on the device migration, then pair again and test landscape/portrait output, touch, disconnect and reconnect. Previous 0.2.0 device results do not establish acceptance of 0.2.1.

## Deutsch

Companion 1.1.0 ist die aktuelle Version mit versionName 1.1.0 und versionCode 9: dunkles Standard-Design und optionale Automatik über die mDNS-Ankündigung des gekoppelten PCs. Die früheren API-36-/Inset-Änderungen sind enthalten; Protokoll und App-ID bleiben erhalten. `app/build.gradle.kts` nennt dieselben Werte. Freigaben, Handy-Sperre, TV-Grenzen und Prüfgrenzen: [Automatik-Anleitung](../docs/COMPANION.md#automatik-companion-110). Der Gerätetest über USB-Tethering vom 27.10.2026 galt dem Vorgänger 0.2.1-preview (versionCode 3), nicht dieser Version. Für den signierten Build zeigt `SCREENBUDDY_SIGNING_PROPERTIES` auf eine private Properties-Datei außerhalb des Repositorys. Ohne diese Variable bleibt der Release-Build unsigniert. Die vier erforderlichen Felder und die Prüfkommandos stehen oben.

Den privaten Keystore und seine Passwörter vor Veröffentlichung zusätzlich sicher außerhalb dieses Rechners sichern. Alle künftigen Updates brauchen denselben Schlüssel. Der neue Release-Schlüssel unterscheidet sich vom bisherigen Debug-Schlüssel. Eine vorhandene Debug-App deshalb nicht automatisch deinstallieren: lokale Daten und Kopplungen gehen dabei verloren. Migration abstimmen, danach erneut koppeln und Bild, Ausrichtung, Touch sowie Trennen und Wiederverbinden am Gerät prüfen.

API-36 migration: AGP 8.10.1 / Gradle 8.11.1. Setup handles system bars, display cutouts and the keyboard using window insets. Android 8 (minSdk 26) remains supported. Build signed delivery APKs with `./Build-Companion.ps1`; no Play upload is performed. Future AAB creation uses `:app:bundleRelease`, subject to separate Play preparation and review.

Phase 4a (development, 2026-11-06): Android TV uses sensor landscape regardless of the stored orientation and hides that setting; phones retain all choices. The stored value is preserved. No signing or installation is part of this change. PC keepout/overscan and validation status: [Companion guide](../docs/COMPANION.en.md), [test record](../cpp/TESTPROTOKOLL.md).

Phase 4a (Entwicklung, 06.11.2026): Android-TV nutzt unabhängig von der gespeicherten Ausrichtung Sensor-Querformat und blendet die Auswahl aus; Handys behalten alle Möglichkeiten. Der gespeicherte Wert bleibt erhalten. Keine Signierung oder Installation in diesem Paket. PC-Sperrzone/Overscan und Prüfstand: [Companion-Anleitung](../docs/COMPANION.md), [Testprotokoll](../cpp/TESTPROTOKOLL.md).
