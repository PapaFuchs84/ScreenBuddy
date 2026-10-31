// © 2026 FuchsLabs
// Reine Logik der PlayStation-Freunde: Antworten von Sonys (inoffizieller) Schnittstelle lesen.
// Ohne Netz und ohne Windows-Abhängigkeiten, damit sie mit festen JSON-Beispielen unter Linux testbar ist
// (tests/psn_tests.cpp). Die Abrufe selbst stehen in friends.cpp.
// Hinweis: Sony bietet keine offizielle Schnittstelle für Freunde. Der NPSSO-Wert gilt wie ein Passwort.
#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fl::psn {

inline constexpr const char* API_BASE = "https://m.np.playstation.com/api";
inline constexpr const char* AUTH_BASE = "https://ca.account.sony.com/api/authz/v3/oauth";
inline constexpr size_t MAX_FRIENDS = 200;       // so viele Freunde werden höchstens abgerufen
inline constexpr size_t PRESENCE_CHUNK = 50;     // Konten je Presence-Abruf

struct Presence {
    std::string account_id;
    bool online = false;
    std::string platform;  // z. B. PS5
    std::string game;      // leer = kein Spiel
};

struct Profile {
    std::string online_id, avatar_url;
};

// NPSSO: 64 Zeichen aus Buchstaben und Ziffern. Erkennt auch die ganze JSON-Antwort von ssocookie ({"npsso":"…"}).
std::string clean_npsso(const std::string& input);
bool valid_npsso(const std::string& s);

std::vector<std::string> parse_friend_ids(const nlohmann::json& data);
std::vector<Presence> parse_presences(const nlohmann::json& data);   // Sammelabruf ("basicPresences") oder Einzelabruf
bool parse_profile(const nlohmann::json& data, Profile& out);
bool trusted_avatar_url(const std::string& url);   // https und Sony-Domain

// Autorisierungscode aus der Weiterleitung (Location-Header) lesen; leer = nicht gefunden
std::string code_from_location(const std::string& location);

}  // namespace fl::psn
