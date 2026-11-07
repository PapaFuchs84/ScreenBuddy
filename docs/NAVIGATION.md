# Menü und Themes ab ScreenBuddy 2.9.5

Die Seiten heißen in allen Themes gleich. Das Design bestimmt, wie du sie erreichst.

| Gruppe | Einstellungen |
| --- | --- |
| Ansicht | Übersicht, Anzeige, Seiten & Reihenfolge, Farben & Design |
| Seiten | System, Freunde, Discord, Battlefield 6, Blogs (RSS), Streaming-Neustarts, Home Assistant, Spotify |
| App | Companion, Plugins, Plugin-Store, Seite anordnen und Eigene Ansichten (ab 2.10.0, in Vorbereitung), Updates & Autostart |

## Je nach Theme navigieren

- **FuchsLabs:** vollständige Seitenliste links; Live-Vorschau und Geräte rechts.
- **Cupertino:** Seitenliste mit **☰** ein-/ausblenden; **Vorschau** öffnet den Geräte-Inspector. Eine ausgeblendete Vorschau ist kein Verbindungsfehler.
- **Fluent:** Seitentitel im Suchfeld filtern, Treffer öffnen; **‹** geht zur zuvor geöffneten Seite zurück. Suchtext löschen, um wieder alle Seiten zu sehen.
- **Material:** **Ansicht → Farben & Design**. Die fünf Hauptziele sind Übersicht, Ansicht, Seiten, Erweiterungen und App. Plugins und Plugin-Store stehen hier im eigenen Ziel **Erweiterungen**, Companion und Updates unter **App**. Unterseiten unter der Überschrift auswählen; bei wenig Platz erscheint dafür ein Auswahlmenü. **◧** schaltet den Inspector. Die schwebende Speichergruppe erscheint erst nach Änderungen.

## Theme installieren und übernehmen

1. **Ansicht → Farben & Design** öffnen (Fluent kann nach dem Seitentitel suchen).
2. In **Themes aus dem Store** auf **Store aktualisieren** klicken, einen Eintrag auswählen und **Theme installieren** drücken.
3. Das installierte Theme ist oben bereits ausgewählt. **Speichern & anwenden** übernimmt die Wahl und startet ScreenBuddy neu.
4. Nach dem Neustart Auswahl und Vorschau kontrollieren. Eigene Theme-Dateien werden sofort gespeichert; **Verwerfen** nimmt die noch nicht angewandte Auswahl zurück und deinstalliert keine Datei.

Der Plugin-Store enthält die Plugin-Verwaltung; Themes stehen unter Farben & Design. Der gemeinsame Schalter **App → Plugin-Store → Plugin-Store verwenden** steuert weiterhin den Abruf beider signierter Kataloge. Ist er aus, dort einschalten und speichern. Ein Store-Theme kann ein reines Farbdesign sein; es muss nicht das gleichnamige Designprofil aktivieren. **Designsystem** im Theme-Editor bestimmt den Aufbau. Hell/Dunkel oben betrifft das Fenster; das Farbschema unter Farben & Design betrifft die Displayausgabe. Mit **Windows** können Windows-Systemfarben die eigene Palette ersetzen.

## Bilder

Die neuen Menübilder wurden während der Release-Prüfung aufgenommen und anschließend anonymisiert. Überdeckte Konten, Kennungen, Adressen und Vorschauinhalte sind ausdrücklich keine echten Beispielkonten. Rohbilder bleiben privat. Display-Demobilder sind Renderer-Ausgaben mit Beispieldaten und keine Fotos des USB-Panels. Die Galerie zeigt den Menüaufbau in Hell und Dunkel: [Theme-Vergleich](screenshots/themes/README.md).

## Geräteprofil und Sperrzone (Phase 4a, Entwicklung)

Unter **Anzeige → Sperrzone** stehen vier per Tab erreichbare Zahlenfelder oben/rechts/unten/links (Pixel, physischer Rand). USB bezieht sich auf das native Hochformat 320 × 480; der Rand dreht mit der Ausrichtung. Monitore folgen Windows, ohne Software-Drehung. **Speichern & anwenden** (Strg+S) übernimmt die Werte; die danach aktualisierte Vorschau enthält den Rand in der Theme-Hintergrundfarbe.

Unter **Companion → Sperrzone: gemeinsame Vorgabe** gelten die Ränder für den gemeinsamen Betrieb und als Standard. Für das ausgewählte gekoppelte Gerät lässt sich daneben eine eigene Sperrzone sofort speichern oder auf die gemeinsame Vorgabe zurücksetzen; sie wirkt nur bei ausgeschalteter Synchronisation. Das eignet sich für TV-Overscan. Die Gerätevorschau enthält den Rand.

Erlaubt sind ganze Pixel 0–1000; ungültige Eingaben werden mit Meldung abgelehnt. Beim Zeichnen wird jeder Rand auf ein Drittel der kleineren Bildkante begrenzt. Bleiben weniger als 160 × 120 Pixel nutzbar, wird die gesamte Sperrzone ignoriert (ratenbegrenzte Warnung). Ein Tipp im Rand bewirkt nichts; die Drittel-Navigation bezieht sich auf den nutzbaren Inhalt. Veraltete Companion-Bildgrößen werden verworfen.

## Seite anordnen (Entwicklung für 2.10.0)

Unter **App → Seite anordnen** ordnest du die Blöcke eines aktivierten Plugins mit Manifest v2 im Raster an: Zonen festlegen, Blöcke zuweisen, Bedingungen (`show_if`) setzen und die Vorschau in Quer- und Hochformat prüfen. Rückgängig und Wiederholen (100 Schritte) sowie die Tastaturbedienung sind verfügbar. **Übernehmen** speichert nur die Anordnung, mit Sicherungskopie. Inhalte, Linkfreigaben und das Manifest bleiben unverändert. Manifest-v1-Plugins zeigen dort nur den Hinweis „Anordnung braucht Manifest v2“. RSS, Streaming, BF6, Freunde, Discord, System und Spotify sind als eingebaute Seiten enthalten. Home Assistant bleibt im Standard nativ; einzelne Entitäten stehen im separaten Editor „Eigene Ansichten“ bereit. Weitere Einstiege: **Seiten & Reihenfolge** und **Plugins → Anordnen …**. Blöcke lassen sich auch per Maus ziehen (Bausteinleiste, Griffe, Ablage); Tastatur: F6 wechselt den Bereich, Leertaste nimmt auf, Enter legt ab, Esc bricht ab; Strg+Auf/Ab wählt Block-Chips in einer Zone (Leertaste nimmt auf, Entf zurück in die Leiste). Loslassen außerhalb des Rasters bricht still ab. Details: [Plugin-System](PLUGIN-SYSTEM.md#seiten-editor-p6-3-bis-p7-10-entwicklung-für-2100). Maus-, Touchpad- und Touch-Bedienung sind noch nicht abgenommen.

## Plugin-Tipps (Phase 4c, Entwicklung)

Schema-2-Plugins können HTTPS-Links und den nächsten Wechselfeld-Eintrag anbieten. Links öffnen am PC, auch bei einem Tipp auf dem Handy. Die eigene Freigabe unter Einstellungen → Plugins ist standardmäßig aus und zeigt die Link-Hosts. Ziele brauchen 1,0 s Ruhe beim Companion bzw. 0,3 s beim Monitor und sind während Übergängen gesperrt; verweigerte Tipps wechseln die Seite nicht. Ohne Ziel gilt die bisherige Drittel-Regel. Android-TV erhält keine Tap-/D-Pad-Aktion. Details: [Plugin-System](PLUGIN-SYSTEM.md#tap-ziele-für-plugin-blöcke-phase-4c-entwicklung-für-2100). Physische Abnahme offen.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Reiner Built-in-Katalog und strikt versionierte Overlays; Standard behält festen Renderweg, HA später. 14 Suiten grün (Companion offline: 250); Built-in 4.221 Bilder/Trefferkarten: 0 Fehler, Plugin 2.093 Prüfungen: 0 Fehler; physische Abnahme offen.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomare Seitenlayouts mit Backup/Revision, Beispieldatenvorschau, Raster-Touch-Zielen und isoliertem Clip; fester Renderweg unverändert. 14 Suiten grün (Editor 835; Companion offline 250); Built-in 4.221 Bilder/Trefferkarten und Plugin-Baselines unverändert; Hardware offen.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS-Editor mit Beiträge/Pflicht-Zustand, getrennten Quer-/Hochlayouts in `layouts/builtin.rss.layout.json`; Öffnen/Übernehmen von Standard behält den pixelgleichen festen Weg, echte Änderungen nutzen das Raster (Abstände können sich ändern, E2). Zurücksetzen stellt Standard her; Zeilen, Blättern und Touch teilen einen Plan. Kopf/Fuß fest; Built-in-Wechselfelder und HA später. 14 Suiten grün (Katalog 217, Editor 874, Kern 3.470; Companion offline 250); bestehende Baselines bitgleich, zusätzlich 1.024 RSS-Bilder/Trefferkarten und 5.952 Prüfungen ohne Fehler; Hardware offen.

P7-8 (2026-11-06, Agent Codex CLI 0.166.0): Streaming in der Seitenauswahl; Titel/Pflicht-Zustand aus dem vorhandenen Schnappschuss, seitenbezogenes Quer-/Hoch-Overlay, Blättern und Touch aus einem Rasterplan. Standard/Zurücksetzen behalten den festen Weg; TMDB / JustWatch bleibt im Fuß; kompakte Zeilen ohne Datum. 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, 1.792 neue Rasterbilder/Trefferkarten additiv, 6.267 Prüfungen/0; Hardware offen.

P7-9 (2026-11-06, Agent Codex CLI 0.166.0): BF6 in der Seitenauswahl; Rangliste/Pflicht-Zustand, sortierter Schnappschuss, seitenbezogenes Quer-/Hoch-Overlay. Rasterkapazität steuert Blättern und Spielerlinks; kompakte Zeilen zeigen Name, K/D, Rang oder Fehler. Standard/Zurücksetzen bleiben pixelgleich. 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, 1.024 neue Rasterbilder/Trefferkarten additiv, 4.390 Prüfungen/0; Hardware offen.

P7-10 (2026-11-06, Agent Codex CLI 0.166.0): Freunde und Discord in der Seitenauswahl; gemeinsamer Freunde/Pflicht-Zustand-Katalog und sortierte Schnappschüsse, getrennte Overlays `builtin.friends.layout.json` / `builtin.discord.layout.json`, Backups und Revisionen. Rasterzeilen, Blättern und Profilziele aus einem Plan; schmale kompakte Zeilen ohne Avatar/Marken. Standard/Zurücksetzen bleiben pixelgleich (E2 akzeptiert). 14 Suiten grün (Companion offline); bestehende Baselines bitgleich, je 1.024 Rasterbilder/Trefferkarten additiv, zusammen 8.622 Prüfungen/0; Hardware offen.

P7-11 (2026-11-06, Agent Codex CLI 0.166.0): System im Editor mit CPU/GPU/RAM/Datenträger/Netz aus dem vorhandenen Schnappschuss; Temperaturen mit heutigen Schwellenfarben, Netzverlauf in normaler Dichte, kompakt Wert/Balken bzw. Raten. Eigenes Quer-/Hoch-Overlay, Cupertino-Gruppe je Zone, Werkzeugziele aus demselben Rasterplan. Standard/Reset bleiben pixelgleich; E2 bestätigt. 14 Suiten grün (Companion offline); bestehende Baselines unverändert, 1536 System-Rasterbilder/Trefferkarten additiv. Physische Abnahme offen.

P7-12 (2026-11-06, Agent Codex CLI 0.166.0): Spotify im Editor mit Cover, Titel/Interpret/Album, Fortschritt, Tasten, Pegel und Pflicht-Zustand aus vorhandenem Snapshot/Meter; „zuletzt gespielt“ bleibt ohne Fortschritt/Tasten. Quer-/Hoch-Overlay je Seite, kompakter einzeiliger Text, alle drei Pegelfarbmodi, getrennte 48-Punkte-Tastenziele aus derselben Anordnung. Standard/Reset pixelgleich; 14 Suiten grün (Companion offline); bestehende Baselines unverändert, 4096 Spotify-Rasterbilder/Trefferkarten additiv. Physische Abnahme offen.

P7b-0 (2026-11-06, Claude Sonnet 5.5 im Auftrag): Neuer Standard dunkles Material mit Orange-Akzent (`material-orange`; Text/Fläche und Akzent/Fläche mindestens 4,5:1, Leiste mindestens 3:1, Kontrasttests in fl-tests und fl-layout-ui-tests). Neuinstallation startet damit; bestehende Installationen fragen einmalig per Dialog DE/EN „Neuen Standard setzen?“ (Voreinstellung Nein, Antwort in `ui.standard_theme_choice` gespeichert, Schließen ändert nichts). Cupertino, Fluent, Hell, Dunkel, Windows und eigene Themes bleiben wählbar. Bestehende Built-in-/Plugin-Hashdateien unverändert, neue Basis additiv in `p7b0-material-orange-hashes.txt` (768 Hashes).

P7b-6 (2026-11-06, Agent Codex CLI 0.166.0): Geräte-Overlays für Built-ins/Plugins mit Standard-Vererbung (höchstens 16 je Seite, 16 KiB je Overlay, 256 KiB gesamt), atomarer Speicherung/Backup und gemeinsamer Geräteauswahl für Bild/Touch; Display-Tabs mit Größe/Ausrichtung, Offline-Profil und Rückfrage beim Verwerfen. Android-TV am PC markieren (Protokoll v1 meldet keinen Gerätetyp), dann nur Querformat. 14 Suiten grün (Companion offline), elf bestehende Hashdateien unverändert; physische Mehrgeräte-Abnahme offen.

P7b-1 (2026-11-06, Agent Claude Sonnet 5.5): Editor-Fensteraufbau: Kopfleiste (Seitenauswahl, Gerätegröße-Chip, Rückgängig/Wiederholen/Übernehmen/Zurücksetzen), linke Schiene mit Kacheln/Ebenen/Seiten, Fläche und Eigenschaften als Drei-Spalten-Hülle (breit ab 900 DIP; mittel: Eigenschaften unter der Fläche; schmal: untereinander, Palette eingeklappt, Bereichswahl bleibt beim Seitenwechsel); Display-Tab-Leiste unverändert. Neu `ui_editor.*`. 14 Suiten grün, Baselines bitgleich; Handabnahme offen.

P7b-2 (2026-11-06, Agent Claude Sonnet 5.5): Kachelpalette mit Suche, Filter-Chips (Alle/System/Zeit/Medien/Spiele/Sonstiges) und Raster lebender Mini-Vorschauen (derselbe Seitenrenderer mit Beispieldaten: Ausschnitt der Standardseite bzw. Einzelzone bei Plugins; Cache je Seite/Kachel/Größe/Anzeige-Theme, Laden nur beim Seitenwechsel, Kacheln ohne Daten als „keine Daten“). Ziehen auf die Fläche, Klick fügt an der ersten freien Stelle ein (ein Rückgängig-Schritt), Tastatur (Pfeile, Leertaste, Enter, Entf, F6), MSAA „Kachelpalette“. Neu `ui_tile_palette.*`. 14 Suiten grün, Baselines bitgleich.

P7b-3 (2026-11-06, Agent Claude Sonnet 5.5): Fläche im Seitenverhältnis des Display-Tabs: Gerätegrößen-Rahmen, dezentes Zellraster, gestrichelte Ablageplätze „hier ablegen“, Geist mit Einrasten (ungültig: gestrichelt rot und Text „passt nicht“), Griffe, Papierkorb, Zoom −/+/Passend. Opt-in `GridEditor::canvas`; das bisherige Raster bleibt bitgleich. Editor-Snapshots additiv (`p7b-editor-hashes.txt`), Kontrast Text ≥ 4,5:1 und Fokusring/Rahmen ≥ 3:1 in allen Themes geprüft. 14 Suiten grün.

P7b-4 (2026-11-06, Agent Claude Sonnet 5.5): Ebenenliste (Reihenfolge, Sichtbarkeit; Auswahl wirkt in Liste und Fläche), Seitenliste (Wechsel mit Rückfrage bei ungesicherten Änderungen) und Einsteigerkarte „In drei Schritten“ (einmal wegklickbar, gemerkt in `layouts/editor-intro.done`). 14 Suiten grün (fl-layout-ui-tests 1.615), Baselines bitgleich; Handabnahme offen.

## Eigene Ansichten (P8-2, Entwicklung)

Unter **App → Eigene Ansichten** eigene Seiten aus Quellen-Kacheln anlegen, umbenennen, löschen und je aktivem Display anordnen. **Übernehmen** speichert; die Ansicht erscheint als `custom:<id>` im Seitenzyklus. Tabs zeigen nur aktive Geräte; Offline-Layouts bleiben erhalten. Papierkorb, Rastergröße, Bedingungen, Dichte und Rückgängig/Wiederholen: [Bedienung und Grenzen](PHASE8-EIGENE-ANSICHTEN.md).
