// © 2026 FuchsLabs – host-owned, bounded JSON source transport.
#pragma once
#include "plugin_store.h"
#include <chrono>
#include <functional>

namespace fl::plugins {
using Authorization = std::function<bool()>;
// HTTPS only, pinned public addresses, verified TLS, no proxy/redirect/cookies.
// Diagnostics are fixed strings; URLs, headers and response bodies are never logged.
DataResult fetch_public_json(const Request& request, const Authorization& authorized);
bool public_source_ip(const std::string& address);
// Strict bounded HTTP/1.1 decoding, also used by deterministic offline tests.
DataResult decode_source_http(std::string_view response);

class Transport {
public:
    using Clock = std::chrono::steady_clock;
    // Fetch is synchronous; implementations must not retain the authorization callback.
    using Fetch = std::function<DataResult(const Request&, const Authorization&)>;
    using Now = std::function<Clock::time_point()>;
    explicit Transport(Fetch fetch = fetch_public_json, Now now = [] { return Clock::now(); });
    // One worker owns a Transport; never call on the render/UI thread. Rechecks
    // current enabled/consent/manifest generation before every source and send.
    SourceData collect(Store& store, const std::string& id, const Json& settings = Json::object(),
        const Authorization& extra_authorized = [] { return true; });
    void clear();
private:
    struct Cache {
        uint64_t generation = 0;
        std::string settings_fingerprint;
        std::optional<Json> data;
        Clock::time_point next{};
        unsigned failures = 0;
    };
    Fetch fetch_;
    Now now_;
    std::map<std::pair<std::string, std::string>, Cache> cache_;
};
} // namespace fl::plugins
