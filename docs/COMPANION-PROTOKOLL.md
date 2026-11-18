# Companion-Protokoll v1 (Entwurf)

PC-App (Server) <-> Android-App (Client), nur lokales Netz.

- Erkennung: mDNS `_fuchslabs-display._tcp`, TXT: `v=1`, `name=<PC-Name>`. Fallback: manuelle IP.
- Port: Standard 8777 (nie 8765). Nur an LAN-Adressen binden, nur wenn in den Einstellungen aktiviert (Standard aus).
- Transport: WebSocket über TLS (selbstsigniertes Zertifikat, Pinning des SHA-256-Fingerabdrucks nach dem Koppeln).
- Kopplung: PC zeigt QR-Code (`fuchslabs://pair?host=<ip>&port=8777&fp=<sha256>&pin=<6 Ziffern>`). Client sendet `hello` mit PIN. Server antwortet mit `paired` und einem Geräteschlüssel (32 Byte, base64); PC speichert ihn per DPAPI, Client im Android Keystore. Danach `hello` mit `key`. PIN gilt einmalig, 2 Minuten, 5 Fehlversuche -> Sperre.
- Nachrichten (Text-Frames, JSON): 
  - C->S `{"t":"hello","v":1,"key"|"pin":...,"w":<px>,"h":<px>,"name":"..."}`
  - S->C `{"t":"ready","w":480,"h":320}` (tatsächliche Renderfläche; Startwert 480×320, anschließend native Handy-Größe, bei Rotation aktualisiert)
  - C->S `{"t":"touch","x":..,"y":..}` in Renderkoordinaten (0..w-1, 0..h-1), je Tipp ein Event
  - C->S `{"t":"swipe","dir":"left"|"right"}` (Android: Wischgeste; Finger nach links = `left` = nächste Seite, `right` = vorherige)
  - C->S `{"t":"resize","w":..,"h":..}` bei Rotation/Größenänderung (Pixel der Rendefläche, Landscape: lange Kante = `w`; `hello` trägt dieselbe Größe). Server antwortet mit neuem `ready` (Renderkoordinaten); `touch` bleibt in diesen Koordinaten. (beidseitig implementiert; höchstens 4 Größenmeldungen pro Sekunde, erst nach Anmeldung).
  - beidseitig `{"t":"ping"}` / `{"t":"pong"}` alle 5 s; 15 s Stille = Verbindung tot
- Frames (Binary-Frames): Byte 0 = Typ (1 = vollständiges JPEG, Qualität ~80), danach JPEG-Daten. Normale Seiten werden nur bei Änderung und höchstens mit 5 fps gesendet; nach Verbindung folgt sofort ein Vollbild. Ab ScreenBuddy 2.12.0 kann der PC pro Gerät einen bereits am PC getakteten Produzenten sichtbarer Animationen (z. B. [Hardware-HUD](../HARDWARE-HUD-TEST.md), aktiver KI-Status, laufende Spotify-Seite) mit der globalen Zielrate von 5–60 fps freigeben (`display.animation_fps`, Standard 30). Diese vertrauenswürdige interne Metainformation kommt nicht vom Android-Client und ist kein neues Protokollfeld. Für solche animierten Bilder entfällt eine zweite zeitliche Transportbegrenzung; unveränderte Geräteversionen werden weiterhin unterdrückt, und statische Seiten bleiben bei höchstens 5 fps. Das Nachrichtenformat von Protokoll v1 und die Android-App (Companion 1.1.0) bleiben unverändert.
- Die Animations-Zielrate ist ein Rechenlast- und Datenverkehrsbudget, keine garantierte Bildrate oder harte Grenze für alle durch Datenänderungen ausgelösten Veröffentlichungen. Die ursprünglichen 5 fps sind keine Anforderung des JPEG-/WebSocket-Formats oder der Android-App. Aktuelle Bilder ersetzen ältere Bilder im Gerätespeicher; der synchrone Sendeweg mit Frist bleibt bestehen und erzeugt keine unbegrenzte Warteschlange. Höhere Zielraten benötigen mehr Render-/JPEG-Rechenzeit, Bandbreite und Decodierleistung.
- Fehler: `{"t":"error","code":"...","msg":"..."}`, danach Close.
- Launcher-Modus (Android): App deklariert HOME/DEFAULT-Intent, startet optional beim Booten, Vollbild (immersive), Bildschirm bleibt an, verbindet automatisch zum zuletzt gekoppelten PC neu.

## Umsetzung PC (C++, `cpp/src/companion*.cpp`, `network_display.cpp`)

- Aktivierung: `[companion] enabled = true` (unabhängig zusätzlich zum USB-Display/Bildschirm; eigene native Handy-Größe) oder `[display] output = "network"` (nur Companion, native Handy-Größe). Standard aus. `companion.port` darf weder 8765 noch der Port der Einstellungsseite sein.
- Gebunden wird je private IPv4-Adresse (10/8, 172.16/12, 192.168/16) eines aktiven Adapters; Verbindungen von anderen Adressen werden sofort geschlossen. Höchstens 4 gleichzeitige Verbindungen.
- TLS ≥ 1.2, ECDSA P-256, selbstsigniert, 10 Jahre gültig, einmal erzeugt und in `companion.json` gespeichert (privater Schlüssel per DPAPI). `fp` im QR-Code: SHA-256 über das DER-Zertifikat, 64 Hex-Zeichen klein; die Einstellungsseite zeigt ihn zusätzlich mit Doppelpunkten.
- WebSocket: beliebiger Pfad (die App nutzt `/`), `Sec-WebSocket-Version: 13`; Anfragen mit `Origin`-Header (Browser) werden mit 403 abgelehnt. Client-Rahmen müssen maskiert sein; Text höchstens 16 KiB; Binärnachrichten vom Client beenden die Verbindung.
- Ablauf: `hello` innerhalb von 10 s ab dem TCP-Verbindungsaufbau (TLS-Handshake, HTTP-Upgrade und `hello` zusammen; sonst `timeout` bzw. Trennung). Mit `pin`: bei Erfolg `paired` und direkt danach `ready` auf derselben Verbindung, dann sofort ein Vollbild. Mit `key`: `ready`, dann Vollbild. Ein zweites `hello` wird ignoriert.
- Fehlercodes (`error`, danach Close 1008): `pin_wrong`, `pin_expired` (kein/abgelaufener/verbrauchter Code), `locked` (5 Fehlversuche, 5 Minuten; gilt nur fürs Koppeln), `unauthorized` (unbekannter/entfernter Schlüssel, `touch` vor `hello`, Gerät am PC entfernt), `version`, `bad_message`, `timeout`, `storage` (Kopplung konnte am PC nicht gespeichert werden).
- `touch`: nur nach Anmeldung, Koordinaten außerhalb von `w`/`h` werden verworfen, höchstens 10 pro Sekunde. Wirkung wie ein Tipp auf dem Bildschirm-Ausgang.
- Ping: Server sendet nach der Anmeldung alle 5 s `{"t":"ping"}`, beantwortet `ping` mit `pong` und WebSocket-Ping-Rahmen mit Pong; 15 s ohne empfangene Daten trennen die Verbindung.
- Beenden der App: Close 1001; mit „Beim Beenden ausschalten“ vorher ein schwarzes Bild.

## Abgleich 24.10.2026 (unveröffentlicht)

Historischer Implementierungsstand vom 24.10.2026: Das dort beschriebene gemeinsame Größen- und Bildmodell („Größenbesitzer“, „Andere Geräte teilen das Bild“) gilt nicht mehr. Aktuell werden Größe, Frames und JPEG-Zwischenspeicher je Gerät separat verwaltet; Geräte können synchron oder unabhängig navigieren. Die Größenlimits (2560 Pixel je Kante, 4.194.304 Pixel) gelten weiterhin.

- `hello.w/h` und `resize.w/h` bestimmen nach erfolgreicher Anmeldung die Companion-Größe; maximal 2560 Pixel je Kante und 4.194.304 Pixel, größere Größen proportional reduziert. Erstes angemeldetes Gerät mit Größe bleibt Größenbesitzer bis zur Trennung. Andere Geräte teilen das Bild; keine unabhängige Ansicht pro Handy.
- `ready.w/h` werden mit dem JPEG-Snapshot gespeichert und vor einem Bild mit neuen Abmessungen aktualisiert. Ein vorübergehendes initiales `ready` darf noch die Startgröße melden. Erst das folgende `ready` zum neuen Bild bestätigt dessen neue Renderkoordinaten.
- `swipe.dir`: `left` = +1, `right` = -1; aus Kompatibilität auch +1/-1 wie im auf dem Redmi getesteten Schnellfix. Nur nach Anmeldung; zusammen mit `touch` höchstens 10/s. Touch mit inzwischen veralteten Bildabmessungen wird verworfen.
- USB/HDMI und Companion haben getrennte Seitenfilter, Navigation, Timer und Touch-Ziele. Datenquellen und Design bleiben gemeinsam. Konfiguration/Bedienung: [DE](COMPANION.md), [EN](COMPANION.en.md).

## Geräteseitiger Seitenübergang (Fähigkeit `xfade1`, Stufe 3, in Entwicklung)

Bisher rechnet der PC jeden Zwischenframe eines Seitenübergangs (0,45 s), kodiert ihn als JPEG und sendet ihn; bei WLAN-Jitter ruckelt das. Fähige Geräte bekommen stattdessen nur das Zielbild plus eine Übergangsnachricht und animieren selbst (display-synchron). Protokollversion bleibt `v` = 1; alte Clients und alte Server arbeiten unverändert weiter.

- **Fähigkeiten im `hello`:** optionales Feld `"caps": ["xfade1"]` (Strings aus `[a-z0-9_-]`, höchstens 32 Zeichen, höchstens 16 Einträge, Duplikate und ungültige Einträge werden still ignoriert; kein Array = keine Fähigkeiten). Die Regel „genau eines von `key`/`pin`“ ist unverändert. Alte Clients senden das Feld nicht; alte Server ignorieren es.
- **Übergangsnachricht (S->C, Text-Frame) unmittelbar vor dem Binärframe des Zielbilds:**
  `{"t":"transition","style":"hud"|"slide"|"fade","dir":1|-1,"ms":450,"accent":"#RRGGBB","accent2":"#RRGGBB","shade":"#RRGGBB"}`
  `dir`: +1 = nächste Seite (Inhalt wandert nach links), -1 = vorherige. `accent`/`accent2` sind Primär-/Sekundärfarbe der Hardware-HUD-Palette, `shade` die Abdunkelungsfarbe des HUD-Übergangs (Großbuchstaben-Hex). Nur `ms` = 450 ist heute üblich; der Client verwendet den gesendeten Wert.
- **Client:** hält das aktuell angezeigte Bild als „von“, animiert zum nächsten empfangenen Binärframe („nach“) in `ms` Millisekunden und zeigt danach das Zielbild. Easing wie am PC (`render.cpp`): `slide` und `fade` ease-in-out kubisch, `hud` ease-in-out quintisch. Ohne vorheriges Bild oder bei geänderter Bildgröße (`ready`) wird nicht animiert.
- **Server pro Gerät:** Hat das Gerät `xfade1` und der Stil ist nicht `none`, entstehen für dieses Gerät keine Zwischenframes. Der Server sendet einmal Übergangsnachricht + Zielbild und hält danach den Frame-Versand dieses Geräts `ms` + 80 ms zurück (nichts wird verworfen: danach geht der neueste Frame hinaus, falls sich das Bild geändert hat). Der Server rendert in dieser Zeit nicht mit 60 fps weiter; Touch und Wischen bleiben bedienbar. Das erste Bild einer Verbindung und Bilder mit geänderter Größe bekommen nie eine Übergangsnachricht. Geräte ohne Fähigkeit (alte Companion-App, Monitor, serielles Panel) behalten das bisherige Verhalten (am PC gerechnete Zwischenframes); die Entscheidung fällt strikt pro Gerät. Spiel-Intro und andere Animationen bleiben unverändert.
- **Telemetrie (`SB_FRAME_TRACE`):** Die Übergangsnachricht erscheint als eigene Kommentarzeile `#transition,<ts_us>,<device>,<seq>,<ms>,<style>` direkt vor der Zeile des Zielbilds; die Frame-Spalten sind unverändert (`frame_trace_stats.py` überspringt `#`-Zeilen).

## Device-side page transition (capability `xfade1`, stage 3, in development)

Until now the PC computed every intermediate frame of a page transition (0.45 s), JPEG-encoded it and sent it, which stutters under Wi-Fi jitter. Capable devices now receive only the target frame plus a transition message and animate on their own (display-synchronised). The protocol version stays `v` = 1; old clients and old servers keep working unchanged.

- **Capabilities in `hello`:** optional field `"caps": ["xfade1"]` (strings of `[a-z0-9_-]`, at most 32 characters, at most 16 entries; duplicates and invalid entries are silently ignored; a non-array means no capabilities). The "exactly one of `key`/`pin`" rule is unchanged. Old clients do not send the field; old servers ignore it.
- **Transition message (S->C, text frame) immediately before the binary frame of the target image:**
  `{"t":"transition","style":"hud"|"slide"|"fade","dir":1|-1,"ms":450,"accent":"#RRGGBB","accent2":"#RRGGBB","shade":"#RRGGBB"}`
  `dir`: +1 = next page (content moves left), -1 = previous. `accent`/`accent2` are the primary/secondary colours of the hardware-HUD palette, `shade` is the dimming colour of the HUD transition (upper-case hex). `ms` is 450 today; the client uses the value it receives.
- **Client:** keeps the currently shown image as "from", animates to the next received binary frame ("to") over `ms` milliseconds, then shows the target image. Easing as on the PC (`render.cpp`): `slide` and `fade` use ease-in-out cubic, `hud` ease-in-out quintic. With no previous image or after a size change (`ready`) it does not animate.
- **Server, per device:** If the device has `xfade1` and the style is not `none`, no intermediate frames are produced for it. The server sends the transition message + target frame once and then holds back that device's frame delivery for `ms` + 80 ms (nothing is dropped: afterwards the newest frame goes out if the image changed). The server does not keep rendering at 60 fps meanwhile; touch and swipe stay responsive. The first image of a connection and images with a changed size never get a transition message. Devices without the capability (old Companion app, monitor, serial panel) keep today's behaviour (server-computed intermediate frames); the decision is strictly per device. Game intro and other animations are unchanged.
- **Telemetry (`SB_FRAME_TRACE`):** the transition message is logged as its own comment line `#transition,<ts_us>,<device>,<seq>,<ms>,<style>` right before the target frame's row; the frame columns are unchanged (`frame_trace_stats.py` skips `#` lines).
