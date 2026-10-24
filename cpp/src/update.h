// © 2026 FuchsLabs
// Update-Hinweis: fragt höchstens einmal am Tag die öffentlichen GitHub-Releases ab (ohne Zugangsschlüssel) und
// meldet eine neuere Version. Es wird nie etwas heruntergeladen oder installiert – der Nutzer entscheidet selbst.
#pragma once

#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

namespace fl::update {

inline constexpr const char* RELEASES_API = "https://api.github.com/repos/PapaFuchs84/fuchslabs-usb-display/releases/latest";
inline constexpr const char* RELEASES_PAGE_PREFIX = "https://github.com/PapaFuchs84/fuchslabs-usb-display/";

struct Release {
    std::string version;   // "2.2.0" (ohne "v")
    std::string url;       // Seite des Releases auf GitHub
};

std::optional<std::vector<int>> parse_version(const std::string& tag);   // "v2.1.0" → {2,1,0}; Vorabversion/ungültig → nullopt
bool is_newer(const std::string& latest, const std::string& current);
// Antwort der GitHub-API; nur veröffentlichte Releases dieses Projekts, Adresse muss auf github.com/PapaFuchs84/… zeigen
std::optional<Release> parse_release(const nlohmann::json& data);

class UpdateChecker {
public:
    UpdateChecker(std::filesystem::path state_file, std::function<void(const Release&)> on_new);
    ~UpdateChecker();
    void start();
    void stop();
    nlohmann::json info();     // available, current, latest, url, checked_at, error
    bool check_now();          // manuell; höchstens alle 5 Minuten, false = zu früh

private:
    void loop();
    bool fetch();              // eine Abfrage; true = Antwort gelesen
    void load();
    void save();
    void announce_locked(std::unique_lock<std::mutex>& lk);

    std::filesystem::path file_;
    std::function<void(const Release&)> on_new_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::thread th_;
    bool stop_ = false, wake_ = false;
    long long checked_at_ = 0, last_attempt_ = 0;
    std::string latest_, url_, notified_, error_;
};

}  // namespace fl::update