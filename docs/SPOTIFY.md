# Spotify-Seite

**Deutsch** · [English](SPOTIFY.en.md)

Zeigt den aktuell gespielten Titel mit Cover, Künstler, Album und Fortschritt auf dem Display. Mit Touch (Monitor, Companion) steuern die Tasten ⏮ ⏯ ⏭ die Wiedergabe.

## Einrichten (einmalig, kostenlos)

1. Auf [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard) eine App anlegen („Web API“ ankreuzen).
2. Als **Redirect URI** exakt die Adresse aus *Einstellungen → Spotify* eintragen (Standard `http://127.0.0.1:8765/spotify/callback`; folgt dem Port der Einstellungsseite).
3. Die **Client-ID** in ScreenBuddy einfügen, „Seite anzeigen“ einschalten, speichern.
4. „Bei Spotify anmelden“ klicken, im Browser zustimmen.
5. Bei einer bestehenden Installation die Seite unter *Einstellungen → Seiten & Reihenfolge* anhaken (Display und, falls gewünscht, Companion-Seitenliste). Neue Seiten werden nicht automatisch angezeigt. Fertig.

Ein Client Secret wird nicht gebraucht (Anmeldung per PKCE).

## Amplifier-Anzeige

Bei laufender Wiedergabe zeigen 16 Balken den Pegel der Windows-Systemausgabe (Spektrum, nur Querformat). Ausschalten: *Einstellungen → Spotify → Amplifier-Anzeige*. Die Systemausgabe wird lokal gelesen und sofort zu Balken verrechnet; es wird nichts aufgezeichnet, gespeichert oder gesendet, und die Analyse läuft nur, solange die Spotify-Seite zu sehen ist. Wird die Musik nicht über den PC abgespielt (z. B. auf dem Handy), bleiben die Balken flach.

## Grenzen

- **Steuern braucht Spotify Premium** (Vorgabe von Spotify). Die Anzeige geht auch ohne Premium.
- Es muss ein Spotify-Gerät aktiv sein (App am Handy/PC läuft oder wurde zuletzt benutzt), sonst meldet Spotify „kein aktives Gerät“.
- Apps im Spotify-Entwicklungsmodus sind auf wenige Nutzer begrenzt, die du im Dashboard unter *User Management* freigibst. Dein eigenes Konto ist immer dabei.
- Abfrage alle 4 s (Wiedergabe) bzw. 8 s (Pause); bei Spotify-Begrenzung pausiert ScreenBuddy kurz.

## Datenschutz

- Berechtigungen: aktuelle Wiedergabe lesen und steuern. Mehr nicht.
- Das Refresh-Token liegt DPAPI-verschlüsselt in `spotify.token` neben der `config.toml`, nie in der Konfiguration; es ist nur für dein Windows-Konto lesbar. „Abmelden“ löscht die Datei.
- Cover werden nur von `i.scdn.co` geladen.

Spotify ist eine Marke von Spotify AB; FuchsLabs steht in keiner Verbindung dazu.