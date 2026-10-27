# ScreenBuddy 2.5 – Hardware-Abnahme

**27.10.2026, separate Redmi-8-Abnahme:** Signierte Companion-Vorschau 0.2.1-preview + isoliertes Windows 2.5.0 bestanden Kopplung über USB-Tethering, native Quer-/Hochformat-Ausgabe, echte Fingerbedienung, automatischen 5-Sekunden-Wechsel und gespeicherte Wiederverbindung nach Server-Neustart. Nutzerbestätigung und API-Nachweise stehen im [Testprotokoll](../cpp/TESTPROTOKOLL.md). Ein lokaler TLS-Testclient prüfte außerdem die Ablehnung einer falschen PIN. Daraus folgt keine Freigabe des physischen USB-Displays oder des Windows-Zielsystem-Upgrades.

**Status:** Abnahme des finalen Kandidaten offen. Der letzte hier protokollierte serielle Demo-Lauf ist **FEHLGESCHLAGEN**; am 26.10.2026 meldete der Nutzer den Bildfehler als behoben. Keine RC1- oder Release-Freigabe aus diesem Protokoll.

Dieses Verfahren prüft reale Ausgabe und Bedienung auf einem Windows-PC mit USB-Display. Je Lauf Datum, Gerät, Build/Commit und Prüfer festhalten. Build- und automatisierte Tests ersetzen keine physische Abnahme.

## 1. Voraussetzungen und Schutz

- Windows-PC, Kandidaten-Build, USB-Display und Kabel. Für Companion: Pixel-Telefon mit Companion und Netzwerkzugriff zum PC.
- Für diesen Lauf: `COM7`, USB `VID_1A86`, `PID_5722`; Testkonfiguration `C:\Users\admin\FuchsLabs\screenbuddy-hardware-acceptance\2026-10-25\test\config.toml`.
- Vor Installation-, Konfigurations-, Companion- oder Update-Änderungen rückspielbare Backups verifizieren. Backups dieses Laufs liegen privat unter `C:\Users\admin\FuchsLabs\screenbuddy-hardware-acceptance\2026-10-25\private-backup`; Inhalte nicht ins Repository kopieren.
- Vor jedem seriellen Test COM-Port-Besitzer, VID/PID und Gerät prüfen; nur ein Prozess darf den Port belegen. Mit dedizierter Testkonfiguration arbeiten, keine produktiven Geräte/Tokens/URLs verwenden. Keine parallelen App-Instanzen.
- Keine Zugangsdaten, Pairing-PINs, Tokens, privaten URLs, Logs, Screenshots oder Fotos in Git, Issues, Wiki oder Releases aufnehmen. Beweismaterial bleibt privat und wird vor Weitergabe redigiert.

## 2. Vorprüfung und Baseline

1. Version, Commit/Build-Kennung, EXE-Pfad und SHA-256 erfassen.
2. Installierte Version und laufenden Prozess separat prüfen: vollständiger EXE-Pfad, PID und angezeigte Version. Dateiname oder Installerstatus allein reichen nicht.
3. COM-Port, VID/PID, Geräteverbindung und Konfigurationspfad prüfen; sicherstellen, dass die alte App den Port freigegeben hat.
4. Gerät vor dem Test mindestens 60 Sekunden beobachten: statischer und wechselnder Inhalt, Helligkeit, Stabilität, Streifen/Artefakte, Flackern, Aussetzer, Farben und Orientierung. Fotos nur privat.
5. Baseline besteht nur bei stabilem Gerät ohne neue Artefakte. Bei bereits vorhandenem Fehler Lauf als „nicht auswertbar“ markieren.

## 3. Serielle Demo und echte App

### 3.1 Demo-Smoketest

1. Sicherstellen, dass kein anderer Prozess COM7 besitzt und der Testbuild die separate Testkonfiguration verwendet.
2. Demo mit vorgesehener `--demo`-Option starten; Zeit, Kommando, Buildkennung, Port und Exitstatus festhalten.
3. Physisch Start/Ende, Frames, Reihenfolge/Farben, Vollständigkeit, Flackern, Streifen, Aussetzer und Erholung danach beobachten.
4. Log lokal auf Fehler prüfen; Exitcode und gesendete Framezahl erfassen. Private Logs nicht veröffentlichen.
5. Prozess beenden und Portfreigabe prüfen. Echte App separat testen; Demo ist kein App-Nachweis.

### 3.2 Echte App

1. Vorzustand sichern, echte App mit Testkonfiguration starten und erwartete Inhalte am Gerät prüfen.
2. Mindestens 10 Minuten betreiben: statischer Inhalt mindestens 2 Minuten, danach mehrere Aktualisierungen. Vollständige Frames, Farben/Orientierung, Latenz, Flackern, Streifen, Aussetzer, Neustarts und CPU-/Speicheranstieg prüfen.
3. Regulär schließen und erneut öffnen. Ausgabe muss ohne manuelle Portfreigabe oder PC-Neustart wiederkehren.
4. Zeitstempel und Schweregrad erfassen. Visueller Fehler bedeutet Fail, auch bei unauffälligem Exitcode/Log.

### Aktueller Befund

**Nutzer-Update vom 26.10.2026:** Der Bildfehler ist laut Nutzer behoben; eine weitere Ursachenanalyse ist nicht gewünscht. Build-Hash und Sichtabnahme des finalen Kandidaten wurden mit dieser Meldung nicht übermittelt. Der folgende fehlgeschlagene Lauf bleibt als historischer Prüfbeleg erhalten.

**Chronologie der Sichtkontrollen am 25.10.2026:** Ein früherer, separat protokollierter `2.5.0-dev.2`-Kandidat (`ScreenBuddy-2.5.0-dev.2.exe`, SHA-256 `fc5e8afe814bfd9f607bd3574fb3ab6bb6d9a5213bb5b46150579f48a891d2e6`, Stand nach PR #16) lief etwa vier Minuten mit einer Kopie der Nutzereinstellungen auf COM7. Laut Handabnahme zeigte er auf beiden Displays ein gutes Bild; Touch funktionierte. Display, Discord, RSS und Companion-Verbindung waren im Echtlauf aktiv. Das war eine positive, zeitlich begrenzte Sichtkontrolle und keine vollständige Upgrade- oder Dauerabnahme. Der **spätere** COM7-Demolauf mit einem weiterentwickelten lokalen Build schlug wie unten beschrieben fehl. Die frühere positive Beobachtung bleibt dokumentiert; der jüngste Fehler ist für den nächsten Kandidaten offen. Das frühere Protokoll liegt außerhalb des Repositorys unter `release-2.5.0-dev.2/TESTPROTOKOLL-Handabnahme-2026-10-25.md`.

ScreenBuddy `2.5.0-dev.2` auf COM7 (`VID_1A86`, `PID_5722`) sendete im `--demo`-Lauf fünf Frames; Exitcode 0, null Logfehler. Die sichtbare Ausgabe war beschädigt: blaue Streifen und Flackern, auch beim statischen Frame. Laut Nutzer war das Bild vor dem Test normal. Auch die alte App zeigte danach ein gestörtes Bild; ein kontrollierter Neustart der alten App half nicht. Der serielle Sichttest ist **FEHLGESCHLAGEN** und bleibt als historischer Befund erhalten. Für den aktuellen Kandidaten fehlt die Sichtabnahme mit seinem Build-Hash. Anschließend wurde bei gestoppter App das USB-Display für 10 Sekunden abgezogen und wieder angesteckt; nach Neustart der alten App bestätigte der Nutzer: „Bild wieder normal“. COM7 und der Pfad der alten EXE wurden verifiziert. Die Hashes der vorherigen `config.toml` und `companion.json` sind unverändert. Das Wiederherstellen des Bildes nach dem Power-Cycle beseitigt den fehlgeschlagenen 2.5-Demotest nicht und erklärt die Ursache nicht. Das Foto bleibt privat und gehört nicht ins Repository.

### Einordnung und Abnahme des finalen Kandidaten

Der serielle Bildpfad (`cpp/src/serial_display.cpp`) ist seit 2.3.0 bis auf die VID/PID-Erkennung unverändert: RGB565-Umwandlung, 3.840-Byte-Blöcke, 115.200 Baud, Flow-Control und Reset-Ablauf sind gleich. Der Schreib-Timeout von 5 Sekunden gilt für **jeden** `WriteFile`-Aufruf, nicht für ein ganzes Vollbild. Ein 3.840-Byte-Block benötigt bei 115.200 Baud rechnerisch etwa 0,33 Sekunden. Deshalb erklärt die Rechnung „307.200 Byte brauchen 26,7 Sekunden, also läuft der 5-Sekunden-Timeout ab“ den Befund nicht. Ebenso beweisen erfolgreiche Schreibaufrufe und Exitcode 0 nicht, dass das Panel alle Pixel korrekt angezeigt hat.

Vor einer Freigabe die reguläre Sichtabnahme des finalen 2.5-Kandidaten mit Build-Hash, derselben Testkonfiguration und demselben Gerät protokollieren. Die Konfiguration des historischen Fehlversuchs hatte `page_seconds = 8` und `reset_on_start = false`. Bei erneutem Bildfehler Test abbrechen, alte App kontrolliert wiederherstellen und den Gerätzustand erst nach Portfreigabe bewerten. Logs und Bilder bleiben privat.

## 4. Physische Beobachtungsfragen

Für jeden Lauf beantworten:

- Ist das Bild vollständig und stabil, ohne blaue oder andere Streifen? Flackert es statisch oder während Updates?
- Sind Farben, Helligkeit und Orientierung korrekt? Fehlen Pixelzeilen/-spalten oder Bereiche?
- Wie viele Frames/Updates fehlen oder kommen verspätet? Gibt es Unterbrechungen?
- Bleibt die Ausgabe nach App-Neustart und kurzer USB-Neuverbindung stabil?
- Gibt es unbeabsichtigte Touch-Eingaben, falls Touch vorhanden ist? Touch separat prüfen; Bildausgabe besteht keinen Touch-Test.
- Stimmen sichtbares Ergebnis, Log und Exitcode überein? Für Pass/Fail zählt auch die visuelle Beobachtung.

## 5. Companion auf Pixel (unabhängig)

Companion separat von der USB-Ausgabe bewerten; eine bestandene Seite belegt die andere nicht.

1. Pixel-App-Version und Verbindungsvoraussetzungen festhalten; keine persönlichen Konten/Produktivdaten verwenden.
2. Companion-Seite am PC öffnen und nur über angezeigten QR-Code oder zeitlich begrenzten PIN pairen. PIN/QR-Inhalt nicht fotografieren, protokollieren oder teilen.
3. Erwartete Seite am Pixel und ausschließlich beabsichtigte Daten/Funktionen prüfen.
4. Trennen, App/Seite schließen und erneut verbinden. Wiederverbindung muss funktionieren; keine verwaiste oder doppelte Sitzung.
5. Wenn Testkonto/-gerät verfügbar: falsche/abgelaufene PIN und abgelehnte Verbindung prüfen. Zugriff muss verweigert werden, Fehlermeldung darf keine Geheimnisse enthalten.
6. Companion-Pass/Fail separat notieren; Pairing-Geheimnisse nie in Beweise aufnehmen.

### Aktueller isolierter Companion-Hardwaretest

ScreenBuddy `2.5.0-dev.2` lief mit `--no-lcd` an einem separaten Testport; die bestehende USB-App blieb unangetastet. Der erste Lauf war falsch konfiguriert (`[system] enabled=false`) und zeigte 0 Seiten sowie das Willkommensbild. Nach Korrektur auf System + Freunde lieferte die API zwei Seiten. Der automatische Indexwechsel von 0 auf 1 erfolgte nach 9 Sekunden. Es gab einen gepaarten und einen aktiven Client. Der Nutzer bestätigte beide Seiten als korrekt und stabil. Nach Neustart und erneutem Verbinden blieb es bei einem aktiven Client. Testinstanz beendet, Ports freigegeben. **Bestanden:** Seitenanzeige, automatischer Wechsel und Reconnect in diesem isolierten Aufbau. **Noch offen:** negative PIN-Prüfung sowie getrenntes Trennen und erneutes Verbinden. Keine IP, PIN, Fingerprints, Logs oder Fotos im Repository; private Belege bleiben privat.

## 6. Native UI: Maus und Tastatur

1. Mit Maus Einstellungsfenster öffnen/schließen, verschieben, minimieren/wiederherstellen und alle Bedienelemente nutzen.
2. Ohne Maus per Tastatur bedienen: Tab-Reihenfolge, sichtbarer Fokus, Enter/Leertaste, Escape, Pfeiltasten und verständliche Beschriftungen prüfen.
3. Werte ändern/speichern, schließen und erneut öffnen; Persistenz und Laufzeitwirkung prüfen. Ungültige Werte verständlich ablehnen oder sicher korrigieren.
4. Keine unbeabsichtigten Aktionen bei Tastaturbedienung. Bei 100 %, 150 % und 200 % Windows-Skalierung sowie hoher Kontrastdarstellung prüfen, soweit verfügbar.

## 7. Upgrade und Rollback

Nur in kontrolliertem Testprofil mit verifizierten Backups.

1. Ausgangsinstallation, Konfiguration und Companion-/Update-Zustand prüfen; Rückspielbarkeit der Backups bestätigen.
2. Auf den vorgesehenen 2.5-Build upgraden. Installerstatus und danach tatsächlich laufende EXE, Pfad und Version kontrollieren.
3. Einstellungen, Autostart (falls im Umfang), USB-Ausgabe und Companion-Verbindung prüfen. Konfiguration muss erhalten bleiben.
4. Rollback ausführen und tatsächlich gestartete alte Version samt Konfiguration kontrollieren; USB und Companion erneut unabhängig bewerten.
5. Fail, wenn trotz Installations-Erfolg falsche/alte EXE läuft, Konfiguration verloren geht oder Gerät/Companion nicht wiederhergestellt wird.

## 8. Datenschutz und Sicherheit

- Pairing-PINs/QR-Codes, Tokens, Passwörter, private URLs, personenbezogene Inhalte und vollständige Konfigurationen nie in Commit, Repository, öffentliches Issue, Release oder Screenshot aufnehmen.
- Logs vor Weitergabe redigieren; nur minimale nötige Auszüge teilen, Rohlogs privat halten.
- Backup-Ordner und private Fotos bleiben außerhalb des Repositorys. Dieses Dokument enthält absichtlich keine Screenshots oder Logs.
- Abgelehnte Pairing-Versuche, Trennung und Wiederverbindung prüfen. Keine Tests gegen fremde Geräte, Konten oder Netzwerke.

## 9. Nachweis und Freigabegates

Je Testfall mindestens Build/Commit, Datum, Prüfer, Gerät/VID/PID/COM, Konfigurationsvariante, Schritte, Soll/Ist und Pass/Fail festhalten. Öffentliches/versioniertes Beweismaterial muss geheimnisfrei und redigiert sein.

| Gate | Pass-Kriterium | Aktueller Status |
|---|---|---|
| Build/Prozess | Buildkennung und tatsächlich laufende EXE geprüft | COM7 und alte EXE verifiziert; lokal `2.5.0-dev.2` getestet |
| Serielle Demo | Frames sichtbar, artefaktfrei; Port danach frei | **FAIL**: fünf Frames, Streifen/Flackern sichtbar |
| Power-Cycle | Nach USB ab/an stabil; Ergebnis dokumentiert | Bestätigt: 10 s abgezogen, alte App danach mit normalem Bild; Ursache offen |
| Echte App | 10-Minuten-Sichttest und Neustart bestanden | Offen |
| Pixel Companion | Pairing, Funktion, Trennung und Wiederverbindung bestanden | Teilweise bestanden: zwei Seiten, Wechsel und Reconnect bestätigt; negative PIN sowie separater Trennen/Rückverbinden-Test offen |
| Native UI | Maus, Tastatur, Persistenz und Skalierung bestanden | Offen |
| Upgrade/Rollback | Version und Konfiguration in beide Richtungen verifiziert | Offen |
| Datenschutz/Sicherheit | Keine Geheimnisse im Repository; Pairing/Trennung geprüft | Offen |

Gesamtfreigabe nur bei bestandenen anwendbaren Gates und ohne offene kritische Fehler. Der aktuelle Befund ist **keine Hardware- und keine RC1-Freigabe**.
