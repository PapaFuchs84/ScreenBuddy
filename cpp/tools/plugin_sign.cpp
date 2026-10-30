// © 2026 FuchsLabs – signing tool for the plugin store (maintainers/CI only; the app never signs).
// Offline. The private key is a PEM file that must stay outside the repository (CI: GitHub secret written to a temp file).
// Build: c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp src/plugin_sign.cpp tools/plugin_sign.cpp -lcrypto
#include "plugin_sign.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>

using namespace fl::plugins;
namespace fs = std::filesystem;

static void usage() {
    std::puts("Usage:\n"
              "  fl-plugin-sign keygen <private.pem>                    new Ed25519 key (refuses to overwrite); prints public key hex\n"
              "  fl-plugin-sign build-index <plugins-root> <out-dir> <sequence>\n"
              "                                                         reads <root>/<id>/<id>.flplugin.json, writes <out-dir>/index.json\n"
              "  fl-plugin-sign sign index|revoked <file> <private.pem>  writes <file>.sig\n"
              "  fl-plugin-sign verify index|revoked <file> <public-hex> checks <file> against <file>.sig");
}
static bool read_file(const fs::path& p, std::string& out) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}
static bool kind_of(const std::string& s, sign::Kind& k) {
    if (s == "index") k = sign::Kind::index; else if (s == "revoked") k = sign::Kind::revoked; else return false;
    return true;
}

int main(int argc, char** argv) {
    std::vector<std::string> a(argv + 1, argv + argc);
    if (a.empty()) { usage(); return 2; }
    if (a[0] == "keygen" && a.size() == 2) {
        if (fs::exists(a[1])) { std::puts("Refusing to overwrite an existing key file."); return 1; }
        auto pair = sign::generate_keypair();
        if (!pair) { std::puts("Key generation failed."); return 1; }
        { std::ofstream out(a[1], std::ios::binary); out << pair->private_pem; if (!out) { std::puts("Cannot write key file."); return 1; } }
        std::error_code ec;
        fs::permissions(a[1], fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, ec);
        std::printf("Private key written to %s (keep it OFFLINE, never commit it).\nPublic key (hex, pin this in the app):\n%s\n", a[1].c_str(), pair->public_hex.c_str());
        return 0;
    }
    if (a[0] == "build-index" && a.size() == 4) {
        std::vector<sign::BuildInput> inputs;
        std::vector<fs::path> files;
        for (const auto& e : fs::recursive_directory_iterator(a[1]))
            if (e.is_regular_file() && e.path().filename().string().ends_with(".flplugin.json")) files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) {
            std::string data;
            if (!read_file(f, data)) { std::printf("Cannot read %s\n", f.string().c_str()); return 1; }
            auto parsed = parse_manifest(data);
            if (!parsed || f.filename().string() != parsed.manifest->id + ".flplugin.json" ||
                f.parent_path().filename().string() != parsed.manifest->id) {
                std::printf("Not a valid store layout (<id>/<id>.flplugin.json): %s\n", f.string().c_str());
                return 1;
            }
            inputs.push_back({std::move(data)});
        }
        char stamp[32];
        const std::time_t now = std::time(nullptr);
        std::tm utc{};
#ifdef _WIN32
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        std::strftime(stamp, sizeof stamp, "%Y-%m-%dT%H:%M:%SZ", &utc);
        auto built = sign::build_index(inputs, std::atoi(a[3].c_str()), stamp);
        if (!built.index_json) { std::printf("%s\n", built.error.c_str()); return 1; }
        fs::create_directories(a[2]);
        std::ofstream out(fs::path(a[2]) / "index.json", std::ios::binary);
        out << *built.index_json;
        std::printf("index.json with %zu plugins written.\n", inputs.size());
        return out ? 0 : 1;
    }
    sign::Kind kind;
    if ((a[0] == "sign" || a[0] == "verify") && a.size() == 4 && kind_of(a[1], kind)) {
        std::string doc;
        if (!read_file(a[2], doc)) { std::puts("Cannot read document."); return 1; }
        if (a[0] == "sign") {
            std::string pem;
            if (!read_file(a[3], pem)) { std::puts("Cannot read private key."); return 1; }
            auto sig = sign::sign_document(kind, pem, doc);
            if (!sig) { std::puts("Signing failed."); return 1; }
            std::ofstream out(a[2] + ".sig", std::ios::binary);
            out << *sig << "\n";
            std::printf("%s.sig written.\n", a[2].c_str());
            return out ? 0 : 1;
        }
        std::string sig;
        auto key = sign::public_key_from_hex(a[3]);
        if (!key || !read_file(a[2] + ".sig", sig)) { std::puts("Bad public key or missing .sig file."); return 1; }
        while (!sig.empty() && (sig.back() == '\n' || sig.back() == '\r')) sig.pop_back();
        const bool ok = sign::verify(kind, *key, doc, sig);
        std::puts(ok ? "Signature OK." : "Signature INVALID.");
        return ok ? 0 : 1;
    }
    usage();
    return 2;
}
