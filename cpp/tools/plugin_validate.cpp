// © 2026 FuchsLabs – plugin validator for authors and store CI. Uses the app's own parser (plugins.cpp).
// Offline: reads files only, no HTTP. Exit code 0 = all plugins valid, 1 = problems, 2 = usage error.
// Build (Linux/Windows): c++ -std=c++20 -I<json-include> -Isrc src/plugins.cpp tools/plugin_validate.cpp
#include "plugins.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

using namespace fl::plugins;
namespace fs = std::filesystem;

static void usage() {
    std::puts("Usage: fl-plugin-validate [--store] [--app-version X.Y.Z] <file.flplugin.json | folder>...\n"
              "  --store             additionally enforce store rules (license, author, category, min_app_version,\n"
              "                      file name = <id>.flplugin.json, unique IDs)\n"
              "  --app-version X.Y.Z check min_app_version against this ScreenBuddy version");
}

int main(int argc, char** argv) {
    bool store = false;
    std::string app_version;
    std::vector<fs::path> files;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--store") store = true;
        else if (arg == "--app-version" && i + 1 < argc) app_version = argv[++i];
        else if (arg == "--help" || arg == "-h") { usage(); return 0; }
        else if (arg.starts_with("--")) { usage(); return 2; }
        else if (fs::is_directory(arg)) {
            for (const auto& e : fs::recursive_directory_iterator(arg))
                if (e.is_regular_file() && e.path().filename().string().ends_with(".flplugin.json")) files.push_back(e.path());
        } else files.emplace_back(arg);
    }
    if (files.empty()) { usage(); return 2; }
    std::sort(files.begin(), files.end());

    int bad = 0;
    std::set<std::string> ids;
    for (const auto& file : files) {
        std::vector<std::string> problems;
        std::ifstream input(file, std::ios::binary);
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        std::string hosts;
        if (!input && !fs::exists(file)) problems.push_back("File not found.");
        else {
            auto parsed = parse_manifest(data);
            if (!parsed) problems.push_back("Manifest: " + parsed.error);
            else {
                const auto& m = *parsed.manifest;
                for (const auto& s : m.sources) {
                    hosts += (hosts.empty() ? "" : ", ") + s.host;
                    if (!parse_source_data(s.sample.dump())) problems.push_back("Source sample is not valid source data.");
                    if (!prepare_request(m, s, Json::object())) problems.push_back("Source request cannot be prepared.");
                }
                if (!app_version.empty() && !app_compatible(m, app_version))
                    problems.push_back("Needs ScreenBuddy " + m.min_app_version + " or newer.");
                if (store) {
                    for (auto& p : store_problems(m)) problems.push_back(std::move(p));
                    if (file.filename().string() != m.id + ".flplugin.json") problems.push_back("Store: file name must be <id>.flplugin.json.");
                    if (!ids.insert(m.id).second) problems.push_back("Store: duplicate plugin ID.");
                }
                if (problems.empty())
                    std::printf("OK    %s  (%s %s%s%s)\n", file.string().c_str(), m.id.c_str(), m.version.c_str(),
                                hosts.empty() ? ", no network" : ", hosts: ", hosts.c_str());
            }
        }
        if (!problems.empty()) {
            ++bad;
            std::printf("FAIL  %s\n", file.string().c_str());
            for (const auto& p : problems) std::printf("      - %s\n", p.c_str());
        }
    }
    std::printf("%zu checked, %d with problems.\n", files.size(), bad);
    return bad ? 1 : 0;
}
