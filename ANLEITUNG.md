# ScreenBuddy – Schnellstart

**Companion-Vorschau (ab 2.3.0):** USB/HDMI und Android gleichzeitig mit eigenen Seiten, Wechselintervallen und Touch-Zielen; automatische Handy-Auflösung einschließlich Rotation. Zusätzliche Einstellungen nur bei aktivem Companion. [Einrichtung und Grenzen](docs/COMPANION.md).

> **Bebilderte Anleitung:** Sie öffnet sich über **Anleitung öffnen** im Menü des Symbols im Infobereich im Standardbrowser und ist in der EXE enthalten. Sprache und Hell/Dunkel des Einstellungsfensters stellst du oben rechts um (DE | EN).

Diese App zeigt auf deinem kleinen USB-Display im Wechsel: **System** (Auslastung, Temperaturen, Netzwerk), **Discord-Freunde**, **Freunde von Steam, Xbox und Discord** in einer Liste, eine **Battlefield-6-Rangliste** und **Blog-Updates**. Du brauchst keine Vorkenntnisse.

## In 3 Minuten loslegen

1. **EXE herunterladen:** [aktueller Release](https://github.com/PapaFuchs84/ScreenBuddy/releases/latest), derzeit **2.9.1**. Die EXE ist unsigniert; kein ZIP entpacken. Mit `SHA256SUMS.txt` lässt sich die Datei prüfen.
2. Windows 10/11 x64 verwenden. Seit 2.5.0 wird keine Microsoft Edge WebView2 Runtime mehr benötigt (die frühere Version 2.3.0 braucht sie noch).
3. Display anschließen und `ScreenBuddy.exe` starten. Der Installer richtet die App unter `%LOCALAPPDATA%\FuchsLabs\USB-Display` ein. Er benötigt dafür keine Administratorrechte.
4. Bei der ersten Einrichtung öffnet sich ein **eigenes Windowsfenster** für die Einstellungen.

Die Einstellungen öffnest du später jederzeit mit einem **Doppelklick auf das Symbol im Infobereich** (unten rechts neben der Uhr, ggf. über den kleinen Pfeil ^) oder indem du die EXE noch einmal startest.

## Wenn das Display nicht angeht

| Du siehst | Das hilft |
|---|---|
| „Die Hersteller-App des Displays läuft und belegt den Anschluss“ | Unter **Updates & Autostart** auf **Hersteller-App beenden** klicken. Danach auf **Autostart der Hersteller-App abschalten**, damit sie nicht bei jedem Windows-Start wieder aufgeht (Windows fragt einmal nach). |
| „Display nicht gefunden“ | USB-Kabel prüfen, einen anderen USB-Anschluss am PC probieren. |
| Bild steht auf dem Kopf | **Anzeige → Ausrichtung** auf „Quer 180°“ stellen. |
| Display zu dunkel | **Anzeige → Helligkeit** erhöhen. |

## HDMI/USB-C und Touch

1. Bildschirm per HDMI oder USB-C anschließen und in Windows auf **Erweitern** stellen. Für Touch zusätzlich die USB-Touchverbindung herstellen.
2. Unter **Anzeige → Display-Typ** die Ausgabe **Monitor** und unter **Bildschirm** den vorgesehenen Monitor wählen, dann **Speichern & anwenden**. Die Anzeige bleibt über der Taskleiste, ohne den Tastaturfokus zu übernehmen.
3. Erscheinen Berührungen auf einem anderen Bildschirm, unter **Touch-Bildschirm zuordnen** den **Windows-Assistent öffnen**. Die Anzeige pausiert während des Windows-Assistenten.
4. Andere Bildschirme mit **Enter** überspringen. Steht der Assistententext auf dem gewünschten Touchscreen, diesen berühren. **Esc** bricht ab.
5. Danach **Erneut prüfen** wählen; bei Bedarf **Touch-Steuerung** einschalten und speichern. Nach links wischen = nächste Seite, nach rechts = vorherige; Tippen links = zurück, sonst weiter. Tippen auf einen Blogbeitrag öffnet ihn im Browser auf dem PC (ab 2.0.6).

Ohne erkannten Touchscreen ist der Einrichtungsbutton deaktiviert. Dann USB-Verbindung und Treiber prüfen. Die bisherige Prüfung nutzte injizierte Windows-Berührungen; echte Touch-Hardware und der vollständige Assistentenablauf bleiben offen.

## Touch-Ziele und Spielstunden (ab 2.0.8)

Entwickelt als 2.0.7; diese Version wurde nicht einzeln veröffentlicht, die Funktion steht ab 2.0.8 zur Verfügung. Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](docs/TOUCH-2.0.7.md). [Prüfstand](cpp/TESTPROTOKOLL.md).

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick darauf öffnet die Einstellungen), und **Einstellungen → Updates & Autostart** zeigt „Neue Version verfügbar: …“. **Release-Seite öffnen** führt zur Download-Seite, **Jetzt nach Updates suchen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Updates & Autostart → Automatisch nach Updates suchen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Home Assistant (ab 2.1.0)

Neue Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. http://homeassistant.local:8123), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in config.toml. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit http:// laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort https://, wenn Home Assistant es anbietet.

## Android Companion App koppeln und nutzen (ab 2.3.0)

Mit der Companion-App kannst du dein Android-Smartphone oder Tablet als zusätzliches Display nutzen – entweder parallel zum USB-/HDMI-Display mit komplett eigenen Seiten und Wechselintervallen oder als alleinige Anzeige.

### 1. Vorbereitung und Installation
1. **Netzwerk:** Smartphone und PC müssen im selben lokalen Heimnetz (WLAN) oder per USB-Tethering verbunden sein.
2. **APK installieren:** Die Companion-App 1.0.2 als separaten Download aus dem [aktuellen GitHub-Release](https://github.com/PapaFuchs84/ScreenBuddy/releases/latest) herunterladen und auf dem Android-Gerät installieren (Android 8.0 oder neuer). Eine ältere Debug-APK lässt sich nicht direkt aktualisieren; siehe [Companion-Einrichtung](docs/COMPANION.md).

### 2. Companion am PC aktivieren
1. In den Einstellungen der Windows-App die Seite **Companion** öffnen.
2. Den Schalter **Companion zusätzlich aktivieren** einschalten (**TLS-Port**, Standard: 8777).
3. Auf **Speichern & anwenden** klicken. Der TLS-gesicherte Hintergrunddienst startet.

### 3. Smartphone mit dem PC koppeln
1. In den Windows-Einstellungen unter **Companion → Gerät koppeln** auf **Kopplungscode anzeigen** klicken.
2. Auf der Seite erscheinen ein **QR-Code**, ein 6-stelliger **PIN-Code** (gültig für 2 Minuten) und der SHA-256-Fingerabdruck des Zertifikats.
3. In der Android-App auf **Koppeln** tippen:
   - **Per QR-Code:** Kamera-Berechtigung erteilen und den QR-Code am Monitor scannen. Adresse, Port, PIN und Fingerabdruck werden automatisch übernommen.
   - **Manuell:** Lokale IP-Adresse des PCs, Port (8777) und 6-stelligen PIN-Code eintippen.
4. Den in der App angezeigten **Zertifikats-Fingerabdruck** kurz mit dem am PC vergleichen und bestätigen.
5. Nach erfolgreicher Kopplung speichert das Smartphone den kryptografischen Schlüssel lokal ab. Künftige Verbindungen erfolgen automatisch ohne erneute PIN-Eingabe.

### 4. Eigene Seiten für das Smartphone festlegen
1. Unter **Companion → Seiten für Companion** die gewünschten Seiten für das Handy anhaken (▲/▼ sortiert) und den **Seitenwechsel** (3–600 Sekunden) einstellen. Ist **Geräte synchron anzeigen** ausgeschaltet, kann jedes gekoppelte Gerät unter **Seiten für das ausgewählte Gerät** eine eigene Liste erhalten.
2. Das USB-Display bzw. der HDMI-Monitor behält seine eigene Seitenliste und Anzeigedauer unter **Seiten & Reihenfolge** und **Anzeige → Anzeigedauer pro Seite**.
3. Datenquellen (System, Discord, Freunde, Battlefield, Blogs, Home Assistant) werden auf ihren jeweiligen Seiten aktiviert und konfiguriert; beide Anzeigen greifen gemeinsam darauf zu.

### 5. Bedienung auf dem Smartphone
- **Wischen:** Ein Wisch nach links wechselt zur nächsten Seite, nach rechts zur vorherigen Seite. Das Blättern wirkt nicht auf das Hauptdisplay. Bei synchroner Anzeige (Standard) wechseln alle gekoppelten Handys gemeinsam, bei ausgeschalteter Synchronisation jedes Gerät einzeln.
- **Tippen:** Tippen wird wie bei der Touchbedienung am Bildschirm an den PC gemeldet; Details und Grenzen: [docs/COMPANION.md](docs/COMPANION.md).
- **Automatische Auflösung & Drehung:** Beim Drehen zwischen Hochformat und Querformat meldet die App die native Bildschirmauflösung automatisch an den PC. Der PC rendert das Bild scharf und passgenau für die jeweilige Handyausrichtung.

Weitere technische Hintergründe und Grenzwerte: [docs/COMPANION.md](docs/COMPANION.md).


## Ansichten durchblättern (2.0.4)

Seit 2.0.4 lässt sich die laufende Anzeige per Knopfdruck weiterschalten, ohne zu speichern oder neu zu starten. In der nativen Oberfläche (ab 2.5.0) stehen dafür **Vorige Seite zeigen** und **Nächste Seite zeigen** unter **Einstellungen → Companion** bereit; sie wirken auf die Companion-Ausgabe. Für die Hauptanzeige gibt es den Schritt nur über die lokale Schnittstelle (`POST /api/pages/step`). Die früheren Knöpfe „Vorherige Ansicht“/„Nächste Ansicht“ unter **Übersicht** sind in der nativen Oberfläche nicht vorhanden. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

## Die Seiten einrichten

Alles ist optional, schalte nur ein, was du magst (**Seiten & Reihenfolge**: Haken zum Ein-/Ausblenden und ▲/▼ für die Reihenfolge).

- **System:** läuft sofort. Für CPU-Temperatur und Lüfter brauchst du das kostenlose Programm *HWiNFO* (siehe `DOKUMENTATION.md`, Kapitel „HWiNFO“).
- **Discord:** Die App braucht einen eigenen kleinen Bot, den du in 5 Minuten anlegst:
  1. <https://discord.com/developers/applications> öffnen → **New Application** → Name z. B. „Mein Display“.
  2. Links **Bot** → **Reset Token** → den Token kopieren. Weiter unten **Presence Intent** und **Server Members Intent** einschalten → Speichern.
  3. Links **OAuth2 → URL Generator** → Haken bei **bot** → die erzeugte Adresse öffnen und deinen Server auswählen.
  4. In der Einstellungsseite bei **Discord** den Token einfügen und mit **Speichern & anwenden** bestätigen. Danach kannst du unter **Personen auswählen** festlegen, wen du sehen willst.
- **Freunde (Steam, Xbox, Discord):** Bei **Freunde** die Schlüssel eintragen. Unter jedem Feld klappt eine kleine Hilfe auf, mit Knöpfen, die direkt die richtige Seite öffnen.
- **Battlefield 6:** Bei **Battlefield 6** die EA-Namen deiner Freunde eintragen und mit **Prüfen** testen. (EA gibt keine Freundesliste heraus, deshalb trägst du die Namen selbst ein.)
- **Blogs:** Bei **Blogs** die Adresse deiner Lieblingsblogs eintragen. Den Feed sucht die App selbst.
- **Streaming-Neustarts:** Bei **Streaming-Neustarts** einen kostenlosen TMDB-Key (themoviedb.org → Einstellungen → API) eintragen, **Key prüfen & Anbieter laden** wählen und deine Anbieter ankreuzen. Die Seite zeigt neue Filme und Serien der letzten 30 Tage. This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.

Nach jeder Änderung auf **Speichern & anwenden** klicken. Die Anzeige startet dann kurz neu.

## Beim Windows-Start automatisch starten

Auf der Seite **Updates & Autostart** den Schalter **Mit Windows starten** einschalten. Das geht ohne Administratorrechte und lässt sich dort auch wieder ausschalten.

## Beenden und Entfernen

- **Beenden:** Rechtsklick auf das Symbol im Infobereich → **Beenden**.
- **Entfernen:** In Windows **Einstellungen → Apps** den Eintrag **ScreenBuddy** (bis 2.3.0: **FuchsLabs USB-Display**) deinstallieren. Zugangsdaten und Einstellungen lassen sich auf Wunsch zusätzlich entfernen.
- **Fenster schließen:** Blendet nur die Einstellungen aus; die Anzeige läuft weiter.
- **Update von 1.x:** Der Installer kann die alte Konfiguration übernehmen. Der alte Ordner wird mit `-1.x-alt` gesichert. Die Übernahme ist an die erkannten bisherigen Speicherorte gebunden.

Mehr Details, Fehlerbehebung und technische Hintergründe: `DOKUMENTATION.md`.

© 2026 FuchsLabs
