// © 2026 FuchsLabs
#include "app.h"

#include <windows.h>
#include <shellapi.h>

#include <fstream>
#include <ctime>

#include "bf6.h"
#include "discord.h"
#include "friends.h"
#include "ha.h"
#include "i18n.h"
#include "native_settings.h"
#include "plugin_render.h"
#include "rss.h"
#include "sysinfo.h"
#include "settings_window.h"
#include "tray.h"
#include "update.h"
#include "util.h"
#include "webui.h"
#include "winsetup.h"

namespace fl {

struct App::RendererSet {
    int width, height;
    render::Renderer discord, friends;
    render::Bf6Renderer bf6;
    render::RssRenderer rss;
    render::SysRenderer system;
    render::HaRenderer ha;
    RendererSet(int w, int h, const ojson& cfg)
        : width(w), height(h), discord(w, h, cfg["display"].value("title", "Discord-Freunde")),
          friends(w, h, cfg["friends"].value("title", "Freunde")),
          bf6(w, h, cfg["bf6"].value("title", "Battlefield 6 · Rangliste")),
          rss(w, h, cfg["rss"].value("title", "Blog-Updates")),
          system(w, h, cfg["system"].value("title", "System"), cfg["system"].value("net_unit", "bit")),
          ha(w, h, cfg["homeassistant"].value("title", "Home Assistant")) {}
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
    primary_view_.switched = companion_view_.switched = last_full_ = mono();
    // Companion-App: entweder der Ausgang selbst (display.output = "network") oder zusätzlich zum Display
    companion_ = dynamic_cast<NetworkDisplay*>(out_.get());
    if (!companion_ && b(cfg_["companion"], "enabled") && !preview_dir_) {
        companion_output_ = std::make_unique<NetworkDisplay>(cfg_["companion"].value("port", 8777), cfg_["ui"].value("port", 8765),
                                                             cfg_path_.parent_path() / L"companion.json", size_.first, size_.second);
        companion_ = companion_output_.get();
    }
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
    if (b(cfg_["homeassistant"], "enabled")) {
        ha_ = std::make_unique<ha::HaClient>(cfg_["homeassistant"], touch_enabled(), dirty);
        if (!ha_->configured()) ha_.reset();
    }
    if (b(fr, "enabled"))
        friends_ = std::make_unique<friends::FriendsHub>(fr, b(fr, "include_discord", true) ? discord_.get() : nullptr, dirty);
    plugins_ = std::make_unique<plugins::Store>(cfg_path_.parent_path() / L"plugins");
    if (auto result = plugins_->load(); !result) LOGW("plugins", "Plugin-Bibliothek: %s", result.error.c_str());
    // Update-Hinweis: nie in einer Testinstanz und nur, wenn in den Einstellungen nicht abgeschaltet
    if (b(cfg_["ui"], "update_check", true) && !test_instance() && !preview_dir_)
        update_ = std::make_unique<update::UpdateChecker>(cfg_path_.parent_path() / L"update-check.json", [this](const update::Release& rel) {
            bool en = i18n::lang() == "en";
            std::lock_guard lk(notify_mu_);
            if (tray_)
                tray_->notify(en ? "New version available" : "Neue Version verfügbar",
                              strprintf(en ? "Version %s is available. Click for details." : "Version %s ist verfügbar. Zum Ansehen klicken.",
                                        rel.version.c_str()));
        });
}

ojson App::update_info() { return update_ ? ojson::parse(update_->info().dump()) : ojson(); }
bool App::update_check_now() { return update_ && update_->check_now(); }

ojson App::plugin_catalog() {
    ojson catalog = ojson::array();
    if (!plugins_) return catalog;
    for (const auto& entry : plugins_->entries()) {
        ojson hosts = ojson::array();
        for (const auto& source : entry.manifest.sources) hosts.push_back(source.host);
        catalog.push_back({{"id", entry.manifest.id}, {"name", entry.manifest.name},
                           {"version", entry.manifest.version}, {"description", entry.manifest.description},
                           {"enabled", entry.enabled}, {"network_required", !entry.manifest.sources.empty()},
                           {"source_hosts", hosts}});
    }
    return catalog;
}

bool App::plugin_import(std::string_view manifest, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    auto result = plugins_->import_manifest(manifest);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

bool App::plugin_set_enabled(const std::string& id, bool enabled, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    auto result = plugins_->set_enabled(id, enabled);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

bool App::plugin_remove(const std::string& id, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    auto result = plugins_->remove(id);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

std::optional<Image> App::plugin_preview(std::string_view manifest, std::string& error) {
    auto result = plugins::parse_manifest(manifest);
    if (!result) { error = result.error; return std::nullopt; }
    plugins::RenderContext context;
    context.sources = plugins::sample_data(*result.manifest);
    context.time = clock_hhmm();
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char date[32] = {};
    std::strftime(date, sizeof date, "%Y-%m-%d", &local);
    context.date = date;
    auto page = plugins::render_page(*result.manifest, context);
    std::lock_guard draw_lock(render::mutex());
    render::apply_theme(dp_);
    auto [width, height] = size();
    return plugins::draw_page(page, width, height, true);
}

bool App::touch_enabled() const {
    // Schalter brauchen Touch: nur bei Ausgabe auf einen Bildschirm, mit eingeschaltetem Touch und erkanntem Touchscreen
    // Die Companion-App meldet Tipps immer (Touchscreen des Handys/Tablets)
    if (companion_) return true;
    return dp_.value("output", "serial") == "monitor" && b(dp_, "touch", true) && touch_info().value("available", false);
}

App::~App() {
    { std::lock_guard lock(stop_threads_mu_); stop_threads_.clear(); }
    if (update_) update_->stop();
    if (ha_) ha_->stop();
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

void App::step_page(int delta, bool companion) {
    (companion && companion_output_ ? companion_view_.step : primary_view_.step) += delta;
    mark_dirty();
}

void App::request_stop(double delay) {
    if (delay <= 0) { stop_ = true; cv_.notify_all(); return; }
    std::lock_guard lock(stop_threads_mu_);
    stop_threads_.emplace_back([this, delay] {
        Sleep(static_cast<DWORD>(delay * 1000));
        stop_ = true;
        cv_.notify_all();
    });
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

std::vector<std::string> App::active_pages(bool companion) {
    // Die reine Discord-Seite entfällt, solange die gemeinsame Freundesliste Discord schon enthält
    // (sonst stünden dieselben Leute auf zwei Seiten).
    const ojson& fr = cfg_["friends"];
    bool discord_page = b(cfg_["discord"], "enabled", true) && !(friends_ && b(fr, "include_discord", true));
    std::map<std::string, bool> have = {{"system", sys_ != nullptr},
                                        {"discord", discord_page && discord_ != nullptr},
                                        {"friends", friends_ != nullptr},
                                        {"bf6", bf6_ != nullptr},
                                        {"rss", rss_ != nullptr},
                                        {"homeassistant", ha_ != nullptr}};
    std::vector<std::string> out;
    bool network = dp_.value("output", "serial") == "network";
    const auto& selected = (companion || network) ? cfg_["companion"]["pages"] : dp_["visible_pages"];
    bool separate = companion_ != nullptr;
    const auto& order = (companion || network) ? selected : dp_["pages"];
    for (auto& p : order)
        if (p.is_string() && have[p.get<std::string>()] &&
            (!separate || std::find(selected.begin(), selected.end(), p) != selected.end())) out.push_back(p.get<std::string>());
    // Only offline declarative plugins are shown as live pages. Sample data is exclusive to preview.
    if (plugins_) for (const auto& entry : plugins_->entries())
        if (entry.enabled && entry.manifest.sources.empty()) out.push_back("plugin:" + entry.manifest.id);
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
    {
        const ojson& hc = cfg_["homeassistant"];
        bool set = !s(hc, "url").empty() && !s(hc, "token").empty();
        ojson info = ha_ ? ha_->info() : ojson();
        if (info.is_object() && b(info, "ok"))
            out["homeassistant"] = {{"state", "ok"}, {"detail", std::to_string(info.value("count", 0)) + " Entitäten"}, {"at", info["at"]}};
        else if (info.is_object() && !s(info, "error").empty())
            out["homeassistant"] = {{"state", "bad"}, {"error", s(info, "error")}, {"at", info.contains("at") ? info["at"] : ojson()}};
        else
            out["homeassistant"] = {{"state", set ? "idle" : "off"}};
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

std::pair<int, int> App::size() const {
    std::lock_guard lk(mu_);
    return last_frame_ ? std::make_pair(last_frame_->w, last_frame_->h) : size_;
}

std::optional<Image> App::last_frame(bool companion) {
    std::lock_guard lk(mu_);
    return companion && companion_output_ ? companion_frame_ : last_frame_;
}

ojson App::page_navigation(bool companion) {
    std::lock_guard lk(mu_);
    return companion && companion_output_ ? companion_navigation_ : navigation_;
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

std::vector<App::PageEntry> App::page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r, bool companion) {
    // [(Name, Seitenanzahl, Zeichenfunktion(seite, zeichner))] in der eingestellten Reihenfolge
    std::vector<PageEntry> out;
    for (auto& key : active_pages(companion)) {
        if (key.rfind("plugin:", 0) == 0) {
            auto installed = plugins_ ? plugins_->find(key.substr(7)) : std::nullopt;
            if (!installed || !installed->enabled || !installed->manifest.sources.empty()) continue;
            auto manifest = std::move(installed->manifest);
            plugins::RenderContext context;
            context.time = clock_hhmm();
            std::time_t now = std::time(nullptr);
            std::tm local{};
            localtime_s(&local, &now);
            char date[32] = {};
            std::strftime(date, sizeof date, "%Y-%m-%d", &local);
            context.date = date;
            const auto page = plugins::render_page(manifest, context);
            out.push_back({key, 1, [page](int, RendererSet& rs) {
                // RendererSet owns the tile size through its standard renderer.
                return plugins::draw_page(page, rs.width, rs.height);
            }});
        } else if (key == "system") {
            PageEntry pe{key, 1, [this](int, RendererSet& rs) { return rs.system.render(sys_->snapshot()); }};
            // Tippen auf RAM / Datenträger / Netzwerk öffnet das passende Windows-Werkzeug bzw. den Geschwindigkeitstest
            for (int zone : {render::SysRenderer::ZONE_RAM, render::SysRenderer::ZONE_DISK, render::SysRenderer::ZONE_NET})
                pe.actions.push_back([zone] {
                    if (touch::open_system(zone)) LOGI("app", "Touch: Systemwerkzeug %d geöffnet", zone);
                    else LOGW("app", "Touch: Systemwerkzeug %d ließ sich nicht öffnen", zone);
                });
            out.push_back(std::move(pe));
        } else if (key == "discord") {
            std::string st = discord_->status();
            out.push_back({key, r.discord.pages(friends.size()), [friends, st](int i, RendererSet& rs) {
                               return rs.discord.render_friends(friends, i, st.empty(), st);
                           }, [friends, st](int i, RendererSet& rs) {
                               return st.empty() ? touch::friend_rows(friends, i, rs.discord.rows_per_page()) : touch::Rows{};
                           }});
        } else if (key == "friends") {
            std::string msg = friends_message();
            bool bad = !msg.empty() && people.empty();
            out.push_back({key, r.friends.pages(people.size()), [people, msg, bad](int i, RendererSet& rs) {
                               return rs.friends.render_friends(people, i, !bad, msg);
                           }, [people, bad](int i, RendererSet& rs) {
                               return bad ? touch::Rows{} : touch::friend_rows(people, i, rs.friends.rows_per_page());
                           }});
        } else if (key == "bf6") {
            auto players = bf6_->players();
            auto updated = bf6_->updated();
            out.push_back({key, r.bf6.pages(players.size()), [players, updated](int i, RendererSet& rs) {
                               return rs.bf6.render(players, i, updated);
                           }, [players, updated](int i, RendererSet& rs) {
                               return updated ? touch::player_rows(players, i, rs.bf6.rows_per_page()) : touch::Rows{};
                           }});
        } else if (key == "homeassistant") {
            auto entities = ha_->entities();
            std::string err = ha_->status();
            bool data = ha_->has_data();
            ojson inf = ha_->info();
            std::optional<std::time_t> updated;
            if (data && inf.contains("at") && inf["at"].is_number()) updated = (std::time_t)inf["at"].get<long long>();
            PageEntry pe{key, 1, [entities, err, data, updated](int, RendererSet& rs) { return rs.ha.render(entities, data, err, updated); }};
            for (auto& e : entities)
                pe.actions.push_back(e.toggle ? std::function<void()>([this, id = e.id] { ha_->request_toggle(id); }) : std::function<void()>());
            out.push_back(std::move(pe));
        } else if (key == "rss") {
            auto posts = rss_->posts();
            auto updated = rss_->updated();
            out.push_back({key, 1, [posts, updated](int, RendererSet& rs) { return rs.rss.render(posts, updated); },
                           [posts, updated](int, RendererSet&) { return updated ? touch::post_rows(posts) : touch::Rows{}; }});
        }
    }
    return out;
}

App::DrawResult App::draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes, int start, std::pair<int, int> size) {
    // Seiten ab start auf die Kacheln verteilen (eine Kachel = normales Display)
    int total = 0;
    for (auto& p : pages) total += p.n;
    std::vector<Image> frames;
    std::vector<std::string> keys;
    std::vector<touch::Tile> taps;
    for (size_t k = 0; k < boxes.size(); ++k) {
        int idx = (start + (int)k) % total;
        for (auto& p : pages) {
            if (idx < p.n) {
                frames.push_back(p.fn(idx, renderers_for(boxes[k].w, boxes[k].h)));
                keys.push_back(p.key);
                touch::Tile tile{boxes[k], p.key == "rss", p.targets ? p.targets(idx, renderers_for(boxes[k].w, boxes[k].h)) : touch::Rows{}};
                tile.ha = p.key == "homeassistant";
                tile.sys = p.key == "system";
                tile.cells = p.actions;
                taps.push_back(std::move(tile));
                break;
            }
            idx -= p.n;
        }
    }
    return {render::compose(size.first, size.second, boxes, frames), keys, taps};
}

Image App::welcome(std::pair<int, int> size) {
    int port = cfg_["ui"].value("port", 8765);
    return renderers_for(size.first, size.second)
        .discord.welcome("Willkommen!", "Öffne die Einstellungen über das Fuchs-Symbol im Infobereich. Lokale Adresse: 127.0.0.1:" +
                                            std::to_string(port));
}

bool App::connect_display() {
    try {
        out_->connect();
    } catch (const std::exception& e) {
        std::string err;
        if (out_.get() == companion_) {
            err = std::string("Companion-App · ") + e.what();
            LOGW("app", "Companion: %s", e.what());
        } else if (out_->is_monitor()) {
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

void App::render_view(bool secondary, bool dirty) {
    auto& view = secondary ? companion_view_ : primary_view_;
    bool phone = companion_ && (secondary || out_.get() == companion_);
    auto size = phone ? companion_->size() : size_;
    auto& settings = phone ? cfg_["companion"] : dp_;
    bool due = mono() - view.switched >= settings.value("page_seconds", 15);
    auto minute = clock_hhmm();
    bool live = std::count(view.current.begin(), view.current.end(), "system") > 0;
    if (!dirty && !due && !live && view.step == 0 && view.size == size && view.minute == minute) return;
    view.minute = minute;
    view.size = size;
    auto active = active_pages(phone);
    std::vector<Friend> friends, people;
    if (discord_ && std::count(active.begin(), active.end(), "discord")) friends = discord_->friends();
    if (friends_) people = friends_->people();
    Image frame;
    std::vector<touch::Tile> taps;
    ojson navigation = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    bool switched = false;
    {
        std::lock_guard rl(render::mutex());
        // Bound cached renderer sets when a resizable window reports many intermediate sizes.
        if (sets_.size() > 16) sets_.clear();
        auto boxes = render::tile_layout(size.first, size.second);
        auto pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h), phone);
        int total = 0;
        for (auto& p : pages) total += p.n;
        if (0 < total && total < (int)boxes.size()) {
            boxes = render::tile_layout(size.first, size.second, total);
            pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h), phone);
            total = 0; for (auto& p : pages) total += p.n;
        }
        int step = view.step.exchange(0);
        if (!total) {
            frame = welcome(size); view.current.clear();
            if (!secondary) { std::lock_guard lk(mu_); redraw_ = [this, size] { return welcome(size); }; }
        } else {
            int cols = std::min((int)boxes.size(), total);
            if (due || step) {
                if (total > cols) {
                    switched = true;
                    view.page = (((view.page + (step ? step : 1) * cols) % total) + total) % total;
                }
                view.switched = mono();
            }
            view.page = total > cols ? view.page % total : 0;
            int start = view.page;
            if (!secondary) {
                std::lock_guard lk(mu_);
                redraw_ = [this, pages, boxes, start, size] { return draw(pages, boxes, start, size).frame; };
            }
            auto [f, keys, targets] = draw(pages, boxes, start, size);
            frame = std::move(f); view.current = keys; taps = std::move(targets);
            navigation = {{"index", start}, {"total", total}, {"visible", cols}, {"current", keys}, {"can_step", total > cols}};
        }
    }
    {
        std::lock_guard lk(tap_mu_);
        if (secondary) {
            // Snapshot and its touch targets are published together.
            companion_output_->show(frame);
            companion_taps_ = std::move(taps);
            std::lock_guard state(mu_);
            companion_frame_ = frame; companion_navigation_ = navigation;
        } else {
            output(frame, switched, navigation); tap_tiles_ = std::move(taps);
        }
    }
    if (switched) view.switched = mono();
}

void App::display_loop() {
    double next_try = 0, companion_try = 0;
    if (out_) connect_display();
    while (!stop_) {
        bool dirty;
        {
            std::unique_lock lk(mu_);
            cv_.wait_for(lk, std::chrono::seconds(1), [&] { return dirty_ || stop_; });
            dirty = dirty_;
            dirty_ = false;
        }
        if (stop_) break;
        if (out_ && !display_ok() && mono() >= next_try)
            if (!connect_display()) next_try = mono() + 10;
        if (companion_output_ && !companion_output_->running() && mono() >= companion_try) {
            try { companion_output_->connect(); }
            catch (const std::exception& e) { LOGW("app", "Companion: %s", e.what()); }
            companion_try = mono() + 30;
        }
        render_view(false, dirty);
        if (companion_output_) render_view(true, dirty);
        for (int i = 0; i < 10 && !stop_; ++i) Sleep(50);
    }
}

void App::show_settings() { if (show_settings_callback_) show_settings_callback_(); }

bool App::open_item_at(int x, int y) {
    if (!b(dp_, "touch", true)) return false;
    return tap_at(x, y);
}

void App::companion_tap(int x, int y, int w, int h) {
    // Wie Tippen auf dem Bildschirm-Ausgang: erst Ziel/Schalter, sonst links (Drittel) = zurück, sonst weiter
    bool secondary = companion_output_ != nullptr;
    if (tap_at(x, y, secondary, w, h)) return;
    auto frame = last_frame(secondary);
    if (frame) step_page(x < frame->w / 3 ? -1 : 1, secondary);
}

bool App::tap_at(int x, int y, bool secondary, int w, int h) {
    touch::Targets targets;
    std::function<void()> action;
    {
        std::lock_guard lk(tap_mu_);
        if (w > 0) {
            auto image = last_frame(secondary);
            if (!image || image->w != w || image->h != h) return true;
        }
        if (auto* a = touch::action_at(secondary ? companion_taps_ : tap_tiles_, x, y)) action = *a;
        if (!action) {
            auto selected = touch::at(secondary ? companion_taps_ : tap_tiles_, x, y);
            if (!selected) return false;
            targets = *selected;
        }
    }
    if (action) {
        action();
        return true;   // Schalter angetippt: nicht zusätzlich die Seite wechseln
    }
    bool opened = touch::open(targets);
    if (opened) LOGI("app", "Touch-Ziel an Browser/Chat-App übergeben");
    else LOGW("app", "Touch-Ziel nicht geöffnet: keine passende App oder Start fehlgeschlagen");
    return true;  // Tipp verbraucht, auch bei Startfehler nicht unvermittelt die Seite wechseln
}

bool App::setup_touch() {
    if (auto* mon = dynamic_cast<MonitorDisplay*>(out_.get())) return mon->setup_touch();
    return open_touch_setup();
}

void App::run() {
    if (auto* mon = dynamic_cast<MonitorDisplay*>(out_.get())) {
        mon->on_step = [this](int d) { step_page(d); };
        mon->on_tap = [this](int x, int y) { return open_item_at(x, y); };
    }
    if (companion_) {
        companion_->on_tap = [this](int x, int y, int w, int h) { companion_tap(x, y, w, h); };
        companion_->on_swipe = [this](int d) { step_page(d, true); };
        companion_->on_resize = [this] { mark_dirty(); };
    }
    if (sys_) sys_->start();
    auto dirty = [this] { mark_dirty(); };
    if (bf6_) bf6_->start(dirty);
    if (rss_) rss_->start(dirty);
    if (ha_) ha_->start();
    if (discord_) discord_->start();
    if (friends_) friends_->start();
    int port = cfg_["ui"].value("port", 8765);
    std::unique_ptr<SettingsWindow> settings_window;
    std::unique_ptr<NativeSettings> native_settings;
    std::unique_ptr<SettingsServer> server;
    if (b(cfg_["ui"], "enabled", true)) {
        server = std::make_unique<SettingsServer>(*this, [this, &native_settings, &settings_window] {
            if (native_settings) native_settings->show();
            else if (settings_window) settings_window->show();
        });
        if (server->start()) {
            settings_window = std::make_unique<SettingsWindow>(port);
            NativePluginActions actions;
            actions.list = [this] {
                std::vector<NativePluginEntry> result;
                for (const auto& item : plugin_catalog())
                    result.push_back({item.value("id", ""), item.value("name", ""),
                                      item.value("version", ""), item.value("enabled", false)});
                return result;
            };
            actions.import_manifest = [this](const std::filesystem::path& path) {
                std::error_code ec;
                auto length = std::filesystem::file_size(path, ec);
                if (ec || length > plugins::manifest_limit) throw std::runtime_error("Manifest ist nicht lesbar oder zu groß.");
                std::ifstream file(path, std::ios::binary);
                if (!file) throw std::runtime_error("Manifest konnte nicht geöffnet werden.");
                std::string content(static_cast<size_t>(length), '\0');
                if (!file.read(content.data(), static_cast<std::streamsize>(content.size())))
                    throw std::runtime_error("Manifest konnte nicht gelesen werden.");
                std::string error;
                if (!plugin_import(content, error)) throw std::runtime_error(error);
            };
            actions.set_enabled = [this](const std::string& id, bool enabled) {
                std::string error;
                if (!plugin_set_enabled(id, enabled, error)) throw std::runtime_error(error);
            };
            actions.remove = [this](const std::string& id) {
                std::string error;
                if (!plugin_remove(id, error)) throw std::runtime_error(error);
            };
            actions.open_advanced = [&settings_window] { if (settings_window) settings_window->show(); };
            try { native_settings = std::make_unique<NativeSettings>(*this, std::move(actions)); }
            catch (const std::exception& error) { LOGW("settings", "Native UI: %s", error.what()); }
            show_settings_callback_ = [&native_settings, &settings_window] {
                if (native_settings) native_settings->show();
                else if (settings_window) settings_window->show();
            };
            if (show_settings_on_start_) show_settings();
        }
    }
    Tray tray(
        "ScreenBuddy",
        [this] { show_settings(); },
        [this] { request_restart(); }, [this] { request_stop(); });
    // Bebilderte Anleitung im Standardbrowser, unabhängig vom Einstellungsfenster
    int guide_port = cfg_["ui"].value("port", 8765);
    tray.on_guide = [guide_port] {
        std::wstring url = L"http://127.0.0.1:" + std::to_wstring(guide_port) + L"/anleitung?lang=" + widen(i18n::lang());
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    };
    tray.on_balloon = [this] { show_settings(); };
    tray.start();
    {
        std::lock_guard lk(notify_mu_);
        tray_ = &tray;
    }
    if (update_) update_->start();
    display_loop();
    if (update_) update_->stop();
    {
        std::lock_guard lk(notify_mu_);
        tray_ = nullptr;
    }
    tray.stop();
    // War das Einstellungsfenster beim Neustart offen (z. B. nach dem Speichern), öffnet es die neue Instanz wieder
    settings_open_at_exit_ = (native_settings && native_settings->visible()) ||
                             (settings_window && settings_window->visible());
    if (server) server->stop();
    if (companion_output_) {
        if (b(dp_, "screen_off_on_exit", true) && !restart_) companion_output_->screen_off();
        companion_output_->close();
    }
    show_settings_callback_ = {};
    native_settings.reset();
    settings_window.reset();
    if (friends_) friends_->stop();
    if (ha_) ha_->stop();
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
