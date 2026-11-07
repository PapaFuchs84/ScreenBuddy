// © 2026 FuchsLabs — independent local status protocol; no third-party implementation.
#pragma once
#include <nlohmann/json.hpp>
#include <ctime>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace fl::ai {
using Json = nlohmann::json;
inline constexpr int PORT = 18765;
inline constexpr int RESERVED_PORT = 8765;
struct Event {
    std::string provider, session, kind, tool, model, source;
    std::map<std::string, double> metrics;
};
std::optional<Event> parse(std::string_view body);
struct Session {
    std::string provider, id, state = "off", tool, model;
    std::time_t since = 0, updated = 0;
    uint64_t revision = 0;
};
struct Snapshot {
    std::vector<Session> sessions; // active, priority waiting > working > idle
    std::map<std::string, Json> providers;
    std::map<std::string, Session> representatives; // includes last known off state
    std::time_t updated = 0;
};
// Public tile data and presence use the same immutable frame snapshot.
Json tile_values(const Snapshot& snapshot, std::time_t now);
bool tile_present(const Snapshot& snapshot, const std::string& block);
class Model {
public:
    bool ingest(std::string_view body, std::time_t now);
    Snapshot snapshot(std::time_t now) const;
    Json tiles(std::time_t now) const; // {claude:{...},codex:{...}}; unknown = null
    void usage(const std::string& provider, const Json& metrics, const std::string& source,
               const std::string& reason, std::time_t now);
private:
    mutable std::mutex mutex_;
    std::map<std::string, Session> sessions_;
    std::map<std::string, Json> metrics_;
    std::string day_;
    std::time_t updated_ = 0;
    uint64_t revision_ = 0;
};
// Read-only CLI collector. One worker, bounded child lifetime; never sends a model turn.
struct UsageResult { Json metrics=Json::object(); std::string source, reason; bool ok=false; };
UsageResult codex_usage(const Json& reply);
std::string usage_reason(const std::string& reason);
class UsageWorker {
public:
    UsageWorker(Model& model, std::filesystem::path directory);
    ~UsageWorker();
    void enable(bool enabled);
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
// Preview retains original bytes; apply rejects concurrent edits. Caller supplies explicit paths.
struct HookPlan {
    std::filesystem::path file;
    std::string before, after;
    bool existed = false;
};
HookPlan plan_hooks(const std::filesystem::path& file, const std::string& command, bool remove);
void apply_hooks(const HookPlan& plan);
std::string hook_preview(const HookPlan& plan, const std::string& command);
std::string hook_command(const std::filesystem::path& script, const std::string& provider);
// Transport independent of secret storage, allowing tests with synthetic ephemeral credentials.
class Receiver {
public:
    Receiver(Model& model, std::string key);
    ~Receiver();
    bool start(int port = PORT); // zero selects an available loopback port
    int port() const;
    bool running() const;
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
class Service {
public:
    explicit Service(std::filesystem::path directory);
    ~Service();
    bool start(int port = PORT); // never accepts port 8765
    bool running() const;
    Model& model();
    std::filesystem::path script_path() const;
    void prepare_adapter(); // DPAPI key + fixed script, never called by tests on real home paths
    void enable_usage(bool enabled);
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
} // namespace fl::ai
