// © 2026 FuchsLabs – reine Zeitplanung, Uhr vom Aufrufer.
#pragma once
#include <map>
#include <limits>
#include <optional>
#include <string>
#include <vector>
namespace fl { struct Image; }
namespace fl::plugins::schedule {
inline constexpr double never = std::numeric_limits<double>::infinity();
enum class Mode { rotate, first_available };
enum class Shape { cut, fade, reveal };
enum class OutputKind { none, monitor, companion, serial };
struct EntryInput { std::string block; int seconds = 8; bool available = true; };
struct ZoneInput { std::string zone; Mode mode; bool animate; std::vector<EntryInput> entries; };
struct Budget { Shape shape = Shape::cut; int steps = 0; double step_seconds = 0, min_entry_seconds = 0; bool skip_late = false; };
struct ZoneState {
    std::optional<size_t> shown; double since = 0;
    struct Fade { size_t from, to; double start; int steps; double step_seconds; Shape shape; bool skip_late; };
    std::optional<Fade> fade;
    double last_now = -never; int last_step = 0;
};
void reset(std::map<std::string,ZoneState>& state, bool generation_changed, bool size_changed);
struct ZoneChoice { std::string zone; std::optional<std::string> block, from_block; Shape shape = Shape::cut; float mix = 1; };
struct Result { std::vector<ZoneChoice> zones; double next_deadline = never; bool animating = false; };
Result advance(const std::vector<ZoneInput>&, std::map<std::string,ZoneState>&, double now, const Budget&, const std::vector<std::string>& requests = {});
struct Wake { double min_gap, wait_until; };
Wake wake_at(double now, const std::vector<double>& deadlines, bool animating, bool meter);
bool should_render(bool dirty, bool due, bool live, int step, bool size_changed, bool minute_changed, double now, double deadline);
Budget budget_for(OutputKind kind, std::optional<double> rate, double last_seconds, double render_ms, int rows, int width, size_t clients);
size_t serial_bytes(int rows, int width);
// Pixelvergleich nutzt dieselben Bänder wie der serielle Ausgang (Implementierung dort).
size_t serial_bytes(const Image& prev, const Image& next);
}
