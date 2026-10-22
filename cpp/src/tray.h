// © 2026 FuchsLabs
// Tray-Symbol (Infobereich): Doppelklick oder Menü „Einstellungen“ öffnet die Einstellungsseite; Rechtsklick zeigt das Menü.
#pragma once

#include <functional>
#include <string>
#include <thread>

namespace fl {

void* make_hicon(int size);  // Fuchs-Symbol in passender Größe (HICON)

class Tray {
public:
    Tray(std::string tip, std::function<void()> on_open, std::function<void()> on_restart, std::function<void()> on_quit);
    ~Tray();
    std::function<void()> on_guide;  // „Anleitung öffnen“ im Menü (im Standardbrowser), vor start() setzen
    void start();
    void stop();
    bool handle(void* hwnd, unsigned msg, unsigned long long wp, long long lp, long long& result);

private:
    void run();
    void menu(void* hwnd);
    std::string tip_;
    std::function<void()> on_open_, on_restart_, on_quit_;
    void* hwnd_ = nullptr;
    void* icon_ = nullptr;
    bool added_ = false;
    unsigned taskbar_created_ = 0;
    std::thread th_;
};

}  // namespace fl
