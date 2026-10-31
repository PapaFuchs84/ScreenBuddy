// © 2026 FuchsLabs
// Seiten des nativen Einstellungsfensters. Neue Seite: ui_page_<name>.cpp mit make_<name>_page() anlegen und in
// create_pages() (ui_pages.cpp) eintragen – Schnittstelle siehe Kopfkommentar in ui_kit.h.
#pragma once

#include "ui_kit.h"

namespace fl::ui {

std::vector<std::unique_ptr<Page>> create_pages();  // Reihenfolge innerhalb einer Gruppe = Reihenfolge hier

std::unique_ptr<Page> make_overview_page();
std::unique_ptr<Page> make_display_page();
std::unique_ptr<Page> make_pages_page();
std::unique_ptr<Page> make_system_page();
std::unique_ptr<Page> make_friends_page();
std::unique_ptr<Page> make_discord_page();
std::unique_ptr<Page> make_bf6_page();
std::unique_ptr<Page> make_rss_page();
std::unique_ptr<Page> make_streaming_page();
std::unique_ptr<Page> make_homeassistant_page();
std::unique_ptr<Page> make_spotify_page();
std::unique_ptr<Page> make_companion_page();
std::unique_ptr<Page> make_plugins_page();
std::unique_ptr<Page> make_store_page();
std::unique_ptr<Page> make_design_page();
std::unique_ptr<Page> make_app_page();

// Gemeinsame Helfer
Text source_text(const std::string& key);                      // Seitentitel DE/EN (system, friends, …)
using PluginPageInfo = std::pair<std::string, Text>;                                        // ("plugin:<id>", Name)
std::vector<ItemList::Item> page_items(const ojson& order, const ojson& selected,
                                       const std::vector<PluginPageInfo>& plugins = {});  // Liste aller Seiten
Text connection_text(const ojson& connection, Label::Tone& tone);  // ok/bad/idle/off aus App::connections()
bool flag(const ojson& cfg, const char* sec, const char* key);  // bool aus der öffentlichen Sicht
Text join(const Text& a, const std::wstring& b);                // Text + gemeinsamer Zusatz

}  // namespace fl::ui
