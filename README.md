<div align="center">

**ScreenBuddy 2.5.0 – 27.10.2026:** native Windows-Einstellungen und signierte Companion-Vorschau 0.2.1-preview. Windows-Sicht-/Touch-Abnahme, Zielsystem-Upgrade/Rollback und Redmi-8-Tests bestanden. [Release-Notizen](docs/RELEASE-NOTES-2.5.md).


<img src="docs/logo/fuchslabs-logo.png" alt="FuchsLabs" width="96"><br>

# ScreenBuddy

**Neu in 2.5:** native Direct2D/DirectWrite-Einstellungen, geführte Einrichtung, Display-/Touch-Konfiguration, Datenquellen, unabhängige Companion-Ausgabe und lokale deklarative Plugins. WebView2 wird nicht mehr benötigt. Online-Plugins bleiben experimentell: Host-Zustimmung ist erforderlich; CRL/OCSP-Sperrprüfung und vollständige UI-Paritätsprüfung stehen aus. [Stand und Grenzen](docs/SCREENBUDDY-2.5.md) · [Plugin-Anleitung](docs/PLUGIN-SYSTEM.md) · [Beispiele](cpp/examples/plugins/README.md).

**Deutsch** · [English](README.en.md)

**Dein Zusatzbildschirm zeigt, was dich interessiert: Systemwerte, Freunde, Spielstände und Neuigkeiten.**

![Version](https://img.shields.io/badge/version-2.5.0-F77F00)
![Plattform](https://img.shields.io/badge/plattform-Windows%2010%20%7C%2011-14213D)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-Testphase-F77F00)
![Sprachen](https://img.shields.io/badge/sprachen-Deutsch%20%7C%20English-14213D)
![Lizenz](https://img.shields.io/badge/lizenz-MIT-green)

[![Spendier mir einen Kaffee](https://img.shields.io/badge/☕_Spendier_mir_einen_Kaffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/display-system.png" width="360" alt="Systemseite im hellen Design">&nbsp;
<img src="docs/screenshots/display-freunde.png" width="360" alt="Freundesliste im hellen Design">

</div>

Windows-App für 3,5-Zoll-USB-Displays (480 × 320) und zusätzliche HDMI-/USB-C-Bildschirme, auch mit Touch. Sie läuft im Hintergrund und bietet native Windows-Einstellungen. Die C++20-App enthält den Installer in derselben EXE; Python, WebView2 und ein Visual-C++-Redistributable werden nicht benötigt. Die frühere Version 2.3.0 benötigte noch WebView2.

## Inhalt

- [Funktionen](#funktionen)
- [Bilder](#bilder)
- [Schnellstart](#schnellstart)
- [Konfiguration](#konfiguration)
- [Unterstützte Hardware](#unterstützte-hardware)
- [Selbst bauen und entwickeln](#selbst-bauen-und-entwickeln)
- [Projektaufbau](#projektaufbau)
- [Sicherheit und Datenschutz](#sicherheit-und-datenschutz)
- [Testphase und Rückmeldungen](#testphase-und-rückmeldungen)
- [Dokumente](#dokumente)
- [Lizenz](#lizenz)

## Funktionen

| Seite | Inhalt | Quelle | Aktualisierung |
|---|---|---|---|
| **System** | CPU (Last, Takt, Temperatur, Lüfter), GPU (Last, Temperatur, Lüfter, VRAM), RAM, Datenträger, Netzwerk mit Verlauf | Windows-APIs, `nvidia-smi`, HWiNFO-Shared-Memory | jede Sekunde |
| **Freunde** | Discord, Steam und Xbox in einer Liste; dieselbe Person erscheint nur einmal | eigener Discord-Bot, Steam Web API, [OpenXBL](https://xbl.io) | sofort / 1 min / 3 min |
| **Discord-Freunde** | Status, Aktivität, Avatar | eigener Discord-Bot | sofort |
| **Battlefield 6 · Rangliste** | eingetragene EA-Freunde nach Rang, mit Abzeichen, K/D und Siegquote | [gametools.network](https://api.gametools.network) | 15 min |
| **Blog-Updates** | neuester Beitrag je Blog; den Feed findet die App selbst | RSS/Atom | 30 min |
| **Home Assistant** | bis zu 4 Sensoren (Wert, Einheit) oder Schalter (per Touch schaltbar) | Home-Assistant-REST-API | 15 s |

Dazu kommen:

- **Eigenes Windows-Einstellungsfenster** (WebView2; interner lokaler Dienst unter `http://127.0.0.1:8765/`) mit Live-Vorschau, Seitenreihenfolge, Verbindungsstatus je Plattform und aufklappbaren Hilfen neben jedem Schlüsselfeld
- **Ersteinrichtung ohne Vorkenntnisse**: erkennt das Display, beendet und deaktiviert auf Wunsch die Hersteller-App, Autostart ohne Administratorrechte
- **Design**: FuchsLabs-CI in Hell oder Dunkel, oder **eigene Farben** per Farbwähler mit Kontrastwarnung
- **Jede Auflösung**: Seiten werden scharf auf die echte Größe gerendert; breite Leisten zeigen mehrere Seiten nebeneinander
- **HDMI/USB-C-Bildschirme** als randloses Vollbild über der Taskleiste, optional mit **Touch** (wischen oder tippen zum Blättern, Tippen auf einen Blogbeitrag öffnet ihn im Browser, Tippen auf RAM, Datenträger oder Netzwerk der Systemseite öffnet Task-Manager, „Programme und Features“ bzw. den Geschwindigkeitstest); Prüfung und Einrichtung der Windows-Touch-Zuordnung unter **Display**
- **Deutsch oder Englisch**: Einstellungsseite, Display und Infobereich lassen sich umschalten; beim ersten Start folgt die Sprache Windows
- **Bebilderte Anleitung** zum Durchklicken, verständlich auch für Kinder, in der EXE eingebettet auf Deutsch und Englisch
- **Update-Hinweis** (abschaltbar): meldet höchstens einmal am Tag eine neue GitHub-Version im Infobereich und in den Einstellungen, installiert aber nie selbst
- **Eine EXE** für Installation, Updates und Deinstallation; kein Python und kein Visual-C++-Redistributable erforderlich. WebView2 wird nicht benötigt.

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick öffnet die Einstellungen), und **Einstellungen → Start** zeigt „Neue Version … verfügbar“ mit Link zur Download-Seite. **Jetzt prüfen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Start → Auf neue Versionen hinweisen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Touch-Ziele und Spielstunden (ab 2.0.7)

Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](docs/TOUCH-2.0.7.md). [Prüfstand](cpp/TESTPROTOKOLL.md).

## Home Assistant (ab 2.1.0)

Neue Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. http://homeassistant.local:8123), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in config.toml. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit http:// laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort https://, wenn Home Assistant es anbietet.

## Android-Companion-App (ab 2.3.0)

Mit der Android-Companion-App lässt sich ein Smartphone oder Tablet als unabhängige Zusatzanzeige parallel zum USB-/HDMI-Display einbinden:
- **Getrennte Seiten & Intervalle:** Eigene Seitenliste und eigene Anzeigedauer für das Smartphone (unter **Companion → Unabhängige Companion-Anzeige**); Datenquellen und Design bleiben gemeinsam.
- **Kopplung per QR-Code oder PIN:** Im Reiter **Companion** auf **Gerät koppeln** klicken. Die Android-App scannt den QR-Code oder verbindet sich über Adresse, Port (8777) und 6-stelligen PIN-Code; der SHA-256-Fingerabdruck sichert die TLS-Verbindung ab.
- **Touch & Gesten:** Wischen schaltet nur das Smartphone-Display um. Interaktive Kacheln (Smart Home, Freunde, Systemaktionen) lassen sich per Fingertipp bedienen.
- **Native Auflösung & Rotation:** Das Handy meldet Ausrichtung (Hoch-/Querformat) und Pixelmaße automatisch; der PC rendert die Ansicht passgenau.
- [Schritt-für-Schritt-Anleitung zur Kopplung](ANLEITUNG.md#android-companion-app-koppeln-und-nutzen-ab-230) · [Einrichtung und Grenzen](docs/COMPANION.md)

## Ansichten durchblättern (2.0.4)


Unter **Einstellungen → Übersicht** kannst du mit **Vorherige Ansicht** und **Nächste Ansicht** direkt durch die laufenden Display-Ansichten blättern. Die Position und die sichtbaren Seiten stehen unter der Live-Vorschau. Lange Listen blättern mit ihren Unterseiten, breite Displays wechseln wie bei der Touchbedienung um eine Kachelgruppe. Nach einem manuellen Wechsel beginnt die eingestellte Anzeigedauer neu; danach läuft der automatische Wechsel weiter. Bei keiner oder nur einer vollständig sichtbaren Ansicht sind die Knöpfe deaktiviert. Dafür ist kein Speichern oder Neustart nötig. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.

## Bilder

| Display | |
|---|---|
| <img src="docs/screenshots/display-bf6-dunkel.png" width="380" alt="Battlefield-6-Rangliste im dunklen Design"> | <img src="docs/screenshots/display-blogs-dunkel.png" width="380" alt="Blog-Updates im dunklen Design"> |
| Battlefield-6-Rangliste (Dunkel) | Blog-Updates (Dunkel) |

| Einstellungsseite | |
|---|---|
| <img src="anleitung/bilder/start.png" width="380" alt="Startseite der Einstellungen"> | <img src="anleitung/bilder/prev0.png" width="380" alt="Übersicht mit Live-Vorschau"> |
| Start mit Ersteinrichtung | Übersicht mit Live-Vorschau |
| <img src="anleitung/bilder/discord.png" width="380" alt="Discord-Einstellungen mit Hilfe zum Aufklappen"> | <img src="anleitung/bilder/bf6.png" width="380" alt="Battlefield-6-Spieler eintragen"> |
| Discord mit Hilfe zum Aufklappen | Battlefield-6-Spieler eintragen und prüfen |

Alle Bilder zeigen Beispieldaten; einzelne Screenshots stammen aus der bisherigen Oberfläche 1.x.

## Schnellstart

1. **ScreenBuddy.exe** aus [Release 2.5.0](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.5.0) herunterladen. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` prüfen. Die signierte Android-Vorschau-APK ist ein separater Download.
2. Windows 10/11 x64 mit installierter **Microsoft Edge WebView2 Evergreen Runtime** verwenden. Die Laufzeit ist nicht in der EXE enthalten und wird vom Installer nicht automatisch installiert.
3. Display anschließen, EXE starten und den Installer abschließen. Standardziel: `%LOCALAPPDATA%\FuchsLabs\USB-Display`, ohne Administratorrechte.
4. Bei der ersten Einrichtung öffnet sich ein **eigenes Windowsfenster**. Gewünschte Seiten aktivieren und die Datenquellen einrichten, dann **Speichern & anwenden**.
5. Später öffnen das Fuchs-Symbol im Infobereich oder ein erneuter Start der installierten App das Einstellungsfenster. Dessen Schließen lässt das Display weiterlaufen; **Beenden** im Infobereich stoppt die App.

Der Installer kann Einstellungen aus Version 1.x übernehmen und bewahrt den alten Ordner mit dem Zusatz `-1.x-alt` auf. Bei einem Update bleibt eine vorhandene Zielkonfiguration erhalten. Die bebilderte Anleitung öffnet sich über die Einstellungen oder **Anleitung öffnen** im Tray im Standardbrowser und ist in der EXE eingebettet; ZIP und `_internal`-Ordner entfallen.

Für einen HDMI-/USB-C-Touchscreen unter **Display** den vorgesehenen Monitor wählen. Erscheinen Berührungen auf einem anderen Bildschirm, **Touch-Bildschirm zuordnen** öffnen: andere Bildschirme mit Enter überspringen und den vorgesehenen Touchscreen berühren. Danach **Erneut prüfen** wählen. Die USB-Touchverbindung muss bestehen. Details im [Schnellstart](ANLEITUNG.md#hdmiusb-c-und-touch).

### Zugänge für die Datenquellen

| Quelle | Was du brauchst | Kosten |
|---|---|---|
| Discord | eigener Bot im [Developer Portal](https://discord.com/developers/applications) mit *Presence Intent* und *Server Members Intent*, auf deinen Server eingeladen | kostenlos |
| Steam | [Steam-Web-API-Schlüssel](https://steamcommunity.com/dev/apikey) und dein Profil-Link; Freundesliste im Profil auf „Öffentlich“ | kostenlos |
| Xbox | Schlüssel von [OpenXBL](https://xbl.io) | kostenlose Stufe |
| Battlefield 6 | nur die EA-IDs deiner Freunde | kostenlos |
| CPU-Temperatur und Lüfter | [HWiNFO64](https://www.hwinfo.com/), eingerichtet mit `hwinfo-einrichten.bat` | kostenlos |

PlayStation und ein automatischer EA-Freundesabruf sind bewusst nicht eingebaut: Dafür gibt es keine offiziellen Schnittstellen, und inoffizielle Wege können zur Kontosperre führen.

## Konfiguration

Alles lässt sich in der Einstellungsseite einstellen. Die reguläre Installation speichert `config.toml` und `usb-display.log` unter `%LOCALAPPDATA%\FuchsLabs\USB-Display`. Eigene Pfade per `-c` und `--log` haben Vorrang. Ein Auszug:

```toml
[display]
output = "serial"          # "serial" = 3,5"-USB-Display, "monitor" = HDMI/USB-C-Bildschirm
orientation = "landscape"  # auch reverse_landscape, portrait, reverse_portrait
brightness = 90
page_seconds = 15
pages = ["system", "friends", "discord", "bf6", "rss", "homeassistant"]   # Reihenfolge
theme = "hell"             # "hell", "dunkel" oder "eigen"
touch = true               # nur bei output = "monitor"

[bf6]
enabled = true
players = ["FuchsKumpel", "Lena_Sniper@psn"]

[rss]
enabled = true
feeds = ["https://winfuture.de/"]

[homeassistant]
enabled = true
url = "http://homeassistant.local:8123"
entities = ["sensor.wohnzimmer_temperatur", "light.stehlampe"]
```

Tokens und Schlüssel stehen dort nur verschlüsselt (`dpapi:…`). Alle Schlüssel mit Standardwerten beschreibt [`DOKUMENTATION.md`](DOKUMENTATION.md#4-konfigurationsdatei).

### Kommandozeile

| Option | Wirkung |
|---|---|
| `-c DATEI` | andere Konfigurationsdatei verwenden |
| `--demo` | alle Seiten mit Beispieldaten zeigen |
| `--no-lcd` | ohne Display starten (zum Testen der Einstellungsseite) |
| `--preview ORDNER` | jedes Bild zusätzlich als PNG speichern |
| `--log DATEI`, `-v` | Protokoll in eine Datei, ausführlich |

Die Sprache steht unter `[ui]` als `language = "de"` oder `"en"`.

## Unterstützte Hardware

| Gerät | Anschluss | Hinweis |
|---|---|---|
| 3,5"-USB-Display 480 × 320 („Revision A“) | USB (serielle Schnittstelle, 115200 Baud) | wird automatisch gefunden; COM-Port bei Bedarf von Hand |
| Zusatzbildschirm beliebiger Größe, z. B. 10,25" 1920 × 720 | HDMI oder USB-C (DisplayPort) | randloses Vollbild auf dem gewählten Bildschirm, optional Touch |
| GPU-Werte | Nvidia mit Treiber (`nvidia-smi`) | andere GPUs zeigen „–“ |

## Selbst bauen und entwickeln

Voraussetzungen: Windows x64, Visual Studio Build Tools 2022 mit C++/Windows SDK und CMake/Ninja sowie ein eingerichteter vcpkg-Checkout unter `%USERPROFILE%\FuchsLabs\vcpkg`.

```bat
git clone https://github.com/PapaFuchs84/ScreenBuddy.git
cd ScreenBuddy\cpp
build.bat nopause
```

Der C++-Build installiert die Abhängigkeiten aus dem festgelegten vcpkg-Manifest/Overlay, baut Release, führt Kern-, Plugin-, Monitor-, Companion- und isolierte Installer-Prüfungen aus und erzeugt lokal `cpp\dist\ScreenBuddy.exe`. Die `build.bat` im Repository-Hauptordner gehört zur historischen Python-Version 1.x. Details: [C++-Buildhinweise](cpp/README.md#build) und [Mitarbeiten](CONTRIBUTING.md).

Die WebView2-SDK-Dateien und Lizenzhinweise liegen weiterhin unter `cpp/third_party/webview2`, werden von der aktuellen 2.5-Quellfassung aber nicht eingebunden. Der lokale HTTP-Dienst und die eingebettete Anleitung bleiben erhalten. Übersetzungen: `cpp/src/i18n.cpp` für App/Display, native Texte unter `cpp/src/ui_page_*.cpp`; die frühere HTML-Quelldatei liegt nur noch für historische Tests unter `cpp/res/webui.html` und wird nicht in die 2.5-EXE eingebettet. `/` liefert keine HTML-Einstellungen mehr aus.

## Projektaufbau

| Pfad | Aufgabe |
|---|---|
| `cpp/src/` | C++-App, Displayausgabe, Datenquellen, natives Einstellungsfenster und Installer |
| `cpp/res/` | Eingebettete Anleitungen, Symbol und Versionsressourcen |
| `cpp/tests/` | C++-Prüfungen |
| `cpp/overlay-ports/`, `cpp/vcpkg.json` | Abhängigkeiten und D++-Buildkonfiguration |
| `cpp/TESTPROTOKOLL.md` | Lokaler Windows-/Displaytest |
| `*.py`, `build.bat` im Hauptordner, `anleitung/` | Historischer Python-Stand 1.x und Generator für die aktuelle DE/EN-Anleitung |

## Sicherheit und Datenschutz

- Läuft **ohne Administratorrechte**, ohne Telemetrie, ohne Konto. Die einzige App-eigene Anfrage ohne deine Datenquellen ist der abschaltbare Update-Hinweis an die öffentlichen GitHub-Releases (höchstens 1× am Tag, ohne Schlüssel).
- Zugangsdaten werden mit **Windows-DPAPI** verschlüsselt gespeichert, nie protokolliert und nie an den Browser gegeben.
- Die Einstellungsseite ist **nur auf `127.0.0.1`** erreichbar und gegen DNS-Rebinding, CSRF und Einbetten in fremde Seiten geschützt.
- Alle Verbindungen laufen über HTTPS mit Zertifikatsprüfung; Pakete sind auf genaue Versionen festgelegt.

Historischer Python-1.x-Prüfbericht: [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md); er ist kein vollständiger Audit der C++-Version. Aktuelle Validierung: [Testprotokoll](cpp/TESTPROTOKOLL.md). Eine Lücke meldest du wie in [`SECURITY.md`](SECURITY.md) beschrieben, bitte nicht als öffentliches Issue.

## Testphase und Rückmeldungen

Die App ist in der Testphase. Fehler und Ideen bitte über **Issues → New issue** mit den Vorlagen „Fehlerbericht“ oder „Idee oder Rückmeldung“ melden. Wo das Protokoll `usb-display.log` liegt und was nie mitgeschickt werden darf, steht in [`TESTPHASE.md`](TESTPHASE.md).

## Dokumente

| Datei | Inhalt |
|---|---|
| [`README.en.md`](README.en.md) | dieses README auf Englisch |
| [`ANLEITUNG.md`](ANLEITUNG.md) | Schnellstart für Anwender |
| [`DOKUMENTATION.md`](DOKUMENTATION.md) | ausführliche Referenz: Seiten, Einstellungen, `config.toml`, HWiNFO, Fehlerbehebung |
| [`CHANGELOG.md`](CHANGELOG.md) | Änderungen je Version ([English](CHANGELOG.en.md)) |
| [`PROTOKOLL.md`](PROTOKOLL.md) | Entstehungsverlauf und Entscheidungen |
| [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) | Sicherheits- und Performance-Prüfung |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Mitarbeiten: Ablauf, Stil, Regeln ([English](CONTRIBUTING.en.md)) |
| [`SECURITY.md`](SECURITY.md) | Sicherheitslücken melden ([English](SECURITY.en.md)) |
| [`TESTPHASE.md`](TESTPHASE.md) | Rückmeldungen in der Testphase ([English](TESTPHASE.en.md)) |

## Unterstützen

Dir gefällt die App? Über einen Kaffee freut sich FuchsLabs: [☕ paypal.me/SHerbst84](https://www.paypal.me/SHerbst84)

## Lizenz

Der Code steht unter der [MIT-Lizenz](LICENSE), © 2026 FuchsLabs. Name und Fuchskopf-Logo sind ausgenommen ([TRADEMARKS.md](TRADEMARKS.md)). Für eingebundene Drittkomponenten gelten deren eigene Lizenzen ([THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)).

Battlefield, Discord, Steam und Xbox sind Marken ihrer jeweiligen Inhaber. FuchsLabs steht in keiner Verbindung zu ihnen.

## Prüfstand 2.3.0

Release-Build: 236 Kern-, 11 Monitor- und 335 Companion-/TLS-Prüfungen bestanden. Außerdem 39 isolierte App-Integrationsprüfungen, 2 Installer-Sicherungstests, 22 Browserprüfungen (DE/EN) und 20 Android-Protokolltests. Auf dem Redmi 8 wurden native Auflösung, Hoch-/Querformat, automatischer Seitenwechsel, Tippen, Wischen und Wiederverbindung bestätigt; der Nutzer bestätigte zusätzlich scharfes Bild und Fingerbedienung. **Offen:** physischer USB-Ausgang und Redmi gemeinsam in derselben neuen PC-Instanz, WLAN, Android-Start beim Booten und mehrere reale Geräte. Siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## Prüfstand 2.2.0 (historisch)

Release-Build, 225 Kernprüfungen (neu: Trefferflächen der Systemseite, Versionsvergleich und Auswertung der GitHub-Antwort) und 11 Monitor-Basisprüfungen bestanden; die Trefferflächen stimmen mit der gerenderten Systemseite überein (Vorschaubild). **Noch offen:** Tippen auf echter Touch-Hardware (Task-Manager, „Programme und Features“, Speedtest), die Sprechblase und die Anzeige in den Einstellungen mit einer tatsächlich neueren Veröffentlichung sowie ein Windows-Anmeldetest für 2.2.0. Siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## Prüfstand 2.0.3

Release-Build, 86 Kernprüfungen und 56 Monitor-/Touchprüfungen mit injizierten Windows-Eingaben bestanden (die 56 enthalten 11 Basisprüfungen). Installierte 2.0.3: tatsächliche Datei-Version, Hash, API-Version, Autostart-Eintrag und `display_ok=true` außerhalb der Paketumleitung geprüft. Echte Touch-Hardware, der vollständige Windows-Zuordnungsassistent und ein neuer Windows-Anmeldetest für 2.0.3 bleiben offen. Siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

Die frühere Abnahme von 2.0 umfasste den Gegenvergleich mit 1.x an COM7 und die vom Nutzer bestätigte physische Anzeige sowie einen erfolgreichen Windows-Neustart mit Tray-Symbol.

Der erste reale Neustarttest scheiterte an der Paketumleitung des Installationshosts. Installation und tatsächlicher Windows-Autostarteintrag wurden danach außerhalb dieses Kontexts korrigiert; der Nutzer bestätigte anschließend den erfolgreichen Windows-Neustart mit Tray-Symbol.
