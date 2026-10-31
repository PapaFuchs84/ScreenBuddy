// © 2026 FuchsLabs
// Streaming-Neustarts: neue Filme und Serien der letzten 30 Tage bei den gewählten Streaming-Anbietern.
// Quelle: TMDB API v3 (nur GET über HTTPS). Der Zugangsschlüssel liegt wie die anderen DPAPI-geschützt in der
// Konfiguration und taucht nie in Protokoll oder Anzeige auf. Cache: 1 Stunde, wenige Anfragen je Aktualisierung.
// This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.
#pragma once

#include <atomic>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"
#include "tmdb_logic.h"

namespace fl::tmdb {

struct TmdbError : std::runtime_error {   // Meldung, die der Nutzer versteht (enthält nie den Schlüssel)
    using std::runtime_error::runtime_error;
};

// Anbieterliste der Region (Filme und Serien zusammengeführt) für die Auswahl in den Einstellungen
std::vector<Provider> list_providers(const std::string& key, const std::string& region, const std::string& lang);

class TmdbFeed {
public:
    TmdbFeed(const ojson& cfg, std::string lang);
    ~TmdbFeed();
    void start(std::function<void()> on_update);
    void stop();

    struct Snapshot {
        State state = State::Loading;
        std::string message;                 // bei Error: Grund für das Protokoll/die Einstellungen (ohne Schlüssel)
        std::vector<StreamItem> items;
        std::optional<std::time_t> updated;
    };
    Snapshot snapshot();

private:
    struct Got {
        int status = 0;
        std::string body;
    };
    Got get(const std::string& path);
    void refresh();
    void set_error(const std::string& msg);
    bool sleep_for(double seconds);

    std::string key_, region_, lang_;
    std::vector<Selected> sel_;
    int max_items_, refresh_minutes_;
    Snapshot snap_;
    double retry_s_ = 0;                     // > 0: nach Fehler früher erneut versuchen
    std::map<std::string, std::vector<std::string>> kuerzel_cache_;   // "m:ID" bzw. "s:ID" → Anbieter-Kürzel
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::tmdb
