// © 2026 FuchsLabs – PlayStation-Freunde: Antworten lesen mit festen JSON-Beispielen; kein Netzzugriff.
#include "psn_logic.h"

#include <cstdio>
#include <string>

using namespace fl::psn;
using nlohmann::json;
static int passed = 0, failed = 0;
#define CHECK(condition) do { if (condition) ++passed; else { ++failed; \
    std::printf("FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

int main() {
    // NPSSO
    const std::string code(64, 'a');
    CHECK(valid_npsso(code));
    CHECK(!valid_npsso(code.substr(1)));
    CHECK(!valid_npsso(std::string(63, 'a') + "-"));
    CHECK(clean_npsso("  " + code + "\n") == code);
    CHECK(clean_npsso("{\"npsso\":\"" + code + "\"}") == code);
    CHECK(clean_npsso("{ \"npsso\" : \"" + code + "\" }") == code);
    CHECK(clean_npsso("\"" + code + "\"") == code);

    // Freunde: nur Ziffern-IDs, Obergrenze
    auto ids = parse_friend_ids(json::parse(R"({"friends":["111","222",333,"x1","",{"a":1}],"totalItemCount":6})"));
    CHECK(ids.size() == 3 && ids[0] == "111" && ids[2] == "333");
    CHECK(parse_friend_ids(json::parse("[]")).empty());
    CHECK(parse_friend_ids(json::parse(R"({"friends":"nein"})")).empty());
    json many = {{"friends", json::array()}};
    for (int i = 0; i < 500; ++i) many["friends"].push_back(std::to_string(1000 + i));
    CHECK(parse_friend_ids(many).size() == MAX_FRIENDS);

    // Presence: Sammelabruf und Einzelabruf
    auto pr = parse_presences(json::parse(R"({"basicPresences":[
      {"accountId":"111","availability":"availableToPlay","primaryPlatformInfo":{"onlineStatus":"online","platform":"PS5"},
       "gameTitleInfoList":[{"titleName":"Astro Bot","format":"PS5"}]},
      {"accountId":"222","primaryPlatformInfo":{"onlineStatus":"offline","platform":"PS4","lastOnlineDate":"2026-10-30T10:00:00Z"},
       "gameTitleInfoList":[{"titleName":"Alt"}]},
      {"accountId":"333","primaryPlatformInfo":{"onlineStatus":"online","platform":"PS4"}},
      {"accountId":"444"}]})"));
    CHECK(pr.size() == 4);
    CHECK(pr[0].online && pr[0].platform == "PS5" && pr[0].game == "Astro Bot");
    CHECK(!pr[1].online && pr[1].game.empty());   // Offline: kein Spiel anzeigen
    CHECK(pr[2].online && pr[2].game.empty());
    CHECK(!pr[3].online);
    auto one = parse_presences(json::parse(R"({"basicPresence":{"primaryPlatformInfo":{"onlineStatus":"online","platform":"PS5"}}})"));
    CHECK(one.size() == 1 && one[0].online);
    CHECK(parse_presences(json::parse("null")).empty());

    // Profil
    Profile p;
    CHECK(parse_profile(json::parse(R"({"profile":{"onlineId":"Fuchs_84","avatarUrls":[
      {"size":"xl","avatarUrl":"https://psn-rsc.prod.dl.playstation.net/xl.png"},
      {"size":"m","avatarUrl":"https://static-resource.np.community.playstation.net/m.png"}]}})"), p));
    CHECK(p.online_id == "Fuchs_84" && p.avatar_url.find("/m.png") != std::string::npos);
    CHECK(parse_profile(json::parse(R"({"profile":{"onlineId":"X","avatarUrls":[{"size":"m","avatarUrl":"http://evil.example/a.png"}]}})"), p));
    CHECK(p.avatar_url.empty());   // keine fremden oder unverschlüsselten Bild-Adressen
    CHECK(!parse_profile(json::parse(R"({"profile":{}})"), p));
    CHECK(!parse_profile(json::parse("[]"), p));

    // Bild-Adressen
    CHECK(trusted_avatar_url("https://psn-rsc.prod.dl.playstation.net/a.png"));
    CHECK(trusted_avatar_url("https://Image.API.PlayStation.com/a.png"));
    CHECK(!trusted_avatar_url("http://psn-rsc.prod.dl.playstation.net/a.png"));
    CHECK(!trusted_avatar_url("https://playstation.net.evil.example/a.png"));
    CHECK(!trusted_avatar_url("https://evil.example/.playstation.net/a.png"));
    CHECK(!trusted_avatar_url("https://x.playstation.net@evil.example/a.png"));
    CHECK(!trusted_avatar_url("https://evilplaystation.net/a.png"));
    CHECK(!trusted_avatar_url(""));

    // Weiterleitung nach der Anmeldung
    CHECK(code_from_location("com.scee.psxandroid.scecompcall://redirect/?code=Abc123&cid=1") == "Abc123");
    CHECK(code_from_location("https://x/?error=access_denied").empty());
    CHECK(code_from_location("https://x/?xcode=1").empty());
    CHECK(code_from_location("https://x/?code=a%20b").empty());

    std::printf("PSN: %d bestanden, %d fehlgeschlagen\n", passed, failed);
    return failed ? 1 : 0;
}
