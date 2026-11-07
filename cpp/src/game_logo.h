// © 2026 FuchsLabs
#pragma once
#include "game_detect.h"
#include "gfx.h"
#include <functional>
namespace fl::game {
class LogoCache {
public:
    using Fetch=std::function<std::string(const std::string&)>;
    explicit LogoCache(std::filesystem::path directory, Fetch fetch={});
    // Worker-thread only. Numeric Steam IDs are the only remote cache keys.
    ImagePtr load(const Game& game);
    static bool fresh(double age_seconds) { return age_seconds>=0 && age_seconds<7*24*3600; }
    static std::vector<std::string> urls(const std::string& steam_id);
private:
    std::filesystem::path directory_;
    Fetch fetch_;
};
ImagePtr executable_icon(const std::filesystem::path& exe);
}
