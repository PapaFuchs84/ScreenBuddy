# Store listing draft / Store-Eintrag als Entwurf

Not submitted. The exact Store title ScreenBuddy by FuchsLabs is reserved; official identity is recorded below. Live IARC rating and remaining submission fields are being completed in Partner Center. App-specific privacy/support pages are public and verified; four current sample images are prepared. Never publish placeholders. The description, product features, keywords and “what's new” text below target Windows 2.12.0 (updated 14 November 2026; first certified Store edition was 2.11.0; public page https://apps.microsoft.com/detail/9P03461RLGG0); older status sentences in this file are historical. Screenshots/ contains four current 2.11.0 German/English native settings/editor renders at 1600 × 1200. They use actual Page/TreeCanvas code under a temporary developer package identity with sample data, not desktop captures or physical-device evidence; see provenance and captions.

## Deutsch

Name: ScreenBuddy by FuchsLabs

Kurzbeschreibung: Systemwerte und konfigurierbare Infoseiten auf USB-, Monitor- und Android-Anzeigen.

### Beschreibung (2.12.0; Store-Grenze 10.000 Zeichen)

```text
ScreenBuddy macht aus einem kleinen Zusatzbildschirm eine persönliche Infozentrale für deinen PC: Systemwerte, Freunde, Musik, Spiele, Smart Home und Neuigkeiten – auf einem USB-Display, einem zweiten Monitor oder einem Android-Gerät. Die App läuft im Hintergrund, wird über ein natives Windows-Einstellungsfenster in Deutsch oder Englisch bedient und blättert automatisch durch die Seiten, die du auswählst.

DREI ARTEN VON ANZEIGEN
• USB-Display: unterstützte 3,5-Zoll-USB-Displays mit 480 × 320 Pixeln („Revision A“, serielle Schnittstelle). Das Display wird automatisch gesucht; nicht jedes USB-Display wird unterstützt.
• Monitor: jeder zusätzliche HDMI- oder USB-C-Bildschirm als randloses Vollbild, auf Wunsch mit Touch (wischen oder tippen zum Blättern). Seiten werden in der echten Auflösung gerendert; breite Leisten zeigen mehrere Seiten nebeneinander.
• Android-Companion: Handy, Tablet oder Android-TV als weitere Anzeige – auch mehrere Geräte gleichzeitig (siehe unten).
USB-Display bzw. Monitor und Companion können gleichzeitig laufen, mit eigenen Seitenlisten, Wechselintervallen und Touch-Zielen.

SYSTEMWERTE UND HARDWARE-HUD
• Systemseite: CPU (Last, Takt, Temperatur, Lüfter), GPU (Last, Temperatur, Lüfter, VRAM), Arbeitsspeicher, Datenträger und Netzwerk mit Verlauf.
• Neu: optionales Hardware-HUD für die Systemseite – Neon-Vektorgrafik mit automatisch wechselnden Seiten Übersicht, GPU und CPU, animierten Lüftern, Ringen und Impulsen sowie eigenen Layouts für breite, normale und Hochkant-Displays.
• Neu: CPU-Package-Leistung in Watt aus passenden HWiNFO-Sensoren.
• CPU-Temperatur, Lüfterdrehzahl und Watt benötigen das kostenlose HWiNFO mit aktivierter Shared-Memory-Ausgabe. GPU-Werte gibt es für NVIDIA-Grafikkarten; andere GPUs liefern weniger Werte. Fehlende Messwerte werden als unbekannt angezeigt, nie geschätzt.

GAME-MODE
• Erkennt laufende Spiele aus Steam, Epic, Xbox, GOG Galaxy, Ubisoft Connect, EA App, Battle.net und Rockstar – rein lesend.
• Optionales Logo-Intro beim Spielstart, danach eine eigene Spielansicht im Cockpit-Design mit Kacheln wie FPS, Frametime, 1%-Low, GPU, VRAM und Ping samt Verlauf.
• FPS und Frametime erscheinen nur, wenn eine Messquelle verfügbar ist. Die Store-Ausgabe fordert dafür keine Administratorrechte an: Die nötige Windows-Gruppenberechtigung richtet ein Administrator einmalig manuell ein; vorhandene Berechtigungen oder HWiNFO-Messwerte werden genutzt.

FREUNDE, SPIELE UND NEUIGKEITEN
• Freundesliste aus Discord, Steam und Xbox in einer gemeinsamen Liste; dieselbe Person erscheint nur einmal. PlayStation-Freunde sind optional, standardmäßig aus und nutzen eine inoffizielle Schnittstelle auf eigenes Risiko.
• Discord-Seite mit Status, Aktivität und Avatar (eigener Discord-Bot).
• Battlefield-6-Rangliste eingetragener Freunde mit Rang, K/D und Siegquote.
• Blog-Updates per RSS/Atom: der neueste Beitrag je Blog.
• Neu bei Streaming: neue Filme und Serien deiner gewählten Anbieter (TMDB-Daten, eigener kostenloser Schlüssel).

MUSIK, SMART HOME UND KI
• Spotify: aktueller Titel mit Cover und Fortschritt, Amplifier-Balken und – mit Touch und Spotify Premium – Zurück, Pause und Weiter.
• Home Assistant: bis zu vier Sensoren oder Schalter; Schalter lassen sich per Touch bedienen.
• KI-Status für Claude Code und Codex über lokale Hooks; die Abfrage von Kontolimits ist optional und standardmäßig aus.

EIGENE ANSICHTEN, PLUGINS UND DESIGN
• Eigene Ansichten: Im nativen Editor kombinierst du Einzelkacheln für System, Listen, Spotify, Home Assistant und KI im Raster zu eigenen Seiten – je Display, mit Rückgängig/Wiederholen und einem Touch-Ziel je Kachel.
• Plugins: zusätzliche Seiten aus deklarativen Plugins mit Zonen-Layout; der integrierte, signierte Plugin-Katalog ist abschaltbar, und installierte Plugins bleiben aus, bis du sie aktivierst.
• Design: Hell oder Dunkel, an Windows anpassen (Akzentfarbe), eigene Themes mit Editor, Import und Export sowie Designprofile im Cupertino-, Fluent- und Material-Stil.
• Neu: globale Animationsrate von 5 bis 60 Bildern pro Sekunde (Standard 30) unter Einstellungen → Anzeige → Animationen. Sie gilt für sichtbare Animationen wie Hardware-HUD, KI-Status, Spotify und Seitenübergänge. Der Wert ist ein Ziel, keine Garantie; das serielle USB-Display überträgt bauartbedingt deutlich langsamer.

KOSTENLOSE ANDROID-COMPANION-APP
Mit der kostenlosen ScreenBuddy-Companion-App wird ein Android-Handy, -Tablet oder Android-TV zur zusätzlichen Anzeige. Gekoppelt wird per QR-Code oder PIN; die Verbindung läuft verschlüsselt direkt zwischen PC und Gerät im privaten Netz oder über USB-Tethering, ohne Cloud. Der PC rendert in der Auflösung des Geräts, Hoch- und Querformat werden unterstützt, und du blätterst per Wischen. Mehrere Geräte können synchron oder unabhängig laufen. Die optionale Automatik öffnet die Anzeige, sobald der gekoppelte PC erreichbar ist.
Die Companion-App ist nicht Teil dieses Store-Pakets und nicht bei Google Play erhältlich. Du lädst die APK kostenlos von GitHub und installierst sie selbst:
Download (Companion 1.1.0): https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/companion-v1.1.0
Anleitung: https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.md

DATENSCHUTZ UND SICHERHEIT
• Keine Telemetrie, kein ScreenBuddy-Konto, normaler Betrieb ohne Administratorrechte.
• Zugangsdaten werden mit Windows-DPAPI verschlüsselt gespeichert.
• Die lokale Einstellungsschnittstelle ist nur auf diesem PC erreichbar; der Companion-Dienst ist standardmäßig aus.
• Verbindungen zu Diensten bestehen nur für Datenquellen, die du selbst einrichtest.

GUT ZU WISSEN
• Viele Seiten benötigen ein eigenes Konto oder einen eigenen kostenlosen API-Schlüssel beim jeweiligen Dienst (z. B. Discord-Bot, Steam, OpenXBL, TMDB, Spotify, Home Assistant). Hilfen dazu stehen direkt neben den Eingabefeldern und in der eingebauten, bebilderten Anleitung.
• Die Store-Ausgabe erhält Updates über den Microsoft Store.
• ScreenBuddy ist ein unabhängiges Projekt von FuchsLabs. Battlefield, Discord, Steam, Xbox, PlayStation, Spotify und weitere Namen sind Marken ihrer jeweiligen Inhaber. This product uses the TMDB API but is not endorsed or certified by TMDB.
```

### Produktfeatures (höchstens 20, je höchstens 200 Zeichen)

1. Ausgabe auf unterstützten 3,5-Zoll-USB-Displays (480 × 320), jedem zusätzlichen HDMI-/USB-C-Monitor oder Android-Geräten mit der Companion-App
2. Systemseite mit CPU, GPU, Arbeitsspeicher, Datenträgern und Netzwerk samt Verlauf
3. Neu: optionales Hardware-HUD mit Neon-Vektorgrafik und automatisch wechselnden Seiten Übersicht, GPU und CPU
4. Neu: CPU-Package-Leistung in Watt aus passenden HWiNFO-Sensoren; fehlende Werte bleiben unbekannt
5. Neu: globale Animationsrate von 5 bis 60 Bildern pro Sekunde für sichtbare Animationen
6. Game-Mode mit Spielerkennung, optionalem Logo-Intro und eigener Spielansicht (FPS, Frametime, GPU, VRAM, Ping je nach Messquelle)
7. Gemeinsame Freundesliste aus Discord, Steam und Xbox; PlayStation optional und inoffiziell
8. Battlefield-6-Rangliste eingetragener Freunde mit Rang, K/D und Siegquote
9. Spotify-Seite mit Titel, Cover, Fortschritt und Amplifier-Balken; Touch-Steuerung mit Spotify Premium
10. Home Assistant: bis zu vier Sensoren oder per Touch schaltbare Schalter
11. Blog-Updates per RSS/Atom und neue Filme und Serien deiner Streaming-Anbieter (TMDB)
12. KI-Status für Claude Code und Codex über lokale Hooks
13. Eigene Ansichten: Kacheln im nativen Editor zu eigenen Seiten kombinieren, je Display
14. Plugins mit Zonen-Layout und signiertem, abschaltbarem Plugin-Katalog
15. Designs: Hell, Dunkel, an Windows anpassen, eigene Themes sowie Cupertino-, Fluent- und Material-Profile
16. Touch-Unterstützung auf Monitoren: wischen, tippen und passende Ziele öffnen
17. Kostenlose Android-Companion-App für Handy, Tablet und Android-TV, auch mehrere Geräte gleichzeitig
18. Native Windows-Einstellungen in Deutsch und Englisch mit Live-Vorschau und bebilderter Anleitung
19. Keine Telemetrie, kein ScreenBuddy-Konto; Zugangsdaten per Windows-DPAPI verschlüsselt
20. Normaler Betrieb ohne Administratorrechte; Updates über den Microsoft Store

### Suchbegriffe (höchstens 7)

USB-Display; Systemmonitor; Hardware-Monitor; Zweitbildschirm; Game-Mode FPS; Sensoranzeige HWiNFO; Android-Companion

### Neues in dieser Version (2.12.0; höchstens 1500 Zeichen)

```text
Neu in 2.12.0:
• Hardware-HUD: optionale Neon-Ansicht für die Systemseite mit automatisch wechselnden Seiten Übersicht, GPU und CPU, animierten Lüftern, Ringen und Impulsen sowie Layouts für breite, normale und Hochkant-Displays. Einschalten in den Einstellungen bei der Systemseite.
• CPU-Leistung in Watt aus passenden HWiNFO-Sensoren. Fehlende Werte bleiben unbekannt („--“).
• Globale Animationsrate: Einstellungen → Anzeige → Animationen, 5 bis 60, Standard 30. Gilt für sichtbare Animationen wie Hardware-HUD, KI-Status, Spotify, eigene Ansichten, Game-Intro und Plugin-Überblendungen. Statische Seiten und die Sensorabfrage bleiben unverändert sparsam.
• Companion: Animationen kommen ohne zusätzliche Verzögerung an; statische Seiten werden nicht unnötig neu gezeichnet. Die Android-App bleibt 1.1.0, ein Update ist nicht nötig.
• USB-Display: Die Übertragung läuft in einem eigenen Thread und bremst die Companion-Anzeige nicht mehr.
• Store-Ausgabe: Hinweise zu FPS-Berechtigung und Hersteller-Autostart, Links zu Hilfe und Datenschutz sowie kleinere Korrekturen.
Noch nicht mit echter Hardware abgenommen: Spotify-Steuerung mit echtem Konto, echtes Spielen und WLAN-Betrieb des Companion mit den neuen Animationen.
```

## English

Name: ScreenBuddy by FuchsLabs

Short description: PC system information and configurable pages on USB, monitor and Android displays.

### Description (2.12.0; Store limit 10,000 characters)

```text
ScreenBuddy turns a small secondary screen into a personal information hub for your PC: system stats, friends, music, games, smart home and news – on a USB display, a second monitor or an Android device. The app runs in the background, is configured in a native Windows settings window in English or German, and cycles automatically through the pages you choose.

THREE KINDS OF DISPLAY
• USB display: supported 3.5-inch USB displays with 480 × 320 pixels (“Revision A”, serial interface). The display is detected automatically; not every USB display is supported.
• Monitor: any additional HDMI or USB-C screen as borderless full screen, optionally with touch (swipe or tap to change pages). Pages render at the real resolution; wide bar displays show several pages side by side.
• Android Companion: a phone, tablet or Android TV as a further display – several devices at once if you like (see below).
The USB display or monitor and the Companion can run at the same time, each with its own page list, page timer and touch targets.

SYSTEM STATS AND HARDWARE HUD
• System page: CPU (load, clock, temperature, fan), GPU (load, temperature, fan, VRAM), memory, drives and network with history.
• New: optional Hardware HUD for the System page – neon vector graphics with automatically rotating Overview, GPU and CPU pages, animated fans, rings and pulses, and dedicated layouts for wide, regular and portrait displays.
• New: CPU package power in watts from suitable HWiNFO sensors.
• CPU temperature, fan speed and watts need the free HWiNFO with shared-memory output enabled. GPU readings are available for NVIDIA graphics cards; other GPUs provide fewer values. Missing readings are shown as unknown, never estimated.

GAME MODE
• Detects running games from Steam, Epic, Xbox, GOG Galaxy, Ubisoft Connect, EA App, Battle.net and Rockstar – read-only.
• Optional logo intro when a game starts, followed by a dedicated cockpit-style gaming view with tiles such as FPS, frametime, 1% low, GPU, VRAM and ping with history.
• FPS and frametime appear only when a measurement source is available. The Store edition does not request administrator rights for this: an administrator sets up the required Windows group membership manually once; existing permissions or HWiNFO measurements are used.

FRIENDS, GAMES AND NEWS
• Friends from Discord, Steam and Xbox in one combined list; the same person appears only once. PlayStation friends are optional, off by default and use an unofficial interface at your own risk.
• Discord page with status, activity and avatar (your own Discord bot).
• Battlefield 6 leaderboard of the friends you enter, with rank, K/D and win rate.
• Blog updates via RSS/Atom: the latest post from each blog.
• New on streaming: new films and series from your chosen providers (TMDB data, your own free key).

MUSIC, SMART HOME AND AI
• Spotify: current track with cover and progress, amplifier bars and – with touch and Spotify Premium – previous, pause and next.
• Home Assistant: up to four sensors or switches; switches can be toggled by touch.
• AI status for Claude Code and Codex through local hooks; querying account limits is optional and off by default.

CUSTOM VIEWS, PLUGINS AND DESIGN
• Custom views: in the native editor you combine single tiles for system, lists, Spotify, Home Assistant and AI on a grid into your own pages – per display, with undo/redo and a touch target per tile.
• Plugins: additional pages from declarative plugins with a zone layout; the integrated, signed plugin catalogue can be switched off, and installed plugins stay off until you enable them.
• Design: light or dark, match Windows (accent colour), custom themes with editor, import and export, plus design profiles in Cupertino, Fluent and Material style.
• New: global animation rate from 5 to 60 frames per second (default 30) under Settings → Display → Animations. It applies to visible animations such as the Hardware HUD, AI status, Spotify and page transitions. The value is a target, not a guarantee; the serial USB display transfers considerably more slowly by design.

FREE ANDROID COMPANION APP
The free ScreenBuddy Companion app turns an Android phone, tablet or Android TV into an additional display. Pairing uses a QR code or PIN; the connection is encrypted and runs directly between PC and device on your private network or over USB tethering, without any cloud. The PC renders at the device's resolution, portrait and landscape are supported, and you change pages by swiping. Several devices can run in sync or independently. Optional automatic mode opens the display as soon as the paired PC is reachable.
The Companion app is not part of this Store package and is not available on Google Play. You download the APK free of charge from GitHub and install it yourself:
Download (Companion 1.1.0): https://github.com/PapaFuchs84/ScreenBuddy/releases/tag/companion-v1.1.0
Guide: https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.en.md

PRIVACY AND SECURITY
• No telemetry, no ScreenBuddy account, normal operation without administrator rights.
• Credentials are stored encrypted with Windows DPAPI.
• The local settings interface is reachable only on this PC; the Companion service is off by default.
• Connections to services exist only for the data sources you set up yourself.

GOOD TO KNOW
• Many pages need your own account or your own free API key with the respective service (for example a Discord bot, Steam, OpenXBL, TMDB, Spotify, Home Assistant). Help is shown right next to the input fields and in the built-in illustrated guide.
• The Store edition receives updates through the Microsoft Store.
• ScreenBuddy is an independent project by FuchsLabs. Battlefield, Discord, Steam, Xbox, PlayStation, Spotify and other names are trademarks of their respective owners. This product uses the TMDB API but is not endorsed or certified by TMDB.
```

### Product features (at most 20, each at most 200 characters)

1. Output to supported 3.5-inch USB displays (480 × 320), any additional HDMI/USB-C monitor, or Android devices running the Companion app
2. System page with CPU, GPU, memory, drives and network including history
3. New: optional Hardware HUD with neon vector graphics and automatically rotating Overview, GPU and CPU pages
4. New: CPU package power in watts from suitable HWiNFO sensors; missing values stay unknown
5. New: global animation rate from 5 to 60 frames per second for visible animations
6. Game Mode with game detection, optional logo intro and a dedicated gaming view (FPS, frametime, GPU, VRAM, ping depending on the measurement source)
7. Combined friends list from Discord, Steam and Xbox; PlayStation optional and unofficial
8. Battlefield 6 leaderboard of the friends you enter, with rank, K/D and win rate
9. Spotify page with track, cover, progress and amplifier bars; touch controls with Spotify Premium
10. Home Assistant: up to four sensors or touch-operated switches
11. Blog updates via RSS/Atom and new films and series from your streaming providers (TMDB)
12. AI status for Claude Code and Codex through local hooks
13. Custom views: combine tiles into your own pages in the native editor, per display
14. Plugins with zone layout and a signed plugin catalogue that can be switched off
15. Designs: light, dark, match Windows, custom themes, plus Cupertino, Fluent and Material profiles
16. Touch support on monitors: swipe, tap and open matching targets
17. Free Android Companion app for phone, tablet and Android TV, including several devices at once
18. Native Windows settings in English and German with live preview and an illustrated guide
19. No telemetry, no ScreenBuddy account; credentials encrypted with Windows DPAPI
20. Normal operation without administrator rights; updates through the Microsoft Store

### Keywords (at most 7)

USB display; system monitor; hardware monitor; second screen; game mode FPS; HWiNFO sensor display; Android companion

### What's new in this version (2.12.0; at most 1500 characters)

```text
New in 2.12.0:
• Hardware HUD: optional neon view for the System page with automatically rotating Overview, GPU and CPU pages, animated fans, rings and pulses, and layouts for wide, regular and portrait displays. Enable it in settings next to the system page.
• CPU power in watts from suitable HWiNFO sensors. Missing values stay unknown (“--”).
• Global animation rate: Settings → Display → Animations, 5 to 60, default 30. Applies to visible animations such as the Hardware HUD, AI status, Spotify, custom views, the Game intro and plugin fades. Static pages and sensor polling stay as economical as before.
• Companion: animations arrive without an extra delay; static pages are not redrawn unnecessarily. The Android app stays at 1.1.0; no update is needed.
• USB display: transfers run in their own thread and no longer slow down the Companion display.
• Store edition: guidance on FPS permission and vendor autostart, help and privacy links, and minor fixes.
Not yet accepted on real hardware: Spotify control with a real account, real gameplay, and Wi-Fi operation of the Companion with the new animations.
```

## Certification notes

Classic x64 Win32 desktop app. runFullTrust supports device access, the local TCP/TLS server and desktop integrations. No driver installation in the MSIX. The separate startup executable launches the packaged app with --autostart. Normal operation requires no administrator privileges. Packaged FPS setup refuses elevation and the --grant-perflog helper; the UI explains administrator-assisted Performance Log Users membership setup instead. No allowElevation capability is requested. Packaged vendor-autostart management is disabled in the UI and blocked in the backend before an elevated scheduler command can run; users are directed to Windows Task Scheduler and administrator assistance. Stopping a user-level vendor process remains available.

Reviewers can inspect settings without a USB display and use monitor output. --demo --no-lcd --preview <folder> renders samples. No personal credentials or pairing codes may be included in screenshots or review data.

## Remaining inputs

Category and live IARC rating; final Companion APK download URL and hardware documentation; final device/certification record. Four 2.11.0 DE/EN native-rendered sample images are under Screenshots/ (see captions and provenance), visually checked for correct language and example-only data. They are not hardware, live-account or official Store-identity evidence. Publication remains pending.

## Verified app links (10 November 2026)

Privacy: https://fuchslabs.duckdns.org/screenbuddy/privacy/

Support: https://fuchslabs.duckdns.org/screenbuddy/support/

Public contact: fuchslabs2026@gmail.com. Both HTTPS pages returned 200 with exact reviewed content hashes and German/English sections; see website-pages/deployment-verification.json.

## Verified Store identity (10 November 2026)

Store ID: `9P03461RLGG0`

Package identity name: `49510Herbst.ScreenBuddybyFuchsLabs`

Package publisher: `CN=816E06C8-23FD-4A47-9D83-2F67C6F1C211`

Publisher display name: `Herbst` (exact Partner Center value; do not replace with the product brand).

Product display name: `ScreenBuddy by FuchsLabs`

Package family: `49510Herbst.ScreenBuddybyFuchsLabs_x6tj6b3k2dr8c`

The brand inside the Windows application remains ScreenBuddy. The exact plain title ScreenBuddy was unavailable in the Store; the owner selected this reserved alternative.

## Portal progress (11 November 2026)

Submission draft: `1152921505702369346`. Developer Agreement 8.11 acceptance is verified after owner approval. Properties is Complete; primary category Utilities + tools with no misleading subcategory and no MR-PC/OneDrive-backup/game-recording/alternate-drive declarations. Pricing is Complete at 0 EUR. Localized listings, live IARC and package upload are being completed; the package has transferred and shows Validating…, but package acceptance, certification submission and Store publication are not yet confirmed.
