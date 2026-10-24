// © 2026 FuchsLabs
// Windows-Teil des Companion-Ausgangs: Heimnetz-Adressen, mDNS-Ankündigung (DnsServiceRegister, Windows 10 1809+),
// JPEG über WIC, Geräteschlüssel per DPAPI. Protokoll, TLS und Kopplung: companion.cpp / companion_server.cpp.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <objbase.h>

#include <ctime>

#include "companion.h"
#include "companion_server.h"
#include "output.h"
#include "secure.h"
#include "util.h"

namespace fl {

namespace {

// Heimnetz-IPv4-Adressen aktiver Adapter; Adapter mit Standard-Gateway (also das eigentliche WLAN/LAN) zuerst
std::vector<std::string> lan_addresses() {
    std::vector<std::string> with_gw, without;
    ULONG len = 16 * 1024;
    std::vector<unsigned char> buf;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG rc = ERROR_BUFFER_OVERFLOW;
    for (int i = 0; i < 3 && rc == ERROR_BUFFER_OVERFLOW; ++i) {
        buf.resize(len);
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()), &len);
    }
    if (rc != NO_ERROR) return {};
    for (auto* a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data()); a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
            char ip[INET_ADDRSTRLEN] = "";
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(u->Address.lpSockaddr)->sin_addr, ip, sizeof ip);
            if (!companion::is_lan_ipv4(ip)) continue;
            (a->FirstGatewayAddress ? with_gw : without).push_back(ip);
        }
    }
    with_gw.insert(with_gw.end(), without.begin(), without.end());
    return with_gw;
}

std::wstring computer_name() {
    wchar_t buf[256];
    DWORD n = 256;
    if (GetComputerNameExW(ComputerNameDnsHostname, buf, &n) && n) return std::wstring(buf, n);
    return L"FuchsLabs-PC";
}

// ---------- mDNS (dnsapi.dll, zur Laufzeit geladen: fehlt die Funktion, läuft alles ohne Ankündigung) ----------
// Strukturen wie in windns.h (DNS_SERVICE_INSTANCE / DNS_SERVICE_REGISTER_REQUEST), eigene Namen gegen Kollisionen.
struct SvcInstance {
    LPWSTR pszInstanceName;
    LPWSTR pszHostName;
    DWORD* ip4Address;
    void* ip6Address;
    WORD wPort, wPriority, wWeight;
    DWORD dwPropertyCount;
    PWSTR* keys;
    PWSTR* values;
    DWORD dwInterfaceIndex;
};
using SvcComplete = VOID(WINAPI*)(DWORD, PVOID, SvcInstance*);
struct SvcRegisterRequest {
    ULONG Version;
    ULONG InterfaceIndex;
    SvcInstance* pServiceInstance;
    SvcComplete pRegisterCompletionCallback;
    PVOID pQueryContext;
    HANDLE hCredentials;
    BOOL unicastEnabled;
};
using FnConstruct = SvcInstance*(WINAPI*)(PCWSTR, PCWSTR, DWORD*, void*, WORD, WORD, WORD, DWORD, PCWSTR*, PCWSTR*);
using FnRegister = DWORD(WINAPI*)(SvcRegisterRequest*, void*);
using FnFree = VOID(WINAPI*)(SvcInstance*);
constexpr DWORD DNS_PENDING = 9506;  // DNS_REQUEST_PENDING

struct Mdns {
    HMODULE dll = nullptr;
    FnRegister reg = nullptr, dereg = nullptr;
    FnFree free_instance = nullptr;
    SvcInstance* instance = nullptr;
    SvcRegisterRequest req{};
    HANDLE done = nullptr;
    DWORD ip = 0;
};

VOID WINAPI mdns_done(DWORD status, PVOID ctx, SvcInstance* inst) {
    auto* m = static_cast<Mdns*>(ctx);
    if (status != ERROR_SUCCESS) LOGW("companion", "mDNS-Ankündigung: Fehler %lu", status);
    if (inst && inst != m->instance && m->free_instance) m->free_instance(inst);
    if (m->done) SetEvent(m->done);
}

}  // namespace

NetworkDisplay::NetworkDisplay(int port, int settings_port, std::filesystem::path store, int w, int h)
    : port_(port), settings_port_(settings_port), w_(w), h_(h), store_path_(std::move(store)) {
    store_ = std::make_unique<companion::Store>(
        store_path_, [](const std::string& s) { return secure::protect(s); }, [](const std::string& s) { return secure::unprotect(s); },
        [] { return (double)std::time(nullptr); });
    try {
        store_->load();
    } catch (const std::exception& e) {
        LOGW("companion", "Kopplungen nicht lesbar: %s", e.what());
    }
}

NetworkDisplay::~NetworkDisplay() { close(); }

std::shared_ptr<companion::Server> NetworkDisplay::server() {
    std::lock_guard lk(mu_);
    return server_;
}

bool NetworkDisplay::running() {
    auto srv = server();
    return srv && srv->running();
}

std::string NetworkDisplay::error() {
    std::lock_guard lk(mu_);
    return error_;
}

void NetworkDisplay::connect() {
    if (running()) return;
    auto fail = [&](const std::string& msg) {
        std::lock_guard lk(mu_);
        error_ = msg;
        throw std::runtime_error(msg);
    };
    if (!companion::valid_port(port_, settings_port_)) fail("Ungültiger Port für die Companion-App");
    auto addrs = lan_addresses();
    companion::Server::Options opt;
    opt.bind = addrs;
    opt.port = port_;
    opt.settings_port = settings_port_;
    opt.w = w_;
    opt.h = h_;
    companion::Server::Hooks hooks;
    hooks.encode = [this] { return encode_current(); };
    hooks.on_touch = [this](int x, int y) {
        std::lock_guard lk(tap_mu_);
        if (taps_.size() < 8) taps_.emplace_back(x, y);
        tap_cv_.notify_one();
    };
    hooks.log = [](int lv, const std::string& msg) {
        if (lv >= 2) LOGW("companion", "%s", msg.c_str());
        else if (lv == 1) LOGI("companion", "%s", msg.c_str());
        else LOGD("companion", "%s", msg.c_str());
    };
    hooks.now = [] { return mono(); };
    auto srv = std::make_shared<companion::Server>(opt, *store_, hooks);
    try {
        srv->start();
    } catch (const std::exception& e) {
        fail(e.what());
    }
    {
        std::lock_guard lk(mu_);
        server_ = srv;
        addresses_ = srv->options().bind;  // nur die Adressen, an die der Server tatsächlich gebunden ist
        error_.clear();
    }
    if (!tap_thread_.joinable()) {
        tap_stop_ = false;
        tap_thread_ = std::thread([this] { tap_loop(); });
    }
    mdns_register();
}

void NetworkDisplay::tap_loop() {
    // Touch-Ziele öffnen Browser/Apps per ShellExecute: dafür ein STA-Thread wie beim Fenster des Bildschirm-Ausgangs
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    for (;;) {
        std::pair<int, int> tap;
        {
            std::unique_lock lk(tap_mu_);
            tap_cv_.wait(lk, [&] { return tap_stop_ || !taps_.empty(); });
            if (tap_stop_) break;
            tap = taps_.front();
            taps_.erase(taps_.begin());
        }
        try {
            if (on_tap) on_tap(tap.first, tap.second);
        } catch (const std::exception& e) {
            LOGW("companion", "Touch: %s", e.what());
        }
    }
    CoUninitialize();
}

std::string NetworkDisplay::encode_current() {
    std::optional<Image> img;
    {
        std::lock_guard lk(mu_);
        img = frame_;
    }
    if (!img || img->empty()) return "";
    return encode_jpeg(*img, 0.8f);
}

int NetworkDisplay::show(const Image& frame) {
    {
        std::lock_guard lk(mu_);
        if (frame_ && frame_->w == frame.w && frame_->h == frame.h && frame_->px == frame.px) return 0;  // unverändert
        frame_ = frame;
    }
    if (auto srv = server()) srv->frame_changed();
    return frame.h;
}

void NetworkDisplay::invalidate() {}  // TCP verliert keine Bilder; nach dem Verbinden schickt der Server ohnehin ein Vollbild

void NetworkDisplay::screen_off() {
    {
        std::lock_guard lk(mu_);
        frame_ = Image(w_, h_, Color{0, 0, 0});
    }
    auto srv = server();
    if (srv && srv->running()) {
        srv->frame_changed();
        if (!srv->clients().empty()) Sleep(400);  // Clients Zeit geben, das schwarze Bild noch abzuholen
    }
}

void NetworkDisplay::close() {
    mdns_unregister();
    if (auto srv = server()) srv->stop();
    {
        std::lock_guard lk(tap_mu_);
        tap_stop_ = true;
    }
    tap_cv_.notify_all();
    if (tap_thread_.joinable()) tap_thread_.join();
}

void NetworkDisplay::mdns_register() {
    std::string first;
    {
        std::lock_guard lk(mu_);
        if (!addresses_.empty()) first = addresses_[0];
    }
    if (mdns_ || first.empty()) return;
    auto* m = new Mdns();
    m->dll = LoadLibraryExW(L"dnsapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    auto construct = m->dll ? reinterpret_cast<FnConstruct>(GetProcAddress(m->dll, "DnsServiceConstructInstance")) : nullptr;
    m->reg = m->dll ? reinterpret_cast<FnRegister>(GetProcAddress(m->dll, "DnsServiceRegister")) : nullptr;
    m->dereg = m->dll ? reinterpret_cast<FnRegister>(GetProcAddress(m->dll, "DnsServiceDeRegister")) : nullptr;
    m->free_instance = m->dll ? reinterpret_cast<FnFree>(GetProcAddress(m->dll, "DnsServiceFreeInstance")) : nullptr;
    if (!construct || !m->reg || !m->dereg || !m->free_instance) {
        LOGI("companion", "mDNS-Ankündigung nicht verfügbar (Windows 10 1809 oder neuer nötig) – Verbindung über die IP-Adresse");
        if (m->dll) FreeLibrary(m->dll);
        delete m;
        return;
    }
    std::wstring pc = computer_name();
    std::wstring instance = pc + L"." + widen(companion::SERVICE_TYPE) + L".local";
    std::wstring host = pc + L".local";
    in_addr a{};
    inet_pton(AF_INET, first.c_str(), &a);
    m->ip = a.s_addr;
    std::wstring k1 = L"v", v1 = std::to_wstring(companion::PROTOCOL_VERSION), k2 = L"name", v2 = pc;
    PCWSTR keys[] = {k1.c_str(), k2.c_str()};
    PCWSTR values[] = {v1.c_str(), v2.c_str()};
    m->instance = construct(instance.c_str(), host.c_str(), &m->ip, nullptr, (WORD)port_, 0, 0, 2, keys, values);
    if (!m->instance) {
        LOGW("companion", "mDNS-Ankündigung konnte nicht vorbereitet werden");
        FreeLibrary(m->dll);
        delete m;
        return;
    }
    m->done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m->req.Version = 1;  // DNS_QUERY_REQUEST_VERSION1
    m->req.InterfaceIndex = 0;
    m->req.pServiceInstance = m->instance;
    m->req.pRegisterCompletionCallback = mdns_done;
    m->req.pQueryContext = m;
    DWORD rc = m->reg(&m->req, nullptr);
    if (rc != DNS_PENDING && rc != ERROR_SUCCESS) {
        LOGW("companion", "mDNS-Ankündigung fehlgeschlagen (%lu) – Verbindung über die IP-Adresse", rc);
        m->free_instance(m->instance);
        CloseHandle(m->done);
        FreeLibrary(m->dll);
        delete m;
        return;
    }
    WaitForSingleObject(m->done, 2000);
    mdns_ = m;
    LOGI("companion", "mDNS: %s", narrow(instance).c_str());
}

void NetworkDisplay::mdns_unregister() {
    auto* m = static_cast<Mdns*>(mdns_);
    if (!m) return;
    mdns_ = nullptr;
    ResetEvent(m->done);
    if (m->dereg(&m->req, nullptr) == DNS_PENDING) WaitForSingleObject(m->done, 2000);
    m->free_instance(m->instance);
    CloseHandle(m->done);
    FreeLibrary(m->dll);
    delete m;
}

ojson NetworkDisplay::status() {
    auto srv = server();
    bool up = srv && srv->running();
    ojson out = {{"enabled", true}, {"running", up}, {"error", error()}, {"port", port_}};
    ojson addrs = ojson::array();
    {
        std::lock_guard lk(mu_);
        for (auto& a : addresses_) addrs.push_back(a);
    }
    out["addresses"] = addrs;
    out["mdns"] = mdns_ != nullptr;
    out["devices"] = store_->list();
    if (up) {
        out["fingerprint"] = companion::format_fingerprint(srv->fingerprint());
        out["clients"] = srv->clients();
        auto code = srv->pairing().active();
        out["pairing"] = code ? ojson{{"active", true}, {"expires_in", (int)std::max(0.0, code->expires - mono())}} : ojson{{"active", false}};
        out["locked_for"] = (int)srv->pairing().locked_for();
    } else {
        out["clients"] = ojson::array();
        out["pairing"] = {{"active", false}};
        out["locked_for"] = 0;
    }
    return out;
}

ojson NetworkDisplay::start_pairing() {
    auto srv = server();
    std::string host;
    {
        std::lock_guard lk(mu_);
        if (!addresses_.empty()) host = addresses_[0];
    }
    if (!srv || !srv->running() || host.empty())
        return {{"ok", false}, {"error", "Der Companion-Server läuft nicht – Companion einschalten und speichern"}};
    auto code = srv->pairing().start();
    if (!code) {
        int minutes = (int)(srv->pairing().locked_for() / 60) + 1;
        return {{"ok", false}, {"error", strprintf("Zu viele Fehlversuche – Kopplung in %d Min. wieder möglich", minutes)}};
    }
    std::string fp = srv->fingerprint();
    std::string uri = companion::pair_uri(host, port_, fp, code->pin);
    LOGI("companion", "Kopplungscode angezeigt (gültig 2 Minuten)");
    return {{"ok", true}, {"pin", code->pin}, {"uri", uri},
            {"qr", "data:image/svg+xml;base64," + companion::base64(companion::qr_svg(uri))},
            {"expires_in", (int)companion::PIN_SECONDS}, {"host", host}, {"port", port_},
            {"fingerprint", companion::format_fingerprint(fp)}};
}

void NetworkDisplay::cancel_pairing() {
    if (auto srv = server()) srv->pairing().cancel();
}

bool NetworkDisplay::remove_device(const std::string& id) {
    bool ok = store_->remove(id);
    if (auto srv = server(); ok && srv) srv->drop_device(id);
    return ok;
}

}  // namespace fl
