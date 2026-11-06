# Menü und Themes ab ScreenBuddy 2.9.5

Die Seiten heißen in allen Themes gleich. Das Design bestimmt, wie du sie erreichst.

| Gruppe | Einstellungen |
| --- | --- |
| Ansicht | Übersicht, Anzeige, Seiten & Reihenfolge, Farben & Design |
| Seiten | System, Freunde, Discord, Battlefield 6, Blogs (RSS), Streaming-Neustarts, Home Assistant, Spotify |
| App | Companion, Plugins, Plugin-Store, Seite anordnen (ab 2.10.0, in Vorbereitung), Updates & Autostart |

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

Unter **App → Seite anordnen** ordnest du die Blöcke eines aktivierten Plugins mit Manifest v2 im Raster an: Zonen festlegen, Blöcke zuweisen, Bedingungen (`show_if`) setzen und die Vorschau in Quer- und Hochformat prüfen. Rückgängig und Wiederholen (100 Schritte) sowie die Tastaturbedienung sind verfügbar. **Übernehmen** speichert nur die Anordnung, mit Sicherungskopie. Inhalte, Linkfreigaben und das Manifest bleiben unverändert. Manifest-v1-Plugins zeigen dort nur den Hinweis „Anordnung braucht Manifest v2“. RSS ist als erste eingebaute Seite enthalten; weitere eingebaute Seiten und Home Assistant folgen später. Weitere Einstiege: **Seiten & Reihenfolge** und **Plugins → Anordnen …**. Blöcke lassen sich auch per Maus ziehen (Bausteinleiste, Griffe, Ablage); Tastatur: F6 wechselt den Bereich, Leertaste nimmt auf, Enter legt ab, Esc bricht ab; Strg+Auf/Ab wählt Block-Chips in einer Zone (Leertaste nimmt auf, Entf zurück in die Leiste). Loslassen außerhalb des Rasters bricht still ab. Details: [Plugin-System](PLUGIN-SYSTEM.md#seiten-editor-p6-3-bis-p7-7-entwicklung-für-2100). Maus-, Touchpad- und Touch-Bedienung sind noch nicht abgenommen.

## Plugin-Tipps (Phase 4c, Entwicklung)

Schema-2-Plugins können HTTPS-Links und den nächsten Wechselfeld-Eintrag anbieten. Links öffnen am PC, auch bei einem Tipp auf dem Handy. Die eigene Freigabe unter Einstellungen → Plugins ist standardmäßig aus und zeigt die Link-Hosts. Ziele brauchen 1,0 s Ruhe beim Companion bzw. 0,3 s beim Monitor und sind während Übergängen gesperrt; verweigerte Tipps wechseln die Seite nicht. Ohne Ziel gilt die bisherige Drittel-Regel. Android-TV erhält keine Tap-/D-Pad-Aktion. Details: [Plugin-System](PLUGIN-SYSTEM.md#tap-ziele-für-plugin-blöcke-phase-4c-entwicklung-für-2100). Physische Abnahme offen.

P7-5 (2026-11-06, Agent Codex CLI 0.166.0): Reiner Built-in-Katalog und strikt versionierte Overlays; Standard behält festen Renderweg, HA später. 14 Suiten grün (Companion offline: 250); Built-in 4.221 Bilder/Trefferkarten: 0 Fehler, Plugin 2.093 Prüfungen: 0 Fehler; physische Abnahme offen.

P7-6 (2026-11-06, Agent Codex CLI 0.166.0): Atomare Seitenlayouts mit Backup/Revision, Beispieldatenvorschau, Raster-Touch-Zielen und isoliertem Clip; fester Renderweg unverändert. 14 Suiten grün (Editor 835; Companion offline 250); Built-in 4.221 Bilder/Trefferkarten und Plugin-Baselines unverändert; Hardware offen.

P7-7 (2026-11-06, Agent Codex CLI 0.166.0): RSS-Editor mit Beiträge/Pflicht-Zustand, getrennten Quer-/Hochlayouts in `layouts/builtin.rss.layout.json`; Öffnen/Übernehmen von Standard behält den pixelgleichen festen Weg, echte Änderungen nutzen das Raster (Abstände können sich ändern, E2). Zurücksetzen stellt Standard her; Zeilen, Blättern und Touch teilen einen Plan. Kopf/Fuß fest; Built-in-Wechselfelder und HA später. 14 Suiten grün (Katalog 217, Editor 874, Kern 3.470; Companion offline 250); bestehende Baselines bitgleich, zusätzlich 1.024 RSS-Bilder/Trefferkarten und 5.952 Prüfungen ohne Fehler; Hardware offen.
