# Built-in-Snapshots (P5-0)

Rechnergebundene, bitgleiche DirectWrite-Baseline der eingebauten Seiten. HA und Willkommen sind nur Kontrollbilder; es findet noch keine Seitenmigration statt.

Das Ziel `fl-builtin-snapshot-tests` prüft zuerst die installierten DirectWrite-Familien **Roboto** und **Segoe UI Variable**. Ohne diese Schriften wird vor dem Zeichnen, Schreiben oder Vergleichen abgebrochen. **Auf diesem Sandbox-Rechner fehlt Roboto: `hashes.txt` und Referenz-PNGs konnten deshalb noch nicht erzeugt werden. P5-0 ist noch nicht abgenommen.** Keine Ersatzschrift-Baseline verwenden.

Nach Installation der Schriften auf dem vorgesehenen Referenzrechner, aus `cpp/`:

```powershell
cmake --build build/store --target fl-builtin-snapshot-tests
& build/store/fl-builtin-snapshot-tests.exe update
& build/store/fl-builtin-snapshot-tests.exe
$env:TZ = 'NZST-12NZDT'
& build/store/fl-builtin-snapshot-tests.exe
& build/store/fl-builtin-snapshot-tests.exe export build/builtin-export
```

`pruefen` (auch `prüfen`) ist der Standard. `update` schreibt `hashes.txt` und nur die DE/dunkel-Referenz-PNGs; `export <Ordner>` schreibt alle Bilder und dieselbe Hashkarte, ohne die Baseline zu verändern. Vor/nach einer Migration auf demselben Rechner exportieren und die Hashdateien byteweise vergleichen. Baseline-Updates getrennt von Migrationen reviewen/committen.

Matrix: **63 Zustände × (7 Designs × 9 Größen DE + 2 Designs × 2 Größen EN) = 4.221 Bilder und 4.221 Trefferkarten**, zusammen 8.442 Hashzeilen. DE-Referenz: 567 PNGs. Designs wie im Plugin-Test; EN nur dunkel/custom-material bei 480×320 und 320×480. Größen außerdem 240×160, 160×240, 960×640, erste Kachel aus `tile_layout(1920,720)` (958×720), 480×300 als Körper nach 20 px Sperrzone, 1080×2400 und 2400×1080.

Zustände und Eingaben stehen vollständig in `../builtin_snapshot_tests.cpp`, gemeinsame eingefrorene Daten in `../../src/page_samples.cpp`. Listen-Seite 2 bekommt ausreichend Daten, künstliche Cover/Avatare/Rangicons und Anbieterlogos benötigen kein Netzwerk. System enthält insbesondere nullopt, 75/85 °C, ≥90 % und 0/1 Verlaufspunkte. Spotify umfasst alle Statusvarianten, drei Pegelfarben, simuliert, ohne Steuerung, Cover, zuletzt gespielt und Fehlertexte. Freunde und Discord werden getrennt geprüft.

Zeit: Test setzt selbst `TZ=UTC0` mit `_tzset()`, Wanduhr 1792606500, Spotify `now=fetched_at=1000`. Theme immer vor Konstruktion des Zeichners. Kein Netzwerk, Port, Audio oder Registryzugriff.

Hashformat wie Plugin-Test: FNV-1a 64 über little-endian 32-Bit-Werte (Breite, Höhe, ARGB-Pixel). Schlüssel `<Sprache>.<Design>.<Größe>.<Seite>.<Zustand>`, Trefferkarten zusätzlich `.touch`. Für die Karte werden die unveränderten statischen `row_at`, `zone_at`, `button_at`, `cell_at` auf dem 4-px-Raster aufgerufen (Geometrie, keine App-Aktionsfreigabe). Ihr Bild hat `ceil(w/4)` Spalten und `ceil(h/4)+1` Zeilen; Metadatenzeile zunächst null, Pixel 0/1 enthalten `rows_per_page()`/`pages(n)`, danach Rasterwerte als uint32_t (−1 = 0xffffffff). Willkommen hat nur −1. Auch HA/Willkommen bekommen diese Metadaten.

Abweichungen: tatsächliche PNGs und Bild-/Trefferkarten-Hashes unter `%TEMP%/fl-builtin-snapshot-actual`; fehlende und veraltete Hashschlüssel sind Fehler. Keine Toleranz. Plugin-Baseline bleibt unverändert. Aktueller Prüfstand: [Testprotokoll](../../TESTPROTOKOLL.md).
