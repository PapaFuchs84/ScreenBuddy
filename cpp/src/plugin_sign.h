// © 2026 FuchsLabs – plugin store signing: Ed25519 signatures, SHA-256 package hashes, signed index and revocation list.
// Pure data + OpenSSL: no network, no file access. The app only verifies; signing helpers exist for the tools and tests.
#pragma once
#include "plugins.h"
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fl::plugins::sign {
inline constexpr size_t index_limit = 1024 * 1024; // index.json and revoked.json, bytes
inline constexpr size_t max_entries = 2000;

// Public key pinned in the app (hex). EMPTY until the maintainer generates the production key offline
// (fl-plugin-sign keygen) and enters it here; while empty the app must treat the store as unavailable.
inline constexpr const char* store_public_key_hex = "";

std::string sha256_hex(std::string_view data);

// Signed documents carry a domain prefix so a signature for one kind never validates as another.
enum class Kind { index, revoked };
struct PublicKey { std::array<unsigned char, 32> raw{}; };
std::optional<PublicKey> public_key_from_hex(std::string_view hex64);

// Detached signature, standard Base64 (88 characters) over the exact document bytes.
bool verify(Kind kind, const PublicKey& key, std::string_view document, std::string_view signature_base64);

struct IndexEntry {
    std::string id, name, description, version, author, license, category, min_app_version;
    std::string file;   // always plugins/<id>/<id>.flplugin.json
    std::string sha256; // of the manifest bytes
    size_t size = 0;
    std::vector<std::string> hosts; // sorted, unique; shown before installation
};
struct Index {
    int sequence = 0;          // grows with every published index; the app refuses to go back
    std::string generated;     // ISO-8601 UTC, informational
    std::vector<IndexEntry> plugins;
};
struct IndexResult {
    std::optional<Index> index;
    std::string error; // fixed diagnostics, never document values
    explicit operator bool() const { return index.has_value(); }
};
// Verifies the signature FIRST, then parses strictly. Unsigned or altered data is never parsed further.
IndexResult open_index(const PublicKey& key, std::string_view document, std::string_view signature_base64);
// Entry must match the downloaded manifest: size, SHA-256, valid manifest, same id/version/metadata.
bool package_matches(const IndexEntry& entry, std::string_view manifest_bytes);
// Rollback protection: an offered sequence older than the last accepted one is refused.
inline bool sequence_acceptable(int last_accepted, int offered) { return offered >= last_accepted; }

struct Revocation { std::string id; std::vector<std::string> versions; bool all = false; std::string reason; };
struct Revocations { int sequence = 0; std::vector<Revocation> items; };
struct RevokedResult {
    std::optional<Revocations> list;
    std::string error;
    explicit operator bool() const { return list.has_value(); }
};
RevokedResult open_revoked(const PublicKey& key, std::string_view document, std::string_view signature_base64);
bool is_revoked(const Revocations& list, std::string_view id, std::string_view version);

// ---- Signing side (tools/tests only; the app never holds a private key) ----
struct KeyPair { std::string private_pem, public_hex; };
std::optional<KeyPair> generate_keypair();
std::optional<std::string> sign_document(Kind kind, const std::string& private_pem, std::string_view document);
// Builds the index for already validated store plugins (manifest bytes). Fails if any plugin
// does not pass parse_manifest + store_problems or if an ID repeats.
struct BuildInput { std::string manifest_bytes; };
struct BuildResult { std::optional<std::string> index_json; std::string error; };
BuildResult build_index(const std::vector<BuildInput>& manifests, int sequence, const std::string& generated);
} // namespace fl::plugins::sign
