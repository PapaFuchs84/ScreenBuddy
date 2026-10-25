// © 2026 FuchsLabs – local, offline-only plugin installation store.
#pragma once
#include "plugins.h"
#include <filesystem>
#include <mutex>
#include <set>
#include <cstdint>

namespace fl::plugins {
struct Installed {
    Manifest manifest;
    bool enabled = true;
    std::set<std::string> consented_hosts;
    uint64_t generation = 0; // changes on every mutation; never persisted
};
struct StoreResult {
    bool ok = false;
    std::string error;
    explicit operator bool() const { return ok; }
};
// Directory must be a dedicated per-user directory. No secret/settings values are
// persisted here. A host-owned protected settings store can be added separately.
// Every operation is synchronous, bounded and offline; copy entries before drawing.
class Store {
public:
    explicit Store(std::filesystem::path directory);
    StoreResult load(); // skips invalid files; reports a generic partial-load error
    StoreResult import_manifest(std::string_view utf8); // update by validated ID; sources force disabled
    StoreResult set_enabled(const std::string& id, bool enabled); // sources require every host's consent
    StoreResult set_host_consent(const std::string& id, const std::string& host, bool allowed);
    bool source_authorized(const std::string& id, const std::string& source_id, uint64_t generation) const;
    StoreResult remove(const std::string& id);
    std::vector<Installed> entries() const;
    std::optional<Installed> find(const std::string& id) const;
    static constexpr size_t installation_limit = 32;
private:
    std::filesystem::path directory_;
    mutable std::mutex mutex_;
    std::map<std::string, Installed> installed_;
    bool loaded_ = false;
    uint64_t generation_ = 0;
};
} // namespace fl::plugins
