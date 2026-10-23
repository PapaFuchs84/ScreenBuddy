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
#include <vector>

#include "config.h"
#include "gfx.h"
#include "models.h"
#include "output.h"
#include "render.h"
#include "touch.h"

namespace fl {

namespace bf6 { class Bf6Stats; }
namespace rss { class RssFeeds; }
namespace sysinfo { class SysMonitor; }
namespace discord { class DiscordFeed; }
namespace friends { class FriendsHub; }
namespace ha { class HaClient; }

class App {
public:
    App(ojson cfg, std::filesystem::path cfg_path, std::unique_ptr<Output> out, std::optional<std::filesystem::path> preview_dir, bool show_settings_on_start = false);
    ~App();
    void run();  // bis Beenden/Neustart

    // ---------- für die Einstellungsseite ----------
    ojson cfg();                                    // Kopie der aktuellen Konfiguration
    void set_setup_done();
    const std::filesystem::path& cfg_path() const { return cfg_path_; }
    std::pair<int, int> size() const { return size_; }
    bool discord_ready();
    ojson member_list();
    std::vector<std::string> active_pages();
    ojson connections();
    std::optional<Image> last_frame();
    ojson page_navigation();
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
    void step_page(int delta);                      // Touch/Einstellungen: eine Ansicht vor (+1) oder zurück (-1)
    void mark_dirty();
    void show_settings();
    bool setup_touch();

private:
    struct RendererSet;
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
    std::vector<PageEntry> page_list(const std::vector<Friend>& friends, const std::vector<Friend>& people, RendererSet& r);
    DrawResult draw(const std::vector<PageEntry>& pages, const std::vector<render::Box>& boxes, int start);
    Image welcome();
    std::string friends_message();
    bool connect_display();
    void output(const Image& frame, bool transition, const ojson& navigation);
    void display_loop();

    std::mutex cfg_mu_;
    ojson cfg_;
    std::atomic<bool> setup_done_{false};
    ojson dp_;
    std::filesystem::path cfg_path_;
    std::unique_ptr<Output> out_;
    std::optional<std::filesystem::path> preview_dir_;
    bool show_settings_on_start_ = false;
    bool settings_open_at_exit_ = false;
    std::mutex tap_mu_;
    std::vector<touch::Tile> tap_tiles_;     // Zeilenziele aus demselben Snapshot wie das letzte Bild
    std::function<void()> show_settings_callback_;
    std::pair<int, int> size_;

    std::mutex mu_;               // Zustand der Anzeige
    std::condition_variable cv_;
    bool dirty_ = false;
    std::atomic<bool> stop_{false};
    std::atomic<bool> restart_{false};
    std::optional<Image> last_frame_;
    ojson navigation_ = {{"index", 0}, {"total", 0}, {"visible", 0}, {"current", ojson::array()}, {"can_step", false}};
    bool display_ok_;
    std::string display_error_;
    int page_ = 0;
    double page_switched_, last_full_;
    std::vector<std::string> current_;
    std::atomic<int> step_{0};
    std::function<Image()> redraw_;
    std::map<std::pair<int, int>, std::unique_ptr<RendererSet>> sets_;

    std::unique_ptr<bf6::Bf6Stats> bf6_;
    std::unique_ptr<rss::RssFeeds> rss_;
    std::unique_ptr<sysinfo::SysMonitor> sys_;
    std::unique_ptr<discord::DiscordFeed> discord_;
    std::unique_ptr<friends::FriendsHub> friends_;
    std::unique_ptr<ha::HaClient> ha_;
};

}  // namespace fl
