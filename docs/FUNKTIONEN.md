# ScreenBuddy · Funktionen im Detail

Aus dem README ausgelagert (Stand 2.7.0). Englisch: [FEATURES.en.md](FEATURES.en.md). Übersicht aller Dokumente: [docs/README.md](README.md).

## Inhalt

- [Systemseite per Touch und Update-Hinweis (ab 2.2.0)](#systemseite-per-touch-und-update-hinweis-ab-220)
- [Touch-Ziele und Spielstunden (ab 2.0.7)](#touch-ziele-und-spielstunden-ab-207)
- [Home Assistant (ab 2.1.0)](#home-assistant-ab-210)
- [Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)](#android-companion-app-ab-230-mehrere-geräte-ab-270)
- [Plugin-Store und Plugins (ab 2.6.0)](#plugin-store-und-plugins-ab-260)
- [Ansichten durchblättern (ab 2.0.4)](#ansichten-durchblättern-ab-204)

## Systemseite per Touch und Update-Hinweis (ab 2.2.0)

**Touch auf der Systemseite:** Bei aktivem Touch (nur bei Ausgabe auf einen Bildschirm) öffnet ein Tipp auf die Karte **RAM** den Task-Manager, auf **Datenträger** „Programme und Features“ (`appwiz.cpl`) und auf **Netzwerk** den Geschwindigkeitstest <https://www.speedtest.net/de> im Standardbrowser. Die CPU- und GPU-Karten reagieren nicht. Die Fenster öffnet Windows, meist auf dem Hauptbildschirm.

**Update-Hinweis:** Die App fragt höchstens einmal am Tag (frühestens 45 Sekunden nach dem Start) die öffentlichen GitHub-Releases ab, ohne Zugangsschlüssel und ohne Konto. Gibt es eine neuere, veröffentlichte Version, erscheint eine Sprechblase am Fuchs-Symbol (ein Klick öffnet die Einstellungen), und **Einstellungen → Start** zeigt „Neue Version … verfügbar“ mit Link zur Download-Seite. **Jetzt prüfen** fragt außerplanmäßig nach (höchstens alle 5 Minuten). Es wird nie etwas heruntergeladen oder installiert; zum Aktualisieren lädst du die neue EXE selbst herunter und startest sie. Abschalten: **Einstellungen → Start → Auf neue Versionen hinweisen** bzw. `update_check = false` unter `[ui]`. Gesendet wird nur eine gewöhnliche Abfrage mit dem Programmnamen und der Version im User-Agent; GitHub sieht dabei wie bei jedem Abruf deine IP-Adresse. Der Zeitpunkt der letzten Abfrage steht in `update-check.json` neben der `config.toml`.

## Touch-Ziele und Spielstunden (ab 2.0.7)

Bei aktivierter Touch-Steuerung öffnet ein Tipp auf einen BF6-Spieler sein GameTools-Profil im Browser. Die Rangliste zeigt zusätzlich Spielstunden. Ein Tipp auf einen Freund öffnet bevorzugt den Steam-Chat mit dieser Person (Steam-App nötig). Hat die Person kein Steam-Konto, öffnet sich ihr Discord-Profil (dort **Nachricht** wählen), sonst ihr Xbox-Profil auf xbox.com im Browser. Ist die App des Ziels nicht installiert, wird das nächste Ziel verwendet. Fenster und Tabs bestimmt die Plattform-App bzw. der Browser. [Bedienung, Voraussetzungen und Grenzen](TOUCH-2.0.7.md). [Prüfstand](../cpp/TESTPROTOKOLL.md).

## Home Assistant (ab 2.1.0)

Seite **Home Assistant** mit bis zu 4 Kacheln. **Sensoren** (auch Binärsensoren) zeigen Wert und Einheit. **Schalter** (Schalter, Licht, Ventilator, Eingabe-Schalter) zeigen An/Aus und lassen sich bei aktivem Touch per Tipp umlegen. Ohne Touch kannst du in den Einstellungen nur Sensoren wählen. Einrichtung unter **Einstellungen → Home Assistant**: Adresse (z. B. `http://homeassistant.local:8123`), ein langlebiges Zugriffstoken (Profil in Home Assistant → Sicherheit) und die Entitäten aus der Liste, die **Verbindung prüfen** direkt aus Home Assistant lädt. Das Token liegt wie die anderen Schlüssel DPAPI-verschlüsselt in `config.toml`. Die App fragt die REST-API alle 15 Sekunden ab; ein Tipp auf einen Schalter wirkt sofort. Schlösser, Rollläden und Alarmanlagen lassen sich bewusst nicht schalten. Bei einer Adresse mit `http://` laufen Token und Werte unverschlüsselt durchs Heimnetz; nutze dort `https://`, wenn Home Assistant es anbietet.

## Android-Companion-App (ab 2.3.0, mehrere Geräte ab 2.7.0)

Ein Smartphone oder Tablet dient als zusätzliche Anzeige parallel zum USB-/HDMI-Display. Die Companion-App liegt in Version 1.0.0 als separater Download vor. Seit 2.7.0 lassen sich mehrere Companions gleichzeitig betreiben, **synchron oder unabhängig**.

- **Eigene Seiten und Intervalle:** je Gerät eigene Seitenliste und Anzeigedauer (unter **Companion**); Datenquellen und Design bleiben gemeinsam.
- **Kopplung per QR-Code oder PIN:** im Reiter **Companion** auf **Gerät koppeln** klicken. Die Android-App scannt den QR-Code oder verbindet sich über Adresse, Port (8777) und 6-stelligen PIN; der SHA-256-Fingerabdruck sichert die TLS-Verbindung.
- **Touch und Gesten:** Wischen schaltet nur das jeweilige Gerät um. Interaktive Kacheln (Smart Home, Freunde, Systemaktionen) lassen sich per Fingertipp bedienen.
- **Native Auflösung und Drehung:** das Handy meldet Ausrichtung und Pixelmaße; der PC rendert passgenau.
- [Schritt-für-Schritt-Anleitung zur Kopplung](../ANLEITUNG.md#android-companion-app-koppeln-und-nutzen-ab-230) · [Einrichtung und Grenzen](COMPANION.md) · [Protokoll](COMPANION-PROTOKOLL.md)

Prüfstand: siehe [Release-Notizen 2.7](RELEASE-NOTES-2.7.md). Die automatisierten Mehrgeräteprüfungen sind Simulationen; adb-Gesten ersetzen keine physische Fingerprüfung. Der APK-Wechsel von der Vorschau 0.2.1 auf 1.0 erfordert wegen anderer Signatur eine Deinstallation.

## Plugin-Store und Plugins (ab 2.6.0)

Einstellungsseite **Plugin-Store** (neben Companion und Plugins, abschaltbar mit `[ui] plugin_store`): geprüfte Datenplugins aus dem signierten Store `PapaFuchs84/ScreenBuddy-Plugins` suchen, prüfen und installieren. Signierter Index und signierte Sperrliste (Ed25519, fester öffentlicher Schlüssel), Pakete per SHA-256 und Hostangaben geprüft, täglicher Hintergrundabgleich. Vor der Installation steht, an welche Hosts ein Plugin Daten senden würde. Installierte Plugins bleiben aus, bis du sie unter **Plugins** aktivierst; jeder Host braucht deine Freigabe. Seit 2.7.0 bleiben Store-Installationen und Updates auch ohne Netzwerkquellen ausgeschaltet; der lokale direkte Manifestimport verhält sich wie zuvor. Aktivierte Plugins lassen sich unter **Seiten & Reihenfolge** sortieren und ein-/ausblenden. Der Store zeigt signierte Vorschaubilder.

Weiter: [Plugin-System](PLUGIN-SYSTEM.md) · [Tutorial](PLUGIN-TUTORIAL.md) · [Store-Bedingungen](PLUGIN-STORE-BEDINGUNGEN.md) · [Beispiele](../cpp/examples/plugins/README.md). Online-Plugins und CRL/OCSP-Prüfungen bleiben experimentell.

## Ansichten durchblättern (ab 2.0.4)

Unter **Einstellungen → Übersicht** kannst du mit **Vorherige Ansicht** und **Nächste Ansicht** direkt durch die laufenden Display-Ansichten blättern. Die Position und die sichtbaren Seiten stehen unter der Live-Vorschau. Lange Listen blättern mit ihren Unterseiten, breite Displays wechseln wie bei der Touchbedienung um eine Kachelgruppe. Nach einem manuellen Wechsel beginnt die eingestellte Anzeigedauer neu; danach läuft der automatische Wechsel weiter. Bei keiner oder nur einer vollständig sichtbaren Ansicht sind die Knöpfe deaktiviert. Dafür ist kein Speichern oder Neustart nötig. Geänderte Seitenauswahl und Reihenfolge gelten nach **Speichern & anwenden**.
