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
| 12 | `CanvasWindow`: Kacheln eigener Ansichten zeichnen nur ihren Ausschnitt statt einer ganzen Seite (eingebaute Quellen; Plugins unverändert) | 52,3 | 51,8 | 52,5 | Redmi eigene Ansicht Zeichnen 19,0 -> 4,4 ms; Cockpit p95 1520×720 13,3 -> 6,9 ms, Pad 64 -> 14 ms (Messung R12–R14 gemeinsam) |
| 13 | Companion-Verbindung wird bei neuem Bild geweckt (statt 8-ms-Abfrage) | (s. 12) | | | gleichmäßigere Abstände, bis 8 ms weniger Verzögerung |
| 14 | Einzelkachel und Vollbildfläche ohne Bildkopie (`compose`/`compose_view` mit Verschieben), Gerätebild verschoben statt kopiert | (s. 12) | | | 2–3 Kopien à 4–16 MB weniger je Bild |
| 15 | JPEG: BGR selbst packen statt WIC-Konverter | – | – | – | verworfen: 1920×720 3,7 -> 3,8 ms, Pad 11,3 -> 11,4 ms (Kodierer dominiert) |
| 16 | HUD-Grundraster einmal je Größe zeichnen und als Bild einsetzen | – | – | – | verworfen: Bitmap-Zeichnen teurer als die Pfade (1920×720 3,4 -> 4,2 ms, Pad 7,0 -> 8,5 ms) |
| 17 | DirectWrite-Textlayouts je Thread zwischenspeichern (Format + Text, max. 1024) | 54,1 | 52,8 | 54,5 | KI 1920×720 2,49 -> 1,72 ms, HUD 3,41 -> 2,75 ms, 480×320 etwa halbiert (Messung R17–R18 gemeinsam) |
| 18 | Fester Animationstakt ab der Soll-Frist statt ab der verspäteten Weckzeit | (s. 17) | | | Host 57,3 -> 59–60 Bilder/s (Timer-Überschwingen summierte sich) |
| 19 | `Canvas::line`: Pfad des vorigen Aufrufs wiederverwenden (Glühleitungen zeichnen denselben Pfad dreimal), Strichstil einmal je Thread | – | – | – | HUD Pad 6,63 -> 5,98 ms, 480×320 1,39 -> 1,33 ms; Codex-Prüfung ohne Befund |
| – | Fehlerbehebung Game-Mode: Ziel-Display-Liste unterscheidet gleichnamige Geräte (ID-Ende) und zeigt verbunden/offline; Logwarnung, wenn ein erkanntes Spiel kein verbundenes Ziel hat | | | | Ursache beim Nutzer: Redmi neu gekoppelt (neue ID), Game-Mode zeigte auf die alte Kopplung; Codex-Befunde (3) umgesetzt |
| 20 | Pinsel-Cache je Zeichenfläche 16 -> 64 Farben | – | – | – | verworfen: im Rauschen, eher schlechter (HUD 1920×720 2,71 -> 2,87–2,94 ms) |
| 21 | Recycling-Allokator für Bildpuffer ab 1 MB (Mikromessung: frischer Puffer +0,8–2,4 ms) | – | – | – | verworfen: im Renderpfad kein Gewinn messbar (HUD Pad 5,98 -> 6,46–6,66 ms), Typumbau ohne Nutzen |
| 22 | Benchmark robuster: hohe Priorität, drei Durchgänge je Fall (bester zählt), Summenzeile TOTAL als Hauptkennzahl | – | – | – | Baseline TOTAL Rendern 31,8 ms (Summe aller 12 Fälle); Codex-Befund (Werte aus einem Durchgang) umgesetzt |
| 23 | `Canvas::finish()` kopiert direkt aus dem gesperrten WIC-Bitmap statt Nullfüllen + CopyPixels | – | – | – | TOTAL 31,8 -> 25,4 ms (−20 %); Codex ohne Befund, alle Tests grün |
| 24 | JPEG: Kodierer liest über eine eigene `IWICBitmapSource` direkt aus dem Bild statt aus einer WIC-Vollkopie | – | – | – | TOTAL JPEG 61,1 -> 44,6 ms (−27 %); Codex: 2 Überlauf-Befunde in CopyPixels umgesetzt |
