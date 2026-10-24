// © 2026 FuchsLabs
#include "plugin_store.h"
#include <atomic>
#include <chrono>
#include <fstream>
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
constexpr const char* suffix = ".flplugin.json";
bool plain_path(const fs::path& path) {
    // Reject symbolic links, junctions and other reparse points, including parents.
    for (auto current = path; !current.empty();) {
        std::error_code ec;
        auto status = fs::symlink_status(current, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) return false;
        if (!ec && fs::is_symlink(status)) return false;
#ifdef _WIN32
        const auto attr = GetFileAttributesW(current.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
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
    auto status = fs::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory) return true;
    return !ec && (status.type() == fs::file_type::not_found || fs::is_regular_file(status));
}
bool ready_directory(const fs::path& path, bool create) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    if (create) fs::create_directories(path, ec);
    if (ec) return false;
    return fs::is_directory(path, ec) && !ec && plain_path(path);
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
        info.nFileSizeHigh == 0 && info.nFileSizeLow <= manifest_limit;
    if (ok) {
        out.resize(info.nFileSizeLow);
        DWORD bytes = 0;
        ok = ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &bytes, nullptr) && bytes == out.size();
    }
    CloseHandle(handle);
    return ok;
#else
    int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size >= 0 &&
        static_cast<uint64_t>(info.st_size) <= manifest_limit;
    if (ok) {
        out.resize(static_cast<size_t>(info.st_size));
        size_t total = 0;
        while (total < out.size()) {
            auto n = read(fd, out.data() + total, out.size() - total);
            if (n <= 0) { ok = false; break; }
            total += static_cast<size_t>(n);
        }
    }
    close(fd);
    return ok;
#endif
}
bool atomic_write(const fs::path& target, std::string_view data) {
    if (!regular_or_missing(target) || !ready_directory(target.parent_path(), false)) return false;
    static std::atomic<uint64_t> sequence{0};
    // Exclusive creation prevents following existing links or overwriting a temp file.
    const auto token = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path temporary = target;
    temporary += ".tmp-" + std::to_string(token) + "-" + std::to_string(++sequence);
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
    int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    bool ok = true;
    size_t total = 0;
    while (total < data.size()) {
        auto n = write(fd, data.data() + total, data.size() - total);
        if (n <= 0) { ok = false; break; }
        total += static_cast<size_t>(n);
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
bool marker_state(const fs::path& marker, bool& disabled) {
    if (!regular_or_missing(marker)) return false;
    std::error_code ec;
    disabled = fs::exists(marker, ec);
    if (ec) return false;
    return !disabled || (fs::file_size(marker, ec) == 0 && !ec);
}
StoreResult failure(const char* error) { return {false, error}; }
} // namespace

Store::Store(std::filesystem::path directory) : directory_(fs::absolute(std::move(directory)).lexically_normal()) {}
StoreResult Store::load() {
    std::lock_guard lock(mutex_);
    installed_.clear(); loaded_ = false;
    if (!plain_path(directory_)) return failure("Plugin directory contains an unsafe link.");
    std::error_code ec;
    if (!fs::exists(directory_, ec) && !ec) { loaded_ = true; return {true, {}}; }
    if (ec || !ready_directory(directory_, false)) return failure("Plugin directory is unavailable.");
    bool invalid = false;
    size_t scanned = 0;
    fs::directory_iterator end, it(directory_, ec);
    if (ec) return failure("Plugin directory cannot be read.");
    while (it != end) {
        if (++scanned > 256) { invalid = true; break; }
        const auto path = it->path();
        const auto name = path.filename();
        if (name.extension() == ".json" && name.stem().extension() == ".flplugin") {
            std::string text;
            if (!read_bounded(path, text)) invalid = true;
            else {
                auto parsed = parse_manifest(text);
                bool disabled = false;
                if (!parsed || name != fs::path(parsed.manifest->id + suffix) || installed_.size() >= installation_limit ||
                    !marker_state(directory_ / (parsed.manifest->id + ".disabled"), disabled)) invalid = true;
                else {
                    auto id = parsed.manifest->id;
                    // Offline build: even hand-edited files without a marker may
                    // never activate sources before a safe transport exists.
                    const bool enabled = !disabled && parsed.manifest->sources.empty();
                    if (!installed_.emplace(id, Installed{std::move(*parsed.manifest), enabled}).second) invalid = true;
                }
            }
        }
        it.increment(ec);
        if (ec) { invalid = true; break; }
    }
    loaded_ = true;
    return invalid ? failure("Some plugin files were invalid or exceeded the store limit; valid plugins were loaded.") : StoreResult{true, {}};
}
StoreResult Store::import_manifest(std::string_view utf8) {
    auto parsed = parse_manifest(utf8);
    if (!parsed) return {false, parsed.error};
    std::lock_guard lock(mutex_);
    if (!loaded_) return failure("Load the plugin store before importing.");
    auto& manifest = *parsed.manifest;
    auto existing = installed_.find(manifest.id);
    if (existing == installed_.end() && installed_.size() >= installation_limit) return failure("At most 32 plugins may be installed.");
    if (!ready_directory(directory_, true)) return failure("Plugin directory cannot be created safely.");
    bool disabled = false;
    if (!marker_state(directory_ / (manifest.id + ".disabled"), disabled)) return failure("Plugin state file is invalid.");
    if (!manifest.sources.empty() && !disabled) {
        if (!atomic_write(directory_ / (manifest.id + ".disabled"), "")) return failure("Source plugin could not be disabled safely.");
        disabled = true;
        // The marker is already committed. Keep in-memory state consistent even
        // if replacing the manifest subsequently fails.
        if (existing != installed_.end()) existing->second.enabled = false;
    }
    // Persist the original validated bytes. Canonical reserialization can expand
    // exponent notation beyond the input limit and must not produce unloadable files.
    if (!atomic_write(directory_ / (manifest.id + suffix), utf8)) return failure("Plugin could not be saved atomically.");
    auto id = manifest.id;
    installed_.insert_or_assign(id, Installed{std::move(manifest), !disabled});
    return {true, {}};
}
StoreResult Store::set_enabled(const std::string& id, bool enabled) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    if (enabled && !it->second.manifest.sources.empty()) return failure("Live plugin sources are unavailable in this offline build.");
    if (!ready_directory(directory_, false)) return failure("Plugin directory is unavailable.");
    auto marker = directory_ / (it->first + ".disabled");
    if (!regular_or_missing(marker)) return failure("Plugin state file is invalid.");
    if (enabled) {
        std::error_code ec; fs::remove(marker, ec);
        if (ec) return failure("Plugin could not be enabled.");
    } else if (!atomic_write(marker, "")) return failure("Plugin could not be disabled.");
    it->second.enabled = enabled;
    return {true, {}};
}
StoreResult Store::remove(const std::string& id) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    const auto path = directory_ / (it->first + suffix), marker = directory_ / (it->first + ".disabled");
    if (!ready_directory(directory_, false) || !regular_or_missing(path) || !regular_or_missing(marker))
        return failure("Plugin files contain an unsafe link.");
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) return failure("Plugin manifest could not be removed.");
    // Removing the manifest is the committed uninstall; a failed marker cleanup
    // must not keep this plugin alive in memory.
    installed_.erase(it);
    fs::remove(marker, ec);
    if (ec) return failure("Plugin was removed but its disabled marker could not be cleaned up.");
    return {true, {}};
}
std::vector<Installed> Store::entries() const {
    std::lock_guard lock(mutex_);
    std::vector<Installed> out;
    for (const auto& [id, entry] : installed_) out.push_back(entry);
    return out;
}
std::optional<Installed> Store::find(const std::string& id) const {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    return it == installed_.end() ? std::nullopt : std::optional<Installed>(it->second);
}
} // namespace fl::plugins
