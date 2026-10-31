// © 2026 FuchsLabs – Prüft die Tabelle der Schlüssel-Hilfen (offizielle https-Links, DE/EN vollständig, keine Geheimnisse).
#include <cstdio>
#include <cstdlib>
#include <regex>

#include "key_help.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

int main() {
    using namespace fl::keyhelp;
    for (const char* id : {"discord", "steam", "xbox", "tmdb", "spotify", "homeassistant"}) {
        const Help* h = find(id);
        CHECK(h != nullptr);
        if (!h) continue;
        CHECK(!h->title_de.empty() && !h->title_en.empty());
        CHECK(h->steps_de.size() >= 3);
        CHECK(h->steps_de.size() == h->steps_en.size());
        CHECK(!h->links.empty());
        for (const auto& l : h->links) {
            CHECK(is_official_url(l.url));
            CHECK(!l.de.empty() && !l.en.empty());
        }
    }
    CHECK(find("unbekannt") == nullptr);
    CHECK(all().size() == 6);
    // Nur Adressen aus der Erlaubnisliste, nur https
    CHECK(!is_official_url("http://discord.com/developers"));
    CHECK(!is_official_url("https://discord.com.evil.example/"));
    CHECK(!is_official_url("https://evil.example/discord.com"));
    CHECK(!is_official_url("https://discord.com@evil.example/"));
    CHECK(!is_official_url("https://discord.com:8443/"));
    CHECK(!is_official_url("javascript:alert(1)"));
    CHECK(!is_official_url("https://discord.com/a b"));
    CHECK(!is_official_url(""));
    CHECK(is_official_url("https://Discord.com/developers/applications"));
    // Kein Text enthält etwas, das wie ein Schlüssel aussieht (32+ Zeichen Hex/Base64 am Stück)
    std::regex secret("[A-Za-z0-9_\\-]{32,}");
    for (const auto& h : all())
        for (const auto& l : h.links) CHECK(!std::regex_search(l.url.substr(8), secret));
    std::printf(fails ? "key-help: %d Fehler\n" : "key-help: OK\n", fails);
    return fails ? 1 : 0;
}
