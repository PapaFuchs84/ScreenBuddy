// © 2026 FuchsLabs
// Konfiguration: Standardwerte, Laden (config.toml), Prüfen der Eingaben der Einstellungsseite und Schreiben.
// Intern als geordnetes JSON wie das dict der Version 1.x – so passt es 1:1 zur unveränderten Einstellungsseite.
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fl {

using ojson = nlohmann::ordered_json;

namespace config {

extern const std::vector<std::string> PAGES;
const ojson& defaults();

ojson load(const std::filesystem::path& path);         // Standardwerte + Datei, Geheimnisse entschlüsselt
bool token_is_plaintext(const std::filesystem::path& path);
ojson merge(const ojson& cfg, const ojson& incoming);  // wirft std::invalid_argument mit verständlicher Meldung
ojson public_view(const ojson& cfg);                   // ohne Geheimnisse, IDs als Text
void save(const std::filesystem::path& path, const ojson& cfg);
bool is_color_key(const std::string& key);             // color_bg … color_muted

}  // namespace config
}  // namespace fl
