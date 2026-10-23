# Touch-Ziele und Spielstunden in 2.0.7

[English](TOUCH-2.0.7.en.md)

**Lokaler Entwicklungsstand, noch nicht veröffentlicht oder installiert.** Der aktuelle Download bleibt [v2.0.6](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.6).

Unter **Display → Touch-Steuerung** Touch einschalten und **Speichern & anwenden** wählen. Bei vorhandenen Einstellungen bleibt ein ausgeschalteter Touch-Schalter ausgeschaltet. HDMI/USB-C benötigt zusätzlich die USB-Touchverbindung und die richtige Windows-Bildschirmzuordnung. Windows 10/11 x64 und WebView2 Evergreen für das Einstellungsfenster bleiben erforderlich.

- **Battlefield-6-Rangliste:** Einen Spieler antippen, um dessen GameTools-Profil im Standardbrowser zu öffnen. Plattform und Name stammen aus dem eingetragenen Konto; Sonderzeichen werden als URL-Bestandteil kodiert. Fehlerzeilen und die Ladeanzeige öffnen kein Profil. Die Rangliste zeigt zusätzlich die von der API gelieferten Spielstunden, auf ganze Stunden gerundet; fehlende Werte werden weggelassen. Rang, K/D und Siegquote bleiben erhalten.
- **Freunde und Discord-Freunde:** Eine Person antippen. Zuerst werden online erreichbare verknüpfte Konten geprüft, danach die übrigen. Innerhalb beider Gruppen gilt **Discord → Steam → Xbox**. Discord/Steam werden nur bei registrierter Desktop-App aufgerufen. Fehlt die App oder schlägt ihr Start fehl, folgt der nächste Eintrag.
- **Steam** öffnet den Privatchat. **Discord** und **Xbox** öffnen ab 2.0.8 nichts mehr: Für Discord fehlt die Direktnachrichten-Kanal-ID, für Xbox gibt es keinen Chat-Link; die Profil-Variante aus 2.0.7 entfällt auf Wunsch des Nutzers. Ob ein eigenes Fenster, ein Tab oder ein vorhandenes Fenster verwendet wird, bestimmt die jeweilige App bzw. der Browser. Die Display-App sendet keine Nachricht.
- **Blogs:** Ein Beitrag öffnet weiter seinen HTTP-/HTTPS-Link. Wischen blättert weiterhin; Tippen auf freie Bereiche, Kopf-/Fußzeilen oder Einträge ohne Ziel verwendet die bisherige Seitennavigation.

Die Auswahl ist an den zuletzt gezeichneten Datenstand gebunden. Sortierung, Unterseiten, breite Kacheln und leere Zeilen der letzten Seite werden berücksichtigt. Neue Hintergrunddaten können daher nicht die Identität hinter einer bereits angezeigten Zeile ändern. Ein fehlgeschlagener App-Start blättert die Seite nicht zusätzlich um; `usb-display.log` enthält einen Hinweis ohne Konto-ID oder Ziel-URL.

## Wenn etwas nicht öffnet

| Beobachtung | Maßnahme |
|---|---|
| Wischen und Tippen reagieren nicht | Touch einschalten, USB-Touchverbindung und Bildschirmzuordnung prüfen. |
| Es öffnet sich Steam statt Discord | Discord-Desktop-App und deren Windows-Linkzuordnung prüfen; auch der Online-Status und die Kontenverknüpfung bestimmen die Reihenfolge. |
| Es öffnet sich nur das Profil | Bei Discord und Xbox ist das die bestätigte Variante; dort „Nachricht“ wählen. |
| Xbox öffnet das falsche Profil oder zeigt keine Nachrichten | Mit dem richtigen Microsoft-Konto anmelden, ursprünglichen Gamertag und Privatsphäre-/Kontaktberechtigungen prüfen. |
| Gar kein Ziel öffnet | Installierte Plattform-Apps, Windows-Standardbrowser und das lokale App-Protokoll prüfen. Die App kann einen erfolgreichen Start bestätigen, jedoch keine erfolgreiche Anmeldung oder Chatberechtigung. |

## Prüfung und technische Grundlage

Automatische Prüfungen verwenden ausschließlich Beispieldaten und ersetzte Startfunktionen: Es werden keine echten Chats oder Browser-Tabs geöffnet und keine Nachrichten gesendet. Build, Testzahlen und offene Abnahmen stehen im [Testprotokoll](../cpp/TESTPROTOKOLL.md). Physischer Touch und das Öffnen in angemeldeten Plattform-Clients müssen für 2.0.7 noch abgenommen werden. Frühere Testergebnisse gelten nicht automatisch für diesen Stand.

Die Profil-URL folgt der [GameTools-Webseite](https://github.com/community-network/gametools.network-website). Steam verwendet das [Steam-Browserprotokoll](https://developer.valvesoftware.com/wiki/Steam_browser_protocol). Discord verwendet den im [Discord-Dokumentationsrepository beschriebenen Profil-Link](https://github.com/discord/discord-api-docs/discussions/5183); er liefert keine garantierte Direktnachrichten- oder Fenstersteuerung. Xbox verwendet das [Xbox-Webprofil](https://account.xbox.com/Profile).
