// © 2026 FuchsLabs
// Discord-Freunde (Online-Status + Aktivität) über einen eigenen Bot (D++).
// Freundeslisten selbst sind per Bot-API nicht abrufbar: Der Bot sieht die Mitglieder der Server, auf die er
// eingeladen ist (Intents „Presence“ + „Server Members“).
#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::discord {

std::string duration_text(long long start_s_utc);  // " · 42 min" / " · 1 h 05 min"

class DiscordFeed {
public:
    DiscordFeed(const ojson& cfg, std::function<void()> on_change);
    ~DiscordFeed();
    void start();
    void stop();

    bool ready();
    std::string status();       // "" = verbunden, sonst verständliche Meldung
    std::string bot_name();
    int server_count();
    std::vector<Friend> friends();
    ojson member_list();        // alle Mitglieder der beobachteten Server (Auswahl in den Einstellungen)

    struct Activity {
        int type = 0;           // 0 Spielt, 1 Streamt, 2 Hört, 3 Schaut, 4 eigener Status, 5 Tritt an
        std::string name, state, details;
        long long start_ms = 0;
        bool spotify = false;
    };
    struct Member {
        uint64_t id = 0, guild = 0;
        std::string username, global_name, nick, guild_name;
        std::string avatar, guild_avatar;  // Hashes
        bool bot = false;
        std::string status = "offline";
        bool mobile = false;
        std::vector<Activity> activities;
    };

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

}  // namespace fl::discord
