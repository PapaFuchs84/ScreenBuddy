// © 2026 FuchsLabs
#include "app.h"
#include <set>
#include <cmath>

#include <windows.h>
#include <shellapi.h>

#include <fstream>
#include <chrono>
#include <ctime>
#include <set>

#include "bf6.h"
#include "page_samples.h"
#include "discord.h"
#include "friends.h"
#include "ha.h"
#include "i18n.h"
#include "native_settings.h"
#include "plugin_catalog_net.h"
#include "plugin_render.h"
#include "plugin_transport.h"
#include "rss.h"
#include "amp_logic.h"
#include "audio_levels.h"
#include "spotify.h"
#include "theme.h"
#include "tmdb.h"
#include "sysinfo.h"
#include "tray.h"
#include "update.h"
#include "distribution.h"
#include "util.h"
#include "webui.h"
#include "winsetup.h"

namespace fl {

namespace builtin {
namespace {
namespace fs=std::filesystem;
bool plain_path(const fs::path& path) {
    // Symlinks, Junctions und andere Reparse-Punkte einschließlich Eltern ablehnen.
    for (auto current = path; !current.empty();) {
        std::error_code ec;
        auto status = fs::symlink_status(current, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) return false;
        if (!ec && fs::is_symlink(status)) return false;
#ifdef _WIN32
        const auto attr = GetFileAttributesW(current.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return true;
}
bool regular_or_missing(const fs::path& path) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    auto status = fs::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory) return true;
    return !ec && (status.type() == fs::file_type::not_found || fs::is_regular_file(status));
}
bool ready_directory(const fs::path& path, bool create) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    if (create) fs::create_directories(path, ec);
    if (ec) return false;
    return fs::is_directory(path, ec) && !ec && plain_path(path);
}
bool read_bounded(const fs::path& path, std::string& out, size_t limit = plugins::manifest_limit) {
    if (!regular_or_missing(path)) return false;
#ifdef _WIN32
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    bool ok = GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow <= limit;
    if (ok) {
        out.resize(info.nFileSizeLow);
        DWORD bytes = 0;
        ok = ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &bytes, nullptr) && bytes == out.size();
    }
    CloseHandle(handle);
    return ok;
#else
    int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size >= 0 &&
        static_cast<uint64_t>(info.st_size) <= limit;
    if (ok) {
        out.resize(static_cast<size_t>(info.st_size));
        size_t total = 0;
        while (total < out.size()) {
            auto n = read(fd, out.data() + total, out.size() - total);
            if (n <= 0) { ok = false; break; }
            total += static_cast<size_t>(n);
        }
    }
    close(fd);
    return ok;
#endif
}
bool atomic_write(const fs::path& target, std::string_view data) {
    if (!regular_or_missing(target) || !ready_directory(target.parent_path(), false)) return false;
    static std::atomic<uint64_t> sequence{0};
    // Exklusiv anlegen: keine bestehenden Links verfolgen oder temporäre Dateien überschreiben.
    const auto token = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path temporary = target;
    temporary += ".tmp-" + std::to_string(token) + "-" + std::to_string(++sequence);
#ifdef _WIN32
    HANDLE handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0;
    bool ok = WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &count, nullptr) && count == data.size();
    if (ok) ok = FlushFileBuffers(handle) != 0;
    if (!CloseHandle(handle)) ok = false;
    if (ok) ok = regular_or_missing(target) && MoveFileExW(temporary.c_str(), target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    bool ok = true;
    size_t total = 0;
    while (total < data.size()) {
        auto n = write(fd, data.data() + total, data.size() - total);
        if (n <= 0) { ok = false; break; }
        total += static_cast<size_t>(n);
    }
    if (ok) ok = fsync(fd) == 0;
    if (close(fd) != 0) ok = false;
    if (ok) {
        std::error_code ec;
        if (!regular_or_missing(target)) ok = false;
        else { fs::rename(temporary, target, ec); ok = !ec; }
    }
#endif
    if (!ok) { std::error_code ignored; fs::remove(temporary, ignored); }
    return ok;
}
}
LayoutStore::LayoutStore(fs::path directory):directory_(std::move(directory)) {
    const bool ready=ready_directory(directory_,true);
    for(const auto& c:catalogs()) {
        auto& state=states_[c.key];
        auto path=directory_/("builtin."+c.key+".layout.json");
        std::error_code ec;
        if(!ready || !regular_or_missing(path)) state.state="invalid";
        else if(fs::exists(path,ec)) {
            std::string text;
            if(!read_bounded(path,text,plugins::arrangement_limit)) state.state="invalid";
            else {
                auto parsed=parse(c,text);
                if(parsed) { if(!is_standard(c,*parsed.overlay)) state.overlay=std::move(parsed.overlay); state.state=state.overlay?"ok":"none"; }
                else state.state=parsed.error=="Catalog version is stale."?"stale":"invalid";
            }
        } else if(ec) state.state="invalid";
        if(state.state=="stale" || state.state=="invalid") LOGW("layout","Eingebaute Anordnung %s: %s; Standard aktiv",c.key.c_str(),state.state.c_str());
    }
}
LayoutState LayoutStore::get(const std::string& key) const {
    std::lock_guard lock(mutex_); auto it=states_.find(key); return it==states_.end()?LayoutState{}:it->second;
}
Json LayoutStore::view(const std::string& key) const {
    auto c=find(key); if(!c) return {};
    auto state=get(key);
    return {{"state",state.state},{"layout_revision",state.revision},{"overlay",to_json(state.overlay.value_or(standard(*c)))}};
}
std::string LayoutStore::apply(const std::string& key,std::string_view text) {
    auto c=find(key); if(!c) return "Unknown built-in page.";
    auto parsed=parse(*c,text); if(!parsed) return parsed.error;
    std::lock_guard lock(mutex_);
    if(is_standard(*c,*parsed.overlay)) return reset_locked(key);
    auto next=states_.at(key);
    if(next.overlay && to_json(*next.overlay)==to_json(*parsed.overlay)) return {};
    next.overlay=std::move(parsed.overlay); next.state="ok"; ++next.revision;
    const auto bytes=to_json(*next.overlay).dump();
    const auto path=directory_/("builtin."+key+".layout.json"), backup=fs::path(path.wstring()+L".bak");
    if(!ready_directory(directory_,false) || !regular_or_missing(path) || !regular_or_missing(backup)) return "Unsafe arrangement path.";
    std::error_code ec; const bool exists=fs::exists(path,ec); if(ec) return "Arrangement unavailable.";
    if(exists) { std::string old; if(!read_bounded(path,old,plugins::arrangement_limit) || !atomic_write(backup,old)) return "Arrangement backup failed."; }
    if(!atomic_write(path,bytes)) return "Atomic arrangement write failed.";
    states_.at(key)=std::move(next); return {};
}
std::string LayoutStore::reset_locked(const std::string& key) {
    const auto path=directory_/("builtin."+key+".layout.json"), backup=fs::path(path.wstring()+L".bak");
    if(!ready_directory(directory_,false) || !regular_or_missing(path) || !regular_or_missing(backup)) return "Unsafe arrangement path.";
    std::error_code ec; fs::remove(backup,ec); if(ec) return "Arrangement backup removal failed.";
    fs::remove(path,ec); if(ec) return "Arrangement removal failed.";
    auto& state=states_.at(key);
    if(state.overlay || state.state!="none") ++state.revision;
    state.overlay.reset(); state.state="none"; return {};
}
std::string LayoutStore::reset(const std::string& key) {
    std::lock_guard lock(mutex_); if(!find(key)) return "Unknown built-in page."; return reset_locked(key);
}
} // namespace builtin

struct App::RendererSet {
    int width, height;
    render::Renderer discord, friends;
    render::Bf6Renderer bf6;
    render::RssRenderer rss;
    render::StreamRenderer stream;
    render::SysRenderer system;
    render::HaRenderer ha;
    render::SpotifyRenderer spotify;
    RendererSet(int w, int h, const ojson& cfg)
        : width(w), height(h), discord(w, h, cfg["display"].value("title", "Discord-Freunde")),
          friends(w, h, cfg["friends"].value("title", "Freunde")),
          bf6(w, h, cfg["bf6"].value("title", "Battlefield 6 · Rangliste")),
          rss(w, h, cfg["rss"].value("title", "Blog-Updates")),
          stream(w, h, cfg["streaming"].value("title", "Neu bei Streaming")),
          system(w, h, cfg["system"].value("title", "System"), cfg["system"].value("net_unit", "bit")),
          ha(w, h, cfg["homeassistant"].value("title", "Home Assistant")),
          spotify(w, h, cfg["spotify"].value("title", "Spotify")) {}
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
    // Betriebsart gilt von Anfang an, auch für Geräte, die sich vor der ersten Zeichenrunde verbinden
    was_independent_ = independent();
    if (companion_) companion_->set_independent(was_independent_);
    render::apply_theme(dp_);
    const ojson &bf = cfg_["bf6"], &rs = cfg_["rss"], &sy = cfg_["system"], &dc = cfg_["discord"], &fr = cfg_["friends"];
    auto dirty = [this] { mark_dirty(); };
    if (b(bf, "enabled") && bf.contains("players") && bf["players"].is_array() && !bf["players"].empty())
        bf6_ = std::make_unique<bf6::Bf6Stats>(bf);
    if (b(rs, "enabled") && rs.contains("feeds") && rs["feeds"].is_array() && !rs["feeds"].empty())
        rss_ = std::make_unique<rss::RssFeeds>(rs);
    // Streaming-Neustarts: auch ohne Key anlegen, damit die Seite „Kein Key“ zeigt (Anfragen gibt es erst mit Key)
    if (b(cfg_["streaming"], "enabled") && !preview_dir_)
        streaming_ = std::make_unique<tmdb::TmdbFeed>(cfg_["streaming"], cfg_["ui"].value("language", "de"),
                                                      cfg_path_.parent_path() / L"cache" / L"tmdb-logos");
    if (b(sy, "enabled", true)) sys_ = std::make_unique<sysinfo::SysMonitor>(sy);
    // Discord wird gebraucht, wenn seine Seite an ist oder die gemeinsame Freundesliste es einbezieht
    bool want_discord = b(dc, "enabled", true) || (b(fr, "enabled") && b(fr, "include_discord", true));
    if (want_discord && !trim(s(dc, "token")).empty()) discord_ = std::make_unique<discord::DiscordFeed>(dc, dirty);
    if (b(cfg_["homeassistant"], "enabled")) {
        ha_ = std::make_unique<ha::HaClient>(cfg_["homeassistant"], touch_enabled(), dirty);
        if (!ha_->configured()) ha_.reset();
    }
    if (b(cfg_["spotify"], "enabled")) {
        spotify_ = std::make_unique<spotify::SpotifyClient>(cfg_["spotify"], cfg_["ui"].value("port", 8765), touch_enabled(),
                                                            cfg_path_.parent_path() / L"spotify.token", dirty);
        if (!spotify_->configured()) spotify_.reset();
        else if (b(cfg_["spotify"], "amp", true)) meter_ = std::make_unique<audio::LevelMeter>(dirty);
    }
    if (b(fr, "enabled"))
        friends_ = std::make_unique<friends::FriendsHub>(fr, b(fr, "include_discord", true) ? discord_.get() : nullptr, dirty);
    builtin_layouts_=std::make_unique<builtin::LayoutStore>(cfg_path_.parent_path()/L"layouts");
    plugins_ = std::make_unique<plugins::Store>(cfg_path_.parent_path() / L"plugins");
    if (auto result = plugins_->load(); !result) LOGW("plugins", "Plugin-Bibliothek: %s", result.error.c_str());
    plugin_settings_ = std::make_unique<plugins::SettingsStore>(cfg_path_.parent_path() / L"plugin-settings");
    for (const auto& entry : plugins_->entries())
        if (auto result = plugin_settings_->load(entry.manifest); !result)
            LOGW("plugins", "Plugin-Einstellungen: %s", result.error.c_str());
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

ojson App::update_info() {
    if (distribution::packaged()) return {{"channel", "microsoft-store"}, {"current", VERSION}, {"available", false}};
    return update_ ? ojson::parse(update_->info().dump()) : ojson();
}
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

bool App::plugin_import(std::string_view manifest, std::string& error, bool force_disabled) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    auto parsed = plugins::parse_manifest(manifest);
    if (!parsed) { error = parsed.error; return false; }
    // Fingerprint of the installed version (if any) before the update replaces it.
    const auto previous = plugins_->find(parsed.manifest->id);
    auto result = plugins_->import_manifest(manifest, force_disabled);
    if (!result) { error = result.error; return false; }
    if (plugin_settings_) {
        // Stored credentials were entered for the old hosts/URLs/headers. If those change (or no
        // installed version is known), discard them so an update cannot redirect them to new hosts.
        if (!previous || plugins::source_fingerprint(previous->manifest) != plugins::source_fingerprint(*parsed.manifest)) {
            auto discarded = plugin_settings_->discard_secrets(parsed.manifest->id);
            if (!discarded) {
                error = "Plugin importiert, aber gespeicherte Zugangsdaten konnten nicht verworfen werden: " + discarded.error;
                LOGW("plugins", "%s", error.c_str());
                ++plugin_settings_revision_;
                mark_dirty();
                return false;
            }
        }
        auto settings = plugin_settings_->load(*parsed.manifest);
        if (!settings) {
            error = "Plugin importiert, aber Einstellungen konnten nicht sicher geladen werden: " + settings.error;
            LOGW("plugins", "%s", error.c_str());
            mark_dirty();
            return false;
        }
        ++plugin_settings_revision_;
    }
    mark_dirty();
    return true;
}

bool App::plugin_set_enabled(const std::string& id, bool enabled, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    if (enabled) {
        auto entry = plugins_->find(id);
        if (!entry) { error = "Plugin nicht installiert"; return false; }
        if (!entry->manifest.sources.empty() && plugin_settings_) {
            auto values = plugin_settings_->runtime_values(entry->manifest);
            if (!values) { error = "Erforderliche Plugin-Einstellungen fehlen"; return false; }
        }
    }
    auto result = plugins_->set_enabled(id, enabled);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

bool App::plugin_set_host_consent(const std::string& id, const std::string& host, bool allowed, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    auto result = plugins_->set_host_consent(id, host, allowed);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

bool App::plugin_set_link_consent(const std::string& id, bool allowed, std::string& error) {
    if(!plugins_) {error="Plugin store unavailable";return false;}
    auto result=plugins_->set_link_consent(id,allowed);
    if(!result) {error=result.error;return false;}
    mark_dirty();return true;
}
ojson App::plugin_settings_view(const std::string& id) {
    auto entry = plugins_ ? plugins_->find(id) : std::nullopt;
    if (!entry || !plugin_settings_) return ojson::object();
    return ojson::parse(plugin_settings_->public_view(entry->manifest).dump());
}

bool App::plugin_save_settings(const std::string& id, const ojson& patch, std::string& error) {
    auto entry = plugins_ ? plugins_->find(id) : std::nullopt;
    if (!entry || !plugin_settings_) { error = "Plugin nicht verfügbar"; return false; }
    auto result = plugin_settings_->save(entry->manifest, plugins::Json::parse(patch.dump()));
    if (!result) { error = result.error; return false; }
    ++plugin_settings_revision_;
    mark_dirty();
    return true;
}

bool App::plugin_remove(const std::string& id, std::string& error) {
    if (!plugins_) { error = "Plugin-Bibliothek nicht verfügbar"; return false; }
    if (!plugins_->find(id)) { error = "Plugin nicht installiert"; return false; }
    if (plugin_settings_) {
        auto settings = plugin_settings_->remove(id);
        if (!settings) { error = settings.error; return false; }
        ++plugin_settings_revision_;
    }
    auto result = plugins_->remove(id);
    if (!result) { error = result.error; return false; }
    mark_dirty();
    return true;
}

std::optional<Image> App::plugin_preview(std::string_view manifest, std::string& error) {
    return plugin_preview(manifest, error, size());
}
std::optional<Image> App::plugin_preview(std::string_view manifest, std::string& error, std::pair<int,int> dimensions, bool fixed_sample_clock) {
    error.clear();
    if (dimensions.first <= 0 || dimensions.second <= 0 || dimensions.first > 4096 || dimensions.second > 4096) {
        error = "Invalid preview size."; return std::nullopt;
    }
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
    // Nur der Editor wählt reproduzierbare Beispiele; bestehende Vorschauaufrufe bleiben unverändert.
    if(fixed_sample_clock) { context.time="12:34"; context.date="2026-01-01"; }
    auto page = plugins::render_page(*result.manifest, context);
    std::lock_guard draw_lock(render::mutex());
    render::apply_theme(dp_);
    auto [width, height] = dimensions;
    return plugins::draw_page(page, width, height, true);
}

nlohmann::json App::builtin_layout_get(const std::string& key) { return builtin_layouts_->view(key); }
void App::builtin_layout_apply(const std::string& key,const nlohmann::json& overlay) {
    const auto c=builtin::find(key);
    if(!c || !c->enabled) throw std::runtime_error("Built-in page is not enabled for editing.");
    auto error=builtin_layouts_->apply(key,overlay.dump()); if(!error.empty()) throw std::runtime_error(error);
    mark_dirty();
}
void App::builtin_layout_reset(const std::string& key) {
    const auto c=builtin::find(key);
    if(!c || !c->enabled) throw std::runtime_error("Built-in page is not enabled for editing.");
    auto error=builtin_layouts_->reset(key); if(!error.empty()) throw std::runtime_error(error);
    mark_dirty();
}
std::optional<Image> App::page_preview(const std::string& key,const nlohmann::json& overlay,
                                     std::pair<int,int> dimensions,std::string& error) {
    error.clear(); const auto c=builtin::find(key);
    if(!c) { error="Unknown built-in page."; return {}; }
    auto parsed=builtin::parse(*c,overlay.dump()); if(!parsed) {error=parsed.error;return {};}
    if(!c->enabled && !builtin::is_standard(*c,*parsed.overlay)) {error="Built-in arranged renderer is not enabled yet.";return {};}
    const auto [width,height]=dimensions;
    if(width<1 || height<1 || width>4096 || height>4096) {error="Invalid preview size.";return {};}
    std::lock_guard lock(render::mutex()); render::apply_theme(dp_);
    // Dieselben Renderer und fest eingefrorenen Daten wie im Built-in-Baseline-Test.
    RendererSet rs(width,height,cfg_); using namespace render::samples;
    if(key=="rss") return rs.rss.render_arranged(posts(i18n::lang()),CLOCK,*parsed.overlay,0,CLOCK).image;
    if(key=="streaming") {auto logos=stream_logos();return rs.stream.render_arranged(tmdb::State::Ok,stream(),CLOCK,*parsed.overlay,0,CLOCK,&logos).image;}
    if(key=="bf6") return rs.bf6.render_arranged(players(),CLOCK,*parsed.overlay,0,CLOCK).image;
    if(key=="friends") return rs.friends.render_friends_arranged(render::samples::friends(),true,"",*parsed.overlay,0,CLOCK).image;
    if(key=="discord") return rs.discord.render_friends_arranged(render::samples::friends(),true,"",*parsed.overlay,0,CLOCK).image;
    if(key=="system") return rs.system.render_arranged(system(),*parsed.overlay,CLOCK).image;
    if(key=="spotify") {
        // Eingefrorene Beispielpegel statt Zugriff auf Audiohardware in der Editorvorschau.
        const std::vector<float> levels={.05f,.35f,.55f,.8f,.65f,.9f,1.f,.7f,.5f,.75f,.85f,.6f,.45f,.25f,.15f};
        return rs.spotify.render_arranged(render::samples::spotify(),*parsed.overlay,1000,&levels,1,true,CLOCK).image;
    }
    error="Unknown built-in page.";return {};
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
    if (meter_) meter_->stop();
    if (spotify_) spotify_->stop();
    if (friends_) friends_->stop();
    if (discord_) discord_->stop();
    if (bf6_) bf6_->stop();
    if (rss_) rss_->stop();
    if (streaming_) streaming_->stop();
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
    if (companion && independent()) {
        std::lock_guard lk(dev_mu_);
        for (auto& [id, d] : devices_) d->view.step += delta;
    } else {
        (companion && companion_output_ ? companion_view_.step : primary_view_.step) += delta;
    }
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

std::vector<std::string> App::active_pages(bool companion, const std::vector<std::string>* pages_override) {
    // Die reine Discord-Seite entfällt, solange die gemeinsame Freundesliste Discord schon enthält
    // (sonst stünden dieselben Leute auf zwei Seiten).
    const ojson& fr = cfg_["friends"];
    bool discord_page = b(cfg_["discord"], "enabled", true) && !(friends_ && b(fr, "include_discord", true));
    std::map<std::string, bool> have = {{"system", sys_ != nullptr},
                                        {"discord", discord_page && discord_ != nullptr},
                                        {"friends", friends_ != nullptr},
                                        {"bf6", bf6_ != nullptr},
                                        {"rss", rss_ != nullptr},
                                        {"streaming", streaming_ != nullptr},
                                        {"homeassistant", ha_ != nullptr}, {"spotify", spotify_ != nullptr}};
    std::vector<std::string> out;
    bool network = dp_.value("output", "serial") == "network";
    const ojson own = pages_override ? ojson(*pages_override) : ojson();   // eigene Liste eines Geräts ersetzt companion.pages
    const auto& selected = (companion || network) ? (pages_override ? own : cfg_["companion"]["pages"]) : dp_["visible_pages"];
    const auto& order = (companion || network) ? selected : dp_["pages"];
    auto chosen = [&](const std::string& key) {
        return std::find_if(selected.begin(), selected.end(), [&](const ojson& v) { return v.is_string() && v.get<std::string>() == key; }) != selected.end();
    };
    // Validated declarative plugins appear as pages. Live data remains absent until
    // every source has a current host grant and the worker has fetched it.
    std::vector<std::string> plugin_keys;
    if (plugins_) for (const auto& entry : plugins_->entries())
        if (entry.enabled) plugin_keys.push_back("plugin:" + entry.manifest.id);
    std::vector<std::string> listed;
    for (auto& p : order) {
        if (!p.is_string()) continue;
        const std::string key = p.get<std::string>();
        const bool is_plugin = key.rfind("plugin:", 0) == 0;
        if (is_plugin ? std::find(plugin_keys.begin(), plugin_keys.end(), key) == plugin_keys.end() : !have[key]) continue;
        if (is_plugin) listed.push_back(key);
        if (chosen(key)) out.push_back(key);
    }
    // Plugins, die in der Reihenfolge noch nicht vorkommen (frisch aktiviert), erscheinen hinten.
    // Eine eigene Geräteliste ist dagegen verbindlich: was dort fehlt, wird auf dem Gerät nicht gezeigt.
    if (!pages_override) for (const auto& key : plugin_keys)
        if (std::find(listed.begin(), listed.end(), key) == listed.end()) out.push_back(key);
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
    {
        const ojson& sc = cfg_["spotify"];
        bool set = !s(sc, "client_id").empty();
        ojson info = spotify_ ? spotify_->info() : ojson();
        if (info.is_object() && b(info, "ok")) out["spotify"] = {{"state", "ok"}};
        else if (info.is_object() && !s(info, "error").empty()) out["spotify"] = {{"state", "bad"}, {"error", s(info, "error")}};
        else if (info.is_object() && b(info, "logged_in")) out["spotify"] = {{"state", "idle"}};
        else out["spotify"] = {{"state", set ? "idle" : "off"}};
    }
    for (auto plat : {"steam", "xbox", "psn"}) {
        bool key_set = std::string(plat) == "steam" ? !s(fr, "steam_api_key").empty() && !s(fr, "steam_id").empty()
                       : std::string(plat) == "psn" ? !s(fr, "psn_npsso").empty()
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

std::optional<Image> App::device_frame(const std::string& device_id) {
    if (auto* nd = companion())
        if (auto f = nd->frame_for(device_id)) return f;
    return last_frame(true);
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

void App::system_theme_changed() {
    const std::string key = s(dp_, "theme", "hell");
    if (key != "windows" && key.rfind("custom:", 0) != 0) return;
    {
        std::lock_guard rl(render::mutex());
        render::apply_theme(dp_);
    }
    mark_dirty();
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
    bool has_source = (discord_ && inc) || !s(fr, "steam_api_key").empty() || !s(fr, "xbox_api_key").empty() || !s(fr, "psn_npsso").empty();
    if (!has_source) return "Noch keine Konten verbunden – bitte in den Einstellungen einrichten";
    return join(problems, " · ");
}

std::vector<App::PageEntry> App::page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r, bool companion,
                                           const std::vector<std::string>* pages_override, ViewState* view, double tick, bool phone) {
    // [(Name, Seitenanzahl, Zeichenfunktion(seite, zeichner))] in der eingestellten Reihenfolge
    std::vector<PageEntry> out;
    for (auto& key : active_pages(companion, pages_override)) {
        if(view && builtin::find(key)) {
            const auto revision=builtin_layouts_->get(key).revision;
            if(view->builtin_revisions[key]!=revision) {
                view->cycles.erase("builtin:"+key); view->fade_caches.erase("builtin:"+key);
                view->builtin_revisions[key]=revision;
            }
        }
        if (key.rfind("plugin:", 0) == 0) {
            auto installed = plugins_ ? plugins_->find(key.substr(7)) : std::nullopt;
            if (!installed || !installed->enabled) continue;
            auto manifest = installed->effective();
            plugins::RenderContext context;
            if (plugin_settings_) {
                auto public_settings = plugin_settings_->public_view(manifest);
                if (public_settings.contains("values") && public_settings["values"].is_object())
                    context.settings = public_settings["values"];
            }
            if (!manifest.sources.empty()) {
                std::lock_guard lk(plugin_data_mu_);
                auto snapshot = plugin_data_.find(manifest.id);
                if (snapshot != plugin_data_.end() && snapshot->second.generation == installed->generation &&
                    snapshot->second.settings_revision == plugin_settings_revision_.load()) {
                    for (const auto& source : manifest.sources)
                        if (plugins_->source_authorized(manifest.id, source.id, installed->generation)) {
                            auto value = snapshot->second.data.find(source.id);
                            if (value != snapshot->second.data.end()) context.sources.emplace(source.id, value->second);
                        }
                }
            }
            context.time = clock_hhmm();
            std::time_t now = std::time(nullptr);
            std::tm local{};
            localtime_s(&local, &now);
            char date[32] = {};
            std::strftime(date, sizeof date, "%Y-%m-%d", &local);
            context.date = date;
            const auto page = plugins::render_page(manifest, context);
            PageEntry entry{key, 1, [page](int, RendererSet& rs) { return plugins::draw_page(page,rs.width,rs.height); }};
            entry.plugin_draw=[page,generation=installed->generation](RendererSet& rs) {
                auto out=plugins::draw_page_out(page,rs.width,rs.height);
                for(auto& target:out.targets) target.generation=generation;
                return out;
            };
            if (view && manifest.layout && std::any_of(manifest.layout->zones.begin(),manifest.layout->zones.end(),[](const auto& z){return z.cycle.has_value();})) {
                entry.prepare = [this,view,tick,phone,page,manifest,generation=installed->generation,layout_revision=installed->layout_revision,tile_width=r.width,tile_height=r.height](PageEntry& entry) {
                    using namespace plugins::schedule;
                    auto& state=view->cycles[manifest.id];
                    if (view->cycle_generations[manifest.id]!=std::pair{generation,layout_revision}) { plugins::schedule::reset(state,true,false); view->fade_caches.erase(manifest.id); }
                    view->cycle_generations[manifest.id]={generation,layout_revision};
                    std::vector<ZoneInput> inputs;
                    for (const auto& z:manifest.layout->zones) if (z.cycle) {
                        ZoneInput input{z.id,z.cycle->mode=="rotate" ? Mode::rotate : Mode::first_available,z.cycle->transition=="fade",{}};
                        for (const auto& w:manifest.widgets) if (w.zone==z.id) {
                            auto found=std::find_if(page.widgets.begin(),page.widgets.end(),[&](const auto& r){return r.id==w.id;});
                            input.entries.push_back({w.id,w.seconds,found!=page.widgets.end() && !found->missing});
                        }
                        inputs.push_back(std::move(input));
                    }
                    auto kind=phone ? OutputKind::companion : !out_ ? OutputKind::none : out_->is_monitor() ? OutputKind::monitor :
                        out_.get()==companion_ ? OutputKind::companion : OutputKind::serial;
                    auto transfer=phone || !out_ ? TransferStats{} : out_->transfer();
                    const float scale=render::scale_for(tile_width,tile_height);
                    const float spacing=theme::design(render::theme().style).spacing/12.f;
                    const float margin=10*spacing;
                    auto geometry=plugins::zones::plan({margin,39,tile_width/scale-2*margin,tile_height/scale-58-margin},*manifest.layout,{},7*spacing);
                    int zone_rows=0;
                    for(const auto& zone:geometry.zones)
                        if(std::any_of(inputs.begin(),inputs.end(),[&](const auto& input){return input.zone==zone.id;}))
                            zone_rows=std::max(zone_rows,int(std::ceil(zone.area.height*scale)));
                    auto budget=budget_for(kind,transfer.bytes_per_second,transfer.last_seconds,view->render_ms.value_or(0),zone_rows,view->size.first,
                        companion_ ? companion_->client_ids().size() : 0);
                    std::vector<std::string> requests;
                    const auto prefix=manifest.id+"/";
                    for(const auto& request:view->frame_next_requests)
                        if(request.starts_with(prefix)) requests.push_back(request.substr(prefix.size()));
                    auto result=advance(inputs,state,tick,budget,requests);
                    if (std::any_of(state.begin(),state.end(),[&](const auto& pair){return pair.second.fade && pair.second.fade->start==tick;}))
                        view->fade_caches.erase(manifest.id);
                    view->next_deadline=std::min(view->next_deadline,result.next_deadline);
                    view->animating=view->animating || result.animating;
                    auto& cache=view->fade_caches[manifest.id]; if (!cache) cache=std::make_shared<plugins::FadeCache>();
                    std::map<std::string,std::shared_ptr<plugins::FadeCache>> caches{{manifest.id,cache}};
                    entry.plugin_draw=[page,choices=result.zones,caches,generation](RendererSet& rs) mutable {
                        auto out=plugins::draw_choices_out(page,rs.width,rs.height,choices,caches);
                        for(auto& target:out.targets) target.generation=generation;
                        return out;
                    };
                    entry.fn=[page,choices=result.zones,caches](int,RendererSet& rs) mutable { return plugins::draw_choices(page,rs.width,rs.height,choices,caches); };
                };
            }
            out.push_back(std::move(entry));
        } else if (key == "system") {
            PageEntry pe{key, 1, [this](int, RendererSet& rs) { return rs.system.render(sys_->snapshot()); }};
            // Tippen auf RAM / Datenträger / Netzwerk öffnet das passende Windows-Werkzeug bzw. den Geschwindigkeitstest
            for (int zone : {render::SysRenderer::ZONE_RAM, render::SysRenderer::ZONE_DISK, render::SysRenderer::ZONE_NET})
                pe.actions.push_back([zone] {
                    if (touch::open_system(zone)) LOGI("app", "Touch: Systemwerkzeug %d geöffnet", zone);
                    else LOGW("app", "Touch: Systemwerkzeug %d ließ sich nicht öffnen", zone);
                });
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(arrangement && r.system.uses_arrangement(*arrangement)) {
                const auto snap=sys_->snapshot();const auto actions=pe.actions;
                pe.fn=[snap,arrangement](int,RendererSet& rs){return rs.system.render_arranged(snap,*arrangement).image;};
                pe.builtin_draw=[snap,arrangement,actions](int,RendererSet& rs) {
                    auto drawn=rs.system.render_arranged(snap,*arrangement);
                    std::vector<touch::ArrangedTarget> targets;
                    for(const auto& target:drawn.targets) targets.push_back({target.rect_px,{},actions.at(target.item)});
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
            }
            out.push_back(std::move(pe));
        } else if (key == "discord") {
            std::string st = discord_->status();
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(!arrangement || !r.discord.uses_friends_arrangement(*arrangement)) {
            out.push_back({key, r.discord.pages(friends.size()), [friends, st](int i, RendererSet& rs) {
                               return rs.discord.render_friends(friends, i, st.empty(), st);
                           }, [friends, st](int i, RendererSet& rs) {
                               return st.empty() ? touch::friend_rows(friends, i, rs.discord.rows_per_page()) : touch::Rows{};
                           }});
            } else {
                const auto sorted=render::sort_friends(friends);
                const bool connected=st.empty();
                PageEntry entry{key,connected?r.discord.friend_pages(sorted.size(),*arrangement):1,{}};
                entry.builtin_draw=[sorted,connected,st,arrangement](int page,RendererSet& rs) {
                    auto drawn=rs.discord.render_friends_arranged(sorted,connected,st,*arrangement,page);
                    std::vector<touch::ArrangedTarget> targets;
                    // Profilziele stammen ausschließlich vom gezeichneten sortierten Schnappschuss.
                    for(const auto& target:drawn.targets)
                        targets.push_back({target.rect_px,touch::friend_targets(sorted.at(target.item)),{}});
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
                entry.fn=[sorted,connected,st,arrangement](int page,RendererSet& rs){return rs.discord.render_friends_arranged(sorted,connected,st,*arrangement,page).image;};
                out.push_back(std::move(entry));
            }
        } else if (key == "friends") {
            std::string msg = friends_message();
            bool bad = !msg.empty() && people.empty();
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(!arrangement || !r.friends.uses_friends_arrangement(*arrangement)) {
            out.push_back({key, r.friends.pages(people.size()), [people, msg, bad](int i, RendererSet& rs) {
                               return rs.friends.render_friends(people, i, !bad, msg);
                           }, [people, bad](int i, RendererSet& rs) {
                               return bad ? touch::Rows{} : touch::friend_rows(people, i, rs.friends.rows_per_page());
                           }});
            } else {
                const auto sorted=render::sort_friends(people);
                const bool connected=!bad;
                PageEntry entry{key,connected?r.friends.friend_pages(sorted.size(),*arrangement):1,{}};
                entry.builtin_draw=[sorted,connected,msg,arrangement](int page,RendererSet& rs) {
                    auto drawn=rs.friends.render_friends_arranged(sorted,connected,msg,*arrangement,page);
                    std::vector<touch::ArrangedTarget> targets;
                    // Profilziele stammen ausschließlich vom gezeichneten sortierten Schnappschuss.
                    for(const auto& target:drawn.targets)
                        targets.push_back({target.rect_px,touch::friend_targets(sorted.at(target.item)),{}});
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
                entry.fn=[sorted,connected,msg,arrangement](int page,RendererSet& rs){return rs.friends.render_friends_arranged(sorted,connected,msg,*arrangement,page).image;};
                out.push_back(std::move(entry));
            }
        } else if (key == "bf6") {
            auto players = bf6_->players();
            auto updated = bf6_->updated();
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(!arrangement || !r.bf6.uses_arrangement(*arrangement)) {
            out.push_back({key, r.bf6.pages(players.size()), [players, updated](int i, RendererSet& rs) {
                               return rs.bf6.render(players, i, updated);
                           }, [players, updated](int i, RendererSet& rs) {
                               return updated ? touch::player_rows(players, i, rs.bf6.rows_per_page()) : touch::Rows{};
                           }});
            } else {
                // Derselbe sortierte Schnappschuss für Rangfolge, Zeichnen und Spielerlinks.
                players=render::sort_players(std::move(players));
                PageEntry entry{key,updated?r.bf6.pages(players.size(),*arrangement):1,{}};
                entry.builtin_draw=[players,updated,arrangement](int page,RendererSet& rs) {
                    auto drawn=rs.bf6.render_arranged(players,updated,*arrangement,page);
                    std::vector<touch::ArrangedTarget> targets;
                    for(const auto& target:drawn.targets)
                        targets.push_back({target.rect_px,touch::player_targets(players.at(target.item)),{}});
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
                entry.fn=[players,updated,arrangement](int page,RendererSet& rs){return rs.bf6.render_arranged(players,updated,*arrangement,page).image;};
                out.push_back(std::move(entry));
            }
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
        } else if (key == "spotify") {
            spotify::Snapshot snap = spotify_->snapshot();
            const std::string amp_color = cfg_["spotify"].value("amp_color", std::string("level"));
            PageEntry pe{key, 1, [this, snap, amp_color](int, RendererSet& rs) {
                std::vector<float> lv;
                bool sim = false;
                // Erst beim Zeichnen einer sichtbaren Kachel aktivieren, nicht beim Auflisten aller Seiten.
                if (meter_ && snap.playing) {
                    meter_->touch(audio::amp_seed(snap.title, snap.artist));
                    lv = meter_->levels();
                    sim = meter_->simulated();   // Wiedergabe läuft woanders (Handy/Lautsprecher/Connect): Animation statt Messung
                }
                const auto mode = audio::parse_amp_color(amp_color);
                return rs.spotify.render(snap, 0, lv.empty() ? nullptr : &lv, mode == audio::AmpColor::Theme ? 0 : mode == audio::AmpColor::Level ? 1 : 2, sim);
            }};
            if (snap.controls && (snap.state == spotify::State::Playing || snap.state == spotify::State::Paused)) {
                using Cmd = spotify::SpotifyClient::Command;
                for (Cmd c : {Cmd::Previous, Cmd::PlayPause, Cmd::Next})
                    pe.actions.push_back([this, c] { if (spotify_) spotify_->request_command(c); });
            }
            pe.actions.resize(4);
            if (snap.cover && !snap.cover->empty() &&
                (snap.state == spotify::State::Playing || snap.state == spotify::State::Paused)) {
                pe.actions[render::SpotifyRenderer::COVER] = [] {
                    if (!touch::open({{"spotify:", "spotify"}}))
                        LOGW("app", "Spotify-App konnte nicht geöffnet werden");
                };
            }
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(arrangement && r.spotify.uses_arrangement(*arrangement)) {
                auto draw=[this,snap,amp_color,arrangement](RendererSet& rs) {
                    std::vector<float> levels;bool simulated=false;
                    if(meter_ && snap.playing) {meter_->touch(audio::amp_seed(snap.title,snap.artist));levels=meter_->levels();simulated=meter_->simulated();}
                    const auto mode=audio::parse_amp_color(amp_color);
                    return rs.spotify.render_arranged(snap,*arrangement,0,levels.empty()?nullptr:&levels,
                        mode==audio::AmpColor::Theme?0:mode==audio::AmpColor::Level?1:2,simulated);
                };
                const auto actions=pe.actions;
                pe.fn=[draw](int,RendererSet& rs){return draw(rs).image;};
                pe.builtin_draw=[draw,actions](int,RendererSet& rs) {
                    auto drawn=draw(rs);std::vector<touch::ArrangedTarget> targets;
                    for(const auto& target:drawn.targets) if(actions.at(target.item)) targets.push_back({target.rect_px,{},actions.at(target.item)});
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
            }
            out.push_back(std::move(pe));
        } else if (key == "streaming") {
            auto snap = streaming_->snapshot();
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(!arrangement || !r.stream.uses_arrangement(*arrangement)) {
            out.push_back({key, 1, [snap](int, RendererSet& rs) { return rs.stream.render(snap.state, snap.items, snap.updated, 0, &snap.logos); },
                [snap](int, RendererSet&) { return snap.state == tmdb::State::Ok ? touch::stream_rows(snap.items) : touch::Rows{}; }});
            } else {
                PageEntry entry{key,snap.state==tmdb::State::Ok?r.stream.pages(snap.items.size(),*arrangement):1,{}};
                entry.builtin_draw=[snap,arrangement](int page,RendererSet& rs) {
                    auto drawn=rs.stream.render_arranged(snap.state,snap.items,snap.updated,*arrangement,page,0,&snap.logos);
                    std::vector<touch::ArrangedTarget> targets;
                    // Ziele bleiben an den gezeichneten Titel-Schnappschuss gebunden (Film/Serie).
                    for(const auto& target:drawn.targets) {
                        auto links=touch::stream_rows({snap.items.at(target.item)});
                        targets.push_back({target.rect_px,links.empty()?touch::Targets{}:std::move(links.front()),{}});
                    }
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
                entry.fn=[snap,arrangement](int page,RendererSet& rs){return rs.stream.render_arranged(snap.state,snap.items,snap.updated,*arrangement,page,0,&snap.logos).image;};
                out.push_back(std::move(entry));
            }
        } else if (key == "rss") {
            auto posts = rss_->posts();
            auto updated = rss_->updated();
            auto arrangement=builtin_layouts_->get(key).overlay;
            if(!arrangement || !r.rss.uses_arrangement(*arrangement)) {
                out.push_back({key, 1, [posts, updated](int, RendererSet& rs) { return rs.rss.render(posts, updated); },
                               [posts, updated](int, RendererSet&) { return updated ? touch::post_rows(posts) : touch::Rows{}; }});
            } else {
                PageEntry entry{key,updated?r.rss.pages(posts.size(),*arrangement):1,{}};
                entry.builtin_draw=[posts,updated,arrangement](int page,RendererSet& rs) {
                    auto drawn=rs.rss.render_arranged(posts,updated,*arrangement,page);
                    std::vector<touch::ArrangedTarget> targets;
                    // Alle Links werden an genau den gezeichneten Feed-Schnappschuss gebunden.
                    for(const auto& target:drawn.targets) {
                        auto links=touch::post_rows({posts.at(target.item)});
                        targets.push_back({target.rect_px,links.empty()?touch::Targets{}:std::move(links.front()),{}});
                    }
                    return std::pair{std::move(drawn.image),std::move(targets)};
                };
                entry.fn=[posts,updated,arrangement](int page,RendererSet& rs){return rs.rss.render_arranged(posts,updated,*arrangement,page).image;};
                out.push_back(std::move(entry));
            }
        }
    }
    return out;
}

App::DrawResult App::draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes, int start, std::pair<int, int> size, device::Area area) {
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
                plugins::DrawOut plugin_out;
                std::vector<touch::ArrangedTarget> arranged_targets;
                auto& renderer=renderers_for(boxes[k].w,boxes[k].h);
                if(p.builtin_draw) { auto out=p.builtin_draw(idx,renderer); frames.push_back(std::move(out.first)); arranged_targets=std::move(out.second); }
                else if(p.plugin_draw) { plugin_out=p.plugin_draw(renderer);frames.push_back(std::move(plugin_out.image)); }
                else frames.push_back(p.fn(idx,renderer));
                keys.push_back(p.key);
                touch::Tile tile{boxes[k], p.key == "rss", p.targets ? p.targets(idx, renderers_for(boxes[k].w, boxes[k].h)) : touch::Rows{}};
                tile.ha = p.key == "homeassistant";
                tile.sys = p.key == "system";
                tile.spotify = p.key == "spotify";
                tile.streaming = p.key == "streaming";
                tile.cells = p.actions;
                tile.arranged=bool(p.builtin_draw); tile.arranged_targets=std::move(arranged_targets);
                tile.plugin=std::move(plugin_out.targets);
                taps.push_back(std::move(tile));
                break;
            }
            idx -= p.n;
        }
    }
    auto image = render::compose(area.w, area.h, boxes, frames);
    device::shift_tiles(taps, area);
    return {device::compose_view(image, size, area, render::theme().bg), keys, taps};
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
        // A slow serial transfer still displays this playback page. Keep the
        // meter alive until it completes, including exception cleanup.
        struct PlaybackTransfer {
            audio::LevelMeter* meter;
            explicit PlaybackTransfer(audio::LevelMeter* m) : meter(m) { if (meter) meter->visible_transfer(true); }
            ~PlaybackTransfer() { if (meter) { meter->touch(); meter->visible_transfer(false); } }
        } playback_transfer(meter_ && spotify_ && spotify_->snapshot().playing &&
                            std::count(primary_view_.current.begin(), primary_view_.current.end(), "spotify")
                            ? meter_.get() : nullptr);
        int rows = out_->show(frame);
        auto transfer = out_->transfer();
        LOGD("app", "%d Zeilen, %.0f ms, %.0f kB/s", rows, transfer.last_seconds*1000, transfer.bytes_per_second.value_or(0)/1000);
    } catch (const std::exception& e) {
        std::lock_guard lk(mu_);
        display_ok_ = false;
        display_error_ = "Verbindung zum Display verloren – wird erneut versucht";
        LOGW("app", "Display: %s", e.what());
    }
}

device::Profile App::profile_for(bool phone, const std::string& id) {
    device::Profile p;
    auto& settings = phone ? cfg_["companion"] : dp_;
    p.keepout_physical = device::parse_insets(settings.value("keepout", ojson()));
    if (phone) p.keepout_physical = device::select_keepout(p.keepout_physical,
        id.empty() ? std::nullopt : companion_->device_keepout(id), cfg_["companion"].value("sync", true));
    if (!phone) {
        auto o = s(dp_, "orientation", "landscape");
        p.orientation = o == "portrait" ? Orientation::Portrait : o == "reverse_portrait" ? Orientation::ReversePortrait :
                        o == "reverse_landscape" ? Orientation::ReverseLandscape : Orientation::Landscape;
        p.physical_rotates = s(dp_, "output", "serial") == "serial";
    }
    return p;
}

bool App::independent() const { return companion_ && !b(cfg_["companion"], "sync", true); }

App::RenderOut App::render_core(ViewState& view, std::pair<int, int> size, double page_seconds, bool phone,
                                const std::vector<std::string>* pages_override, bool dirty, bool register_redraw, const std::string& id) {
    RenderOut out;
    const double now = mono();
    auto profile = profile_for(phone, id);
    auto area = device::usable(size, profile);
    auto inset = profile.physical_rotates ? device::to_frame(profile.keepout_physical, profile.orientation) : profile.keepout_physical;
    if (area == device::Area{0,0,size.first,size.second} &&
        (inset.top > 0 || inset.right > 0 || inset.bottom > 0 || inset.left > 0) && mono()-view.keepout_warning >= 60) {
        LOGW("app", "Sperrzone ignoriert: nutzbare Fläche zu klein");
        view.keepout_warning = mono();
    }
    out.area = area;
    bool due = now - view.switched >= page_seconds;
    auto minute = clock_hhmm();
    bool live = std::count(view.current.begin(), view.current.end(), "system") > 0;
    // Keep visible playback alive even when silent capture publishes no changes.
    // Otherwise the 3 s activity lease expires before the 4 s simulation delay.
    live = live || (meter_ && spotify_ && spotify_->snapshot().playing &&
                    std::count(view.current.begin(), view.current.end(), "spotify") > 0);
    if (!plugins::schedule::should_render(dirty,due,live,view.step,view.size!=size || view.area!=area,view.minute!=minute,now,view.next_deadline)) return out;
    if (view.size!=size || view.area!=area) { for (auto& [plugin,zones]:view.cycles) plugins::schedule::reset(zones,false,true); view.fade_caches.clear(); }
    { std::lock_guard lk(view.next_mu); view.frame_next_requests.clear(); view.frame_next_requests.swap(view.next_requests); }
    view.next_deadline=plugins::schedule::never; view.animating=false;
    out.skipped = false;
    view.minute = minute;
    view.size = size;
    view.area = area;
    auto active = active_pages(phone, pages_override);
    std::vector<Friend> friends, people;
    if (discord_ && std::count(active.begin(), active.end(), "discord")) friends = discord_->friends();
    if (friends_) people = friends_->people();
    out.navigation = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    {
        std::lock_guard rl(render::mutex());
        // Bound cached renderer sets when a resizable window reports many intermediate sizes.
        if (sets_.size() > 16) sets_.clear();
        auto boxes = render::tile_layout(area.w, area.h);
        auto pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h), phone, pages_override, &view, now, phone);
        int total = 0;
        for (auto& p : pages) total += p.n;
        if (0 < total && total < (int)boxes.size()) {
            boxes = render::tile_layout(area.w, area.h, total);
            pages = page_list(friends, people, renderers_for(boxes[0].w, boxes[0].h), phone, pages_override, &view, now, phone);
            total = 0; for (auto& p : pages) total += p.n;
        }
        int step = view.step.exchange(0);
        if (!total) {
            out.frame = device::compose_view(welcome({area.w, area.h}), size, area, render::theme().bg); view.current.clear(); view.cycles.clear(); view.fade_caches.clear();
            if (register_redraw) { std::lock_guard lk(mu_); redraw_ = [this, size, area] { return device::compose_view(welcome({area.w, area.h}), size, area, render::theme().bg); }; }
        } else {
            int cols = std::min((int)boxes.size(), total);
            if (due || step) {
                if (total > cols) {
                    out.switched = true;
                    view.page = (((view.page + (step ? step : 1) * cols) % total) + total) % total;
                }
                view.switched = mono();
            }
            view.page = total > cols ? view.page % total : 0;
            int start = view.page;
            std::set<std::string> visible_plugins;
            int offset=0;
            for (auto& entry:pages) {
                bool visible=false;
                for (int i=0;i<cols;++i) { int index=(start+i)%total; if (index>=offset && index<offset+entry.n) visible=true; }
                if (visible && entry.prepare) { entry.prepare(entry); visible_plugins.insert(entry.key.substr(7)); }
                offset+=entry.n;
            }
            std::erase_if(view.cycles,[&](const auto& pair){return !visible_plugins.contains(pair.first);});
            std::erase_if(view.fade_caches,[&](const auto& pair){return !visible_plugins.contains(pair.first);});
            // Wechselfeld-Frames setzen nie out.switched; nur die bestehende Seitennavigation tut das.

            if (register_redraw) {
                std::lock_guard lk(mu_);
                redraw_ = [this, pages, boxes, start, size, area] { return draw(pages, boxes, start, size, area).frame; };
            }
            auto [f, keys, targets] = draw(pages, boxes, start, size, area);
            out.frame = std::move(f); view.current = keys; out.taps = std::move(targets);
            out.navigation = {{"index", start}, {"total", total}, {"visible", cols}, {"current", keys}, {"can_step", total > cols}};
        }
    }
    if (out.switched) view.switched = mono();
    view.render_ms = ewma(view.render_ms,(mono()-now)*1000);
    LOGD("app", "Renderzeit %.1f ms",view.render_ms.value_or(0));
    return out;
}

void App::render_view(bool secondary, bool dirty) {
    auto& view = secondary ? companion_view_ : primary_view_;
    bool phone = companion_ && (secondary || out_.get() == companion_);
    auto size = phone ? companion_->size() : size_;
    auto& settings = phone ? cfg_["companion"] : dp_;
    RenderOut r = render_core(view, size, settings.value("page_seconds", 15), phone, nullptr, dirty, !secondary);
    if (r.skipped) return;
    std::lock_guard lk(tap_mu_);
    if (secondary) {
        // Snapshot and its touch targets are published together.
        companion_output_->show(r.frame);
        touch::stabilize_plugins(r.taps,companion_taps_,mono());
        companion_taps_ = std::move(r.taps); companion_tap_area_ = r.area;
        std::lock_guard state(mu_);
        companion_frame_ = r.frame; companion_navigation_ = r.navigation;
    } else {
        output(r.frame, r.switched, r.navigation); touch::stabilize_plugins(r.taps,tap_tiles_,mono()); tap_tiles_ = std::move(r.taps); tap_area_ = r.area;
    }
}

// Unabhängiger Companion-Betrieb: jedes verbundene Gerät hat Ansicht, Größe und (optional) Seitenliste für sich.
void App::render_devices(bool dirty) {
    const auto ids = companion_->client_ids();
    const double now = mono();
    const double seconds = cfg_["companion"].value("page_seconds", 15);
    bool first = true;
    for (const auto& id : ids) {
        DeviceState* d = nullptr;
        {
            std::lock_guard lk(dev_mu_);
            auto& slot = devices_[id];
            if (!slot) { slot = std::make_unique<DeviceState>(); slot->view.switched = now; }
            slot->last_seen = now;
            d = slot.get();   // bleibt gültig: nur dieser Thread löscht Einträge
        }
        auto own = companion_->device_pages(id);
        std::pair<int, int> size;
        { std::lock_guard lk(dev_mu_); size = d->size.first > 0 ? d->size : companion_->size(); }
        RenderOut r = render_core(d->view, size, seconds, true, own ? &*own : nullptr, dirty, false, id);
        if (r.skipped) { first = false; continue; }
        {
            std::lock_guard lk(dev_mu_);
            companion_->show_for(id, r.frame, r.navigation);
            touch::stabilize_plugins(r.taps,d->taps,mono());
            d->frame = r.frame; d->area = r.area; d->taps = std::move(r.taps); d->navigation = r.navigation;
        }
        if (first) {   // Vorschau und Seitennavigation der Einstellungen folgen dem ersten Gerät
            std::lock_guard state(mu_);
            if (companion_output_) { companion_frame_ = r.frame; companion_navigation_ = r.navigation; }
            else { last_frame_ = r.frame; navigation_ = r.navigation; }
        }
        first = false;
    }
    std::vector<std::string> gone;
    {
        std::lock_guard lk(dev_mu_);
        for (auto& [id, d] : devices_) {
            if (std::find(ids.begin(),ids.end(),id)==ids.end()) {
                d->view.cycles.clear();d->view.fade_caches.clear();
                d->view.next_deadline=plugins::schedule::never;d->view.animating=false;
            }
            if (now - d->last_seen > 60) gone.push_back(id);
        }
        for (const auto& id : gone) devices_.erase(id);
    }
    for (const auto& id : gone) companion_->forget_device(id);
}

touch::PluginAccess App::plugin_access(const std::string& id) {
    auto installed=plugins_ ? plugins_->find(id) : std::nullopt;
    if(!installed) return {};
    return {installed->enabled,installed->links_allowed,installed->generation,installed->manifest.links};
}
void App::queue_plugin_next(ViewState& view,const touch::PluginDecision& decision) {
    std::lock_guard lk(view.next_mu);
    auto key=decision.plugin+"/"+decision.zone;
    if(view.next_requests.size()<8 && std::find(view.next_requests.begin(),view.next_requests.end(),key)==view.next_requests.end())
        view.next_requests.push_back(std::move(key));
}
void App::execute_plugin(const touch::PluginDecision& decision) {
    if(decision.kind==touch::PluginDecision::next) {mark_dirty();return;}
    if(decision.kind!=touch::PluginDecision::open_link) {LOGD("plugins","Plugin-Link verworfen (Prüfung)");return;}
    std::lock_guard lk(plugin_tap_mu_);
    bool opened=touch::plugin_dispatch(decision,mono(),plugin_last_open_,[this,&decision](const std::string& url) {
        auto grant=plugin_access(decision.plugin);
        plugins::Manifest manifest;manifest.links=grant.hosts;
        if(!grant.enabled || !grant.consent || grant.generation!=decision.generation || !plugins::tap_url_valid(url) || !plugins::link_allowed(manifest,url)) return false;
        return touch::open({{url,""}});
    });
    if(opened) LOGI("plugins","Plugin-Link geöffnet (Plugin, Block)");
    else LOGD("plugins","Plugin-Link verworfen (Rate oder Start)");
}
void App::device_tap(const std::string& id, int x, int y, int w, int h) {
    touch::PluginDecision plugin;
    touch::Targets targets;
    std::function<void()> action;
    bool have_target = false;
    {
        std::lock_guard lk(dev_mu_);
        auto it = devices_.find(id);
        if (it == devices_.end()) return;
        DeviceState& d = *it->second;
        if (d.frame.empty() || d.frame.w != w || d.frame.h != h) return;   // veraltete Koordinaten verwerfen
        auto tap = device::resolve_tap(d.area, x, y);
        if (tap.blocked) return;
        plugin=touch::plugin_decide(d.taps,x,y,mono(),1.0,[this](const auto& id){return plugin_access(id);});
        if(plugin.kind==touch::PluginDecision::next) queue_plugin_next(d.view,plugin);
        if(plugin.kind==touch::PluginDecision::none) {
            if (auto* a = touch::action_at(d.taps, x, y)) action = *a;
            else if (auto selected = touch::at(d.taps, x, y)) { targets = *selected; have_target = true; }
            else { d.view.step += tap.step; }   // wie sonst: links = zurück, sonst weiter
        }
    }
    if(plugin.kind!=touch::PluginDecision::none) {execute_plugin(plugin);return;}
    if (action) { action(); return; }
    if (have_target) { if (!touch::open(targets)) LOGW("app", "Touch-Ziel nicht geöffnet: keine passende App oder Start fehlgeschlagen"); return; }
    mark_dirty();
}
void App::display_loop() {
    double next_try = 0, companion_try = 0;
    if (out_) connect_display();
    double wait_deadline = plugins::schedule::never;
    while (!stop_) {
        bool dirty;
        {
            std::unique_lock lk(mu_);
            if (wait_deadline == plugins::schedule::never)
                cv_.wait_for(lk, std::chrono::seconds(1), [&] { return dirty_ || stop_; });
            else cv_.wait_for(lk, std::chrono::duration<double>(std::max(0.0,std::min(1.0,wait_deadline-mono()))), [&] { return dirty_ || stop_; });
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
        const bool indep = independent();
        if (companion_) companion_->set_independent(indep);
        if (indep != was_independent_) {   // Betriebsart gewechselt: alles neu zeichnen
            was_independent_ = indep;
            dirty = true;
            primary_view_.size = companion_view_.size = {0, 0};
            std::vector<std::string> ids;
            { std::lock_guard lk(dev_mu_); for (auto& [id, d] : devices_) ids.push_back(id); devices_.clear(); }
            for (const auto& id : ids) companion_->forget_device(id);
        }
        if (indep) {
            if (out_ && out_.get() != companion_) render_view(false, dirty);   // USB-Display/Bildschirm bleibt wie gehabt
            render_devices(dirty);
        } else {
            render_view(false, dirty);
            if (companion_output_) render_view(true, dirty);
        }
        std::vector<double> deadlines;
        bool animating=false;
        if (!indep || (out_ && out_.get()!=companion_)) { deadlines.push_back(primary_view_.next_deadline); animating=primary_view_.animating; }
        if (!indep && companion_output_) { deadlines.push_back(companion_view_.next_deadline); animating=animating || companion_view_.animating; }
        { std::lock_guard lk(dev_mu_); for (const auto& [id,d]:devices_) if (indep) { deadlines.push_back(d->view.next_deadline); animating=animating || d->view.animating; } }
        bool metering=meter_ && meter_->active() && (meter_->available() || meter_->simulated());
        auto wake=plugins::schedule::wake_at(mono(),deadlines,animating,metering);
        for (int i=0;i<int(std::lround(wake.min_gap/.05)) && !stop_;++i) Sleep(50);
        wait_deadline=std::all_of(deadlines.begin(),deadlines.end(),[](double d){return d==plugins::schedule::never;})
            ? plugins::schedule::never : plugins::schedule::wake_at(mono(),deadlines,animating,metering).wait_until;
    }
}

void App::show_settings() { if (show_settings_callback_) show_settings_callback_(); }

bool App::open_item_at(int x, int y) {
    if (!b(dp_, "touch", true)) return false;
    return tap_at(x, y);
}

void App::companion_tap(int x, int y, int w, int h) {
    tap_at(x, y, companion_output_ != nullptr, w, h);
}

bool App::tap_at(int x, int y, bool secondary, int w, int h) {
    touch::PluginDecision plugin;
    touch::Targets targets;
    std::function<void()> action;
    int step = 0;
    {
        std::lock_guard lk(tap_mu_);
        if (w > 0) {
            auto image = last_frame(secondary);
            if (!image || image->w != w || image->h != h) return true;
        }
        const auto tap = device::resolve_tap(secondary ? companion_tap_area_ : tap_area_, x, y);
        if (tap.blocked) return true;
        const auto& tiles=secondary ? companion_taps_ : tap_tiles_;
        plugin=touch::plugin_decide(tiles,x,y,mono(),w>0 ? 1.0 : .3,[this](const auto& id){return plugin_access(id);});
        if(plugin.kind==touch::PluginDecision::next) queue_plugin_next(secondary ? companion_view_ : primary_view_,plugin);
        if(plugin.kind==touch::PluginDecision::none)
            if (auto* a = touch::action_at(tiles,x,y)) action = *a;
        if (!action && plugin.kind==touch::PluginDecision::none) {
            auto selected = touch::at(secondary ? companion_taps_ : tap_tiles_, x, y);
            if (!selected) step = tap.step;
            else targets = *selected;
        }
    }
    if(plugin.kind!=touch::PluginDecision::none) {execute_plugin(plugin);return true;}
    if (step) { step_page(step, secondary); return true; }
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
        companion_->on_device_tap = [this](const std::string& id, int x, int y, int w, int h) { device_tap(id, x, y, w, h); };
        companion_->on_device_swipe = [this](const std::string& id, int d) {
            { std::lock_guard lk(dev_mu_); if (auto it = devices_.find(id); it != devices_.end()) it->second->view.step += d; }
            mark_dirty();
        };
        companion_->on_device_resize = [this](const std::string& id, int w, int h) {
            { std::lock_guard lk(dev_mu_); auto& slot = devices_[id]; if (!slot) { slot = std::make_unique<DeviceState>(); slot->view.switched = mono(); } slot->size = {w, h}; slot->last_seen = mono(); }
            mark_dirty();
        };
        companion_->on_device_pages = [this](const std::string&) { mark_dirty(); };
    }
    if (sys_) sys_->start();
    auto dirty = [this] { mark_dirty(); };
    if (bf6_) bf6_->start(dirty);
    if (rss_) rss_->start(dirty);
    if (streaming_) streaming_->start(dirty);
    if (ha_) ha_->start();
    if (spotify_) spotify_->start();
    if (meter_) meter_->start();
    if (discord_) discord_->start();
    if (friends_) friends_->start();
    // The network transport runs on one owned worker, never in rendering or
    // settings callbacks. Store generations reject a response after revoke,
    // disable, removal or manifest replacement.
    std::jthread plugin_worker;
    if (plugins_) plugin_worker = std::jthread([this](std::stop_token stop) {
        plugins::Transport transport;
        uint64_t settings_revision = plugin_settings_revision_.load();
        while (!stop.stop_requested()) {
            if (auto revision = plugin_settings_revision_.load(); revision != settings_revision) {
                settings_revision = revision;
                transport.clear();
            }
            for (const auto& entry : plugins_->entries()) {
                if (stop.stop_requested()) break;
                if (!entry.enabled || entry.manifest.sources.empty()) continue;
                plugins::SourceData data;
                auto settings = plugin_settings_ ? plugin_settings_->runtime_values(entry.manifest)
                    : plugins::SettingsValuesResult{};
                if (settings) data = transport.collect(*plugins_, entry.manifest.id, *settings.values,
                    [this, settings_revision, stop] {
                        return !stop.stop_requested() && plugin_settings_revision_.load() == settings_revision;
                    });
                if (stop.stop_requested() || plugin_settings_revision_.load() != settings_revision) continue;
                auto current = plugins_->find(entry.manifest.id);
                if (!current || !current->enabled || current->generation != entry.generation) continue;
                bool changed = false;
                {
                    std::lock_guard lk(plugin_data_mu_);
                    auto& snapshot = plugin_data_[entry.manifest.id];
                    changed = snapshot.generation != entry.generation ||
                              snapshot.settings_revision != settings_revision || snapshot.data != data;
                    snapshot.generation = entry.generation;
                    snapshot.settings_revision = settings_revision;
                    snapshot.data = std::move(data);
                }
                if (changed) mark_dirty();
            }
            for (int i = 0; i < 10 && !stop.stop_requested(); ++i) Sleep(100);
        }
    });
    std::unique_ptr<NativeSettings> native_settings;
    std::unique_ptr<SettingsServer> server;
    std::jthread store_worker;
    if (b(cfg_["ui"], "enabled", true)) {
        NativePluginActions actions;
        actions.list = [this] {
            std::vector<NativePluginEntry> result;
            if (!plugins_) return result;
            for (const auto& installed : plugins_->entries()) {
                NativePluginEntry item;
                item.id = installed.manifest.id;
                item.name = installed.manifest.name;
                item.version = installed.manifest.version;
                item.enabled = installed.enabled;
                item.links = installed.manifest.links; item.links_allowed = installed.links_allowed;
                std::set<std::string> hosts, setting_keys;
                for (const auto& source : installed.manifest.sources) {
                    hosts.insert(source.host);
                    setting_keys.insert(source.setting_keys.begin(), source.setting_keys.end());
                }
                item.hosts.assign(hosts.begin(), hosts.end());
                item.consented_hosts.assign(installed.consented_hosts.begin(), installed.consented_hosts.end());
                item.setting_keys.assign(setting_keys.begin(), setting_keys.end());
                const auto public_settings = plugin_settings_view(installed.manifest.id);
                const auto values = public_settings.value("values", ojson::object());
                const auto secrets = public_settings.value("secrets", ojson::object());
                for (const auto& setting : installed.manifest.settings) {
                    NativePluginSetting field;
                    field.key = setting.key;
                    field.label = setting.label;
                    field.type = setting.type;
                    field.required = setting.required;
                    if (setting.type == "secret") field.secret_set = secrets.value(setting.key, false);
                    else if (values.contains(setting.key)) {
                        const auto& value = values.at(setting.key);
                        field.value = value.is_string() ? value.get<std::string>() : value.dump();
                    }
                    item.settings.push_back(std::move(field));
                }
                result.push_back(std::move(item));
            }
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
        actions.set_host_consent = [this](const std::string& id, const std::string& host, bool allowed) {
            std::string error;
            if (!plugin_set_host_consent(id, host, allowed, error)) throw std::runtime_error(error);
        };
        actions.set_link_consent = [this](const std::string& id,bool allowed) {
            std::string error; if(!plugin_set_link_consent(id,allowed,error)) throw std::runtime_error(error);
        };
        actions.builtin_layout_get=[this](const std::string& key){return builtin_layout_get(key);};
        actions.builtin_layout_apply=[this](const std::string& key,const nlohmann::json& overlay){builtin_layout_apply(key,overlay);};
        actions.builtin_layout_reset=[this](const std::string& key){builtin_layout_reset(key);};
        actions.arrangement_get = [this](const std::string& id) {
            auto result = plugins_ ? plugins_->arrangement_get(id) : plugins::Json();
            if (result.is_null()) throw std::runtime_error("layout:Plugin is not available.");
            return result;
        };
        actions.arrangement_apply = [this](const std::string& id, const nlohmann::json& overlay) {
            if (!plugins_) throw std::runtime_error("layout:Plugin library is not available.");
            auto result = plugins_->set_arrangement(id, overlay.dump());
            if (!result) throw std::runtime_error("layout:"+result.error);
            mark_dirty();
        };
        actions.arrangement_reset = [this](const std::string& id) {
            if (!plugins_) throw std::runtime_error("layout:Plugin library is not available.");
            auto result = plugins_->clear_arrangement(id);
            if (!result) throw std::runtime_error("layout:"+result.error);
            mark_dirty();
        };
        actions.save_settings = [this](const std::string& id, const nlohmann::json& patch) {
            std::string error;
            if (!plugin_save_settings(id, ojson::parse(patch.dump()), error)) throw std::runtime_error(error);
        };
        actions.remove = [this](const std::string& id) {
            std::string error;
            if (!plugin_remove(id, error)) throw std::runtime_error(error);
        };
        // Plugin store client. Stays idle without the pinned production key, with the switch off, or offline.
        struct StoreClient {
            std::mutex mu;
            std::optional<plugins::catalog::Catalog> catalog;
            bool created = false;
            long long last_refresh = 0;
            std::string state = "offline", message;
            bool from_cache = false;
        };
        auto store_client = std::make_shared<StoreClient>();
        const auto store_dir = cfg_path_.parent_path() / L"plugin-store";
        auto store_enabled = [this] { return b(cfg()["ui"], "plugin_store", true); };
        auto store_ensure = [store_dir](StoreClient& c) { // caller holds c.mu
            if (c.created) return;
            c.created = true;
            c.catalog = plugins::catalog::Catalog::from_pinned_key(store_dir, plugins::catalog::public_https_fetcher(), VERSION);
            if (c.catalog && c.catalog->index()) { c.state = "available"; c.from_cache = true; }
        };
        auto store_snapshot = [this, store_client, store_enabled, store_ensure](const std::string& search, const std::string& category) {
            NativeStoreState out;
            if (!store_enabled()) { out.state = "disabled"; return out; }
            std::lock_guard lk(store_client->mu);
            store_ensure(*store_client);
            if (!store_client->catalog) { out.state = "unavailable"; return out; }
            out.state = store_client->state;
            out.from_cache = store_client->from_cache;
            out.message = store_client->message;
            const auto installed = plugins_ ? plugins_->entries() : std::vector<plugins::Installed>{};
            for (const auto& card : store_client->catalog->list({search, category, false}, installed)) {
                NativeStoreCard c;
                const auto& e = card.entry;
                c.id = e.id; c.name = e.name; c.description = e.description; c.version = e.version; c.author = e.author;
                c.license = e.license; c.category = e.category; c.min_app_version = e.min_app_version; c.hosts = e.hosts;
                c.installed_version = card.installed_version; c.new_hosts = card.new_hosts;
                c.has_preview = store_client->catalog->has_preview(e.id);
                switch (card.status) {
                    case plugins::catalog::Status::installed: c.status = "installed"; break;
                    case plugins::catalog::Status::update_available: c.status = "update_available"; break;
                    case plugins::catalog::Status::unsupported: c.status = "unsupported"; break;
                    case plugins::catalog::Status::revoked: c.status = "revoked"; break;
                    default: c.status = "not_installed"; break;
                }
                out.cards.push_back(std::move(c));
            }
            // Themes (signed themes.json): "installed" = a local theme with the same name exists
            {
                std::set<std::string> local_names;
                for (const auto& t : theme::Store().list()) local_names.insert(t.name);
                for (const auto& tc : store_client->catalog->list_themes(search, {})) {
                    NativeThemeCard t;
                    t.id = tc.entry.id; t.name = tc.entry.name; t.description = tc.entry.description; t.version = tc.entry.version;
                    t.author = tc.entry.author; t.license = tc.entry.license;
                    t.status = local_names.count(t.name) ? "installed" : "not_installed";
                    out.themes.push_back(std::move(t));
                }
            }
            return out;
        };
        actions.store_list = store_snapshot;
        actions.store_refresh = [this, store_client, store_enabled, store_ensure, store_snapshot](bool force) {
            if (store_enabled()) {
                std::lock_guard lk(store_client->mu);
                store_ensure(*store_client);
                const long long now = static_cast<long long>(std::time(nullptr));
                const bool due = now - store_client->last_refresh >= (force ? 60 : 24 * 3600);
                if (store_client->catalog && due) {
                    const auto result = store_client->catalog->refresh();
                    store_client->last_refresh = now;
                    store_client->from_cache = result.from_cache;
                    store_client->message = result.error;
                    const bool have_index = store_client->catalog->index().has_value();
                    store_client->state = result.state == plugins::catalog::Availability::invalid ? "invalid"
                        : (result.state == plugins::catalog::Availability::available || have_index) ? "available" : "offline";
                    // Revoked plugins are switched off after every attempt, including offline ones (cached list).
                    if (plugins_ && !store_client->catalog->enforce_revocations(*plugins_).empty()) mark_dirty();
                }
            }
            return store_snapshot("", "");
        };
        actions.store_preview = [store_client, store_enabled](const std::string& id) -> std::string {
            if (!store_enabled()) return {};
            std::lock_guard lk(store_client->mu);
            if (!store_client->catalog) return {};
            return store_client->catalog->preview(id).value_or(std::string());
        };
        actions.store_theme_install = [store_client, store_enabled](const std::string& id) {
            if (!store_enabled()) throw std::runtime_error("Der Plugin-Store ist ausgeschaltet.");
            std::lock_guard lk(store_client->mu);
            if (!store_client->catalog) throw std::runtime_error("Der Plugin-Store ist nicht verfügbar.");
            std::string error;
            const auto result = store_client->catalog->install_theme(id, [&error](std::string_view bytes) {
                plugins::StoreResult out;
                out.ok = theme::Store().import_text(std::string(bytes), error).has_value();   // prüft nochmals streng und speichert atomar
                out.error = error;
                return out;
            });
            if (!result) throw std::runtime_error(result.error.empty() ? "Theme-Installation fehlgeschlagen." : result.error);
        };
        actions.store_install = [this, store_client](const std::string& id) {
            std::lock_guard lk(store_client->mu);
            if (!store_client->catalog || !plugins_) throw std::runtime_error("Der Plugin-Store ist nicht verfügbar.");
            std::string error;
            const auto result = store_client->catalog->install(*plugins_, id, [this, &error](std::string_view bytes, bool force_disabled) {
                plugins::StoreResult out;
                out.ok = plugin_import(bytes, error, force_disabled);
                out.error = error;
                return out;
            });
            if (!result) throw std::runtime_error(result.error.empty() ? "Installation fehlgeschlagen." : result.error);
        };
        // Background sync: first after 30 s, then hourly; the callback itself limits real fetches to once a day.
        if (actions.store_refresh) {
            store_worker = std::jthread([refresh = actions.store_refresh](std::stop_token stop) {
                for (int i = 0; i < 300 && !stop.stop_requested(); ++i) Sleep(100);
                while (!stop.stop_requested()) {
                    try { refresh(false); } catch (...) {}
                    for (int i = 0; i < 36000 && !stop.stop_requested(); ++i) Sleep(100);
                }
            });
        }
        try { native_settings = std::make_unique<NativeSettings>(*this, std::move(actions)); }
        catch (const std::exception& error) { LOGE("settings", "Native UI konnte nicht gestartet werden: %s", error.what()); }
        catch (...) { LOGE("settings", "Native UI konnte nicht gestartet werden: unbekannter Fehler"); }
        show_settings_callback_ = [&native_settings] {
            if (native_settings) native_settings->show();
            else MessageBoxW(nullptr,
                i18n::lang() == "en" ? L"Native settings could not be opened. See the ScreenBuddy log for details."
                                     : L"Die nativen Einstellungen konnten nicht geöffnet werden. Einzelheiten stehen im ScreenBuddy-Protokoll.",
                L"ScreenBuddy", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
        };
        // The loopback API and embedded guide are independent of the native settings window.
        server = std::make_unique<SettingsServer>(*this, [this] { show_settings(); });
        if (!server->start()) server.reset();
        if (show_settings_on_start_) show_settings();
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
    tray.on_system_theme = [this] { system_theme_changed(); };
    tray.start();
    {
        std::lock_guard lk(notify_mu_);
        tray_ = &tray;
    }
    if (update_) update_->start();
    display_loop();
    plugin_worker.request_stop();
    if (plugin_worker.joinable()) plugin_worker.join();
    store_worker.request_stop();
    if (store_worker.joinable()) store_worker.join();
    if (update_) update_->stop();
    {
        std::lock_guard lk(notify_mu_);
        tray_ = nullptr;
    }
    tray.stop();
    // War das Einstellungsfenster beim Neustart offen (z. B. nach dem Speichern), öffnet es die neue Instanz wieder
    settings_open_at_exit_ = native_settings && native_settings->visible();
    if (server) server->stop();
    if (companion_output_) {
        if (b(dp_, "screen_off_on_exit", true) && !restart_) companion_output_->screen_off();
        companion_output_->close();
    }
    show_settings_callback_ = {};
    native_settings.reset();
    if (friends_) friends_->stop();
    if (ha_) ha_->stop();
    if (meter_) meter_->stop();
    if (spotify_) spotify_->stop();
    if (discord_) discord_->stop();
    if (bf6_) bf6_->stop();
    if (rss_) rss_->stop();
    if (streaming_) streaming_->stop();
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
