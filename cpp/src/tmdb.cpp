// © 2026 FuchsLabs
#include "tmdb.h"

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <iterator>

#include "http.h"
#include "netguard.h"
#include "util.h"

namespace fl::tmdb {
namespace {

constexpr int TIMEOUT_MS = 15000;
constexpr size_t MAX_BYTES = 2'000'000;
constexpr DWORD PACE_MS = 300;             // Abstand zwischen Anfragen (TMDB erlaubt deutlich mehr, wir bleiben sparsam)
constexpr double RETRY_AFTER_ERROR_S = 600;
constexpr double RETRY_AFTER_LIMIT_S = 900;
constexpr size_t LOGO_MAX_BYTES = 200'000;   // w92-Logos sind wenige KB groß
constexpr int LOGO_PX = 64;                   // Speichergröße (quadratisch); der Renderer skaliert auf die Zeilenhöhe
constexpr int LOGO_DECODE_MAX = 512;

bool is_png(const std::string& b) { return b.size() >= 12 && b.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0; }
bool is_jpeg(const std::string& b) { return b.size() >= 12 && (unsigned char)b[0] == 0xFF && (unsigned char)b[1] == 0xD8 && (unsigned char)b[2] == 0xFF; }

// Bytes → quadratisches, abgerundetes Logo; nur PNG/JPEG, nur plausible Größen
ImagePtr make_logo(const std::string& bytes, bool round) {
    if (!is_png(bytes) && !is_jpeg(bytes)) return nullptr;
    auto img = decode_image(bytes);
    if (!img || img->w > LOGO_DECODE_MAX || img->h > LOGO_DECODE_MAX) return nullptr;
    Image sq = thumbnail(*img, LOGO_PX, LOGO_PX);
    if (sq.w != LOGO_PX || sq.h != LOGO_PX) sq = resize(sq, LOGO_PX, LOGO_PX);
    if (round) round_corners(sq.px, sq.w, sq.h, LOGO_PX * 0.2f);
    return std::make_shared<const Image>(std::move(sq));
}

std::string str(const ojson& cfg, const char* k) {
    return cfg.contains(k) && cfg[k].is_string() ? cfg[k].get<std::string>() : std::string();
}

// Eine Anfrage; Pfad ohne Schlüssel. Der Schlüssel wird nur hier in Anfrage/Header gesetzt, nie protokolliert.
http::Response fetch(const std::string& path, const std::string& key) {
    http::Request r;
    r.url = std::string(API_HOST) + with_key(path, key);
    r.headers = auth_headers(key);
    r.headers.push_back({"User-Agent", UA});
    r.timeout_ms = TIMEOUT_MS;
    r.max_bytes = MAX_BYTES;
    r.follow_redirects = false;   // die API leitet nicht weiter; keine Umleitung mit Schlüssel erlauben
    return http::fetch(r);
}

std::string explain(const http::Response& r) {
    if (r.status == 0) return "TMDB ist nicht erreichbar – Netzwerk prüfen";
    if (r.status == 401 || r.status == 403) return "TMDB-Key ungültig";
    if (r.status == 429) return "TMDB-Limit erreicht – später erneut";
    if (r.status == 200 && r.truncated) return "TMDB: unerwartete Antwort";
    return "TMDB antwortet mit Fehler " + std::to_string(r.status);
}

}  // namespace

std::vector<Provider> list_providers(const std::string& key_in, const std::string& region_in, const std::string& lang) {
    const std::string key = trim(key_in);
    if (key_kind(key) == KeyKind::None) throw TmdbError("Bitte zuerst den TMDB-Key eintragen");
    const std::string region = normalize_region(region_in);
    if (region.empty()) throw TmdbError("Ungültige Region (zwei Buchstaben, z. B. DE)");
    if (!netguard::public_url(API_HOST)) throw TmdbError("TMDB ist nicht erreichbar – Netzwerk prüfen");
    std::vector<Provider> all;
    bool any = false;
    for (bool series : {false, true}) {
        auto r = fetch(providers_path(series, region, lang), key);
        if (r.status != 200 || r.truncated) {
            if (r.status == 401 || r.status == 403 || r.status == 429 || !any) throw TmdbError(explain(r));
            continue;
        }
        auto list = parse_providers(r.body);
        if (!list) throw TmdbError("TMDB: unerwartete Antwort");
        all = merge_providers(std::move(all), *list);
        any = true;
        if (!series) Sleep(PACE_MS);
    }
    return all;
}

TmdbFeed::TmdbFeed(const ojson& cfg, std::string lang, std::filesystem::path logo_dir)
    : lang_(lang == "en" ? "en" : "de"), logo_dir_(std::move(logo_dir)) {
    key_ = trim(str(cfg, "api_key"));
    region_ = region_for(lang_, str(cfg, "region"));
    std::vector<std::string> lines;
    if (cfg.contains("providers") && cfg["providers"].is_array())
        for (auto& e : cfg["providers"])
            if (e.is_string()) lines.push_back(e.get<std::string>());
    sel_ = parse_selection(lines);
    max_items_ = clamp_items(cfg.contains("max_items") && cfg["max_items"].is_number() ? cfg["max_items"].get<long long>() : 5);
    refresh_minutes_ = clamp_refresh(cfg.contains("refresh_minutes") && cfg["refresh_minutes"].is_number() ? cfg["refresh_minutes"].get<long long>() : 60);
    if (key_kind(key_) == KeyKind::None) snap_.state = State::NoKey;
}

TmdbFeed::~TmdbFeed() { stop(); }

void TmdbFeed::stop() {
    stop_ = true;
    if (th_.joinable()) th_.join();
}

bool TmdbFeed::sleep_for(double seconds) {
    double end = mono() + seconds;
    while (mono() < end) {
        if (stop_) return false;
        Sleep(100);
    }
    return !stop_;
}

TmdbFeed::Snapshot TmdbFeed::snapshot() {
    std::lock_guard lk(mu_);
    return snap_;
}

void TmdbFeed::set_error(const std::string& msg) {
    std::lock_guard lk(mu_);
    snap_.message = msg;
    if (snap_.items.empty()) snap_.state = State::Error;   // alte Daten bleiben sichtbar, bis neue da sind
}

TmdbFeed::Got TmdbFeed::get(const std::string& path) {
    auto r = fetch(path, key_);
    if (r.status == 200 && !r.truncated) return {200, std::move(r.body)};
    if (r.status == 401 || r.status == 403) retry_s_ = refresh_minutes_ * 60.0;   // falscher Key: nicht bedrängen
    else retry_s_ = r.status == 429 ? RETRY_AFTER_LIMIT_S : RETRY_AFTER_ERROR_S;
    set_error(explain(r));
    return {r.status ? r.status : -1, {}};
}

void TmdbFeed::refresh() {
    retry_s_ = 0;
    if (sel_.empty()) {
        set_error("Keine Anbieter gewählt");
        retry_s_ = refresh_minutes_ * 60.0;
        return;
    }
    if (!netguard::public_url(API_HOST)) {
        set_error("TMDB ist nicht erreichbar – Netzwerk prüfen");
        retry_s_ = RETRY_AFTER_ERROR_S;
        return;
    }
    const std::time_t now = std::time(nullptr);
    const std::string from = add_days(now, -WINDOW_DAYS), to = date_ymd(now);
    std::vector<Item> lists[2];
    for (int s = 0; s < 2; ++s) {
        if (stop_) return;
        auto got = get(discover_path(s == 1, region_, sel_, from, to, lang_));
        if (got.status != 200) return;   // Fehler gemeldet, Pause gesetzt
        auto parsed = parse_discover(got.body, s == 1, from, to);
        if (!parsed) {
            set_error("TMDB: unerwartete Antwort");
            retry_s_ = RETRY_AFTER_ERROR_S;
            return;
        }
        lists[s] = std::move(*parsed);
        Sleep(PACE_MS);
    }
    auto top = merge_items(std::move(lists[0]), std::move(lists[1]), (size_t)max_items_);
    // Kürzel nur für die angezeigten Einträge; einmal geholt bleibt es im Cache
    for (auto& it : top) {
        if (stop_) return;
        const std::string ck = (it.series ? "s:" : "m:") + std::to_string(it.id);
        auto apply = [&](const std::vector<ProviderMatch>& ms) {
            for (const auto& m : ms) {
                it.providers.push_back(m.abbr);
                it.provider_ids.push_back(m.id);
                if (logo_url(m.logo_path)) logo_paths_[m.id] = m.logo_path;
            }
        };
        if (auto c = kuerzel_cache_.find(ck); c != kuerzel_cache_.end()) {
            apply(c->second);
            continue;
        }
        auto got = get(item_providers_path(it.series, it.id));
        if (got.status == 200) {
            auto ms = match_provider_details(got.body, region_, sel_);
            if (kuerzel_cache_.size() > 200) kuerzel_cache_.clear();
            apply(ms);
            kuerzel_cache_[ck] = std::move(ms);
        } else if (sel_.size() == 1) {
            it.providers = {abbreviation(sel_[0].id, sel_[0].name)};   // eindeutig, auch ohne Einzelabfrage
            it.provider_ids = {sel_[0].id};
        }
        if (got.status == 401 || got.status == 403 || got.status == 429) break;
        Sleep(PACE_MS);
    }
    // Logos nachladen: jedes einmal, danach aus dem Cache; fehlende bleiben beim Kürzel
    load_logos(top);
    std::lock_guard lk(mu_);
    snap_.items = std::move(top);
    snap_.state = State::Ok;
    snap_.message.clear();
    snap_.updated = std::time(nullptr);
    retry_s_ = 0;
}

// Logo laden: Speicher → Cache-Datei → TMDB (einmal). Dateiname nur aus der numerischen ID; nie ein Pfad aus Fremddaten.
ImagePtr TmdbFeed::load_logo(int id, const std::string& logo_path) {
    namespace fs = std::filesystem;
    const std::string name = logo_cache_name(id);
    if (name.empty()) return nullptr;
    const fs::path file = logo_dir_.empty() ? fs::path() : logo_dir_ / fs::path(name);
    if (!file.empty()) {
        std::error_code ec;
        const auto size = fs::file_size(file, ec);
        if (!ec && size > 0 && size <= LOGO_MAX_BYTES) {
            std::ifstream in(file, std::ios::binary);
            std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (auto img = make_logo(bytes, true)) return img;
        }
    }
    auto url = logo_url(logo_path);
    if (!url || !netguard::public_url(*url)) return nullptr;
    http::Request r;
    r.url = *url;
    r.headers.push_back({"User-Agent", UA});
    r.timeout_ms = TIMEOUT_MS;
    r.max_bytes = LOGO_MAX_BYTES;
    r.follow_redirects = false;
    auto resp = http::fetch(r);
    if (resp.status != 200 || resp.truncated) return nullptr;
    auto img = make_logo(resp.body, false);   // ungerundet speichern, Ecken erst im Speicher
    if (!img) return nullptr;
    if (!file.empty()) {
        std::error_code ec;
        fs::create_directories(logo_dir_, ec);
        const std::string png = encode_png(*img);
        const fs::path tmp = file.wstring() + L".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            out.write(png.data(), (std::streamsize)png.size());
        }
        fs::rename(tmp, file, ec);
        if (ec) fs::remove(tmp, ec);
    }
    Image rounded = *img;
    round_corners(rounded.px, rounded.w, rounded.h, LOGO_PX * 0.2f);
    return std::make_shared<const Image>(std::move(rounded));
}

void TmdbFeed::load_logos(const std::vector<Item>& items) {
    bool changed = false;
    for (const auto& it : items)
        for (int id : it.provider_ids) {
            if (stop_) return;
            if (logos_.count(id) || logo_failed_.count(id)) continue;
            auto p = logo_paths_.find(id);
            if (p == logo_paths_.end()) continue;
            ImagePtr img;
            try {
                img = load_logo(id, p->second);
            } catch (const std::exception& e) {
                LOGW("tmdb", "Logo %d nicht lesbar (%s)", id, e.what());
            }
            if (img) {
                logos_[id] = std::move(img);
                changed = true;
            } else {
                logo_failed_.insert(id);   // Kürzel bleibt; neuer Versuch erst nach Neustart
            }
            Sleep(PACE_MS);
        }
    if (changed) {
        std::lock_guard lk(mu_);
        snap_.logos = logos_;
    }
}

void TmdbFeed::start(std::function<void()> on_update) {
    th_ = std::thread([this, on_update] {
        if (key_kind(key_) == KeyKind::None) {   // ohne Key keine Anfragen; die Seite zeigt „Kein Key“
            on_update();
            return;
        }
        while (!stop_) {
            double t = mono();
            try {
                refresh();
            } catch (const std::exception& e) {
                set_error("TMDB: unerwarteter Fehler");
                retry_s_ = RETRY_AFTER_ERROR_S;
                LOGW("tmdb", "Streaming: unerwarteter Fehler (%s)", e.what());
            }
            auto s = snapshot();
            if (s.state == State::Ok) LOGI("tmdb", "Streaming: %d Einträge (%.1f s)", (int)s.items.size(), mono() - t);
            else LOGW("tmdb", "Streaming: %s", s.message.c_str());   // Meldungen enthalten nie den Schlüssel
            on_update();
            if (!sleep_for(retry_s_ > 0 ? retry_s_ : refresh_minutes_ * 60.0)) return;
        }
    });
}

}  // namespace fl::tmdb
