// © 2026 FuchsLabs
// Home Assistant: bis zu 4 Entitäten (Sensoren, Schalter) über die REST-API abfragen und Schalter umlegen.
// Zugang: Adresse der Installation + „Langlebiges Zugriffstoken“ (Profil in Home Assistant). Das Token liegt wie die
// anderen Geheimnisse DPAPI-verschlüsselt in der Konfiguration.
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::ha {

inline constexpr size_t MAX_ENTITIES = 4;

struct HaError : std::runtime_error {  // Meldung, die der Nutzer versteht
    using std::runtime_error::runtime_error;
};

struct Spec {
    std::string id, label;  // label leer = Name aus Home Assistant
};

// "sensor.temp" oder "sensor.temp|Wohnzimmer"; ungültige Einträge → nullopt
std::optional<Spec> parse_spec(const std::string& entry);
bool toggle_domain(const std::string& entity_id);   // switch, input_boolean, light, fan – nichts Sicherheitsrelevantes (kein lock/cover/alarm)
std::string base_url(const std::string& url);       // ohne abschließenden Schrägstrich; leer = ungültig
HaEntity parse_state(const nlohmann::json& data, const Spec& spec, bool touch);
std::string format_value(const std::string& state);   // "21.53" → "21,5"; Nicht-Zahlen unverändert

// Alle Entitäten für die Auswahl in den Einstellungen (nur Sensoren und – mit Touch – Schalter)
nlohmann::json list_entities(const std::string& url, const std::string& token, bool touch);

class HaClient {
public:
    HaClient(const ojson& cfg, bool touch, std::function<void()> on_change);
    ~HaClient();
    void start();
    void stop();

    std::vector<HaEntity> entities();
    std::string status();                       // Fehlertext, leer = alles in Ordnung
    ojson info();                               // ok, count, at, error – für die Verbindungsanzeige
    bool has_data();
    bool configured() const { return !url_.empty() && !token_.empty() && !specs_.empty(); }
    void request_toggle(const std::string& entity_id);   // asynchron; die Anzeige zeigt danach den echten Zustand

private:
    void loop();
    void poll_all();
    bool sleep_until_wake(double seconds);

    std::string url_, token_;
    bool touch_;
    std::vector<Spec> specs_;
    std::function<void()> on_change_;
    std::vector<HaEntity> entities_;
    std::string error_;
    double ok_at_ = 0;
    std::vector<std::string> pending_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::ha
