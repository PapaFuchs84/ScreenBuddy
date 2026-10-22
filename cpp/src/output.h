// © 2026 FuchsLabs
// Ausgabe der fertigen Bilder: 3,5"-USB-Display (seriell) oder ein normaler Bildschirm als Vollbildfenster.
#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "config.h"
#include "gfx.h"

namespace fl {

class Output {
public:
    virtual ~Output() = default;
    virtual std::pair<int, int> size() const = 0;
    virtual void connect() = 0;               // wirft std::runtime_error mit verständlicher Meldung
    virtual int show(const Image& frame) = 0;  // Rückgabe: übertragene Zeilen
    virtual void invalidate() = 0;            // nächstes Bild komplett senden
    virtual void screen_off() = 0;
    virtual void close() = 0;
    virtual bool is_monitor() const { return false; }
};

enum class Orientation { Portrait = 0, ReversePortrait = 1, Landscape = 2, ReverseLandscape = 3 };

// 3,5"-USB-Display (480 × 320, Hardware-Revision A): USB-CDC-Serial, 115200 Baud, RGB565 little-endian
class SerialDisplay : public Output {
public:
    SerialDisplay(std::string port, Orientation o, int brightness, bool reset_on_start);
    ~SerialDisplay() override;
    std::pair<int, int> size() const override;
    void connect() override;
    int show(const Image& frame) override;
    void invalidate() override;
    void screen_off() override;
    void close() override;

    static std::optional<std::string> find_port();
    static std::string rgb565le(const Image& img, int y0, int y1);

private:
    void open(int attempts = 15);
    void close_port();
    void write(const std::string& data);
    void hello();
    void set_brightness(int level);
    void set_orientation();
    void blit(const Image& img, int y0, int y1);
    int show_locked(const Image& frame);

    std::string port_cfg_;
    Orientation orientation_;
    int brightness_;
    bool reset_on_start_;
    int band_height_ = 8;
    void* handle_ = nullptr;
    std::optional<Image> last_frame_;
    std::mutex mu_;
};

struct MonitorInfo {
    std::string device, name;
    int x = 0, y = 0, w = 0, h = 0;
    bool primary = false;
};
std::vector<MonitorInfo> list_monitors();
ojson monitors_json();
ojson touch_info();
bool open_touch_setup();  // Windows-Assistent; keine automatische Änderung der Gerätezuordnung
void dpi_aware();

// HDMI/USB-C-Bildschirm (z. B. Leisten mit 1920 × 720): randloses Vollbildfenster, Touch per Wischen/Tippen
class MonitorDisplay : public Output {
public:
    MonitorDisplay(std::string device, bool touch);
    ~MonitorDisplay() override;
    std::pair<int, int> size() const override;
    void connect() override;
    int show(const Image& frame) override;
    void invalidate() override {}
    void screen_off() override;
    void close() override;
    bool is_monitor() const override { return true; }
    bool setup_touch();

    std::function<void(int)> on_step;  // +1 nächste Seite, -1 vorige (aus dem Fenster-Thread)

    // Geste: Wischen nach links = nächste Seite, nach rechts = vorige; Tippen links (Drittel) = vorige, sonst nächste
    struct Gesture {
        int w, h;
        std::optional<std::tuple<int, int, double>> start;
        void down(int x, int y);
        int end(int x, int y);
    };

    // Fensterprozedur (intern)
    bool handle(void* hwnd, unsigned msg, unsigned long long wp, long long lp, long long& result);

private:
    void run();
    void fullscreen(bool show);

    std::string device_;
    bool touch_;
    std::optional<MonitorInfo> mon_;
    std::atomic<void*> hwnd_{nullptr};
    void* taskbar_ = nullptr;       // nur im Fenster-Thread
    void* setup_process_ = nullptr;
    bool visible_ = true;
    unsigned pointer_id_ = 0;
    std::thread thread_;
    std::mutex mu_;
    std::vector<uint32_t> bits_;
    std::atomic<bool> ready_{false};
    Gesture gesture_{0, 0, {}};
    Gesture mouse_gesture_{0, 0, {}};
};

}  // namespace fl
