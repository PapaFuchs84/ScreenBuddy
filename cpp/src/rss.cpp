// © 2026 FuchsLabs
#include "rss.h"

#include <windows.h>

#include <algorithm>
#include <future>
#include <regex>
#include <unordered_map>

#include <pugixml.hpp>

#include "http.h"
#include "netguard.h"
#include "util.h"

namespace fl::rss {

const char* UA_STRING = "Mozilla/5.0 (ScreenBuddy RSS)";
static const char* ACCEPT = "application/rss+xml, application/atom+xml, application/xml;q=0.9, text/html;q=0.8, */*;q=0.5";
static const std::string ATOM = "{http://www.w3.org/2005/Atom}";
static const std::string RSS1 = "{http://purl.org/rss/1.0/}";
static const std::string DC = "{http://purl.org/dc/elements/1.1/}";
// Seiten, die ihren Feed nicht im HTML verlinken
static const std::map<std::string, std::string> KNOWN = {{"heise.de", "https://www.heise.de/rss/heise-atom.xml"}};

// ---------- Text ----------
std::string html_unescape(const std::string& s) {
    static const std::unordered_map<std::string, char32_t> named = {
        {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", 0xA0},
        {"auml", 0xE4}, {"ouml", 0xF6}, {"uuml", 0xFC}, {"Auml", 0xC4}, {"Ouml", 0xD6}, {"Uuml", 0xDC},
        {"szlig", 0xDF}, {"eacute", 0xE9}, {"egrave", 0xE8}, {"aacute", 0xE1}, {"agrave", 0xE0}, {"oacute", 0xF3},
        {"ccedil", 0xE7}, {"ntilde", 0xF1}, {"iacute", 0xED}, {"uacute", 0xFA}, {"Eacute", 0xC9},
        {"hellip", 0x2026}, {"ndash", 0x2013}, {"mdash", 0x2014}, {"lsquo", 0x2018}, {"rsquo", 0x2019},
        {"sbquo", 0x201A}, {"ldquo", 0x201C}, {"rdquo", 0x201D}, {"bdquo", 0x201E}, {"laquo", 0xAB},
        {"raquo", 0xBB}, {"lsaquo", 0x2039}, {"rsaquo", 0x203A}, {"euro", 0x20AC}, {"copy", 0xA9}, {"reg", 0xAE},
        {"trade", 0x2122}, {"middot", 0xB7}, {"bull", 0x2022}, {"deg", 0xB0}, {"times", 0xD7}, {"shy", 0xAD},
        {"pound", 0xA3}, {"sect", 0xA7}, {"para", 0xB6}, {"frac12", 0xBD}, {"thinsp", 0x2009}, {"ensp", 0x2002},
        {"emsp", 0x2003}, {"zwnj", 0x200C}, {"zwj", 0x200D}};
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') { out += s[i]; continue; }
        size_t semi = s.find(';', i + 1);
        if (semi == std::string::npos || semi - i > 12) { out += s[i]; continue; }
        std::string ent = s.substr(i + 1, semi - i - 1);
        char32_t cp = 0;
        if (!ent.empty() && ent[0] == '#') {
            try {
                cp = (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) ? (char32_t)std::stoul(ent.substr(2), nullptr, 16)
                                                                         : (char32_t)std::stoul(ent.substr(1));
            } catch (...) {
                cp = 0;
            }
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
        } else if (auto it = named.find(ent); it != named.end()) {
            cp = it->second;
        }
        if (!cp) { out += s[i]; continue; }
        out += from_u32(std::u32string(1, cp));
        i = semi;
    }
    return out;
}

static std::string clean(const std::string& text) {
    static const std::regex tags("<[^>]+>");
    return collapse_spaces(html_unescape(std::regex_replace(text, tags, "")));
}

// ---------- Datum ----------
static bool days_from_civil(int y, int m, int d, long long& out) {
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    out = era * 146097 + (long long)doe - 719468;
    return true;
}

static std::optional<std::time_t> make_time(int y, int mo, int d, int h, int mi, int s, int offset_min) {
    long long days;
    if (!days_from_civil(y, mo, d, days) || h > 24 || mi > 59 || s > 61) return std::nullopt;
    return (std::time_t)(days * 86400 + h * 3600 + mi * 60 + s - offset_min * 60);
}

std::optional<std::time_t> parse_date(const std::string& in) {
    std::string t = trim(in);
    if (t.empty()) return std::nullopt;
    // RFC 2822: "Wed, 02 Oct 2002 13:00:00 GMT" (Wochentag optional, Sekunden optional)
    static const std::regex rfc(
        R"(^(?:[A-Za-z]{3},?\s*)?(\d{1,2})\s+([A-Za-z]{3})[a-z]*\.?\s+(\d{2,4})\s+(\d{1,2}):(\d{2})(?::(\d{2}))?\s*([+-]\d{4}|[A-Za-z]{1,5})?\s*$)");
    std::smatch m;
    if (std::regex_match(t, m, rfc)) {
        static const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
        std::string mon = lower_ascii(m[2].str());
        int mo = 0;
        for (int i = 0; i < 12; ++i)
            if (mon == months[i]) mo = i + 1;
        if (!mo) return std::nullopt;
        int y = std::stoi(m[3].str());
        if (m[3].length() == 2) y += y < 50 ? 2000 : 1900;
        int off = 0;
        std::string tz = m[7].matched ? m[7].str() : "";
        if (!tz.empty() && (tz[0] == '+' || tz[0] == '-')) {
            int v = std::stoi(tz.substr(1));
            off = (v / 100) * 60 + v % 100;
            if (tz[0] == '-') off = -off;
        } else {
            static const std::map<std::string, int> zones = {{"ut", 0},     {"utc", 0},    {"gmt", 0},    {"z", 0},
                                                             {"est", -300}, {"edt", -240}, {"cst", -360}, {"cdt", -300},
                                                             {"mst", -420}, {"mdt", -360}, {"pst", -480}, {"pdt", -420},
                                                             {"cet", 60},   {"cest", 120}, {"mez", 60},   {"mesz", 120}};
            if (auto it = zones.find(lower_ascii(tz)); it != zones.end()) off = it->second;
        }
        return make_time(y, mo, std::stoi(m[1].str()), std::stoi(m[4].str()), std::stoi(m[5].str()),
                         m[6].matched ? std::stoi(m[6].str()) : 0, off);
    }
    // ISO 8601: "2024-01-31T10:00:00.123+02:00", "2024-01-31 10:00Z", "2024-01-31"
    static const std::regex iso(
        R"(^(\d{4})-(\d{2})-(\d{2})(?:[T ](\d{2}):(\d{2})(?::(\d{2})(?:[.,]\d+)?)?)?\s*(Z|[+-]\d{2}(?::?\d{2})?)?$)");
    if (std::regex_match(t, m, iso)) {
        int off = 0;
        if (m[7].matched && m[7].str() != "Z") {
            std::string z = m[7].str();
            std::string digits;
            for (char c : z)
                if (isdigit((unsigned char)c)) digits += c;
            int hh = std::stoi(digits.substr(0, 2)), mm = digits.size() >= 4 ? std::stoi(digits.substr(2, 2)) : 0;
            off = hh * 60 + mm;
            if (z[0] == '-') off = -off;
        }
        return make_time(std::stoi(m[1].str()), std::stoi(m[2].str()), std::stoi(m[3].str()),
                         m[4].matched ? std::stoi(m[4].str()) : 0, m[5].matched ? std::stoi(m[5].str()) : 0,
                         m[6].matched ? std::stoi(m[6].str()) : 0, off);
    }
    return std::nullopt;
}

// ---------- XML ----------
// Name eines Elements wie ElementTree: "{Namensraum}lokal" oder nur "lokal"
static std::string ns_uri(pugi::xml_node n, const std::string& prefix) {
    std::string attr = prefix.empty() ? "xmlns" : "xmlns:" + prefix;
    for (auto p = n; p; p = p.parent())
        if (auto a = p.attribute(attr.c_str())) return a.value();
    return "";
}

static std::string tag(pugi::xml_node n) {
    std::string name = n.name();
    auto colon = name.find(':');
    std::string prefix = colon == std::string::npos ? "" : name.substr(0, colon);
    std::string local = colon == std::string::npos ? name : name.substr(colon + 1);
    std::string uri = ns_uri(n, prefix);
    return uri.empty() ? local : "{" + uri + "}" + local;
}

static pugi::xml_node child(pugi::xml_node n, const std::string& t) {
    for (auto c : n.children())
        if (c.type() == pugi::node_element && tag(c) == t) return c;
    return {};
}

// Text vor dem ersten Kindelement (wie ElementTree .text, inklusive CDATA)
static std::string text_of(pugi::xml_node n) {
    std::string out;
    for (auto c : n.children()) {
        if (c.type() == pugi::node_pcdata || c.type() == pugi::node_cdata) out += c.value();
        else if (c.type() == pugi::node_element) break;
    }
    return out;
}

static std::string findtext(pugi::xml_node n, const std::string& t) {
    auto c = child(n, t);
    return c ? text_of(c) : "";
}

template <class F>
static void iter(pugi::xml_node n, const std::string& t, F f) {
    for (auto c : n.children()) {
        if (c.type() != pugi::node_element) continue;
        if (tag(c) == t) f(c);
        iter(c, t, f);
    }
}

// Feeds in Latin-1/Windows-1252 nach UTF-8 umwandeln (pugixml liest nur Unicode)
static std::string to_utf8(const std::string& data) {
    static const std::regex decl(R"xx(^\s*<\?xml[^>]*encoding\s*=\s*["']([A-Za-z0-9_\-]+)["'])xx", std::regex::icase);
    std::smatch m;
    std::string head = data.substr(0, 200);
    if (!std::regex_search(head, m, decl)) return data;
    std::string enc = lower_ascii(m[1].str());
    UINT cp = 0;
    if (enc == "iso-8859-1" || enc == "latin1" || enc == "latin-1" || enc == "windows-1252" || enc == "cp1252") cp = 1252;
    else if (enc == "iso-8859-15") cp = 28605;
    if (!cp) return data;
    int n = MultiByteToWideChar(cp, 0, data.data(), (int)data.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(cp, 0, data.data(), (int)data.size(), w.data(), n);
    std::string out = narrow(w);
    // Kodierungsangabe anpassen, sonst wäre die Deklaration falsch
    if (auto p = out.find(m[1].str()); p != std::string::npos && p < 200) out.replace(p, m[1].length(), "UTF-8");
    return out;
}

std::optional<Parsed> parse_feed(const std::string& raw, const std::string& fallback_name) {
    if (raw.find("<!ENTITY") != std::string::npos) return std::nullopt;  // schützt vor Entitäten-Bomben
    std::string data = to_utf8(raw);
    pugi::xml_document doc;
    if (!doc.load_buffer(data.data(), data.size(), pugi::parse_default, pugi::encoding_auto)) return std::nullopt;
    pugi::xml_node root = doc.document_element();
    if (!root) return std::nullopt;
    std::string rt = tag(root), lt = lower_ascii(rt);
    Parsed out;
    auto ends = [&](const char* s) { return lt.size() >= strlen(s) && lt.compare(lt.size() - strlen(s), std::string::npos, s) == 0; };
    if (ends("rss") || ends("rdf")) {
        bool rss = ends("rss");
        auto ch = child(root, "channel");
        std::string name = ch ? clean(findtext(ch, "title")) : "";
        iter(root, rss ? "item" : RSS1 + "item", [&](pugi::xml_node it) {
            std::string title = findtext(it, "title");
            if (title.empty()) title = findtext(it, RSS1 + "title");
            title = clean(title);
            std::string when = findtext(it, "pubDate");
            if (when.empty()) when = findtext(it, DC + "date");
            std::string link = trim(findtext(it, "link"));
            if (!title.empty()) out.posts.push_back({name.empty() ? fallback_name : name, title, parse_date(when), link});
        });
        out.name = name.empty() ? fallback_name : name;
    } else if (rt == ATOM + "feed") {
        std::string name = clean(findtext(root, ATOM + "title"));
        iter(root, ATOM + "entry", [&](pugi::xml_node e) {
            std::string title = clean(findtext(e, ATOM + "title"));
            std::string when = findtext(e, ATOM + "published");
            if (when.empty()) when = findtext(e, ATOM + "updated");
            auto le = child(e, ATOM + "link");
            if (!title.empty())
                out.posts.push_back({name.empty() ? fallback_name : name, title, parse_date(when), le ? le.attribute("href").value() : ""});
        });
        out.name = name.empty() ? fallback_name : name;
    } else {
        return std::nullopt;
    }
    return out;
}

std::vector<std::string> discover(const std::string& page, const std::string& base) {
    std::vector<std::string> links;
    static const std::regex alt_rx(R"(rel=["']?alternate)", std::regex::icase);
    static const std::regex type_rx(R"(application/(rss|atom)\+xml)", std::regex::icase);
    static const std::regex href_rx(R"x(href=["']([^"']+))x", std::regex::icase);
    // <link …>-Tags von Hand suchen (std::regex wäre auf großen Seiten langsam), Prüfung je Tag per Muster
    std::string low = lower_ascii(page);
    for (size_t pos = low.find("<link"); pos != std::string::npos; pos = low.find("<link", pos + 5)) {
        char next = pos + 5 < low.size() ? low[pos + 5] : '>';
        if (isalnum((unsigned char)next) || next == '_' || next == '-') continue;  // \b: z. B. <linkset> überspringen
        size_t end = low.find('>', pos);
        if (end == std::string::npos) break;
        if (end - pos > 2000) continue;
        std::string t = page.substr(pos, end - pos + 1);
        std::smatch h;
        if (std::regex_search(t, alt_rx) && std::regex_search(t, type_rx) && std::regex_search(t, h, href_rx))
            links.push_back(http::url_join(base, html_unescape(h[1].str())));
    }
    http::Url u = http::parse_url(base);
    std::string root = u.scheme + "://" + u.host;
    if ((u.scheme == "https" && u.port != 443) || (u.scheme == "http" && u.port != 80)) root += ":" + std::to_string(u.port);
    for (const char* p : {"/feed", "/rss", "/feed.xml", "/atom.xml", "/rss.xml", "/index.xml"}) links.push_back(root + p);
    return links;
}

// ---------- Dienst ----------
RssFeeds::RssFeeds(const ojson& cfg) {
    auto i = [&](const char* k, int def) { return cfg.contains(k) && cfg[k].is_number() ? cfg[k].get<int>() : def; };
    if (cfg.contains("feeds") && cfg["feeds"].is_array())
        for (auto& e : cfg["feeds"]) {
            std::string s = trim(e.is_string() ? e.get<std::string>() : e.dump());
            if (!s.empty()) entries_.push_back(s);
        }
    refresh_ = std::max(10, i("refresh_minutes", 30)) * 60;
    max_items_ = i("max_items", 4);
    max_per_feed_ = i("max_per_feed", 1);
}

RssFeeds::~RssFeeds() { stop(); }

void RssFeeds::stop() {
    stop_ = true;
    if (th_.joinable()) th_.join();
}

bool RssFeeds::sleep_for(double seconds) {
    double end = mono() + seconds;
    while (mono() < end) {
        if (stop_) return false;
        Sleep(100);
    }
    return !stop_;
}

std::vector<Post> RssFeeds::posts() {
    std::lock_guard lk(mu_);
    return posts_;
}

std::optional<std::time_t> RssFeeds::updated() {
    std::lock_guard lk(mu_);
    return updated_;
}

std::string RssFeeds::resolved(const std::string& entry) {
    std::lock_guard lk(mu_);
    auto it = resolved_.find(entry);
    return it == resolved_.end() ? entry : it->second;
}

void RssFeeds::start(std::function<void()> on_update) {
    th_ = std::thread([this, on_update] {
        while (!stop_) {
            double t = mono();
            std::vector<std::future<std::optional<std::vector<Post>>>> jobs;
            for (auto& e : entries_) jobs.push_back(std::async(std::launch::async, [this, e] { return feed(e); }));
            std::vector<std::string> failed;
            std::vector<Post> all;
            {
                std::vector<std::optional<std::vector<Post>>> results;
                for (auto& j : jobs) results.push_back(j.get());
                std::lock_guard lk(mu_);
                for (size_t i = 0; i < entries_.size(); ++i) {
                    if (results[i]) cache_[entries_[i]] = *results[i];
                    else failed.push_back(entries_[i]);
                }
                for (auto& e : entries_)
                    if (auto it = cache_.find(e); it != cache_.end()) all.insert(all.end(), it->second.begin(), it->second.end());
            }
            auto sel = select(all);
            {
                std::lock_guard lk(mu_);
                posts_ = sel;
                updated_ = std::time(nullptr);
            }
            LOGI("rss", "RSS: %d/%d Feeds, %d Beiträge (%.1f s)", (int)(entries_.size() - failed.size()), (int)entries_.size(),
                 (int)sel.size(), mono() - t);
            if (!failed.empty()) LOGW("rss", "RSS: kein Feed gefunden für %s", join(failed, ", ").c_str());
            on_update();
            if (!sleep_for(refresh_)) return;
        }
    });
}

std::vector<Post> RssFeeds::select(std::vector<Post> posts) const {
    // Neueste zuerst, höchstens max_per_feed je Quelle, damit ein fleißiger Blog nicht alles füllt
    std::stable_sort(posts.begin(), posts.end(),
                     [](const Post& a, const Post& b) { return a.published.value_or(0) > b.published.value_or(0); });
    std::vector<Post> out;
    std::map<std::string, int> per;
    for (auto& p : posts)
        if (per[p.source] < max_per_feed_) {
            per[p.source]++;
            out.push_back(p);
        }
    if ((int)out.size() > max_items_) out.resize(std::max(0, max_items_));
    return out;
}

std::optional<RssFeeds::Got> RssFeeds::get(std::string url) {
    for (int i = 0; i < 6; ++i) {  // Weiterleitungen selbst folgen, damit jedes Ziel geprüft wird
        if (stop_) return std::nullopt;
        if (!netguard::public_url(url)) {
            LOGW("rss", "RSS: Adresse nicht öffentlich, übersprungen: %s", url.c_str());
            return std::nullopt;
        }
        http::Request r;
        r.url = url;
        r.headers = {{"User-Agent", UA_STRING}, {"Accept", ACCEPT}};
        r.timeout_ms = 25000;
        r.max_bytes = 3'000'000;  // Schutz vor riesigen Seiten
        r.follow_redirects = false;
        auto resp = http::fetch(r);
        if ((resp.status == 301 || resp.status == 302 || resp.status == 303 || resp.status == 307 || resp.status == 308) &&
            !resp.location.empty()) {
            url = http::url_join(resp.final_url, resp.location);
            continue;
        }
        if (resp.status != 200) {
            if (!resp.error.empty()) LOGD("rss", "RSS %s → %s", url.c_str(), resp.error.c_str());
            return std::nullopt;
        }
        return Got{std::move(resp.body), resp.final_url};
    }
    return std::nullopt;
}

std::optional<std::vector<Post>> RssFeeds::feed(const std::string& entry) {
    std::string url = entry.find("://") != std::string::npos ? entry : "https://" + entry;
    http::Url u = http::parse_url(url);
    if (!u.valid) return std::nullopt;
    // Name wie netloc in Version 1.x (mit Port, falls angegeben), ohne "www."
    std::string name = u.host;
    {
        auto se = url.find("://");
        std::string rest = url.substr(se + 3);
        std::string netloc = rest.substr(0, rest.find_first_of("/?#"));
        if (auto at = netloc.rfind('@'); at != std::string::npos) netloc = netloc.substr(at + 1);
        name = lower_ascii(netloc);
    }
    if (name.rfind("www.", 0) == 0) name = name.substr(4);
    std::string path = u.path.substr(0, u.path.find('?'));
    {
        std::lock_guard lk(mu_);
        if (KNOWN.count(name) && (path.empty() || path == "/") && !resolved_.count(entry)) resolved_[entry] = KNOWN.at(name);
    }
    std::string res;
    {
        std::lock_guard lk(mu_);
        if (auto it = resolved_.find(entry); it != resolved_.end()) res = it->second;
    }
    if (!res.empty()) {
        auto got = get(res);
        auto parsed = got ? parse_feed(got->data, name) : std::nullopt;
        if (!parsed) return std::nullopt;
        return parsed->posts;
    }
    auto got = get(url);
    if (!got) return std::nullopt;
    if (auto parsed = parse_feed(got->data, name)) {
        std::lock_guard lk(mu_);
        resolved_[entry] = url;
        return parsed->posts;
    }
    for (auto& cand : discover(got->data, got->url)) {
        auto g = get(cand);
        auto parsed = g ? parse_feed(g->data, name) : std::nullopt;
        if (parsed) {
            LOGI("rss", "RSS: Feed für %s gefunden: %s", entry.c_str(), cand.c_str());
            std::lock_guard lk(mu_);
            resolved_[entry] = cand;
            return parsed->posts;
        }
    }
    return std::nullopt;
}

}  // namespace fl::rss
