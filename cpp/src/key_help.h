// © 2026 FuchsLabs
// Hilfe zum Beschaffen von API-Keys und Tokens: offizielle Links und kurze Schritte (DE/EN) je Dienst.
// Reine Daten und Prüfungen, ohne Fenster und ohne Netz – unter Linux testbar (tests/key_help_tests.cpp).
// Enthält nie Schlüssel; die App öffnet nur Adressen aus dieser Tabelle.
#pragma once

#include <string>
#include <vector>

namespace fl::keyhelp {

struct Link {
    std::string url;          // https, Host aus der Erlaubnisliste
    std::wstring de, en;      // Beschriftung des Knopfs
};

struct Help {
    std::string id;                    // discord | steam | xbox | tmdb | spotify | homeassistant
    std::wstring title_de, title_en;   // Überschrift der Hilfe-Karte
    std::vector<std::wstring> steps_de, steps_en;
    std::vector<Link> links;           // erster Link = Hauptziel
};

const std::vector<Help>& all();
const Help* find(const std::string& id);   // nullptr, wenn unbekannt

// true nur für https-Adressen ohne Zugangsdaten/Port, deren Host exakt in der Erlaubnisliste steht.
bool is_official_url(const std::string& url);

}  // namespace fl::keyhelp
