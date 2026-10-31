// © 2026 FuchsLabs
// Die App: Datenquellen, Seitenwechsel und Ausgabe. Die Einstellungsseite (webui) und das Tray-Symbol greifen
// über die öffentlichen Methoden zu (alle threadsicher).
#pragma once

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "config.h"
#include "gfx.h"
#include "models.h"
#include "output.h"
#include "plugin_store.h"
#include "plugin_settings.h"
#include "render.h"
#include "touch.h"

namespace fl {

namespace bf6 { class Bf6Stats; }
namespace rss { class RssFeeds; }
namespace tmdb { class TmdbFeed; }
namespace sysinfo { class SysMonitor; }
namespace discord { class DiscordFeed; }
namespace friends { class FriendsHub; }
namespace ha { class HaClient; }
namespace spotify { class SpotifyClient; }
namespace update { class UpdateChecker; }
class Tray;

class App {
public:
    App(ojson cfg, std::filesystem::path cfg_path, std::unique_ptr<Output> out, std::optional<std::filesystem::path> preview_dir, bool show_settings_on_start = false);
    ~App();
    void run();  // bis Beenden/Neustart

    // ---------- für die Einstellungsseite ----------
    ojson cfg();                                    // Kopie der aktuellen Konfiguration
    void set_setup_done();
    const std::filesystem::path& cfg_path() const { return cfg_path_; }
    std::pair<int, int> size() const;
    bool discord_ready();
    ojson member_list();
    std::vector<std::string> active_pages(bool companion = false, const std::vector<std::string>* pages_override = nullptr);
    ojson connections();
    std::optional<Image> last_frame(bool companion = false);
    std::optional<Image> device_frame(const std::string& device_id);   // Vorschau einer Companion-App (eigenes Bild, sonst das gemeinsame)
    ojson page_navigation(bool companion = false);
    std::optional<Image> preview_theme(const ojson& display_override);
    bool display_ok();
    std::string display_error();
    std::optional<std::string> discord_status();   // nullopt = Discord nicht eingerichtet
    ojson friend_accounts();
    bool touch_enabled() const;                     // Touch an und Monitor-Ausgabe: nur dann lassen sich Schalter bedienen
    void request_stop(double delay = 0);
    void request_restart(double delay = 0);
    bool restart_requested() const { return restart_; }
    bool settings_open_at_exit() const { return settings_open_at_exit_; }
    bool open_item_at(int x, int y);        // Ein Ziel des tatsächlich angezeigten Bildes öffnen
    void step_page(int delta, bool companion = false);                      // Touch/Einstellungen: eine Ansicht vor (+1) oder zurück (-1)
    void mark_dirty();
    void show_settings();
    bool setup_touch();
    ojson update_info();                            // Update-Hinweis für die Einstellungsseite (leer, wenn aus)
    bool update_check_now();                        // „Jetzt prüfen“; false = aus oder zu früh

    // Deklarative Plugins: Import, Vorschau und einzeln autorisierte Datenquellen.
    ojson plugin_catalog();
    bool plugin_import(std::string_view manifest, std::string& error, bool force_disabled = false);
    bool plugin_set_enabled(const std::string& id, bool enabled, std::string& error);
    bool plugin_set_host_consent(const std::string& id, const std::string& host, bool allowed, std::string& error);
    ojson plugin_settings_view(const std::string& id);
    bool plugin_save_settings(const std::string& id, const ojson& patch, std::string& error);
    bool plugin_remove(const std::string& id, std::string& error);
    std::optional<Image> plugin_preview(std::string_view manifest, std::string& error);
    spotify::SpotifyClient* spotify() { return spotify_.get(); }   // nullptr = Seite aus oder keine Client-ID
    NetworkDisplay* companion() { return companion_; }  // Companion-App (Android); nullptr = aus

private:
    struct RendererSet;
    struct ViewState {
        int page = 0;
        double switched = 0;
        std::atomic<int> step{0};
        std::vector<std::string> current;
        std::string minute;
        std::pair<int, int> size{0, 0};
    } primary_view_, companion_view_;
    // Unabhängiger Companion-Betrieb (companion.sync = aus): Ansicht, Größe, letztes Bild und Touch-Ziele je Gerät
    struct DeviceState {
        ViewState view;
        std::pair<int, int> size{0, 0};
        Image frame;
        std::vector<touch::Tile> taps;
        ojson navigation = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
        double last_seen = 0;
    };
    struct RenderOut {
        bool skipped = true;
        Image frame;
        std::vector<touch::Tile> taps;
        ojson navigation;
        bool switched = false;
    };
    struct PageEntry {
        std::string key;
        int n;
        std::function<Image(int, RendererSet&)> fn;
        std::function<touch::Rows(int, RendererSet&)> targets;
        std::vector<std::function<void()>> actions;   // Home Assistant: Schalt-Aktion je Kachel
    };
    struct DrawResult {
        Image frame;
        std::vector<std::string> keys;
        std::vector<touch::Tile> taps;
    };
    RendererSet& renderers_for(int w, int h);
    std::vector<PageEntry> page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r, bool companion = false,
                                    const std::vector<std::string>* pages_override = nullptr);
    DrawResult draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes, int start, std::pair<int, int> size);
    Image welcome(std::pair<int, int> size);
    void render_view(bool secondary, bool dirty);
    RenderOut render_core(ViewState& view, std::pair<int, int> size, double page_seconds, bool phone,
                          const std::vector<std::string>* pages_override, bool dirty, bool register_redraw);
    bool independent() const;
    void render_devices(bool dirty);
    void device_tap(const std::string& id, int x, int y, int w, int h);
    std::string friends_message();
    bool connect_display();
    void output(const Image& frame, bool transition, const ojson& navigation);
    void display_loop();
    bool tap_at(int x, int y, bool secondary = false, int w = 0, int h = 0);
    void companion_tap(int x, int y, int w, int h);

    std::mutex cfg_mu_;
    ojson cfg_;
    std::atomic<bool> setup_done_{false};
    ojson dp_;
    std::filesystem::path cfg_path_;
    std::unique_ptr<Output> out_;
    std::unique_ptr<NetworkDisplay> companion_output_;   // zusätzlich zum USB-Display/Bildschirm
    NetworkDisplay* companion_ = nullptr;               // out_ (Ausgabe „network“) oder companion_output_
    std::optional<std::filesystem::path> preview_dir_;
    bool show_settings_on_start_ = false;
    bool settings_open_at_exit_ = false;
    std::mutex dev_mu_;                      // devices_
    std::map<std::string, std::unique_ptr<DeviceState>> devices_;
    bool was_independent_ = false;
    std::mutex tap_mu_;
    std::vector<touch::Tile> companion_taps_;
    std::vector<touch::Tile> tap_tiles_;     // Zeilenziele aus demselben Snapshot wie das letzte Bild
    std::function<void()> show_settings_callback_;
    std::pair<int, int> size_;

    mutable std::mutex mu_;       // Zustand der Anzeige
    std::condition_variable cv_;
    bool dirty_ = false;
    std::atomic<bool> stop_{false};
    std::atomic<bool> restart_{false};
    std::mutex stop_threads_mu_;
    std::vector<std::jthread> stop_threads_;
    std::optional<Image> last_frame_, companion_frame_;
    ojson companion_navigation_;
    ojson navigation_ = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    bool display_ok_;
    std::string display_error_;
    double last_full_;
    std::function<Image()> redraw_;
    std::map<std::pair<int, int>, std::unique_ptr<RendererSet>> sets_;

    std::unique_ptr<bf6::Bf6Stats> bf6_;
    std::unique_ptr<rss::RssFeeds> rss_;
    std::unique_ptr<tmdb::TmdbFeed> streaming_;
    std::unique_ptr<sysinfo::SysMonitor> sys_;
    std::unique_ptr<discord::DiscordFeed> discord_;
    std::unique_ptr<friends::FriendsHub> friends_;
    std::unique_ptr<ha::HaClient> ha_;
    std::unique_ptr<spotify::SpotifyClient> spotify_;
    std::unique_ptr<update::UpdateChecker> update_;
    std::unique_ptr<plugins::Store> plugins_;
    std::unique_ptr<plugins::SettingsStore> plugin_settings_;
    std::atomic<uint64_t> plugin_settings_revision_{0};
    struct PluginSnapshot { uint64_t generation = 0, settings_revision = 0; plugins::SourceData data; };
    std::mutex plugin_data_mu_;
    std::map<std::string, PluginSnapshot> plugin_data_;
    std::mutex notify_mu_;
    Tray* tray_ = nullptr;                        // nur während run() gültig
};

}  // namespace fl
