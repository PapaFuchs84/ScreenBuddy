// © 2026 FuchsLabs – isolated temporary-directory tests, no hardware or network.
#include "plugin_store.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <array>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

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
static std::string read(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static void test_arrangements(const fs::path& root) {
    const auto dir=root/"arrangements";
    Store store(dir); CHECK(store.load());
    auto j=Json::parse(manifest("arranged")); j["schema_version"]=2; j["widgets"][0]["id"]="clock";
    j["links"]={"news.example.org"};
    j["sources"]=Json::array({{{"id","feed"},{"url","https://api.example.org/data"}}});
    CHECK(store.import_manifest(j.dump()));
    CHECK(store.set_host_consent("arranged","api.example.org",true));
    CHECK(store.set_link_consent("arranged",true)); CHECK(store.set_enabled("arranged",true));
    auto before=*store.find("arranged");
    auto initial=store.arrangement_get("arranged");
    CHECK(initial["state"]=="none"); CHECK(initial["definition"]==before.manifest.definition);
    CHECK(store.arrangement_get("missing").is_null());
    auto a=initial["overlay"]; auto path=dir/"arranged.layout.json", backup=dir/"arranged.layout.json.bak";
    auto manifest_bytes=read(dir/"arranged.flplugin.json");
    auto consent_bytes=read(dir/"arranged.consent.json"), links_bytes=read(dir/"arranged.links.json");
    CHECK(store.set_arrangement("arranged",a.dump()));
    auto after=*store.find("arranged");
    CHECK(after.arranged && after.arrangement_state=="ok");
    CHECK(after.generation==before.generation && after.layout_revision>before.layout_revision);
    CHECK(after.enabled && after.links_allowed && after.consented_hosts==before.consented_hosts);
    CHECK(store.source_authorized("arranged","feed",before.generation));
    CHECK(read(dir/"arranged.flplugin.json")==manifest_bytes);
    CHECK(read(dir/"arranged.consent.json")==consent_bytes && read(dir/"arranged.links.json")==links_bytes);
    CHECK(!fs::exists(backup));
    auto previous=read(path); a["layout"]["columns"]={2};
    CHECK(store.set_arrangement("arranged",a.dump())); CHECK(read(backup)==previous);
    CHECK(store.find("arranged")->effective().layout->columns[0]==2);
    Store reload(dir); CHECK(reload.load()); CHECK(reload.find("arranged")->arranged);
    CHECK(reload.find("arranged")->enabled && reload.find("arranged")->links_allowed);
    CHECK(reload.arrangement_get("arranged")["state"]=="ok");
    CHECK(reload.arrangement_get("arranged")["overlay"]==a);
    const auto revision=store.find("arranged")->layout_revision;
    const auto committed=read(path); a["layout"]["columns"]={3};
#ifdef _WIN32
    // Lesen für das Backup erlaubt, Ersetzen der aktiven Datei durch den offenen Handle gesperrt.
    HANDLE handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    CHECK(handle!=INVALID_HANDLE_VALUE);
    if(handle!=INVALID_HANDLE_VALUE) {
        CHECK(!store.set_arrangement("arranged",a.dump()));
        CHECK(store.find("arranged")->layout_revision==revision);
        CHECK(store.find("arranged")->effective().layout->columns[0]==2);
        CHECK(read(path)==committed); CHECK(read(backup)==committed);
        CloseHandle(handle);
    }
#endif
    // Backupfehler ist ebenfalls vor dem Commit zu behandeln.
    fs::remove(backup); fs::create_directory(backup);
    CHECK(!store.set_arrangement("arranged",a.dump())); CHECK(read(path)==committed);
    CHECK(store.find("arranged")->layout_revision==revision);
    CHECK(!store.clear_arrangement("arranged"));CHECK(store.find("arranged")->arranged);
    fs::remove(backup);
    CHECK(!store.set_arrangement("missing",a.dump())); CHECK(!store.clear_arrangement("missing"));
    auto bad=a;bad["manifest_sha256"]=std::string(64,'f');CHECK(!store.set_arrangement("arranged",bad.dump()));
    bad=a;bad["blocks"][0]["text"]="injected";CHECK(!store.set_arrangement("arranged",bad.dump()));
    CHECK(!store.set_arrangement("arranged",std::string(arrangement_limit+1,' ')));
    CHECK(store.clear_arrangement("arranged"));CHECK(!fs::exists(path) && !fs::exists(backup));
    CHECK(!store.find("arranged")->arranged && store.find("arranged")->arrangement_state=="none");
    CHECK(store.find("arranged")->generation==before.generation);
    CHECK(store.clear_arrangement("arranged"));
    CHECK(store.set_arrangement("arranged",a.dump()));
    // Update ersetzt nur die Basis; das alte Overlay bleibt sichtbar als stale liegen.
    j["version"]="1.0.1";CHECK(store.import_manifest(j.dump()));
    CHECK(store.find("arranged")->manifest.version=="1.0.1");
    CHECK(!store.find("arranged")->arranged && store.arrangement_get("arranged")["state"]=="stale");
    CHECK(fs::exists(path)); CHECK(!store.set_arrangement("arranged",a.dump()));
    CHECK(reload.load()); CHECK(reload.arrangement_get("arranged")["state"]=="stale");
    CHECK(reload.find("arranged")->effective().version=="1.0.1");
    write(path,"{broken"); CHECK(!reload.load()); CHECK(reload.find("arranged"));
    CHECK(reload.arrangement_get("arranged")["state"]=="invalid");CHECK(!reload.find("arranged")->arranged);
    write(path,std::string(arrangement_limit+1,' '));CHECK(!reload.load());CHECK(reload.find("arranged"));
    CHECK(reload.clear_arrangement("arranged")); CHECK(reload.load());
    auto fresh=reload.arrangement_get("arranged")["overlay"];
    CHECK(reload.set_arrangement("arranged",fresh.dump()));CHECK(reload.set_arrangement("arranged",fresh.dump()));
    CHECK(fs::exists(backup));CHECK(reload.remove("arranged"));CHECK(!fs::exists(path) && !fs::exists(backup));
    CHECK(store.import_manifest(manifest("legacy")));auto legacy=store.arrangement_get("legacy")["overlay"];
    CHECK(!store.set_arrangement("legacy",legacy.dump()));
    // 32 Plugins mit je sechs Dateien bleiben unter der Scan-Grenze von 256.
    Store matrix(root/"arrangement-matrix"); CHECK(matrix.load());
    for(int i=0;i<32;++i) {
        j["id"]="matrix-"+std::to_string(i); auto id=j["id"].get<std::string>();
        CHECK(matrix.import_manifest(j.dump(),true));
        auto overlay=matrix.arrangement_get(id)["overlay"];
        CHECK(matrix.set_arrangement(id,overlay.dump()));CHECK(matrix.set_arrangement(id,overlay.dump()));
    }
    CHECK(matrix.load());CHECK(matrix.entries().size()==32);
    for(const auto& e:matrix.entries())CHECK(e.arranged && e.arrangement_state=="ok");
    for(int i=0;i<65;++i)write(root/"arrangement-matrix"/("extra-"+std::to_string(i)),"");
    CHECK(!matrix.load());
    // Hardlinks: atomarer Austausch darf niemals den verlinkten Zielinhalt ändern.
    Store safety(root/"arrangement-safety");CHECK(safety.load());
    j["id"]="safe";CHECK(safety.import_manifest(j.dump()));auto safe=safety.arrangement_get("safe")["overlay"];
    const auto safe_path=root/"arrangement-safety/safe.layout.json";
    const auto outside=root/"arrangement-target";write(outside,"outside");
    std::error_code ec;fs::create_hard_link(outside,safe_path,ec);CHECK(!ec);
    if(!ec) {CHECK(safety.set_arrangement("safe",safe.dump()));CHECK(read(outside)=="outside");CHECK(safety.clear_arrangement("safe"));}
    fs::create_symlink(outside,safe_path,ec);
    if(!ec) {CHECK(!safety.set_arrangement("safe",safe.dump()));CHECK(!safety.clear_arrangement("safe"));CHECK(!safety.remove("safe"));CHECK(read(outside)=="outside");fs::remove(safe_path);}
    else std::printf("SKIP arrangement symlink: unavailable.\n");
    const auto safe_backup=root/"arrangement-safety/safe.layout.json.bak";
    CHECK(safety.set_arrangement("safe",safe.dump()));
    fs::create_hard_link(outside,safe_backup,ec); CHECK(!ec);
    if(!ec) {CHECK(safety.set_arrangement("safe",safe.dump()));CHECK(read(outside)=="outside");fs::remove(safe_backup);}
    fs::create_symlink(outside,safe_backup,ec);
    if(!ec) {CHECK(!safety.set_arrangement("safe",safe.dump()));CHECK(!safety.clear_arrangement("safe"));CHECK(read(outside)=="outside");fs::remove(safe_backup);}
    else std::printf("SKIP arrangement backup symlink: unavailable.\n");
    // Mehrere Aufrufer werden durch denselben Store serialisiert; Backup bleibt ein gültiger Vorgänger.
    std::array<int,4> results{}; std::vector<std::thread> threads;
    auto start=safety.find("safe")->layout_revision;
    for(int i=0;i<4;++i) threads.emplace_back([&,i] {
        auto overlay=safe;overlay["layout"]["columns"]={i+1};
        results[i]=bool(safety.set_arrangement("safe",overlay.dump()));
    });
    for(auto& t:threads)t.join();for(auto ok:results)CHECK(ok);
    CHECK(safety.find("safe")->layout_revision==start+4);
    CHECK(parse_arrangement(read(safe_path)));CHECK(parse_arrangement(read(safe_backup)));
    CHECK(read(outside)=="outside");
    // Keine exklusiven temporären Schreibdateien bleiben zurück.
    for(const auto& f:fs::directory_iterator(dir))CHECK(f.path().filename().string().find(".tmp-")==std::string::npos);
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const auto root = fs::temp_directory_path() /
        ("fl-plugin-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 2;
    {
        Store links(root/"link-grants");CHECK(links.load());
        auto j=Json::parse(manifest("link-test"));j["schema_version"]=2;j["widgets"][0]["id"]="clock";
        j["links"]={"news.example.org"};j["sources"]=Json::array({{{"id","feed"},{"url","https://api.example.org/data"}}});
        CHECK(links.import_manifest(j.dump()));CHECK(!links.find("link-test")->links_allowed);
        CHECK(links.set_host_consent("link-test","api.example.org",true));CHECK(links.set_enabled("link-test",true));
        auto generation=links.find("link-test")->generation;
        CHECK(links.set_link_consent("link-test",true));CHECK(links.find("link-test")->generation>generation);
        Store reload_links(root/"link-grants");CHECK(reload_links.load());CHECK(reload_links.find("link-test")->links_allowed);
        j["links"]={"other.example.org"};CHECK(links.import_manifest(j.dump()));
        auto e=links.find("link-test");CHECK(!e->links_allowed);CHECK(e->consented_hosts.contains("api.example.org"));CHECK(e->enabled);
        Store changed_reload(root/"link-grants");CHECK(changed_reload.load());
        CHECK(!changed_reload.find("link-test")->links_allowed && changed_reload.find("link-test")->consented_hosts==e->consented_hosts);
        CHECK(links.set_link_consent("link-test",true));CHECK(links.set_link_consent("link-test",false));
        CHECK(!links.find("link-test")->links_allowed);CHECK(!links.set_link_consent("missing",true));
    }
    for(const auto& change:{"widget","setting","tap","identical"}) {
        Store store(root/(std::string("update-")+change));CHECK(store.load());
        auto j=Json::parse(manifest("review"));j["schema_version"]=2;j["widgets"][0]["id"]="clock";
        j["links"]={"news.example.org"};j["sources"]=Json::array({{{"id","feed"},{"url","https://api.example.org/data"}}});
        CHECK(store.import_manifest(j.dump()));CHECK(store.set_host_consent("review","api.example.org",true));
        CHECK(store.set_enabled("review",true));CHECK(store.set_link_consent("review",true));
        if(std::string(change)!="identical") j["links"].push_back("other.example.org");
        if(std::string(change)=="widget") j["widgets"][0]["label"]="Changed";
        if(std::string(change)=="setting") j["settings"]=Json::array({{{"key","city"},{"type","text"},{"label","City"},{"default","Berlin"}}});
        if(std::string(change)=="tap") j["widgets"][0]["tap"]={{"action","open_link"},{"url","https://news.example.org/changed"}};
        CHECK(store.import_manifest(j.dump()));auto e=store.find("review");
        CHECK(e && !e->enabled && e->consented_hosts.empty() && !e->links_allowed);
        Store reload(root/(std::string("update-")+change));CHECK(reload.load());
        e=reload.find("review");CHECK(e && !e->enabled && e->consented_hosts.empty() && !e->links_allowed);
    }
    {
        auto dir=root/"broken-link-grants";Store store(dir);CHECK(store.load());
        auto j=Json::parse(manifest());j["schema_version"]=2;j["widgets"][0]["id"]="clock";j["links"]={"news.example.org"};
        CHECK(store.import_manifest(j.dump()));CHECK(store.set_link_consent("clock",true));
        write(dir/"clock.links.json","{broken");
        Store broken(dir);CHECK(!broken.load());CHECK(broken.entries().empty());
        fs::remove(dir/"clock.links.json");fs::create_directory(dir/"clock.links.json");
        CHECK(!store.set_link_consent("clock",false));CHECK(!store.find("clock")->links_allowed);
        CHECK(!store.set_link_consent("clock",true));CHECK(!store.find("clock")->links_allowed);
        CHECK(!store.import_manifest(j.dump()));CHECK(!store.find("clock")->enabled);
    }
    // Cleanup only the exact newly created test directory; never any caller path.
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    test_arrangements(root);
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
