# ScreenBuddy 2.9.6 – Veröffentlichungsvorbereitung

Stand: 01.11.2026. **Nicht veröffentlicht.** Microsoft-Konto wartet auf Reaktivierung; keine App-Reservierung/Store-Zertifizierung erfolgt. Companion 1.0.4 wird als APK vorbereitet, ohne Play-Store-Einreichung.

## Fertig

- MSIX-Build, Laufzeiterkennung, Store-Updateweg, eigene Paketdaten und paketfester Autostart.
- Expliziter Import vorhandener Einstellungen, Kopplungen, Plugins, Plugin-Einstellungen und Themes mit Schutz vor Überschreiben.
- Codesignierung als verpflichtender Schritt vor öffentlichem Windows-EXE-Export, einschließlich Zeitstempel und Signaturprüfung. Lokale Entwicklungsbuilds sind weiterhin möglich.
- Companion 1.0.4 / Code 8 mit compileSdk/targetSdk 36, minSdk 26 und sicherer Einrichtung bei Systemleisten, Kamera-Ausschnitten und Bildschirmtastatur. Android-8-Ressourcenfehler behoben.
- Deutsch-englische Store-Texte, vier native PNG-Screenshots mit Beispieldaten, Identitätsvorlage, Signier-/Build-/Prüfskripte und Datenübersicht für die Datenschutzerklärung.

## Geprüft

17 Windows-Suiten bestanden, darunter 3.327 Kernprüfungen, zwei Installer-Erhaltungsfälle und 39 isolierte App-/Companion-Prüfungen. MSIX-Entwicklerregistrierung: Autostart ein/aus mit Rücksetzen, Import samt WhatIf und Überschreibschutz, synthetischer Paketversionswechsel 2.9.6.0 → 2.9.7.0 mit erhaltenen Daten sowie Entfernung des Testpakets bestanden. Der Paketversionswechsel verwendet dieselben Programmdateien und ist keine Store-Auslieferung.

Android: 22 Protokolltests bestanden; Release-Lint ohne Fehler, 31 verbleibende Warnungen. Release-APK-Signatur geprüft, App-ID und Zertifikat stimmen mit Companion 1.0.3 überein. Kein neuer physischer Gerätetest und keine Installation auf vorhandenen Android-Geräten erfolgt.

Vier Screenshots DE/EN mit je 3440 × 1392 Pixeln; aufgenommen aus isolierter Testinstanz ohne echte Konten oder Hardware. Die Verbindungsanzeige im Testmodus ist kein Hardware-Nachweis. Paketinhalt auf Konfigurationen, Logs und Schlüssel geprüft. Eigene Testprozesse/-pakete wurden entfernt; die produktive Windows-Installation blieb bestehen.

## Noch offen

1. Microsoft-Konto reaktivieren, Registrierung vervollständigen und ScreenBuddy reservieren; offizielle Produktidentität übernehmen.
2. Vertrauenswürdiges Codesignierungszertifikat bzw. Signierdienst bereitstellen. Aktuell fehlt beides; deshalb wurde **keine öffentliche EXE ausgeliefert**. Ein selbstsigniertes Zertifikat ist keine Lösung für öffentliche Installationen. Auch gültige Signaturen garantieren keine sofortige SmartScreen-Reputation.
3. Datenschutz-/Supportseiten auf App-Inhalte und öffentliche Erreichbarkeit prüfen; Herausgeberkontakt, Kategorie und IARC-Einstufung abschließen.
4. Signierte MSIX-Installation, echte Store-Updates und Windows App Certification Kit mit finaler Identität prüfen.
5. Abschließenden Gerätetestumfang abstimmen und durchführen: USB/COM, Monitor, Android-Handy/Tablet/TV, WLAN/Tethering, Pairing, Rotation, Touch, Reconnect und Start nach Anmeldung/Boot; API-36-Oberfläche auf Android 15/16 prüfen.
6. Finaler Review/CI und Store-Zertifizierung vor Freigabe.

## Dateien und Fortsetzung

Die signierte APK liegt unter `dist/android/ScreenBuddy-Companion-1.0.4.apk`. Das MSIX unter `dist/windows-local-test/` trägt ausdrücklich eine **lokale Testidentität**, ist unsigniert und nicht zur öffentlichen Einreichung/Verteilung bestimmt. `dist/ScreenBuddy-2.9.6-preparation.zip` bündelt die Unterlagen und Kandidaten mit Prüfsummen.

Details und Befehle: [DE](STORE-DISTRIBUTION.md), [EN](STORE-DISTRIBUTION.en.md), [Freigabe-Checkliste](../packaging/windows/RELEASE-CHECKLIST.md), [Store-Textentwurf](../packaging/windows/STORE-LISTING.md), [Datenschutz-Datenübersicht](../packaging/windows/PRIVACY-DATA-INVENTORY.md).
