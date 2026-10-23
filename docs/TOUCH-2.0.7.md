# Touch-Ziele und Spielstunden in 2.0.7

[English](TOUCH-2.0.7.en.md)

**Das Freunde-Verhalten dieser Seite gilt ab 2.1.0 (die Änderung stammt aus 2.0.9).** Neu in 2.1.0: Auf der Home-Assistant-Seite schaltet ein Tipp auf eine Schalter-Kachel den Schalter um (nur mit aktivem Touch; Sensoren reagieren nicht auf Tippen). In [v2.0.8](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.8) öffnet ein Tipp auf einen Freund nur den Steam-Chat.

Unter **Display → Touch-Steuerung** Touch einschalten und **Speichern & anwenden** wählen. Bei vorhandenen Einstellungen bleibt ein ausgeschalteter Touch-Schalter ausgeschaltet. HDMI/USB-C benötigt zusätzlich die USB-Touchverbindung und die richtige Windows-Bildschirmzuordnung. Windows 10/11 x64 und WebView2 Evergreen für das Einstellungsfenster bleiben erforderlich.

- **Battlefield-6-Rangliste:** Einen Spieler antippen, um dessen GameTools-Profil im Standardbrowser zu öffnen. Plattform und Name stammen aus dem eingetragenen Konto; Sonderzeichen werden als URL-Bestandteil kodiert. Fehlerzeilen und die Ladeanzeige öffnen kein Profil. Die Rangliste zeigt zusätzlich die von der API gelieferten Spielstunden, auf ganze Stunden gerundet; fehlende Werte werden weggelassen. Rang, K/D und Siegquote bleiben erhalten.
- **Freunde und Discord-Freunde:** Eine Person antippen. Die Reihenfolge ist fest: **Steam-Chat**, sonst **Discord-Profil**, sonst **Xbox-Profil**. Steam und Discord werden nur bei registrierter Desktop-App aufgerufen; fehlt die App oder schlägt ihr Start fehl, folgt das nächste Ziel. Der Online-Status spielt keine Rolle.
- **Steam** öffnet den Privatchat. **Discord** öffnet das Profil der Person; dort **Nachricht** wählen (eine direkte Chat-Verknüpfung gibt es nicht, weil dem Bot die Direktnachrichten-Kanal-ID fehlt). **Xbox** öffnet das Profil auf xbox.com im Browser (kein Chat-Link verfügbar), ebenfalls mit Weg zu „Nachricht“. Ob ein eigenes Fenster, ein Tab oder ein vorhandenes Fenster verwendet wird, bestimmt die jeweilige App bzw. der Browser. Die Display-App sendet keine Nachricht.
- **Blogs:** Ein Beitrag öffnet weiter seinen HTTP-/HTTPS-Link. Wischen blättert weiterhin; Tippen auf freie Bereiche, Kopf-/Fußzeilen oder Einträge ohne Ziel verwendet die bisherige Seitennavigation.

Die Auswahl ist an den zuletzt gezeichneten Datenstand gebunden. Sortierung, Unterseiten, breite Kacheln und leere Zeilen der letzten Seite werden berücksichtigt. Neue Hintergrunddaten können daher nicht die Identität hinter einer bereits angezeigten Zeile ändern. Ein fehlgeschlagener App-Start blättert die Seite nicht zusätzlich um; `usb-display.log` enthält einen Hinweis ohne Konto-ID oder Ziel-URL.

## Wenn etwas nicht öffnet

| Beobachtung | Maßnahme |
|---|---|
| Wischen und Tippen reagieren nicht | Touch einschalten, USB-Touchverbindung und Bildschirmzuordnung prüfen. |
| Es öffnet sich Steam statt Discord | Gewollt: Steam hat Vorrang, wenn die Person ein Steam-Konto verknüpft hat. |
| Es öffnet sich nur das Profil | Bei Discord und Xbox ist das gewollt (kein Chat-Link verfügbar); dort „Nachricht“ wählen. Ohne Discord-Desktop-App wird Xbox verwendet, falls verknüpft. |
| Xbox öffnet das falsche Profil oder zeigt keine Nachrichten | Mit dem richtigen Microsoft-Konto anmelden, ursprünglichen Gamertag und Privatsphäre-/Kontaktberechtigungen prüfen. |
| Gar kein Ziel öffnet | Installierte Plattform-Apps, Windows-Standardbrowser und das lokale App-Protokoll prüfen. Die App kann einen erfolgreichen Start bestätigen, jedoch keine erfolgreiche Anmeldung oder Chatberechtigung. |

## Prüfung und technische Grundlage

Automatische Prüfungen verwenden ausschließlich Beispieldaten und ersetzte Startfunktionen: Es werden keine echten Chats oder Browser-Tabs geöffnet und keine Nachrichten gesendet. Build, Testzahlen und offene Abnahmen stehen im [Testprotokoll](../cpp/TESTPROTOKOLL.md). Physischer Touch und das Öffnen in angemeldeten Plattform-Clients müssen für 2.0.7 noch abgenommen werden. Frühere Testergebnisse gelten nicht automatisch für diesen Stand.

Die Profil-URL folgt der [GameTools-Webseite](https://github.com/community-network/gametools.network-website). Steam verwendet das [Steam-Browserprotokoll](https://developer.valvesoftware.com/wiki/Steam_browser_protocol). Discord verwendet den im [Discord-Dokumentationsrepository beschriebenen Profil-Link](https://github.com/discord/discord-api-docs/discussions/5183); er liefert keine garantierte Direktnachrichten- oder Fenstersteuerung. Xbox verwendet das [Xbox-Webprofil](https://account.xbox.com/Profile).
