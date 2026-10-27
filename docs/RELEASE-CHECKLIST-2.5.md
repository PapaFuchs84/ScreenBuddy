# ScreenBuddy 2.5.0 – Release-Checkliste

Stand: 27.10.2026. Dies ist eine Arbeitsliste für den **stabilen** Release. Aktuell bleibt 2.3.0 der veröffentlichte Download; der frühere lokale Build trug `2.5.0-dev.2`.

Aktuelle Fortsetzung: lokaler Windows-Kandidat `2.5.0` (Hash `17A3DB7C…`) und signierte Companion-Vorschau `0.2.1-preview` (versionCode 3) vorbereitet. Komponenten, native UI, API, isoliertes echtes 2.3.0-Upgrade und 20 Android-Protokolltests bestanden. Headless Edge prüfte 23 Schritte je Anleitung auf JS-Fehler, defekte Bilder und horizontalen Überlauf. Vollständige Sicht-/Geräteabnahme offen; keine Veröffentlichung. Siehe neuestes [Testprotokoll](../cpp/TESTPROTOKOLL.md) und [Android-Signierung/Migration](../android/README.md). Signiermaterial vor Veröffentlichung zusätzlich sicher außerhalb dieses Rechners sichern.


## Vor dem Release

- [x] Windows-Entwicklungsbuild mit Kern-, Plugin-, Monitor-, Companion- und isolierten Installerprüfungen gebaut. Der aktuelle EXE-Hash steht im [Testprotokoll](../cpp/TESTPROTOKOLL.md).
- [x] Native Einstellungen und Navigation in isolierten Instanzen geprüft; Website-Staging lokal mit 15 Seiten gebaut.
- [x] Isoliertes Upgrade mit der bytegleich zum GitHub-Release geprüften 2.3.0-EXE auf den lokalen 2.5.0-dev.2-Kandidaten bestanden: Konfiguration und Companion-Datei erhalten, alte EXE gesichert, Test-Registry unverändert. Kein Zielsystemtest.
- [ ] Eine fehlerfreie Sichtabnahme des **finalen** Kandidaten mit Build-Hash dokumentieren. Der Nutzer meldete den früheren Bildfehler am 26.10.2026 als behoben; der zuvor fehlgeschlagene Lauf bleibt in der [Hardware-Abnahme](HARDWARE-ACCEPTANCE-2.5.md) dokumentiert. Keine weitere Ursachenanalyse dieses Fehlers.
- [ ] Upgrade auf dem Zielsystem mit tatsächlich installierter Vorgängerversion und eine Rückkehr zum gesicherten Stand prüfen. Der isolierte Test ersetzt diese Abnahme nicht.
- [ ] Plugin-Import, Pflicht-Einstellungen und Host-Freigabe in der Oberfläche prüfen; eine freigegebene externe HTTPS-Quelle live testen und die Entscheidung zu Zertifikatssperren (CRL/OCSP) festhalten.
- [ ] Native Oberfläche mit Maus, Tastatur, DPI und Screenreader visuell und interaktiv abnehmen. Geänderte DE/EN-Anleitungsbilder auf Anonymisierung und Übereinstimmung mit der Oberfläche prüfen.
- [x] Englische RSS-/Home-Assistant-Demobilder aus lokalisierten Fixtures neu erzeugt und mit den `docs/screenshots/en/`-Dubletten sowie der eingebetteten englischen Anleitung abgeglichen. `en/page_bf6.png` zeigt bereits korrekt „Rank“; Bildmanifest und Hashes geprüft.
- [x] Eingebettete DE/EN-Anleitung auf vollständige anonymisierte Aufnahmen der nativen 2.5-Oberfläche umgestellt; Generator und Schritttexte aktualisiert. JavaScript-Syntax und 17 eingebettete Bilder je Sprache geprüft. Die EN-Anleitung kennzeichnet die derzeit deutsch beschrifteten nativen Aufnahmen ausdrücklich.
- [ ] Alle 23 Schritte je Sprache der erzeugten HTML-Anleitungen im Browser visuell prüfen und native EN-Aufnahmen ergänzen. Alle 46 DE/EN-Schritte wurden mit Headless Edge nach Ende der Einblendanimation gerendert und in Übersichtsaufnahmen visuell geprüft; native EN-Aufnahmen bleiben eine dokumentierte Lokalisierungsgrenze.
- [ ] Änderungen des Arbeitsbaums reviewen und integrieren; Build und Tests auf genau dem zu veröffentlichenden Commit erneut ausführen. Versionsangaben in `cpp/CMakeLists.txt`, `cpp/src/util.h`, `cpp/src/main.cpp` und `cpp/res/app.rc` auf `2.5.0` synchronisieren; DE/EN-Dokumentation, eingebettete Hilfe und Changelogs danach erneut prüfen.
- [x] Neue Android-APK auf Nutzerwunsch vorbereitet: 0.2.1-preview mit gültiger Release-Signierung.
- [x] Signierte APK am Redmi 8 über USB-Tethering abgenommen: Quer-/Hochformat, echte Fingerbedienung, Timer und Wiederverbindung ohne neue PIN. Signaturwechsel durch ausdrücklich freigegebene Neuinstallation geprüft.
- Empfehlung: Signiermaterial zusätzlich sicher außerhalb dieses Rechners sichern.

## Veröffentlichung und Rückfall

- [ ] Finalen EXE-Hash und Größe erfassen, Prüfsumme und Release-Notizen aus dem **finalen** Build erstellen. Die aktuelle 2.5-Quellfassung linkt WebView2 nicht mehr; verbleibende Grenzen und die WebView2-Anforderung des veröffentlichten 2.3.0-Downloads klar trennen.
- [ ] Erst nach erfüllten Gates `v2.5.0` und Release veröffentlichen; Download und Prüfsumme gegen das veröffentlichte Artefakt vergleichen. Bis dahin 2.3.0 als aktuellen Download belassen.
- [ ] Website-Daten nach Veröffentlichung neu von GitHub holen, erzeugte DE/EN-Seiten und Downloadlinks prüfen und erst dann deployen.
- [ ] Vor produktivem Upgrade EXE, `config.toml` und `companion.json` rückspielbar sichern. Nach Installation EXE-Pfad, Datei- und API-Version, einen Prozess, Autostart, Displaybild, Touch und Companion prüfen. Bei Bildstörungen, fehlendem Start oder Datenverlust App stoppen und gesicherten 2.3.0-Stand wiederherstellen.

Ein grüner Build oder eine frühere Sichtkontrolle setzt keinen offenen Fehler des finalen Kandidaten außer Kraft.
