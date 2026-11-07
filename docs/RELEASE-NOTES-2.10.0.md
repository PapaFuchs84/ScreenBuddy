# ScreenBuddy 2.10.0 – in Vorbereitung (nicht veröffentlicht)

**Status:** Entwicklungsstand. Es gibt noch keinen Tag, keine ausgelieferte EXE und keine Companion-APK für diese Version. Die aktuelle Veröffentlichung bleibt **2.9.5**; Companion bleibt **1.0.3**. Änderungen im Detail: [CHANGELOG.md](../CHANGELOG.md).

## Neu: Plugin-Seiten

- **Zonen-Layout (Manifest v2):** `schema_version: 2` beschreibt ein Raster mit Zonen und Blöcken mit stabilen IDs. Grenzen: 6 × 6 Raster, 8 Zonen, 4 Blöcke je Zone, 12 Widgets mit Layout. Manifest v1 bleibt unverändert. Beschreibung und Beispiel: [Plugin-System](PLUGIN-SYSTEM.md#manifest-v2-zonen-layout-2100-in-vorbereitung).
- **Dichtestufen:** kompakt, normal und ausführlich je Bausteintyp. Passt ein Block nicht, wird er ausgeblendet statt überlappt.
- **Bedingte Anzeige (`show_if`)** und **Fehlerisolierung:** Ein Block erscheint nur bei vorhandenem Wert oder einfachem Vergleich; schlägt ein Block fehl, fehlt nur dieser Block.
- **Wechselfeld:** Eine Zone wechselt zwischen Blöcken (Modus `rotate` mit 3–600 s oder `first_available`).
- **Tap-Ziele:** Blöcke können HTTPS-Links am PC öffnen. Die Freigabe je Plugin ist standardmäßig aus.
- **Seiten-Editor:** unter **App → Seite anordnen**: Raster, Blockliste, Bedingungen, Vorschau in Quer- und Hochformat, Rückgängig/Wiederholen, Tastaturbedienung. Gespeichert wird nur die Anordnung, mit Sicherungskopie.
- **Rahmenstil:** Plugin-Karten übernehmen Radius und Abstände aus dem Theme.

## Neu: Eigene Ansichten (P8-2)

Unter **App → Eigene Ansichten** Quellen-Kacheln kombinieren, mit Palette/Raster/Eigenschaften bearbeiten und je aktivem Display speichern. HA-Entitäten bleiben nativ gezeichnet; Listen und Spotify-Inhalte sind einzeln verfügbar. Offline-Tabs entfallen, gespeicherte Geräte-Layouts bleiben erhalten. Noch keine eigenen Touch-Aktionen. [Bedienung, Grenzen und aktueller Prüfstand](PHASE8-EIGENE-ANSICHTEN.md).

## Neu: Geräte und Darstellung

- **Sperrzone pro Gerät:** Ränder in ganzen Pixeln für USB-Display und Monitor (unter **Anzeige**). Companion mit gemeinsamer Vorgabe und eigener Sperrzone je gekoppeltem Gerät im unabhängigen Betrieb. Details: [Navigation](NAVIGATION.md#geräteprofil-und-sperrzone-phase-4a-entwicklung) · [Companion](COMPANION.md).
- **Android-TV** läuft immer im Querformat; die Ausrichtungswahl ist dort ausgeblendet.
- **Eingebaute Seiten** (System, Freunde, Discord, Battlefield 6, Blogs, Streaming, Spotify) zeichnen ihre Karten über den gemeinsamen Rahmen. Home Assistant bleibt nativ.
- **Kontrast:** Warn- und Kopfzeilenfarben in drei Paletten angehoben; Textkontrast ≥ 4,5:1 wird in einer Themenmatrix geprüft.

## Behoben

- **BF6-Rangliste:** PC-Konten werden mit `platform=pc` abgefragt, mit Rückfall auf `ea` und den kleingeschriebenen Namen.
- **Spotify:** Läuft nichts, zeigt die Seite den zuletzt gespielten Titel. Dafür einmal unter **Einstellungen › Spotify** neu anmelden, damit der Zugriff „zuletzt gespielt“ erteilt wird.

## Sicherheit und Kompatibilität

- Tap-URLs nur über HTTPS, strikt geprüft und beim Tipp erneut kontrolliert; Link-Freigabe bei jeder anderen Plugin-Änderung widerrufen (nur eine reine Linklisten-Änderung behält sie); höchstens ein Linkversuch je Plugin alle zwei Sekunden.
- Geheime Einstellungen sind in Bedingungen verboten; ein ungültiges `show_if` lehnt das Manifest ab.
- Schema 2 braucht im Store `min_app_version` 2.10.0. Ältere Apps zeigen das Plugin als „braucht neueres ScreenBuddy“ und lehnen es ab.
- Companion-Protokoll v1 bleibt unverändert; ältere Companion-Versionen funktionieren ohne Anpassung.

## Prüfung und Grenzen

- Automatisierte Prüfungen (Zonen, Bedingungen, Beispiele, Snapshots, Tap-Ziele, Wechselfeld-Planung) laufen gegen feste Daten. Prüfzahlen und Umgebungsgrenzen stehen in [`cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md).
- **Offen:** physische Abnahme an USB-Display (auch Hochformat-Gehäuse), Monitor, Companion und Android-TV; native Sichtprüfung der Oberfläche; gemessene serielle Bildrate für das Wechselfeld; Companion-Prozesstests; APK-Build.
- **Eingebaute Seiten:** Die Basislinie ist rechnergebunden (Roboto als Benutzerschrift nötig). Auf dem Referenzrechner sind alle migrierten Seiten bitgleich (0 Abweichungen); die Hardware-Abnahme steht aus.
- Die Anleitungen in der EXE (`cpp/res/GUIDE.html`, `ANLEITUNG.html`) sind noch nicht auf 2.10.0 abgeglichen; sie werden nur zusammen mit dem Build-Hash-Mechanismus geändert.

## English

**Status: in preparation, not released.** No tag, no EXE and no Companion APK exist for this version. The current release remains **2.9.5**; the Companion app remains **1.0.3**.

- **Plugin pages:** manifest v2 with a zone grid (up to 8 zones, 4 blocks per zone, 12 widgets with a layout), density levels, conditional display (`show_if`), fault isolation per block, cycling zones (`rotate`, 3–600 s, or `first_available`), and tap targets that open HTTPS links on the PC. Link consent is off by default. The page editor is under **App → Arrange page**. Manifest v1 is unchanged. See [Plugin system](PLUGIN-SYSTEM.en.md).
- **Custom views (P8-2):** dedicated editor under App, with source palette, grid, properties and layouts per active display; native HA entity/list/Spotify tiles reuse existing painters. Offline tabs are hidden while saved layouts remain. Custom touch actions are pending. [Usage and validation](PHASE8-EIGENE-ANSICHTEN.md).
- **Devices:** keepout in whole pixels 0–1000 for the USB display and monitor, plus a shared default and an individual keepout per paired Companion device in independent mode. Android TV always runs in landscape.
- **Built-in pages** draw their cards through the shared frame; Home Assistant stays native.
- **Fixed:** BF6 leaderboard requests use `platform=pc` with fallbacks; Spotify shows the last played track when nothing is playing (re-sign-in required for the new permission).
- **Security:** strict HTTPS tap URLs checked again at tap time; link consent revoked on any other plugin change (a pure link-list change keeps it); secrets are forbidden in conditions.
- **Open:** physical acceptance on USB display, monitor, Companion and Android TV; native UI visual check; measured serial frame rate for cycling zones; Companion process tests; APK build. The built-in baseline is machine-bound (Roboto as a user font); on the reference machine all migrated pages compare bit-exact (0 differences). Details: [`cpp/TESTPROTOKOLL.md`](../cpp/TESTPROTOKOLL.md).
