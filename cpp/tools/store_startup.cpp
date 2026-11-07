#include <windows.h>
#include <filesystem>
#include <string>
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    wchar_t self[32768];
    DWORD length = GetModuleFileNameW(nullptr, self, 32768);
    if (!length || length >= 32768) return 1;
    auto exe = std::filesystem::path(self).parent_path() / L"ScreenBuddy.exe";
    std::wstring command = L"\"" + exe.wstring() + L"\" --run --autostart";
    STARTUPINFOW startup{sizeof startup}; PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) return 1;
    CloseHandle(process.hThread); CloseHandle(process.hProcess); return 0;
}
