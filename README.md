<div align="center">

# 🦊 FuchsLabs USB-Display

**Deutsch** · [English](README.en.md)

**Dein Zusatzbildschirm zeigt, was dich interessiert: Systemwerte, Freunde, Spielstände und Neuigkeiten.**

![Version](https://img.shields.io/badge/version-2.0.3-F77F00)
![Plattform](https://img.shields.io/badge/plattform-Windows%2010%20%7C%2011-14213D)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-Testphase-F77F00)
![Sprachen](https://img.shields.io/badge/sprachen-Deutsch%20%7C%20English-14213D)
![Lizenz](https://img.shields.io/badge/lizenz-alle%20Rechte%20vorbehalten-lightgrey)

[![Spendier mir einen Kaffee](https://img.shields.io/badge/☕_Spendier_mir_einen_Kaffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/display-system.png" width="360" alt="Systemseite im hellen Design">&nbsp;
<img src="docs/screenshots/display-freunde.png" width="360" alt="Freundesliste im hellen Design">

</div>

Windows-App für 3,5"-USB-Displays (480 × 320) und Zusatzbildschirme über HDMI/USB-C in jeder Auflösung (z. B. 10,25"-Leisten mit 1920 × 720, auch mit Touch). Sie ersetzt die Hersteller-App, läuft unauffällig im Hintergrund und wird über ein eigenes Windows-Einstellungsfenster mit WebView2 bedient. Version 2.0 ist in C++20 geschrieben und enthält den Installer in derselben EXE. Python wird nicht benötigt; die Microsoft Edge WebView2 Evergreen Runtime muss separat vorhanden sein.

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

Dazu kommen:

- **Eigenes Windows-Einstellungsfenster** (WebView2; interner lokaler Dienst unter `http://127.0.0.1:8765/`) mit Live-Vorschau, Seitenreihenfolge, Verbindungsstatus je Plattform und aufklappbaren Hilfen neben jedem Schlüsselfeld
- **Ersteinrichtung ohne Vorkenntnisse**: erkennt das Display, beendet und deaktiviert auf Wunsch die Hersteller-App, Autostart ohne Administratorrechte
- **Design**: FuchsLabs-CI in Hell oder Dunkel, oder **eigene Farben** per Farbwähler mit Kontrastwarnung
- **Jede Auflösung**: Seiten werden scharf auf die echte Größe gerendert; breite Leisten zeigen mehrere Seiten nebeneinander
- **HDMI/USB-C-Bildschirme** als randloses Vollbild über der Taskleiste, optional mit **Touch** (wischen oder tippen zum Blättern); Prüfung und Einrichtung der Windows-Touch-Zuordnung unter **Display**
- **Deutsch oder Englisch**: Einstellungsseite, Display und Infobereich lassen sich umschalten; beim ersten Start folgt die Sprache Windows
- **Bebilderte Anleitung** zum Durchklicken, verständlich auch für Kinder, in der EXE eingebettet auf Deutsch und Englisch
- **Eine EXE** für Installation, Updates und Deinstallation; kein Python und kein Visual-C++-Redistributable erforderlich. WebView2 bleibt eine Voraussetzung.

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

1. **FuchsLabs-USB-Display.exe** aus dem [aktuellen Release](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/latest) herunterladen (Stand: **2.0.3**). Die EXE ist unsigniert. Daneben liegt `SHA256SUMS.txt` zur Dateiprüfung.
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
pages = ["system", "friends", "discord", "bf6", "rss"]   # Reihenfolge
theme = "hell"             # "hell", "dunkel" oder "eigen"
touch = false              # nur bei output = "monitor"

[bf6]
enabled = true
players = ["FuchsKumpel", "Lena_Sniper@psn"]

[rss]
enabled = true
feeds = ["https://winfuture.de/"]
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
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display\cpp
build.bat nopause
```

Der C++-Build installiert die Abhängigkeiten aus dem festgelegten vcpkg-Manifest/Overlay, baut Release, führt `fl-tests.exe` aus und erzeugt `cpp\dist\FuchsLabs-USB-Display.exe`. Die `build.bat` im Repository-Hauptordner gehört zur historischen Python-Version 1.x. Details: [C++-Buildhinweise](cpp/README.md#build) und [Mitarbeiten](CONTRIBUTING.md).

WebView2-SDK-Header, statischer Loader und Lizenzhinweise liegen unter `cpp/third_party/webview2`; die Laufzeit selbst ist extern. Übersetzungen: `cpp/src/i18n.cpp` für App/Display und `cpp/res/webui.html` für die Einstellungen.

## Projektaufbau

| Pfad | Aufgabe |
|---|---|
| `cpp/src/` | C++-App, Displayausgabe, Datenquellen, natives Einstellungsfenster und Installer |
| `cpp/res/` | Eingebettete Einstellungen, Anleitungen, Symbol und Versionsressourcen |
| `cpp/tests/` | C++-Prüfungen |
| `cpp/overlay-ports/`, `cpp/vcpkg.json` | Abhängigkeiten und D++-Buildkonfiguration |
| `cpp/TESTPROTOKOLL.md` | Lokaler Windows-/Displaytest |
| `*.py`, `build.bat` im Hauptordner, `anleitung/` | Historischer Python-Stand 1.x und dessen Anleitungsgenerator |

## Sicherheit und Datenschutz

- Läuft **ohne Administratorrechte**, ohne Telemetrie, ohne Konto.
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

Keine Open-Source-Lizenz: © 2026 FuchsLabs, alle Rechte vorbehalten. Das Repository ist öffentlich lesbar; daraus folgt keine Open-Source-Lizenz. Für eingebundene Drittkomponenten gelten deren eigene Lizenzen.

Battlefield, Discord, Steam und Xbox sind Marken ihrer jeweiligen Inhaber. FuchsLabs steht in keiner Verbindung zu ihnen.

## Prüfstand 2.0.3

Release-Build, 86 Kernprüfungen und 56 Monitor-/Touchprüfungen mit injizierten Windows-Eingaben bestanden (die 56 enthalten 11 Basisprüfungen). Installierte 2.0.3: tatsächliche Datei-Version, Hash, API-Version, Autostart-Eintrag und `display_ok=true` außerhalb der Paketumleitung geprüft. Echte Touch-Hardware, der vollständige Windows-Zuordnungsassistent und ein neuer Windows-Anmeldetest für 2.0.3 bleiben offen. Siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

Die frühere Abnahme von 2.0 umfasste den Gegenvergleich mit 1.x an COM7 und die vom Nutzer bestätigte physische Anzeige sowie einen erfolgreichen Windows-Neustart mit Tray-Symbol.

Der erste reale Neustarttest scheiterte an der Paketumleitung des Installationshosts. Installation und tatsächlicher Windows-Autostarteintrag wurden danach außerhalb dieses Kontexts korrigiert; der Nutzer bestätigte anschließend den erfolgreichen Windows-Neustart mit Tray-Symbol.
