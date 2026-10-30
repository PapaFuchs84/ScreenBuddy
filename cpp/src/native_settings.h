// © 2026 FuchsLabs
#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace fl {
class App;

// All plugin callbacks run serially on the settings job thread, never on the UI thread.
// The owner must keep App and callback dependencies alive until this window is destroyed.
struct NativePluginSetting {
    std::string key, label, type, value; // Public value only; secret values must always be empty.
    bool required = false, secret_set = false;
};
struct NativePluginEntry {
    std::string id, name, version;
    bool enabled = false;
    std::vector<std::string> hosts, consented_hosts, setting_keys;
    std::vector<NativePluginSetting> settings;
};
// Plugin store (signed index, see plugin_catalog.h). Data only; the store never executes plugin code.
struct NativeStoreCard {
    std::string id, name, description, version, author, license, category, min_app_version;
    std::string status;            // not_installed | installed | update_available | unsupported | revoked
    std::string installed_version;
    std::vector<std::string> hosts, new_hosts;
};
struct NativeStoreState {
    std::string state = "unavailable"; // unavailable | disabled | offline | invalid | available
    bool from_cache = false;
    std::string message;               // fixed diagnostics, German/English chosen by the UI
    std::vector<NativeStoreCard> cards;
};
struct NativePluginActions {
    std::function<std::vector<NativePluginEntry>()> list;
    std::function<void(const std::filesystem::path&)> import_manifest;
    std::function<void(const std::string&, bool)> set_enabled;
    std::function<void(const std::string&)> remove;
    std::function<void(const std::string&, const std::string&, bool)> set_host_consent;
    std::function<void(const std::string&, const nlohmann::json&)> save_settings;
    std::function<void()> open_advanced;
    // Store: all three run on the settings job thread (they may block on the network).
    std::function<NativeStoreState(const std::string& search, const std::string& category)> store_list;
    std::function<NativeStoreState(bool force)> store_refresh; // force = user pressed "Refresh"; otherwise at most once a day
    std::function<void(const std::string& id)> store_install; // throws std::runtime_error with a user-facing message
};

// Browser-free settings host. Closing hides the window; destruction joins both owned threads.
class NativeSettings {
public:
    explicit NativeSettings(App& app, NativePluginActions plugins = {});
    ~NativeSettings();
    NativeSettings(const NativeSettings&) = delete;
    NativeSettings& operator=(const NativeSettings&) = delete;
    void show();
    bool visible() const;
private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};
}  // namespace fl
