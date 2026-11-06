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
| 2 Plugin-Fundament: Zonen, Block-Schnittstelle, Dichtestufen (Teil a: Schema, Parser, Geometrie, Dichtestufen-Modell) | 2026-11-06 | Claude Opus 5.5 | ohne Einsicht in Fremdcode implementiert. Gelesen: Plan-Zeilen 1–50 (Kopf, Leitplanken, Clean-Room-Notiz, Phasen 0–2, Überschrift Phase 3) und ScreenBuddy-Code; nicht gelesen: Abschnitt „Wie libre-panel es macht“, `REA-libre-panel-vorschlaege.md`, libre-panel-Code. Schlüssel (`layout`, `columns`, `rows`, `zones`, `column_span`, `row_span`, `zone`, `id`) selbst gewählt ohne Abgleich mit Fremdcode | ausstehend |
| 2 Plugin-Fundament (Teil b: Zeichnen nach Zonen, Beispiel, Doku) | 2026-11-06 | Claude Sonnet 5.5 | ohne Einsicht in Fremdcode implementiert. Gelesen: der gesamte Plan PLAN-screenbuddy-zonen-layout.md (Zeilen 1-131), und zwar die bereinigte Fassung: Das Plan-File enthält den Abschnitt „Wie libre-panel es macht“ nicht (Zeile 20 verweist nur darauf); es wurde kein libre-panel-Quellcode und kein Fremdabschnitt gelesen. Außerdem ScreenBuddy-Code (plugin_zones.*, plugin_render.cpp, gfx.*, plugins.*, Tests, Doku); kein libre-panel-Code, keine Fremddateien. Zeichenlogik, Dichtestufen-Schwellen und Beispiel selbst aus der ScreenBuddy-Zeichengeometrie abgeleitet | ausstehend |
| 3 Bedingte Anzeige und Fehlerisolierung | 2026-11-06 | Claude Opus 5.5 | ohne libre-panel-Einsicht implementiert. Gelesen: PLAN-screenbuddy-zonen-layout.md (bereinigte Fassung ohne Abschnitt „Wie libre-panel es macht“) und ScreenBuddy-Code (plugins.*, plugin_render.*, plugin_zones.*, gfx.*, util.*, Aufrufstellen in app.cpp, Tests, Doku); kein libre-panel-Code, keine Fremddateien, nicht `REA-libre-panel-vorschlaege.md`. Schlüssel (`show_if`, `path`, `op`, `value`), Wertsemantik, Grenzen und Fehlerisolierung selbst gewählt ohne Abgleich mit Fremdcode | ausstehend |
| P4a-1 | 2026-11-06 | Codex CLI 0.166.0 | Geräteprofil ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4a-2 | 2026-11-06 | Codex CLI 0.166.0 | Gerätemaske und gerätegebundene Touch-Koordinaten ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4a-3 | 2026-11-06 | Codex CLI 0.166.0 | Einstellungen Sperrzone DE/EN ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4a-4 | 2026-11-06 | Codex CLI 0.166.0 | Android-TV nur Querformat ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4b-0 | 2026-11-06 | Codex CLI 0.166.0 | ohne Einsicht in Fremdcode implementiert; nur Entwurf und ScreenBuddy-Code. | ausstehend |
| P4b-1 | 2026-11-06 | Codex CLI 0.166.0 | ohne Einsicht in Fremdcode implementiert; nur Entwurf und ScreenBuddy-Code. | ausstehend |
| P4b-2 | 2026-11-06 | Codex CLI 0.166.0 | ohne Einsicht in Fremdcode implementiert; nur Entwurf und ScreenBuddy-Code. | ausstehend |
| P4b-3 | 2026-11-06 | Codex CLI 0.166.0 | ohne Einsicht in Fremdcode implementiert; nur Entwurf und ScreenBuddy-Code. | ausstehend |
| P4b-4 | 2026-11-06 | Codex CLI 0.166.0 | ohne Einsicht in Fremdcode implementiert; nur Entwurf und ScreenBuddy-Code. | ausstehend |
| Phase-4b-Reviewkorrekturen | 2026-11-06 | Codex | ohne Einsicht in Fremdcode implementiert; nur ScreenBuddy-Code und Phase-4-Entwurf gelesen. | ausstehend |
| P4c-1 | 2026-11-06 | Codex CLI 0.166.0 | Parser und kodierte Tap-Expansion ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4c-2 | 2026-11-06 | Codex CLI 0.166.0 | Ziele im Zeichenaufruf und gemeinsame Listenzeilen ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4c-3 | 2026-11-06 | Codex CLI 0.166.0 | Separate persistente Linkfreigabe und DE/EN-Schalter ohne Einsicht in Fremdcode implementiert. | ausstehend |
| P4c-4 | 2026-11-06 | Codex CLI 0.166.0 | Snapshot-Entscheidung, Ruhezeit, Rate und gesicherte Next-Liste ohne Einsicht in Fremdcode implementiert. | ausstehend |
| 4 Geräteprofile, Wechselfeld, Tap-Ziele | | | | |
| P5-0 Baseline-Gerüst | 2026-11-06 | Codex CLI 0.166.0 | Vollständigen PHASE5-MIGRATIONSPLAN.md und ScreenBuddy-Code gelesen; keine Einsicht in libre-panel-Code oder andere Fremdimplementierungen. Snapshot-Matrix, Zeitnahtstellen und Beispieldatenmodul aus eigener Repo-Geometrie abgeleitet. Noch keine Migration. | ausstehend |
| 5 Eingebaute Seiten und gemeinsamer Renderer | | | | |
| P5-1 Seitenrahmen + RSS | 2026-11-06 | Codex CLI 0.166.0 | Vollständigen PHASE5-MIGRATIONSPLAN.md und ScreenBuddy-Code gelesen; RSS mechanisch auf eigenen Seitenrahmen und Block-Schnittstelle umgestellt, ohne Einsicht in libre-panel-Code oder andere Fremdimplementierungen. | ausstehend |
| P5-2 Streaming | 2026-11-06 | Codex | Nur P5-Migrationsplan, RSS-Commit b1d9c59 und eigenen ScreenBuddy-Code gelesen; Streaming mechanisch nach RSS-Muster migriert, ohne Einsicht in libre-panel-Code oder andere Fremdimplementierungen. | ausstehend |
| P5-3 BF6 | 2026-11-06 | Codex | Nur PHASE5-MIGRATIONSPLAN.md, RSS-Commit b1d9c59 und ScreenBuddy-Code gelesen; BF6 mechanisch auf eigenen Seitenrahmen und Block-Schnittstelle umgestellt, ohne Einsicht in libre-panel-Code oder andere Fremdimplementierungen. | ausstehend |
| P5-5 Spotify | 2026-11-06 | Codex | Nur PHASE5-MIGRATIONSPLAN.md, eigene RSS-/Streaming-/BF6-Migrationen und ScreenBuddy-Code gelesen; sechs feste Spotify-Blöcke über draw_framed, ohne Einsicht in libre-panel-Code oder andere Fremdimplementierungen. Vollflächige Platzierungen erhalten die absoluten layout()-Ausdrücke und Überlappungen. | ausstehend |
| 6 Editor und Erststart | | | | |

Phase-4c-Sicherheitsreview M1/M2/N1–N4 (06.11.2026, Codex): ausschließlich eigenen Repo-Code und lokale Arbeitsanweisungen gelesen; URL-, Freigabe-, Geometrie- und Busy-Schalter-Korrekturen ohne Einsicht in Fremdcode. Automatischer Prüfstand und offene physische/UI-Abnahme: [Testprotokoll](../cpp/TESTPROTOKOLL.md).
