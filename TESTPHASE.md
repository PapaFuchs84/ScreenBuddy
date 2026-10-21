# Testphase – Rückmeldungen geben

**Deutsch** · [English](TESTPHASE.en.md)

Danke, dass du die App testest! So hilfst du am meisten.

## Fehler melden

1. Auf GitHub im Repository auf **Issues → New issue** gehen und **Fehlerbericht** wählen. Das Formular fragt alles Nötige ab.
2. Kein GitHub-Konto? Dann schick dieselben Angaben einfach per Nachricht an Basti.

Wichtig sind immer: **Was hast du gemacht, was ist passiert, was hättest du erwartet.** Ein Foto vom Display oder ein Screenshot der Einstellungsseite hilft sehr.

## Das Protokoll (Log) mitschicken

Die App schreibt alles Wichtige in die Datei **`usb-display.log`**:

- normalerweise **im Ordner der App**, neben `FuchsLabs-USB-Display.exe`,
- liegt die App in einem Ordner ohne Schreibrecht (z. B. „Programme“), stattdessen unter `%APPDATA%\FuchsLabs\USB-Display\`.

Die Datei enthält **keine Tokens oder Schlüssel**. Sie kann aber Namen deiner Freunde, Server und Spiele enthalten. Schau kurz drüber und ersetze, was du nicht teilen willst, bevor du sie anhängst. Für mehr Details die App einmal mit `-v` starten (Verknüpfung oder Eingabeaufforderung: `FuchsLabs-USB-Display.exe -v`).

**Niemals mitschicken:** die Datei `config.toml`, Bot-Token, Steam- oder Xbox-Schlüssel.

## Wünsche und Ideen

Unter **Issues → New issue → Idee oder Rückmeldung** kannst du alles loswerden, was dir gefällt, fehlt oder umständlich ist.
