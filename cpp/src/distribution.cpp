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
namespace {
bool plain(const std::filesystem::path& p) {   // keine Symlinks/Junctions importieren
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_REPARSE_POINT);
}
bool copy_tree(const std::filesystem::path& from, const std::filesystem::path& to) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!plain(from)) return true;
    if (fs::is_directory(from, ec)) {
        fs::create_directories(to, ec);
        for (auto& e : fs::directory_iterator(from, ec))
            if (!copy_tree(e.path(), to / e.path().filename())) return false;
        return true;
    }
    if (fs::exists(to, ec)) return true;   // nie überschreiben
    fs::create_directories(to.parent_path(), ec);
    ec.clear();
    fs::copy_file(from, to, fs::copy_options::skip_existing, ec);
    return !ec;
}
}  // namespace
std::filesystem::path standalone_dir() {
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, static_cast<DWORD>(std::size(buf)));
    if (n == 0 || n >= std::size(buf)) return {};
    return std::filesystem::path(buf) / L"FuchsLabs" / L"USB-Display";
}
bool import_standalone(const std::filesystem::path& source, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (source.empty() || !fs::exists(source / L"config.toml", ec) || fs::exists(destination / L"config.toml", ec)) return false;
    if (fs::equivalent(source, destination, ec)) return false;
    for (const wchar_t* name : {L"companion.json", L"spotify.token", L"views", L"themes", L"plugins", L"plugin-settings", L"config.toml"})
        if (!copy_tree(source / name, destination / name)) {
            LOGW("store", "Übernahme der EXE-Einstellungen unvollständig (%ls)", name);
            return false;
        }
    LOGI("store", "EXE-Einstellungen übernommen");
    return true;
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
