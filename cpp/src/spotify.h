// © 2026 FuchsLabs
// Spotify: aktuellen Titel samt Cover anzeigen und per Tipp steuern (Spotify Web API).
// Anmeldung per Authorization Code mit PKCE (kein Client-Secret). Jede Person trägt die Client-ID ihrer eigenen
// Spotify-Developer-App ein. Die Tokens liegen DPAPI-verschlüsselt in einer eigenen Datei neben der Konfiguration,
// nie in der config.toml. Steuern (Play/Pause/Weiter/Zurück) verlangt Spotify Premium, die Anzeige nicht.
#pragma once

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

#include "config.h"
#include "gfx.h"

namespace fl::spotify {

struct SpotifyError : std::runtime_error {  // Meldung, die der Nutzer versteht
    using std::runtime_error::runtime_error;
};

enum class State { NotSetUp, LoggedOut, Connecting, Idle, Playing, Paused, Error };

struct Snapshot {
    State state = State::NotSetUp;
    std::string title, artist, album, error;
    long long progress_ms = 0, duration_ms = 0;
    double fetched_at = 0;      // mono()-Zeitpunkt der Abfrage, für die laufende Fortschrittsanzeige
    bool playing = false;
    bool controls = false;      // Tippen steuert die Wiedergabe (Touch an)
    std::shared_ptr<const Image> cover;
};

// ---------- reine Funktionen (testbar) ----------
std::string base64url(const std::string& bytes);
std::string make_verifier();                                 // 64 Zeichen aus dem erlaubten PKCE-Alphabet
std::string challenge_of(const std::string& verifier);      // BASE64URL(SHA256(verifier))
std::string redirect_uri(int ui_port);                       // http://127.0.0.1:<Port>/spotify/callback
std::string authorize_url(const std::string& client_id, const std::string& redirect, const std::string& challenge,
                          const std::string& state);
bool valid_client_id(const std::string& id);                 // 32 Hexzeichen
Snapshot parse_player(const nlohmann::json& data, bool controls);  // /v1/me/player; kein Objekt = nichts aktiv
std::string cover_url(const nlohmann::json& data);          // kleinstes Cover ab 200 px, nur https://i.scdn.co
std::string format_time(long long ms);                       // "3:07"
long long live_progress(const Snapshot& s, double now);      // Fortschritt mit Hochrechnung, in [0, Dauer]

class SpotifyClient {
public:
    SpotifyClient(const ojson& cfg, int ui_port, bool touch, std::filesystem::path token_file, std::function<void()> on_change);
    ~SpotifyClient();
    void start();
    void stop();

    bool configured() const { return !client_id_.empty(); }
    Snapshot snapshot();
    bool logged_in();
    ojson info();                                        // ok, error – für die Verbindungsanzeige
    std::string redirect() const { return redirect_uri(ui_port_); }

    // Anmeldung: gibt die Adresse für den Browser zurück; der Browser kehrt zu /spotify/callback zurück.
    std::string begin_login();
    // Rückkehr von Spotify; Fehlertext, leer = angemeldet
    std::string finish_login(const std::string& code, const std::string& state, const std::string& error);
    void logout();

    enum class Command { PlayPause, Next, Previous };
    void request_command(Command c);                     // asynchron

private:
    void loop();
    void poll();
    void run_commands();
    bool ensure_token(bool force = false);               // false = nicht angemeldet / abgelaufen
    void token_request(const std::vector<std::pair<std::string, std::string>>& form);
    void load_tokens();
    void save_tokens();
    void set_error(const std::string& e);
    bool sleep_until_wake(double seconds);
    void fetch_cover(const std::string& url);

    std::string client_id_;
    int ui_port_;
    bool touch_;
    std::filesystem::path token_file_;
    std::function<void()> on_change_;

    std::mutex mu_;
    std::condition_variable cv_;
    std::thread th_;
    std::atomic<bool> stop_{false};
    std::string access_, refresh_, verifier_, login_state_, error_, cover_for_;
    double access_until_ = 0, login_until_ = 0, backoff_until_ = 0;
    Snapshot snap_;
    bool had_reply_ = false;
    std::vector<Command> pending_;
};

}  // namespace fl::spotify
