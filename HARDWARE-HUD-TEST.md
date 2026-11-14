# Hardware-HUD

[English](HARDWARE-HUD-TEST.en.md)

Das Hardware-HUD ist seit ScreenBuddy 2.12.0 (14.11.2026) eine optionale Darstellung der Systemseite: eine dunkle Neon-Anzeige in Cyan und Magenta mit eigenen Vektorgrafiken für PC, Grafikkarte und Prozessor. Es ist standardmäßig ausgeschaltet; die gewohnte Systemansicht bleibt unverändert verfügbar.

## Einschalten

1. Einstellungen öffnen und unter **Seiten → System** die Systemseite aktivieren.
2. Dort **Hardware HUD** einschalten und die Einstellungen speichern.

Die Systemseite zeigt danach das HUD. Zum Zurückwechseln **Hardware HUD** wieder ausschalten. Andere aktivierte Seiten erscheinen weiterhin im normalen Seitenwechsel; für eine durchgehende HUD-Anzeige nur die Systemseite auswählen. Das HUD verwendet ein eigenes Farbschema.

## Seiten

Das HUD wechselt standardmäßig alle acht Sekunden automatisch zwischen drei Seiten:

- **Übersicht:** die wichtigsten Werte von CPU und GPU auf einen Blick.
- **GPU:** Werte der Grafikkarte mit Ring- und Lüftergrafik.
- **CPU:** Auslastung, Takt, Temperatur, Lüfterdrehzahl und Package-Leistung in Watt, soweit verfügbar.

Ringe, Lüfter und Signalimpulse sind animiert, Seitenübergänge ebenfalls. Die Bewegung ist dekorativ: Die Grafiken sind schematisch und keine unabhängige Messung, etwa von Gehäuselüftern.

Es gibt eigene Layouts für breite, normale und hochkant betriebene Displays. Hardwarebezeichnungen werden anhand ihrer gemessenen Textbreite umbrochen, damit Beschriftungen nicht überlappen.

## Messwerte und HWiNFO

- CPU-Auslastung und Takt kommen aus Windows.
- CPU-Temperatur, CPU-Lüfterdrehzahl und **CPU-Package-Leistung in Watt** benötigen ein laufendes HWiNFO mit aktivierter Shared-Memory-Ausgabe und passenden Sensoren.
- CPU-Watt werden nur aus einem Leistungssensor (Typ Power, Einheit `W`) einer CPU-Sensorgruppe mit bekanntem originalem Package-/PPT-Namen übernommen: `CPU Package Power`, `CPU Package Power (SMU)`, `CPU Package`, `CPU PPT`, `CPU Power`. In HWiNFO umbenannte Sensoren funktionieren über ihren ursprünglichen englischen Namen. Unbekannte Sensorbezeichnungen bleiben unbesetzt.
- NVIDIA-GPU-Daten kommen aus der vorhandenen NVML-Anbindung. Nicht jeder Treiber und nicht jedes Modell liefert alle Werte; andere GPUs können weniger Werte anbieten.
- Fehlende oder ungültige Messwerte bleiben `--`. Es wird nichts geschätzt oder erfunden.
- Individuell auswählbare zusätzliche Gehäuselüfter sind nicht enthalten.

## Animationsrate

Unter **Einstellungen → Anzeige → Animationen** lässt sich die globale Animationsrate von **5 bis 60** einstellen (`display.animation_fps`), Standard **30**. Sie gilt für sichtbare Animationen: Hardware-HUD, aktive KI-Pulse und Zeilenübergänge, die laufende Spotify-Seite samt Pegel-/Fortschrittskacheln, animierte eigene Ansichten, Game-Intro und Plugin-Überblendungen.

- Der Wert ist ein Ziel, keine garantierte Bildrate. Höhere Werte kosten mehr CPU und beim Companion mehr Datenverkehr.
- Statische Ansichten behalten ihre eigene langsame Aktualisierung.
- Sensoren werden nicht häufiger abgefragt; nur die Grafik bewegt sich flüssiger.
- Ein früherer lokaler Schlüssel `system.hardware_hud_fps` wird automatisch migriert; ein ausdrücklich gesetzter globaler Wert hat Vorrang.

## Ausgabegeräte

- **Windows-Monitor (HDMI/USB-C):** nutzt den normalen Monitorausgang.
- **Android-Companion:** Am PC getaktete Animationen erhalten keine zweite Transportverzögerung. Normale statische Companion-Seiten bleiben auf 5 Bilder/s begrenzt und werden nicht neu gezeichnet, nur weil die Hauptanzeige schnell animiert. Protokoll und Android-App sind unverändert; Companion 1.1.0 bleibt aktuell.
- **Serielles USB-Display (Rev. A, 480 × 320):** Die Übertragung läuft in einem eigenen Ausgabethread und übergibt stets das neueste Bild. Ein animiertes Bild benötigt hier etwa 0,4–0,5 s; die Animation richtet sich deshalb nach der gemessenen Übertragungszeit und erreicht auf diesem Display nicht die eingestellte Zielrate. Eine langsame USB-Übertragung blockiert das Companion-Rendering nicht mehr.

## Grenzen und Prüfstatus

Prüfstatus 2.12.0: siehe Release-Notizen
<!-- PRÜFSTATUS-2.12 -->

**Softwareprüfungen (Kandidatenstand vor der Integration):** 119 Prüfungen zu HUD, globaler Migration und KI, 434 Companion, 3556 Kernregression, 2144 eigene Ansichten, 845 Plugin-Zeitplanung und 290 Game-Prüfungen bestanden. Der vollständige Lauf des integrierten 2.12.0-Builds wird wiederholt.

**Physisch geprüft:**

- Echtes Redmi 8 mit Android 14 als Companion über USB-Tunnel mit den Zielraten 5/30/60: bei aktiver KI etwa 4,9/21,3/26,2 UI-Neuzeichnungen/s, auf der normalen Systemseite etwa 0,6/s. Hoch- und Querformat, Wischen und automatischer Wechsel ohne Überlappungen.
- Echtes serielles USB-Display Rev. A an COM7: Routing- und Übertragungsprüfungen. Eine optische Messung der Bildrate am Panel fand nicht statt.

**Nicht geprüft:** echtes Spotify-Konto und Wiedergabesteuerung, echtes Spielen, WLAN/LAN, Hintergrundbetrieb, Neustart sowie Langzeit-, Akku- und Temperaturverhalten.

**Gemeinsame Abnahme USB-Display + Companion:**
<!-- GEMEINSAM-2.12 -->

Die Messungen der lokalen HUD-Testbuilds vor 2.12.0 stehen als historische Analyse in [Hardware-HUD: Bildratenanalyse](HARDWARE-HUD-FPS-ANALYSE.md).
