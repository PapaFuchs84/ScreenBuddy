# Testprotokoll vom 22.10.2026

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
