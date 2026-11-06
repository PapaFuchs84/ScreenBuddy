// © 2026 FuchsLabs
// Ziele gehören zum gezeichneten Datenstand, nicht zum später aktualisierten Feed.
#pragma once
#include <functional>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "models.h"
#include "render.h"

namespace fl::touch {
struct Target {
    std::string url, protocol;  // leer = Webbrowser; sonst registrierte Desktop-App
};
using Targets = std::vector<Target>;
using Rows = std::vector<Targets>;
struct PluginTarget {
    render::Box rect_px;
    std::string plugin, block, zone, action, url;
    uint64_t generation = 0;
    double stable_since = 0;
    bool transitioning = false;
};
// Rechtecke relativ zur Kachel, aus demselben Rasterplan wie das veröffentlichte Bild.
struct ArrangedTarget {
    render::Box rect_px;
    Targets links;
    std::function<void()> action;
};
struct Tile {
    render::Box box;
    bool rss = false;
    Rows rows;
    bool streaming = false;
    bool sys = false;                             // Systemansicht: cells[0..2] = RAM, Datenträger, Netzwerk
    bool spotify = false;                         // Spotify: cells[0..2] = Wiedergabe, cells[3] = Cover öffnet App
    bool ha = false;                              // Home-Assistant-Kacheln: je Kachel eine Aktion (Schalter umlegen) oder leer
    std::vector<std::function<void()>> cells;
    std::vector<PluginTarget> plugin;
    bool arranged = false; // auch eine leere Zielkarte unterbindet den festen Trefferweg
    std::vector<ArrangedTarget> arranged_targets;
};
const PluginTarget* plugin_at(const std::vector<Tile>& tiles, int x, int y);
// Reine Entscheidung auf dem veröffentlichten Bild; kein Browser, keine Navigation.
struct PluginAccess { bool enabled = false, consent = false; uint64_t generation = 0; std::vector<std::string> hosts; };
struct PluginDecision {
    enum Kind { none, swallow, open_link, next } kind = none;
    std::string plugin, zone, url;
    uint64_t generation = 0;
};
PluginDecision plugin_decide(const std::vector<Tile>&, int x, int y, double now, double stable_min,
                            const std::function<PluginAccess(const std::string&)>& access);
void stabilize_plugins(std::vector<Tile>& current, const std::vector<Tile>& previous, double now);
bool plugin_dispatch(const PluginDecision&, double now, std::map<std::string,double>& last_open,
                     const std::function<bool(const std::string&)>& launch);
Targets friend_targets(const Friend& person);
Targets player_targets(const Bf6Player& player);
Rows friend_rows(std::vector<Friend> people, int page, int per);
Rows player_rows(std::vector<Bf6Player> players, int page, int per);
Rows post_rows(const std::vector<Post>& posts);
Rows stream_rows(const std::vector<StreamItem>& items);
// Schalt-Aktion der angetippten Home-Assistant-Kachel (nullptr = keine)
const std::function<void()>* action_at(const std::vector<Tile>& tiles, int x, int y);
const Targets* at(const std::vector<Tile>& tiles, int x, int y);
// Abhängigkeiten injizierbar: Tests öffnen keine echten Chats/Browser.
bool dispatch(const Targets& targets, const std::function<bool(const std::string&)>& available,
              const std::function<bool(const std::string&)>& launch);
bool open(const Targets& targets);
// Systemansicht: RAM → Task-Manager, Datenträger → „Programme und Features“, Netzwerk → Geschwindigkeitstest im Browser
bool open_system(int zone);
}  // namespace fl::touch
