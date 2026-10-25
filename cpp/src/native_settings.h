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
struct NativePluginActions {
    std::function<std::vector<NativePluginEntry>()> list;
    std::function<void(const std::filesystem::path&)> import_manifest;
    std::function<void(const std::string&, bool)> set_enabled;
    std::function<void(const std::string&)> remove;
    std::function<void(const std::string&, const std::string&, bool)> set_host_consent;
    std::function<void(const std::string&, const nlohmann::json&)> save_settings;
    std::function<void()> open_advanced;
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
