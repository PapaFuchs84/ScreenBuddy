// © 2026 FuchsLabs
#include "ui_pages.h"

#include <algorithm>

#include "util.h"

namespace fl::ui {

std::vector<std::unique_ptr<Page>> create_pages() {
    std::vector<std::unique_ptr<Page>> pages;
    // Allgemein
    pages.push_back(make_overview_page());
    pages.push_back(make_display_page());
    pages.push_back(make_pages_page());
    // Datenquellen
    pages.push_back(make_system_page());
    pages.push_back(make_friends_page());
    pages.push_back(make_discord_page());
    pages.push_back(make_bf6_page());
    pages.push_back(make_rss_page());
    pages.push_back(make_homeassistant_page());
    // Verbindung
    pages.push_back(make_companion_page());
    pages.push_back(make_plugins_page());
    pages.push_back(make_store_page());
    // App
    pages.push_back(make_design_page());
    pages.push_back(make_app_page());
    return pages;
}

Text source_text(const std::string& key) { return Text(source_title(key, false), source_title(key, true)); }

std::vector<ItemList::Item> page_items(const ojson& order, const ojson& selected, const std::vector<PluginPageInfo>& plugins) {
    std::vector<ItemList::Item> items;
    auto has = [&](const std::string& k) { return std::any_of(items.begin(), items.end(), [&](auto& i) { return i.key == k; }); };
    auto chosen = [&](const std::string& k) {
        return selected.is_array() && std::any_of(selected.begin(), selected.end(), [&](const ojson& s) { return s.is_string() && s.get<std::string>() == k; });
    };
    if (order.is_array())
        for (const auto& key : order) {
            if (!key.is_string()) continue;
            std::string k = key.get<std::string>();
            if (has(k)) continue;
            if (std::find(config::PAGES.begin(), config::PAGES.end(), k) != config::PAGES.end()) {
                items.push_back({k, source_text(k), {}, chosen(k)});
                continue;
            }
            auto plugin = std::find_if(plugins.begin(), plugins.end(), [&](auto& p) { return p.first == k; });
            if (plugin != plugins.end()) items.push_back({k, plugin->second, T(L"Plugin", L"Plugin"), chosen(k)});
        }
    for (const auto& k : config::PAGES)
        if (!has(k)) items.push_back({k, source_text(k), {}, false});
    // Plugins, die noch nicht in der Reihenfolge stehen, sind wie zur Laufzeit zunächst sichtbar (hinten).
    for (const auto& p : plugins)
        if (!has(p.first)) items.push_back({p.first, p.second, T(L"Plugin", L"Plugin"), true});
    return items;
}

Text connection_text(const ojson& c, Label::Tone& tone) {
    const std::string state = c.is_object() ? c.value("state", "off") : "off";
    std::wstring detail = c.is_object() ? widen(c.value("detail", c.value("error", std::string()))) : L"";
    std::wstring name = c.is_object() ? widen(c.value("name", std::string())) : L"";
    std::wstring extra = name.empty() ? detail : (detail.empty() ? name : name + L" · " + detail);
    auto with = [&](const wchar_t* de, const wchar_t* en) {
        return extra.empty() ? T(de, en) : Text(std::wstring(de) + L" – " + extra, std::wstring(en) + L" – " + extra);
    };
    if (state == "ok") { tone = Label::Success; return with(L"Verbunden", L"Connected"); }
    if (state == "bad") { tone = Label::Danger; return with(L"Fehler", L"Error"); }
    if (state == "idle") { tone = Label::Muted; return with(L"Eingerichtet, gerade nicht aktiv", L"Configured, currently idle"); }
    tone = Label::Muted;
    return T(L"Nicht eingerichtet", L"Not configured");
}

bool flag(const ojson& cfg, const char* sec, const char* key) {
    return cfg.contains(sec) && cfg.at(sec).is_object() && cfg.at(sec).value(key, false);
}

Text join(const Text& a, const std::wstring& b) { return Text(a.de + b, a.en + b); }

}  // namespace fl::ui
