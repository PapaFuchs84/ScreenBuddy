# FuchsLabs USB-Display – Schnellstart

> **Am einfachsten:** `ANLEITUNG.html` doppelklicken (englisch: `GUIDE.html`; die App lässt sich unter **Start › Sprache · Language** auf Englisch umstellen). Das ist die bebilderte Anleitung, Schritt für Schritt mit Weiter-Knopf. Diese Datei hier ist die Kurzfassung.

Diese App zeigt auf deinem kleinen USB-Display im Wechsel: **System** (Auslastung, Temperaturen, Netzwerk), **Discord-Freunde**, **Freunde von Steam, Xbox und Discord** in einer Liste, eine **Battlefield-6-Rangliste** und **Blog-Updates**. Du brauchst keine Vorkenntnisse.

## In 3 Minuten loslegen

1. **Ordner entpacken** und an einen festen Platz legen, z. B. `Dokumente\FuchsLabs-USB-Display`.
2. **Display anstecken** (USB).
3. **`FuchsLabs-USB-Display.exe` doppelklicken.**
   - Windows meldet eventuell „Der Computer wurde durch Windows geschützt“. Das ist normal, weil das Programm nicht signiert ist: auf **Weitere Informationen → Trotzdem ausführen** klicken.
4. Es öffnet sich automatisch die **Einstellungsseite** im Browser. Dort steht oben, ob dein Display erkannt wurde, und du stellst alles Weitere per Klick ein.

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
- **Entfernen:** Autostart ausschalten, App beenden, Ordner löschen. Es bleibt nichts weiter auf dem PC zurück (Einstellungen liegen im Ordner der App).

Mehr Details, Fehlerbehebung und technische Hintergründe: `DOKUMENTATION.md`.

© 2026 FuchsLabs
