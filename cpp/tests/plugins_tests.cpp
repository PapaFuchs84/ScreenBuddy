// © 2026 FuchsLabs – pure plugin tests: no files, HTTP, Windows API or hardware.
// Standalone: c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp tests/plugins_tests.cpp
#include "plugins.h"
#include <cmath>
#include <cstdio>
#include <functional>

using namespace fl::plugins;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static Json minimal() {
    return {{"schema_version", 1}, {"id", "offline-clock"}, {"name", "Uhr"}, {"version", "1.0.0"},
        {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}};
}
static Json fixture() {
    auto j = minimal();
    j["settings"] = Json::array({
        {{"key", "city"}, {"label", "Stadt"}, {"type", "text"}, {"default", "Berlin"}},
        {{"key", "token"}, {"label", "Token"}, {"type", "secret"}, {"required", true}},
        {{"key", "limit"}, {"label", "Limit"}, {"type", "number"}, {"default", 12}}
    });
    j["sources"] = Json::array();
    j["sources"].push_back({
        {"id", "feed"}, {"url", "https://API.Example.com:443/v1/{{settings.city}}?n={{settings.limit}}"},
        {"headers", {{"Authorization", "Bearer {{settings.token}}"}, {"Accept", "application/json"}}},
        {"sample", {{"temperature", 21.5}, {"progress", 150}, {"object", {{"x", 1}}},
            {"entries", Json::array({{{"title", "Eins"}}, {{"title", "Zwei"}}, {{"title", "Drei"}}})}}}
    });
    j["widgets"] = Json::array({
        {{"type", "text"}, {"text", "{{settings.city}} {{time}} {{date}}"}},
        {{"type", "metric"}, {"label", "Temperatur"}, {"value", "{{feed.temperature}}"}, {"unit", "°C"}},
        {{"type", "bar"}, {"value", "{{feed.progress}}"}, {"min", 0}, {"max", 100}},
        {{"type", "list"}, {"items", "feed.entries"}, {"item_template", "{{item.title}}"}, {"max_items", 2}},
        {{"type", "text"}, {"text", "{{feed.entries.0.title}} {{feed.object}}"}},
        {{"type", "clock"}, {"format", "datetime"}}
    });
    return j;
}
static void reject(const std::function<void(Json&)>& mutation) {
    auto j = fixture(); mutation(j); CHECK(!parse_manifest(j.dump()));
}
static void test_validation() {
    auto empty = parse_manifest(minimal().dump());
    CHECK(empty);
    if (empty) {
        CHECK(empty.manifest->refresh_seconds == 60);
        CHECK(empty.manifest->sources.empty());
        CHECK(empty.manifest->definition["settings"].is_array());
    }
    CHECK(parse_manifest(fixture().dump()));
    CHECK(!parse_manifest(""));
    CHECK(!parse_manifest("```json\n{}\n```"));
    CHECK(!parse_manifest("{\"schema_version\":1,\"schema_version\":1}"));
    CHECK(!parse_manifest(std::string(manifest_limit + 1, ' ')));
    CHECK(!parse_manifest(std::string(33, '[') + "0" + std::string(33, ']')));
    reject([](auto& j) { j["schema_version"] = 2; });
    reject([](auto& j) { j["schema_version"] = 1.0; });
    reject([](auto& j) { j["refresh_seconds"] = 14; });
    reject([](auto& j) { j["refresh_seconds"] = 3601; });
    reject([](auto& j) { j["refresh_seconds"] = 18446744073709551615ULL; });
    reject([](auto& j) { j["id"] = "Upper"; });
    reject([](auto& j) { j["id"] = "two--dashes"; });
    reject([](auto& j) { j["id"] = "end-"; });
    reject([](auto& j) { j["version"] = "1.2.3-beta"; });
    reject([](auto& j) { j["version"] = "10000.2.3"; });
    reject([](auto& j) { j["name"] = ""; });
    reject([](auto& j) { j["description"] = std::string(501, 'a'); });
    reject([](auto& j) { j["script"] = "run"; });
    reject([](auto& j) { j["settings"].push_back(j["settings"][0]); });
    reject([](auto& j) { j["settings"][0]["key"] = "bad-key"; });
    reject([](auto& j) { j["settings"][0]["required"] = 1; });
    reject([](auto& j) { j["settings"][0]["default"] = 5; });
    reject([](auto& j) { j["settings"][2]["default"] = "5"; });
    reject([](auto& j) { j["settings"][1]["default"] = "DO-NOT-EXPOSE"; });
    reject([](auto& j) { j["settings"][0]["other"] = true; });
    reject([](auto& j) { j["sources"].push_back(j["sources"][0]); });
    reject([](auto& j) { j["sources"][0]["id"] = "settings"; });
    reject([](auto& j) { j["sources"][0]["sample"] = Json::array(); });
    reject([](auto& j) { j["sources"][0]["format"] = "xml"; });
    reject([](auto& j) { j["sources"][0]["headers"]["Host"] = "elsewhere.com"; });
    reject([](auto& j) { j["sources"][0]["headers"]["accept"] = "duplicate"; });
    reject([](auto& j) { j["sources"][0]["headers"]["X-Bad"] = "ok\r\nInjected: value"; });
    reject([](auto& j) { j["sources"][0]["headers"]["X-Bad"] = std::string("nul\0value", 9); });
    for (const char* url : {"http://api.example.com/feed", "https://user@api.example.com/feed", "https://127.0.0.1/",
        "https://[::1]/", "https://localhost/", "https://machine.local/", "https://home.arpa/",
        "https://api.example.com:444/feed", "https://{{settings.city}}.example.com/feed", "https://example.com/#fragment",
        "https://example.com\\@evil.example/x", "https://example.com/{{feed.value}}", "https://example.com/{{time}}",
        "https://example.com/{{settings.unknown}}", "https://example.com /feed", "https://example..com/"}) {
        reject([&](auto& j) { j["sources"][0]["url"] = url; });
    }
    reject([](auto& j) { j["widgets"] = Json::array(); });
    reject([](auto& j) { j["widgets"].push_back(j["widgets"][0]); });
    reject([](auto& j) { j["widgets"][0]["type"] = "html"; });
    reject([](auto& j) { j["widgets"][0]["format"] = "time"; });
    reject([](auto& j) { j["widgets"][0].erase("text"); });
    reject([](auto& j) { j["widgets"][2]["max"] = 0; });
    reject([](auto& j) { j["widgets"][3]["max_items"] = 6; });
    reject([](auto& j) { j["widgets"][3]["items"] = "settings.city"; });
    reject([](auto& j) { j["widgets"][3]["items"] = "feed"; });
    reject([](auto& j) { j["widgets"][5]["format"] = "custom"; });
    for (const char* text : {"{{settings.token}}", "{{settings.unknown}}", "{{source.feed.temperature}}",
        "{{item.title}}", "{{feed.a-b}}", "{{feed.entries[0]}}", "{{feed..value}}", "{{feed.x + 1}}",
        "{{feed.temperature}", "unmatched }}", "{{{{time}}}}", "{{time.value}}"}) {
        reject([&](auto& j) { j["widgets"][0]["text"] = text; });
    }
    reject([](auto& j) { j["widgets"][1]["unit"] = "{{settings.token}}"; });
    reject([](auto& j) { j["widgets"][1]["label"] = "{{settings.token}}"; });
    reject([](auto& j) { j["widgets"][3]["item_template"] = "{{settings.token}}"; });
    auto unicode = minimal(); unicode["name"] = std::string();
    for (int i = 0; i < 80; ++i) unicode["name"].get_ref<std::string&>() += "ä";
    CHECK(parse_manifest(unicode.dump()));
    unicode["name"].get_ref<std::string&>() += "ä";
    CHECK(!parse_manifest(unicode.dump()));
    auto secret_error = fixture(); secret_error["widgets"][0]["text"] = "{{DO_NOT_EXPOSE}}";
    auto result = parse_manifest(secret_error.dump());
    CHECK(!result && result.error.find("DO_NOT_EXPOSE") == std::string::npos);
}
static void test_render() {
    auto parsed = parse_manifest(fixture().dump());
    CHECK(parsed); if (!parsed) return;
    const auto& m = *parsed.manifest;
    RenderContext ctx{sample_data(m), {{"token", "PRIVATE"}}, "12:34", "24.10.2026"};
    auto page = render_page(m, ctx);
    CHECK(!page.missing_data);
    CHECK(page.widgets.size() == 6);
    CHECK(page.widgets[0].text == "Berlin 12:34 24.10.2026");
    CHECK(page.widgets[1].text == "21.5");
    CHECK(page.widgets[2].bar_fraction == 1.0);
    CHECK(page.widgets[3].items == std::vector<std::string>({"Eins", "Zwei"}));
    CHECK(page.widgets[4].text == "Eins [Daten]");
    CHECK(page.widgets[5].text == "24.10.2026 12:34");
    CHECK(m.definition.dump().find("PRIVATE") == std::string::npos);
    ctx.sources["feed"]["progress"] = -5;
    CHECK(render_page(m, ctx).widgets[2].bar_fraction == 0.0);
    ctx.sources["feed"]["progress"] = 25;
    CHECK(render_page(m, ctx).widgets[2].bar_fraction == 0.25);
    for (const char* bad : {"nan", "inf", "1e9999", "12oops", ""}) {
        ctx.sources["feed"]["progress"] = bad;
        auto bad_page = render_page(m, ctx);
        CHECK(!bad_page.widgets[2].bar_fraction && bad_page.missing_data);
    }
    ctx.sources.clear();
    auto missing = render_page(m, ctx);
    CHECK(missing.missing_data);
    CHECK(missing.widgets[1].text == "—"); // no automatic sample fallback
    CHECK(missing.widgets[3].items == std::vector<std::string>({"—"}));
    ctx.sources = sample_data(m);
    ctx.sources["feed"]["entries"] = Json::array();
    CHECK(render_page(m, ctx).widgets[3].items.empty());
    ctx.sources["feed"]["temperature"] = std::string(display_text_limit * 4, 'a');
    CHECK(render_page(m, ctx).widgets[1].text.size() == display_text_limit);
    ctx.sources["feed"]["temperature"] = std::string();
    for (int i = 0; i < 2000; ++i) ctx.sources["feed"]["temperature"].get_ref<std::string&>() += "€";
    auto utf8 = render_page(m, ctx).widgets[1].text;
    CHECK(utf8.size() <= display_text_limit && utf8.size() % 3 == 0);
    CHECK(Json::accept(Json(utf8).dump()));
    // Defense in depth: the display context never retains secret settings.
    auto modified = m; modified.widgets[0].text = "{{settings.token}}";
    CHECK(render_page(modified, ctx).widgets[0].text == "—");
    auto enormous_range = m; enormous_range.widgets[2].min = -1.0e308; enormous_range.widgets[2].max = 1.0e308;
    ctx.sources["feed"]["progress"] = 0;
    CHECK(render_page(enormous_range, ctx).widgets[2].bar_fraction == 0.5);
    auto data = parse_source_data("{\"x\":42}");
    CHECK(data && (*data.data)["x"] == 42);
    CHECK(!parse_source_data("[]"));
    CHECK(!parse_source_data("{\"x\":1,\"x\":2}"));
    CHECK(!parse_source_data(std::string(response_limit + 1, ' ')));
    CHECK(!parse_source_data("{\"x\":" + std::string(32, '[') + "0" + std::string(32, ']') + "}"));
    CHECK(parse_source_data("{\"x\":" + std::string(31, '[') + "0" + std::string(31, ']') + "}"));
    CHECK(!parse_source_data("{\"n\":1e9999}"));
}
static void test_requests() {
    auto parsed = parse_manifest(fixture().dump());
    CHECK(parsed); if (!parsed) return;
    const auto& m = *parsed.manifest;
    CHECK(m.sources[0].host == "api.example.com");
    CHECK(m.sources[0].setting_keys == std::vector<std::string>({"city", "limit", "token"}));
    CHECK(!prepare_request(m, m.sources[0], Json::object()));
    auto req = prepare_request(m, m.sources[0], {{"city", "A/B?x=1#ü"}, {"token", "PRIVATE"}});
    CHECK(req); if (!req) return;
    CHECK(req.request->url == "https://API.Example.com:443/v1/A%2FB%3Fx%3D1%23%C3%BC?n=12");
    CHECK(req.request->headers["Authorization"] == "Bearer PRIVATE");
    CHECK(m.definition.dump().find("PRIVATE") == std::string::npos);
    auto injected = prepare_request(m, m.sources[0], {{"token", "PRIVATE\r\nHost: evil"}});
    CHECK(!injected && injected.error.find("PRIVATE") == std::string::npos);
    CHECK(!prepare_request(m, m.sources[0], {{"token", "ok"}, {"limit", "12"}}));
    CHECK(!prepare_request(m, m.sources[0], {{"token", std::string(manifest_limit + 1, 'a')}}));
    auto giant = fixture(); giant["sources"][0]["headers"]["X-Large"] = "{{settings.city}}" + std::string(120000, 'x');
    // Manifest itself exceeds its limit once combined with a large literal header.
    auto p = parse_manifest(giant.dump());
    if (p) CHECK(!prepare_request(*p.manifest, p.manifest->sources[0], {{"token", "ok"}, {"city", std::string(20000, 'a')}}));
    auto secret_url = fixture(); secret_url["sources"][0]["url"] = "https://api.example.com/?key={{settings.token}}";
    auto s = parse_manifest(secret_url.dump()); CHECK(s);
    if (s) {
        auto q = prepare_request(*s.manifest, s.manifest->sources[0], {{"token", "a&b=c"}});
        CHECK(q && q.request->url == "https://api.example.com/?key=a%26b%3Dc");
    }
}
int main() {
    test_validation(); test_render(); test_requests();
    std::printf("plugins: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
