// © 2026 FuchsLabs
// Eingebauter Installer: dieselbe EXE installiert sich (ohne Administratorrechte) nach
// %LOCALAPPDATA%\FuchsLabs\USB-Display, legt Startmenü-Eintrag, Autostart und den Eintrag unter
// „Apps & Features“ an, übernimmt die Einstellungen der Version 1.x und dient später als Updater und Deinstaller.
#pragma once

#include <string>

namespace fl::installer {

bool running_installed();           // läuft die EXE aus dem Installationsordner?
int run_setup(bool silent, bool no_lcd = false);         // Installieren/Aktualisieren (Fenster); Rückgabe: Exit-Code
int run_uninstall(bool silent);     // Deinstallieren (aus „Apps & Features“)
int finish_uninstall(const std::wstring& dir, unsigned long parent_pid);  // Aufräumen aus dem Temp-Ordner
bool stop_running_app(int port);    // laufende App (auch 1.x) beenden
std::string preview_png(bool update, bool legacy, int state, const std::string& status, float scale);  // für Tests

}  // namespace fl::installer
