// © 2026 FuchsLabs
#include "ha.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <regex>

#include "http.h"
#include "util.h"

namespace fl::ha {

namespace {

constexpr double REFRESH_S = 15;
constexpr int TIMEOUT_MS = 6000;

bool valid_id(const std::string& id) {
    static const std::regex rx("[a-z_]+\\.[a-z0-9_]+");
    return id.size() <= 120 && std::regex_match(id, rx);
}

std::string domain_of(const std::string& id) { return id.substr(0, id.find('.')); }

using Headers = std::vector<std::pair<std::string, std::string>>;

Headers auth(const std::string& token, bool json = false) {
    Headers h = {{"Authorization", "Bearer " + token}, {"Accept", "application/json"}, {"User-Agent", UA}};
    if (json) h.push_back({"Content-Type", "application/json"});
    return h;
}

// Antwort prüfen; wirft HaError mit verständlichem Text
void check(const http::Response& r) {
    if (r.status == 0) throw HaError("Home Assistant ist nicht erreichbar – Adresse und Netzwerk prüfen");
    if (r.status == 401 || r.status == 403) throw HaError("Home Assistant: Token ungültig oder abgelaufen");
    if (r.status == 404 && r.body.find("Entity not found") == std::string::npos && r.body.find("entity") == std::string::npos)
        throw HaError("Home Assistant: Adresse stimmt nicht (keine API gefunden)");
    if (r.status >= 500) throw HaError(strprintf("Home Assistant antwortet mit Fehler %d", r.status));
}

}  // namespace

std::optional<Spec> parse_spec(const std::string& entry) {
    std::string id = trim(entry), label;
    if (auto bar = id.find('|'); bar != std::string::npos) {
        label = trim(id.substr(bar + 1));
        id = trim(id.substr(0, bar));
    }
    if (!valid_id(id)) return std::nullopt;
    std::u32string u = to_u32(label);
    if (u.size() > 40) u.resize(40);
    return Spec{id, from_u32(u)};
}

bool toggle_domain(const std::string& id) {
    static const char* d[] = {"switch", "input_boolean", "light", "fan"};
    std::string dom = domain_of(id);
    return std::any_of(std::begin(d), std::end(d), [&](const char* x) { return dom == x; });
}

std::string base_url(const std::string& url_in) {
    std::string url = trim(url_in);
    if (url.find("://") == std::string::npos && !url.empty()) url = "http://" + url;
    auto u = http::parse_url(url);
    if (!u.valid) return "";
    while (!url.empty() && url.back() == '/') url.pop_back();
    // Nur Adresse (kein Pfad/Abfrage mit Zugangsdaten): Schema, Host und Port genügen
    auto scheme_end = url.find("://") + 3;
    auto slash = url.find_first_of("/?#", scheme_end);
    if (url.find('@', scheme_end) != std::string::npos && (slash == std::string::npos || url.find('@', scheme_end) < slash)) return "";
    return slash == std::string::npos ? url : url.substr(0, slash);
}

std::string format_value(const std::string& state) {
    std::string t = trim(state);
    if (t.empty()) return "–";
    char* end = nullptr;
    double d = strtod(t.c_str(), &end);
    if (end && *end == '\0' && std::isfinite(d)) {
        double a = std::fabs(d);
        int dec = a >= 100 ? 0 : a >= 10 ? 1 : 2;
        std::string s = format_num(d, dec);
        if (s.find(',') != std::string::npos) {
            while (!s.empty() && s.back() == '0') s.pop_back();
            if (!s.empty() && s.back() == ',') s.pop_back();
        }
        return s == "-0" ? "0" : s;
    }
    if (t == "on") return "An";
    if (t == "off") return "Aus";
    if (t == "open") return "Offen";
    if (t == "closed") return "Zu";
    if (t == "home") return "Zuhause";
    if (t == "not_home") return "Unterwegs";
    return t;
}

HaEntity parse_state(const nlohmann::json& data, const Spec& spec, bool touch) {
    HaEntity e;
    e.id = spec.id;
    e.name = spec.label;
    if (!data.is_object()) return e;
    std::string st = data.contains("state") && data["state"].is_string() ? data["state"].get<std::string>() : "";
    const nlohmann::json empty = nlohmann::json::object();
    const auto& attrs = data.contains("attributes") && data["attributes"].is_object() ? data["attributes"] : empty;
    if (e.name.empty() && attrs.contains("friendly_name") && attrs["friendly_name"].is_string())
        e.name = attrs["friendly_name"].get<std::string>();
    if (e.name.empty()) e.name = spec.id;
    if (attrs.contains("unit_of_measurement") && attrs["unit_of_measurement"].is_string())
        e.unit = attrs["unit_of_measurement"].get<std::string>();
    e.state = st;
    e.available = !st.empty() && st != "unavailable" && st != "unknown";
    e.on = st == "on";
    e.toggle = touch && e.available && toggle_domain(spec.id) && (st == "on" || st == "off");
    return e;
}

nlohmann::json list_entities(const std::string& url_in, const std::string& token, bool touch) {
    std::string url = base_url(url_in);
    if (url.empty()) throw HaError("Bitte zuerst die Adresse von Home Assistant eintragen");
    if (trim(token).empty()) throw HaError("Bitte zuerst das Zugriffstoken eintragen");
    auto r = http::fetch({"GET", url + "/api/states", auth(trim(token)), "", 15000, 20'000'000});
    check(r);
    if (r.status != 200 || r.truncated) throw HaError(strprintf("Home Assistant antwortet mit Fehler %d", r.status));
    nlohmann::json data = nlohmann::json::parse(r.body, nullptr, false);
    if (!data.is_array()) throw HaError("Home Assistant: unerwartete Antwort");
    nlohmann::json out = nlohmann::json::array();
    for (auto& s : data) {
        if (!s.is_object() || !s.contains("entity_id") || !s["entity_id"].is_string()) continue;
        std::string id = s["entity_id"].get<std::string>();
        if (!valid_id(id)) continue;
        std::string dom = domain_of(id);
        if (dom != "sensor" && dom != "binary_sensor" && !(touch && toggle_domain(id))) continue;
        HaEntity e = parse_state(s, {id, ""}, touch);
        out.push_back({{"id", id}, {"name", e.name}, {"domain", dom}, {"state", format_value(e.state)}, {"unit", e.unit}});
        if (out.size() >= 3000) break;
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return casefold(a["name"].template get<std::string>()) < casefold(b["name"].template get<std::string>());
    });
    return out;
}

// ---------- Hintergrundabfrage ----------
HaClient::HaClient(const ojson& cfg, bool touch, std::function<void()> on_change)
    : touch_(touch), on_change_(std::move(on_change)) {
    auto str = [&](const char* k) { return cfg.contains(k) && cfg[k].is_string() ? cfg[k].get<std::string>() : std::string(); };
    url_ = base_url(str("url"));
    token_ = trim(str("token"));
    if (cfg.contains("entities") && cfg["entities"].is_array())
        for (auto& e : cfg["entities"]) {
            if (!e.is_string() || specs_.size() >= MAX_ENTITIES) continue;
            auto spec = parse_spec(e.get<std::string>());
            if (!spec) continue;
            // Schalter nur mit Touch: ohne Touch lässt sich nichts schalten, die Kachel entfällt ganz
            if (!touch_ && toggle_domain(spec->id)) continue;
            specs_.push_back(*spec);
        }
    for (auto& s : specs_) {
        HaEntity e;
        e.id = s.id;
        e.name = s.label.empty() ? s.id : s.label;
        entities_.push_back(e);
    }
}

HaClient::~HaClient() { stop(); }

void HaClient::start() { th_ = std::thread([this] { loop(); }); }

void HaClient::stop() {
    stop_ = true;
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

std::vector<HaEntity> HaClient::entities() {
    std::lock_guard lk(mu_);
    return entities_;
}

std::string HaClient::status() {
    std::lock_guard lk(mu_);
    return error_;
}

bool HaClient::has_data() {
    std::lock_guard lk(mu_);
    return ok_at_ > 0;
}

ojson HaClient::info() {
    std::lock_guard lk(mu_);
    ojson j = {{"ok", error_.empty() && ok_at_ > 0}, {"count", entities_.size()}, {"error", error_}};
    if (ok_at_ > 0) j["at"] = (long long)std::time(nullptr) - (long long)(mono() - ok_at_);
    return j;
}

void HaClient::request_toggle(const std::string& id) {
    {
        std::lock_guard lk(mu_);
        if (!touch_ || !toggle_domain(id)) return;
        if (std::find(pending_.begin(), pending_.end(), id) == pending_.end()) pending_.push_back(id);
    }
    cv_.notify_all();
}

bool HaClient::sleep_until_wake(double seconds) {
    std::unique_lock lk(mu_);
    cv_.wait_for(lk, std::chrono::duration<double>(seconds), [&] { return stop_ || !pending_.empty(); });
    return !stop_;
}

void HaClient::poll_all() {
    std::vector<HaEntity> out;
    std::string error;
    bool any_ok = false;
    for (auto& spec : specs_) {
        if (stop_) return;
        HaEntity e;
        e.id = spec.id;
        e.name = spec.label.empty() ? spec.id : spec.label;
        try {
            auto r = http::fetch({"GET", url_ + "/api/states/" + spec.id, auth(token_), "", TIMEOUT_MS, 1'000'000});
            if (r.status == 404) {
                e.state = "nicht gefunden";
                if (error.empty()) error = "Home Assistant: Entität " + spec.id + " nicht gefunden";
                out.push_back(e);
                any_ok = true;  // Verbindung steht, nur dieser Name stimmt nicht
                continue;
            }
            check(r);
            if (r.status != 200) throw HaError(strprintf("Home Assistant antwortet mit Fehler %d", r.status));
            nlohmann::json data = nlohmann::json::parse(r.body, nullptr, false);
            if (!data.is_object()) throw HaError("Home Assistant: unerwartete Antwort");
            e = parse_state(data, spec, touch_);
            any_ok = true;
        } catch (const HaError& ex) {
            error = ex.what();
            std::lock_guard lk(mu_);   // alte Werte behalten, aber als nicht verfügbar kennzeichnen
            auto it = std::find_if(entities_.begin(), entities_.end(), [&](auto& x) { return x.id == spec.id; });
            if (it != entities_.end()) { e = *it; e.available = false; e.toggle = false; }
        }
        out.push_back(e);
    }
    {
        std::lock_guard lk(mu_);
        entities_ = std::move(out);
        error_ = error;
        if (any_ok) ok_at_ = mono();
    }
    if (on_change_) on_change_();
}

void HaClient::loop() {
    if (url_.empty() || token_.empty() || specs_.empty()) {
        std::lock_guard lk(mu_);
        error_ = "Home Assistant ist nicht fertig eingerichtet";
        return;
    }
    while (!stop_) {
        poll_all();
        // Bis zur nächsten Abfrage warten; ein Tipp auf einen Schalter weckt sofort
        while (!stop_) {
            double end = mono() + REFRESH_S;
            std::vector<std::string> todo;
            while (!stop_ && mono() < end) {
                if (!sleep_until_wake(end - mono())) return;
                std::lock_guard lk(mu_);
                if (!pending_.empty()) { todo.swap(pending_); break; }
            }
            if (stop_) return;
            for (auto& id : todo) {
                auto it = std::find_if(specs_.begin(), specs_.end(), [&](auto& s) { return s.id == id; });
                if (it == specs_.end()) continue;
                try {
                    nlohmann::json body = {{"entity_id", id}};
                    auto r = http::fetch({"POST", url_ + "/api/services/homeassistant/toggle", auth(token_, true), body.dump(),
                                          TIMEOUT_MS, 1'000'000});
                    check(r);
                    LOGI("ha", "Schalter %s umgelegt (Antwort %d)", id.c_str(), r.status);
                } catch (const HaError& ex) {
                    LOGW("ha", "Schalten von %s fehlgeschlagen: %s", id.c_str(), ex.what());
                    std::lock_guard lk(mu_);
                    error_ = ex.what();
                }
            }
            break;   // danach (oder nach Ablauf der Zeit) alles neu abfragen
        }
    }
}

}  // namespace fl::ha
