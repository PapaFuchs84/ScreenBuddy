# ScreenBuddy Plugin-Plattform und Pluginstore – Plan

© 2026 FuchsLabs · Status: **Entwurf zur Freigabe** (30.10.2026), noch nichts gebaut.

## 1. Ausgangslage

Seit 2.5 gibt es ein **deklaratives** Pluginsystem: Plugins sind reine JSON-Manifeste (`*.flplugin.json`, Schema 1, max. 128 KiB), enthalten **keinen ausführbaren Code**. Die App holt Daten selbst (nur HTTPS, öffentliche IPs, Host-Zustimmung durch den Nutzer, Geheimnisse per DPAPI) und rendert Widgets (`text`, `metric`, `bar`, Listen). Installieren geht lokal und offline (`plugin_store`).

**Das ist der größte Sicherheitsvorteil:** Ein Store mit Nutzer-Uploads ist hier ein Store für *Daten*, nicht für Programme. Diese Eigenschaft wird zum Grundsatz der Plattform.

## 2. Leitprinzipien

1. **Kein Code im Store.** Plugins bleiben deklarativ. Skripting (WASM o. Ä.) nur als spätere, getrennt freigegebene Stufe.
2. **Nutzer entscheidet.** Jede Netzwerkverbindung und jede Berechtigung braucht Zustimmung, wie heute.
3. **Store ist optional.** Ohne Store (nur lokale Dateien) bleibt alles wie in 2.5. Abschaltbar, keine Telemetrie.
4. **Günstig betreibbar.** Start ohne eigenen Server.

## 3. Architektur: Store auf GitHub (empfohlen) statt eigener Server

| | A: GitHub-basiert (empfohlen) | B: Eigener Server |
|---|---|---|
| Kosten | 0 € | ca. 5–15 €/Monat plus Pflegeaufwand |
| Uploads | Pull Request ins Repo `ScreenBuddy-Plugins` oder Einreichungs-Formular (Issue-Vorlage) | Upload-Formular, Konten, Login |
| Review | PR-Review + automatische Prüfung (GitHub Actions) | selbst bauen |
| Index | `index.json` + `index.json.sig` per GitHub Pages / Raw-CDN | Datenbank + API |
| Datenschutz | Nutzer brauchen kein Konto, App lädt nur Index | Konten, DSGVO-Pflichten |
| Risiko | GitHub-Abhängigkeit, Nutzer brauchen GitHub-Konto zum Einreichen | Missbrauch, Spam, Haftung, Betrieb |

**Empfehlung:** Variante A. Später, wenn der Store wächst, kann ein Upload-Formular (Cloudflare Worker, Gratis-Kontingent) PRs automatisch anlegen, damit Einreichende kein GitHub-Wissen brauchen. Variante B erst, wenn Bewertungen/Konten wirklich gebraucht werden.

Ablauf: Autor reicht Manifest ein → CI validiert (gleicher Parser wie die App, Größen, Hosts, Lizenz, Vorschaubild) → Maintainer-Review → Merge → CI baut `index.json`, signiert es → App lädt Index (max. 1×/Tag, wie Update-Prüfung).

## 4. Sicherheit

- **Signierung:** Der Index und jedes Manifest-Hash wird mit einem FuchsLabs-Schlüssel (Ed25519) signiert; Public Key fest in der App. Signaturschlüssel offline/als GitHub-Secret, Rotation dokumentiert. Die App prüft Signatur *und* SHA-256 vor Installation.
- **Review (Pflicht vor Aufnahme):** automatisch (Schema, Größen, nur HTTPS, keine privaten/lokalen Hosts, keine URL-Weiterleitung, keine Geheimnisse im Manifest, Secret-Scanning) und manuell (Zweck passt zur Beschreibung, Host plausibel, Lizenz). Codex/Agenten dürfen vorprüfen, Freigabe bleibt beim Menschen.
- **Berechtigungen:** Manifest listet Hosts und Einstellungen offen auf. Store-UI zeigt vor Installation „Dieses Plugin sendet Daten an: api.example.com“. Aktivierung weiterhin standardmäßig **aus**, Host-Zustimmung je Host.
- **Sandbox:** Durch deklaratives Format bereits gegeben (kein Dateizugriff, kein Code). Zusätzlich: Limits für Quellen, Antwortgröße, Aktualisierungsintervall (Untergrenze), Anzahl Plugins.
- **Updates:** Neue Version, die **neue Hosts oder Berechtigungen** verlangt, wird nicht stillschweigend übernommen, sondern erneut zur Zustimmung vorgelegt (Fingerprint wie heute).
- **Widerruf:** Signierte Sperrliste (`revoked.json`); App deaktiviert gesperrte Plugins beim nächsten Abgleich.
- **Meldeweg:** „Plugin melden“-Knopf (öffnet vorbefülltes GitHub-Issue), SECURITY.md um Plugin-Meldungen ergänzen.

## 5. Versionierung und API-Stabilität

- `schema_version` bleibt die Kompatibilitätsachse. Neues Schema = abwärtskompatibel erweitern, Schema 1 bleibt dauerhaft lesbar (mindestens bis ScreenBuddy 4.0, Ankündigung 2 Releases vorher).
- Manifest bekommt `min_app_version` und optional `requires` (Widget-Typen/Fähigkeiten). Nicht unterstützte Plugins werden angezeigt, aber ausgegraut mit Hinweis.
- Plugin-Versionen: SemVer. Der Index hält pro Plugin die aktuelle und die letzte stabile Version.
- Schema-Dokumentation und Beispiele versioniert in `docs/` (DE/EN), Tutorial bleibt Referenz.
- Conformance-Tests: die Beispiel-Plugins laufen in CI gegen jede neue App-Version.

## 6. Store-UI in der App (native, FuchsLabs-CI)

- Neuer Reiter **Plugins → Store** in der nativen Oberfläche: Suche, Kategorien (System, Spiele, Smart Home, Wetter/Info, Sonstiges), Sortierung (neu, beliebt, empfohlen).
- Karte je Plugin: Name, Autor, Version, Vorschau (480×320 Beispielbild aus den `sample`-Daten, in der App gerendert, nicht vom Autor hochgeladen), Hosts, Lizenz, Prüfstatus-Siegel („Geprüft“, „Community“).
- Installieren in zwei Klicks, Berechtigungsdialog, danach in der bestehenden Plugin-Liste. Updates mit Changelog, nie automatisch ohne Hinweis.
- Offline-Verhalten: gecachter Index, klare Meldung. Zweisprachig DE/EN.
- Touch-tauglich (Display/Companion nur Anzeige; Store ist reine PC-Oberfläche).

## 7. Moderation und Lizenzen

- **Vertrag für Einreichende:** Plugin unter **MIT** oder CC0 (Manifest-Feld `license`, SPDX-Wert Pflicht, nur Whitelist). Autor bestätigt Urheberschaft und Rechte an Texten/Bildern.
- **Content-Regeln:** keine Rechtsverstöße, Markenmissbrauch, Täuschung, Tracking, Werbung/Affiliate in Anzeigen, keine Daten Dritter ohne Berechtigung, API-Nutzungsbedingungen des Anbieters einhalten (Hinweis wie beim Wetter-Beispiel).
- **Moderation:** Maintainer-Team (zunächst Basti). Meldungen via Issue, Reaktion Ziel 72 h, Sperrung über Sperrliste. Wiederholungstäter blockierbar (GitHub-Account).
- **Haftung/Recht:** Store-Nutzungsbedingungen und Datenschutzhinweis (kurz, DE/EN), Impressum-Verweis. „Keine Gewähr“-Klausel; Drittanbieter-Plugins sind nicht von FuchsLabs. Rechtsprüfung der Texte durch Basti/Jurist empfohlen.
- **Marke:** Name FuchsLabs/Logo bleiben per TRADEMARKS.md geschützt; Plugins dürfen sie nicht als Absender nutzen („Offiziell“-Siegel nur FuchsLabs).

## 8. Phasen

| Phase | Inhalt | Aufwand |
|---|---|---|
| 0 | Entscheidungen (siehe 9), Repo `ScreenBuddy-Plugins` anlegen, Einreichungs-Regeln | klein |
| 1 | Plugin-Paketformat + Manifest-Erweiterung (`license`, `author`, `min_app_version`, `category`), CLI/CI-Validator (Parser der App wiederverwenden) | mittel |
| 2 | Signierung: Schlüssel, Index-Build per Actions, Signaturprüfung in der App, Sperrliste | mittel |
| 3 | Store-UI nativ: Index laden, Liste, Detail, Installieren, Updates | groß |
| 4 | Moderation, Nutzungsbedingungen, Meldeweg, Doku/Wiki/READMEs DE/EN, 5–10 Start-Plugins (eigene) | mittel |
| 5 | Beta mit wenigen Testern, dann Release (z. B. ScreenBuddy 2.6) nach Bastis Ja | klein |
| später | Upload-Formular ohne GitHub-Konto, Bewertungen, Skript-Plugins (WASM) nur nach eigenem Sicherheitskonzept | offen |

## 9. Entscheidungen für Basti

1. **Hosting:** GitHub-basiert (empfohlen) oder eigener Server?
2. **Einreichung:** zunächst nur per GitHub-PR/Issue (empfohlen) – oder gleich Formular?
3. **Lizenz der Plugins:** MIT/CC0 Pflicht (empfohlen)?
4. **Umfang erste Version:** nur deklarative Plugins (empfohlen), kein Skript-Support?
5. **Release:** Store als ScreenBuddy 2.6?

## 10. Umsetzung mit Agenten (nach Freigabe)

Nacheinander, sparsam: Opus für Signierung/Sicherheitsdesign, Sonnet für Code (Validator, Store-Logik, UI), Haiku für Doku/Wiki, Codex für unabhängiges Sicherheits- und Code-Review jeder Phase. Tests nie gegen Bastis laufende App/Port 8765.

## 11. Risiken

Missbrauch bleibt durch Datenformat begrenzt (Datenabfluss an Fremd-Hosts ist das Hauptrisiko → Host-Anzeige + Zustimmung + Review). Moderationsaufwand wächst mit Beliebtheit. Rechtliche Texte brauchen Prüfung. Schlüsselverlust → Rotationsplan nötig.
