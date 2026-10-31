# ScreenBuddy · Prüfstände früherer Versionen (historisch)

Aus dem README ausgelagert. Der **aktuelle** Prüfstand steht in den [Release-Notizen 2.7](RELEASE-NOTES-2.7.md), das Testprotokoll in [`cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md) (Stand 2.6.0, 30.10.2026; ohne Eintrag für 2.7.0). Die Einträge sind unverändert und werden nicht auf neuere Versionen umgeschrieben. Fachdoku, nur Deutsch.

## Prüfstand 2.6.0

Prüfgrenzen und Testzahlen stehen in den [Release-Notizen 2.6.0](RELEASE-NOTES-2.6.md#prüfgrenzen). Die Store-Installation in einer isolierten Instanz ist dort gesondert ausgewiesen.

## Prüfstand 2.3.0

Release-Build: 236 Kern-, 11 Monitor- und 335 Companion-/TLS-Prüfungen bestanden. Außerdem 39 isolierte App-Integrationsprüfungen, 2 Installer-Sicherungstests, 22 Browserprüfungen (DE/EN) und 20 Android-Protokolltests. Auf dem Redmi 8 wurden native Auflösung, Hoch-/Querformat, automatischer Seitenwechsel, Tippen, Wischen und Wiederverbindung bestätigt; der Nutzer bestätigte zusätzlich scharfes Bild und Fingerbedienung. **Offen:** physischer USB-Ausgang und Redmi gemeinsam in derselben neuen PC-Instanz, WLAN, Android-Start beim Booten und mehrere reale Geräte. Siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md).

## Prüfstand 2.2.0

Release-Build, 225 Kernprüfungen (neu: Trefferflächen der Systemseite, Versionsvergleich und Auswertung der GitHub-Antwort) und 11 Monitor-Basisprüfungen bestanden; die Trefferflächen stimmen mit der gerenderten Systemseite überein (Vorschaubild). **Noch offen:** Tippen auf echter Touch-Hardware (Task-Manager, „Programme und Features“, Speedtest), die Sprechblase und die Anzeige in den Einstellungen mit einer tatsächlich neueren Veröffentlichung sowie ein Windows-Anmeldetest für 2.2.0. Siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md).

## Prüfstand 2.0.3

Release-Build, 86 Kernprüfungen und 56 Monitor-/Touchprüfungen mit injizierten Windows-Eingaben bestanden (die 56 enthalten 11 Basisprüfungen). Installierte 2.0.3: tatsächliche Datei-Version, Hash, API-Version, Autostart-Eintrag und `display_ok=true` außerhalb der Paketumleitung geprüft. Echte Touch-Hardware, der vollständige Windows-Zuordnungsassistent und ein neuer Windows-Anmeldetest für 2.0.3 bleiben offen. Siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md).

Die frühere Abnahme von 2.0 umfasste den Gegenvergleich mit 1.x an COM7 und die vom Nutzer bestätigte physische Anzeige sowie einen erfolgreichen Windows-Neustart mit Tray-Symbol.

Der erste reale Neustarttest scheiterte an der Paketumleitung des Installationshosts. Installation und tatsächlicher Windows-Autostarteintrag wurden danach außerhalb dieses Kontexts korrigiert; der Nutzer bestätigte anschließend den erfolgreichen Windows-Neustart mit Tray-Symbol.
