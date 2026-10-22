# FuchsLabs USB-Display – Schnellstart

> **Version 2.0:** Die bebilderte Anleitung öffnet sich über **Bebilderte Anleitung** im eigenen Einstellungsfenster und ist in der EXE enthalten. Sprache unter **Start › Sprache · Language** umschalten.

Diese App zeigt auf deinem kleinen USB-Display im Wechsel: **System** (Auslastung, Temperaturen, Netzwerk), **Discord-Freunde**, **Freunde von Steam, Xbox und Discord** in einer Liste, eine **Battlefield-6-Rangliste** und **Blog-Updates**. Du brauchst keine Vorkenntnisse.

## In 3 Minuten loslegen

1. **EXE herunterladen:** [Version 2.0.0](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.0). Es ist eine unsignierte Vorabversion; kein ZIP entpacken.
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
