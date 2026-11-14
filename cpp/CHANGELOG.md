# Changelog

## 2.12.0 – 2026-11-14

Prüfstatus / Test status 2.12.0: 25 C++-Suiten/suites 0 Fehler/failures (3556 Kern/core, 1877 Layout-UI, 4221 Snapshots, 2101 Plugin-Snapshots, 448 Companion, 152 HUD, 2144 Custom, 845 Schedule, 290 Game); Integration: Installer 2, App-Companion 40, Standalone ok.

- HUD-Farben / HUD colours: `system.hud_colors` = `theme` (Standard, aus dem aktiven Design) | `neon` | `custom` mit `hud_color_primary`/`_secondary`/`_background` (#RRGGBB, leer = Design); Kontrastschutz für Text und Akzente; neues `Form::color`-Feld. `system.hud_colors` = `theme` (default, from the active design) | `neon` | `custom` with primary/secondary/background (#RRGGBB, empty = design); contrast guard for text and accents.

- Hardware-HUD / Hardware HUD: Optionale HUD-Ansicht der Systemseite (Einstellung unter „System“) mit Neon-Vektorgrafik, automatisch wechselnden Seiten Übersicht/GPU/CPU, Lüfter-/Ring-/Pulsanimationen und Layouts für breite, normale und Hochkant-Displays; gemessene Textbreiten und Umbrüche verhindern überlappende Beschriftungen. Optional HUD view for the System page (setting under “System”) with neon vector graphics, auto-rotating Overview/GPU/CPU pages, fan/ring/pulse animations and layouts for wide, normal and portrait displays; measured text widths and wrapping prevent overlapping labels.

- CPU-Leistung / CPU power: CPU-Package-Leistung in Watt aus geeigneten HWiNFO-Sensoren (Typ Power, Einheit W); fehlende oder ungültige Werte bleiben unbekannt („--“). CPU package power in watts from suitable HWiNFO sensors (type Power, unit W); missing or invalid values stay unknown (“--”).

- Animationsrate / Animation rate: Globaler Schlüssel `display.animation_fps` (5–60, Standard 30; Einstellungen → Anzeige → Animationen) taktet sichtbare Animationen: HUD, aktive KI-Pulse/Zeilenübergänge, laufende Spotify-Seite und Pegel-/Fortschrittskacheln, animierte eigene Ansichten, Game-Intro, Plugin-Überblendungen. Statische Ansichten behalten ihre langsame Aktualisierung, Sensoren werden nicht häufiger abgefragt. `system.hardware_hud_fps` wird migriert; ein ausdrücklicher globaler Wert hat Vorrang. Global key `display.animation_fps` (5–60, default 30; Settings → Display → Animations) paces visible animations: HUD, active AI pulses/row transitions, the playing Spotify page and level/progress tiles, animated custom views, Game intro, plugin fades. Static views keep their slow refresh and sensors are not polled more often. `system.hardware_hud_fps` is migrated; an explicit global value wins.

- Companion: Am PC getaktete Animationen erhalten keine zweite Transportverzögerung; statische Seiten bleiben bei 5 fps und werden nicht mehr neu gezeichnet, nur weil die Hauptanzeige schnell animiert. Protokoll und Android-App unverändert (Companion 1.1.0). PC-paced animations get no second transport delay; static pages stay at 5 fps and are no longer redrawn merely because the main display animates quickly. Wire protocol and Android app unchanged (Companion 1.1.0).

- USB-Display / USB display (seriell/serial Rev. A, 480 × 320): Eigener Ausgabethread mit Übergabe des neuesten Bildes; eine langsame Übertragung (je nach Bildinhalt, beim HUD etwa 1,7 s) blockiert das Companion-Rendering nicht mehr, serielle Animationen folgen der gemessenen Übertragungszeit statt einer festen Ein-Sekunden-Pause. Dedicated output thread with a latest-frame handoff; a slow transfer (depending on content, about 1.7 s for the HUD) no longer blocks Companion rendering, and serial animations follow the measured transfer time instead of a fixed one-second pause.

- Prüfgrenzen / Test limits: Physisch geprüft sind Redmi 8 (Android 14, USB-Tunnel, Zielraten 5/30/60) und das serielle Rev.-A-Display an COM7 (Routing/Übertragung, keine optische Bildratenmessung). Nicht geprüft: echtes Spotify-Konto/Wiedergabesteuerung, echtes Spielen, WLAN/LAN, Hintergrund/Neustart/Langzeit/Akku/Temperatur. Physically tested: Redmi 8 (Android 14, USB tunnel, targets 5/30/60) and the serial Rev. A display on COM7 (routing/transfer, no optical frame-rate measurement). Not tested: real Spotify account/playback control, real gameplay, Wi-Fi/LAN, background/restart/long-term/battery/thermal. Gemeinsame Abnahme / joint USB + Companion acceptance:
Gemeinsame Abnahme / Joint acceptance (14.11.2026, COM7 + Redmi 8): Companion 30/60 allein/alone 21,4/22,9, mit USB-System/with USB system 19,9/25,5, mit USB-HUD/with USB HUD 20,6/26,2 UI-Neuzeichnungen/s / UI redraws/s; HUD auf seriellem Panel/on serial panel ≈0,6 fps (≈1,7 s Transfer).

Die folgenden Store-Einträge waren bereits im Microsoft-Store-Paket 2.11.0.0 enthalten und erscheinen erstmals als reguläre Version 2.12.0. / The following Store entries were already contained in the Microsoft Store package 2.11.0.0 and appear for the first time as regular version 2.12.0.

- Windows-Testumgebung / Windows test environment: `tests/Run-ReleaseChecks.ps1` startet den unveränderten vollständigen Runner auf einem temporären privaten Desktop ohne sichtbaren Desktopwechsel. Die Geräteeditor-Prüfung pumpt Aufgaben bis zu einem tatsächlichen UI-Leerlauf und prüft „Übernehmen“ auf Aktivierung. No assertions or goldens are removed; real HWND input paths remain exercised independently of other applications.

- Companion-Start / Companion startup: Initialer Navigationszustand ist wie bei der Hauptanzeige ein leeres Objektmodell statt JSON-`null`; die bestehende App-Integration prüft zusätzlich den Objektvertrag während des Starts. Initial navigation uses the same empty object state as the primary display instead of JSON `null`; app integration also checks the startup object contract.

- Store-Ausgabe / Store edition: „Hilfe & Datenschutz“ in „Updates & Autostart“ öffnet die appbezogene Datenschutzerklärung und Supportseite von FuchsLabs im Standardbrowser. “Help & privacy” links the app-specific privacy policy and support page in the default browser.

- Store-Bilder / Store images: `fl-layout-ui-tests --export-store-listing <neuer Ordner>` exportiert vier DE/EN-Beispielbilder mit dem echten nativen Seitenrenderer und protokolliert Version/Paketkontext; kein Desktopzugriff, keine echten Konten/Geräte und keine Änderung an Snapshot-Referenzen. Exports four German/English sample renders and records version/package context without desktop capture, real accounts/devices or golden changes.

- Tests: Game-Mode-Iconprüfung verwendet die App neben der Test-EXE statt eines festen Buildordners. / Game Mode icon tests resolve the app beside the test executable instead of a fixed build directory.

- Microsoft Store: Paketierte Builds starten weder den FPS-Gruppenhelfer noch die erhöhte Hersteller-Autostart-Abschaltung. DE/EN-Hinweise erklären die Einrichtung durch einen Administrator außerhalb der App; vorhandene ETW-Rechte und HWiNFO-FPS bleiben verfügbar. Die Paketprüfung kontrolliert beide gesperrten Einstiegspunkte ohne Änderungen an Windows-Gruppen oder Aufgaben.
- Microsoft Store: Packaged builds do not launch the FPS group helper or elevated vendor-startup changes. German/English guidance explains administrator setup outside the app; existing ETW permissions and HWiNFO FPS remain available. The package probe checks blocked entry points without changing Windows groups or tasks.

## Frühere Einträge / Earlier entries

BF6: Spieler-ID im Eintrag (`Name#ID`, `Name@psn#ID`, ab 6 Ziffern), Abfrage per `playerid`, Auto-Heilung (ID aus Namenssuche im Speicher und in `config.toml`), Rückfall über `/bf6/player`-Suche und Namensvarianten; „Prüfen“ liefert `id`, Web-Link nutzt `/playerid/`. Neue Tests für `parse_entry_full`, `pick_search_id` und Touch-Ziele.

P9-9: Toolhelp-Einträge auch bei verweigertem Prozesshandle/Image-Pfad erhalten; begrenzter EXE-Cache je Bibliothek, eindeutiger Dateinamen-Fallback mit Vordergrund/Vollbild und PID-/Namensbindung. EAAntiCheat.GameService*.exe sowie Installer/Crash-/Hilfsprogramme ausschließen. Simulierte BF6-/Entprellungs-/Filtertests, DE/EN-Dokumentation; echte Javelin-/FPS-/Displayabnahme offen.

P9-8: Reservierte Game-Ansicht aus normalen Seiten-/Tap-Listen filtern; Intro im Preview-Frame nachweisen. EXE-Icons bis 256 px, passende AppxManifest-Assets, gemeinsame CPU-/GPU-/Spielkacheln, neue Vorlage und 60-Sekunden-Messwertverlauf. Synthetische und isolierte Tests; keine reale Launcher-/Displayabnahme.

P9-7: Fünf weitere lesende Spielebibliotheken, austauschbare HKLM-Registry-Abfragen, begrenzter Battle.net-Ordner-Fallback, Launcher-/Anti-Cheat-Filter und Vordergrund-/Vollbildprüfung für Registry- und Epic-EXE-Treffer. Synthetische Tests unter `tests/game_tests/`; DE/EN-Dokumentation und eingebettete Hilfen. Echte Launcher-/Hardwareabnahme bleibt offen; [Prüfstand](../docs/PLAN-GAME-MODE.md#p9-7-weitere-spielebibliotheken).

BF6: HTTP 5xx der API ergibt den Status „Quelle gestört“ (`bf6::is_server_error`, `bf6::failure_text`; EN „source unavailable“) statt „nicht gefunden“, auch in „Prüfen“ (`ui_page_bf6.cpp`, `webui.cpp`); alte Werte und `ids_` bleiben unberührt, Netzwerkfehler/404 unverändert. Touch-Link mit ID: `https://tracker.gg/bf6/profile/<ID>/overview`. Tests: `test_bf6_platforms`, `test_touch_targets`.

P9-6: 26/26 Prüfsuiten und isolierter Dummy-Test bestanden; reproduzierbare Start-/Prüfskripte, lokaler Bildnachweis und ehrlicher Prüfstatus im [Game-Mode-Plan](../docs/PLAN-GAME-MODE.md). Keine EXE direkt gestartet/installiert, kein Test auf 8765, keine hörbaren Töne, kein Push/Release.

P9-5: Runtime/Display-Override, native Einstellungen, Editor-Vorlage, Konfigurationsvalidierung und DE/EN-Hilfen. Historische Editor-Snapshots bleiben erhalten; neue P9-Referenzen. Steam-VDF unterstützt leere Felder. 114 Game-Unit-Prüfungen und isolierte Dummy-Simulation bestanden; Gesamtlauf siehe Plan.

P9-4: Dokumentierte, dynamisch geladene GPU-APIs und eigene ETW-Sitzung; Frame-Statistik und Katalog/Renderer mit Offline-Prüfungen. Kein vollständiger PresentMon-/Scanout-Ersatz.

P9-3: Intro-Renderer und deterministische Zeit-/Fade-Prüfungen; App-Integration folgt in P9-5.

P9-2: Logo-Cache mit Ablauf-, Beschädigungs- und URL-Prüfung; dynamisches EXE-Icon ohne Ausführung.

P9-1: Spielerkennung und Parser als getrennte, testbare Komponenten; keine Prozessspeicherzugriffe.

P8-4 (2026-11-07): KI-Status gemergt; Claude-/Codex-/Limits-Kacheln in eigenen Ansichten, vorhandener Renderer/Render-Takt, öffentliche Bedingungen und Seiten-Touch-Ziele, additive Editor-Snapshots DE/EN. Standardseiten-Baselines unverändert. Portkonflikt 18765 dokumentiert; keine Portänderung oder Veröffentlichung. [P8-4](../docs/PHASE8-EIGENE-ANSICHTEN.md).

P8-2 (2026-11-07): Native Einzelkacheln aller eingebauten Quellen (einschließlich HA-Entität, Listen und Spotify), separater Ansichts-Editor und aktive Display-Tabs mit Standard-Rückfall. Bestehende Renderer und Standardseiten-Verträge erhalten. [Bedienung, Grenzen und Prüfstand](../docs/PHASE8-EIGENE-ANSICHTEN.md).

P8-1 (2026-11-07): Eigene Ansichten: `custom_view.*`, strikter Store mit Backup, Geräte-Vererbung und `custom:`-Seitenzyklus; bestehende Renderer für System/Plugin-v2/Quellstatus. Grenzen (HA, Listen-/Spotify-Inhalte, Editor, Touch) und Prüfstand: [Phase 8](../docs/PHASE8-EIGENE-ANSICHTEN.md).

- KI-Status: vorhandene Konto-Einwilligung nutzt Windows-Login-Dateien für inoffizielle Claude-/Codex-HTTPS-Limits; überschriebene Tokenpuffer, kein Refresh, unabhängiger 3-/15-Minuten-Takt, konfigurierbarer Empfängerport mit Fallback. Synthetische Offline-Prüfung; kein Release.
- KI-Status-Korrekturen / AI Status fixes: feste Seitenzahltests, AI aus Layout-Katalog entfernt, Temp-Dateien bei Fehlern aufgeräumt und verwaiste reguläre Dateien ersetzt (Symlinks abgewiesen), Nummer beim Ausblenden erhalten, nur eigene Hook-Einträge in Vorschau, dynamischer Testport. Prüfstand: `ai-status-report.md`.
- KI-Status / AI Status (`ai`): lokale Claude-Code-/Codex-Karten und Sitzungsanimationen, DPAPI-geschützter Loopback-Empfänger (18765), Hook-Vorschau/Backup/selektive Entfernung, DE/EN-Texte und `ai.*`-Kachelquelle. Fehlende Messwerte `n/a`; optionale lesende Konto-/Limitabfragen siehe `AI-STATUS.md`. Neue fl-tests-Prüfungen, siehe `ai-status-report.md` für ausgeführte Tests und offene Abnahme. Kein Release.

P7b-0 (2026-11-06, Claude Sonnet 5.5 im Auftrag): Neuer Standard dunkles Material mit Orange-Akzent (`material-orange`; Text/Fläche und Akzent/Fläche mindestens 4,5:1, Leiste mindestens 3:1, Kontrasttests in fl-tests und fl-layout-ui-tests). Neuinstallation startet damit; bestehende Installationen fragen einmalig per Dialog DE/EN „Neuen Standard setzen?“ (Voreinstellung Nein, Antwort in `ui.standard_theme_choice` gespeichert, Schließen ändert nichts). Cupertino, Fluent, Hell, Dunkel, Windows und eigene Themes bleiben wählbar. Bestehende Built-in-/Plugin-Hashdateien unverändert, neue Basis additiv in `p7b0-material-orange-hashes.txt` (768 Hashes).

P7b-6 (2026-11-06, Agent Codex CLI 0.166.0): Geräte-Overlays für Built-ins/Plugins mit Standard-Vererbung (höchstens 16 je Seite, 16 KiB je Overlay, 256 KiB gesamt), atomarer Speicherung/Backup und gemeinsamer Geräteauswahl für Bild/Touch; Display-Tabs mit Größe/Ausrichtung, Offline-Profil und Rückfrage beim Verwerfen. Android-TV am PC markieren (Protokoll v1 meldet keinen Gerätetyp), dann nur Querformat. 14 Suiten grün (Companion offline), elf bestehende Hashdateien unverändert; physische Mehrgeräte-Abnahme offen.

P7b-1 (2026-11-06, Agent Claude Sonnet 5.5): Editor-Fensteraufbau: Kopfleiste (Seitenauswahl, Gerätegröße-Chip, Rückgängig/Wiederholen/Übernehmen/Zurücksetzen), linke Schiene mit Kacheln/Ebenen/Seiten, Fläche und Eigenschaften als Drei-Spalten-Hülle (breit ab 900 DIP; mittel: Eigenschaften unter der Fläche; schmal: untereinander, Palette eingeklappt, Bereichswahl bleibt beim Seitenwechsel); Display-Tab-Leiste unverändert. Neu `ui_editor.*`. 14 Suiten grün, Baselines bitgleich; Handabnahme offen.

P7b-2 (2026-11-06, Agent Claude Sonnet 5.5): Kachelpalette mit Suche, Filter-Chips (Alle/System/Zeit/Medien/Spiele/Sonstiges) und Raster lebender Mini-Vorschauen (derselbe Seitenrenderer mit Beispieldaten: Ausschnitt der Standardseite bzw. Einzelzone bei Plugins; Cache je Seite/Kachel/Größe/Anzeige-Theme, Laden nur beim Seitenwechsel, Kacheln ohne Daten als „keine Daten“). Ziehen auf die Fläche, Klick fügt an der ersten freien Stelle ein (ein Rückgängig-Schritt), Tastatur (Pfeile, Leertaste, Enter, Entf, F6), MSAA „Kachelpalette“. Neu `ui_tile_palette.*`. 14 Suiten grün, Baselines bitgleich.

P7b-3 (2026-11-06, Agent Claude Sonnet 5.5): Fläche im Seitenverhältnis des Display-Tabs: Gerätegrößen-Rahmen, dezentes Zellraster, gestrichelte Ablageplätze „hier ablegen“, Geist mit Einrasten (ungültig: gestrichelt rot und Text „passt nicht“), Griffe, Papierkorb, Zoom −/+/Passend. Opt-in `GridEditor::canvas`; das bisherige Raster bleibt bitgleich. Editor-Snapshots additiv (`p7b-editor-hashes.txt`), Kontrast Text ≥ 4,5:1 und Fokusring/Rahmen ≥ 3:1 in allen Themes geprüft. 14 Suiten grün.

P7b-4 (2026-11-06, Agent Claude Sonnet 5.5): Ebenenliste (Reihenfolge, Sichtbarkeit; Auswahl wirkt in Liste und Fläche), Seitenliste (Wechsel mit Rückfrage bei ungesicherten Änderungen) und Einsteigerkarte „In drei Schritten“ (einmal wegklickbar, gemerkt in `layouts/editor-intro.done`). 14 Suiten grün (fl-layout-ui-tests 1.615), Baselines bitgleich; Handabnahme offen.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Reiner Built-in-Katalog und strikt versionierte Overlays; Standard behält festen Renderweg, HA später. 14 Suiten grün (Companion offline: 250); Built-in 4.221 Bilder/Trefferkarten: 0 Fehler, Plugin 2.093 Prüfungen: 0 Fehler; physische Abnahme offen.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomare Seitenlayouts mit Backup/Revision, Beispieldatenvorschau, Raster-Touch-Zielen und isoliertem Clip; fester Renderweg unverändert. 14 Suiten grün (Editor 835; Companion offline 250); Built-in 4.221 Bilder/Trefferkarten und Plugin-Baselines unverändert; Hardware offen.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS-Editor mit Beiträge/Pflicht-Zustand, getrennten Quer-/Hochlayouts in `layouts/builtin.rss.layout.json`; Öffnen/Übernehmen von Standard behält den pixelgleichen festen Weg, echte Änderungen nutzen das Raster (Abstände können sich ändern, E2). Zurücksetzen stellt Standard her; Zeilen, Blättern und Touch teilen einen Plan. Kopf/Fuß fest; Built-in-Wechselfelder und HA später. 14 Suiten grün (Katalog 217, Editor 874, Kern 3.470; Companion offline 250); bestehende Baselines bitgleich, zusätzlich 1.024 RSS-Bilder/Trefferkarten und 5.952 Prüfungen ohne Fehler; Hardware offen.

P7-8 (2026-11-06, Agent Codex CLI 0.166.0): Streaming in der Seitenauswahl; Titel/Pflicht-Zustand aus dem vorhandenen Schnappschuss, seitenbezogenes Quer-/Hoch-Overlay, Blättern und Touch aus einem Rasterplan. Standard/Zurücksetzen behalten den festen Weg; TMDB / JustWatch bleibt im Fuß; kompakte Zeilen ohne Datum. 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, 1.792 neue Rasterbilder/Trefferkarten additiv, 6.267 Prüfungen/0; Hardware offen.

P7-9 (2026-11-06, Agent Codex CLI 0.166.0): BF6 in der Seitenauswahl; Rangliste/Pflicht-Zustand, sortierter Schnappschuss, seitenbezogenes Quer-/Hoch-Overlay. Rasterkapazität steuert Blättern und Spielerlinks; kompakte Zeilen zeigen Name, K/D, Rang oder Fehler. Standard/Zurücksetzen bleiben pixelgleich. 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, 1.024 neue Rasterbilder/Trefferkarten additiv, 4.390 Prüfungen/0; Hardware offen.

P7-10 (2026-11-06, Agent Codex CLI 0.166.0): Freunde und Discord in der Seitenauswahl; gemeinsamer Freunde/Pflicht-Zustand-Katalog und sortierte Schnappschüsse, getrennte Overlays `builtin.friends.layout.json` / `builtin.discord.layout.json`, Backups und Revisionen. Rasterzeilen, Blättern und Profilziele aus einem Plan; schmale kompakte Zeilen ohne Avatar/Marken. Standard/Zurücksetzen bleiben pixelgleich (E2 akzeptiert). 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, je 1.024 Rasterbilder/Trefferkarten additiv, zusammen 8.622 Prüfungen/0; Hardware offen.

P7-11 (2026-11-06, Agent Codex CLI 0.166.0): System im Editor mit CPU/GPU/RAM/Datenträger/Netz aus dem vorhandenen Schnappschuss; Temperaturen mit heutigen Schwellenfarben, Netzverlauf in normaler Dichte, kompakt Wert/Balken bzw. Raten. Eigenes Quer-/Hoch-Overlay, Cupertino-Gruppe je Zone, Werkzeugziele aus demselben Rasterplan. Standard/Reset bleiben pixelgleich; E2 bestätigt. 14 Suiten grün (Companion offline); bestehende Baselines unverändert, 1536 System-Rasterbilder/Trefferkarten additiv. Physische Abnahme offen.

P7-12 (2026-11-06, Agent Codex CLI 0.166.0): Spotify im Editor mit Cover, Titel/Interpret/Album, Fortschritt, Tasten, Pegel und Pflicht-Zustand aus vorhandenem Snapshot/Meter; „zuletzt gespielt“ bleibt ohne Fortschritt/Tasten. Quer-/Hoch-Overlay je Seite, kompakter einzeiliger Text, alle drei Pegelfarbmodi, getrennte 48-Punkte-Tastenziele aus derselben Anordnung. Standard/Reset pixelgleich; 14 Suiten grün (Companion offline); bestehende Baselines unverändert, 4096 Spotify-Rasterbilder/Trefferkarten additiv. Physische Abnahme offen.


### 2.10.0 – preparation (unreleased)

Technical changelog for the development line. Highlights and the open items are in `CHANGELOG.md`; test figures are in `TESTPROTOKOLL.md`.
- **P8-3:** `ui_pages.cpp` no longer registers the layout page; `make_layout_page(selected)` is embedded by `ui_page_custom_views.cpp` (page list entries `builtin:`/`plugin:`). `custom::Tile::tap` (`parse_tap`/`tap_json`, strict, invalid becomes Default), `custom::draw_out` returns hit targets mapped with the composition transform, `custom::tap_override`/`TapEnv` resolve page/link targets against the active cycle and link grants; `App::queue_custom_page` + `ViewState::page_request` apply a page switch in `render_core`. Test seam `App::render_primary_for_test`.

- **Zone layout (manifest v2):** `schema_version: 2`, stable block IDs, optional `layout` (flat grid of integer shares, zones with column/row/span, no overlap, no nesting). Limits: 6×6 grid, 8 zones, 4 blocks per zone, 12 widgets with a layout (6 without). Pure geometry in `plugin_zones.h/.cpp`; drawing by zones in `plugin_render.cpp` (`draw_zoned`). `store_problems`: schema 2 needs `min_app_version` ≥ 2.10.0.
- **Density levels:** compact, regular, detailed per block type (`text`, `metric`, `bar`, `list`, `clock`) via `zones::DensityTable`; thresholds are central in the app, not in the manifest. Blocks that do not fit are hidden instead of overlapping.
- **Conditional display (`show_if`, phase 3):** `Widget::show_if` with `path`, `op`, `value`; presence first, then strict same-type comparison (`condition_holds()`), fail closed. Secret settings are rejected in conditions; an invalid condition rejects the manifest.
- **Fault isolation (phase 3):** per-block try/catch in `render_page` (`Page::failed`) and in `draw_page` (`draw_zoned` and the v1 grid); the clip stack is unwound to the block's own depth and the area is refilled with the theme background. Logging via `LOGW("plugins", …)` with fixed text (IDs and stage only), rate-limited (`plugin_isolation.h`).
- **Device profile and keepout (phase 4a):** `companion.keepout` shared default and per-device keepout (`device_profile.h/.cpp`, `companion.cpp`, `ui_page_companion.cpp`); `display.keepout` on the Display page; whole pixels 0–1000; fallback below 160 × 120 usable pixels. Android TV forced to `SENSOR_LANDSCAPE`.
- **Cycling zones (phase 4b):** schema-2 `cycle` (`rotate`, `first_available`) and `seconds` (3–600); pure scheduling with injected clock (`zones::Cycle`, `layout_edit.cpp`, `app.cpp`); quiet transitions with per-view image cache. Serial rate not measured.
- **Tap targets (phase 4c):** schema-2 `tap` and `links`, percent-encoded non-secret values, HTTPS/host/generation check at tap time, one launch attempt per plugin every two seconds, link consent off by default (`ui_page_plugins.cpp`: "Links aus diesem Plugin am PC öffnen"). Companion rest time 1.0 s, monitor 0.3 s.
- **Built-in pages (P5-0 to P5-6):** sample inputs moved to `render::samples`; System, Friends, Discord, BF6, RSS, Streaming and Spotify migrated mechanically to `draw_framed`. Bit-exact against the baselines on the reference machine (4221 images and hit maps, 0 differences, machine-bound baseline needing Roboto as a user font). Home Assistant stays native.
- **Page editor (P6-1 to P6-5, incl. review):** strictly validated arrangement overlay with predecessor backup and manifest fingerprint binding (`layout_edit.cpp`), app callbacks, native editor under **App → Arrange plugin page** (`ui_page_layout.cpp`): grid, block list, conditions, 100-step history, keyboard paths, MSAA. The manifest is not changed. Global light focus ring darkened to `accent_text`; high-contrast selection only in the editor.
- **Drag-and-drop page editor (P7-1 to P7-4):** `ui_kit`: `Widget::cursor` (pointer per position) and `Widget::cancel` on capture loss and Esc. `layout_edit`: `place_block`, `remove_block`, `move_block`, `probe` (transactional; `probe` never changes the model). `GridEditor`: block palette, drag/resize/trash, keyboard pick-up/drop, MSAA, `pointer()` shared by UI and tests, one undo step per gesture. Overlay and manifest format unchanged. Native mouse/settings smoke tests not run (need a running app).
- **Contrast (≥ 4.5:1):** light theme warning colour (190,120,0) → (146,92,0); Windows high contrast warning colour (240,178,50); Cupertino dark header (10,132,255) → (17,136,255). `g_known_findings` empty; theme matrix in `fl-plugin-snapshot-tests`.
- **Frame style:** plugin cards take radius and spacing from `theme::Design` (FuchsLabs radius 9 → 8).

### Fixed

- **BF6 leaderboard:** `platform=ea` is rejected by gametools; PC accounts are queried with `pc`, falling back to `ea` and the lower-case name; PSN and Xbox try several platform values (`bf6.cpp`, `bf6.h`).
- **Spotify:** idle state shows the last played track via `/me/player/recently-played` (scope `user-read-recently-played`, `parse_recent`, 60 s cache, 403 → old message); `Snapshot::last_played`, dimmed rendering without controls (`spotify.cpp/.h`, `render.cpp`, `i18n.cpp`); tests in `tests/tests.cpp`.

### 2.9.6 – preparation

Mandatory signing and verification step before public EXE export; local development builds remain possible.

MSIX runtime detection, own data path, Microsoft Store update path and a StartupTask instead of version-bound registry autostart. The standalone installation remains available.

## 2.9.5 – 1 November 2026

- Settings navigation regrouped: View, Pages, App (Companion, Plugins, Plugin store moved to App; Colors & design belongs to View); store themes are installed on Colors & design instead of the Plugin store page (`ui_page_design.cpp`, `ui_page_store.cpp`, `ui_pages.cpp`, `ui_kit.h`).

- Recalculate the theme-store list layout after asynchronous loads/installs so rows do not overlap action buttons.
- Refresh bilingual illustrated guides with anonymized release-test captures and profile navigation.

## 2.9.4 – 31 October 2026

- snapshot-bound TMDB touch rows, Spotify desktop cover action, and clearer bilingual TMDB application help.

## 2.9.3 – 31 October 2026

- PlayStation friends in the friends list (unofficial Sony interface via NPSSO, DPAPI-encrypted, off by default); authorization codes with a dot are accepted, diagnostics in the log.

## 2.9.2 – 31 October 2026

- Distinct structural shells: Cupertino collapsible sidebar and optional inspector, Fluent functional search/Back/breadcrumb, Material five destinations/context tabs and pending-change actions. System page compositions and shared touch regions differ per profile.

- Paired profile palettes for native appearance; six sample files; base editor loads reference colors explicitly; preserve explicit FuchsLabs override on starter-theme export.

- Optional whitelisted theme style profiles, backward-compatible starter recognition, editor round-trip; native design tokens and pressed/capture feedback; display typography/cache family key, row geometry and matching hit targets. Development branch; validation described in TESTPROTOKOLL.md.

## 2.9.2 (Nachtrag)

- Tiles::height wraps captions and grows with them; Slider track derived from accent outside FuchsLabs style; SysRenderer::regions uses an 8 px bottom margin for Material/Cupertino; light profile palettes, example themes and light status colors (warn/idle/offline) raised to 4.5:1 text / 3:1 UI; POST /api/config rejects bodies without "config" object.
## 2.9.1 – 31 October 2026

- Amplifier: render_core keeps visible Spotify playback live during silence, preventing the 3 s meter activity lease from expiring before the 4 s simulation delay. A scoped transfer hold covers slow serial writes; release preserves the title seed. Regression included (482 core checks).

- Amplifier: detect default audio device changes via IMMNotificationClient; labelled simulated remote playback after 4 s of silence; selectable bar colors. Fix brush-cache crash in gfx.cpp.

- Plugin store type "theme": separate signed `themes.json` (Kind::themes, own signature domain; `open_themes`, `theme_problem`, `theme_matches`, `build_themes` in plugin_sign), `Catalog::list_themes/install_theme` (cache `themes.cache`, rollback protection, revocation applies), host importer = `theme::Store::import_text`; store page card "Themes"; tool commands `build-themes`, `check-themes`, `sign|verify themes`; seed themes in `store-seed/themes`. Linux: 111 + 77 checks and tool e2e passed; Windows validation: see TESTPROTOKOLL.md.
- Tooltip on nav status dots (`Widget::tip`, `Surface::tip_update`, 500 ms timer, own-drawn). Contrast correction merged into one `push` per color (theme.cpp). Windows validation: see TESTPROTOKOLL.md.
- Key help (`key_help.{h,cpp}`, `add_key_help` in ui_pages): table of official https links and DE/EN steps for Discord, Steam, Xbox, TMDB, Spotify and Home Assistant; cards on the native pages; `is_official_url` allows only listed hosts (https, no userinfo/port). Test target `fl-key-help-tests` (Linux via `tests/build-linux.sh`). Windows validation: see TESTPROTOKOLL.md.

## 2.9.0 – 31 October 2026

- Themes (theme.h/.cpp, ui_page_design.cpp): display.theme = hell | dunkel | windows | custom:<id>; "Match Windows" (AppsUseLightTheme, accent from DWM, high contrast = system colors, live via WM_SETTINGCHANGE, contrast correction text 4.5:1 / bars 3:1); custom themes as strict JSON v1 in the themes folder with editor, preview, duplicate/delete/import/export; old "Eigene Farben" migrate automatically; three example themes in cpp/examples/themes. Settings window gets a "Windows" appearance. Plugin-store type "theme" and device tests are still open.
- Navigation status dots only for data sources/connections (green = set up and on, grey = not set up or off), with legend and accessible hint; settings-only pages show no dot.

- Streaming page (tmdb.cpp, tmdb_logic): one discover query per provider group (`group_selection`, max 8, Prime 9/119 together), `fair_mix` takes the newest title per group round-robin, then sorts by date; fixes Netflix-only lists. Tests: 14 new checks in tmdb_tests (119 total, Linux). Windows build not run.
- Amplifier: touch the activity timer only from the rendered Spotify tile while playing; publish every decay step including the final zero.

- Amplifier (udio_levels.cpp): WASAPI loopback capture thread (LevelMeter), 1024-point FFT to 16 log bands (60 Hz–14 kHz), attack/decay smoothing, runs only while the Spotify page is shown (	ouch()); SpotifyRenderer::render(snap, now, levels) draws bars in landscape while playing; display loop shortens its sleep to 100 ms while active; config key spotify.amp. Tests: 22 new checks in 	est_spotify (sine → band, silence, bounds, rendering). Windows build: all suites passed.

## 2.8.0 – 31 October 2026

- Settings shell: navigation compacts to fit the height (min. item 26 px); device panel lists the main display and all Companion clients from NetworkDisplay::status() (clients); overview shortcuts with icon glyphs and links to /anleitung (de/en) and /plugin-werkstatt via the local server. Companion 1.0.1 (versionCode 5): FrameView draws the frame with CLAMP shader edge to edge and keeps content inside cut-out/rounded-corner insets.
- Spotify page (`spotify.cpp`, `SpotifyRenderer`, `ui_page_spotify.cpp`, `/spotify/callback` in the local server, `touch::Tile::spotify`): PKCE login, DPAPI-protected refresh token file, polling of `/v1/me/player`, cover download (i.scdn.co only), touch buttons via `Command`. Windows build: 321 + 251 + 41 + 127 + 140 + 135 + 11 + 361 checks passed; real Spotify account flow not tested.
- Uninstall guard: `run_uninstall` and `finish_uninstall` refuse to run in a test instance without `FUCHSLABS_INSTALL_DIR` (`test_on_real_install()`), so tests can no longer delete the real install.
- New page `streaming` (New on streaming): `tmdb_logic.{h,cpp}` (platform-neutral: key kind, region, provider selection/abbreviations, request paths, discover/provider parsing, merge, dates) with test target `fl-tmdb-tests` (77 checks, fixed JSON samples, no network; Linux via `tests/build-linux.sh`, Windows via `build.bat`), `tmdb.{h,cpp}` (TMDB v3 client: worker thread with 1 h cache, pacing, back-off, key only in request/Bearer header and never logged; `list_providers` for the settings), `StreamRenderer`/`demo_stream` in render, `ui_page_streaming.cpp` (key field, region, provider multi-select, entries slider), credits card on the Updates & startup page, config section `[streaming]` (`api_key` DPAPI secret, `region`, `providers` as `ID|Name`, `max_items` 1-6, `refresh_minutes` >= 60), `streaming` added to `config::PAGES`, `i18n::to_english`, DE/EN strings, guide step (text only, templates and `cpp/res`). Expected test-count changes in `tests.cpp` (7 default pages). Not built on Windows/MSVC here; `tests.cpp`, the GUI and the display output were not run.
- Streaming: provider logos instead of abbreviations (`tmdb_logic`: `logo_url`, `logo_cache_name`, `logos_shown`, `round_corners`, `match_provider_details`; `tmdb`: logo loader with disk cache). Windows part not yet compiled.

## 2.7.0 – 31 October 2026

- Store installation passes force_disabled through the host importer to Store::import_manifest; the disabled marker is persisted before the manifest, including offline plugins and updates. Direct local imports keep their previous behavior. Regression coverage checks reload and host importer handling.

- Release validation: 1,493 compiled checks plus installer/app/native UI checks passed; Pixel 10 Pro and Redmi 8 tested with Companion 1.0.0. See [release notes](../docs/RELEASE-NOTES-2.7.md) for exact evidence and limits. The following entries preserve earlier development-stage evidence.

- Store preview images: `Kind::previews`, `open_previews`/`preview_matches`/`build_previews` in plugin_sign, `Catalog::preview`/`has_preview` (best-effort previews.json, hash-named verified cache), `NativePluginActions::store_preview`, `ImageView` in ui_page_store, tools `fl-plugin-preview` and `fl-plugin-sign build-previews|check-previews`. Windows build: sign 81, catalog 61 and all other test programs passed; the store page was not clicked through.
- Companion independent mode (`companion.sync=false`): `Server::Hooks` per-device variants (`independent`, `encode_frame_for`, `on_*_for`, `Frame::version`), `NetworkDisplay::show_for/client_ids/device_pages`, `Store` per-device `pages`, `App::render_core/render_devices/device_tap` with `DeviceState`, device page list UI. Windows build: companion tests 361 checks (new `test_independent`), `tests/app_companion_integration.py` 39 and new `tests/app_companion_multi.py` 17 checks passed (simulated clients, isolated ports); not tested with several real phones.

- Pages & order lists enabled plugins (plugin:<id> keys, config::is_page_key, page_items with plugin info, async list load in PagesPage); App::active_pages honours order and checkboxes for plugins and now always applies display.visible_pages. Windows build: 279 + 251 + 41 + 127 + 140 + 135 + 11 + 335 checks passed; the GUI itself was not clicked through.

## 2.6.0 – 30 October 2026

- Fix: `ui_page_store.cpp` now implements `collect()`, so the switch `ui.plugin_store` is saved (verified in an isolated instance: save + restart). The store repository now also serves plugin files from `docs/plugins/` (GitHub Pages).
- Plugin platform phase 3 part 2: `fetch_public_bytes`/`decode_http_body` in `plugin_transport` (transport tests 139 on Linux/g++), `plugin_catalog_net`, store wiring and daily background sync in `app.cpp`, `NativeStoreState` actions, new page `ui_page_store.cpp`, config key `ui.plugin_store`. Windows build status: see test log.
- Plugin platform phase 3 part 1: new `src/plugin_catalog.{h,cpp}` (Catalog: refresh with signature + sequence checks and verified cache, list/filter, install via `Store::import_manifest`, `enforce_revocations`) and target `fl-plugin-catalog-tests` (40 checks passed on Linux/g++). Real HTTPS `Fetcher` (needs raw-byte transport) and the native UI page are not written; Windows build not run here.
- Plugin platform phase 2: new `src/plugin_sign.{h,cpp}` (OpenSSL Ed25519/SHA-256; `verify`, `open_index`, `package_matches`, `open_revoked`, `is_revoked`, signing helpers for tools/tests), `parse_strict_json` exported from `plugins.cpp`, tools `fl-plugin-sign`, test target `fl-plugin-sign-tests` (60 checks passed on Linux/g++, plus end-to-end tool run in `tests/build-validator-linux.sh`). `store_public_key_hex` is empty until the production key exists. Windows/MSVC build not run here. See [signing](../docs/PLUGIN-SIGNING.md).
- Plugin platform phase 1: `Manifest` gains `license`, `author`, `category`, `min_app_version` (optional; invalid values are rejected, schema 1 unchanged); new `store_problems()` and `app_compatible()` in `plugins.cpp`; new tool `tools/plugin_validate.cpp` (CMake target `fl-plugin-validate`, Linux script `tests/build-validator-linux.sh`, workflow `Plugin validator`). `fl-plugin-tests`: 249 checks passed (was 223); 41 example checks passed. Verified on Linux with g++ only; the Windows/MSVC build of the new target was not run here. See [package format](../docs/PLUGIN-PACKAGE-FORMAT.md).

- GitHub repository renamed to `PapaFuchs84/ScreenBuddy` (formerly `fuchslabs-usb-display`). Update check, release links, guides and clone commands use the new address. Installed versions still find updates through GitHub's redirect.

## 2.5.0 – 27 October 2026

Native Direct2D/DirectWrite settings without WebView2, guided setup, display/touch selection and declarative plugins. Signed Companion preview 0.2.1-preview included separately. Windows visual/touch/interaction acceptance, real 2.3.0 → 2.5.0 upgrade and rollback to saved 2.3.0 passed; settings and pairing file preserved. Redmi 8: USB tethering, landscape/portrait, real finger gestures, 5-second timer and reconnect without a new PIN passed. Automated: 1,429 Windows checks and 20 Android protocol tests.

Limits: online plugins remain experimental; CRL/OCSP revocation checking and the complete UI import/consent workflow are not accepted. DPI/screen-reader and complete feature parity are not fully tested. The English guide explicitly identifies its German native screenshots. Windows sign-in, Wi-Fi, launcher/boot and multiple Android devices remain untested.

### Historical development notes (earlier state)

- 27 October 2026: validated the existing 2.5.0 candidate with component, native UI, API, integration and isolated real-2.3.0 upgrade checks; synchronized dist with the tested executable. No production installation or publication. Prepared signed Android 0.2.1-preview separately; hardware acceptance remains pending.

- 27 October 2026: removed the legacy HTML settings endpoint and embedded resource while retaining Companion/API and `/anleitung`. Regenerated the DE/EN illustrated guides from full anonymous native captures. Rebuilt ScreenBuddy and passed the component, isolated UI, installer and 51 navigation/API checks; browser layout acceptance remains open.

- Native Updates & Autostart: a reported newer release can open its validated GitHub release page in the default browser. The action is disabled when the URL is absent or invalid; it never downloads or installs automatically.
- Native settings, phase 2: the self-drawn Direct2D/DirectWrite UI now includes native pages for Discord, Friends, Home Assistant, Companion and Colors & design, in addition to the existing display, page-order, app and plugin pages. The current native settings smoke test passes 27 checks in an isolated `--no-lcd` instance; this checks selected controls and workflows, not complete visual/interaction acceptance or full feature parity.
- Native setup links on Overview and touch assignment on Display: status from Windows, direct wizard launch and refresh. The wizard still requires physical touch acceptance.
- Settings now launch only the native Direct2D/DirectWrite window. The active WebView2 fallback and static loader dependency were removed; native startup failures are logged and shown on request. The loopback API, Companion endpoints and embedded guide remain; the former HTML settings page is no longer embedded or served at `/`. Its source file remains for historical tests. Build and smoke verification passed on 26 October 2026.
- Native monitor chooser on Display lists detected screens and resolutions, supports refresh, and preserves a configured monitor that is currently disconnected.
- mDNS status fix: report success only after successful registration, and log registration and deregistration separately. Full build and tests passed after the fix; the error callback still has no dedicated mock test.
- Plugin examples and tutorial: `examples/plugins/` now includes the offline clock, configurable offline focus board and optional public Open-Meteo weather source. The paired German/English tutorials explain importing, configuring, host consent, sample data and limits. `fl-plugin-example-tests` parsed and rendered all three manifests: 41 checks passed, 0 failed. These checks do not make live HTTPS requests or verify hardware output. See [examples](examples/plugins/README.md), [DE tutorial](../docs/PLUGIN-TUTORIAL.md) and [EN tutorial](../docs/PLUGIN-TUTORIAL.en.md).
- Development line: ScreenBuddy executable and installer name; validated JSON manifests and source fetching with host consent. Earlier dev.2 builds included a WebView2 settings window; current source uses only the native window. One public HTTPS source was fetched in isolation; the full app consent flow has not been accepted.
- Plugin DNS fix: `GetAddrInfoExW` completion now uses a separate wait event while `OVERLAPPED.hEvent` stays null, as required with a completion callback. The previous combination returned `WSAEINVAL` on Windows and blocked live HTTPS. A local numeric-address WinSock regression test, an isolated public Open-Meteo fetch and the later full build with 134 transport checks passed.
- Current 26 October 2026 build: 272 core, 223 plugin parser, 127 store, 134 transport, 135 plugin-settings, 41 plugin-example, 11 monitor, 335 Companion-protocol, 27 native UI, 16 mouse, 16 Companion rebuild, 39 isolated app, 49 navigation and 2 installer checks passed. Real target-system upgrade and full UI/final-build acceptance remain open; the user reports the earlier image fault resolved.
- Security hardening after review: stored plugin secrets are discarded when a manifest update changes the source (host, URL or headers); Windows device names (`con`, `nul`, `com1`–`com9`, `lpt1`–`lpt9`) are rejected as plugin IDs; plugin secrets use their own DPAPI entropy with automatic migration; bidi and zero-width characters are rejected in manifest text; `.` and `..` are rejected as setting values in URL paths; DNS completion is protected against double approval. Open: certificate revocation checks (CRL/OCSP) and full external-source acceptance in the app.
- No 2.5 release or download; 2.3.0 remains the latest published version. See [development status](../docs/SCREENBUDDY-2.5.md).

## 2.3.0 – 2026-10-24

- Independent output state, page filters and timers; shared data sources/theme. Native resolution from authenticated `hello`/`resize`, bounded to 2560 pixels per edge and 4,194,304 pixels, first connected reporting device owns sizing. Each cached JPEG carries dimensions from the same snapshot; `ready` updates precede the corresponding JPEG. Touch coordinates from older dimensions are dropped.
- `/api/companion/pages`, `/api/companion/pages/step`, `/api/companion/preview.png` provide independent local controls. Extra DE/EN settings appear only when Companion is active. Companion disabled preserves the existing main page order/visibility.
- Installer now migrates settings beside a portable EXE when creating a local installation. Existing settings are snapshotted before updates; plaintext credentials are re-serialized with DPAPI encryption in the backup.
- Supports `swipe.dir` strings left/right and the tested numeric quick fix. Swipe/touch share the 10-per-second limit; resize is authenticated and limited to 4 per second.
- Fixed: Tray notification icon initialization uses non-blocking `WM_TIMER` retries instead of synchronous `Sleep`, preventing up to 30 seconds shutdown delays on `POST /api/quit` in headless/test environments.
- Fixed: Uninstaller with `--purge` now removes `config.toml.install-backup`, `companion.json` (with temporary/backup files), and `update-check.json`.
- Fixed: Installer copies existing `companion.json` when migrating settings alongside a portable EXE.
- Fixed: Guide templates (`anleitung.template.*`) and documentation synchronized for 2.3.0 release consistency.
- Performance & Cleanup: Optimized multi-tile frame pasting using row-level `std::memcpy` in `paste()`, added stack-buffer fast path to `strprintf()` eliminating dynamic allocations for common messages, replaced regex and substring conversions in `parse_color()` with fast zero-allocation hex parsing, added inline flat cache for Direct2D brushes in `Canvas::Impl`, avoided string formatting overhead when debug logging is disabled via relaxed atomic `is_verbose()`, enabled compiler string pooling (`/GF`) and linker security hardening (`/DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA`), and resolved all compiler conversion and deprecation warnings in `render.cpp` and `sysinfo.cpp`.
- Current Windows and Android checks: see TESTPROTOKOLL.md. Historical pre-Windows prototype statements below describe the earlier implementation.

### Companion foundation

- Companion output (`NetworkDisplay`, `output.h`/`network_display.cpp`): `display.output = "network"` (480 × 320) or `[companion] enabled = true` as a mirror next to the serial/monitor output. Platform-neutral core in `companion.cpp` (RFC 6455 handshake/framing, JSON messages, `Pairing` with 6-digit PIN, 120 s validity, single use, lock for 300 s after 5 failures, `FrameGate` ≤ 5 fps, `Store` for `companion.json`, pairing URI and QR SVG) and `companion_server.cpp` (OpenSSL TLS ≥ 1.2, ECDSA P-256 self-signed certificate generated once, listeners only on private IPv4 addresses, peer must be private too, max. 4 connections, `Origin` header rejected, ping every 5 s, 15 s silence closes). Windows part: LAN addresses via `GetAdaptersAddresses`, mDNS via `DnsServiceRegister` (loaded at runtime), JPEG via WIC (`encode_jpeg`), keys via DPAPI, taps on an STA thread into `App::tap_at` / page step. Settings: tab **Companion** (DE/EN) with QR code, PIN countdown, fingerprint, connected and paired devices; endpoints `/api/companion/status|pair|pair/cancel|devices/remove`. Config `[companion] enabled=false, port=8777` (rejected: 8765 or the settings port while in use).
- Companion hardening (review findings): bounded input before and after login (read budget per loop, at most 64 KiB unparsed per connection, no busy loop on a partial frame); one deadline of 10 s from `accept()` for TLS handshake, HTTP upgrade and `hello` (non-blocking, `stop()` no longer waits for slow clients); no exception leaves a worker thread (storage errors during pairing answer `storage`, non-`std` exceptions from hooks are caught); a device removed while it is logging in is rejected (device id is attached before the store is re-checked, so `drop_device` or the check always catches it); after a server (re)start the current image is sent as initial frame even if it did not change; `poll()` instead of `select()` on POSIX (descriptors ≥ `FD_SETSIZE`), listener count capped at `FD_SETSIZE` on Windows; WebSocket lengths must be minimally encoded, close payloads need a valid code and UTF-8 reason. mDNS (Windows): callbacks may arrive after the 2 s waits, so the registration context is reference-counted per pending request and `dnsapi.dll` stays loaded; `mdns_` is atomic.
- New dependency: Nayuki QR Code generator 1.8.0 (MIT), vendored in `third_party/qrcodegen`. OpenSSL is now listed directly in `vcpkg.json` (was already linked through D++).
- Tests: the earlier platform-neutral `fl-companion-tests` and Linux ASan/UBSan/TSan run covered the TLS protocol foundation. The later Windows integration, TLS tests and Redmi preview checks are recorded under 2.3.0 in [TESTPROTOKOLL.md](TESTPROTOKOLL.md).

## 2.2.0 – 2026-10-24

- System page touch: `SysRenderer::zone_at` maps taps to the RAM, drive and network cards (same geometry as `render()`); `touch::open_system` opens Task Manager, `control.exe appwiz.cpl` or https://www.speedtest.net/de. Only the system page with touch active reacts.
- New `update.cpp`/`update.h`: `UpdateChecker` queries `releases/latest` of the public repository at most once a day (first check 45 s after start, retry after 6 h on failure, manual check at most every 5 min), keeps the state in `update-check.json`, and reports only non-draft, non-prerelease versions with a github.com project URL. Tray balloon (`Tray::notify`, click opens settings), `/api/status` field `update`, `POST /api/update/check`, setting `[ui] update_check` (default true). Skipped in test instances and preview runs; nothing is downloaded or installed.
- Tests: hit areas of the system page, version comparison, parsing of the GitHub response.

- Documentation follow-up for 2.0.3: current download links, matching German/English HDMI/touch steps in the guide templates and HTML resources, validation status, and permanent documentation rules. Updated embedded guides will be included in the next app build; the existing 2.0.3 EXE is unchanged.

## 2.1.0 – 2026-10-23

Combined state of the two latest changes; 2.0.9 is not released on its own.

- New Home Assistant page: up to 4 entities. Sensors show value and unit, switches/lights/fans/input booleans show On/Off and can be flipped by tap when touch is active (only sensors can be chosen without touch). REST API with a DPAPI-protected long-lived token, 15 s polling, entity picker in the settings. Locks, covers and alarms are not switchable.

- Tapping a friend now follows a fixed order: Steam chat, otherwise the Discord profile (`discord://-/users/<id>`, choose Message there), otherwise the Xbox profile on xbox.com in the browser. Missing apps fall through to the next target; online status is no longer considered.

## 2.0.8 – 2026-10-23

- Tapping a BF6 player now opens the GameTools profile with the website's platform slug (`pc`, `xboxone`, `ps4`) instead of `ea`, which produced a blank page. Players GameTools does not know for PC still show "not found" on the website although the API returns them.
- Tapping a friend now only opens the Steam chat; Discord and Xbox open nothing for now (no chat link available), the 2.0.7 profile variant is removed.

## 2.0.7 – not published on its own, included in 2.0.8

- Long page titles are shortened in narrow headers so the player count and clock remain readable.
- Tapping a BF6 player opens their GameTools profile in the browser; the leaderboard also shows hours played.
- Tapping friends prefers online accounts, using Discord → Steam → Xbox order within each group; checks registered apps and falls through on launch failures. Steam opens the chat; Discord/Xbox open the profile with access to Message (user-approved variant). Window/tab choice belongs to the platform app/browser.
- Selection uses the displayed snapshot, sorting and subpage, including wide tiles. Blog targets also use the rendered snapshot.
- German/English usage, troubleshooting and embedded guides updated. Automated checks open no real chats. Physical touch/platform acceptance remains open; see [test record](TESTPROTOKOLL.md).

## 2.0.6 — 2026-10-23

- Tapping a blog post on a touch display opens the article in the default browser (http/https links only).
- Fixed: after saving, the settings window no longer disappears into the tray; the restarted app reopens it (`--settings`).
- Settings server binds its port with SO_EXCLUSIVEADDRUSE (httplib set SO_REUSEADDR, so a second instance could bind the same port).
- LICENSE and THIRD-PARTY-NOTICES.md are embedded as resources and written to the installation folder; removed on uninstall.
- Published as v2.0.6.

## 2.0.5 — 2026-10-23

- HDMI/USB-C output hides the Windows taskbar on the display's screen while the app shows there (restored when the output is hidden or the app quits). Before, a touch near the bottom edge opened the Start menu instead of reaching the app.
- Touch on: new configurations start with touch enabled; a touch while it is switched off writes a hint to the log.
- Every touch gesture is logged (distance and result), so problems on a touch display can be diagnosed from the log.
- Handed to a tester only (touch confirmed on real hardware); included in 2.0.6.

## 2.0.4 — 2026-10-22

- Overview adds Previous view / Next view, live page position, and disabled controls when no further view is available. Uses the existing touch navigation, including list subpages and wide tile groups. Automatic cycling resumes with a full page duration.
- Added loopback-only GET /api/pages and validated POST /api/pages/step (delta -1 or 1), protected by the existing request checks. No config write or restart is needed.
- German/English documentation and embedded guides updated. Installed locally with matching version/hash, unchanged settings, preserved autostart and display_ok=true. Published as v2.0.4 on 2026-10-23.

## 2.0.3 — 2026-10-22

- HDMI/USB-C output now stays above the taskbar without taking keyboard focus, including after hiding/restoring the display and Explorer/taskbar changes.
- Touch uses native Windows pointer events instead of mouse emulation, with one gesture per contact and cancellation handling.
- Display settings show the Windows touch-to-monitor assignment and warn about another or unknown target monitor.
- Added a Windows touch setup button. The fullscreen output pauses while the assignment wizard is open and resumes afterwards. Physical assignment requires touching the intended screen once in the wizard.

## 2.0.2 — 2026-10-22

- Illustrated guide (DE/EN) updated for 2.0: new texts and new screenshots of the current settings page (previously still from 1.1).
- The guide opens in the default browser, no longer inside the settings window; new tray menu entry *Open guide*.

## 2.0.1 — 2026-10-22

- Fixed: Discord stayed on "Connecting …" forever if there was no internet when the app started; it now retries after 90 seconds.
- Fixed: services answering with `Content-Encoding: deflate` (e.g. Open-Meteo) failed to load; only gzip is requested now.

## 2.0.0 — 2026-10-22 (prerelease)

- Reimplemented the Windows display application in C++20.
- Added a single-EXE installer, updater, and uninstaller with Version 1.x configuration migration.
- Ported the system, friends, Battlefield 6, RSS, renderer, touch/monitor, tray, and local settings features.
- Integrated D++ for Discord and Windows DPAPI for saved tokens.
- Added a native Windows settings window using the Microsoft WebView2 SDK and static loader.
- Added a local test suite and static dependency build.

Hardware comparison on COM7 and the native settings window were validated on 2026-10-22. The first reboot test failed due to packaged-host installation redirection; the user confirmed the corrected normal-user installation starts automatically with its tray icon after a real Windows reboot. The EXE remains unsigned. See [test record](TESTPROTOKOLL.md).
