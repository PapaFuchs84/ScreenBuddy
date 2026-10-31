<div align="center">

<img src="docs/logo/fuchslabs-logo.png" alt="FuchsLabs" width="96"><br>

# ScreenBuddy

**Deutsch** · [English](README.en.md)

**Dein Zusatzbildschirm zeigt, was dich interessiert: Systemwerte, Freunde, Spielstände und Neuigkeiten.**

![Version](https://img.shields.io/badge/version-2.8.0-F77F00)
![Plattform](https://img.shields.io/badge/plattform-Windows%2010%20%7C%2011-14213D)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-Testphase-F77F00)
![Sprachen](https://img.shields.io/badge/sprachen-Deutsch%20%7C%20English-14213D)
![Lizenz](https://img.shields.io/badge/lizenz-MIT-green)

[![Spendier mir einen Kaffee](https://img.shields.io/badge/☕_Spendier_mir_einen_Kaffee-PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.me/SHerbst84)

<img src="docs/screenshots/display-system.png" width="360" alt="Systemseite im hellen Design">&nbsp;
<img src="docs/screenshots/display-freunde.png" width="360" alt="Freundesliste im hellen Design">

</div>

Windows-App für 3,5-Zoll-USB-Displays (480 × 320) und zusätzliche HDMI-/USB-C-Bildschirme, auch mit Touch. Sie läuft im Hintergrund und bietet native Windows-Einstellungen. Die C++20-App enthält den Installer in derselben EXE; Python, WebView2 und ein Visual-C++-Redistributable werden nicht benötigt.

**Neu in 2.8.0:** Spotify-Seite (Titel, Cover, Touch-Tasten; [Anleitung](docs/SPOTIFY.md)) und Seite „Neu bei Streaming“ mit TMDB-Daten; Fehlerkorrektur im Uninstall-Schutz für Testinstanzen. [Release-Notizen 2.8](docs/RELEASE-NOTES-2.8.md).`n`n**Neu in 2.7.0:** mehrere Companion-Geräte gleichzeitig (synchron oder unabhängig), sortier- und ausblendbare Plugin-Seiten, signierte Store-Vorschaubilder, interaktive Plugin-Werkstatt; Store-Installationen bleiben ausgeschaltet, bis du sie aktivierst. [Release-Notizen 2.7](docs/RELEASE-NOTES-2.7.md).

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
| **Streaming-Neustarts** (neu in 2.8.0) | neue Filme und Serien der letzten 30 Tage bei deinen gewählten Streaming-Anbietern (Titel, Datum, Film/Serie, Anbieter) | [TMDB](https://www.themoviedb.org) API, Anbieterdaten von JustWatch; eigener Key | 1 h |
| **Home Assistant** | bis zu 4 Sensoren (Wert, Einheit) oder Schalter (per Touch schaltbar) | Home-Assistant-REST-API | 15 s |
| **Spotify** (neu in 2.8.0) | aktueller Titel mit Cover und Fortschritt; mit Touch Zurück/Pause/Weiter (Premium) | Spotify Web API, [Anleitung](docs/SPOTIFY.md) | 4 s |
| **Plugins** | eigene Seiten aus geprüften, deklarativen Plugins | Plugin-Store oder lokales Manifest | je Plugin |

*Streaming-Neustarts nutzen die TMDB-API: This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.*

Dazu kommen:

- **Natives Windows-Einstellungsfenster** (Direct2D/DirectWrite, ohne WebView2) mit Live-Vorschau, Seitenreihenfolge, Verbindungsstatus je Plattform und aufklappbaren Hilfen neben jedem Schlüsselfeld. Der lokale Dienst unter `http://127.0.0.1:8765/` stellt die lokale Schnittstelle sowie die Anleitung unter `/anleitung` und die Plugin-Werkstatt unter `/plugin-werkstatt` bereit; die Companion-Verbindung läuft getrennt über Port 8777.
- **Ersteinrichtung ohne Vorkenntnisse**: erkennt das Display, beendet und deaktiviert auf Wunsch die Hersteller-App, Autostart ohne Administratorrechte
- **Plugin-Store** (ab 2.6.0, abschaltbar): signierter Store mit Prüfung vor der Installation; Plugins bleiben aus, bis du sie aktivierst. Mehr: [Plugin-Store und Plugins](docs/FUNKTIONEN.md#plugin-store-und-plugins-ab-260)
- **Android-Companion** (ab 2.3.0, mehrere Geräte ab 2.7.0): Handy oder Tablet als weitere Anzeige, Kopplung per QR-Code oder PIN. Mehr: [Companion](docs/FUNKTIONEN.md#android-companion-app-ab-230-mehrere-geräte-ab-270)
- **Design**: FuchsLabs-CI in Hell oder Dunkel, oder **eigene Farben** per Farbwähler mit Kontrastwarnung
- **Jede Auflösung**: Seiten werden scharf auf die echte Größe gerendert; breite Leisten zeigen mehrere Seiten nebeneinander
- **HDMI/USB-C-Bildschirme** als randloses Vollbild über der Taskleiste, optional mit **Touch** (wischen oder tippen zum Blättern, Tipp auf Beiträge, Freunde, Spieler und Systemkarten öffnet passende Ziele); Prüfung und Einrichtung der Windows-Touch-Zuordnung unter **Anzeige**. Details: [Touch und Update-Hinweis](docs/FUNKTIONEN.md#systemseite-per-touch-und-update-hinweis-ab-220), [Touch-Ziele](docs/FUNKTIONEN.md#touch-ziele-und-spielstunden-ab-208)
- **Home Assistant** mit Sensoren und Schaltern: [Einrichtung und Sicherheitshinweise](docs/FUNKTIONEN.md#home-assistant-ab-210)
- **Deutsch oder Englisch**: Einstellungsseite, Display und Infobereich lassen sich umschalten; beim ersten Start folgt die Sprache Windows
- **Bebilderte Anleitung** zum Durchklicken, verständlich auch für Kinder, in der EXE eingebettet auf Deutsch und Englisch
- **Seiten weiterschalten** per Knopfdruck (Companion-Seite der Einstellungen): [Beschreibung](docs/FUNKTIONEN.md#ansichten-durchblättern-ab-204)
- **Update-Hinweis** (abschaltbar): meldet höchstens einmal am Tag eine neue GitHub-Version, installiert aber nie selbst
- **Eine EXE** für Installation, Updates und Deinstallation

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

1. **ScreenBuddy.exe** aus [Release 2.8.0](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.8.0) herunterladen. Die Windows-EXE ist unsigniert; mit `SHA256SUMS.txt` prüfen. Die Companion-App 1.0.1 für Android ist ein separater Download.
2. Windows 10/11 x64 verwenden. Ab Version 2.5 wird **keine** Microsoft Edge WebView2 Runtime mehr benötigt; die frühere Version 2.3.0 braucht sie noch.
3. Display anschließen, EXE starten und den Installer abschließen. Standardziel: `%LOCALAPPDATA%\FuchsLabs\USB-Display`, ohne Administratorrechte.
4. Bei der ersten Einrichtung öffnet sich ein **eigenes Windowsfenster**. Gewünschte Seiten aktivieren und die Datenquellen einrichten, dann **Speichern & anwenden**.
5. Später öffnen ein Doppelklick auf das Fuchs-Symbol im Infobereich, dessen Menüpunkt **Einstellungen öffnen** oder ein erneuter Start der installierten App das Einstellungsfenster. Dessen Schließen lässt das Display weiterlaufen; **Beenden** im Infobereich stoppt die App.

Der Installer kann Einstellungen aus Version 1.x übernehmen und bewahrt den alten Ordner mit dem Zusatz `-1.x-alt` auf. Bei einem Update bleibt eine vorhandene Zielkonfiguration erhalten. Vor jedem Upgrade EXE, `config.toml` und `companion.json` sichern. Die bebilderte Anleitung öffnet sich über **Anleitung öffnen** im Menü des Fuchs-Symbols im Standardbrowser und ist in der EXE eingebettet.

Für einen HDMI-/USB-C-Touchscreen unter **Anzeige** den vorgesehenen Monitor wählen. Erscheinen Berührungen auf einem anderen Bildschirm, **Touch-Bildschirm zuordnen** öffnen: andere Bildschirme mit Enter überspringen und den vorgesehenen Touchscreen berühren. Danach **Erneut prüfen** wählen. Die USB-Touchverbindung muss bestehen. Details im [Schnellstart](ANLEITUNG.md#hdmiusb-c-und-touch).

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
pages = ["system", "friends", "discord", "bf6", "rss", "streaming", "homeassistant", "spotify"]   # Reihenfolge
theme = "hell"             # "hell", "dunkel" oder "eigen"
touch = true               # nur bei output = "monitor"

[bf6]
enabled = true
players = ["FuchsKumpel", "Lena_Sniper@psn"]

[rss]
enabled = true
feeds = ["https://winfuture.de/"]

[streaming]
enabled = true
region = ""                # leer = Deutsch: DE, Englisch: US
providers = ["8|Netflix", "337|Disney+"]   # TMDB-Provider-IDs; der Key steht verschlüsselt in api_key

[spotify]
enabled = true
client_id = ""             # Client-ID deiner Spotify-Developer-App

[homeassistant]
enabled = true
url = "http://homeassistant.local:8123"
entities = ["sensor.wohnzimmer_temperatur", "light.stehlampe"]
```

Tokens und Schlüssel stehen dort nur verschlüsselt (`dpapi:…`). Alle Schlüssel mit Standardwerten beschreibt [`DOKUMENTATION.md`](DOKUMENTATION.md#4-konfigurationsdatei). Die Sprache steht unter `[ui]` als `language = "de"` oder `"en"`.

### Kommandozeile

| Option | Wirkung |
|---|---|
| `-c DATEI` | andere Konfigurationsdatei verwenden |
| `--demo` | alle Seiten mit Beispieldaten zeigen |
| `--no-lcd` | ohne Display starten (zum Testen der Einstellungsseite) |
| `--preview ORDNER` | jedes Bild zusätzlich als PNG speichern |
| `--log DATEI`, `-v` | Protokoll in eine Datei, ausführlich |

## Unterstützte Hardware

| Gerät | Anschluss | Hinweis |
|---|---|---|
| 3,5"-USB-Display 480 × 320 („Revision A“) | USB (serielle Schnittstelle, 115200 Baud) | wird automatisch gefunden; COM-Port bei Bedarf von Hand |
| Zusatzbildschirm beliebiger Größe (Beispiel in den Tests: 1920 × 720) | HDMI oder USB-C (DisplayPort) | randloses Vollbild auf dem gewählten Bildschirm, optional Touch |
| Android-Handy oder -Tablet (Android 8+) | WLAN/USB-Tethering, Companion-App | mehrere Geräte möglich |
| GPU-Werte | Nvidia mit Treiber (`nvidia-smi`) | andere GPUs zeigen „–“ |

## Selbst bauen und entwickeln

Voraussetzungen: Windows x64, Visual Studio Build Tools 2022 mit C++/Windows SDK und CMake/Ninja sowie ein eingerichteter vcpkg-Checkout unter `%USERPROFILE%\FuchsLabs\vcpkg`.

```bat
git clone https://github.com/PapaFuchs84/ScreenBuddy.git
cd ScreenBuddy\cpp
build.bat nopause
```

Der C++-Build installiert die Abhängigkeiten aus dem festgelegten vcpkg-Manifest/Overlay, baut Release, führt Kern-, Plugin-, Monitor-, Companion- und isolierte Installer-Prüfungen aus und erzeugt lokal `cpp\dist\ScreenBuddy.exe`. Die `build.bat` im Repository-Hauptordner gehört zur historischen Python-Version 1.x. Details: [C++-Buildhinweise](cpp/README.md#build), [Architektur](docs/ARCHITEKTUR.md) und [Mitarbeiten](CONTRIBUTING.md).

Übersetzungen: `cpp/src/i18n.cpp` für App/Display, native Texte unter `cpp/src/ui_page_*.cpp`. WebView2-Dateien liegen nur noch unter `cpp/third_party/webview2` und werden nicht eingebunden; `/` liefert keine HTML-Einstellungen mehr aus.

## Projektaufbau

| Pfad | Aufgabe |
|---|---|
| `cpp/src/` | C++-App, Displayausgabe, Datenquellen, natives Einstellungsfenster und Installer |
| `cpp/res/` | Eingebettete Anleitungen, Symbol und Versionsressourcen |
| `cpp/tests/` | C++-Prüfungen |
| `cpp/overlay-ports/`, `cpp/vcpkg.json` | Abhängigkeiten und D++-Buildkonfiguration |
| `cpp/TESTPROTOKOLL.md` | Lokaler Windows-/Displaytest |
| `android/` | Companion-App für Android |
| `docs/` | Fachdokumentation, siehe [Übersicht](docs/README.md) |
| `*.py`, `build.bat` im Hauptordner, `anleitung/` | Historischer Python-Stand 1.x und Generator für die aktuelle DE/EN-Anleitung |

## Sicherheit und Datenschutz

- Läuft **ohne Administratorrechte**, ohne Telemetrie, ohne Konto. Die einzige App-eigene Anfrage ohne deine Datenquellen ist der abschaltbare Update-Hinweis an die öffentlichen GitHub-Releases (höchstens 1× am Tag, ohne Schlüssel).
- Zugangsdaten werden mit **Windows-DPAPI** verschlüsselt gespeichert, nie protokolliert und nie an den Browser gegeben.
- Der lokale Dienst ist **nur auf `127.0.0.1`** erreichbar und gegen DNS-Rebinding, CSRF und Einbetten in fremde Seiten geschützt.
- Alle Verbindungen laufen über HTTPS mit Zertifikatsprüfung; Pakete sind auf genaue Versionen festgelegt.

Historischer Python-1.x-Prüfbericht: [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md); er ist kein vollständiger Audit der C++-Version. Aktuelle Validierung: [Release-Notizen 2.7](docs/RELEASE-NOTES-2.7.md); das [Testprotokoll](cpp/TESTPROTOKOLL.md) ist auf dem Stand 2.6.0 (30.10.2026) und enthält keinen Eintrag für 2.7.0. Eine Lücke meldest du wie in [`SECURITY.md`](SECURITY.md) beschrieben, bitte nicht als öffentliches Issue.

## Testphase und Rückmeldungen

Die App ist in der Testphase. Fehler und Ideen bitte über **Issues → New issue** mit den Vorlagen „Fehlerbericht“ oder „Idee oder Rückmeldung“ melden. Wo das Protokoll `usb-display.log` liegt und was nie mitgeschickt werden darf, steht in [`TESTPHASE.md`](TESTPHASE.md).

**Prüfstand 2.7.0:** 1.493 automatisierte Prüfungen im Release-Build bestanden; Echtsystem mit zwei Android-Geräten und dem USB-Display (480 × 320) beobachtet. Mehrgeräteprüfungen per adb sind Simulationen; DPI-/Screenreader-, Windows-Anmelde-, WLAN- und Boot-Abnahme stehen aus. Vollständig und mit allen Grenzen: [Release-Notizen 2.7](docs/RELEASE-NOTES-2.7.md). Ältere Prüfstände: [Historie](docs/PRUEFSTAND-HISTORIE.md).

## Dokumente

Alle Dokumente nach Zielgruppe: [`docs/README.md`](docs/README.md). Die wichtigsten:

| Datei | Inhalt |
|---|---|
| [`README.en.md`](README.en.md) | dieses README auf Englisch |
| [`ANLEITUNG.md`](ANLEITUNG.md) | Schnellstart für Anwender |
| [`docs/FUNKTIONEN.md`](docs/FUNKTIONEN.md) | Funktionen im Detail ([English](docs/FEATURES.en.md)) |
| [`DOKUMENTATION.md`](DOKUMENTATION.md) | ausführliche Referenz: Seiten, Einstellungen, `config.toml`, HWiNFO, Fehlerbehebung |
| [`docs/ARCHITEKTUR.md`](docs/ARCHITEKTUR.md) | Architektur für Entwickler |
| [`docs/PLUGIN-SYSTEM.md`](docs/PLUGIN-SYSTEM.md) | Plugin-Anleitung ([English](docs/PLUGIN-SYSTEM.en.md)), [Tutorial](docs/PLUGIN-TUTORIAL.md), [Store-Bedingungen](docs/PLUGIN-STORE-BEDINGUNGEN.md) |
| [`docs/COMPANION.md`](docs/COMPANION.md) | Companion einrichten ([English](docs/COMPANION.en.md)) |
| [`docs/RELEASE-NOTES-2.7.md`](docs/RELEASE-NOTES-2.7.md) | Release-Notizen der aktuellen Version |
| [`CHANGELOG.md`](CHANGELOG.md) | Änderungen je Version ([English](CHANGELOG.en.md)) |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Mitarbeiten ([English](CONTRIBUTING.en.md)) |
| [`SECURITY.md`](SECURITY.md) | Sicherheitslücken melden ([English](SECURITY.en.md)) |
| [`TESTPHASE.md`](TESTPHASE.md) | Rückmeldungen in der Testphase ([English](TESTPHASE.en.md)) |
| [`PROTOKOLL.md`](PROTOKOLL.md) | Entstehungsverlauf und Entscheidungen |

## Unterstützen

Dir gefällt die App? Über einen Kaffee freut sich FuchsLabs: [☕ paypal.me/SHerbst84](https://www.paypal.me/SHerbst84)

## Lizenz

Der Code steht unter der [MIT-Lizenz](LICENSE), © 2026 FuchsLabs. Name und Fuchskopf-Logo sind ausgenommen ([TRADEMARKS.md](TRADEMARKS.md)). Für eingebundene Drittkomponenten gelten deren eigene Lizenzen ([THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)).

Battlefield, Discord, Steam und Xbox sind Marken ihrer jeweiligen Inhaber. FuchsLabs steht in keiner Verbindung zu ihnen.
