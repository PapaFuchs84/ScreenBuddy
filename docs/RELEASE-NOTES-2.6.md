# ScreenBuddy 2.6.0

30 October 2026 · Windows 10/11 x64 · Companion-App 1.0 (Android 8+, separater Download).

## Deutsch

Erste vollständige Plugin-Plattform: ein signierter **Plugin-Store** direkt in der App. Plugins sind reine Datendateien ohne Programmcode. Der Store lädt einen mit Ed25519 signierten Index und eine signierte Sperrliste aus dem Repository `PapaFuchs84/ScreenBuddy-Plugins`; die App installiert nur Pakete, deren Prüfsumme und Host-Angaben zum signierten Eintrag passen.

Neu in dieser Version:

- **Plugin-Store** als Einstellungsseite (neben Companion und Plugins): Suche, Kategorien (Alle, System, Spiele, Smart Home, Info, Sonstiges), Karte mit Autor, Version, Lizenz und Status. Vor der Installation steht dort, an welche Hosts ein Plugin Daten senden würde.
- **Installieren und Aktualisieren** mit Bestätigung. Installierte Plugins bleiben **ausgeschaltet**; sie müssen unter **Plugins** bewusst aktiviert werden. Jeder Host braucht deine Freigabe. Bei einem Update erscheinen neu hinzugekommene Hosts in der Karte, und alle Freigaben werden zurückgesetzt.
- **Gesperrte Plugins** werden beim Abgleich automatisch ausgeschaltet, auch aus dem Zwischenspeicher.
- **Abgleich:** höchstens einmal täglich im Hintergrund (erster Versuch 30 Sekunden nach dem Start), manuell über **Store aktualisieren** (frühestens nach 60 Sekunden). Ohne Netz wird der zuletzt geprüfte Stand aus dem Zwischenspeicher angezeigt; er wird beim Laden erneut geprüft.
- **Schalter „Plugin-Store verwenden“** (`[ui] plugin_store`, Standard: an). Aus bedeutet: kein Abruf, nur lokale Plugins. Die Änderung wirkt nach **Speichern & anwenden**.
- **Signierung:** Index und Sperrliste sind mit Ed25519 signiert; der öffentliche Schlüssel ist in der App fest eingetragen. Die Signatur zeigt, dass der Maintainer den Index veröffentlicht hat. Sie ersetzt nicht die menschliche Prüfung eines Plugins. Rollback-Schutz über eine laufende `sequence`.
- **Start-Plugins:** acht Plugins im Store (Uhr, Fokus, Wetter, Spieleabend, Pomodoro-Tafel, Zitat-Karte, Drei Aufgaben, Trink-Erinnerung). Die Wetterquelle fragt öffentliche Daten ab; ihre Hosts stehen vor der Installation in der Karte.
- **Nutzungsbedingungen, Regeln und Datenschutz** ([Deutsch](PLUGIN-STORE-BEDINGUNGEN.md), [English](PLUGIN-STORE-TERMS.md)) als **Entwurf, juristisch nicht geprüft**. **Autorenleitfaden** für Einreichungen: [PLUGIN-AUTOREN.md](PLUGIN-AUTOREN.md).
- **Companion-App 1.0** als separater Download. Die Android-App wird nicht über die Windows-EXE ausgeliefert.

Datenschutz: Mit eingeschaltetem Store ruft ScreenBuddy automatisch höchstens einmal täglich (manuell frühestens alle 60 Sekunden) den Index und die Sperrliste von GitHub Pages ab, dabei übermittelt GitHub die IP-Adresse wie bei jedem Abruf. Es gibt keine Telemetrie. Zum Abschalten: **Plugin-Store verwenden** ausschalten und speichern.

Plattform und Signierung: [Plugin-Signierung](PLUGIN-SIGNIERUNG.md) · [Paketformat](PLUGIN-PAKETFORMAT.md) · [Plugin-Tutorial](PLUGIN-TUTORIAL.md).

### Prüfgrenzen

- **Store-Installation und -Update** wurden in einer isolierten Instanz geprüft: Installation des Plugins „Wetter“ aus dem Live-Store bestanden (Plugin bleibt aus, kein Host zugestimmt); Schalter „Plugin-Store verwenden“ speichern und Neustart bestanden; Update nur als Simulation (Version lokal auf 0.9.0 gesetzt) bestanden, ein echtes Live-Update war mangels zweiter Version nicht prüfbar; Suchfeld nicht automatisiert prüfbar. Dabei gefundene Fehler (Plugin-Dateien unter GitHub Pages nicht ausgeliefert, Schalter nicht speicherbar) sind behoben.
- **Automatisierte Prüfungen:** 1.312 Windows-Prüfungen und 2 Installer-Prüfungen bestanden, Android-Protokolltests bestanden
- Live-Index: `sequence` 2 mit 8 Plugins, abgerufen am 30.10.2026. Dass ein Plugin ungefährlich ist, beweist die Signatur nicht.
- Die Signaturschlüssel-Rotation ist noch nicht implementiert; es gibt genau einen Schlüssel. Bei Verlust hilft nur ein neues App-Release.
- Eine alte, korrekt signierte Plugin-Version kann weiter im Umlauf sein, solange ihr Eintrag nicht gesperrt ist.
- Online-Plugins bleiben experimentell. CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen.
- Die Companion-Geräteprüfung vom 27.10.2026 bezieht sich auf 0.2.1-preview (versionCode 3). Eine eigene Geräteprüfung für Companion 1.0 ist im Testprotokoll nicht dokumentiert.
- Offen bleiben Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus, DPI-/Screenreader-Prüfung und mehrere Android-Geräte.

### Download und Prüfung

- Die Windows-EXE ist **unsigniert**. Vergleiche sie mit `SHA256SUMS.txt` aus dem Release.
- Die Companion-App 1.0 ist ein separater Download. Prüfe ihre Prüfsumme und Signatur gegen die Angaben auf der Release-Seite.
- Vor einem Upgrade EXE, `config.toml` und `companion.json` sichern. Der Installer legt bei Updates ohnehin eine Einstellungssicherung an.
- Bei Companion-Wechsel von einer Debug-APK: Die Signatur unterscheidet sich, eine direkte Aktualisierung ist nicht möglich. Nicht automatisch deinstallieren; lokale Daten und Kopplungen gingen verloren. Siehe [Companion-Einrichtung](COMPANION.md).

Historische Entwicklungsnotizen: [Archiv 2.5](DEVELOPMENT-NOTES-2.5.md). Vorgängerstand: [Release-Notizen 2.5](RELEASE-NOTES-2.5.md).

## English

First complete plugin platform: a signed **plugin store** inside the app. Plugins are pure data files without program code. The store loads an Ed25519-signed index and a signed revocation list from the `PapaFuchs84/ScreenBuddy-Plugins` repository; the app installs only packages whose checksum and host declarations match the signed entry.

New in this version:

- **Plugin store** as a settings page (next to Companion and Plugins): search, categories (All, System, Games, Smart home, Info, Other), cards with author, version, licence and status. Before installation the card shows which hosts a plugin would send data to.
- **Install and update** with confirmation. Installed plugins stay **off**; enable them deliberately under **Plugins**. Every host needs your consent. On update, newly added hosts appear on the card and all consents are reset.
- **Revoked plugins** are switched off automatically during a sync, including from the cache.
- **Sync:** at most once a day in the background (first attempt 30 seconds after start) and manually via **Refresh store** (at the earliest after 60 seconds). Without a network connection the last verified state from the cache is shown and verified again on load.
- **Switch "Use the plugin store"** (`[ui] plugin_store`, default on). Off means no requests at all, local plugins only. The change takes effect after **Save & apply**.
- **Signing:** index and revocation list are signed with Ed25519; the public key is pinned in the app. The signature shows that the maintainer published the index. It does not replace human review of a plugin. Rollback protection via a monotonically increasing `sequence`.
- **Starter plugins:** eight plugins in the store (clock, focus, weather, game night, Pomodoro board, quote card, three tasks, water reminder). The weather source queries public data; its hosts are shown on the card before installation.
- **Terms of use, rules and privacy** ([German](PLUGIN-STORE-BEDINGUNGEN.md), [English](PLUGIN-STORE-TERMS.md)) as a **draft, not legally reviewed**. **Author guide** for submissions: [PLUGIN-AUTHORS.md](PLUGIN-AUTHORS.md).
- **Companion app 1.0** as a separate download. The Android app is not shipped inside the Windows EXE.

Privacy: with the store switched on, ScreenBuddy fetches the index and the revocation list from GitHub Pages automatically at most once a day (manually at most every 60 seconds); as with any request, GitHub sees the IP address. There is no telemetry. To switch off, turn off **Use the plugin store** and save.

Platform and signing: [Plugin signing](PLUGIN-SIGNING.md) · [Package format](PLUGIN-PACKAGE-FORMAT.md) · [Plugin tutorial](PLUGIN-TUTORIAL.en.md).

### Test limits

- **Store installation and update** were checked in an isolated instance: installing the plugin “Weather” from the live store passed (plugin stays off, no host consented); saving the “Use the plugin store” switch and restarting passed; update passed only as a simulation (version set to 0.9.0 locally), a real live update could not be tested because no second version exists; the search field was not testable by automation. Defects found on the way (plugin files not served by GitHub Pages, switch not saved) are fixed.
- **Automated checks:** 1,312 Windows checks and 2 installer checks passed, Android protocol tests passed
- Live index: `sequence` 2 with 8 plugins, retrieved on 30 Oct 2026. The signature does not prove that a plugin is harmless.
- Signing-key rotation is not implemented yet; there is exactly one key. If it is lost, only a new app release helps.
- An old, correctly signed plugin version can remain in circulation as long as its entry is not revoked.
- Online plugins remain experimental. CRL/OCSP revocation checks and the complete UI import/consent workflow are not accepted.
- The Companion device test of 27 Oct 2026 refers to 0.2.1-preview (versionCode 3). A separate device test for Companion 1.0 is not documented in the test record.
- Still open: Windows sign-in test, Wi-Fi, launcher/boot mode, DPI/screen-reader checks and several Android devices.

### Download and verification

- The Windows EXE is **unsigned**. Compare it with `SHA256SUMS.txt` from the release.
- The Companion app 1.0 is a separate download. Check its checksum and signature against the release page.
- Before an upgrade, back up the EXE, `config.toml` and `companion.json`. The installer already creates a settings backup on updates.
- When moving from a debug APK of Companion: the signature differs, a direct update is not possible. Do not uninstall automatically; local data and pairings would be lost. See [Companion setup](COMPANION.en.md).

Historical development notes: [Archive 2.5](DEVELOPMENT-NOTES-2.5.en.md). Previous release: [Release notes 2.5](RELEASE-NOTES-2.5.md).
