# Hardware HUD – lokale Testversion v3

[English](HARDWARE-HUD-TEST.en.md)

Lokaler Entwicklungsstand vom 14.11.2026, auf Basis von ScreenBuddy 2.11.0. Keine Veröffentlichung auf GitHub oder im Microsoft Store; kein neuer öffentlicher Versionsstand. Eine lokale Entwicklungs-EXE kann unsigniert sein.

## Änderungen in v3

Das lokale Paket heißt `ScreenBuddy-Hardware-HUD-Test-v3.zip`; die Anzeige trägt die Kennung `TEST3`. Der EXE-Dateiname bleibt `ScreenBuddy-Hardware-HUD-Test.exe`, damit die getrennten Start-/Stop-Helfer weiter funktionieren. Das v3-Paket in einen eigenen Ordner entpacken und den Starter aus diesem Ordner verwenden.

Hardwarebezeichnungen werden anhand ihrer gemessenen Textbreite innerhalb begrenzter Flächen umgebrochen. Die 1200 × 360 Pixel große Vorschau ist das Referenzformat. Ringe und Lüfter bewegen sich deutlicher, Signalimpulse laufen entlang der Leitungen und Seitenübergänge sind animiert. Diese Bewegung bleibt dekorativ und ersetzt keine Sensorwerte.

Die Animationsrate wird jetzt global unter **Anzeige → Animationen** von **5 bis 60** eingestellt (`display.animation_fps`), Standard **30**. Sie gilt bei sichtbarer Bewegung für Monitor und Companion: Hardware HUD, aktive KI-Zustände und Zeilenübergänge, laufende Spotify-Wiedergabe samt Fortschritt/Pegel sowie sichtbare native, eigene, Game-Intro- und Plugin-Übergänge. Statische Ansichten behalten ihren normalen Quellen-/Anzeigetakt und die normale Companion-Transportgrenze von 5 Bildern/s. Der alte Schlüssel `system.hardware_hud_fps` dient nur noch der Migration; ein ausdrücklich gesetztes `display.animation_fps` hat Vorrang.

Der Wert ist ein Ziel und Ressourcenhinweis, keine garantierte Lieferbildrate. Letztes Bild, Versionsvergleich, synchroner Rückstau und begrenztes Beenden bleiben erhalten. Android-App, Drahtformat und Sensorabfragetakt ändern sich nicht. Die globale App-/Geräteabnahme und die Prüfung gemischter schneller/statischer Ausgaben stehen noch aus. Vor der zusätzlichen Trennung konnten statische Companion-Seiten unnötig mit der schnellen Hauptanzeige neu gezeichnet werden; der neue Schutz dagegen ist noch zu prüfen.

Die alte 5-Bilder/s-Grenze ist keine Android-/Protokollvorgabe. Im früheren ausschließlich auf HUD bezogenen Transportkandidaten ergaben Ziele von 20/30/60 auf dem echten Redmi 16,06/21,31/31,38 UI-Neuzeichnungen/s. 30 bietet den gewählten Kompromiss; 60 war am flüssigsten gemessen, benötigte aber rund 26,2 statt 17,5 Mbit/s TLS-Verkehr sowie mehr CPU. Das belegt weder erreichte 60 Bilder/s noch eine Hardwareobergrenze. [Vollständige Messung und Grenzen](HARDWARE-HUD-FPS-ANALYSE.md).

## Bedienung

Für den vorbereiteten lokalen Test `local-test/Start-Hardware-HUD.cmd` doppelt anklicken. Der Starter verwendet die frische `local-test/config.toml`, einen eigenen Datenordner und die getrennte Instanz `HardwareHudTest`. Die HUD-Ansicht ist bereits aktiviert und erscheint als 1200 × 360 Pixel großes Vorschaufenster. Eine Installation ist nicht erforderlich.

Zum Beenden `local-test/Stop-Hardware-HUD.cmd` doppelt anklicken. Der Helfer prüft vor der lokalen Beenden-Anfrage, dass der Listener auf Port 18769 genau zu `ScreenBuddy-Hardware-HUD-Test.exe` in diesem Testordner gehört. Bei abweichendem Prozesspfad wird nichts beendet. Falls die verwaltete Testumgebung keine Portzuordnung liefert, beendet der Helfer ausschließlich Prozesse mit dem exakt passenden Test-EXE-Pfad direkt; dieser Rückfall ist kein geordnetes Beenden über die App. Alternativ im Tray-Menü dieser Testinstanz **Beenden** wählen.

Die folgende Einstellung gilt für die manuelle Einrichtung mit dieser Test-EXE:

In den Einstellungen unter **Seiten → System** die Systemseite und **Hardware HUD** aktivieren und die Einstellungen speichern. Die Systemansicht zeigt dann eine dunkle Cyan-/Magenta-Anzeige mit eigenen Vektorgrafiken. Übersicht, GPU und CPU wechseln standardmäßig alle acht Sekunden. Andere aktivierte Seiten können durch den normalen Seitenwechsel dazwischen erscheinen; für eine durchgehende HUD-Anzeige nur die Systemseite auswählen.

Die Ansicht passt sich breiten, normalen und hochkant ausgerichteten Ausgabegrößen an. Die normale Systemansicht lässt sich durch Ausschalten von **Hardware HUD** wiederherstellen. Das HUD verwendet ein eigenes Farbschema.

Der vorbereitete Starter begrenzt die Ausgabe bewusst auf das Vorschaufenster. Für einen echten Monitor muss die passende Ausgabe in einer eigenen Testkonfiguration eingestellt und `FUCHSLABS_TEST_RECT` im Starter entfernt werden. Die bisherige ScreenBuddy-Konfiguration wird dafür nicht kopiert.

## Messwerte

- CPU-Auslastung und Takt kommen aus Windows. CPU-Temperatur, CPU-Lüfterdrehzahl und CPU-Package-Leistungsaufnahme benötigen laufendes HWiNFO mit aktivierter Shared-Memory-Ausgabe und passende Sensoren.
- CPU-Watt werden nur aus einem Power-Sensor in einer CPU-Sensorgruppe mit Einheit `W` und bekanntem originalem Package-/PPT-Namen übernommen. Individuell umbenannte HWiNFO-Sensoren funktionieren über den ursprünglichen englischen Namen. Unterstützte Originalnamen: `CPU Package Power`, `CPU Package Power (SMU)`, `CPU Package`, `CPU PPT`, `CPU Power`. Unbekannte Sensorbezeichnungen bleiben unbesetzt.
- NVIDIA-GPU-Daten kommen aus der vorhandenen NVML-Anbindung. Nicht jeder Treiber bzw. jedes Modell liefert alle Werte. Andere GPUs können weniger Werte anbieten.
- Fehlende oder ungültige Messwerte bleiben `--`; sie werden nicht durch erfundene Werte ersetzt. Die Vektorgrafiken sind schematisch. Bewegte Grafiken sind dekorativ und keine unabhängige Messung von Gehäuselüftern.
- Hardwarewerte werden im vorhandenen Sensortakt aktualisiert; die Grafikanimation läuft davon getrennt. Zusätzliche individuell auswählbare Gehäuselüfter sind nicht Teil dieser Testversion.

## Anzeige und Prüfgrenzen

**Aktueller globaler Stand:** App-/Geräteprüfungen für alle sichtbaren Animationen, gemischte statische Ausgaben und die neue native USB-Ausgabe stehen noch aus. Die folgenden bestandenen HUD-Prüfungen beziehen sich auf ältere Artefakte und belegen keine globale Abnahme.

Beim echten nativen Rev-A-USB-Display (480 × 320) wurden bisher nur Controller-Schreibvorgänge angenommen, ohne optische Rücklesung: Quellenziel 5 ergab etwa 1,65 Ausgabeaufrufe/s, 30 und 60 jeweils etwa 2,27/s; mittlere USB-Ausgabezeit 428–540 ms. Eine Teiltransferrate lag bei ungefähr 170 kB/s. Volle 30/60 Bilder/s sind auf diesem vollständigen Bildpfad nicht belegt. Die zusätzliche feste HUD-Sekundenpause wird entfernt; native Animationen richten sich nach der tatsächlichen Übertragungszeit mit kurzer Leerlaufpause. Die echten globalen App-Fälle und Plugin-Übergangsbudgets sind noch zu prüfen. Während des isolierten Rohtransfer-Tests wurde die normale Windows-Instanz kurz pausiert und danach mit bestätigtem Displaystatus wiederhergestellt.

Spotify-/Game-/andere Seitenbeispiele aus synthetischen Render-/Transportprüfungen belegen keine echte Spotify-Konto-/Steuerungs- oder Spielabnahme.

Ein als Windows-Monitor erkanntes HDMI-/USB-C-Display verwendet den bestehenden Monitorausgang. Unterstützte serielle Displays und Companion-Ausgaben verwenden ihre vorhandenen Ausgabepfade. Das Modell und Protokoll des Displays aus dem Referenzvideo sind unbekannt; eine Unterstützung dieses konkreten Geräts ist dadurch nicht nachgewiesen.

Der frühere HUD-only-v3-Build mit Standard 30 bestand 88 HUD- und 3556 Regressionsprüfungen. Die 434 Companion-Prüfungen desselben Transportcodes bestanden; enthalten sind unveränderte Standardseiten mit 5 Bildern/s sowie begrenztes Beenden bei nicht lesendem Client (0,44 Sekunden im Test). Der abschließende Test dieses exakten Builds auf dem echten Redmi bestand: 36,41 Sekunden im Querformat 1520 × 720 und 35,80 Sekunden im Hochformat 720 × 1520, je zwölf Geräteaufnahmen sowie ein zusätzliches 34-Sekunden-Gerätevideo. Alle drei Seiten zeigten Live-Werte, automatische Wechsel und sichtbare Bewegung ohne beobachtete Überlappungen; die Verbindung blieb bestehen. Die Testkonfiguration ließ `hardware_hud_fps` bewusst weg, die API bestätigte den kompilierten Standard 30. Das ist eine Funktionsabnahme, keine neue Bildratenmessung. Nach dem Test wurden Host und Weiterleitungen beendet, Einstellungen wiederhergestellt und die normale Companion-App mit erhaltener Kopplung geöffnet.

Lokaler Gerätebericht: [finaler Test](redmi-validation/test3-final-30fps-smoke/FINAL-DEVICE-RESULT.md).

Der folgende ältere Prüfstand gehört zur bisherigen Testversion v2 und ist keine aktuelle v3-Monitorbildratenmessung. Die lokale Windows-Testversion v2 wurde erfolgreich gebaut. 79 gezielte HUD-Prüfungen und 3556 bestehende Regressionsprüfungen bestanden ohne Fehler; der KI-Anteil umfasst 300 erfolgreiche Prüfungen. Die HUD-Prüfungen decken unter anderem 1200 × 360, 1520 × 720 und 720 × 1520 Pixel ab. Die deutsche Vorschau wurde für alle drei HUD-Seiten bei 1200 × 360 sowie im Hoch-/Querformat ohne sichtbare Textkollisionen geprüft.

Im tatsächlichen isolierten 1200-×-360-Monitorbetrieb wurden 83 Renderbilder über 5,113 Sekunden erfasst, entsprechend 16,04 Bildern/s. Alle zwölf zusätzlich abgetasteten Vorschaubilder unterschieden sich. Das ist ein kurzer Lauf auf dem Test-PC und kein Nachweis dauerhaft erreichter 20 Bilder/s. Geordnetes Beenden über die lokale API und anschließendes Prozessende wurden bestätigt.

Der Test auf dem echten Redmi 8 mit Android 14 bestand im Vordergrund bei eingeschaltetem Bildschirm: alle drei Seiten in 1520 × 720 Pixel Querformat (neun Aufnahmen über 31,7 Sekunden und ein 26-Sekunden-Gerätevideo) sowie 720 × 1520 Pixel Hochformat (acht Aufnahmen über ungefähr 28 Sekunden). Live-PC-Werte, automatische Acht-Sekunden-Seitenwechsel und bewegte Ringe, Lüfter und Signalimpulse waren sichtbar; es wurden keine Überlappungen beobachtet. Fehlende GPU-Watt blieben `--`.

Verwendet wurden eine getrennte Debug-Test-App und USB mit ADB-Reverse sowie einer temporären lokalen Weiterleitung. Die normale Host-Zugriffsregel wurde nicht geändert. Das ist kein bestandener LAN-Test. Beim ursprünglichen v2-Test wurde keine Telefon-Bildrate gemessen. Ein erneuter Redmi-Test ohne Bildschirmaufnahme erfasste 49 UI-Neuzeichnungen in 12,093 Sekunden, entsprechend 4,05/s; dabei wurde die bisherige globale 5-Bilder/s-Transportgrenze festgestellt. Das belegt keine erreichten 10 Bilder/s. Hintergrund-, Neustart- und Batterieverhalten wurden nicht abgenommen. Anschließend wurden Test-App, Testhost und Weiterleitungen beendet und die normale Companion-App wieder geöffnet; deren Kopplung und Daten blieben erhalten.

Eine vollständige reale Abnahme aller Displaygrößen, Sensoren und verbundenen Geräte steht aus. Persönliche Konfigurationen, Zugangsdaten und Logs gehören nicht in das Testpaket. Der Paket-Prüfstand steht zusätzlich in `local-test/PRUEFSTATUS.txt`; der lokale Gerätebericht ist [redmi-validation/REDMI-TESTRESULT.md](redmi-validation/REDMI-TESTRESULT.md).
