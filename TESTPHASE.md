# Testphase – Rückmeldungen geben

**Deutsch** · [English](TESTPHASE.en.md)

Danke, dass du die App testest! So hilfst du am meisten.

## Fehler melden

1. Auf GitHub im Repository auf **Issues → New issue** gehen und **Fehlerbericht** wählen. Das Formular fragt alles Nötige ab.
2. Kein GitHub-Konto? Dann schick dieselben Angaben einfach per Nachricht an Basti.

Wichtig sind immer: **Was hast du gemacht, was ist passiert, was hättest du erwartet.** Ein Foto vom Display oder ein Screenshot der Einstellungsseite hilft sehr.

## Das Protokoll (Log) mitschicken

Die App schreibt alles Wichtige in die Datei **`usb-display.log`**:

- Version 2.0 bei regulärer Installation: `%LOCALAPPDATA%\FuchsLabs\USB-Display\usb-display.log`.
- Ein eigener Pfad per `--log DATEI` hat Vorrang.

Die Datei enthält **keine Tokens oder Schlüssel**. Sie kann aber Namen deiner Freunde, Server und Spiele enthalten. Schau kurz drüber und ersetze, was du nicht teilen willst, bevor du sie anhängst. Für mehr Details die App einmal mit `-v` starten (Verknüpfung oder Eingabeaufforderung: `FuchsLabs-USB-Display.exe -v`).

**Niemals mitschicken:** die Datei `config.toml`, Bot-Token, Steam- oder Xbox-Schlüssel.

## Wünsche und Ideen

Unter **Issues → New issue → Idee oder Rückmeldung** kannst du alles loswerden, was dir gefällt, fehlt oder umständlich ist.

## Windows-Neustarttest

Nach der Anmeldung prüfen: Aktualisiert sich das physische Display, erscheint das Fuchs-Symbol im Infobereich und öffnen die Einstellungen in einem eigenen Windowsfenster? Version und beobachtetes Verhalten melden. Der erste Test scheiterte; die Bestätigung der korrigierten Installation steht aus.
