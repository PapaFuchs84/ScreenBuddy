// © 2026 FuchsLabs
#include "spotify.h"

#include <windows.h>
#include <shellapi.h>

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include "http.h"
#include "i18n.h"
#include "secure.h"
#include "util.h"

namespace fl::spotify {

namespace {

constexpr const char* TOKEN_URL = "https://accounts.spotify.com/api/token";
constexpr const char* AUTH_URL = "https://accounts.spotify.com/authorize";
constexpr const char* API = "https://api.spotify.com/v1";
constexpr const char* SCOPES = "user-read-playback-state user-modify-playback-state user-read-currently-playing";
constexpr const char* ENTROPY = "screenbuddy-spotify-v1";
constexpr const char* COVER_HOST = "https://i.scdn.co/";
constexpr int TIMEOUT_MS = 8000;
constexpr double PLAYING_POLL_S = 4, IDLE_POLL_S = 8, LOGIN_VALID_S = 600;

using Headers = std::vector<std::pair<std::string, std::string>>;

Headers bearer(const std::string& token) {
    return {{"Authorization", "Bearer " + token}, {"Accept", "application/json"}, {"User-Agent", UA}};
}

std::string str_of(const nlohmann::json& j, const char* key) {
    return j.is_object() && j.contains(key) && j[key].is_string() ? j[key].get<std::string>() : std::string();
}

long long int_of(const nlohmann::json& j, const char* key) {
    return j.is_object() && j.contains(key) && j[key].is_number() ? j[key].get<long long>() : 0;
}

void open_in_browser(const std::string& url) {
    // Nur die feste Spotify-Anmeldeadresse wird geöffnet
    if (url.rfind(AUTH_URL, 0) != 0) return;
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}  // namespace

// ---------- reine Funktionen ----------
std::string base64url(const std::string& bytes) {
    std::string s = secure::base64_encode(bytes);
    for (char& c : s) c = c == '+' ? '-' : c == '/' ? '_' : c;
    while (!s.empty() && s.back() == '=') s.pop_back();
    return s;
}

std::string make_verifier() {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    unsigned char raw[64];
    if (RAND_bytes(raw, sizeof raw) != 1) throw SpotifyError("Zufallszahlen nicht verfügbar");
    std::string out;
    for (unsigned char b : raw) out += alphabet[b % (sizeof alphabet - 1)];
    return out;
}

std::string challenge_of(const std::string& verifier) {
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(verifier.data()), verifier.size(), md);
    return base64url(std::string(reinterpret_cast<char*>(md), sizeof md));
}

std::string redirect_uri(int ui_port) { return "http://127.0.0.1:" + std::to_string(ui_port) + "/spotify/callback"; }

std::string authorize_url(const std::string& client_id, const std::string& redirect, const std::string& challenge,
                          const std::string& state) {
    return std::string(AUTH_URL) + "?" +
           http::query({{"client_id", client_id}, {"response_type", "code"}, {"redirect_uri", redirect},
                        {"code_challenge_method", "S256"}, {"code_challenge", challenge}, {"state", state},
                        {"scope", SCOPES}});
}

bool valid_client_id(const std::string& id) {
    return id.size() == 32 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::string cover_url(const nlohmann::json& data) {
    if (!data.is_object() || !data.contains("item") || !data["item"].is_object()) return "";
    const auto& item = data["item"];
    const nlohmann::json* images = nullptr;
    if (item.contains("album") && item["album"].is_object() && item["album"].contains("images")) images = &item["album"]["images"];
    else if (item.contains("images")) images = &item["images"];
    if (!images || !images->is_array()) return "";
    std::string best;
    long long best_w = 0;
    for (const auto& im : *images) {
        std::string url = str_of(im, "url");
        if (url.rfind(COVER_HOST, 0) != 0) continue;   // nur Spotifys Bild-Server
        long long w = int_of(im, "width");
        bool better = best.empty() || (w >= 200 && (best_w < 200 || w < best_w)) || (best_w < 200 && w > best_w);
        if (better) { best = url; best_w = w; }
    }
    return best;
}

Snapshot parse_player(const nlohmann::json& data, bool controls) {
    Snapshot s;
    s.controls = controls;
    if (!data.is_object()) { s.state = State::Idle; return s; }
    s.playing = data.contains("is_playing") && data["is_playing"].is_boolean() && data["is_playing"].get<bool>();
    s.progress_ms = std::max(0LL, int_of(data, "progress_ms"));
    s.state = s.playing ? State::Playing : State::Paused;
    if (data.contains("item") && data["item"].is_object()) {
        const auto& item = data["item"];
        s.title = str_of(item, "name");
        s.duration_ms = std::max(0LL, int_of(item, "duration_ms"));
        if (item.contains("artists") && item["artists"].is_array()) {
            for (const auto& a : item["artists"]) {
                std::string n = str_of(a, "name");
                if (n.empty()) continue;
                s.artist += (s.artist.empty() ? "" : ", ") + n;
            }
        }
        if (item.contains("album") && item["album"].is_object()) s.album = str_of(item["album"], "name");
        if (s.artist.empty() && item.contains("show") && item["show"].is_object()) {   // Podcast-Folge
            s.artist = str_of(item["show"], "name");
            s.album = str_of(item["show"], "publisher");
        }
    } else {
        s.title = "Werbung";   // z. B. Anzeige: kein Titel im Antwortobjekt
    }
    return s;
}

std::string format_time(long long ms) {
    long long total = std::max(0LL, ms) / 1000;
    if (total >= 3600) return strprintf("%lld:%02lld:%02lld", total / 3600, total / 60 % 60, total % 60);
    return strprintf("%lld:%02lld", total / 60, total % 60);
}

long long live_progress(const Snapshot& s, double now) {
    long long p = s.progress_ms;
    if (s.playing && s.fetched_at > 0) p += (long long)((now - s.fetched_at) * 1000);
    if (s.duration_ms > 0) p = std::min(p, s.duration_ms);
    return std::max(0LL, p);
}

// ---------- Client ----------
SpotifyClient::SpotifyClient(const ojson& cfg, int ui_port, bool touch, std::filesystem::path token_file,
                             std::function<void()> on_change)
    : ui_port_(ui_port), touch_(touch), token_file_(std::move(token_file)), on_change_(std::move(on_change)) {
    std::string id = trim(str_of(cfg, "client_id"));
    if (valid_client_id(id)) client_id_ = id;
    snap_.controls = touch_;
    if (client_id_.empty()) {
        snap_.state = State::NotSetUp;
        error_ = "Spotify ist nicht fertig eingerichtet";
        return;
    }
    load_tokens();
    snap_.state = refresh_.empty() ? State::LoggedOut : State::Connecting;
}

SpotifyClient::~SpotifyClient() { stop(); }

void SpotifyClient::start() {
    if (client_id_.empty()) return;
    th_ = std::thread([this] { loop(); });
}

void SpotifyClient::stop() {
    stop_ = true;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

Snapshot SpotifyClient::snapshot() {
    std::lock_guard lk(mu_);
    Snapshot s = snap_;
    s.error = error_;
    return s;
}

bool SpotifyClient::logged_in() {
    std::lock_guard lk(mu_);
    return !refresh_.empty();
}

ojson SpotifyClient::info() {
    std::lock_guard lk(mu_);
    return {{"ok", error_.empty() && !refresh_.empty() && had_reply_}, {"logged_in", !refresh_.empty()}, {"error", error_}};
}

void SpotifyClient::set_error(const std::string& e) {
    std::lock_guard lk(mu_);
    error_ = e;
}

void SpotifyClient::load_tokens() {
    std::ifstream f(token_file_, std::ios::binary);
    if (!f) return;
    std::stringstream ss;
    ss << f.rdbuf();
    auto j = nlohmann::json::parse(secure::unprotect(ss.str(), ENTROPY), nullptr, false);
    if (!j.is_object() || str_of(j, "client_id") != client_id_) return;   // andere App → neu anmelden
    refresh_ = str_of(j, "refresh");
}

void SpotifyClient::save_tokens() {
    std::string text;
    {
        std::lock_guard lk(mu_);
        if (refresh_.empty()) {
            std::error_code ec;
            std::filesystem::remove(token_file_, ec);
            return;
        }
        text = secure::protect(nlohmann::json{{"client_id", client_id_}, {"refresh", refresh_}}.dump(), ENTROPY);
    }
    std::ofstream f(token_file_, std::ios::binary | std::ios::trunc);
    f << text;
}

void SpotifyClient::token_request(const Headers& form) {
    auto r = http::fetch({"POST", TOKEN_URL,
                          {{"Content-Type", "application/x-www-form-urlencoded"}, {"Accept", "application/json"}, {"User-Agent", UA}},
                          http::query(form), TIMEOUT_MS, 200'000});
    if (r.status == 0) throw SpotifyError("Spotify ist nicht erreichbar – Netzwerk prüfen");
    auto j = nlohmann::json::parse(r.body, nullptr, false);
    if (r.status != 200 || !j.is_object() || str_of(j, "access_token").empty()) {
        std::string err = str_of(j, "error");
        if (err == "invalid_grant" || r.status == 400 || r.status == 401) {
            std::lock_guard lk(mu_);
            if (form.size() && form[0].second == "refresh_token") refresh_.clear();   // widerrufen → neu anmelden
            throw SpotifyError(err == "invalid_client" ? "Spotify: Client-ID ungültig" : "Spotify: Anmeldung abgelaufen – bitte neu anmelden");
        }
        throw SpotifyError(strprintf("Spotify antwortet mit Fehler %d", r.status));
    }
    std::lock_guard lk(mu_);
    access_ = str_of(j, "access_token");
    long long ttl = std::max(60LL, int_of(j, "expires_in"));
    access_until_ = mono() + (double)ttl - 30;
    if (!str_of(j, "refresh_token").empty()) refresh_ = str_of(j, "refresh_token");
}

bool SpotifyClient::ensure_token(bool force) {
    std::string refresh;
    {
        std::lock_guard lk(mu_);
        if (refresh_.empty()) return false;
        if (!force && !access_.empty() && mono() < access_until_) return true;
        refresh = refresh_;
    }
    try {
        token_request({{"grant_type", "refresh_token"}, {"refresh_token", refresh}, {"client_id", client_id_}});
    } catch (const SpotifyError& e) {
        set_error(e.what());
        if (!logged_in()) save_tokens();
        return false;
    }
    save_tokens();
    return true;
}

std::string SpotifyClient::begin_login() {
    if (client_id_.empty()) throw SpotifyError("Bitte zuerst die Client-ID eintragen und speichern");
    std::string verifier = make_verifier(), state = make_verifier().substr(0, 32);
    {
        std::lock_guard lk(mu_);
        verifier_ = verifier;
        login_state_ = state;
        login_until_ = mono() + LOGIN_VALID_S;
    }
    std::string url = authorize_url(client_id_, redirect_uri(ui_port_), challenge_of(verifier), state);
    open_in_browser(url);
    return url;
}

std::string SpotifyClient::finish_login(const std::string& code, const std::string& state, const std::string& error) {
    std::string verifier;
    {
        std::lock_guard lk(mu_);
        bool ok = !login_state_.empty() && state == login_state_ && mono() < login_until_;
        verifier = verifier_;
        login_state_.clear();
        verifier_.clear();
        if (!ok) return "Anmeldung abgelaufen oder ungültig – bitte erneut anmelden";
    }
    if (!error.empty() || code.empty()) return error == "access_denied" ? "Zugriff abgelehnt" : "Anmeldung abgebrochen";
    try {
        token_request({{"grant_type", "authorization_code"}, {"code", code}, {"redirect_uri", redirect_uri(ui_port_)},
                       {"client_id", client_id_}, {"code_verifier", verifier}});
    } catch (const SpotifyError& e) {
        return e.what();
    }
    save_tokens();
    {
        std::lock_guard lk(mu_);
        error_.clear();
        snap_.state = State::Connecting;
    }
    cv_.notify_all();
    if (on_change_) on_change_();
    return "";
}

void SpotifyClient::logout() {
    {
        std::lock_guard lk(mu_);
        refresh_.clear();
        access_.clear();
        error_.clear();
        had_reply_ = false;
        snap_ = Snapshot{};
        snap_.state = State::LoggedOut;
        snap_.controls = touch_;
        cover_for_.clear();
    }
    save_tokens();
    if (on_change_) on_change_();
}

void SpotifyClient::request_command(Command c) {
    {
        std::lock_guard lk(mu_);
        if (!touch_ || refresh_.empty() || pending_.size() >= 4) return;
        pending_.push_back(c);
    }
    cv_.notify_all();
}

bool SpotifyClient::sleep_until_wake(double seconds) {
    std::unique_lock lk(mu_);
    cv_.wait_for(lk, std::chrono::duration<double>(seconds), [&] { return stop_ || !pending_.empty(); });
    return !stop_;
}

void SpotifyClient::fetch_cover(const std::string& url) {
    {
        std::lock_guard lk(mu_);
        if (url == cover_for_) return;
    }
    if (url.empty()) {
        std::lock_guard lk(mu_);
        cover_for_.clear();
        snap_.cover.reset();
        return;
    }
    auto r = http::fetch({"GET", url, {{"User-Agent", UA}}, "", TIMEOUT_MS, 2'000'000});
    if (r.status != 200 || r.truncated) return;
    auto img = decode_image(r.body);
    if (!img || img->empty()) return;
    auto thumb = std::make_shared<const Image>(thumbnail(*img, 320, 320));
    std::lock_guard lk(mu_);
    cover_for_ = url;
    snap_.cover = thumb;
}

void SpotifyClient::poll() {
    if (!ensure_token()) {
        std::lock_guard lk(mu_);
        if (refresh_.empty()) { snap_.state = State::LoggedOut; snap_.cover.reset(); cover_for_.clear(); }
        else snap_.state = State::Error;
        return;
    }
    std::string token;
    {
        std::lock_guard lk(mu_);
        token = access_;
    }
    auto r = http::fetch({"GET", std::string(API) + "/me/player", bearer(token), "", TIMEOUT_MS, 1'000'000});
    if (r.status == 401 && ensure_token(true)) {   // Token früher abgelaufen: einmal erneuern und wiederholen
        {
            std::lock_guard lk(mu_);
            token = access_;
        }
        r = http::fetch({"GET", std::string(API) + "/me/player", bearer(token), "", TIMEOUT_MS, 1'000'000});
    }
    if (r.status == 0) { set_error("Spotify ist nicht erreichbar – Netzwerk prüfen"); return; }
    if (r.status == 429) {
        set_error("Spotify bremst die Abfragen – kurze Pause");
        std::lock_guard lk(mu_);
        backoff_until_ = mono() + 30;
        return;
    }
    if (r.status == 401 || r.status == 403) { set_error("Spotify: Zugriff verweigert – bitte neu anmelden"); return; }
    if (r.status >= 500) { set_error(strprintf("Spotify antwortet mit Fehler %d", r.status)); return; }
    if (r.status != 200 && r.status != 204) { set_error(strprintf("Spotify antwortet mit Fehler %d", r.status)); return; }

    Snapshot s;
    std::string cover;
    if (r.status == 204 || r.body.empty()) {
        s = parse_player(nlohmann::json(), touch_);
    } else {
        auto j = nlohmann::json::parse(r.body, nullptr, false);
        if (!j.is_object()) { set_error("Spotify: unerwartete Antwort"); return; }
        s = parse_player(j, touch_);
        cover = cover_url(j);
    }
    s.fetched_at = mono();
    bool changed;
    {
        std::lock_guard lk(mu_);
        changed = s.state != snap_.state || s.title != snap_.title || s.playing != snap_.playing || !error_.empty();
        s.cover = snap_.cover;
        snap_ = s;
        error_.clear();
        had_reply_ = true;
    }
    if (s.state == State::Idle || cover.empty()) fetch_cover("");
    else fetch_cover(cover);
    if (changed && on_change_) on_change_();
}

void SpotifyClient::run_commands() {
    std::vector<Command> todo;
    bool playing;
    std::string token;
    {
        std::lock_guard lk(mu_);
        todo.swap(pending_);
        playing = snap_.playing;
        token = access_;
    }
    for (Command c : todo) {
        if (!ensure_token()) return;
        {
            std::lock_guard lk(mu_);
            token = access_;
        }
        std::string method = "POST", path = "/me/player/next";
        if (c == Command::Previous) path = "/me/player/previous";
        else if (c == Command::PlayPause) { method = "PUT"; path = playing ? "/me/player/pause" : "/me/player/play"; playing = !playing; }
        auto r = http::fetch({method, std::string(API) + path, bearer(token), "", TIMEOUT_MS, 100'000});
        if (r.status == 0) { set_error("Spotify ist nicht erreichbar – Netzwerk prüfen"); return; }
        if (r.status == 403) { set_error("Steuern braucht Spotify Premium"); return; }
        if (r.status == 404) { set_error("Kein aktives Spotify-Gerät – Wiedergabe in Spotify starten"); return; }
        if (r.status == 429) { set_error("Spotify bremst die Abfragen – kurze Pause"); return; }
        if (r.status >= 300) { set_error(strprintf("Spotify antwortet mit Fehler %d", r.status)); return; }
        LOGI("spotify", "Befehl %s gesendet", path.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(350));   // Spotify braucht einen Moment, bis der Zustand stimmt
    }
}

void SpotifyClient::loop() {
    std::string last_error;
    while (!stop_) {
        double wait;
        bool back_off;
        {
            std::lock_guard lk(mu_);
            back_off = mono() < backoff_until_;
        }
        if (!back_off) {
            run_commands();
            poll();
            std::string now_error;
            {
                std::lock_guard lk(mu_);
                now_error = error_;
            }
            if (now_error != last_error && on_change_) on_change_();   // Fehler erscheinen und verschwinden auf dem Display
            last_error = now_error;
        }
        {
            std::lock_guard lk(mu_);
            wait = refresh_.empty() ? 2.0 : back_off ? 5.0 : snap_.playing ? PLAYING_POLL_S : IDLE_POLL_S;
        }
        if (!sleep_until_wake(wait)) return;
    }
}

}  // namespace fl::spotify
