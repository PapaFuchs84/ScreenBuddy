# FuchsLabs USB-Display 1.1 – Sicherheits- und Performance-Review

Stand 21.10.2026, geprüft auf Meanmachine (Quellcode `usb-display-src`, installierte EXE `USB-Display`).

## Ergebnis in Kürze

Die App ist grundsätzlich solide gebaut (nur lokal erreichbar, Token per DPAPI, TLS-Prüfung aktiv, keine Shell-Aufrufe mit Fremddaten, keine Geheimnisse im Log). Zwei Punkte waren wirklich wichtig und sind behoben: der Bot-Token lag noch im Klartext in Sicherungsdateien, und die Bildbibliothek Pillow hatte 45 bekannte Sicherheitslücken.

## Befunde nach Priorität

| # | Schwere | Befund | Status |
|---|---|---|---|
| 1 | **Hoch** | Bot-Token im Klartext in `USB-Display\config.toml.bak`, `FuchsLabs\config.backup-vor-1.1.toml` und `turzx-discord-friends\config.toml`. Ursache: Beim Verschlüsseln wurde die alte Datei als `.bak` kopiert. | Behoben: Token in allen drei Dateien durch die verschlüsselte Fassung ersetzt; `save_config` legt nie mehr eine Klartext-Sicherung an. |
| 2 | **Hoch** | Pillow 11.3.0 hat 45 bekannte Lücken (OSV/GHSA). Die App dekodiert fremde Bilder (Discord-Avatare, BF6-Rang-Icons, ab jetzt Steam/Xbox-Avatare). | Behoben im Quellcode: Pillow 13.0.0 (0 bekannte Lücken), getestet. Wirkt nach dem nächsten EXE-Neubau. Zusätzlich dekodieren Avatare/Icons nur noch PNG/JPEG/WEBP/GIF. |
| 3 | Mittel | Einstellungsseite: Fremde Webseiten konnten GET-Abrufe auslösen (z. B. `<img src="http://127.0.0.1:8765/api/check/rss?url=…">` → die App ruft beliebige Adressen ab, auch im Heimnetz). Lesen konnten sie die Antwort nicht. | Behoben: Abrufe mit `Sec-Fetch-Site: cross-site/same-site` werden abgelehnt (nur das Öffnen der Startseite per Link bleibt erlaubt). |
| 4 | Mittel | Einstellungsseite ließ sich in fremde Seiten einbetten (Klick-Täuschung auf „Beenden“, „Hersteller-App beenden“). | Behoben: `X-Frame-Options: DENY`, `frame-ancestors 'none'`, `nosniff`. |
| 5 | Niedrig | Feed-Parser: XXE ist mit Pythons `xml.etree` nicht möglich (keine externen Entitäten, Expat mit Bomben-Schutz). | Zusätzlich gehärtet: Feeds mit `<!ENTITY` werden verworfen; nur http/https-Adressen. |
| 6 | Niedrig | `POST /api/config` mit JSON-Liste statt Objekt → Serverfehler 500. | Behoben (sauberer 400). |
| 7 | Info | Steam-API-Schlüssel steht technisch bedingt in der Abruf-URL; Fehler werden ohne URL geloggt. | In Ordnung, Hinweis an den Freundesliste-Thread. |

**Geprüft und in Ordnung:** Host-Prüfung gegen DNS-Rebinding (fremder Host → 403), CSRF-Schutz für alle POST (eigener Header erzwingt Vorabprüfung, Origin-Prüfung), Bindung nur an 127.0.0.1, Token wird nie an die Seite ausgeliefert, Eingaben werden typ- und bereichsgeprüft, Seite baut HTML nur mit `textContent`/DOM (kein XSS über Discord-Namen), TLS-Zertifikatsprüfung überall aktiv, Download-Grenze 3 MB bei Feeds, Ordnerrechte nur für SYSTEM/Administratoren/Benutzer, keine Administratorrechte nötig (nur „Hersteller-Autostart abschalten“ fragt per UAC), Log enthält keine Geheimnisse, übrige Pakete ohne bekannte Lücken.

## Performance (laufende EXE)

| Messwert | Ergebnis | Bewertung |
|---|---|---|
| CPU | ca. 1,1 % eines Kerns (≈ 0,07 % des 16-Kern-Systems) | sehr gut |
| RAM | 97 MB Arbeitsspeicher, 62 MB privat | gut |
| Startzeit | Einstellungsseite nach ≈ 2 s, Display-Reset bis erstes Bild ≈ 7 s (davon 5 s gewollter Reset) | gut |
| Größe | EXE 4,8 MB + `_internal` 31,6 MB | nach Neubau ≈ 7,5 MB kleiner (ungenutztes AVIF-Modul ausgeschlossen) |

## Geänderte Dateien

`webui.py` (nur Schutz-Middleware, Fehlerbehandlung), `appconfig.py` (nur Sicherung in `save_config`), `rss.py`, `bf6.py`, `discord_feed.py`, `requirements.txt`, `build.bat`, `FuchsLabs-USB-Display.spec`.

## Offen

- EXE-Neubau: erst wenn der Freundesliste-Thread fertig ist, dann ein gemeinsamer Build.
- Optional: Den alten Ordner `turzx-discord-friends` samt alter Aufgabe „FuchsLabs Discord-Display“ entfernen, wenn er nicht mehr gebraucht wird.
- Ob der Token noch an anderen Stellen außerhalb von `FuchsLabs` liegt (z. B. Notizen, Downloads), konnte ich nicht durchsuchen. Falls er je irgendwo geteilt wurde: im Discord Developer Portal neu erzeugen.
