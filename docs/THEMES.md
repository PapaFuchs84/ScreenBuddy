# Themes / Farbdesigns

**Deutsch** · [English below](#english)

## Designsysteme (in Entwicklung)

Die drei Stile verändern zusätzlich zur Palette die Gestaltung des Displays und des nativen Einstellungsfensters. Cupertino nutzt gruppierte Flächen, ruhige Kopfzeilen, größere Zeilen und zurückhaltende Druckrückmeldung. Fluent nutzt kompakte Radien, Segoe UI Variable (Fallback Segoe UI), einen vertikalen Auswahlindikator und Konturfeedback. Material nutzt größere Kartenradien, Pillenknöpfe, tonale Zustandsflächen und einen je Zustand unterschiedlich großen Schaltergriff. Zeilenhöhen und Touch-Zuordnung verwenden dieselbe Geometrie. Navigation, Einstellungen, Speichern und Sicherheitsregeln bleiben ScreenBuddy-Funktionen.

Im Theme-Editor wählt **Designsystem** das Profil. Displayvorschau wirkt sofort; native Einstellungen folgen nach **Speichern & anwenden**. Optionaler JSON-Schlüssel `style`: `fuchslabs`, `cupertino`, `fluent` oder `material`. Andere Werte/Typen sind ungültig. Ohne Schlüssel bleibt das bisherige FuchsLabs-Layout; nur die exakten bisherigen Startnamen `Cupertino-Stil`, `Fluent-Stil`, `Material-Stil` erhalten beim Laden das passende Profil. Ein explizites `fuchslabs` verhindert diese Zuordnung. Duplizieren und Export erhalten das Profil.

Dies sind eigenständige, an offiziellen Designsystemen orientierte Windows-Implementierungen, keine Original-UI-Frameworks und keine vollständigen OS-Kopien. Keine fremden Logos oder mitgelieferten Apple-/Google-Schriften; Roboto wird nur verwendet, falls lokal installiert, sonst Segoe UI. Kein echtes Mica/Glass, keine Plattformanimationen, kein Android-Companion-Umbau: dort erscheint das vom PC gerenderte Bild. Bei Darstellung „Windows“ werden Systemfarben weiterhin verwendet. Alte Apps können Dateien mit explizitem `style` ablehnen; die bisherigen Store-Seed-Dateien bleiben daher unverändert. Kein Katalog wurde veröffentlicht oder signiert.

Quellen: [Apple HIG Foundations](https://developer.apple.com/design/human-interface-guidelines/foundations), [Fluent design tokens](https://fluent2.microsoft.design/design-tokens), [Fluent shapes](https://fluent2.microsoft.design/shapes/), [Material typography](https://m3.material.io/styles/typography/applying-type).

## Auswahl
Einstellungen › **Farben & Design**: Hell (FuchsLabs), Dunkel (FuchsLabs), **An Windows anpassen** oder eigene Themes. Die Wahl gilt für USB-Display, Bildschirm und Companion (das Bild entsteht am PC). Die Auswahl wird mit „Speichern & anwenden“ übernommen; Theme-Dateien werden sofort gespeichert.

## An Windows anpassen
- Hell/Dunkel nach „Apps: hell/dunkel“ in Windows, Akzentfarbe aus der Windows-Akzentfarbe. Nur der Akzent wird ersetzt; Kopfzeile und Fuchs-Logo bleiben Fuchs-Orange.
- Änderungen in Windows (WM_SETTINGCHANGE „ImmersiveColorSet“) wirken sofort.
- Kontrast wird automatisch korrigiert: Text und Nebentext mindestens 4,5:1, Akzent auf Hintergrund 4,5:1 und auf Karten 3:1, Balken 3:1, Statusfarben 3:1 auf Karten.
- **Kontrastdesign von Windows**: Das Display nutzt die Systemfarben (Hintergrund, Text, Hervorhebung); Kopfzeile und Balken verwenden die Hervorhebungsfarbe.
- Nicht über UISettings, sondern über Registry (AppsUseLightTheme, DWM AccentColor) gelesen.

## Eigene Themes
Dateien: `%LOCALAPPDATA%\FuchsLabs\USB-Display\themes\<id>.json` (max. 8 KB, max. 64 Themes).
```json
{ "version": 1, "name": "Mein Design", "base": "hell", "dark": false,
  "colors": { "bg": "#F2F2F7", "card": "#FFFFFF", "line": "#D1D1D6", "header": "#007AFF",
              "accent": "#0056B3", "text": "#1C1C1E", "muted": "#636366",
              "success": "#248A3D", "danger": "#D70015" } }
```
Streng geprüft: diese Schlüssel plus optional `style`, Version 1, alle neun Farben als `#RRGGBB`, Name 1–40 Zeichen ohne `< > " \ / & ``, `base` hell/dunkel. Ungültige Dateien werden ignoriert und angezeigt. Der Editor bietet Hex-Eingabe, Live-Vorschau, Kontrastwarnung, „Kontrast korrigieren“, Duplizieren, Löschen, Import und Export. Die alte Einstellung „Eigene Farben“ wird beim ersten Start in das Theme „Eigene Farben“ umgezogen. Beispiele (Stil-Anlehnung, ohne fremde Logos): `cpp/examples/themes`.

## Themes aus dem Store
Einstellungen › **Plugin-Store** › Karte „Themes“: signierte Farbdesigns suchen und mit einem Klick installieren. Das Format ist dasselbe JSON wie bei eigenen Themes (neun Farben und optionales festes Designprofil, kein Code); der Store liefert sie in einer eigenen, signierten Liste `themes.json` (ältere App-Versionen lesen sie nicht und bleiben unberührt). Vor der Installation werden Signatur, Größe, SHA-256 und das strenge Theme-Schema geprüft, gesperrte Themes (Sperrliste) erscheinen nicht. Ein Theme mit gleichem Namen wird nie überschrieben (Schutz vor Verlust eigener Änderungen); zum Aktualisieren erst unter „Farben & Design“ löschen.

**Für Autoren (Store-Repo):** `themes/<id>/<id>.fltheme.json` (das Theme) und `themes/<id>/<id>.fltheme.meta.json` (`description`, `version` x.y.z, `author`, `license` MIT oder CC0-1.0). Wartung: `fl-plugin-sign build-themes <Ordner> <out> <Folge>`, `sign themes`, `check-themes` (CI). Beispiele: `store-seed/themes`.

## Statuspunkte
Seitenleiste: Maus kurz über dem Punkt ruhen lassen (0,5 s) zeigt „Eingerichtet und eingeschaltet“ (grün) bzw. „Nicht eingerichtet oder ausgeschaltet“ (grau).

## Design systems (in development)

Profiles extend colors to display and native settings geometry, typography, spacing and interaction feedback. Cupertino uses grouped surfaces, quiet headers and spacious rows; Fluent uses compact geometry, a selection indicator and contour feedback; Material uses larger cards, pill buttons, tonal state layers and state-dependent switch thumbs. Hit targets share the rendering row geometry. Select **Design system** in the editor; apply and restart to update native settings.

Optional JSON `style` accepts only `fuchslabs`, `cupertino`, `fluent`, `material`. Existing files retain FuchsLabs geometry except the three exact starter names, recognized on load; explicit `fuchslabs` overrides recognition. Duplicate/export preserve profiles. Old clients may reject an explicit style, so published-compatible seed files remain unchanged. No store catalog was published.

These are Windows interpretations of the official systems linked above, not original frameworks or full OS replicas. No third-party logos or redistributed fonts; unavailable fonts fall back to Segoe UI. No real Glass/Mica or platform motion, no Companion UI redesign. The Windows appearance option continues to use system colors. Validation status is recorded in TESTPROTOKOLL.md.

## English
Settings › **Colors & design**: Light (FuchsLabs), Dark (FuchsLabs), **Match Windows** or your own themes. The choice applies to the USB display, monitor and Companion (the image is rendered on the PC). The selection is kept with "Save & apply"; theme files are saved immediately.

**Match Windows**: light/dark from Windows "app mode", accent from the Windows accent colour; only the accent is replaced, header and fox logo stay fox orange. Changes in Windows apply live. Contrast is corrected automatically (text 4.5:1, bars 3:1). In Windows high-contrast mode the display uses the system colours. Read from the registry (AppsUseLightTheme, DWM AccentColor), not UISettings.

**Custom themes**: files in `%LOCALAPPDATA%\FuchsLabs\USB-Display\themes\<id>.json`, same JSON format as above (version 1, nine `#RRGGBB` colours, name 1–40 characters), strictly validated; invalid files are ignored and listed. The editor offers hex input, live preview, contrast warning, "Fix contrast", duplicate, delete, import and export. The old "Custom colors" migrate on first start. Example themes (style-inspired, no third-party logos): `cpp/examples/themes`.

**Themes from the store**: Settings › **Plugin store** › "Themes" card: browse signed color designs and install with one click. Same JSON as custom themes (nine colors and an optional fixed design profile, no code), delivered in a separate signed list `themes.json` (older app versions ignore it). Signature, size, SHA-256 and the strict theme schema are checked before installation; revoked themes are hidden. A theme with the same name is never overwritten; delete it under "Colors & design" first to update.

**Authors (store repo)**: `themes/<id>/<id>.fltheme.json` plus `<id>.fltheme.meta.json` (`description`, `version`, `author`, `license` MIT or CC0-1.0). Maintenance: `fl-plugin-sign build-themes|sign themes|check-themes`. Examples: `store-seed/themes`.

**Status dots**: hover the dot in the sidebar for half a second to see "Set up and switched on" (green) or "Not set up or switched off" (grey).

## Hell und Dunkel / Light and dark

Alle drei Profile bieten getrennte helle und dunkle Referenzpaletten. Die sechs Beispiele unter `cpp/examples/themes` kombinieren denselben Aufbau mit passenden Flächen, Text-, Akzent- und Statusfarben. Der Hell/Dunkel-Schalter des nativen Fensters wechselt auch bei einem aktiven Designprofil die Palette; das Profil bleibt erhalten. Unter **Basis** im Theme-Editor lädt ein Profilwechsel die Referenzpalette und ersetzt die neun Farbwerte, danach sind eigene Anpassungen möglich. Theme-Dateien werden erst mit **Theme speichern** gesichert.

All three profiles provide separate light and dark reference palettes. The six examples in `cpp/examples/themes` share the profile structure while adapting surfaces, text, accent and status colors. The native window's light/dark switch also switches an active profile's palette. Changing a design profile's base in the editor loads the reference palette and replaces all nine color values; you can customize them afterwards. Files are only saved with **Save theme**.

Quellen / sources: [Fluent color roles and interaction states](https://fluent2.microsoft.design/color), [Apple Dark Mode](https://developer.apple.com/design/human-interface-guidelines/dark-mode), [Material color roles](https://m3.material.io/styles/color/roles).
