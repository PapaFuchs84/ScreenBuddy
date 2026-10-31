# Themes / Farbdesigns

**Deutsch** · [English below](#english)

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
Streng geprüft: genau diese Schlüssel, Version 1, alle neun Farben als `#RRGGBB`, Name 1–40 Zeichen ohne `< > " \ / & ``, `base` hell/dunkel. Ungültige Dateien werden ignoriert und angezeigt. Der Editor bietet Hex-Eingabe, Live-Vorschau, Kontrastwarnung, „Kontrast korrigieren“, Duplizieren, Löschen, Import und Export. Die alte Einstellung „Eigene Farben“ wird beim ersten Start in das Theme „Eigene Farben“ umgezogen. Beispiele (Stil-Anlehnung, ohne fremde Logos): `cpp/examples/themes`.

**Noch nicht enthalten:** Teilen über den Plugin-Store (Typ „theme“), Maus-Tooltip der Statuspunkte.

## English
Settings › **Colors & design**: Light (FuchsLabs), Dark (FuchsLabs), **Match Windows** or your own themes. The choice applies to the USB display, monitor and Companion (the image is rendered on the PC). The selection is kept with "Save & apply"; theme files are saved immediately.

**Match Windows**: light/dark from Windows "app mode", accent from the Windows accent colour; only the accent is replaced, header and fox logo stay fox orange. Changes in Windows apply live. Contrast is corrected automatically (text 4.5:1, bars 3:1). In Windows high-contrast mode the display uses the system colours. Read from the registry (AppsUseLightTheme, DWM AccentColor), not UISettings.

**Custom themes**: files in `%LOCALAPPDATA%\FuchsLabs\USB-Display\themes\<id>.json`, same JSON format as above (version 1, nine `#RRGGBB` colours, name 1–40 characters), strictly validated; invalid files are ignored and listed. The editor offers hex input, live preview, contrast warning, "Fix contrast", duplicate, delete, import and export. The old "Custom colors" migrate on first start. Example themes (style-inspired, no third-party logos): `cpp/examples/themes`.

**Not included yet**: sharing via the plugin store (type "theme"), mouse tooltip for status dots.
