// © 2026 FuchsLabs
// Reine Logik der Seite „Streaming-Neustarts“ (TMDB API v3): Anfragen bauen, Antworten lesen, Einträge auswählen.
// Ohne Netz und ohne Windows-Abhängigkeiten, damit sie mit festen JSON-Beispielen unter Linux testbar ist
// (tests/tmdb_tests.cpp). Der Abruf selbst steht in tmdb.cpp.
// Hinweis: This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.
#pragma once

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fl::tmdb {

inline constexpr const char* API_HOST = "https://api.themoviedb.org/3";
inline constexpr int WINDOW_DAYS = 30;       // Neustarts der letzten 30 Tage
inline constexpr int MAX_ITEMS = 6;          // so viele Zeilen passen lesbar aufs Display
inline constexpr int MAX_PROVIDERS = 30;
inline constexpr int MIN_REFRESH_MINUTES = 60;   // Cache 1 h, schont das Kontingent
inline constexpr const char* LOGO_HOST = "https://image.tmdb.org/t/p/w92";   // Anbieter-Logos, nur GET über HTTPS
inline constexpr int MAX_LOGOS = 3;          // höchstens so viele Logos je Titel, der Rest als „+n“
inline constexpr size_t MAX_LOGO_PATH = 100;

// Eintrag der Seite
struct Item {
    int id = 0;
    std::string title;
    std::string date;                    // YYYY-MM-DD
    bool series = false;                 // false = Film
    std::vector<std::string> providers;  // Kürzel der gewählten Anbieter, bei denen er im Abo läuft
    std::vector<int> provider_ids;       // je Kürzel die TMDB-Provider-ID (gleiche Reihenfolge), für das Logo
};

// Treffer aus /…/watch/providers: ein gewählter Anbieter, bei dem der Titel im Abo läuft
struct ProviderMatch {
    int id = 0;
    std::string abbr;        // Kürzel (Rückfall, wenn das Logo fehlt)
    std::string logo_path;   // unvalidierter Fremdwert von TMDB; erst über logo_url() verwenden
};

struct Provider {
    int id = 0;
    std::string name;
};

// Anbieter aus der Konfiguration: Zeile „8|Netflix“ (Name optional)
struct Selected {
    int id = 0;
    std::string name;
};

enum class State { NoKey, Loading, Error, Ok };   // Zustand der Seite: „Kein Key“, „Lade …“, „Keine Daten“, Liste

enum class KeyKind { None, V3, Bearer };

// ---- Schlüssel ----
KeyKind key_kind(const std::string& key);   // None = leer oder unzulässige Zeichen (nie in eine Anfrage übernehmen)

// ---- Region, Anbieter ----
std::string normalize_region(const std::string& text);                      // „de“ → „DE“; ungültig → leer
std::string region_for(const std::string& lang, const std::string& override_region);   // de → DE, en → US
std::vector<Selected> parse_selection(const std::vector<std::string>& lines);           // ungültige/doppelte fallen weg
std::string abbreviation(int id, const std::string& name);                              // „NF“, „D+“ …
std::vector<std::string> default_provider_lines();                                      // Standardvorauswahl

// Antwort von /watch/providers/{movie|tv} → Anbieter (nullopt = kein gültiges Ergebnis)
std::optional<std::vector<Provider>> parse_providers(const std::string& json);
std::vector<Provider> merge_providers(std::vector<Provider> a, const std::vector<Provider>& b);   // je ID einmal, sortiert

// ---- Anfragen (Pfad mit Abfrage; der Schlüssel kommt erst über with_key dazu) ----
std::string providers_path(bool series, const std::string& region, const std::string& lang);
std::string discover_path(bool series, const std::string& region, const std::vector<Selected>& providers,
                          const std::string& from, const std::string& to, const std::string& lang);
std::string item_providers_path(bool series, int id);                    // /movie/{id}/watch/providers
std::string with_key(const std::string& path, const std::string& key);   // v3: &api_key=…; Bearer: unverändert
std::vector<std::pair<std::string, std::string>> auth_headers(const std::string& key);   // Bearer: Authorization

// ---- Antworten ----
std::optional<std::vector<Item>> parse_discover(const std::string& json, bool series, const std::string& from,
                                                const std::string& to);
// Filme und Serien mischen: neueste zuerst, doppelte (Art + ID) einmal, höchstens max
std::vector<Item> merge_items(std::vector<Item> movies, std::vector<Item> series, size_t max);
// Aus /…/watch/providers: Kürzel der gewählten Anbieter, die in der Region im Abo (flatrate) laufen
std::vector<std::string> match_providers(const std::string& json, const std::string& region, const std::vector<Selected>& sel);
// Wie match_providers, aber mit ID und logo_path; je Kürzel einmal (Prime Video 9/119 zählt als ein Anbieter)
std::vector<ProviderMatch> match_provider_details(const std::string& json, const std::string& region, const std::vector<Selected>& sel);

// ---- Anbieter-Logos ----
// URL aus dem logo_path von TMDB; nullopt bei allem, was nicht sicher ist: nur „/“ + [A-Za-z0-9_./-],
// Endung .jpg oder .png, kein „..“, kein „//“, höchstens MAX_LOGO_PATH Zeichen
std::optional<std::string> logo_url(const std::string& logo_path);
// Dateiname im Cache: nur aus der numerischen Provider-ID („8.png“); leer bei ungültiger ID
std::string logo_cache_name(int provider_id);
// Anzahl der Logos, die gezeigt werden (höchstens MAX_LOGOS), und Text „+n“ für den Rest (leer, wenn keiner fehlt)
int logos_shown(size_t count);
std::string logos_overflow(size_t count);
// Ecken eines Bildes abrunden (vormultipliziertes 0xAARRGGBB, w × h Pixel); radius in Pixeln, Kante geglättet
void round_corners(std::vector<uint32_t>& px, int w, int h, float radius);

// ---- Datum ----
std::string date_ymd(std::time_t utc);                        // „2026-10-31“
std::string add_days(std::time_t utc, int days);              // Datum von utc + days Tagen
std::string format_date(const std::string& ymd, const std::string& lang);   // de „31.10.“, en „10/31“
int clamp_items(long long n);                                  // 1 … MAX_ITEMS
int clamp_refresh(long long minutes);                          // mindestens 60

}  // namespace fl::tmdb
