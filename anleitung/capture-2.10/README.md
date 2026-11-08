# Aufnahme-Werkzeuge für docs/screenshots/2.10

Nur für Testinstanzen. Nie gegen die echte App (Port 8765/18765/8777) ausführen.

- `start.ps1 -Lang de|en` startet eine Kopie der Release-EXE mit `FUCHSLABS_INSTANCE`, eigenem Datenordner (`shot-runtime`) und Port 18955.
  Erwartet `shot-runtime\ScreenBuddy.exe` und `shot-runtime\demo.toml` (Material-Orange dunkel, Quellen aus, `[ai] port = 18965`).
- `resize.ps1`, `click.ps1`, `go.ps1`, `scroll.ps1`, `cap.ps1` bedienen und fotografieren das Einstellungsfenster (PrintWindow, Klicks nur, wenn das Testfenster oben liegt).
- `run-all.ps1`, `run-extra.ps1` erzeugen die nummerierten Bilder je Sprache.
- `mkgif.py` baut das Game-Mode-Intro-GIF aus Frames von `/api/preview.png` (Dummy-Spiel per `additional_games`).
- `mkvid.py`, `anon_bf6.py`, `anon_spotify.py` anonymisieren Redmi-Aufnahmen (Namen und Cover werden ersetzt).
- Koordinaten gelten für ein 1520×940-Fenster; Pillow und `imageio-ffmpeg` werden benötigt.
