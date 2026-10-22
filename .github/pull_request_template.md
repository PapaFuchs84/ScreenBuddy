## Vorher · Before

<!-- Was war bisher, oder was hat nicht funktioniert? · What was there before, or what didn't work? -->

## Nachher · After

<!-- Was ist jetzt anders? Bei sichtbaren Änderungen bitte ein Bild (nur Beispielnamen). · What changed? Add a picture for visual changes (sample names only). -->

## Wie getestet · How it was tested

- [ ] C++-Release-Build sowie `fl-tests.exe` und `fl-monitor-tests.exe` bestanden · C++ Release build and both test executables pass
- [ ] App startet mit `--no-lcd` · App starts with `--no-lcd`
- [ ] Betroffene Seiten mit `--demo --preview` in Hell/Dunkel und Deutsch/Englisch angesehen · Affected pages checked with `--demo --preview` in light/dark and German/English
- [ ] Am echten Display bzw. Bildschirm geprüft (falls Hardware betroffen) · Checked on the real display or screen (if hardware is affected)

## Checkliste · Checklist

- [ ] `CHANGELOG.md` und `CHANGELOG.en.md` unter „Unveröffentlicht/Unreleased“ ergänzt · updated
- [ ] Neue Texte in beiden Sprachen (`cpp/src/i18n.cpp`, `cpp/res/webui.html`) · New texts in both languages
- [ ] Neue Einstellungen in `DOKUMENTATION.md` beschrieben · New settings documented
- [ ] Betroffene README-/Anleitungsfassungen, Versionen, Downloadlinks und Prüfstatus im selben Arbeitsgang aktualisiert · Affected README/guide versions, release/download links and validation status updated in the same task
- [ ] Bebilderte Anleitungen: Vorlagen und HTML-Ressourcen konsistent, nur Beispieldaten · Illustrated guides: templates and HTML resources consistent, sample data only
- [ ] Keine Tokens, Schlüssel, `config.toml`, Logs oder echten Namen im Commit · No tokens, keys, `config.toml`, logs or real names in the commit
