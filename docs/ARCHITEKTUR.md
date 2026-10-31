# ScreenBuddy · Architektur (arc42 kompakt)

Stand: 2.9.3 (Datenquellen, Companion-Version und Testverweise am 31.10.2026 gegen `cpp/src/` geprüft). Dieses Dokument ist aus der Repository-Struktur (`cpp/src/`), dem [C++-README](../cpp/README.md) und den Release-Notizen abgeleitet. Aussagen zu einzelnen Dateien beruhen auf deren Namen und den dokumentierten Funktionen; was nicht im Quellcode nachgeprüft wurde, steht unter [Offene Punkte](#9-offene-punkte).

## 1. Einführung und Ziele

ScreenBuddy ist eine native Windows-App (C++20, eine EXE inklusive Installer) für 3,5-Zoll-USB-Displays (480 × 320) und zusätzliche HDMI-/USB-C-Bildschirme, optional mit Touch. Android-Geräte lassen sich über die Companion-App als weitere Anzeige koppeln.

| Ziel | Bedeutung |
|---|---|
| Einsatz ohne Vorkenntnisse | Assistent, Auto-Erkennung, bebilderte Anleitung DE/EN |
| Sicher | keine Administratorrechte, DPAPI für Schlüssel, Loopback-Dienst, geprüfte Plugins |
| Effizient | eine EXE, kein Python, kein WebView2 (ab 2.5), kein Visual-C++-Redistributable |
| Erweiterbar | deklarative, signierte Plugins aus dem Store |

## 2. Randbedingungen

- Windows 10/11 x64; Build mit Visual Studio Build Tools 2022, CMake/Ninja, vcpkg (Details: [`cpp/README.md`](../cpp/README.md#build)).
- Abhängigkeiten statisch gelinkt (u. a. D++ für Discord, OpenSSL).
- Die Windows-EXE ist unsigniert; Prüfsummen stehen in `SHA256SUMS.txt` am Release.
- MIT-Lizenz; Name und Logo per [Markenhinweis](../TRADEMARKS.md) ausgenommen.

## 3. Kontext

```mermaid
flowchart LR
  user[Nutzer] --> settings[Natives Einstellungsfenster]
  app[ScreenBuddy.exe]
  settings <--> app
  app --> lcd[USB-Display seriell COM]
  app --> mon[HDMI/USB-C-Bildschirm + Touch]
  app <-->|TLS WebSocket 8777, QR/PIN| phone[Android-Companion]
  app --> discord[Discord-Bot]
  app --> steam[Steam Web API]
  app --> xbox[OpenXBL]
  app -.->|optional, inoffiziell, NPSSO| psn[PlayStation Network]
  app --> spotify[Spotify Web API]
  app --> bf6[gametools.network]
  app --> rss[RSS/Atom-Feeds]
  app --> tmdb[TMDB API v3 + JustWatch-Anbieterdaten]
  app --> ha[Home Assistant REST]
  app --> gh[GitHub Releases: Update-Hinweis]
  app --> store[Plugin-Store ScreenBuddy-Plugins]
  app --> hw[HWiNFO Shared Memory / nvidia-smi / Windows-APIs]
```

## 4. Lösungsstrategie

- **Ein Prozess, mehrere Ausgaben:** Seiten werden einmal pro Ausgabegröße gerendert und an USB-Display, Monitor und Companion geliefert.
- **Datenquellen entkoppelt:** jede Quelle liefert Modelle (`models.h`), die Renderer ist von den Quellen getrennt.
- **Selbst gezeichnete Oberfläche** (Direct2D/DirectWrite) statt WebView2; der lokale HTTP-Dienst (`webui`, Standardport 8765, nur `127.0.0.1`) bleibt als Schnittstelle und für die eingebettete Anleitung; `/` liefert keine HTML-Einstellungsseite mehr.
- **Sicherheit durch Voreinstellung:** Plugins sind rein deklarativ, Installationen bleiben ausgeschaltet, Hosts brauchen Freigabe.

## 5. Bausteinsicht

Gruppierung nach den Dateien in `cpp/src/`:

| Baustein | Dateien | Aufgabe |
|---|---|---|
| Start und Ablauf | `main`, `app`, `tray` | Prozessstart, Hauptschleife, Infobereich-Symbol |
| Installation | `installer`, `winsetup`, `update` | Installer in der EXE, Windows-Einrichtung/Autostart, Update-Hinweis |
| Konfiguration | `config`, `secure`, `i18n`, `brand`, `theme`, `key_help` | `config.toml`, DPAPI-Schlüssel, DE/EN-Texte, CI, Farbdesigns und Designsysteme, Schlüssel-Hilfen mit festen Anbieter-Adressen |
| Datenquellen | `sysinfo`, `discord`, `friends`, `psn_logic`, `bf6`, `rss`, `tmdb`, `tmdb_logic`, `ha`, `spotify`, `amp_logic`, `audio_levels` | System, Discord, Freunde (Discord/Steam/Xbox und optional PlayStation über Sonys inoffizielle Schnittstelle; `psn_logic` liest nur die Antworten, plattformneutral und unter Linux testbar), Battlefield 6, Blogs, Streaming-Neustarts (TMDB; `tmdb_logic` plattformneutral und unter Linux testbar), Home Assistant, Spotify mit Amplifier (`amp_logic` plattformneutral; `audio_levels` liest den Pegel der Systemausgabe lokal) |
| Netz und Schutz | `http`, `netguard`, `webui` | HTTPS-Abrufe, Prüfung auf nicht öffentliche Ziele, lokaler Loopback-Dienst |
| Darstellung | `render`, `gfx`, `models` | Seiten rendern, Grafikhilfen, Datenmodelle |
| Ausgabe | `output.h`, `serial_display`, `monitor_display`, `network_display`, `touch` | USB-Display, Vollbild-Monitor, Netzwerk-/Companion-Ausgabe, Touch-Eingabe |
| Companion | `companion`, `companion_server` | Kopplung, TLS-WebSocket, Gerätesitzungen (plattformneutral, unter Linux testbar) |
| Plugins | `plugins`, `plugin_render`, `plugin_settings`, `plugin_transport`, `plugin_catalog`, `plugin_catalog_net`, `plugin_store`, `plugin_sign` | Manifest, Rendern, Einstellungen, Abruf (nur GET auf öffentliche HTTPS-Hosts), Katalog/Index, Store, Signaturprüfung |
| Einstellungsoberfläche | `native_settings`, `ui_kit`, `ui_logic`, `ui_pages`, `ui_page_*` | Fenster, Steuerelemente, Logik, eine Datei je Einstellungsseite |
| Ressourcen | `resources` | eingebettete Anleitung, Symbole, Versionsdaten |

Nicht Teil der aktuellen App: der historische Python-Stand 1.x im Hauptordner (`*.py`, `build.bat`), `cpp/src/settings_window.cpp` (alte WebView2-Einstellungsseite, nicht in `CMakeLists.txt` eingebunden) und `cpp/third_party/webview2` (Altbestand mit Headern und Bibliotheken, wird nicht gelinkt).

## 6. Laufzeitsicht

```mermaid
sequenceDiagram
  participant Q as Datenquellen
  participant R as Renderer
  participant O as Ausgaben
  loop pro Seite (page_seconds, Standard 15 s)
    Q->>R: aktuelle Modelle
    R->>O: Bild je Ausgabegröße
  end
  O-->>R: Touch/Wischen (Monitor, Companion)
```

- Mehrere Companions laufen seit 2.7 gleichzeitig, synchron oder unabhängig, mit eigener Auflösung und optional eigener Seitenliste.
- Kopplung: QR-Code oder PIN (6-stellig, 2 Minuten gültig, Sperre nach Fehlversuchen), Standardport 8777 (muss sich vom Port 8765 unterscheiden), nur private IPv4-Adressen, mDNS `_fuchslabs-display._tcp`, SHA-256-Fingerabdruck sichert TLS, Schlüssel DPAPI-geschützt in `companion.json` ([Protokoll](COMPANION-PROTOKOLL.md)).
- Update-Hinweis: höchstens einmal am Tag, nie automatische Installation.

## 7. Verteilung

Eine unsignierte `ScreenBuddy.exe` (Installer, App, Updater, Deinstaller). Standardziel `%LOCALAPPDATA%\FuchsLabs\USB-Display`. Daten: `config.toml`, `companion.json`, `usb-display.log`, `update-check.json`. Die Companion-APK (Quellstand 1.0.2, versionCode 6; signiert) ist ein separater Download. Plugins kommen aus dem Store-Repo `PapaFuchs84/ScreenBuddy-Plugins` (signierter Index, Pages).

## 8. Querschnitt

- **Sicherheit:** keine Admin-Rechte, keine Telemetrie; DPAPI (auch für den optionalen PlayStation-NPSSO-Code, der nur an Sony gesendet wird); Dienst nur auf `127.0.0.1` mit Host-, Fetch-Metadata- und `X-FL: 1`-Prüfung; Ziele und Weiterleitungen gegen nicht öffentliche IP-Bereiche geprüft; Ed25519-signierter Store-Index mit festem öffentlichem Schlüssel und Sperrliste; Aktionen wie Schlösser oder Alarmanlagen in Home Assistant bewusst nicht schaltbar.
- **Sprache:** Standard folgt Windows; Texte in `cpp/src/i18n.cpp` und `ui_page_*.cpp`.
- **Design:** FuchsLabs-CI (Fuchs-Orange, Creme, Navy), hell/dunkel/eigene Farben mit Kontrastwarnung.
- **Tests:** Kern-, Parser- (u. a. TMDB, PlayStation), Plugin-, Monitor-, Companion- und Installer-Prüfungen im Build (`cpp/tests/`); Echtgeräteprüfungen und Grenzen im [Testprotokoll](../cpp/TESTPROTOKOLL.md) (aktuell mit Eintrag „Release 2.9.3“).

## 9. Offene Punkte

- Dateiliste, Ports, Pfade, PIN-Regeln, Loopback-Prüfungen, Update-Takt und Plugin-Transport (nur GET, HTTPS, keine Weiterleitungen) wurden stichprobenartig gegen `cpp/src/` und `CMakeLists.txt` geprüft; das Dokument ist nicht Zeile für Zeile verifiziert.
- Online-Plugins und CRL/OCSP-Prüfungen gelten weiter als experimentell.
- Packaging-Umleitung unter Windows kann einen anderen Einstellungsstand sichtbar machen (siehe [Release-Notizen 2.7](RELEASE-NOTES-2.7.md)).
- Juristische Prüfung der Store-Bedingungen steht aus.
