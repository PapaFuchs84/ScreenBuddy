# FuchsLabs USB-Display – Schnellstart

**Unveröffentlichter Companion-Prototyp:** USB/HDMI und Android gleichzeitig mit eigenen Seiten, Wechselintervallen und Touch-Zielen; automatische Handy-Auflösung einschließlich Rotation. Zusätzliche Einstellungen nur bei aktivem Companion. [Einrichtung und Grenzen](docs/COMPANION.md).

> **Version 2.1.0:** Die bebilderte Anleitung öffnet sich über **Bebilderte Anleitung** in den Einstellungen oder **Anleitung öffnen** im Tray im Standardbrowser. Sie ist in der EXE enthalten. Sprache unter **Start › Sprache · Language** umschalten.

Diese App zeigt auf deinem kleinen USB-Display im Wechsel: **System** (Auslastung, Temperaturen, Netzwerk), **Discord-Freunde**, **Freunde von Steam, Xbox und Discord** in einer Liste, eine **Battlefield-6-Rangliste** und **Blog-Updates**. Du brauchst keine Vorkenntnisse.

## In 3 Minuten loslegen

1. **EXE herunterladen:** [aktueller Release](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/latest), derzeit **2.1.0**. Die EXE ist unsigniert; kein ZIP entpacken. Mit `SHA256SUMS.txt` lässt sich die Datei prüfen.
2. Windows 10/11 x64 mit **Microsoft Edge WebView2 Evergreen Runtime** verwenden. Die Laufzeit muss vorhanden sein; sie wird nicht mitgeliefert oder automatisch installiert.
3. Display anschließen und `FuchsLabs-USB-Display.exe` starten. Der Installer richtet die App unter `%LOCALAPPDATA%\FuchsLabs\USB-Display` ein. Er benötigt dafür keine Administratorrechte.
4. Bei der ersten Einrichtung öffnet sich ein **eigenes Windowsfenster** für die Einstellungen.

Die Einstellungen öffnest du später jederzeit mit einem **Doppelklick auf das Symbol im Infobereich** (unten rechts neben der Uhr, ggf. über den kleinen Pfeil ^) oder indem du die EXE noch einmal startest.

## Wenn das Display nicht angeht

| Du siehst | Das hilft |
|---|---|
| „Die Hersteller-App des Displays läuft und belegt den Anschluss“ | Auf der Seite **Start** auf **Hersteller-App beenden** klicken. Danach auf **Abschalten**, damit sie nicht bei jedem Windows-Start wieder aufgeht (Windows fragt einmal nach). |
| „Display nicht gefunden“ | USB-Kabel prüfen, einen anderen USB-Anschluss am PC probieren. |
| Bild steht auf dem Kopf | **Display → Ausrichtung** auf „Querformat, gedreht“ stellen. |
| Display zu dunkel | **Übersicht → Helligkeit** erhöhen. |

## HDMI/USB-C und Touch

1. Bildschirm per HDMI oder USB-C anschließen und in Windows auf **Erweitern** stellen. Für Touch zusätzlich die USB-Touchverbindung herstellen.
2. Unter **Display → Display-Typ** die Ausgabe **Bildschirm über HDMI oder USB-C** und den vorgesehenen Monitor wählen, dann **Speichern & anwenden**. Die Anzeige bleibt über der Taskleiste, ohne den Tastaturfokus zu übernehmen.
3. Erscheinen Berührungen auf einem anderen Bildschirm, **Touch-Bildschirm zuordnen** wählen. Die Anzeige pausiert während des Windows-Assistenten.
4. Andere Bildschirme mit **Enter** überspringen. Steht der Assistententext auf dem gewünschten Touchscreen, diesen berühren. **Esc** bricht ab.
5. Danach **Erneut prüfen** wählen; bei Bedarf **Touch-Steuerung** einschalten und speichern. Nach links wischen = nächste Seite, nach rechts = vorherige; Tippen links = zurück, sonst weiter. Tippen auf einen Blogbeitrag öffnet ihn im Browser auf dem PC (ab 2.0.6).

Ohne erkannten Touchscreen ist der Einrichtungsbutton deaktiviert. Dann USB-Verbindung und Treiber prüfen. Die bisherige Prüfung nutzte injizierte Windows-Berührungen; echte Touch-Hardware und der vollständige Assistentenablauf bleiben offen.

## Touch-Ziele und Spielstunden (ab 2.0.7)

Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](docs/TOUCH-2.0.7.md). [Prüfstand](cpp/TESTPROTOKOLL.md).

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick öffnet die Einstellungen), und **Einstellungen → Start** zeigt „Neue Version … verfügbar“ mit Link zur Download-Seite. **Jetzt prüfen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Start → Auf neue Versionen hinweisen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Home Assistant (ab 2.1.0)

Neue Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. http://homeassistant.local:8123), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in config.toml. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit http:// laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort https://, wenn Home Assistant es anbietet.

## Ansichten durchblättern (2.0.4)

Unter **Einstellungen → Übersicht** kannst du mit **Vorherige Ansicht** und **Nächste Ansicht** direkt durch die laufenden Display-Ansichten blättern. Die Position und die sichtbaren Seiten stehen unter der Live-Vorschau. Lange Listen blättern mit ihren Unterseiten, breite Displays wechseln wie bei der Touchbedienung um eine Kachelgruppe. Nach einem manuellen Wechsel beginnt die eingestellte Anzeigedauer neu; danach läuft der automatische Wechsel weiter. Bei keiner oder nur einer vollständig sichtbaren Ansicht sind die Knöpfe deaktiviert. Dafür ist kein Speichern oder Neustart nötig. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

## Die Seiten einrichten

Alles ist optional, schalte nur ein, was du magst (**Übersicht**: Schalter und Pfeile für Reihenfolge).

- **System:** läuft sofort. Für CPU-Temperatur und Lüfter brauchst du das kostenlose Programm *HWiNFO* (siehe `DOKUMENTATION.md`, Kapitel „HWiNFO“).
- **Discord:** Die App braucht einen eigenen kleinen Bot, den du in 5 Minuten anlegst:
  1. <https://discord.com/developers/applications> öffnen → **New Application** → Name z. B. „Mein Display“.
  2. Links **Bot** → **Reset Token** → den Token kopieren. Weiter unten **Presence Intent** und **Server Members Intent** einschalten → Speichern.
  3. Links **OAuth2 → URL Generator** → Haken bei **bot** → die erzeugte Adresse öffnen und deinen Server auswählen.
  4. In der Einstellungsseite bei **Discord** den Token einfügen und mit **Speichern & anwenden** bestätigen. Danach kannst du bei **Personen** auswählen, wen du sehen willst.
- **Freunde (Steam, Xbox, Discord):** Bei **Freunde** die Schlüssel eintragen. Unter jedem Feld klappt eine kleine Hilfe auf, mit Knöpfen, die direkt die richtige Seite öffnen.
- **Battlefield 6:** Bei **Battlefield 6** die EA-Namen deiner Freunde eintragen und mit **Prüfen** testen. (EA gibt keine Freundesliste heraus, deshalb trägst du die Namen selbst ein.)
- **Blogs:** Bei **Blogs** die Adresse deiner Lieblingsblogs eintragen. Den Feed sucht die App selbst.

Nach jeder Änderung auf **Speichern & anwenden** klicken. Die Anzeige startet dann etwa 10 Sekunden neu.

## Beim Windows-Start automatisch starten

Auf der Seite **Start** den Schalter **Mit Windows starten** einschalten. Das geht ohne Administratorrechte und lässt sich dort auch wieder ausschalten.

## Beenden und Entfernen

- **Beenden:** Rechtsklick auf das Symbol im Infobereich → **Beenden**.
- **Entfernen:** In Windows **Einstellungen → Apps** den Eintrag **FuchsLabs USB-Display** deinstallieren. Zugangsdaten und Einstellungen lassen sich auf Wunsch zusätzlich entfernen.
- **Fenster schließen:** Blendet nur die Einstellungen aus; die Anzeige läuft weiter.
- **Update von 1.x:** Der Installer kann die alte Konfiguration übernehmen. Der alte Ordner wird mit `-1.x-alt` gesichert. Die Übernahme ist an die erkannten bisherigen Speicherorte gebunden.

Mehr Details, Fehlerbehebung und technische Hintergründe: `DOKUMENTATION.md`.

© 2026 FuchsLabs
