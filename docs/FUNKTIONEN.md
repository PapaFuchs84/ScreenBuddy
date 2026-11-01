# ScreenBuddy · Funktionen im Detail

Aus dem README ausgelagert (Stand 2.9.4). Englisch: [FEATURES.en.md](FEATURES.en.md). Übersicht aller Dokumente: [docs/README.md](README.md).

## Inhalt

- [Systemseite per Touch und Update-Hinweis (ab 2.2.0)](#systemseite-per-touch-und-update-hinweis-ab-220)
- [Touch-Ziele und Spielstunden (ab 2.0.8)](#touch-ziele-und-spielstunden-ab-208)
- [Home Assistant (ab 2.1.0)](#home-assistant-ab-210)
- [Streaming-Neustarts (ab 2.8.0)](#streaming-neustarts-ab-280)
- [Spotify (ab 2.8.0)](#spotify-ab-280)
- [Hilfe für API-Keys und Tokens (ab 2.9.1)](#hilfe-für-api-keys-und-tokens-ab-291)
- [PlayStation-Freunde (ab 2.9.3)](#playstation-freunde-ab-293)
- [Farben & Design (ab 2.9.0)](#farben--design-ab-290)
- [Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)](#android-companion-app-ab-230-mehrere-geräte-ab-270)
- [Plugin-Store und Plugins (ab 2.6.0)](#plugin-store-und-plugins-ab-260)
- [Ansichten durchblättern (ab 2.0.4)](#ansichten-durchblättern-ab-204)

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick darauf öffnet die Einstellungen), und **Einstellungen → Updates & Autostart** zeigt „Neue Version verfügbar: …“. **Release-Seite öffnen** führt zur Download-Seite, **Jetzt nach Updates suchen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Updates & Autostart → Automatisch nach Updates suchen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Touch-Ziele und Spielstunden (ab 2.0.8)

Entwickelt als 2.0.7; diese Version wurde nicht einzeln veröffentlicht, die Funktion steht ab 2.0.8 zur Verfügung. Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com oder (ab 2.9.3) ihr PlayStation-Profil auf my.playstation.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](TOUCH-2.0.7.md). [Prüfstand](../cpp/TESTPROTOKOLL.md).

## Home Assistant (ab 2.1.0)

Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. `http://homeassistant.local:8123`), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml`. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit `http://` laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort `https://`, wenn Home Assistant es anbietet.

## Streaming-Neustarts (ab 2.8.0)

Seite **Streaming-Neustarts** („New on streaming“): zeigt Filme und Serien, die in den letzten 30 Tagen erschienen sind und bei den gewählten Anbietern im Abo (Flatrate) laufen. Pro Zeile Titel, Datum, Film oder Serie und das Logo des Anbieters (höchstens 3 je Titel, weitere als „+n“). Bis zu 6 Zeilen. Ab 2.9.0 fragt die App je Anbieter getrennt ab (bis zu 8 Gruppen) und wählt reihum je Anbieter den neuesten Titel, damit alle gewählten Anbieter erscheinen. Fehlt ein Logo oder lädt es noch, steht stattdessen das Kürzel (z. B. NF, PV, D+, ATV+, WOW, RTL+).

- **Einrichtung:** unter **Einstellungen → Streaming-Neustarts** die Seite einschalten und einen kostenlosen TMDB-Key eintragen (bei bestehender Installation die Seite zusätzlich unter *Seiten & Reihenfolge* anhaken) (themoviedb.org → Einstellungen → API; der v3-Key oder das „API Read Access Token“ funktionieren). Der Key liegt wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml` und wird nie protokolliert oder angezeigt. **Key prüfen & Anbieter laden** holt die Anbieterliste deiner Region aus TMDB; mit Haken wählst du die Anbieter (bis zu 30). Vorgewählt sind Netflix (8), Prime Video (9 und 119), Disney+ (337), Apple TV+ (350), WOW (30) und RTL+ (2750, Region DE). Weicht eine ID in deiner Region ab, korrigiere sie über die geladene Liste.
- **Region:** folgt der App-Sprache (Deutsch: DE, Englisch: US); im Feld **Region** (`region` unter `[streaming]`) mit zwei Buchstaben überschreibbar, z. B. `AT`.
- **Abruf:** nur GET über HTTPS an `api.themoviedb.org/3`. Je Aktualisierung zwei Suchanfragen (Filme, Serien) plus – nur für die angezeigten Einträge und einmal gemerkt – eine Anbieterabfrage je Eintrag; höchstens stündlich (Cache 1 Stunde), kleine Pausen zwischen den Anfragen. Bei Fehlern bleiben alte Einträge sichtbar, die App versucht es nach 10 Minuten (bei „Limit erreicht“ nach 15 Minuten, bei ungültigem Key erst zur nächsten Stunde) erneut.
- **Anbieter-Logos:** TMDB liefert zu jedem Anbieter einen `logo_path`; das Bild kommt von `image.tmdb.org` (Größe w92, nur GET über HTTPS, keine Weiterleitungen, Größenlimit, nur PNG/JPEG). Jedes Logo wird einmal im Hintergrund geladen und unter `cache/tmdb-logos/<Anbieter-ID>.png` im Datenordner abgelegt (Dateiname nur aus der Zahl), dann im Speicher als abgerundetes Quadrat auf Zeilenhöhe skaliert. Das Display wartet nie darauf; bis dahin oder bei Fehlern steht das Kürzel. Die Logos sind Marken der Anbieter ([THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md)). TMDB sieht dafür zusätzlich den Bildabruf.
- **Anzeige ohne Daten:** „Kein Key“ (Seite an, aber kein Key), „Lade Daten …“, „Keine Daten“ (Fehler oder offline, nie ein Absturz).
- **Datenschutz:** TMDB sieht deine IP-Adresse und die gewählte Region/Anbieterliste, nicht dein Konto bei den Streaming-Diensten.
- **Nachweis:** This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch. Die Seite trägt die Fußzeile „TMDB / JustWatch“; der Hinweis steht auch in den Einstellungen (Updates & Autostart → Nachweise) und in [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md). Das offizielle TMDB-Logo steht in der Quellenangabe von README und THIRD-PARTY-NOTICES.md (ab 2.9.0).

## Spotify (ab 2.8.0)

Zeigt den aktuell gespielten Titel mit Cover und Fortschritt; mit Touch (Monitor, Companion) steuern Tasten Zurück, Wiedergabe/Pause und Weiter (Spotify Premium nötig). Anmeldung per PKCE mit eigener Spotify-Developer-App, das Token liegt DPAPI-verschlüsselt in spotify.token. Bei laufender Wiedergabe zeigt ein Amplifier (16 Balken) den Pegel der Windows-Systemausgabe, lokal gelesen und nicht aufgezeichnet (abschaltbar). Läuft die Musik woanders (Handy, Lautsprecher, Connect) und die Systemausgabe bleibt 4 s still, zeigt er eine als „simuliert“ gekennzeichnete Animation. Die Balkenfarbe ist wählbar (Pegel Grün/Gelb/Rot, Verlauf im Design, einfarbig). Ein Wechsel des Standard-Audiogeräts wird erkannt. Einrichtung und Grenzen: [SPOTIFY.md](SPOTIFY.md). Ab 2.9.0 startet die Analyse erst beim Zeichnen der sichtbaren Wiedergabeseite und endet nach dem Seitenwechsel nach höchstens drei Sekunden. Ab 2.9.1 ist bei Remote-Wiedergabe die simulierte Anzeige verfügbar.

## Hilfe für API-Keys und Tokens (ab 2.9.1)

Überall, wo ScreenBuddy einen Zugang braucht, steht in den Einstellungen unter dem Feld eine Karte **„So bekommst du …“** mit kurzen nummerierten Schritten und Knöpfen, die die offizielle Seite des Dienstes im Browser öffnen: Discord (Bot-Token), Steam (Web-API-Schlüssel und Datenschutz), Xbox (OpenXBL), TMDB (API-Key), Spotify (Client-ID) und Home Assistant (langlebiges Zugriffstoken). Geöffnet werden nur fest hinterlegte https-Adressen der Anbieter (`cpp/src/key_help.cpp`). Die App liest, protokolliert oder wiederholt dabei keine Schlüssel; eingefügte Werte werden wie bisher verschlüsselt gespeichert. Battlefield 6 und Blogs brauchen keinen Schlüssel. Für den optionalen PlayStation-Code gibt es die Karte **„So bekommst du den PlayStation-Code (NPSSO)“** mit den Schritten und zwei Knöpfen (playstation.com und die Code-Seite von Sony nach der Anmeldung); Details im Abschnitt [PlayStation-Freunde](#playstation-freunde-ab-293).

## PlayStation-Freunde (ab 2.9.3)

**Inoffiziell, optional, standardmäßig aus, auf eigenes Risiko.** Sony bietet für Freundeslisten keine offizielle Schnittstelle. ScreenBuddy nutzt deshalb denselben Weg wie die PlayStation-App: Der NPSSO-Code der angemeldeten Browser-Sitzung wird gegen ein kurzlebiges Zugriffstoken getauscht (dafür sind die öffentlich bekannten Anmeldekennungen der Sony-Android-App eingebaut). Sony kann diese Schnittstelle jederzeit ändern oder sperren und das Konto einschränken; die Nutzung liegt in deiner Verantwortung.

- **Einrichtung:** **Einstellungen → Freunde**, Karte **PlayStation**. Die Hilfe **„So bekommst du den PlayStation-Code (NPSSO)“** führt in wenigen Schritten zum Code: bei playstation.com anmelden, im selben Browser die Code-Seite von Sony öffnen, den Code (oder die ganze Zeile) ins Feld **PlayStation-Code (NPSSO)** einfügen, **Verbindung prüfen**, dann **Speichern & anwenden**. Ohne gespeicherten Code ist PlayStation aus und es werden keine Anfragen an Sony gesendet.
- **Code:** 64 Zeichen, wirkt wie ein Passwort und läuft nach etwa zwei Monaten ab; dann einfach einen neuen holen. Er wird wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml` gespeichert, nie protokolliert, nie an den Browser ausgeliefert und nur an Sony gesendet. Auch die ganze Antwort der Code-Seite (`{"npsso":"…"}`) wird akzeptiert; Sonys Autorisierungscode mit Punkt (`v3.…`) wird bei der Anmeldung angenommen. Die Verbindungsdiagnose steht im Log (ohne den Code).
- **Anzeige:** PlayStation-Freunde erscheinen in der gemeinsamen Freundesliste mit der Marke **P**, Online-Status und gespieltem Spiel. Abruf alle 2 Minuten, höchstens 200 Freunde; Avatare werden nur von Sony-Domains über HTTPS geladen. Dieselbe Person auf mehreren Plattformen erscheint nur einmal; für Personen mit verschiedenen Namen unter **Personen zusammenführen** verknüpfen oder die Zeile `Name|…|psn:Online-ID` ergänzen.
- **Touch:** Ein Tipp auf einen Freund öffnet bevorzugt Steam-Chat, Discord- und Xbox-Profil; hat die Person nur PlayStation, öffnet sich ihr Profil auf my.playstation.com im Browser.
- **Prüfstand:** Die Auswertung der Sony-Antworten ist mit festen Beispieldaten automatisiert getestet (`fl-psn-tests`). Live gegen Sony: mit frischem Code lieferte die Freundesliste in 2.9.3 ein PlayStation-Konto ohne Fehler (siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md)); Online-Status und Spiel wurden nicht auf den Geräten gesichtet.

## Farben & Design (ab 2.9.0)

**Einstellungen → Farben & Design:** Hell (FuchsLabs), Dunkel (FuchsLabs), **An Windows anpassen** oder eigene Themes. Windows-Modus: Hell/Dunkel und Akzent von Windows, Änderungen wirken live, bei hohem Kontrast nutzt das Display die Systemfarben; Text und Balken werden automatisch kontrastkorrigiert. Eigene Themes sind JSON-Dateien (Editor mit Live-Vorschau, Kontrastwarnung, Duplizieren, Löschen, Import, Export); sechs Beispiele (Cupertino-, Material- und Fluent-Stil, jeweils hell und dunkel) liegen in `cpp/examples/themes`. Ab 2.9.1 lassen sich Store-Themes installieren (seit 2.9.5 direkt hier in der Karte „Themes aus dem Store“, nicht mehr im Plugin-Store), ab 2.9.2 gibt es zusätzlich Designsysteme mit eigenen Fensteraufbauten. Die Wahl gilt für USB-Display, Bildschirm und Companion. Details, Format und Grenzen: [THEMES.md](THEMES.md) (zweisprachig).

**Statuspunkte in der Navigation:** grün = eingerichtet und an, grau = nicht eingerichtet oder aus; nur für die Seiten (Datenquellen) und Companion, mit Legende. Die Navigation hat seit 2.9.5 die Gruppen **Ansicht**, **Seiten** und **App** (dort Companion, Plugins, Plugin-Store, Updates & Autostart); Farben & Design steht unter **Ansicht**. Seit 2.9.1 erklärt ein Hinweis beim Verweilen mit der Maus auf dem Punkt den Zustand.

## Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)

Ein Android-Handy oder -Tablet als zusätzliche Anzeige neben dem USB-/HDMI-Bildschirm. Ab 2.3.0 gab es den Companion als Vorschau-APK; die aktuelle Companion-App 1.0.3 (versionCode 7, Android 8+) ist ein separater Download. Seit 2.7.0 laufen mehrere Companions gleichzeitig, **synchron oder unabhängig**.

- **Eigene Seiten und Zeitgeber:** Der Companion hat gegenüber USB/HDMI eine eigene Seitenliste und ein eigenes Intervall (unter **Companion**). Im unabhängigen Modus (`sync = false`) hat zusätzlich jedes Gerät eigene Seitenposition, Auflösung, Bedienung und optional eine eigene Seitenliste. Datenquellen und Design bleiben gemeinsam. Standard ist die synchrone Anzeige.
- **Kopplung per QR-Code oder PIN:** Den Kopplungscode zeigst du unter **Einstellungen → Companion → Gerät koppeln**. Die Android-App scannt den QR-Code oder verbindet sich mit Adresse, Port (Standard 8777) und 6-stelliger PIN; ein SHA-256-Zertifikat-Fingerabdruck sichert die TLS-Verbindung.
- **Touch und Gesten:** Im unabhängigen Modus blättert ein Wisch nur dieses Gerät, im synchronen alle. Tippen und Wischen beschreiben die [Companion-Hinweise](COMPANION.md).
- **Native Auflösung und Drehung:** Das Handy meldet Ausrichtung und Auflösung, der PC rendert passende Bilder.
- [Einrichtung und Grenzen](COMPANION.md) · [Protokoll](COMPANION-PROTOKOLL.md)

Prüfstand: siehe [Release-Notizen 2.7](RELEASE-NOTES-2.7.md). Die automatisierten Mehrgeräte-Prüfungen sind Simulationen; adb-Gesten ersetzen keine Abnahme mit dem Finger. Beim Wechsel von einer Debug-APK unterscheidet sich die Signatur, ein direktes Update ist nicht möglich (siehe [Release-Notizen 2.6](RELEASE-NOTES-2.6.md)). Die Android-App nie automatisch deinstallieren: lokale Daten und Kopplungen gingen verloren.

## Plugin-Store und Plugins (ab 2.6.0)

Einstellungsseite **Plugin-Store** (Gruppe **App**, neben Companion und Plugins, abschaltbar mit `[ui] plugin_store`): geprüfte Daten-Plugins aus dem signierten Store `PapaFuchs84/ScreenBuddy-Plugins` suchen, prüfen und installieren. Signierter Index und signierte Sperrliste (Ed25519, fest hinterlegter öffentlicher Schlüssel), Pakete per SHA-256 und Host-Angaben geprüft, täglicher Hintergrundabgleich. Vor der Installation zeigt die Karte, an welche Hosts ein Plugin Daten senden würde. Installierte Plugins bleiben aus, bis du sie unter **Plugins** einschaltest; jeder Host braucht deine Zustimmung. Seit 2.7.0 bleiben Store-Installationen und -Updates auch ohne Netzquellen ausgeschaltet; der direkte Import eines lokalen Manifests verhält sich wie bisher. Aktivierte Plugins lassen sich unter **Seiten & Reihenfolge** sortieren und ein-/ausblenden. Der Store zeigt signierte Vorschaubilder.

Mehr: [Plugin-System](PLUGIN-SYSTEM.md) · [Tutorial](PLUGIN-TUTORIAL.md) · [Store-Bedingungen](PLUGIN-STORE-BEDINGUNGEN.md) · [Beispiele](../cpp/examples/plugins/README.md). Online-Plugins und CRL/OCSP-Prüfungen bleiben experimentell.

## Ansichten durchblättern (ab 2.0.4)

Seit 2.0.4 lässt sich die laufende Anzeige per Knopfdruck weiterschalten, ohne zu speichern oder neu zu starten. In der nativen Oberfläche (ab 2.5.0) stehen dafür **Vorige Seite zeigen** und **Nächste Seite zeigen** unter **Einstellungen → Companion** bereit; sie wirken auf die Companion-Ausgabe (im unabhängigen Modus auf alle verbundenen Geräte). Für die Hauptanzeige gibt es den Schritt nur über die lokale Schnittstelle (`POST /api/pages/step`). Die früheren Knöpfe „Vorherige Ansicht“/„Nächste Ansicht“ unter **Übersicht** gibt es in der nativen Oberfläche nicht. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

## Streaming- und Spotify-Touch (2.9.4)

Bei aktivem Touch öffnet ein Streaming-Titel seine TMDB-Detailseite am PC im Standardbrowser. Auf Meanmachine mit Chrome als neuer Tab geprüft; Fenster-/Tab-Verhalten richtet sich nach dem Browser. Ein Tipp auf das geladene Spotify-Cover öffnet die Spotify-App am PC, auch ohne Premium. Ohne Cover ist die Aktion inaktiv; bei fehlender App bleibt ScreenBuddy stabil. Die vorhandenen Wiedergabetasten bleiben getrennt. Die TMDB-Hilfe erklärt die Beantragung mit Feldbeispielen.
