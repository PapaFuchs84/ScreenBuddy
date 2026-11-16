#pragma once
#include <filesystem>
namespace fl::distribution {
bool packaged();
std::filesystem::path data_dir();
// Einmalige Übernahme der EXE-Einstellungen (config.toml, companion.json, spotify.token, views, themes, plugins, plugin-settings) in einen leeren Store-Datenordner.
// Überschreibt nie vorhandene Dateien; config.toml wird zuletzt kopiert, ein Abbruch wird beim nächsten Start wiederholt.
bool import_standalone(const std::filesystem::path& source, const std::filesystem::path& destination);
std::filesystem::path standalone_dir();   // %LOCALAPPDATA%\FuchsLabs\USB-Display
bool startup_enabled();
void set_startup(bool enabled);
void open_updates();
}
