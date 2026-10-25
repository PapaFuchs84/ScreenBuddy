// © 2026 FuchsLabs – isolated per-user DPAPI/filesystem tests; no network/hardware.
#include "plugin_settings.h"
#include "secure.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

using namespace fl::plugins;
namespace fs = std::filesystem;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)
static Manifest manifest() {
    const Json data = {{"schema_version", 1}, {"id", "settings-test"}, {"name", "Settings test"}, {"version", "1.0.0"},
        {"settings", Json::array({
            {{"key", "city"}, {"label", "City"}, {"type", "text"}, {"default", "Berlin"}, {"required", true}},
            {{"key", "count"}, {"label", "Count"}, {"type", "number"}, {"default", 7}},
            {{"key", "token"}, {"label", "Token"}, {"type", "secret"}, {"required", true}}})},
        {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}};
    return *parse_manifest(data.dump()).manifest;
}
static std::string read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
static void write(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc); stream << text;
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const auto root = fs::temp_directory_path() /
        ("fl-plugin-settings-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 2;
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    const auto directory = root / "plugin-settings";
    const auto path = directory / "settings-test.settings.json";
    auto m = manifest();
    SettingsStore settings(directory);
    CHECK(!settings.save(m, {{"city", "Paris"}}));
    CHECK(settings.load(m));
    CHECK(!fs::exists(directory));
    CHECK(settings.public_view(m)["values"]["city"] == "Berlin");
    CHECK(settings.public_view(m)["values"]["count"] == 7);
    CHECK(!settings.public_view(m)["values"].contains("token"));
    CHECK(settings.public_view(m)["secrets"]["token"] == false);
    CHECK(settings.public_view(m)["missing_required"] == Json::array({"token"}));
    CHECK(!settings.runtime_values(m));
    const std::string secret = "PRIVATE-PLUGIN-SECRET-DO-NOT-EXPORT";
    const auto initial_save = settings.save(m, {{"city", "Paris"}, {"count", 12.5}, {"token", secret}});
    CHECK(initial_save);
    if (!initial_save) { std::printf("Initial save diagnostic: %s\n", initial_save.error.c_str()); return 2; }
    CHECK(settings.runtime_values(m) && (*settings.runtime_values(m).values)["token"] == secret);
    const auto persisted = read(path);
    CHECK(persisted.find(secret) == std::string::npos);
    CHECK(fl::secure::is_protected(Json::parse(persisted)["settings"]["token"]["value"].get<std::string>()));
    const auto view = settings.public_view(m);
    CHECK(view["secrets"]["token"] == true);
    CHECK(view["missing_required"].empty());
    CHECK(view.dump().find(secret) == std::string::npos && view.dump().find("dpapi:") == std::string::npos);
    SettingsStore second(directory);
    CHECK(second.load(m));
    CHECK(second.runtime_values(m) && (*second.runtime_values(m).values)["token"] == secret);
    CHECK(second.save(m, {{"city", "London"}}));
    CHECK((*second.runtime_values(m).values)["token"] == secret); // omitted secret preserved
    const auto before_invalid = read(path);
    CHECK(!second.save(m, {{"count", "12"}}));
    CHECK(!second.save(m, {{"unknown", "x"}}));
    CHECK(!second.save(m, Json::array()));
    CHECK(!second.save(m, {{"token", 1}}));
    CHECK(!second.save(m, {{"city", std::string(SettingsStore::value_limit + 1, 'a')}}));
    CHECK(!second.save(m, {{"token", "header\r\ninjection"}}));
    CHECK(!second.save(m, {{"city", std::string("embedded\0nul", 12)}}));
    CHECK(read(path) == before_invalid);
    CHECK(second.save(m, {{"city", nullptr}, {"count", nullptr}}));
    CHECK((*second.runtime_values(m).values)["city"] == "Berlin");
    CHECK((*second.runtime_values(m).values)["count"] == 7);
    CHECK(second.save(m, {{"token", ""}}));
    CHECK(!second.runtime_values(m));
    CHECK(second.public_view(m)["secrets"]["token"] == false);
    CHECK(second.save(m, {{"token", secret}, {"city", ""}}));
    CHECK(!second.runtime_values(m));
    CHECK(second.public_view(m)["missing_required"] == Json::array({"city"}));
    CHECK(second.save(m, {{"city", nullptr}}));
    CHECK(second.save(m, {{"token", nullptr}}));
    CHECK(!second.runtime_values(m));
    CHECK(second.save(m, {{"token", secret}}));

    // Defaults follow the current manifest; explicit overrides survive same-type updates.
    auto updated = m;
    updated.settings[0].default_value = "Vienna";
    updated.settings[1].default_value = 99;
    CHECK(!second.runtime_values(updated)); // must load the current schema first
    CHECK(second.load(updated));
    CHECK((*second.runtime_values(updated).values)["city"] == "Vienna");
    CHECK((*second.runtime_values(updated).values)["count"] == 99);
    CHECK((*second.runtime_values(updated).values)["token"] == secret);
    updated.settings[0].type = "secret"; updated.settings[0].default_value = "";
    CHECK(second.load(updated));
    CHECK(!second.public_view(updated)["values"].contains("city"));
    CHECK(second.public_view(updated)["secrets"]["city"] == false);
    CHECK(!second.runtime_values(updated));

    // Removed credentials are deleted on disk and cannot return when a key is re-added.
    auto removed = m; removed.settings.pop_back();
    CHECK(second.load(removed));
    CHECK(read(path).find("token") == std::string::npos);
    CHECK(second.load(m));
    CHECK(!second.runtime_values(m));
    CHECK(second.public_view(m)["secrets"]["token"] == false);
    CHECK(second.save(m, {{"token", secret}}));

    // Failed disk writes preserve the last committed cache; no partial mutation.
    fs::rename(path, directory / "saved.json");
    fs::create_directory(path);
    CHECK(!second.save(m, {{"city", "Failed"}}));
    CHECK((*second.runtime_values(m).values)["city"] == "Berlin");
    CHECK(!second.remove(m.id));
    fs::remove(path);
    fs::rename(directory / "saved.json", path);
    CHECK(second.remove(m.id));
    CHECK(!fs::exists(path));
    CHECK(!second.runtime_values(m));
    CHECK(second.remove(m.id)); // idempotent deletion makes uninstall recovery possible
    CHECK(second.load(m));
    CHECK(!second.runtime_values(m));
    CHECK(!second.remove("../escape"));
    auto invalid = m; invalid.id = "../escape";
    CHECK(!second.load(invalid));

    // Malformed/oversized/duplicate/protected-by-another-user files fail closed.
    CHECK(second.save(m, {{"token", secret}}));
    auto disk = Json::parse(read(path));
    disk["settings"]["token"]["value"] = secret;
    write(path, disk.dump());
    CHECK(!second.load(m));
    CHECK(!second.runtime_values(m));
    CHECK(second.public_view(m).dump().find(secret) == std::string::npos);
    disk["settings"]["token"]["value"] = "dpapi:AAAA";
    write(path, disk.dump());
    CHECK(!second.load(m));
    write(path, std::string(SettingsStore::file_limit + 1, ' '));
    CHECK(!second.load(m));
    write(path, std::string(9, '[') + "0" + std::string(9, ']'));
    CHECK(!second.load(m));
    write(path, "{\"schema_version\":1,\"id\":\"settings-test\",\"settings\":{},\"settings\":{}}");
    CHECK(!second.load(m));
    CHECK(second.remove(m.id));
    CHECK(second.load(m));

    // Save/retrieve concurrently, as native settings and the transport worker do.
    CHECK(second.save(m, {{"token", secret}}));
    std::atomic<bool> concurrent_ok{true};
    std::thread writer([&] {
        for (int i = 0; i < 16; ++i) if (!second.save(m, {{"count", i}})) concurrent_ok = false;
    });
    std::thread reader([&] {
        for (int i = 0; i < 64; ++i) {
            const auto values = second.runtime_values(m);
            if (!values || (*values.values)["token"] != secret ||
                second.public_view(m).dump().find(secret) != std::string::npos) concurrent_ok = false;
        }
    });
    writer.join(); reader.join();
    CHECK(concurrent_ok.load());
    CHECK((*second.runtime_values(m).values)["count"] == 15);
    CHECK(second.remove(m.id));

    // Plugin secrets use their own DPAPI entropy (plugin ID + key): neither the shared main-config
    // entropy nor another plugin/key can open them; legacy shared-entropy entries migrate on load.
    CHECK(second.load(m));
    CHECK(second.save(m, {{"token", secret}, {"city", "Hamburg"}}));
    const auto ciphertext = Json::parse(read(path))["settings"]["token"]["value"].get<std::string>();
    CHECK(fl::secure::unprotect(ciphertext).empty());
    CHECK(fl::secure::unprotect(ciphertext, "FuchsLabs.USB-Display.v1").empty());
    CHECK(fl::secure::unprotect(ciphertext, "FuchsLabs.USB-Display.plugin.v1:other-plugin:token").empty());
    CHECK(fl::secure::unprotect(ciphertext, "FuchsLabs.USB-Display.plugin.v1:settings-test:city").empty());
    CHECK(fl::secure::unprotect(ciphertext, "FuchsLabs.USB-Display.plugin.v1:settings-test:token") == secret);
    // A ciphertext with foreign entropy is not usable for this plugin/key.
    auto swapped = Json::parse(read(path));
    swapped["settings"]["token"]["value"] = fl::secure::protect(secret, "x");
    write(path, swapped.dump());
    CHECK(!second.load(m));
    // Main-config secrets keep working with the original shared entropy.
    CHECK(fl::secure::unprotect(fl::secure::protect(secret)) == secret);
    // Legacy file (shared entropy) is readable and rewritten with plugin entropy.
    auto legacy = Json::parse(read(path));
    legacy["settings"]["token"]["value"] = fl::secure::protect(secret);
    write(path, legacy.dump());
    CHECK(second.load(m));
    CHECK((*second.runtime_values(m).values)["token"] == secret);
    const auto migrated = Json::parse(read(path))["settings"]["token"]["value"].get<std::string>();
    CHECK(fl::secure::unprotect(migrated).empty());
    CHECK(fl::secure::unprotect(migrated, "FuchsLabs.USB-Display.plugin.v1:settings-test:token") == secret);

    // Manifest update with changed hosts: stored secrets are discarded, plain values stay.
    CHECK(second.save(m, {{"token", secret}, {"city", "Hamburg"}}));
    CHECK(second.discard_secrets(m.id));
    CHECK(read(path).find("token") == std::string::npos && read(path).find("dpapi:") == std::string::npos);
    CHECK(second.load(m));
    CHECK(!second.runtime_values(m)); // required token is gone again
    CHECK(second.public_view(m)["secrets"]["token"] == false);
    CHECK(second.public_view(m)["values"]["city"] == "Hamburg");
    CHECK(second.save(m, {{"token", secret}}));
    CHECK((*second.runtime_values(m).values)["token"] == secret);
    // Only secrets left: the file is removed. Missing and unreadable files are fine too.
    CHECK(second.save(m, {{"city", nullptr}}));
    CHECK(second.discard_secrets(m.id));
    CHECK(!fs::exists(path));
    CHECK(second.discard_secrets(m.id));
    write(path, "{broken");
    CHECK(second.discard_secrets(m.id));
    CHECK(!fs::exists(path));
    CHECK(!second.discard_secrets("../escape"));
    CHECK(second.load(m));

    // Windows device names are refused as plugin IDs by the store as well.
    for (const char* device : {"con", "nul", "aux", "prn", "com1", "lpt9"}) {
        auto reserved = m; reserved.id = device;
        CHECK(!second.load(reserved));
        CHECK(!second.remove(device));
        CHECK(!second.discard_secrets(device));
    }
    CHECK(second.remove(m.id));

    // Refuse links both at file level and in the dedicated directory's parents.
    write(root / "outside.json", "untouched");
    std::error_code link_error;
    fs::create_symlink(root / "outside.json", path, link_error);
    if (!link_error) {
        CHECK(!second.load(m));
        CHECK(!second.remove(m.id));
        CHECK(read(root / "outside.json") == "untouched");
    } else std::printf("SKIP symbolic-link test: unavailable on this account.\n");
    link_error.clear();
    fs::create_directory_symlink(directory, root / "linked", link_error);
    if (!link_error) {
        SettingsStore linked(root / "linked");
        CHECK(!linked.load(m));
        CHECK(!linked.remove(m.id));
    } else std::printf("SKIP directory-link test: unavailable on this account.\n");
    std::printf("plugin settings: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
