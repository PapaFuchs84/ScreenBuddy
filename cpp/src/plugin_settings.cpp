// © 2026 FuchsLabs
#include "plugin_settings.h"
#include "secure.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <set>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fl::plugins {
namespace {
namespace fs = std::filesystem;
SettingsResult fail(const char* message) { return {false, message}; }
bool id_ok(std::string_view id) {
    if (id.empty() || id.size() > 64 || id.front() < 'a' || id.front() > 'z' || id.back() == '-' || reserved_device_name(id)) return false;
    char previous = 0;
    for (char c : id) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
        if (previous == '-' && c == '-') return false;
        previous = c;
    }
    return true;
}
// Per plugin and key: a stolen file entry cannot be replayed under another plugin/key, and other
// DPAPI users of this process (the main config) cannot decrypt it.
std::string entropy_for(const std::string& id, const std::string& key) {
    return "FuchsLabs.USB-Display.plugin.v1:" + id + ":" + key;
}
std::string protect_secret(const std::string& id, const std::string& key, const std::string& plaintext) {
    return secure::protect(plaintext, entropy_for(id, key));
}
// Reads the plugin entropy first; falls back to the pre-2.5.0-dev.3 shared entropy so existing
// settings stay readable. `legacy` tells the caller to re-protect.
std::string reveal_secret(const std::string& id, const std::string& key, const std::string& ciphertext, bool* legacy = nullptr) {
    auto plaintext = secure::unprotect(ciphertext, entropy_for(id, key));
    if (!plaintext.empty() || ciphertext.empty()) return plaintext;
    plaintext = secure::unprotect(ciphertext);
    if (!plaintext.empty() && legacy) *legacy = true;
    return plaintext;
}
bool key_ok(std::string_view key) {
    return !key.empty() && key.size() <= 40 && key.front() >= 'a' && key.front() <= 'z' &&
        std::all_of(key.begin(), key.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        });
}
bool value_ok(const std::string& type, const Json& value) {
    if (type == "number") return value.is_number() && std::isfinite(value.get<double>());
    if (type != "text" && type != "secret") return false;
    if (!value.is_string()) return false;
    const auto& text = value.get_ref<const std::string&>();
    if (text.size() > SettingsStore::value_limit) return false;
    // Match request-template restrictions; reject embedded controls and NUL.
    if (std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32 || c == 127; })) return false;
    try { (void)value.dump(); } catch (...) { return false; } // require valid UTF-8
    return true;
}
std::optional<Json> schema_for(const Manifest& manifest) {
    if (!id_ok(manifest.id) || manifest.settings.size() > 12) return std::nullopt;
    Json schema = Json::array();
    std::set<std::string> keys;
    for (const auto& setting : manifest.settings) {
        if (!key_ok(setting.key) || !keys.insert(setting.key).second || !value_ok(setting.type, setting.default_value) ||
            (setting.type == "secret" && setting.default_value != "")) return std::nullopt;
        schema.push_back({{"key", setting.key}, {"type", setting.type}, {"default", setting.default_value},
            {"required", setting.required}});
    }
    return schema;
}
bool plain_path(const fs::path& path) {
    for (auto current = path; !current.empty();) {
        std::error_code ec;
        const auto status = fs::symlink_status(current, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) return false;
        if (!ec && fs::is_symlink(status)) return false;
#ifdef _WIN32
        const auto attributes = GetFileAttributesW(current.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return true;
}
bool regular_or_missing(const fs::path& path) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    const auto status = fs::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory) return true;
    return !ec && (status.type() == fs::file_type::not_found || fs::is_regular_file(status));
}
bool ready_directory(const fs::path& path, bool create) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    if (create) fs::create_directories(path, ec);
    return !ec && fs::is_directory(path, ec) && !ec && plain_path(path);
}
bool read_bounded(const fs::path& path, std::string& out) {
    if (!regular_or_missing(path)) return false;
#ifdef _WIN32
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    bool ok = GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow > 0 && info.nFileSizeLow <= SettingsStore::file_limit;
    if (ok) {
        out.resize(info.nFileSizeLow);
        DWORD count = 0;
        ok = ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &count, nullptr) && count == out.size();
    }
    CloseHandle(handle);
    return ok;
#else
    const int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size > 0 &&
        static_cast<uint64_t>(info.st_size) <= SettingsStore::file_limit;
    if (ok) {
        out.resize(static_cast<size_t>(info.st_size));
        size_t total = 0;
        while (total < out.size()) {
            const auto count = read(fd, out.data() + total, out.size() - total);
            if (count <= 0) { ok = false; break; }
            total += static_cast<size_t>(count);
        }
    }
    close(fd);
    return ok;
#endif
}
bool atomic_write(const fs::path& target, const std::string& data) {
    if (data.size() > SettingsStore::file_limit || !regular_or_missing(target) ||
        !ready_directory(target.parent_path(), true)) return false;
    static std::atomic<uint64_t> sequence{0};
    auto temporary = target;
    temporary += ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
        "-" + std::to_string(++sequence);
#ifdef _WIN32
    HANDLE handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0;
    bool ok = WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &count, nullptr) && count == data.size();
    if (ok) ok = FlushFileBuffers(handle) != 0;
    if (!CloseHandle(handle)) ok = false;
    if (ok) ok = regular_or_missing(target) && MoveFileExW(temporary.c_str(), target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    const int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    bool ok = true;
    size_t total = 0;
    while (total < data.size()) {
        const auto count = write(fd, data.data() + total, data.size() - total);
        if (count <= 0) { ok = false; break; }
        total += static_cast<size_t>(count);
    }
    if (ok) ok = fsync(fd) == 0;
    if (close(fd) != 0) ok = false;
    if (ok) {
        std::error_code ec;
        if (!regular_or_missing(target)) ok = false;
        else { fs::rename(temporary, target, ec); ok = !ec; }
    }
#endif
    if (!ok) { std::error_code ignored; fs::remove(temporary, ignored); }
    return ok;
}
Json parse_records(const std::string& text) {
    // Bound recursion before parsing, then reject duplicate JSON keys.
    size_t depth = 0;
    bool quoted = false, escaped = false;
    for (char c : text) {
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') { if (++depth > 8) return Json(); }
        else if (c == '}' || c == ']') { if (depth == 0) return Json(); --depth; }
    }
    if (quoted || depth) return Json();
    bool duplicate = false;
    std::vector<std::set<std::string>> keys;
    auto callback = [&](int, Json::parse_event_t event, Json& parsed) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::object_end) keys.pop_back();
        else if (event == Json::parse_event_t::key && !keys.back().insert(parsed.get<std::string>()).second) duplicate = true;
        return true;
    };
    auto parsed = Json::parse(text, callback, false);
    return duplicate ? Json() : parsed;
}
bool valid_records(const std::string& id, const Json& records) {
    if (!records.is_object() || records.size() > 12) return false;
    for (auto it = records.begin(); it != records.end(); ++it) {
        const auto& record = it.value();
        if (!key_ok(it.key()) || !record.is_object() || record.size() != 2 || !record.contains("type") ||
            !record["type"].is_string() || !record.contains("value")) return false;
        const auto type = record["type"].get<std::string>();
        const auto& value = record["value"];
        if (type == "secret") {
            if (!value.is_string() || value.get_ref<const std::string&>().size() > 16 * 1024) return false;
            const auto ciphertext = value.get<std::string>();
            if (!ciphertext.empty() && !secure::is_protected(ciphertext)) return false;
            const auto plaintext = reveal_secret(id, it.key(), ciphertext);
            if ((!ciphertext.empty() && plaintext.empty()) || !value_ok(type, plaintext)) return false;
        } else if (!value_ok(type, value)) return false;
    }
    return true;
}
std::string serialize(const std::string& id, const Json& records) {
    return Json({{"schema_version", 1}, {"id", id}, {"settings", records}}).dump();
}
Json values_for(const Manifest& manifest, const Json& records, bool secrets) {
    Json values = Json::object();
    for (const auto& setting : manifest.settings) {
        if (setting.type == "secret" && !secrets) continue;
        Json value = setting.default_value;
        const auto it = records.find(setting.key);
        if (it != records.end() && (*it)["type"] == setting.type) {
            value = (*it)["value"];
            if (setting.type == "secret") value = secure::unprotect(value.get<std::string>(), entropy_for(manifest.id, setting.key));
        }
        values[setting.key] = std::move(value);
    }
    return values;
}
Json missing_for(const Manifest& manifest, const Json& values) {
    Json missing = Json::array();
    for (const auto& setting : manifest.settings) {
        if (!setting.required) continue;
        const auto it = values.find(setting.key);
        if (it == values.end() || !value_ok(setting.type, *it) || (it->is_string() && it->get_ref<const std::string&>().empty()))
            missing.push_back(setting.key);
    }
    return missing;
}
} // namespace

SettingsStore::SettingsStore(fs::path directory) : directory_(fs::absolute(std::move(directory)).lexically_normal()) {}
SettingsResult SettingsStore::load(const Manifest& manifest) {
    std::lock_guard lock(mutex_);
    const auto schema = schema_for(manifest);
    if (!schema) return fail("Invalid plugin settings schema.");
    entries_.erase(manifest.id); // a failed reload cannot leave stale/removed credentials usable
    if (entries_.size() >= plugin_limit) return fail("Plugin settings limit reached.");
    const auto path = directory_ / (manifest.id + ".settings.json");
    if (!regular_or_missing(path)) return fail("Plugin settings path is unavailable or unsafe.");
    std::error_code ec;
    const bool exists = fs::exists(path, ec);
    if (ec) return fail("Plugin settings cannot be read.");
    Json records = Json::object();
    if (exists) {
        std::string text;
        if (!read_bounded(path, text)) return fail("Plugin settings cannot be read.");
        try {
            const auto data = parse_records(text);
            if (!data.is_object() || data.size() != 3 || !data.contains("schema_version") ||
                !data["schema_version"].is_number_integer() || data["schema_version"] != 1 ||
                data.value("id", Json()) != manifest.id || !data.contains("settings") || !valid_records(manifest.id, data["settings"]))
                return fail("Plugin settings file is invalid or cannot be decrypted.");
            const auto& stored = data["settings"];
            for (const auto& setting : manifest.settings) {
                const auto it = stored.find(setting.key);
                if (it == stored.end() || (*it)["type"] != setting.type) continue;
                records[setting.key] = *it;
                if (setting.type == "secret" && !(*it)["value"].get_ref<const std::string&>().empty()) {
                    bool legacy = false;
                    const auto plaintext = reveal_secret(manifest.id, setting.key, (*it)["value"].get<std::string>(), &legacy);
                    if (legacy) records[setting.key]["value"] = protect_secret(manifest.id, setting.key, plaintext); // migrate
                }
            }
            // Persist removal/type changes now so removed credentials never return on re-add.
            if (records != stored && !atomic_write(path, serialize(manifest.id, records)))
                return fail("Plugin settings update could not be saved.");
        } catch (...) { return fail("Plugin settings file is invalid or cannot be decrypted."); }
    }
    entries_.emplace(manifest.id, Entry{std::move(records), *schema});
    return {true, {}};
}
SettingsResult SettingsStore::save(const Manifest& manifest, const Json& patch) {
    std::lock_guard lock(mutex_);
    const auto schema = schema_for(manifest);
    const auto entry = entries_.find(manifest.id);
    if (!schema || entry == entries_.end() || entry->second.schema != *schema)
        return fail("Plugin settings must be loaded for the current manifest.");
    if (!patch.is_object() || patch.size() > manifest.settings.size()) return fail("Invalid plugin settings values.");
    auto records = entry->second.records;
    try {
        for (auto it = patch.begin(); it != patch.end(); ++it) {
            const auto setting = std::find_if(manifest.settings.begin(), manifest.settings.end(),
                [&](const Setting& value) { return value.key == it.key(); });
            if (setting == manifest.settings.end()) return fail("Unknown plugin setting.");
            if (it->is_null()) { records.erase(it.key()); continue; }
            if (!value_ok(setting->type, *it)) return fail("Invalid plugin setting value type or size.");
            Json value = *it;
            if (setting->type == "secret") {
                const auto plaintext = value.get<std::string>();
                if (plaintext.empty()) { records.erase(it.key()); continue; }
                const auto ciphertext = protect_secret(manifest.id, it.key(), plaintext);
                if (!secure::is_protected(ciphertext) || ciphertext == plaintext)
                    return fail("Plugin secret encryption failed.");
                value = ciphertext;
            }
            records[it.key()] = {{"type", setting->type}, {"value", std::move(value)}};
        }
        if (!atomic_write(directory_ / (manifest.id + ".settings.json"), serialize(manifest.id, records)))
            return fail("Plugin settings could not be saved.");
    } catch (...) { return fail("Plugin settings could not be encrypted or saved."); }
    entry->second.records = std::move(records);
    return {true, {}};
}
Json SettingsStore::public_view(const Manifest& manifest) const {
    std::lock_guard lock(mutex_);
    Json result = {{"values", Json::object()}, {"secrets", Json::object()}, {"missing_required", Json::array()}};
    const auto schema = schema_for(manifest);
    if (!schema) return result;
    const auto entry = entries_.find(manifest.id);
    const auto records = entry != entries_.end() && entry->second.schema == *schema ? entry->second.records : Json::object();
    result["values"] = values_for(manifest, records, false);
    // Decrypt only to verify availability. Nothing secret enters the returned JSON.
    const auto values = values_for(manifest, records, true);
    for (const auto& setting : manifest.settings)
        if (setting.type == "secret") result["secrets"][setting.key] = !values[setting.key].get_ref<const std::string&>().empty();
    result["missing_required"] = missing_for(manifest, values);
    return result;
}
SettingsValuesResult SettingsStore::runtime_values(const Manifest& manifest) const {
    std::lock_guard lock(mutex_);
    const auto schema = schema_for(manifest);
    const auto entry = entries_.find(manifest.id);
    if (!schema || entry == entries_.end() || entry->second.schema != *schema)
        return {std::nullopt, "Plugin settings are unavailable for the current manifest."};
    try {
        auto values = values_for(manifest, entry->second.records, true);
        if (!missing_for(manifest, values).empty()) return {std::nullopt, "Required plugin settings are missing."};
        for (const auto& setting : manifest.settings) {
            if (!value_ok(setting.type, values[setting.key])) return {std::nullopt, "Plugin settings are invalid."};
            if (setting.type == "secret" && entry->second.records.contains(setting.key) &&
                values[setting.key] == "" && entry->second.records[setting.key]["value"] != "")
                return {std::nullopt, "Plugin secret cannot be decrypted."};
        }
        return {std::move(values), {}};
    } catch (...) { return {std::nullopt, "Plugin settings cannot be decrypted."}; }
}
SettingsResult SettingsStore::discard_secrets(const std::string& id) {
    std::lock_guard lock(mutex_);
    if (!id_ok(id)) return fail("Invalid plugin ID.");
    entries_.erase(id); // nothing stale stays usable, even if the rewrite below fails
    const auto path = directory_ / (id + ".settings.json");
    if (!regular_or_missing(path)) return fail("Plugin settings path is unavailable or unsafe.");
    std::error_code ec;
    if (!fs::exists(path, ec)) return ec ? fail("Plugin settings cannot be read.") : SettingsResult{true, {}};
    std::string text;
    Json kept = Json::object();
    if (read_bounded(path, text)) {
        const auto data = parse_records(text);
        if (data.is_object() && data.contains("settings") && data["settings"].is_object() && data.value("id", Json()) == id)
            for (auto it = data["settings"].begin(); it != data["settings"].end(); ++it)
                if (key_ok(it.key()) && it->is_object() && it->value("type", Json()) != "secret" && it->size() == 2 && it->contains("value") &&
                    it->value("type", Json()).is_string() && value_ok(it->value("type", Json()).get<std::string>(), (*it)["value"]))
                    kept[it.key()] = *it;
    } // an unreadable/invalid file is dropped completely: it is being discarded anyway
    if (kept.empty()) {
        fs::remove(path, ec);
        return ec ? fail("Plugin settings could not be removed.") : SettingsResult{true, {}};
    }
    return atomic_write(path, serialize(id, kept)) ? SettingsResult{true, {}} : fail("Plugin settings could not be saved.");
}
SettingsResult SettingsStore::remove(const std::string& id) {
    std::lock_guard lock(mutex_);
    if (!id_ok(id)) return fail("Invalid plugin ID.");
    const auto path = directory_ / (id + ".settings.json");
    if (!regular_or_missing(path)) return fail("Plugin settings path is unavailable or unsafe.");
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) return fail("Plugin settings could not be removed.");
    entries_.erase(id);
    return {true, {}};
}
} // namespace fl::plugins
