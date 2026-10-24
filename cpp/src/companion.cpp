// © 2026 FuchsLabs
#include "companion.h"
#include <cmath>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "qrcodegen.hpp"

namespace fl::companion {

// ---------- Krypto ----------
std::string random_bytes(size_t n) {
    std::string out(n, '\0');
    if (n && RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), (int)n) != 1)
        throw std::runtime_error("Zufallsgenerator nicht verfügbar");
    return out;
}

std::string base64(const std::string& data) {
    std::string out(4 * ((data.size() + 2) / 3) + 1, '\0');
    int n = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(out.data()), reinterpret_cast<const unsigned char*>(data.data()),
                            (int)data.size());
    out.resize(n < 0 ? 0 : (size_t)n);
    return out;
}

std::string unbase64(const std::string& text, bool* ok) {
    auto fail = [&] {
        if (ok) *ok = false;
        return std::string();
    };
    if (text.empty()) {
        if (ok) *ok = true;
        return {};
    }
    if (text.size() % 4 != 0 || text.size() > 1 << 20) return fail();
    for (char c : text)
        if (!(isalnum((unsigned char)c) || c == '+' || c == '/' || c == '=')) return fail();
    size_t pad = 0;
    if (text.back() == '=') ++pad;
    if (text.size() >= 2 && text[text.size() - 2] == '=') ++pad;
    if (text.find('=') < text.size() - pad) return fail();
    std::string out(text.size() / 4 * 3, '\0');
    int n = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(out.data()), reinterpret_cast<const unsigned char*>(text.data()),
                            (int)text.size());
    if (n < 0 || (size_t)n < pad) return fail();
    out.resize((size_t)n - pad);
    if (ok) *ok = true;
    return out;
}

std::string sha256_hex(const std::string& data) {
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), md);
    static const char hex[] = "0123456789abcdef";
    std::string out;
    for (unsigned char c : md) {
        out += hex[c >> 4];
        out += hex[c & 15];
    }
    return out;
}

bool equal_ct(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

std::string random_pin() {
    // Verwerfen statt Modulo: gleichverteilt über 000000…999999
    for (;;) {
        std::string r = random_bytes(4);
        uint32_t v = (uint32_t)(unsigned char)r[0] << 24 | (uint32_t)(unsigned char)r[1] << 16 |
                     (uint32_t)(unsigned char)r[2] << 8 | (unsigned char)r[3];
        if (v >= 4294000000u) continue;  // 4294000000 ist durch 10^6 teilbar
        char buf[8];
        std::snprintf(buf, sizeof buf, "%06u", v % 1000000u);
        return buf;
    }
}

// ---------- Netz ----------
static std::optional<uint32_t> parse_v4(const std::string& ip) {
    uint32_t out = 0;
    int parts = 0;
    size_t i = 0;
    while (parts < 4) {
        if (i >= ip.size() || !isdigit((unsigned char)ip[i])) return std::nullopt;
        unsigned v = 0;
        size_t start = i;
        while (i < ip.size() && isdigit((unsigned char)ip[i])) {
            v = v * 10 + (ip[i] - '0');
            if (v > 255 || i - start >= 3) return std::nullopt;
            ++i;
        }
        if (i - start > 1 && ip[start] == '0') return std::nullopt;  // keine führenden Nullen (oktal-Verwechslung)
        out = out << 8 | v;
        ++parts;
        if (parts < 4) {
            if (i >= ip.size() || ip[i] != '.') return std::nullopt;
            ++i;
        }
    }
    if (i != ip.size()) return std::nullopt;
    return out;
}

bool is_lan_ipv4(const std::string& ip) {
    auto a = parse_v4(ip);
    if (!a) return false;
    return (*a & 0xFF000000u) == 0x0A000000u || (*a & 0xFFF00000u) == 0xAC100000u || (*a & 0xFFFF0000u) == 0xC0A80000u;
}

bool is_loopback_ipv4(const std::string& ip) {
    auto a = parse_v4(ip);
    return a && (*a & 0xFF000000u) == 0x7F000000u;
}

bool valid_port(int port, int settings_port) {
    return port >= 1024 && port <= 65535 && port != SETTINGS_PORT && port != settings_port;
}

// ---------- WebSocket ----------
std::string ws_accept(const std::string& client_key) {
    std::string s = client_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char md[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(s.data()), s.size(), md);
    return base64(std::string(reinterpret_cast<const char*>(md), sizeof md));
}

static std::string lower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}
static std::string strip(const std::string& s) {
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
static bool has_token(const std::string& value, const std::string& token) {
    std::stringstream ss(lower(value));
    std::string part;
    while (std::getline(ss, part, ','))
        if (strip(part) == token) return true;
    return false;
}

std::optional<HttpRequest> parse_http_request(const std::string& raw) {
    size_t end = raw.find("\r\n\r\n");
    if (end == std::string::npos) return std::nullopt;
    HttpRequest req;
    size_t pos = 0;
    bool first = true;
    while (pos < end) {
        size_t nl = raw.find("\r\n", pos);
        std::string line = raw.substr(pos, nl - pos);
        pos = nl + 2;
        for (unsigned char c : line)
            if (c < 0x20 && c != '\t') return std::nullopt;
        if (first) {
            first = false;
            size_t a = line.find(' '), b = a == std::string::npos ? a : line.find(' ', a + 1);
            if (b == std::string::npos || line.find(' ', b + 1) != std::string::npos) return std::nullopt;
            req.method = line.substr(0, a);
            req.target = line.substr(a + 1, b - a - 1);
            req.version = line.substr(b + 1);
            continue;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos || colon == 0) return std::nullopt;
        std::string name = lower(line.substr(0, colon));
        if (name.find_first_of(" \t") != std::string::npos) return std::nullopt;
        std::string value = strip(line.substr(colon + 1));
        auto it = req.headers.find(name);
        if (it != req.headers.end()) it->second += ", " + value;
        else req.headers[name] = value;
    }
    if (first) return std::nullopt;
    return req;
}

static std::string http_error(int code, const char* text) {
    return "HTTP/1.1 " + std::to_string(code) + " " + text + "\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
}

Upgrade ws_upgrade(const std::string& raw) {
    Upgrade u;
    auto req = parse_http_request(raw);
    auto bad = [&](const char* why, int code = 400, const char* text = "Bad Request") {
        u.ok = false;
        u.reason = why;
        u.response = http_error(code, text);
        return u;
    };
    if (!req) return bad("kein gültiger HTTP-Request");
    if (req->method != "GET") return bad("Methode", 405, "Method Not Allowed");
    if (req->version != "HTTP/1.1") return bad("HTTP-Version");
    if (req->target.empty() || req->target[0] != '/') return bad("Pfad");
    auto h = [&](const char* k) {
        auto it = req->headers.find(k);
        return it == req->headers.end() ? std::string() : it->second;
    };
    if (req->headers.count("origin")) return bad("Origin-Header (Browser)", 403, "Forbidden");
    if (!has_token(h("upgrade"), "websocket")) return bad("Upgrade fehlt", 426, "Upgrade Required");
    if (!has_token(h("connection"), "upgrade")) return bad("Connection: Upgrade fehlt");
    if (h("sec-websocket-version") != "13") {
        u.ok = false;
        u.reason = "WebSocket-Version";
        u.response = "HTTP/1.1 426 Upgrade Required\r\nSec-WebSocket-Version: 13\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
        return u;
    }
    std::string key = h("sec-websocket-key");
    bool ok = false;
    if (unbase64(key, &ok).size() != 16 || !ok) return bad("Sec-WebSocket-Key");
    u.ok = true;
    u.response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
                 ws_accept(key) + "\r\n\r\n";
    return u;
}

std::string ws_frame(Op op, const std::string& payload, std::optional<uint32_t> mask) {
    std::string f;
    f += (char)(0x80 | (uint8_t)op);
    uint8_t m = mask ? 0x80 : 0;
    size_t n = payload.size();
    if (n < 126) {
        f += (char)(m | n);
    } else if (n <= 0xFFFF) {
        f += (char)(m | 126);
        f += (char)(n >> 8);
        f += (char)(n & 0xFF);
    } else {
        f += (char)(m | 127);
        for (int i = 7; i >= 0; --i) f += (char)((uint64_t)n >> (8 * i) & 0xFF);
    }
    if (!mask) return f + payload;
    unsigned char k[4] = {(unsigned char)(*mask >> 24), (unsigned char)(*mask >> 16), (unsigned char)(*mask >> 8),
                          (unsigned char)*mask};
    f.append((const char*)k, 4);
    size_t start = f.size();
    f += payload;
    for (size_t i = 0; i < n; ++i) f[start + i] = (char)(f[start + i] ^ k[i & 3]);
    return f;
}

bool valid_utf8(const std::string& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t n = s.size();
    for (size_t i = 0; i < n;) {
        unsigned char c = p[i];
        if (c < 0x80) {
            ++i;
            continue;
        }
        size_t len;
        uint32_t cp, min;
        if ((c & 0xE0) == 0xC0) len = 2, cp = c & 0x1F, min = 0x80;
        else if ((c & 0xF0) == 0xE0) len = 3, cp = c & 0x0F, min = 0x800;
        else if ((c & 0xF8) == 0xF0) len = 4, cp = c & 0x07, min = 0x10000;
        else return false;
        if (i + len > n) return false;
        for (size_t k = 1; k < len; ++k) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
            cp = cp << 6 | (p[i + k] & 0x3F);
        }
        if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
        i += len;
    }
    return true;
}

bool valid_close_payload(const std::string& payload) {
    if (payload.empty()) return true;
    if (payload.size() < 2) return false;
    unsigned code = (unsigned)(unsigned char)payload[0] << 8 | (unsigned char)payload[1];
    bool ok = (code >= 1000 && code <= 1003) || (code >= 1007 && code <= 1014) || (code >= 3000 && code <= 4999);
    return ok && valid_utf8(payload.substr(2));
}

std::string ws_close_frame(uint16_t code, const std::string& reason) {
    std::string p;
    p += (char)(code >> 8);
    p += (char)(code & 0xFF);
    p += reason.substr(0, 120);
    return ws_frame(Op::Close, p);
}

WsParser::Status WsParser::next(WsMessage& out, std::string* error) {
    auto err = [&](const char* why) {
        if (error) *error = why;
        buf_.clear();
        return Status::Error;
    };
    for (;;) {
        if (buf_.size() < 2) return Status::NeedMore;
        const unsigned char* b = reinterpret_cast<const unsigned char*>(buf_.data());
        bool fin = b[0] & 0x80;
        if (b[0] & 0x70) return err("RSV-Bits gesetzt");
        uint8_t op = b[0] & 0x0F;
        bool masked = b[1] & 0x80;
        uint64_t len = b[1] & 0x7F;
        size_t pos = 2;
        if (len == 126) {
            if (buf_.size() < 4) return Status::NeedMore;
            len = (uint64_t)b[2] << 8 | b[3];
            if (len < 126) return err("Länge nicht minimal kodiert");
            pos = 4;
        } else if (len == 127) {
            if (buf_.size() < 10) return Status::NeedMore;
            len = 0;
            for (int i = 0; i < 8; ++i) len = len << 8 | b[2 + i];
            if (len >> 63) return err("Länge");
            if (len <= 0xFFFF) return err("Länge nicht minimal kodiert");
            pos = 10;
        }
        if (require_mask_ && !masked) return err("unmaskierter Client-Rahmen");
        bool control = op >= 8;
        if (control && (op != 8 && op != 9 && op != 10)) return err("unbekannter Steuerrahmen");
        if (control && (!fin || len > 125)) return err("ungültiger Steuerrahmen");
        if (!control && op > 2) return err("unbekannter Datenrahmen");
        if (len > max_) return err("Nachricht zu groß");
        size_t need = pos + (masked ? 4 : 0) + (size_t)len;
        if (buf_.size() < need) return Status::NeedMore;
        std::string payload = buf_.substr(pos + (masked ? 4 : 0), (size_t)len);
        if (masked) {
            const unsigned char* k = b + pos;
            for (size_t i = 0; i < payload.size(); ++i) payload[i] = (char)(payload[i] ^ k[i & 3]);
        }
        buf_.erase(0, need);
        if (control) {
            if (op == 8 && !valid_close_payload(payload)) return err("ungültiger Close-Rahmen");
            out = {(Op)op, std::move(payload)};
            return Status::Message;
        }
        if (op == 0) {
            if (partial_op_ < 0) return err("Fortsetzung ohne Anfang");
            if (partial_.size() + payload.size() > max_) return err("Nachricht zu groß");
            partial_ += payload;
            if (!fin) continue;
            out = {(Op)partial_op_, std::move(partial_)};
            partial_.clear();
            partial_op_ = -1;
            return Status::Message;
        }
        if (partial_op_ >= 0) return err("neue Nachricht mitten in einer fragmentierten");
        if (!fin) {
            partial_op_ = op;
            partial_ = std::move(payload);
            continue;
        }
        out = {(Op)op, std::move(payload)};
        return Status::Message;
    }
}

// ---------- Nachrichten ----------
std::string clean_name(const std::string& name) {
    // Steuerzeichen raus, auf 40 Unicode-Zeichen kürzen, ohne ein UTF-8-Zeichen zu zerschneiden
    std::string out;
    int chars = 0;
    for (size_t i = 0; i < name.size() && chars < 40;) {
        unsigned char c = (unsigned char)name[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (!len || i + len > name.size()) break;
        bool cont_ok = true;
        for (size_t k = 1; k < len; ++k) cont_ok &= ((unsigned char)name[i + k] >> 6) == 2;
        if (!cont_ok) break;
        if (!(len == 1 && (c < 0x20 || c == 0x7F))) {
            out.append(name, i, len);
            ++chars;
        }
        i += len;
    }
    size_t a = out.find_first_not_of(' '), b = out.find_last_not_of(' ');
    return a == std::string::npos ? "" : out.substr(a, b - a + 1);
}

std::pair<int, int> render_size(int w, int h) {
    if (w < 1 || h < 1) return {480, 320};
    double scale = std::min({1.0, 2560.0 / w, 2560.0 / h, std::sqrt(4194304.0 / (double(w) * h))});
    return {std::max(1, int(w * scale)), std::max(1, int(h * scale))};
}

ClientMsg parse_client(const std::string& text) {
    ClientMsg m;
    json j;
    try {
        j = json::parse(text);
    } catch (const std::exception&) {
        m.error = "kein gültiges JSON";
        return m;
    }
    if (!j.is_object() || !j.contains("t") || !j["t"].is_string()) {
        m.error = "Feld t fehlt";
        return m;
    }
    auto int_field = [&](const char* k, int& dst, int lo, int hi) {
        if (!j.contains(k) || !j[k].is_number_integer()) return false;
        long long v = j[k].get<long long>();
        if (v < lo || v > hi) return false;
        dst = (int)v;
        return true;
    };
    std::string t = j["t"].get<std::string>();
    if (t == "hello") {
        if (!int_field("v", m.v, 0, 1000)) {
            m.error = "Feld v fehlt";
            return m;
        }
        int_field("w", m.w, 1, 16384);
        int_field("h", m.h, 1, 16384);
        if (j.contains("name") && j["name"].is_string()) m.name = clean_name(j["name"].get<std::string>());
        bool has_key = j.contains("key") && j["key"].is_string(), has_pin = j.contains("pin") && j["pin"].is_string();
        if (has_key == has_pin) {
            m.error = "genau eines von key und pin erwartet";
            return m;
        }
        if (has_key) m.key = j["key"].get<std::string>().substr(0, 128);
        if (has_pin) m.pin = j["pin"].get<std::string>().substr(0, 16);
        m.type = ClientMsg::Type::Hello;
    } else if (t == "touch") {
        if (!int_field("x", m.x, 0, 16383) || !int_field("y", m.y, 0, 16383)) {
            m.error = "x/y fehlen oder außerhalb";
            return m;
        }
        m.type = ClientMsg::Type::Touch;
    } else if (t == "swipe") {
        if (!j.contains("dir")) { m.error = "dir fehlt"; return m; }
        if (j["dir"].is_string()) {
            auto dir = j["dir"].get<std::string>();
            m.dir = dir == "left" ? 1 : dir == "right" ? -1 : 0;
        } else if (!int_field("dir", m.dir, -1, 1)) m.dir = 0;
        if (!m.dir) { m.error = "dir: left/right oder -1/+1 erwartet"; return m; }
        m.type = ClientMsg::Type::Swipe;
    } else if (t == "resize") {
        if (!int_field("w", m.w, 1, 16384) || !int_field("h", m.h, 1, 16384)) {
            m.error = "w/h fehlen oder außerhalb"; return m;
        }
        m.type = ClientMsg::Type::Resize;
    } else if (t == "ping") {
        m.type = ClientMsg::Type::Ping;
    } else if (t == "pong") {
        m.type = ClientMsg::Type::Pong;
    } else {
        m.type = ClientMsg::Type::Unknown;
    }
    return m;
}

std::string msg_ready(int w, int h) { return json{{"t", "ready"}, {"w", w}, {"h", h}}.dump(); }
std::string msg_paired(const std::string& key_b64) { return json{{"t", "paired"}, {"key", key_b64}}.dump(); }
std::string msg_error(const std::string& code, const std::string& msg) {
    return json{{"t", "error"}, {"code", code}, {"msg", msg}}.dump(-1, ' ', false, json::error_handler_t::replace);
}
std::string msg_ping() { return R"({"t":"ping"})"; }
std::string msg_pong() { return R"({"t":"pong"})"; }
std::string frame_payload(const std::string& jpeg) { return std::string(1, (char)FRAME_JPEG) + jpeg; }

// ---------- Kopplung ----------
static std::string pct(const std::string& s) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~' || c == ':') out += (char)c;
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

std::string pair_uri(const std::string& host, int port, const std::string& fingerprint, const std::string& pin) {
    return "fuchslabs://pair?host=" + pct(host) + "&port=" + std::to_string(port) + "&fp=" + pct(fingerprint) + "&pin=" + pct(pin);
}

std::string qr_svg(const std::string& text) {
    using qrcodegen::QrCode;
    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
    const int border = 4, size = qr.getSize();
    std::string path;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
            if (qr.getModule(x, y)) path += "M" + std::to_string(x + border) + "," + std::to_string(y + border) + "h1v1h-1z";
    std::string dim = std::to_string(size + 2 * border);
    return "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " + dim + " " + dim +
           "\" shape-rendering=\"crispEdges\"><rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/><path d=\"" + path +
           "\" fill=\"#000000\"/></svg>";
}

std::string format_fingerprint(const std::string& hex) {
    std::string out;
    for (size_t i = 0; i < hex.size(); ++i) {
        if (i && i % 2 == 0) out += ':';
        out += (char)toupper((unsigned char)hex[i]);
    }
    return out;
}

Pairing::Pairing(Clock now, std::function<std::string()> make_pin) : now_(std::move(now)), make_pin_(std::move(make_pin)) {}

void Pairing::expire_locked() {
    double t = now_();
    if (code_ && t >= code_->expires) code_.reset();
    if (locked_until_ && t >= locked_until_) {
        locked_until_ = 0;
        failures_ = 0;
    }
}

std::optional<Pairing::Code> Pairing::start() {
    std::lock_guard lk(mu_);
    expire_locked();
    if (locked_until_) return std::nullopt;
    code_ = Code{make_pin_(), now_() + PIN_SECONDS};
    return code_;
}

Pairing::Result Pairing::check(const std::string& pin) {
    std::lock_guard lk(mu_);
    double t = now_();
    if (locked_until_ && t < locked_until_) return Result::Locked;
    bool had = code_.has_value();
    expire_locked();
    if (!code_) return had ? Result::Expired : Result::NoPin;
    if (equal_ct(pin, code_->pin)) {
        code_.reset();  // einmalig
        failures_ = 0;
        return Result::Ok;
    }
    if (++failures_ >= MAX_FAILURES) {
        code_.reset();
        locked_until_ = t + LOCK_SECONDS;
    }
    return Result::Wrong;
}

void Pairing::cancel() {
    std::lock_guard lk(mu_);
    code_.reset();
}

std::optional<Pairing::Code> Pairing::active() {
    std::lock_guard lk(mu_);
    expire_locked();
    return code_;
}

double Pairing::locked_for() {
    std::lock_guard lk(mu_);
    expire_locked();
    return locked_until_ ? locked_until_ - now_() : 0;
}

int Pairing::failures() {
    std::lock_guard lk(mu_);
    expire_locked();
    return failures_;
}

// ---------- Store ----------
Store::Store(std::filesystem::path path, Codec protect, Codec unprotect, std::function<double()> wall_clock)
    : path_(std::move(path)), protect_(std::move(protect)), unprotect_(std::move(unprotect)), wall_(std::move(wall_clock)) {}

static std::string hex_of(const std::string& raw) {
    static const char hex[] = "0123456789abcdef";
    std::string out;
    for (unsigned char c : raw) {
        out += hex[c >> 4];
        out += hex[c & 15];
    }
    return out;
}

void Store::load() {
    std::lock_guard lk(mu_);
    cert_.clear();
    key_.clear();
    devices_.clear();
    std::ifstream f(path_, std::ios::binary);
    if (!f) return;
    std::stringstream ss;
    ss << f.rdbuf();
    json j = json::parse(ss.str(), nullptr, false);
    if (!j.is_object()) return;
    if (j.contains("cert") && j["cert"].is_string() && j.contains("key") && j["key"].is_string()) {
        std::string key = unprotect_(j["key"].get<std::string>());
        if (!key.empty()) {
            cert_ = j["cert"].get<std::string>();
            key_ = key;
        }
    }
    if (j.contains("devices") && j["devices"].is_array())
        for (auto& d : j["devices"]) {
            if (!d.is_object() || !d.contains("id") || !d["id"].is_string() || !d.contains("key") || !d["key"].is_string()) continue;
            bool ok = false;
            std::string raw = unbase64(unprotect_(d["key"].get<std::string>()), &ok);
            if (!ok || raw.size() != 32) continue;  // z. B. anderes Windows-Konto: Eintrag unbrauchbar
            Device dev{d["id"].get<std::string>(), d.contains("name") && d["name"].is_string() ? clean_name(d["name"].get<std::string>()) : "",
                       raw, d.value("added", 0.0), d.value("last_seen", 0.0)};
            if (devices_.size() < MAX_DEVICES) devices_.push_back(std::move(dev));
        }
}

void Store::save_locked() {
    json j = json::object();
    j["version"] = 1;
    if (!cert_.empty()) {
        j["cert"] = cert_;
        j["key"] = protect_(key_);
    }
    json arr = json::array();
    for (auto& d : devices_)
        arr.push_back({{"id", d.id}, {"name", d.name}, {"key", protect_(base64(d.key))}, {"added", d.added}, {"last_seen", d.last_seen}});
    j["devices"] = arr;
    std::string text = j.dump(2, ' ', false, json::error_handler_t::replace);
    auto tmp = path_;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        f.write(text.data(), (std::streamsize)text.size());
        if (!f) throw std::runtime_error("Companion-Kopplungen konnten nicht gespeichert werden");
    }
    std::filesystem::rename(tmp, path_);  // ersetzt die alte Datei
}

void Store::save() {
    std::lock_guard lk(mu_);
    save_locked();
}

std::string Store::cert_pem() {
    std::lock_guard lk(mu_);
    return cert_;
}
std::string Store::key_pem() {
    std::lock_guard lk(mu_);
    return key_;
}
void Store::set_identity(const std::string& cert_pem, const std::string& key_pem) {
    std::lock_guard lk(mu_);
    cert_ = cert_pem;
    key_ = key_pem;
    save_locked();
}

std::string Store::add_device(const std::string& name, std::string* id) {
    std::lock_guard lk(mu_);
    Device d{hex_of(random_bytes(8)), clean_name(name), random_bytes(32), wall_(), wall_()};
    if (d.name.empty()) d.name = "Android";
    if (devices_.size() >= MAX_DEVICES) {  // ältestes (am längsten nicht gesehenes) Gerät fällt heraus
        auto oldest = std::min_element(devices_.begin(), devices_.end(),
                                       [](const Device& a, const Device& b) { return a.last_seen < b.last_seen; });
        devices_.erase(oldest);
    }
    devices_.push_back(d);
    save_locked();
    if (id) *id = d.id;
    return d.key;
}

std::optional<Device> Store::find(const std::string& raw_key) {
    std::lock_guard lk(mu_);
    std::optional<Device> hit;
    for (auto& d : devices_)  // alle vergleichen, kein früher Abbruch
        if (equal_ct(d.key, raw_key) && !hit) hit = d;
    return hit;
}

bool Store::contains(const std::string& id) {
    std::lock_guard lk(mu_);
    return std::any_of(devices_.begin(), devices_.end(), [&](const Device& d) { return d.id == id; });
}

void Store::seen(const std::string& id) {
    std::lock_guard lk(mu_);
    for (auto& d : devices_)
        if (d.id == id) d.last_seen = wall_();
    try {
        save_locked();
    } catch (...) {
    }
}

bool Store::remove(const std::string& id) {
    std::lock_guard lk(mu_);
    auto it = std::find_if(devices_.begin(), devices_.end(), [&](const Device& d) { return d.id == id; });
    if (it == devices_.end()) return false;
    devices_.erase(it);
    save_locked();
    return true;
}

json Store::list() {
    std::lock_guard lk(mu_);
    json arr = json::array();
    for (auto& d : devices_) arr.push_back({{"id", d.id}, {"name", d.name}, {"added", d.added}, {"last_seen", d.last_seen}});
    return arr;
}

}  // namespace fl::companion
