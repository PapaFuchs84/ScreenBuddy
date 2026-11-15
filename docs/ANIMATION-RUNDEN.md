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
