# Companion Android build

[Deutsch](../docs/COMPANION.md) · [English](../docs/COMPANION.en.md)

Companion app version 1.0 uses versionName 1.0.0 and versionCode 4. It keeps the protocol and application ID of the earlier previews. `app/build.gradle.kts` declares the same values. The Redmi 8 acceptance over USB tethering passed on 27 October 2026 for the predecessor 0.2.1-preview (versionCode 3); see the test record for scope and limits.

Use JDK 21 and Android SDK 34. Run `gradlew.bat :protocol:test :app:assembleDebug` for development. To sign a release, set `SCREENBUDDY_SIGNING_PROPERTIES` to an absolute properties-file path outside the checkout, then run `gradlew.bat :app:assembleRelease`. The properties must contain `storeFile` (absolute path, forward slashes), `storePassword`, `keyAlias` and `keyPassword`. Never put secrets in command arguments or the repository. Without this environment variable, release builds remain unsigned and must not be distributed.

Verify `app/build/outputs/apk/release/app-release.apk` using SDK Build Tools `apksigner verify --verbose --print-certs`, and record its SHA-256 and certificate fingerprint. Back up the private keystore and its passwords securely outside this machine before publication; future APK updates require the same key. No signing material is included here.

The new release certificate differs from the old debug certificate. Android cannot install it as an update over that debug APK. Do not uninstall automatically: this removes local app data and pairings. Keep the old APK available, agree on the device migration, then pair again and test landscape/portrait output, touch, disconnect and reconnect. Previous 0.2.0 device results do not establish acceptance of 0.2.1.

## Deutsch

Die Companion-App 1.0 verwendet versionName 1.0.0 und versionCode 4; Protokoll und App-ID bleiben erhalten. `app/build.gradle.kts` nennt dieselben Werte. Der Gerätetest über USB-Tethering vom 27.10.2026 galt dem Vorgänger 0.2.1-preview (versionCode 3). Für den signierten Build zeigt `SCREENBUDDY_SIGNING_PROPERTIES` auf eine private Properties-Datei außerhalb des Repositorys. Ohne diese Variable bleibt der Release-Build unsigniert. Die vier erforderlichen Felder und die Prüfkommandos stehen oben.

Den privaten Keystore und seine Passwörter vor Veröffentlichung zusätzlich sicher außerhalb dieses Rechners sichern. Alle künftigen Updates brauchen denselben Schlüssel. Der neue Release-Schlüssel unterscheidet sich vom bisherigen Debug-Schlüssel. Eine vorhandene Debug-App deshalb nicht automatisch deinstallieren: lokale Daten und Kopplungen gehen dabei verloren. Migration abstimmen, danach erneut koppeln und Bild, Ausrichtung, Touch sowie Trennen und Wiederverbinden am Gerät prüfen.
