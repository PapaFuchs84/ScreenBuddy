// © 2026 FuchsLabs
#include "plugin_sign.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <algorithm>
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
    return kind == Kind::index ? "FuchsLabs-ScreenBuddy-Plugin-Index-v1\n" : "FuchsLabs-ScreenBuddy-Plugin-Revoked-v1\n";
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
    if (manifests.size() > max_entries || sequence < 0) return {std::nullopt, "Invalid build input."};
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
} // namespace fl::plugins::sign
