# ScreenBuddy: Hilfe und Support

ScreenBuddy zeigt PC-Informationen auf kompatiblen USB-Anzeigen, einem zusätzlichen Windows-Monitor oder Android-Geräten mit der separat installierten Companion-App. Windows- und Companion-Versionen haben getrennte Versionsnummern.

## Einstieg

- [Anleitung und Quellcode](https://github.com/PapaFuchs84/ScreenBuddy)
- [Downloads und Versionshinweise](https://github.com/PapaFuchs84/ScreenBuddy/releases)
- [Probleme melden](https://github.com/PapaFuchs84/ScreenBuddy/issues)
- E-Mail: [fuchslabs2026@gmail.com](mailto:fuchslabs2026@gmail.com)
- [App-Datenschutz](https://fuchslabs.duckdns.org/screenbuddy/privacy/)

## Häufige Fragen

**Welche Windows-Version wird benötigt?** Das Store-Paket ist für x64-Windows ab Build 19041 ausgelegt. Ein zusätzlicher Bildschirm oder ein kompatibles USB-Panel ist nur für den jeweiligen Ausgabeweg nötig. Nicht jedes USB-Display wird unterstützt. Android-Ausgabe benötigt die Companion-App und eine Verbindung zum PC, etwa WLAN oder USB-Tethering; sie ist kein Bestandteil des Windows-MSIX.

**Wo sind meine Daten?** Die Store-Version speichert ihre Daten im Paketordner `LocalState`. Die Standalone-Version behält ihren separaten Datenordner. Vor einem Wechsel beide Apps beenden. Der dokumentierte Import kopiert ausgewählte Daten ausdrücklich und überschreibt keine vorhandenen Zieldateien; die Quelle bleibt erhalten. Keine vollständigen Konfigurationsdateien öffentlich anhängen.

**Updates und Autostart?** Store-Updates liefert Microsoft nach deinen Windows-/Store-Einstellungen. Den Autostart aktiviert der Benutzer in ScreenBuddy; Windows-Richtlinien können ihn verhindern. Die Registry-Autostart-Einstellung der Standalone-Version wird nicht automatisch umgestellt.

**USB-Anzeige bleibt leer?** Kompatibilität, Kabel und den ausgewählten COM-Port prüfen. Hersteller-Software kann den Anschluss belegen. Die Store-App kann den laufenden Hersteller-Prozess mit deinen vorhandenen Benutzerrechten beenden; dessen Autostart verwaltest du außerhalb von ScreenBuddy über die Windows-Aufgabenplanung, bei Bedarf mit einem Administrator.

**Companion verbindet sich nicht?** PC-Adresse und erreichbaren Port, Netzwerk/Tethering, Firewall und die richtige Kopplung prüfen. Keine Kopplungscodes veröffentlichen. Nach einem Gerätewechsel ggf. die alte Verbindung in der Companion-App vergessen und neu koppeln. Nicht ungefragt Windows-Daten oder Android-App-Daten löschen.

**FPS steht auf n/a?** Game-Mode aktivieren und eine passende Ansicht wählen. ETW benötigt die passenden Windows-Rechte; „FPS-Berechtigung prüfen“ beschreibt den Weg über einen Administrator und die Gruppe „Leistungsprotokollbenutzer“, danach ab-/anmelden. Die Store-App ändert keine Gruppen selbst. ScreenBuddy vor einem Anti-Cheat-Spiel starten. Bereits vorhandene Rechte oder lesend bereitgestellte HWiNFO-PresentMon-Werte können genutzt werden; nicht jede Messquelle ist immer verfügbar.

**Drittanbieter oder KI-Werte fehlen?** Die betreffende Integration und deren Einrichtung prüfen. Anbieter können eigene Konten/Schlüssel verlangen und ihre Schnittstellen ändern. KI-Kontozugriff ist freiwillig, separat zu bestätigen und abschaltbar. Fehlende Werte sind `n/a`, keine geschätzten Livewerte.

**Companion-Automatik (ab 1.1.0)?** Erst koppeln und die manuelle Anzeige prüfen. Die optionale Automatik sucht den PC lokal per mDNS bzw. TCP-Erreichbarkeit. Android kann Hintergrundstarts und Alarme verzögern. Benachrichtigungen und gegebenenfalls „Über anderen Apps anzeigen“ in den Android-Einstellungen prüfen. Die optionale Geräteadmin-Freigabe für Bildschirmsperren gibt es nur auf unterstützten Handys; sie kann vor einer Deinstallation in Android zu widerrufen sein. Keine Firewall-/Sperreinstellungen pauschal abschalten. Details stehen in der aktuellen [Companion-Anleitung](https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.md).

## Eine hilfreiche Fehlermeldung senden

Nenne Windows-Build, ScreenBuddy-Version, Store oder Standalone, Ausgabeweg/Gerätemodell, genaue Schritte und erwartetes/tatsächliches Verhalten. Bei Companion zusätzlich dessen App-Version und Android-Version nennen. Verwende Beispieldaten für Bilder. Entferne Namen, private URLs, Tokens, Passwörter, Kopplungscodes und persönliche Inhalte aus Protokollen und Screenshots. Öffentliche GitHub-Issues sind öffentlich sichtbar; sende vertrauliche Angaben nicht dorthin.

Support ist ein privates Projektangebot ohne zugesagte Reaktionszeit. Bei Datenschutzanfragen nutze die Kontaktadresse und die [App-Datenschutzerklärung](https://fuchslabs.duckdns.org/screenbuddy/privacy/).
