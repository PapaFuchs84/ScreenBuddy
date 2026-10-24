// © 2026 FuchsLabs
#include "update.h"

#include <chrono>
#include <fstream>

#include "http.h"
#include "util.h"

namespace fl::update {

namespace {
constexpr long long DAY = 24 * 3600, RETRY = 6 * 3600, MANUAL_GAP = 300, FIRST_DELAY = 45;
long long now_s() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
}  // namespace

std::optional<std::vector<int>> parse_version(const std::string& tag) {
    std::string t = trim(tag);
    if (!t.empty() && (t[0] == 'v' || t[0] == 'V')) t.erase(0, 1);
    std::vector<int> out;
    for (auto& part : split(t, ".")) {
        if (part.empty() || part.size() > 6) return std::nullopt;
        for (char c : part)
            if (c < '0' || c > '9') return std::nullopt;   // "2.2.0-rc1" u. Ä. zählt nicht als Veröffentlichung
        out.push_back(std::stoi(part));
    }
    if (out.size() < 2 || out.size() > 4) return std::nullopt;
    return out;
}

bool is_newer(const std::string& latest, const std::string& current) {
    auto a = parse_version(latest), b = parse_version(current);
    if (!a || !b) return false;
    a->resize(4);
    b->resize(4);
    return *a > *b;
}

std::optional<Release> parse_release(const nlohmann::json& data) {
    if (!data.is_object()) return std::nullopt;
    if (data.value("draft", false) || data.value("prerelease", false)) return std::nullopt;
    if (!data.contains("tag_name") || !data["tag_name"].is_string()) return std::nullopt;
    auto v = parse_version(data["tag_name"].get<std::string>());
    if (!v) return std::nullopt;
    std::string version = trim(data["tag_name"].get<std::string>());
    if (version[0] == 'v' || version[0] == 'V') version.erase(0, 1);
    std::string url = data.contains("html_url") && data["html_url"].is_string() ? data["html_url"].get<std::string>() : "";
    if (url.rfind(RELEASES_PAGE_PREFIX, 0) != 0) url = std::string(RELEASES_PAGE_PREFIX) + "releases/latest";
    return Release{version, url};
}

UpdateChecker::UpdateChecker(std::filesystem::path state_file, std::function<void(const Release&)> on_new)
    : file_(std::move(state_file)), on_new_(std::move(on_new)) {
    load();
}

UpdateChecker::~UpdateChecker() { stop(); }

void UpdateChecker::load() {
    try {
        std::ifstream f(file_, std::ios::binary);
        if (!f) return;
        auto j = nlohmann::json::parse(f, nullptr, false);
        if (!j.is_object()) return;
        checked_at_ = j.value("checked_at", 0LL);
        latest_ = j.value("latest", std::string());
        url_ = j.value("url", std::string());
        notified_ = j.value("notified", std::string());
        if (url_.rfind(RELEASES_PAGE_PREFIX, 0) != 0) url_.clear();
    } catch (...) {
    }
}

void UpdateChecker::save() {
    try {
        std::error_code ec;
        std::filesystem::create_directories(file_.parent_path(), ec);
        std::ofstream f(file_, std::ios::binary | std::ios::trunc);
        f << nlohmann::json{{"checked_at", checked_at_}, {"latest", latest_}, {"url", url_}, {"notified", notified_}}.dump(2);
    } catch (...) {
    }
}

void UpdateChecker::start() {
    th_ = std::thread([this] { loop(); });
}

void UpdateChecker::stop() {
    {
        std::lock_guard lk(mu_);
        stop_ = true;
    }
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

bool UpdateChecker::fetch() {
    http::Request r;
    r.url = RELEASES_API;
    r.headers = {{"Accept", "application/vnd.github+json"}, {"X-GitHub-Api-Version", "2022-11-28"}};
    r.timeout_ms = 15000;
    r.max_bytes = 512 * 1024;
    http::Response resp = http::fetch(r);
    std::string err;
    std::optional<Release> rel;
    if (resp.status == 200 && !resp.truncated) {
        auto j = nlohmann::json::parse(resp.body, nullptr, false);
        rel = parse_release(j);
        if (!rel) err = "Antwort von GitHub nicht lesbar";
    } else if (resp.status == 404) {
        err = "";   // noch kein Release veröffentlicht
    } else {
        err = resp.status ? "GitHub antwortet mit Fehler " + std::to_string(resp.status) : "GitHub nicht erreichbar";
    }
    std::unique_lock lk(mu_);
    last_attempt_ = now_s();
    error_ = err;
    bool ok = err.empty();
    if (ok) {
        checked_at_ = last_attempt_;
        if (rel) {
            latest_ = rel->version;
            url_ = rel->url;
        }
        save();
        announce_locked(lk);
    } else {
        LOGW("update", "Update-Abfrage fehlgeschlagen: %s", err.c_str());
    }
    return ok;
}

void UpdateChecker::announce_locked(std::unique_lock<std::mutex>& lk) {
    if (latest_.empty() || !is_newer(latest_, VERSION) || notified_ == latest_) return;
    notified_ = latest_;
    save();
    Release rel{latest_, url_};
    auto cb = on_new_;
    lk.unlock();
    LOGI("update", "Neue Version %s verfügbar", rel.version.c_str());
    if (cb) cb(rel);
    lk.lock();
}

void UpdateChecker::loop() {
    std::unique_lock lk(mu_);
    // Kurz nach dem Start warten: nicht beim Windows-Start gegen das Netz arbeiten
    cv_.wait_for(lk, std::chrono::seconds(FIRST_DELAY), [&] { return stop_ || wake_; });
    if (stop_) return;
    announce_locked(lk);   // gespeicherte Meldung (z. B. Abfrage gestern, App erst jetzt gestartet)
    while (!stop_) {
        long long due = error_.empty() ? checked_at_ + DAY : last_attempt_ + RETRY;
        long long wait = due - now_s();
        if (wait > 0 && !wake_) {
            cv_.wait_for(lk, std::chrono::seconds(std::min<long long>(wait, 3600)), [&] { return stop_ || wake_; });
            if (stop_) break;
            if (!wake_ && due - now_s() > 0) continue;
        }
        wake_ = false;
        lk.unlock();
        fetch();
        lk.lock();
    }
}

bool UpdateChecker::check_now() {
    {
        std::lock_guard lk(mu_);
        if (now_s() - last_attempt_ < MANUAL_GAP) return false;
        last_attempt_ = now_s();   // reserviert den Takt, bis die Abfrage durch ist
        wake_ = true;
    }
    cv_.notify_all();
    return true;
}

nlohmann::json UpdateChecker::info() {
    std::lock_guard lk(mu_);
    bool newer = !latest_.empty() && is_newer(latest_, VERSION);
    return {{"available", newer}, {"current", VERSION}, {"latest", latest_}, {"url", newer ? url_ : std::string()},
            {"checked_at", checked_at_}, {"error", error_}};
}

}  // namespace fl::update