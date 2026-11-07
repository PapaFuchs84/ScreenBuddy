# Built-in-Snapshots (P5-0)

Rechnergebundene, bitgleiche DirectWrite-Baseline der eingebauten Seiten. HA und Willkommen sind nur Kontrollbilder; es findet noch keine Seitenmigration statt.

Das Ziel `fl-builtin-snapshot-tests` prüft zuerst die installierten DirectWrite-Familien **Roboto** und **Segoe UI Variable**. Ohne diese Schriften wird vor dem Zeichnen, Schreiben oder Vergleichen abgebrochen. Die Baseline wurde nach Benutzerinstallation von Roboto auf dem Referenzrechner erzeugt: `hashes.txt` enthält 8.442 Hashzeilen, dazu liegen 567 DE/dunkel-PNGs vor. **Rechnergebunden, noch nicht abgenommen:** Ein anschließender Vergleich mit äußerem `TZ=NZST-12NZDT` meldete 33 abweichende Bilder. In der aktuellen Sandbox ist die Benutzerschrift für DirectWrite unsichtbar; der Fontordner ist nicht lesbar. Reguläre Wiederholungen brechen deshalb vor dem Rendern ab. Keine Ersatzschrift-Baseline verwenden. Windows-/Schriftversionen und Schriftzugriff müssen für Update, Vergleich und Export übereinstimmen. Der Hinweis zum Segoe-UI-Variable-Fallback bleibt bestehen.

Nach Sichtbarkeit der Schriften im ausführenden Prozess auf dem vorgesehenen Referenzrechner, aus `cpp/` (CMake in einer mit `vcvars64.bat` vorbereiteten Shell; dieser Ablauf ist wegen der noch offenen Ursache erneut erforderlich):

```powershell
cmake --build build/store --target fl-builtin-snapshot-tests
$savedTZ = $env:TZ
try {
    $env:TZ = 'UTC0'
    & build/store/fl-builtin-snapshot-tests.exe update
    if ($LASTEXITCODE -ne 0) { throw 'Update fehlgeschlagen' }
    foreach ($tz in @('UTC0', 'NZST-12NZDT', $null)) {
        if ($null -eq $tz) { Remove-Item Env:TZ -ErrorAction SilentlyContinue }
        else { $env:TZ = $tz }
        & build/store/fl-builtin-snapshot-tests.exe
        if ($LASTEXITCODE -ne 0) { throw "Vergleich fehlgeschlagen: TZ=$tz" }
    }
    $baselineHash = (Get-FileHash tests/builtin-snapshots/hashes.txt).Hash
    foreach ($tz in @('UTC0', 'NZST-12NZDT')) {
        $env:TZ = $tz
        $dest = "build/store/builtin-export-$tz"
        & build/store/fl-builtin-snapshot-tests.exe export $dest
        if ($LASTEXITCODE -ne 0) { throw "Export fehlgeschlagen: TZ=$tz" }
        if ((Get-FileHash "$dest/hashes.txt").Hash -ne $baselineHash) {
            throw "Export-Hashkarte abweichend: TZ=$tz"
        }
    }
} finally {
    if ($null -eq $savedTZ) { Remove-Item Env:TZ -ErrorAction SilentlyContinue }
    else { $env:TZ = $savedTZ }
}
```

`pruefen` (auch `prüfen`) ist der Standard. `update` schreibt `hashes.txt` und nur die DE/dunkel-Referenz-PNGs; `export <Ordner>` schreibt alle Bilder und dieselbe Hashkarte, ohne die Baseline zu verändern. Vor/nach einer Migration auf demselben Rechner exportieren und die Hashdateien byteweise vergleichen. Baseline-Updates getrennt von Migrationen reviewen/committen.

Matrix: **63 Zustände × (7 Designs × 9 Größen DE + 2 Designs × 2 Größen EN) = 4.221 Bilder und 4.221 Trefferkarten**, zusammen 8.442 Hashzeilen. DE-Referenz: 567 PNGs. Designs wie im Plugin-Test; EN nur dunkel/custom-material bei 480×320 und 320×480. Größen außerdem 240×160, 160×240, 960×640, erste Kachel aus `tile_layout(1920,720)` (958×720), 480×300 als Körper nach 20 px Sperrzone, 1080×2400 und 2400×1080.

Zustände und Eingaben stehen vollständig in `../builtin_snapshot_tests.cpp`, gemeinsame eingefrorene Daten in `../../src/page_samples.cpp`. Listen-Seite 2 bekommt ausreichend Daten, künstliche Cover/Avatare/Rangicons und Anbieterlogos benötigen kein Netzwerk. System enthält insbesondere nullopt, 75/85 °C, ≥90 % und 0/1 Verlaufspunkte. Spotify umfasst alle Statusvarianten, drei Pegelfarben, simuliert, ohne Steuerung, Cover, zuletzt gespielt und Fehlertexte. Freunde und Discord werden getrennt geprüft.

Zeit: Test setzt selbst `TZ=UTC0` mit `_tzset()`, Wanduhr 1792606500, Spotify `now=fetched_at=1000`. Theme immer vor Konstruktion des Zeichners. Kein Netzwerk, Port, Audio oder Registryzugriff.

Hashformat wie Plugin-Test: FNV-1a 64 über little-endian 32-Bit-Werte (Breite, Höhe, ARGB-Pixel). Schlüssel `<Sprache>.<Design>.<Größe>.<Seite>.<Zustand>`, Trefferkarten zusätzlich `.touch`. Für die Karte werden die unveränderten statischen `row_at`, `zone_at`, `button_at`, `cell_at` auf dem 4-px-Raster aufgerufen (Geometrie, keine App-Aktionsfreigabe). Ihr Bild hat `ceil(w/4)` Spalten und `ceil(h/4)+1` Zeilen; Metadatenzeile zunächst null, Pixel 0/1 enthalten `rows_per_page()`/`pages(n)`, danach Rasterwerte als uint32_t (−1 = 0xffffffff). Willkommen hat nur −1. Auch HA/Willkommen bekommen diese Metadaten.

Abweichungen: tatsächliche PNGs und Bild-/Trefferkarten-Hashes unter `%TEMP%/fl-builtin-snapshot-actual`; fehlende und veraltete Hashschlüssel sind Fehler. Keine Toleranz. Plugin-Baseline bleibt unverändert. Aktueller Prüfstand: [Testprotokoll](../../TESTPROTOKOLL.md).

**Hinweis (2026-11-06):** Das Schreiben von PNGs (WIC) zwischen Zeichnungen veraendert die spaetere Textdarstellung einzelner Seiten (System, Streaming). Der Test hasht/vergleicht deshalb in einem ersten Durchlauf ohne PNG-Ausgabe und schreibt PNGs (update/export) erst in einem zweiten Durchlauf. Mit Roboto (Benutzerschrift) auf dem Referenzrechner: update, dann pruefen mit TZ=UTC0, NZST-12NZDT und ohne TZ jeweils 0 Fehler. Fehlendes 'Segoe UI Variable' wird per Rechner-Fallback der App aufgeloest (Baseline rechnergebunden).
