// © 2026 FuchsLabs
#include "app.h"

#include <windows.h>
#include <shellapi.h>

#include <windows.h>

#include <fstream>

#include "bf6.h"
#include "discord.h"
#include "friends.h"
#include "i18n.h"
#include "rss.h"
#include "sysinfo.h"
#include "settings_window.h"
#include "tray.h"
#include "util.h"
#include "webui.h"
#include "winsetup.h"

namespace fl {

struct App::RendererSet {
    render::Renderer discord, friends;
    render::Bf6Renderer bf6;
    render::RssRenderer rss;
    render::SysRenderer system;
    RendererSet(int w, int h, const ojson& cfg)
        : discord(w, h, cfg["display"].value("title", "Discord-Freunde")),
          friends(w, h, cfg["friends"].value("title", "Freunde")),
          bf6(w, h, cfg["bf6"].value("title", "Battlefield 6 · Rangliste")),
          rss(w, h, cfg["rss"].value("title", "Blog-Updates")),
          system(w, h, cfg["system"].value("title", "System"), cfg["system"].value("net_unit", "bit")) {}
};

static bool b(const ojson& j, const char* k, bool def = false) {
    return j.contains(k) && j[k].is_boolean() ? j[k].get<bool>() : def;
}
static std::string s(const ojson& j, const char* k, const char* def = "") {
    return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(def);
}

App::App(ojson cfg, std::filesystem::path cfg_path, std::unique_ptr<Output> out, std::optional<std::filesystem::path> preview_dir, bool show_settings_on_start)
    : cfg_(std::move(cfg)), cfg_path_(std::move(cfg_path)), out_(std::move(out)), preview_dir_(std::move(preview_dir)), show_settings_on_start_(show_settings_on_start) {
    dp_ = cfg_["display"];
    if (out_) size_ = out_->size();
    else size_ = s(dp_, "orientation", "landscape").find("landscape") != std::string::npos ? std::make_pair(480, 320)
                                                                                         : std::make_pair(320, 480);
    display_ok_ = out_ == nullptr;
    page_switched_ = last_full_ = mono();
    render::apply_theme(dp_);
    const ojson &bf = cfg_["bf6"], &rs = cfg_["rss"], &sy = cfg_["system"], &dc = cfg_["discord"], &fr = cfg_["friends"];
    auto dirty = [this] { mark_dirty(); };
    if (b(bf, "enabled") && bf.contains("players") && bf["players"].is_array() && !bf["players"].empty())
        bf6_ = std::make_unique<bf6::Bf6Stats>(bf);
    if (b(rs, "enabled") && rs.contains("feeds") && rs["feeds"].is_array() && !rs["feeds"].empty())
        rss_ = std::make_unique<rss::RssFeeds>(rs);
    if (b(sy, "enabled", true)) sys_ = std::make_unique<sysinfo::SysMonitor>(sy);
    // Discord wird gebraucht, wenn seine Seite an ist oder die gemeinsame Freundesliste es einbezieht
    bool want_discord = b(dc, "enabled", true) || (b(fr, "enabled") && b(fr, "include_discord", true));
    if (want_discord && !trim(s(dc, "token")).empty()) discord_ = std::make_unique<discord::DiscordFeed>(dc, dirty);
    if (b(fr, "enabled"))
        friends_ = std::make_unique<friends::FriendsHub>(fr, b(fr, "include_discord", true) ? discord_.get() : nullptr, dirty);
}

App::~App() {
    if (friends_) friends_->stop();
    if (discord_) discord_->stop();
    if (bf6_) bf6_->stop();
    if (rss_) rss_->stop();
    if (sys_) sys_->stop();
}

App::RendererSet& App::renderers_for(int w, int h) {
    // Zeichner je Kachelgröße (eine Kachel = ganzes Display, bei breiten Leisten mehrere nebeneinander)
    auto& p = sets_[{w, h}];
    if (!p) p = std::make_unique<RendererSet>(w, h, cfg_);
    return *p;
}

void App::mark_dirty() {
    {
        std::lock_guard lk(mu_);
        dirty_ = true;
    }
    cv_.notify_all();
}

void App::step_page(int delta) {
    step_ += delta;
    mark_dirty();
}

void App::request_stop(double delay) {
    std::thread([this, delay] {
        if (delay > 0) Sleep((DWORD)(delay * 1000));
        stop_ = true;
        cv_.notify_all();
    }).detach();
}

void App::request_restart(double delay) {
    restart_ = true;
    request_stop(delay);
}

ojson App::cfg() {
    // cfg_ bleibt während der Laufzeit unverändert (Änderungen führen zum Neustart); nur setup_done kommt dazu
    ojson c = cfg_;
    if (setup_done_) c["ui"]["setup_done"] = true;
    return c;
}

void App::set_setup_done() {
    std::lock_guard lk(cfg_mu_);
    setup_done_ = true;
    config::save(cfg_path_, cfg());
}

bool App::discord_ready() { return discord_ && discord_->ready(); }

ojson App::member_list() { return discord_ ? discord_->member_list() : ojson::array(); }

std::optional<std::string> App::discord_status() {
    if (!discord_) return std::nullopt;
    return discord_->status();
}

ojson App::friend_accounts() {
    ojson accounts = friends_ ? friends_->all_accounts() : ojson::array();
    const ojson& fr = cfg_["friends"];
    std::vector<std::string> links;
    if (fr.contains("links") && fr["links"].is_array())
        for (auto& l : fr["links"])
            if (l.is_string()) links.push_back(l.get<std::string>());
    return {{"accounts", accounts}, {"suggestions", friends::suggest_links(accounts, links, b(fr, "auto_merge", true))}};
}

std::vector<std::string> App::active_pages() {
    // Die reine Discord-Seite entfällt, solange die gemeinsame Freundesliste Discord schon enthält
    // (sonst stünden dieselben Leute auf zwei Seiten).
    const ojson& fr = cfg_["friends"];
    bool discord_page = b(cfg_["discord"], "enabled", true) && !(friends_ && b(fr, "include_discord", true));
    std::map<std::string, bool> have = {{"system", sys_ != nullptr},
                                        {"discord", discord_page && discord_ != nullptr},
                                        {"friends", friends_ != nullptr},
                                        {"bf6", bf6_ != nullptr},
                                        {"rss", rss_ != nullptr}};
    std::vector<std::string> out;
    for (auto& p : dp_["pages"])
        if (p.is_string() && have[p.get<std::string>()]) out.push_back(p.get<std::string>());
    return out;
}

ojson App::connections() {
    // Dauerhafter Verbindungsstatus für Discord, Steam und Xbox (Einstellungsseite).
    // state: ok = verbunden · bad = Fehler · idle = Zugang gespeichert, aber nicht in Benutzung · off = nicht eingerichtet
    const ojson &dc = cfg_["discord"], &fr = cfg_["friends"];
    ojson out = ojson::object();
    if (discord_) {
        std::string st = discord_->status();
        if (st.empty())
            out["discord"] = {{"state", "ok"}, {"name", discord_->bot_name()},
                              {"detail", std::to_string(discord_->server_count()) + " Server"}, {"live", true}};
        else  // "Verbinde …" ist kein Fehler, alles andere schon
            out["discord"] = {{"state", st.rfind("Verbinde", 0) == 0 ? "idle" : "bad"}, {"error", st}};
    } else {
        out["discord"] = {{"state", !s(dc, "token").empty() ? "idle" : "off"}};
    }
    for (auto plat : {"steam", "xbox"}) {
        bool key_set = std::string(plat) == "steam" ? !s(fr, "steam_api_key").empty() && !s(fr, "steam_id").empty()
                                                    : !s(fr, "xbox_api_key").empty();
        ojson info = friends_ ? friends_->info(plat) : ojson();
        if (info.is_object()) {
            out[plat] = {{"state", b(info, "ok") ? "ok" : "bad"}, {"name", s(info, "name")},
                         {"detail", strprintf("%d Freunde, %d online", info.value("count", 0), info.value("online", 0))},
                         {"at", info.contains("at") ? info["at"] : ojson()}, {"error", s(info, "error")}};
        } else {
            out[plat] = {{"state", key_set ? "idle" : "off"}};
        }
    }
    return out;
}

std::optional<Image> App::last_frame() {
    std::lock_guard lk(mu_);
    return last_frame_;
}

ojson App::page_navigation() {
    std::lock_guard lk(mu_);
    return navigation_;
}

bool App::display_ok() {
    std::lock_guard lk(mu_);
    return display_ok_;
}

std::string App::display_error() {
    std::lock_guard lk(mu_);
    return display_error_;
}

std::optional<Image> App::preview_theme(const ojson& override) {
    // Aktuelles Bild mit anderen Farben zeichnen, ohne sie zu übernehmen (unter der Zeichensperre, die Anzeige kann
    // also nicht dazwischen mit den Vorschaufarben zeichnen)
    std::function<Image()> redraw;
    {
        std::lock_guard lk(mu_);
        redraw = redraw_;
    }
    if (!redraw) return std::nullopt;
    std::lock_guard rl(render::mutex());
    ojson dp = dp_;
    for (auto& [k, v] : override.items()) dp[k] = v;
    render::apply_theme(dp);
    std::optional<Image> img;
    try {
        img = redraw();
    } catch (const std::exception& e) {
        LOGW("app", "Vorschau: %s", e.what());
    }
    render::apply_theme(dp_);
    return img;
}

std::string App::friends_message() {
    // Hinweis für die Freunde-Seite, solange keine Daten da sind oder etwas nicht klappt
    const ojson& fr = cfg_["friends"];
    std::vector<std::string> problems;
    if (std::string st = friends_->status(); !st.empty()) problems.push_back(st);
    bool inc = b(fr, "include_discord", true);
    if (discord_ && inc)
        if (std::string ds = discord_->status(); !ds.empty()) problems.push_back(ds);
    bool has_source = (discord_ && inc) || !s(fr, "steam_api_key").empty() || !s(fr, "xbox_api_key").empty();
    if (!has_source) return "Noch keine Konten verbunden – bitte in den Einstellungen einrichten";
    return join(problems, " · ");
}

std::vector<App::PageEntry> App::page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r) {
    // [(Name, Seitenanzahl, Zeichenfunktion(seite, zeichner))] in der eingestellten Reihenfolge
    std::vector<PageEntry> out;
    for (auto& key : active_pages()) {
        if (key == "system") {
            out.push_back({key, 1, [this](int, RendererSet& rs) { return rs.system.render(sys_->snapshot()); }});
        } else if (key == "discord") {
            std::string st = discord_->status();
            out.push_back({key, r.discord.pages(friends.size()), [friends, st](int i, RendererSet& rs) {
                               return rs.discord.render_friends(friends, i, st.empty(), st);
                           }});
        } else if (key == "friends") {
            std::string msg = friends_message();
            bool bad = !msg.empty() && people.empty();
            out.push_back({key, r.friends.pages(people.size()), [people, msg, bad](int i, RendererSet& rs) {
                               return rs.friends.render_friends(people, i, !bad, msg);
                           }});
        } else if (key == "bf6") {
            auto players = bf6_->players();
            auto updated = bf6_->updated();
            out.push_back({key, r.bf6.pages(players.size()), [players, updated](int i, RendererSet& rs) {
                               return rs.bf6.render(players, i, updated);
                           }});
        } else if (key == "rss") {
            auto posts = rss_->posts();
            auto updated = rss_->updated();
            out.push_back({key, 1, [posts, updated](int, RendererSet& rs) { return rs.rss.render(posts, updated); }});
        }
    }
    return out;
}

std::pair<Image, std::vector<std::string>> App::draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes,
                                                     int start) {
    // Seiten ab start auf die Kacheln verteilen (eine Kachel = normales Display)
    int total = 0;
    for (auto& p : pages) total += p.n;
    std::vector<Image> frames;
    std::vector<std::string> keys;
    for (size_t k = 0; k < boxes.size(); ++k) {
        int idx = (start + (int)k) % total;
        for (auto& p : pages) {
            if (idx < p.n) {
                frames.push_back(p.fn(idx, renderers_for(boxes[k].w, boxes[k].h)));
                keys.push_back(p.key);
                break;
            }
            idx -= p.n;
        }
    }
    return {render::compose(size_.first, size_.second, boxes, frames), keys};
}

Image App::welcome() {
    int port = cfg_["ui"].value("port", 8765);
    return renderers_for(size_.first, size_.second)
        .discord.welcome("Willkommen!", "Öffne die Einstellungen über das Fuchs-Symbol im Infobereich. Lokale Adresse: 127.0.0.1:" +
                                            std::to_string(port));
}

bool App::connect_display() {
    try {
        out_->connect();
    } catch (const std::exception& e) {
        std::string err;
        if (out_->is_monitor()) {
            err = std::string(e.what()) + " – Bildschirm in den Einstellungen prüfen";
            LOGW("app", "Bildschirm: %s", e.what());
        } else {
            err = winsetup::vendor_running() ? "Die Hersteller-App des Displays läuft und belegt den Anschluss"
                                             : "Display nicht gefunden – USB-Kabel und Anschluss prüfen";
            LOGW("app", "Display: %s (%s)", err.c_str(), e.what());
        }
        std::lock_guard lk(mu_);
        display_error_ = err;
        return false;
    }
    std::lock_guard lk(mu_);
    display_ok_ = true;
    display_error_.clear();
    return true;
}

void App::output(const Image& frame, bool transition, const ojson& navigation) {
    {
        std::lock_guard lk(mu_);
        last_frame_ = frame;
        navigation_ = navigation;
    }
    if (preview_dir_) {
        std::ofstream f(*preview_dir_ / "live.png", std::ios::binary);
        std::string png = encode_png(frame);
        f.write(png.data(), (std::streamsize)png.size());
    }
    if (!out_ || !display_ok()) return;
    // Teil-Updates übertragen nur geänderte Bänder; ein verlorenes Datenpaket bliebe sonst als Streifen stehen.
    // Deshalb bei jedem Seitenwechsel und alle 5 Minuten ein Vollbild.
    double now = mono();
    if (transition || now - last_full_ > 300) {
        out_->invalidate();
        last_full_ = now;
    }
    try {
        int rows = out_->show(frame);
        LOGD("app", "%d Zeilen übertragen", rows);
    } catch (const std::exception& e) {
        std::lock_guard lk(mu_);
        display_ok_ = false;
        display_error_ = "Verbindung zum Display verloren – wird erneut versucht";
        LOGW("app", "Display: %s", e.what());
    }
}

void App::display_loop() {
    std::string last_minute;
    double next_try = 0;
    if (out_) connect_display();
    double page_seconds = dp_.value("page_seconds", 15);
    while (!stop_) {
        {
            std::unique_lock lk(mu_);
            cv_.wait_for(lk, std::chrono::seconds(1), [&] { return dirty_ || stop_; });
        }
        if (stop_) break;
        if (out_ && !display_ok() && mono() >= next_try)
            if (!connect_display()) next_try = mono() + 10;
        std::string minute = clock_hhmm();
        bool page_due = mono() - page_switched_ >= page_seconds;
        // Die Systemansicht wird jede Sekunde neu gezeichnet, die anderen nur bei Änderungen
        bool live = std::count(current_.begin(), current_.end(), "system") > 0;
        bool dirty;
        {
            std::lock_guard lk(mu_);
            dirty = dirty_;
            if (!(dirty || page_due || live || minute != last_minute)) continue;
            dirty_ = false;
        }
        last_minute = minute;
        auto active = active_pages();
        std::vector<Friend> friends, people;
        if (discord_ && std::count(active.begin(), active.end(), "discord")) friends = discord_->friends();
        if (friends_) people = friends_->people();
        Image frame;
        ojson navigation = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
        bool switched = false;
        {
            std::lock_guard rl(render::mutex());
            // Breite Leisten: mehrere Kacheln; gibt es weniger Seiten als Kacheln, werden die Kacheln breiter
            auto boxes = render::tile_layout(size_.first, size_.second);
            auto pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h));
            int total = 0;
            for (auto& p : pages) total += p.n;
            if (0 < total && total < (int)boxes.size()) {
                boxes = render::tile_layout(size_.first, size_.second, total);
                pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h));
                total = 0;
                for (auto& p : pages) total += p.n;
            }
            int step = step_.exchange(0);
            if (!total) {
                {
                    std::lock_guard lk(mu_);
                    redraw_ = [this] { return welcome(); };
                }
                frame = welcome();
                current_.clear();
                {
                    std::lock_guard lk(tap_mu_);
                    tap_boxes_.clear();
                    tap_keys_.clear();
                }
            } else {
                int cols = std::min((int)boxes.size(), total);
                if (page_due || step) {
                    if (total > cols) {
                        switched = true;
                        page_ = (((page_ + (step ? step : 1) * cols) % total) + total) % total;
                    }
                    page_switched_ = mono();
                }
                page_ = total > cols ? page_ % total : 0;
                int start = page_;
                {
                    std::lock_guard lk(mu_);
                    redraw_ = [this, pages, boxes, start] { return draw(pages, boxes, start).first; };
                }
                auto [f, keys] = draw(pages, boxes, start);
                frame = std::move(f);
                current_ = keys;
                {
                    std::lock_guard lk(tap_mu_);  // für „Tippen öffnet Blogbeitrag“
                    tap_boxes_ = boxes;
                    tap_keys_ = keys;
                }
                navigation = {{"index", start}, {"total", total}, {"visible", cols},
                              {"current", keys}, {"can_step", total > cols}};
            }
        }
        output(frame, switched, navigation);
        if (switched) page_switched_ = mono();  // Anzeigedauer ab Ende der Übertragung zählen
        for (int i = 0; i < 10 && !stop_; ++i) Sleep(50);  // max. ~2 Bilder/s, damit Status-Stürme das Display nicht fluten
    }
}

void App::show_settings() { if (show_settings_callback_) show_settings_callback_(); }

bool App::open_post_at(int x, int y) {
    // Tippen auf einen Blogbeitrag (Touch an): Artikel im Standardbrowser auf dem PC öffnen
    if (!rss_) return false;
    render::Box box{};
    {
        std::lock_guard lk(tap_mu_);
        size_t i = 0;
        for (; i < tap_boxes_.size(); ++i) {
            auto& b = tap_boxes_[i];
            if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) break;
        }
        if (i >= tap_boxes_.size() || i >= tap_keys_.size() || tap_keys_[i] != "rss") return false;
        box = tap_boxes_[i];
    }
    int row = render::RssRenderer::row_at(box.w, box.h, y - box.y);
    auto posts = rss_->posts();
    if (row < 0 || row >= (int)posts.size()) return false;
    std::string link = trim(posts[row].link);
    // Nur Webadressen aus dem Feed öffnen (keine Dateien oder anderen Programme)
    std::string scheme = lower_ascii(link.substr(0, link.find(':')));
    if (link.empty() || (scheme != "http" && scheme != "https") || link.find("://") == std::string::npos) return false;
    LOGI("app", "Blogbeitrag per Tippen geöffnet: %s", posts[row].source.c_str());
    ShellExecuteW(nullptr, L"open", widen(link).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

bool App::setup_touch() {
    if (auto* mon = dynamic_cast<MonitorDisplay*>(out_.get())) return mon->setup_touch();
    return open_touch_setup();
}

void App::run() {
    if (auto* mon = dynamic_cast<MonitorDisplay*>(out_.get())) {
        mon->on_step = [this](int d) { step_page(d); };
        mon->on_tap = [this](int x, int y) { return open_post_at(x, y); };
    }
    if (sys_) sys_->start();
    auto dirty = [this] { mark_dirty(); };
    if (bf6_) bf6_->start(dirty);
    if (rss_) rss_->start(dirty);
    if (discord_) discord_->start();
    if (friends_) friends_->start();
    int port = cfg_["ui"].value("port", 8765);
    std::unique_ptr<SettingsWindow> settings_window;
    std::unique_ptr<SettingsServer> server;
    if (b(cfg_["ui"], "enabled", true)) {
        server = std::make_unique<SettingsServer>(*this, [this, &settings_window] { if (settings_window) settings_window->show(); });
        if (server->start()) {
            settings_window = std::make_unique<SettingsWindow>(port);
            show_settings_callback_ = [this, &settings_window] { if (settings_window) settings_window->show(); };
            if (show_settings_on_start_) settings_window->show();
        }
    }
    Tray tray(
        "FuchsLabs USB-Display",
        [this] { show_settings(); },
        [this] { request_restart(); }, [this] { request_stop(); });
    // Bebilderte Anleitung im Standardbrowser, unabhängig vom Einstellungsfenster
    int guide_port = cfg_["ui"].value("port", 8765);
    tray.on_guide = [guide_port] {
        std::wstring url = L"http://127.0.0.1:" + std::to_wstring(guide_port) + L"/anleitung?lang=" + widen(i18n::lang());
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    };
    tray.start();
    display_loop();
    tray.stop();
    // War das Einstellungsfenster beim Neustart offen (z. B. nach dem Speichern), öffnet es die neue Instanz wieder
    settings_open_at_exit_ = settings_window && settings_window->visible();
    if (server) server->stop();
    show_settings_callback_ = {};
    settings_window.reset();
    if (friends_) friends_->stop();
    if (discord_) discord_->stop();
    if (bf6_) bf6_->stop();
    if (rss_) rss_->stop();
    if (sys_) sys_->stop();
    if (out_) {
        if (b(dp_, "screen_off_on_exit", true) && !restart_) {
            try {
                out_->screen_off();
            } catch (...) {
            }
        }
        out_->close();
    }
}

}  // namespace fl
