// © 2026 FuchsLabs – declarative plugins (manifest v1, v2 with zone layout). Pure data; no I/O or code execution.
#pragma once
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include "plugin_zones.h" // header-only Strukturen; plugins.cpp braucht plugin_zones.cpp nicht

namespace fl::plugins {
using Json = nlohmann::json;
inline constexpr size_t manifest_limit = 128 * 1024;
inline constexpr size_t response_limit = 512 * 1024;
inline constexpr size_t depth_limit = 32;
inline constexpr size_t display_text_limit = 2048; // UTF-8 bytes, never split a code point
inline constexpr size_t setting_value_limit = 4096; // UTF-8 bytes; shared by parser and protected store

struct Setting {
    std::string key, label, type;
    Json default_value;
    bool required = false;
};
struct Source {
    std::string id, url, host;
    std::map<std::string, std::string> headers;
    Json sample = Json::object();
    // All settings this host can receive (including non-secret settings).
    std::vector<std::string> setting_keys;
};
// Manifest v2 only: conditional display (`show_if`). One test per block, no expression language:
// presence of `path` alone, or presence plus one comparison against a literal. Semantics table:
// docs/PLUGIN-SYSTEM.md ("Bedingte Anzeige"). Secret settings are rejected by the parser.
inline constexpr size_t condition_path_limit = 128;   // characters of `path`
inline constexpr size_t condition_segment_limit = 8;  // dot-separated segments of `path`
inline constexpr size_t condition_text_limit = 256;   // characters of a text literal
struct Condition {
    std::string path; // "<source>[.<field>...]" or "settings.<non-secret key>"
    std::string op;   // empty = presence only; otherwise "==", "!=", "<", "<=", ">", ">="
    Json value;       // comparison literal: non-empty text, finite number or boolean; null for presence
};
// Value present = not missing, not null, not a non-finite number, not "" and not an empty array/object.
// false and 0 are present. Every comparison requires a present value of the literal's JSON type (no
// conversion); otherwise the condition is false, also for "!=". `value` nullptr = path not found.
bool condition_holds(const Condition& condition, const Json* value);
struct Tap { std::string action, url; };
struct Widget {
    std::string type, label, text, value, unit, items, item_template, format;
    double min = 0, max = 100;
    int max_items = 1;
    int seconds = 8;
    std::optional<Tap> tap;
    // Manifest v2 only (empty in v1): stable block ID and, with a layout, the zone the block sits in.
    std::string id, zone;
    std::optional<Condition> show_if; // v2 only; absent = always shown
};
struct Manifest {
    int schema_version = 1; // 1 or 2; v2 adds widgets[].id, optional layout and up to 12 widgets
    std::string id, name, description, version;
    // Store metadata (optional for local use, required by store_problems): SPDX license from the
    // whitelist, author display name, category slug and minimum ScreenBuddy version (X.Y.Z).
    std::string license, author, category, min_app_version;
    int refresh_seconds = 60;
    std::vector<Setting> settings;
    std::vector<Source> sources;
    std::vector<std::string> links;
    std::vector<Widget> widgets;
    std::optional<zones::Layout> layout; // v2 only; absent = automatic grid as in v1
    // Validated manifest only: never merge installed settings into this export.
    Json definition;
};
struct ParseResult {
    std::optional<Manifest> manifest;
    std::string error; // fixed diagnostics; never includes manifest values or secrets
    explicit operator bool() const { return manifest.has_value(); }
};
struct DataResult {
    std::optional<Json> data;
    std::string error;
    explicit operator bool() const { return data.has_value(); }
};
ParseResult parse_manifest(std::string_view utf8);
// Same strict JSON rules as manifests (size limit, nesting 32, no duplicate keys); nullopt on any violation.
std::optional<Json> parse_strict_json(std::string_view utf8, size_t limit);
// Store metadata whitelists (plugin store, phase 1).
inline constexpr const char* store_licenses[] = {"MIT", "CC0-1.0"};
inline constexpr const char* store_categories[] = {"system", "games", "smart-home", "info", "other"};
// True when `app_version` (X.Y.Z) satisfies the manifest's min_app_version; a manifest without
// the field, or an unparsable app version, counts as compatible. Pure, no I/O.
bool app_compatible(const Manifest& manifest, std::string_view app_version);
// Extra rules for plugin store submissions on top of parse_manifest. Empty = acceptable.
// Fixed diagnostics, never manifest values. Local imports do not call this.
std::vector<std::string> store_problems(const Manifest& manifest);
// Windows device names (con, prn, aux, nul, com1-9, lpt1-9; case-insensitive) are never valid plugin IDs.
bool reserved_device_name(std::string_view id);
// Canonical text over everything that decides where data/secrets may flow: per source
// ID, URL, host and headers. Different text means previously stored secrets must not be reused.
std::string source_fingerprint(const Manifest& manifest);
std::string link_fingerprint(const Manifest& manifest);
bool tap_url_valid(const std::string& url, bool placeholders = false);
bool link_allowed(const Manifest& manifest, const std::string& url);
DataResult parse_source_data(std::string_view utf8);

using SourceData = std::map<std::string, Json>;
SourceData sample_data(const Manifest& manifest);
struct RenderContext {
    SourceData sources; // Live: only parse_source_data results. Never fall back to sample.
    Json settings = Json::object(); // renderer removes every secret setting again
    std::string time, date; // local formatted clock supplied by caller
};
struct RenderedWidget {
    bool missing = false;
    std::string type, label, text, unit;
    std::string id, zone; // v2 block ID / zone ID; empty for v1
    size_t index = 0;     // position in manifest.widgets (v1 block key "#<index>")
    std::vector<std::string> items;
    std::string tap_action, tap_url;
    std::vector<std::string> item_urls;
    std::optional<double> bar_fraction; // absent for a missing/non-finite number; otherwise 0..1
};
struct Page {
    std::string id, title;
    std::vector<RenderedWidget> widgets; // shown blocks only (manifest order); hidden/failed ones are left out
    bool missing_data = false;
    std::optional<zones::Layout> layout; // copied from a v2 manifest; draw_page lays the blocks out by zone (phase 2b); absent = automatic grid
    // Block keys (v2: block ID; v1: "#<index>") left out of `widgets`:
    std::vector<std::string> hidden; // show_if was false (v2 only)
    std::vector<std::string> failed; // evaluating the block's data threw; draw_page logs these (rate-limited)
};
// Does not throw: every block is evaluated separately (a failing block is left out and listed in Page::failed,
// the rest of the page is unaffected). A failure outside the blocks (e.g. out of memory while copying ID, title
// or layout) yields a page without blocks and with missing_data set; ID and title are kept if they can be copied.
Page render_page(const Manifest& manifest, const RenderContext& context);

struct Request {
    std::string host, url;
    std::map<std::string, std::string> headers;
};
struct RequestResult {
    std::optional<Request> request;
    std::string error; // never contains URL or header values
    explicit operator bool() const { return request.has_value(); }
};
// Pure expansion, NOT authorization or SSRF protection. The transport must require
// current host consent, public DNS + connected-address checks, no redirects/proxy,
// 5 s timeout and response_limit, and recheck enabled/removed state per source.
// Returned URL/headers can contain secrets: do not log or expose them in UI/API.
RequestResult prepare_request(const Manifest& manifest, const Source& source, const Json& settings);
} // namespace fl::plugins
