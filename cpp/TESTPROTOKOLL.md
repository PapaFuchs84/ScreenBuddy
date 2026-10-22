# Testprotokoll vom 22.10.2026

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
