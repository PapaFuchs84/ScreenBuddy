// © 2026 FuchsLabs
// Companion-Server: WebSocket über TLS (OpenSSL, selbstsigniertes Zertifikat) nur an Heimnetz-Adressen.
// Plattformneutral (Winsock bzw. BSD-Sockets); Bildkodierung, Schlüsselschutz und Touch kommen als Hooks von außen
// (unter Windows: NetworkDisplay in network_display.cpp).
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "companion.h"

namespace fl::companion {

struct Identity {
    std::string cert_pem, key_pem;
};
Identity generate_identity(const std::string& common_name);   // ECDSA P-256, 10 Jahre gültig
std::string cert_fingerprint(const std::string& cert_pem);    // SHA-256 über DER, 64 Hex-Zeichen; leer = ungültig

class Server {
public:
    struct Options {
        std::vector<std::string> bind;   // IPv4-Adressen; nur Heimnetz (oder Loopback, wenn allow_loopback)
        int port = DEFAULT_PORT;
        int settings_port = SETTINGS_PORT;
        int w = 480, h = 320;            // Renderfläche für "ready"
        bool allow_loopback = false;     // nur für Tests
        int max_clients = 4;
        double handshake_seconds = HANDSHAKE_SECONDS;  // TLS + Upgrade + hello, ab accept() (Tests: kürzer)
    };
    struct Hooks {
        std::function<std::string()> encode;           // aktuelles Bild als JPEG (leer = noch keins)
        std::function<void(int, int)> on_touch;        // Renderkoordinaten, aus dem Verbindungs-Thread
        std::function<void(int, const std::string&)> log;  // 0 Debug, 1 Info, 2 Warnung
        std::function<void()> on_change;               // Clients/Kopplungen geändert (für die Einstellungsseite)
        std::function<double()> now;                   // monotone Sekunden (Tests: steuerbar)
        std::function<void(const std::string&)> before_auth;  // nur Tests: nach Schlüssel-/PIN-Prüfung, vor der Anmeldung
    };

    Server(Options opt, Store& store, Hooks hooks);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();                        // wirft std::runtime_error (kein Heimnetz, Port belegt, TLS)
    void stop();
    bool running() const { return running_; }
    void frame_changed();                // neues Bild vorhanden: Clients holen es (höchstens 5/s)
    std::string fingerprint() const { return fingerprint_; }
    const Options& options() const { return opt_; }
    Pairing& pairing() { return pairing_; }
    json clients();                      // verbundene, angemeldete Geräte
    void drop_device(const std::string& id);  // Verbindung eines entfernten Geräts trennen

private:
    struct Conn;
    void accept_loop();
    void serve(std::shared_ptr<Conn> c);  // Thread-Einstieg: fängt alles, räumt auf
    void session(Conn& c, void*& ssl, bool& was_authed);
    void log(int lv, const std::string& msg) const;
    double now() const;

    Options opt_;
    Store& store_;
    Hooks hooks_;
    Pairing pairing_;
    std::string fingerprint_;
    void* ctx_ = nullptr;                // SSL_CTX*
    std::vector<intptr_t> listeners_;
    std::thread accept_thread_;
    std::atomic<bool> running_{false}, stop_{false};
    std::atomic<uint64_t> seq_{0};
    std::mutex mu_;
    std::list<std::pair<std::shared_ptr<Conn>, std::thread>> workers_;
    void reap(bool all);
    std::mutex jpeg_mu_;
    uint64_t jpeg_seq_ = 0;
    std::shared_ptr<const std::string> jpeg_;
};

}  // namespace fl::companion
