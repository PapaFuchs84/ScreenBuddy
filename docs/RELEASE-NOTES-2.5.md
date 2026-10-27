# ScreenBuddy 2.5.0 – lokaler Release-Kandidat / local release candidate

Status: 27 October 2026. Not published. Source: feature/native-ui-framework, working tree based on 2efe036; not yet integrated into a final release commit.

## Deutsch

Native Windows-Einstellungen mit Direct2D/DirectWrite ersetzen die bisherige WebView2-Oberfläche. Enthalten sind Einrichtung, Anzeige, Seiten, Datenquellen, Companion, Design, Updates und Plugin-Verwaltung. Declarative Plugins unterstützen lokale Beispiele sowie begrenzte HTTPS-Quellen mit Host-Zustimmung und geschützten Einstellungen. Companion bleibt eine Vorschau mit unabhängigen Seiten und Auflösung.

Windows-Kandidat 2.5.0 und signierte Android-Vorschau 0.2.1-preview (versionCode 3). Die Android-Signatur unterscheidet sich von der bisherigen Debug-APK: kein direktes Update darüber; nicht ohne abgestimmte Datensicherung deinstallieren. Lokale Kopplungen müssen bei Neuinstallation neu eingerichtet werden.

Automatisierte Komponenten-, native UI-, API-, Integrations-, Installer- und isolierte echte Upgrade-Prüfungen bestanden; Android: 20 Protokolltests, Release-Build und APK-v2-Signatur bestanden. Keine Freigabe für produktive Installation: finale physische Display-/Touch-/Android-Abnahme, Zielsystem-Upgrade/Rollback, vollständige UI-/DPI-/Screenreader- und Guide-Sichtprüfung sowie Integration auf dem finalen Commit stehen aus. Englische Anleitung enthält als solche gekennzeichnete deutsche native Aufnahmen. Vor Upgrade EXE, config.toml und companion.json rückspielbar sichern; bei Fehlern den alten Stand wiederherstellen.

## English

Native Direct2D/DirectWrite Windows settings replace the former WebView2 interface. Setup, displays, pages, data sources, Companion, design, updates and plugin management are included. Declarative plugins support local examples and bounded HTTPS sources with host consent and protected settings. Companion remains a preview with independent pages and resolution.

Windows candidate 2.5.0 and signed Android preview 0.2.1-preview (versionCode 3). The Android certificate differs from the previous debug APK: it cannot directly update that APK. Agree on data backup before uninstalling; a fresh installation requires pairing again.

Automated component, native UI, API, integration, installer and isolated real-upgrade checks passed; Android: 20 protocol tests, release build and APK v2 signing verified. Production acceptance remains pending: final physical display/touch/Android tests, target-system upgrade/rollback, complete UI/DPI/screen-reader and guide visual review, plus integration on the final commit. The English guide identifies its German native screenshots. Back up the executable, config.toml and companion.json before upgrading; restore the saved predecessor if problems occur.
