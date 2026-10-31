// © 2026 FuchsLabs
// Farbdesigns: FuchsLabs-CI, "An Windows anpassen" und eigene Themes (JSON, versioniert, strikt geprüft).
// Reine Logik ohne Zeichnen; nur read_windows_settings() fragt Windows ab.
#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "gfx.h"

namespace fl::theme {

inline constexpr int FORMAT_VERSION = 1;
inline constexpr size_t MAX_FILE_BYTES = 8192;   // größere Dateien werden nie gelesen
inline constexpr size_t MAX_THEMES = 64;
inline constexpr size_t MAX_NAME = 40;           // Zeichen (UTF-8-Bytes ≤ 4 × MAX_NAME)
inline constexpr const char* WINDOWS_ID = "windows";
inline constexpr const char* CUSTOM_PREFIX = "custom:";

// Die neun Farben eines Themes (Reihenfolge = Reihenfolge im JSON und im Editor)
inline constexpr const char* COLOR_KEYS[9] = {"bg", "card", "line", "header", "accent", "text", "muted", "success", "danger"};

struct Palette {
    Color bg, card, line, header, accent, text, muted, success, danger;
    Color* slot(const std::string& key);
    const Color* slot(const std::string& key) const;
    bool operator==(const Palette& o) const;
};

struct Custom {
    std::string id;     // Dateiname ohne .json (nur [a-z0-9-])
    std::string name;   // Anzeigename
    std::string base;   // "hell" | "dunkel": woher Zusatzfarben (Warnung/Status) kommen
    bool dark = false;
    Palette colors{};
};

// FuchsLabs-CI
Palette builtin(bool dark);

// ---------------------------------------------------------------- Windows
struct WindowsSettings {
    bool light = true;              // AppsUseLightTheme
    bool has_accent = false;
    Color accent{0, 120, 212};
    bool high_contrast = false;     // Kontrastdesign aktiv
    Color hc_bg{0, 0, 0}, hc_text{255, 255, 255}, hc_accent{255, 255, 0}, hc_muted{192, 192, 192};
};
WindowsSettings read_windows_settings();   // Registry + SystemParametersInfo; Fallback: hell, kein Akzent
// Hell/Dunkel plus Akzentfarbe von Windows. Kopfzeile bleibt Fuchs-Orange (nur accent wird ersetzt);
// Kontrastdesign nutzt die Systemfarben. Das Ergebnis ist bereits kontrastkorrigiert.
Palette windows_palette(const WindowsSettings& w);

// ---------------------------------------------------------------- Kontrast
inline constexpr double MIN_TEXT = 4.5;   // Text auf Hintergrund und Karten
inline constexpr double MIN_BAR = 3.0;    // Balken/Akzent als Fläche
struct Warning {
    std::string a, b;     // Farbschlüssel, z. B. "text", "bg"
    double ratio, needed;
};
std::vector<Warning> contrast_warnings(const Palette& p);
// Passt Farben minimal an (heller/dunkler), bis alle Mindestkontraste erreicht sind. Hintergrund bleibt unverändert.
Palette correct_contrast(const Palette& p, std::vector<std::string>* changed = nullptr);
double contrast(Color a, Color b);
Color readable_on(Color bg);            // Navy oder Weiß, je nach Kontrast

// ---------------------------------------------------------------- JSON
std::optional<Custom> parse(const ojson& j, std::string& err);   // strikt: genau diese Schlüssel, #RRGGBB, Version 1
ojson to_json(const Custom& t);
std::string slugify(const std::string& name);                    // Dateiname aus dem Namen, leer = unbrauchbar
bool valid_id(const std::string& id);
bool valid_name(const std::string& name);

// Migration der alten Schlüssel color_base/color_* (theme = "eigen") in ein Theme
std::optional<Custom> migrate_legacy(const ojson& display, const std::string& name);

// ---------------------------------------------------------------- Ablage
std::filesystem::path themes_dir();                              // %LOCALAPPDATA%\FuchsLabs\USB-Display\themes
class Store {
public:
    explicit Store(std::filesystem::path dir = themes_dir());
    std::vector<Custom> list(std::vector<std::string>* problems = nullptr) const;
    std::optional<Custom> load(const std::string& id, std::string* err = nullptr) const;
    bool save(Custom& t, std::string& err) const;                 // legt id aus dem Namen an, wenn leer; schreibt atomar
    bool remove(const std::string& id, std::string& err) const;
    std::optional<Custom> duplicate(const std::string& id, std::string& err) const;
    bool rename(const std::string& id, const std::string& new_name, std::string& err) const;
    std::optional<Custom> import_file(const std::filesystem::path& file, std::string& err) const;
    bool export_file(const std::string& id, const std::filesystem::path& file, std::string& err) const;
    // Neu importieren/aus Plugin-Store: Text statt Datei
    std::optional<Custom> import_text(const std::string& text, std::string& err) const;
    const std::filesystem::path& dir() const { return dir_; }
private:
    std::string unique_id(const std::string& base) const;
    std::filesystem::path dir_;
};

// Alte Einstellung theme="eigen" samt color_base/color_* wird ein gespeichertes Theme ("Eigene Farben"); true = cfg geändert
bool migrate_config(ojson& cfg, const Store& store);

// "hell" | "dunkel" | "windows" | "custom:<id>" → Palette (+ dark-Kennzeichen); unbekanntes Theme → FuchsLabs hell
struct Resolved {
    Palette colors{};
    Color bar{};            // Balken/Schalter: bei Windows die korrigierte Akzentfarbe (≥ 3:1), sonst die Kopfzeilenfarbe
    bool dark = false;
    std::string base = "hell";
    bool windows = false;
    bool found = true;
};
Resolved resolve(const std::string& theme_key, const Store& store, const WindowsSettings* ws = nullptr);
Resolved resolve_custom(const Custom& t);
bool valid_theme_key(const std::string& key);

}  // namespace fl::theme
