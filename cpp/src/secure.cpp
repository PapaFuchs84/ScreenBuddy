// © 2026 FuchsLabs
#include "secure.h"

#include <windows.h>
#include <dpapi.h>

#include <stdexcept>

namespace fl::secure {

static const char PREFIX[] = "dpapi:";
static const char ENTROPY[] = "FuchsLabs.USB-Display.v1";  // zusätzliche App-Bindung, wie in Version 1.x

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::string& in) {
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        unsigned v = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8 | (unsigned char)in[i + 2];
        out += B64[v >> 18 & 63]; out += B64[v >> 12 & 63]; out += B64[v >> 6 & 63]; out += B64[v & 63];
    }
    if (i + 1 == in.size()) {
        unsigned v = (unsigned char)in[i] << 16;
        out += B64[v >> 18 & 63]; out += B64[v >> 12 & 63]; out += "==";
    } else if (i + 2 == in.size()) {
        unsigned v = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8;
        out += B64[v >> 18 & 63]; out += B64[v >> 12 & 63]; out += B64[v >> 6 & 63]; out += '=';
    }
    return out;
}

std::string base64_decode(const std::string& text, bool* ok) {
    auto val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::string out;
    unsigned buf = 0;
    int bits = 0, pad = 0;
    bool good = text.size() % 4 == 0;
    for (char c : text) {
        if (c == '=') { ++pad; continue; }
        int v = val(c);
        if (v < 0 || pad) { good = false; break; }
        buf = buf << 6 | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += char(buf >> bits & 0xFF);
        }
    }
    if (pad > 2) good = false;
    if (ok) *ok = good;
    return good ? out : std::string();
}

bool is_protected(const std::string& value) { return value.rfind(PREFIX, 0) == 0; }

std::string protect(const std::string& text) { return protect(text, std::string(ENTROPY, sizeof ENTROPY - 1)); }

std::string protect(const std::string& text, const std::string& entropy) {
    if (text.empty()) return "";
    DATA_BLOB src{(DWORD)text.size(), (BYTE*)text.data()};
    DATA_BLOB ent{(DWORD)entropy.size(), (BYTE*)entropy.data()};
    DATA_BLOB out{};
    // UI_FORBIDDEN: nie einen Dialog öffnen (läuft auch in Hintergrund-Threads).
    if (!CryptProtectData(&src, L"FuchsLabs", &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        throw std::runtime_error("Verschlüsselung fehlgeschlagen");
    std::string raw((const char*)out.pbData, out.cbData);
    LocalFree(out.pbData);
    return PREFIX + base64_encode(raw);
}

std::string unprotect(const std::string& value) { return unprotect(value, std::string(ENTROPY, sizeof ENTROPY - 1)); }

std::string unprotect(const std::string& value, const std::string& entropy) {
    if (!is_protected(value)) return value;
    bool ok = false;
    std::string raw = base64_decode(value.substr(sizeof PREFIX - 1), &ok);
    if (!ok) return "";
    DATA_BLOB src{(DWORD)raw.size(), (BYTE*)raw.data()};
    DATA_BLOB ent{(DWORD)entropy.size(), (BYTE*)entropy.data()};
    DATA_BLOB out{};
    if (!CryptUnprotectData(&src, nullptr, &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) return "";
    std::string plain((const char*)out.pbData, out.cbData);
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    return plain;
}

}  // namespace fl::secure
