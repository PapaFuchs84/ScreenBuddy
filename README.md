# FuchsLabs USB-Display

Windows-App für 3,5"-USB-Displays (480 × 320). Sie ersetzt die Hersteller-App und zeigt im Wechsel:

- **System:** CPU, GPU, RAM, Datenträger, Netzwerk, Temperaturen und Lüfter (über HWiNFO)
- **Freunde:** Discord, Steam und Xbox in einer gemeinsamen Liste
- **Battlefield 6 · Rangliste** der eingetragenen EA-Freunde
- **Blog-Updates** per RSS

Bedient wird alles über eine lokale Einstellungsseite im Browser (`http://127.0.0.1:8765/`), erreichbar über das Fuchs-Symbol im Infobereich. Im FuchsLabs-Design mit Hell- und Dunkelmodus.

## Für Anwender

Die fertige EXE gibt es als ZIP. Entpacken, `FuchsLabs-USB-Display.exe` starten, die Einstellungsseite öffnet sich von selbst.

- `ANLEITUNG.md` – Schnellstart
- `ANLEITUNG.html` – bebilderte Schritt-für-Schritt-Anleitung (liegt im ZIP; wird beim Build erzeugt)
- `DOKUMENTATION.md` – ausführliche Referenz: Einstellungen, `config.toml`, HWiNFO, Sicherheit, Fehlerbehebung
- `TESTPHASE.md` – Fehler melden und Rückmeldungen geben

## Selbst bauen

Voraussetzung: Windows 10/11, Python 3.11+ (64 Bit, von python.org).

```bat
build.bat
```

legt eine eigene `.venv` an, installiert die festgelegten Pakete und baut nach `dist\FuchsLabs-USB-Display\`. Zum Entwickeln ohne Display:

```bat
.venv\Scripts\python fuchslabs_display.py --no-lcd
```

Die bebilderte Anleitung entsteht aus `anleitung/anleitung.template.html` und den Screenshots in `anleitung/bilder` mit `anleitung/make_images.py`; Screenshots nimmt `anleitung/screenshots.mjs` auf. Zwei Screenshots sind derzeit nicht im Repository (echte Namen), siehe `PROTOKOLL.md`.

## Aufbau

| Datei | Aufgabe |
|---|---|
| `fuchslabs_display.py` | Hauptprogramm, Seitenrotation, Dienste |
| `display_driver.py` | Treiber für das serielle Display |
| `render.py`, `brand.py` | Zeichnen der Seiten, FuchsLabs-CI und Logo |
| `sysinfo.py` | Systemwerte (Windows-APIs, `nvidia-smi`, HWiNFO) |
| `discord_feed.py`, `friends.py`, `bf6.py`, `rss.py` | Datenquellen |
| `appconfig.py`, `secure.py` | Konfiguration, DPAPI-Verschlüsselung |
| `webui.py`, `webui.html` | Einstellungsseite |
| `tray.py`, `winsetup.py` | Infobereich, Autostart, Hersteller-App |

Mehr in `DOKUMENTATION.md` (Kapitel 10). Änderungen stehen in `CHANGELOG.md`, der Entstehungsverlauf in `PROTOKOLL.md`, die Sicherheitsprüfung in `SICHERHEITSREVIEW.md`.

## Sicherheit

Zugangsdaten (Discord-Token, Steam- und Xbox-Schlüssel) liegen nur verschlüsselt in der lokalen `config.toml` und gehören **nie** ins Repository. `.gitignore` schließt `config.toml`, Logs und Build-Ergebnisse aus.

© 2026 FuchsLabs
