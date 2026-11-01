// Store package identity is detected at runtime; standalone builds keep their behavior.
#include "distribution.h"
#include <windows.h>
#include <appmodel.h>
#include <shellapi.h>
#include <future>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include "util.h"
namespace fl::distribution {
namespace {
struct Apartment {
    Apartment() { winrt::init_apartment(); }
    ~Apartment() { winrt::uninit_apartment(); }
};
}  // namespace
bool packaged() {
    static const bool result = [] { UINT32 size = 0; return GetCurrentPackageFullName(&size, nullptr) == ERROR_INSUFFICIENT_BUFFER; }();
    return result;
}
std::filesystem::path data_dir() {
    // Resolve on a dedicated initialized thread: callers include plugin workers without a COM apartment.
    static const auto result = std::async(std::launch::async, [] {
        Apartment apartment;
        return std::filesystem::path(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str());
    }).get();
    std::filesystem::create_directories(result);
    return result;
}
// A dedicated MTA avoids blocking an STA while waiting for WinRT operations.
bool startup_enabled() {
    return std::async(std::launch::async, [] {
        try {
            Apartment apartment;
            auto task = winrt::Windows::ApplicationModel::StartupTask::GetAsync(L"ScreenBuddyStartup").get();
            bool enabled = task.State() == winrt::Windows::ApplicationModel::StartupTaskState::Enabled;
            return enabled;
        } catch (...) { return false; }
    }).get();
}
void set_startup(bool enabled) {
    std::async(std::launch::async, [enabled] {
        try {
            Apartment apartment;
            auto task = winrt::Windows::ApplicationModel::StartupTask::GetAsync(L"ScreenBuddyStartup").get();
            if (enabled) task.RequestEnableAsync().get(); else task.Disable();
        } catch (...) { LOGW("store", "Startup task unavailable"); }
    }).get();
}
void open_updates() { ShellExecuteW(nullptr, L"open", L"ms-windows-store://downloadsandupdates", nullptr, nullptr, SW_SHOWNORMAL); }
}
