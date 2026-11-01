# Themes / Farbdesigns

[Menü und Themes / Menu and themes](NAVIGATION.md) · [English](NAVIGATION.en.md)

**Deutsch** · [English below](#english)

## Designsysteme (ab 2.9.2)

Die Profile haben unterschiedliche Fensteraufbauten und Bedienwege, jeweils in Hell und Dunkel:

- **Cupertino:** einblendbare Sidebar, schwebende Werkzeugleiste, optionaler Geräte-Inspector und gruppierte Übersicht. Auf dem Systemdisplay stehen CPU, GPU, RAM, Datenträger und Netzwerk in einer gemeinsamen Liste mit Trennlinien.
- **Fluent:** Seitensuche mit echter Ergebnisnavigation, Zurück-Verlauf, Breadcrumb, kompakte Statuskacheln und Auswahlindikator. Auf dem Display bleibt ein dichtes Dashboard mit zwei Spalten.
- **Material:** fünf Hauptziele in einer Symbolleiste, kontextbezogene Unterseiten, große Überschrift, hervorgehobene Kennzahl und tonale Flächen. Bei engem Platz werden Unterseiten als Auswahl gezeigt. Speichern/Verwerfen erscheinen als schwebende Aktionsgruppe nur bei Änderungen. Auf dem Systemdisplay steht eine CPU-Hauptkarte über GPU- und kompakten Statuskarten.

Zeilenhöhen und System-Touchziele verwenden dieselbe Geometrie wie das Rendering. Einstellungen und Sicherheitsregeln bleiben ScreenBuddy-Funktionen.

Im Theme-Editor wählt **Designsystem** das Profil. Displayvorschau wirkt sofort; native Einstellungen folgen nach **Speichern & anwenden**. Optionaler JSON-Schlüssel `style`: `fuchslabs`, `cupertino`, `fluent` oder `material`. Andere Werte/Typen sind ungültig. Ohne Schlüssel bleibt das bisherige FuchsLabs-Layout; nur die exakten bisherigen Startnamen `Cupertino-Stil`, `Fluent-Stil`, `Material-Stil` erhalten beim Laden das passende Profil. Ein explizites `fuchslabs` verhindert diese Zuordnung. Duplizieren und Export erhalten das Profil.

Dies sind eigenständige, an offiziellen Designsystemen orientierte Windows-Implementierungen, keine Original-UI-Frameworks und keine vollständigen OS-Kopien. Keine fremden Logos oder mitgelieferten Apple-/Google-Schriften; Roboto wird nur verwendet, falls lokal installiert, sonst Segoe UI. Fluent fordert auf unterstützten Windows-Versionen einen echten DWM-Mica-Fenstertitel an; die Inhaltsflächen bleiben deckend. Cupertino verwendet eine eigene ruhige Werkzeugleistenfläche, kein echtes Liquid Glass. Keine Plattformanimationen und kein Android-Companion-Umbau: dort erscheint das vom PC gerenderte Bild. Bei Darstellung „Windows“ werden Systemfarben weiterhin verwendet. Alte Apps können Dateien mit explizitem `style` ablehnen; die bisherigen Store-Seed-Dateien bleiben daher unverändert. Kein Katalog wurde veröffentlicht oder signiert.

Layout-Referenzen: [Apple sidebars](https://developer.apple.com/design/human-interface-guidelines/sidebars), [Apple materials](https://developer.apple.com/design/human-interface-guidelines/materials), [Windows NavigationView](https://learn.microsoft.com/en-us/windows/apps/develop/ui/controls/navigationview), [Windows Mica](https://learn.microsoft.com/en-us/windows/apps/design/style/mica), [Material 3](https://developer.android.com/develop/ui/compose/designsystems/material3).

Quellen: [Apple HIG Foundations](https://developer.apple.com/design/human-interface-guidelines/foundations), [Fluent design tokens](https://fluent2.microsoft.design/design-tokens), [Fluent shapes](https://fluent2.microsoft.design/shapes/), [Material typography](https://m3.material.io/styles/typography/applying-type).

## Auswahl
Einstellungen › **Farben & Design**: Hell (FuchsLabs), Dunkel (FuchsLabs), **An Windows anpassen** oder eigene Themes. Die Wahl gilt für USB-Display, Bildschirm und Companion (das Bild entsteht am PC). Die Auswahl wird mit „Speichern & anwenden“ übernommen; Theme-Dateien werden sofort gespeichert.

## An Windows anpassen
- Hell/Dunkel nach „Apps: hell/dunkel“ in Windows, Akzentfarbe aus der Windows-Akzentfarbe. Nur der Akzent wird ersetzt; Kopfzeile und Fuchs-Logo bleiben Fuchs-Orange (Profil FuchsLabs). Mit Profil Cupertino, Fluent oder Material kommen die übrigen Farben aus der Profilpalette (hell/dunkel nach Windows), nicht aus der FuchsLabs-Palette.
- Änderungen in Windows (WM_SETTINGCHANGE „ImmersiveColorSet“) wirken sofort.
- Kontrast wird automatisch korrigiert: Text und Nebentext mindestens 4,5:1, Akzent auf Hintergrund 4,5:1 und auf Karten 3:1, Balken 3:1, Statusfarben 3:1 auf Karten.
- **Kontrastdesign von Windows**: Das Display nutzt die Systemfarben (Hintergrund, Text, Hervorhebung); Kopfzeile und Balken verwenden die Hervorhebungsfarbe.
- Nicht über UISettings, sondern über Registry (AppsUseLightTheme, DWM AccentColor) gelesen.

## Eigene Themes
Dateien: `%LOCALAPPDATA%\FuchsLabs\USB-Display\themes\<id>.json` (max. 8 KB, max. 64 Themes).
```json
{ "version": 1, "name": "Mein Design", "base": "hell", "dark": false,
  "colors": { "bg": "#F2F2F7", "card": "#FFFFFF", "line": "#8A8A8D", "header": "#0066D6",
              "accent": "#0056B3", "text": "#1C1C1E", "muted": "#606063",
              "success": "#207C37", "danger": "#D70015" } }
```
Streng geprüft: diese Schlüssel plus optional `style`, Version 1, alle neun Farben als `#RRGGBB`, Name 1–40 Zeichen ohne `< > " \ / & ``, `base` hell/dunkel. Ungültige Dateien werden ignoriert und angezeigt. Version 2.9.1 lehnt Themes mit dem Schlüssel `style` als „unbekannter Schlüssel“ ab. Der Editor bietet Hex-Eingabe, Live-Vorschau, Kontrastwarnung, „Kontrast korrigieren“, Duplizieren, Löschen, Import und Export. Die alte Einstellung „Eigene Farben“ wird beim ersten Start in das Theme „Eigene Farben“ umgezogen. Beispiele (Stil-Anlehnung, ohne fremde Logos): `cpp/examples/themes`. Die hellen Beispiele (Cupertino, Fluent, Material hell) halten Text, Nebentext, Akzent, Kopfzeilenschrift, Erfolg und Fehler mit mindestens 4,5:1 sowie Trennlinien und Balken mit mindestens 3:1 gegen Hintergrund und Karte ein (seit 2.9.2).

## Themes aus dem Store
Einstellungen › **Farben & Design** › Karte „Themes aus dem Store“: signierte Farbdesigns mit einem Klick installieren; das neue Theme ist danach ausgewählt (übernehmen mit „Speichern & anwenden“). Der Abruf folgt dem Schalter „Plugin-Store verwenden“. Das Format ist dasselbe JSON wie bei eigenen Themes (neun Farben und optionales festes Designprofil, kein Code); der Store liefert sie in einer eigenen, signierten Liste `themes.json` (ältere App-Versionen lesen sie nicht und bleiben unberührt). Vor der Installation werden Signatur, Größe, SHA-256 und das strenge Theme-Schema geprüft, gesperrte Themes (Sperrliste) erscheinen nicht. Ein Theme mit gleichem Namen wird nie überschrieben (Schutz vor Verlust eigener Änderungen); zum Aktualisieren erst unter „Farben & Design“ löschen.

**Für Autoren (Store-Repo):** `themes/<id>/<id>.fltheme.json` (das Theme) und `themes/<id>/<id>.fltheme.meta.json` (`description`, `version` x.y.z, `author`, `license` MIT oder CC0-1.0). Wartung: `fl-plugin-sign build-themes <Ordner> <out> <Folge>`, `sign themes`, `check-themes` (CI). Beispiele: `store-seed/themes`.

## Statuspunkte
Seitenleiste: Maus kurz über dem Punkt ruhen lassen (0,5 s) zeigt „Eingerichtet und eingeschaltet“ (grün) bzw. „Nicht eingerichtet oder ausgeschaltet“ (grau).

## Design systems (since 2.9.2)

Profiles use different window compositions and navigation. Cupertino has a collapsible sidebar, floating toolbar, optional inspector and grouped overview. Fluent has functional page search, Back history, a breadcrumb and compact tiles. Material has five rail destinations, contextual subpages (a menu when space is tight), a large headline, a hero metric and floating Save/Discard actions only while changes are pending. System displays also differ: Cupertino grouped list, Fluent compact dashboard, Material CPU hero with secondary cards. Hit targets share their rendering geometry. Select **Design system** in the editor; apply and restart to update native settings.

Optional JSON `style` accepts only `fuchslabs`, `cupertino`, `fluent`, `material`. Existing files retain FuchsLabs geometry except the three exact starter names, recognized on load; explicit `fuchslabs` overrides recognition. Duplicate/export preserve profiles. Old clients may reject an explicit style (2.9.1 rejects themes with the key `style` as "unknown key"), so published-compatible seed files remain unchanged. No store catalog was published.

These are Windows interpretations of the official systems linked above, not original frameworks or full OS replicas. No third-party logos or redistributed fonts; unavailable fonts fall back to Segoe UI. Fluent requests a DWM Mica caption on supported Windows; client surfaces stay opaque. Cupertino uses a custom toolbar surface rather than Liquid Glass. No platform motion or Companion UI redesign. The Windows appearance option continues to use system colors. Validation status is recorded in TESTPROTOKOLL.md.

## English
Settings › **Colors & design**: Light (FuchsLabs), Dark (FuchsLabs), **Match Windows** or your own themes. The choice applies to the USB display, monitor and Companion (the image is rendered on the PC). The selection is kept with "Save & apply"; theme files are saved immediately.

**Match Windows**: light/dark from Windows "app mode", accent from the Windows accent colour; only the accent is replaced, header and fox logo stay fox orange (FuchsLabs profile). With the Cupertino, Fluent or Material profile the remaining colours come from the profile palette (light/dark following Windows), not from the FuchsLabs palette. Changes in Windows apply live. Contrast is corrected automatically (text 4.5:1, bars 3:1). In Windows high-contrast mode the display uses the system colours. Read from the registry (AppsUseLightTheme, DWM AccentColor), not UISettings.

**Custom themes**: files in `%LOCALAPPDATA%\FuchsLabs\USB-Display\themes\<id>.json`, same JSON format as above (version 1, nine `#RRGGBB` colours, name 1–40 characters), strictly validated; invalid files are ignored and listed. The editor offers hex input, live preview, contrast warning, "Fix contrast", duplicate, delete, import and export. The old "Custom colors" migrate on first start. Example themes (style-inspired, no third-party logos): `cpp/examples/themes`.

**Themes from the store**: Settings › **Colors & design** › "Themes from the store" card: install signed color designs with one click; the new theme is then selected (keep it with "Save & apply"). The lookup follows the "Use the plugin store" switch. Same JSON as custom themes (nine colors and an optional fixed design profile, no code), delivered in a separate signed list `themes.json` (older app versions ignore it). Signature, size, SHA-256 and the strict theme schema are checked before installation; revoked themes are hidden. A theme with the same name is never overwritten; delete it under "Colors & design" first to update.

**Authors (store repo)**: `themes/<id>/<id>.fltheme.json` plus `<id>.fltheme.meta.json` (`description`, `version`, `author`, `license` MIT or CC0-1.0). Maintenance: `fl-plugin-sign build-themes|sign themes|check-themes`. Examples: `store-seed/themes`.

**Status dots**: hover the dot in the sidebar for half a second to see "Set up and switched on" (green) or "Not set up or switched off" (grey).

## Hell und Dunkel / Light and dark

Alle drei Profile bieten getrennte helle und dunkle Referenzpaletten. Die sechs Beispiele unter `cpp/examples/themes` kombinieren denselben Aufbau mit passenden Flächen, Text-, Akzent- und Statusfarben. Der Hell/Dunkel-Schalter des nativen Fensters wechselt auch bei einem aktiven Designprofil die Palette; das Profil bleibt erhalten. Unter **Basis** im Theme-Editor lädt ein Profilwechsel die Referenzpalette und ersetzt die neun Farbwerte, danach sind eigene Anpassungen möglich. Theme-Dateien werden erst mit **Theme speichern** gesichert.

All three profiles provide separate light and dark reference palettes. The six examples in `cpp/examples/themes` share the profile structure while adapting surfaces, text, accent and status colors. The native window's light/dark switch also switches an active profile's palette. Changing a design profile's base in the editor loads the reference palette and replaces all nine color values; you can customize them afterwards. Files are only saved with **Save theme**.

Quellen / sources: [Fluent color roles and interaction states](https://fluent2.microsoft.design/color), [Apple Dark Mode](https://developer.apple.com/design/human-interface-guidelines/dark-mode), [Material color roles](https://m3.material.io/styles/color/roles).
