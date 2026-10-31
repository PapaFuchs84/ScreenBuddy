// © 2026 FuchsLabs
// Datenquelle Blog-Updates (RSS).
#include "ui_pages.h"

#include "app.h"
#include "netguard.h"
#include "rss.h"
#include "util.h"

namespace fl::ui {
namespace {

class RssPage : public Page {
public:
    PageInfo info() const override {
        return {"rss", Group::Sources, T(L"Blogs (RSS)", L"Blogs (RSS)"),
                T(L"Neueste Beiträge deiner Lieblingsblogs", L"Latest posts of your favourite blogs")};
    }
    bool shows_status() const override { return true; }
    bool active(const ojson& cfg) const override { return flag(cfg, "rss", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "rss", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "rss", "title", T(L"Titel", L"Title"));
        form_.number(*page, "rss", "refresh_minutes", T(L"Aktualisieren", L"Refresh"), T(L"10–720 Minuten", L"10–720 minutes"), 10, 720);
        form_.slider(*page, "rss", "max_items", T(L"Einträge insgesamt", L"Total items"), T(L"1–4", L"1–4"), 1, 4);
        form_.slider(*page, "rss", "max_per_feed", T(L"Einträge je Feed", L"Items per feed"), T(L"1–4", L"1–4"), 1, 4);
        Card* feeds = root.add<Card>(T(L"Blogs", L"Blogs"));
        form_.list(*feeds, "rss", "feeds", T(L"Feed-Adressen", L"Feed URLs"), T(L"Eine pro Zeile", L"One per line"), 5);
        Card* check = root.add<Card>(T(L"Feed prüfen", L"Check a feed"),
                                     T(L"Zeigt, ob unter der Adresse ein Feed gefunden wird (Netzabfrage, ohne zu speichern).",
                                       L"Shows whether a feed is found at the address (network request, nothing is saved)."));
        url_ = check->row<TextField>(T(L"Adresse", L"URL"), Text(), TextField::Single, L"");
        url_->track_dirty = false;
        url_->width = 340;
        url_->limit = 300;
        url_->hint = Text(L"https://beispiel.de/blog");
        result_ = check->row<Label>(T(L"Ergebnis", L"Result"), Text(), Text(L"–"), Font::Small, Label::Muted);
        result_->width = 340;
        button_ = check->add<ButtonRow>()->add(T(L"Prüfen", L"Check"), Button::Secondary, [this, h = &host] { run_check(*h); });
    }
    void tick(Host& host, const Status&) override { if (button_) button_->enabled = !host.busy(); }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
    TextField* url_ = nullptr;
    Label* result_ = nullptr;
    Button* button_ = nullptr;
    void run_check(Host& host) {
        const std::string url = trim(narrow(url_->text()));
        if (url.empty()) {
            result_->text = T(L"Bitte eine Adresse eintragen.", L"Please enter an address.");
            result_->tone = Label::Danger;
            return;
        }
        result_->text = T(L"Prüfe …", L"Checking …");
        result_->tone = Label::Muted;
        Host* h = &host;
        h->run([this, url]() -> std::function<void()> {
            Text message;
            Label::Tone tone = Label::Danger;
            // Nur öffentliche Adressen (kein Heimnetz/localhost), wie bei der bisherigen Prüfung
            if (!netguard::public_url(url.find("://") != std::string::npos ? url : "https://" + url))
                message = T(L"Ungültige oder unsichere Adresse.", L"Invalid or unsafe address.");
            else {
                rss::RssFeeds feeds(ojson{{"feeds", ojson::array()}});
                auto posts = feeds.feed(url);
                if (!posts) message = T(L"Kein Feed gefunden.", L"No feed found.");
                else {
                    auto sel = feeds.select(*posts);
                    std::wstring latest = sel.empty() ? L"" : widen(sel[0].source + " – " + sel[0].title);
                    message = Text(L"Feed gefunden. Neuester Beitrag: " + (latest.empty() ? L"–" : latest),
                                   L"Feed found. Latest post: " + (latest.empty() ? L"–" : latest));
                    tone = Label::Success;
                }
            }
            return [this, message, tone] { result_->text = message; result_->tone = tone; };
        });
    }
};

}  // namespace
std::unique_ptr<Page> make_rss_page() { return std::make_unique<RssPage>(); }
}  // namespace fl::ui
