// © 2026 FuchsLabs – store client tests: refresh, rollback, cache, listing, install, revocation. Offline, temp dirs only.
// Standalone: c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp src/plugin_sign.cpp src/plugin_store.cpp src/plugin_catalog.cpp tests/plugin_catalog_tests.cpp -lcrypto
#include "plugin_catalog.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <random>

using namespace fl::plugins;
using namespace fl::plugins::catalog;
namespace fs = std::filesystem;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static std::string manifest(const std::string& id, const std::string& version, const std::string& host = "") {
    Json j = {{"schema_version", 1}, {"id", id}, {"name", "Plugin " + id}, {"description", "Beschreibung fuer " + id},
        {"version", version}, {"license", "MIT"}, {"author", "FuchsLabs"}, {"category", host.empty() ? "system" : "info"},
        {"min_app_version", "2.5.0"}, {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}};
    if (!host.empty()) {
        j["refresh_seconds"] = 900;
        j["sources"] = Json::array({{{"id", "w"}, {"url", "https://" + host + "/x"}, {"sample", {{"t", 1}}}}});
        j["widgets"] = Json::array({{{"type", "metric"}, {"value", "{{w.t}}"}}});
    }
    return j.dump();
}

// A fake "server": path -> bytes. Missing path = network failure.
struct Server {
    std::map<std::string, std::string> files;
    sign::KeyPair keys;
    bool online = true;
    int requests = 0;
    Fetcher fetcher() {
        return [this](const std::string& path, size_t limit) -> std::optional<std::string> {
            ++requests;
            auto it = files.find(path);
            if (!online || it == files.end() || it->second.size() > limit) return std::nullopt;
            return it->second;
        };
    }
    void publish(const std::vector<std::string>& manifests, int sequence, const Json& revoked = Json::array()) {
        std::vector<sign::BuildInput> in;
        files.clear();
        for (const auto& m : manifests) {
            in.push_back({m});
            auto parsed = parse_manifest(m);
            files["plugins/" + parsed.manifest->id + "/" + parsed.manifest->id + ".flplugin.json"] = m;
        }
        auto built = sign::build_index(in, sequence, "2026-10-30T12:00:00Z");
        files["index.json"] = *built.index_json;
        files["index.json.sig"] = *sign::sign_document(sign::Kind::index, keys.private_pem, *built.index_json) + "\n";
        Json r = {{"schema_version", 1}, {"sequence", sequence}, {"revoked", revoked}};
        files["revoked.json"] = r.dump();
        files["revoked.json.sig"] = *sign::sign_document(sign::Kind::revoked, keys.private_pem, files["revoked.json"]) + "\n";
    }
};

int main() {
    const auto root = fs::temp_directory_path() / ("fl-catalog-test-" + std::to_string(std::random_device{}()));
    fs::remove_all(root);
    Server server;
    server.keys = *sign::generate_keypair();
    const auto key = *sign::public_key_from_hex(server.keys.public_hex);

    // Without a pinned production key the store is unavailable and makes no request.
    CHECK(std::string(sign::store_public_key_hex).empty());
    CHECK(!Catalog::from_pinned_key(root / "x", server.fetcher(), "2.5.0").has_value());
    CHECK(server.requests == 0);

    server.publish({manifest("clock-a", "1.0.0"), manifest("weather-b", "1.0.0", "api.open-meteo.com"), manifest("new-c", "1.0.0")}, 5);
    Catalog catalog(key, root / "cache", server.fetcher(), "2.5.0");
    CHECK(!catalog.index().has_value()); // empty cache
    auto r = catalog.refresh();
    CHECK(r.state == Availability::available && !r.from_cache && catalog.index() && catalog.index()->sequence == 5);

    // Listing, search, category.
    auto cards = catalog.list({}, {});
    CHECK(cards.size() == 3 && cards[0].entry.id == "clock-a");
    CHECK(catalog.list({"WEATHER", "", false}, {}).size() == 1);
    CHECK(catalog.list({"", "info", false}, {}).size() == 1);
    CHECK(catalog.list({"nichts", "", false}, {}).empty());

    // Install: verified, disabled with sources, no consent.
    Store store(root / "plugins");
    CHECK(store.load());
    CHECK(catalog.install(store, "weather-b"));
    auto w = store.find("weather-b");
    CHECK(w && !w->enabled && w->consented_hosts.empty());
    CHECK(catalog.install(store, "clock-a"));
    CHECK(store.find("clock-a") && store.find("clock-a")->enabled);
    CHECK(!catalog.install(store, "missing"));
    cards = catalog.list({}, store.entries());
    CHECK(cards[0].status == Status::installed);

    // Update: new version adds a host -> update_available + new_hosts; consent is reset by the store.
    server.publish({manifest("clock-a", "1.0.0"), manifest("weather-b", "1.1.0", "evil.example.org"), manifest("new-c", "1.0.0")}, 6);
    CHECK(catalog.refresh().state == Availability::available);
    cards = catalog.list({}, store.entries());
    const Card* wb = nullptr;
    for (const auto& c : cards) if (c.entry.id == "weather-b") wb = &c;
    CHECK(wb && wb->status == Status::update_available && wb->installed_version == "1.0.0" &&
          wb->new_hosts == std::vector<std::string>({"evil.example.org"}));
    CHECK(store.set_host_consent("weather-b", "api.open-meteo.com", true));
    CHECK(catalog.install(store, "weather-b"));
    CHECK(store.find("weather-b")->consented_hosts.empty() && !store.find("weather-b")->enabled); // consent reset

    // Tampering: wrong bytes for a package, altered index, other key, rollback, old revocation list.
    server.files["plugins/new-c/new-c.flplugin.json"] = manifest("new-c", "1.0.1");
    CHECK(!catalog.install(store, "new-c"));
    CHECK(!store.find("new-c"));
    server.publish({manifest("clock-a", "1.0.0")}, 4); // older sequence, correctly signed
    r = catalog.refresh();
    CHECK(r.state == Availability::invalid && catalog.index()->sequence == 6 && catalog.index()->plugins.size() == 3);
    server.publish({manifest("clock-a", "1.0.0")}, 7);
    server.files["index.json"] += " ";
    r = catalog.refresh();
    CHECK(r.state == Availability::invalid && catalog.index()->sequence == 6);
    auto stranger = *sign::generate_keypair();
    Server evil; evil.keys = stranger; evil.publish({manifest("bad", "1.0.0")}, 99);
    Catalog victim(key, root / "cache2", evil.fetcher(), "2.5.0");
    CHECK(victim.refresh().state == Availability::invalid && !victim.index());

    // Offline: verified cache is used; tampered cache is ignored.
    server.publish({manifest("clock-a", "1.0.0")}, 9);
    CHECK(catalog.refresh().state == Availability::available);
    server.online = false;
    Catalog restarted(key, root / "cache", server.fetcher(), "2.5.0");
    CHECK(restarted.index() && restarted.index()->sequence == 9);
    r = restarted.refresh();
    CHECK(r.state == Availability::offline && r.from_cache);
    { std::ofstream bad(root / "cache" / "index.json", std::ios::app); bad << " "; }
    Catalog tampered(key, root / "cache", server.fetcher(), "2.5.0");
    CHECK(!tampered.index());
    r = tampered.refresh();
    CHECK(r.state == Availability::offline && !r.from_cache);
    server.online = true;

    // Cache without a verified revocation list is not used at all.
    server.online = true;
    server.publish({manifest("clock-a", "1.0.0")}, 11);
    Catalog c0(key, root / "cache5", server.fetcher(), "2.5.0");
    CHECK(c0.refresh().state == Availability::available);
    fs::remove(root / "cache5" / "revoked.json");
    Catalog c0b(key, root / "cache5", server.fetcher(), "2.5.0");
    CHECK(!c0b.index() && !c0b.revocations());
    // A verified revocation list survives a missing index (offline start still switches plugins off).
    server.publish({manifest("clock-a", "1.0.0")}, 11);
    Catalog c0e(key, root / "cache6", server.fetcher(), "2.5.0");
    CHECK(c0e.refresh().state == Availability::available);
    fs::remove(root / "cache6" / "index.json");
    Catalog c0f(key, root / "cache6", server.fetcher(), "2.5.0");
    CHECK(!c0f.index() && c0f.revocations() && c0f.revocations()->sequence == 11);
    // Host-supplied importer is used for installs.
    {
        Store store2(root / "plugins2"); CHECK(store2.load());
        int calls = 0;
        CHECK(c0.install(store2, "clock-a", [&](std::string_view bytes) { ++calls; return store2.import_manifest(bytes); }));
        CHECK(calls == 1 && store2.find("clock-a"));
    }
    // Refresh replaces an existing cache (sequence advances on disk).
    server.publish({manifest("clock-a", "1.0.0")}, 12);
    CHECK(c0b.refresh().state == Availability::available);
    Catalog c0c(key, root / "cache5", server.fetcher(), "2.5.0");
    CHECK(c0c.index() && c0c.index()->sequence == 12);
    server.publish({manifest("clock-a", "1.0.0")}, 13);
    CHECK(c0c.refresh().state == Availability::available);
    Catalog c0d(key, root / "cache5", server.fetcher(), "2.5.0");
    CHECK(c0d.index() && c0d.index()->sequence == 13);

    // Compatibility and revocation.
    server.publish({manifest("clock-a", "1.0.0"), manifest("weather-b", "1.1.0", "evil.example.org")}, 10,
                   Json::array({{{"id", "weather-b"}, {"versions", "all"}, {"reason", "Datenabfluss"}}}));
    Catalog c2(key, root / "cache3", server.fetcher(), "2.4.0");
    CHECK(c2.refresh().state == Availability::available);
    auto list2 = c2.list({}, {});
    CHECK(list2.size() == 1 && list2[0].status == Status::unsupported);   // weather hidden (revoked), clock needs 2.5.0
    CHECK(!c2.install(store, "clock-a"));                                  // too old
    Catalog c3(key, root / "cache4", server.fetcher(), "2.5.0");
    CHECK(c3.refresh().state == Availability::available);
    CHECK(c3.list({"", "", true}, {}).size() == 2);
    CHECK(!c3.install(store, "weather-b"));                                // revoked
    // An installed, enabled, revoked plugin is switched off.
    store.import_manifest(manifest("weather-b", "1.1.0", "evil.example.org"));
    store.set_host_consent("weather-b", "evil.example.org", true);
    CHECK(store.set_enabled("weather-b", true));
    auto off = c3.enforce_revocations(store);
    CHECK(off == std::vector<std::string>({"weather-b"}) && !store.find("weather-b")->enabled);
    CHECK(c3.enforce_revocations(store).empty());

    fs::remove_all(root);
    std::printf("plugin catalog: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
