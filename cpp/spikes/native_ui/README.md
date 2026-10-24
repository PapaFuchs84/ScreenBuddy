# Browserfreier Native-UI-Spike

**Deutsch** · [English](README.en.md)

Dieser isolierte ScreenBuddy-2.5-Prototyp zeigt native Windows-Einstellungen, einen ausdrücklich simulierten Status und eine Vorschau des vorhandenen C++-Renderers. Er ist kein vollständiger Ersatz der Einstellungsseite. Der Produktionsstart, das bestehende Einstellungsfenster und dessen HTTP-Server werden durch diese Dateien nicht verändert.

## Lokale Werkzeugprüfung am 24.10.2026

- MSVC Build Tools 2022, Toolset 14.44.35207 und Windows SDK 10.0.26100.0 sind installiert; SDK 19041 und 22621 sind ebenfalls vorhanden.
- In `%USERPROFILE%\.nuget\packages` und den Projektabhängigkeiten wurde kein Windows-App-SDK-/WinUI-3-Entwicklungspaket gefunden. Die AppX-Abfrage der installierten Windows-App-Runtime wurde mit „Zugriff verweigert“ beendet. Eine installierte Runtime ist deshalb weder bestätigt noch ausgeschlossen.
- `qmake` und `qtpaths` sind nicht im PATH; in `C:\Qt`, `D:\Qt`, `C:\Program Files\Qt` und den Projektabhängigkeiten wurde kein Qt-SDK gefunden. Dies ist keine vollständige Datenträgerinventur.

Damit ist Win32 mit Windows-Standardsteuerelementen der lokal unmittelbar baubare Spike. Für WinUI 3 müssten zuerst eine konkrete Windows-App-SDK-Version, C++/WinRT-Generierung, Bootstrap/Deployment und Paketierungsstrategie eingerichtet und geprüft werden. Eine Installation wurde nicht vorgenommen.

## Bauen, ohne etwas zu starten

Voraussetzung: Das vorhandene `cpp/build/flcore.lib` wurde mit MSVC x64 und statischer C++-Runtime gebaut. Es wird wiederverwendet; der Helfer startet weder einen vollständigen Produktionsbuild noch Tests oder die erzeugte EXE.

```powershell
./cpp/spikes/native_ui/build.ps1
```

Optional: `-CoreBuildDir <Verzeichnis-mit-flcore.lib>`. Das Ergebnis liegt im Unterordner `native-ui-spike` dieses Buildverzeichnisses. Compiler/Linker werden lokal gefunden; es gibt keinen Download und keine Paketinstallation. Das eingebettete Manifest aktiviert Common Controls 6 und Per-Monitor-V2-DPI-Skalierung.

## Repräsentativer Ablauf

Bei einem später ausdrücklich freigegebenen UI-Test öffnet die EXE ein Fenster mit Beispieldaten. `--en` wählt Englisch, ohne Argument Deutsch. Designänderungen zeichnen die Vorschau unmittelbar neu. „Übernehmen (nur Demo)“ validiert Helligkeit/Touch/Design mit `config::merge`, übernimmt die Werte ausschließlich im Speicher und aktualisiert den Status. „Zurücksetzen“ stellt die Standardwerte wieder her. Helligkeit und Touch lösen keine Gerätebefehle aus.

Der Spike verwendet `config::defaults/merge`, `render::Renderer`, `render::demo_friends`, `render::apply_theme` und `gfx`. Er erstellt keine `App`, keine `Output`-Instanz und keine Datenquellen, liest keine Benutzerkonfiguration, speichert keine Datei und öffnet weder seriellen Port noch HTTP-Listener. COM wird ausschließlich vom bestehenden Grafikcode für WIC verwendet; dies ist keine serielle COM-Port-Verbindung.

## Integrationsschritte

1. Optionales CMake-Ziel hinzufügen: `add_executable(screenbuddy-native-spike WIN32 spikes/native_ui/native_settings_spike.cpp)` und `target_link_libraries(screenbuddy-native-spike PRIVATE flcore)`; das Manifest wie im Helfer mit `/MANIFEST:EMBED` und `/MANIFESTINPUT:<absoluter Pfad>` einbetten. Das reguläre App-Ziel zunächst unverändert lassen.
2. Gemeinsame Service-Schicht aus den Aktionen in `webui.cpp` herauslösen. `config::merge/save`, `App::cfg/last_frame/preview_theme/connections`, Neustart und Fehlerbehandlung brauchen dieselben Abläufe in beiden Oberflächen. Der Spike demonstriert absichtlich nur den sicheren In-Memory-Pfad.
3. Konfiguration, Status und Vorschau über kurzlebige Snapshots an die UI liefern. Netzwerk, Dateioperationen und laufende App-Abfragen aus dem UI-Thread verlagern; Ergebnisse per Fensternachricht zustellen. Produktionsvorschauen dürfen das globale Renderer-Theme nicht dauerhaft ändern: `App::preview_theme` verwenden oder Theme unter `render::mutex` sichern und wiederherstellen.
4. Die übrigen Einstellungen und Aktionen vollständig abbilden: Ausgabe/Monitor/Orientation, Quellen und Geheimnisse, Seitenreihenfolge, Companion-Kopplung, Updates, Autostart und Setup. DPAPI und bestehende Validierung weiterverwenden. Erst nach Funktionsparität den WebView-Host und den dafür genutzten Settings-HTTP-Server aus dem Produktionspfad entfernen; Companion-Dienste getrennt beurteilen.
5. UI-Prüfung mit Beispieldaten bei 100/150/200 % DPI, DE/EN, Tastatur, Screenreader, hohem Kontrast und kleinem Bildschirm durchführen. Win32 ist browserfrei, liefert aber keine automatische WinUI-3-/Fluent-Optik.

## Prüfstand und Grenzen

Am 24.10.2026 wurde der Spike mit MSVC C++20, `/W4`, `/MT` fehler- und warnungsfrei kompiliert und gelinkt. Das Ergebnis wurde **nicht ausgeführt**. Statische PE-Abhängigkeiten: D2D1, DWrite, COMCTL32, USER32, GDI32, OLE32, SHELL32 und KERNEL32; keine WebView2-/Edge-Abhängigkeit. WIC wird über COM erzeugt. Dies belegt den Build und den statischen Abhängigkeitsstand, keine sichtbare UI-/DPI-/Barrierefreiheitsabnahme und keinen Hardwaretest.

Offen sind die vollständige Feature-Parität, produktive Thread-/Lebenszyklusintegration, eine Oberfläche für Geheimnisse und eine feinere Anpassung an kleine Bildschirme. Die Mindestfenstergröße ist 960 × 720 logische Einheiten. Das globale Renderer-Theme und die Sprache sind im eigenständigen Spike prozesslokal; beim Einbau in die laufende App sind diese Zustände ausdrücklich zu koordinieren.
