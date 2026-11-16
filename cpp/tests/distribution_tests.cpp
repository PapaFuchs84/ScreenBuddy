// Integration probe: run once unpackaged and once using Invoke-CommandInDesktopPackage.
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <future>
#include <winrt/Windows.Storage.h>
#include "distribution.h"
#include "perflog.h"
#include "winsetup.h"
#include "update.h"
int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"import") {
        // Offline-Test der Store-Übernahme: argv[2] = leeres Arbeitsverzeichnis.
        namespace fs = std::filesystem;
        fs::path root(argv[2]), src = root / L"src", dst = root / L"dst";
        fs::create_directories(src / L"views"); fs::create_directories(src / L"themes"); fs::create_directories(dst);
        std::ofstream(src / L"config.toml") << "[ui]\nlanguage = \"de\"\n";
        std::ofstream(src / L"companion.json") << "{\"version\":1}";
        std::ofstream(src / L"spotify.token") << "dpapi:x";
        std::ofstream(src / L"views" / L"a.view.json") << "{}";
        std::ofstream(src / L"themes" / L"t.json") << "{}";
        std::ofstream(src / L"usb-display.log") << "log";
        if (!fl::distribution::import_standalone(src, dst)) return 20;
        for (const wchar_t* f : {L"config.toml", L"companion.json", L"spotify.token", L"views/a.view.json", L"themes/t.json"})
            if (!fs::exists(dst / f)) return 21;
        if (fs::exists(dst / L"usb-display.log")) return 22;           // Logs werden nicht übernommen
        std::ofstream(dst / L"config.toml") << "user";
        std::ofstream(src / L"config.toml") << "changed";
        if (fl::distribution::import_standalone(src, dst)) return 23;  // zweiter Lauf: nichts überschreiben
        std::ifstream in(dst / L"config.toml"); std::string s; in >> s;
        return s == "user" ? 0 : 24;
    }
    if (argc < 3 || argc > 4) return 2;
    bool expected = std::wstring(argv[1]) == L"packaged";
    try {
        if (fl::distribution::packaged() != expected) return 3;
        std::filesystem::path report(argv[2]);
        fl::update::UpdateChecker checker(report.parent_path() / L"must-not-write-update.json", {});
        auto info = checker.info();
        if (expected) {
            // Both entry points must reject packaged elevation before inspecting or
            // changing the supplied SID. No group membership is changed by this test.
            if (fl::perflog::request_grant()) return 9;
            if (fl::perflog::run_helper(L"S-1-5-21-1-2-3-1001") != ERROR_NOT_SUPPORTED) return 10;
            info["fps_elevation_blocked"] = true;
            if (fl::winsetup::vendor_disable_autostart()) return 11;
            info["vendor_elevation_blocked"] = true;
            winrt::init_apartment();
            auto data = std::async(std::launch::async, [] { return fl::distribution::data_dir(); }).get();
            auto local = std::filesystem::path(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str());
            if (data != local) return 4;
            // No standalone state may be written and no GitHub update request is started.
            checker.start();
            if (info.value("channel", "") != "microsoft-store") return 5;
            checker.stop();
            if (std::filesystem::exists(report.parent_path() / L"must-not-write-update.json")) return 7;
            std::ofstream(data / L"distribution-test-marker.txt") << "package-data-persistence";
            bool initial = fl::distribution::startup_enabled();
            info["startup_enabled"] = initial;
            if (argc == 4 && std::wstring(argv[3]) == L"exercise-startup") {
                fl::distribution::set_startup(!initial);
                bool changed = fl::distribution::startup_enabled() == !initial;
                fl::distribution::set_startup(initial);
                if (!changed || fl::distribution::startup_enabled() != initial) return 8;
                info["startup_roundtrip"] = true;
            }
            info["data_dir"] = data.string();
            winrt::uninit_apartment();
        } else if (info.contains("channel")) return 6;
        info["packaged"] = expected;
        std::ofstream(report) << info.dump(2);
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what(); return 1; }
}
