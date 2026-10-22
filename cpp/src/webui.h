// © 2026 FuchsLabs
// Einstellungsoberfläche: kleine Webseite, nur lokal (127.0.0.1) im Browser erreichbar.
// Schutz gegen fremde Webseiten: nur Host 127.0.0.1/localhost, keine Abrufe von fremden Seiten (Sec-Fetch-Site),
// Schreibzugriffe brauchen einen eigenen Header (löst im Browser eine Vorabprüfung aus, die wir nicht erlauben).
#pragma once

#include <memory>
#include <functional>
#include <thread>

namespace fl {

class App;

class SettingsServer {
public:
    explicit SettingsServer(App& app, std::function<void()> show_settings);
    ~SettingsServer();
    bool start();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

}  // namespace fl
