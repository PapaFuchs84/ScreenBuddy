// © 2026 FuchsLabs
// Streaming-Neustarts: neue Filme und Serien der letzten 30 Tage bei den gewählten Streaming-Anbietern.
// Quelle: TMDB API v3 (nur GET über HTTPS). Der Zugangsschlüssel liegt wie die anderen DPAPI-geschützt in der
// Konfiguration und taucht nie in Protokoll oder Anzeige auf. Cache: 1 Stunde, wenige Anfragen je Aktualisierung.
// This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.
#pragma once

#include <atomic>
#include <ctime>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
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
    // logo_dir: Cache der Anbieter-Logos (<id>.png); leer = Logos nur im Speicher, ohne Datei
    TmdbFeed(const ojson& cfg, std::string lang, std::filesystem::path logo_dir = {});
    ~TmdbFeed();
    void start(std::function<void()> on_update);
    void stop();

    struct Snapshot {
        State state = State::Loading;
        std::string message;                 // bei Error: Grund für das Protokoll/die Einstellungen (ohne Schlüssel)
        std::vector<StreamItem> items;
        StreamLogos logos;                   // geladene Anbieter-Logos; fehlende ersetzt die Seite durch das Kürzel
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
    void load_logos(const std::vector<Item>& items);   // im Arbeits-Thread: Cache lesen, sonst einmal laden
    ImagePtr load_logo(int id, const std::string& logo_path);
    void set_error(const std::string& msg);
    bool sleep_for(double seconds);

    std::string key_, region_, lang_;
    std::vector<Selected> sel_;
    int max_items_, refresh_minutes_;
    Snapshot snap_;
    double retry_s_ = 0;                     // > 0: nach Fehler früher erneut versuchen
    std::map<std::string, std::vector<ProviderMatch>> kuerzel_cache_;   // "m:ID" bzw. "s:ID" → Anbieter (Kürzel, Logo)
    std::filesystem::path logo_dir_;
    std::map<int, std::string> logo_paths_;   // Provider-ID → logo_path (nur Arbeits-Thread)
    StreamLogos logos_;                        // geladene Logos (nur Arbeits-Thread)
    std::set<int> logo_failed_;                // in dieser Sitzung nicht erneut versuchen
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::tmdb
