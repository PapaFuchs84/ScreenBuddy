// © 2026 FuchsLabs
// Treiber für das 3,5"-USB-Display (480 × 320, Hardware-Revision A).
// Protokoll (öffentlich dokumentiert; Open-Source-Referenzimplementierungen nutzen dasselbe „Revision A“-Format):
// 6-Byte-Kommando (x/y/ex/ey als 10-Bit-Werte gepackt, Byte 5 = Befehl), Bilddaten als RGB565 little-endian.
#include <windows.h>
#include <devguid.h>
#include <initguid.h>
#include <ntddser.h>
#include <setupapi.h>

#include <stdexcept>

#include "output.h"
#include "util.h"

namespace fl {

namespace {

constexpr int NATIVE_W = 320, NATIVE_H = 480;  // Panel ist nativ Hochformat
const char* DEVICE_SERIAL = "USB35INCHIPSV2";
const std::pair<unsigned, unsigned> DEVICE_IDS[] = {{0x1A86, 0x5722}, {0x1A86, 0xF111}};  // USB-Kennungen (VID, PID)

enum Cmd : uint8_t {
    RESET = 101, CLEAR = 102, SCREEN_OFF = 108, SCREEN_ON = 109, SET_BRIGHTNESS = 110, SET_ORIENTATION = 121,
    DISPLAY_BITMAP = 197, HELLO = 69
};

std::string encode(uint8_t cmd, int x = 0, int y = 0, int ex = 0, int ey = 0) {
    std::string b(6, '\0');
    b[0] = (char)(x >> 2);
    b[1] = (char)(((x & 3) << 6) + (y >> 4));
    b[2] = (char)(((y & 15) << 4) + (ex >> 6));
    b[3] = (char)(((ex & 63) << 2) + (ey >> 8));
    b[4] = (char)(ey & 255);
    b[5] = (char)cmd;
    return b;
}

}  // namespace

std::optional<std::string> SerialDisplay::find_port() {
    HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return std::nullopt;
    std::optional<std::string> found;
    SP_DEVINFO_DATA dev{sizeof dev};
    for (DWORD i = 0; !found && SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
        wchar_t id[512] = {};
        if (!SetupDiGetDeviceInstanceIdW(set, &dev, id, 512, nullptr)) continue;
        std::string inst = narrow(id);  // z. B. USB\VID_1A86&PID_5722\USB35INCHIPSV2
        std::string up = inst;
        for (auto& c : up) c = (char)toupper((unsigned char)c);
        unsigned vid = 0, pid = 0;
        try {
            if (auto p = up.find("VID_"); p != std::string::npos && p + 8 <= up.size())
                vid = std::stoul(up.substr(p + 4, 4), nullptr, 16);
            if (auto p = up.find("PID_"); p != std::string::npos && p + 8 <= up.size())
                pid = std::stoul(up.substr(p + 4, 4), nullptr, 16);
        } catch (...) {
        }
        std::string serial = inst.substr(inst.rfind('\\') + 1);
        bool match = serial == DEVICE_SERIAL;
        for (auto& [v, p] : DEVICE_IDS) match |= v == vid && p == pid;
        if (!match) continue;
        HKEY k = SetupDiOpenDevRegKey(set, &dev, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (k == INVALID_HANDLE_VALUE) continue;
        wchar_t name[64] = {};
        DWORD sz = sizeof name;
        if (RegQueryValueExW(k, L"PortName", nullptr, nullptr, (BYTE*)name, &sz) == ERROR_SUCCESS) found = narrow(name);
        RegCloseKey(k);
    }
    SetupDiDestroyDeviceInfoList(set);
    return found;
}

std::string SerialDisplay::rgb565le(const Image& img, int y0, int y1) {
    std::string out;
    out.resize(size_t(img.w) * (y1 - y0) * 2);
    size_t o = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = 0; x < img.w; ++x) {
            uint32_t p = img.px[size_t(y) * img.w + x];
            uint8_t r = p >> 16 & 255, g = p >> 8 & 255, b = p & 255;
            out[o++] = (char)((((g >> 2) & 7) << 5) | (b >> 3));  // erst Low-, dann High-Byte = little-endian
            out[o++] = (char)(((r >> 3) << 3) | (g >> 5));
        }
    return out;
}

SerialDisplay::SerialDisplay(std::string port, Orientation o, int brightness, bool reset_on_start)
    : port_cfg_(std::move(port)), orientation_(o), brightness_(brightness), reset_on_start_(reset_on_start) {}

SerialDisplay::~SerialDisplay() { close(); }

std::pair<int, int> SerialDisplay::size() const {
    if (orientation_ == Orientation::Portrait || orientation_ == Orientation::ReversePortrait) return {NATIVE_W, NATIVE_H};
    return {NATIVE_H, NATIVE_W};
}

void SerialDisplay::open(int attempts) {
    for (int i = 1; i <= attempts; ++i) {
        std::string up = port_cfg_;
        for (auto& c : up) c = (char)toupper((unsigned char)c);
        std::optional<std::string> port = up == "AUTO" ? find_port() : std::optional<std::string>(port_cfg_);
        if (port) {
            std::wstring path = L"\\\\.\\" + widen(*port);
            HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                DCB dcb{sizeof dcb};
                GetCommState(h, &dcb);
                dcb.BaudRate = 115200;
                dcb.ByteSize = 8;
                dcb.Parity = NOPARITY;
                dcb.StopBits = ONESTOPBIT;
                dcb.fBinary = TRUE;
                dcb.fOutxCtsFlow = TRUE;                   // RTS/CTS wie Version 1.x
                dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
                dcb.fDtrControl = DTR_CONTROL_ENABLE;
                dcb.fOutxDsrFlow = FALSE;
                dcb.fOutX = dcb.fInX = FALSE;
                SetCommState(h, &dcb);
                COMMTIMEOUTS t{};
                t.ReadIntervalTimeout = 0;
                t.ReadTotalTimeoutConstant = 1000;          // Lesen: 1 s
                t.WriteTotalTimeoutConstant = 5000;         // Schreiben: 5 s
                SetCommTimeouts(h, &t);
                PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
                handle_ = h;
                LOGI("display", "Display verbunden an %s", port->c_str());
                return;
            }
            LOGW("display", "Anschluss %s nicht zu öffnen (Fehler %lu) – läuft noch die Hersteller-App des Displays? (%d/%d)",
                 port->c_str(), GetLastError(), i, attempts);
        } else {
            LOGW("display", "Display nicht gefunden (%d/%d)", i, attempts);
        }
        Sleep(1000);
    }
    throw std::runtime_error("Display nicht erreichbar");
}

void SerialDisplay::close_port() {
    if (handle_) CloseHandle(handle_);
    handle_ = nullptr;
}

void SerialDisplay::write(const std::string& data) {
    if (!handle_) throw std::runtime_error("Display nicht verbunden");
    size_t off = 0;
    while (off < data.size()) {
        DWORD n = 0;
        if (!WriteFile(handle_, data.data() + off, (DWORD)(data.size() - off), &n, nullptr) || n == 0)
            throw std::runtime_error(strprintf("Schreiben fehlgeschlagen (%lu)", GetLastError()));
        off += n;
    }
}

void SerialDisplay::hello() {
    // Dieses Display antwortet nicht; eine Antwort wird nur geloggt
    write(std::string(6, (char)HELLO));
    char buf[6];
    DWORD n = 0;
    ReadFile(handle_, buf, 6, &n, nullptr);
    PurgeComm(handle_, PURGE_RXCLEAR);
    std::string hex;
    for (DWORD i = 0; i < n; ++i) hex += strprintf("%02x", (unsigned char)buf[i]);
    LOGI("display", "Display-Antwort: %s", hex.empty() ? "(keine, normal für dieses Modell)" : hex.c_str());
}

void SerialDisplay::set_brightness(int level) {
    level = std::clamp(level, 0, 100);
    write(encode(SET_BRIGHTNESS, (int)(255 - level / 100.0 * 255)));
}

void SerialDisplay::set_orientation() {
    auto [w, h] = size();
    std::string buf(16, '\0');
    buf.replace(0, 6, encode(SET_ORIENTATION));
    buf[6] = (char)((int)orientation_ + 100);
    buf[7] = (char)(w >> 8);
    buf[8] = (char)(w & 255);
    buf[9] = (char)(h >> 8);
    buf[10] = (char)(h & 255);
    write(buf);
}

void SerialDisplay::connect() {
    std::lock_guard lk(mu_);
    open();
    if (reset_on_start_) {
        // Wie die Referenz: Reset, danach meldet sich das Gerät ggf. an neuem Port
        write(encode(RESET));
        close_port();
        Sleep(5000);
        open();
    }
    hello();
    write(encode(SCREEN_ON));
    set_brightness(brightness_);
    set_orientation();
    last_frame_.reset();
}

void SerialDisplay::screen_off() {
    std::lock_guard lk(mu_);
    if (handle_) write(encode(SCREEN_OFF));
}

void SerialDisplay::close() {
    std::lock_guard lk(mu_);
    close_port();
}

void SerialDisplay::invalidate() {
    std::lock_guard lk(mu_);
    last_frame_.reset();
}

void SerialDisplay::blit(const Image& img, int y0, int y1) {
    int w = img.w;
    write(encode(DISPLAY_BITMAP, 0, y0, w - 1, y1 - 1));
    std::string data = rgb565le(img, y0, y1);
    size_t step = (size_t)size().first * 8;  // gleiche Blockgröße wie die Referenz
    for (size_t i = 0; i < data.size(); i += step) write(data.substr(i, step));
}

int SerialDisplay::show_locked(const Image& frame) {
    int w = frame.w, h = frame.h;
    if (!last_frame_) {
        blit(frame, 0, h);
        last_frame_ = frame;
        return h;
    }
    const Image& old = *last_frame_;
    int bh = band_height_;
    std::vector<int> changed;
    for (int y = 0; y < h; y += bh) {
        int end = std::min(h, y + bh);
        if (memcmp(&frame.px[size_t(y) * w], &old.px[size_t(y) * w], size_t(end - y) * w * 4) != 0) changed.push_back(y);
    }
    int sent = 0;
    // Zusammenhängende Bänder zu einem Rechteck zusammenfassen
    for (size_t i = 0; i < changed.size(); ++i) {
        int start = changed[i], end = start + bh;
        while (i + 1 < changed.size() && changed[i + 1] == end) {
            ++i;
            end += bh;
        }
        end = std::min(h, end);
        blit(frame, start, end);
        sent += end - start;
    }
    last_frame_ = frame;
    return sent;
}

int SerialDisplay::show(const Image& frame) {
    auto [w, h] = size();
    if (frame.w != w || frame.h != h) throw std::runtime_error("Bildgröße passt nicht zum Display");
    std::lock_guard lk(mu_);
    if (!handle_) open();
    try {
        return show_locked(frame);
    } catch (const std::exception& e) {
        LOGW("display", "Übertragung fehlgeschlagen (%s) – verbinde neu", e.what());
        close_port();
        last_frame_.reset();
        open();
        set_orientation();
        return show_locked(frame);
    }
}

}  // namespace fl
