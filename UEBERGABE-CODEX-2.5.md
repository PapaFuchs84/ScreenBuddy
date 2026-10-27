# Übergabedokumentation: Release-Vorbereitung ScreenBuddy 2.5.0 & Companion APK

**Datum:** 25.10.2026, 17:05 Uhr

**Bearbeiter:** Antigravity (Release- & Debugging-Agent)

**Zielperson:** Codex

**Branch:** `feature/native-ui-framework` (5 Commits vor `origin/main`, mit uncommitteten Arbeitsbaum-Änderungen)

**Zielversionen:** Windows ScreenBuddy 2.5.0 (aktueller Build: `2.5.0-dev.2`) / Android Companion APK (`0.2.0-preview`)

**Gesamtstatus:** **NICHT RELEASEBEREIT** (Befund: Entwicklungsbuild mit offenen Release-Blockern bei Display-Hardware und APK-Signierung)

---

## 1. Ausgangslage und Ziel

Ziel war die belastbare Beurteilung der Releasefähigkeit von Version 2.5 (Windows ScreenBuddy und zugehörige Android Companion APK), Behebung reproduzierbarer Blocker sowie eine vollständige Übergabe an Codex bis zur Abgabefrist (17:50 Uhr).

### Repository- und Umgebungszustand
- **Git-Status:** Auf Branch `feature/native-ui-framework`. Umfangreiche, wertvolle Arbeitsbaumänderungen (u. a. native Einstellungsseiten Phase 2, Plugin-Dokumentation, mDNS-Härtung, Vorlagen und Screenshots) vorhanden. Keine Zerstörung oder voreiliges Committen.
- **Entwicklungsumgebung:**
  - MSVC 2022 Build Tools (vcvars64.bat, CMake, Ninja, vcpkg mit statischen Bibliotheken)
  - Java: JDK 21 (Temurin 21.0.12.1+1-LTS unter `C:\Users\admin\scoop\apps\temurin21-jdk\current`)
  - Android SDK: Platform 34, Build-Tools 34.0.0 unter `%LOCALAPPDATA%\Android\Sdk`
  - Python: 3.15
  - Node.js: v26.12.1 (Playwright nicht global installiert)

---

## 2. Vorgenommene Änderungen

| Datei | Art der Änderung | Begründung & Verifikation |
|---|---|---|
| `cpp/tests/native_settings_smoke.py` | Fehlerbehebung (`EnumWindows`-Prüfung) | `USER32.EnumWindows` gibt `FALSE` (0) zurück, wenn während des Aufrufs noch keine Zielfenster existieren oder die Aufzählung endet, wobei `GetLastError() == 0` (ERROR_SUCCESS) bleibt. Die bisherige Prüfung `if not USER32.EnumWindows(...)` löste sofort fälschlicherweise `[WinError 0] Der Vorgang wurde erfolgreich beendet.` aus. Vor Aufruf wird nun `ctypes.set_last_error(0)` gesetzt und `WinError` nur geworfen, wenn `err != 0`. **Verifikation:** `native_settings_smoke.py` bestand unmittelbar danach alle 20 Prüfungen. |

*Hinweis:* Alle übrigen Dateien im Arbeitsbaum wurden unverändert im Zustand des Nutzers belassen.

---

## 3. Build- und Prüfergebnisse

### A. Windows (ScreenBuddy C++20)

**Build-Befehl:**
```cmd
cd cpp
build.bat nopause
```
Ergebnis: **Erfolgreich kompiliert und gelinkt.**

**Artefakt:**
- **Pfad:** `cpp/dist/ScreenBuddy.exe` (sowie `cpp/build/screenbuddy-release/ScreenBuddy.exe`)
- **Dateigröße:** 14.844.928 Bytes
- **SHA-256:** `A832B76E77C9754AA2D37DEFE6C88A23A9D7B6E2E15CC84222F482A57920E146`
- **Dateiversion:** 2.5.0.0 (Produktversion: `2.5.0-dev.2`, Pre-Release)

**Ausgeführte und verifizierte Windows-Tests:**
1. `fl-tests.exe`: **271 Prüfungen bestanden**, 0 Fehler (Config, Secure/DPAPI, i18n, RSS, URLs, Netguard, Friends, Home Assistant, UI-Logic, Touch-Ziele, Update-Check).
2. `fl-plugin-tests.exe`: **223 Prüfungen bestanden**, 0 Fehler (Manifest-Validierung, Schema 1, Grenzen).
3. `fl-plugin-example-tests.exe`: **41 Prüfungen bestanden**, 0 Fehler (Offline-Clock, Offline-Focus, Public-Weather).
4. `fl-plugin-store-tests.exe`: **127 Prüfungen bestanden**, 0 Fehler (Import, Aktivierung, Löschung).
5. `fl-plugin-transport-tests.exe`: **133 Prüfungen bestanden**, 0 Fehler (Begrenzter HTTPS-Worker, Host-Freigaben).
6. `fl-plugin-settings-tests.exe`: **135 Prüfungen bestanden**, 0 Fehler (DPAPI-Schutz, Migration).
7. `fl-monitor-tests.exe`: **11 Prüfungen bestanden**, 0 Fehler (Monitor-Ausgabe).
8. `fl-companion-tests.exe`: **335 Prüfungen bestanden**, 0 Fehler (TLS-Server 127.0.0.1, WebSocket, Handshake, Reconnect).
9. `installer_preserves_config.py`: **2 Prüfungen bestanden** (Konfigurationserhalt, Upgrade-Sicherung).
10. `native_settings_smoke.py`: **20 Prüfungen bestanden** (Fenster, MSAA-Namen, Tastatur, Dirty-Zähler, Verwerfen, Sprache, Speichern/Neustart, Plugins).
11. `native_mouse_smoke.py`: **16 Prüfungen bestanden** (Mausnavigation, Loslassen außerhalb, Capture-Verlust, Slider-Drag, Verwerfen).
12. `native_companion_rebuild_smoke.py`: **16 Prüfungen bestanden** (Gleiche Sitzung, Deadline, Abbruch, Server-Abschluss).
13. `app_companion_integration.py`: **39 Prüfungen bestanden** (Echte App-Prozesse, neutrale Ports, keine LCD-Hardware).
14. `test_page_navigation.py`: **49 Prüfungen bestanden** (API-Version 2.5.0-dev.2, Seitenwechsel, Wrap-Around, Schutz gegen fremde Origins).

---

### B. Android Companion APK

**Build-Befehle:**
```powershell
$env:JAVA_HOME = "C:\Users\admin\scoop\apps\temurin21-jdk\current"
$env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
$env:PATH = "$env:JAVA_HOME\bin;$env:PATH"
cd android

# Protokoll-Tests
.\gradlew.bat :protocol:test --rerun-tasks

# Debug APK bauen
.\gradlew.bat :app:assembleDebug

# Release APK bauen
.\gradlew.bat :app:assembleRelease
```

**Protokoll-Unit-Tests (`:protocol:test`):**
- **20 Tests bestanden**, 0 Fehler, 0 übersprungen (Host-Policy für private IPv4, Pairing-URI, HandshakeGate, Letterbox-Mapping, VideoFrame, Server/Client-Nachrichten).

**Erzeugte APK-Artefakte:**
1. **Debug-APK:**
   - **Pfad:** `android/app/build/outputs/apk/debug/app-debug.apk`
   - **Dateigröße:** 4.149.316 Bytes
   - **SHA-256:** `5008C2E40CBD8191939CAA08098C9E8C727D301FCA3D5C660019BC6442D481E1`
   - **Signatur:** Android Debug Keystore
   - **Verifikationsstatus:** Baut fehlerfrei; nur für Entwicklungs-/Testzwecke geeignet.
2. **Release-APK:**
   - **Pfad:** `android/app/build/outputs/apk/release/app-release-unsigned.apk`
   - **Dateigröße:** 3.256.948 Bytes
   - **SHA-256:** `320FDAD783985D852D2FC1A7DDCE97D3FA65185180241A63D7856A40C0494D9C`
   - **Signatur:** **Unsigniert** (`app-release-unsigned.apk`)
   - **Verifikationsstatus:** Baut fehlerfrei durch; **nicht releasefähig**, da unsigniert.

---

## 4. Prüfmatrix (Übersicht)

| Komponente / Test | Status | Details / Begründung |
|---|---|---|
| **C++ Core Unit-Tests** (8 Test-Suites) | **BESTANDEN** | 1.276 Einzelprüfungen erfolgreich ausgeführt. |
| **Windows UI Smoke- & Integrationstests** | **BESTANDEN** | 140 automatisierte Tests bestanden (`native_settings`, `mouse`, `companion_rebuild`, `app_integration`, `page_navigation`). |
| **Android Protokoll-Unit-Tests** | **BESTANDEN** | 20 Tests in `:protocol:test` erfolgreich. |
| **Android APK Debug & Release Build** | **BESTANDEN** | Gradle 8.9 baut beide Varianten erfolgreich. |
| **Physisches USB-Display (COM7)** | **BLOCKIERT / FAIL** | Laut `docs/HARDWARE-ACCEPTANCE-2.5.md`: `--demo`-Lauf auf COM7 zeigte blaue Streifen und Flackern. Hardware-Fehler bleibt offener Release-Blocker. |
| **Physisches Android-Gerät (ADB)** | **NICHT AUSGEFÜHRT** | `adb devices` ergab 0 verbundene Geräte. Kein physischer Lauf auf Redmi 8 oder Pixel durchgeführt. |
| **Playwright UI-Test (`app_companion_ui.cjs`)** | **NICHT AUSGEFÜHRT** | Node-Paket `playwright` ist auf dem Host nicht global installiert. |
| **Externe HTTPS-Live-Abfragen (Plugins)** | **NICHT AUSGEFÜHRT** | Isolierte Unit-Tests mocken Transporte; keine externen Drittanbieterdienste live abgefragt. |
| **Echtes Upgrade von 2.3.0** | **NICHT AUSGEFÜHRT** | Bisher nur isolierte Verzeichnis-Tests; echtes System-Upgrade und Rollback stehen noch aus. |

---

## 5. Bekannte Release-Blocker und Risiken

### Blocker (Priorität P0 – Verhindern Release 2.5.0)
1. **USB-Display Bildstörung (Hardware-Blocker):**
   - *Befund:* In `docs/HARDWARE-ACCEPTANCE-2.5.md` dokumentiert: ScreenBuddy `2.5.0-dev.2` verursachte auf COM7 (`VID_1A86`, `PID_5722`) trotz Exitcode 0 blaue Streifen und Flackern. Ein Power-Cycle (10 s USB abziehen) stellte das normale Bild unter 2.3.0 wieder her. Ursache im C++-Rendering/Serial-Display-Timing bisher ungeklärt.
   - *Nächster Schritt:* `cpp/src/serial_display.cpp` und Baudrate/Flow-Control prüfen. Kontrollierten Hardware-Test erst nach Freigabe und mit Backup durchführen.
2. **Android Release-Signierung:**
   - *Befund:* `assembleRelease` erzeugt nur `app-release-unsigned.apk`. Kein Produktions-Keystore und keine Passwörter im Repository.
   - *Nächster Schritt:* Keystore und Release-Signierkonfiguration müssen vom Projektinhaber außerhalb des Git-Repositorys bereitgestellt oder manuell signiert werden.

### Risiken (Priorität P1 – Vor Freigabe zu klären)
1. **Pre-Release-Versionsstand:**
   - Windows-EXE ist als `2.5.0-dev.2` gekennzeichnet. `CMakeLists.txt` hat zwar `2.5.0`, aber die Metadaten und `version-info.txt` müssen bei finalem Release synchronisiert werden.
   - Companion-Versionierung steht in `android/app/build.gradle.kts` auf `versionName = "0.2.0-preview"` (versionCode: 2). Es muss entschieden werden, ob Companion eine 2.5-Versionsnummer oder weiterhin einen eigenen Versionierungszyklus (0.2.x) nutzt.
2. **Keine physische Geräte-Verifikation:**
   - Da kein Android-Gerät an ADB angeschlossen ist, konnte die aktuelle APK nicht live auf Gesten/Wiederverbindung getestet werden.

---

## 6. Konkrete Handlungsanweisungen für Codex

1. **Arbeitsbaum nicht zurücksetzen:**
   - Auf keinen Fall `git checkout .` oder `git reset --hard` ausführen. Der Branch enthält wichtige Phase-2- und Doku-Arbeiten des Nutzers.
2. **Keine Signierschlüssel erfinden:**
   - Keinen Ad-hoc-Release-Keystore mit Fantasie-Passwörtern ins Repository einchecken.
3. **Priorisierte nächste Schritte:**
   - **Schritt 1:** Display-Treiber / Serial-Kommunikation prüfen: Diff zwischen `2.3.0` und `dev.2` in `cpp/src/serial_display.cpp`, `cpp/src/render.cpp` und `cpp/src/main.cpp` analysieren, warum das Display gestört wurde (z. B. RGB565-Byteorder, Banding, Pufferüberlauf oder Reset-Kommando).
   - **Schritt 2:** Klärung mit Nutzer bezüglich Android Release-Signierung und Versionierung der APK (`0.2.0-preview` vs. `2.5.0-preview`).
   - **Schritt 3:** Sobald Hardware-Ursache behoben: Physische Abnahme nach `docs/HARDWARE-ACCEPTANCE-2.5.md` durchführen.
   - **Schritt 4:** Nach erfolgreicher Hardware-Abnahme Version von `2.5.0-dev.2` auf `2.5.0` finalisieren, Doku synchronisieren und erst nach ausdrücklicher Freigabe committen.

---

## 7. Begründung des Gesamtstatus

**Status: NICHT RELEASEBEREIT**

*Begründung:* Sowohl die Windows-Anwendung als auch die Android-Companion-APK weisen harte Blocker für einen produktiven Release auf:
1. Die Hardware-Ausgabe auf dem Ziel-USB-Display zeigte im letzten Sichttest reproduzierbare Signal-/Bildstörungen (blaue Streifen/Flackern), womit das primäre Release-Gate („Serielle Demo artefaktfrei“) nicht bestanden ist.
2. Die Android-Release-APK liegt ausschließlich unsigniert vor und kann auf Endgeräten nicht produktiv installiert oder verteilt werden.
3. Wesentliche Freigabe-Gates (physische Geräte-Tests, echtes System-Upgrade, externe Live-HTTPS-Validierung) sind offen.

---

## 8. Codeanalyse & Optimierungspotenziale (Ergebnisse für Codex)

Im Zuge der Vorbereitung wurden gezielte Analysen der Rendering- und Kommunikationspfade durchgeführt:

### A. Windows & Serieller Display-Treiber (`cpp/src/serial_display.cpp`)
1. **Timeout- und Übertragungs-Fehlanpassung (Wahrscheinlichste Ursache für die Display-Artefakte!):**
   - In `SerialDisplay::open`: `WriteTotalTimeoutConstant = 5000` (5 s), `WriteTotalTimeoutMultiplier = 0`.
   - Bei 115.200 Baud (10 Bits/Byte = max. 11.520 Bytes/s) benötigt ein Vollbild (480 × 320 × 2 Bytes = 307.200 Bytes) rechnerisch **mindestens 26,6 Sekunden** Übertragungszeit.
   - Läuft der UART-Puffer voll oder verzögert das Panel die Annahme, bricht `WriteFile` nach 5 Sekunden mit Timeout ab. Der Display-Controller verharrt im `DISPLAY_BITMAP`-Modus und interpretiert folgende Befehle als Pixeldaten -> **Ergebnis: Farbverschiebungen, blaue Streifen und Flackern**.
   - *Empfehlung:* `WriteTotalTimeoutMultiplier` auf z. B. 2 ms/Byte setzen und `WriteTotalTimeoutConstant` auf mindestens 30.000 ms erhöhen.
2. **Zero-Copy Optimierung in `SerialDisplay::blit`:**
   - Aktuell ruft `for (size_t i = 0; i < data.size(); i += step) write(data.substr(i, step));` für jeden 3.840-Byte-Block `std::string::substr` auf, was unnötige Heap-Allokationen erzeugt.
   - *Empfehlung:* `write(const char* buf, size_t len)` bzw. `std::string_view` nutzen, um direkt aus dem Puffer zu schreiben.
3. **Soft-Reset Vorsicht (`reset_on_start`):**
   - Das Senden von `RESET` (Befehl 101) versetzt CH340/CH552-basierte Display-Firmwares oft in einen Zustand, der erst durch Power-Cycle bereinigt werden kann. `reset_on_start = false` als Standard testen.

### B. Android Companion App (`android/app/src/main/java/de/fuchslabs/display/`)
1. **Bitmap-Pooling zur GC-Vermeidung (`FrameView.kt`):**
   - `BitmapFactory.decodeByteArray(...)` erzeugt für jedes empfangene Frame ein neues `Bitmap`-Objekt auf dem Heap. Bei kontinuierlichem Streaming führt das zu erhöhtem GC-Druck (Ruckler/Frame-Drops).
   - *Empfehlung:* Nutzung von `BitmapFactory.Options.inBitmap` mit einem wiederverwendeten Bitmap-Puffer gleicher Dimension.
2. **Doppelter Decode-Pass vermeiden:**
   - In `FrameView.decodeLatest()` wird das JPEG aktuell zweimal decodiert (einmal mit `inJustDecodeBounds = true`, einmal vollständig). Da die Bilddimensionen durch `ServerMessage.Ready` bzw. Layout bereits bekannt sind, kann der Bounds-Pass übersprungen werden.
3. **Screen-Wake-Lock / Keep-Screen-On:**
   - In `DisplayActivity` sicherstellen, dass `FLAG_KEEP_SCREEN_ON` aktiv ist, wenn das Display als dauerhafter Monitor genutzt wird.
