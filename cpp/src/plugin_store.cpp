// © 2026 FuchsLabs
#include "plugin_store.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <system_error>
#include <openssl/sha.h>
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
bool read_bounded(const fs::path& path, std::string& out, size_t limit = manifest_limit) {
    if (!regular_or_missing(path)) return false;
#ifdef _WIN32
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    bool ok = GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow <= limit;
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
        static_cast<uint64_t>(info.st_size) <= limit;
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
std::string fingerprint(const Manifest& manifest) {
    const auto bytes = manifest.definition.dump();
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), digest);
    std::string out;
    for (auto b : digest) { out += "0123456789abcdef"[b >> 4]; out += "0123456789abcdef"[b & 15]; }
    return out;
}
// Ungültige/veraltete Anordnungen sperren nie die gültige Basis.
bool read_arrangement(const fs::path& path, Installed& entry) {
    if (!regular_or_missing(path)) { entry.arrangement_state = "invalid"; return false; }
    std::error_code ec;
    if (!fs::exists(path, ec) && !ec) return true;
    std::string text;
    if (ec || !read_bounded(path, text, arrangement_limit)) { entry.arrangement_state = "invalid"; return false; }
    auto parsed = parse_arrangement(text);
    if (!parsed) { entry.arrangement_state = "invalid"; return false; }
    entry.arrangement = std::move(parsed.arrangement);
    if (entry.arrangement->manifest_sha256 != fingerprint(entry.manifest)) {
        entry.arrangement_state = "stale";
        return true;
    }
    auto merged = merge_arrangement(entry.manifest, *entry.arrangement);
    if (!merged) { entry.arrangement_state = "invalid"; return false; }
    entry.arranged = std::move(merged.manifest);
    entry.arrangement_state = "ok";
    return true;
}

std::string write_devices(const fs::path& path,const device::OverlayMap& next,bool reset=false) {
    const auto backup=fs::path(path.wstring()+L".bak");
    if(!regular_or_missing(path) || !regular_or_missing(backup) || !ready_directory(path.parent_path(),false)) return "Unsafe device arrangement path.";
    std::error_code ec;
    if(reset) {fs::remove(backup,ec);if(ec) return "Device backup removal failed.";}
    else if(fs::exists(path,ec)) {std::string old;if(!read_bounded(path,old,device::overlay_bytes_limit) || !atomic_write(backup,old)) return "Device backup failed.";}
    if(ec || !atomic_write(path,device::overlays_json(next).dump())) return "Device arrangement write failed.";
    return {};
}
bool read_devices(const fs::path& path,Installed& entry) {
    if(!regular_or_missing(path)) return false;
    std::error_code ec;if(!fs::exists(path,ec)) return !ec;
    std::string text;device::OverlayMap map;
    if(!read_bounded(path,text,device::overlay_bytes_limit) ||
       !device::parse_overlays(text,map,[&](const auto& bytes){return bool(parse_arrangement(bytes));})) return false;
    for(auto it=map.begin();it!=map.end();) {
        auto parsed=parse_arrangement(it->second.dump());
        auto merged=merge_arrangement(entry.manifest,*parsed.arrangement);
        if(parsed.arrangement->manifest_sha256!=fingerprint(entry.manifest) || !merged) {it=map.erase(it);continue;}
        entry.device_arranged.emplace(it->first,std::move(*merged.manifest));++it;
    }
    entry.device_overlays=std::move(map);return true;
}
bool all_hosts_allowed(const Installed& entry) {
    for (const auto& source : entry.manifest.sources)
        if (!entry.consented_hosts.contains(source.host)) return false;
    return true;
}
bool read_consent(const fs::path& path, Installed& entry) {
    if (!regular_or_missing(path)) return false;
    std::error_code ec;
    if (!fs::exists(path, ec)) return !ec;
    std::string text;
    if (!read_bounded(path, text)) return false;
    const auto parsed = parse_source_data(text);
    if (!parsed) return false;
    const auto& data = *parsed.data;
    if (!data.is_object() || data.size() != 3 || data.value("schema_version", Json()) != 1 ||
        !data.contains("manifest_sha256") || !data["manifest_sha256"].is_string() ||
        !data.contains("hosts") || !data["hosts"].is_array() || data["hosts"].size() > 8) return false;
    if (data["manifest_sha256"] != fingerprint(entry.manifest)) return true; // stale: no grant
    std::set<std::string> hosts;
    for (const auto& value : data["hosts"]) {
        if (!value.is_string()) return false;
        const auto host = value.get<std::string>();
        bool declared = false;
        for (const auto& source : entry.manifest.sources) declared |= source.host == host;
        if (!declared || !hosts.insert(host).second) return false;
    }
    entry.consented_hosts = std::move(hosts);
    return true;
}
bool write_consent(const fs::path& path, const Installed& entry) {
    return atomic_write(path, Json({{"schema_version", 1}, {"manifest_sha256", fingerprint(entry.manifest)},
        {"hosts", entry.consented_hosts}}).dump());
}
bool write_links(const fs::path& path, const Installed& entry) {
    return atomic_write(path, Json({{"fingerprint",link_fingerprint(entry.manifest)}, {"allowed",entry.links_allowed}}).dump());
}
bool read_links(const fs::path& path, Installed& entry) {
    if (!regular_or_missing(path)) return false;
    std::error_code ec; if (!fs::exists(path,ec)) return !ec;
    std::string text; if(!read_bounded(path,text)) return false;
    auto j=parse_strict_json(text,manifest_limit);
    if(!j || !j->is_object() || j->size()!=2 || !j->contains("fingerprint") || !(*j)["fingerprint"].is_string() ||
       !j->contains("allowed") || !(*j)["allowed"].is_boolean()) return false;
    entry.links_allowed=(*j)["fingerprint"]==link_fingerprint(entry.manifest) && (*j)["allowed"].get<bool>();
    return true;
}
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
                    // Missing/stale host grants always leave source plugins disabled.
                    Installed entry{std::move(*parsed.manifest), false, {}, ++generation_};
                    if (!read_consent(directory_ / (id + ".consent.json"), entry) || !read_links(directory_ / (id + ".links.json"), entry)) invalid = true;
                    else {
                        if (!read_arrangement(directory_ / (id + ".layout.json"), entry)) invalid = true;
                        if (!read_devices(directory_ / (id + ".devices.json"), entry)) invalid = true;
                        entry.layout_revision = ++layout_revision_;
                        entry.enabled = !disabled && all_hosts_allowed(entry);
                        if (!installed_.emplace(id, std::move(entry)).second) invalid = true;
                    }
                }
            }
        }
        it.increment(ec);
        if (ec) { invalid = true; break; }
    }
    loaded_ = true;
    return invalid ? failure("Some plugin files were invalid or exceeded the store limit; valid plugins were loaded.") : StoreResult{true, {}};
}
StoreResult Store::import_manifest(std::string_view utf8, bool force_disabled) {
    auto parsed = parse_manifest(utf8);
    if (!parsed) return {false, parsed.error};
    std::lock_guard lock(mutex_);
    if (!loaded_) return failure("Load the plugin store before importing.");
    auto& manifest = *parsed.manifest;
    auto existing = installed_.find(manifest.id);
    if (existing == installed_.end() && installed_.size() >= installation_limit) return failure("At most 32 plugins may be installed.");
    // Only a changed link list with an otherwise identical manifest retains source grants and activation.
    // Identical reimports and all other updates revoke every grant.
    std::set<std::string> retained_hosts;
    bool retain_enabled=false;
    if(existing!=installed_.end()) {
        auto before=existing->second.manifest.definition, after=manifest.definition;
        before.erase("links"); after.erase("links");
        if(before==after && link_fingerprint(existing->second.manifest)!=link_fingerprint(manifest)) {
            retained_hosts=existing->second.consented_hosts;
            retain_enabled=existing->second.enabled;
        }
    }
    // An update invalidates in-flight work and grants even when persistence fails.
    if (existing != installed_.end()) {
        existing->second.enabled = false;
        existing->second.consented_hosts.clear();
        existing->second.links_allowed = false;
        existing->second.generation = ++generation_;
    }
    if (!ready_directory(directory_, true)) return failure("Plugin directory cannot be created safely.");
    bool disabled = false;
    if (!marker_state(directory_ / (manifest.id + ".disabled"), disabled)) return failure("Plugin state file is invalid.");
    if ((force_disabled || (!manifest.sources.empty() && !retain_enabled)) && !disabled) {
        if (!atomic_write(directory_ / (manifest.id + ".disabled"), "")) return failure("Plugin could not be disabled safely.");
        disabled = true;
        // The marker is already committed. Keep in-memory state consistent even
        // if replacing the manifest subsequently fails.
        if (existing != installed_.end()) { existing->second.enabled = false; existing->second.generation = ++generation_; }
    }
    auto id = manifest.id;
    // Persist revoked grants before replacement, except source grants for link-list-only updates.
    // A failed invalidation must never leave a reusable old grant.
    Installed replacement{manifest, !disabled, retained_hosts, ++generation_};
    replacement.links_allowed=false;
    if(!write_links(directory_ / (id + ".links.json"),replacement)) return failure("Plugin link consent could not be saved.");
    if (!write_consent(directory_ / (id + ".consent.json"), replacement)) {
        if (existing != installed_.end()) { existing->second.enabled = false; existing->second.generation = ++generation_; }
        return failure("Plugin consent could not be reset safely.");
    }
    // Persist original validated bytes only after revoking any old grant. Even an
    // exact reimport interrupted between writes must require fresh consent.
    if (!atomic_write(directory_ / (manifest.id + suffix), utf8)) return failure("Plugin could not be saved atomically.");
    read_arrangement(directory_ / (id + ".layout.json"), replacement);
    read_devices(directory_ / (id + ".devices.json"), replacement);
    replacement.layout_revision = ++layout_revision_;
    installed_.insert_or_assign(id, std::move(replacement));
    return {true, {}};
}
StoreResult Store::set_enabled(const std::string& id, bool enabled) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    if (enabled && !all_hosts_allowed(it->second)) return failure("Approve every declared host before enabling this plugin.");
    if (!enabled) {
        // Stop in-flight requests immediately, including a failed disk update.
        it->second.enabled = false;
        it->second.generation = ++generation_;
    }
    if (!ready_directory(directory_, false)) return failure("Plugin directory is unavailable.");
    auto marker = directory_ / (it->first + ".disabled");
    if (!regular_or_missing(marker)) return failure("Plugin state file is invalid.");
    if (enabled) {
        std::error_code ec; fs::remove(marker, ec);
        if (ec) return failure("Plugin could not be enabled.");
    } else if (!atomic_write(marker, "")) return failure("Plugin could not be disabled.");
    it->second.enabled = enabled;
    it->second.generation = ++generation_;
    return {true, {}};
}
StoreResult Store::set_arrangement(const std::string& id, std::string_view utf8, const std::string& device) {
    auto parsed = parse_arrangement(utf8);
    if (!parsed) return {false, parsed.error};
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    if (parsed.arrangement->manifest_sha256 != fingerprint(it->second.manifest))
        return failure("Arrangement does not match the plugin version.");
    auto merged = merge_arrangement(it->second.manifest, *parsed.arrangement);
    if (!merged) return {false, merged.error};
    if(!device.empty()) {
        if(!device::valid_layout_id(device)) return failure("Invalid device ID.");
        auto next=it->second;
        next.device_overlays[device]=to_json(*parsed.arrangement);
        if(!device::overlay_fits(next.device_overlays)) return failure("Device overlay limit exceeded.");
        next.device_arranged.insert_or_assign(device,std::move(*merged.manifest));
        auto error=write_devices(directory_/(id+".devices.json"),next.device_overlays);
        if(!error.empty()) return {false,error};
        next.layout_revision=++layout_revision_;it->second=std::move(next);return {true,{}};
    }
    // Alle speicherintensiven Kopien vor dem Dateitausch vorbereiten.
    auto next = it->second;
    next.arranged = std::move(merged.manifest);
    next.arrangement = std::move(parsed.arrangement);
    next.arrangement_state = "ok";
    const auto text = to_json(*next.arrangement).dump();
    const auto path = directory_ / (id + ".layout.json"), backup = directory_ / (id + ".layout.json.bak");
    if (!ready_directory(directory_, false) || !regular_or_missing(path) || !regular_or_missing(backup))
        return failure("Arrangement files contain an unsafe link.");
    std::error_code ec;
    const bool exists = fs::exists(path, ec);
    if (ec) return failure("Arrangement file is unavailable.");
    if (exists) {
        std::string previous;
        if (!read_bounded(path, previous, arrangement_limit) || !atomic_write(backup, previous))
            return failure("Arrangement backup could not be saved.");
    }
    if (!atomic_write(path, text)) return failure("Arrangement could not be saved atomically.");
    next.layout_revision = ++layout_revision_;
    it->second = std::move(next);
    return {true, {}};
}
StoreResult Store::clear_arrangement(const std::string& id, const std::string& device) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    if(!device.empty()) {
        if(!device::valid_layout_id(device)) return failure("Invalid device ID.");
        auto next=it->second;next.device_overlays.erase(device);next.device_arranged.erase(device);
        auto error=write_devices(directory_/(id+".devices.json"),next.device_overlays,true);
        if(!error.empty()) return {false,error};
        next.layout_revision=++layout_revision_;it->second=std::move(next);return {true,{}};
    }
    const auto path = directory_ / (id + ".layout.json"), backup = directory_ / (id + ".layout.json.bak");
    if (!ready_directory(directory_, false) || !regular_or_missing(path) || !regular_or_missing(backup))
        return failure("Arrangement files contain an unsafe link.");
    // Backup zuerst entfernen: schlägt dies fehl, bleibt die aktive Anordnung erhalten.
    std::error_code ec;
    fs::remove(backup, ec);
    if (ec) return failure("Arrangement backup could not be removed.");
    fs::remove(path, ec);
    if (ec) return failure("Arrangement could not be removed.");
    it->second.arranged.reset();
    it->second.arrangement.reset();
    it->second.arrangement_state = "none";
    it->second.layout_revision = ++layout_revision_;
    return {true, {}};
}
Json Store::arrangement_get(const std::string& id, const std::string& device) const {
    std::lock_guard lock(mutex_);
    const auto it = installed_.find(id);
    if (it == installed_.end()) return Json();
    const auto& e = it->second;
    // Ausschließlich die validierte Basis, niemals installierte Einstellungswerte.
    auto own=e.device_overlays.find(device);
    return {{"definition", e.manifest.definition}, {"state", own!=e.device_overlays.end()?"ok":e.arrangement_state},
        {"inherited",!device.empty() && own==e.device_overlays.end()},
        {"overlay", own!=e.device_overlays.end()?own->second:to_json(e.arrangement.value_or(arrangement_from(e.manifest, fingerprint(e.manifest))))}};
}
StoreResult Store::remove(const std::string& id) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    const auto path = directory_ / (it->first + suffix), marker = directory_ / (it->first + ".disabled");
    if (!ready_directory(directory_, false) || !regular_or_missing(path) || !regular_or_missing(marker) ||
        !regular_or_missing(directory_ / (id + ".layout.json")) || !regular_or_missing(directory_ / (id + ".layout.json.bak")))
        return failure("Plugin files contain an unsafe link.");
    std::error_code ec;
    for(const auto& ext:{".devices.json",".devices.json.bak"})
        if(!regular_or_missing(directory_/(id+ext))) return failure("Unsafe device arrangement path.");
    fs::remove(path, ec);
    if (ec) return failure("Plugin manifest could not be removed.");
    // Removing the manifest is the committed uninstall; a failed marker cleanup
    // must not keep this plugin alive in memory.
    installed_.erase(it);
    for(const auto& ext:{".devices.json",".devices.json.bak"}) {fs::remove(directory_/(id+ext),ec);if(ec) return failure("Device arrangement cleanup failed.");}
    fs::remove(marker, ec);
    if (ec) return failure("Plugin was removed but its disabled marker could not be cleaned up.");
    fs::remove(directory_ / (id + ".consent.json"), ec);
    if (ec) return failure("Plugin was removed but its consent could not be cleaned up.");
    fs::remove(directory_ / (id + ".links.json"), ec);
    if (ec) return failure("Plugin link consent could not be cleaned up.");
    fs::remove(directory_ / (id + ".layout.json"), ec);
    if (ec) return failure("Plugin arrangement could not be cleaned up.");
    fs::remove(directory_ / (id + ".layout.json.bak"), ec);
    if (ec) return failure("Plugin arrangement backup could not be cleaned up.");
    return {true, {}};
}
StoreResult Store::set_host_consent(const std::string& id, const std::string& host, bool allowed) {
    std::lock_guard lock(mutex_);
    auto it = installed_.find(id);
    if (it == installed_.end()) return failure("Plugin is not installed.");
    bool declared = false;
    for (const auto& source : it->second.manifest.sources) declared |= source.host == host;
    if (!declared) return failure("Host is not declared by this plugin.");
    Installed next = it->second;
    if (allowed) next.consented_hosts.insert(host); else next.consented_hosts.erase(host);
    if (!allowed) {
        // Revoke memory authorization immediately even if the disk write fails.
        it->second.enabled = false;
        it->second.consented_hosts.erase(host);
        it->second.generation = ++generation_;
        next.enabled = false;
        if (!ready_directory(directory_, false)) return failure("Plugin directory is unavailable.");
        if (!atomic_write(directory_ / (id + ".disabled"), "")) return failure("Plugin could not be disabled safely.");
    }
    if (!ready_directory(directory_, false)) return failure("Plugin directory is unavailable.");
    if (!write_consent(directory_ / (id + ".consent.json"), next)) return failure("Plugin consent could not be saved.");
    next.generation = ++generation_;
    it->second = std::move(next);
    return {true, {}};
}
StoreResult Store::set_link_consent(const std::string& id, bool allowed) {
    std::lock_guard lock(mutex_);
    auto it=installed_.find(id);
    if(it==installed_.end()) return failure("Plugin is not installed.");
    if(allowed && it->second.manifest.links.empty()) return failure("Plugin has no declared link hosts.");
    if(!allowed) { it->second.links_allowed=false; it->second.generation=++generation_; }
    auto next=it->second; next.links_allowed=allowed;
    if(!ready_directory(directory_,false) || !write_links(directory_/(id+".links.json"),next))
        return failure("Plugin link consent could not be saved.");
    next.generation=++generation_; it->second=std::move(next);return {true,{}};
}
bool Store::source_authorized(const std::string& id, const std::string& source_id, uint64_t generation) const {
    std::lock_guard lock(mutex_);
    const auto it = installed_.find(id);
    if (it == installed_.end() || !it->second.enabled || it->second.generation != generation) return false;
    for (const auto& source : it->second.manifest.sources)
        if (source.id == source_id) return it->second.consented_hosts.contains(source.host);
    return false;
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
