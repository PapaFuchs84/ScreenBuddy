// © 2026 FuchsLabs – clean-room, read-only game discovery.
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
namespace fl::game {
struct Game {
    std::string id, name, steam_id;
    std::filesystem::path install, executable;
    uint32_t pid = 0;
    std::string process_name; // Toolhelp basename, also available for protected processes.
    std::vector<std::filesystem::path> cached_executables;
    bool operator==(const Game& rhs) const { return id==rhs.id && pid==rhs.pid; }
};
std::optional<Game> parse_acf(std::string_view text, const std::filesystem::path& steamapps);
std::optional<Game> parse_epic(std::string_view text);
std::vector<std::filesystem::path> parse_libraries(std::string_view text);
bool beneath(const std::filesystem::path& child, const std::filesystem::path& root);
// Read-only HKLM, explicit native/WOW6432Node paths; missing/inaccessible = empty.
class Registry {
public:
    virtual ~Registry() = default;
    virtual std::vector<std::wstring> subkeys(const std::wstring& key) const = 0;
    virtual std::wstring value(const std::wstring& key, const wchar_t* name) const = 0;
};
using DirectoryExists = std::function<bool(const std::filesystem::path&)>;
std::vector<Game> registry_games(const Registry& registry, const DirectoryExists& exists);
bool default_excluded(const std::filesystem::path& executable);
bool matches_game(const Game& game, const std::filesystem::path& executable,
                  bool foreground_fullscreen, const std::string& running_steam_app);
// Cache at refresh only: at most 2,000 entries, three nested directories, no reparse points.
void cache_executables(Game& game);
struct Process { uint32_t pid; std::filesystem::path path; std::string name; };
struct ProcessWindow { uint32_t foreground_pid=0; bool fullscreen=false; };
// Pure selection used by poll and synthetic tests. A title alone never identifies a game.
std::optional<Game> select_game(const std::vector<Game>& library, const std::vector<Process>& processes,
    ProcessWindow window, const std::string& running_steam_app, const std::optional<Game>& active={},
    const std::vector<std::string>& exclusions={});
enum class State { idle, starting, running, ending };
class Detector {
public:
    bool update(std::optional<Game> candidate, double now);
    State state() const { return state_; }
    std::optional<Game> active() const { return state_==State::running || state_==State::ending ? game_ : std::nullopt; }
    double started() const { return started_; }
private:
    State state_=State::idle;
    std::optional<Game> game_;
    double since_=0, started_=0, last_=0;
};
// Paths are exact executables, one per line. Exclusions may also be basenames.
class Discovery {
public:
    Discovery(std::string additional={}, std::string exclusions={}, const Registry* registry=nullptr);
    std::optional<Game> poll(const std::optional<Game>& active, double now);
private:
    void refresh();
    std::vector<Game> library_;
    std::vector<std::string> additional_, excluded_;
    double refresh_at_=0;
    const Registry* registry_; // Optional borrowed registry; must outlive Discovery.
};
}
