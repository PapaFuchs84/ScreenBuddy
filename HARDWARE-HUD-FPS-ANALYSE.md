# Globale Animationen: Bildrate und Grenzen

[English](HARDWARE-HUD-FPS-ANALYSE.en.md) · lokaler Prüfstand vom 14.11.2026, keine Veröffentlichung.

Die bisherige Grenze von fünf Bildern/s ist keine Vorgabe des Companion-Protokolls oder der Android-App. Im ursprünglichen lokalen Commit ist keine Messbegründung für den Wert belegt. Sie begrenzt vor allem den Aufwand für vollständige JPEG-Bilder, Kodierung, Übertragung und Dekodierung. Im erneuten Redmi-Test von v2 wurden bei dieser Grenze 49 UI-Neuzeichnungen über 12,093 Sekunden beobachtet: 4,05/s.

## Entscheidung für Testversion v3

Der Zielwert gilt jetzt global für **sichtbare Animationen** auf Monitor und Companion: **30 Bilder/s**, einstellbar von **5 bis 60** unter **Anzeige → Animationen** (`display.animation_fps`). Dazu gehören HUD, aktive KI-Zustände/Zeilenübergänge, laufender Spotify-Fortschritt und Pegel sowie sichtbare native, eigene, Game-Intro- und Plugin-Übergänge. Statische Ansichten behalten ihren Quellen-/Anzeigetakt und die normale Companion-Transportgrenze von fünf Bildern/s. `system.hardware_hud_fps` ist nur ein Migrationsalias; ein ausdrücklich gesetzter globaler Wert hat Vorrang. Die globale App-/Geräteprüfung steht noch aus.

Animierte Bilder werden bereits vom Produzenten zeitlich getaktet. Der schnelle Pfad soll deshalb für vertrauenswürdige, sichtbare Animationen ohne zusätzlichen Transporttimer arbeiten; diese globale Erweiterung ist noch zu prüfen. Letztes Bild, Versionsvergleich, synchroner Sendepfad mit Rückstau und begrenztes Beenden bleiben erhalten. Es entsteht keine unbegrenzte Bildwarteschlange. Der Zielwert und die FPS-Metadaten sind ein Takt-/Ressourcenhinweis, keine Garantie oder harte Grenze der tatsächlich ausgelieferten Bilder; gemischte oder anderweitig veränderte Ansichten können zusätzliche Aktualisierungen erzeugen. Android-App und Drahtformat ändern sich nicht. Sensoren werden nicht häufiger abgefragt.

**30 ist der gewählte Ausgangswert für die globale Einstellung:** Im früheren HUD-only-Transportkandidaten wurden rund 21 UI-Neuzeichnungen/s erreicht. 60 als Ziel war mit rund 31/s die flüssigste gemessene Einstellung, kostete aber mehr CPU und Bandbreite. 60 bleibt eine Option; ungebremste Ausgabe wurde nicht getestet und ist nicht als unbedenkliche Vorgabe belegt.

## Historischer HUD-only-Kandidat: echter Redmi 8

Diese Messung stammt vom 13.11.2026 und gilt nur für das damalige HUD, nicht für alle neuen global animierten Ansichten. Android 14, 1520 × 720 Pixel, getrennte Debug-Test-App, unabhängiger Companion-Ausgang ohne zusätzliche Display-Ausgabe. Drei Messfenster von jeweils zwölf Sekunden ohne Screenshot oder Bildschirmaufnahme während der Messung; gleiche GPU-/CPU-Seitenphase.

| Ziel Bilder/s | Android-UI-Neuzeichnungen/s | Host-CPU-Kerne | Redmi-CPU-Kerne | TLS Mbit/s |
|---:|---:|---:|---:|---:|
| 20 | 16,06 | 0,414 | 0,848 | 13,090 |
| 30 | 21,31 | 0,439 | 1,029 | 17,456 |
| 60 | 31,38 | 0,693 | 1,261 | 26,196 |

Ein CPU-Kern-Äquivalent entspricht einem durchgehend ausgelasteten logischen Kern, nicht einem Prozentwert der gesamten Maschine. Die UI-Rate stammt aus Androids `gfxinfo` und ist weder eine gezählte Netzwerk-Bildrate noch die physische Bildwiederholrate des Panels. TLS-Verkehr enthält Transportaufwand und beschreibt keine geprüfte LAN-Kapazität.

Alle drei Verbindungen blieben bestehen. Thermischer Status war 0, die Batterietemperatur stieg im kurzen Vergleich von 27,0 auf 27,2 °C. Beim 60er Ziel meldete Android 4,39 % Jank; diese Deadline-Metrik ist nicht mit der Bildrate des Streams gleichzusetzen. Die PC-Hintergrundlast wurde nicht beseitigt. Das ist keine Langzeit-, Batterie- oder Hardware-Maximalmessung und kein Nachweis erreichter 60 Bilder/s.

Historischer HUD-only-Kandidat für diese Tabelle: `F099702F29136203E44E20602A9813A844ED62B3B56466923704F761263CBFC4`. Die Tabelle charakterisiert diesen Transportkandidaten. Der spätere HUD-only-Build mit Standard 30 bestand 88 HUD- und 3556 Regressionsprüfungen; 434 Companion-Prüfungen desselben Transportcodes bestanden. Der abschließende physische Smoke-Test des endgültigen Builds `96BC99AD4D179E8E0A8C9407415E5CB0C2238E1B83B31151664DE92408B18316` bestand für alle drei Seiten im Querformat (36,41 s) und Hochformat (35,80 s): Live-Werte, Wechsel, Bewegung und keine beobachteten Überlappungen. `hardware_hud_fps` fehlte bewusst in der Testkonfiguration; die API bestätigte den kompilierten Standard 30. Das ist keine neue Zeitmessung der Bildrate.

Lokaler Bericht: [producer-paced/BENCHMARK-RESULT.md](redmi-validation/fps-benchmark-2026-11-13/producer-paced/BENCHMARK-RESULT.md).

## Vorheriger Transportkandidat: sechs Zielwerte

Dieser getrennte Durchlauf verwendete noch den zusätzlichen Transporttimer. Er wird als Verlauf erhalten und darf nicht als Messung des finalen Pfads ausgegeben werden. Hintergrundlast und kurze Einzelmessungen erlauben keine strenge Ursache-Wirkungs-Aussage zwischen den Varianten.

| Ziel Bilder/s | Android-UI-Neuzeichnungen/s | Host-CPU-Kerne | Redmi-CPU-Kerne | TLS Mbit/s |
|---:|---:|---:|---:|---:|
| 5 | 3,97 | 0,113 | 0,232 | 3,26 |
| 10 | 7,97 | 0,137 | 0,458 | 6,48 |
| 15 | 10,65 | 0,275 | 0,592 | 8,66 |
| 20 | 15,85 | 0,263 | 0,846 | 12,91 |
| 30 | 16,82 | 0,444 | 0,891 | 13,84 |
| 60 | 31,11 | 0,607 | 1,262 | 25,52 |

Lokaler Bericht: [erster Vergleich](redmi-validation/fps-benchmark-2026-11-13/BENCHMARK-RESULT.md).

## Gemischte Ausgaben und natives USB: neue Fälle noch offen

Vor der zusätzlichen Trennung wurde eine statische Companion-Seite durch ein schnelles Haupt-HUD unnötig mitgezeichnet: bei Hauptziel 5 ungefähr 5 Renderaufrufe/s, bei 60 ungefähr 17,5/s, obwohl die Messwertoberfläche nur ungefähr einmal/s neue Daten brauchte. Ein eigener 0,5-Sekunden-Schutz für die Messwertansicht soll dies begrenzen; die globale App-Prüfung dieses Schutzes steht aus. Das erhöht keine Sensor-Abfragerate.

Auf einem echten nativen Rev-A-USB-Display (480 × 320) akzeptierte der Controller Schreibvorgänge ohne optische Rücklesung. Quellenziel 5 ergab ungefähr 1,65 Ausgabeaufrufe/s, Quellenziel 30/60 jeweils ungefähr 2,27/s; mittlere USB-Ausgabezeit 428–540 ms, gemessener Teiltransfer ungefähr 170 kB/s. Das ist kein Nachweis optisch angezeigter Bilder oder voller 30/60 Bilder/s. Die zusätzliche feste HUD-Sekundenpause entfällt zugunsten von Übertragungstakt plus kurzer Leerlaufpause; Plugin-Übergangsbudgets bleiben begrenzt. Echte globale App-Fälle stehen noch aus. Die normale Windows-App wurde für den isolierten Rohtransfer kurz pausiert und anschließend mit bestätigtem Displaystatus wiederhergestellt.

KI-/Spotify-/Spiel-/andere Seiten werden zusätzlich mit synthetischen Render-/Transportbeispielen geprüft. Diese Beispiele sind kein Nachweis realer Spotify-Anmeldung/-Steuerung oder echten Spielbetriebs. Die folgenden erfolgreichen HUD-Prüfungen und Benchmarkzahlen sind kein bestandener globaler Test.

## Geltungsbereich

Beide Vergleiche liefen im Vordergrund bei eingeschaltetem Bildschirm über USB-ADB-Reverse und eine temporäre lokale Weiterleitung. LAN, Hintergrund, Neustart und längerer Batteriebetrieb wurden nicht abgenommen. Die normale Host-Zugriffsregel und die Kopplung der normalen Companion-App blieben erhalten; Testhost und Weiterleitungen wurden anschließend beendet. Persönliche Konfigurationen, Kopplungsdaten und Rohlogs gehören nicht in das Testpaket.
