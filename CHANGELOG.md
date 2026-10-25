# Changelog – FuchsLabs USB-Display

**Deutsch** · [English](CHANGELOG.en.md)

Alle nennenswerten Änderungen an der App. Format angelehnt an [Keep a Changelog](https://keepachangelog.com/de/1.1.0/), Versionen nach [SemVer](https://semver.org/lang/de/).

## [Unreleased] – ScreenBuddy 2.5 in Entwicklung

### Architektur
- Native Win32-Hülle nutzt den vorhandenen C++20-Kern für Rendering, USB und Companion; dadurch ist keine zusätzliche Electron-Laufzeit nötig. WebView2 bleibt für geführte Abläufe erhalten. Kein Flutter-/Tauri-Umbau in dev.2. Release-Gates sind UI-, Upgrade-, Hardware- und externe-Dienst-Abnahme. Der Installer sichert eine Vorgänger-EXE; ein echtes Rollback ist noch nicht abgenommen. Öffentliche Umbenennung von Repository und Homepage ist bis nach der Freigabe aufgeschoben.

### Implementiert im unveröffentlichten Entwicklungsbuild
- ScreenBuddy-Branding für EXE, Installer und Autostart; bestehender Uninstall-Schlüssel für Upgrades erhalten.
- Native Win32-Einstellungen mit sieben Reitern, darunter Quellen-Editor, Seitenreihenfolge, Updates und Autostart; geführte Workflows, benutzerdefinierte Farben und Companion-Kopplung bleiben in WebView2.
- Streng validierte Plugin-Manifeste, lokale Verwaltung, begrenzter HTTPS-Abruf im Worker, gespeicherte Host-Zustimmungen, DPAPI-geschützte Plugin-Einstellungen und Live-Rendering im Display-Thread. Quellen-Plugins sind nach Pflicht-Einstellungen und Einzelzustimmung pro Host aktivierbar; externe HTTPS-Liveabfragen wurden nicht geprüft.
- Details zum Entwicklungsstand und zu den Kompatibilitätsgrenzen: [ScreenBuddy 2.5](docs/SCREENBUDDY-2.5.md).

### Sicherheitshärtung nach Review
- Gespeicherte Plugin-Secrets werden verworfen, wenn ein Manifest-Update Quelle (Host, URL oder Header) ändert.
- Windows-Gerätenamen wie `con`, `nul`, `com1`–`com9` und `lpt1`–`lpt9` werden als Plugin-ID abgelehnt.
- Plugin-Secrets nutzen eine eigene DPAPI-Entropie; vorhandene Werte werden automatisch migriert.
- Bidi- und Zero-Width-Zeichen in Manifesttexten werden abgelehnt.
- `.` und `..` werden als Einstellungswert im URL-Pfad abgelehnt.
- Der DNS-Abschluss ist gegen Doppelfreigabe abgesichert.
- Offen: Zertifikats-Sperrprüfung (CRL/OCSP) fehlt noch; externe HTTPS-Liveabfragen sind weiterhin nicht abgenommen.

### Status
- Kein 2.5-Release oder Download. Die bestehende 2.3.0 bleibt die letzte veröffentlichte Version. Bestanden: 236 Kern-, 220 Parser-, 127 Speicher-, 133 Transport-, 135 Plugin-Einstellungs-, 11 Monitor- und 335 Companion-Prüfungen; außerdem 39 isolierte App-, 49 Navigations- und zwei Installer-Prüfungen. Externe HTTPS-Abfragen, interaktive/visuelle UI-Abnahme, echtes Upgrade und Hardware offen.

## [2.3.0] – 2026-10-24

### Neu
- **Companion-Vorschau für Android:** USB/HDMI und Handy haben getrennte Seitenlisten, Seitenstände, Wechselintervalle und Touch-Ziele. Eigene Companion-Einstellungen erscheinen nur, wenn Companion aktiv ist; Datenquellen und Design bleiben gemeinsam.
- Android meldet native Bildgröße und Rotation. PC sendet dazu passende Bilder und verwirft veraltete Touch-Koordinaten. Seitenwahl, Timer und Vorschau sind separat steuerbar. Siehe [Einrichtung und Grenzen](docs/COMPANION.md).
- Begleitende Android-Vorschau-APK `FuchsLabs-Companion-0.2.0-preview.apk`.

### Geändert
- Windows-Version auf 2.3.0 aktualisiert; Deutsch-/Englisch-Anleitung, Testprotokoll und eingebettete Hilfe synchronisiert.
- Performance & Verschlankung: Zeilenweises `std::memcpy` beim Kacheln (`paste()`), Stack-Puffer-Schnellpfad für `strprintf()` zur Vermeidung dynamischer Speicherbelegung, schneller Hex-Farb-Parser ohne Regex/Heap-Allokation, lokaler Inline-Pinsel-Cache in `Canvas::Impl`, Vermeidung von Formatierungsaufwand bei inaktivem Debug-Logging (`is_verbose()`), String-Pooling (`/GF`) sowie Beseitigung aller Compiler-Warnungen in `render.cpp` und `sysinfo.cpp`.
- Die Installation übernimmt vorhandene Einstellungen neben einer portablen EXE. Vor einem Update wird eine lokale Einstellungs-Sicherung angelegt; Klartext-Zugangsdaten werden darin verschlüsselt abgelegt.

### Behoben
- Beenden und `POST /api/quit` blockieren nicht mehr bis zu 30 Sekunden im Infobereich (`Tray`), wenn die Taskleiste nicht verfügbar ist (nicht blockierender `WM_TIMER` statt synchronem `Sleep`).
- Die Deinstallation mit Bereinigung (`--purge`) löscht nun auch `config.toml.install-backup`, `companion.json` (samt Sicherungen/Temporärdateien) und `update-check.json`.
- Die Installation übernimmt neben einer portablen EXE vorhandene Companion-Kopplungen (`companion.json`).
- Bebilderte Anleitungsvorlagen (`anleitung.template.html`, `anleitung.template.en.html`) mit den eingebetteten Ressourcen synchronisiert; Dokumentations- und Versionsverweise auf 2.3.0 korrigiert.

### Prüfumfang
- Redmi 8 bestätigt: automatische Auflösung, Hoch-/Querformat, Seitenwechsel, Fingerwischen und Tippen, Kopplung und Wiederverbindung. Der physische USB-Ausgang gemeinsam mit dem Companion sowie WLAN, Start beim Android-Boot und mehrere reale Android-Geräte bleiben offen.

## [2.2.0] – 2026-10-24

### Neu
- Systemseite per Touch: Tipp auf **RAM** öffnet den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`), auf **Netzwerk** den Geschwindigkeitstest speedtest.net/de im Standardbrowser. Nur bei aktivem Touch.
- Update-Hinweis: höchstens einmal am Tag (45 s nach dem Start) werden die öffentlichen GitHub-Releases abgefragt, ohne Schlüssel. Eine neuere veröffentlichte Version erscheint als Sprechblase am Tray-Symbol und unter **Einstellungen → Start** (Link zur Download-Seite, **Jetzt prüfen**). Es wird nie etwas heruntergeladen oder installiert. Abschaltbar unter **Start → Auf neue Versionen hinweisen** (`[ui] update_check`). Zeitpunkt und gemeldete Version stehen in `update-check.json` neben der `config.toml`; Testinstanzen fragen nie ab.

### Geändert
- Bebilderte Anleitung (DE/EN) um Systemseite per Touch und Update-Hinweis ergänzt; Dokumentation auf 2.2.0.
- Dokumentation auf 2.1.0: README, Doku, Changelogs und Download-Verweise aktualisiert; bebilderte Anleitung (DE/EN) mit neuem Schritt „Home Assistant“ (Adresse, Token, Verbindung prüfen, Kacheln wählen). Die schon veröffentlichte EXE enthält noch die alte Anleitung; die neue kommt mit dem nächsten Build.
- Dokumentation auf 2.0.3 nachgezogen: README/Schnellstart, Downloadlinks, HDMI-/Touch-Anleitung, Prüfstand und Sicherheitsübersicht; vorhandene deutsche/englische Fassungen gemeinsam aktualisiert. Die Projektregeln und PR-Vorlage verlangen Doku-Pflege ab jetzt bei jeder Änderung.
- Bebilderte Anleitung (ANLEITUNG.html / GUIDE.html) auf C++ 2.0 gebracht: Download der einen EXE statt ZIP, eingebauter Installer (ohne Administratorrechte, Übernahme von 1.x), eigenes Einstellungsfenster (WebView2), Tray-Autostart, Deinstallation über Windows-Apps.
- Beim Start mit Windows startet die App still im Infobereich, ohne die Einstellungsseite zu öffnen (Autostart-Eintrag mit `--autostart`; ältere Einträge werden beim nächsten Start umgestellt). Auch nach einem Neustart aus den Einstellungen öffnet sich kein weiterer Browser-Tab.

### Sicherheit
- CodeQL-Prüfung auf GitHub. Schutz vor Abrufen ins Heimnetz (SSRF) gilt für jeden Blog-Abruf und jede Weiterleitung (`rss.public_url`).

### Behoben
- Prozentwerte in der Einstellungsseite (Helligkeit zeigte nach einer automatischen Korrektur „90 90“).

## [2.1.0] – 2026-10-23

Gemeinsamer Stand aus den beiden zuletzt entwickelten Änderungen; 2.0.9 wird nicht einzeln veröffentlicht.

### Neu
- **Home-Assistant-Seite**: bis zu 4 Entitäten aus Home Assistant auf dem Display. Sensoren (und Binärsensoren) zeigen Wert und Einheit, Schalter (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Einrichtung unter Einstellungen → Home Assistant: Adresse, langlebiges Zugriffstoken (DPAPI-verschlüsselt) und Auswahl der Entitäten aus einer Liste, die direkt aus Home Assistant geladen wird. Ohne Touch lassen sich nur Sensoren wählen; bereits gewählte Schalter erscheinen dann nicht. Zugriff über die REST-API, Abfrage alle 15 Sekunden. Schlösser, Rollläden und Alarmanlagen sind bewusst nicht schaltbar.

### Geändert
- Tipp auf einen Freund hat eine feste Reihenfolge: Steam-Chat, sonst das Discord-Profil (dort „Nachricht“ wählen), sonst das Xbox-Profil auf xbox.com im Browser. Fehlt die App eines Ziels (Steam, Discord), wird das nächste verwendet. Der Online-Status spielt keine Rolle mehr; in 2.0.8 öffnete nur Steam etwas.

## [2.0.8] – 2026-10-23

- Tipp auf einen BF6-Spieler öffnet das GameTools-Profil jetzt mit dem Plattformkürzel der Webseite (`pc`, `xboxone`, `ps4`) statt `ea`, das eine leere Seite ergab. Spieler, die GameTools für PC nicht kennt, zeigt die Webseite weiterhin als „nicht gefunden“, obwohl die API sie liefert.
- Tipp auf einen Freund öffnet nur noch den Steam-Chat; Discord und Xbox öffnen vorerst nichts (kein Chat-Link verfügbar), die Profil-Variante aus 2.0.7 entfällt.

## [2.0.7] – nicht einzeln veröffentlicht, enthalten in 2.0.8

- Lange Seitentitel werden in schmalen Kopfzeilen gekürzt, damit Spielerzahl und Uhrzeit lesbar bleiben.
- Touch auf einen BF6-Spieler öffnet sein GameTools-Profil im Browser; die Rangliste zeigt zusätzlich Spielstunden.
- Touch auf Freunde: online erreichbare Konten bevorzugen, je Gruppe Discord → Steam → Xbox; registrierte Apps prüfen und bei Startfehlern weiterprobieren. Steam öffnet den Chat; Discord/Xbox das Profil mit dem Weg zu „Nachricht“ (vom Nutzer bestätigte Variante). Fensterwahl liegt bei der Plattform-App bzw. dem Browser.
- Auswahl verwendet denselben Datenstand, dieselbe Sortierung und Unterseite wie die Anzeige, auch bei breiten Kacheln. Blog-Ziele verwenden ebenfalls den angezeigten Snapshot.
- Deutsche und englische Bedienung, Fehlerbehebung und eingebettete Anleitungen aktualisiert. Automatische Tests öffnen keine echten Chats. Physische Touch-/Plattform-Abnahme offen; siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [2.0.6] – 2026-10-23

- Touch: Tippen auf einen Blogbeitrag öffnet ihn im Standardbrowser auf dem PC; Tippen anderswo und Wischen blättern wie bisher.
- Behoben: Nach „Speichern & anwenden“ verschwand das Einstellungsfenster in den Infobereich. Es öffnet sich nach dem Neustart jetzt wieder.
- Behoben: Eine zweite Instanz (z. B. eine Testversion) konnte den Port der Einstellungsseite mitbenutzen; der Port wird jetzt exklusiv belegt.
- `LICENSE.txt` und `THIRD-PARTY-NOTICES.md` sind in der EXE enthalten; der Installer legt sie in den Installationsordner.
- Download und Prüfsummen: [v2.0.6](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.6). Prüfstatus siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [2.0.5] – 2026-10-23

- HDMI/USB-C: Solange die Anzeige läuft, wird die Windows-Taskleiste auf diesem Bildschirm ausgeblendet (vorher öffnete ein Tippen unten das Startmenü); beim Beenden kommt sie zurück.
- Jede Touch-Geste steht im Protokoll; Hinweis, wenn Touch in den Einstellungen aus ist. Neue Konfigurationen starten mit Touch an.
- Nur an einen Tester weitergegeben (Touch auf echter Hardware bestätigt), nicht einzeln veröffentlicht; enthalten in 2.0.6.

## [2.0.4] – 2026-10-22

- Übersicht: „Vorherige Ansicht“ und „Nächste Ansicht“ steuern direkt die laufende Anzeige, mit aktueller Position unter der Live-Vorschau. Unterseiten und breite Kachelgruppen werden berücksichtigt; der automatische Wechsel läuft weiter.
- Bedienung auf Deutsch und Englisch, deaktivierte Steuerung bei vollständig sichtbaren oder fehlenden Ansichten; keine Speicherung und kein Neustart nötig.
- Download und Prüfsummen: [v2.0.4](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.4). Build- und Prüfstatus siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [2.0.3] – 2026-10-22

- HDMI/USB-C-Vollbild bleibt über der Taskleiste, ohne den Tastaturfokus zu übernehmen; Ausblenden und Wiederherstellen sind abgesichert.
- Native Windows-Touch-Ereignisse statt Maus-Emulation; abgebrochene Gesten und doppelte Seitenwechsel werden abgefangen. Echte Mausbedienung bleibt erhalten.
- Die Display-Einstellungen prüfen die Windows-Zuordnung der Touchgeräte zum gewählten Monitor. „Touch-Bildschirm zuordnen“ öffnet den Windows-Assistenten und pausiert die Vollbildausgabe während der Einrichtung.
- Release-Build und 86 Kernprüfungen bestanden; 56 Monitor-/Touchprüfungen mit injizierten Windows-Eingaben bestanden. Echte Touch-Hardware und der vollständige Zuordnungsassistent bleiben offen. [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [2.0.0] – 2026-10-22 (Vorabversion)

- C++20-Neuimplementierung unter `cpp/`, Discord über D++ und eigenes Win32/WebView2-Einstellungsfenster.
- Eine EXE enthält App, Installer, Updater und Deinstaller; Übernahme von 1.x-Einstellungen möglich. Python-Distributionsordner entfällt.
- WebView2 Evergreen Runtime bleibt externe Voraussetzung; EXE unsigniert.
- 86 Prüfungen bestanden; natives Fenster sichtbar geprüft, COM7-Gegenvergleich vom Nutzer bestätigt. Der erste echte Neustarttest scheiterte an der Windows-Paketumleitung von Installation/Autostart im Codex-Kontext; nach korrigierter Installation bestätigte der Nutzer den erfolgreichen Windows-Neustart mit Tray-Symbol.
- Download und Prüfsummen: [v2.0.0](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.0). [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [1.2] – 2026-10-21

### Hinzugefügt
- **Englische Oberfläche zum Umschalten** (`ui.language = "de" | "en"`): Einstellungsseite, Display-Seiten, Meldungen und Infobereich. Umschalter unter **Start** und **Display › Programm**; beim ersten Start folgt die Sprache der Windows-Sprache. Übersetzungen in `i18n.py` (App) und `webui.html` (Einstellungsseite), deutsche Texte bleiben die Quelle.
- **Englische Anleitung** `GUIDE.html` (aus `anleitung/anleitung.template.en.html`, Bilder mit Beispieldaten); beide Anleitungen verlinken sich gegenseitig, `/anleitung?lang=en` liefert die englische.
- Kaffee-Knopf (PayPal) in der Einstellungsseite links unten, im README und als Sponsor-Knopf auf GitHub (`.github/FUNDING.yml`).
- Git-Repository mit README, CHANGELOG, Projektprotokoll (`PROTOKOLL.md`) und `.gitignore`.
- Vorlagen für Fehlerberichte und Rückmeldungen in der Testphase (`.github/ISSUE_TEMPLATE/`), Hinweise dazu in `TESTPHASE.md`.
- **Eigene Farben** (`display.theme = "eigen"`): Hintergrund, Karten, Kopfzeile & Balken, Akzentschrift, Schrift und Nebentext per Farbwähler über der Vorlage Hell oder Dunkel (`display.color_*`, leer = Vorlage). Live-Vorschau der aktuellen Seite (`POST /api/preview-theme.png`), „Zurücksetzen“ und Warnung bei zu wenig Kontrast (WCAG). Die Kopfzeilenschrift wählt Navy oder Weiß selbst.
- **Beliebige Auflösungen**: Alle Seiten zeichnen in logischen 480 × 320-Koordinaten und werden scharf auf die echte Größe gerendert (`render.ScaledDraw`). Beim 3,5"-Display bleibt das Bild pixelgleich. Breite Leisten zeigen mehrere Seiten nebeneinander (1920 × 720: zwei, 1920 × 480: drei).
- **Bildschirm über HDMI/USB-C** (`display.output = "monitor"`, `monitor_output.py`): randloses Vollbildfenster auf dem gewählten Bildschirm (`display.monitor`, leer = erster Nicht-Hauptbildschirm), ohne Taskleisteneintrag und ohne Fokus zu stehlen, Mauszeiger ausgeblendet. Die App ist jetzt DPI-bewusst (echte Pixelgrößen).
- **Touch-Seitenwechsel** (`display.touch`, nur bei Bildschirmausgabe und wenn Windows einen Touchscreen meldet): nach links wischen = nächste Seite, nach rechts = vorige, Tippen ins linke Drittel = zurück, sonst weiter.

### Dokumentation
- Repository zweisprachig: README, CHANGELOG, CONTRIBUTING, SECURITY und TESTPHASE jeweils auch auf Englisch (`*.en.md`), Issue-Formulare und PR-Vorlage zweisprachig.
- README nach üblichem Aufbau: Badges, Funktionen, Bilder mit Beispieldaten (`docs/screenshots/`), Schnellstart, Zugänge, Konfiguration, Hardware, Bauen, Projektaufbau, Sicherheit, Lizenzhinweis.
- Neu: `CONTRIBUTING.md` (Ablauf, Code-Regeln, Umgang mit Geheimnissen), `SECURITY.md` (Lücken melden, Sicherheitsmodell) und eine Vorlage für Pull Requests.

## [1.1] – 2026-10-21

### Hinzugefügt
- **Einstellungsseite** im Browser unter `http://127.0.0.1:8765/`, geöffnet über das neue Symbol im Infobereich (Doppelklick) oder einen zweiten Programmstart. Bereiche: Start, Übersicht (Live-Vorschau, Seiten an/aus und Reihenfolge, Anzeigedauer, Helligkeit), Freunde, Discord, Battlefield 6, Blogs, System, Display.
- **Ersteinrichtung** für Freunde ohne Vorkenntnisse: Zustand von Display und Discord, Knöpfe zum Beenden und Abschalten der Hersteller-App, Autostart über `HKCU\…\Run` ohne Administratorrechte.
- **Gemeinsame Freundesliste** (`friends.py`): Discord, Steam (offizielle Web-API) und Xbox (OpenXBL) in einer Liste, dieselbe Person nur einmal (automatisch bei gleichem Namen oder per „Personen zusammenführen“, mit Vorschlägen bei ähnlichen Namen), Marken D/S/X je Plattform. PlayStation und EA bewusst nicht eingebaut (keine offizielle Schnittstelle, Sperrrisiko). Tests in `test_friends.py`.
- **Verbindungsstatus** in den Reitern Discord und Freunde: grün mit Kontoname, Anzahl und letzter Abfrage, rot mit Fehlermeldung, grau wenn nicht eingerichtet.
- **Hilfen zum Aufklappen** neben jedem Token- und Schlüsselfeld (Discord, Steam, Profil-Link, Xbox, EA-ID, Blogs) mit Knöpfen, die direkt die richtige Seite öffnen.
- **Bebilderte, interaktive Anleitung** `ANLEITUNG.html` (kindgerecht, „Schritt X von N“, Auswahl der gewünschten Seiten am Anfang, „Klappt nicht?“-Hilfe je Schritt), erreichbar unter `/anleitung` und aus der Einstellungsseite. Quellen in `anleitung/`.
- Screenshots in der Anleitung mit Beispielnamen erneuert; private Xbox-Namen werden in der Aufnahme anonymisiert.
- **FuchsLabs-CI** (`brand.py`): Fuchs-Orange `#F77F00` und Creme `#FFF4E6` als Hauptfarben, Navy `#14213D` für Schrift, geometrisches Fuchs-Logo für Display, Infobereich und EXE-Symbol. Schriften Bahnschrift (Titel, Zahlen) und Segoe UI (Text), keine Web-Fonts.
- **Hell/Dunkel-Umschalter** (`display.theme`), gilt für Display, Einstellungsseite und Anleitung. Hell ist voreingestellt.
- Willkommensbild auf dem Display, solange keine Seite eingerichtet ist.

### Geändert
- Neuer Name **FuchsLabs USB-Display**, alle Hinweise auf den Display-Hersteller entfernt (Code, Texte, EXE).
- Als eigenständige EXE gebaut (PyInstaller, `build.bat`), Python wird beim Anwender nicht mehr gebraucht. Programmgröße von 66 MB auf rund 36 MB gesenkt.
- Discord wird nur geladen, wenn eine Discord-Seite aktiv ist.
- Die reine Seite „Discord-Freunde“ entfällt automatisch, solange die gemeinsame Liste Discord einbezieht.

### Sicherheit
- Discord-Token, Steam- und Xbox-Schlüssel werden mit Windows-DPAPI verschlüsselt gespeichert (`secure.py`) und nie an den Browser oder ins Log gegeben. Alte Klartext-Tokens werden beim Start verschlüsselt.
- `save_config` legt keine Klartext-Sicherung mehr an (Token stand vorher in `.bak`-Dateien).
- Pillow von 11.3.0 (45 bekannte Lücken) auf 13.0.0 angehoben; fremde Bilder nur als PNG/JPEG/WebP/GIF und höchstens 2 MB.
- Einstellungsseite: nur `127.0.0.1`, Host-Prüfung gegen DNS-Rebinding, eigener Header für Änderungen (CSRF), Ablehnung von `Sec-Fetch-Site: cross-site/same-site`, `X-Frame-Options: DENY`, `frame-ancestors 'none'`, `nosniff`.
- Feeds mit `<!ENTITY` werden verworfen, nur http/https, Antworten auf 3 MB begrenzt. AVIF-Modul aus dem Build ausgeschlossen.
- Details in `SICHERHEITSREVIEW.md`.

### Behoben
- Ungültige JSON-Eingaben an `/api/config` führen zu 400 statt 500.
- Im Reiter „Freunde“ stand „null“.

## [1.0] – 2026-10-21

Erste Fassung als Python-Skript, danach als EXE `FuchsLabs-USB-Display-1.0.zip`.

### Hinzugefügt
- Eigener Treiber für das 3,5"-USB-Display (480 × 320, serielles Protokoll über USB-CDC, RGB565, Teil-Updates in Zeilenbändern), ersetzt die Hersteller-App.
- Seite **Discord-Freunde** über einen eigenen Bot (Presence und Server Members Intent), Status, Aktivität, Avatare.
- Seite **Battlefield 6 · Rangliste** über `api.gametools.network`: Rang mit Abzeichen, K/D, Siegquote, Spieler in der Konfiguration.
- Seite **System**: CPU, GPU (`nvidia-smi`), RAM, Datenträger, Netzwerk über den Adapter der Internetverbindung; CPU-Temperatur und Lüfter aus dem HWiNFO-Shared-Memory. `hwinfo-einrichten.bat` stellt HWiNFO sparsam ein und startet es alle 11 Stunden neu.
- Seite **Blog-Updates** (RSS/Atom, Feed wird zur Blog-Adresse gesucht), neuester Beitrag je Blog.
- Seitenrotation alle 15 Sekunden, Autostart, `DOKUMENTATION.md`.

### Behoben
- Bild stand auf dem Kopf: Ausrichtung `reverse_landscape`.
- Netzwerkwerte falsch, weil die Hersteller-App den Loopback-Adapter maß.
- Ab- und Aufblenden beim Seitenwechsel entfernt: Helligkeitsbefehle zwischen Bilddaten erzeugten Bildfehler.
- Helligkeit von 50 auf 90 angehoben.
- Ganzes Bild wird bei jedem Seitenwechsel und alle 5 Minuten neu gesendet. Ein Streifen an fester Stelle bei einfarbigen Testbildern liegt an Display oder Kabel, nicht an der App.

[Unreleased]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v2.3.0...HEAD
[2.3.0]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v2.2.0...v2.3.0
[1.2]: https://github.com/PapaFuchs84/fuchslabs-usb-display/compare/v1.1...v1.2
[1.1]: https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v1.1
