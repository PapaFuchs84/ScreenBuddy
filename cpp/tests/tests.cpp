// © 2026 FuchsLabs – Tests ohne Netz und ohne Display: fl-tests.exe [render <ordner>]
#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "config.h"
#include "friends.h"
#include "ha.h"
#include "gfx.h"
#include "http.h"
#include "i18n.h"
#include "installer.h"
#include "netguard.h"
#include "render.h"
#include "rss.h"
#include "secure.h"
#include "util.h"
#include "touch.h"
#include "update.h"
#include "ui_logic.h"

using namespace fl;
namespace fs = std::filesystem;

static int g_fail = 0, g_ok = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) ++g_ok;                                                        \
        else {                                                                   \
            ++g_fail;                                                            \
            std::printf("FEHLER %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                        \
    } while (0)
#define CHECK_EQ(a, b)                                                           \
    do {                                                                         \
        auto _a = (a);                                                           \
        auto _b = (b);                                                           \
        if (_a == _b) ++g_ok;                                                    \
        else {                                                                   \
            ++g_fail;                                                            \
            std::printf("FEHLER %s:%d: %s\n", __FILE__, __LINE__, #a " == " #b); \
        }                                                                        \
    } while (0)

static void save_png(const fs::path& p, const Image& img) {
    std::ofstream f(p, std::ios::binary);
    std::string png = encode_png(img);
    f.write(png.data(), (std::streamsize)png.size());
}

static void test_config() {
    auto tmp = fs::temp_directory_path() / "fl-test-config.toml";
    fs::remove(tmp);
    ojson cfg = config::load(tmp);
    CHECK_EQ(cfg["ui"]["port"].get<int>(), 8765);
    CHECK_EQ(cfg["display"]["pages"].size(), (size_t)6);
    ojson in = {{"display", {{"brightness", "250"}, {"theme", "dunkel"}, {"color_bg", "#aabbcc"}}},
                {"discord", {{"token", "geheim"}, {"guild_ids", {"123456789012345678", " ", "5"}}}},
                {"rss", {{"feeds", {"a.de", "a.de", " b.de "}}}}};
    ojson m = config::merge(cfg, in);
    CHECK_EQ(m["display"]["brightness"].get<int>(), 100);
    CHECK_EQ(m["display"]["color_bg"].get<std::string>(), std::string("#AABBCC"));
    CHECK_EQ(m["discord"]["guild_ids"].size(), (size_t)2);
    CHECK_EQ(m["rss"]["feeds"].size(), (size_t)2);
    bool threw = false;
    try {
        config::merge(cfg, {{"display", {{"theme", "rosa"}}}});
    } catch (const std::invalid_argument& e) {
        threw = std::string(e.what()).find("display.theme: erlaubt:") == 0;
    }
    CHECK(threw);
    threw = false;
    try {
        config::merge(cfg, {{"display", {{"touch", "ja"}}}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    // Darstellung des nativen Einstellungsfensters: auto/hell/dunkel
    CHECK_EQ(cfg["ui"]["settings_theme"].get<std::string>(), std::string("auto"));
    CHECK_EQ(config::merge(cfg, {{"ui", {{"settings_theme", "dunkel"}}}})["ui"]["settings_theme"].get<std::string>(), std::string("dunkel"));
    threw = false;
    try {
        config::merge(cfg, {{"ui", {{"settings_theme", "rosa"}}}});
    } catch (const std::invalid_argument& e) {
        threw = std::string(e.what()).find("ui.settings_theme") == 0;
    }
    CHECK(threw);
    // Companion: standardmäßig aus, Port 8777; nie 8765 bzw. der Port der Einstellungsseite
    CHECK(!cfg["companion"]["enabled"].get<bool>());
    CHECK_EQ(cfg["companion"]["port"].get<int>(), 8777);
    CHECK_EQ(config::merge(cfg, {{"display", {{"output", "network"}}}})["display"]["output"].get<std::string>(), std::string("network"));
    for (ojson bad : {ojson{{"companion", {{"enabled", true}, {"port", 8765}}}},
                      ojson{{"companion", {{"enabled", true}, {"port", 9000}}}, {"ui", {{"port", 9000}}}}}) {
        threw = false;
        try {
            config::merge(cfg, bad);
        } catch (const std::invalid_argument& e) {
            threw = std::string(e.what()).find("companion.port") == 0;
        }
        CHECK(threw);
    }
    CHECK_EQ(config::merge(cfg, {{"companion", {{"port", 8765}}}})["companion"]["port"].get<int>(), 8765);  // aus: egal
    auto separate = config::merge(cfg, {{"display", {{"visible_pages", {"system"}}, {"page_seconds", 60}}},
        {"companion", {{"pages", {"rss", "rss", "invalid", "system"}}, {"page_seconds", 5}}}});
    CHECK_EQ(separate["display"]["visible_pages"], ojson::array({"system"}));
    CHECK_EQ(separate["companion"]["pages"], ojson::array({"rss","system"}));
    CHECK_EQ(separate["display"]["page_seconds"].get<int>(), 60);
    CHECK_EQ(separate["companion"]["page_seconds"].get<int>(), 5);
    CHECK(config::merge(cfg, {{"companion", {{"pages", ojson::array()}}}})["companion"]["pages"].empty());
    ojson pub = config::public_view(m);
    CHECK_EQ(pub["discord"]["token"].get<std::string>(), std::string(""));
    CHECK(pub["discord"]["token_set"].get<bool>());
    CHECK(pub["discord"]["guild_ids"][0].is_string());
    config::save(tmp, m);
    std::string text;
    {
        std::ifstream f(tmp);
        text.assign((std::istreambuf_iterator<char>(f)), {});
    }
    CHECK(text.find("geheim") == std::string::npos);  // Token nie im Klartext
    CHECK(text.find("dpapi:") != std::string::npos);
    ojson back = config::load(tmp);
    CHECK_EQ(back["discord"]["token"].get<std::string>(), std::string("geheim"));
    CHECK_EQ(back["display"]["theme"].get<std::string>(), std::string("dunkel"));
    CHECK_EQ(back["discord"]["guild_ids"][1].get<uint64_t>(), 123456789012345678ull);
    CHECK(!config::token_is_plaintext(tmp));
    fs::remove(tmp);
    // Zeichen, die TOML sprengen würden
    ojson odd = config::merge(cfg, {{"display", {{"title", "a\"b\\c\td"}}}});
    config::save(tmp, odd);
    CHECK_EQ(config::load(tmp)["display"]["title"].get<std::string>(), std::string("a\"b\\c\td"));
    fs::remove(tmp);
    fs::path bak = tmp;
    bak.replace_extension(".toml.bak");
    fs::remove(bak);
}

static void test_secure() {
    std::string p = secure::protect("Hallo Welt ✓");
    CHECK(secure::is_protected(p));
    CHECK_EQ(secure::unprotect(p), std::string("Hallo Welt ✓"));
    CHECK_EQ(secure::unprotect("klartext"), std::string("klartext"));
    CHECK_EQ(secure::unprotect("dpapi:!!!"), std::string(""));
    CHECK_EQ(secure::base64_encode("abc"), std::string("YWJj"));
    CHECK_EQ(secure::base64_decode("YWI="), std::string("ab"));
    // Datei mit einem von Version 1.x (Python) verschlüsselten Wert, falls vorhanden
    if (const char* f = std::getenv("FL_PY_DPAPI")) {
        std::ifstream in(f);
        std::string v;
        std::getline(in, v);
        CHECK_EQ(secure::unprotect(trim(v)), std::string("kompatibel-1.x"));
    }
}

static void test_i18n() {
    i18n::set_lang("en");
    CHECK_EQ(tr("Spielt Battlefield 6 · 42 min"), std::string("Playing Battlefield 6 · 42 min"));
    CHECK_EQ(tr("Hört Numb – Linkin Park"), std::string("Listening to Numb – Linkin Park"));
    CHECK_EQ(tr("Seite 1/3 · Stand 20:15"), std::string("Page 1/3 · Updated 20:15"));
    CHECK_EQ(tr("display.theme: erlaubt: hell, dunkel"), std::string("display.theme: allowed: hell, dunkel"));
    CHECK_EQ(tr("12 Freunde, 3 online"), std::string("12 friends, 3 online"));
    CHECK_EQ(tr("Unbekannt"), std::string("Unbekannt"));
    CHECK_EQ(tr("Nicht verfügbar"), std::string("Unavailable"));
    CHECK_EQ(tr("3 Entitäten"), std::string("3 entities"));
    CHECK_EQ(tr("Rang 174 · First Lieutenant III"), std::string("Rank 174 · First Lieutenant III"));
    CHECK_EQ(tr("An"), std::string("An"));   // Namen wie "An" bleiben unverändert
    CHECK_EQ(ha::format_value("on"), std::string("On"));
    i18n::set_lang("de");
    CHECK_EQ(tr("Spielt X"), std::string("Spielt X"));
}

static void test_rss() {
    std::string rss2 = R"(<?xml version="1.0"?><rss version="2.0" xmlns:dc="http://purl.org/dc/elements/1.1/"><channel>
<title>Mein &amp; Blog</title><item><title><![CDATA[Hallo <b>Welt</b>]]></title><pubDate>Wed, 02 Oct 2002 13:00:00 GMT</pubDate>
<link> https://x.de/1 </link></item><item><title>Zwei</title><dc:date>2024-01-31T10:00:00+02:00</dc:date></item></channel></rss>)";
    auto p = rss::parse_feed(rss2, "x.de");
    CHECK(p.has_value());
    if (p) {
        CHECK_EQ(p->name, std::string("Mein & Blog"));
        CHECK_EQ(p->posts.size(), (size_t)2);
        CHECK_EQ(p->posts[0].title, std::string("Hallo Welt"));
        CHECK_EQ(p->posts[0].link, std::string("https://x.de/1"));
        CHECK(p->posts[0].published && *p->posts[0].published == 1033563600);
        CHECK(p->posts[1].published && *p->posts[1].published == 1706688000);
    }
    std::string atom = R"(<feed xmlns="http://www.w3.org/2005/Atom"><title>Atom-Blog</title>
<entry><title>Eintrag</title><updated>2024-01-31T08:00:00Z</updated><link href="https://a/b"/></entry></feed>)";
    auto a = rss::parse_feed(atom, "a");
    CHECK(a && a->name == "Atom-Blog" && a->posts.size() == 1 && a->posts[0].link == "https://a/b");
    std::string rdf = R"(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns="http://purl.org/rss/1.0/">
<channel><title>RDF</title></channel><item><title>R1</title></item></rdf:RDF>)";
    auto r = rss::parse_feed(rdf, "fallback");
    CHECK(r && r->posts.size() == 1 && r->posts[0].title == "R1" && r->name == "fallback");
    std::string latin = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><rss><channel><title>Gr\xFC\xDF" "e</title>"
                        "<item><title>\xC4pfel</title></item></channel></rss>";
    auto l = rss::parse_feed(latin, "l");
    CHECK(l && l->name == "Grüße" && l->posts[0].title == "Äpfel");
    CHECK(!rss::parse_feed("<!DOCTYPE x [<!ENTITY a \"b\">]><rss/>", "x"));
    CHECK(!rss::parse_feed("kein xml", "x"));
    CHECK(rss::parse_date("Tue, 1 Oct 2024 10:00 +0200") == 1727769600);
    CHECK(rss::parse_date("2024-10-01") == 1727740800);
    auto links = rss::discover(R"(<html><link rel="alternate" type="application/rss+xml" href="/feed.xml?a=1&amp;b=2"></html>)",
                               "https://blog.de/post/1");
    CHECK(!links.empty() && links[0] == "https://blog.de/feed.xml?a=1&b=2");
    CHECK_EQ(links.back(), std::string("https://blog.de/index.xml"));
    CHECK_EQ(rss::html_unescape("&auml;&#228;&#xE4;&bogus;"), std::string("äää&bogus;"));
}

static void test_urls() {
    CHECK_EQ(http::url_join("https://a.de/x/y.html", "z.xml"), std::string("https://a.de/x/z.xml"));
    CHECK_EQ(http::url_join("https://a.de/x/y.html", "../z"), std::string("https://a.de/z"));
    CHECK_EQ(http::url_join("https://a.de/x/", "/feed"), std::string("https://a.de/feed"));
    CHECK_EQ(http::url_join("https://a.de/x", "//b.de/f"), std::string("https://b.de/f"));
    CHECK_EQ(http::url_join("https://a.de/x", "http://c.de/"), std::string("http://c.de/"));
    auto u = http::parse_url("https://www.heise.de:8443/rss?x=1");
    CHECK(u.valid && u.host == "www.heise.de" && u.port == 8443 && u.path == "/rss?x=1");
    CHECK(!http::parse_url("ftp://x").valid);
}

static void test_netguard() {
    for (auto ip : {"127.0.0.1", "10.1.2.3", "192.168.178.1", "172.16.0.1", "169.254.1.1", "100.64.0.1", "0.0.0.0", "::1",
                    "fe80::1", "fd00::1", "::ffff:192.168.0.1", "224.0.0.1", "2001:db8::1"})
        CHECK(!netguard::is_global_ip(ip));
    for (auto ip : {"1.1.1.1", "8.8.8.8", "2606:4700:4700::1111", "::ffff:8.8.8.8"}) CHECK(netguard::is_global_ip(ip));
    CHECK(!netguard::public_url("http://localhost/feed"));
    CHECK(!netguard::public_url("http://127.0.0.1:8765/api/quit"));
    CHECK(!netguard::public_url("file:///c:/x"));
}

static void test_friends() {
    // Werte wie difflib.SequenceMatcher(None, a, b).ratio()
    CHECK(std::abs(friends::similarity("fuchsbau", "fuchsbaux") - 0.9411764705882353) < 1e-9);
    CHECK(std::abs(friends::similarity("abcd", "bcde") - 0.75) < 1e-9);
    CHECK(std::abs(friends::similarity("papafox", "papafuchs") - 0.625) < 1e-9);
    CHECK_EQ(friends::norm("Ünïcödé-Fuchs_84"), std::string("unicodefuchs84"));
    std::vector<friends::Account> acc = {
        {"discord", "1", "Basti", "online", "Spielt BF6"}, {"steam", "765", "basti", "offline"}, {"xbox", "x1", "B4sti", "online"},
        {"steam", "766", "Lena", "idle"}};
    auto merged = friends::merge_accounts(acc, {}, true);
    CHECK_EQ(merged.size(), (size_t)3);
    CHECK(merged[0].name == "Basti" && merged[0].platforms.size() == 2 && merged[0].activity == "Spielt BF6");
    auto linked = friends::merge_accounts(acc, {"Basti|discord:1|xbox:B4sti|steam:765"}, true);
    CHECK_EQ(linked.size(), (size_t)2);
    CHECK_EQ(linked[0].platforms.size(), (size_t)3);
    ojson accounts = ojson::array();
    for (auto& a : acc) accounts.push_back({{"platform", a.platform}, {"id", a.uid}, {"name", a.name}});
    ojson sug = friends::suggest_links(accounts, {}, true);
    CHECK(!sug.empty());
    nlohmann::json steam = nlohmann::json::parse(
        R"({"response":{"players":[{"steamid":"7656","personaname":"Fuchs 🦊","personastate":1,"gameextrainfo":"Battlefield 6","avatarmedium":"https://a/b.jpg"}]}})");
    auto sp = friends::parse_steam_players(steam);
    CHECK(sp.size() == 1 && sp[0].name == "Fuchs" && sp[0].activity == "Spielt Battlefield 6" && sp[0].status == "online");
    nlohmann::json xbox = nlohmann::json::parse(
        R"({"content":{"people":[{"xuid":"1","gamertag":"Fox","presenceState":"Online","presenceText":"Playing Halo"},{"xuid":"2","gamertag":"Off","presenceState":"Offline","presenceText":"Zuletzt online"}]}})");
    auto xp = friends::parse_xbox_people(xbox);
    CHECK(xp.size() == 2 && xp[0].activity == "Spielt Halo" && xp[1].status == "offline" && xp[1].activity.empty());
}

static void test_ui_logic() {
    using namespace fl::ui::logic;
    CHECK(normalize_hex("#aBc") == std::optional<std::string>("#AABBCC"));
    CHECK(normalize_hex(" 12ef90 ") == std::optional<std::string>("#12EF90"));
    CHECK(!normalize_hex("") && !normalize_hex("#12") && !normalize_hex("#GG0000") && !normalize_hex("#1234567"));
    CHECK_EQ(lines_join(lines_of(" a \r\n\r\nb\nc\r")), std::string("a\r\nb\r\nc"));
    CHECK_EQ(lines_of("").size(), (size_t)0);
    CHECK_EQ(lines_join(id_list("12, 34;12\n5x6")), std::string("12\r\n34\r\n5\r\n6"));
    CHECK_EQ(set_id("1\r\n2", "3", true), std::string("1\r\n2\r\n3"));
    CHECK_EQ(set_id("1\r\n2", "2", true), std::string("1\r\n2"));
    CHECK_EQ(set_id("1\r\n2\r\n3", "2", false), std::string("1\r\n3"));
    CHECK_EQ(set_id("1", "9", false), std::string("1"));
    // Verknüpfen: neue Zeile, vorhandene erweitern, keine Doppelten
    auto l1 = link_accounts({}, {"steam", "765", "Basti"}, {"xbox", "B4sti", "B4sti"});
    CHECK_EQ(l1.size(), (size_t)1);
    CHECK_EQ(l1[0], std::string("Basti|steam:765|xbox:B4sti"));
    auto l2 = link_accounts(l1, {"xbox", "b4sti", "B4sti"}, {"discord", "1", "Basti"});
    CHECK_EQ(l2.size(), (size_t)1);
    CHECK_EQ(l2[0], std::string("Basti|steam:765|xbox:B4sti|discord:1"));
    auto l3 = link_accounts(l2, {"steam", "766", "Lena"}, {"xbox", "lena", "lena"});
    CHECK_EQ(l3.size(), (size_t)2);
    // Die Verknüpfung versteht friends.cpp
    ojson accounts = ojson::array({{{"platform", "steam"}, {"id", "765"}, {"name", "basti"}},
                                   {{"platform", "xbox"}, {"id", "B4sti"}, {"name", "B4sti"}},
                                   {{"platform", "discord"}, {"id", "1"}, {"name", "Basti"}}});
    std::vector<fl::friends::Account> acc = {{"steam", "765", "basti"}, {"xbox", "B4sti", "B4sti"}, {"discord", "1", "Basti"}};
    CHECK_EQ(fl::friends::merge_accounts(acc, l2, false).size(), (size_t)1);
    // Home-Assistant-Entitäten
    std::vector<std::string> ents;
    CHECK_EQ(add_entity(ents, "Sensor.Temp", 4), std::string());
    CHECK_EQ(add_entity(ents, "sensor.temp", 4), std::string());  // schon da
    CHECK_EQ(ents.size(), (size_t)1);
    CHECK_EQ(add_entity(ents, "kein id", 4), std::string("invalid"));
    CHECK_EQ(add_entity(ents, "sensor", 4), std::string("invalid"));
    ents = {"sensor.a|Küche", "sensor.b", "light.c", "switch.d"};
    CHECK_EQ(add_entity(ents, "sensor.e", 4), std::string("full"));
    remove_entity(ents, "sensor.a");
    CHECK(ents.size() == 3 && ents[0] == "sensor.b" && !has_entity(ents, "sensor.a"));
    CHECK(entity_id_of("Sensor.A | Küche") == "sensor.a");
    // Lesbarkeit
    CHECK(color_warnings({{"text", "#111111"}, {"bg", "#FFFFFF"}, {"card", "#EEEEEE"}, {"muted", "#555555"}, {"accent", "#003366"}}).empty());
    auto w = color_warnings({{"text", "#CCCCCC"}, {"bg", "#FFFFFF"}, {"card", "#EEEEEE"}});
    CHECK(w.size() == 2 && w[0] == "text_bg" && w[1] == "text_card");
    CHECK(color_warnings({{"text", "#CCCCCC"}}).empty());  // unvollständig: nichts zu prüfen
    // Filter: Suche ohne Groß/Kleinschreibung, Angehakte zuerst, Obergrenze
    std::vector<std::string> names = {"Anna", "Bernd", "Anja", "Clara"};
    auto idx = filter_indices(names, {false, false, true, false}, "AN", 10);
    CHECK(idx.size() == 2 && idx[0] == 2 && idx[1] == 0);
    CHECK_EQ(filter_indices(names, {}, "", 3).size(), (size_t)3);
    CHECK(filter_indices(names, {}, "xyz", 3).empty());
    // QR: quadratisch, weiße Ruhezone, dunkle Module
    Image qr = qr_image("screenbuddy://pair?x=1", 4);
    CHECK(qr.w == qr.h && qr.w > 40 && qr.px.front() == 0xFFFFFFFFu);
    CHECK(std::count(qr.px.begin(), qr.px.end(), 0xFF000000u) > 100);
}
static void test_ha() {
    auto sp = ha::parse_spec(" sensor.wohnzimmer_temp | Wohnzimmer ");
    CHECK(sp && sp->id == "sensor.wohnzimmer_temp" && sp->label == "Wohnzimmer");
    CHECK(!ha::parse_spec("Sensor.X"));
    CHECK(!ha::parse_spec("kein id"));
    CHECK(!ha::parse_spec("sensor.a/../b"));
    CHECK(ha::toggle_domain("switch.x") && ha::toggle_domain("light.x") && ha::toggle_domain("input_boolean.x"));
    CHECK(!ha::toggle_domain("lock.haustuer") && !ha::toggle_domain("cover.garage") && !ha::toggle_domain("sensor.x"));
    CHECK_EQ(ha::base_url("http://homeassistant.local:8123/"), std::string("http://homeassistant.local:8123"));
    CHECK_EQ(ha::base_url("192.168.1.5:8123"), std::string("http://192.168.1.5:8123"));
    CHECK_EQ(ha::base_url("https://ha.example.org/lovelace/0"), std::string("https://ha.example.org"));
    CHECK_EQ(ha::base_url("http://user:pw@host:8123"), std::string(""));
    CHECK_EQ(ha::base_url(""), std::string(""));
    CHECK_EQ(ha::format_value("21.53"), std::string("21,5"));
    CHECK_EQ(ha::format_value("2840"), std::string("2840"));
    CHECK_EQ(ha::format_value("0.05"), std::string("0,05"));
    CHECK_EQ(ha::format_value("on"), std::string("An"));
    CHECK_EQ(ha::format_value("Sonnig"), std::string("Sonnig"));
    nlohmann::json st = {{"entity_id", "switch.kaffee"}, {"state", "on"}, {"attributes", {{"friendly_name", "Kaffeemaschine"}}}};
    auto e = ha::parse_state(st, {"switch.kaffee", ""}, true);
    CHECK(e.available && e.on && e.toggle && e.name == "Kaffeemaschine");
    CHECK(!ha::parse_state(st, {"switch.kaffee", ""}, false).toggle);   // ohne Touch nie schaltbar
    nlohmann::json off = {{"state", "unavailable"}, {"attributes", {}}};
    CHECK(!ha::parse_state(off, {"switch.kaffee", "Eigen"}, true).available);
    nlohmann::json se = {{"state", "21.5"}, {"attributes", {{"unit_of_measurement", "°C"}}}};
    CHECK_EQ(ha::parse_state(se, {"sensor.t", ""}, true).unit, std::string("°C"));
    CHECK(!ha::parse_state(se, {"sensor.t", ""}, true).toggle);
    // Konfiguration: höchstens 4 Entitäten, Token verschlüsselt/verborgen, ohne Touch keine Schalter im Client
    ojson cfg = config::load(fs::temp_directory_path() / "fl-test-ha-none.toml");
    ojson m = config::merge(cfg, {{"homeassistant", {{"entities", {"sensor.a", "sensor.b", "sensor.c", "sensor.d", "sensor.e"}}, {"token", "geheim"}}}});
    CHECK_EQ(m["homeassistant"]["entities"].size(), (size_t)4);
    CHECK_EQ(config::public_view(m)["homeassistant"]["token"].get<std::string>(), std::string(""));
    CHECK(config::public_view(m)["homeassistant"]["token_set"].get<bool>());
    ojson hc = {{"url", "http://x:8123"}, {"token", "t"}, {"entities", {"sensor.a", "switch.b", "light.c|Lampe"}}};
    ha::HaClient no_touch(hc, false, nullptr), with_touch(hc, true, nullptr);
    CHECK_EQ(no_touch.entities().size(), (size_t)1);
    CHECK_EQ(with_touch.entities().size(), (size_t)3);
    // Kacheln und Treffer
    auto cs = render::HaRenderer::cells(4, 480, 320);
    CHECK_EQ(cs.size(), (size_t)4);
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 4, 100, 80), 0);
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 4, 400, 80), 1);
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 4, 100, 250), 2);
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 4, 400, 250), 3);
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 4, 100, 10), -1);   // Kopfzeile
    CHECK_EQ(render::HaRenderer::cell_at(480, 320, 3, 400, 250), 2);   // dritte Kachel über die ganze Breite
    CHECK_EQ(render::HaRenderer::cell_at(960, 640, 4, 800, 500), 3);   // skaliertes Display
}

// Systemansicht: Tippen auf RAM / Datenträger / Netzwerk
static void test_system_touch() {
    using S = render::SysRenderer;
    CHECK_EQ(S::zone_at(480, 320, 380, 60), (int)S::ZONE_RAM);
    CHECK_EQ(S::zone_at(480, 320, 380, 120), (int)S::ZONE_DISK);
    CHECK_EQ(S::zone_at(480, 320, 380, 250), (int)S::ZONE_NET);
    CHECK_EQ(S::zone_at(480, 320, 100, 60), -1);    // CPU-Karte links
    CHECK_EQ(S::zone_at(480, 320, 380, 10), -1);    // Kopfzeile
    CHECK_EQ(S::zone_at(480, 320, 380, 315), -1);   // Fußzeile
    CHECK_EQ(S::zone_at(960, 640, 760, 500), (int)S::ZONE_NET);   // skaliertes Display
    int hit = -1;
    touch::Tile t{{0, 0, 480, 320}};
    t.sys = true;
    for (int i = 0; i < 3; ++i) t.cells.push_back([&hit, i] { hit = i; });
    std::vector<touch::Tile> tiles{t};
    const auto* a = touch::action_at(tiles, 380, 120);
    CHECK(a != nullptr);
    if (a) (*a)();
    CHECK_EQ(hit, 1);
    CHECK(touch::action_at(tiles, 100, 120) == nullptr);
}

// Update-Hinweis: Versionsvergleich und Auswertung der GitHub-Antwort
static void test_update() {
    CHECK(update::is_newer("v2.2.0", "2.1.0"));
    CHECK(update::is_newer("2.10.0", "2.9.5"));
    CHECK(!update::is_newer("2.1.0", "2.1.0"));
    CHECK(!update::is_newer("v2.0.9", "2.1.0"));
    CHECK(!update::is_newer("2.2.0-rc1", "2.1.0"));   // Vorabversion
    CHECK(!update::is_newer("kaputt", "2.1.0"));
    auto ok = update::parse_release({{"tag_name", "v2.2.0"}, {"html_url", "https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.2.0"}});
    CHECK(ok.has_value());
    if (ok) CHECK_EQ(ok->version, std::string("2.2.0"));
    auto foreign = update::parse_release({{"tag_name", "v2.2.0"}, {"html_url", "https://boese.example/x"}});
    CHECK(foreign.has_value());
    if (foreign) CHECK(foreign->url.rfind(update::RELEASES_PAGE_PREFIX, 0) == 0);   // fremde Adresse wird nicht übernommen
    CHECK(!update::parse_release({{"tag_name", "v2.3.0"}, {"prerelease", true}}).has_value());
    CHECK(!update::parse_release({{"tag_name", "v2.3.0"}, {"draft", true}}).has_value());
    CHECK(!update::parse_release(nlohmann::json::array()).has_value());
}

// Nur mit FL_HA_FAKE=http://127.0.0.1:PORT (Testserver, Token testtoken123): echte Abfrage und Schalten
static void test_ha_live() {
    char* fake = nullptr;
    size_t n = 0;
    if (_dupenv_s(&fake, &n, "FL_HA_FAKE") != 0 || !fake) return;
    std::string url = fake;
    free(fake);
    ojson hc = {{"url", url}, {"token", "testtoken123"},
                {"entities", {"sensor.wohnzimmer_temperatur", "binary_sensor.fenster", "light.schreibtisch", "switch.kaffee|Kaffee", "sensor.gibts_nicht"}}};
    ha::HaClient cl(hc, true, nullptr);
    CHECK_EQ(cl.entities().size(), (size_t)4);   // höchstens 4
    cl.start();
    for (int i = 0; i < 100 && !cl.has_data(); ++i) Sleep(100);
    CHECK(cl.has_data());
    auto es = cl.entities();
    CHECK(es.size() == 4 && es[0].unit == "°C" && es[0].name == "Wohnzimmer Temperatur" && !es[0].toggle);
    CHECK(es[2].toggle && es[2].on && es[2].name == "Schreibtischlampe");
    CHECK(es[3].toggle && !es[3].on && es[3].name == "Kaffee");
    cl.request_toggle("switch.kaffee");
    bool flipped = false;
    for (int i = 0; i < 100 && !flipped; ++i) {
        Sleep(100);
        auto e2 = cl.entities();
        flipped = e2.size() == 4 && e2[3].on;
    }
    CHECK(flipped);
    cl.request_toggle("lock.haustuer");   // kein Schalter-Bereich: wird nie gesendet
    cl.request_toggle("sensor.wohnzimmer_temperatur");
    Sleep(500);
    cl.stop();
    auto r = http::get(url + "/_calls");
    CHECK_EQ(r.body, std::string("[\"switch.kaffee\"]"));
    // Falsches Token
    ojson bad = hc;
    bad["token"] = "falsch";
    try {
        ha::list_entities(url, "falsch", true);
        CHECK(false);
    } catch (const ha::HaError& e) {
        CHECK(std::string(e.what()).find("Token") != std::string::npos);
    }
    auto list = ha::list_entities(url, "testtoken123", false);
    bool has_light = false, has_lock = false;
    for (auto& e : list) { has_light |= e["id"] == "light.schreibtisch"; has_lock |= e["id"] == "lock.haustuer"; }
    CHECK(!has_light && !has_lock);   // ohne Touch keine Schalter, Schlösser nie
    CHECK(ha::list_entities(url, "testtoken123", true).size() == list.size() + 2);
}

static void test_touch_targets() {
    Friend f;
    f.ids = {{"discord", "123456789012345678"}, {"steam", "76561198000000001"}, {"xbox", "Fox #1234"}};
    f.online_on = {"discord", "steam", "xbox"};
    auto targets = touch::friend_targets(f);
    CHECK(targets.size() == 3 && targets[0].protocol == "steam" && targets[1].protocol == "discord");
    CHECK_EQ(targets[0].url, std::string("steam://friends/message/76561198000000001"));
    CHECK_EQ(targets[1].url, std::string("discord://-/users/123456789012345678"));
    CHECK_EQ(targets[2].url, std::string("https://account.xbox.com/Profile?gamertag=Fox%20%231234"));
    std::vector<std::string> launches;
    auto launcher = [&](const std::string& url) { launches.push_back(url); return true; };
    CHECK(touch::dispatch(targets, [](auto&) { return true; }, launcher));
    CHECK(launches.size() == 1 && launches[0] == targets[0].url);
    launches.clear();
    CHECK(touch::dispatch(targets, [](auto& p) { return p == "discord"; }, launcher)); // Steam fehlt -> Discord
    CHECK(launches.size() == 1 && launches[0] == targets[1].url);
    launches.clear();
    CHECK(touch::dispatch(targets, [](auto&) { return false; }, launcher)); // keine App -> Xbox im Browser
    CHECK(launches.size() == 1 && launches[0] == targets[2].url);
    CHECK(!touch::dispatch(targets, [](auto&) { return true; }, [](auto&) { return false; }));
    CHECK(!touch::dispatch({}, [](auto&) { return true; }, launcher));
    f.ids = {{"discord", "123456789012345678"}, {"xbox", "Fox #1234"}};
    targets = touch::friend_targets(f);
    CHECK(targets.size() == 2 && targets[0].protocol == "discord"); // ohne Steam: Discord vor Xbox
    f.ids = {{"xbox", "Fox #1234"}};
    targets = touch::friend_targets(f);
    CHECK(targets.size() == 1 && targets[0].protocol.empty());
    f.ids = {{"steam", "1?command=x"}, {"discord", "1/../../bad"}};
    CHECK(touch::friend_targets(f).empty());

    Bf6Player p;
    p.query = "Fox #1@xbox";
    CHECK_EQ(touch::player_targets(p)[0].url, std::string("https://gametools.network/stats/xboxone/name/Fox%20%231?game=bf6"));
    p.query = "Üni/code?x=1@psn";
    CHECK_EQ(touch::player_targets(p)[0].url, std::string("https://gametools.network/stats/ps4/name/%C3%9Cni%2Fcode%3Fx%3D1?game=bf6"));
    p.query = "Fox@steam";
    CHECK_EQ(touch::player_targets(p)[0].url, std::string("https://gametools.network/stats/pc/name/Fox?game=bf6"));
    p.error = "nicht gefunden";
    CHECK(touch::player_targets(p).empty());
    p.error.clear(); p.query.clear();
    CHECK(touch::player_targets(p).empty());

    auto people = render::demo_friends();
    for (auto& person : people) person.ids["steam"] = std::to_string(person.user_id);
    auto rows = touch::friend_rows(people, 0, 6);
    CHECK(rows.size() == 6 && rows[0][0].url == "steam://friends/message/1");
    auto second = touch::friend_rows(people, 1, 6);
    CHECK(second.size() == 2 && second[1][0].url == "steam://friends/message/6"); // offline Tom last
    CHECK_EQ(touch::friend_rows(people, -1, 6)[1][0].url, second[1][0].url);
    CHECK_EQ(touch::friend_rows(people, 2, 6)[0][0].url, rows[0][0].url);
    CHECK(touch::friend_rows({}, 0, 6).empty());
    CHECK(touch::friend_rows(people, 0, 0).empty());
    auto players = render::demo_players();
    std::reverse(players.begin(), players.end());
    auto bfrows = touch::player_rows(players, 0, 6);
    CHECK(bfrows.size() == 6 && bfrows[0][0].url.find("/name/Fuchs?") != std::string::npos && bfrows.back().empty());
    auto saved = bfrows[0][0].url;
    players[0].query = "Changed";
    CHECK_EQ(bfrows[0][0].url, saved);  // Ziele bleiben an den angezeigten Snapshot gebunden

    auto posts = touch::post_rows({{"", "", {}, "https://example.org/article"}, {"", "", {}, "file:///C:/evil.exe"},
                                  {"", "", {}, "steam://run/123"}, {"", "", {}, "https://example.org/\n"},
                                  {"", "", {}, "https://example.org/hidden"}});
    CHECK(posts.size() == 4 && posts[0].size() == 1 && posts[1].empty() && posts[2].empty());
    // Zeilengrenzen, Leerraum und Kopf-/Fußzeile dürfen keine fremde Zeile auswählen.
    for (auto [w, h] : {std::pair{480, 320}, {320, 480}, {637, 720}, {1280, 720}}) {
        render::Renderer renderer(w, h, "Test");
        float scale = render::scale_for(w, h);
        CHECK(render::Renderer::row_at(w, h, 0) == -1);
        CHECK(render::Renderer::row_at(w, h, h - 1) == -1);
        for (int i = 0; i < renderer.rows_per_page(); ++i)
            CHECK(render::Renderer::row_at(w, h, (int)((36 + 44 * i + 22) * scale)) == i);
    }
    std::vector<touch::Tile> tiles = {{{0, 0, 480, 320}, false, rows}, {{480, 0, 480, 320}, false, second},
                                      {{960, 0, 480, 320}, true, posts}};
    CHECK(touch::at(tiles, 10, 58) && touch::at(tiles, 10, 58)->at(0).url == rows[0][0].url);
    CHECK(touch::at(tiles, 490, 102) && touch::at(tiles, 490, 102)->at(0).url == second[1][0].url);
    CHECK(touch::at(tiles, 970, 58) && touch::at(tiles, 970, 58)->at(0).url == posts[0][0].url);
    CHECK(!touch::at(tiles, 490, 146)); // unbefüllte Zeile der letzten Unterseite
    CHECK(!touch::at(tiles, 970, 120)); // RSS-Zeile mit Dateipfad
    CHECK(!touch::at(tiles, 10, 301)); // Leerraum nach den sechs Zeilen
    CHECK(!touch::at(tiles, 10, 10));
    CHECK(!touch::at(tiles, -1, 58));
    CHECK(!touch::at(tiles, 1440, 58));
    CHECK(!touch::at(tiles, 10, 320));

    auto xbox = friends::parse_xbox_people(nlohmann::json::parse(
        R"({"people":[{"xuid":"1","gamertag":"Fox #1234","presenceState":"Online"}]})"));
    auto merged = friends::merge_accounts(xbox, {}, true);
    CHECK(merged.size() == 1 && merged[0].ids.at("xbox") == "Fox #1234" && merged[0].online_on.size() == 1);
}

static void test_render(const fs::path& out) {
    fs::create_directories(out);
    std::lock_guard rl(render::mutex());
    bool english = i18n::lang() == "en";
    for (auto [w, h, tag] : {std::tuple{480, 320, "480x320"}, std::tuple{320, 480, "320x480"}, std::tuple{1920, 720, "1920x720"},
                             std::tuple{637, 720, "637x720"}}) {
        for (auto th : {"hell", "dunkel"}) {
            render::set_theme(th);
            std::string pre = std::string(tag) + "-" + th + "-";
            render::Renderer r(w, h, "Discord-Freunde");
            save_png(out / (pre + "freunde.png"), r.render_friends(render::demo_friends(), 0, true, "", 1792606500));
            save_png(out / (pre + "welcome.png"), r.welcome("Willkommen!", "Öffne die Einstellungen über das Fuchs-Symbol im "
                                                                           "Infobereich oder im Browser: 127.0.0.1:8765"));
            render::Bf6Renderer b(w, h, "Battlefield 6 · Rangliste");
            save_png(out / (pre + "bf6.png"), b.render(render::demo_players(), 0, 1792606500, 1792606500));
            render::RssRenderer rr(w, h, "Blog-Updates");
            std::vector<Post> posts = english
                ? std::vector<Post>{{"Example blog", "A short title for the preview", 1792606500 - 600, ""},
                                    {"Another blog", "Second post with a slightly longer title that wraps onto another line", 1792606500 - 7200, ""},
                                    {"Tech digest", "A longer headline for the example feed", std::nullopt, ""}}
                : std::vector<Post>{{"Beispiel-Blog", "Ein kurzer Titel für die Vorschau", 1792606500 - 600, ""},
                                    {"Noch ein Blog", "Zweiter Beitrag mit etwas längerem Titel, der umbricht und noch weiter geht als gedacht", 1792606500 - 7200, ""},
                                    {"heise online", "heise+ | Test: Sehr langer Titel mit Präfix", std::nullopt, ""}};
            save_png(out / (pre + "blogs.png"), rr.render(posts, 1792606500, 1792606500));
            render::SysRenderer sr(w, h, "System", "bit");
            SysSnapshot sn;
            sn.cpu_name = "Ryzen 7 5700X";
            sn.cpu_pct = 23; sn.cpu_ghz = 4.45; sn.cpu_temp = 61; sn.cpu_fan = 1180;
            sn.gpu_name = "RTX 4070"; sn.gpu_pct = 91; sn.gpu_temp = 77; sn.gpu_fan = 45; sn.gpu_mem_used = 5300; sn.gpu_mem_total = 12282;
            sn.ram_pct = 48; sn.ram_used = 15.4; sn.ram_total = 32;
            sn.disk_pct = 71; sn.net_name = "Skynet"; sn.net_down = 2.5e6; sn.net_up = 180e3; sn.hwinfo = true;
            for (int i = 0; i < 60; ++i) {
                sn.down_hist.push_back(1e6 + 1e6 * std::sin(i / 6.0));
                sn.up_hist.push_back(2e5 + 1e5 * std::cos(i / 5.0));
            }
            save_png(out / (pre + "system.png"), sr.render(sn, 1792606500));
            render::HaRenderer hr(w, h, "Home Assistant");
            auto demo = render::demo_ha();
            if (english) {
                demo[0].name = "Living room";
                demo[2].name = "Desk lamp";
                demo[3].name = "Coffee maker";
            }
            save_png(out / (pre + "ha.png"), hr.render(demo, true, "", 1792606500, 1792606500));
            save_png(out / (pre + "ha3.png"), hr.render({demo[0], demo[1], demo[2]}, true, "", 1792606500, 1792606500));
            save_png(out / (pre + "ha2.png"), hr.render({demo[2], demo[0]}, true, "", 1792606500, 1792606500));
            save_png(out / (pre + "ha1.png"), hr.render({demo[0]}, true, "", 1792606500, 1792606500));
            save_png(out / (pre + "ha-fehler.png"), hr.render({}, false, "Home Assistant ist nicht erreichbar – Adresse und Netzwerk prüfen", std::nullopt, 1792606500));
        }
    }
    auto write = [&](const char* name, const std::string& png) {
        std::ofstream f(out / name, std::ios::binary);
        f.write(png.data(), (std::streamsize)png.size());
    };
    write("setup-neu.png", installer::preview_png(false, false, 0, "", 1.0f));
    write("setup-update.png", installer::preview_png(true, true, 0, "", 1.5f));
    write("setup-arbeit.png", installer::preview_png(true, true, 1, tr("Dateien werden kopiert …"), 1.0f));
    write("setup-fehler.png", installer::preview_png(false, false, 3,
        tr("Die laufende App ließ sich nicht beenden. Bitte über das Fuchs-Symbol beenden und erneut versuchen."), 1.0f));
    render::set_theme("eigen", {{"header", "#202020"}, {"bg", "#FFFFFF"}});
    render::Renderer r(480, 320, "Eigen");
    save_png(out / "480x320-eigen-freunde.png", r.render_friends(render::demo_friends(), 0, true, "", 1792606500));
    render::set_theme("hell");
}

#include <dpp/dpp.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>

static void ssl_probe(bool winstore) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    int dp = SSL_CTX_set_default_verify_paths(ctx);
    int ws = winstore ? SSL_CTX_load_verify_store(ctx, "org.openssl.winstore://") : -1;
    std::printf("default_paths=%d winstore=%d file=%s dir=%s\n", dp, ws, X509_get_default_cert_file(), X509_get_default_cert_dir());
    BIO* b = BIO_new_ssl_connect(ctx);
    SSL* ssl = nullptr;
    BIO_get_ssl(b, &ssl);
    SSL_set_tlsext_host_name(ssl, "discord.com");
    SSL_set1_host(ssl, "discord.com");
    BIO_set_conn_hostname(b, "discord.com:443");
    int rc = BIO_do_handshake(b);
    std::printf("handshake=%d verify=%ld (%s) err=%lu\n", rc, SSL_get_verify_result(ssl),
                X509_verify_cert_error_string(SSL_get_verify_result(ssl)), ERR_get_error());
    BIO_free_all(b);
    SSL_CTX_free(ctx);
}

static bool argc_probe_winstore = false;
static int dpp_probe() {
    // Diagnose: kann D++ (OpenSSL) eine HTTPS-Verbindung zu Discord aufbauen?
    if (argc_probe_winstore) _putenv_s("SSL_CERT_URI", "org.openssl.winstore://");
    dpp::cluster c("x", 0, 0, 0, 1, true, dpp::cache_policy::cpol_none, 1);
    c.on_log([](const dpp::log_t& e) { std::printf("[%d] %s\n", (int)e.severity, e.message.c_str()); });
    std::atomic<bool> done{false};
    c.start(dpp::st_return);
    Sleep(3000);
    c.request("https://discord.com/api/v10/gateway", dpp::m_get, [&](const dpp::http_request_completion_t& r) {
        std::printf("Status %u, Fehler %d, %zu Bytes: %.200s\n", r.status, (int)r.error, r.body.size(), r.body.c_str());
        done = true;
    });
    for (int i = 0; i < 100 && !done; ++i) Sleep(100);
    return done ? 0 : 1;
}

// Diagnose: fl-tests touch <x0> <y0> <x1> <y1> – eine Berührung (Tippen, wenn Start = Ende) per Windows-Touch-Injektion
static int touch_probe(int x0, int y0, int x1, int y1) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (!InitializeTouchInjection(2, TOUCH_FEEDBACK_NONE)) return 2;
    auto contact = [](DWORD flags, int x, int y) {
        POINTER_TOUCH_INFO p{};
        p.pointerInfo.pointerType = PT_TOUCH;
        p.pointerInfo.pointerId = 1;
        p.pointerInfo.pointerFlags = flags;
        p.pointerInfo.ptPixelLocation = {x, y};
        p.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
        p.rcContact = {x - 2, y - 2, x + 2, y + 2};
        p.orientation = 90;
        p.pressure = 32000;
        Sleep(25);
        bool r = InjectTouchInput(1, &p) != FALSE;
        if (!r) std::printf("InjectTouchInput Fehler %lu bei %d/%d\n", GetLastError(), x, y);
        return r;
    };
    bool ok = contact(POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, x0, y0);
    if (x1 != x0 || y1 != y0) ok &= contact(POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT, x1, y1);
    ok &= contact(POINTER_FLAG_UP, x1, y1);
    std::printf("touch %s\n", ok ? "ok" : "fehlgeschlagen");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    if (argc >= 6 && std::string(argv[1]) == "touch")
        return touch_probe(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    gfx_init();
    if (argc >= 2 && std::string(argv[1]) == "dpp") {
        argc_probe_winstore = argc >= 3;
        ssl_probe(false);
        ssl_probe(true);
        return dpp_probe();
    }
    if (argc >= 3 && std::string(argv[1]) == "render") {
        i18n::set_lang(argc >= 4 ? argv[3] : "de");
        test_render(argv[2]);
        std::printf("Bilder geschrieben: %s\n", argv[2]);
        return 0;
    }
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::set_terminate([] {
        try {
            if (auto e = std::current_exception()) std::rethrow_exception(e);
        } catch (const std::exception& ex) {
            std::printf("Ausnahme: %s\n", ex.what());
        } catch (...) {
        }
        std::abort();
    });
    for (auto [name, fn] : {std::pair{"config", test_config}, {"secure", test_secure}, {"i18n", test_i18n}, {"rss", test_rss},
                            {"urls", test_urls}, {"netguard", test_netguard}, {"friends", test_friends}, {"homeassistant", test_ha}, {"ui-logic", test_ui_logic}, {"homeassistant-live", test_ha_live}, {"touch-targets", test_touch_targets}, {"system-touch", test_system_touch}, {"update", test_update}}) {
        std::printf("· %s\n", name);
        fn();
    }
    std::printf("%d Prüfungen bestanden, %d fehlgeschlagen\n", g_ok, g_fail);
    return g_fail ? 1 : 0;
}
