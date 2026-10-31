# Changelog – ScreenBuddy

**Deutsch** · [English](CHANGELOG.en.md)

Alle nennenswerten Änderungen an der App. Format angelehnt an [Keep a Changelog](https://keepachangelog.com/de/1.1.0/), Versionen nach [SemVer](https://semver.org/lang/de/).

## [Unreleased]

### Behoben

- Streaming-Neustarts: Die Seite zeigte fast nur Netflix, weil alle gewählten Anbieter in einer Abfrage nach Datum sortiert wurden. Jetzt fragt die App je Anbieter getrennt ab (bis zu 8 Gruppen) und wählt reihum je Anbieter den neuesten Titel; so erscheinen alle gewählten Anbieter, jeweils mit Logo.
- Spotify-Amplifier: Aktivierung erst beim Zeichnen der sichtbaren Wiedergabeseite; nach dem Seitenwechsel endet die Analyse nach höchstens drei Sekunden. Restbalken klingen vollständig auf null ab.

- **Companion / Android TV:** Auch die sichtbare Einrichtung hält den Bildschirm eingeschaltet und verhindert den Bildschirmschoner. Die Vollbildanzeige behält ihren bestehenden Schutz; nach dem Verlassen der App gelten die normalen TV-Einstellungen.

### Hinzugefügt

- **Companion / Android TV:** eigener Eintrag im TV-Launcher (Leanback, mit Banner); Fernbedienung: Pfeil links/rechts wechselt die Seite (ein Druck, ein Wechsel), OK ohne Aktion, Zurück unverändert; kurzer Hinweis beim Verbinden. Ohne Kamera wird der QR-Scan nicht angeboten und keine Kameraberechtigung erfragt, manuelle Kopplung bleibt. Geprüft am BeyondTV (Android 11) per ADB-Tasteneingaben; physische Fernbedienung, Standby/CEC und Langzeit sind nicht geprüft.
- **Spotify: Amplifier-Anzeige**: bei laufender Wiedergabe zeigen 16 Balken den Pegel der Windows-Systemausgabe (Spektrum, ca. 8 Bilder pro Sekunde, nur im Querformat). Die Systemausgabe wird lokal per WASAPI-Loopback gelesen, nichts wird aufgezeichnet oder gespeichert, die Analyse läuft nur, solange die Spotify-Seite sichtbar ist. Schalter: Einstellungen → Spotify → *Amplifier-Anzeige* (spotify.amp, Standard an). Gilt für Display, Monitor und Companion; die tatsächliche Bildrate hängt vom Ausgabegerät ab.

## [2.8.0] – 2026-10-31

### Hinzugefügt

- **Einstellungen**: Der Schnellzugriff auf der Übersicht hat Symbole und neue Knöpfe für die Anleitung (Deutsch/Englisch) und die Plugin-Werkstatt. Die rechte Seitenleiste „Gerät“ zeigt jedes verbundene Gerät (USB-Display bzw. Monitor, jede Companion-App mit Name, IP-Adresse und Seitenposition) als eigene anwählbare Karte; die Live-Vorschau zeigt das gewählte Gerät.
- **Spotify-Seite**: aktueller Titel mit Cover, Fortschritt und – mit Touch – Tasten für Zurück, Wiedergabe/Pause und Weiter (Spotify Web API). Anmeldung per PKCE ohne Client Secret mit eigener Spotify-Developer-App; Tokens DPAPI-verschlüsselt in `spotify.token`. Einstellungen → Spotify, Doku: [docs/SPOTIFY.md](docs/SPOTIFY.md). Automatisiert geprüft (PKCE-Testvektor, Antwort-Auswertung, Tasten, Rendering); der echte Spotify-Login wurde noch nicht mit einem Konto getestet.
- **Streaming-Neustarts**: neue Display-Seite mit Filmen und Serien der letzten 30 Tage bei den gewählten Streaming-Anbietern (Titel, Datum, Film/Serie, Anbieter-Kürzel). Quelle: TMDB API v3 (nur GET über HTTPS, Cache 1 h), Anbieterdaten von JustWatch. Einstellungen: Seite an/aus, Titel, TMDB-Key (maskiert, DPAPI), Region (Automatik aus der Sprache, überschreibbar), Anbieter-Mehrfachauswahl aus der TMDB-Liste (vorbelegt: Netflix, Prime Video, Disney+, Apple TV+, WOW, RTL+) und Anzahl Einträge. Ohne Key zeigt die Seite „Kein Key“, bei Fehlern oder offline „Keine Daten“. Pflicht-Nachweis „This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.“ in den Einstellungen, der Anleitung und `THIRD-PARTY-NOTICES.md`; Fußzeile „TMDB / JustWatch“ auf der Seite. Neuer Konfigurationsabschnitt `[streaming]`, neuer Seitenschlüssel `streaming`. Das offizielle TMDB-Logo ist noch nicht eingebunden. Automatisiert nur die Logik unter Linux geprüft; Windows-Oberfläche und Anzeige noch nicht gebaut oder am Gerät geprüft.
- **Streaming-Neustarts: Anbieter-Logos** statt der Kürzel. Die Logos stammen von TMDB/JustWatch (`logo_path` der Anbieter, Bild von `image.tmdb.org`, w92), werden nur per GET über HTTPS ohne Weiterleitung geladen (Größenlimit, nur PNG/JPEG), einmal im Datenordner unter `cache/tmdb-logos/<Anbieter-ID>.png` abgelegt und quadratisch mit abgerundeten Ecken auf Zeilenhöhe skaliert. Je Titel höchstens 3 Logos, weitere als „+n“. Fehlt ein Logo, steht das Kürzel. Geladen wird im Hintergrund-Thread. Die Logos sind Marken der jeweiligen Anbieter. Nur die Logik unter Linux getestet; Windows-Teil und Darstellung am Gerät noch nicht geprüft.

### Behoben

- Einstellungsfenster: Die linke Navigation passt sich der Fensterhöhe an und läuft nicht mehr unter die Fußleiste (kein Überlappen mit „Keine ungespeicherten Änderungen“).
- **Companion-App 1.0.1**: Der Hintergrund läuft randlos bis an die Bildschirmkanten, der Inhalt bleibt im sicheren Bereich vor abgerundeten Display-Ecken (Android 12+, Radius aus der System-Auskunft) und Kamera-Ausschnitt; gemeldete Auflösung und Touch-Koordinaten beziehen sich auf den sicheren Bereich. Geprüft am Pixel 10 Pro (Ecken 157 px, Ausschnitt oben); Redmi 8 ohne Rundungen unverändert.
- TMDB: Voreinstellung für RTL+ nutzt die TMDB-ID 2750 (Region DE) statt 298.
- `--uninstall` bricht in Testinstanzen (`FUCHSLABS_INSTANCE`) ohne eigenen `FUCHSLABS_INSTALL_DIR` ab, statt die echte Installation (EXE) zu löschen und Autostart-Eintrag samt Programmliste zu hinterlassen.

## [2.7.0] – 2026-10-31

### Hinzugefügt

- **Plugin-Werkstatt**: kindgerechte, interaktive Anleitung (Deutsch/Englisch) zum Bauen eigener Plugins mit Baukasten, Live-Vorschau, Fehler-Detektiv und Datei-Export. In der App unter `/plugin-werkstatt` (Link in der bebilderten Anleitung), als Datei `cpp/res/PLUGIN-WERKSTATT.html`.
- Plugin-Store: Vorschaubilder. Signierte `previews.json` (getrennt vom Index, ältere Apps ignorieren sie), Anzeige im Store-Detailbereich, Werkzeuge `fl-plugin-sign build-previews|check-previews` und `fl-plugin-preview` (rendert Beispielbilder). Bilder für die 8 Start-Plugins unter `store-seed/previews/`.
- Companion: mehrere Handys gleichzeitig, unabhängig steuerbar (`[companion] sync = false`) oder synchron (Standard). Im unabhängigen Betrieb hat jedes Gerät eigene Seitenposition, Auflösung, Wisch-/Tippbedienung und optional eine eigene Seitenliste (Einstellungen → Companion, gespeichert in `companion.json`); Schalter „Geräte synchron anzeigen“. Protokoll unverändert. Automatisiert mit simulierten Geräten sowie auf Pixel 10 Pro und Redmi 8 mit eigenen Listen und injizierten Gesten geprüft; Grenzen siehe Release-Notizen 2.7.0.
- Seiten & Reihenfolge: aktivierte Plugins stehen jetzt in der Liste, lassen sich mit ▲/▼ sortieren und per Haken ein-/ausblenden (Konfigschlüssel `plugin:<id>` in `display.pages` / `display.visible_pages`). Neu aktivierte Plugins erscheinen zunächst hinten.

### Behoben

- Die Haken in Seiten & Reihenfolge wirken jetzt auch ohne laufende Companion-Ausgabe (vorher wurde `display.visible_pages` nur mit Companion ausgewertet).

- Store-Installationen und Updates bleiben jetzt auch bei Plugins ohne Netzwerkquellen ausgeschaltet, bis sie bewusst aktiviert werden. Im Echtsystem mit „Drei Aufgaben“ reproduziert; Regressionstest für Neuinstallation, erneuten Import und Neustart ergänzt.

## [2.6.0] – 2026-10-30

### Hinzugefügt

- Plugin-Plattform, Schlüssel: Produktionsschlüssel erzeugt, Public Key in `plugin_sign.h` eingetragen; Werkzeug `fl-plugin-sign check-index` für die Store-CI.
- Plugin-Plattform, Phase 4 (Texte und Start-Plugins): [Nutzungsbedingungen, Regeln und Datenschutz](docs/PLUGIN-STORE-BEDINGUNGEN.md) (Entwurf, juristisch nicht geprüft), [Autorenleitfaden](docs/PLUGIN-AUTOREN.md), Issue-Vorlage „Plugin melden“, `SECURITY.md` um Plugin-Meldungen ergänzt und 8 Start-Plugins unter `store-seed/plugins/` (Validator und Beispieltests bestehen). Das Store-Repository `PapaFuchs84/ScreenBuddy-Plugins` ist live (Index mit 8 Start-Plugins); der Signaturschlüssel ist erzeugt und in der App eingetragen.

- Plugin-Plattform, Phase 3 (Teil 2): Rohbyte-HTTPS-Abruf, native Seite „Plugin-Store“ (Suche, Kategorien, Hosts vor Installation, Installieren/Aktualisieren, Plugins bleiben aus), Tagesabgleich im Hintergrund, Schalter „Plugin-Store verwenden“. Ohne Signaturschlüssel meldet die Seite „nicht verfügbar“. Siehe [Plugin-Signierung](docs/PLUGIN-SIGNIERUNG.md#store-in-der-app-phase-3-teil-2).
- Plugin-Plattform, Phase 3 (Teil 1, Store-Logik): `plugin_catalog` lädt Index und Sperrliste über einen eingespeisten HTTPS-Abruf, prüft Signatur und Rollback-Schutz, hält einen erneut verifizierten Offline-Cache, listet mit Suche/Kategorie/Status (installiert, Update, nicht unterstützt, gesperrt), zeigt neue Hosts bei Updates, installiert nur signierte und passende Pakete (Plugin bleibt aus, Hosts ohne Zustimmung) und schaltet gesperrte Plugins ab. Ohne eingetragenen Produktionsschlüssel meldet der Store „nicht verfügbar“. **Noch offen:** native Store-Seite in der App und der echte HTTPS-Abruf (Windows). Doku: [Plugin-Signierung](docs/PLUGIN-SIGNIERUNG.md#store-logik-phase-3-teil-1).
- Plugin-Plattform, Phase 2: Ed25519-Signierung für Index und Sperrliste (`plugin_sign`), SHA-256-Paketprüfung inklusive Host-Abgleich, Rollback-Schutz und Werkzeug `fl-plugin-sign`. Der Produktionsschlüssel fehlt noch (Basti erzeugt ihn offline), die App lädt noch nichts herunter. Doku: [Plugin-Signierung](docs/PLUGIN-SIGNIERUNG.md).
- Plugin-Plattform, Phase 1: Manifest-Felder `license` (`MIT`/`CC0-1.0`), `author`, `category` und `min_app_version` (optional, Schema bleibt 1), Prüffunktionen `store_problems` und `app_compatible` sowie der Offline-Validator `fl-plugin-validate` (`--store`, `--app-version`) mit GitHub-Actions-Workflow. Beispiel-Plugins tragen die neuen Felder. Doku: [Paketformat](docs/PLUGIN-PAKETFORMAT.md). Es gibt noch keinen Pluginstore, keine Signierung und keine Store-Oberfläche.

### Behoben

- Plugin-Store: Der Schalter „Plugin-Store verwenden“ wurde beim Speichern nicht übernommen (die Store-Seite hatte keine `collect()`-Methode).
- Plugin-Store-Repository: Plugin-Dateien liegen jetzt zusätzlich unter `docs/plugins/`, weil GitHub Pages nur `docs/` ausliefert; vorher scheiterte die Installation mit „Plugin download failed“.
- Hinweis: Die Phasen-Einträge oben beschreiben den Entwicklungsverlauf; Formulierungen wie „noch kein Store“ oder „Schlüssel fehlt“ gelten in 2.6.0 nicht mehr.

### Geändert

- GitHub-Repository in `PapaFuchs84/ScreenBuddy` umbenannt (vorher `fuchslabs-usb-display`). Update-Prüfung, Release-Links, Anleitungen und Klon-Befehle zeigen auf die neue Adresse. Bereits installierte Versionen finden Updates weiterhin über die Weiterleitung von GitHub.

## [2.5.0] – 2026-10-27

### Hinzugefügt

- Native Direct2D/DirectWrite-Einstellungen ersetzen WebView2; geführte Einrichtung und Display-/Touch-Auswahl.
- Lokale deklarative Plugins und optionale experimentelle HTTPS-Quellen mit Zustimmung je Host.
- Signierte Companion-Vorschau 0.2.1-preview mit dokumentiertem Wechsel von Debug- zu Release-Zertifikat.

### Geprüft

- Windows-Sicht-/Touch-/Native-Bedienung und echtes 2.3.0-Upgrade/Rollback bestanden; Einstellungen erhalten.
- Redmi 8 über USB-Tethering: Quer-/Hochformat, echte Gesten, Timer und gespeicherte Wiederverbindung bestanden.
- 1.429 Windows-Prüfungen, 20 Android-Protokolltests und alle 46 DE/EN-Anleitungsansichten geprüft.

Native Direct2D/DirectWrite-Einstellungen ohne WebView2, geführte Einrichtung, Display-/Touch-Auswahl und deklarative Plugins. Signierte Companion-Vorschau 0.2.1-preview separat enthalten. Windows-Sicht-/Touch-/Bedienprüfung, reales Upgrade 2.3.0 → 2.5.0 und Rückfall auf die gesicherte 2.3.0 bestanden; Konfiguration und Kopplungsdatei erhalten. Redmi 8: USB-Tethering, Quer-/Hochformat, Fingerbedienung, 5-Sekunden-Wechsel und Wiederverbindung ohne neue PIN bestanden. Automatisiert: 1.429 Windows-Prüfungen und 20 Android-Protokolltests.

Prüfgrenzen: Online-Plugins bleiben experimentell; CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen. DPI-/Screenreader- und vollständige Funktionsparität sind nicht vollständig geprüft. EN-Anleitung mit ausdrücklich gekennzeichneten deutschen nativen Beispielbildern. Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus und mehrere Android-Geräte bleiben offen.

Die Windows-EXE ist unsigniert. Android verwendet ein neues Release-Zertifikat; eine alte Debug-APK kann nicht direkt aktualisiert werden. [Release-Notizen](docs/RELEASE-NOTES-2.5.md).

Historische Entwicklungsnotizen und frühere Prüfstände: [Archiv](docs/DEVELOPMENT-NOTES-2.5.md).

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
- Download und Prüfsummen: [v2.0.6](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.6). Prüfstatus siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

## [2.0.5] – 2026-10-23

- HDMI/USB-C: Solange die Anzeige läuft, wird die Windows-Taskleiste auf diesem Bildschirm ausgeblendet (vorher öffnete ein Tippen unten das Startmenü); beim Beenden kommt sie zurück.
- Jede Touch-Geste steht im Protokoll; Hinweis, wenn Touch in den Einstellungen aus ist. Neue Konfigurationen starten mit Touch an.
- Nur an einen Tester weitergegeben (Touch auf echter Hardware bestätigt), nicht einzeln veröffentlicht; enthalten in 2.0.6.

## [2.0.4] – 2026-10-22

- Übersicht: „Vorherige Ansicht“ und „Nächste Ansicht“ steuern direkt die laufende Anzeige, mit aktueller Position unter der Live-Vorschau. Unterseiten und breite Kachelgruppen werden berücksichtigt; der automatische Wechsel läuft weiter.
- Bedienung auf Deutsch und Englisch, deaktivierte Steuerung bei vollständig sichtbaren oder fehlenden Ansichten; keine Speicherung und kein Neustart nötig.
- Download und Prüfsummen: [v2.0.4](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.4). Build- und Prüfstatus siehe [Testprotokoll](cpp/TESTPROTOKOLL.md).

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
- Download und Prüfsummen: [v2.0.0](https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v2.0.0). [Testprotokoll](cpp/TESTPROTOKOLL.md).

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

[Unreleased]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.6.0...HEAD
[2.6.0]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.5.0...v2.6.0
[2.3.0]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v2.2.0...v2.3.0
[1.2]: https://github.com/PapaFuchs84/ScreenBuddy/compare/v1.1...v1.2
[1.1]: https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/v1.1
