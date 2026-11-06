# ScreenBuddy · Clean-Room-Herkunft Zonen-Layout

Stand: 2026-11-06. Diese Datei dokumentiert die Herkunft der Ideen und die Arbeitsweise für das Zonen-Layout-System. Sie wird pro Phase fortgeschrieben.

## 1. Zweck

Das Zonen-Layout-System erlaubt, Seiten in benannte Zonen mit Blöcken aufzuteilen, Blöcke in Dichtestufen darzustellen und Rahmenstile über das Theme zu steuern. Es ist für ScreenBuddy 2.10.0 (vorgeschlagen) geplant und führt das Manifest `schema_version: 2` ein. Manifest v1 bleibt unverändert lauffähig.

## 2. Quelle der Idee

- Grundlage sind Konzeptvorschläge aus einer externen Recherche zu libre-panel.
- libre-panel steht unter GPL-3.0-or-later. ScreenBuddy steht unter MIT.
- Übernommen werden ausschließlich Ideen und Konzepte. Kein Code, keine Dateien, Themes, Assets, Texte, Namen oder Schlüssellisten aus libre-panel.

## 3. Regeln

1. Implementierende Agenten erhalten nur den Umsetzungsplan `PLAN-screenbuddy-zonen-layout.md` ohne den Abschnitt zur Fremdumsetzung sowie den ScreenBuddy-Code.
2. Implementierende Agenten erhalten keinen libre-panel-Quellcode.
3. Eigene Begriffe: Zone, Block, Dichtestufe, Wechselfeld, Rahmenstil, Geräteprofil.
4. Eigenes Manifest-Schema mit `schema_version: 2`.
5. Der Codex-Review prüft jede Phase auf auffällige Ähnlichkeit in Namen, Schlüsseln und Struktur.

## 4. Beteiligte

- Auftraggeber: Basti
- Autoren: Claude-Agenten (Sonnet, Opus, Haiku)
- Review: Codex CLI 0.166.0

## 5. Erklärung

Die Umsetzung erfolgt ohne Einsicht in Fremdcode implementiert.

## 6. Fortschreibung pro Phase

| Phase | Datum | Agent | Bestätigung | Codex-Ähnlichkeitsprüfung |
|---|---|---|---|---|
| 0 Vorbereitung | 2026-11-06 | Claude Haiku 5.5 | ohne Einsicht in Fremdcode implementiert | ausstehend |
| 1 Rahmenstil an Theme anbinden | 2026-11-06 | Claude Sonnet 5.5 | ohne Einsicht in Fremdcode implementiert | ausstehend |
| 2 Plugin-Fundament: Zonen, Block-Schnittstelle, Dichtestufen | | | | |
| 3 Bedingte Anzeige und Fehlerisolierung | | | | |
| 4 Geräteprofile, Wechselfeld, Tap-Ziele | | | | |
| 5 Eingebaute Seiten und gemeinsamer Renderer | | | | |
| 6 Editor und Erststart | | | | |
