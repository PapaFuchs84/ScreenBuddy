# Plugin-Store: Nutzungsbedingungen, Regeln und Datenschutz

© 2026 FuchsLabs · **Entwurf, juristisch nicht geprüft.** Vor der Veröffentlichung des Stores sollten Basti oder eine juristisch kundige Person die Texte prüfen. Offen ist auch der Verweis auf das Impressum des Betreibers (von Basti zu ergänzen). Englisch: [PLUGIN-STORE-TERMS.md](PLUGIN-STORE-TERMS.md).

## 1. Worum es geht

Der Plugin-Store ist eine Sammlung kleiner Datendateien (`*.flplugin.json`) für ScreenBuddy. Ein Plugin enthält **keinen Programmcode**. Die Plugins stammen von FuchsLabs oder von Dritten. **Drittanbieter-Plugins sind nicht von FuchsLabs**, auch wenn sie im Store stehen.

## 2. Für Nutzer

- Der Store ist **optional** und in den Einstellungen abschaltbar. Ohne Store funktionieren lokale Plugins weiter.
- Jedes Plugin zeigt vor der Installation, **an welche Hosts es Daten sendet**. Es bleibt nach der Installation ausgeschaltet. Jeder Host braucht Ihre Freigabe; bei einem Update werden Freigaben zurückgesetzt.
- Der Store wird „wie besehen“ bereitgestellt, **ohne Gewähr** für Verfügbarkeit, Eignung oder Richtigkeit von Plugins. Prüfen Sie die Nutzungsbedingungen von Diensten, die ein Plugin abfragt (z. B. Wetter-APIs).
- Siegel: **„Offiziell“** nur für Plugins von FuchsLabs, **„Geprüft“** nach Prüfung durch einen Maintainer, **„Community“** für Übrige. Ein Siegel ist keine Garantie.

## 3. Für Einreichende

1. **Lizenz:** Ihr Plugin steht unter **MIT** oder **CC0-1.0** (Feld `license`). Sie bestätigen, dass Sie Urheber sind oder alle nötigen Rechte an Texten und Inhalten haben.
2. **Inhalt:** keine Rechtsverstöße, kein Markenmissbrauch, keine Täuschung über Zweck oder Absender, **kein Tracking, keine Werbung oder Affiliate-Links** in Anzeigen, keine Daten Dritter ohne Berechtigung, keine Umgehung von Nutzungsbedingungen des abgefragten Dienstes.
3. **Marke:** Der Name FuchsLabs und das Logo dürfen nicht als Absender genutzt werden (siehe [TRADEMARKS.md](../TRADEMARKS.md)).
4. **Technik:** Das Plugin besteht `fl-plugin-validate --store`. Hosts müssen zum Zweck passen und in der Beschreibung genannt sein.
5. FuchsLabs darf Plugins **ablehnen, ändern lassen oder sperren** (signierte Sperrliste). Wiederholte Verstöße können zum Ausschluss des GitHub-Kontos führen.

## 4. Meldungen

Auffällige Plugins melden Sie über **Issues → „Plugin melden“**, bei akuter Gefahr privat wie in [SECURITY.md](../SECURITY.md). Ziel: Antwort innerhalb von 72 Stunden.

## 5. Datenschutz

- ScreenBuddy ruft den Store höchstens **einmal täglich** (und beim Klick auf „Store aktualisieren“) von der Adresse des Store-Repositorys ab (GitHub Pages). Dabei sieht der Betreiber des Hostings technisch Ihre IP-Adresse; FuchsLabs sendet **keine Nutzungsdaten, Kennungen oder Telemetrie** und speichert nichts über Sie.
- Plugin-Einstellungen und Zugangsdaten bleiben lokal (DPAPI-geschützt). Sie gehen nur an Hosts, die Sie freigegeben haben.
- Für Einreichungen über GitHub gelten die Datenschutzhinweise von GitHub; Ihr GitHub-Name erscheint als Autor.

## 6. Haftung

FuchsLabs haftet nicht für Drittanbieter-Plugins. Die gesetzliche Haftung bei Vorsatz und grober Fahrlässigkeit sowie für Verletzung von Leben, Körper und Gesundheit bleibt unberührt.
