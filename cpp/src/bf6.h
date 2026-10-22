// © 2026 FuchsLabs
// Battlefield-6-Rangliste der Freunde – Daten von der Community-API api.gametools.network.
// EA bietet keine öffentliche Freundesliste an; die Spielernamen stehen in config.toml unter [bf6] players.
#pragma once

#include <atomic>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::bf6 {

std::pair<std::string, std::string> parse_entry(const std::string& entry);  // "Name@psn" → (Name, psn)

class Bf6Stats {
public:
    explicit Bf6Stats(const ojson& cfg);
    ~Bf6Stats();
    void start(std::function<void()> on_update);
    void stop();
    Bf6Player fetch(const std::string& entry);  // auch für die Prüfung in den Einstellungen
    std::vector<Bf6Player> players();
    std::optional<std::time_t> updated();

private:
    std::optional<nlohmann::json> get(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params);
    ImagePtr icon(const std::string& url);
    bool sleep_for(double seconds);

    std::vector<std::string> entries_;
    int refresh_;
    std::vector<Bf6Player> players_;
    std::optional<std::time_t> updated_;
    std::map<std::string, ImagePtr> icons_;
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::bf6
