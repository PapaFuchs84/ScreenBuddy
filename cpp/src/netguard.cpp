// © 2026 FuchsLabs
#include "netguard.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <cstdint>

#include "http.h"
#include "util.h"

namespace fl::netguard {

namespace {

struct WsaInit {
    WsaInit() {
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
    }
} g_wsa;

bool v4_global(uint32_t a) {  // a in Host-Reihenfolge
    struct Net { uint32_t net; int bits; };
    static const Net blocked[] = {
        {0x00000000, 8},  {0x0A000000, 8},   {0x64400000, 10}, {0x7F000000, 8},  {0xA9FE0000, 16},
        {0xAC100000, 12}, {0xC0000000, 24},  {0xC0000200, 24}, {0xC0A80000, 16}, {0xC6120000, 15},
        {0xC6336400, 24}, {0xCB007100, 24},  {0xE0000000, 4},  {0xF0000000, 4},
    };
    for (auto& n : blocked) {
        uint32_t mask = n.bits ? 0xFFFFFFFFu << (32 - n.bits) : 0;
        if ((a & mask) == n.net) return false;
    }
    return true;
}

bool v6_global(const std::array<uint8_t, 16>& b) {
    auto prefix = [&](std::initializer_list<uint8_t> p, int bits) {
        int i = 0;
        for (uint8_t v : p) {
            int take = std::min(8, bits - i * 8);
            if (take <= 0) break;
            uint8_t mask = (uint8_t)(0xFF << (8 - take));
            if ((b[i] & mask) != (v & mask)) return false;
            ++i;
        }
        return true;
    };
    bool zero10 = true;
    for (int i = 0; i < 10; ++i) zero10 &= b[i] == 0;
    if (zero10 && b[10] == 0xFF && b[11] == 0xFF)  // ::ffff:a.b.c.d – eingebettete IPv4 prüfen
        return v4_global((uint32_t)b[12] << 24 | b[13] << 16 | b[14] << 8 | b[15]);
    bool all_zero = zero10 && b[10] == 0 && b[11] == 0 && b[12] == 0 && b[13] == 0 && b[14] == 0;
    if (all_zero && (b[15] == 0 || b[15] == 1)) return false;  // :: und ::1
    if (zero10 && b[10] == 0 && b[11] == 0) return false;      // ::a.b.c.d (veraltet)
    if (prefix({0xFE, 0x80}, 10) || prefix({0xFE, 0xC0}, 10)) return false;  // link-/site-local
    if (prefix({0xFC}, 7)) return false;                        // eindeutige lokale Adressen (ULA)
    if (prefix({0xFF}, 8)) return false;                        // Multicast
    if (prefix({0x20, 0x01, 0x0D, 0xB8}, 32)) return false;     // Dokumentation
    if (prefix({0x20, 0x01}, 23)) return false;                 // IETF-Sonderbereich (Teredo u. a.)
    if (prefix({0x01, 0x00, 0, 0, 0, 0, 0, 0}, 64)) return false;  // Verwerfen
    if (prefix({0x00, 0x64, 0xFF, 0x9B}, 32)) {                 // NAT64: eingebettete IPv4 prüfen
        return v4_global((uint32_t)b[12] << 24 | b[13] << 16 | b[14] << 8 | b[15]);
    }
    if (prefix({0x20, 0x02}, 16))  // 6to4: eingebettete IPv4 prüfen
        return v4_global((uint32_t)b[2] << 24 | b[3] << 16 | b[4] << 8 | b[5]);
    return true;
}

bool sockaddr_global(const sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        auto* s4 = (const sockaddr_in*)sa;
        return v4_global(ntohl(s4->sin_addr.s_addr));
    }
    if (sa->sa_family == AF_INET6) {
        auto* s6 = (const sockaddr_in6*)sa;
        std::array<uint8_t, 16> b;
        memcpy(b.data(), &s6->sin6_addr, 16);
        return v6_global(b);
    }
    return false;
}

}  // namespace

bool is_global_ip(const std::string& ip) {
    sockaddr_storage ss{};
    int len = sizeof ss;
    std::wstring w = widen(ip);
    if (WSAStringToAddressW(w.data(), AF_INET, nullptr, (sockaddr*)&ss, &len) == 0) return sockaddr_global((sockaddr*)&ss);
    len = sizeof ss;
    if (WSAStringToAddressW(w.data(), AF_INET6, nullptr, (sockaddr*)&ss, &len) == 0) return sockaddr_global((sockaddr*)&ss);
    return false;
}

bool public_url(const std::string& url) {
    http::Url u = http::parse_url(url);
    if (!u.valid) return false;
    std::string host = u.host;
    if (!host.empty() && host.front() == '[' && host.back() == ']') host = host.substr(1, host.size() - 2);
    ADDRINFOW hints{};
    hints.ai_family = AF_UNSPEC;
    ADDRINFOW* res = nullptr;
    if (GetAddrInfoW(widen(host).c_str(), nullptr, &hints, &res) != 0 || !res) return false;
    bool ok = true;
    for (auto* p = res; p; p = p->ai_next)
        if (!sockaddr_global(p->ai_addr)) {
            ok = false;
            break;
        }
    FreeAddrInfoW(res);
    return ok;
}

}  // namespace fl::netguard
