# Mitarbeiten an ScreenBuddy

**Deutsch** · [English](CONTRIBUTING.en.md)

Die aktuelle Implementierung ist C++20 unter `cpp/`. Die Python-Dateien im Hauptordner bleiben als historischer Stand 1.x erhalten. Der Code steht unter der MIT-Lizenz; mit einem Beitrag stimmst du zu, dass er unter dieser Lizenz veröffentlicht wird.

## Entwicklungsumgebung

Windows x64, Visual Studio Build Tools 2022 mit C++/Windows SDK und CMake/Ninja, Git und eingerichteter vcpkg-Checkout unter `%USERPROFILE%\FuchsLabs\vcpkg`.

```bat
git clone https://github.com/PapaFuchs84/ScreenBuddy.git
cd ScreenBuddy\cpp
build.bat nopause
build\screenbuddy-release\ScreenBuddy.exe --run --no-lcd -c test.toml
```

`cpp/build.bat` baut Release, führt Prüfungen aus und kopiert die einzelne App-/Installer-EXE nach `cpp/dist/`. Die `build.bat` im Hauptordner baut weiterhin Python 1.x. Die aktuelle 2.6-Quellfassung verwendet für alle Einstellungen ein selbst gezeichnetes natives Win32-Fenster und bindet WebView2 nicht ein. Die frühere Version 2.3.0 benötigt noch WebView2 Evergreen. Siehe [Buildhinweise](cpp/README.md#build).

## Ablauf

1. Branch von `main` anlegen und die resultierende Änderung verständlich beschreiben.
2. Kompilieren und zur Änderung passende Prüfungen ausführen. `cpp/build/fl-tests.exe` prüft Kernfunktionen und `cpp/build/fl-monitor-tests.exe` Gestengrundlagen; dessen Modus `window` prüft sichtbar einen Nebenmonitor mit injizierten Touch-Eingaben; `--demo --no-lcd --preview vorschau` erzeugt Beispielbilder.
3. Betroffene Oberfläche auf Deutsch und Englisch prüfen. Vor Hardwaretests die andere Display-App beenden, damit der COM-Port exklusiv frei ist.
4. Dokumentation gehört zu jeder Änderung: Betroffene READMEs, Schnellstart, Referenz, Versions-/Downloadlinks, Bedienung und Prüfstand im selben Arbeitsgang aktualisieren, ohne gesonderte Aufforderung. Vorhandene deutsche/englische Fassungen und beide Haupt-Changelogs konsistent halten; bei C++ auch `cpp/CHANGELOG.md` pflegen. Siehe [Projektregeln](AGENTS.md).
5. PR gegen `main` mit Validierung öffnen; Screenshots nur mit Beispieldaten.

## Regeln für den Code

- Vorhandenen C++20-Stil beibehalten, RAII verwenden und Hintergrundarbeit begrenzen.
- App-/Display-Texte: `cpp/src/i18n.cpp`; Texte der nativen Einstellungen: `cpp/src/ui_page_*.cpp`. Die frühere HTML-Einstellungsseite `cpp/res/webui.html` bleibt nur für historische Tests im Quellbaum.
- Farben/Logo: `cpp/src/brand.h`; Grafik: `gfx.cpp` und `render.cpp`. Windows-Schriften nutzen; keine Helligkeitsbefehle während einer Bildübertragung.
- Abhängigkeiten: `cpp/vcpkg.json`, festgelegte Baseline und `cpp/overlay-ports/`; Drittanbieter-Lizenzhinweise erhalten.
- Nur autorisierte/öffentliche Datenschnittstellen; keine Discord-Self-Bots oder inoffiziellen PlayStation-Zugänge.
- Zugangsdaten über `cpp/src/secure.cpp` (Windows-DPAPI) speichern. Nie Konfigurationen, Zugangsdaten, Logs oder private Bilder einchecken; `git diff --cached` prüfen.

## Versionen und Meldungen

Bei einem Code-Release `cpp/CMakeLists.txt`, `cpp/src/util.h` und `cpp/res/app.rc` konsistent aktualisieren. EXE und SHA-256-Prüfsumme veröffentlichen; Laufzeitvoraussetzungen und offene Tests nennen. Reine Dokumentationsänderungen ersetzen keine bereits veröffentlichte EXE.

Fehler: [Rückmeldungen](TESTPHASE.md). Sicherheitslücken: [SECURITY.md](SECURITY.md), keine Geheimnisse im öffentlichen Issue. Der alte [Prüfbericht](SICHERHEITSREVIEW.md) ist kein vollständiger C++-Audit.
