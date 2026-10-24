// © 2026 FuchsLabs
#include "webui.h"

#include <winsock2.h>
#include <windows.h>

#include <httplib.h>

#include "app.h"
#include "bf6.h"
#include "friends.h"
#include "ha.h"
#include "i18n.h"
#include "netguard.h"
#include "resources.h"
#include "rss.h"
#include "sysinfo.h"
#include "util.h"
#include "winsetup.h"

namespace fl {

namespace {

// Meldungstexte, die i18n übersetzt (wie Version 1.x: nur Werte in Objekten, Listen bleiben unverändert)
ojson tr_messages(const ojson& data) {
    if (!data.is_object()) return data;
    ojson out = ojson::object();
    for (auto& [k, v] : data.items()) {
        if ((k == "error" || k == "display_error" || k == "discord" || k == "detail") && v.is_string())
            out[k] = tr(v.get<std::string>());
        else
            out[k] = tr_messages(v);
    }
    return out;
}

void json_reply(httplib::Response& res, const ojson& data, int status = 200) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(tr_messages(data).dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace),
                    "application/json; charset=utf-8");
}

void png_reply(httplib::Response& res, const std::optional<Image>& img) {
    if (!img) {
        res.status = 404;
        return;
    }
    res.set_header("Cache-Control", "no-store");
    res.set_content(encode_png(*img), "image/png");
}

ojson body_json(const httplib::Request& req) {
    return ojson::parse(req.body);  // wirft bei ungültigem JSON
}

}  // namespace

struct SettingsServer::Impl {
    App& app;
    int port;
    std::function<void()> show_settings;
    httplib::Server svr;
    std::thread th;
    explicit Impl(App& a, std::function<void()> show) : app(a), port(a.cfg()["ui"].value("port", 8765)), show_settings(std::move(show)) {}
};

SettingsServer::SettingsServer(App& app, std::function<void()> show_settings) : d_(std::make_unique<Impl>(app, std::move(show_settings))) {}
SettingsServer::~SettingsServer() { stop(); }

bool SettingsServer::start() {
    auto& svr = d_->svr;
    App& app = d_->app;
    int port = d_->port;
    svr.set_payload_max_length(2 * 1024 * 1024);
    // Port exklusiv belegen: httplib setzt sonst SO_REUSEADDR, und unter Windows könnte eine zweite Instanz denselben
    // Port zusätzlich öffnen – Anfragen landeten dann zufällig bei der einen oder anderen App.
    svr.set_socket_options([](socket_t sock) {
        int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&yes), sizeof yes);
    });
    svr.set_keep_alive_max_count(20);

    svr.set_pre_routing_handler([port](const httplib::Request& req, httplib::Response& res) {
        std::string host = req.get_header_value("Host");
        if (host != "127.0.0.1:" + std::to_string(port) && host != "localhost:" + std::to_string(port)) {
            res.status = 403;
            res.set_content("Nur lokaler Zugriff", "text/plain; charset=utf-8");
            return httplib::Server::HandlerResponse::Handled;
        }
        // Fremde Webseiten dürfen auch keine GET-Abrufe auslösen (z. B. <img src=…/api/check/rss?url=…>);
        // nur das Öffnen der Startseite per Link bleibt erlaubt.
        std::string site = req.has_header("Sec-Fetch-Site") ? req.get_header_value("Sec-Fetch-Site") : "same-origin";
        if (site != "same-origin" && site != "none" &&
            !(req.path == "/" && req.get_header_value("Sec-Fetch-Mode") == "navigate")) {
            res.status = 403;
            res.set_content("Anfrage abgelehnt", "text/plain; charset=utf-8");
            return httplib::Server::HandlerResponse::Handled;
        }
        if (req.method != "GET" && req.method != "HEAD") {
            std::string origin = req.get_header_value("Origin");
            if (req.get_header_value("X-FL") != "1" || (!origin.empty() && origin != "http://" + host)) {
                res.status = 403;
                res.set_content("Anfrage abgelehnt", "text/plain; charset=utf-8");
                return httplib::Server::HandlerResponse::Handled;
            }
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    svr.set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        // Nicht in fremde Seiten einbetten (Klick-Täuschung)
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("Content-Security-Policy", "frame-ancestors 'none'");
        res.set_header("X-Content-Type-Options", "nosniff");
    });
    svr.set_exception_handler([](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        std::string what = "Fehler";
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            what = e.what();
        } catch (...) {
        }
        LOGW("webui", "%s %s: %s", req.method.c_str(), req.path.c_str(), what.c_str());
        res.status = 500;
        res.set_content("Interner Fehler", "text/plain; charset=utf-8");
    });

    // ---------- Seiten ----------
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        auto html = res::get(res::WEBUI);
        res.set_header("Cache-Control", "no-store");
        res.set_content(std::string(html), "text/html; charset=utf-8");
    });
    svr.Get("/anleitung", [](const httplib::Request& req, httplib::Response& res) {
        std::string lang = req.has_param("lang") ? req.get_param_value("lang") : i18n::lang();
        auto en = res::get(res::GUIDE_EN);
        auto html = lang == "en" && !en.empty() ? en : res::get(res::GUIDE_DE);
        res.set_content(std::string(html), "text/html; charset=utf-8");
    });
    svr.Post("/api/open-window", [show = d_->show_settings](const httplib::Request&, httplib::Response& res) { if (show) show(); json_reply(res, {{"ok", true}}); });
    svr.Get("/api/config", [&app](const httplib::Request&, httplib::Response& res) {
        ojson cfg = app.cfg();
        auto [w, h] = app.size();
        json_reply(res, {{"config", config::public_view(cfg)}, {"version", VERSION}, {"discord_ready", app.discord_ready()},
                         {"screen", {w, h}}, {"first_run", !cfg["ui"].value("setup_done", false)},
                         {"active", app.active_pages()}, {"config_file", u8(app.cfg_path())}});
    });
    svr.Post("/api/config", [&app](const httplib::Request& req, httplib::Response& res) {
        ojson merged;
        try {
            ojson body = body_json(req);
            if (!body.is_object()) throw std::invalid_argument("JSON-Objekt erwartet");
            merged = config::merge(app.cfg(), body.contains("config") ? body["config"] : ojson::object());
        } catch (const std::exception& e) {
            json_reply(res, {{"ok", false}, {"error", e.what()}}, 400);
            return;
        }
        config::save(app.cfg_path(), merged);
        LOGI("webui", "Einstellungen gespeichert – starte neu");
        app.request_restart(0.4);
        json_reply(res, {{"ok", true}});
    });
    svr.Get("/api/preview.png", [&app](const httplib::Request&, httplib::Response& res) { png_reply(res, app.last_frame()); });
    svr.Get("/api/pages", [&app](const httplib::Request&, httplib::Response& res) {
        json_reply(res, app.page_navigation());
    });
    svr.Get("/api/companion/pages", [&app](const httplib::Request&, httplib::Response& res) {
        json_reply(res, app.companion() ? app.page_navigation(true) : ojson{{"enabled", false}});
    });
    svr.Get("/api/companion/preview.png", [&app](const httplib::Request&, httplib::Response& res) {
        png_reply(res, app.companion() ? app.last_frame(true) : std::nullopt);
    });
    svr.Post("/api/companion/pages/step", [&app](const httplib::Request& req, httplib::Response& res) {
        ojson body;
        try { body = body_json(req); } catch (...) { return json_reply(res, {{"ok", false}}, 400); }
        if (!app.companion() || !body.is_object() || !body.contains("delta") || !body["delta"].is_number_integer() ||
            (body["delta"] != -1 && body["delta"] != 1))
            return json_reply(res, {{"ok", false}, {"error", "Companion und delta -1/+1 erwartet"}}, 400);
        app.step_page(body["delta"].get<int>(), true);
        json_reply(res, {{"ok", true}});
    });
    svr.Post("/api/pages/step", [&app](const httplib::Request& req, httplib::Response& res) {
        ojson body;
        try {
            body = body_json(req);
            if (!body.is_object() || !body.contains("delta") || !body["delta"].is_number_integer() ||
                (body["delta"] != -1 && body["delta"] != 1))
                throw std::invalid_argument("delta: -1 oder 1 erwartet");
        } catch (const std::exception&) {
            return json_reply(res, {{"ok", false}, {"error", "delta: -1 oder 1 erwartet"}}, 400);
        }
        app.step_page(body["delta"].get<int>());
        json_reply(res, {{"ok", true}});
    });
    svr.Post("/api/preview-theme.png", [&app](const httplib::Request& req, httplib::Response& res) {
        // Aktuelles Displaybild mit den (noch nicht gespeicherten) Farben aus dem Formular
        ojson merged;
        try {
            ojson body = body_json(req);
            ojson dp = ojson::object();
            if (body.is_object() && body.contains("display") && body["display"].is_object())
                for (auto& [k, v] : body["display"].items())
                    if (k == "theme" || k == "color_base" || config::is_color_key(k)) dp[k] = v;
            merged = config::merge(app.cfg(), {{"display", dp}})["display"];
        } catch (const std::exception& e) {
            json_reply(res, {{"ok", false}, {"error", e.what()}}, 400);
            return;
        }
        png_reply(res, app.preview_theme(merged));
    });
    svr.Get("/api/display/options", [&app](const httplib::Request&, httplib::Response& res) {
        // Angeschlossene Bildschirme, Touch-Fähigkeit und die Farben der Vorlagen
        auto [w, h] = app.size();
        json_reply(res, {{"monitors", monitors_json()}, {"touch", touch_info()}, {"presets", render::presets_json()}, {"size", {w, h}}});
    });
    svr.Post("/api/display/touch-setup", [&app](const httplib::Request&, httplib::Response& res) {
        if (!touch_info().value("available", false))
            return json_reply(res, {{"ok", false}, {"error", "Windows meldet keinen Touchscreen. USB-Touchkabel und Treiber prüfen."}}, 400);
        bool ok = app.setup_touch();
        json_reply(res, {{"ok", ok}, {"error", ok ? "" : "Die Windows-Touch-Einrichtung konnte nicht geöffnet werden."}}, ok ? 200 : 500);
    });
    svr.Get("/api/discord/members", [&app](const httplib::Request&, httplib::Response& res) {
        json_reply(res, {{"ready", app.discord_ready()}, {"members", app.member_list()}});
    });
    svr.Get("/api/system/info", [](const httplib::Request&, httplib::Response& res) {
        json_reply(res, {{"interfaces", sysinfo::list_interfaces()}, {"sensors", sysinfo::hwinfo_sensors()}});
    });

    // ---------- Prüfen ----------
    svr.Get("/api/check/bf6", [](const httplib::Request& req, httplib::Response& res) {
        std::string name = trim(req.get_param_value("name"));
        if (name.empty()) return json_reply(res, {{"ok", false}, {"error", "Name fehlt"}}, 400);
        bf6::Bf6Stats st(ojson{{"players", ojson::array()}});
        Bf6Player p = st.fetch(name);
        if (!p.error.empty()) return json_reply(res, {{"ok", false}, {"error", "Spieler nicht gefunden"}});
        auto opt = [](const std::optional<double>& v) { return v ? ojson(*v) : ojson(); };
        json_reply(res, {{"ok", true}, {"name", p.name}, {"rank", p.rank ? ojson(*p.rank) : ojson()}, {"rank_name", p.rank_name},
                         {"kd", opt(p.kd)}, {"win_pct", opt(p.win_pct)}});
    });
    svr.Get("/api/check/rss", [](const httplib::Request& req, httplib::Response& res) {
        std::string url = trim(req.get_param_value("url"));
        if (url.empty()) return json_reply(res, {{"ok", false}, {"error", "Adresse fehlt"}}, 400);
        // Nur öffentliche Adressen (kein Heimnetz/localhost); Weiterleitungen prüft RssFeeds erneut
        if (!netguard::public_url(url.find("://") != std::string::npos ? url : "https://" + url))
            return json_reply(res, {{"ok", false}, {"error", "Ungültige oder unsichere Adresse"}}, 400);
        rss::RssFeeds feeds(ojson{{"feeds", ojson::array()}});
        auto posts = feeds.feed(url);
        if (!posts) return json_reply(res, {{"ok", false}, {"error", "Kein Feed gefunden"}});
        auto sel = feeds.select(*posts);
        json_reply(res, {{"ok", true}, {"feed", feeds.resolved(url)}, {"source", sel.empty() ? "" : sel[0].source},
                         {"latest", sel.empty() ? "" : sel[0].title}});
    });
    svr.Post(R"(/api/check/friends/(steam|xbox))", [&app](const httplib::Request& req, httplib::Response& res) {
        // Zugang zu Steam oder Xbox prüfen. Leere Felder kommen aus den gespeicherten Einstellungen
        std::string platform = req.matches[1];
        ojson body = body_json(req);
        ojson saved = app.cfg()["friends"];
        auto str = [](const ojson& j, const std::string& k) {
            return j.is_object() && j.contains(k) && j[k].is_string() ? trim(j[k].get<std::string>()) : std::string();
        };
        std::string key = str(body, "key");
        if (key.empty()) key = str(saved, platform + "_api_key");
        if (key.empty()) return json_reply(res, {{"ok", false}, {"error", "Bitte zuerst den Schlüssel eintragen"}});
        std::vector<friends::Account> accounts;
        try {
            if (platform == "steam") {
                std::string sid = str(body, "id");
                if (sid.empty()) sid = str(saved, "steam_id");
                if (sid.empty()) return json_reply(res, {{"ok", false}, {"error", "Bitte den Link zu deinem Steam-Profil eintragen"}});
                accounts = friends::fetch_steam(key, sid);
            } else {
                accounts = friends::fetch_xbox(key);
            }
        } catch (const friends::FriendsError& e) {
            return json_reply(res, {{"ok", false}, {"error", e.what()}});
        }
        int online = (int)std::count_if(accounts.begin(), accounts.end(), [](auto& a) { return a.status != "offline"; });
        json_reply(res, {{"ok", true}, {"count", accounts.size()}, {"online", online}});
    });
    svr.Post("/api/ha/entities", [&app](const httplib::Request& req, httplib::Response& res) {
        // Verbindung prüfen und die wählbaren Entitäten laden. Leere Felder kommen aus den gespeicherten Einstellungen.
        ojson body = body_json(req);
        ojson saved = app.cfg()["homeassistant"];
        auto str = [](const ojson& j, const std::string& k) {
            return j.is_object() && j.contains(k) && j[k].is_string() ? trim(j[k].get<std::string>()) : std::string();
        };
        std::string url = str(body, "url"), token = str(body, "token");
        if (url.empty()) url = str(saved, "url");
        if (token.empty()) token = str(saved, "token");
        // Schalter nur anbieten, wenn Touch (laut Formular) gewünscht ist und Windows einen Touchscreen meldet
        // (die Companion-App zählt als Touch: sie meldet Tipps vom Handy)
        bool touch = body.is_object() && body.value("touch", false) &&
                     (touch_info().value("available", false) || body.value("companion", false));
        try {
            auto list = ha::list_entities(url, token, touch);
            json_reply(res, {{"ok", true}, {"entities", list}, {"touch", touch}});
        } catch (const ha::HaError& e) {
            json_reply(res, {{"ok", false}, {"error", e.what()}});
        }
    });
    // ---------- Companion-App (Android) ----------
    svr.Get("/api/companion/status", [&app](const httplib::Request&, httplib::Response& res) {
        auto* c = app.companion();
        json_reply(res, c ? c->status() : ojson{{"enabled", false}});
    });
    svr.Post("/api/companion/pair", [&app](const httplib::Request&, httplib::Response& res) {
        // Einmal-PIN (2 Minuten) und QR-Code; nur über diese lokale Seite abrufbar
        auto* c = app.companion();
        if (!c) return json_reply(res, {{"ok", false}, {"error", "Der Companion-Server läuft nicht – Companion einschalten und speichern"}});
        json_reply(res, c->start_pairing());
    });
    svr.Post("/api/companion/pair/cancel", [&app](const httplib::Request&, httplib::Response& res) {
        if (auto* c = app.companion()) c->cancel_pairing();
        json_reply(res, {{"ok", true}});
    });
    svr.Post("/api/companion/devices/remove", [&app](const httplib::Request& req, httplib::Response& res) {
        ojson body = body_json(req);
        std::string id = body.is_object() && body.contains("id") && body["id"].is_string() ? body["id"].get<std::string>() : "";
        auto* c = app.companion();
        bool ok = c && !id.empty() && c->remove_device(id);
        json_reply(res, {{"ok", ok}, {"error", ok ? "" : "Gerät nicht gefunden"}});
    });
    svr.Get("/api/connections", [&app](const httplib::Request&, httplib::Response& res) { json_reply(res, app.connections()); });
    svr.Get("/api/friends/accounts", [&app](const httplib::Request&, httplib::Response& res) { json_reply(res, app.friend_accounts()); });

    // ---------- Status und Windows-Einstellungen ----------
    svr.Get("/api/status", [&app](const httplib::Request&, httplib::Response& res) {
        auto ds = app.discord_status();
        json_reply(res, {{"display_ok", app.display_ok()}, {"display_error", app.display_error()},
                         {"discord", ds ? ojson(*ds) : ojson()}, {"vendor_running", winsetup::vendor_running()},
                         {"vendor_autostart", winsetup::vendor_autostart_enabled()}, {"autostart", winsetup::autostart_enabled()},
                         {"update", app.update_info()}});
    });
    svr.Post("/api/update/check", [&app](const httplib::Request&, httplib::Response& res) {
        json_reply(res, {{"ok", app.update_check_now()}});
    });
    svr.Post("/api/autostart", [](const httplib::Request& req, httplib::Response& res) {
        ojson body = body_json(req);
        bool on = body.is_object() && body.contains("enabled") && body["enabled"].is_boolean() && body["enabled"].get<bool>();
        if (test_instance()) LOGW("webui", "Testinstanz: Autostart wird nicht verändert");
        else winsetup::set_autostart(on);
        json_reply(res, {{"ok", true}, {"autostart", winsetup::autostart_enabled()}});
    });
    svr.Post("/api/vendor/stop", [](const httplib::Request&, httplib::Response& res) {
        bool ok = winsetup::vendor_stop();
        json_reply(res, {{"ok", ok}, {"error", ok ? "" : "Konnte die Hersteller-App nicht beenden"}});
    });
    svr.Post("/api/vendor/disable-autostart", [](const httplib::Request&, httplib::Response& res) {
        bool ok = winsetup::vendor_disable_autostart();
        json_reply(res, {{"ok", ok}, {"error", ok ? "" : "Die Windows-Abfrage wurde abgelehnt"}});
    });
    svr.Post("/api/setup-done", [&app](const httplib::Request&, httplib::Response& res) {
        app.set_setup_done();
        json_reply(res, {{"ok", true}});
    });

    // ---------- Steuerung ----------
    svr.Post("/api/restart", [&app](const httplib::Request&, httplib::Response& res) {
        app.request_restart(0.4);
        json_reply(res, {{"ok", true}});
    });
    svr.Post("/api/quit", [&app](const httplib::Request&, httplib::Response& res) {
        app.request_stop(0.4);
        json_reply(res, {{"ok", true}});
    });

    if (!svr.bind_to_port("127.0.0.1", port)) {
        LOGW("webui", "Einstellungsoberfläche nicht gestartet (Port %d belegt)", port);
        return false;
    }
    d_->th = std::thread([this] { d_->svr.listen_after_bind(); });
    LOGI("webui", "Einstellungen: http://127.0.0.1:%d/", port);
    return true;
}

void SettingsServer::stop() {
    if (d_->svr.is_running()) d_->svr.stop();
    if (d_->th.joinable()) d_->th.join();
}

}  // namespace fl
