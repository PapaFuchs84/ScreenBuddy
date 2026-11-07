// Integration probe: run once unpackaged and once using Invoke-CommandInDesktopPackage.
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <future>
#include <winrt/Windows.Storage.h>
#include "distribution.h"
#include "update.h"
int wmain(int argc, wchar_t** argv) {
    if (argc < 3 || argc > 4) return 2;
    bool expected = std::wstring(argv[1]) == L"packaged";
    try {
        if (fl::distribution::packaged() != expected) return 3;
        std::filesystem::path report(argv[2]);
        fl::update::UpdateChecker checker(report.parent_path() / L"must-not-write-update.json", {});
        auto info = checker.info();
        if (expected) {
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
