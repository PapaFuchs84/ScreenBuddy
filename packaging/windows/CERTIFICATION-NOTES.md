# Certification notes — ScreenBuddy for Windows

Review text prepared for Partner Center, 10 November 2026. Replace this header with the actual submitted package version and official identity after name reservation. Do not submit the local validation identities as an official product.

## English reviewer notes

ScreenBuddy is a native x64 Win32 desktop information/display utility for Windows 10 build 19041 or later. It can show selected PC measurements and optional third-party information on compatible USB displays, another Windows monitor, or a separately installed Android Companion. The Android app is not bundled in the MSIX and is not required to review the Windows settings or layout editor.

No FuchsLabs account or reviewer credentials are required. On first launch, native settings/setup are available; choose German or English at the top. Inspect the system page and Custom views editor with a sample layout. If no compatible display is connected, a disconnected-display state is expected; it does not prevent configuration. Optional provider connections remain unconfigured and may show missing-data or n/a indicators. Never use production credentials supplied outside an authorized review arrangement.

The package declares runFullTrust because the existing desktop application uses Windows desktop APIs for USB/COM and monitor output, performance information, tray UI, optional local Companion TCP/TLS communication and desktop integrations. It installs no driver, Windows service or browser extension. Its initially disabled StartupTask starts the separate ScreenBuddyStartup launcher with --autostart; the user enables startup explicitly in the application and Windows policy can override it.

Store package identity is detected at runtime. The standalone installer/uninstaller and GitHub app updater are bypassed. Data uses the package's LocalState. Microsoft Store owns app updates. The package contains only the two app executables, display assets and license notices; no account files, logs, pairings, plugins or private keys are included.

No allowElevation capability is requested. The packaged FPS setup refuses both UI and direct --grant-perflog elevation paths, explains administrator-assisted Performance Log Users membership setup, and continues to support existing ETW rights or HWiNFO readings. Packaged vendor-autostart changes are blocked before invoking elevated Task Scheduler commands; users manage this outside the app. Normal configuration/display operation does not require administrator rights.

The packaged Updates & startup page includes Help & privacy controls opening the app-specific policy and support pages. Optional network integrations are explained there. The app is not a generative AI client; its AI Status feature displays local client events and separately consented account-usage values.

Local tests and generated sample images are development evidence, not certification. See the supplied current test summary. WACK results must be associated with their tested binary hash and identity; final official-identity package signing and Store-delivered servicing remain distinct checks.

## Deutsche Zusammenfassung für die Einreichung

Native x64-Win32-App für Windows ab Build 19041. Kein FuchsLabs-Konto nötig; Einstellungen und Ansichts-Editor sind ohne angeschlossene USB-Anzeige prüfbar. Die optionale Android-Companion-App ist separat und nicht im MSIX enthalten. Fehlende optionale Datenquellen zeigen n/a.

runFullTrust ist für Desktop-APIs, USB/COM, Monitor-/Tray-Ausgabe, Systemwerte und den optionalen lokalen TCP/TLS-Companion-Dienst erforderlich. Kein Treiber oder Systemdienst wird installiert. Der Autostart ist zunächst aus und wird ausdrücklich vom Nutzer aktiviert. Store-Daten liegen in LocalState, App-Updates kommen über Microsoft.

Das Paket fordert kein allowElevation an. FPS-Gruppenfreischaltung und erhöhte Änderung des Hersteller-Autostarts sind im Store-Kontext gesperrt und durch Hinweise zur externen Administrator-Einrichtung ersetzt. Die App-spezifischen Datenschutz-/Hilfeseiten sind aus den paketierten Einstellungen erreichbar. Keine echten Zugangsdaten oder Kopplungscodes in Prüfbilder aufnehmen.

## Executed local WACK result (10 November 2026)

Official-identity developer registration, final binary SHA-256 `b78e2dc576beea84c169b888349ce1cb80ca594d9403dea43f0dff8eeab2a466`: overall PASS, no partial run, all 13 required tests passed. One optional Blocked executables heuristic flags process-launch APIs and command/interpreter strings; the other ten optional tests passed. The app launches its bundled startup target and opens selected browser/desktop integrations; optional AI hooks use Windows PowerShell and may be unavailable in Windows S mode. No full S-mode compatibility claim is made. Microsoft classifies these Desktop Bridge optional checks as informational: https://learn.microsoft.com/en-us/windows/uwp/debug-test-perf/windows-desktop-bridge-app-tests . No test filters or binary obfuscation were used to hide the warning. Store certification remains a separate Microsoft decision.
