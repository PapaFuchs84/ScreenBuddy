# Mitarbeiten am FuchsLabs USB-Display

**Deutsch** · [English](CONTRIBUTING.en.md)

Schön, dass du helfen willst! Das Repository ist privat; mitarbeiten kann, wer von FuchsLabs eingeladen wurde.

## Fehler und Ideen

- **Fehler** über *Issues → New issue → Fehlerbericht* melden, **Ideen** über *Idee oder Rückmeldung*. Was ein guter Bericht braucht und wo das Protokoll liegt, steht in [`TESTPHASE.md`](TESTPHASE.md).
- **Sicherheitslücken** nie als Issue, sondern wie in [`SECURITY.md`](SECURITY.md) beschrieben.

## Entwicklungsumgebung

Windows 10/11, Python 3.11+ (64 Bit), Git.

```bat
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display
build.bat
.venv\Scripts\python fuchslabs_display.py --no-lcd
```

`build.bat` legt die `.venv` an und baut die EXE. Mit `--no-lcd` läuft die App ohne Display, die Einstellungsseite ist dann unter `http://127.0.0.1:8765/` erreichbar. Beispielbilder aller Seiten ohne Hardware: `--demo --no-lcd --preview vorschau`.

## Ablauf

1. Für jede Änderung einen eigenen Branch von `main` anlegen, z. B. `fix/display-reconnect` oder `feature/neue-seite`.
2. Kleine, in sich geschlossene Commits mit aussagekräftiger Nachricht auf Deutsch („Display: Helligkeit nach Wiederverbindung setzen“).
3. Vor dem Pull Request:
   - `.venv\Scripts\python test_friends.py` läuft ohne Fehler.
   - Die App startet mit `--no-lcd`, und betroffene Seiten sehen mit `--demo --preview` richtig aus (hell und dunkel).
   - Sichtbare Änderungen stehen unter **[Unveröffentlicht]** in [`CHANGELOG.md`](CHANGELOG.md); neue Einstellungen zusätzlich in [`DOKUMENTATION.md`](DOKUMENTATION.md).
4. Pull Request gegen `main` öffnen und kurz beschreiben: vorher, nachher, wie getestet. Bei Änderungen am Aussehen ein Bild anhängen.

## Regeln für den Code

- **Stil:** Python wie im vorhandenen Code, Kommentare und Quelltexte auf Deutsch. Texte in der Einstellungsseite so einfach, dass ein Kind sie versteht.
- **Sprachen:** Jeder Text, den Anwender sehen, gibt es auf Deutsch und Englisch. App- und Display-Texte übersetzt `i18n.py`, Texte der Einstellungsseite die Tabelle `EN` in `webui.html`. Sichtbare Änderungen in beiden Changelogs eintragen.
- **Sparsam:** Die App läuft dauerhaft im Hintergrund. Keine neuen Abhängigkeiten ohne guten Grund; Pakete werden in `requirements.txt` auf genaue Versionen festgelegt.
- **Design:** Farben und Logo nur aus `brand.py`. Nur Windows-Schriften (Bahnschrift, Segoe UI), keine Web-Fonts.
- **Display:** Seiten zeichnen in logischen 480 × 320-Koordinaten; `render.py` skaliert auf die echte Auflösung. Keine Helligkeitsbefehle während einer Bildübertragung.
- **Datenquellen:** nur offizielle oder öffentliche Schnittstellen. Nichts, was gegen Nutzungsbedingungen verstößt oder Konten gefährdet (kein Discord-Self-Bot, kein inoffizieller PlayStation-Zugang).
- **Neue Zugangsfelder** in der Einstellungsseite bekommen eine aufklappbare Hilfe mit Link zur richtigen Seite.

## Geheimnisse und private Daten

- **Niemals einchecken:** `config.toml`, Tokens, API-Schlüssel, Logs, Build-Ordner. `.gitignore` deckt das ab; prüfe trotzdem `git status` vor jedem Commit.
- Zugangsdaten nur über `secure.py` (DPAPI) speichern, nie ins Log schreiben, nie an den Browser zurückgeben.
- **Screenshots** nur mit Beispielnamen. `anleitung/screenshots.mjs` ersetzt private Namen; jedes Bild vor dem Commit selbst ansehen.

## Versionen

Versionen folgen [SemVer](https://semver.org/lang/de/). Für ein Release: Version in `webui.py` und `version-info.txt` anheben, `CHANGELOG.md` von *Unveröffentlicht* in die neue Version verschieben, Tag `vX.Y` setzen.
