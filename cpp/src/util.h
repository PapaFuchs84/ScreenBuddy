// © 2026 FuchsLabs
// Kleine Helfer: Text (UTF-8 ⇄ UTF-16), Protokoll, Zeit, Pfade.
#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fl {

inline constexpr const char* VERSION = "2.5.0-dev.1";
inline constexpr const char* UA = "ScreenBuddy/2.5.0-dev.1";

// ---------- Text ----------
std::wstring widen(std::string_view s);
std::string narrow(std::wstring_view s);
std::string trim(std::string_view s);
std::string lower_ascii(std::string s);
std::string casefold(std::string_view s);   // Unicode-Kleinschreibung (ß → ss), für Vergleiche und Sortierung
bool starts_with_any(std::string_view s, std::initializer_list<std::string_view> prefixes);
std::vector<std::string> split(std::string_view s, std::string_view sep);
std::string join(const std::vector<std::string>& parts, std::string_view sep);
std::string replace_all(std::string s, std::string_view from, std::string_view to);
std::u32string to_u32(std::string_view s);
std::string from_u32(std::u32string_view s);
std::string collapse_spaces(std::string_view s);   // " ".join(s.split())
std::string format_num(double v, int decimals);    // 1,5 statt 1.5 (deutsches Komma)
std::string strprintf(const char* fmt, ...);

// ---------- Zeit ----------
double mono();                                     // Sekunden, monoton
std::string clock_hhmm();                          // "20:15"
std::string clock_hhmm(std::time_t t);

// ---------- Prozesse ----------
// Befehl ohne Fenster ausführen und die Ausgabe (stdout+stderr) lesen; nach timeout_ms abbrechen (dann leer).
// Die Ausgabe wird aus der Windows-Codepage (OEM) nach UTF-8 umgewandelt, wenn utf8 = false.
std::string run_hidden(const std::wstring& cmdline, unsigned timeout_ms, int* exit_code = nullptr, bool oem = false);

// Testinstanz neben der echten App (Umgebungsvariable FUCHSLABS_INSTANCE): eigener Mutex, verändert weder
// Autostart noch installierte Dateien
bool test_instance();

// ---------- Pfade ----------
std::filesystem::path exe_path();
std::filesystem::path install_dir();               // %LOCALAPPDATA%\FuchsLabs\USB-Display
std::filesystem::path legacy_dir();                // %USERPROFILE%\FuchsLabs\USB-Display (Version 1.x)
std::wstring mutex_name();                         // nur eine laufende App (gleicher Name wie Version 1.x)
inline std::string u8(const std::filesystem::path& p) { return narrow(p.wstring()); }  // Pfad als UTF-8-Text

// ---------- Protokoll ----------
enum class Level { Debug, Info, Warning, Error };
void log_init(const std::filesystem::path& file, bool verbose);
void log(Level lv, std::string_view name, std::string_view msg);
bool is_verbose();
#define LOGD(name, ...) do { if (::fl::is_verbose()) ::fl::log(::fl::Level::Debug, name, ::fl::strprintf(__VA_ARGS__)); } while (0)
#define LOGI(name, ...) ::fl::log(::fl::Level::Info, name, ::fl::strprintf(__VA_ARGS__))
#define LOGW(name, ...) ::fl::log(::fl::Level::Warning, name, ::fl::strprintf(__VA_ARGS__))
#define LOGE(name, ...) ::fl::log(::fl::Level::Error, name, ::fl::strprintf(__VA_ARGS__))

}  // namespace fl
