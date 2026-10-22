// © 2026 FuchsLabs
// Neueste Beiträge aus RSS-/Atom-Feeds für die Blog-Seite. In der Konfiguration stehen Feed- oder Blog-Adressen;
// bei einer Blog-Adresse wird der Feed automatisch gesucht (<link rel="alternate"> bzw. /feed, /rss, /atom.xml).
#pragma once

#include <atomic>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::rss {

extern const char* UA_STRING;

struct Parsed {
    std::string name;
    std::vector<Post> posts;
};
std::optional<Parsed> parse_feed(const std::string& data, const std::string& fallback_name);
std::optional<std::time_t> parse_date(const std::string& text);
std::vector<std::string> discover(const std::string& page, const std::string& base);
std::string html_unescape(const std::string& s);

class RssFeeds {
public:
    explicit RssFeeds(const ojson& cfg);
    ~RssFeeds();
    void start(std::function<void()> on_update);
    void stop();

    std::optional<std::vector<Post>> feed(const std::string& entry);  // ein Eintrag (auch für die Prüfung)
    std::vector<Post> select(std::vector<Post> posts) const;
    std::string resolved(const std::string& entry);

    std::vector<Post> posts();
    std::optional<std::time_t> updated();

private:
    struct Got {
        std::string data, url;
    };
    std::optional<Got> get(std::string url);
    bool sleep_for(double seconds);

    std::vector<std::string> entries_;
    int refresh_, max_items_, max_per_feed_;
    std::vector<Post> posts_;
    std::optional<std::time_t> updated_;
    std::map<std::string, std::string> resolved_;   // Eintrag → gefundene Feed-URL
    std::map<std::string, std::vector<Post>> cache_;
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
};

}  // namespace fl::rss
