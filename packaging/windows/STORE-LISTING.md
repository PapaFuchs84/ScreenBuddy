# Store listing draft / Store-Eintrag als Entwurf

Not submitted. The exact Store title ScreenBuddy by FuchsLabs is reserved; official identity is recorded below. Live IARC rating and remaining submission fields are being completed in Partner Center. App-specific privacy/support pages are public and verified; four current sample images are prepared. Never publish placeholders. This draft targets Windows 2.11.0; Screenshots/ contains four current 2.11.0 German/English native settings/editor renders at 1600 × 1200. They use actual Page/TreeCanvas code under a temporary developer package identity with sample data, not desktop captures or physical-device evidence; see provenance and captions.

## Deutsch

Name: ScreenBuddy by FuchsLabs

Kurzbeschreibung: Systemwerte und konfigurierbare Infoseiten auf USB-, Monitor- und Android-Anzeigen.

ScreenBuddy zeigt ausgewählte PC-Systemwerte und Informationsseiten auf unterstützten USB-Anzeigen, einem zusätzlichen Monitor oder einem Android-Gerät mit der ScreenBuddy Companion-App. Richte deine Seiten im nativen Einstellungsfenster ein und wähle passende Designs.

Optionale Integrationen zeigen Musik-, Streaming-, RSS- und Smart-Home-Informationen. Plugins und Themes können über den integrierten Katalog ergänzt werden. Dienste können ein eigenes Konto oder einen eigenen API-Schlüssel erfordern.

Android-Ausgabe benötigt die separat installierte Companion-APK und eine Verbindung zwischen beiden Geräten, etwa im lokalen Netzwerk oder über USB-Tethering. Die Companion-App ist derzeit nicht im Google Play Store veröffentlicht. USB-Ausgabe benötigt kompatible Hardware; nicht jede USB-Anzeige wird unterstützt.

Der optionale Game-Mode erkennt laufende Spiele und zeigt eine eigene Spielansicht mit Systemwerten, FPS und Frametime, sofern eine Messquelle verfügbar ist. Die Store-Ausgabe fordert für die ETW-FPS-Freischaltung keine Administratorrechte an; nötige Gruppenberechtigungen richtet ein Administrator manuell ein. Bereits vorhandene Berechtigungen oder HWiNFO-Messwerte können genutzt werden.

Funktionen: Game-Mode; Systemanzeige; konfigurierbare Seiten; native Einstellungen in Deutsch und Englisch; USB-/Monitor-/Companion-Ausgabe; mehrere Ansichten; Plugins und Themes.

Neue Funktionen: Erste MSIX-Ausgabe mit Store-Updates, paketfestem Autostart und separater Datenablage.

## English

Name: ScreenBuddy by FuchsLabs

Short description: PC system information and configurable pages on USB, monitor and Android displays.

ScreenBuddy presents selected PC system information and configurable pages on supported USB displays, an additional monitor, or an Android device running the ScreenBuddy Companion app. Configure pages in the native settings window and choose a theme.

Optional integrations display music, streaming, RSS and smart-home information. Additional plugins and themes are available through the integrated catalog. Some services require your own account or API key.

Android output requires the separately installed Companion APK and connectivity between both devices, for example a local network or USB tethering. The Companion is currently not published on Google Play. USB output requires compatible hardware; not every USB display is supported.

Optional Game Mode detects running games and displays a dedicated gaming view with system values, FPS and frametime when a measurement source is available. The Store edition does not elevate for ETW FPS setup; an administrator must configure any required group membership manually. Existing permissions or HWiNFO measurements can be used.

Features: Game Mode; system information; configurable pages; German and English native settings; USB/monitor/Companion output; multiple views; plugins and themes.

What's new: initial MSIX edition with Store updates, package-stable startup and separate data storage.

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
