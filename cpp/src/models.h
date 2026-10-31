// © 2026 FuchsLabs
// Daten, die zwischen Quellen (Discord, Steam, BF6, RSS, Streaming, System) und den Seiten ausgetauscht werden.
#pragma once

#include <cstdint>
#include <ctime>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "gfx.h"
#include "tmdb_logic.h"

namespace fl {

struct Friend {
    uint64_t user_id = 0;
    std::string name;
    std::string status = "offline";  // online | idle | dnd | offline
    std::string activity;            // z. B. "Spielt Battlefield 6 · 23 min"
    bool mobile = false;
    ImagePtr avatar;
    std::vector<std::string> platforms;  // z. B. {"discord", "steam"} – kleine Plattform-Marken rechts in der Zeile
    std::map<std::string, std::string> ids;  // Plattform → Discord-ID, SteamID64 bzw. Xbox-Gamertag (für „Tippen öffnet Chat“)
    std::vector<std::string> online_on;      // Plattformen, auf denen die Person gerade online ist
};

struct Bf6Player {
    std::string query;  // Eintrag aus der Konfiguration
    std::string name;
    std::optional<int> rank;
    std::string rank_name;
    std::optional<double> kd, win_pct, kpm, hours;
    ImagePtr rank_icon;
    std::string error;  // z. B. "nicht gefunden"
};

struct Post {
    std::string source, title;
    std::optional<std::time_t> published;  // UTC
    std::string link;
};

// Neuer Film bzw. neue Serie bei einem Streaming-Anbieter (Seite „Streaming-Neustarts“, Quelle TMDB)
using StreamItem = tmdb::Item;
using StreamState = tmdb::State;

// Eine Home-Assistant-Entität (Sensor oder Schalter) in dem Zustand, den die Seite zeichnet
struct HaEntity {
    std::string id, name;
    std::string state;               // Rohwert von Home Assistant, z. B. "21.5", "on", "unavailable"
    std::string unit;                // z. B. "°C"
    bool toggle = false;             // per Tipp schaltbar (Schalter, Licht, …) und Touch ist an
    bool on = false;
    bool available = false;          // false = noch nicht abgefragt oder Home Assistant meldet "unavailable"/"unknown"
};

struct SysSnapshot {
    std::string cpu_name = "CPU";
    std::optional<double> cpu_pct, cpu_ghz, cpu_temp, cpu_fan;  // Lüfter in RPM
    std::string gpu_name = "GPU";
    std::optional<double> gpu_pct, gpu_temp, gpu_fan;           // Lüfter in %
    std::optional<double> gpu_mem_used, gpu_mem_total;          // MiB
    std::optional<double> ram_pct, ram_used, ram_total;         // GiB
    std::string disk_label = "C:";
    std::optional<double> disk_pct, disk_temp;
    std::string net_name;
    std::optional<double> net_down, net_up;                     // Bytes/s
    std::optional<uint64_t> net_link;                           // bit/s
    std::deque<double> down_hist, up_hist;                      // letzte 60 s
    bool hwinfo = false;
};

}  // namespace fl
