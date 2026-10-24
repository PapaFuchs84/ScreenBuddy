## Companion-Prototyp: unabhängige Anzeigen (unveröffentlicht)

USB-Display bzw. HDMI-Bildschirm und Android-Companion können gleichzeitig laufen. Beide haben eigene Seitenlisten, Seitenpositionen, Wechselintervalle und Touch-Ziele. Datenquellen, Konten und Design bleiben gemeinsam. Eine Seite muss in ihrem Datenquellen-Reiter eingeschaltet und eingerichtet sein, bevor sie auf einer Anzeige erscheinen kann.

1. Den bisherigen Display-Typ beibehalten; unter **Companion** den Dienst einschalten und **Speichern & anwenden** wählen.
2. Auf dem Android-Gerät die Debug-APK des Prototyps installieren. PC und Handy müssen sich im selben privaten IPv4-Netz erreichen können; USB-Tethering ist ebenfalls möglich. WLAN-/Router-Probleme werden dadurch nicht behoben.
3. Unter **Companion → Gerät koppeln** den Code anzeigen und in der Handy-App per QR-Code oder Adresse, Port und PIN koppeln; Zertifikat-Fingerabdruck vergleichen. PIN nur lokal verwenden, nicht in Berichten veröffentlichen.
4. Unter **Companion → Unabhängige Companion-Anzeige** Seiten wählen und die Anzeigedauer einstellen (3–600 Sekunden). Die Pfeile schalten ausschließlich die Handy-Ansicht. Die Reihenfolge folgt der gespeicherten Auswahl; erneut angehakte Seiten stehen am Ende.
5. Unter **Display → Seiten auf USB-Display / Bildschirm** die Auswahl für die andere Anzeige festlegen. Diese zusätzlichen Einstellungen erscheinen nur, wenn Companion eingeschaltet oder als Display-Typ gewählt ist. Ohne Companion gilt die bisherige Seitensteuerung.

Das Handy meldet seine tatsächliche Bildfläche bei Verbindung und Rotation automatisch. Der PC rendert in dieser Auflösung: beispielsweise 1520 × 720 im Redmi-Querformat, ohne feste Gerätewerte in der Config. Als Speichergrenze gelten maximal 2560 Pixel je Kante und 4.194.304 Pixel; größere Flächen werden proportional verkleinert. Das zuerst verbundene Gerät mit Größenmeldung bestimmt die gemeinsame Companion-Auflösung. Weitere Handys teilen dessen Seitenzustand und skalieren mit erhaltenem Seitenverhältnis. Nach dessen Trennung kann eine Größenmeldung eines verbleibenden bzw. neu verbundenen Geräts die Größe übernehmen.

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

Prüfstand und Grenzen: siehe [Testprotokoll](../cpp/TESTPROTOKOLL.md). Die neue APK wurde auf dem echten Redmi 8 installiert und über USB-Tethering geprüft: native Hoch-/Querformat-Ausgabe, Android-injizierte Wisch-/Tippgesten, automatischer Wechsel und Wiederverbindung mit gespeichertem Schlüssel erfolgreich. Der Nutzer hat scharfes Bild sowie langsames/normales Wischen und Tippen mit dem Finger bestätigt. Physischer USB-Betrieb in derselben neuen PC-Instanz und WLAN bleiben gesonderte Abnahmepunkte. Die produktive PC-App wurde nicht ersetzt; der Stand bleibt unveröffentlicht.
