// © 2026 FuchsLabs – isolated temporary-directory tests, no hardware or network.
#include "plugin_store.h"
#include <chrono>
#include <cstdio>
#include <fstream>

using namespace fl::plugins;
namespace fs = std::filesystem;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static std::string manifest(const std::string& id = "clock", const std::string& name = "Clock") {
    return Json({{"schema_version", 1}, {"id", id}, {"name", name}, {"version", "1.0.0"},
        {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}}).dump();
}
static void write(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary); stream << text;
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const auto root = fs::temp_directory_path() /
        ("fl-plugin-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 2;
    // Cleanup only the exact newly created test directory; never any caller path.
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    Store store(root / "plugins");
    CHECK(!store.import_manifest(manifest()));
    CHECK(store.load());
    CHECK(!fs::exists(root / "plugins"));
    CHECK(store.entries().empty());
    CHECK(store.import_manifest(manifest()));
    CHECK(store.entries().size() == 1);
    CHECK(store.find("clock") && store.find("clock")->enabled);
    CHECK(fs::exists(root / "plugins/clock.flplugin.json"));
    CHECK(!store.import_manifest("not JSON"));
    CHECK(store.entries().size() == 1);
    CHECK(store.set_enabled("clock", false));
    CHECK(!store.find("clock")->enabled);
    CHECK(fs::file_size(root / "plugins/clock.disabled") == 0);
    CHECK(store.import_manifest(manifest("clock", "Updated")));
    CHECK(store.entries().size() == 1 && store.find("clock")->manifest.name == "Updated");
    CHECK(!store.find("clock")->enabled);
    Store second(root / "plugins");
    CHECK(second.load());
    CHECK(second.entries().size() == 1 && !second.find("clock")->enabled);
    CHECK(second.set_enabled("clock", true));
    CHECK(!fs::exists(root / "plugins/clock.disabled"));
    CHECK(second.load() && second.find("clock")->enabled);
    CHECK(!second.set_enabled("../outside", false));
    CHECK(!second.remove("../outside"));
    CHECK(!second.find("missing"));
    write(root / "plugins/wrong.flplugin.json", manifest("different"));
    write(root / "plugins/invalid.flplugin.json", "{}");
    write(root / "plugins/ignored.txt", "anything");
    CHECK(!second.load());
    CHECK(second.entries().size() == 1);
    CHECK(!second.find("different"));
    CHECK(second.remove("clock"));
    CHECK(second.entries().empty());
    CHECK(!fs::exists(root / "plugins/clock.flplugin.json"));
    CHECK(!second.remove("clock"));
    CHECK(second.import_manifest(manifest()));
    write(root / "plugins/clock.disabled", "invalid-state");
    CHECK(!second.load());
    CHECK(!second.find("clock"));
    CHECK(!second.import_manifest(manifest()));
    auto network_json = Json::parse(manifest("network"));
    network_json["sources"] = Json::array({{{"id", "feed"}, {"url", "https://api.example.com/data"},
        {"sample", {{"value", 42}}}}});
    Store offline_only(root / "offline-only");
    CHECK(offline_only.load());
    CHECK(offline_only.import_manifest(network_json.dump()));
    CHECK(offline_only.find("network") && !offline_only.find("network")->enabled);
    CHECK(fs::exists(root / "offline-only/network.disabled"));
    CHECK(!offline_only.set_enabled("network", true));
    CHECK(offline_only.set_enabled("network", false));
    CHECK(offline_only.import_manifest(network_json.dump()));
    CHECK(!offline_only.find("network")->enabled);
    fs::remove(root / "offline-only/network.disabled");
    CHECK(offline_only.load());
    CHECK(!offline_only.find("network")->enabled); // deleting the marker cannot activate sources
    CHECK(!offline_only.set_enabled("network", true));
    CHECK(offline_only.import_manifest(manifest("clock")));
    network_json["id"] = "clock";
    CHECK(offline_only.import_manifest(network_json.dump()));
    CHECK(!offline_only.find("clock")->enabled); // adding sources to an enabled page disables it
    Store consent(root / "consent");
    network_json["id"] = "network";
    CHECK(consent.load());
    CHECK(consent.import_manifest(network_json.dump()));
    CHECK(!consent.set_host_consent("network", "undeclared.example.com", true));
    CHECK(consent.set_host_consent("network", "api.example.com", true));
    CHECK(!consent.find("network")->enabled); // host approval never silently enables
    CHECK(consent.set_enabled("network", true));
    const auto approved_generation = consent.find("network")->generation;
    CHECK(consent.source_authorized("network", "feed", approved_generation));
    CHECK(!consent.source_authorized("network", "missing", approved_generation));
    CHECK(consent.load());
    CHECK(consent.find("network")->enabled);
    CHECK(!consent.source_authorized("network", "feed", approved_generation));
    CHECK(consent.import_manifest(network_json.dump())); // exact reimport revokes consent
    CHECK(!consent.find("network")->enabled);
    CHECK(consent.find("network")->consented_hosts.empty());
    CHECK(consent.load());
    CHECK(!consent.set_enabled("network", true));
    CHECK(consent.set_host_consent("network", "api.example.com", true));
    CHECK(consent.set_enabled("network", true));
    network_json["version"] = "1.0.1";
    write(root / "consent/network.flplugin.json", network_json.dump());
    CHECK(consent.load());
    CHECK(!consent.find("network")->enabled); // disk edits invalidate the bound grant
    CHECK(consent.find("network")->consented_hosts.empty());
    CHECK(consent.set_host_consent("network", "api.example.com", true));
    CHECK(consent.set_enabled("network", true));
    const auto before_failure = consent.find("network")->generation;
    // An unavailable store must not prevent immediate in-memory revocation.
    fs::rename(root / "consent", root / "consent-moved");
    CHECK(!consent.set_host_consent("network", "api.example.com", false));
    CHECK(!consent.find("network")->enabled);
    CHECK(!consent.source_authorized("network", "feed", before_failure));
    CHECK(consent.find("network")->consented_hosts.empty());
    fs::rename(root / "consent-moved", root / "consent");
    CHECK(consent.set_host_consent("network", "api.example.com", false));
    CHECK(consent.load());
    CHECK(!consent.find("network")->enabled);
    CHECK(consent.set_host_consent("network", "api.example.com", true));
    CHECK(consent.set_enabled("network", true));
    fs::remove(root / "consent/network.consent.json");
    fs::create_directory(root / "consent/network.consent.json");
    CHECK(!consent.import_manifest(network_json.dump()));
    CHECK(!consent.find("network")->enabled);
    CHECK(consent.find("network")->consented_hosts.empty());
    CHECK(!consent.set_enabled("network", true));
    Store capped(root / "capped");
    CHECK(capped.load());
    for (size_t i = 0; i < Store::installation_limit; ++i) CHECK(capped.import_manifest(manifest("clock-" + std::to_string(i))));
    CHECK(!capped.import_manifest(manifest("overflow")));
    CHECK(capped.import_manifest(manifest("clock-0", "Updated at limit")));
    CHECK(capped.entries().size() == Store::installation_limit);
    Store reload(root / "capped");
    CHECK(reload.load());
    CHECK(reload.entries().size() == Store::installation_limit);
    write(root / "outside.flplugin.json", manifest());
    fs::create_directory(root / "links");
    std::error_code link_error;
    fs::create_symlink(root / "outside.flplugin.json", root / "links/clock.flplugin.json", link_error);
    if (!link_error) {
        Store links(root / "links");
        CHECK(!links.load());
        CHECK(links.entries().empty());
        CHECK(!links.import_manifest(manifest()));
        CHECK(fs::exists(root / "outside.flplugin.json"));
    } else std::printf("SKIP symlink test: creating symbolic links is unavailable.\n");
    std::printf("plugin store: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
