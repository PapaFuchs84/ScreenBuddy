// © 2026 FuchsLabs
#include "util.h"

#include <windows.h>
#include <shlobj.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>

namespace fl {

std::wstring widen(std::string_view s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string narrow(std::wstring_view w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

static bool is_space(unsigned char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

std::string trim(std::string_view s) {
    size_t a = 0, b = s.size();
    while (a < b && is_space((unsigned char)s[a])) ++a;
    while (b > a && is_space((unsigned char)s[b - 1])) --b;
    return std::string(s.substr(a, b - a));
}

std::string lower_ascii(std::string s) {
    for (auto& c : s)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return s;
}

std::string casefold(std::string_view s) {
    std::wstring w = widen(s);
    if (w.empty()) return {};
    int n = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, w.c_str(), (int)w.size(), nullptr, 0, nullptr,
                          nullptr, 0);
    std::wstring out(n, L'\0');
    LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, w.c_str(), (int)w.size(), out.data(), n, nullptr, nullptr, 0);
    std::wstring res;
    res.reserve(out.size());
    for (wchar_t c : out) {
        if (c == L'ß' || c == L'ẞ') res += L"ss";
        else res += c;
    }
    return narrow(res);
}

bool starts_with_any(std::string_view s, std::initializer_list<std::string_view> prefixes) {
    for (auto p : prefixes)
        if (s.substr(0, p.size()) == p) return true;
    return false;
}

std::vector<std::string> split(std::string_view s, std::string_view sep) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (true) {
        size_t i = s.find(sep, pos);
        if (i == std::string_view::npos) {
            out.emplace_back(s.substr(pos));
            return out;
        }
        out.emplace_back(s.substr(pos, i - pos));
        pos = i + sep.size();
    }
}

std::string join(const std::vector<std::string>& parts, std::string_view sep) {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

std::string replace_all(std::string s, std::string_view from, std::string_view to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::u32string to_u32(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        char32_t cp;
        int len;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c >> 5) == 6) { cp = c & 0x1F; len = 2; }
        else if ((c >> 4) == 14) { cp = c & 0x0F; len = 3; }
        else if ((c >> 3) == 30) { cp = c & 0x07; len = 4; }
        else { ++i; out += U'�'; continue; }
        if (i + len > s.size()) { out += U'�'; break; }
        bool bad = false;
        for (int k = 1; k < len; ++k) {
            unsigned char cc = (unsigned char)s[i + k];
            if ((cc >> 6) != 2) { bad = true; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (bad) { out += U'�'; ++i; continue; }
        out += cp;
        i += len;
    }
    return out;
}

std::string from_u32(std::u32string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char32_t cp : s) {
        if (cp < 0x80) out += char(cp);
        else if (cp < 0x800) { out += char(0xC0 | (cp >> 6)); out += char(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            out += char(0xE0 | (cp >> 12)); out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        } else {
            out += char(0xF0 | (cp >> 18)); out += char(0x80 | ((cp >> 12) & 0x3F));
            out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

std::string collapse_spaces(std::string_view s) {
    // wie Pythons " ".join(s.split()): auch Unicode-Leerraum (z. B. geschütztes Leerzeichen) trennt
    std::u32string u = to_u32(s), out;
    out.reserve(u.size());
    bool pending = false;
    for (char32_t c : u) {
        bool sp = c == U' ' || (c >= 9 && c <= 13) || c == 0x1C || c == 0x1D || c == 0x1E || c == 0x1F || c == 0x85 ||
                  c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
                  c == 0x202F || c == 0x205F || c == 0x3000;
        if (sp) { pending = !out.empty(); continue; }
        if (pending) { out += U' '; pending = false; }
        out += c;
    }
    return from_u32(out);
}

std::string format_num(double v, int decimals) {
    char buf[64];
    snprintf(buf, sizeof buf, "%.*f", decimals, v);
    std::string s = buf;
    for (auto& c : s)
        if (c == '.') c = ',';
    if (s == "-0") s = "0";
    return s;
}

std::string strprintf(const char* fmt, ...) {
    char stack_buf[256];
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
    va_end(ap);
    if (n >= 0 && (size_t)n < sizeof(stack_buf)) {
        va_end(ap2);
        return std::string(stack_buf, (size_t)n);
    }
    std::string s(n > 0 ? n : 0, '\0');
    if (n > 0) vsnprintf(s.data(), n + 1, fmt, ap2);
    va_end(ap2);
    return s;
}

double mono() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

std::string clock_hhmm(std::time_t t) {
    std::tm tm{};
    localtime_s(&tm, &t);
    char buf[8];
    strftime(buf, sizeof buf, "%H:%M", &tm);
    return buf;
}

std::string clock_hhmm() { return clock_hhmm(std::time(nullptr)); }

std::string run_hidden(const std::wstring& cmdline, unsigned timeout_ms, int* exit_code, bool oem) {
    if (exit_code) *exit_code = -1;
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return "";
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{sizeof si};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = nullptr;
    PROCESS_INFORMATION pi{};
    std::wstring cmd = cmdline;
    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(wr);
    std::string out;
    if (ok) {
        double end = mono() + timeout_ms / 1000.0;
        char buf[4096];
        while (true) {
            DWORD avail = 0;
            if (!PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr)) break;  // Prozess fertig, Pipe zu
            if (avail) {
                DWORD n = 0;
                if (!ReadFile(rd, buf, std::min<DWORD>(avail, sizeof buf), &n, nullptr) || !n) break;
                out.append(buf, n);
                if (out.size() > 1'000'000) break;
                continue;
            }
            if (WaitForSingleObject(pi.hProcess, 20) == WAIT_OBJECT_0) {
                while (PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) && avail) {  // Rest lesen
                    DWORD n = 0;
                    if (!ReadFile(rd, buf, std::min<DWORD>(avail, sizeof buf), &n, nullptr) || !n) break;
                    out.append(buf, n);
                }
                break;
            }
            if (mono() > end) {
                TerminateProcess(pi.hProcess, 1);
                out.clear();
                break;
            }
        }
        WaitForSingleObject(pi.hProcess, 2000);
        DWORD code = 0;
        if (exit_code && GetExitCodeProcess(pi.hProcess, &code)) *exit_code = (int)code;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(rd);
    if (oem && !out.empty()) {
        int n = MultiByteToWideChar(CP_OEMCP, 0, out.data(), (int)out.size(), nullptr, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_OEMCP, 0, out.data(), (int)out.size(), w.data(), n);
        out = narrow(w);
    }
    return out;
}

bool test_instance() {
    wchar_t buf[64];
    return GetEnvironmentVariableW(L"FUCHSLABS_INSTANCE", buf, 64) > 0;
}

std::filesystem::path exe_path() {
    std::wstring buf(MAX_PATH, L'\0');
    while (true) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), (DWORD)buf.size());
        if (n < buf.size()) { buf.resize(n); break; }
        buf.resize(buf.size() * 2);
    }
    return buf;
}

static std::filesystem::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    std::filesystem::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

static std::wstring env(const wchar_t* name) {
    wchar_t buf[1024];
    DWORD n = GetEnvironmentVariableW(name, buf, 1024);
    return n && n < 1024 ? std::wstring(buf, n) : std::wstring();
}

std::filesystem::path install_dir() {
    // FUCHSLABS_INSTALL_DIR nur für Tests (Installer ausprobieren, ohne die echte Installation anzufassen)
    if (auto t = env(L"FUCHSLABS_INSTALL_DIR"); !t.empty() && test_instance()) return t;
    return known_folder(FOLDERID_LocalAppData) / L"FuchsLabs" / L"USB-Display";
}

std::wstring mutex_name() { return L"Local\\FuchsLabsUsbDisplay" + env(L"FUCHSLABS_INSTANCE"); }
std::filesystem::path legacy_dir() {
    // Tests: eigener Ordner statt der echten Version 1.x (die nie umbenannt werden darf)
    if (test_instance()) {
        auto t = env(L"FUCHSLABS_LEGACY_DIR");
        return t.empty() ? std::filesystem::path(L"Z:\\fuchslabs-test-ohne-altversion") : std::filesystem::path(t);
    }
    return known_folder(FOLDERID_Profile) / L"FuchsLabs" / L"USB-Display";
}

// ---------- Protokoll ----------
namespace {
std::mutex g_log_mutex;
std::filesystem::path g_log_file;
std::atomic<bool> g_verbose{false};
constexpr uintmax_t LOG_MAX = 500'000;
}  // namespace

void log_init(const std::filesystem::path& file, bool verbose) {
    std::lock_guard lk(g_log_mutex);
    g_log_file = file;
    g_verbose.store(verbose, std::memory_order_relaxed);
}

bool is_verbose() {
    return g_verbose.load(std::memory_order_relaxed);
}

void log(Level lv, std::string_view name, std::string_view msg) {
    if (lv == Level::Debug && !is_verbose()) return;
    static const char* names[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::string line = strprintf("%04d-%02d-%02d %02d:%02d:%02d,%03d %s %.*s: %.*s\n", st.wYear, st.wMonth, st.wDay,
                                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, names[(int)lv], (int)name.size(),
                                 name.data(), (int)msg.size(), msg.data());
    std::lock_guard lk(g_log_mutex);
    if (g_log_file.empty()) {
        fwrite(line.data(), 1, line.size(), stderr);
        return;
    }
    std::error_code ec;
    if (std::filesystem::exists(g_log_file, ec) && std::filesystem::file_size(g_log_file, ec) > LOG_MAX) {
        auto bak = g_log_file;
        bak += L".1";
        std::filesystem::remove(bak, ec);
        std::filesystem::rename(g_log_file, bak, ec);
    }
    std::ofstream f(g_log_file, std::ios::binary | std::ios::app);
    f.write(line.data(), (std::streamsize)line.size());
}

}  // namespace fl
