// © 2026 FuchsLabs – bounded, per-user plugin configuration; no network or UI.
#pragma once
#include "plugins.h"
#include <filesystem>
#include <mutex>

namespace fl::plugins {
struct SettingsResult {
    bool ok = false;
    std::string error; // fixed diagnostics, never values or exception text
    explicit operator bool() const { return ok; }
};
struct SettingsValuesResult {
    std::optional<Json> values; // PRIVATE: may contain secrets, only pass to transport
    std::string error;
    explicit operator bool() const { return values.has_value(); }
};
class SettingsStore {
public:
    // Dedicated per-user directory, separate from installed manifests/exports.
    explicit SettingsStore(std::filesystem::path directory);
    // Call for every installed manifest at startup and after an import/update.
    // Retains matching key/type pairs; discards removed keys/type changes, applies
    // current defaults. Never imports a plaintext secret from a stored file.
    SettingsResult load(const Manifest& manifest);
    // Patch: absent keys preserve values; null resets to default, secret "" clears.
    // Incomplete configuration can be saved; runtime_values refuses required blanks.
    SettingsResult save(const Manifest& manifest, const Json& patch);
    // {values:{non-secret keys}, secrets:{key:bool}, missing_required:[keys]}.
    // Never returns secret values, ciphertext, request URLs or headers.
    Json public_view(const Manifest& manifest) const;
    SettingsValuesResult runtime_values(const Manifest& manifest) const;
    // Manifest update with changed sources/hosts/headers: drops every stored secret (non-secret
    // values stay), unloads the entry. Call before load() of the new manifest.
    SettingsResult discard_secrets(const std::string& id);
    // Call on uninstall BEFORE deleting the manifest; failure must stop uninstall.
    SettingsResult remove(const std::string& id);
    static constexpr size_t file_limit = 128 * 1024;
    static constexpr size_t value_limit = setting_value_limit; // UTF-8 bytes per text/secret
    static constexpr size_t plugin_limit = 32;
private:
    struct Entry { Json records; Json schema; };
    std::filesystem::path directory_;
    mutable std::mutex mutex_;
    std::map<std::string, Entry> entries_; // secrets stay DPAPI ciphertext here
};
} // namespace fl::plugins
