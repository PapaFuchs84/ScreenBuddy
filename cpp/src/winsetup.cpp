// © 2026 FuchsLabs
#include "winsetup.h"

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include "util.h"

namespace fl::winsetup {

static const wchar_t* RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* RUN_NAME = L"FuchsLabs USB-Display";
// Technische Kennungen der Hersteller-App (Prozess und Autostart-Aufgabe), nötig zum Beenden/Abschalten
static const wchar_t* VENDOR_PROCESS = L"UsbMonitor.exe";
static const wchar_t* VENDOR_TASK = L"UsbMonitor";

std::wstring launch_command() { return L"\"" + exe_path().wstring() + L"\""; }

static std::wstring autostart_command() { return launch_command() + L" " + widen(AUTOSTART_FLAG); }

static std::optional<std::wstring> run_value() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return std::nullopt;
    wchar_t buf[2048] = {};
    DWORD sz = sizeof buf - sizeof(wchar_t), type = 0;
    LSTATUS r = RegQueryValueExW(k, RUN_NAME, nullptr, &type, (BYTE*)buf, &sz);
    RegCloseKey(k);
    if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return std::nullopt;
    return std::wstring(buf);
}

bool autostart_enabled() {
    auto v = run_value();
    return v && (*v == autostart_command() || *v == launch_command());
}

void upgrade_autostart() {
    // Alte Einträge (Version 1.x: andere EXE, oder ohne --autostart) auf diese EXE mit stillem Start umstellen
    auto v = run_value();
    if (v && *v != autostart_command()) set_autostart(true);
}

void set_autostart(bool enabled) { set_autostart_for(exe_path().wstring(), enabled); }

void set_autostart_for(const std::wstring& exe, bool enabled) {
    if (test_instance()) {  // Testinstanz: echten Autostart nie anfassen
        LOGI("winsetup", "Testinstanz: Autostart unverändert");
        return;
    }
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return;
    if (enabled) {
        std::wstring cmd = L"\"" + exe + L"\" " + widen(AUTOSTART_FLAG);
        RegSetValueExW(k, RUN_NAME, 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(k, RUN_NAME);
    }
    RegCloseKey(k);
}

template <class F>
static void for_each_process(F f) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe{sizeof pe};
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) f(pe);
    CloseHandle(snap);
}

bool vendor_running() {
    bool found = false;
    for_each_process([&](const PROCESSENTRY32W& pe) { found |= _wcsicmp(pe.szExeFile, VENDOR_PROCESS) == 0; });
    return found;
}

bool vendor_stop() {
    for_each_process([&](const PROCESSENTRY32W& pe) {
        if (_wcsicmp(pe.szExeFile, VENDOR_PROCESS) != 0) return;
        if (HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID)) {
            TerminateProcess(h, 1);
            WaitForSingleObject(h, 3000);
            CloseHandle(h);
        }
    });
    return !vendor_running();
}

bool vendor_autostart_enabled() {
    int code = -1;
    std::string out = run_hidden(L"schtasks /Query /TN \"" + std::wstring(VENDOR_TASK) + L"\" /FO LIST", 20000, &code, true);
    return code == 0 && out.find("Disabled") == std::string::npos && out.find("Deaktiviert") == std::string::npos;
}

bool vendor_disable_autostart() {
    std::wstring args = L"/Change /TN \"" + std::wstring(VENDOR_TASK) + L"\" /DISABLE";
    auto rc = (INT_PTR)ShellExecuteW(nullptr, L"runas", L"schtasks.exe", args.c_str(), nullptr, SW_HIDE);
    return rc > 32;
}

}  // namespace fl::winsetup
