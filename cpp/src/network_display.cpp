// © 2026 FuchsLabs
// Windows-Teil des Companion-Ausgangs: Heimnetz-Adressen, mDNS-Ankündigung (DnsServiceRegister, Windows 10 1809+),
// JPEG über WIC, Geräteschlüssel per DPAPI. Protokoll, TLS und Kopplung: companion.cpp / companion_server.cpp.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <objbase.h>

#include <atomic>
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

// Lebensdauer: Windows ruft mdns_done auf einem eigenen Thread auf, auch noch nach unseren Wartezeiten (2 s) und
// nach der Abmeldung. Deshalb zählt jede noch offene Anfrage (Registrierung, Abmeldung) eine Referenz; erst wenn
// der letzte Rückruf durch ist und NetworkDisplay losgelassen hat, wird freigegeben. dnsapi.dll bleibt geladen
// (Systembibliothek): würde sie entladen, während ein verspäteter Rückruf läuft, stünde dessen Code nicht mehr da.
struct Mdns {
    enum class Registration : int { pending, active, failed };
    enum class Operation : int { register_service, unregister_service };
    struct CallbackContext { Mdns* owner; Operation operation; HANDLE done; };
    FnRegister reg = nullptr, dereg = nullptr;
    FnFree free_instance = nullptr;
    SvcInstance* instance = nullptr;
    SvcRegisterRequest req{}, dereg_req{};
    HANDLE register_done = nullptr, deregister_done = nullptr;
    CallbackContext register_context{}, deregister_context{};
    DWORD ip = 0;
    std::atomic<int> refs{1};  // 1 = NetworkDisplay
    std::atomic<Registration> registration{Registration::pending};
};

void mdns_release(Mdns* m) {
    if (--m->refs != 0) return;
    if (m->instance && m->free_instance) m->free_instance(m->instance);
    if (m->register_done) CloseHandle(m->register_done);
    if (m->deregister_done) CloseHandle(m->deregister_done);
    delete m;
}

VOID WINAPI mdns_done(DWORD status, PVOID ctx, SvcInstance* inst) {
    auto* callback = static_cast<Mdns::CallbackContext*>(ctx);
    auto* m = callback->owner;
    if (callback->operation == Mdns::Operation::register_service) {
        m->registration.store(status == ERROR_SUCCESS ? Mdns::Registration::active : Mdns::Registration::failed);
        if (status == ERROR_SUCCESS) LOGI("companion", "mDNS-Ankündigung erfolgreich");
        else LOGW("companion", "mDNS-Ankündigung fehlgeschlagen (%lu) – Verbindung über die IP-Adresse", status);
    } else if (status != ERROR_SUCCESS) {
        LOGW("companion", "mDNS-Abmeldung fehlgeschlagen (%lu)", status);
    }
    if (inst && inst != m->instance && m->free_instance) m->free_instance(inst);
    if (callback->done) SetEvent(callback->done);
    mdns_release(m);  // die Referenz der Anfrage, die hiermit fertig ist
}

HMODULE dnsapi() {
    static HMODULE dll = LoadLibraryExW(L"dnsapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);  // nie entladen
    return dll;
}

}  // namespace

struct NetworkDisplay::DeviceFrame {
    Image image;
    uint64_t version = 0;
    std::shared_ptr<const companion::Server::Frame> jpeg;  // zu version passend, träge erzeugt
    ojson navigation;                                       // Seitenposition für die Statusanzeige
};

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
    auto [w, h] = size();
    opt.w = w;
    opt.h = h;
    companion::Server::Hooks hooks;
    hooks.encode_frame = [this] {
        std::optional<Image> image;
        { std::lock_guard lk(mu_); image = frame_; }
        if (!image || image->empty()) return companion::Server::Frame{};
        return companion::Server::Frame{encode_jpeg(*image, 0.8f), image->w, image->h};
    };
    hooks.on_resize = [this](int w, int h) { request_size(w, h); };
    hooks.on_swipe = [this](int dir) { if (on_swipe) on_swipe(dir); };
    hooks.on_touch_frame = [this](int x, int y, int w, int h) {
        std::lock_guard lk(tap_mu_);
        if (taps_.size() < 8) taps_.push_back({"", false, x, y, w, h});
        tap_cv_.notify_one();
    };
    hooks.independent = [this] { return independent_.load(); };
    hooks.encode_frame_for = [this](const std::string& device) {
        std::shared_ptr<DeviceFrame> slot;
        Image image;
        uint64_t version = 0;
        {
            std::lock_guard lk(mu_);
            auto it = device_frames_.find(device);
            if (it == device_frames_.end() || it->second->image.empty()) return companion::Server::Frame{};
            slot = it->second;
            if (slot->jpeg && slot->jpeg->version == slot->version) return *slot->jpeg;
            image = slot->image;
            version = slot->version;
        }
        auto jpeg = std::make_shared<const companion::Server::Frame>(
            companion::Server::Frame{encode_jpeg(image, 0.8f), image.w, image.h, version});
        {
            std::lock_guard lk(mu_);
            if (slot->version == version) slot->jpeg = jpeg;
        }
        return *jpeg;
    };
    hooks.on_resize_for = [this](const std::string& device, int w, int h) { if (on_device_resize) on_device_resize(device, w, h); };
    hooks.on_swipe_for = [this](const std::string& device, int dir) { if (on_device_swipe) on_device_swipe(device, dir); };
    hooks.on_touch_frame_for = [this](const std::string& device, int x, int y, int w, int h) {
        std::lock_guard lk(tap_mu_);
        if (taps_.size() < 8) taps_.push_back({device, true, x, y, w, h});
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
        Tap tap;
        {
            std::unique_lock lk(tap_mu_);
            tap_cv_.wait(lk, [&] { return tap_stop_ || !taps_.empty(); });
            if (tap_stop_) break;
            tap = taps_.front();
            taps_.erase(taps_.begin());
        }
        try {
            if (tap.per_device) { if (on_device_tap) on_device_tap(tap.device, tap.x, tap.y, tap.w, tap.h); }
            else if (on_tap) on_tap(tap.x, tap.y, tap.w, tap.h);
        } catch (const std::exception& e) {
            LOGW("companion", "Touch: %s", e.what());
        }
    }
    CoUninitialize();
}

std::pair<int, int> NetworkDisplay::size() const {
    std::lock_guard lk(mu_); return {w_, h_};
}

void NetworkDisplay::request_size(int w, int h) {
    { std::lock_guard lk(mu_); if (w_ == w && h_ == h) return; w_ = w; h_ = h; }
    if (on_resize) on_resize();
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

void NetworkDisplay::set_independent(bool on) {
    if (independent_.exchange(on) != on)
        if (auto srv = server()) srv->frame_changed();   // Clients holen ihr Bild neu
}

void NetworkDisplay::show_for(const std::string& device, const Image& frame, const ojson& navigation) {
    {
        std::lock_guard lk(mu_);
        auto& slot = device_frames_[device];
        if (!slot) slot = std::make_shared<DeviceFrame>();
        slot->navigation = navigation;
        if (slot->image.w == frame.w && slot->image.h == frame.h && slot->image.px == frame.px) return;  // unverändert
        slot->image = frame;
        slot->version = ++frame_version_;
        slot->jpeg.reset();
    }
    if (auto srv = server()) srv->frame_changed();
}

std::optional<Image> NetworkDisplay::frame_for(const std::string& device) {
    std::lock_guard lk(mu_);
    auto it = device_frames_.find(device);
    if (it == device_frames_.end() || it->second->image.empty()) return std::nullopt;
    return it->second->image;
}

void NetworkDisplay::forget_device(const std::string& device) {
    std::lock_guard lk(mu_);
    device_frames_.erase(device);
}

std::vector<std::string> NetworkDisplay::client_ids() {
    std::vector<std::string> ids;
    if (auto srv = server())
        for (const auto& c : srv->clients())
            if (c.contains("id") && c["id"].is_string()) ids.push_back(c["id"].get<std::string>());
    return ids;
}

std::optional<std::vector<std::string>> NetworkDisplay::device_pages(const std::string& id) { return store_->device_pages(id); }

bool NetworkDisplay::set_device_pages(const std::string& id, const std::optional<std::vector<std::string>>& pages) {
    if (!store_->set_device_pages(id, pages)) return false;
    if (on_device_pages) on_device_pages(id);
    return true;
}

void NetworkDisplay::invalidate() {}  // TCP verliert keine Bilder; nach dem Verbinden schickt der Server ohnehin ein Vollbild

void NetworkDisplay::screen_off() {
    {
        std::lock_guard lk(mu_);
        frame_ = Image(frame_ ? frame_->w : w_, frame_ ? frame_->h : h_, Color{0, 0, 0});
        for (auto& [id, slot] : device_frames_) {   // unabhängiger Betrieb: jedes Gerät bekommt sein schwarzes Bild
            if (slot->image.empty()) continue;
            slot->image = Image(slot->image.w, slot->image.h, Color{0, 0, 0});
            slot->version = ++frame_version_;
            slot->jpeg.reset();
        }
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
    std::unique_lock mdns_lock(mdns_mu_);
    if (mdns_.load() || first.empty()) return;
    HMODULE dll = dnsapi();
    auto* m = new Mdns();
    auto construct = dll ? reinterpret_cast<FnConstruct>(GetProcAddress(dll, "DnsServiceConstructInstance")) : nullptr;
    m->reg = dll ? reinterpret_cast<FnRegister>(GetProcAddress(dll, "DnsServiceRegister")) : nullptr;
    m->dereg = dll ? reinterpret_cast<FnRegister>(GetProcAddress(dll, "DnsServiceDeRegister")) : nullptr;
    m->free_instance = dll ? reinterpret_cast<FnFree>(GetProcAddress(dll, "DnsServiceFreeInstance")) : nullptr;
    if (!construct || !m->reg || !m->dereg || !m->free_instance) {
        LOGI("companion", "mDNS-Ankündigung nicht verfügbar (Windows 10 1809 oder neuer nötig) – Verbindung über die IP-Adresse");
        mdns_release(m);
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
    m->register_done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m->deregister_done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m->instance || !m->register_done || !m->deregister_done) {
        LOGW("companion", "mDNS-Ankündigung konnte nicht vorbereitet werden");
        mdns_release(m);
        return;
    }
    m->req.Version = 1;  // DNS_QUERY_REQUEST_VERSION1
    m->req.InterfaceIndex = 0;
    m->req.pServiceInstance = m->instance;
    m->req.pRegisterCompletionCallback = mdns_done;
    m->register_context = {m, Mdns::Operation::register_service, m->register_done};
    m->req.pQueryContext = &m->register_context;
    ++m->refs;  // für den Rückruf der Registrierung
    DWORD rc = m->reg(&m->req, nullptr);
    if (rc != DNS_PENDING) mdns_release(m);  // kein Rückruf zu erwarten
    if (rc != DNS_PENDING && rc != ERROR_SUCCESS) {
        m->registration.store(Mdns::Registration::failed);
        LOGW("companion", "mDNS-Ankündigung fehlgeschlagen (%lu) – Verbindung über die IP-Adresse", rc);
        mdns_release(m);
        return;
    }
    if (rc == ERROR_SUCCESS) m->registration.store(Mdns::Registration::active);
    if (rc == DNS_PENDING) WaitForSingleObject(m->register_done, 2000);  // verspätete Rückrufe halten ihre Referenz
    mdns_ = m;
    if (m->registration.load() == Mdns::Registration::active) LOGI("companion", "mDNS: %s", narrow(instance).c_str());
}

void NetworkDisplay::mdns_unregister() {
    Mdns* m = nullptr;
    {
        std::lock_guard lk(mdns_mu_);
        m = static_cast<Mdns*>(mdns_.exchange(nullptr));
    }
    if (!m) return;
    ResetEvent(m->deregister_done);
    m->deregister_context = {m, Mdns::Operation::unregister_service, m->deregister_done};
    m->dereg_req = m->req;
    m->dereg_req.pQueryContext = &m->deregister_context;
    ++m->refs;  // für den Rückruf der Abmeldung
    if (m->dereg(&m->dereg_req, nullptr) == DNS_PENDING) WaitForSingleObject(m->deregister_done, 2000);
    else mdns_release(m);
    mdns_release(m);  // freigegeben wird erst, wenn auch verspätete Rückrufe durch sind
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
    {
        std::lock_guard lk(mdns_mu_);
        auto* mdns = static_cast<Mdns*>(mdns_.load());
        out["mdns"] = mdns && mdns->registration.load() == Mdns::Registration::active;
    }
    out["devices"] = store_->list();
    if (up) {
        out["fingerprint"] = companion::format_fingerprint(srv->fingerprint());
        out["clients"] = srv->clients();
        if (independent_) {   // Seitenposition je Gerät (nur im unabhängigen Betrieb)
            std::lock_guard lk(mu_);
            for (auto& cl : out["clients"]) {
                auto it = device_frames_.find(cl.value("id", ""));
                if (it != device_frames_.end() && it->second->navigation.is_object()) cl["navigation"] = it->second->navigation;
            }
        }
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
    if (ok) forget_device(id);
    return ok;
}

}  // namespace fl
