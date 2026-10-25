// © 2026 FuchsLabs
// Datenquelle Blog-Updates (RSS).
#include "ui_pages.h"

namespace fl::ui {
namespace {

class RssPage : public Page {
public:
    PageInfo info() const override {
        return {"rss", Group::Sources, T(L"Blogs (RSS)", L"Blogs (RSS)"),
                T(L"Neueste Beiträge deiner Lieblingsblogs", L"Latest posts of your favourite blogs")};
    }
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
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
private:
    Form form_;
};

}  // namespace
std::unique_ptr<Page> make_rss_page() { return std::make_unique<RssPage>(); }
}  // namespace fl::ui
