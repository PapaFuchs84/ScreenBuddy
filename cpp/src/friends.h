// © 2026 FuchsLabs
// Gemeinsame Freundesliste: führt Discord, Steam und Xbox zu einer Liste zusammen.
// Steam: offizielle Web-API (Freundesliste im Profil muss öffentlich sein). Xbox: Dienst OpenXBL.
// Dieselbe Person auf mehreren Plattformen erscheint nur einmal – automatisch bei gleichem Namen oder manuell
// über „Verknüpfungen“ (Einstellungsseite).
#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::discord {
class DiscordFeed;
}

namespace fl::friends {

struct Account {
    std::string platform, uid, name;
    std::string status = "offline";
    std::string activity;
    bool mobile = false;
    std::string avatar_url;
    ImagePtr avatar;
    std::string handle;  // Xbox: Gamertag wie geliefert (name ist bereinigt)
};

struct FriendsError : std::runtime_error {  // Meldung, die der Nutzer versteht
    using std::runtime_error::runtime_error;
};

std::vector<Account> parse_steam_players(const nlohmann::json& data);
std::vector<Account> parse_xbox_people(const nlohmann::json& data);
std::vector<Account> fetch_steam(const std::string& key, const std::string& steam_id);
std::vector<Account> fetch_xbox(const std::string& key);
std::vector<Friend> merge_accounts(const std::vector<Account>& accounts, const std::vector<std::string>& links, bool auto_merge);
ojson suggest_links(const ojson& accounts, const std::vector<std::string>& links, bool auto_merge, size_t limit = 12);
std::string norm(const std::string& name);
double similarity(const std::string& a, const std::string& b);  // wie difflib.SequenceMatcher.ratio()

class FriendsHub {
public:
    FriendsHub(const ojson& cfg, discord::DiscordFeed* discord, std::function<void()> on_change);
    ~FriendsHub();
    void start();
    void stop();

    std::string status();             // Fehler der Plattformen, mit " · " verbunden
    ojson info(const std::string& platform);  // ok, name, count, online, at, error – oder null
    std::vector<Friend> people();
    ojson all_accounts();

private:
    void loop(const std::string& platform, int every);
    void load_avatars(std::vector<Account>& accs);
    bool sleep_for(double seconds);

    ojson cfg_;
    discord::DiscordFeed* discord_;
    std::function<void()> on_change_;
    std::map<std::string, std::vector<Account>> accounts_{{"steam", {}}, {"xbox", {}}};
    std::map<std::string, std::string> errors_;
    std::map<std::string, ojson> info_;
    std::map<std::string, ImagePtr> avatars_;
    std::map<std::string, bool> avatar_failed_;
    std::mutex mu_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::friends
