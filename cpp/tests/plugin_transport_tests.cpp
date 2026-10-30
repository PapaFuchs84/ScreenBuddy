// Isolated transport policy/framing/cache tests. No external network or hardware.
#include "plugin_transport.h"
#include <chrono>
#include <cstdio>

using namespace fl::plugins;
namespace fs = std::filesystem;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static Json definition() {
    return {{"schema_version", 1}, {"id", "network"}, {"name", "Network"}, {"version", "1.0.0"},
        {"refresh_seconds", 15},
        {"settings", Json::array({{{"key", "account"}, {"label", "Account"}, {"type", "text"}}})},
        {"sources", Json::array({{{"id", "one"}, {"url", "https://api.example.com/data"}},
            {{"id", "two"}, {"url", "https://other.example.com/data"}}})},
        {"widgets", Json::array({{{"type", "text"}, {"text", "Values"}}})}};
}
static std::string response(const std::string& body, const std::string& headers = "") {
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n" + headers + "\r\n" + body;
}
int main() {
#ifdef _WIN32
    CHECK(dns_completion_contract_for_test()); // numeric localhost: no external DNS or network
#endif
    for (const auto* ip : {"8.8.8.8", "1.1.1.1", "93.184.216.34", "2001:4860:4860::8888", "2606:4700:4700::1111"})
        CHECK(public_source_ip(ip));
    for (const auto* ip : {"0.0.0.0", "0.1.2.3", "10.2.3.4", "100.64.0.1", "100.127.255.255", "127.0.0.1",
        "169.254.169.254", "172.16.0.1", "172.31.255.255", "192.0.0.8", "192.0.2.1", "192.88.99.1", "192.168.1.1",
        "198.18.0.1", "198.19.255.255", "198.51.100.1", "203.0.113.1", "224.0.0.1", "255.255.255.255",
        "::", "::1", "::ffff:127.0.0.1", "::ffff:8.8.8.8", "64:ff9b::a00:1", "2002:0808:0808::1",
        "2001::1", "2001:db8::1", "3fff::1", "fc00::1", "fe80::1", "ff02::1", "bad", "8.8.8.8%1"})
        CHECK(!public_source_ip(ip));
    CHECK(!public_source_ip(std::string("8.8.8.8\0hidden", 14)));
    CHECK(public_source_ip("100.63.255.255"));
    CHECK(public_source_ip("100.128.0.1"));
    CHECK(public_source_ip("172.32.0.1"));
    CHECK(decode_source_http(response("{\"n\":1}", "Content-Length: 7\r\n")));
    CHECK(decode_source_http(response("{}")));
    {   // Raw bytes for signed store documents: exact bytes, any content type, bounded
        auto raw = decode_http_body("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nab\ncd", 100, false);
        CHECK(raw && *raw.data == "ab\ncd");
        CHECK(!decode_http_body("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n{}", 100, true)); // JSON mode still strict
        CHECK(!decode_http_body("HTTP/1.1 404 Not Found\r\nContent-Length: 2\r\n\r\n{}", 100, false));
        CHECK(!decode_http_body("HTTP/1.1 200 OK\r\n\r\n" + std::string(101, 'x'), 100, false)); // over limit
        auto chunked = decode_http_body("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n", 100, false);
        CHECK(chunked && *chunked.data == "abc");
        CHECK(!decode_http_body("HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nabc", 100, false)); // truncated
    }
    CHECK(decode_source_http("HTTP/1.1 200 OK\r\nContent-Type: application/problem+json; charset=utf-8\r\n\r\n{}"));
    CHECK(decode_source_http(response("2\r\n{}\r\n0\r\n\r\n", "Transfer-Encoding: chunked\r\n")));
    for (const auto* headers : {"Content-Length: 1\r\n", "Content-Length: 2\r\nContent-Length: 2\r\n",
        "Content-Length: 2\r\nTransfer-Encoding: chunked\r\n", "Content-Encoding: gzip\r\n", "Transfer-Encoding: gzip\r\n",
        "Content-Length: -1\r\n", "Content-Length: 18446744073709551616\r\n", " Content-Length: 2\r\n", "Bad Header: x\r\n"})
        CHECK(!decode_source_http(response("{}", headers)));
    CHECK(!decode_source_http("HTTP/1.1 302 Found\r\nLocation: https://internal.local/\r\nContent-Type: application/json\r\n\r\n{}"));
    CHECK(!decode_source_http("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n{}"));
    CHECK(!decode_source_http(response("[]")));
    CHECK(!decode_source_http(response("{\"x\":1,\"x\":2}")));
    for (const auto* chunks : {"2\r\n{}\r\n", "2\r\n{}\r\n0\r\nX: 1\r\n\r\n", "2;ext=1\r\n{}\r\n0\r\n\r\n",
        "FFFFFFFFFFFFFFFF\r\nx", "3\r\n{}\r\n0\r\n\r\n", "2\r\n{}\r\n0\r\n\r\nextra"})
        CHECK(!decode_source_http(response(chunks, "Transfer-Encoding: chunked\r\n")));
    CHECK(!decode_source_http(response(std::string(response_limit + 1, ' '))));
    CHECK(!decode_source_http(response("{}", "X-Large: " + std::string(16384, 'x') + "\r\n")));
    CHECK(!fetch_public_json({"api.example.com", "https://api.example.com/", {}}, [] { return false; }));
    for (const auto* url : {"http://api.example.com/", "https://other.example.com/", "https://api.example.com:444/", "https://api.example.com/\r\nx"})
        CHECK(!fetch_public_json({"api.example.com", url, {}}, [] { return true; }));
    CHECK(!fetch_public_json({"api.example.com", "https://api.example.com/", {{"Host", "evil.example.com"}}}, [] { return true; }));
    CHECK(!fetch_public_json({"8.8.8.8", "https://8.8.8.8/", {}}, [] { return true; }));

    const auto root = fs::temp_directory_path() / ("fl-transport-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 2;
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path, ec); } } cleanup{root};
    Store store(root / "plugins");
    CHECK(store.load());
    const auto imported = store.import_manifest(definition().dump());
    CHECK(imported);
    if (!imported) { std::printf("Import failed: %s\n", imported.error.c_str()); return 1; }
    auto now = Transport::Clock::time_point{};
    int calls = 0;
    bool fail = false, revoke = false;
    Transport transport([&](const Request&, const Authorization& authorized) -> DataResult {
        CHECK(authorized()); ++calls;
        if (revoke && calls % 2 == 0) CHECK(store.set_host_consent("network", "api.example.com", false));
        return fail ? DataResult{std::nullopt, "Fixed failure"} : DataResult{Json{{"value", calls}}, {}};
    }, [&] { return now; });
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 0);
    CHECK(store.set_host_consent("network", "api.example.com", true));
    CHECK(!store.set_enabled("network", true));
    CHECK(store.set_host_consent("network", "other.example.com", true));
    CHECK(store.set_enabled("network", true));
    CHECK(transport.collect(store, "network").size() == 2); CHECK(calls == 2);
    CHECK(transport.collect(store, "network").size() == 2); CHECK(calls == 2);
    now += std::chrono::seconds(14);
    CHECK(transport.collect(store, "network").size() == 2); CHECK(calls == 2);
    now += std::chrono::seconds(1); fail = true;
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 4);
    now += std::chrono::seconds(29);
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 4);
    now += std::chrono::seconds(1); fail = false;
    CHECK(transport.collect(store, "network").size() == 2); CHECK(calls == 6);
    CHECK(store.set_enabled("network", false));
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 6);
    CHECK(store.set_enabled("network", true)); revoke = true;
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 8);
    CHECK(!store.find("network")->enabled);
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 8);
    CHECK(store.set_host_consent("network", "api.example.com", true));
    CHECK(store.set_enabled("network", true)); revoke = false;
    CHECK(transport.collect(store, "network").size() == 2); CHECK(calls == 10);
    CHECK(transport.collect(store, "network", {{"account", "changed"}}).size() == 2); CHECK(calls == 12);
    CHECK(transport.collect(store, "network", {{"account", "changed"}}, [] { return false; }).empty()); CHECK(calls == 12);
    bool revision_current = true;
    int guarded_calls = 0;
    Transport guarded([&](const Request&, const Authorization& authorized) -> DataResult {
        CHECK(authorized());
        if (++guarded_calls == 2) revision_current = false;
        return {Json{{"value", 1}}, {}};
    }, [&] { return now; });
    CHECK(guarded.collect(store, "network", Json::object(), [&] { return revision_current; }).empty());
    CHECK(guarded_calls == 2); // a settings change during source two discards source one as well
    CHECK(store.import_manifest(definition().dump()));
    CHECK(transport.collect(store, "network").empty()); CHECK(calls == 12);
    CHECK(store.remove("network"));
    CHECK(transport.collect(store, "network").empty());
    std::printf("plugin transport: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
