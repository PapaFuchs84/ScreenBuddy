// © 2026 FuchsLabs
#include "plugin_sign.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <memory>
#include <set>

namespace fl::plugins::sign {
namespace {
using Pkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Bio = std::unique_ptr<BIO, decltype(&BIO_free)>;
using MdCtx = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

constexpr std::string_view b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr size_t signature_bytes = 64;

std::string prefix(Kind kind) {
    switch (kind) {
        case Kind::index: return "FuchsLabs-ScreenBuddy-Plugin-Index-v1\n";
        case Kind::revoked: return "FuchsLabs-ScreenBuddy-Plugin-Revoked-v1\n";
        case Kind::previews: return "FuchsLabs-ScreenBuddy-Plugin-Previews-v1\n";
        case Kind::themes: return "FuchsLabs-ScreenBuddy-Theme-List-v1\n";
    }
    return "";
}
std::string base64_encode(const unsigned char* d, size_t n) {
    std::string out;
    for (size_t i = 0; i < n; i += 3) {
        unsigned v = d[i] << 16 | (i + 1 < n ? d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        out += b64[v >> 18 & 63]; out += b64[v >> 12 & 63];
        out += i + 1 < n ? b64[v >> 6 & 63] : '='; out += i + 2 < n ? b64[v & 63] : '=';
    }
    return out;
}
// Strict: canonical padding only, no whitespace.
std::optional<std::vector<unsigned char>> base64_decode(std::string_view s) {
    if (s.size() % 4 != 0 || s.empty()) return std::nullopt;
    std::vector<unsigned char> out;
    for (size_t i = 0; i < s.size(); i += 4) {
        unsigned v = 0; int pad = 0;
        for (size_t k = 0; k < 4; ++k) {
            const char c = s[i + k];
            if (c == '=') { if (i + 4 != s.size() || k < 2) return std::nullopt; ++pad; v <<= 6; continue; }
            if (pad) return std::nullopt;
            const auto pos = b64.find(c);
            if (pos == b64.npos) return std::nullopt;
            v = v << 6 | static_cast<unsigned>(pos);
        }
        out.push_back(v >> 16 & 255);
        if (pad < 2) out.push_back(v >> 8 & 255);
        if (pad < 1) out.push_back(v & 255);
    }
    return out;
}
bool hex_digits(std::string_view s, size_t n) {
    return s.size() == n && std::all_of(s.begin(), s.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
bool plain_text(const std::string& s, size_t max) {
    return !s.empty() && s.size() <= max && std::none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
bool version_text(const std::string& s) {
    size_t parts = 1, digits = 0;
    for (char c : s) {
        if (c == '.') { if (!digits) return false; ++parts; digits = 0; }
        else if (c >= '0' && c <= '9') { if (++digits > 4) return false; }
        else return false;
    }
    return parts == 3 && digits > 0;
}
struct Invalid { const char* what; };
void need(bool ok, const char* what) { if (!ok) throw Invalid{what}; }
void only(const Json& j, std::initializer_list<const char*> allowed) {
    need(j.is_object(), "Expected an object.");
    for (auto it = j.begin(); it != j.end(); ++it)
        need(std::any_of(allowed.begin(), allowed.end(), [&](const char* x) { return it.key() == x; }), "Unknown field.");
}
std::string text(const Json& j, const char* key, size_t max, bool optional = false) {
    if (optional && !j.contains(key)) return "";
    need(j.contains(key) && j[key].is_string(), "Missing or invalid string field.");
    auto s = j[key].get<std::string>();
    need(plain_text(s, max) || (optional && s.empty()), "String outside allowed range.");
    return s;
}
int seq(const Json& j) {
    need(j.contains("sequence") && j["sequence"].is_number_integer(), "Invalid sequence.");
    const auto v = j["sequence"].get<long long>();
    need(v >= 0 && v <= 2000000000, "Invalid sequence.");
    return static_cast<int>(v);
}
} // namespace

std::string sha256_hex(std::string_view data) {
    unsigned char h[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), h);
    static const char* hex = "0123456789abcdef";
    std::string out;
    for (auto c : h) { out += hex[c >> 4]; out += hex[c & 15]; }
    return out;
}

std::optional<PublicKey> public_key_from_hex(std::string_view hex) {
    if (!hex_digits(hex, 64)) return std::nullopt;
    PublicKey key;
    for (size_t i = 0; i < 32; ++i) {
        auto nib = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        key.raw[i] = static_cast<unsigned char>(nib(hex[2 * i]) << 4 | nib(hex[2 * i + 1]));
    }
    return key;
}

bool verify(Kind kind, const PublicKey& key, std::string_view document, std::string_view signature_base64) {
    if (document.empty() || document.size() > index_limit) return false;
    auto sig = base64_decode(signature_base64);
    if (!sig || sig->size() != signature_bytes) return false;
    Pkey pkey(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, key.raw.data(), key.raw.size()), EVP_PKEY_free);
    if (!pkey) return false;
    MdCtx ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    const auto message = prefix(kind) + std::string(document);
    return ctx && EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) == 1 &&
        EVP_DigestVerify(ctx.get(), sig->data(), sig->size(),
                         reinterpret_cast<const unsigned char*>(message.data()), message.size()) == 1;
}

IndexResult open_index(const PublicKey& key, std::string_view document, std::string_view signature) {
    if (!verify(Kind::index, key, document, signature)) return {std::nullopt, "Index signature invalid."};
    try {
        auto j = parse_strict_json(document, index_limit);
        need(j.has_value(), "Invalid index JSON.");
        only(*j, {"schema_version", "sequence", "generated", "plugins"});
        need(j->contains("schema_version") && (*j)["schema_version"] == 1, "Unsupported index schema.");
        Index index;
        index.sequence = seq(*j);
        index.generated = text(*j, "generated", 32);
        need(j->contains("plugins") && (*j)["plugins"].is_array() && (*j)["plugins"].size() <= max_entries, "Invalid plugin list.");
        std::set<std::string> ids;
        for (const auto& v : (*j)["plugins"]) {
            only(v, {"id", "name", "description", "version", "author", "license", "category", "min_app_version", "file", "sha256", "size", "hosts"});
            IndexEntry e;
            e.id = text(v, "id", 48); e.name = text(v, "name", 320); e.description = text(v, "description", 2000, true);
            e.version = text(v, "version", 14); e.author = text(v, "author", 320); e.license = text(v, "license", 32);
            e.category = text(v, "category", 32); e.min_app_version = text(v, "min_app_version", 14);
            e.file = text(v, "file", 200); e.sha256 = text(v, "sha256", 64);
            need(ids.insert(e.id).second, "Duplicate plugin ID.");
            need(version_text(e.version) && version_text(e.min_app_version), "Invalid version.");
            need(e.file == "plugins/" + e.id + "/" + e.id + ".flplugin.json", "Unexpected package path.");
            need(hex_digits(e.sha256, 64), "Invalid SHA-256.");
            need(v.contains("size") && v["size"].is_number_unsigned() && v["size"].get<size_t>() > 0 && v["size"].get<size_t>() <= manifest_limit, "Invalid size.");
            e.size = v["size"].get<size_t>();
            need(v.contains("hosts") && v["hosts"].is_array() && v["hosts"].size() <= 4, "Invalid host list.");
            for (const auto& h : v["hosts"]) { need(h.is_string(), "Invalid host."); e.hosts.push_back(h.get<std::string>()); need(plain_text(e.hosts.back(), 253), "Invalid host."); }
            index.plugins.push_back(std::move(e));
        }
        return {std::move(index), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid index value."}; }
}

bool package_matches(const IndexEntry& e, std::string_view bytes) {
    if (bytes.size() != e.size || sha256_hex(bytes) != e.sha256) return false;
    auto parsed = parse_manifest(bytes);
    if (!parsed) return false;
    const auto& m = *parsed.manifest;
    if (m.id != e.id || m.version != e.version || m.license != e.license || m.author != e.author ||
        m.category != e.category || m.min_app_version != e.min_app_version) return false;
    std::set<std::string> hosts;
    for (const auto& s : m.sources) hosts.insert(s.host);
    return std::vector<std::string>(hosts.begin(), hosts.end()) == e.hosts;
}

RevokedResult open_revoked(const PublicKey& key, std::string_view document, std::string_view signature) {
    if (!verify(Kind::revoked, key, document, signature)) return {std::nullopt, "Revocation list signature invalid."};
    try {
        auto j = parse_strict_json(document, index_limit);
        need(j.has_value(), "Invalid revocation JSON.");
        only(*j, {"schema_version", "sequence", "revoked"});
        need(j->contains("schema_version") && (*j)["schema_version"] == 1, "Unsupported revocation schema.");
        Revocations list;
        list.sequence = seq(*j);
        need(j->contains("revoked") && (*j)["revoked"].is_array() && (*j)["revoked"].size() <= max_entries, "Invalid revocation list.");
        for (const auto& v : (*j)["revoked"]) {
            only(v, {"id", "versions", "reason"});
            Revocation r;
            r.id = text(v, "id", 48); r.reason = text(v, "reason", 300, true);
            need(v.contains("versions"), "Missing versions.");
            if (v["versions"].is_string()) { need(v["versions"] == "all", "Versions must be \"all\" or a list."); r.all = true; }
            else {
                need(v["versions"].is_array() && !v["versions"].empty() && v["versions"].size() <= 100, "Invalid versions.");
                for (const auto& x : v["versions"]) { need(x.is_string(), "Invalid version."); r.versions.push_back(x.get<std::string>()); }
            }
            list.items.push_back(std::move(r));
        }
        return {std::move(list), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid revocation value."}; }
}

unsigned png_crc32(std::string_view data) {
    static const auto table = [] {
        std::array<unsigned, 256> t{};
        for (unsigned n = 0; n < 256; ++n) {
            unsigned c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[n] = c;
        }
        return t;
    }();
    unsigned c = 0xFFFFFFFFu;
    for (unsigned char ch : data) c = table[(c ^ ch) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

bool png_dimensions_ok(std::string_view b) {
    static constexpr unsigned char magic[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (b.size() < 33 || b.size() > preview_limit || std::memcmp(b.data(), magic, 8) != 0 || std::memcmp(b.data() + 12, "IHDR", 4) != 0) return false;
    auto u32 = [&](size_t o) { return unsigned(static_cast<unsigned char>(b[o])) << 24 | unsigned(static_cast<unsigned char>(b[o + 1])) << 16 |
                                      unsigned(static_cast<unsigned char>(b[o + 2])) << 8 | unsigned(static_cast<unsigned char>(b[o + 3])); };
    const unsigned w = u32(16), h = u32(20);
    if (u32(8) != 13 || u32(29) != png_crc32(b.substr(12, 17))) return false;               // IHDR: Länge 13 und CRC
    const unsigned char depth = static_cast<unsigned char>(b[24]), color = static_cast<unsigned char>(b[25]);
    if (!(color == 0 || color == 2 || color == 3 || color == 4 || color == 6) || depth == 0 || depth > 16) return false;
    static constexpr unsigned char iend[12] = {0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82};
    if (std::memcmp(b.data() + b.size() - 12, iend, 12) != 0) return false;               // vollständiger Abschluss
    return w >= 16 && h >= 16 && w <= preview_max_side && h <= preview_max_side;
}

bool preview_matches(const PreviewEntry& e, std::string_view bytes) {
    return bytes.size() == e.size && sha256_hex(bytes) == e.sha256 && png_dimensions_ok(bytes);
}

PreviewsResult open_previews(const PublicKey& key, std::string_view document, std::string_view signature) {
    if (!verify(Kind::previews, key, document, signature)) return {std::nullopt, "Previews signature invalid."};
    try {
        auto j = parse_strict_json(document, index_limit);
        need(j.has_value(), "Invalid previews JSON.");
        only(*j, {"schema_version", "sequence", "previews"});
        need(j->contains("schema_version") && (*j)["schema_version"] == 1, "Unsupported previews schema.");
        Previews list;
        list.sequence = seq(*j);
        need(j->contains("previews") && (*j)["previews"].is_array() && (*j)["previews"].size() <= max_entries, "Invalid previews list.");
        std::set<std::string> ids;
        for (const auto& v : (*j)["previews"]) {
            only(v, {"id", "file", "sha256", "size"});
            PreviewEntry e;
            e.id = text(v, "id", 48); e.file = text(v, "file", 200); e.sha256 = text(v, "sha256", 64);
            need(ids.insert(e.id).second, "Duplicate preview ID.");
            need(e.file == "plugins/" + e.id + "/preview.png", "Unexpected preview path.");
            need(hex_digits(e.sha256, 64), "Invalid SHA-256.");
            need(v.contains("size") && v["size"].is_number_unsigned() && v["size"].get<size_t>() > 0 && v["size"].get<size_t>() <= preview_limit, "Invalid size.");
            e.size = v["size"].get<size_t>();
            list.items.push_back(std::move(e));
        }
        return {std::move(list), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid previews value."}; }
}

namespace {
bool slug_text(const std::string& s) {
    return !s.empty() && s.size() <= 48 && std::all_of(s.begin(), s.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; }) &&
           s.front() != '-' && s.back() != '-';
}
bool hex_color(const Json& v) {
    if (!v.is_string()) return false;
    const auto s = v.get<std::string>();
    return s.size() == 7 && s[0] == '#' && std::all_of(s.begin() + 1, s.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}
// Port of fl::theme utf8_ok: valid UTF-8, no control/bidi/zero-width characters and none of < > " \\ / & `.
bool theme_name_ok(const std::string& s, size_t& chars) {
    chars = 0;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        const size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (!n || i + n > s.size()) return false;
        uint32_t cp = n == 1 ? c : c & (0xFF >> (n + 1));
        for (size_t k = 1; k < n; ++k) {
            const unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((n == 2 && cp < 0x80) || (n == 3 && cp < 0x800) || (n == 4 && cp < 0x10000) || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
        if (cp < 0x20 || cp == 0x7F || (cp >= 0x80 && cp < 0xA0) || (cp >= 0x202A && cp <= 0x202E) || (cp >= 0x2066 && cp <= 0x2069) ||
            cp == 0x200B || cp == 0xFEFF || cp == 0x2028 || cp == 0x2029) return false;
        if (cp == '<' || cp == '>' || cp == '"' || cp == '\\' || cp == '/' || cp == '&' || cp == '`') return false;
        i += n;
        ++chars;
    }
    return true;
}
} // namespace

std::string theme_problem(std::string_view bytes, std::string* name_out) {
    if (bytes.empty() || bytes.size() > theme_limit) return "Theme size outside the allowed range.";
    auto parsed = parse_strict_json(bytes, theme_limit);
    if (!parsed || !parsed->is_object()) return "Invalid theme JSON.";
    const Json& j = *parsed;
    try {
        only(j, {"version", "name", "base", "dark", "colors"});
        for (const char* k : {"version", "name", "base", "dark", "colors"}) if (!j.contains(k)) return "Theme field missing.";
        if (!j.contains("version") || !j["version"].is_number_integer() || j["version"].get<int>() != 1) return "Unsupported theme version.";
        if (!j.contains("name") || !j["name"].is_string()) return "Invalid theme name.";
        const auto name = j["name"].get<std::string>();
        size_t chars = 0;
        const auto is_space = [](unsigned char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; };
        if (name.empty() || name.size() > 160 || !theme_name_ok(name, chars) || chars < 1 || chars > 40 || is_space(name.front()) || is_space(name.back()))
            return "Invalid theme name.";
        if (!j.contains("base") || !j["base"].is_string() || (j["base"] != "hell" && j["base"] != "dunkel")) return "Invalid theme base.";
        if (!j.contains("dark") || !j["dark"].is_boolean()) return "Invalid theme flag.";
        if (!j.contains("colors") || !j["colors"].is_object()) return "Invalid theme colours.";
        static const char* keys[9] = {"bg", "card", "line", "header", "accent", "text", "muted", "success", "danger"};
        only(j["colors"], {"bg", "card", "line", "header", "accent", "text", "muted", "success", "danger"});
        for (const char* k : keys) if (!j["colors"].contains(k) || !hex_color(j["colors"][k])) return "Theme colours must be #RRGGBB.";
        if (name_out) *name_out = name;
    } catch (const Invalid& e) { return e.what; }
      catch (const Json::exception&) { return "Invalid theme value."; }
    return {};
}

bool theme_matches(const ThemeEntry& e, std::string_view bytes) {
    std::string name;
    return bytes.size() == e.size && sha256_hex(bytes) == e.sha256 && theme_problem(bytes, &name).empty() && name == e.name;
}

ThemesResult open_themes(const PublicKey& key, std::string_view document, std::string_view signature) {
    if (!verify(Kind::themes, key, document, signature)) return {std::nullopt, "Theme list signature invalid."};
    try {
        auto j = parse_strict_json(document, index_limit);
        need(j.has_value(), "Invalid theme list JSON.");
        only(*j, {"schema_version", "sequence", "themes"});
        need(j->contains("schema_version") && (*j)["schema_version"] == 1, "Unsupported theme list schema.");
        ThemeList list;
        list.sequence = seq(*j);
        need(j->contains("themes") && (*j)["themes"].is_array() && (*j)["themes"].size() <= max_entries, "Invalid theme list.");
        std::set<std::string> ids;
        for (const auto& v : (*j)["themes"]) {
            only(v, {"id", "name", "description", "version", "author", "license", "file", "sha256", "size"});
            ThemeEntry e;
            e.id = text(v, "id", 48); e.name = text(v, "name", 160); e.description = text(v, "description", 500, true);
            e.version = text(v, "version", 14); e.author = text(v, "author", 320); e.license = text(v, "license", 32);
            e.file = text(v, "file", 200); e.sha256 = text(v, "sha256", 64);
            need(slug_text(e.id), "Invalid theme ID.");
            need(ids.insert(e.id).second, "Duplicate theme ID.");
            need(version_text(e.version), "Invalid version.");
            need(std::any_of(std::begin(store_licenses), std::end(store_licenses), [&](const char* x) { return e.license == x; }), "Unsupported license.");
            need(e.file == "themes/" + e.id + "/" + e.id + ".fltheme.json", "Unexpected theme path.");
            need(hex_digits(e.sha256, 64), "Invalid SHA-256.");
            need(v.contains("size") && v["size"].is_number_unsigned() && v["size"].get<size_t>() > 0 && v["size"].get<size_t>() <= theme_limit, "Invalid size.");
            e.size = v["size"].get<size_t>();
            list.items.push_back(std::move(e));
        }
        return {std::move(list), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid theme list value."}; }
}

bool is_revoked(const Revocations& list, std::string_view id, std::string_view version) {
    return std::any_of(list.items.begin(), list.items.end(), [&](const Revocation& r) {
        return r.id == id && (r.all || std::find(r.versions.begin(), r.versions.end(), version) != r.versions.end());
    });
}

std::optional<KeyPair> generate_keypair() {
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), EVP_PKEY_CTX_free);
    EVP_PKEY* raw = nullptr;
    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) != 1 || EVP_PKEY_keygen(ctx.get(), &raw) != 1) return std::nullopt;
    Pkey pkey(raw, EVP_PKEY_free);
    unsigned char pub[32]; size_t n = sizeof pub;
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pub, &n) != 1 || n != 32) return std::nullopt;
    Bio bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!bio || PEM_write_bio_PrivateKey(bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) return std::nullopt;
    char* data = nullptr;
    const long len = BIO_get_mem_data(bio.get(), &data);
    static const char* hex = "0123456789abcdef";
    KeyPair pair{std::string(data, static_cast<size_t>(len)), {}};
    for (auto c : pub) { pair.public_hex += hex[c >> 4]; pair.public_hex += hex[c & 15]; }
    return pair;
}

std::optional<std::string> sign_document(Kind kind, const std::string& private_pem, std::string_view document) {
    if (document.empty() || document.size() > index_limit) return std::nullopt;
    Bio bio(BIO_new_mem_buf(private_pem.data(), static_cast<int>(private_pem.size())), BIO_free);
    Pkey pkey(bio ? PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr) : nullptr, EVP_PKEY_free);
    if (!pkey || EVP_PKEY_base_id(pkey.get()) != EVP_PKEY_ED25519) return std::nullopt;
    MdCtx ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    const auto message = prefix(kind) + std::string(document);
    unsigned char sig[signature_bytes]; size_t n = sizeof sig;
    if (!ctx || EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1 ||
        EVP_DigestSign(ctx.get(), sig, &n, reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1 || n != signature_bytes)
        return std::nullopt;
    return base64_encode(sig, n);
}

BuildResult build_index(const std::vector<BuildInput>& manifests, int sequence, const std::string& generated) {
    if (manifests.size() > max_entries || sequence < 0 || sequence > 2000000000) return {std::nullopt, "Invalid build input."};
    std::vector<Json> entries;
    std::set<std::string> ids;
    for (const auto& in : manifests) {
        auto parsed = parse_manifest(in.manifest_bytes);
        if (!parsed) return {std::nullopt, "Manifest invalid: " + parsed.error};
        const auto& m = *parsed.manifest;
        if (!store_problems(m).empty()) return {std::nullopt, "Manifest breaks store rules: " + m.id};
        if (!ids.insert(m.id).second) return {std::nullopt, "Duplicate plugin ID: " + m.id};
        std::set<std::string> hosts;
        for (const auto& s : m.sources) hosts.insert(s.host);
        entries.push_back({{"id", m.id}, {"name", m.name}, {"description", m.description}, {"version", m.version},
            {"author", m.author}, {"license", m.license}, {"category", m.category}, {"min_app_version", m.min_app_version},
            {"file", "plugins/" + m.id + "/" + m.id + ".flplugin.json"}, {"sha256", sha256_hex(in.manifest_bytes)},
            {"size", in.manifest_bytes.size()}, {"hosts", Json(hosts)}});
    }
    std::sort(entries.begin(), entries.end(), [](const Json& a, const Json& b) { return a["id"] < b["id"]; });
    Json doc = {{"schema_version", 1}, {"sequence", sequence}, {"generated", generated}, {"plugins", entries}};
    return {doc.dump(2) + "\n", {}};
}
BuildResult build_previews(const std::vector<PreviewInput>& images, int sequence) {
    if (images.size() > max_entries || sequence < 0 || sequence > 2000000000) return {std::nullopt, "Invalid build input."};
    std::vector<Json> entries;
    std::set<std::string> ids;
    for (const auto& in : images) {
        if (!png_dimensions_ok(in.png_bytes)) return {std::nullopt, "Preview is not a valid PNG within limits: " + in.id};
        if (!ids.insert(in.id).second) return {std::nullopt, "Duplicate preview ID: " + in.id};
        entries.push_back({{"id", in.id}, {"file", "plugins/" + in.id + "/preview.png"}, {"sha256", sha256_hex(in.png_bytes)}, {"size", in.png_bytes.size()}});
    }
    std::sort(entries.begin(), entries.end(), [](const Json& a, const Json& b) { return a["id"] < b["id"]; });
    Json doc = {{"schema_version", 1}, {"sequence", sequence}, {"previews", entries}};
    return {doc.dump(2) + "\n", {}};
}
BuildResult build_themes(const std::vector<ThemeInput>& themes, int sequence) {
    if (themes.size() > max_entries || sequence < 0 || sequence > 2000000000) return {std::nullopt, "Invalid build input."};
    std::vector<Json> entries;
    std::set<std::string> ids;
    for (const auto& in : themes) {
        std::string name;
        const auto problem = theme_problem(in.theme_bytes, &name);
        if (!problem.empty()) return {std::nullopt, "Theme invalid (" + in.id + "): " + problem};
        if (!slug_text(in.id)) return {std::nullopt, "Invalid theme ID: " + in.id};
        if (!ids.insert(in.id).second) return {std::nullopt, "Duplicate theme ID: " + in.id};
        if (std::none_of(std::begin(store_licenses), std::end(store_licenses), [&](const char* x) { return in.license == x; }))
            return {std::nullopt, "Unsupported theme license: " + in.id};
        if (!version_text(in.version) || !plain_text(in.author, 320) || !plain_text(in.license, 32) || (!in.description.empty() && !plain_text(in.description, 500)))
            return {std::nullopt, "Invalid theme metadata: " + in.id};
        entries.push_back({{"id", in.id}, {"name", name}, {"description", in.description}, {"version", in.version}, {"author", in.author},
            {"license", in.license}, {"file", "themes/" + in.id + "/" + in.id + ".fltheme.json"}, {"sha256", sha256_hex(in.theme_bytes)}, {"size", in.theme_bytes.size()}});
    }
    std::sort(entries.begin(), entries.end(), [](const Json& a, const Json& b) { return a["id"] < b["id"]; });
    Json doc = {{"schema_version", 1}, {"sequence", sequence}, {"themes", entries}};
    auto text_out = doc.dump(2) + "\n";
    return {text_out, {}};
}
} // namespace fl::plugins::sign
