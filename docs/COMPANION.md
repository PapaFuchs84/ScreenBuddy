## Companion-App (ab 2.3.0): unabhängige Anzeigen

**Stand 30.10.2026:** Die Companion-App 1.0 ist ein separater Download und nicht Teil der Windows-EXE. Der Gerätetest am Redmi 8 über USB-Tethering vom 27.10.2026 (Quer-/Hochformat, echte Fingerbedienung, automatischer Wechsel und Wiederverbindung ohne neue PIN) galt dem Vorgänger 0.2.1-preview (versionCode 3, mit neuem RSA-3072-Release-Schlüssel signiert, APK-v2-Signatur geprüft). Für 1.0 ist im [Testprotokoll](../cpp/TESTPROTOKOLL.md) kein eigener Gerätetest dokumentiert. Die bisherige Debug-APK hat eine andere Signatur und lässt sich nicht direkt damit aktualisieren. Nicht automatisch deinstallieren: lokale Daten und Kopplungen würden gelöscht. [Build, Signierung und Migration](../android/README.md). Frühere Geräteergebnisse unten gelten für 0.2.0-preview.

USB-Display bzw. HDMI-Bildschirm und Android-Companion können gleichzeitig laufen. Beide haben eigene Seitenlisten, Seitenpositionen, Wechselintervalle und Touch-Ziele. Datenquellen, Konten und Design bleiben gemeinsam. Eine Seite muss in ihrem Datenquellen-Reiter eingeschaltet und eingerichtet sein, bevor sie auf einer Anzeige erscheinen kann.

1. Den bisherigen Display-Typ beibehalten; unter **Companion** den Dienst einschalten und **Speichern & anwenden** wählen.
2. Auf dem Android-Gerät die Companion-App 1.0 installieren (separater Download). PC und Handy müssen sich im selben privaten IPv4-Netz erreichen können; USB-Tethering ist ebenfalls möglich.
3. Unter **Companion → Gerät koppeln** den Code anzeigen und in der Handy-App per QR-Code oder Adresse, Port und PIN koppeln; Zertifikat-Fingerabdruck vergleichen. PIN nur lokal verwenden, nicht in Berichten veröffentlichen.
4. Unter **Companion → Unabhängige Companion-Anzeige** Seiten wählen und die Anzeigedauer einstellen (3–600 Sekunden). Die Pfeile schalten ausschließlich die Handy-Ansicht. Die Reihenfolge folgt der gespeicherten Auswahl; erneut angehakte Seiten stehen am Ende.
5. Unter **Display → Seiten auf USB-Display / Bildschirm** die Auswahl für die andere Anzeige festlegen. Diese zusätzlichen Einstellungen erscheinen nur, wenn Companion eingeschaltet oder als Display-Typ gewählt ist. Ohne Companion gilt die bisherige Seitensteuerung.

Das Handy meldet seine tatsächliche Bildfläche bei Verbindung und Rotation automatisch. Der PC rendert in dieser Auflösung: beispielsweise 1520 × 720 im Redmi-Querformat, ohne feste Gerätewerte in der Config. Als Speichergrenze gelten maximal 2560 Pixel je Kante und 4.194.304 Pixel; größere Flächen werden proportional verkleinert. Mit `[companion] sync = true` (Standard) bestimmt das zuerst verbundene Gerät mit Größenmeldung die gemeinsame Companion-Auflösung; weitere Handys teilen dessen Seitenzustand und skalieren mit erhaltenem Seitenverhältnis. Mit `sync = false` arbeitet jedes Handy unabhängig (siehe unten). Nach dessen Trennung kann eine Größenmeldung eines verbleibenden bzw. neu verbundenen Geräts die Größe übernehmen.

Wischen nach links = nächste Seite, nach rechts = vorige. Antippen trifft die Ziele des jeweiligen Ausgabebildes; während einer Größenumstellung werden veraltete Koordinaten verworfen. Breite Flächen zeigen wie bisher gegebenenfalls mehrere Seiten nebeneinander. Leere Seitenauswahl zeigt den Willkommensbildschirm.

Konfiguration (Auszug, vorhandene Datenquellen bleiben erforderlich):

```toml
[display]
output = "serial"
visible_pages = ["system", "friends"]
page_seconds = 30

[companion]
enabled = true
port = 8777
pages = ["rss", "system"]
page_seconds = 5
```

Nur Handy: `[display] output = "network"`; Seiten und Wechselrhythmus kommen dann ebenfalls aus `[companion]`. Der Dienst bleibt standardmäßig aus. Port 8765 sowie der aktuelle Einstellungsport sind für Companion gesperrt. Kopplungen und Schlüssel bleiben lokal und werden nicht ins Repository übernommen.

Prüfstand und Grenzen: siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md). Der Vorgänger 0.2.1-preview wurde auf dem echten Redmi 8 installiert und über USB-Tethering geprüft: native Hoch-/Querformat-Ausgabe, Android-injizierte Wisch-/Tippgesten, automatischer Wechsel und Wiederverbindung mit gespeichertem Schlüssel erfolgreich. Der Nutzer hat scharfes Bild sowie langsames/normales Wischen und Tippen mit dem Finger bestätigt. Physischer USB-Betrieb in derselben neuen PC-Instanz, WLAN, Start beim Android-Boot und mehrere echte Geräte bleiben offen.

## Mehrere Handys gleichzeitig

Bis zu 8 Geräte können gekoppelt sein, mehrere gleichzeitig verbunden. Der Schalter **Einstellungen → Companion → Geräte synchron anzeigen** (`[companion] sync`, Standard an, wirkt nach Speichern und Neustart) bestimmt das Verhalten:

- **Synchron (an):** alle Handys zeigen dasselbe Bild und wechseln gemeinsam (wie bisher).
- **Unabhängig (aus):** jedes Handy hat seine eigene Seitenposition, Auflösung (Quer-/Hochformat), Wisch- und Tippbedienung und wechselt nach `page_seconds` für sich. Touch-Ziele und Schalter gelten für das Gerät, auf dem getippt wurde. Zusätzlich lässt sich je Gerät eine **eigene Seitenliste** festlegen (Einstellungen → Companion → „Seiten für das ausgewählte Gerät“; wird sofort gespeichert). Eine eigene Liste ist verbindlich: Was fehlt, zeigt das Gerät nicht. „Gemeinsame Liste verwenden“ stellt die Liste unter „Seiten für Companion“ wieder her. Die Position jedes Geräts steht im Status (`/api/companion/status`, `clients[].navigation`).

Vorschau und „Vorige/Nächste Seite zeigen“ in den Einstellungen beziehen sich im unabhängigen Betrieb auf das zuerst verbundene Gerät bzw. auf alle verbundenen Geräte. Die Seitenlisten liegen in `companion.json` (DPAPI-geschützt wie die Kopplungen). Das Protokoll bleibt unverändert; ältere App-Versionen funktionieren ohne Anpassung. **Prüfstand:** automatisierte Prüfungen mit simulierten Geräten (Server-Tests und isolierter App-Lauf mit zwei Test-Clients); Pixel 10 Pro und Redmi 8 wurden am 30.10.2026 mit unterschiedlichen eigenen Seitenlisten, injizierten Wischgesten und automatischem Wechsel geprüft; vollständige Prüfgrenzen stehen in den [Release-Notizen 2.7.0](RELEASE-NOTES-2.7.md).