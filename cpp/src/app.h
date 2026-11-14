// © 2026 FuchsLabs
// Die App: Datenquellen, Seitenwechsel und Ausgabe. Die Einstellungsseite (webui) und das Tray-Symbol greifen
// über die öffentlichen Methoden zu (alle threadsicher).
#pragma once
#include "custom_view.h"
#include "game_runtime.h"
#include "game_intro.h"

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
#include "builtin_catalog.h"
#include "gfx.h"
#include "models.h"
#include "output.h"
#include "frame_pump.h"
#include "device_profile.h"
#include "device_overlays.h"
#include "plugin_store.h"
#include "plugin_render.h"
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
namespace audio { class LevelMeter; }
namespace update { class UpdateChecker; }
class Tray;

// Dateispeicher eingebauter Anordnungen; die App und der serielle UI-Jobweg teilen ihn.
namespace builtin {
struct LayoutState {
    std::optional<Overlay> overlay;
    std::string state = "none";
    uint64_t revision = 0;
    device::OverlayMap devices;
};
class LayoutStore {
public:
    explicit LayoutStore(std::filesystem::path directory);
    LayoutState get(const std::string& key, const std::string& device = {}) const;
    Json view(const std::string& key, const std::string& device = {}) const;
    std::string apply(const std::string& key, std::string_view text, const std::string& device = {});
    std::string reset(const std::string& key, const std::string& device = {});
private:
    std::filesystem::path directory_;
    mutable std::mutex mutex_;
    std::map<std::string,LayoutState> states_;
    std::string reset_locked(const std::string& key);
};
}
class App {
public:
    App(ojson cfg, std::filesystem::path cfg_path, std::unique_ptr<Output> out, std::optional<std::filesystem::path> preview_dir, bool show_settings_on_start = false);
    ~App();
    void run();  // bis Beenden/Neustart

    // ---------- für die Einstellungsseite ----------
    ojson cfg();                                    // Kopie der aktuellen Konfiguration
    ai::Service& ai_status() { return *ai_; }
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
    void system_theme_changed();   // "An Windows anpassen": Farben neu lesen und neu zeichnen
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
    bool plugin_set_link_consent(const std::string& id, bool allowed, std::string& error);
    ojson plugin_settings_view(const std::string& id);
    bool plugin_save_settings(const std::string& id, const ojson& patch, std::string& error);
    bool plugin_remove(const std::string& id, std::string& error);
    std::optional<Image> plugin_preview(std::string_view manifest, std::string& error);
    std::optional<Image> plugin_preview(std::string_view manifest, std::string& error, std::pair<int,int> dimensions, bool fixed_sample_clock = false);
    nlohmann::json builtin_layout_get(const std::string& key, const std::string& device = {});
    void builtin_layout_apply(const std::string& key, const nlohmann::json& overlay, const std::string& device = {});
    void builtin_layout_reset(const std::string& key, const std::string& device = {});
    std::optional<Image> page_preview(const std::string& key, const nlohmann::json& overlay,
                                    std::pair<int,int> dimensions, std::string& error);
    std::optional<Image> custom_preview(const custom::View& view, const std::string& device, std::pair<int,int> dimensions);
    std::vector<custom::Tile> custom_palette();
    void render_primary_for_test() { render_view(false, true); }   // Testnaht: ein synchroner Zeichendurchlauf ohne Anzeige-Thread
    spotify::SpotifyClient* spotify() { return spotify_.get(); }   // nullptr = Seite aus oder keine Client-ID
    NetworkDisplay* companion() { return companion_; }  // Companion-App (Android); nullptr = aus

private:
    struct RendererSet;
    struct ViewState {
        game::Intro game_intro;
        Image previous_frame;
        uint64_t game_generation=0;
        bool game_active=false;
        double game_entered=0, saved_switched=0;
        int saved_page=0;
        std::map<std::string,uint64_t> builtin_revisions;
        std::map<std::string,std::map<std::string,plugins::schedule::ZoneState>> cycles;
        std::map<std::string,std::shared_ptr<plugins::FadeCache>> fade_caches;
        std::map<std::string,std::pair<uint64_t,uint64_t>> cycle_generations;
        double next_deadline = plugins::schedule::never;
        bool animating = false;
        std::optional<double> render_ms;
        double last_render = -plugins::schedule::never;
        std::map<std::string, std::pair<uint64_t, double>> ai_motion;
        void track_ai_animation(const ai::Snapshot& snapshot, const std::string& provider, double tick, bool transitions);
        std::vector<std::string> next_requests;
        std::vector<std::string> frame_next_requests; // nur Anzeige-Thread
        std::mutex next_mu;
        std::string page_request;
        device::Area area;
        double keepout_warning = -60;
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
        device::Area area;
        std::vector<touch::Tile> taps;
        ojson navigation = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
        double last_seen = 0;
    };
    struct RenderOut {
        bool skipped = true;
        Image frame;
        device::Area area;
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
        std::function<void(PageEntry&)> prepare;
        std::function<plugins::DrawOut(RendererSet&)> plugin_draw;
        std::function<custom::DrawOut(RendererSet&)> custom_draw;
        std::function<std::pair<Image,std::vector<touch::ArrangedTarget>>(int,RendererSet&)> builtin_draw;
    };
    struct DrawResult {
        Image frame;
        std::vector<std::string> keys;
        std::vector<touch::Tile> taps;
    };
    RendererSet& renderers_for(int w, int h);
    std::vector<PageEntry> page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r, bool companion = false,
                                    const std::vector<std::string>* pages_override = nullptr, ViewState* view = nullptr, double now = 0, bool phone = false, const std::string& device = {}, const std::string& forced_custom = {});
    DrawResult draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes, int start, std::pair<int, int> size, device::Area area);
    Image welcome(std::pair<int, int> size);
    void render_view(bool secondary, bool dirty);
    RenderOut render_core(ViewState& view, std::pair<int, int> size, double page_seconds, bool phone,
                          const std::vector<std::string>* pages_override, bool dirty, bool register_redraw, const std::string& id = {});
    device::Profile profile_for(bool phone, const std::string& id = {});
    std::string primary_layout_id() const;
    nlohmann::json layout_displays();
    bool independent() const;
    void render_devices(bool dirty);
    void device_tap(const std::string& id, int x, int y, int w, int h);
    std::string friends_message();
    bool connect_display();
    void output(const Image& frame, bool transition, const ojson& navigation);
    void transfer(const FramePump::Job& job);   // synchronous send (serial: on the pump worker)
    void display_loop();
    bool tap_at(int x, int y, bool secondary = false, int w = 0, int h = 0);
    void companion_tap(int x, int y, int w, int h);

    std::mutex cfg_mu_;
    ojson cfg_;
    std::atomic<bool> setup_done_{false};
    ojson dp_;
    std::filesystem::path cfg_path_;
    std::unique_ptr<Output> out_;
    // Serial transfers run here so Companion rendering never waits for USB.
    // Declared after out_ (destroyed first); ~App and shutdown stop it explicitly.
    std::unique_ptr<FramePump> serial_pump_;
    bool serial_output() const { return out_ && !out_->is_monitor() && out_.get() != (Output*)companion_; }
    std::unique_ptr<NetworkDisplay> companion_output_;  // zusätzlich zum USB-Display/Bildschirm
    NetworkDisplay* companion_ = nullptr;               // out_ (Ausgabe „network“) oder companion_output_
    std::optional<std::filesystem::path> preview_dir_;
    bool show_settings_on_start_ = false;
    bool settings_open_at_exit_ = false;
    std::mutex dev_mu_;                      // devices_
    std::map<std::string, std::unique_ptr<DeviceState>> devices_;
    bool was_independent_ = false;
    touch::PluginAccess plugin_access(const std::string& id);
    void execute_plugin(const touch::PluginDecision& decision);
    void queue_custom_page(const std::string& page, bool phone, const std::string& device);
    static void queue_plugin_next(ViewState& view, const touch::PluginDecision& decision);
    std::mutex plugin_tap_mu_;
    std::map<std::string,double> plugin_last_open_;
    std::mutex tap_mu_;
    device::Area tap_area_, companion_tap_area_;  // zusammen mit Bild und Zielen veröffentlicht
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
    ojson companion_navigation_ = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    ojson navigation_ = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    bool display_ok_;
    std::string display_error_;
    double last_full_;
    std::function<Image()> redraw_;
    std::map<std::pair<int, int>, std::unique_ptr<RendererSet>> sets_;

    std::unique_ptr<bf6::Bf6Stats> bf6_;
    std::unique_ptr<ai::Service> ai_;
    std::unique_ptr<rss::RssFeeds> rss_;
    std::unique_ptr<tmdb::TmdbFeed> streaming_;
    std::unique_ptr<sysinfo::SysMonitor> sys_;
    std::unique_ptr<game::Runtime> game_;
    std::unique_ptr<discord::DiscordFeed> discord_;
    std::unique_ptr<friends::FriendsHub> friends_;
    std::unique_ptr<ha::HaClient> ha_;
    std::unique_ptr<spotify::SpotifyClient> spotify_;
    std::unique_ptr<audio::LevelMeter> meter_;   // Amplifier der Spotify-Seite (Systemausgabe, nur lokal)
    std::unique_ptr<update::UpdateChecker> update_;
    std::unique_ptr<builtin::LayoutStore> builtin_layouts_;
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
