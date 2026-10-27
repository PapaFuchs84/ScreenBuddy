# ScreenBuddy 2.5.0 – Release-Checkliste

27.10.2026.

Native Direct2D/DirectWrite-Einstellungen ohne WebView2, geführte Einrichtung, Display-/Touch-Auswahl und deklarative Plugins. Signierte Companion-Vorschau 0.2.1-preview separat enthalten. Windows-Sicht-/Touch-/Bedienprüfung, reales Upgrade 2.3.0 → 2.5.0 und Rückfall auf die gesicherte 2.3.0 bestanden; Konfiguration und Kopplungsdatei erhalten. Redmi 8: USB-Tethering, Quer-/Hochformat, Fingerbedienung, 5-Sekunden-Wechsel und Wiederverbindung ohne neue PIN bestanden. Automatisiert: 1.429 Windows-Prüfungen und 20 Android-Protokolltests.

- [x] Build sowie Komponenten-, Native-UI-, API-, Integrations- und Installerprüfungen bestanden.
- [x] Signierte Android-APK und 20 Protokolltests bestanden; reales Redmi 8 abgenommen.
- [x] Nutzer bestätigte Windows-Sicht-/Touch-/Maus-/Tastaturprüfung nach der Beobachtungszeit.
- [x] Echtes Zielsystem-Upgrade und Rollback mit geprüften EXE-/Konfigurations-/Kopplungs-Sicherungen bestanden.
- [x] 46 DE/EN-Anleitungsansichten gerendert und visuell geprüft; 204 lokale Dokumentationslinks geprüft.

## Veröffentlichung abschließen

- [ ] Finalen installierten EXE-Hash und Konfiguration nach dem Build der Anleitungsressourcen prüfen.
- [ ] Geprüften PR mit bestandener CI integrieren; Tag v2.5.0, Windows-EXE, signierte Vorschau-APK und SHA256SUMS.txt veröffentlichen.
- [ ] Veröffentlichte Downloads erneut laden und Prüfsummen vergleichen.
- [ ] Website-Release-Daten aktualisieren und bestehende Website deployen.

Prüfgrenzen: Online-Plugins bleiben experimentell; CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen. DPI-/Screenreader- und vollständige Funktionsparität sind nicht vollständig geprüft. EN-Anleitung mit ausdrücklich gekennzeichneten deutschen nativen Beispielbildern. Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus und mehrere Android-Geräte bleiben offen.

Entscheidung zu Zertifikatssperren: Zertifikatskette und Hostname werden geprüft; CRL/OCSP ist nicht implementiert. Online-Plugins sind ausdrücklich experimentell. Das ist eine dokumentierte Grenze und keine vollständige Plugin-Abnahme.

Android-Release-Schlüssel und Passwörter sicher zusätzlich sichern; künftige APK-Updates benötigen dasselbe Zertifikat. Für Rückfall App beenden und die privat gesicherte, geprüfte 2.3.0-EXE, config.toml, companion.json und appbezogenen Autostart-/Uninstall-Zustand wiederherstellen. [Testprotokoll](../cpp/TESTPROTOKOLL.md).
