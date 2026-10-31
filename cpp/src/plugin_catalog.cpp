// © 2026 FuchsLabs
#include "plugin_catalog.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

namespace fl::plugins::catalog {
namespace {
namespace fs = std::filesystem;
constexpr size_t sig_limit = 256;

std::string lowered(std::string s) { for (auto& c : s) if (c >= 'A' && c <= 'Z') c += 'a' - 'A'; return s; }
std::string trimmed(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}
bool read_file(const fs::path& p, size_t limit, std::string& out) {
    std::error_code ec;
    if (!fs::is_regular_file(p, ec) || fs::is_symlink(p, ec) || fs::file_size(p, ec) > limit) return false;
    std::ifstream in(p, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}
bool write_atomic(const fs::path& p, const std::string& data) {
    const auto tmp = fs::path(p).concat(".tmp");
    { std::ofstream out(tmp, std::ios::binary | std::ios::trunc); out << data; if (!out) return false; }
    std::error_code ec;
    fs::rename(tmp, p, ec);
    if (ec) { // older Windows runtimes do not replace an existing target
        fs::remove(p, ec);
        fs::rename(tmp, p, ec);
    }
    return !ec;
}
bool contains(const std::string& hay, const std::string& needle) { return lowered(hay).find(needle) != std::string::npos; }
bool matches(const sign::IndexEntry& e, const Filter& f) {
    if (!f.category.empty() && e.category != f.category) return false;
    if (f.text.empty()) return true;
    const auto q = lowered(f.text);
    return contains(e.id, q) || contains(e.name, q) || contains(e.description, q) || contains(e.author, q);
}
std::vector<std::string> hosts_of(const Manifest& m) {
    std::set<std::string> h;
    for (const auto& s : m.sources) h.insert(s.host);
    return {h.begin(), h.end()};
}
} // namespace

Catalog::Catalog(sign::PublicKey key, fs::path cache_dir, Fetcher fetch, std::string app_version)
    : key_(key), dir_(std::move(cache_dir)), fetch_(std::move(fetch)), app_version_(std::move(app_version)) { load_cache(); }

std::optional<Catalog> Catalog::from_pinned_key(fs::path cache_dir, Fetcher fetch, std::string app_version) {
    auto key = sign::public_key_from_hex(sign::store_public_key_hex);
    if (!key) return std::nullopt;
    return Catalog(*key, std::move(cache_dir), std::move(fetch), std::move(app_version));
}

// The cache only ever holds data that passed verification; it is verified again on every load, so a
// tampered cache file is simply ignored. Sequence numbers come from the cached, signed documents.
void Catalog::load_cache() {
    std::string index, isig, rev, rsig;
    if (read_file(dir_ / "index.json", sign::index_limit, index) && read_file(dir_ / "index.json.sig", sig_limit, isig)) {
        auto opened = sign::open_index(key_, index, trimmed(isig));
        if (opened) index_ = std::move(opened.index);
    }
    if (read_file(dir_ / "revoked.json", sign::index_limit, rev) && read_file(dir_ / "revoked.json.sig", sig_limit, rsig)) {
        auto opened = sign::open_revoked(key_, rev, trimmed(rsig));
        if (opened) revoked_ = std::move(opened.list);
    }
    // previews.cache = Signatur, Zeilenumbruch, Dokument: eine Datei, damit ein Abbruch nie ein halbes Paar hinterlässt
    std::string bundle;
    if (read_file(dir_ / "previews.cache", sign::index_limit + sig_limit, bundle)) {
        const auto nl = bundle.find('\n');
        if (nl != std::string::npos) {
            auto opened = sign::open_previews(key_, bundle.substr(nl + 1), trimmed(bundle.substr(0, nl)));
            if (opened) previews_ = std::move(opened.list);
        }
    }
    std::string tbundle;
    if (read_file(dir_ / "themes.cache", sign::index_limit + sig_limit, tbundle)) {
        const auto nl = tbundle.find('\n');
        if (nl != std::string::npos) {
            auto opened = sign::open_themes(key_, tbundle.substr(nl + 1), trimmed(tbundle.substr(0, nl)));
            if (opened) themes_ = std::move(opened.list);
        }
    }
    // An index without its verified revocation list could offer revoked packages: do not use the index.
    // A verified revocation list stays valid on its own (offline start must still switch plugins off).
    if (!revoked_) index_.reset();
}

bool Catalog::save_cache(const std::string& index, const std::string& isig, const std::string& rev, const std::string& rsig) {
    std::error_code ec;
    fs::create_directories(dir_, ec);
    return !ec && write_atomic(dir_ / "index.json", index) && write_atomic(dir_ / "index.json.sig", isig) &&
           write_atomic(dir_ / "revoked.json", rev) && write_atomic(dir_ / "revoked.json.sig", rsig);
}

RefreshResult Catalog::refresh() {
    auto fall_back = [&](const char* why) -> RefreshResult {
        if (index_) return {Availability::offline, true, why};
        return {Availability::offline, false, why};
    };
    auto index = fetch_("index.json", sign::index_limit);
    auto isig = fetch_("index.json.sig", sig_limit);
    auto rev = fetch_("revoked.json", sign::index_limit);
    auto rsig = fetch_("revoked.json.sig", sig_limit);
    if (!index || !isig || !rev || !rsig) return fall_back("Store not reachable.");
    *isig = trimmed(*isig); *rsig = trimmed(*rsig);
    auto opened = sign::open_index(key_, *index, *isig);
    if (!opened) return {Availability::invalid, index_.has_value(), opened.error}; // keep the accepted data untouched
    auto opened_rev = sign::open_revoked(key_, *rev, *rsig);
    if (!opened_rev) return {Availability::invalid, index_.has_value(), opened_rev.error};
    // Rollback protection against the last accepted (cached, verified) documents.
    if (index_ && !sign::sequence_acceptable(index_->sequence, opened.index->sequence))
        return {Availability::invalid, true, "Index is older than the last accepted one."};
    if (revoked_ && !sign::sequence_acceptable(revoked_->sequence, opened_rev.list->sequence))
        return {Availability::invalid, true, "Revocation list is older than the last accepted one."};
    index_ = std::move(opened.index);
    revoked_ = std::move(opened_rev.list);
    if (!save_cache(*index, *isig + "\n", *rev, *rsig + "\n")) return {Availability::available, false, "Cache could not be written."};
    refresh_previews();
    refresh_themes();
    return {Availability::available, false, {}};
}

// Best effort: previews are cosmetic. A missing, invalid or older document keeps whatever was accepted before.
void Catalog::refresh_previews() {
    auto doc = fetch_("previews.json", sign::index_limit);
    auto sig = fetch_("previews.json.sig", sig_limit);
    if (!doc || !sig) return;
    *sig = trimmed(*sig);
    auto opened = sign::open_previews(key_, *doc, *sig);
    if (!opened) return;
    if (previews_ && !sign::sequence_acceptable(previews_->sequence, opened.list->sequence)) return;
    previews_ = std::move(opened.list);
    std::error_code ec;
    fs::create_directories(dir_, ec);
    write_atomic(dir_ / "previews.cache", *sig + "\n" + *doc);
}

// Best effort as well: a missing, invalid or older themes.json keeps whatever was accepted before.
void Catalog::refresh_themes() {
    auto doc = fetch_("themes.json", sign::index_limit);
    auto sig = fetch_("themes.json.sig", sig_limit);
    if (!doc || !sig) return;
    *sig = trimmed(*sig);
    auto opened = sign::open_themes(key_, *doc, *sig);
    if (!opened) return;
    if (themes_ && !sign::sequence_acceptable(themes_->sequence, opened.list->sequence)) return;
    themes_ = std::move(opened.list);
    std::error_code ec;
    fs::create_directories(dir_, ec);
    write_atomic(dir_ / "themes.cache", *sig + "\n" + *doc);
}

std::vector<ThemeCard> Catalog::list_themes(const std::string& text, const std::set<std::string>& installed_ids) const {
    std::vector<ThemeCard> cards;
    if (!themes_ || !index_ || !revoked_) return cards;
    const auto q = lowered(text);
    for (const auto& e : themes_->items) {
        if (sign::is_revoked(*revoked_, e.id, e.version)) continue;
        if (!q.empty() && !contains(e.id, q) && !contains(e.name, q) && !contains(e.description, q) && !contains(e.author, q)) continue;
        cards.push_back({e, installed_ids.count(e.id) ? ThemeStatus::installed : ThemeStatus::not_installed});
    }
    std::sort(cards.begin(), cards.end(), [](const ThemeCard& a, const ThemeCard& b) { return lowered(a.entry.name) < lowered(b.entry.name); });
    return cards;
}

StoreResult Catalog::install_theme(const std::string& id, const ThemeImporter& importer) {
    if (!themes_ || !index_ || !revoked_) return {false, "Theme list is not available."};
    auto it = std::find_if(themes_->items.begin(), themes_->items.end(), [&](const sign::ThemeEntry& e) { return e.id == id; });
    if (it == themes_->items.end()) return {false, "Theme is not in the store."};
    if (sign::is_revoked(*revoked_, it->id, it->version)) return {false, "Theme has been revoked."};
    if (!importer) return {false, "No importer."};
    auto bytes = fetch_(it->file, sign::theme_limit);
    if (!bytes) return {false, "Theme download failed."};
    if (!sign::theme_matches(*it, *bytes)) return {false, "Theme does not match the signed list."};
    return importer(*bytes);
}

bool Catalog::has_preview(const std::string& id) const {
    return previews_ && std::any_of(previews_->items.begin(), previews_->items.end(), [&](const sign::PreviewEntry& e) { return e.id == id; });
}

std::optional<std::string> Catalog::preview(const std::string& id) {
    if (!index_ || !previews_) return std::nullopt;
    if (std::none_of(index_->plugins.begin(), index_->plugins.end(), [&](const sign::IndexEntry& e) { return e.id == id; })) return std::nullopt;
    auto it = std::find_if(previews_->items.begin(), previews_->items.end(), [&](const sign::PreviewEntry& e) { return e.id == id; });
    if (it == previews_->items.end()) return std::nullopt;
    // id and file name are validated by open_previews (plugins/<id>/preview.png); the cache name uses the verified hash.
    const auto cached = dir_ / ("preview-" + it->sha256 + ".png");
    std::string bytes;
    if (read_file(cached, sign::preview_limit, bytes) && sign::preview_matches(*it, bytes)) return bytes;
    auto fetched = fetch_(it->file, sign::preview_limit);
    if (!fetched || !sign::preview_matches(*it, *fetched)) return std::nullopt;
    std::error_code ec;
    fs::create_directories(dir_, ec);
    write_atomic(cached, *fetched);
    return fetched;
}

std::vector<Card> Catalog::list(const Filter& filter, const std::vector<Installed>& installed) const {
    std::vector<Card> cards;
    if (!index_) return cards;
    for (const auto& e : index_->plugins) {
        if (!matches(e, filter)) continue;
        Card card{e, Status::not_installed, {}, {}};
        const bool revoked = revoked_ && sign::is_revoked(*revoked_, e.id, e.version);
        auto it = std::find_if(installed.begin(), installed.end(), [&](const Installed& i) { return i.manifest.id == e.id; });
        if (it != installed.end()) {
            card.installed_version = it->manifest.version;
            card.status = it->manifest.version == e.version ? Status::installed : Status::update_available;
            if (card.status == Status::update_available) {
                const auto old_hosts = hosts_of(it->manifest);
                for (const auto& h : e.hosts)
                    if (std::find(old_hosts.begin(), old_hosts.end(), h) == old_hosts.end()) card.new_hosts.push_back(h);
            }
        }
        if (revoked) { if (!filter.show_revoked) continue; card.status = Status::revoked; }
        else if (card.status != Status::installed) {
            Manifest probe; probe.min_app_version = e.min_app_version;
            if (!app_compatible(probe, app_version_)) card.status = Status::unsupported;
        }
        cards.push_back(std::move(card));
    }
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) { return lowered(a.entry.name) < lowered(b.entry.name); });
    return cards;
}

StoreResult Catalog::install(Store& store, const std::string& id, const Importer& importer) {
    if (!index_) return {false, "Store index is not available."};
    auto it = std::find_if(index_->plugins.begin(), index_->plugins.end(), [&](const sign::IndexEntry& e) { return e.id == id; });
    if (it == index_->plugins.end()) return {false, "Plugin is not in the store index."};
    if (revoked_ && sign::is_revoked(*revoked_, it->id, it->version)) return {false, "Plugin has been revoked."};
    Manifest probe; probe.min_app_version = it->min_app_version;
    if (!app_compatible(probe, app_version_)) return {false, "Plugin needs a newer ScreenBuddy."};
    auto bytes = fetch_(it->file, manifest_limit);
    if (!bytes) return {false, "Plugin download failed."};
    if (!sign::package_matches(*it, *bytes)) return {false, "Plugin does not match the signed index."};
    return importer ? importer(*bytes, true) : store.import_manifest(*bytes, true); // disabled / no consent until the user decides
}

std::vector<std::string> Catalog::enforce_revocations(Store& store) const {
    std::vector<std::string> disabled;
    if (!revoked_) return disabled;
    for (const auto& i : store.entries())
        if (i.enabled && sign::is_revoked(*revoked_, i.manifest.id, i.manifest.version) && store.set_enabled(i.manifest.id, false))
            disabled.push_back(i.manifest.id);
    return disabled;
}
} // namespace fl::plugins::catalog
