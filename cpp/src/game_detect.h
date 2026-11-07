// © 2026 FuchsLabs – clean-room, read-only game discovery.
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
namespace fl::game {
struct Game {
    std::string id, name, steam_id;
    std::filesystem::path install, executable;
    uint32_t pid = 0;
    bool operator==(const Game& rhs) const { return id==rhs.id && pid==rhs.pid; }
};
std::optional<Game> parse_acf(std::string_view text, const std::filesystem::path& steamapps);
std::optional<Game> parse_epic(std::string_view text);
std::vector<std::filesystem::path> parse_libraries(std::string_view text);
bool beneath(const std::filesystem::path& child, const std::filesystem::path& root);
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
    Discovery(std::string additional={}, std::string exclusions={});
    std::optional<Game> poll(const std::optional<Game>& active, double now);
private:
    void refresh();
    std::vector<Game> library_;
    std::vector<std::string> additional_, excluded_;
    double refresh_at_=0;
};
}
