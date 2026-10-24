# Testprotokoll – Stand 24.10.2026

## Version 2.3.0 (24.10.2026): Companion-Vorschau

- **Windows:** Release-Build erfolgreich. 236 Kernprüfungen, 11 Monitorprüfungen und 335 Companion-Protokoll-/TLS-Prüfungen bestanden. Neue TLS-Prüfungen: Anmeldung vor Größen-/Wischmeldungen, native Größen und Rotation, `ready` vor dem passenden JPEG, numerische Schnellfix-Kompatibilität und Schutz vor Größenänderungen durch einen zweiten Client.
- **Android:** `:protocol:test` (20 Tests, keine Fehler) und `:app:assembleDebug` erfolgreich mit Gradle 8.9/JDK 21. Die aktuelle Debug-APK wurde mit ausdrücklicher Freigabe auf dem Redmi 8 installiert.
- **Isolierte App:** 39 Prüfungen mit echten PC-App-Prozessen, eigener Konfiguration und freien Ports: Start ohne Companion, unabhängige Seitenwahl, Navigation in beiden Richtungen, eigener Timer, native Größe, Rotation, Nur-Companion-Modus und sauberes Beenden. Hauptausgang mit `--no-lcd` simuliert; kein serieller Zugriff.
- **Oberfläche:** 22 Headless-Edge-Prüfungen mit ausschließlich synthetischen Daten (DE/EN, Zusatzoptionen bei Companion aus verborgen, getrennte Auswahllisten, keine JavaScript-Fehler, Syntax beider Anleitungs-Vorlagen und eingebetteter HTML-Dateien). Beispielansicht manuell visuell kontrolliert; keine echten Accountbilder im Repository.
- **Echtes Redmi 8, USB-Tethering:** Verbindung zum eigenen PC-Testserver, TLS-Fingerabdruck vor Kopplung verglichen. Native Ausgabe und tatsächlicher Screenshot 1520 × 720. Automatischer Wechsel nach 5 Sekunden, Wischen in beide Richtungen und Tipp auf die Kopfzeile über Android-Eingabeinjektion; dabei bleibt die Hauptansicht des Testprozesses unverändert. Neustart der Android-App verbindet mit gespeichertem Schlüssel ohne neue PIN. Ausrichtungswahl in der App: Hochformat 720 × 1520, anschließend Querformat 1520 × 720 wiederhergestellt.
- **Manuelle Abnahme durch den Nutzer:** Auf die ausdrückliche Prüfung von scharfem Bild, langsamem/normalem Wischen und Antippen am Redmi antwortete der Nutzer: „Alles funktioniert mit dem Finger“. Diese Bestätigung gilt dem aktuellen Gesamtstand.
- **Abgrenzung:** Die automatischen Gesten wurden per ADB injiziert; die manuelle Bestätigung ist separat belegt. Die produktive USB-App wurde nicht ersetzt und ihr COM-Port nicht geöffnet. Ein gemeinsamer physischer USB-/Redmi-Test innerhalb derselben neuen PC-Instanz, WLAN, Launcher-/Boot-Verhalten und mehrere reale Android-Geräte wurden nicht geprüft.

Reproduzieren: `cpp/build.bat nopause`; `python cpp/tests/app_companion_integration.py`; anschließend `node cpp/tests/app_companion_ui.cjs` mit installiertem Playwright und Edge. Android: `android/gradlew.bat :protocol:test :app:assembleDebug`. Lokale Konfigurationen, Testkopplungen, Logs und Gerätescreenshots bleiben unter ignorierten Build-Pfaden.


## Version 2.2.0 (24.10.2026): Systemseite per Touch, Update-Hinweis

- Neu: Tipp auf RAM/Datenträger/Netzwerk der Systemseite (Task-Manager, „Programme und Features“, speedtest.net/de) und Update-Hinweis über die öffentlichen GitHub-Releases (höchstens 1× täglich, Sprechblase und Anzeige in den Einstellungen, Schalter `[ui] update_check`). Release-Build erfolgreich, **225 Kernprüfungen und 11 Monitor-Prüfungen bestanden**. Neue Prüfungen: Trefferflächen der Systemseite (inkl. skaliertem Display, Kopf-/Fußzeile, CPU-Karte, Aktionsaufruf je Zone), Versionsvergleich (Vorabversionen, ungültige Tags), Auswertung der GitHub-Antwort (Entwurf/Vorabversion verworfen, fremde Adresse ersetzt).
- Trefferflächen gegen das mit `--demo --preview` gerenderte Systembild geprüft; das Format der echten Antwort von `releases/latest` (Tag `v2.1.0`) abgerufen und entspricht dem Auswerter. Keine Installation, kein Test gegen Port 8765; die Prüfungen starten weder Task-Manager noch Browser.
- **Nicht getestet:** Tippen auf echter Touch-Hardware (Fenster erscheinen ggf. auf dem Hauptbildschirm statt auf dem Touch-Monitor), die Sprechblase und die Einstellungsanzeige mit einer tatsächlich neueren Veröffentlichung (Testinstanzen fragen bewusst nie ab), Windows-Anmeldetest.

## Version 2.1.0 (23.10.2026): Home-Assistant-Seite, enthält den Freunde-Tipp aus 2.0.9 – veröffentlicht als v2.1.0

- Neue Seite Home Assistant (bis zu 4 Entitäten, REST-API, DPAPI-Token, Auswahlliste in den Einstellungen, Schalter nur mit Touch). Release-Build erfolgreich (Produktversion 2.1.0, 13258240 Bytes, SHA-256 `711F76DF3F8D5DF343E2C1FA6EE5944EB8D012EC7D776F35990D26C27ED5D53A`), **212 Kernprüfungen und 11 Monitor-Prüfungen bestanden**, dazu 49 Seitennavigations-Prüfungen. Neue Prüfungen: Entitäts-Syntax, Schalter-Bereiche (Schlösser/Rollläden ausgeschlossen), Adressprüfung, Wertformat, Kachel-Treffer inkl. skaliertem Display, Konfiguration (höchstens 4, Token verborgen), Zustandsauswertung.
- Gegen einen lokalen Testserver (Python, Port 18123, kein echtes Home Assistant): Abfrage, falsches Token (Fehlermeldung), unbekannte Entität, nicht erreichbar, Umlegen eines Schalters (genau ein Aufruf; Schloss und Sensor werden nie gesendet), Entitätsliste ohne Touch ohne Schalter/Schlösser. Die Seite im Browser (DE und EN) und das Displaybild (Hell/Dunkel, 1–4 Kacheln, Fehlerfall) von Hand geprüft. Testinstanz auf Port 18770, ohne Display; Bastis laufende App und Port 8765 blieben unberührt.
- **Nicht getestet:** Schalten per echtem Fingertipp auf dem Touchscreen und gegen ein echtes Home Assistant (nur die Trefferberechnung und der Serveraufruf einzeln).
## Version 2.0.9 (23.10.2026): Freunde-Tipp Steam → Discord → Xbox – veröffentlicht als v2.1.0

- Auf Wunsch des Nutzers feste Reihenfolge: Steam-Chat, sonst Discord-Profil, sonst Xbox-Profil im Browser; fehlende Apps führen zum nächsten Ziel. Release-Build erfolgreich (Produktversion 2.0.9, SHA-256 `60771E3FED6B69564B731E6C14D3B818CF0E807A7E1D0493303B3730802E592C`), **164 Kernprüfungen und 11 Monitor-Prüfungen bestanden**, darunter neue Prüfungen für Reihenfolge, Fallback bei fehlender Discord-/Steam-App, Xbox-Profil-URL und ungültige IDs. Die Startfunktion wird dabei ersetzt: keine echten Chats oder Profile geöffnet. Keine Installation, kein Test gegen Port 8765.
- Offen: physische Abnahme am Display (Tipp auf Freund mit nur Discord bzw. nur Xbox) in angemeldeten Clients.

## Version 2.0.8 (23.10.2026): BF6-Profil-Link, nur Steam-Absprung – veröffentlicht als v2.1.0

- Ursache der leeren Seite: Die Webseite kennt die Plattform `ea` nicht (Route bleibt leer); mit `pc` lädt z. B. P4p4F0x im Browser korrekt. Für `Schmieselbeehee` liefert die API bei `platform=ea` Daten, bei `pc` (das die Webseite verwendet) 404; die Webseite zeigt dort „Playername not found“. Der Name selbst wird nicht verändert (Konfiguration und API stimmen überein).
- Freunde-Tipp auf Wunsch des Nutzers: nur Steam-Chat, Discord/Xbox ohne Absprung. Release-Build erfolgreich, 158 Prüfungen und 11 Monitor-Prüfungen bestanden. Keine Installation, kein Test gegen Port 8765.
- Offen: Discord-Direktnachricht (der Bot kennt die DM-Kanal-ID zwischen Nutzer und Freund nicht) und Xbox-Chat (kein offizieller Deep-Link, Xbox-App nicht installiert).

## Version 2.0.7 (23.10.2026): Touch-Ziele und Spielstunden – lokal, noch nicht installiert/veröffentlicht

- Übernahme von Claudes begonnenem Branch `codex/2.0.7`, Basis `3ef0ef3` (veröffentlichtes 2.0.6). Die begonnenen Plattform-ID-Ergänzungen wurden vervollständigt; die `Account`-Struktur bleibt mit bisherigen Aggregat-Initialisierungen kompatibel.
- Release-Build mit MSVC/CMake/Ninja erfolgreich. Datei-Version `2.0.7.0`, Produktversion `2.0.7`, Größe 13.150.208 Bytes. SHA-256: `0A55662586E7C9C41CE9F944328CA3DCD5311D9CEF72A9AEA14EB82B11D50396`.
- **164 Kernprüfungen bestanden**, einschließlich 78 neuer Prüfungen für Chat-/Profilziele, URL-Kodierung, Online-Priorität, fehlende Desktop-Apps, Startfehler/Fallback, Listen-Sortierung, Unterseiten und Umlauf, leere Zeilen, Kachelgrenzen und vier Displaygrößen. Die Startfunktion wird in diesen Prüfungen ersetzt: keine echten Chats oder Browser-Tabs geöffnet und keine Nachrichten gesendet.
- **11 Monitor-Basisprüfungen bestanden.** Kein sichtbarer Monitor-/Touch-Injektionstest für 2.0.7 durchgeführt.
- **49 API-/Navigationsprüfungen gegen die echte EXE bestanden**, einschließlich API-Version 2.0.7, manueller/automatischer Seitenwechsel, leerer/einzelner Ansichten, Eingabeprüfung, Host-/Origin-/Fetch-Metadata-/X-FL-Schutz und unveränderter neutraler Testkonfiguration. Eigene Testinstanz auf Port 8766, `--no-lcd`, ohne Zugangsdaten. Das Testskript unterstützt nun eine erwartete Version als Argument und beendet seine eigene Instanz auch bei Fehlern der lokalen Verbindung.
- Der erste Kernprüfungsversuch konnte im umgeleiteten Sandbox-Temp-Ordner keine Testkonfiguration schreiben; Wiederholung mit eigenem Temp-Ordner unter `cpp/build` erfolgreich. Der erste API-Versuch wurde durch die Netzwerk-Sandbox (Windows 10013) blockiert; Wiederholung mit erweiterter Ausführungsberechtigung erfolgreich. Beides waren Testumgebungsprobleme, keine bestandenen Tests.
- Deutsche/englische Beispielbilder in Querformat (480 × 320) und Hochformat (320 × 480) visuell geprüft: Spielstunden, K/D und Siegquote sichtbar. Überlappung von Titel und Spielerzahl in schmalen Kopfzeilen behoben. Weitere Bilder für 1920 × 720 und 637 × 720 erzeugt; keine physische Anzeigeabnahme daraus abgeleitet.
- **88 lokale Markdown-Linkziele und 10 eingebettete JavaScript-Blöcke geprüft**; neue Anleitungstexte in Vorlagen und HTML-Ressourcen stimmen überein. README/Schnellstart, Referenz, beide Haupt-Changelogs und C++-Doku gemeinsam aktualisiert. Download bleibt v2.0.6, 2.0.7 ist ausdrücklich lokal gekennzeichnet.
- Auswahlziele werden zusammen mit dem gezeichneten Datenstand erzeugt, einschließlich Sortierung und Unterseiten. Eine spätere Datenaktualisierung ändert das Ziel einer bereits angezeigten Zeile nicht. Tippen bei deaktivierter Touch-Steuerung öffnet kein Ziel; Wischen bleibt im bisherigen Gestenpfad.
- Vom Nutzer bestätigte Plattformvariante: Steam öffnet den Chat direkt, Discord/Xbox öffnen das Profil mit dem Weg zu „Nachricht“. Online erreichbare Konten zuerst, innerhalb beider Gruppen Discord → Steam → Xbox; fehlende Apps oder Startfehler führen zum nächsten Ziel. Separates Fenster/Tab bestimmt die Plattform-App bzw. der Browser.
- **Offen:** physische Touch-Abnahme der neuen Ziele und Öffnen in angemeldeten Discord-/Steam-/Xbox-Clients, Anmeldung, Chatberechtigungen und deren Fensterverhalten. Protokollzuordnung und Startübergabe sind kein Nachweis für einen erfolgreich geöffneten Chat. Installer, echte Installation, Autostart, COM-Port und laufende produktive App wurden für diesen Stand nicht verändert oder getestet. Installation und Veröffentlichung benötigen wie vereinbart die abschließende Nutzerfreigabe.

Prüfungen wiederholen (Entwicklerkonsole, eigener beschreibbarer Temp-Ordner; API-Test benötigt lokalen Netzwerkzugriff):

```bat
build\fl-tests.exe
build\fl-monitor-tests.exe
python tests/test_page_navigation.py build/FuchsLabs-USB-Display.exe 2.0.7
build\fl-tests.exe render build/preview-2.0.7-de de
build\fl-tests.exe render build/preview-2.0.7-en en
```

## Version 2.0.6 (23.10.2026): Blogbeiträge per Touch, Einstellungsfenster, Lizenzdateien (enthält 2.0.5: Touch, Taskleiste)

- Basis: veröffentlichter Stand 2.0.4 plus Lizenzdateien (main nach PR #12/#13).
- Release-Build erfolgreich; Datei-/Produktversion 2.0.6.0 / 2.0.6. 86 Kernprüfungen und 11 Monitor-Basisprüfungen bestanden; sichtbarer Monitor-/Touchtest mit injizierten Windows-Berührungen: 56 bestanden.
- Isolierte Instanz (eigene Instanzkennung, Port 18767, neutrale Konfiguration, Ausgabe auf einem Nebenmonitor): Taskleiste dieses Monitors wird ausgeblendet und nach dem Beenden wieder gezeigt; injiziertes Tippen auf einen Blogbeitrag öffnete den Artikel im Standardbrowser, Tippen außerhalb blätterte; beides im Protokoll vermerkt.
- Einstellungsfenster geöffnet, dann „Speichern & anwenden“ per API ausgelöst: Die neu gestartete Instanz öffnete das Fenster wieder.
- Touch (Stand 2.0.5: Wischen, Taskleiste) laut Tester auf echter Touch-Hardware einwandfrei. Blog-Tippen und Fenster-Fix nur simuliert bzw. per API geprüft. Veröffentlicht als v2.0.6.

## Version 2.0.4: Ansichten in der Übersicht durchblättern

- Basis: veröffentlichter Quellstand 2.0.3 (Commit 9e32bd3); separate 2.5-Plugin-Änderungen sind nicht enthalten.
- Release-Build erfolgreich; Datei-/Produktversion 2.0.4.0 / 2.0.4.
- 86 bestehende Kernprüfungen und 11 Monitor-Basisprüfungen bestanden. Keine sichtbaren Monitor-/Touchtests für diesen Stand durchgeführt.
- 49 neue Prüfungen gegen die echte EXE in isolierten Instanzen auf Port 8766 mit neutralen Konfigurationen, eigener Instanzkennung und --no-lcd bestanden: vor/zurück, Umlauf in beide Richtungen, keine/eine Ansicht, automatische Fortsetzung und neue Anzeigedauer nach manuellem Wechsel.
- Eingabeprüfung (einschließlich ungültigem JSON, falschen Typen und übergroßen Zahlen) sowie Host-/Origin-/Fetch-Metadata-/X-FL-Schutz geprüft. Unveränderter Prozess und unveränderte Konfigurationsdatei nach Navigation bestätigt.
- Browserprüfung DE/EN bestanden: neue Knöpfe, Positionsanzeige und tatsächlicher Seitenwechsel geprüft; Sprachänderung nur im Formular ausprobiert und verworfen.
- JavaScript-Syntax der Oberfläche und der eingebetteten Anleitungen sowie lokale Links der geänderten Dokumentation geprüft.
- Unterseiten und breite Kachelgruppen verwenden den vorhandenen Touch-Wechselpfad; kein zusätzlicher Laufzeittest mit gefüllten Listen oder breitem Display durchgeführt.
- Bei den isolierten Prüfungen keine Installation, Veröffentlichung, Autostartänderung oder physische Hardwareprüfung durchgeführt.

### Anschließende Installation von 2.0.4

Nach ausdrücklicher Nutzerfreigabe am 22.10.2026 im normalen Windows-Benutzerkontext außerhalb der Paketumleitung installiert. Bestehende EXE und Konfiguration zuvor lokal gesichert. Installer-Exitcode 0; installierte Datei-Version 2.0.4.0 und API-Version 2.0.4. SHA-256 stimmt mit dem geprüften Build überein: `EF7186C5ABF810DD1CC4D9F88E49883FC755AA10C65665D98FA61A39C54A177F`. Einstellungsdatei bytegleich erhalten, serielle Ausgabe und Autostart erhalten, genau ein Prozess aus dem regulären Installationsordner, `display_ok=true`. Neue Ansichten-Schnittstelle und Bedienelemente erreichbar. Unabhängige Abschlussprüfung erneut im normalen Windows-Kontext bestanden. Kein neuer Windows-Anmeldetest und keine visuelle/physische Touch-Abnahme durchgeführt. Am 23.10.2026 unverändert (gleicher Hash) als v2.0.4 auf GitHub veröffentlicht.

## Version 2.0.3: HDMI-Vollbild und Touch

- Basis: aktueller C++-Quellstand 2.0.2 im Veröffentlichungs-Repository. Die älteren Änderungen an Discord, gzip und Anleitung bleiben enthalten.
- Release-Build erfolgreich; Datei- und Produktversion `2.0.3.0` / `2.0.3`.
- Bestehende Kernprüfungen: 86 bestanden. Zusätzliche Gesten-/Touch-Metadatenprüfungen: 11 bestanden.
- Kontrollierter Windows-Test auf einem Nebenmonitor: 56 Prüfungen einschließlich der 11 Basisprüfungen bestanden. Geprüft wurden vollständige Monitorgeometrie, Abdeckung an allen vier Bildschirmrändern, Topmost/NoActivate/ToolWindow, Erhalt des Tastaturfokus, native injizierte Touch-Eingaben, genau ein Seitenwechsel je Tipp/Wischgeste, Abbruch, deaktivierter Touch, Mausbedienung und Ausblenden/Wiederherstellen. Alle eigenen Testfenster danach geschlossen.
- Isolierte Instanz auf Port 8766 mit neutraler Konfiguration und `--no-lcd`: API meldet 2.0.3; neue Touch-Metadaten verfügbar. Deutsche Einstellungsseite im Browser visuell geprüft, Touch-Zuordnung und Aktualisieren-Button funktionieren; ohne Touchgerät ist der Einrichtungsbutton deaktiviert.
- Neuer Einrichtungs-Endpunkt: GET 404, POST ohne Schutzheader 403, fremder Origin 403, Cross-Site 403, kein Touchgerät 400. Es wurde kein Windows-Assistent und keine echte Gerätezuordnung ausgelöst.
- Auf diesem PC ist kein Touchscreen angeschlossen. Echte Touch-Hardware, die Gerätezuordnung im Windows-Assistenten sowie dessen vollständiger Ausblenden/Rückkehr-Ablauf bleiben als physische Abnahme offen. Windows kann die physische Identität eines separat per USB angeschlossenen Touchgeräts nicht allein aus dem HDMI-Bildsignal ableiten; die Zuordnung erfolgt im Assistenten durch einen Fingertipp auf den vorgesehenen Bildschirm.
- Produktive App während der Tests unverändert: API 2.0.2, serielle Ausgabe, `display_ok=true`. Keine Installation, kein Autostart-/Registry-Eingriff und keine Veröffentlichung vorgenommen.

### Anschließende Installation von 2.0.3

Nach ausdrücklicher Nutzerfreigabe am 22.10.2026 über einen normalen Windows-Prozess außerhalb der Codex-Paketumleitung installiert. Unabhängige Abschlussprüfung: Datei-Version `2.0.3.0`, API-Version `2.0.3`, genau ein App-Prozess aus dem regulären LocalAppData-Installationsordner, serielle Ausgabe und `display_ok=true`. Installierter SHA-256 stimmt mit dem getesteten Build überein: `ADED00C0219623D75E4F15C485392C2CBCB0D9D0A443EB0E5A79BBABB914DD84`. Bestehende Einstellungsdatei vorhanden; HKCU-Run-Eintrag zeigt auf die installierte EXE mit `--autostart`. Ein neuer Windows-Anmeldetest und die physische HDMI-/Touch-Abnahme wurden nicht durchgeführt.

## Version 2.0.0

- Release-Build: 86 Prüfungen bestanden.
- Eigenes Win32/WebView2-Fenster sichtbar geöffnet und korrekt gerendert.
- Version 1.x und 2.0 nacheinander an COM7 mit derselben Konfiguration betrieben: beide display_ok=true. Nutzer bestätigte Ausrichtung, Farben, Lesbarkeit und Seitenwechsel für beide.
- Integrierter Installer: Einstellungen übernommen, PC auf 2.0 umgestellt, Autostart aktualisiert. Windows-Neustart mit automatisch erscheinendem Tray-Symbol vom Nutzer am 22.10.2026 bestätigt.
- Installierte und getestete EXE: SHA-256 1068EA18C4BA6C2B7ACDD464A33C3FE499A885734CCA84FC5F8CEFFB9A160998.
- WebView2 Evergreen Runtime erforderlich. EXE nicht signiert.
- Veröffentlichungskontrolle: alle 10 eingebetteten JPEG-Bilder der deutschen und alle 10 der englischen Anleitung einzeln sichtbar geprüft. Plattformbilder zeigen explizite Beispielkonten (FuchsKumpel, Beispielkonto/sample account); Discord zeigt Musternamen Lena/Max/Tom, Battlefield zeigt FuchsKumpel/Lena_Sniper/MaxPower99/TomTheTank/Mia2015. Kein echter Kontoschlüssel oder Bot-Token sichtbar; Felder sind maskiert. Guide-Ressourcen sind die vorliegenden anonymisierten Anleitungen und nicht die früher ausgeschlossenen Originalbilder.

## Windows-Neustarttest und Korrektur

Der erste reale Neustarttest scheiterte: Die App erschien nicht im Infobereich und das letzte Displaybild blieb stehen. Eine rein lesende Diagnose über einen Windows-Prozess außerhalb des Codex-Paketkontexts zeigte `ExeExists=false` für das Installationsziel und noch den alten Run-Eintrag. Die vorherigen Prüfungen waren durch die Paketumleitung des Hosts beeinflusst.

Korrektur: Einstellungen aus dem umgeleiteten Ordner übernommen und Installer aus einem normalen Windows-Prozess erneut ausgeführt. Dort bestätigt: InstallerExit=0, ExeExists=true, EXE-Hash identisch zum Release, Run-Eintrag auf die reguläre 2.0-EXE mit --autostart, API display_ok=true und autostart=true. Der Nutzer bestätigte am 22.10.2026 den erfolgreichen zweiten Windows-Neustart: Die App startete automatisch mit Tray-Symbol.
