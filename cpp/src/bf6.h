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

std::pair<std::string, std::string> parse_entry(const std::string& entry);  // "Name@psn" bzw. "Name@psn#id" → (Name, psn)
// Eintrag mit optionaler Spieler-ID: "Name", "Name@psn", "Name#id", "Name@psn#id" (id = mind. 6 Ziffern, damit
// Gamertags wie "Fox#1234" Namen bleiben)
struct Entry {
    std::string name, platform, id;
};
Entry parse_entry_full(const std::string& entry);
// ID aus einem JSON-Wert (Zahl oder Text); leer bei null/0/false
std::string json_id(const nlohmann::json& v);
// Treffer der Spielersuche (/bf6/player) → ID; nur Treffer mit gleichem Namen (exakt vor ohne Groß/Klein), sonst leer
std::string pick_search_id(const nlohmann::json& response, const std::string& name);
// Plattform-Werte der Stats-API in Reihenfolge der Versuche (die API hat "ea" durch "pc" ersetzt, alte Werte als Fallback)
std::vector<std::string> platform_candidates(const std::string& plat);
// Namensvarianten: Original, danach klein geschrieben (die API findet manche Konten nur so)
std::vector<std::string> name_candidates(const std::string& name);

class Bf6Stats {
public:
    explicit Bf6Stats(const ojson& cfg);
    ~Bf6Stats();
    void start(std::function<void()> on_update);
    void stop();
    Bf6Player fetch(const std::string& entry);  // auch für die Prüfung in den Einstellungen
    std::vector<Bf6Player> players();
    std::optional<std::time_t> updated();
    // Auto-Heilung: wird einmal je Eintrag mit dem um "#id" ergänzten Eintrag aufgerufen (Aufrufer speichert ihn in der Config)
    void set_on_heal(std::function<void(const std::string& entry, const std::string& healed)> f) { on_heal_ = std::move(f); }

private:
    std::optional<nlohmann::json> get(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params);
    ImagePtr icon(const std::string& url);
    bool sleep_for(double seconds);

    std::vector<std::string> entries_;
    int refresh_;
    std::vector<Bf6Player> players_;
    std::optional<std::time_t> updated_;
    std::map<std::string, ImagePtr> icons_;
    std::map<std::string, std::string> ids_;  // Eintrag → zuletzt gefundene ID (Speicher-Cache)
    std::function<void(const std::string&, const std::string&)> on_heal_;
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::bf6
