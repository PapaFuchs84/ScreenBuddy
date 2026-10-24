# Companion-Protokoll v1 (Entwurf)

PC-App (Server) <-> Android-App (Client), nur lokales Netz.

- Erkennung: mDNS `_fuchslabs-display._tcp`, TXT: `v=1`, `name=<PC-Name>`. Fallback: manuelle IP.
- Port: Standard 8777 (nie 8765). Nur an LAN-Adressen binden, nur wenn in den Einstellungen aktiviert (Standard aus).
- Transport: WebSocket über TLS (selbstsigniertes Zertifikat, Pinning des SHA-256-Fingerabdrucks nach dem Koppeln).
- Kopplung: PC zeigt QR-Code (`fuchslabs://pair?host=<ip>&port=8777&fp=<sha256>&pin=<6 Ziffern>`). Client sendet `hello` mit PIN. Server antwortet mit `paired` und einem Geräteschlüssel (32 Byte, base64); PC speichert ihn per DPAPI, Client im Android Keystore. Danach `hello` mit `key`. PIN gilt einmalig, 2 Minuten, 5 Fehlversuche -> Sperre.
- Nachrichten (Text-Frames, JSON): 
  - C->S `{"t":"hello","v":1,"key"|"pin":...,"w":<px>,"h":<px>,"name":"..."}`
  - S->C `{"t":"ready","w":480,"h":320}` (Renderfläche; Prototyp fest 480x320, Client skaliert mit Seitenverhältnis-Letterbox)
  - C->S `{"t":"touch","x":..,"y":..}` in Renderkoordinaten (0..w-1, 0..h-1), je Tipp ein Event
  - beidseitig `{"t":"ping"}` / `{"t":"pong"}` alle 5 s; 15 s Stille = Verbindung tot
- Frames (Binary-Frames): Byte 0 = Typ (1 = vollständiges JPEG, Qualität ~80), danach JPEG-Daten. Der Server sendet nur bei Änderung, höchstens 5 fps; nach Verbindung sofort ein Vollbild.
- Fehler: `{"t":"error","code":"...","msg":"..."}`, danach Close.
- Launcher-Modus (Android): App deklariert HOME/DEFAULT-Intent, startet optional beim Booten, Vollbild (immersive), Bildschirm bleibt an, verbindet automatisch zum zuletzt gekoppelten PC neu.
