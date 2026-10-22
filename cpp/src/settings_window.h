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

    struct Impl;
private:
    std::shared_ptr<Impl> d_;
};

}  // namespace fl