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
- Frames (Binary-Frames): Byte 0 = Typ (1 = vollständiges JPEG, Qualität ~80), danach JPEG-Daten. Der Server sendet nur bei Änderung, höchstens 5 fps; nach Verbindung sofort ein Vollbild.
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

- `hello.w/h` und `resize.w/h` bestimmen nach erfolgreicher Anmeldung die Companion-Größe; maximal 2560 Pixel je Kante und 4.194.304 Pixel, größere Größen proportional reduziert. Erstes angemeldetes Gerät mit Größe bleibt Größenbesitzer bis zur Trennung. Andere Geräte teilen das Bild; keine unabhängige Ansicht pro Handy.
- `ready.w/h` werden mit dem JPEG-Snapshot gespeichert und vor einem Bild mit neuen Abmessungen aktualisiert. Ein vorübergehendes initiales `ready` darf noch die Startgröße melden. Erst das folgende `ready` zum neuen Bild bestätigt dessen neue Renderkoordinaten.
- `swipe.dir`: `left` = +1, `right` = -1; aus Kompatibilität auch +1/-1 wie im auf dem Redmi getesteten Schnellfix. Nur nach Anmeldung; zusammen mit `touch` höchstens 10/s. Touch mit inzwischen veralteten Bildabmessungen wird verworfen.
- USB/HDMI und Companion haben getrennte Seitenfilter, Navigation, Timer und Touch-Ziele. Datenquellen und Design bleiben gemeinsam. Konfiguration/Bedienung: [DE](COMPANION.md), [EN](COMPANION.en.md).
