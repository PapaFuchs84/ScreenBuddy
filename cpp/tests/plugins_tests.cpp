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
    reject([](auto& j) { j["settings"][0]["default"] = "line\nbreak"; });
    reject([](auto& j) { j["settings"][0]["default"] = std::string(setting_value_limit + 1, 'x'); });
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
    ctx.settings["city"] = "Hamburg";
    CHECK(render_page(m, ctx).widgets[0].text == "Hamburg 12:34 24.10.2026");
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
static void test_security_hardening() {
    // Windows device names are never valid plugin IDs.
    for (const char* id : {"con", "prn", "aux", "nul", "com1", "com9", "lpt1", "lpt9"}) {
        CHECK(reserved_device_name(id));
        reject([&](auto& j) { j["id"] = id; });
    }
    for (const char* id : {"CON", "Nul", "COM3"}) CHECK(reserved_device_name(id));
    for (const char* id : {"com0", "com10", "console", "my-con", "lpt", "offline-clock"}) CHECK(!reserved_device_name(id));
    reject([](auto& j) { j["id"] = "com5"; });
    auto ok_id = fixture(); ok_id["id"] = "console"; CHECK(parse_manifest(ok_id.dump()));

    // Bidi / zero-width / control characters in display strings.
    for (const char* bad : {"\xE2\x80\xAE" "evil", "a\xE2\x80\x8B" "b", "a\xE2\x80\x8F" "b", "\xE2\x80\xAA" "x", "x\xE2\x81\xA6",
                            "x\xE2\x81\xA9", "x\xEF\xBB\xBF", "x\xE2\x80\xA8", "tab\there", "line\nbreak"}) {
        reject([&](auto& j) { j["name"] = bad; });
        reject([&](auto& j) { j["description"] = bad; });
        reject([&](auto& j) { j["settings"][0]["label"] = bad; });
        reject([&](auto& j) { j["widgets"][1]["label"] = bad; });
        reject([&](auto& j) { j["widgets"][1]["unit"] = bad; });
    }
    auto readable = fixture(); readable["name"] = "Wetter \xC3\xBC \xE2\x82\xAC \xE2\x80\x93 \xF0\x9F\x8C\xA4"; readable["widgets"][1]["unit"] = "\xC2\xB0" "C";
    CHECK(parse_manifest(readable.dump()));

    // "." and ".." must never become URL dot segments.
    auto parsed = parse_manifest(fixture().dump());
    CHECK(parsed); if (!parsed) return;
    const auto& m = *parsed.manifest;
    CHECK(!prepare_request(m, m.sources[0], {{"city", "."}, {"token", "t"}}));
    CHECK(!prepare_request(m, m.sources[0], {{"city", ".."}, {"token", "t"}}));
    CHECK(prepare_request(m, m.sources[0], {{"city", "..."}, {"token", "t"}}));
    CHECK(prepare_request(m, m.sources[0], {{"city", "a.b"}, {"token", "t"}}));
    auto traversal = prepare_request(m, m.sources[0], {{"city", "../x"}, {"token", "t"}});
    CHECK(traversal && traversal.request->url.find("/../") == std::string::npos);

    // Source fingerprint: changes with host, URL, header, but not with unrelated fields.
    const auto base = source_fingerprint(m);
    CHECK(base == source_fingerprint(*parse_manifest(fixture().dump()).manifest));
    auto other = [&](const std::function<void(Json&)>& mutation) {
        auto j = fixture(); mutation(j); auto r = parse_manifest(j.dump()); return r ? source_fingerprint(*r.manifest) : std::string("<invalid>");
    };
    CHECK(other([](auto& j) { j["sources"][0]["url"] = "https://evil.example.org/v1/{{settings.city}}?n={{settings.limit}}"; }) != base);
    CHECK(other([](auto& j) { j["sources"][0]["url"] = "https://API.Example.com:443/v2/{{settings.city}}?n={{settings.limit}}"; }) != base);
    CHECK(other([](auto& j) { j["sources"][0]["headers"]["X-Extra"] = "{{settings.token}}"; }) != base);
    CHECK(other([](auto& j) { j["sources"][0]["headers"]["Authorization"] = "Bearer x{{settings.token}}"; }) != base);
    CHECK(other([](auto& j) { j["sources"].erase(0); j["widgets"] = Json::array({{{"type", "clock"}, {"format", "time"}}}); }) != base);
    CHECK(other([](auto& j) { j["version"] = "2.0.0"; j["name"] = "Neu"; j["widgets"][0]["text"] = "x"; }) == base);
}
static Json store_ready() {
    auto j = minimal();
    j["description"] = "Einfache Uhr / Simple clock";
    j["license"] = "MIT"; j["author"] = "FuchsLabs"; j["category"] = "system"; j["min_app_version"] = "2.5.0";
    return j;
}
static void test_store_metadata() {
    // Without metadata: valid locally, rejected by store rules.
    auto plain = parse_manifest(minimal().dump());
    CHECK(plain && plain.manifest->license.empty() && plain.manifest->author.empty());
    CHECK(plain && store_problems(*plain.manifest).size() == 5);
    CHECK(plain && app_compatible(*plain.manifest, "1.0.0"));
    {   // store description counts characters, not bytes
        auto j = store_ready(); j["description"] = "äöüäöüäöü"; // 9 characters, 18 bytes
        auto p = parse_manifest(j.dump());
        CHECK(p && store_problems(*p.manifest).size() == 1);
        j["description"] = "äöüäöüäöüä";
        p = parse_manifest(j.dump());
        CHECK(p && store_problems(*p.manifest).empty());
    }

    auto ok = parse_manifest(store_ready().dump());
    CHECK(ok);
    if (ok) {
        CHECK(ok.manifest->license == "MIT" && ok.manifest->author == "FuchsLabs");
        CHECK(ok.manifest->category == "system" && ok.manifest->min_app_version == "2.5.0");
        CHECK(ok.manifest->definition["license"] == "MIT"); // survives export
        CHECK(store_problems(*ok.manifest).empty());
        CHECK(app_compatible(*ok.manifest, "2.5.0"));
        CHECK(app_compatible(*ok.manifest, "2.10.0")); // numeric, not textual comparison
        CHECK(app_compatible(*ok.manifest, "3.0.0"));
        CHECK(!app_compatible(*ok.manifest, "2.4.9"));
        CHECK(!app_compatible(*ok.manifest, "1.99.99"));
        CHECK(app_compatible(*ok.manifest, "garbage")); // unparsable app version never locks plugins out
    }
    auto bad = [](const std::function<void(Json&)>& mutation) {
        auto j = store_ready(); mutation(j); return !parse_manifest(j.dump());
    };
    CHECK(parse_manifest([] { auto j = store_ready(); j["license"] = "CC0-1.0"; return j.dump(); }()));
    CHECK(bad([](auto& j) { j["license"] = "GPL-3.0"; }));
    CHECK(bad([](auto& j) { j["license"] = "mit"; }));
    CHECK(bad([](auto& j) { j["license"] = 1; }));
    CHECK(bad([](auto& j) { j["category"] = "malware"; }));
    CHECK(bad([](auto& j) { j["author"] = ""; }));
    CHECK(bad([](auto& j) { j["author"] = "Evil\xE2\x80\xAE" "Name"; })); // bidi control
    CHECK(bad([](auto& j) { j["author"] = std::string(81, 'a'); }));
    CHECK(bad([](auto& j) { j["min_app_version"] = "2.5"; }));
    CHECK(bad([](auto& j) { j["min_app_version"] = "2.5.0-beta"; }));

    // Store rules for source plugins.
    auto weather = fixture();
    weather["description"] = "Wetter mit Beispieldaten";
    weather["license"] = "MIT"; weather["author"] = "A"; weather["category"] = "info"; weather["min_app_version"] = "2.5.0";
    weather["refresh_seconds"] = 30;
    auto fast = parse_manifest(weather.dump());
    CHECK(fast && store_problems(*fast.manifest).size() == 1);
    weather["refresh_seconds"] = 60;
    weather["sources"][0]["sample"] = Json::object();
    auto nosample = parse_manifest(weather.dump());
    CHECK(nosample && store_problems(*nosample.manifest).size() == 1);
}
static void test_cycle() {
    Json j={{"schema_version",2},{"id","cycle-test"},{"name","Cycle"},{"version","1.0.0"}};
    j["layout"]={{"columns",{1}},{"rows",{1}},{"zones",Json::array({{{"id","a"},{"column",0},{"row",0},{"cycle",{{"mode","rotate"}}}}})}};
    j["widgets"]=Json::array({{{"id","one"},{"zone","a"},{"type","text"},{"text","{{time}}"},{"seconds",3}}});
    auto parse=[&]{return parse_manifest(j.dump());}; CHECK(parse());
    j["widgets"][0]["tap"]={{"action","next"}}; CHECK(parse());
    j["widgets"][0].erase("tap");
    j["description"]="Cycling zone example";j["license"]="MIT";j["author"]="Example";j["category"]="info";j["min_app_version"]="2.10.0";
    CHECK(store_problems(*parse().manifest).empty());
    j["min_app_version"]="2.9.9";CHECK(!store_problems(*parse().manifest).empty());
    j["min_app_version"]="2.10.0";
    for (auto invalid : {Json(2),Json(601),Json(3.5),Json("8")}) { j["widgets"][0]["seconds"]=invalid;CHECK(!parse()); }
    j["widgets"][0]["seconds"]=600;CHECK(parse());
    auto& c=j["layout"]["zones"][0]["cycle"];
    c["unknown"]=true;CHECK(!parse());c.erase("unknown");
    c["mode"]="first_available"; CHECK(!parse());
    j["widgets"][0].erase("seconds");CHECK(parse());
    c["transition"]="blink";CHECK(!parse());c["transition"]="none";CHECK(parse());
    c["mode"]="random";CHECK(!parse());c["mode"]="rotate";
    j["schema_version"]=1;CHECK(!parse());j["schema_version"]=2;
    auto m=parse();CHECK(m && m.manifest->widgets[0].seconds==8);
    auto saved_cycle=c; j["layout"]["zones"][0].erase("cycle");
    j["widgets"][0]["seconds"]=8;CHECK(!parse());
    j["widgets"][0].erase("seconds");j["layout"]["zones"][0]["cycle"]=saved_cycle;
    j.erase("layout");j["widgets"][0].erase("zone");j["widgets"][0]["seconds"]=8;CHECK(!parse());
    j["widgets"][0].erase("seconds");
    j["sources"]=Json::array({{{"id","feed"},{"url","https://api.example.com/test"},{"sample",{{"value",1}}}}});
    j["widgets"][0]["text"]="{{feed.value}}";
    j["widgets"].push_back({{"id","two"},{"type","text"},{"text","Available"}});
    m=parse();CHECK(m);
    if(m) { auto page=render_page(*m.manifest,{});CHECK(page.missing_data && page.widgets[0].missing && !page.widgets[1].missing); }
}
static void test_tap() {
    auto j=fixture(); j["schema_version"]=2;
    for(size_t i=0;i<j["widgets"].size();++i) j["widgets"][i]["id"]="block_"+std::to_string(i);
    j["links"]={"news.example.org"};
    j["widgets"][0]["tap"]={{"action","open_link"},{"url","https://news.example.org/a/{{settings.city}}?q={{feed.temperature}}"}};
    auto parse=[&]{return parse_manifest(j.dump());}; auto m=parse();CHECK(m);
    if(m) {
        auto fp=source_fingerprint(*m.manifest);
        RenderContext c; c.sources=sample_data(*m.manifest); c.settings={{"city","/?#%&ä"}};
        auto p=render_page(*m.manifest,c);
        CHECK(p.widgets[0].tap_url=="https://news.example.org/a/%2F%3F%23%25%26%C3%A4?q=21.5");
        for(auto dot:{".",".."}) { c.settings["city"]=dot;CHECK(render_page(*m.manifest,c).widgets[0].tap_url.empty()); }
        c.settings["city"]=std::string(2048,'/');CHECK(render_page(*m.manifest,c).widgets[0].tap_url.empty());
        c.sources.clear();CHECK(render_page(*m.manifest,c).widgets[0].tap_url.empty());
        j["links"].push_back("other.example.org");auto n=parse();CHECK(n && source_fingerprint(*n.manifest)==fp);
        CHECK(n && link_fingerprint(*n.manifest)!=link_fingerprint(*m.manifest));j["links"]={"news.example.org"};
    }
    for(auto url:{"http://news.example.org/a","https://other.example.org/a","https://{{settings.city}}/a","https://127.0.0.1/a","https://news.example.org/{{settings.token}}","https://news.example.org/{{item.title}}","https://news.example.org/#x"}) {
        j["widgets"][0]["tap"]["url"]=url;CHECK(!parse());
    }
    for(const auto& suffix: {"/a/.","/a/../b","/a/%2e", "/a/.%2E/", "/%", "/%0", "/%GG", "/a b", "/\"", "/<", "/>", "/\\", "/^", "/`", "/{", "/|", "/}", "/ä", "/{{}}", "/{{settings.city}", "/{{settings..city}}"}) {
        j["widgets"][0]["tap"]["url"]=std::string("https://news.example.org")+suffix;CHECK(!parse());
    }
    j["widgets"][0]["tap"]["url"]="https://NEWS.example.org/a";CHECK(!parse());
    j["widgets"][0]["tap"]["url"]="https://news.example.org/a-._~!$&'()*+,;=:@/b?q=%20/?x=1";CHECK(parse());
    j["widgets"][0]["tap"]={{"action","next"}};CHECK(!parse());
    j["widgets"][0].erase("tap");
    j["widgets"][3]["tap"]={{"action","open_link"},{"url","https://news.example.org/{{item.title}}"}};
    m=parse();CHECK(m);if(m) {RenderContext c;c.sources=sample_data(*m.manifest);auto p=render_page(*m.manifest,c);CHECK(p.widgets[3].item_urls==std::vector<std::string>({"https://news.example.org/Eins","https://news.example.org/Zwei"}));}
    j["links"]={"a.example.org","b.example.org","c.example.org","d.example.org","e.example.org"};CHECK(!parse());
    auto v1=minimal();v1["links"]=Json::array();CHECK(!parse_manifest(v1.dump()));v1.erase("links");v1["widgets"][0]["tap"]={{"action","next"}};CHECK(!parse_manifest(v1.dump()));
}
static void test_arrangements() {
    auto j = fixture(); j["schema_version"] = 2;
    j["widgets"].erase(j["widgets"].begin()+4, j["widgets"].end());
    j["layout"] = {{"columns", {1,1}}, {"rows", {1,1}}, {"zones", Json::array()}};
    for (int i=0; i<4; ++i) {
        auto id = "zone_" + std::to_string(i);
        j["layout"]["zones"].push_back({{"id",id},{"column",i%2},{"row",i/2}});
        j["widgets"][i]["id"] = "block_" + std::to_string(i);
        j["widgets"][i]["zone"] = id;
    }
    auto base = parse_manifest(j.dump()); CHECK(base);
    if (!base) return;
    auto a = arrangement_from(*base.manifest);
    auto merged = merge_arrangement(*base.manifest,a);
    CHECK(merged && merged.manifest->definition == base.manifest->definition);
    CHECK(to_json(a) == Json(a));
    struct Case { const char* name; std::function<void(Json&)> edit; bool valid = false; };
    const std::vector<Case> cases = {
        {"unchanged", [](auto&){}, true},
        {"missing block", [](auto& o){o["blocks"].erase(0);}},
        {"duplicate block", [](auto& o){o["blocks"][1]=o["blocks"][0];}},
        {"unknown block", [](auto& o){o["blocks"][0]["id"]="unknown";}},
        {"unknown zone", [](auto& o){o["blocks"][0]["zone"]="unknown";}},
        {"all hidden", [](auto& o){for(auto& b:o["blocks"]) b["shown"]=false;}},
        {"empty zone", [](auto& o){o["blocks"][0]["shown"]=false;}},
        {"seconds without rotate", [](auto& o){o["blocks"][0]["seconds"]=8;}},
        {"rotate seconds", [](auto& o){o["layout"]["zones"][0]["cycle"]={{"mode","rotate"}};o["blocks"][0]["seconds"]=3;},true},
        {"seconds max", [](auto& o){o["layout"]["zones"][0]["cycle"]={{"mode","rotate"}};o["blocks"][0]["seconds"]=600;},true},
        {"seconds low", [](auto& o){o["blocks"][0]["seconds"]=2;}},
        {"seconds high", [](auto& o){o["blocks"][0]["seconds"]=601;}},
        {"seconds float", [](auto& o){o["blocks"][0]["seconds"]=8.0;}},
        {"overlap", [](auto& o){o["layout"]["zones"][1]["column"]=0;}},
        {"6x6", [](auto& o){o["layout"]["columns"]={1,1,1,1,1,12};o["layout"]["rows"]={12,1,1,1,1,1};},true},
        {"7 columns", [](auto& o){o["layout"]["columns"]={1,1,1,1,1,1,1};}},
        {"7 rows", [](auto& o){o["layout"]["rows"]={1,1,1,1,1,1,1};}},
        {"zero weight", [](auto& o){o["layout"]["columns"][0]=0;}},
        {"weight 13", [](auto& o){o["layout"]["rows"][0]=13;}},
        {"bad span", [](auto& o){o["layout"]["zones"][0]["column_span"]=3;}},
        {"bad row", [](auto& o){o["layout"]["zones"][0]["row"]=-1;}},
        {"zone duplicate", [](auto& o){o["layout"]["zones"][1]["id"]="zone_0";}},
        {"secret condition", [](auto& o){o["blocks"][0]["show_if"]={{"path","settings.token"}};}},
        {"public condition", [](auto& o){o["blocks"][0]["show_if"]={{"path","settings.city"}};},true},
        {"source condition", [](auto& o){o["blocks"][0]["show_if"]={{"path","feed.temperature"},{"op",">"},{"value",0}};},true},
        {"unknown condition", [](auto& o){o["blocks"][0]["show_if"]={{"path","unknown.field"}};}},
        {"text injection", [](auto& o){o["blocks"][0]["text"]="changed";}},
        {"type injection", [](auto& o){o["blocks"][0]["type"]="clock";}},
        {"tap injection", [](auto& o){o["blocks"][0]["tap"]={{"action","next"}};}},
        {"source injection", [](auto& o){o["sources"]=Json::array();}},
        {"links injection", [](auto& o){o["links"]={"example.org"};}},
        {"settings injection", [](auto& o){o["settings"]=Json::array();}},
        {"shown missing", [](auto& o){o["blocks"][0].erase("shown");}},
        {"shown number", [](auto& o){o["blocks"][0]["shown"]=1;}},
        {"bad schema", [](auto& o){o["schema_version"]=2;}},
        {"float schema", [](auto& o){o["schema_version"]=1.0;}},
        {"bad fingerprint", [](auto& o){o["manifest_sha256"]="bad";}},
        {"cycle invalid", [](auto& o){o["layout"]["zones"][0]["cycle"]={{"mode","unknown"}};}},
        {"four per zone", [](auto& o){o["layout"]["zones"].erase(o["layout"]["zones"].begin()+1,o["layout"]["zones"].end());for(auto& b:o["blocks"])b["zone"]="zone_0";},true},
        {"reorder", [](auto& o){std::swap(o["blocks"][0],o["blocks"][3]);},true}
    };
    for (const auto& c:cases) {
        auto o=to_json(a); c.edit(o);
        auto parsed=parse_arrangement(o.dump());
        const bool ok=parsed && bool(merge_arrangement(*base.manifest,*parsed.arrangement));
        if(ok!=c.valid) std::printf("Arrangement case: %s\n",c.name);
        CHECK(ok==c.valid);
    }
    CHECK(!parse_arrangement(std::string(arrangement_limit+1,' ')));
    CHECK(!parse_arrangement("{\"schema_version\":1,\"schema_version\":1}"));
    auto serialized=to_json(a).dump();
    serialized.insert(1,"\"schema_version\":1,"); CHECK(!parse_arrangement(serialized));
    auto v1=parse_manifest(minimal().dump()); CHECK(!merge_arrangement(*v1.manifest,a));
    auto automatic=j; automatic.erase("layout");
    for(auto& w:automatic["widgets"])w.erase("zone");
    auto auto_base=parse_manifest(automatic.dump()); CHECK(auto_base);
    CHECK(merge_arrangement(*auto_base.manifest,arrangement_from(*auto_base.manifest)));
    // Gültige Höchstgrenzen: zwölf Blöcke, acht Zonen; fünfter Block in einer Zone scheitert.
    auto many=j; many["widgets"]=Json::array(); many["layout"]={{"columns",{1,1,1,1}},{"rows",{1,1}},{"zones",Json::array()}};
    for(int i=0;i<8;++i)many["layout"]["zones"].push_back({{"id","z_"+std::to_string(i)},{"column",i%4},{"row",i/4}});
    for(int i=0;i<12;++i)many["widgets"].push_back({{"id","b_"+std::to_string(i)},{"zone","z_"+std::to_string(i<8?i:0)},{"type","text"},{"text","Same"}});
    many["widgets"][11]["zone"]="z_1";
    auto maximum=parse_manifest(many.dump()); CHECK(maximum);
    if(maximum) {
        auto model=arrangement_from(*maximum.manifest); CHECK(merge_arrangement(*maximum.manifest,model));
        model.blocks[11]["zone"]="z_0"; CHECK(!merge_arrangement(*maximum.manifest,model));
        model.layout["zones"].push_back({{"id","ninth"},{"column",0},{"row",0}});CHECK(!merge_arrangement(*maximum.manifest,model));
    }
    // Ausblenden und Bedingungen werden auch dann geprüft, wenn die Zone belegt bleibt.
    auto shared=a;
    shared.layout["zones"].erase(shared.layout["zones"].begin()+1,shared.layout["zones"].end());
    for(auto& b:shared.blocks)b["zone"]="zone_0";
    shared.blocks[0]["shown"]=false;
    auto hidden=merge_arrangement(*base.manifest,shared);
    CHECK(hidden && hidden.manifest->widgets.size()==3);
    shared.blocks[0]["show_if"]={{"path","settings.token"}};CHECK(!merge_arrangement(*base.manifest,shared));
    shared.blocks[0].erase("show_if");shared.blocks[0]["seconds"]=8;CHECK(!merge_arrangement(*base.manifest,shared));
    j["links"]={"news.example.org"};
    j["widgets"][1]["tap"]={{"action","open_link"},{"url","https://news.example.org/a"}};
    j["widgets"][0]["show_if"]={{"path","settings.city"}};
    j["layout"]["zones"][0]["cycle"]={{"mode","rotate"},{"transition","none"}};
    j["widgets"][0]["seconds"]=17;
    auto protected_base=parse_manifest(j.dump());CHECK(protected_base);
    if(protected_base) {
        auto model=arrangement_from(*protected_base.manifest);
        auto unchanged=merge_arrangement(*protected_base.manifest,model);
        CHECK(unchanged && unchanged.manifest->definition==protected_base.manifest->definition);
        std::swap(model.blocks[0],model.blocks[1]);auto reordered=merge_arrangement(*protected_base.manifest,model);
        CHECK(reordered && reordered.manifest->widgets[0].tap && reordered.manifest->widgets[0].tap->url=="https://news.example.org/a");
        CHECK(reordered && source_fingerprint(*reordered.manifest)==source_fingerprint(*protected_base.manifest));
        CHECK(reordered && link_fingerprint(*reordered.manifest)==link_fingerprint(*protected_base.manifest));
    }
    j["layout"]["zones"][0]["cycle"]={{"mode","rotate"}};
    j["widgets"][0]["tap"]={{"action","next"}};
    auto next=parse_manifest(j.dump()); CHECK(next);
    if(next) {auto model=arrangement_from(*next.manifest);CHECK(merge_arrangement(*next.manifest,model));model.layout["zones"][0].erase("cycle");CHECK(!merge_arrangement(*next.manifest,model));}
}
int main() {
    test_arrangements(); test_tap(); test_cycle(); test_validation(); test_render(); test_requests(); test_security_hardening(); test_store_metadata();
    std::printf("plugins: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
