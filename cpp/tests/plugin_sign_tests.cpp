// © 2026 FuchsLabs – signing tests: keys, signatures, index, packages, revocation. Offline, no files.
// Standalone: c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp src/plugin_sign.cpp tests/plugin_sign_tests.cpp -lcrypto
#include "plugin_sign.h"
#include <cstdio>
#include <functional>

using namespace fl::plugins;
using namespace fl::plugins::sign;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

static std::string manifest(const std::string& id, const std::string& version = "1.0.0", bool weather = false) {
    Json j = {{"schema_version", 1}, {"id", id}, {"name", "Test " + id}, {"description", "Testplugin fuer die Signatur"},
        {"version", version}, {"license", "MIT"}, {"author", "FuchsLabs"}, {"category", "info"}, {"min_app_version", "2.5.0"},
        {"widgets", Json::array({{{"type", "clock"}, {"format", "time"}}})}};
    if (weather) {
        j["refresh_seconds"] = 900;
        j["sources"] = Json::array({{{"id", "w"}, {"url", "https://api.open-meteo.com/v1/forecast"}, {"sample", {{"t", 1}}}}});
        j["widgets"] = Json::array({{{"type", "metric"}, {"value", "{{w.t}}"}}});
    }
    return j.dump();
}

// Smallest byte sequence that passes the PNG header check (signature + IHDR with width/height).
static std::string fake_png(unsigned w = 480, unsigned h = 320, size_t total = 64) {
    std::string s("\x89PNG\r\n\x1a\n", 8);
    s += std::string("\0\0\0\r", 4) + "IHDR";
    for (unsigned v : {w, h}) for (int shift = 24; shift >= 0; shift -= 8) s += char(v >> shift & 255);
    s.resize(total, 'x');
    return s;
}
int main() {
    auto keys = generate_keypair();
    auto other = generate_keypair();
    CHECK(keys && other && keys->public_hex.size() == 64 && keys->public_hex != other->public_hex);
    if (!keys || !other) return 1;
    auto pub = *public_key_from_hex(keys->public_hex);
    auto pub_other = *public_key_from_hex(other->public_hex);
    CHECK(!public_key_from_hex("zz"));
    CHECK(!public_key_from_hex(std::string(64, 'A'))); // upper case refused: one canonical spelling
    CHECK(!public_key_from_hex(std::string(63, 'a')));

    // SHA-256 known answer.
    CHECK(sha256_hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // Raw sign/verify.
    const std::string doc = "{\"hello\":1}";
    auto sig = sign_document(Kind::index, keys->private_pem, doc);
    CHECK(sig && sig->size() == 88);
    if (!sig) return 1;
    CHECK(verify(Kind::index, pub, doc, *sig));
    CHECK(!verify(Kind::revoked, pub, doc, *sig));            // domain separation
    CHECK(!verify(Kind::index, pub_other, doc, *sig));         // wrong key
    CHECK(!verify(Kind::index, pub, doc + " ", *sig));         // altered document
    CHECK(!verify(Kind::index, pub, doc, ""));
    CHECK(!verify(Kind::index, pub, doc, sig->substr(0, 84))); // truncated
    CHECK(!verify(Kind::index, pub, doc, *sig + "A"));
    CHECK(!verify(Kind::index, pub, "", *sig));
    auto flipped = *sig; flipped[10] = flipped[10] == 'A' ? 'B' : 'A';
    CHECK(!verify(Kind::index, pub, doc, flipped));
    CHECK(!verify(Kind::index, pub, doc, *sig + "\n"));        // no whitespace tolerance
    CHECK(!sign_document(Kind::index, "not a pem", doc));

    // Index build + open.
    auto built = build_index({{manifest("zeta")}, {manifest("alpha", "1.2.0", true)}}, 7, "2026-10-30T12:00:00Z");
    CHECK(built.index_json.has_value());
    if (!built.index_json) return 1;
    const auto& index_json = *built.index_json;
    auto isig = *sign_document(Kind::index, keys->private_pem, index_json);
    auto opened = open_index(pub, index_json, isig);
    CHECK(opened);
    if (opened) {
        CHECK(opened.index->sequence == 7 && opened.index->plugins.size() == 2);
        CHECK(opened.index->plugins[0].id == "alpha" && opened.index->plugins[1].id == "zeta"); // sorted
        CHECK(opened.index->plugins[0].hosts == std::vector<std::string>({"api.open-meteo.com"}));
        CHECK(opened.index->plugins[1].hosts.empty());
        CHECK(package_matches(opened.index->plugins[0], manifest("alpha", "1.2.0", true)));
        CHECK(package_matches(opened.index->plugins[1], manifest("zeta")));
        CHECK(!package_matches(opened.index->plugins[1], manifest("alpha")));            // other package
        CHECK(!package_matches(opened.index->plugins[1], manifest("zeta", "1.0.1")));    // other version
        auto host_swap = manifest("alpha", "1.2.0", true);
        host_swap.replace(host_swap.find("open-meteo"), 10, "evil-host ");
        CHECK(!package_matches(opened.index->plugins[0], host_swap));
        CHECK(!package_matches(opened.index->plugins[1], manifest("zeta") + " "));       // size/hash
    }
    CHECK(!open_index(pub_other, index_json, isig));                // wrong key
    CHECK(!open_index(pub, index_json + "\n", isig));               // altered
    CHECK(!open_index(pub, index_json, "AAAA"));
    // A revocation signature is never an index signature.
    CHECK(!open_index(pub, index_json, *sign_document(Kind::revoked, keys->private_pem, index_json)));

    // Signed but malformed content is rejected after the signature passed.
    auto signed_index = [&](const std::function<void(Json&)>& mutation) {
        auto j = Json::parse(index_json); mutation(j);
        auto text = j.dump();
        return open_index(pub, text, *sign_document(Kind::index, keys->private_pem, text));
    };
    CHECK(signed_index([](Json&) {}));
    CHECK(!signed_index([](Json& j) { j["extra"] = 1; }));
    CHECK(!signed_index([](Json& j) { j["schema_version"] = 2; }));
    CHECK(!signed_index([](Json& j) { j["sequence"] = -1; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["file"] = "../../evil.exe"; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["file"] = "plugins/zeta/zeta.flplugin.json"; })); // wrong id
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["sha256"] = "XYZ"; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["size"] = 0; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["size"] = 1e9; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][1]["id"] = "alpha"; j["plugins"][1]["file"] = "plugins/alpha/alpha.flplugin.json"; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["hosts"] = Json::array({"a", "b", "c", "d", "e"}); }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["name"] = "bad\nname"; }));
    CHECK(!open_index(pub, "{\"a\":1,\"a\":2}", *sign_document(Kind::index, keys->private_pem, "{\"a\":1,\"a\":2}"))); // duplicate key

    // Build refuses bad input.
    CHECK(!build_index({{manifest("a")}, {manifest("a")}}, 1, "x").index_json);                 // duplicate
    CHECK(!build_index({{"{}"}}, 1, "x").index_json);                                           // invalid
    auto local = Json::parse(manifest("nometa")); local.erase("license");
    CHECK(!build_index({{local.dump()}}, 1, "x").index_json);                                   // breaks store rules

    CHECK(!signed_index([](Json& j) { j["plugins"][0]["min_app_version"] = "x"; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["min_app_version"] = "99999.0.0"; }));
    CHECK(!signed_index([](Json& j) { j["plugins"][0]["version"] = "1.0"; }));
    CHECK(!build_index({{manifest("a")}}, 2000000001, "x").index_json);
    CHECK(build_index({{manifest("a")}}, 2000000000, "x").index_json.has_value());

    // Rollback guard.
    CHECK(sequence_acceptable(7, 7) && sequence_acceptable(7, 8) && !sequence_acceptable(7, 6));

    // Revocation list.
    Json rev = {{"schema_version", 1}, {"sequence", 3}, {"revoked", Json::array({
        {{"id", "bad-one"}, {"versions", "all"}, {"reason", "Datenabfluss"}},
        {{"id", "alpha"}, {"versions", Json::array({"1.2.0"})}}})}};
    auto rtext = rev.dump();
    auto rsig = *sign_document(Kind::revoked, keys->private_pem, rtext);
    auto list = open_revoked(pub, rtext, rsig);
    CHECK(list);
    if (list) {
        CHECK(is_revoked(*list.list, "bad-one", "9.9.9"));
        CHECK(is_revoked(*list.list, "alpha", "1.2.0"));
        CHECK(!is_revoked(*list.list, "alpha", "1.2.1"));
        CHECK(!is_revoked(*list.list, "zeta", "1.0.0"));
    }
    CHECK(!open_revoked(pub, rtext + " ", rsig));
    CHECK(!open_revoked(pub_other, rtext, rsig));
    CHECK(!open_revoked(pub, rtext, *sign_document(Kind::index, keys->private_pem, rtext)));
    auto bad_rev = rev; bad_rev["revoked"][0]["versions"] = "some";
    auto btext = bad_rev.dump();
    CHECK(!open_revoked(pub, btext, *sign_document(Kind::revoked, keys->private_pem, btext)));
    auto empty_rev = rev; empty_rev["revoked"] = Json::array();
    auto etext = empty_rev.dump();
    CHECK(open_revoked(pub, etext, *sign_document(Kind::revoked, keys->private_pem, etext))); // empty list is valid

    // Previews: separate signed document, own signature domain, strict schema.
    const std::string png = fake_png();
    CHECK(png_dimensions_ok(png));
    CHECK(!png_dimensions_ok(fake_png(4000, 320)) && !png_dimensions_ok(fake_png(480, 8)) && !png_dimensions_ok("not a png at all, just text........"));
    CHECK(!png_dimensions_ok(fake_png(480, 320, preview_limit + 1)));
    auto pv = build_previews({{"alpha", png}, {"beta", fake_png(320, 480)}}, 3);
    CHECK(pv.index_json.has_value());
    CHECK(!build_previews({{"alpha", png}, {"alpha", png}}, 1).index_json.has_value());   // duplicate
    CHECK(!build_previews({{"alpha", "junk"}}, 1).index_json.has_value());                 // not a PNG
    if (pv.index_json) {
        auto psig = *sign_document(Kind::previews, keys->private_pem, *pv.index_json);
        auto opened = open_previews(pub, *pv.index_json, psig);
        CHECK(opened && opened.list->sequence == 3 && opened.list->items.size() == 2 && opened.list->items[0].file == "plugins/alpha/preview.png");
        CHECK(!open_previews(pub, *pv.index_json + " ", psig));
        CHECK(!open_previews(pub_other, *pv.index_json, psig));
        CHECK(!open_previews(pub, *pv.index_json, *sign_document(Kind::index, keys->private_pem, *pv.index_json))); // domain separation
        CHECK(!open_index(pub, *pv.index_json, psig));                                                              // not an index
        if (opened) {
            CHECK(preview_matches(opened.list->items[0], png));
            CHECK(!preview_matches(opened.list->items[0], png + "x"));
            auto changed = png; changed[40] = 'y';
            CHECK(!preview_matches(opened.list->items[0], changed));
        }
        auto j = Json::parse(*pv.index_json);
        j["previews"][0]["file"] = "plugins/other/preview.png";
        auto tampered = j.dump();
        CHECK(!open_previews(pub, tampered, *sign_document(Kind::previews, keys->private_pem, tampered)));
        j = Json::parse(*pv.index_json); j["previews"][0]["extra"] = 1;
        tampered = j.dump();
        CHECK(!open_previews(pub, tampered, *sign_document(Kind::previews, keys->private_pem, tampered)));
    }

    std::printf("plugin signing: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
