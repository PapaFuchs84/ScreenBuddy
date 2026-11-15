| Runde | Änderung | HUD Redmi UI-fps (Ziel 60) | KI | eigene Ansicht | Bemerkung |
|---|---|---|---|---|---|
| 0 | Ausgang 2.12.0 | 18,0 | 22,9 | 14,7 | Bildzeit HUD 43,6 ms |
| 1 | Messpunkte + fl-anim-bench | – | – | – | Analyse: Seitenaufbau 20 ms, Sperre 7–14 ms |
| 2 | Cache eigene Ansichten | 28,1 | 25,9 | 16,0 | Vorlauf 20–36 ms -> <0,3 ms |
| 3 | Zeichenflächen-Pool | 32,0 | 29,1 | 31,9 | Pad-HUD-Render 15,0 -> 9,7 ms |
| 4 | 1-ms-Timer beim Animieren | 51,0 | 49,4 | 46,6 | Sleep-Raster 15,6 ms war Deckel bei ~32 fps |
| 5 | Seitenübergänge + Navigation ohne Ratenbremse | 48,2 | – | – | Übergang 60 fps, 13 ms/Bild; Spitze 40 ms beim Wechsel |
| 6 | Vorwärmen eingebauter Seiten je Größe | – | – | – | Kaltstart 35–220 ms beim ersten Wechsel entfällt |
| 7 | HUD-Scan-Übergang (Standard), 0,45 s, HUD-Farben | – | – | – | 3,5 ms/Bild bei 1520×720; Slide 1,2 ms, Fade 2,3 ms; Gerätemessung offen |
| 8 | HUD-Scan v2: Gitter, Lineal-Ticks, Eckklammern, Schaltkreis-Impulse, Nachleuchten; zeilenweise statt je Pixel | 49,7 | 50,4 | 39,6 | Übergang 3,5 -> 1,7 ms/Bild (1520×720); Gerät gemessen |
| 9 | Game-Intro im HUD-Stil (Raster, Klammern, Scanstrahl, Ladebalken), Logo-Zoom gerastert/gecacht, HUD-Elemente auf transparenter Fläche | – | – | – | Intro Pad 2534×1574: 21,2 -> 13,7 ms (p95 42,8 -> 16,7); 1520×720: 6,1 -> 3,8 ms |
| 10 | `blend_rect` mit gepackten Kanälen (bit-identisch) | – | – | – | Intro-Ausblendung Pad p95 27 -> 19 ms; wirkt auch bei Plugin-Überblendungen |
| 11 | Spielansicht ohne Intro und Rückkehr aus dem Spiel mit Seitenübergang; Cockpit-Logo je Thread gecacht | – | – | – | vorher harter Schnitt; Cockpit 1520×720 15 ms, Pad 79 ms (Basis für R12) |
