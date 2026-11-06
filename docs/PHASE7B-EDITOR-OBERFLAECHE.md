# Phase 7b: Editor-Oberfläche mit Kachelpalette (Beschreibung zum Nachbauen)

Stand 2026-11-06. Eigene Beschreibung des **Bedienkonzepts** (nicht des Aussehens), abgeleitet aus Bastis Auftrag und der öffentlichen Beschreibung eines Modul-Editors (nur Bild/Text, kein Quellcode). Clean-Room: keine Formen, Farben, Texte, Namen oder Assets von dort; Oberfläche im FuchsLabs-Stil (Design-Tokens aus `theme::Design`). Implementierende Agenten lesen nur dieses Dokument und den ScreenBuddy-Code.

**Stil (Vorgabe Basti, neuere Fassung 20:27 gilt):** Standard-Theme von ScreenBuddy wird das **dunkle Material-Theme mit angepassten Orange-Akzenten** (Akzent aus der FuchsLabs-Palette, z. B. ORANGE/GLUT; Text/Fläche-Kontrast ≥ 4,5:1, Fokusring ≥ 3:1). Der künftige Aufbau, auch der Editor, orientiert sich daran (Karten, Radien, Abstände, Schriften aus `theme::Design` Material + `ui_kit`). Der Editor nutzt keine eigene Sonderoptik, sondern immer das aktive Theme. Vorgabe-Fassung 20:23 (Fuchsbau-Cupertino beibehalten) ist damit abgelöst, Rückfrage an Basti offen: nur für Neuinstallationen oder auch für bestehende Konfigurationen als Standard?

## 1. Aufbau (Drei-Spalten-Fenster, Querformat; schmal: Palette einklappbar)

- **Kopfleiste:** Seitenauswahl (Chip mit Name der Seite, Wechsel mit Rückfrage bei ungesicherten Änderungen), Gerätegröße der Vorschau (Chip, z. B. 480 × 320), Rückgängig, Wiederholen, **Übernehmen**, Zurücksetzen.
- **Linke Schiene (schmal, Symbole + Text):** Bereiche **Kacheln**, **Ebenen** (Liste der platzierten Kacheln mit Reihenfolge, Sichtbarkeit, Auswahl), **Seiten** (Seiten der App, öffnet den Editor der gewählten Seite).
- **Linkes Panel „Kacheln“:** Suchfeld, Filter-Chips (Alle, System, Zeit, Medien, Spiele, Sonstiges nach Seitentyp), darunter ein **Raster vorgefertigter kleiner Kacheln**. Jede Kachel zeigt eine **lebende Mini-Vorschau** (derselbe Renderer, Beispieldaten) und darunter den Namen. Anfassen und ziehen; Klick fügt an der ersten freien Stelle ein.
- **Mitte „Fläche“:** Die Seite maßstabsgetreu im gewählten Gerätegrößen-Rahmen, Raster sichtbar (dezent), **leere Ablageplätze gestrichelt** mit Hinweis „hier ablegen“, Einrasten mit Vorschau (Geist) beim Ziehen, Auswahlrahmen mit Griffen (Größe), Papierkorb-Ablage beim Ziehen, Zoom/Passend.
- **Rechtes Panel „Eigenschaften“:** zur gewählten Kachel: Name, Größe (Spalten × Zeilen), „Anzeigen wenn“ (Daten vorhanden/Vergleich), Wechsel (Wechselfeld), Dichte; ohne Auswahl: Eigenschaften der Seite (Raster, Gerätegrenzen).
- **Einsteigerkarte** beim ersten Öffnen („In drei Schritten“): 1 Kachel hinzufügen oder ziehen, 2 Kachel anklicken und rechts einstellen, 3 Übernehmen. Einmal wegklickbar, merkt sich das.

## 1a. Ein Tab pro Display (Vorgabe Basti 20:30)

Der Editor ist für **alle Ausgänge** nutzbar und behandelt jedes Display einzeln: oben eine **Tab-Leiste mit einem Tab pro Display** (USB-Display z. B. COM7 480 × 320, Monitor-Ausgabe, Netzwerk-Display, jedes gekoppelte Companion-Gerät wie Redmi 8 und Pixel 10 Pro, Android-TV). Jeder Tab zeigt Name, Auflösung und Ausrichtung des Geräts (aus Geräteprofil/Companion-Info; Android-TV nur Querformat; Hoch-/Querformat-Phones mit nutzbarer Fläche nach Sperrzone). Die Fläche zeigt die Seite im **Seitenverhältnis dieses Displays**.
**Datenmodell:** Das Overlay einer Seite wird **je Display** gespeichert (Schlüssel: stabile Geräte-ID; Fallback „Standard für alle Displays“, den ein Display erbt, bis es eigene Anordnung hat; Tab zeigt „erbt von Standard“ und „Eigene Anordnung anlegen/zurücksetzen“). Parser/Limits wie bisher je Overlay, zusätzlich Grenzen für Anzahl Geräte-Overlays je Seite (z. B. ≤ 16) und Gesamtgröße; fehlendes/unbekanntes Gerät fällt auf Standard zurück, nie Fehler. App/Renderer wählen beim Zeichnen für das jeweilige Gerät sein Overlay (primär, Monitor, Netzwerk, Companion-Gerät je ID). Companion-Geräte ohne Editor-Fähigkeit am Gerät selbst: Bearbeitet wird am PC; das Handy bekommt das fertige Bild wie bisher (kein Protokoll-Umbau, keine neue APK nötig). Offline-Geräte bleiben als Tab sichtbar (zuletzt bekannte Größe).

## 2. Verhalten

- Kacheln **passen sich der Fläche an** (Dichtestufen, vorhanden), zeigen mit mehr Platz mehr Inhalt; Kacheln ohne Daten treten zurück (`show_if`), in der Vorschau als „keine Daten“ markiert.
- Ziehen aus Palette, Verschieben, Größe, Entfernen in den Papierkorb, **ein Rückgängig-Schritt pro Geste**; ungültige Ziele sind sichtbar (gestrichelt rot + Text „passt nicht“, nicht nur Farbe).
- Live-Vorschau erst nach dem Ablegen (nie während des Ziehens).
- Strg+K: **Befehlspalette** (später, optional): Suche nach Kacheln, Seiten, Befehlen.
- Tastatur gleichwertig (bestehende Bedienung bleibt: F6/Strg+Tab Bereiche, Leertaste aufnehmen, Enter ablegen, Esc abbrechen, Pfeile, Entf, Strg+Z/Y); Fokus sichtbar, Kontrast ≥ 4,5:1 Text / ≥ 3:1 Fokusring in hellen Themes; MSAA-Namen/Werte für Palette, Kacheln, Fläche, Eigenschaften.

## 3. Daten

Unverändert: Overlay pro Seite (Plugin-Overlay bzw. `builtin_catalog`), Manifest wird nie verändert, Katalog der Kacheln je Seite aus den vorhandenen Datenquellen (`builtin_catalog.*`, Plugin-Blöcke). Neu ist nur die Darstellung: Palette mit Mini-Vorschauen, Ebenenliste, Eigenschaftenpanel, Kopfleiste.

## 4. Pakete (nur ein Schreibender gleichzeitig)

- **P7b-0 Standard-Theme (Entscheidung Basti 20:29):** Alles auf Material: Standard-Theme = dunkles Material mit angepassten Orange-Akzenten (neues eingebautes Theme oder Material-Profil dunkel mit Akzent aus FuchsLabs-ORANGE/GLUT, Kontrast ≥ 4,5:1 Text, Fokusring ≥ 3:1, mit Kontrasttests in den Theme-Matrixtests). Neuinstallation: neuer Standard. **Bestehende Installationen:** nach dem Update einmalig Dialog (DE/EN) „Neuen Standard (dunkles Material mit Orange) setzen?“ mit Ja/Nein (Standard Nein), Entscheidung wird gespeichert und nie wieder gefragt, ohne Antwort ändert sich nichts. Cupertino/Fluent/FuchsLabs bleiben als wählbare Themes (nur nicht mehr Standard und nicht Vorbild für den Editor). Plugin- und Built-in-Baselines bleiben bitgleich (Standard betrifft nur den Auslieferungszustand), neue Theme-Snapshots additiv. Dateien: `theme.*`, `config.*`, Start-/Update-Pfad in `app.cpp`/`ui_page_design.cpp`.
- **P7b-1 Fensteraufbau:** Drei-Spalten-Layout, Kopfleiste, linke Schiene mit drei Bereichen, einklappbare Palette bei schmalem Fenster, Eigenschaftenpanel (bestehende Felder umziehen). Dateien: `ui_page_layout.cpp`, `ui_layout_grid.*` (Aufteilung), `ui_kit.*` nach Bedarf, Tests/Snapshots.
- **P7b-2 Kachelpalette:** Suchfeld, Filter, Raster der Kacheln mit Mini-Vorschau über den bestehenden Renderer (Beispieldaten, gecachte Bilder je Größe/Theme), Ziehen und Klick-Einfügen.
- **P7b-3 Fläche:** Gerätegrößen-Rahmen, sichtbares Raster, gestrichelte Ablageplätze, Geist/Einrasten, Griffe, Papierkorb, Zoom.
- **P7b-4 Ebenen und Seiten:** Ebenenliste (Reihenfolge, Sichtbarkeit, Auswahl), Seitenliste, Einsteigerkarte.
- **P7b-6 Display-Tabs und Overlay je Display (Datenmodell zuerst, vor P7b-1 umzusetzen):** Tab-Leiste, Overlay je Geräte-ID mit Standard-Vererbung (§1a), Store/Renderer/Touch je Gerät (`plugin_store.*`, `builtin_catalog.*`, `app.cpp` Geräteauswahl, `device_profile.*`), Editor-Tab-Wechsel mit Rückfrage bei ungesicherten Änderungen, Tests (Vererbung, Grenzen, unbekannte ID, Gerät offline, Android-TV nur Querformat, Mehrgeräte: zwei Geräte unterschiedliche Anordnung bei gleicher Seite) und Mehrgeräte-Prozesstest später durch Basti. Alle Baselines bleiben bitgleich (ohne Overlay = Standard = heute).
- **P7b-5 Abnahme:** Tastatur/MSAA/Kontrast, Editor-Snapshots, Doku DE/EN, Handabnahme durch Basti offen.

Akzeptanz je Paket: alle Suiten grün, Built-in- und Plugin-Baselines unverändert (Editor-Snapshots additiv), Clean-Room-Zeile, kein Netzwerk, nie Port 8765.
