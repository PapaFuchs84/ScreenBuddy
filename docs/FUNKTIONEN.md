# ScreenBuddy · Funktionen im Detail

Aus dem README ausgelagert (Stand 2.7.0). Englisch: [FEATURES.en.md](FEATURES.en.md). Übersicht aller Dokumente: [docs/README.md](README.md).

## Inhalt

- [Systemseite per Touch und Update-Hinweis (ab 2.2.0)](#systemseite-per-touch-und-update-hinweis-ab-220)
- [Touch-Ziele und Spielstunden (ab 2.0.8)](#touch-ziele-und-spielstunden-ab-208)
- [Home Assistant (ab 2.1.0)](#home-assistant-ab-210)
- [Streaming-Neustarts (ab 2.8.0)](#streaming-neustarts-ab-280)
- [Spotify (ab 2.8.0)](#spotify-ab-280)
- [Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)](#android-companion-app-ab-230-mehrere-geräte-ab-270)
- [Plugin-Store und Plugins (ab 2.6.0)](#plugin-store-und-plugins-ab-260)
- [Ansichten durchblättern (ab 2.0.4)](#ansichten-durchblättern-ab-204)

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick darauf öffnet die Einstellungen), und **Einstellungen → Updates & Autostart** zeigt „Neue Version verfügbar: …“. **Release-Seite öffnen** führt zur Download-Seite, **Jetzt nach Updates suchen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Updates & Autostart → Automatisch nach Updates suchen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Touch-Ziele und Spielstunden (ab 2.0.8)

Entwickelt als 2.0.7; diese Version wurde nicht einzeln veröffentlicht, die Funktion steht ab 2.0.8 zur Verfügung. Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](TOUCH-2.0.7.md). [Prüfstand](../cpp/TESTPROTOKOLL.md).

## Home Assistant (ab 2.1.0)

Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. `http://homeassistant.local:8123`), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml`. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit `http://` laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort `https://`, wenn Home Assistant es anbietet.

## Streaming-Neustarts (ab 2.8.0)

Seite **Streaming-Neustarts** („New on streaming“): zeigt Filme und Serien, die in den letzten 30 Tagen erschienen sind und bei den gewählten Anbietern im Abo (Flatrate) laufen. Pro Zeile Titel, Datum, Film oder Serie und das Kürzel des Anbieters (z. B. NF, PV, D+, ATV+, WOW, RTL+). Bis zu 6 Zeilen, neueste zuerst.

- **Einrichtung:** unter **Einstellungen → Streaming-Neustarts** die Seite einschalten und einen kostenlosen TMDB-Key eintragen (bei bestehender Installation die Seite zusätzlich unter *Seiten & Reihenfolge* anhaken) (themoviedb.org → Einstellungen → API; der v3-Key oder das „API Read Access Token“ funktionieren). Der Key liegt wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml` und wird nie protokolliert oder angezeigt. **Key prüfen & Anbieter laden** holt die Anbieterliste deiner Region aus TMDB; mit Haken wählst du die Anbieter (bis zu 30). Vorgewählt sind Netflix (8), Prime Video (9 und 119), Disney+ (337), Apple TV+ (350), WOW (30) und RTL+ (298). Weicht eine ID in deiner Region ab, korrigiere sie über die geladene Liste.
- **Region:** folgt der App-Sprache (Deutsch: DE, Englisch: US); im Feld **Region** (`region` unter `[streaming]`) mit zwei Buchstaben überschreibbar, z. B. `AT`.
- **Abruf:** nur GET über HTTPS an `api.themoviedb.org/3`. Je Aktualisierung zwei Suchanfragen (Filme, Serien) plus – nur für die angezeigten Einträge und einmal gemerkt – eine Anbieterabfrage je Eintrag; höchstens stündlich (Cache 1 Stunde), kleine Pausen zwischen den Anfragen. Bei Fehlern bleiben alte Einträge sichtbar, die App versucht es nach 10 Minuten (bei „Limit erreicht“ nach 15 Minuten, bei ungültigem Key erst zur nächsten Stunde) erneut.
- **Anzeige ohne Daten:** „Kein Key“ (Seite an, aber kein Key), „Lade Daten …“, „Keine Daten“ (Fehler oder offline, nie ein Absturz).
- **Datenschutz:** TMDB sieht deine IP-Adresse und die gewählte Region/Anbieterliste, nicht dein Konto bei den Streaming-Diensten.
- **Nachweis:** This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch. Die Seite trägt die Fußzeile „TMDB / JustWatch“; der Hinweis steht auch in den Einstellungen (Updates & Autostart → Nachweise) und in [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md). Das offizielle TMDB-Logo ist noch nicht eingebunden.

## Spotify (ab 2.8.0)

Zeigt den aktuell gespielten Titel mit Cover und Fortschritt; mit Touch (Monitor, Companion) steuern Tasten Zurück, Wiedergabe/Pause und Weiter (Spotify Premium nötig). Anmeldung per PKCE mit eigener Spotify-Developer-App, das Token liegt DPAPI-verschlüsselt in spotify.token. Einrichtung und Grenzen: [SPOTIFY.md](SPOTIFY.md).

## Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)

Ein Smartphone oder Tablet dient als zusätzliche Anzeige parallel zum USB-/HDMI-Display. Ab 2.3.0 gibt es den Companion als Vorschau-APK; die aktuelle Companion-App 1.0.0 (versionCode 4, Android 8+) ist ein separater Download. Seit 2.7.0 lassen sich mehrere Companions gleichzeitig betreiben, **synchron oder unabhängig**.

- **Eigene Seiten und Intervalle:** der Companion hat gegenüber USB/HDMI eigene Seitenliste und Anzeigedauer (unter **Companion**). Im unabhängigen Betrieb (`sync = false`) hat zusätzlich jedes Gerät eigene Seitenposition, Auflösung, Bedienung und optional eine eigene Seitenliste. Datenquellen und Design bleiben gemeinsam. Voreinstellung ist synchrone Anzeige.
- **Kopplung per QR-Code oder PIN:** unter **Einstellungen → Companion → Gerät koppeln** den Kopplungscode anzeigen. Die Android-App scannt den QR-Code oder verbindet sich über Adresse, Port (Standard 8777) und 6-stelligen PIN; der SHA-256-Fingerabdruck sichert die TLS-Verbindung.
- **Touch und Gesten:** im unabhängigen Betrieb schaltet Wischen nur das jeweilige Gerät um, im synchronen Betrieb alle. Tippen und Wischen sind in den [Companion-Hinweisen](COMPANION.md) beschrieben.
- **Native Auflösung und Drehung:** das Handy meldet Ausrichtung und Pixelmaße; der PC rendert passgenau.
- [Schritt-für-Schritt-Anleitung zur Kopplung](../ANLEITUNG.md#android-companion-app-koppeln-und-nutzen-ab-230) · [Einrichtung und Grenzen](COMPANION.md) · [Protokoll](COMPANION-PROTOKOLL.md)

Prüfstand: siehe [Release-Notizen 2.7](RELEASE-NOTES-2.7.md). Die automatisierten Mehrgeräteprüfungen sind Simulationen; adb-Gesten ersetzen keine physische Fingerprüfung. Beim Wechsel von einer Debug-APK unterscheidet sich die Signatur; eine direkte Aktualisierung ist dann nicht möglich (siehe [Release-Notizen 2.6](RELEASE-NOTES-2.6.md)). Die Companion-App nie automatisch deinstallieren: lokale Daten und Kopplungen gingen verloren.

## Plugin-Store und Plugins (ab 2.6.0)

Einstellungsseite **Plugin-Store** (neben Companion und Plugins, abschaltbar mit `[ui] plugin_store`): geprüfte Datenplugins aus dem signierten Store `PapaFuchs84/ScreenBuddy-Plugins` suchen, prüfen und installieren. Signierter Index und signierte Sperrliste (Ed25519, fester öffentlicher Schlüssel), Pakete per SHA-256 und Hostangaben geprüft, täglicher Hintergrundabgleich. Vor der Installation steht, an welche Hosts ein Plugin Daten senden würde. Installierte Plugins bleiben aus, bis du sie unter **Plugins** aktivierst; jeder Host braucht deine Freigabe. Seit 2.7.0 bleiben Store-Installationen und Updates auch ohne Netzwerkquellen ausgeschaltet; der lokale direkte Manifestimport verhält sich wie zuvor. Aktivierte Plugins lassen sich unter **Seiten & Reihenfolge** sortieren und ein-/ausblenden. Der Store zeigt signierte Vorschaubilder.

Weiter: [Plugin-System](PLUGIN-SYSTEM.md) · [Tutorial](PLUGIN-TUTORIAL.md) · [Store-Bedingungen](PLUGIN-STORE-BEDINGUNGEN.md) · [Beispiele](../cpp/examples/plugins/README.md). Online-Plugins und CRL/OCSP-Prüfungen bleiben experimentell.

## Ansichten durchblättern (ab 2.0.4)

Seit 2.0.4 lässt sich die laufende Anzeige per Knopfdruck weiterschalten, ohne zu speichern oder neu zu starten. In der nativen Oberfläche (ab 2.5.0) stehen dafür **Vorige Seite zeigen** und **Nächste Seite zeigen** unter **Einstellungen → Companion** bereit; sie wirken auf die Companion-Ausgabe (im unabhängigen Betrieb auf alle verbundenen Geräte). Für die Hauptanzeige gibt es den Schritt nur über die lokale Schnittstelle (`POST /api/pages/step`). Die früheren Knöpfe „Vorherige Ansicht“/„Nächste Ansicht“ unter **Übersicht** sind in der nativen Oberfläche nicht vorhanden. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.
