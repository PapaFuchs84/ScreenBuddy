// © 2026 FuchsLabs – plugin store client logic (phase 3): verified index cache, listing, safe install, revocation.
// Platform-neutral. All network access goes through the injected Fetcher (HTTPS bytes), so this is testable offline.
// The app never executes plugin code; installation = validated manifest into the local Store, disabled until consent.
#pragma once
#include "plugin_sign.h"
#include "plugin_store.h"
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace fl::plugins::catalog {
// Planned location of the signed store (GitHub Pages of the ScreenBuddy-Plugins repository). Not live yet:
// until the repository and the production key exist the store reports "unavailable".
inline constexpr const char* base_url = "https://papafuchs84.github.io/ScreenBuddy-Plugins/";

// Returns the raw response body for `relative_path` below base_url, or nullopt on any failure.
// Must enforce HTTPS, public addresses, no redirects/proxy and the byte limit. Never called on the UI thread.
using Fetcher = std::function<std::optional<std::string>(const std::string& relative_path, size_t byte_limit)>;

enum class Availability { available, offline, invalid, unavailable };
struct RefreshResult {
    Availability state = Availability::unavailable;
    bool from_cache = false;   // offline, but a previously verified index is shown
    std::string error;         // fixed diagnostics
};

enum class Status { not_installed, installed, update_available, unsupported, revoked };
struct Card {
    sign::IndexEntry entry;
    Status status = Status::not_installed;
    std::string installed_version;
    std::vector<std::string> new_hosts; // hosts an update would add (needs fresh consent)
};
struct Filter {
    std::string text;      // case-insensitive match on id, name, description, author
    std::string category;  // empty = all
    bool show_revoked = false;
};

class Catalog {
public:
    // `cache_dir`: dedicated directory for verified copies + the last accepted sequence numbers.
    Catalog(sign::PublicKey key, std::filesystem::path cache_dir, Fetcher fetch, std::string app_version);
    // Public key empty/invalid => Availability::unavailable without any network access.
    static std::optional<Catalog> from_pinned_key(std::filesystem::path cache_dir, Fetcher fetch, std::string app_version);

    // Fetch + verify index and revocation list. Signature failure or a lower sequence NEVER replaces
    // accepted data. Network failure falls back to the verified cache (re-verified on load).
    RefreshResult refresh();
    // Cards for the UI (sorted by name). `installed` comes from Store::entries().
    std::vector<Card> list(const Filter& filter, const std::vector<Installed>& installed) const;
    // Download, verify (size, SHA-256, manifest vs signed entry, not revoked, compatible) and import.
    // The plugin stays disabled / without host consent; the UI shows entry.hosts before calling this.
    // `importer` lets the host run extra steps (e.g. discard stored secrets); default = Store::import_manifest.
    using Importer = std::function<StoreResult(std::string_view)>;
    StoreResult install(Store& store, const std::string& id, const Importer& importer = {});
    // Disables installed plugins that are revoked; returns their IDs. Call after every refresh.
    std::vector<std::string> enforce_revocations(Store& store) const;

    const std::optional<sign::Index>& index() const { return index_; }
    const std::optional<sign::Revocations>& revocations() const { return revoked_; }

private:
    void load_cache();
    bool save_cache(const std::string& index, const std::string& index_sig, const std::string& revoked, const std::string& revoked_sig);
    sign::PublicKey key_;
    std::filesystem::path dir_;
    Fetcher fetch_;
    std::string app_version_;
    std::optional<sign::Index> index_;
    std::optional<sign::Revocations> revoked_;
};
} // namespace fl::plugins::catalog
