# ScreenBuddy 2.5 – Entwicklungsstand

**ScreenBuddy 2.5.0 – 27.10.2026.** Native Direct2D/DirectWrite-Einstellungen ohne WebView2, geführte Einrichtung, Display-/Touch-Auswahl und deklarative Plugins. Signierte Companion-Vorschau 0.2.1-preview separat enthalten. Windows-Sicht-/Touch-/Bedienprüfung, reales Upgrade 2.3.0 → 2.5.0 und Rückfall auf die gesicherte 2.3.0 bestanden; Konfiguration und Kopplungsdatei erhalten. Redmi 8: USB-Tethering, Quer-/Hochformat, Fingerbedienung, 5-Sekunden-Wechsel und Wiederverbindung ohne neue PIN bestanden. Automatisiert: 1.429 Windows-Prüfungen und 20 Android-Protokolltests.

Prüfgrenzen: Online-Plugins bleiben experimentell; CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen. DPI-/Screenreader- und vollständige Funktionsparität sind nicht vollständig geprüft. EN-Anleitung mit ausdrücklich gekennzeichneten deutschen nativen Beispielbildern. Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus und mehrere Android-Geräte bleiben offen.

Die offenen Freigabepunkte stehen in der [Release-Checkliste](RELEASE-CHECKLIST-2.5.md).

Die signierte Companion-Vorschau 0.2.1-preview ist separat vorbereitet; ihr neuer Schlüssel unterscheidet sich von der bisherigen Debug-APK. Die Redmi-8-Abnahme über USB-Tethering ist bestanden; Grenzen stehen im Testprotokoll. [Signierung und Migration](../android/README.md).

## Implementiert

- EXE, Installer-Metadaten und Autostart-Eintrag verwenden ScreenBuddy. FuchsLabs bleibt als Herausgeber genannt. Der alte Registry-Uninstall-Schlüssel bleibt für Upgrades bestehen; die alte EXE wird als Rückfallkopie umbenannt.
- Das Einstellungsfenster ist ausschließlich nativ und selbst gezeichnet (Direct2D/DirectWrite), mit gruppierter Navigation, Live-Vorschau, Hell/Dunkel und DE/EN-Wechsel. Phase 2 enthält native Seiten für Discord, gemeinsame Freunde, Home Assistant, Companion-Kopplung und -Geräte sowie Farben & Design. Anzeige, Seitenreihenfolge, Updates, Autostart und Plugin-Verwaltung bleiben ebenfalls verfügbar. Änderungen lassen sich mit **Speichern & anwenden** speichern oder mit **Verwerfen** zurücksetzen. Bei einem Startfehler wird das native Fenster nicht durch WebView2 ersetzt; die App protokolliert den Fehler und zeigt ihn beim Öffnungsversuch an. Vollständige Funktionsparität mit der früheren HTML-Oberfläche ist noch nicht abgenommen.
- Die native Übersicht führt in drei Schritten zur Ausgabe, zu den Datenquellen und zur Seitenauswahl. Unter **Anzeige** zeigt die native Oberfläche die Windows-Touch-Zuordnung und öffnet den Windows-Assistenten direkt; eine erneute Prüfung aktualisiert die Anzeige. Der Assistent wurde auf echter Touch-Hardware in diesem Stand noch nicht abgenommen.
- Unter **Anzeige** bietet die native Monitorauswahl „Automatisch“, erkannte Bildschirme mit Auflösung und eine erneute Suche. Eine gespeicherte, gerade getrennte Monitor-ID bleibt auswählbar; ohne erkannten Bildschirm bleibt die automatische Wahl verfügbar. Die Auswahl ändert die Ausgabe erst nach **Speichern & anwenden**.
- Unter **Updates & Autostart** öffnet **Release-Seite öffnen** bei einem gemeldeten neueren Release dessen geprüfte GitHub-Seite im Standardbrowser. Der Knopf bleibt ohne gültige Release-Adresse deaktiviert; ScreenBuddy lädt oder installiert dadurch nichts automatisch.
- Deklarative Plugin-Manifeste (Schema 1) werden streng validiert. Der lokale Speicher kann Plugins importieren, aktivieren, deaktivieren und entfernen. Quellenbasierte Plugins lassen sich nach Eingabe ihrer Pflicht-Einstellungen und Zustimmung zu jedem deklarierten Host aktivieren. Der begrenzte HTTPS-Abruf läuft in einem eigenen Worker; Plugin-Einstellungen werden mit DPAPI geschützt. Import führt keinen Code aus.
- Datenabrufe laufen im eigenen Worker; das Rendern bleibt im Display-Thread. Ein öffentlicher HTTPS-Beispielabruf bestand zuvor isoliert; der vollständige Ablauf mit Plugin-Import, Einstellungen und Host-Freigabe in der App ist noch offen.
- Die Beispiele [Uhr](../cpp/examples/plugins/offline-clock.flplugin.json), [Fokus-Board](../cpp/examples/plugins/offline-focus.flplugin.json) und [Wetterquelle](../cpp/examples/plugins/public-weather.flplugin.json) zeigen Offline-Konfigurationen und einen optionalen öffentlichen HTTPS-Abruf. [Beispielübersicht](../cpp/examples/plugins/README.md) und [deutsche Anleitung](PLUGIN-TUTORIAL.md) erklären Import und Grenzen. Der Beispieltest prüft nur Manifest und Rendering; er führt keine Live-Abfrage aus.

## Architektur und Rollout

Die native Win32-Hülle nutzt die vorhandene C++20-Implementierung für Rendering, USB und Companion weiter. Der am 27.10.2026 neu gebaute 2.5-Kandidat bindet WebView2 nicht mehr ein; dessen Laufzeit ist für diesen Build keine App-Voraussetzung. Der lokale HTTP-Dienst bleibt für Companion/API und die eingebettete Anleitung erhalten; die bisherige HTML-Einstellungsseite wird nicht mehr eingebettet oder unter `/` ausgeliefert. Ihre Quelldatei bleibt für historische Tests erhalten. In dev.2 wurde weder auf Flutter noch Tauri umgeschrieben. Vor einer Freigabe sind UI-, Upgrade-, Hardware- und externe-Dienst-Abnahmen erforderlich. Der Installer sichert eine Vorgänger-EXE als Rückfallkopie; Zielsystem-Upgrade und Rückfall auf die gesicherte 2.3.0 sind bestanden. Öffentliche Repository- und Homepage-Umbenennung bleibt bis nach der Freigabe zurückgestellt.

Die vorläufige Entwicklerschnittstelle steht unter [Plugin-System](PLUGIN-SYSTEM.md); eine schrittweise Anleitung enthält [Dein erstes ScreenBuddy-Plugin](PLUGIN-TUTORIAL.md).

## Kompatibilität und offene Arbeit

Prüfgrenzen: Online-Plugins bleiben experimentell; CRL/OCSP-Sperrprüfungen und der vollständige UI-Import-/Consent-Ablauf sind nicht abgenommen. DPI-/Screenreader- und vollständige Funktionsparität sind nicht vollständig geprüft. EN-Anleitung mit ausdrücklich gekennzeichneten deutschen nativen Beispielbildern. Windows-Anmeldetest, WLAN, Launcher-/Boot-Modus und mehrere Android-Geräte bleiben offen.

## Prüfstatus

Am 26.10.2026 bestand der neu gebaute Entwicklungsstand 272 Kern-, 223 Manifest-Parser-, 127 Plugin-Speicher-, 134 Plugin-Transport-, 135 Plugin-Einstellungs-, 41 Plugin-Beispiel-, 11 Monitor- und 335 Companion-Protokollprüfungen sowie 27 native Einstellungs-, 16 Maus-, 16 Companion-Rebuild-, 39 isolierte App-, 49 Navigations- und zwei Installer-Prüfungen. Diese App-Läufe verwendeten `--no-lcd`, eigene Konfigurationen und lokale Testports. Die automatisierten Prüfungen belegen keine physische Ausgabe, keinen vollständigen externen Plugin-Ablauf, kein Zielsystem-Upgrade und keine vollständige visuelle/interaktive UI-Abnahme. Ein externer HTTPS-Beispielabruf bestand zuvor separat in einer isolierten Instanz. [Testprotokoll](../cpp/TESTPROTOKOLL.md).
