## Companion-App (ab 2.3.0): unabhängige Anzeigen

Aktuelle Version: Companion 1.1.1 (Code 10), targetSdk 36, dunkles Standard-Design mit Orange-Akzenten; [Build- und Store-Vorbereitung](STORE-DISTRIBUTION.md). 1.1.1 ist eine Fehlerkorrektur für 1.1.0 (Bildübergabe, Automatik, erneute Kopplung); das Protokoll ist unverändert und bleibt mit ScreenBuddy 2.12.0 und älter kompatibel. Vorgänger waren 1.1.0 und 1.0.3.

## Automatik (Companion 1.1.0)

Die Automatik ist standardmäßig aus und benötigt eine vorhandene Kopplung. Sie nutzt die vorhandene lokale mDNS-Ankündigung des PC-Companion-Dienstes; eine Protokolländerung am PC ist dafür nicht nötig. PC und Android-Gerät müssen sich im privaten Netz erreichen, und mDNS darf nicht durch Gastnetz, Router oder Firewall blockiert sein. Ein Netzwechsel oder eine geänderte PC-Adresse kann die Erkennung verhindern; dann manuell verbinden und die gespeicherte Verbindung prüfen.

Während der mDNS-Suche hält die App einen WLAN-Multicast-Lock, damit Android die Ankündigungen auch im Hintergrund empfangen kann; bei Suchende oder Suchfehler wird er freigegeben. Die kontinuierliche Suche bei eingeschalteter Automatik kann den Akkuverbrauch erhöhen. Globale WLAN-/Energiespareinstellungen werden nicht geändert; es gibt keinen dauerhaften CPU-Wakelock.

Manche Geräte (am Redmi 8 beobachtet) empfangen bei gesperrtem Bildschirm kein mDNS mehr; ein zeitlich begrenzter CPU-Wakelock änderte daran nichts. Deshalb prüft die App zusätzlich per Android-Alarm (angestrebt jede Minute, auf dem Redmi 8 gemessen alle rund 105 Sekunden), ob der gekoppelte PC auf seinem Companion-Port erreichbar ist (reiner TCP-Verbindungsaufbau ohne TLS, PIN oder Daten, nur im privaten Netz; der PC belegt dafür kurz einen Verbindungsplatz). Ist der PC erreichbar, öffnet die Automatik die Anzeige, auch wenn mDNS schweigt. Android begrenzt solche Alarme im Ruhezustand (Doze): Das Öffnen kann dann spürbar später als eine Minute erfolgen. Bei ausgeschaltetem Bildschirm gilt mDNS allein nicht als Beleg für die Anwesenheit des PCs.

1. Zuerst wie unten koppeln und die manuelle Anzeige prüfen. Den Companion-Dienst in ScreenBuddy eingeschaltet lassen.
2. In der Android-Einrichtung unter **Automatik** den Schalter zum Öffnen beim ScreenBuddy-Start aktivieren. Ein Bereitschaftsdienst sucht anschließend den gekoppelten PC; Android zeigt dafür eine Dienstbenachrichtigung.
3. Ab Android 13 Benachrichtigungen erlauben. Über **Anzeige über anderen Apps erlauben** kann die Anzeige aus dem Hintergrund geöffnet werden. Ohne diese Freigabe wird eine normale Benachrichtigung zum Antippen angeboten; die App fordert keine Vollbildbenachrichtigungs-Freigabe an. Hintergrundstart und Aufwecken sind je nach Android-Version und Hersteller eingeschränkt.
4. Nur auf Handys: **Bildschirmsperre erlauben** öffnet die Android-Geräteadmin-Freigabe. Sie wird ausschließlich für das Sperren benötigt. Ohne diese Freigabe kann die Anzeige schließen, der Bildschirm wird aber nicht von der App gesperrt. Seit 1.1.1 sperrt die App außerdem nur, wenn die Automatik die Anzeige selbst geöffnet hat und diese beim Schließen noch im Vordergrund ist; wurde nur die Benachrichtigung angeboten, die Anzeige inzwischen verlassen oder war eine manuell geöffnete Anzeige beim Erscheinen des PCs bereits sichtbar, bleibt das Gerät ungesperrt. Nach einer Geräteadmin-Sperre kann Android beim nächsten Entsperren PIN, Muster oder Passwort statt Biometrie verlangen. Auf Android-TV wird diese Freigabe nicht angeboten; die App sendet keine ausdrücklichen TV-Aus-, Standby- oder HDMI-CEC-Befehle. Das Aufwecken kann je nach Gerät über das Android-System dennoch HDMI-CEC auslösen.

Nach erkannter Abwesenheit des PCs gilt eine Karenzzeit von 20 Sekunden. Seit 1.1.1 gilt eine fehlende mDNS-Ankündigung oder ein Fehler der mDNS-Suche allein nicht mehr als Abwesenheit: Die App prüft dann sofort per TCP nach und wiederholt eine fehlgeschlagene Prüfung nach wenigen Sekunden. Erst nach zwei fehlgeschlagenen Prüfungen in Folge beginnt die Karenzzeit; ein weiterhin erreichbarer PC behält seine Anzeige. Kehrt die Ankündigung zurück oder ist der PC während der Karenzzeit wieder erreichbar, wird das Schließen abgebrochen. Eine fehlgeschlagene mDNS-Suche startet die Automatik mit wachsendem Abstand (5 Sekunden bis höchstens 5 Minuten) neu. Die Erkennung ist kein exakter Timer ab dem Beenden der Windows-App: Netzwerkunterbrechungen können dieselbe Wirkung haben, und Android kann die Erkennung verzögern. Eine vorhandene PIN-/Biometriesperre wird nicht aufgehoben.

In den Android-Systemeinstellungen heißt die App **FuchsLabs Display**. Unter diesem Namen nach Benachrichtigungen, „Über anderen Apps anzeigen“, Geräteadmin und Akkuoptimierung suchen, auch wenn sie hier als Companion-App beschrieben wird. Falls die Automatik im Hintergrund aussetzt, unter **Akkuoptimierung → Alle Apps → FuchsLabs Display** den gerätespezifischen Status prüfen. Die Bezeichnungen unterscheiden sich nach Android-Version; auf dem Redmi kann es nötig sein, in der App-Ansicht nach unten zu scrollen, um die Optionen zu sehen. Steht dort bereits, dass die App uneingeschränkte Akkunutzung benötigt, und ist die erlaubte Hintergrundnutzung ausgegraut, ist keine zusätzliche Akku-Ausnahme einzustellen. Dieser Status erklärt für sich allein keinen Erkennungsfehler.

Zum Beenden der Automatik den Schalter in der Einrichtung ausschalten. Eine erteilte Geräteadmin-Freigabe bleibt eine Android-Systemeinstellung und kann dort unter Geräteadministratoren widerrufen werden; vor einer Deinstallation gegebenenfalls dort deaktivieren. Nach einem Geräteneustart versucht die aktivierte Automatik wieder Bereitschaft herzustellen. Android-Batteriesparen, erzwungenes Beenden und Herstellerbeschränkungen können das verhindern; bei Problemen App manuell öffnen und Systemfreigaben prüfen. Manuelle Nutzung bleibt ohne Automatik möglich.

**Automatisierter Prüfstand 08.11.2026:** 40 JVM-Protokolltests bestanden (inkl. Logik der TCP-Erreichbarkeitsprüfung), Debug-APK und signierte Release-APK gebaut. **Geräteprüfung Redmi 8 (Android 14), 09.11.2026, bei ausgeschaltetem Bildschirm:** ScreenBuddy am PC beendet, die Anzeige schloss nach zwei fehlgeschlagenen Prüfungen und 20 Sekunden Karenz, rund 2,5 Minuten nach dem Ausschalten des Bildschirms. Nach dem Neustart von ScreenBuddy öffnete sich die Anzeige und der Bildschirm ging an, 74 Sekunden nach dem PC-Start. Jeweils ein Durchlauf, nicht wiederholt. Pixel: von Basti selbst getestet, läuft laut seiner Rückmeldung (ohne Messwerte). Noch nicht geprüft: Android-TV mit echtem PC-Start/-Ende, Wiederholungen und andere Geräte. Die APK-Signatur wurde geprüft; das Signierzertifikat stimmt mit der veröffentlichten Companion-App 1.0.3 überein. Debug- und Release-Lint: 37 Warnungen, 0 Fehler. Die Version bleibt unveröffentlicht; Build und Signaturprüfung belegen keine Geräteabnahme.

**Prüfstand 1.1.1, 14.11.2026 (nur automatisiert):** 56 JVM-Tests des Protokollmoduls bestanden (darunter neu: Übergabe nur des neuesten Bildes, TCP-Bestätigung nach mDNS-Verlust, Wiederholabstände der Suche) und der Kotlin-Debug-Stand kompiliert. Keine APK signiert oder auf einem Gerät installiert; die Korrekturen sind am Redmi, Pixel und TV noch nicht abgenommen. Die Geräteergebnisse oben gelten für 1.1.0: Die dort gemessene Zeit bis zum Schließen kann in 1.1.1 kürzer ausfallen, weil eine fehlgeschlagene Prüfung nach wenigen Sekunden statt beim nächsten Alarm wiederholt wird.

**Prüfgrenzen von 1.1.0:** Reale Freigabedialoge, Aufwecken aus gesperrtem Zustand, Start nach Android-Boot, PC-Start/-Ende, Netzunterbrechung und Wiederverbindung sowie Finger-/Fernbedienung am Redmi, Pixel und TV sind für 1.1.0 noch abzunehmen. Frühere Prüfstände unten beziehen sich auf die jeweils genannte Version.

**Stand 31.10.2026:** Die aktuelle Companion-App 1.0.3 (versionCode 7; neues Login-Design in Hell und Dunkel; seit 1.0.1 mit Abstand zu abgerundeten Ecken und Kamera-Ausschnitt) ist ein separater Download und nicht Teil der Windows-EXE. Der Gerätetest am Redmi 8 über USB-Tethering vom 27.10.2026 (Quer-/Hochformat, echte Fingerbedienung, automatischer Wechsel und Wiederverbindung ohne neue PIN) galt dem Vorgänger 0.2.1-preview (versionCode 3, mit neuem RSA-3072-Release-Schlüssel signiert, APK-v2-Signatur geprüft). Für die Companion-App 1.0.3 wurde die Oberfläche am 31.10.2026 auf Pixel 10 Pro, Redmi 8 und BeyondTV installiert und per Screenshot geprüft; ein vollständiger Verbindungstest ist im [Testprotokoll](../cpp/TESTPROTOKOLL.md) kein eigener Gerätetest dokumentiert. Die bisherige Debug-APK hat eine andere Signatur und lässt sich nicht direkt damit aktualisieren. Nicht automatisch deinstallieren: lokale Daten und Kopplungen würden gelöscht. [Build, Signierung und Migration](../android/README.md). Frühere Geräteergebnisse unten gelten für 0.2.0-preview.

USB-Display bzw. HDMI-Bildschirm und Android-Companion können gleichzeitig laufen. Beide haben eigene Seitenlisten, Seitenpositionen, Wechselintervalle und Touch-Ziele. Datenquellen, Konten und Design bleiben gemeinsam. Eine Seite muss in ihrem Datenquellen-Reiter eingeschaltet und eingerichtet sein, bevor sie auf einer Anzeige erscheinen kann.

Solange die Einrichtung oder Vollbildanzeige der Companion-App sichtbar ist, bleibt der Bildschirm eingeschaltet; der Android-TV-Bildschirmschoner wird in dieser Zeit unterdrückt. Nach dem Verlassen der App gelten wieder die normalen TV-Einstellungen. Die App ändert keine globalen Bildschirmschoner- oder Energiespareinstellungen.

Auf Android TV erscheint die App im TV-Launcher. Mit der Fernbedienung wechseln Pfeil links/rechts die Seite, OK hat keine Funktion, Zurück (zweimal) verlässt die Anzeige. Ohne Kamera entfällt der QR-Scan; Adresse, Port und PIN manuell eingeben. **Prüfstand:** BeyondTV (Android 11) am 31.10.2026 mit ADB-Tasteneingaben; echte Fernbedienung, Standby/CEC und Langzeitbetrieb offen.

1. Den bisherigen Display-Typ beibehalten; unter **Companion** den Dienst einschalten und **Speichern & anwenden** wählen.
2. Auf dem Android-Gerät die Companion-App 1.0.3 installieren (separater Download). PC und Handy müssen sich im selben privaten IPv4-Netz erreichen können; USB-Tethering ist ebenfalls möglich.
3. Unter **Companion → Gerät koppeln** den Code anzeigen und in der Handy-App per QR-Code oder Adresse, Port und PIN koppeln; Zertifikat-Fingerabdruck vergleichen. PIN nur lokal verwenden, nicht in Berichten veröffentlichen.
4. Unter **Companion → Seiten für Companion** Seiten wählen und oben unter **Companion-Ausgabe** den **Seitenwechsel** einstellen (3–600 Sekunden). Die Pfeile schalten ausschließlich die Handy-Ansicht. Die Reihenfolge folgt der gespeicherten Auswahl; erneut angehakte Seiten stehen am Ende.
5. Unter **Seiten & Reihenfolge → Seiten auf USB-Display / Bildschirm** die Auswahl für die andere Anzeige festlegen. Diese zusätzlichen Einstellungen erscheinen nur, wenn Companion eingeschaltet oder als Display-Typ gewählt ist. Ohne Companion gilt die bisherige Seitensteuerung.

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

## Geräteprofil: Sperrzone und Android-TV (Phase 4a, Entwicklung)

Teilpatches wie `{"companion":{"keepout":{"top":5}}}` erhalten alle nicht genannten Ränder. Im gemeinsamen Betrieb gilt ausschließlich `companion.keepout`; die Gerätekarte weist auf die dann unwirksamen eigenen Ränder hin. Auch abweichend große Bildinhalte bleiben auf die Nutzfläche begrenzt.

Android-TV bleibt im Querformat (`SENSOR_LANDSCAPE`). Die Ausrichtungswahl ist dort ausgeblendet; eine früher gespeicherte Auswahl wird ignoriert und nicht gelöscht. Auf Handys bleiben Quer/Hoch/Automatisch unverändert. TV hat keine Touch-Aktionen; D-Pad links/rechts navigiert wie bisher, CENTER/ENTER bleibt ohne Aktion.

Die PC-Einstellungen bieten unter Companion vier Ränder als gemeinsame Vorgabe (`companion.keepout`) und für das ausgewählte gekoppelte Gerät eine eigene Sperrzone. Eigene Ränder werden sofort über den Companion-Store gespeichert, wirken ausschließlich im unabhängigen Betrieb (`companion.sync = false`) und lassen sich auf die gemeinsame Vorgabe zurücksetzen. Fehlendes Feld in alten Kopplungen bedeutet: keine eigene Vorgabe; standardmäßig alle Ränder 0. Handy-Kameraausschnitte bleiben zusätzlich clientseitig.

Die Ränder oben/rechts/unten/links gelten in der gemeldeten vollen Bildgröße ohne Drehung. Ganze Pixel 0–1000 sind erlaubt; beim Rendern je Rand höchstens ein Drittel der kleineren Bildkante. Unter 160 × 120 nutzbaren Pixeln wird die komplette Sperrzone ignoriert (Warnung höchstens einmal je Ansicht/Minute). Inhalte werden in der freien Größe gezeichnet und in ein Vollbild mit Theme-Hintergrund eingesetzt; auch Vorschauen zeigen diesen Rand. `ready.w/h` und Touch-Koordinaten bleiben volle Bildkoordinaten (Protokoll unverändert). Der PC verschiebt Ziele je Gerät; Randtipps bewirken weder Aktionen noch Navigation, veraltete Bildgrößen werden verworfen.

Bedienung: [Navigation](NAVIGATION.md#geräteprofil-und-sperrzone-phase-4a-entwicklung). Prüfstand: [Testprotokoll](../cpp/TESTPROTOKOLL.md); physische TV-/Gehäuseabnahme offen.

## Plugin-Tipps (Phase 4c, Entwicklung)

Schema-2-Plugins können HTTPS-Links und den nächsten Wechselfeld-Eintrag anbieten. Links öffnen am PC, auch bei einem Tipp auf dem Handy. Die eigene Freigabe unter Einstellungen → Plugins ist standardmäßig aus und zeigt die Link-Hosts. Ziele brauchen 1,0 s Ruhe beim Companion bzw. 0,3 s beim Monitor und sind während Übergängen gesperrt; verweigerte Tipps wechseln die Seite nicht. Ohne Ziel gilt die bisherige Drittel-Regel. Android-TV erhält keine Tap-/D-Pad-Aktion. Details: [Plugin-System](PLUGIN-SYSTEM.md#tap-ziele-für-plugin-blöcke-phase-4c-entwicklung-für-2100). Physische Abnahme offen.
