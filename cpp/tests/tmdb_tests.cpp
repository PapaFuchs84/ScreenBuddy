// © 2026 FuchsLabs – Logik der Seite „Streaming-Neustarts“ (TMDB) mit festen JSON-Beispielen; kein Netzzugriff.
#include "tmdb_logic.h"

#include <cstdio>
#include <string>

using namespace fl::tmdb;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

// Feste Beispielantworten (frei erfunden, nur Form wie bei TMDB v3)
static const char* PROVIDERS_JSON = R"({"results":[
 {"display_priorities":{"DE":0},"display_priority":0,"logo_path":"/x.jpg","provider_name":"Netflix","provider_id":8},
 {"provider_name":"Disney Plus","provider_id":337},
 {"provider_name":"Amazon Prime Video","provider_id":9},
 {"provider_name":"Apple TV Plus","provider_id":350},
 {"provider_name":"","provider_id":5},
 {"provider_name":"Kaputt","provider_id":"x"},
 {"provider_name":"Alpha Beta Gamma Delta","provider_id":777},
 "kein objekt"]})";

static const char* DISCOVER_MOVIE = R"({"page":1,"total_results":4,"results":[
 {"id":101,"title":"Neuer Film","release_date":"2026-10-28"},
 {"id":102,"title":"Zu alt","release_date":"2026-08-01"},
 {"id":103,"title":"Zukunft","release_date":"2026-11-20"},
 {"id":104,"title":"Ohne Datum","release_date":""},
 {"id":105,"release_date":"2026-10-20"},
 {"id":106,"title":"  Älterer Film  ","release_date":"2026-10-05"}]})";

static const char* DISCOVER_TV = R"({"page":1,"results":[
 {"id":201,"name":"Neue Serie","first_air_date":"2026-10-28"},
 {"id":101,"name":"Gleiche ID wie Film","first_air_date":"2026-10-30"},
 {"id":202,"name":"Aaa Serie","first_air_date":"2026-10-28"}]})";

static const char* ITEM_PROVIDERS = R"({"id":101,"results":{
 "DE":{"link":"https://example.invalid","flatrate":[
   {"provider_id":8,"provider_name":"Netflix"},
   {"provider_id":9,"provider_name":"Amazon Prime Video"},
   {"provider_id":119,"provider_name":"Amazon Prime Video"},
   {"provider_id":999,"provider_name":"Nicht gewählt"}],
   "rent":[{"provider_id":337,"provider_name":"Disney Plus"}]},
 "US":{"flatrate":[{"provider_id":337,"provider_name":"Disney Plus"}]}}})";

static void test_dates() {
    CHECK(date_ymd(0) == "1970-01-01");
    CHECK(date_ymd(1792606500) == "2026-10-21");   // 21.10.2026 18:15 UTC
    CHECK(date_ymd(1793404800) == "2026-10-31");   // 31.10.2026 00:00 UTC
    CHECK(add_days(1793404800, -30) == "2026-10-01");
    CHECK(add_days(1793404800, 1) == "2026-11-01");
    CHECK(add_days(1709078400, 1) == "2024-02-29");   // 28.02.2024 + 1 Tag (Schaltjahr)
    CHECK(add_days(1709078400, 2) == "2024-03-01");
    CHECK(add_days(1767225600, -1) == "2025-12-31");   // 01.01.2026 - 1 Tag
    CHECK(format_date("2026-10-28", "de") == "28.10.");
    CHECK(format_date("2026-10-28", "en") == "10/28");
    CHECK(format_date("kaputt", "de").empty());
    CHECK(format_date("2026-13-01", "de").empty());
    CHECK(clamp_items(0) == 1 && clamp_items(99) == MAX_ITEMS && clamp_items(4) == 4);
    CHECK(clamp_refresh(5) == 60 && clamp_refresh(90) == 90 && clamp_refresh(9999) == 720);
}

static void test_region_and_key() {
    CHECK(region_for("de", "") == "DE");
    CHECK(region_for("en", "") == "US");
    CHECK(region_for("en", "gb") == "GB");        // Überschreibung gewinnt
    CHECK(region_for("de", "Deutschland") == "DE");   // ungültige Überschreibung → Automatik
    CHECK(normalize_region(" at ") == "AT");
    CHECK(normalize_region("1a").empty() && normalize_region("").empty());
    CHECK(key_kind("") == KeyKind::None);
    CHECK(key_kind("zu kurz") == KeyKind::None);
    CHECK(key_kind("0123456789abcdef0123456789abcdef") == KeyKind::V3);
    CHECK(key_kind("eyJhbGciOiJIUzI1NiJ9.eyJhdWQiOiJ4In0.abcdefghijklmnop") == KeyKind::Bearer);
    CHECK(key_kind("0123456789abcdef0123456789abcdef&x=1") == KeyKind::None);          // keine Einschleusung
    CHECK(key_kind("0123456789abcdef0123456789abcdef\r\nX: y") == KeyKind::None);
    const std::string v3 = "0123456789abcdef0123456789abcdef";
    CHECK(with_key("/discover/movie?a=b", v3) == "/discover/movie?a=b&api_key=" + v3);
    CHECK(with_key("/x", v3) == "/x?api_key=" + v3);
    const std::string bearer = "eyJhbGciOiJIUzI1NiJ9.eyJhdWQiOiJ4In0.abcdefghijklmnop";
    CHECK(with_key("/x?a=b", bearer) == "/x?a=b");   // Bearer nie in die URL
    bool has_auth = false;
    for (auto& [k, v] : auth_headers(bearer)) has_auth |= k == "Authorization" && v == "Bearer " + bearer;
    CHECK(has_auth);
    for (auto& [k, v] : auth_headers(v3)) { (void)v; CHECK(k != "Authorization"); }
}

static void test_providers() {
    auto list = parse_providers(PROVIDERS_JSON);
    CHECK(list && list->size() == 5);   // leere/kaputte Einträge fallen weg
    auto merged = merge_providers(*list, *parse_providers(R"({"results":[{"provider_name":"Netflix","provider_id":8},{"provider_name":"Hulu","provider_id":15}]})"));
    CHECK(merged.size() == 6);
    CHECK(merged.front().name == "Alpha Beta Gamma Delta");   // alphabetisch
    CHECK(!parse_providers("kein json") && !parse_providers("{}") && !parse_providers(R"({"results":{}})"));
    CHECK(abbreviation(8, "") == "NF" && abbreviation(9, "x") == "PV" && abbreviation(119, "") == "PV");
    CHECK(abbreviation(337, "") == "D+" && abbreviation(350, "") == "ATV+" && abbreviation(30, "") == "WOW" && abbreviation(298, "") == "RTL+" && abbreviation(2750, "") == "RTL+");
    CHECK(abbreviation(15, "Hulu") == "Hulu");
    CHECK(abbreviation(777, "Alpha Beta Gamma Delta") == "ABG");
    CHECK(abbreviation(531, "Paramount Plus") == "Par+");
    CHECK(abbreviation(42, "") == "#42");
    auto sel = parse_selection({"8|Netflix", " 337 ", "8|doppelt", "abc", "-1", "", "350|Apple TV+|x", "12345678"});
    CHECK(sel.size() == 3 && sel[0].id == 8 && sel[0].name == "Netflix" && sel[1].id == 337 && sel[2].id == 350);
    CHECK(parse_selection(default_provider_lines()).size() == 7);
    std::vector<std::string> many;
    for (int i = 1; i <= 50; ++i) many.push_back(std::to_string(i));
    CHECK((int)parse_selection(many).size() == MAX_PROVIDERS);
}

static void test_paths() {
    auto sel = parse_selection({"8", "9", "337"});
    const std::string p = discover_path(false, "DE", sel, "2026-10-01", "2026-10-31", "de");
    CHECK(p.rfind("/discover/movie?", 0) == 0);
    CHECK(p.find("watch_region=DE") != std::string::npos);
    CHECK(p.find("with_watch_providers=8%7C9%7C337") != std::string::npos);   // | getrennt (URL-kodiert)
    CHECK(p.find("with_watch_monetization_types=flatrate") != std::string::npos);
    CHECK(p.find("sort_by=primary_release_date.desc") != std::string::npos);
    CHECK(p.find("primary_release_date.gte=2026-10-01") != std::string::npos);
    CHECK(p.find("primary_release_date.lte=2026-10-31") != std::string::npos);
    CHECK(p.find("language=de-DE") != std::string::npos);
    CHECK(p.find("api_key") == std::string::npos);
    const std::string t = discover_path(true, "US", sel, "2026-10-01", "2026-10-31", "en");
    CHECK(t.rfind("/discover/tv?", 0) == 0);
    CHECK(t.find("sort_by=first_air_date.desc") != std::string::npos && t.find("first_air_date.lte=2026-10-31") != std::string::npos);
    CHECK(t.find("language=en-US") != std::string::npos);
    CHECK(providers_path(false, "DE", "de") == "/watch/providers/movie?watch_region=DE&language=de-DE");
    CHECK(providers_path(true, "US", "en").rfind("/watch/providers/tv?watch_region=US", 0) == 0);
    CHECK(item_providers_path(false, 101) == "/movie/101/watch/providers");
    CHECK(item_providers_path(true, 7) == "/tv/7/watch/providers");
}

static void test_items() {
    auto movies = parse_discover(DISCOVER_MOVIE, false, "2026-10-01", "2026-10-31");
    CHECK(movies && movies->size() == 2);   // zu alt, Zukunft, ohne Datum, ohne Titel entfallen
    CHECK((*movies)[0].title == "Neuer Film" && (*movies)[0].date == "2026-10-28" && !(*movies)[0].series);
    CHECK((*movies)[1].title == "Älterer Film");   // getrimmt
    auto tv = parse_discover(DISCOVER_TV, true, "2026-10-01", "2026-10-31");
    CHECK(tv && tv->size() == 3 && (*tv)[0].series);
    CHECK(!parse_discover("", false, "", "") && !parse_discover(R"({"status_code":7,"status_message":"Invalid API key"})", false, "", ""));
    auto all = merge_items(*movies, *tv, 10);
    CHECK(all.size() == 5);   // gleiche ID bei Film und Serie bleibt getrennt
    CHECK(all[0].id == 101 && all[0].series);   // 30.10. zuerst
    CHECK(all[1].title == "Aaa Serie" && all[2].title == "Neue Serie" && all[3].title == "Neuer Film");   // gleiches Datum: nach Titel
    CHECK(merge_items(*movies, *tv, 2).size() == 2);
    CHECK(merge_items({}, {}, 5).empty());
    auto dup = merge_items(*movies, *movies, 10);
    CHECK(dup.size() == 2);   // Doppelte nur einmal
}

static void test_fair_mix() {
    auto sel = parse_selection({"8|Netflix", "9|Prime Video", "119|Prime Video", "337|Disney+"});
    auto g = group_selection(sel);
    CHECK(g.size() == 3 && g[1].size() == 2);   // Prime 9/119 eine Gruppe
    auto many = group_selection(sel, 2);
    CHECK(many.size() == 2 && many[1].size() == 3);   // Rest in letzter Gruppe
    CHECK(group_selection({}).empty());
    auto mk = [](int id, const char* d) { Item i; i.id = id; i.title = "T" + std::to_string(id); i.date = d; return i; };
    std::vector<Item> nf = {mk(1, "2026-10-30"), mk(2, "2026-10-29"), mk(3, "2026-10-28"), mk(4, "2026-10-27")};
    std::vector<Item> dp = {mk(10, "2026-10-10")};
    std::vector<Item> pv = {mk(20, "2026-10-12"), mk(1, "2026-10-30")};
    auto r = fair_mix({nf, dp, pv}, 4);
    CHECK(r.size() == 4);
    CHECK(r[0].id == 1 && r[1].id == 2);   // neueste zuerst
    bool has10 = false, has20 = false;
    for (auto& i : r) { has10 |= i.id == 10; has20 |= i.id == 20; }
    CHECK(has10 && has20);   // kleinere Anbieter nicht verdrängt
    CHECK(fair_mix({nf}, 2).size() == 2 && fair_mix({}, 3).empty());
    CHECK(fair_mix({nf, dp}, 99).size() == 5);
}

static void test_matching() {
    auto sel = parse_selection({"8|Netflix", "9|Prime Video", "119|Prime Video", "337|Disney+"});
    auto de = match_providers(ITEM_PROVIDERS, "DE", sel);
    CHECK(de.size() == 2 && de[0] == "NF" && de[1] == "PV");   // 9 und 119 zählen als ein Kürzel, rent/999 nicht
    CHECK(match_providers(ITEM_PROVIDERS, "US", sel) == std::vector<std::string>{"D+"});
    CHECK(match_providers(ITEM_PROVIDERS, "FR", sel).empty());
    CHECK(match_providers("kein json", "DE", sel).empty());
    CHECK(match_providers(R"({"results":{"DE":{"rent":[]}}})", "DE", sel).empty());
    CHECK(match_providers(ITEM_PROVIDERS, "DE", {}).empty());
}

static const char* ITEM_LOGOS = R"({"id":101,"results":{"DE":{"flatrate":[
   {"provider_id":8,"provider_name":"Netflix","logo_path":"/pbpMk2JmcoNnQwx5JGpXngfoWtp.jpg"},
   {"provider_id":9,"provider_name":"Amazon Prime Video","logo_path":"/emthp39XA2YScoYL1p0sdbAH2WA.jpg"},
   {"provider_id":119,"provider_name":"Amazon Prime Video","logo_path":"/other.jpg"},
   {"provider_id":337,"provider_name":"Disney Plus"}]}}})";

static void test_logos() {
    auto m = match_provider_details(ITEM_LOGOS, "DE", parse_selection({"8|Netflix", "9|Prime Video", "119|Prime Video", "337|Disney+"}));
    CHECK(m.size() == 3);   // 9 und 119: ein Anbieter
    CHECK(m[0].id == 8 && m[0].abbr == "NF" && m[0].logo_path == "/pbpMk2JmcoNnQwx5JGpXngfoWtp.jpg");
    CHECK(m[1].id == 9 && m[2].id == 337 && m[2].logo_path.empty());
    CHECK(match_provider_details("kaputt", "DE", {}).empty());

    CHECK(logo_url("/pbpMk2JmcoNnQwx5JGpXngfoWtp.jpg") == "https://image.tmdb.org/t/p/w92/pbpMk2JmcoNnQwx5JGpXngfoWtp.jpg");
    CHECK(logo_url("/a-b_c.d/e.png") == "https://image.tmdb.org/t/p/w92/a-b_c.d/e.png");
    CHECK(!logo_url(""));
    CHECK(!logo_url("/"));
    CHECK(!logo_url("x.jpg"));                    // muss mit / beginnen
    CHECK(!logo_url("/../etc/x.jpg"));            // kein ..
    CHECK(!logo_url("/a/..b.jpg"));
    CHECK(!logo_url("//evil.example/x.jpg"));     // kein //
    CHECK(!logo_url("/x.gif"));                   // nur .jpg/.png
    CHECK(!logo_url("/x.jpg.exe"));
    CHECK(!logo_url("/.jpg"));
    CHECK(!logo_url("/x y.jpg"));
    CHECK(!logo_url("/x?a=1.jpg"));
    CHECK(!logo_url("/x%2e.jpg"));
    CHECK(!logo_url("/x\\y.jpg"));
    CHECK(!logo_url("/x:80.jpg"));
    CHECK(!logo_url("/x@h.jpg"));
    CHECK(!logo_url(std::string("/x\0y.jpg", 9)));
    CHECK(!logo_url("/" + std::string(MAX_LOGO_PATH, 'a') + ".jpg"));   // zu lang

    CHECK(logo_cache_name(8) == "8.png" && logo_cache_name(2100) == "2100.png");
    CHECK(logo_cache_name(0).empty() && logo_cache_name(-3).empty());

    CHECK(logos_shown(0) == 0 && logos_shown(2) == 2 && logos_shown(3) == 3 && logos_shown(7) == MAX_LOGOS);
    CHECK(logos_overflow(3).empty() && logos_overflow(0).empty());
    CHECK(logos_overflow(4) == "+1" && logos_overflow(7) == "+4");

    // Ecken abrunden: Mitte unverändert, Ecke transparent, Randpixel teilweise
    std::vector<uint32_t> px(20 * 20, 0xFF808080u);
    round_corners(px, 20, 20, 6.f);
    CHECK(px[10 * 20 + 10] == 0xFF808080u);
    CHECK((px[0] >> 24) == 0);
    CHECK((px[10] >> 24) == 0xFF);   // Kantenmitte bleibt deckend
    uint32_t edge = px[1 * 20 + 2];
    CHECK((edge >> 24) < 0xFF);
    CHECK(((edge >> 16) & 0xFF) <= (edge >> 24));   // vormultipliziert: Farbe nie heller als Alpha
    std::vector<uint32_t> small(4, 0xFFFFFFFFu);
    round_corners(small, 4, 4, 0.f);   // Radius 0: nichts ändern
    CHECK(small[0] == 0xFFFFFFFFu);
    round_corners(small, 0, 0, 5.f);   // ungültige Größe: kein Absturz
}

int main() {
    test_logos();
    test_dates();
    test_region_and_key();
    test_providers();
    test_paths();
    test_items();
    test_fair_mix();
    test_matching();
    std::printf("tmdb: %d ok, %d fehlgeschlagen\n", passed, failed);
    return failed ? 1 : 0;
}
