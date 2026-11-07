# KI-Status (Entwicklung, noch nicht veröffentlicht)

[English](AI-STATUS.en.md)

Die optionale Seite `ai` zeigt auf 480 × 320 zwei Karten für Claude Code und Codex, den zuletzt beobachteten Zustand mit Dauer, Tool und Modell sowie bis zu drei Sitzungen. Reihenfolge: wartend, arbeitend, inaktiv; gleiche Priorität nach letztem Ereignis, dann stabil nach Anbieter/ID. Beendete Sitzungen blenden aus und behalten dabei ihre letzte Nummer (ab 1). Arbeitspunkte pulsieren, Wartepunkte blinken weich, Zeilen bewegen sich beim Wechsel. Alle Animationen laufen im vorhandenen Render-Takt, ohne eigene UI-Threads oder Sleeps. Der bestehende Standard `material-orange` ist dunkel mit Fuchs-Orange; die Seite respektiert die gewählte Anzeigevorlage.

## Schnellstart und Entfernen

1. In den nativen Einstellungen **KI-Status** aktivieren, unter **Seiten** für das gewünschte Display auswählen, speichern und ScreenBuddy neu starten. Der eigene HTTP-Empfänger verwendet Loopback (Standard 18765, konfigurierbar mit automatischem Fallback); die Einstellungsseite auf 8765 wird dafür nicht verwendet. Bei Belegung wird ein freier Port gewählt; den tatsächlichen Port zeigt die Statuszeile. Keine Firewallfreigabe nötig. Geräteeigene Companion-Seitenlisten bei Bedarf ebenfalls ergänzen.
2. **Einrichtung anzeigen** zeigt ausschließlich die eigenen Hook-Einträge vor und nach der Änderung für die Dateien `%USERPROFILE%/.claude/settings.json` und `%USERPROFILE%/.codex/hooks.json` sowie den Adapterpfad. Noch wird keine Hook-Konfiguration geändert. Andere Einstellungen, Umgebungsvariablen und fremde Hook-Befehle werden nicht in die Vorschau übernommen.
3. **Angezeigte Änderung anwenden** erzeugt den PowerShell-Adapter und einen zufälligen 256-Bit-Schlüssel unter `<Konfigurationsordner>/ai-status/`. `key.dpapi` ist über den vorhandenen Windows-DPAPI-Mechanismus an das Windows-Konto gebunden. Der Schlüssel steht weder im Hook-Befehl noch im JSON und wird nicht geloggt. Windows PowerShell 5.1 wird benötigt; die Ausführungsrichtlinie wird nicht umgangen.
4. Vorhandene Hook-Dateien werden vor der ersten Änderung einmalig als `.bak` gesichert. Andere Einträge bleiben erhalten. Zwischen Vorschau und Anwendung veränderte Dateien werden abgewiesen. Verwaiste reguläre `.screenbuddy-ai.tmp`-Dateien werden ersetzt; temporäre Symlinks (auch ohne Ziel) werden abgewiesen. Bei Schreib-/Ersetzungsfehlern wird die eigene Temp-Datei aufgeräumt. Die Ersetzung erfolgt je Datei atomar; die beiden Dateien bilden keine gemeinsame Transaktion. Bei Teilfehlern Vorschau erneut laden.
5. Clients neu starten und gegebenenfalls deren Hook-Vertrauensdialog bestätigen. Keine Freigaben oder Berechtigungsentscheidungen werden automatisch erteilt. Die Status-Hooks antworten ohne Inhalt mit Exitcode 0.

**Entfernung anzeigen → Angezeigte Änderung anwenden** entfernt nur Hook-Befehle, die exakt diesem Adapter und Anbieter entsprechen; fremde Befehle, auch in gemeinsam genutzten Gruppen, bleiben erhalten. Leere Ereignislisten dürfen bestehen bleiben. Adapter, verschlüsselter Schlüssel und `.bak` bleiben lokal. Danach die Seite deaktivieren, speichern und ScreenBuddy neu starten, um den Empfänger zu beenden. Nach einem Verschieben des Konfigurationsordners alte Befehle zuerst über den alten Adapterpfad entfernen. Sicherungen können weitere private Einstellungen enthalten.

## Lokales Ereignisprotokoll

`POST http://127.0.0.1:<actual-port>/events` (Standard/default 18765), Header `X-ScreenBuddy-AI-Key: <lokaler Schlüssel>`, JSON, maximal 8192 Bytes. Keine CORS-Freigabe; Requests mit `Origin` oder `Sec-Fetch-Site` werden abgewiesen. Keine Ereignisdateien oder Transkripte werden gespeichert. Status geht beim App-Neustart verloren; bis zum nächsten Hook gibt es keine laufenden Sitzungsdaten.

Beispiel mit ausschließlich erfundenen Daten (Schlüssel bewusst nicht enthalten):

```json
{
  "provider": "claude",
  "session_id": "example-1",
  "hook_event_name": "PreToolUse",
  "tool_name": "Read",
  "model": "example-model",
  "context": 42,
  "five_hour": 23,
  "five_hour_reset": 1800000500
}
```

Anbieter: `claude` oder `codex`. Unterstützte Ereignisse: `SessionStart` → idle, `UserPromptSubmit`/`PreToolUse`/`PostToolUse`/`PostToolUseFailure` → working, `PermissionRequest` → waiting, `Stop`/`Interrupt` → idle, `SessionEnd` → off. Ein spätes Tool-/Stop-Ereignis nach SessionEnd reaktiviert keine Sitzung; ein neuer SessionStart darf dies. Tool wird bei Prompt, Stop, Interrupt und Ende gelöscht. Es gibt höchstens 128 gespeicherte Sitzungen; freie Plätze werden zuerst aus beendeten Sitzungen gewonnen. Ohne End-Hook bleibt der letzte beobachtete Zustand erhalten; die Fußzeile zeigt sein Alter. Es gibt keine erfundene Timeout-Erkennung.

Der mitgelieferte Adapter reicht nur Sitzungs-ID, Ereignis, Toolname und (falls im Hook enthalten) Modell weiter. Er liest keine Transkript- oder Anmeldedateien; im Hook-Eingang enthaltene Prompts und Tool-Argumente werden nicht ausgewertet oder weitergegeben. Bei Empfangsproblemen wartet er höchstens ungefähr 1,2 Sekunden auf HTTP, damit die Arbeit fortgesetzt werden kann. Hooks selbst haben ein Limit von drei Sekunden. Unter einer restriktiven PowerShell-Richtlinie oder einer rein cloudbasierten Codex-Sitzung können lokale Befehls-Hooks ausbleiben. Adapterpfade mit Shell-Sonderzeichen werden abgewiesen.

Optionale Messwerte müssen andere autorisierte lokale Produzenten explizit liefern: `context`, `five_hour`, `week` als Prozent 0–100; `five_hour_reset`, `week_reset` als Unix-Sekunden; `cost_today` als anbieterglobaler heutiger USD-Schätzwert; `tokens_today` als anbieterglobaler heutiger ganzzahliger Tokenstand. Keine Token-Deltas senden: die Werte werden ersetzt, nicht addiert. Fehlende Felder erhalten den letzten gemeldeten Wert. Tageswerte werden beim lokalen Datumswechsel ungültig; sie sind nicht persistent. Ohne Meldung steht **n/a**, niemals ein geschätzter Limitverbrauch oder ein erfundener Preis. Limits und Kontext werden durch die Standard-Hooks nicht automatisch ermittelt. ScreenBuddy liest keine Anmeldedateien, Preislisten oder Transkripte. Die optionale Kontonutzung über den Anbieter-Client ist unten beschrieben.

## Kachelquellen

Die integrierte, nur lesbare Quelle `ai` steht in deklarativen Plugin-/Zonen-Templates ohne HTTP-Quelle zur Verfügung, z. B. `{{ai.claude.state}}`, `{{ai.codex.tokens_today}}` oder `{{ai.claude.s1.tool}}`. `ai` ist als Quell-ID reserviert und darf nicht durch ein Plugin überschrieben werden. Direkte AI-Referenzen in Links sind verboten.

| Je `ai.claude` / `ai.codex` | Inhalt |
| --- | --- |
| `state`, `tool`, `model`, `state_seconds` | Höchste aktive Priorität; ohne aktive Sitzung letzter bekannter off-Zustand, sonst off mit unbekannter Dauer |
| `context`, `five_hour`, `week` | Prozent oder `null` |
| `five_hour_reset`, `week_reset` | Unix-Sekunden oder `null` |
| `cost_today`, `tokens_today` | Gemeldeter Tageswert oder `null` |
| `s1`, `s2`, `s3` | Je Anbieter bis zu drei aktive Sitzungen mit state/tool/model/state_seconds, sonst `null`; keine privaten Sitzungs-IDs |

Die Standardseite und ihre bestehenden Overlays behalten ihre Geometrie; Standardseite und Builtin-Renderpfad nutzen dieselben Zustandsdaten, und der Builtin-Katalog führt KI. Eigene Ansichten verwenden denselben Renderer für Anbieter-/Limits-Kacheln; siehe [Phase 8 P8-4](../docs/PHASE8-EIGENE-ANSICHTEN.md). Die KI-Quelle wird separat aktiviert.

## Prüfstatus und Quellen

`fl-tests` enthält Parser-, Zustands-, Sortierungs-, Begrenzungs-, Hook-Datei-, DPAPI-, HTTP-Authentifizierungs- und Render-Prüfungen. Dateien liegen ausschließlich unter einem eindeutigen temporären Verzeichnis; der Testempfänger wählt einen freien Loopback-Port im dynamischen Bereich. `fl-tests ai-status <Ausgabeordner>` erzeugt DE/EN-Beispielbilder. Den tatsächlich ausgeführten Build und Prüfstand nennt [ai-status-report.md](ai-status-report.md). Echte Client-Hooks, physisches Display und Mehrgerätebetrieb sind gesondert abzunehmen.

Hook-Formate nach [Anthropic: Hooks reference](https://code.claude.com/docs/en/hooks) und [OpenAI Docs: Hooks](https://learn.chatgpt.com/docs/hooks), geprüft am 2026-11-07. Eigenständige Implementierung ohne libre-panel-/GPL-Code.

## Nutzungsabfrage und echte Konten (Opt-in)

Vorhandene getrennte Schalter **Echtes Konto lesend nutzen** (ai.codex_real_account / ai.claude_real_account), Standard AUS. Automatische Nutzungsabfrage einschalten, Einwilligung lesen, Aktivierung bestätigen, speichern und neu starten. Ausschalten pausiert den gesamten Worker bis Neustart; laufende HTTPS-Aufrufe enden innerhalb ihrer Zeitgrenzen, ihre Ergebnisse werden verworfen. Hooks bleiben separat zu bestätigen.

**Risiko:** Inoffizielle Schnittstellen, können sich ändern; Anbieter-Duldung unklar. Nur lesende GET-Abfragen ohne Modellnachricht. Token verlassen den Rechner ausschließlich per HTTPS zum jeweiligen Anbieter. Keine Speicherung, Anzeige, Protokollierung oder Erneuerung der Token. Login-Dateien werden nie geschrieben; Refresh-Token werden nicht verwendet. Nur Windows-Dateien, kein Keychain-Zugriff.

- Claude: ausschließlich %USERPROFILE%/.claude/.credentials.json, Feld claudeAiOauth.accessToken; GET https://api.anthropic.com/api/oauth/usage, Bearer und anthropic-beta: oauth-2025-04-20. five_hour/seven_day mit utilization und ISO-8601-Reset. Optionale Modellfenster und extra_usage werden toleriert, aber nicht als Gesamtlimit oder Tageskosten ausgegeben.
- Codex: ausschließlich %USERPROFILE%/.codex/auth.json, Feld tokens.access_token, optional tokens.account_id als ChatGPT-Account-Id; GET https://chatgpt.com/backend-api/wham/usage. Fenster nach limit_window_seconds: 18000 = 5h, 604800 = Woche, unabhängig von primary/secondary. secondary_window darf null sein, Prozentwerte werden auf 0–100 begrenzt. API-Keys ersetzen keinen Login-Token.
- Fehlende Datei: Nicht angemeldet. Ungültige/unvollständige Datei: lesbarer Grund. Abgelaufener Claude-Zeitstempel, abgelaufener Codex-JWT oder HTTP 401: Token abgelaufen: Claude/Codex einmal starten. Fehler löschen direkte Limitwerte auf n/a.

Ein Hintergrund-Worker fragt Anbieter unabhängig frühestens alle 3 Minuten ab; jeder Fehler (auch 401/403/429 oder fehlende Anmeldung) bewirkt für diesen Anbieter 15 Minuten Pause. Umschalten umgeht die Pause nicht. Im Konto-Modus startet kein Anbieterprogramm. Ohne Konto-Einwilligung bleibt der bisherige isolierte CLI-Pfad erhalten, ohne Benutzer-Login-Dateien. Hooks/Claude-Statuszeile liefern weiter Kontext und Sitzungskosten. Keine erfundenen Tageswerte.

HTTPS-Hosts/Pfade fest, alle Weiterleitungen gesperrt; Windows-Zertifikats-/Hostprüfung aktiv. Kein Proxy, keine Cookies oder automatische HTTP-Anmeldung. Zeitgrenzen: 3 Sekunden je DNS-/Verbindungs-/Sende-/Empfangsschritt, zusätzlich 12 Sekunden Body-Leseabschnitt. Maximal 64 KiB Antwort und 256 KiB Login-Datei. Eigene Credential-/Parser-/Headerpuffer werden auch auf Fehlerpfaden beim Freigeben überschrieben. Betriebssystem-/WinHTTP-interne Kopien liegen außerhalb der eigenen Pufferverwaltung.

ai.port ist in Config und KI-Einstellungen konfigurierbar: Standard 18765, 1024–65535, Einstellungsport ausgeschlossen. Bei Belegung freier dynamischer Fallback-Port. Statuszeile der Einstellungsseite und gemeinsamer Hook-/Claude-Statuszeilenadapter verwenden den tatsächlichen Port. DPAPI-/Dateifehler halten den Empfänger inaktiv.

Prüfzahlen und offene Konto-/UI-/Hardware-Abnahme: [Bericht](ai-status-report.md). Dieser Arbeitsgang nutzt nur synthetische Antworten und Temp-Dateien, keine echten Logins oder Anbieter-Netzwerkaufrufe.

## Editor-Kacheln (P8-4)

Eigene Ansichten bieten `ai.claude`, `ai.codex`, `ai.limits` und einzelne Messwerte. Öffentliche Bedingungen, Dichte, Geräte-Layouts und Seiten-Touch-Ziele verwenden den bestehenden Kachelvertrag. Datenquelle ist die Template-Quelle `ai` (siehe oben); vorhandene KI-Builtin-Blöcke und deren Renderpfad bleiben erhalten. Fehlende Daten bleiben leer; fehlende Limits bleiben `n/a`. Siehe [Bedienung, Prüfstand und Portkonflikt](../docs/PHASE8-EIGENE-ANSICHTEN.md).
