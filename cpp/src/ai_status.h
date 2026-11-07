// © 2026 FuchsLabs — independent local status protocol; no third-party implementation.
#pragma once
#include <nlohmann/json.hpp>
#include <ctime>
#include <functional>
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
// Read-only usage collector. One worker; bounded HTTPS/isolated CLI, no model turns.
struct UsageResult { Json metrics=Json::object(); std::string source, reason; bool ok=false; };
UsageResult codex_usage(const Json& reply);
struct UsageHttpReply { int status=0; std::string body; };
using UsageTransport = std::function<UsageHttpReply(bool claude, std::string_view token, std::string_view account)>;
UsageResult endpoint_usage(bool claude, const Json& reply);
// Explicit path and transport allow offline tests; production calls only fixed HTTPS hosts.
UsageResult login_usage(bool claude, const std::filesystem::path& file, const UsageTransport& transport);
using UsageQuery = std::function<UsageResult(bool claude)>;
int usage_delay(bool success); // seconds, per provider
std::string usage_reason(const std::string& reason);
class UsageWorker {
public:
    UsageWorker(Model& model, std::filesystem::path directory, bool codex_account=false, bool claude_account=false, UsageQuery query={});
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
std::string adapter_script(int port);
class Service {
public:
    explicit Service(std::filesystem::path directory);
    ~Service();
    bool start(int port = PORT); // never accepts port 8765
    bool running() const;
    int port() const;
    Model& model();
    std::filesystem::path script_path() const;
    void prepare_adapter(); // DPAPI key + fixed script, never called by tests on real home paths
    void enable_usage(bool enabled, bool codex_account=false, bool claude_account=false);
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
} // namespace fl::ai
