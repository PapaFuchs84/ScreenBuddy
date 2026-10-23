// © 2026 FuchsLabs
// Ziele gehören zum gezeichneten Datenstand, nicht zum später aktualisierten Feed.
#pragma once
#include <functional>
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
struct Tile {
    render::Box box;
    bool rss = false;
    Rows rows;
};
Targets friend_targets(const Friend& person);
Targets player_targets(const Bf6Player& player);
Rows friend_rows(std::vector<Friend> people, int page, int per);
Rows player_rows(std::vector<Bf6Player> players, int page, int per);
Rows post_rows(const std::vector<Post>& posts);
const Targets* at(const std::vector<Tile>& tiles, int x, int y);
// Abhängigkeiten injizierbar: Tests öffnen keine echten Chats/Browser.
bool dispatch(const Targets& targets, const std::function<bool(const std::string&)>& available,
              const std::function<bool(const std::string&)>& launch);
bool open(const Targets& targets);
}  // namespace fl::touch
