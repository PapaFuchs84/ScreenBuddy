// © 2026 FuchsLabs
// Reine Logik der nativen Einstellungsseiten (ohne Fenster, ohne Netz) – testbar in fl-tests.
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "gfx.h"

namespace fl::ui::logic {

// "abc", "#abc", "aabbcc", "#AABBCC" → "#AABBCC"; sonst nullopt. Leer ist keine Farbe (siehe Aufrufer).
std::optional<std::string> normalize_hex(const std::string& input);

// Nicht leere, getrimmte Zeilen (CR/LF/CRLF) und die Rückrichtung mit CRLF (EDIT-Felder).
std::vector<std::string> lines_of(const std::string& text);
std::string lines_join(const std::vector<std::string>& lines);

// Zahlen-IDs (Discord): alle Ziffernfolgen, ohne Doppelte, Reihenfolge bleibt. set_id fügt hinzu bzw. entfernt.
std::vector<std::string> id_list(const std::string& text);
std::string set_id(const std::string& text, const std::string& id, bool on);

// Verknüpfungen „Name|discord:ID|steam:ID|xbox:Name“. Beide Konten landen in derselben Zeile (vorhandene wird erweitert).
struct Account {
    std::string platform, id, name;
};
std::vector<std::string> link_accounts(std::vector<std::string> links, const Account& a, const Account& b);

// Home-Assistant-Entitäten „sensor.x“ oder „sensor.x|Name“, höchstens max Einträge.
// Rückgabe leer bei Erfolg, sonst Fehlerschlüssel: "invalid" | "full".
std::string add_entity(std::vector<std::string>& lines, const std::string& id, size_t max);
void remove_entity(std::vector<std::string>& lines, const std::string& id);
bool has_entity(const std::vector<std::string>& lines, const std::string& id);
std::string entity_id_of(const std::string& line);  // Teil vor „|“, kleingeschrieben

// Lesbarkeit der eigenen Farben (WCAG wie auf dem Display). Schlüssel: bg, card, text, muted, accent (#RRGGBB).
// Rückgabe: Kennungen der zu schwachen Paare: "text_bg", "text_card", "muted_bg", "accent_bg".
std::vector<std::string> color_warnings(const std::map<std::string, std::string>& colors);

// Gefilterte Reihenfolge für Auswahllisten: Treffer der Suche (Groß/Kleinschreibung egal), Angehakte zuerst, höchstens cap.
// names: Anzeigename je Eintrag; selected: angehakt je Eintrag. Rückgabe: Indizes.
std::vector<size_t> filter_indices(const std::vector<std::string>& names, const std::vector<bool>& selected,
                                   const std::string& query, size_t cap);

// QR-Code als Bild (schwarz auf weiß mit Ruhezone), module_px Pixel je Modul.
Image qr_image(const std::string& text, int module_px = 6);

}  // namespace fl::ui::logic
