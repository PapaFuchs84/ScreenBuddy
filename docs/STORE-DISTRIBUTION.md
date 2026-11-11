# Microsoft Store und Companion-Auslieferung

[English](STORE-DISTRIBUTION.en.md)

Diese Store-Vorbereitung basiert auf ScreenBuddy 2.11.0. Die Veröffentlichung der MSIX-Ausgabe ist noch offen; die Companion-App bleibt ein separater Auslieferungsweg. Die unten beschriebene APK 1.0.4 ist historische Vorbereitung. Upstream `origin/main` enthält inzwischen Companion 1.1.0 (`4b39e04`, am 10. November geprüft); diese Windows-Store-Arbeit baut oder veröffentlicht keine Android-Version. Kein automatischer Upload und keine Kontoerstellung sind Bestandteil der Buildskripte.

**Aktuell, 10. November:** Der Store-Entwurf „ScreenBuddy by FuchsLabs“ ist reserviert; das Paket mit offizieller Identität ist gebaut. Der vollständige ursprüngliche native Testlauf bestand **25/25 Suiten** (zusätzlich Game-Mode 290 und Eigene Ansichten 2.144 Prüfungen). WACK meldet insgesamt PASS und 13/13 Pflichtprüfungen; ein optionaler S-Modus-Hinweis bleibt dokumentiert. Bereinigte Belege: `packaging/windows/validation-summary-20261110.json`. Microsoft-Zertifizierung, signierte Store-Auslieferung und eine neue physische Geräteabnahme werden damit nicht behauptet.

## Windows bauen und paketieren

Windows x64, Visual Studio 2022 C++/CMake, Windows SDK ab 19041 und die Abhängigkeiten aus `cpp/README.md` werden benötigt. `cpp/build.bat nopause` baut und testet die Standalone-Ausgabe und verlangt vor der Auslieferung ein vertrauenswürdiges Codesignierungszertifikat. Ohne Zertifikat bleibt der lokale Build unter `cpp/build/`, eine öffentliche EXE wird nicht exportiert. CMake baut zusätzlich `ScreenBuddyStartup.exe`; beide EXEs müssen im selben Ausgabeordner liegen.

Die Produktidentität muss nach Kontoerstellung und Namensreservierung aus dem Partner Center übernommen werden. Keine erfundene Herausgeberidentität einreichen. Beispiel mit ausdrücklich lokaler Testidentität:

```powershell
./packaging/windows/Build-Msix.ps1 -IdentityName FuchsLabs.ScreenBuddy.Test -Publisher 'CN=ScreenBuddy Development' -Executable ./cpp/build/screenbuddy-release/ScreenBuddy.exe
```

Das Skript prüft die EXE-Version gegen die MSIX-Version (automatisch aus der EXE abgeleitet, bei 2.11.0: 2.11.0.0), erzeugt einen frischen Paketordner mit ausschließlich EXEs, Logos und Lizenzhinweisen, und validiert mit MakeAppx. Ohne Zertifikat entsteht ein unsigniertes Paket. Für lokale Installation ist ein passendes vertrauenswürdiges Testzertifikat nötig; `-CertificateThumbprint` signiert mit einem bereits im eigenen Zertifikatspeicher vorhandenen Zertifikat. Kein Zertifikat wird automatisch erzeugt oder als vertrauenswürdig installiert. Microsoft signiert die spätere Store-Ausgabe.

## Verhalten der Store-Ausgabe

Paketidentität wird zur Laufzeit erkannt. Eigener Installer/Deinstaller und GitHub-Updateprüfung werden umgangen. „Microsoft Store öffnen“ führt zur Store-Updateseite; Updatezeitpunkt und automatische Updates hängen von den Windows-/Store-Einstellungen ab. Benutzerdaten liegen in `LocalState` der Paketfamilie; Themes und Plugins verwenden denselben Datenstamm. Das Paket enthält keine privaten Konfigurationen, Logs oder Signierschlüssel.

Autostart verwendet eine deaktiviert ausgelieferte StartupTask und einen eigenen Starter mit `--autostart`. Der Benutzer aktiviert ihn in ScreenBuddy; Windows-Richtlinien oder eine manuelle Deaktivierung können das verhindern. Registry-Autostart der Standalone-Ausgabe wird nicht automatisch geändert. Beide Ausgaben verwenden denselben Instanz-Mutex; vor Wechsel eine laufende Ausgabe beenden.

### FPS-Berechtigung im Store-Paket

Das Store-Paket fordert für die FPS-Freischaltung keine erhöhten Rechte an und führt `--grant-perflog` nicht aus. „FPS-Berechtigung prüfen“ erklärt den manuellen Weg: Ein Administrator fügt den Windows-Benutzer zur Gruppe „Leistungsprotokollbenutzer“ hinzu; danach einmal ab- und anmelden. Bereits vorhandene ETW-Berechtigungen und HWiNFO-FPS bleiben verwendbar. Das Manifest fordert keine zusätzliche `allowElevation`-Berechtigung an.

Auch das Abschalten des Hersteller-App-Autostarts wird im Store-Paket nicht erhöht ausgeführt. Der entsprechende Knopf ist deaktiviert; die App verweist auf die Windows-Aufgabenplanung und bei Bedarf einen Administrator. Das Beenden eines laufenden Hersteller-Prozesses mit den vorhandenen Benutzerrechten bleibt verfügbar.

## Vorhandene Installation übernehmen

Beide Programme schließen. Die Paketfamilie mit `Get-AppxPackage` ermitteln. Das Importscript kopiert nur Konfiguration, Kopplungen, Plugins und Themes, ohne bestehende Zieldaten zu überschreiben; die Quelle bleibt erhalten. Verschlüsselte Geheimnisse sind nur im bisherigen Windows-Benutzerkonto verwendbar. Keine Daten an andere Nutzer weitergeben.

```powershell
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<Paketfamilie>' -WhatIf
./packaging/windows/Import-StandaloneData.ps1 -PackageFamilyName '<Paketfamilie>'
```

Ein Start kann bereits `config.toml` erzeugen. In diesem Fall die Store-Konfiguration zuerst sichern und beiseitelegen. Nach Import Verbindungen und Pfade prüfen. Bei Rückkehr zur EXE-Version die Store-Ausgabe beenden; ursprüngliche Dateien wurden nicht verändert.

## Companion-APK (historische Vorbereitung vom 1. November)

Für aktuelle Companion-Downloads und die optionale Automatik ab 1.1.0 die [aktuellen Releases](https://github.com/PapaFuchs84/ScreenBuddy/releases) und [Companion-Anleitung](https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.md) verwenden. Die folgenden Buildangaben dokumentieren die damalige 1.0.4-Vorbereitung und sind keine Empfehlung, eine ältere APK über eine neuere zu installieren.

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

`packaging/windows/store-identity.example.json` in eine lokale JSON-Datei kopieren, alle Identitätsfelder durch die reservierte Produktidentität ersetzen und dann ausführen:

```powershell
./packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/store
```

Platzhalter und bekannte lokale Testidentitäten werden abgelehnt. Der Befehl erzeugt das Einreichungs-MSIX, den Store-Textentwurf, Screenshots samt Herkunftsmanifest, ListingAssets und SHA-256 ohne Upload. Die Prüfsummendatei nennt ausschließlich das gerade gebaute MSIX; ältere MSIX-Dateien im Ausgabeordner gehören nicht zu dieser Einreichung. Existierende Zielordner `Screenshots` oder `ListingAssets` werden vor dem Bauen abgelehnt, damit keine alten oder privaten Bilder hineinmischen; einen neuen Ausgabeordner mit `-OutputDirectory` verwenden. Vier aktuelle DE/EN-Beispielbilder unter `Screenshots/` wurden mit dem echten nativen Page-/TreeCanvas-Renderer im temporären Entwicklerpaket erzeugt (1600 × 1200), sprachlich/visuell geprüft und mit Herkunftsmanifest versehen. Sie zeigen Game-Mode und Editor mit Beispieldaten, keine Desktopaufnahme oder Geräte-/Kontenabnahme. Vor Veröffentlichung fehlen noch Identitätsprüfung, Produktreservierung, öffentliche Datenschutz-/Supportseiten, Altersfreigabe, Geräteabnahme und Microsoft-Zertifizierung.

## Kontostatus am 9. November 2026

Die Anmeldung mit dem persönlichen Microsoft-Konto funktioniert. Nach Auswahl von Einzelentwickler blockiert das Portal die Einrichtung jedoch mit einem deaktivierten Kontostatus und verweist für die Aktivierungsanfrage an `reportap@microsoft.com`. Es wurden weder ein neues Entwicklerkonto angelegt noch ein Produktname reserviert oder eine offizielle Identität erhalten. Der lokale E-Mail-Entwurf `packaging/windows/ACCOUNT-REACTIVATION-DRAFT.txt` wurde nicht versendet.

## Aktueller Prüfstand am 9. November 2026

Der aktuelle Windows-x64-Release-Build und der anschließende UI-Neubau sind erfolgreich. Die lokale MSIX-Prüfung mit Testidentität bestand MakeAppx, Autostart ein/zurück, Import-WhatIf und Überschreibschutz sowie einen synthetischen Wechsel von 2.11.0.0 auf 2.11.1.0 mit erhaltenem LocalState; das Testpaket wurde entfernt. Der paketierte Prüfprozess bestätigt beide Elevationssperren für FPS und Hersteller-Autostart, ohne Windows-Gruppen oder Aufgaben zu ändern. Belege: `cpp/build/msix-checks-20261109/results.json`, `before-update.json`, `after-update.json`. Die nativen Release-Prüfungen wurden mit Fortsetzungsprotokollen für alle 25 Suiten ausgeführt: 23 bestanden. Layout-UI meldete 1.864 bestandene Prüfungen und zehn Snapshot-Abweichungen. Der Vergleich mit unveränderten produktiven Quelldateien aus 7b8d0c1 reproduzierte alle zehn Abweichungen; sämtliche 77 gerenderten PNG-Dateien sind per SHA-256 bytegleich mit dem Store-Build. Der Ausgangsvergleich zeigte zusätzlich sechs Fokus-/Tastaturfehler am aktiv genutzten Desktop (1.858 bestanden / 16 Fehler). Die zehn Snapshot-Abweichungen bestanden somit bereits vor diesen Store-Änderungen; die Suite bleibt fehlschlagend, Referenzbilder wurden nicht geändert. Belege: `cpp/build/layout-base-audit/layout-base-result.txt` und `comparison.json`. Builtin-Snapshots endeten mit Code 2, weil DirectWrite in der Test-Sandbox die installierte benutzerspezifische Roboto-Schrift nicht sieht (dokumentierte Testumgebungsbedingung); keine Produkt-/Schriftänderung wurde vorgenommen. Zusätzlich bestanden Kernprüfungen (3.556), Eigene Ansichten (2.144), Companion-Integration (39) und zwei Installer-Datenerhaltungsfälle. Interaktive WACK-Prüfung blieb wegen des genutzten Desktops offen. Der erste Versuch einer Fensteraufnahme scheiterte ohne gespeichertes Bild. Anschließend wurden vier aktuelle DE/EN-Beispielbilder mit dem tatsächlichen nativen Renderer im Entwicklerpaket erzeugt und visuell geprüft; Manifest und Bildunterschriften unterscheiden diese UI-Beispiele von Desktopaufnahmen und Hardwarebelegen. Die separate Game-Mode-Prüfung scheiterte zunächst an einem fest verdrahteten Test-Buildpfad. Nach dessen Korrektur bestand sie alle 290 Prüfungen (`cpp/build/game-tests-20261109-rerun.txt`); produktiver Game-Mode-Code wurde dafür nicht verändert. Dies ersetzt weder signierte Store-Updates noch Geräteabnahme oder Microsoft-Zertifizierung.

## Aktueller Konten- und Seitenstand am 10. November 2026

Partner Center zeigt das Windows-Entwicklerprogramm jetzt als registriert/aktiv; Der vom Nutzer gewählte Store-Name `ScreenBuddy by FuchsLabs` ist reserviert (Store-ID `9P03461RLGG0`); die exakten offiziellen Werte stehen in `packaging/windows/STORE-LISTING.md`. Der Sperrhinweis vom 9. November ist historisch. Die app-spezifischen zweisprachigen Seiten [Datenschutz](https://fuchslabs.duckdns.org/screenbuddy/privacy/) und [Support](https://fuchslabs.duckdns.org/screenbuddy/support/) sind veröffentlicht: HTTPS 200 und exakte SHA-256-Übereinstimmung mit den geprüften Dateien am 10. November bestätigt (`packaging/windows/website-pages/deployment-verification.json`). Die paketierte App bietet entsprechende Hilfe-/Datenschutzknöpfe. Die tatsächlichen Vertrags-, IARC- und Zertifizierungsschritte werden im Partner Center abgeschlossen; die vorbereiteten Formulardaten und Prüferhinweise liegen unter `packaging/windows/STORE-FORMS-DRAFT.md` und `CERTIFICATION-NOTES.md`.

`DisplayName` in der Identitätsdatei entspricht exakt dem reservierten Store-Titel (`ScreenBuddy by FuchsLabs`). `PublisherDisplayName` bleibt der exakte Microsoft-Wert `Herbst`. Ältere Identitätsdateien ohne `DisplayName` verwenden für lokale Alt-Prüfungen weiterhin `ScreenBuddy`; für die aktuelle Einreichung das Feld ausdrücklich setzen. Das Buildskript übernimmt den XML-maskierten Titel in Produkteigenschaften, App-Anzeige und StartupTask.

## Offizielles Paket und WACK am 10. November

Das offizielle Paket `ScreenBuddy-2.11.0.0-x64.msix` hat SHA-256 `d050177351a490e56d84ff4b61235466e7ba1bf111d2f2e3774a046165d7f6f7` und enthält die geprüfte Windows-EXE `b78e2dc5…`. Die Paketfelder entsprechen der reservierten Identität und dem Titel „ScreenBuddy by FuchsLabs“; Microsoft signiert erst bei der Store-Zertifizierung. WACK wurde tatsächlich unter dieser offiziellen Entwickler-Paketidentität ausgeführt: Gesamtergebnis PASS, kein Teillauf, alle 13 Pflichtprüfungen bestanden. Zehn optionale Prüfungen bestanden; „Blockierte ausführbare Dateien“ meldete Referenzen auf Prozessstart/CLI-Namen. Microsoft beschreibt diese Desktop-Bridge-Prüfung als optionalen Hinweis zur Windows-S-Modus-Kompatibilität, nicht als Store-Aufnahmekriterium. Es wird weder 24/24 noch vollständige S-Modus-Unterstützung behauptet. Bereinigter Beleg: `cpp/build/wack-official-20261110/sanitized-summary.json`. Das rohe XML mit Systemdetails bleibt lokal. Die installierte App wurde nach dem manuellen Beenden wieder mit `--autostart` gestartet und geprüft; das künstliche Entwickler-Testpaket wurde entfernt. Keine Zertifikats-Vertrauensstellung oder Benutzerkonfiguration wurde verändert.

[Microsoft: Desktop Bridge required/optional tests](https://learn.microsoft.com/en-us/windows/uwp/debug-test-perf/windows-desktop-bridge-app-tests).

## Portalstand am 11. November 2026

Der aktive Entwurf ist `1152921505702369346` für Store-ID `9P03461RLGG0`. Microsoft-Entwicklervereinbarung 8.11 wurde nach ausdrücklicher Zustimmung angenommen und im Portal bestätigt. Eigenschaften sind als vollständig bestätigt: Utilities + tools, keine irreführende Unterkategorie und keine MR-/OneDrive-Sicherungs-/Spielaufnahme-/Alternativlaufwerk-Angaben. Beide App-Seiten enthalten inzwischen die freigegebenen Companion-1.1-Automatik-/Berechtigungshinweise; deren Veröffentlichung und HTTPS-Prüfsummen sind bestätigt. Preise sind als vollständig mit 0 EUR bestätigt. Store-Einträge, Live-IARC und Paketupload werden noch im Portal bearbeitet. Ein erfolgreicher Upload, Zertifizierungsauftrag oder eine öffentliche Store-Veröffentlichung wird erst nach bestätigtem Portalstatus gemeldet.
