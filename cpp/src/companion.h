// © 2026 FuchsLabs
// Companion-Protokoll v1 (docs/COMPANION-PROTOKOLL.md): plattformneutrale Bausteine für den Netzwerk-Ausgang.
// WebSocket-Rahmen und -Handshake, JSON-Nachrichten, PIN-Kopplung mit Sperre, Gerätespeicher, Kopplungs-URI und
// QR-Code als SVG. Kein Windows-Code: läuft auch in den Linux-Tests (cpp/tests/companion_tests.cpp).
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fl::companion {

using json = nlohmann::ordered_json;

inline constexpr int PROTOCOL_VERSION = 1;
inline constexpr int DEFAULT_PORT = 8777;
inline constexpr int SETTINGS_PORT = 8765;          // Port der Einstellungsseite: für den Companion-Server verboten
inline constexpr const char* SERVICE_TYPE = "_fuchslabs-display._tcp";
inline constexpr double PIN_SECONDS = 120;          // PIN gilt 2 Minuten
inline constexpr int MAX_FAILURES = 5;              // danach Sperre
inline constexpr double LOCK_SECONDS = 300;         // Dauer der Sperre
inline constexpr double PING_SECONDS = 5;
inline constexpr double DEAD_SECONDS = 15;
inline constexpr double MIN_FRAME_SECONDS = 0.2;    // höchstens 5 Bilder/s
inline constexpr size_t MAX_DEVICES = 8;
inline constexpr size_t MAX_DEVICE_PAGES = 64;      // Einträge in der eigenen Seitenliste eines Geräts
inline constexpr size_t MAX_TEXT_MESSAGE = 16 * 1024;
inline constexpr size_t MAX_BUFFERED = 4 * MAX_TEXT_MESSAGE;  // ungeparste Eingangsdaten je Verbindung, danach Trennung
inline constexpr size_t READ_BUDGET = 2 * MAX_TEXT_MESSAGE;   // höchstens so viel lesen, bevor wieder geparst wird
inline constexpr double HANDSHAKE_SECONDS = 10;     // TLS + HTTP-Upgrade + hello zusammen, ab accept()
inline constexpr uint8_t FRAME_JPEG = 1;

// ---------- Krypto-Helfer (OpenSSL) ----------
std::string random_bytes(size_t n);                 // wirft std::runtime_error, wenn der Zufallsgenerator versagt
std::string base64(const std::string& data);
std::string unbase64(const std::string& text, bool* ok = nullptr);
std::string sha256_hex(const std::string& data);    // 64 Zeichen, Kleinbuchstaben
bool equal_ct(const std::string& a, const std::string& b);  // Vergleich in konstanter Zeit
std::string random_pin();                           // 6 Ziffern, gleichverteilt

// ---------- Netz ----------
bool is_lan_ipv4(const std::string& ip);            // 10/8, 172.16/12, 192.168/16 (private Heimnetz-Adressen)
bool is_loopback_ipv4(const std::string& ip);
bool valid_port(int port, int settings_port = SETTINGS_PORT);  // 1024..65535, nie der Port der Einstellungsseite

// ---------- WebSocket (RFC 6455, Serverseite) ----------
bool valid_utf8(const std::string& s);              // streng: keine Überlangen, keine Surrogate, höchstens U+10FFFF
// Close-Payload: leer, oder 2 Byte Code (1000–1003, 1007–1014, 3000–4999) plus UTF-8-Grund
bool valid_close_payload(const std::string& payload);
std::string ws_accept(const std::string& client_key);

struct HttpRequest {
    std::string method, target, version;
    std::map<std::string, std::string> headers;     // Namen in Kleinbuchstaben
};
std::optional<HttpRequest> parse_http_request(const std::string& raw);  // raw endet mit \r\n\r\n

struct Upgrade {
    bool ok = false;
    std::string response;                           // komplette HTTP-Antwort (101 oder Fehler)
    std::string reason;                             // für das Protokoll
};
// Prüft den Upgrade-Request. Abgelehnt werden u. a. fehlende/ungültige Schlüssel, falsche Version und jeder
// Origin-Header (Browser-Seiten sollen den Dienst nicht ansprechen; die App sendet keinen).
Upgrade ws_upgrade(const std::string& raw);

enum class Op : uint8_t { Cont = 0, Text = 1, Binary = 2, Close = 8, Ping = 9, Pong = 10 };
std::string ws_frame(Op op, const std::string& payload, std::optional<uint32_t> mask = std::nullopt);
std::string ws_close_frame(uint16_t code, const std::string& reason = "");

struct WsMessage {
    Op op = Op::Text;
    std::string payload;
};

// Zerlegt eingehende Client-Rahmen (maskiert, wie RFC 6455 verlangt), setzt Fragmente zusammen. Längen müssen
// minimal kodiert sein, Close-Rahmen einen gültigen Code/Grund tragen; sonst Error.
class WsParser {
public:
    enum class Status { NeedMore, Message, Error };
    explicit WsParser(size_t max_message = MAX_TEXT_MESSAGE, bool require_mask = true)
        : max_(max_message), require_mask_(require_mask) {}
    void feed(const char* data, size_t n) { buf_.append(data, n); }
    void feed(const std::string& s) { buf_ += s; }
    Status next(WsMessage& out, std::string* error = nullptr);
    size_t buffered() const { return buf_.size(); }

private:
    std::string buf_, partial_;
    int partial_op_ = -1;
    size_t max_;
    bool require_mask_;
};

// ---------- Nachrichten ----------
struct ClientMsg {
    enum class Type { Hello, Touch, Swipe, Resize, Ping, Pong, Unknown, Invalid } type = Type::Invalid;
    int v = 0, w = 0, h = 0, x = 0, y = 0, dir = 0;
    std::string key, pin, name, error;
};
std::pair<int, int> render_size(int w, int h);  // bounded, aspect-preserving client resolution
ClientMsg parse_client(const std::string& text);
std::string msg_ready(int w, int h);
std::string msg_paired(const std::string& key_b64);
std::string msg_error(const std::string& code, const std::string& msg);
std::string msg_ping();
std::string msg_pong();
std::string frame_payload(const std::string& jpeg);  // Byte 0 = 1 (JPEG), danach die Daten
std::string clean_name(const std::string& name);     // Gerätename: ohne Steuerzeichen, höchstens 40 Zeichen

// ---------- Kopplung ----------
std::string pair_uri(const std::string& host, int port, const std::string& fingerprint, const std::string& pin);
std::string qr_svg(const std::string& text);         // QR-Code (Fehlerkorrektur M) als eigenständiges SVG
std::string format_fingerprint(const std::string& hex);  // "ab:cd:…" in Großbuchstaben, zum Vorlesen

class Pairing {
public:
    using Clock = std::function<double()>;
    explicit Pairing(Clock now, std::function<std::string()> make_pin = random_pin);
    struct Code {
        std::string pin;
        double expires = 0;
    };
    enum class Result { Ok, Wrong, Expired, NoPin, Locked };
    std::optional<Code> start();                     // nullopt = gesperrt
    Result check(const std::string& pin);            // PIN gilt einmal; 5 Fehlversuche → PIN weg und Sperre
    void cancel();
    std::optional<Code> active();
    double locked_for();                             // verbleibende Sekunden der Sperre (0 = frei)
    int failures();

private:
    void expire_locked();
    std::mutex mu_;
    Clock now_;
    std::function<std::string()> make_pin_;
    std::optional<Code> code_;
    int failures_ = 0;
    double locked_until_ = 0;
};

// Bilder nur bei Änderung und höchstens alle MIN_FRAME_SECONDS; das erste Bild sofort.
class FrameGate {
public:
    bool due(uint64_t seq, double now) const { return seq != 0 && seq != sent_ && (!any_ || now - last_ >= MIN_FRAME_SECONDS - 1e-9); }
    void sent(uint64_t seq, double now) { sent_ = seq; last_ = now; any_ = true; }
    void reset() { sent_ = 0; any_ = false; }

private:
    uint64_t sent_ = 0;
    double last_ = 0;
    bool any_ = false;
};

std::vector<std::string> clean_pages(const std::vector<std::string>& in);  // gültig, eindeutig, begrenzt

// ---------- Gespeicherte Kopplungen und TLS-Identität (companion.json) ----------
// Geräteschlüssel und privater TLS-Schlüssel gehen durch protect() (unter Windows DPAPI, siehe secure.cpp).
struct Device {
    std::string id, name, key;  // key = 32 Byte roh
    double added = 0, last_seen = 0;
    std::optional<std::vector<std::string>> pages;  // eigene Seitenliste (Reihenfolge = Auswahl); leer = Standardliste
};

class Store {
public:
    using Codec = std::function<std::string(const std::string&)>;
    Store(std::filesystem::path path, Codec protect, Codec unprotect, std::function<double()> wall_clock);
    void load();                                     // fehlende Datei = leer; kaputte Einträge werden übersprungen
    void save();
    std::string cert_pem();
    std::string key_pem();
    void set_identity(const std::string& cert_pem, const std::string& key_pem);
    std::string add_device(const std::string& name, std::string* id = nullptr);  // liefert den rohen Schlüssel
    std::optional<Device> find(const std::string& raw_key);  // vergleicht in konstanter Zeit
    bool contains(const std::string& id);
    void seen(const std::string& id);
    bool remove(const std::string& id);
    // Eigene Seitenliste je Gerät (nullopt = die gemeinsame Liste companion.pages). Ungültige Einträge fallen weg.
    std::optional<std::vector<std::string>> device_pages(const std::string& id);
    bool set_device_pages(const std::string& id, const std::optional<std::vector<std::string>>& pages);
    json list();                                     // ohne Schlüssel

private:
    void save_locked();
    std::mutex mu_;
    std::filesystem::path path_;
    Codec protect_, unprotect_;
    std::function<double()> wall_;
    std::string cert_, key_;
    std::vector<Device> devices_;
};

}  // namespace fl::companion
