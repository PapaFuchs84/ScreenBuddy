// © 2026 FuchsLabs
#include "companion_server.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace fl::companion {

namespace {

#ifdef _WIN32
using sock_t = SOCKET;
const sock_t BAD_SOCK = INVALID_SOCKET;
void sclose(sock_t s) { closesocket(s); }
void set_nonblock(sock_t s, bool on) {
    u_long m = on ? 1 : 0;
    ioctlsocket(s, FIONBIO, &m);
}
void set_timeouts(sock_t s, int ms) {
    DWORD t = (DWORD)ms;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&t), sizeof t);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&t), sizeof t);
}
struct WsaInit {
    WsaInit() {
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
    }
} g_wsa;
#else
using sock_t = int;
const sock_t BAD_SOCK = -1;
void sclose(sock_t s) { ::close(s); }
void set_nonblock(sock_t s, bool on) {
    int fl = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, on ? fl | O_NONBLOCK : fl & ~O_NONBLOCK);
}
void set_timeouts(sock_t s, int ms) {
    timeval tv{ms / 1000, (ms % 1000) * 1000};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
}
#endif

// true = bereit; false = Zeit abgelaufen oder Fehler
bool wait_sock(sock_t s, bool write, int ms) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    timeval tv{ms / 1000, (ms % 1000) * 1000};
    int r = select((int)s + 1, write ? nullptr : &set, write ? &set : nullptr, nullptr, &tv);
    return r > 0;
}

double steady() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string ssl_error() {
    unsigned long e = ERR_get_error();
    char buf[256] = "unbekannt";
    if (e) ERR_error_string_n(e, buf, sizeof buf);
    ERR_clear_error();
    return buf;
}

struct BioFree {
    void operator()(BIO* b) const { BIO_free(b); }
};
struct X509Free {
    void operator()(X509* x) const { X509_free(x); }
};
struct PkeyFree {
    void operator()(EVP_PKEY* k) const { EVP_PKEY_free(k); }
};

std::string bio_string(BIO* b) {
    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(b, &mem);
    return mem ? std::string(mem->data, mem->length) : std::string();
}

std::unique_ptr<X509, X509Free> read_cert(const std::string& pem) {
    std::unique_ptr<BIO, BioFree> b(BIO_new_mem_buf(pem.data(), (int)pem.size()));
    return std::unique_ptr<X509, X509Free>(b ? PEM_read_bio_X509(b.get(), nullptr, nullptr, nullptr) : nullptr);
}

}  // namespace

// ---------- TLS-Identität ----------
Identity generate_identity(const std::string& common_name) {
    std::unique_ptr<EVP_PKEY, PkeyFree> key(EVP_EC_gen("P-256"));
    if (!key) throw std::runtime_error("TLS-Schlüssel: " + ssl_error());
    std::unique_ptr<X509, X509Free> x(X509_new());
    X509_set_version(x.get(), 2);
    std::string serial = random_bytes(16);
    serial[0] = (char)(serial[0] & 0x7F);
    BIGNUM* bn = BN_bin2bn(reinterpret_cast<const unsigned char*>(serial.data()), (int)serial.size(), nullptr);
    BN_to_ASN1_INTEGER(bn, X509_get_serialNumber(x.get()));
    BN_free(bn);
    X509_gmtime_adj(X509_getm_notBefore(x.get()), -86400L);
    X509_gmtime_adj(X509_getm_notAfter(x.get()), 10L * 365 * 86400);
    X509_set_pubkey(x.get(), key.get());
    X509_NAME* name = X509_get_subject_name(x.get());
    std::string cn = common_name.substr(0, 64);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_UTF8, reinterpret_cast<const unsigned char*>("FuchsLabs"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_UTF8, reinterpret_cast<const unsigned char*>(cn.c_str()), -1, -1, 0);
    X509_set_issuer_name(x.get(), name);
    X509V3_CTX v3;
    X509V3_set_ctx(&v3, x.get(), x.get(), nullptr, nullptr, 0);
    for (auto [nid, value] : {std::pair{NID_basic_constraints, "critical,CA:FALSE"}, std::pair{NID_key_usage, "critical,digitalSignature"},
                              std::pair{NID_ext_key_usage, "serverAuth"}}) {
        X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, &v3, nid, value);
        if (ext) {
            X509_add_ext(x.get(), ext, -1);
            X509_EXTENSION_free(ext);
        }
    }
    if (!X509_sign(x.get(), key.get(), EVP_sha256())) throw std::runtime_error("TLS-Zertifikat: " + ssl_error());
    std::unique_ptr<BIO, BioFree> cb(BIO_new(BIO_s_mem())), kb(BIO_new(BIO_s_mem()));
    PEM_write_bio_X509(cb.get(), x.get());
    PEM_write_bio_PrivateKey(kb.get(), key.get(), nullptr, nullptr, 0, nullptr, nullptr);
    return {bio_string(cb.get()), bio_string(kb.get())};
}

std::string cert_fingerprint(const std::string& cert_pem) {
    auto x = read_cert(cert_pem);
    if (!x) return "";
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int n = 0;
    if (!X509_digest(x.get(), EVP_sha256(), md, &n)) return "";
    static const char hex[] = "0123456789abcdef";
    std::string out;
    for (unsigned i = 0; i < n; ++i) {
        out += hex[md[i] >> 4];
        out += hex[md[i] & 15];
    }
    return out;
}

static SSL_CTX* make_ctx(const std::string& cert_pem, const std::string& key_pem) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return nullptr;
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION | SSL_OP_NO_RENEGOTIATION | SSL_OP_CIPHER_SERVER_PREFERENCE);
    SSL_CTX_set_cipher_list(ctx, "ECDHE+AESGCM:ECDHE+CHACHA20");
    SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    auto x = read_cert(cert_pem);
    std::unique_ptr<BIO, BioFree> kb(BIO_new_mem_buf(key_pem.data(), (int)key_pem.size()));
    std::unique_ptr<EVP_PKEY, PkeyFree> key(kb ? PEM_read_bio_PrivateKey(kb.get(), nullptr, nullptr, nullptr) : nullptr);
    if (!x || !key || SSL_CTX_use_certificate(ctx, x.get()) != 1 || SSL_CTX_use_PrivateKey(ctx, key.get()) != 1 ||
        SSL_CTX_check_private_key(ctx) != 1) {
        ERR_clear_error();
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

// ---------- Server ----------
struct Server::Conn {
    sock_t sock = BAD_SOCK;
    std::string ip;
    std::atomic<bool> done{false}, kick{false};
    std::mutex mu;
    bool authed = false;
    std::string device_id, name;
};

Server::Server(Options opt, Store& store, Hooks hooks)
    : opt_(std::move(opt)), store_(store), hooks_(std::move(hooks)), pairing_([this] { return now(); }) {}

Server::~Server() { stop(); }

double Server::now() const { return hooks_.now ? hooks_.now() : steady(); }

void Server::log(int lv, const std::string& msg) const {
    if (hooks_.log) hooks_.log(lv, msg);
}

void Server::start() {
    if (running_) return;
    if (!valid_port(opt_.port, opt_.settings_port)) throw std::runtime_error("Ungültiger Port für die Companion-App");
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);  // Schreiben auf getrennte Verbindung: Fehlercode statt Programmende
#endif
    // TLS-Identität laden oder einmalig erzeugen (bleibt gleich, damit gekoppelte Geräte dem Fingerabdruck trauen)
    std::string cert = store_.cert_pem(), key = store_.key_pem();
    SSL_CTX* ctx = cert.empty() ? nullptr : make_ctx(cert, key);
    if (!ctx) {
        if (!cert.empty()) log(2, "TLS-Zertifikat unbrauchbar – erzeuge ein neues (Geräte müssen neu gekoppelt werden)");
        Identity id = generate_identity("FuchsLabs USB-Display");
        store_.set_identity(id.cert_pem, id.key_pem);
        cert = id.cert_pem;
        ctx = make_ctx(id.cert_pem, id.key_pem);
        if (!ctx) throw std::runtime_error("TLS konnte nicht eingerichtet werden");
    }
    fingerprint_ = cert_fingerprint(cert);
    ctx_ = ctx;

    std::vector<std::string> bound;
    for (auto& addr : opt_.bind) {
        bool allowed = is_lan_ipv4(addr) || (opt_.allow_loopback && is_loopback_ipv4(addr));
        if (!allowed) {
            log(2, "Adresse " + addr + " ist keine Heimnetz-Adresse – übersprungen");
            continue;
        }
        sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == BAD_SOCK) continue;
        int yes = 1;
#ifdef _WIN32
        setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&yes), sizeof yes);
#else
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
#endif
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons((uint16_t)opt_.port);
        inet_pton(AF_INET, addr.c_str(), &sa.sin_addr);
        if (bind(s, reinterpret_cast<sockaddr*>(&sa), sizeof sa) != 0 || listen(s, 8) != 0) {
            log(2, "Port " + std::to_string(opt_.port) + " auf " + addr + " nicht verfügbar");
            sclose(s);
            continue;
        }
        listeners_.push_back((intptr_t)s);
        bound.push_back(addr);
    }
    if (listeners_.empty()) {
        SSL_CTX_free((SSL_CTX*)ctx_);
        ctx_ = nullptr;
        throw std::runtime_error(opt_.bind.empty() ? "Kein Heimnetz gefunden (WLAN/LAN verbinden)"
                                                   : "Companion-Port " + std::to_string(opt_.port) + " ist belegt");
    }
    opt_.bind = bound;
    stop_ = false;
    running_ = true;
    accept_thread_ = std::thread([this] { accept_loop(); });
    std::string list;
    for (auto& b : bound) list += (list.empty() ? "" : ", ") + b + ":" + std::to_string(opt_.port);
    log(1, "Companion-Server läuft auf " + list);
}

void Server::stop() {
    if (!running_) return;
    stop_ = true;
    if (accept_thread_.joinable()) accept_thread_.join();
    reap(true);
    for (auto l : listeners_) sclose((sock_t)l);
    listeners_.clear();
    if (ctx_) SSL_CTX_free((SSL_CTX*)ctx_);
    ctx_ = nullptr;
    running_ = false;
    log(1, "Companion-Server beendet");
}

void Server::reap(bool all) {
    std::list<std::pair<std::shared_ptr<Conn>, std::thread>> finished;
    {
        std::lock_guard lk(mu_);
        for (auto it = workers_.begin(); it != workers_.end();) {
            if (all || it->first->done) {
                finished.push_back(std::move(*it));
                it = workers_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& [c, t] : finished)
        if (t.joinable()) t.join();
}

void Server::accept_loop() {
    while (!stop_) {
        reap(false);
        fd_set set;
        FD_ZERO(&set);
        sock_t maxfd = 0;
        for (auto l : listeners_) {
            FD_SET((sock_t)l, &set);
            if ((sock_t)l > maxfd) maxfd = (sock_t)l;
        }
        timeval tv{0, 200000};
        if (select((int)maxfd + 1, &set, nullptr, nullptr, &tv) <= 0) continue;
        for (auto l : listeners_) {
            if (!FD_ISSET((sock_t)l, &set)) continue;
            sockaddr_in peer{};
            socklen_t len = sizeof peer;
            sock_t s = accept((sock_t)l, reinterpret_cast<sockaddr*>(&peer), &len);
            if (s == BAD_SOCK) continue;
            char ip[INET_ADDRSTRLEN] = "";
            inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
            bool lan = is_lan_ipv4(ip) || (opt_.allow_loopback && is_loopback_ipv4(ip));
            size_t active;
            {
                std::lock_guard lk(mu_);
                active = workers_.size();
            }
            if (!lan || (int)active >= opt_.max_clients) {
                log(2, std::string("Verbindung von ") + ip + (lan ? " abgewiesen (zu viele Geräte)" : " abgewiesen (nicht im Heimnetz)"));
                sclose(s);
                continue;
            }
            auto c = std::make_shared<Conn>();
            c->sock = s;
            c->ip = ip;
            std::lock_guard lk(mu_);
            workers_.emplace_back(c, std::thread([this, c] { serve(c); }));
        }
    }
}

void Server::frame_changed() { ++seq_; }

json Server::clients() {
    json arr = json::array();
    std::lock_guard lk(mu_);
    for (auto& [c, t] : workers_) {
        std::lock_guard ck(c->mu);
        if (c->authed && !c->done) arr.push_back({{"id", c->device_id}, {"name", c->name}, {"ip", c->ip}});
    }
    return arr;
}

void Server::drop_device(const std::string& id) {
    std::lock_guard lk(mu_);
    for (auto& [c, t] : workers_) {
        std::lock_guard ck(c->mu);
        if (c->device_id == id) c->kick = true;
    }
}

void Server::serve(std::shared_ptr<Conn> c) {
    sock_t s = c->sock;
    SSL* ssl = nullptr;
    bool was_authed = false;
    auto finish = [&] {
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
#ifdef _WIN32
        // Kein TIME_WAIT auf dem Server-Port: mit SO_EXCLUSIVEADDRUSE ließe sich 8777 nach einem Neustart der App
        // sonst minutenlang nicht wieder binden. Vorher kurz auf das Schließen der Gegenseite warten.
        wait_sock(s, false, 300);
        linger lg{1, 0};
        setsockopt(s, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lg), sizeof lg);
#endif
        sclose(s);
        c->done = true;
        if (was_authed && hooks_.on_change) hooks_.on_change();
    };
    set_timeouts(s, 10000);
    ssl = SSL_new((SSL_CTX*)ctx_);
    if (!ssl) return finish();
    SSL_set_fd(ssl, (int)s);
    if (SSL_accept(ssl) != 1) {
        log(0, "TLS-Handshake mit " + c->ip + " fehlgeschlagen: " + ssl_error());
        SSL_free(ssl);
        ssl = nullptr;
        return finish();
    }
    // HTTP-Upgrade lesen (höchstens 8 KiB)
    std::string req;
    char buf[16384];
    while (req.find("\r\n\r\n") == std::string::npos) {
        int n = SSL_read(ssl, buf, sizeof buf);
        if (n <= 0 || req.size() + n > 8192) {
            ERR_clear_error();
            return finish();
        }
        req.append(buf, n);
    }
    size_t head_end = req.find("\r\n\r\n") + 4;
    Upgrade up = ws_upgrade(req.substr(0, head_end));
    SSL_write(ssl, up.response.data(), (int)up.response.size());
    if (!up.ok) {
        log(0, "Upgrade von " + c->ip + " abgelehnt: " + up.reason);
        return finish();
    }
    WsParser parser;
    parser.feed(req.substr(head_end));
    set_nonblock(s, true);

    auto send_raw = [&](const std::string& data) {
        size_t off = 0;
        double deadline = steady() + 5;
        while (off < data.size()) {
            int chunk = (int)std::min<size_t>(data.size() - off, 1 << 20);
            int n = SSL_write(ssl, data.data() + off, chunk);
            if (n > 0) {
                off += (size_t)n;
                continue;
            }
            int e = SSL_get_error(ssl, n);
            if ((e == SSL_ERROR_WANT_WRITE || e == SSL_ERROR_WANT_READ) && steady() < deadline) {
                wait_sock(s, e == SSL_ERROR_WANT_WRITE, 100);
                continue;
            }
            ERR_clear_error();
            return false;
        }
        return true;
    };
    auto send_text = [&](const std::string& t) { return send_raw(ws_frame(Op::Text, t)); };
    auto fail = [&](const std::string& code, const std::string& msg) {
        send_text(msg_error(code, msg));
        send_raw(ws_close_frame(1008, code));
    };

    double t0 = now(), last_rx = t0, last_ping = t0, hello_deadline = t0 + 10;
    double touch_window = t0;
    int touches = 0;
    bool authed = false;
    FrameGate gate;
    bool open = true;
    while (open && !stop_ && !c->kick) {
        if (SSL_pending(ssl) == 0 && parser.buffered() == 0) wait_sock(s, false, 50);
        // lesen, bis nichts mehr da ist
        bool closed = false;
        for (;;) {
            int n = SSL_read(ssl, buf, sizeof buf);
            if (n > 0) {
                parser.feed(buf, (size_t)n);
                last_rx = now();
                continue;
            }
            int e = SSL_get_error(ssl, n);
            if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) closed = true;
            ERR_clear_error();
            break;
        }
        WsMessage m;
        std::string perr;
        WsParser::Status st;
        while (open && (st = parser.next(m, &perr)) == WsParser::Status::Message) {
            switch (m.op) {
                case Op::Close:
                    send_raw(ws_close_frame(1000));
                    open = false;
                    break;
                case Op::Ping: send_raw(ws_frame(Op::Pong, m.payload)); break;
                case Op::Pong: break;
                case Op::Binary:
                    fail("bad_message", "Binärnachrichten werden nicht angenommen");
                    open = false;
                    break;
                default: {
                    ClientMsg cm = parse_client(m.payload);
                    if (cm.type == ClientMsg::Type::Invalid) {
                        fail("bad_message", cm.error);
                        open = false;
                    } else if (cm.type == ClientMsg::Type::Ping) {
                        send_text(msg_pong());
                    } else if (cm.type == ClientMsg::Type::Hello && !authed) {
                        if (cm.v != PROTOCOL_VERSION) {
                            fail("version", "Protokollversion wird nicht unterstützt");
                            open = false;
                            break;
                        }
                        std::string id, name = cm.name;
                        if (!cm.key.empty()) {
                            bool ok = false;
                            std::string raw = unbase64(cm.key, &ok);
                            auto dev = ok ? store_.find(raw) : std::nullopt;
                            if (!dev) {
                                log(2, "Anmeldung von " + c->ip + " mit unbekanntem Geräteschlüssel abgelehnt");
                                fail("unauthorized", "Gerät ist nicht (mehr) gekoppelt");
                                open = false;
                                break;
                            }
                            id = dev->id;
                            if (name.empty()) name = dev->name;
                            store_.seen(id);
                        } else {
                            switch (pairing_.check(cm.pin)) {
                                case Pairing::Result::Ok: {
                                    std::string key = store_.add_device(name, &id);
                                    send_text(msg_paired(base64(key)));
                                    log(1, "Gerät „" + (name.empty() ? std::string("Android") : name) + "“ (" + c->ip + ") gekoppelt");
                                    break;
                                }
                                case Pairing::Result::Locked:
                                    fail("locked", "Zu viele Fehlversuche – Kopplung für einige Minuten gesperrt");
                                    open = false;
                                    break;
                                case Pairing::Result::Wrong:
                                    log(2, "Falsche Kopplungs-PIN von " + c->ip + " (" + std::to_string(pairing_.failures()) + "/5)");
                                    fail(pairing_.locked_for() > 0 ? "locked" : "pin_wrong",
                                         pairing_.locked_for() > 0 ? "Zu viele Fehlversuche – Kopplung für einige Minuten gesperrt" : "PIN falsch");
                                    open = false;
                                    break;
                                default:
                                    fail("pin_expired", "Kein gültiger Kopplungscode – am PC neu anzeigen");
                                    open = false;
                                    break;
                            }
                            if (!open) break;
                        }
                        {
                            std::lock_guard ck(c->mu);
                            c->authed = true;
                            c->device_id = id;
                            c->name = name.empty() ? "Android" : name;
                        }
                        authed = was_authed = true;
                        send_text(msg_ready(opt_.w, opt_.h));
                        gate.reset();
                        last_ping = now();
                        if (hooks_.on_change) hooks_.on_change();
                    } else if (cm.type == ClientMsg::Type::Touch) {
                        if (!authed) {
                            fail("unauthorized", "Erst anmelden (hello)");
                            open = false;
                            break;
                        }
                        double t = now();
                        if (t - touch_window >= 1) {
                            touch_window = t;
                            touches = 0;
                        }
                        if (cm.x < opt_.w && cm.y < opt_.h && ++touches <= 10 && hooks_.on_touch) {
                            try {
                                hooks_.on_touch(cm.x, cm.y);
                            } catch (const std::exception& e) {
                                log(2, std::string("Touch: ") + e.what());
                            }
                        }
                    }
                    // pong, unbekannte Typen und ein zweites hello werden ignoriert
                }
            }
        }
        if (open && st == WsParser::Status::Error) {
            log(0, "Ungültiger WebSocket-Rahmen von " + c->ip + ": " + perr);
            send_raw(ws_close_frame(1002, "protocol"));
            break;
        }
        if (!open || closed) break;
        double t = now();
        if (!authed && t > hello_deadline) {
            fail("timeout", "Keine Anmeldung erhalten");
            break;
        }
        if (t - last_rx > DEAD_SECONDS) {
            log(1, "Companion " + c->ip + ": 15 s keine Antwort – Verbindung getrennt");
            break;
        }
        if (!authed) continue;
        if (t - last_ping >= PING_SECONDS) {
            if (!send_text(msg_ping())) break;
            last_ping = t;
        }
        uint64_t seq = seq_;
        if (gate.due(seq, t)) {
            std::shared_ptr<const std::string> jpeg;
            {
                std::lock_guard lk(jpeg_mu_);
                if (jpeg_seq_ != seq || !jpeg_) {
                    std::string data;
                    try {
                        data = hooks_.encode ? hooks_.encode() : std::string();
                    } catch (const std::exception& e) {
                        log(2, std::string("JPEG: ") + e.what());
                    }
                    jpeg_ = std::make_shared<const std::string>(std::move(data));
                    jpeg_seq_ = seq;
                }
                jpeg = jpeg_;
            }
            if (jpeg && !jpeg->empty()) {
                if (!send_raw(ws_frame(Op::Binary, frame_payload(*jpeg)))) break;
            }
            gate.sent(seq, t);
        }
    }
    if (stop_ && open) send_raw(ws_close_frame(1001, "shutdown"));
    if (c->kick) {
        send_text(msg_error("unauthorized", "Kopplung am PC entfernt"));
        send_raw(ws_close_frame(1008, "removed"));
    }
    finish();
}

}  // namespace fl::companion
