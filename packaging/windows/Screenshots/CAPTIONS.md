# Store sample image captions / Bildunterschriften

| File | Caption |
|---|---|
| de-game-mode.png | Game-Mode einrichten und FPS-Berechtigungen der Store-Ausgabe prüfen. |
| de-editor.png | Eigene Spielansicht im nativen Editor – Vorschau mit Beispieldaten. |
| en-game-mode.png | Configure Game Mode and check FPS permissions in the Store edition. |
| en-editor.png | Custom gaming view in the native editor, with sample preview data. |

Neu erzeugt am 10. November 2026 aus ScreenBuddy 2.11.0: Der echte native Seiten-/Widgetrenderer (`Page`/`TreeCanvas`) rendert in einer isolierten, entwicklerregistrierten MSIX-Paketidentität. Die vier unveränderten PNG-Dateien sind je 1600 × 1200 Pixel groß. Die lokale Identität ist keine offizielle Store-Identität. Die Bilder zeigen native Seiteninhalte ohne Fensterrahmen; sie sind keine Desktop-Fensteraufnahmen.

Fresh renders on 10 November 2026 from ScreenBuddy 2.11.0. The real native page/widget renderer (`Page`/`TreeCanvas`) runs in an isolated developer-registered MSIX package context. Four unedited PNGs, each 1600 × 1200 pixels. The local identity is not an official Store identity. Images show native page contents without window chrome; they are not desktop window captures.

Alle Geräte, Messwerte und Spielinformationen im Editor sind Beispieldaten. Die FPS-Berechtigung wird für das Beispiel fest auf „inaktiv“ gesetzt; Quellen und Integrationen sind deaktiviert. Keine echten Konten, Zugangsdaten oder privaten Pfade und keine Hardware-/Live-Spielabnahme. Alle vier Bilder wurden visuell auf Lesbarkeit, DE/EN-Texte und fehlende private Daten geprüft. Die früheren 2.9.6-Dateien wurden ersetzt, nicht umbenannt oder neu beschriftet.

All devices, metrics and game information in the editor are synthetic samples. FPS permission is fixed to inactive for the sample; sources and integrations are disabled. No real accounts, credentials or private paths, and no hardware/live-game acceptance evidence. All four images were visually checked for readability, DE/EN text and absence of private data. Earlier 2.9.6 images were replaced, not relabeled.

Reproduction: build `fl-layout-ui-tests`, then run `fl-layout-ui-tests.exe --export-store-listing <fresh-directory>` using `Invoke-CommandInDesktopPackage` in an isolated developer-registered test package. The exporter writes `capture-report.json` with version, package context and native help-control verification. It never records or updates snapshot goldens. Roboto 3.016 is the existing snapshot-test font prerequisite. `manifest.json` records exact PNG and capture-binary SHA-256 values. Microsoft certification and final Partner Center listing review remain separate.
