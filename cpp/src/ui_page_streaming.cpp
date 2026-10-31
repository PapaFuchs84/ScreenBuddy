// © 2026 FuchsLabs
// Datenquelle Streaming-Neustarts (TMDB): Seite, Zugangsschlüssel, Region, Anbieter-Mehrfachauswahl, Anzahl Einträge.
// This product uses the TMDB API but is not endorsed or certified by TMDB. Anbieterdaten: JustWatch.
#include "ui_pages.h"

#include <algorithm>

#include "app.h"
#include "i18n.h"
#include "tmdb.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

class StreamingPage : public Page {
public:
    PageInfo info() const override {
        return {"streaming", Group::Sources, T(L"Streaming-Neustarts", L"New on streaming"),
                T(L"Neue Filme und Serien bei deinen Streaming-Anbietern", L"New movies and series on your streaming services")};
    }
    bool shows_status() const override { return true; }
    bool active(const ojson& cfg) const override { return flag(cfg, "streaming", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "streaming", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "streaming", "title", T(L"Titel", L"Title"));
        form_.slider(*page, "streaming", "max_items", T(L"Einträge", L"Entries"),
                     T(L"1–6 Zeilen; Neustarts der letzten 30 Tage, Daten höchstens stündlich aktualisiert",
                       L"1–6 rows; releases of the last 30 days, data refreshed at most hourly"), 1, tmdb::MAX_ITEMS);

        Card* acc = root.add<Card>(T(L"TMDB-Zugang", L"TMDB access"),
                                   T(L"Kostenloser Key von themoviedb.org (Einstellungen → API). Der Key wird DPAPI-verschlüsselt gespeichert.",
                                     L"Free key from themoviedb.org (Settings → API). The key is stored DPAPI-encrypted."));
        key_ = form_.secret(*acc, "streaming", "api_key", T(L"API-Key", L"API key"),
                            T(L"v3-Key oder Read Access Token; leer lassen = gespeicherten behalten",
                              L"v3 key or Read Access Token; leave empty to keep the saved one"));
        region_ = form_.text(*acc, "streaming", "region", T(L"Region", L"Region"),
                             T(L"Leer = automatisch (Deutsch: DE, Englisch: US); sonst zwei Buchstaben, z. B. AT",
                               L"Empty = automatic (German: DE, English: US); otherwise two letters, e.g. AT"), Text(L"DE"));
        region_->limit = 2;
        result_ = acc->row<Label>(T(L"Prüfung", L"Check"), Text(), Text(L"–"), Font::Small, Label::Muted);
        result_->width = 340;
        check_ = acc->add<ButtonRow>()->add(T(L"Key prüfen & Anbieter laden", L"Check key & load providers"),
                                            Button::Secondary, [this] { load(); });
        add_key_help(root, host, "tmdb");

        Card* prov = root.add<Card>(T(L"Anbieter", L"Providers"),
                                    T(L"Höchstens 30. Zeilen „ID|Name“ (TMDB-Provider-ID). Vorbelegt: Netflix, Prime Video, Disney+, Apple TV+, WOW, RTL+.",
                                      L"At most 30. Lines “ID|Name” (TMDB provider ID). Preset: Netflix, Prime Video, Disney+, Apple TV+, WOW, RTL+."));
        providers_ = form_.list(*prov, "streaming", "providers", T(L"Anbieter-IDs", L"Provider IDs"),
                                T(L"Eine pro Zeile, z. B. 8|Netflix", L"One per line, e.g. 8|Netflix"), 7);
        providers_->on_change = [this] { if (!syncing_) refill(); };

        Card* pick = root.add<Card>(T(L"Anbieter auswählen", L"Pick providers"),
                                    T(L"Zuerst oben „Key prüfen & Anbieter laden“. Haken setzen trägt den Anbieter oben ein.",
                                      L"First press “Check key & load providers” above. Checking an entry adds it above."));
        info_ = pick->row<Label>(T(L"Liste", L"List"), Text(), T(L"Noch nicht geladen.", L"Not loaded yet."), Font::Small, Label::Muted);
        search_ = pick->row<TextField>(T(L"Suche", L"Search"), Text(), TextField::Single, L"");
        search_->width = 300;
        search_->track_dirty = false;
        search_->hint = T(L"Anbieter suchen …", L"Search providers …");
        search_->name = T(L"Anbieter suchen", L"Search providers");
        search_->on_change = [this] { refill(); };
        list_ = pick->add<ItemList>(std::vector<ItemList::Item>{});
        list_->name = T(L"Anbieter", L"Providers");
        list_->reorder = false;
        list_->track_dirty = false;
        list_->empty_text = T(L"Nichts gefunden.", L"Nothing found.");
        list_->on_change = [this] { sync_from_list(); };
        refill(false);

        Card* credits = root.add<Card>(T(L"Nachweis", L"Credit"));
        credits->add<Note>(Text(L"This product uses the TMDB API but is not endorsed or certified by TMDB."));
        credits->add<Note>(T(L"Anbieterdaten: JustWatch", L"Provider data: JustWatch"));
        credits->add<Note>(T(L"Anbieter-Logos: Marken der jeweiligen Anbieter", L"Provider logos: trademarks of the respective providers"));
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status&) override { if (check_) check_->enabled = !host_->busy(); }

private:
    Form form_;
    Host* host_ = nullptr;
    Label *result_ = nullptr, *info_ = nullptr;
    TextField *key_ = nullptr, *region_ = nullptr, *providers_ = nullptr, *search_ = nullptr;
    ItemList* list_ = nullptr;
    Button* check_ = nullptr;
    std::vector<tmdb::Provider> loaded_;
    bool syncing_ = false;

    void load() {
        // Eingegebene Werte; leere ersetzt der Arbeits-Thread durch die gespeicherten
        std::string key = trim(narrow(key_->text())), region = trim(narrow(region_->text()));
        result_->text = T(L"Prüfe …", L"Checking …");
        result_->tone = Label::Muted;
        Host* h = host_;
        h->run([this, h, key, region]() mutable -> std::function<void()> {
            const ojson saved = h->app().cfg()["streaming"];
            auto str = [&](const char* k) { return saved.contains(k) && saved[k].is_string() ? trim(saved[k].get<std::string>()) : std::string(); };
            if (key.empty()) key = str("api_key");
            if (region.empty()) region = str("region");
            const std::string lang = i18n::lang();
            region = tmdb::region_for(lang, region);
            std::vector<tmdb::Provider> list;
            Text message;
            Label::Tone tone = Label::Danger;
            try {
                list = tmdb::list_providers(key, region, lang);
                message = Text(L"Verbunden: " + std::to_wstring(list.size()) + L" Anbieter in " + widen(region),
                               L"Connected: " + std::to_wstring(list.size()) + L" providers in " + widen(region));
                tone = Label::Success;
            } catch (const tmdb::TmdbError& e) {
                // Meldungen enthalten nie den Key; Deutsch ist die Quellsprache, Englisch kommt aus i18n
                const std::string de = e.what();
                message = Text(widen(de), widen(i18n::to_english(de)));
            }
            key.clear();
            const bool ok = tone == Label::Success;
            return [this, list = std::move(list), message, tone, ok]() mutable {
                result_->text = message;
                result_->tone = tone;
                if (ok) loaded_ = std::move(list);
                refill();
            };
        });
    }
    // Zeilen „ID|Name“ des Textfelds ⇄ Haken der Liste
    void refill(bool layout = true) {
        if (!list_ || !providers_ || !search_) return;
        const auto selected = tmdb::parse_selection(logic::lines_of(narrow(providers_->text())));
        auto is_selected = [&](int id) { return std::any_of(selected.begin(), selected.end(), [&](auto& s) { return s.id == id; }); };
        // Vor dem Laden zeigt die Liste die gewählten Anbieter; danach alle der Region
        std::vector<tmdb::Provider> shown = loaded_;
        for (const auto& s : selected)
            if (std::none_of(shown.begin(), shown.end(), [&](auto& p) { return p.id == s.id; })) shown.push_back({s.id, s.name.empty() ? "#" + std::to_string(s.id) : s.name});
        std::vector<std::string> names;
        std::vector<bool> picked;
        for (const auto& p : shown) {
            names.push_back(p.name + " " + std::to_string(p.id));
            picked.push_back(is_selected(p.id));
        }
        const auto order = logic::filter_indices(names, picked, narrow(search_->text()), 40);
        std::vector<ItemList::Item> items;
        for (size_t i : order) {
            const auto& p = shown[i];
            items.push_back({std::to_string(p.id), Text(widen(p.name)), Text(L"ID " + std::to_wstring(p.id)), picked[i]});
        }
        list_->items = items;
        list_->original = items;
        if (list_->selected >= int(items.size()) || (list_->selected < 0 && !items.empty())) list_->selected = items.empty() ? -1 : 0;
        if (loaded_.empty())
            info_->text = T(L"Noch nicht geladen – gezeigt werden die gewählten Anbieter.", L"Not loaded yet – showing the selected providers.");
        else {
            const std::wstring n = std::to_wstring(loaded_.size());
            const bool more = shown.size() > items.size();
            info_->text = Text(n + L" Anbieter" + (more ? L" – Suche nutzen" : L""), n + L" providers" + (more ? L" – use the search" : L""));
        }
        if (layout) host_->relayout();
    }
    void sync_from_list() {
        auto lines = logic::lines_of(narrow(providers_->text()));
        auto selected = tmdb::parse_selection(lines);
        auto name_of = [&](int id) {
            for (const auto& p : loaded_) if (p.id == id) return p.name;
            return std::string();
        };
        bool full = false;
        for (auto& item : list_->items) {
            const int id = std::stoi(item.key);
            auto it = std::find_if(selected.begin(), selected.end(), [&](auto& s) { return s.id == id; });
            if (item.enabled && it == selected.end()) {
                if ((int)selected.size() >= tmdb::MAX_PROVIDERS) { item.enabled = false; full = true; continue; }
                std::string name = name_of(id);
                if (name.empty()) name = narrow(item.title.get(false));
                selected.push_back({id, name});
            } else if (!item.enabled && it != selected.end()) {
                selected.erase(it);
            }
        }
        if (full) host_->toast(T(L"Es sind höchstens 30 Anbieter möglich.", L"At most 30 providers are possible."), ToastKind::Error);
        std::vector<std::string> out;
        for (const auto& s : selected) out.push_back(std::to_string(s.id) + (s.name.empty() ? "" : "|" + s.name));
        syncing_ = true;
        providers_->set_text(widen(logic::lines_join(out)));
        syncing_ = false;
    }
};

}  // namespace
std::unique_ptr<Page> make_streaming_page() { return std::make_unique<StreamingPage>(); }
}  // namespace fl::ui
