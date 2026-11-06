# Phase 4 – Umsetzungsentwurf: Geräteprofile, Wechselfeld, Tap-Ziele

Stand 2026-11-06, Branch `feature/zonen-layout` (Phase 0–3 committet, Kopf `44e0dca`). **Nur Entwurf, kein Produktivcode, nichts committet.**
Grundlage: `PLAN-screenbuddy-zonen-layout.md` (Abschnitt Phase 4, Leitplanken, Clean-Room) und der ScreenBuddy-Code. libre-panel-Code wurde weder geholt noch gelesen. Alle Begriffe und Schlüssel sind eigene ScreenBuddy-Begriffe.
Pfade relativ zu `screenbuddy-store/`. Zeilenangaben beziehen sich auf den Stand `44e0dca`.

Entscheidungen von Basti (gesetzt):
- Android-TV läuft nur im Querformat.
- Das Wechselfeld bekommt eine Animation: ruhiger Übergang, keine Töne, keine aufdringlichen Effekte.
- Eingebaute Seiten werden erst in Phase 5 migriert. Phase 4 gilt nur für Plugin-Seiten (Manifest v2 mit `layout`) und für die Gerätemaske, die für alle Seiten gilt.

## 0. Ist-Stand (relevant für Phase 4)

| Stelle | Befund |
|---|---|
| `cpp/src/app.cpp` 66–68 | Die Größe kommt aus `out_->size()`. Ohne Ausgang gilt `display.orientation` (480×320 bzw. 320×480). |
| `cpp/src/serial_display.cpp` 95–98, 175–185 | Das Panel ist nativ 320×480. Die Ausrichtung schickt das Gerät selbst (`SET_ORIENTATION`), die App zeichnet in logischer Ausrichtung. Kein Touch. |
| `serial_display.cpp` 219–255 | Der Bilddiff arbeitet in Bändern zu 8 Zeilen, **immer mit voller Breite**. Kosten einer Änderung: geänderte Zeilen (auf 8 gerundet) × Breite × 2 Byte, egal wie schmal die Zone ist. |
| `app.cpp` 701–738 | `output()`: `transition=true` oder 300 s ⇒ `invalidate()` ⇒ beim nächsten Mal ein Vollbild. `show()` läuft synchron im Anzeige-Thread. |
| `app.cpp` 742–752 | `render_core` überspringt den Frame, wenn nichts davon zutrifft: dirty, Seitenwechsel fällig (`page_seconds`, 745), live (System/Amplifier), Schritt, Größe, Minute. |
| `app.cpp` 881–919 | `display_loop`: `wait_for` höchstens 1 s (888), danach fest `Sleep` 10×50 ms (917–918), mit Amplifier 2×50 ms. Ergebnis: bis zu ~1,5 s Verspätung, kein Animationstakt. |
| `app.cpp` 802–819 | Bild und Touch-Ziele werden gemeinsam unter `tap_mu_` veröffentlicht. Bei langsamem Seriell-Ausgang läuft der Companion erst danach (914–915). |
| `app.cpp` 515–553 | Plugin-Seiten: `page_list` rendert `render_page` und fängt `draw_page` in einem Lambda. Das Lambda läuft auch in `redraw_` (791, Theme-Vorschau). `page_list` wird bis zu zweimal je Frame aufgerufen (766/771). Plugins haben **keine** Touch-Ziele. |
| `app.cpp` 863–880, 924–961 | `device_tap`/`tap_at`: zuerst eine Aktion, dann ein Ziel. Sonst gilt die Regel „linkes Drittel = zurück, sonst weiter“. Companion-Tipps werden nur über `w/h` gegen das letzte Bild geprüft. |
| `cpp/src/touch.h` 11–40, `touch.cpp` 84–129 | `Tile{box, rows, cells}` mit seitenspezifischer Trefferprüfung. `open()` nutzt ShellExecute. Ziele gehören zum gezeichneten Datenstand (Kommentar Z. 2). |
| `cpp/src/network_display.cpp` 284–311 | `show`/`show_for` vergleichen Pixel und überspringen unveränderte Bilder. Bei Änderung wird je Gerät ein JPEG erzeugt (0,8). |
| `cpp/src/companion_server.cpp` 666–690, 721–761 | Touch höchstens 10/s mit `sent_w/sent_h`, **ohne Bildversion**. Frames gehen über ein Tor mit höchstens 5 fps. |
| `android/.../DisplayActivity.kt` 57–61, `MainActivity.kt` 95–104 | Ausrichtung aus `Store.orientation`, auch auf dem TV. `FrameView.kt` 53–89 spart Kamera-Ausschnitt und Rundungen bereits aus (`contentWidth/Height`). Das TV hat keinen Touch, D-Pad LEFT/RIGHT ist Wischen, CENTER bleibt ohne Aktion. |
| `cpp/src/plugins.cpp` 525–597 | `missing_data` gilt nur für die ganze Seite. Pro Block gibt es kein Kennzeichen „hat Daten“. |
| `cpp/src/plugins.cpp` 666–673 | `store_problems` verlangt für v2 `min_app_version` ≥ 2.10.0. |
| `cpp/TESTPROTOKOLL.md` 71 | „keine gemessene serielle Bildrate“. Die 26,6 s aus `UEBERGABE-CODEX-2.5.md` 182 sind eine Baud-Rechnung. USB-CDC ignoriert die Baudrate meist, die Zahl taugt daher **nicht** als Budget. |

## 4a – Geräteprofil: Ausrichtung, Sperrzone, Touch pro Gerät

### Entscheidungen
1. **Geräteprofil** = Ausrichtung + Sperrzone (Ränder in Pixeln, die physisch verdeckt oder unbrauchbar sind, z. B. Rand des Hochkant-Gehäuses, TV-Overscan). Daraus ergibt sich die **nutzbare Fläche**.
2. **Ausrichtung je Ausgang:**
   - Seriell: wie bisher `display.orientation`, 4 Werte.
   - Monitor: folgt der Windows-Anzeige. In Phase 4 gibt es keine Software-Drehung.
   - Companion-Handy: wie bisher in der App wählbar.
   - **Android-TV: fest Querformat.** Die Auswahl wird auf dem TV ausgeblendet.
3. **Sperrzone in physischen Panel-Koordinaten** (Kante oben, rechts, unten, links des Geräts in Grundlage-Ausrichtung). Die App rechnet sie je `Orientation` in Bildkoordinaten um. Ein Gehäuserand bleibt so beim Drehen am selben physischen Rand.
   - Seriell: Grundlage ist das native Hochformat 320×480.
   - Monitor und Companion: Grundlage ist die gemeldete Größe, ohne Umrechnung.
4. **Eine Stelle wendet die Maske an:** Seiten werden in der nutzbaren Größe gezeichnet und in ein Vollbild mit Rand in `theme.bg` gesetzt.
   - Die Touch-Boxen werden um den Versatz verschoben.
   - Tipps in die Sperrzone werden verworfen: kein Ziel, kein Seitenwechsel.
   - Die Drittel-Regel nutzt die **nutzbare** Breite.
5. **Grenzen:** je Rand 0..⌊min(B,H)/3⌋. Die nutzbare Fläche muss ≥ 160×120 bleiben, sonst wird die Sperrzone ignoriert (Log, ratenbegrenzt).
6. **Speicherorte:**
   - `display.keepout` = `{"top":0,"right":0,"bottom":0,"left":0}` für Seriell/Monitor.
   - `companion.keepout` gilt im gemeinsamen Betrieb und als Standard.
   - Je Gerät optional in `companion.json` über `companion::Store`, neben `device_pages` (`companion.cpp` 723–735). Nur im unabhängigen Betrieb.
7. **Handy-Ausschnitte** bleiben clientseitig (`FrameView`). Serverseitig gibt es für Handys standardmäßig keine Maske (0).
8. **Android-TV:** Die Sperrzone dient als Overscan-Rand, eingestellt am PC je Gerät. Touch gibt es dort nicht, 4c greift auf dem TV nicht. D-Pad CENTER bleibt ohne Aktion (kein Fokus-Modell in Phase 4).
9. **Kein Protokollwechsel:** `ready.w/h` bleibt die volle Fläche. Touch-Koordinaten bleiben in voller Fläche, die Umrechnung macht der PC.

### Datenmodell und Schnittstellen
Neues reines Modul `cpp/src/device_profile.h/.cpp`, Namensraum `fl::device`, ohne Win32. Testziel `fl-device-profile-tests`.
```cpp
struct Insets { int top = 0, right = 0, bottom = 0, left = 0; };
struct Profile { Orientation orientation = Orientation::Landscape; Insets keepout_physical; bool physical_rotates = false; };
struct Area { int x = 0, y = 0, w = 0, h = 0; };                  // nutzbare Fläche in Bildpixeln
Insets to_frame(const Insets& physical, Orientation o);           // physische Ränder -> Bildränder
Area usable(std::pair<int,int> frame, const Profile& p);          // fällt bei Grenzverletzung auf {0,0,B,H} zurück
std::optional<std::pair<int,int>> to_content(const Area& a, int x, int y); // nullopt = Sperrzone
Image place(const Image& content, std::pair<int,int> frame, const Area& a, Color bg);
Insets parse_insets(const ojson& j);                              // fehlend/ungültig -> 0, geklemmt
```
Abbildung `to_frame`, Bild = Gerät gedreht. Seriell: `Portrait` = physisch.
- `Landscape`: Bild-oben = physisch-rechts, Bild-rechts = physisch-unten, Bild-unten = physisch-links, Bild-links = physisch-oben.
- `ReverseLandscape` ist die Gegenrichtung, `ReversePortrait` tauscht oben/unten und links/rechts.

**Prüfpflicht:** Die Richtung von `Landscape` vs. `ReverseLandscape` bestätigt Basti einmal am Gerät (siehe Test 4a-H1). Ist sie falsch, wird nur die Tabelle getauscht.

App-Anbindung:
- `ViewState` bekommt `device::Area area`.
- `render_core` (`app.cpp` 742) rechnet `area = usable(size, profile)` und zeichnet mit `area.w × area.h` statt `size`. Das gilt für `tile_layout`, `page_list` und `draw`.
- Danach `place(...)`, die `touch::Tile.box` werden um `area.x/y` verschoben.
- `tap_at`/`device_tap`/`companion_tap`: zuerst `to_content`. Bei `nullopt` gilt der Tipp als verbraucht und es geschieht nichts. Die Drittel-Regel nutzt `area.w`.
- Profil-Quelle:
  - `profile_for(primary)`: `display.orientation` + `display.keepout`. `physical_rotates` nur bei Seriell.
  - `profile_for(companion, device)`: `companion::Store::device_keepout(id)`, sonst `companion.keepout`.

### Reihenfolge
P4a-1 (reines Modul) → P4a-2 (Konfiguration + App + Companion-Store) → P4a-3 (Einstellungs-UI) → P4a-4 (Android-TV Querformat).

### Risiken
- `redraw_` (791) und die Vorschau-Pfade (`last_frame`, `device_frame`) müssen das maskierte Bild liefern. Deshalb geschieht `place` in `draw()`, nicht erst in `output()`.
- Die `RendererSet`-Cache-Schlüssel ändern sich mit der nutzbaren Größe. Die vorhandene Grenze `sets_.size() > 16` (764) reicht.
- Eine falsche Drehtabelle legt die Maske an die falsche Kante. Abhilfe: Hardware-Test, Tabelle zentral an einer Stelle.

### Tests
- `fl-device-profile-tests`:
  - `to_frame` für 4 Ausrichtungen × asymmetrische Ränder (z. B. 10/20/30/40).
  - Grenzklemmen, Rückfall bei zu kleiner Fläche.
  - `to_content` an allen vier Kanten (erstes/letztes Pixel innen/außen).
  - `place`: Randfarbe und Inhalt pixelgenau.
- Testgrundlage: `App` ist in keinem Test konstruierbar (`fl-tests` linkt `flcore`, bindet aber nur Header ein, `tests.cpp` 9–30). Die App-Logik wird daher als **freie Funktionen** in `device_profile` getestet:
  - `compose_view(content, frame, area, bg)`;
  - `shift_tiles(tiles, area)`;
  - `resolve_tap(area, x, y)` → {Sperrzone | Inhalt(x', y') | Drittel-Schritt ±1 bezogen auf `area.w`}.
  - `app.cpp` ruft nur diese Funktionen auf.
- **Mehrgeräte, rein** (`fl-device-profile-tests`): USB 480×320 mit Rand links 40, Gerät A 2400×1080 mit Rand 0, Gerät B 1920×1080 mit Rand 48. Je Profil stimmen nutzbare Fläche, Versatz und verschobene Ziele. `resolve_tap(x=20)` auf USB ⇒ Sperrzone.
- **Mehrgeräte, Prozess** (Erweiterung von `cpp/tests/app_companion_multi.py`: `--no-lcd`, `FUCHSLABS_INSTANCE`, rohe WebSocket-Clients):
  - zwei Clients mit 2400×1080 und 1920×1080, unabhängiger Betrieb, `keepout` für Gerät B in der Testkonfiguration;
  - geprüft: `ready.w/h` je Client = volle Größe; JPEG-Abmessungen stimmen; der Randstreifen von B ist einfarbig (JPEG-Toleranz ±8);
  - ein `touch` in B's Sperrzone ändert die Navigation **nicht** (Status über die lokale Einstellungs-API wie im bestehenden Skript).
- **Touch-Snapshot:** Bild und Ziele aus demselben `RenderOut`. Ein Tipp mit veralteter Größe wird verworfen (vorhandene Prüfung, 872/944, bleibt).
- Hardware (Basti): Hochkant-Gehäuse in allen 4 Ausrichtungen (4a-H1), TV-Overscan-Rand, TV dreht nicht ins Hochformat.

## 4b – Wechselfeld pro Zone

### Entscheidungen
1. Eine Zone mit `cycle` ist ein **Wechselfeld**: Ihre Blöcke (höchstens 4, wie bisher) sind **Einträge**. Es ist immer genau einer sichtbar, und er bekommt die ganze Zonenfläche. Er wird nicht gestapelt.
2. Modi:
   - `rotate`: Einträge mit Daten der Reihe nach, je Eintrag `seconds` (3..600, Standard 8).
   - `first_available`: der erste Eintrag mit Daten. Er wechselt nur, wenn sich die Datenlage ändert.
3. **„Hat Daten“** = Block sichtbar (Phase 3, `show_if`) **und** keine fehlende Referenz beim Auswerten (neu: `RenderedWidget::missing`).
   - Hat kein Eintrag Daten, bleibt die Zone leer (Hintergrund), wie bei ausgeblendeten Blöcken.
   - Ein Eintrag ohne Daten wird in `rotate` übersprungen. Er verlängert die Zeit des aktuellen nicht.
4. **Zustand je Ansicht** (`ViewState`; im unabhängigen Betrieb je Gerät über `DeviceState.view`), Schlüssel `(plugin-id, zone-id)`.
   - **Zurücksetzen**, sobald die Seite die sichtbaren Kacheln verlässt. Beim nächsten Eintritt startet die Zone beim ersten Eintrag mit Daten, ohne Übergang.
   - Größenwechsel: Ein laufender Übergang endet sofort.
   - Manifest-Update, also Generation geändert: zurücksetzen.
5. **Übergang** `transition`: `"fade"` (Standard) oder `"none"`. Die tatsächliche Form legt das **Geräteprofil** fest, nicht das Manifest:

| Ausgang | Form | Schritte | Dauer | Bedingung |
|---|---|---|---|---|
| Monitor | Überblendung | 4 | 400 ms (100 ms je Schritt) | Renderzeit-EWMA ≤ 50 ms, sonst 2 Schritte, über 100 ms Schnitt |
| Companion | Überblendung | 2 | 400 ms (200 ms, passend zum 5-fps-Tor) | Gerät verbunden; ohne Clients Schnitt |
| Seriell | **Band-Vorhang** von oben nach unten (je Schritt ein Viertel der Zonenhöhe, auf 8-Zeilen-Bänder gerundet) | 4 | so lange wie die Übertragung, Ziel ≤ 600 ms | gemessene Rate R: `zone_bytes / R ≤ 0,6 s`, sonst Schnitt |

   - Warum seriell keine Überblendung: Eine Überblendung schickt die ganze Zone in jedem Schritt erneut (4× Bytes). Der Vorhang schickt jedes Band genau einmal und kostet damit **dasselbe wie ein harter Schnitt**.
   - Die Mischwerte sind auf die Schrittzahl quantisiert. Zwischen den Schritten entstehen keine Bilder. Gleiche Bilder werden von Seriell und Netzwerk ohnehin übersprungen.
   - Keine Töne, kein Blinken, keine Bewegung außerhalb der Zone, keine Easing-Spielereien. Die Mischung ist linear.
6. **Ein Wechselfeld-Wechsel setzt nie `RenderOut.switched`.** Sonst ruft `output()` (716) `invalidate()` auf und sendet seriell ein Vollbild. Wechselfeld-Bilder laufen ausschließlich über den Banddiff. Der Vollbild-Takt (300 s) bleibt.
7. **Gleichzeitige Wechsel:** Wird während eines Übergangs eine weitere Zone derselben Kachel fällig, wartet sie bis zum Ende, höchstens 600 ms. Die Kacheln verschiedener Seiten sind unabhängig.
8. **Langsame Ausgänge:** Effektive Eintragsdauer = max(`seconds`, 4 × letzte Übertragungsdauer des Ausgangs). So bekommt der Inhalt immer mehr Zeit als das Senden.
9. **Tipp während eines Übergangs** in diese Zone: verschluckt (siehe 4c), kein Seitenwechsel.

### Mechanismus Zeitplanung
Neues reines Modul `cpp/src/plugin_schedule.h/.cpp`, Namensraum `fl::plugins::schedule`. Die Uhr wird injiziert, keine Grafik. Testziel `fl-plugin-schedule-tests`.
```cpp
enum class Mode { rotate, first_available };
enum class Shape { cut, fade, reveal };                 // Form kommt aus dem Geräteprofil
struct EntryInput { std::string block; int seconds; bool available; };
struct ZoneInput  { std::string zone; Mode mode; bool animate; std::vector<EntryInput> entries; };
struct Budget     { Shape shape = Shape::cut; int steps = 0; double step_seconds = 0; double min_entry_seconds = 0; };
struct ZoneState  {                                    // je (Ansicht, Plugin, Zone)
    std::optional<size_t> shown; double since = 0;     // aktueller Eintrag, seit wann
    struct Fade { size_t from, to; double start; int steps; double step_seconds; Shape shape; };
    std::optional<Fade> fade;
};
struct ZoneChoice { std::string zone; std::optional<std::string> block, from_block; Shape shape; float mix; /* 0..1, quantisiert */ };
struct Result { std::vector<ZoneChoice> zones; double next_deadline; bool animating; };
// Idempotent für gleiches `now` (page_list läuft bis zu zweimal je Frame).
// `next_requests`: Zonen-IDs mit Tipp-Aktion „next“ (4c), vom Anzeige-Thread übergeben.
Result advance(const std::vector<ZoneInput>& zones, std::map<std::string, ZoneState>& state, double now, const Budget& budget,
               const std::vector<std::string>& next_requests = {});
```
Ablauf je Frame:
1. `render_core` holt `now = mono()` **einmal** und reicht es durch.
2. `page_list(…, ViewState& view, double now)` ruft bei Plugin-Seiten `render_page` auf, baut daraus die `ZoneInput` (Manifest-`cycle` + `RenderedWidget::missing`/sichtbar), ruft `advance` mit `view.cycles` auf und fängt das Ergebnis **als Wert** im Lambda.
   - Das Lambda bleibt rein, und `redraw_` (Theme-Vorschau) verändert keinen Zustand.
3. `advance` liefert `next_deadline`:
   - ohne Übergang: Ende des aktuellen Eintrags (`since + max(seconds, min_entry_seconds)`), bei `first_available` +∞;
   - während eines Übergangs: der nächste Schritt (`start + (k+1)·step_seconds`).
4. `render_core` setzt `view.next_deadline = min` über alle sichtbaren Wechselfelder. Neue Skip-Bedingung (752):
   `if (!dirty && !due && !live && view.step == 0 && view.size == size && view.minute == minute && now < view.next_deadline) return out;`
   Ohne Wechselfeld ist `next_deadline = +∞`, das Verhalten bleibt dann **bitgleich** zu heute.
5. `display_loop` (881–919) bekommt Fristen. Reihenfolge und Werte sind fest:
   - **Zuerst** die Ruhe nach dem Zeichnen wie heute (917–918): `Sleep` in 50-ms-Schritten mit `stop_`-Prüfung, insgesamt `min_gap`. `min_gap` = 500 ms, mit Amplifier 100 ms, **während eines Übergangs 50 ms**.
   - **Danach** `cv_.wait_until(min(now + 1 s, alle Wechselfeld-next_deadline (primär, Companion, jedes Gerät)))` statt `wait_for(1 s)`. `dirty_` weckt wie bisher früher.
   - `page_seconds` kommt **nicht** in die Fristen und wird wie heute vom 1-s-Warten erfasst. Ohne Wechselfeld ist jede Frist +∞, die Schleife ist dann **identisch** zu heute.
   - Die Berechnung steckt in der reinen Hilfsfunktion `schedule::wake_at(now, deadlines, animating, meter)` → `{min_gap, wait_until}` im gleichen Modul, testbar.
6. Zeichnen eines Übergangs (P4b-3, `plugin_render.cpp`):
   - Bild A = Seite mit `from_block`, Bild B = Seite mit `block`. Beide werden mit denselben Daten (`page`) gezeichnet.
   - Dann wird nur im Pixelrechteck der Zone gemischt (`fade`) bzw. zeilenweise von B übernommen (`reveal`). Außerhalb der Zone ist das Bild = B.
   - A und B liegen in einem `shared_ptr<plugins::FadeCache>` (definiert in `plugin_render.h`), **app-seitig** in `ViewState::fade_caches`, Schlüssel `(plugin, zone)`. `ZoneState` bleibt rein, ohne Bilder. Das Plugin-Lambda fängt den Zeiger neben dem `ZoneChoice`-Wert. Cache-Schlüssel innen: Breite, Höhe, `render::theme_revision()` (neuer Zähler in `apply_theme`). Je Schritt kostet das nur Kopie + Mischung, kein Neuzeichnen. Zugriff nur unter `render::mutex()`, wie alles Zeichnen (762, 488).
   - Die Daten bleiben während des Übergangs (≤ 600 ms) eingefroren. Neue Daten erscheinen im ersten Frame danach.

7. **Budgetquelle je Ansicht:**
   - Die Primäransicht nimmt die Art von `out_`: `SerialDisplay` ⇒ Seriell, `is_monitor()` ⇒ Monitor, `NetworkDisplay` (Ausgang `network`) ⇒ Companion.
   - Die Sekundäransicht (`companion_output_`) und jedes unabhängige Gerät sind immer Companion.
   - Ohne Ausgang (Vorschau, `preview_dir_`) wird geschnitten.

### Frame-, USB- und Energiebudget (verbindlich)
- **Frame-Budget** (Release-PC, Release-Build): `render_core` ohne Übergang ≤ 30 ms bei 480×320 und ≤ 60 ms bei 1920×720. Ein Übergangsschritt aus dem Cache ≤ 10 ms bzw. ≤ 25 ms (2400×1080).
  - Neue Messung `render_ms` als EWMA je Ansicht, LOGD.
  - Liegt die EWMA über der Hälfte von `step_seconds`, halbiert `Budget` die Schritte. Bei < 2 Schritten wird geschnitten.
- **USB-Budget:** Paket P4b-0 misst zuerst. `SerialDisplay::show` liefert Bytes und Dauer, `Output::transfer_rate()` (EWMA in Byte/s, `std::optional`, leer bis zum ersten Teil-Update).
  - Kosten je Zonenwechsel = `ceil8(zone_rows) × width × 2` Byte, einmal (Schnitt/Vorhang).
  - Regeln: Ist R noch unbekannt ⇒ Schnitt. Vorhang nur bei `zone_bytes / R ≤ 0,6 s`. `min_entry_seconds = 4 × letzte Übertragungsdauer`.
  - Beispiel nur zur Einordnung: Zone 480×160 = 153.600 Byte. Bei R = 1 MB/s dauert das ≈ 0,15 s, Vorhang mit 4 Schritten möglich. Bei R ≤ 256 kB/s wird geschnitten.
  - Der reale Wert R wird von Basti am Gerät gemessen und in `cpp/TESTPROTOKOLL.md` eingetragen.
- **Companion:** Je Gerät und Schritt ein JPEG. Bei 2 Schritten sind das 2 zusätzliche JPEGs je Wechsel, gedeckelt durch das 5-fps-Tor. Ohne verbundene Clients (`client_ids()` leer) wird immer geschnitten.
- **Energie/CPU:** kein Polling. Der Thread wacht nur zu Fristen auf. Ohne Wechselfeld bleibt die Last unverändert (≤ 1 Wecken/s wie heute).
  - Je Wechsel gibt es 1 Frame (Schnitt) bzw. 3–5 Frames (Übergang). Bei 8 s je Eintrag und 4 Schritten sind das ≈ 0,6 zusätzliche Frames/s im Mittel.
  - Nicht sichtbare Seiten haben keine Fristen.
- **Bekanntes Risiko:** Seriell `show()` blockiert den Anzeige-Thread vor dem Companion (914–915). Eine lange Seriell-Übertragung verzögert Companion-Schritte. Gegenmaßnahme in Phase 4:
  - Kommt ein Companion-Schritt mehr als 1 Schrittdauer zu spät, springt `advance` direkt zum Ende des Übergangs (`mix = 1`) statt nachzuholen.
  - Einen eigenen Seriell-Thread gibt es erst später (nicht Phase 4).

### Reihenfolge
P4b-0 (Messung) → P4b-1 (Parser/Daten) → P4b-2 (`plugin_schedule`) → P4b-3 (Zeichnen mit Auswahl/Übergang) → P4b-4 (App-Zeitplanung).

### Risiken
- Doppelter `page_list`-Aufruf: abgesichert durch die Idempotenz von `advance` bei gleichem `now`. Test vorhanden.
- `redraw_` darf keinen Zustand fortschreiben: Das Lambda fängt nur Werte und den Cache-Zeiger.
- Theme-Wechsel während eines Übergangs: Der Cache-Schlüssel enthält die Theme-Revision.
- Ein Übergang verlängert Frames bei vielen Geräten: Die Schritte richten sich nach der Renderzeit-EWMA.

### Tests
- `fl-plugin-schedule-tests`, simulierte Uhr:
  - `rotate` mit 3 Einträgen (Dauern 3/5/8) über 2 Runden.
  - Eintrag ohne Daten wird übersprungen; alle ohne Daten ⇒ leer.
  - `first_available` wechselt nur bei Datenänderung.
  - Idempotenz (zweimal `advance(now)`).
  - Zurücksetzen bei Seitenaustritt.
  - Quantisierung (`mix` ∈ {0, ¼, ½, ¾, 1}).
  - Verspäteter Schritt springt ans Ende.
  - Zweite Zone wartet ≤ 600 ms.
  - `min_entry_seconds`-Dehnung.
  - `wake_at` mit Kombinationen aus Seitenwechsel, Frist, `min_gap`.
- **Frame-Skip:** `fl-plugin-schedule-tests`. Die Skip-Entscheidung von `render_core` wird als reine `schedule::should_render(inputs)` herausgelöst und dort getestet, `app.cpp` ruft nur sie auf.
  - Ohne Wechselfeld bleibt die Folge der gerenderten Frames über 60 simulierte Sekunden **identisch** zu vorher: 1/Minute + `page_seconds`, Referenzliste im Test.
  - Mit Wechselfeld werden genau die erwarteten Frames gerendert, und zwischen Fristen wird nichts gerendert.
  - Kein Wechselfeld-Frame setzt `switched`. `advance` liefert dafür kein Feld; geprüft wird im Review (P4b-4) und über den Prozess-Test. Die Navigation (`index`) ändert sich bei Wechseln nicht.
- **USB:** reine Kostenfunktion `schedule::serial_bytes(prev, next)` über dieselbe 8-Zeilen-Bandlogik. Sie wird aus `SerialDisplay::show_locked` 234–252 als freie Funktion `changed_bands(old, new, band)` in `output.h`/`serial_display.cpp` herausgelöst, `show_locked` nutzt sie. Geprüft: Bytes je Wechsel = `ceil8(rows)·w·2`, Summe des Vorhangs = Schnitt, Überblendung = Schritte × Zone.
  - Bei unbekannter Rate wird geschnitten.
  - Hardware-Messung R durch Basti (Protokoll).
- **Snapshots** (`fl-plugin-snapshot-tests`): je Eintrag; `fade` bei mix ½; `reveal` bei Schritt 2/4; leere Zone. Alle Themes, Quer und Hoch.
- **Mehrgeräte:**
  - Rein: zwei `ZoneState`-Maps (Gerät A/B) mit verschiedenen Budgets haben getrennte Fristen. Ein Seitenaustritt auf A setzt B nicht zurück.
  - Verspätung: `advance` mit `now` 800 ms nach dem fälligen Schritt (simulierte langsame Seriell-Übertragung vor dem Companion) springt ans Ende.
  - Prozess: `app_companion_multi.py` mit dem Beispiel `offline-wechselfeld` und zwei Clients. Beide erhalten innerhalb von `seconds + 2 s` ein neues Bild, ohne dass sich die Navigation ändert.

## 4c – Tap-Ziele für Plugin-Zonen

### Entscheidungen: erlaubte Aktionen (geschlossene Liste)
| Aktion | Wirkung | Berechtigung |
|---|---|---|
| `open_link` | öffnet eine **https**-Adresse im Standardbrowser des **PCs** | Touch an (`display.touch` bzw. Companion verbunden) **und** Plugin-Freigabe „Links öffnen“ (Standard aus) |
| `next` | zeigt im Wechselfeld der Zone sofort den nächsten Eintrag mit Daten (mit dem normalen Übergang des Geräteprofils) | nur Touch an |

- Ausdrücklich **nicht** erlaubt: andere Protokolle (`file:`, `ms-settings:`, `steam:` …), App-Starts, Home-Assistant-Schalter, Systemwerkzeuge, POST/Netzanfragen, Einstellungsänderungen, Aktualisieren der Quellen. Das schließt sich bewusst an den Grundsatz „deklarativ, keine Code-Ausführung“ (`plugins.h` Z. 1) an.
- **Hinweis für Nutzer und Doku:** Ein Tipp auf dem Handy öffnet den Link **am PC**, wie die bestehenden Ziele (`network_display.cpp` 253–273).
- Android-TV: Es gibt keinen Touch, 4c entfällt dort.

### Manifest (v2, neue Schlüssel)
- Oberste Ebene: `"links": ["news.example.org", …]`, höchstens 4 Hosts. Geprüft mit **denselben** Host-Regeln wie Quellen (`url_host`, `plugins.cpp` 168–188: nur https, keine IP-Literale, keine lokalen Namen, kein Port außer 443, kein `#`).
- Block: `"tap": {"action": "open_link", "url": "https://news.example.org/a/{{feed.id}}"}` oder `"tap": {"action": "next"}`.
  - `url`: Schema exakt `https://`, geprüft mit `url_host` (also kein Fragment). Der Host ist **wörtlich** und steht in `links`, Platzhalter sind im Host unzulässig. Platzhalter nur in Pfad und Query. Höchstens 2048 Zeichen nach der Expansion.
  - Platzhalter: Quellenfelder und **nicht geheime** Einstellungen. Geheime Einstellungen werden abgelehnt (gleiche Prüfung wie `show_if`, `plugins.cpp` 268). `item.*` gilt nur in `list`-Blöcken (Ziel je Zeile).
  - Bei der Expansion wird **jeder** Platzhalterwert prozentkodiert (RFC 3986 unreserved). Fehlt ein Wert, gibt es **kein Ziel**: Der Block ist nicht antippbar, es wird kein halber Link geöffnet.
  - `next` ist nur in Zonen mit `cycle` erlaubt.
- Zone: `"cycle": {"mode": "rotate" | "first_available", "transition": "fade" | "none"}`, Block: `"seconds": 3..600`, nur in `rotate`-Zonen.
- Alle neuen Schlüssel gibt es nur bei `schema_version: 2`, über die `fields()`-Positivlisten (`plugins.cpp` 297, 341–342, 434–438). v1 bleibt unverändert, ebenso v2 ohne die Schlüssel.
- **Version:**
  - Phase 4 erscheint **zusammen mit Phase 2/3 in 2.10.0**. `store_problems` (666–673) verlangt dann wie bisher `min_app_version` ≥ 2.10.0.
  - Wird 4b oder 4c später als 2.10.0 veröffentlicht, ist Pflicht: `store_problems` bekommt eine Tabelle „Schlüssel → Mindestversion“ (z. B. `cycle`/`tap`/`links` → 2.11.0). Der strenge Parser einer älteren 2.10 lehnt unbekannte Schlüssel ab. Das ist sicher, aber für Nutzer unschön, und der Katalog zeigt dann „unsupported“.

### Berechtigungsmodell
1. `plugins::Store` bekommt `links_allowed` je Plugin (persistiert wie `consented_hosts`, `plugin_store.h` 10–31), `set_link_consent(id, bool)`.
   - Zusätzlich gibt es einen `link_fingerprint(manifest)` (sortierte `links`).
   - Ändert ein Update die Link-Hosts, wird **nur** diese Freigabe zurückgesetzt, die Quellenfreigabe nicht.
   - Jede Änderung erhöht `generation`.
2. Die Freigabe ist Standard aus. Die Einstellungsseite des Plugins zeigt die Hosts aus `links` und einen Schalter „Links aus diesem Plugin am PC öffnen“.
3. Prüfung **beim Tipp**, nicht nur beim Zeichnen:
   - Plugin aktiv, `generation` gleich der des Bildes, Freigabe an, Host ∈ `links`;
   - erneutes `http::parse_url`, Schema https, keine Steuerzeichen;
   - danach `touch::open({{url, ""}})`, also nur ShellExecute ohne Protokollziel.
4. Ratenbegrenzung: `open_link` höchstens 1 je 2 s je Plugin.
5. Log mit festem Text: „Plugin-Link geöffnet (Plugin, Block)“ bzw. „… verworfen (Grund-Code)“. **Nie** URL oder Daten.

### Bindung an den angezeigten Inhalt
- Die Ziele entstehen **im selben Zeichenaufruf** wie die Pixel. Neue Variante:
  `plugins::DrawOut draw_page_out(const Page&, int w, int h, const schedule::Result*, bool sample)` → `{Image image; std::vector<PluginTarget> targets;}`.
  - `PluginTarget{rect_px, plugin, block, zone, action, url, generation}`.
  - Die Rechtecke kommen aus `zones::plan` (Block-Fläche) bzw. bei `list` aus einer neuen reinen Funktion `list_rows(area, density, k, has_label, count)`. Das Zeichnen von `ListBlock` (`plugin_render.cpp` 251–278) nutzt **dieselbe** Funktion, Geometrie und Treffer sind also identisch.
  - Umrechnung logisch → Pixel mit `scale_for`, dann Kachel-Offset (`draw()`, 640–666), dann Sperrzonen-Offset (4a).
- `touch::Tile` bekommt `std::vector<PluginTarget> plugin`. Neu ist `touch::plugin_at(tiles, x, y)`, das vor `action_at`/`at` geprüft wird.
- Veröffentlichung wie heute gemeinsam mit dem Bild unter `tap_mu_` (`app.cpp` 809–818) bzw. in `DeviceState` (842–844).
- Die URL wird **beim Zeichnen** aus genau dem gezeigten `Page` expandiert und nie beim Tipp neu ausgewertet.
- **Ruhezeit gegen Companion-Verzug** (kein APK-Update nötig): Jedes Ziel trägt `stable_since`, den Zeitpunkt, seit dem Rechteck + Aktion + URL unverändert veröffentlicht sind (Vergleich mit dem vorigen Satz derselben Ansicht).
  - Ein Ziel greift erst nach **1,0 s Ruhe** beim Companion und 0,3 s beim Monitor.
  - Während eines Übergangs der Zone ist es gesperrt.
  - Ein Tipp auf ein gesperrtes oder unruhiges Plugin-Ziel wird **verschluckt**: kein Ziel, kein Seitenwechsel.
  - Eine Bildkennung im Protokoll (Echo der Frame-Version) kommt erst mit Protokoll v2 und einem neuen APK, nicht in Phase 4.
- Tipps auf eine Plugin-Fläche **ohne** Ziel verhalten sich wie heute (Drittel-Regel).
- **Threads bei `next`:** `tap_at`/`device_tap`/`companion_tap` laufen im Tap- bzw. Fensterthread, `ViewState` gehört dem Anzeige-Thread. Deshalb schreibt `next` nie direkt in `ViewState::cycles`.
  - Das Muster ist `view.step` (`app.h` 96): `ViewState::next_requests` (`std::vector<std::string>` mit Schlüssel „plugin/zone“, geschützt durch einen neuen `std::mutex next_mu`, höchstens 8 Einträge) plus `mark_dirty()`.
  - `render_core` übernimmt die Liste unter `next_mu` und reicht sie an `advance(…, requests)` weiter. `advance` setzt das Ende des aktuellen Eintrags der Zone auf `now`.
  - Bei Geräten gilt der Schutz durch `dev_mu_` wie bei `view.step` (979).

### Reihenfolge
P4c-1 (Parser `links`/`tap`, Expansion in `render_page`) → P4c-2 (Ziele aus dem Zeichnen, `list_rows`) → P4c-3 (Freigabe im Store + UI) → P4c-4 (App-Treffer, Ruhezeit, Ausführung).

### Risiken
- Phishing über Daten: Der Host ist wörtlich und freigegeben, Platzhalter sind kodiert, Schema nur https.
- Datenabfluss in die Browser-Historie: keine geheimen Einstellungen, die Freigabe ist ausdrücklich.
- Falsches Ziel durch Verzug: Ruhezeit, Generation, Größenprüfung.
- Versehentliches Öffnen beim Wischen: Die Gestenerkennung bleibt unverändert, nur Tippen (`Gesture::tapped`, `monitor_display.cpp` 151–164) löst etwas aus.

### Tests
- `fl-plugin-tests` (Parser):
  - gültige `tap`/`links`;
  - Ablehnung von: Host nicht in `links`, Platzhalter im Host, `http://`, IP-Literal, geheime Einstellung, `item.*` außerhalb von `list`, `next` ohne `cycle`, `seconds` ohne `rotate`, mehr als 4 Hosts, die neuen Schlüssel in v1;
  - v1-Bestandstests unverändert.
- Expansion: Prozentkodierung (`/`, `?`, `#`, `%`, `&`, Unicode in Werten); fehlender Wert ⇒ kein Ziel.
- `fl-plugin-snapshot-tests`: Ziele von `draw_page_out` liegen exakt auf den gezeichneten Karten bzw. Listenzeilen (Pixelprüfung über die Testhooks `after_card`), Quer und Hoch, Skalierung < 1.
- **Touch-Snapshot**, rein in `fl-tests` über die neue freie Funktion `touch::plugin_decide(tiles, x, y, now, stable_min, consent, generation)` → {öffnen(url) | next(zone) | verschlucken | kein Plugin-Ziel}. `app.cpp` ruft nur sie auf, die Ausführung bekommt eine injizierte `launch`-Funktion:
  - Tipp auf Bild n trifft Ziel n, auch wenn Bild n+1 schon berechnet, aber nicht veröffentlicht ist.
  - Unruhiges Ziel wird verschluckt.
  - Ziel während eines Übergangs wird verschluckt.
  - Freigabe aus ⇒ nichts geöffnet, kein Seitenwechsel.
  - Generation geändert ⇒ verworfen.
  - Rate 1/2 s.
  - Injizierte `launch`-Funktion wie bei `touch::dispatch` (`touch.h` 36–37): Es werden nie echte Browser geöffnet.
- `fl-plugin-store-tests`: Freigabe persistiert. Ein Update mit anderen `links` setzt nur die Link-Freigabe zurück.
- **Mehrgeräte:**
  - Rein: getrennte Zielsätze A/B mit eigenem `stable_since`. Eine Entscheidung auf A mit den Kacheln von B ist nicht möglich, weil der Aufruf je `DeviceState` erfolgt; geprüft über zwei Sätze mit gleichen Koordinaten und anderen URLs.
  - Prozess: `app_companion_multi.py`. Ein `touch` auf ein Plugin-Ziel bei ausgeschalteter Freigabe ändert die Navigation nicht.

## Bewusst nicht in Phase 4
- Software-Drehung für Monitore, eigene Layouts je Ausrichtung (`layout_portrait`), Bildkennung im Companion-Protokoll, eigener Seriell-Thread.
- Tap-Aktionen außer `open_link`/`next`, Fokus-Navigation per D-Pad.
- Eingebaute Seiten (Phase 5).

## Aufgabenpakete für Codex

Allgemein für **jedes** Paket:
- Nur ein Paket gleichzeitig. Nur die genannten Dateien ändern.
- **Nicht committen, nicht pushen.**
- Clean-Room: nur dieser Entwurf, der Plan und ScreenBuddy-Code; kein libre-panel-Code.
- Zeile in `docs/CLEAN-ROOM-LAYOUT.md` §6 ergänzen (Phase, Datum, Agent, „ohne Einsicht in Fremdcode implementiert“).
- Build: `cpp\build.bat nopause`, oder schneller CMake-Build in `cpp\build\screenbuddy-release` plus die genannten Testziele. Neue Testziele werden in `cpp/CMakeLists.txt`, `cpp/build.bat` und `cpp/tests/run_release_checks.py` (Liste `suites`) eingetragen.
- Alle bestehenden Suiten bleiben grün (Parser 223, Speicher 127, Transport 134, Settings 135; Snapshot-Hashes unverändert, außer das Paket nennt neue).
- Feste Log-Texte, nie Daten/URLs/Geheimnisse. Kommentare deutsch, Stil wie die Umgebung.

**P4b-0 Messung USB-Übertragung** (vorgezogen, klein)
- Dateien: `cpp/src/output.h`, `cpp/src/serial_display.cpp`, `cpp/src/app.cpp` (`output()` 701–738), `cpp/tests/tests.cpp`. Test in `fl-tests`, das `flcore` linkt und `output.h` einbinden darf.
- Umsetzung:
  - `struct TransferStats { std::optional<double> bytes_per_second; double last_seconds = 0; }`;
  - `inline double ewma(std::optional<double> old, double sample, double alpha = 0.3)` in `output.h`;
  - `virtual TransferStats transfer() const { return {}; }`.
  - `changed_bands(const Image& old, const Image& next, int band)` → `std::vector<std::pair<int,int>>` als freie Funktion herauslösen (aus `show_locked` 234–252, Verhalten gleich).
  - `SerialDisplay` misst Bytes und Dauer je `show_locked` (nur Übertragungen ≥ 8 Zeilen zählen).
  - `output()` loggt LOGD „%d Zeilen, %.0f ms, %.0f kB/s“.
- Akzeptanz:
  - EWMA-Funktion rein und getestet (Folge 3 Werte ⇒ erwarteter Wert).
  - `changed_bands` getestet: keine Änderung ⇒ leer; eine Zeile ⇒ ein Band; zwei benachbarte Bänder ⇒ ein Bereich; letztes Band kürzer als 8.
  - Ohne Messung `nullopt`.
  - Kein Verhaltensunterschied in der Ausgabe.
  - `cpp/TESTPROTOKOLL.md` bekommt einen Abschnitt „Messung serielle Rate (offen, Basti)“.

**P4a-1 Reines Modul Geräteprofil**
- Dateien: neu `cpp/src/device_profile.h/.cpp` (in die `flcore`-Quellliste, `CMakeLists.txt` 28–45), neu `cpp/tests/device_profile_tests.cpp`, `cpp/CMakeLists.txt` (Ziel `fl-device-profile-tests` wie `fl-plugin-snapshot-tests` 113–118: nur die Testdatei, `flcore` PRIVATE; der `Image`-Konstruktor steht in `gfx.cpp`), `cpp/build.bat`, `cpp/tests/run_release_checks.py`.
- Umsetzung:
  - Schnittstelle wie in 4a plus `compose_view`, `shift_tiles` (auf `touch::Tile`, also `device_profile.cpp` bindet `touch.h` ein) und `resolve_tap`.
  - Drehtabelle als eine `constexpr`-Tabelle. `Orientation` kommt aus `output.h`.
- Akzeptanz: alle reinen 4a-Tests grün (inkl. „Mehrgeräte, rein“); `device_profile.h` bindet kein `windows.h` ein; Grenzen wie Entscheidung 5.

**P4a-2 Gerätemaske in der App**
- Dateien: `cpp/src/app.h` (`ViewState.area`, `profile_for`), `cpp/src/app.cpp` (66–68, 640–666 `draw`, 742–800 `render_core`, 822–880, 924–961), `cpp/src/config.cpp` (Standard `display.keepout`/`companion.keepout`, Prüfung 0..1000, ganzzahlig), `cpp/src/companion.h/.cpp` (`device_keepout`/`set_device_keepout` neben `device_pages`, 723–735), `cpp/src/output.h`/`network_display.cpp` (Durchreichen wie `device_pages`), `cpp/tests/app_companion_multi.py`, `cpp/tests/companion_tests.cpp` (Store-Feld).
- Umsetzung wie in 4a, „App-Anbindung“. Es gibt **keinen** App-Test-Hook und es wird keiner gebaut. `app.cpp` ruft nur `compose_view`/`shift_tiles`/`resolve_tap` aus P4a-1 auf. Die Integration prüft der Prozess-Test.
- Akzeptanz:
  - Ohne `keepout` liefert `compose_view` das Eingabebild unverändert (reiner Test). Die Snapshot-Hashes bleiben unverändert.
  - Der Prozess-Test „Mehrgeräte, Prozess“ aus 4a ist grün. Er wird mit `python cpp/tests/app_companion_multi.py <build>\ScreenBuddy.exe` gestartet, Aufruf wie im bestehenden Skript.
  - Ein Store-Test bestätigt, dass `device_keepout` die Daten erhält.
  - Die Drittel-Regel nutzt die nutzbare Breite.
  - `companion.json` alt bleibt ladbar (fehlendes Feld = 0).

**P4a-3 Einstellungen Sperrzone**
- Dateien: `cpp/src/ui_page_general.cpp` (Karte neben `orientation`, Z. 115: vier Zahlenfelder „Sperrzone oben/rechts/unten/links (Pixel, physischer Rand)“), `cpp/src/ui_page_companion.cpp` (je Gerät neben „Eigene Liste“, 108–110/279–330), `cpp/src/i18n.cpp` falls Texte dort.
- Akzeptanz:
  - DE/EN-Texte, Tastaturbedienung.
  - Ungültige Werte werden mit Meldung abgelehnt (`config::merge`).
  - Die Vorschau zeigt den Rand.
  - `docs/NAVIGATION.md`/`.en.md` sind ergänzt.

**P4a-4 Android-TV nur Querformat** (Companion-APK)
- Dateien: `android/app/src/main/java/de/fuchslabs/display/DisplayActivity.kt` (57–61: bei `isTv` immer `SCREEN_ORIENTATION_SENSOR_LANDSCAPE`), `MainActivity.kt` (95–104: Abschnitt Ausrichtung bei TV ausblenden), `android/protocol/...` nur wenn eine reine Hilfsfunktion `orientationFor(isTv, stored)` testbar ausgelagert wird (empfohlen, Test in `ProtocolTest.kt`).
- Akzeptanz:
  - Auf dem TV wird `Store.orientation` ignoriert, ohne den Wert zu löschen.
  - Das Handy verhält sich unverändert.
  - `ProtocolTest` ist grün, `Build-Companion.ps1` baut.
  - Versionsvorschlag Companion 1.0.4 (keine Signierung im Paket).

**P4b-1 Parser und Daten für Wechselfeld**
- Dateien: `cpp/src/plugin_zones.h` (`Zone::cycle` optional: `mode`, `transition`), `cpp/src/plugins.h` (`Widget::seconds`, `RenderedWidget::missing`), `cpp/src/plugins.cpp` (Zonen 296–312, Blöcke 430–474, `render_page_unguarded` 545–597: je Block ein lokales Fehlend-Kennzeichen, das zusätzlich in `page.missing_data` einfließt), `cpp/tests/plugins_tests.cpp`, `cpp/tests/plugin_zones_tests.cpp`.
- Regeln: `cycle` nur v2 + `layout`; `seconds` 3..600 nur in `rotate`-Zonen (Standard 8). In einer Wechselfeld-Zone plant `zones::plan` später nur den gewählten Block. Der Parser ändert dafür nichts an `max_blocks_per_zone`.
- Akzeptanz:
  - Parser-Tests aus 4b/4c, soweit `cycle`/`seconds`.
  - `missing` ist je Block korrekt (Wertetabelle Phase 3 erweitert).
  - v1 und v2 ohne `cycle` sind unverändert.

**P4b-2 Reines Modul Zeitplan**
- Dateien: neu `cpp/src/plugin_schedule.h/.cpp`, neu `cpp/tests/plugin_schedule_tests.cpp`, `cpp/CMakeLists.txt` (Ziel `fl-plugin-schedule-tests`), `cpp/build.bat`, `cpp/tests/run_release_checks.py`.
- Umsetzung: Schnittstelle aus 4b inkl.
  - `wake_at(now, deadlines, animating, meter)`;
  - `should_render(dirty, due, live, step, size_changed, minute_changed, now, next_deadline)`;
  - `budget_for(output_kind, transfer, render_ms_ewma, zone_rows, width, clients)`;
  - `serial_bytes`.
  Die Tabellen der Übergangsformen stehen fest in diesem Modul. Das Modul enthält keine Bilder (der Cache gehört der App bzw. `plugin_render`).
- Akzeptanz: alle Zeitplan-, Budget- und Frame-Skip-Tests aus 4b grün; keine Uhr- oder Grafikabhängigkeit.

**P4b-3 Zeichnen mit Auswahl und Übergang**
- Dateien: `cpp/src/plugin_render.h/.cpp` (`draw_zoned` 322–347: bei Wechselfeld-Zone nur den gewählten Block an `zones::plan` geben; neue Mischfunktionen `blend_rect`/`reveal_rect` auf Pixelrechteck), `cpp/src/render.h/.cpp` (`theme_revision()`), `cpp/tests/plugin_snapshot_tests.cpp`, `cpp/tests/plugin-snapshots/hashes.txt` (nur neue Einträge), `cpp/examples/plugins/` neu `offline-wechselfeld.flplugin.json`.
- Akzeptanz:
  - Die Snapshots aus 4b sind grün, alte Hashes unverändert.
  - Fehlerisolierung aus Phase 3 gilt auch für den gewählten Block.
  - Ein Übergangsschritt aus dem Cache bleibt im Frame-Budget (Messung im Test, nur Bericht, kein harter Fehlschlag in Debug).

**P4b-4 App-Zeitplanung**
- Dateien: `cpp/src/app.h` (`ViewState::cycles`, `fade_caches`, `next_deadline`, `render_ms`, `next_requests` + `next_mu`), `cpp/src/app.cpp` (515–553 `page_list`-Signatur mit `ViewState&`/`now`, 742–800, 802–861, 881–919), `cpp/src/plugin_schedule.h/.cpp` (nur `should_render`, falls in P4b-2 noch nicht vorhanden), `cpp/tests/app_companion_multi.py`.
- Umsetzung:
  - exakt nach „Mechanismus Zeitplanung“ 1–6, Budget nach Entscheidung 7 über `budget_for`;
  - Skip über `should_render`, Warten über `wake_at`;
  - Wechselfeld-Frames setzen nie `switched`.
- Akzeptanz:
  - Die reinen Tests in `fl-plugin-schedule-tests` sind grün: Ohne Wechselfeld bleiben Frame-Folge und Wartezeiten von `should_render`/`wake_at` über 60 simulierte s gleich der Referenzliste von heute. Leerlauf weckt höchstens 1/s.
  - Code-Review-Kriterium: Kein Wechselfeld-Pfad setzt `out.switched` (grep-bar, ein Kommentar an der Stelle).
  - Der Prozess-Test 4b „Mehrgeräte, Prozess“ ist grün.

**P4c-1 Parser und Expansion Tap/Links**
- Dateien: `cpp/src/plugins.h` (`Manifest::links`, `Widget::tap`, `RenderedWidget::tap_url`/`tap_action`, `item`-Zeilenziele), `cpp/src/plugins.cpp` (oberste Ebene 341–342, Blöcke 434–474, neue `Usage::link` in `validate_template`, Expansion mit Prozentkodierung in `render_block`, `store_problems` 660–678 dokumentiert die Versionsregel), `cpp/src/plugin_catalog*.cpp` nur falls der Validator neue Felder anzeigen muss, `cpp/tools/plugin_validate.cpp`, `cpp/tests/plugins_tests.cpp`.
- Akzeptanz: alle Parser- und Expansionstests aus 4c grün; `source_fingerprint` unverändert; neuer `link_fingerprint`.

**P4c-2 Ziele aus dem Zeichnen**
- Dateien: `cpp/src/plugin_render.h/.cpp` (`draw_page_out`, `list_rows` gemeinsam mit `ListBlock`), `cpp/src/touch.h/.cpp` (`PluginTarget`, `Tile::plugin`, `plugin_at`), `cpp/tests/plugin_snapshot_tests.cpp`.
- Akzeptanz: Die Zielrechtecke liegen pixelgenau auf den Karten/Zeilen (Quer, Hoch, Skalierung < 1). `draw_page` bleibt als dünner Wrapper und liefert bitgleiche Bilder.

**P4c-3 Freigabe „Links öffnen“**
- Dateien: `cpp/src/plugin_store.h/.cpp` (`links_allowed`, `set_link_consent`, Rücksetzen bei geändertem `link_fingerprint`), `cpp/src/app.h/.cpp` (`plugin_set_link_consent`, 196–202 analog), `cpp/src/ui_page_plugins.cpp` (Schalter + Hostliste), `cpp/src/native_settings.*`/`webui.cpp` nur, wenn die Plugin-Einstellungen dort gespiegelt werden, `cpp/tests/plugin_store_tests.cpp`.
- Akzeptanz: Store-Tests aus 4c; Standard aus; DE/EN-Texte; die Quellenfreigabe bleibt unberührt.

**P4c-4 Treffer, Ruhezeit, Ausführung**
- Dateien:
  - `cpp/src/app.h/.cpp`:
    - Der `page_list`-Plugin-Zweig ruft `draw_page_out`; die Ziele werden um Kachel- und Sperrzonen-Offset verschoben.
    - `stable_since` je Ansicht.
    - `tap_at`/`device_tap`/`companion_tap` rufen zuerst `touch::plugin_decide`.
    - Prüfkette und Rate wie in „Berechtigungsmodell“ 3–5.
    - `next` nur über `next_requests` (siehe „Threads bei `next`“).
  - `cpp/src/touch.h/.cpp` (`plugin_decide`), `cpp/tests/tests.cpp`, `cpp/tests/app_companion_multi.py`.
- Akzeptanz:
  - Die Touch-Snapshot- und Mehrgeräte-Tests aus 4c sind grün.
  - Keine echten Browserstarts in Tests (injizierte Startfunktion).
  - Bestehende Ziele (RSS, Freunde, HA, Spotify, System) verhalten sich unverändert.

**P4-Doku** (nach P4c-4, Haiku)
- Dateien: `docs/PLUGIN-SYSTEM.md`/`.en.md`, `docs/PLUGIN-PAKETFORMAT.md`/`PLUGIN-PACKAGE-FORMAT.md`, `docs/PLUGIN-TUTORIAL.md`/`.en.md`, `docs/COMPANION.md`/`.en.md` (Sperrzone, TV quer, Links öffnen am PC), `docs/NAVIGATION*.md`, `CHANGELOG.md`/`.en.md`, `cpp/res/GUIDE.html`, `cpp/res/ANLEITUNG.html`, `docs/CLEAN-ROOM-LAYOUT.md`.
- Akzeptanz: Schlüssel, Grenzen, Berechtigungen und Versionsregel sind beschrieben; die Beispiele bestehen den Validator.

## Hardware-Abnahme (Basti, vor Release)
- 4a-H1: Hochkant-Gehäuse in allen 4 Ausrichtungen, Rand liegt an der richtigen physischen Kante.
- 4a-H2: TV im Querformat fest, Overscan-Rand passt.
- 4b-H1: Serielle Rate R messen (P4b-0-Log) und in `TESTPROTOKOLL.md` eintragen. Wechselfeld seriell (Vorhang oder Schnitt), Monitor und Handy (Überblendung) ruhig, ohne Flackern oder Streifen.
- 4c-H1: Link-Tipp auf Monitor und Handy öffnet am PC nur nach Freigabe. Tipp direkt nach einem Wechsel bewirkt nichts.

## Phase-4b-Review: bekannte offene Punkte (06.11.2026)

Diese Punkte sind dokumentiert und in diesem Korrekturlauf bewusst unverändert:
- Zonengeometrie wird dreifach berechnet (`app.cpp`: `zone_rows`, sowie Geometrie-/Zeichenpfade in `plugin_render.cpp`). Eine gemeinsame Berechnung bleibt offen.
- Die Seitenwarnung „Plugin-Daten fehlen“ erscheint auch bei einem absichtlich leeren Eintrag.
- Ein `next`-Wunsch kann in einer wartenden Zone verloren gehen; in Phase 4c behandeln.
- Lock-Verschachtelung: `render::mutex` → `SerialDisplay::mu_`; separater Seriell-Thread bleibt außerhalb von Phase 4.
