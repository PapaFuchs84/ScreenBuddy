// © 2026 FuchsLabs
// Windows-Integration ohne Administratorrechte: Autostart (Registry, nur dieses Benutzerkonto) und der Umgang mit
// der Hersteller-App des Displays, die den Anschluss sonst belegt.
#pragma once

#include <string>

namespace fl::winsetup {

inline constexpr const char* AUTOSTART_FLAG = "--autostart";  // Start mit Windows: nur ins Tray

std::wstring launch_command();
bool autostart_enabled();
void set_autostart(bool enabled);
void set_autostart_for(const std::wstring& exe, bool enabled);  // für den Installer (andere EXE als die laufende)
void upgrade_autostart();       // Einträge älterer Versionen (ohne --autostart oder andere EXE) umstellen

bool vendor_running();
bool vendor_stop();
bool vendor_autostart_enabled();
bool vendor_disable_autostart();  // mit Windows-Abfrage (UAC), der Nutzer bestätigt selbst

}  // namespace fl::winsetup
