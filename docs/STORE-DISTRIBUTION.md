# Microsoft Store und Companion-Auslieferung

[English](STORE-DISTRIBUTION.en.md)

ScreenBuddy 2.9.6 und Companion 1.0.4 sind unveröffentlichte Kandidaten. Die Microsoft-Store-Ausgabe verwendet MSIX; die Companion-App wird vorerst ausschließlich als signierte APK vorbereitet. Kein automatischer Upload und keine Kontoerstellung sind Bestandteil der Buildskripte.

## Windows bauen und paketieren

Windows x64, Visual Studio 2022 C++/CMake, Windows SDK ab 19041 und die Abhängigkeiten aus `cpp/README.md` werden benötigt. `cpp/build.bat nopause` baut und testet die Standalone-Ausgabe und verlangt vor der Auslieferung ein vertrauenswürdiges Codesignierungszertifikat. Ohne Zertifikat bleibt der lokale Build unter `cpp/build/`, eine öffentliche EXE wird nicht exportiert. CMake baut zusätzlich `ScreenBuddyStartup.exe`; beide EXEs müssen im selben Ausgabeordner liegen.

Die Produktidentität muss nach Kontoerstellung und Namensreservierung aus dem Partner Center übernommen werden. Keine erfundene Herausgeberidentität einreichen. Beispiel mit ausdrücklich lokaler Testidentität:

```powershell
./packaging/windows/Build-Msix.ps1 -IdentityName FuchsLabs.ScreenBuddy.Test -Publisher 'CN=ScreenBuddy Development' -Executable ./cpp/build/screenbuddy-release/ScreenBuddy.exe
```

Das Skript prüft die EXE-Version gegen die MSIX-Version (Standard 2.9.6.0), erzeugt einen frischen Paketordner mit ausschließlich EXEs, Logos und Lizenzhinweisen, und validiert mit MakeAppx. Ohne Zertifikat entsteht ein unsigniertes Paket. Für lokale Installation ist ein passendes vertrauenswürdiges Testzertifikat nötig; `-CertificateThumbprint` signiert mit einem bereits im eigenen Zertifikatspeicher vorhandenen Zertifikat. Kein Zertifikat wird automatisch erzeugt oder als vertrauenswürdig installiert. Microsoft signiert die spätere Store-Ausgabe.

## Verhalten der Store-Ausgabe

Paketidentität wird zur Laufzeit erkannt. Eigener Installer/Deinstaller und GitHub-Updateprüfung werden umgangen. „Microsoft Store öffnen“ führt zur Store-Updateseite; Updatezeitpunkt und automatische Updates hängen von den Windows-/Store-Einstellungen ab. Benutzerdaten liegen in `LocalState` der Paketfamilie; Themes und Plugins verwenden denselben Datenstamm. Das Paket enthält keine privaten Konfigurationen, Logs oder Signierschlüssel.

Autostart verwendet eine deaktiviert ausgelieferte StartupTask und einen eigenen Starter mit `--autostart`. Der Benutzer aktiviert ihn in ScreenBuddy; Windows-Richtlinien oder eine manuelle Deaktivierung können das verhindern. Registry-Autostart der Standalone-Ausgabe wird nicht automatisch geändert. Beide Ausgaben verwenden denselben Instanz-Mutex; vor Wechsel eine laufende Ausgabe beenden.

## Vorhandene Installation übernehmen

Beide Programme schließen. Die Paketfamilie mit `Get-AppxPackage` ermitteln. Das Importscript kopiert nur Konfiguration, Kopplungen, Plugins und Themes, ohne bestehende Zieldaten zu überschreiben; die Quelle bleibt erhalten. Verschlüsselte Geheimnisse sind nur im bisherigen Windows-Benutzerkonto verwendbar. Keine Daten an andere Nutzer weitergeben.

```powershell
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<Paketfamilie>' -WhatIf
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<Paketfamilie>'
```

Ein Start kann bereits `config.toml` erzeugen. In diesem Fall die Store-Konfiguration zuerst sichern und beiseitelegen. Nach Import Verbindungen und Pfade prüfen. Bei Rückkehr zur EXE-Version die Store-Ausgabe beenden; ursprüngliche Dateien wurden nicht verändert.

## Companion-APK

JDK 21, Android SDK 36, AGP 8.10.1 und Gradle 8.11.1. App-ID `de.fuchslabs.display`, minSdk 26, targetSdk 36, Version 1.0.4 / Code 8. Einrichtung berücksichtigt Systemleisten, Kamera-Ausschnitt und Bildschirmtastatur. Android-16-Großbildschirmregeln können Orientierungsvorgaben übersteuern; Darstellung und Touch müssen auf Tablet/TV geprüft werden.

`SCREENBUDDY_SIGNING_PROPERTIES` verweist auf die bisherige private Signing-Datei außerhalb des Repositorys. Dann `./android/Build-Companion.ps1` ausführen. Das Skript prüft Protokolltests, Release-Lint und APK-Signatur und erzeugt `dist/android/ScreenBuddy-Companion-1.0.4.apk` mit SHA-256. Gleichbleibende App-ID und gleicher Release-Schlüssel sind Voraussetzung für Updates vorhandener Release-APKs. Debug-Installationen nicht ungefragt deinstallieren.

Play-Veröffentlichung ist ausdrücklich vertagt. Ein späteres AAB kann mit `:app:bundleRelease` erstellt werden; Play App Signing, Datenschutz-/Datensicherheitsangaben und gegebenenfalls geschlossene Tests sind vor einem Upload gesondert vorzubereiten.

## Store-Einreichung und Freigabe

Noch erforderlich: Entwicklerkonto, Identitätsprüfung, Namensreservierung, offizielle Paketidentität, öffentlich erreichbare Datenschutz-/Supportseiten, Beschreibung DE/EN, Screenshots mit Beispieldaten und Altersfreigabe. `runFullTrust` ist nötig, weil ScreenBuddy eine klassische Desktop-App mit Gerätezugriff und lokalen Integrationen ist. Kein eigener Treiber wird im MSIX installiert.

Vor Einreichung: Installation/Deinstallation und Update zwischen zwei MSIX-Versionen, Datenbestand, Autostart, USB/COM, Monitor, WLAN/Tethering, Companion-Pairing/Reconnect/Touch und Plugins/Themes prüfen. Automatische Tests oder MakeAppx-Validierung ersetzen weder Geräteabnahme noch Store-Zertifizierung. Microsoft- und APK-Releases dürfen sich zeitlich unterscheiden; Protokoll und App-ID bleiben erhalten.

Quellen: [MSIX-Vorbereitung](https://learn.microsoft.com/en-us/windows/uwp/porting/desktop-to-uwp-prepare), [Google-Ziel-API](https://support.google.com/googleplay/android-developer/answer/11926878), [AGP 8.10](https://developer.android.com/build/releases/agp-8-10-0-release-notes), [Android-16-Verhalten](https://developer.android.com/about/versions/16/behavior-changes-16).

## Codesignierung

`SCREENBUDDY_CODESIGN_THUMBPRINT` wählt ein öffentlich vertrauenswürdiges Codesignierungszertifikat mit zugänglichem privatem Schlüssel in `Cert:/CurrentUser/My`. `Sign-Windows.ps1` signiert beide EXEs mit SHA-256 und RFC-3161-Zeitstempel. Vor Kopie und Prüfsummenbildung prüft `Export-WindowsRelease.ps1` Vertrauenskette, Zeitstempel und Herausgeber. Private Schlüssel werden nicht exportiert. Selbstsignierte Zertifikate werden für öffentliche EXE-Auslieferungen abgelehnt. Ein geeignetes Zertifikat bzw. Signierdienst fehlt derzeit noch. Die spätere Microsoft-Signierung des Store-Pakets ist ein eigener Weg. Eine gültige Signatur garantiert keine sofortige SmartScreen-Reputation.

[Microsoft signing options](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options) · [SmartScreen reputation](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation)

## Wiederholbare Prüfungen

```powershell
python cpp/tests/run_release_checks.py --build cpp/build/store --output cpp/build/release-checks-new
./packaging/windows/Test-Msix.ps1 -BuildDirectory ./cpp/build/store -OutputDirectory ./cpp/build/msix-checks-new
```

Der zweite Befehl benötigt bereits aktivierten Windows-Entwicklermodus. Er verwendet eine isolierte Testidentität und Beispieldaten, prüft Autostart ein/aus, Importschutz, einen synthetischen Paketversionswechsel und den Erhalt von LocalState und entfernt sein eigenes Testpaket danach. Er installiert kein Zertifikatsvertrauen und beweist keine signierte MSIX-/Store-Auslieferung. Ein unsigniertes Paket mit dieser ausführbaren Autostart-Aktivierung lässt sich nicht mit `Add-AppxPackage -AllowUnsigned` installieren. Signierte Installation und Store-Updates sind nach Bereitstellung der offiziellen Identität/Signierung separat zu prüfen.

## Nach Kontoaktivierung

`packaging/windows/store-identity.example.json` in eine lokale JSON-Datei kopieren, alle drei Felder durch die reservierte Produktidentität ersetzen und dann ausführen:

```powershell
./packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/store
```

Platzhalter und bekannte lokale Testidentitäten werden abgelehnt. Der Befehl erzeugt Einreichungs-MSIX, Store-Textentwurf und SHA-256 ohne Upload. Vor Veröffentlichung fehlen noch Identitätsprüfung, Produktreservierung, öffentliche Datenschutz-/Supportseiten, Altersfreigabe, Geräteabnahme und Microsoft-Zertifizierung.
