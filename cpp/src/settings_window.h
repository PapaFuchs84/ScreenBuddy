// © 2026 FuchsLabs
// Native Windows host for the embedded settings UI.
#pragma once

#include <memory>

namespace fl {

class SettingsWindow {
public:
    explicit SettingsWindow(int port);
    ~SettingsWindow();
    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;
    void show();
    bool visible() const;  // offen und nicht minimiert (für den Neustart nach dem Speichern)

    struct Impl;
private:
    std::shared_ptr<Impl> d_;
};

}  // namespace fl