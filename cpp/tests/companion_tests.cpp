// © 2026 FuchsLabs – Tests des Companion-Protokolls (plattformneutral: Windows-Build fl-companion-tests, Linux:
// tests/build-linux.sh). Der Integrationstest startet den echten TLS-WebSocket-Server auf 127.0.0.1 an einem
// zufälligen Port (nie 8765) und spricht ihn mit einem kleinen Test-Client an.
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

#include "companion.h"
#include "companion_server.h"

using namespace fl::companion;
namespace fs = std::filesystem;

static int g_fail = 0, g_ok = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) ++g_ok;                                                 \
        else {                                                            \
            ++g_fail;                                                     \
            std::printf("FEHLER %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))

static std::string client_frame(Op op, const std::string& payload) { return ws_frame(op, payload, 0x12345678u); }

static void test_crypto() {
    bool ok = false;
    CHECK_EQ(base64("hallo"), "aGFsbG8=");
    CHECK_EQ(unbase64("aGFsbG8=", &ok), "hallo");
    CHECK(ok);
    CHECK_EQ(unbase64("aGFsbA==", &ok), "hall");
    unbase64("aGF$bG8=", &ok);
    CHECK(!ok);
    unbase64("aGFsbG8", &ok);
    CHECK(!ok);
    unbase64("aG=sbG8=", &ok);
    CHECK(!ok);
    std::string raw = random_bytes(32);
    CHECK_EQ(unbase64(base64(raw), &ok), raw);
    CHECK_EQ(sha256_hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(equal_ct("abc", "abc"));
    CHECK(!equal_ct("abc", "abd"));
    CHECK(!equal_ct("abc", "ab"));
    for (int i = 0; i < 50; ++i) {
        std::string p = random_pin();
        CHECK(p.size() == 6 && p.find_first_not_of("0123456789") == std::string::npos);
    }
    CHECK_EQ(format_fingerprint("ab01"), "AB:01");
}

static void test_net() {
    CHECK(is_lan_ipv4("192.168.1.20"));
    CHECK(is_lan_ipv4("10.0.0.1"));
    CHECK(is_lan_ipv4("172.16.0.1"));
    CHECK(is_lan_ipv4("172.31.255.255"));
    CHECK(!is_lan_ipv4("172.32.0.1"));
    CHECK(!is_lan_ipv4("8.8.8.8"));
    CHECK(!is_lan_ipv4("127.0.0.1"));
    CHECK(!is_lan_ipv4("169.254.1.1"));
    CHECK(!is_lan_ipv4("192.168.1"));
    CHECK(!is_lan_ipv4("192.168.01.1"));
    CHECK(!is_lan_ipv4("192.168.1.256"));
    CHECK(!is_lan_ipv4("192.168.1.1 "));
    CHECK(is_loopback_ipv4("127.0.0.1"));
    CHECK(valid_port(8777));
    CHECK(!valid_port(8765));
    CHECK(!valid_port(8777, 8777));
    CHECK(!valid_port(80));
    CHECK(!valid_port(70000));
}

static const char* REQ =
    "GET / HTTP/1.1\r\nHost: 192.168.1.20:8777\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";

static void test_handshake() {
    CHECK_EQ(ws_accept("dGhlIHNhbXBsZSBub25jZQ=="), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");  // Beispiel aus RFC 6455
    Upgrade u = ws_upgrade(REQ);
    CHECK(u.ok);
    CHECK(u.response.find("101 Switching Protocols") != std::string::npos);
    CHECK(u.response.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != std::string::npos);
    std::string r = REQ;
    std::string with_origin = r;
    with_origin.insert(with_origin.size() - 2, "Origin: http://evil.example\r\n");
    CHECK(!ws_upgrade(with_origin).ok);
    CHECK(ws_upgrade(with_origin).response.find("403") != std::string::npos);
    std::string v12 = r;
    v12.replace(v12.find("Version: 13"), 11, "Version: 12");
    CHECK(!ws_upgrade(v12).ok);
    std::string nokey = r;
    nokey.replace(nokey.find("dGhlIHNhbXBsZSBub25jZQ=="), 24, "kurz");
    CHECK(!ws_upgrade(nokey).ok);
    std::string post = r;
    post.replace(0, 3, "POST");
    CHECK(!ws_upgrade(post).ok);
    CHECK(!ws_upgrade("Unsinn\r\n\r\n").ok);
    CHECK(!ws_upgrade("GET / HTTP/1.1\r\nUpgrade: websocket\r\n").ok);  // unvollständig
    std::string keepalive = r;
    keepalive.replace(keepalive.find("Connection: Upgrade"), 19, "Connection: keep-alive, Upgrade");
    CHECK(ws_upgrade(keepalive).ok);
}

static void test_frames() {
    WsParser p;
    WsMessage m;
    std::string err;
    p.feed(client_frame(Op::Text, "hallo"));
    CHECK(p.next(m, &err) == WsParser::Status::Message);
    CHECK(m.op == Op::Text && m.payload == "hallo");
    CHECK(p.next(m, &err) == WsParser::Status::NeedMore);
    // 126er- und 127er-Längen, stückweise gefüttert
    for (size_t n : {125u, 126u, 300u, 70000u}) {
        WsParser q(100000);
        std::string data(n, 'x');
        std::string f = client_frame(Op::Text, data);
        for (size_t i = 0; i < f.size(); i += 7) q.feed(f.substr(i, 7));
        CHECK(q.next(m) == WsParser::Status::Message);
        CHECK(m.payload == data);
    }
    // zu groß
    {
        WsParser q(1000);
        q.feed(client_frame(Op::Text, std::string(2000, 'x')));
        CHECK(q.next(m) == WsParser::Status::Error);
    }
    // unmaskiert vom Client: Fehler
    {
        WsParser q;
        q.feed(ws_frame(Op::Text, "x"));
        CHECK(q.next(m) == WsParser::Status::Error);
    }
    // Fragmentiert mit Ping dazwischen
    {
        WsParser q;
        std::string a = client_frame(Op::Text, "hal");
        a[0] = (char)0x01;  // FIN=0, Text
        std::string b = client_frame(Op::Cont, "lo");
        q.feed(a + client_frame(Op::Ping, "p") + b);
        CHECK(q.next(m) == WsParser::Status::Message);
        CHECK(m.op == Op::Ping && m.payload == "p");
        CHECK(q.next(m) == WsParser::Status::Message);
        CHECK(m.op == Op::Text && m.payload == "hallo");
    }
    // Steuerrahmen zu lang / RSV gesetzt / unbekannter Opcode
    {
        WsParser q;
        q.feed(client_frame(Op::Ping, std::string(126, 'x')));
        CHECK(q.next(m) == WsParser::Status::Error);
        WsParser r;
        std::string f = client_frame(Op::Text, "x");
        f[0] = (char)(f[0] | 0x40);
        r.feed(f);
        CHECK(r.next(m) == WsParser::Status::Error);
        WsParser s;
        std::string g = client_frame(Op::Text, "x");
        g[0] = (char)0x83;
        s.feed(g);
        CHECK(s.next(m) == WsParser::Status::Error);
    }
    // Serverrahmen: unmaskiert, Länge korrekt kodiert
    std::string big = ws_frame(Op::Binary, std::string(70000, 'y'));
    CHECK((unsigned char)big[0] == 0x82 && (unsigned char)big[1] == 127 && big.size() == 70000 + 10);
    std::string close = ws_close_frame(1008, "pin");
    CHECK((unsigned char)close[0] == 0x88 && (unsigned char)close[2] == 0x03 && (unsigned char)close[3] == 0xF0);
}

static void test_messages() {
    auto h = parse_client(R"({"t":"hello","v":1,"pin":"123456","w":1080,"h":2400,"name":"Pixel\u0007 8"})");
    CHECK(h.type == ClientMsg::Type::Hello);
    CHECK(h.pin == "123456" && h.key.empty() && h.w == 1080 && h.h == 2400);
    CHECK_EQ(h.name, "Pixel 8");
    auto k = parse_client(R"({"t":"hello","v":1,"key":"abc"})");
    CHECK(k.type == ClientMsg::Type::Hello && k.key == "abc");
    CHECK(parse_client(R"({"t":"hello","v":1,"key":"a","pin":"1"})").type == ClientMsg::Type::Invalid);
    CHECK(parse_client(R"({"t":"hello","v":1})").type == ClientMsg::Type::Invalid);
    CHECK(parse_client(R"({"t":"hello","pin":"1"})").type == ClientMsg::Type::Invalid);
    auto t = parse_client(R"({"t":"touch","x":10,"y":319})");
    CHECK(t.type == ClientMsg::Type::Touch && t.x == 10 && t.y == 319);
    CHECK(parse_client(R"({"t":"touch","x":-1,"y":3})").type == ClientMsg::Type::Invalid);
    CHECK(parse_client(R"({"t":"touch","x":1.5,"y":3})").type == ClientMsg::Type::Invalid);
    CHECK(parse_client(R"({"t":"ping"})").type == ClientMsg::Type::Ping);
    CHECK(parse_client(R"({"t":"pong"})").type == ClientMsg::Type::Pong);
    CHECK(parse_client(R"({"t":"zukunft"})").type == ClientMsg::Type::Unknown);
    CHECK(parse_client("{kaputt").type == ClientMsg::Type::Invalid);
    CHECK(parse_client("[1]").type == ClientMsg::Type::Invalid);
    CHECK_EQ(msg_ready(480, 320), R"({"t":"ready","w":480,"h":320})");
    CHECK_EQ(msg_error("pin_wrong", "PIN falsch"), R"({"t":"error","code":"pin_wrong","msg":"PIN falsch"})");
    CHECK_EQ(frame_payload("J"), std::string("\x01J"));
    CHECK_EQ(clean_name(std::string(60, 'a')).size(), (size_t)40);
    CHECK_EQ(clean_name("Ä\xC3"), "Ä");  // abgeschnittenes UTF-8-Zeichen fällt weg
}

static void test_pairing_uri_qr() {
    std::string fp(64, 'a');
    CHECK_EQ(pair_uri("192.168.1.20", 8777, fp, "012345"), "fuchslabs://pair?host=192.168.1.20&port=8777&fp=" + fp + "&pin=012345");
    CHECK_EQ(pair_uri("a&b", 8777, fp, "1"), "fuchslabs://pair?host=a%26b&port=8777&fp=" + fp + "&pin=1");
    std::string svg = qr_svg(pair_uri("192.168.1.20", 8777, fp, "012345"));
    CHECK(svg.rfind("<svg", 0) == 0);
    CHECK(svg.find("<path d=\"M") != std::string::npos);
}

static void test_pairing() {
    double t = 1000;
    int n = 0;
    Pairing p([&] { return t; }, [&] { return std::to_string(100000 + n++); });
    CHECK(p.check("100000") == Pairing::Result::NoPin);
    auto c = p.start();
    CHECK(c && c->pin == "100000");
    CHECK(p.check("999999") == Pairing::Result::Wrong);
    CHECK(p.check("100000") == Pairing::Result::Ok);
    CHECK(p.check("100000") == Pairing::Result::NoPin);  // einmalig
    c = p.start();
    t += PIN_SECONDS + 1;
    CHECK(p.check(c->pin) == Pairing::Result::Expired);
    CHECK(!p.active());
    // 5 Fehlversuche: Sperre, PIN verfällt, kein neuer Code
    c = p.start();
    for (int i = 0; i < MAX_FAILURES; ++i) CHECK(p.check("000000") == Pairing::Result::Wrong);
    CHECK(p.locked_for() > 0);
    CHECK(p.check(c->pin) == Pairing::Result::Locked);
    CHECK(!p.start());
    t += LOCK_SECONDS + 1;
    CHECK(p.locked_for() == 0);
    CHECK(p.failures() == 0);
    c = p.start();
    CHECK(c.has_value());
    CHECK(p.check(c->pin) == Pairing::Result::Ok);
    // Fehlversuche über mehrere Codes hinweg zählen weiter
    for (int i = 0; i < 3; ++i) {
        p.start();
        CHECK(p.check("x") == Pairing::Result::Wrong);
    }
    p.start();
    CHECK(p.check("x") == Pairing::Result::Wrong);
    p.start();
    CHECK(p.check("x") == Pairing::Result::Wrong);
    CHECK(p.locked_for() > 0);
}

static void test_gate() {
    FrameGate g;
    CHECK(!g.due(0, 0));  // noch kein Bild
    CHECK(g.due(1, 10));  // erstes Bild sofort
    g.sent(1, 10);
    CHECK(!g.due(1, 20));     // unverändert: nichts senden
    CHECK(!g.due(2, 10.1));   // zu früh (5 fps)
    CHECK(g.due(2, 10.2));
    g.reset();
    CHECK(g.due(2, 10.21));   // nach neuer Anmeldung sofort ein Vollbild
}

static fs::path tmpfile(const char* name) {
    auto p = fs::temp_directory_path() / name;
    std::error_code ec;
    fs::remove(p, ec);
    return p;
}

static Store::Codec enc = [](const std::string& s) { return "enc:" + base64(s); };
static Store::Codec dec = [](const std::string& s) {
    if (s.rfind("enc:", 0) != 0) return std::string();
    return unbase64(s.substr(4));
};

static void test_store() {
    auto path = tmpfile("fl-companion-store.json");
    {
        Store s(path, enc, dec, [] { return 42.0; });
        s.load();
        CHECK(s.list().empty());
        std::string id;
        std::string key = s.add_device("Pixel", &id);
        CHECK_EQ(key.size(), (size_t)32);
        CHECK(s.find(key).has_value());
        CHECK(!s.find(std::string(32, 'x')).has_value());
        s.set_identity("CERT", "KEY");
    }
    {
        std::ifstream f(path);
        std::string text((std::istreambuf_iterator<char>(f)), {});
        CHECK(text.find("\"enc:") != std::string::npos);  // Schlüssel nur geschützt in der Datei
        CHECK(text.find("KEY\"") == std::string::npos);
    }
    Store s(path, enc, dec, [] { return 43.0; });
    s.load();
    CHECK_EQ(s.list().size(), (size_t)1);
    CHECK_EQ(s.key_pem(), "KEY");
    CHECK_EQ(s.cert_pem(), "CERT");
    std::string id = s.list()[0]["id"].get<std::string>();
    CHECK(s.list()[0].contains("key") == false);
    CHECK(s.remove(id));
    CHECK(!s.remove(id));
    for (int i = 0; i < 12; ++i) s.add_device("Gerät " + std::to_string(i));
    CHECK_EQ(s.list().size(), MAX_DEVICES);
    // Mit fremdem Schutz (anderes Konto) bleibt nichts Brauchbares übrig
    Store other(path, enc, [](const std::string&) { return std::string(); }, [] { return 0.0; });
    other.load();
    CHECK(other.list().empty());
    CHECK(other.cert_pem().empty());
    fs::remove(path);
}

static void test_identity() {
    Identity id = generate_identity("FuchsLabs USB-Display");
    CHECK(id.cert_pem.find("BEGIN CERTIFICATE") != std::string::npos);
    CHECK(id.key_pem.find("PRIVATE KEY") != std::string::npos);
    std::string fp = cert_fingerprint(id.cert_pem);
    CHECK_EQ(fp.size(), (size_t)64);
    CHECK_EQ(fp, cert_fingerprint(id.cert_pem));
    CHECK(cert_fingerprint("Unsinn").empty());
    CHECK(fp != cert_fingerprint(generate_identity("x").cert_pem));
}

// ---------- Integration: echter Server, Test-Client über TLS ----------
#ifdef _WIN32
using sock_t = SOCKET;
static void sclose(sock_t s) { closesocket(s); }
#else
using sock_t = int;
static void sclose(sock_t s) { ::close(s); }
#endif

struct Client {
    sock_t s = (sock_t)-1;
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    WsParser parser{1 << 22, false};
    std::string fingerprint, http;

    bool open(int port, const std::string& extra_headers = "") {
        s = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons((uint16_t)port);
        inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
        if (::connect(s, (sockaddr*)&sa, sizeof sa) != 0) return false;
#ifdef _WIN32
        DWORD tv = 3000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#else
        timeval tv{3, 0};
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
#endif
        ctx = SSL_CTX_new(TLS_client_method());
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, (int)s);
        if (SSL_connect(ssl) != 1) return false;
        X509* cert = SSL_get1_peer_certificate(ssl);
        unsigned char md[32];
        unsigned n = 0;
        X509_digest(cert, EVP_sha256(), md, &n);
        X509_free(cert);
        static const char hex[] = "0123456789abcdef";
        for (unsigned i = 0; i < n; ++i) {
            fingerprint += hex[md[i] >> 4];
            fingerprint += hex[md[i] & 15];
        }
        std::string req = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                          "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n" +
                          extra_headers + "\r\n";
        SSL_write(ssl, req.data(), (int)req.size());
        char buf[4096];
        while (http.find("\r\n\r\n") == std::string::npos) {
            int r = SSL_read(ssl, buf, sizeof buf);
            if (r <= 0) return false;
            http.append(buf, r);
        }
        size_t end = http.find("\r\n\r\n") + 4;
        parser.feed(http.substr(end));
        http.resize(end);
        return http.find(" 101 ") != std::string::npos;
    }
    void send(const std::string& text) {
        std::string f = client_frame(Op::Text, text);
        SSL_write(ssl, f.data(), (int)f.size());
    }
    void send_raw(const std::string& f) { SSL_write(ssl, f.data(), (int)f.size()); }
    // nächste Nachricht (Ping vom Server wird übersprungen, wenn skip_ping)
    std::optional<WsMessage> recv(bool skip_ping = true) {
        char buf[16384];
        for (;;) {
            WsMessage m;
            auto st = parser.next(m);
            if (st == WsParser::Status::Message) {
                if (skip_ping && m.op == Op::Text && m.payload == msg_ping()) continue;
                return m;
            }
            if (st == WsParser::Status::Error) return std::nullopt;
            int r = SSL_read(ssl, buf, sizeof buf);
            if (r <= 0) return std::nullopt;
            parser.feed(buf, (size_t)r);
        }
    }
    void close() {
        if (ssl) SSL_free(ssl);
        if (ctx) SSL_CTX_free(ctx);
        if (s != (sock_t)-1) sclose(s);
        ssl = nullptr;
        ctx = nullptr;
        s = (sock_t)-1;
    }
    ~Client() { close(); }
};

static std::string code_of(const std::optional<WsMessage>& m) {
    if (!m || m->op != Op::Text) return "";
    auto j = json::parse(m->payload, nullptr, false);
    return j.is_object() && j.value("t", "") == "error" ? j.value("code", "") : "";
}

static void test_server() {
#ifdef _WIN32
    WSADATA d;
    WSAStartup(MAKEWORD(2, 2), &d);
#endif
    auto path = tmpfile("fl-companion-server.json");
    Store store(path, enc, dec, [] { return 1.0; });
    store.load();
    std::atomic<double> offset{0};
    std::atomic<int> encodes{0}, tx{-1}, ty{-1};
    Server::Hooks hooks;
    hooks.encode = [&] {
        ++encodes;
        return std::string("JPEGDATA");
    };
    hooks.on_touch = [&](int x, int y) {
        tx = x;
        ty = y;
    };
    hooks.now = [&] {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count() + offset.load();
    };
    std::mt19937 rng((unsigned)std::random_device{}());
    std::unique_ptr<Server> srv;
    int port = 0;
    for (int attempt = 0; attempt < 20 && !srv; ++attempt) {
        port = 20000 + (int)(rng() % 40000);
        if (port == SETTINGS_PORT || port == DEFAULT_PORT) continue;
        Server::Options o;
        o.bind = {"127.0.0.1"};
        o.port = port;
        o.allow_loopback = true;
        auto s = std::make_unique<Server>(o, store, hooks);
        try {
            s->start();
            srv = std::move(s);
        } catch (const std::exception&) {
        }
    }
    CHECK(srv != nullptr);
    if (!srv) return;
    // Ohne allow_loopback wird 127.0.0.1 nicht gebunden: kein Heimnetz → Fehler
    {
        Server::Options o;
        o.bind = {"127.0.0.1"};
        o.port = port + 1 == SETTINGS_PORT ? port + 2 : port + 1;
        Server s(o, store, hooks);
        bool threw = false;
        try {
            s.start();
        } catch (const std::exception&) {
            threw = true;
        }
        CHECK(threw);
    }
    // Port 8765 wird nie gestartet
    {
        Server::Options o;
        o.bind = {"127.0.0.1"};
        o.port = SETTINGS_PORT;
        o.allow_loopback = true;
        Server s(o, store, hooks);
        bool threw = false;
        try {
            s.start();
        } catch (const std::exception&) {
            threw = true;
        }
        CHECK(threw);
    }

    std::string fp = srv->fingerprint();
    CHECK_EQ(fp.size(), (size_t)64);
    srv->frame_changed();

    // Browser (Origin) wird abgewiesen
    {
        Client c;
        CHECK(!c.open(port, "Origin: http://evil.example\r\n"));
        CHECK(c.http.find("403") != std::string::npos);
    }
    // Ohne gültigen Code: pin_expired
    {
        Client c;
        CHECK(c.open(port));
        CHECK_EQ(c.fingerprint, fp);  // Pinning-Grundlage: Fingerabdruck des Zertifikats
        c.send(R"({"t":"hello","v":1,"pin":"123456","name":"Test"})");
        CHECK_EQ(code_of(c.recv()), "pin_expired");
    }
    auto code = srv->pairing().start();
    CHECK(code.has_value());
    std::string wrong = code->pin == "000000" ? "111111" : "000000";
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"pin":")" + wrong + R"("})");
        CHECK_EQ(code_of(c.recv()), "pin_wrong");
    }
    // Falsche Version
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":2,"pin":"1"})");
        CHECK_EQ(code_of(c.recv()), "version");
    }
    // Kopplung mit richtiger PIN: paired, ready, sofort ein Vollbild
    std::string key_b64;
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"pin":")" + code->pin + R"(","w":1080,"h":2400,"name":"Pixel"})");
        auto m = c.recv();
        CHECK(m && m->op == Op::Text);
        auto j = json::parse(m ? m->payload : "{}", nullptr, false);
        CHECK(j.value("t", "") == "paired");
        key_b64 = j.value("key", "");
        bool ok = false;
        CHECK_EQ(unbase64(key_b64, &ok).size(), (size_t)32);
        m = c.recv();
        CHECK(m && m->payload == msg_ready(480, 320));
        m = c.recv();
        CHECK(m && m->op == Op::Binary && m->payload == std::string("\x01") + "JPEGDATA");
        CHECK_EQ(srv->clients().size(), (size_t)1);
        // Touch landet in der Touch-Logik; außerhalb der Fläche nicht
        c.send(R"({"t":"touch","x":500,"y":10})");
        c.send(R"({"t":"touch","x":12,"y":34})");
        c.send(R"({"t":"ping"})");
        m = c.recv();
        CHECK(m && m->payload == msg_pong());
        CHECK(tx == 12 && ty == 34);
        // WebSocket-Ping (Steuerrahmen) → Pong mit gleichem Inhalt
        c.send_raw(client_frame(Op::Ping, "abc"));
        m = c.recv();
        CHECK(m && m->op == Op::Pong && m->payload == "abc");
        // Viele Änderungen in kurzer Zeit: höchstens ~5 Bilder pro Sekunde, kein Bild ohne Änderung
        int frames = 0;
        auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
        std::thread pump([&] {
            while (std::chrono::steady_clock::now() < until) {
                srv->frame_changed();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        c.send(R"({"t":"ping"})");
        while (std::chrono::steady_clock::now() < until + std::chrono::milliseconds(300)) {
            auto r = c.recv();
            if (!r) break;
            if (r->op == Op::Binary) ++frames;
            if (r->op == Op::Text && r->payload == msg_pong()) c.send(R"({"t":"ping"})");
        }
        pump.join();
        CHECK(frames >= 3 && frames <= 7);
        std::printf("  %d Bilder in ~1 s bei 50 Änderungen/s\n", frames);
    }
    // Code ist verbraucht
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"pin":")" + code->pin + R"("})");
        CHECK_EQ(code_of(c.recv()), "pin_expired");
    }
    // Wiederverbinden mit Geräteschlüssel
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"key":")" + key_b64 + R"("})");
        auto m = c.recv();
        CHECK(m && m->payload == msg_ready(480, 320));
        m = c.recv();
        CHECK(m && m->op == Op::Binary);
        // 15 s Stille (Uhr vorgestellt): Server trennt
        offset = offset + 20;
        bool closed = false;
        for (int i = 0; i < 10 && !closed; ++i) {
            auto r = c.recv();
            if (!r || r->op == Op::Close) closed = true;
        }
        CHECK(closed);
    }
    // Unbekannter Schlüssel
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"key":")" + base64(std::string(32, 'k')) + R"("})");
        CHECK_EQ(code_of(c.recv()), "unauthorized");
    }
    // Touch ohne Anmeldung
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"touch","x":1,"y":1})");
        CHECK_EQ(code_of(c.recv()), "unauthorized");
    }
    // Entferntes Gerät: Schlüssel gilt nicht mehr
    {
        auto list = store.list();
        CHECK_EQ(list.size(), (size_t)1);
        store.remove(list[0]["id"].get<std::string>());
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"key":")" + key_b64 + R"("})");
        CHECK_EQ(code_of(c.recv()), "unauthorized");
    }
    // 5 Fehlversuche → gesperrt, auch die richtige PIN hilft nicht mehr
    code = srv->pairing().start();
    wrong = code->pin == "000000" ? "111111" : "000000";
    std::string last;
    for (int i = 0; i < MAX_FAILURES; ++i) {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"pin":")" + wrong + R"("})");
        last = code_of(c.recv());
    }
    CHECK_EQ(last, "locked");
    {
        Client c;
        CHECK(c.open(port));
        c.send(R"({"t":"hello","v":1,"pin":")" + code->pin + R"("})");
        CHECK_EQ(code_of(c.recv()), "locked");
    }
    CHECK(!srv->pairing().start());
    // Ungültiges JSON / unmaskierter Rahmen
    {
        Client c;
        CHECK(c.open(port));
        c.send("{kaputt");
        CHECK_EQ(code_of(c.recv()), "bad_message");
    }
    {
        Client c;
        CHECK(c.open(port));
        c.send_raw(ws_frame(Op::Text, R"({"t":"ping"})"));
        auto m = c.recv();
        CHECK(!m || m->op == Op::Close);
    }
    srv->stop();
    CHECK(!srv->running());
    CHECK(encodes >= 1);
    std::error_code ec;
    fs::remove(path, ec);
}

int main() {
    test_crypto();
    test_net();
    test_handshake();
    test_frames();
    test_messages();
    test_pairing_uri_qr();
    test_pairing();
    test_gate();
    test_store();
    test_identity();
    test_server();
    std::printf("%d Prüfungen ok, %d Fehler\n", g_ok, g_fail);
    return g_fail ? 1 : 0;
}
