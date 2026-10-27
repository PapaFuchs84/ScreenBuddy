// © 2026 FuchsLabs
#include "plugin_transport.h"
#include <algorithm>
#include <charconv>
#include <set>
#include <atomic>
#include <cstring>
#include <memory>
#include <limits>
#include <type_traits>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/sha.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#endif

namespace fl::plugins {
namespace {
DataResult failure(const char* text) { return {std::nullopt, text}; }
std::string lower(std::string_view text) {
    std::string out(text);
    for (char& c : out) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return out;
}
std::string_view trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
    return text;
}
bool number(std::string_view text, size_t& out, int base = 10) {
    if (text.empty()) return false;
    auto result = std::from_chars(text.data(), text.data() + text.size(), out, base);
    return result.ec == std::errc() && result.ptr == text.data() + text.size();
}
bool public_bytes(const unsigned char* b, size_t length) {
    if (length == 4) {
        const uint32_t address = uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 8 | b[3];
        struct Range { uint32_t network; unsigned bits; };
        constexpr Range denied[] = {{0,8}, {0x0a000000,8}, {0x64400000,10}, {0x7f000000,8},
            {0xa9fe0000,16}, {0xac100000,12}, {0xc0000000,24}, {0xc0000200,24},
            {0xc0586300,24}, {0xc0a80000,16}, {0xc6120000,15}, {0xc6336400,24},
            {0xcb007100,24}, {0xe0000000,3}};
        for (const auto& range : denied)
            if ((address & (0xffffffffu << (32 - range.bits))) == range.network) return false;
        return true;
    }
    // Only native global unicast. Reject mapped/NAT64/6to4/Teredo and special
    // allocations, rather than trusting an embedded address or local translator.
    return length == 16 && (b[0] & 0xe0) == 0x20 &&
        !(b[0] == 0x20 && b[1] == 0x01 && b[2] <= 1) &&
        !(b[0] == 0x20 && b[1] == 0x01 && b[2] == 0x0d && b[3] == 0xb8) &&
        !(b[0] == 0x20 && b[1] == 0x02) &&
        !(b[0] == 0x3f && b[1] == 0xff && (b[2] & 0xf0) == 0);
}
bool request_valid(const Request& request, std::string& target) {
    if (request.host.empty() || request.host.size() > 253 || request.host.find('.') == std::string::npos ||
        request.host.back() == '.' || request.headers.size() > 16 || request.url.size() > manifest_limit) return false;
    for (unsigned char c : request.host)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.')) return false;
    size_t label_start = 0;
    while (label_start < request.host.size()) {
        auto end = request.host.find('.', label_start);
        if (end == request.host.npos) end = request.host.size();
        if (end == label_start || end - label_start > 63 || request.host[label_start] == '-' || request.host[end - 1] == '-') return false;
        label_start = end + 1;
    }
    const auto last_dot = request.host.rfind('.');
    if (std::all_of(request.host.begin() + last_dot + 1, request.host.end(), [](char c) { return c >= '0' && c <= '9'; })) return false;
    if (request.host.ends_with(".local") || request.host.ends_with(".localhost") ||
        request.host.ends_with(".internal") || request.host.ends_with(".lan") ||
        request.host.ends_with(".home") || request.host == "home.arpa" || request.host.ends_with(".home.arpa")) return false;
    if (!request.url.starts_with("https://")) return false;
    const auto end = request.url.find_first_of("/?", 8);
    auto authority = lower(request.url.substr(8, end == std::string::npos ? end : end - 8));
    if (authority.ends_with(":443")) authority.resize(authority.size() - 4);
    if (authority != request.host) return false;
    target = end == std::string::npos ? "/" : request.url.substr(end);
    if (target.front() == '?') target.insert(target.begin(), '/');
    for (unsigned char c : target) if (c <= 32 || c >= 127 || c == '\\' || c == '#') return false;
    std::set<std::string> names;
    size_t total = 0;
    for (const auto& [key, value] : request.headers) {
        const auto name = lower(key);
        if (name != "accept" && name != "authorization" && !name.starts_with("x-")) return false;
        for (unsigned char c : name)
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
        if (!names.insert(name).second) return false;
        for (unsigned char c : value) if (c < 32 || c == 127) return false;
        total += key.size() + value.size();
        if (total > manifest_limit) return false;
    }
    return true;
}
#ifdef _WIN32
using Clock = std::chrono::steady_clock;
struct DnsQuery {
    OVERLAPPED overlapped{}; // first member: completion routine receives this address
    std::atomic<unsigned> references{2}; // caller and completion routine
    std::atomic<DWORD> error{WSA_IO_PENDING};
    PADDRINFOEXW results = nullptr;
    HANDLE cancel = nullptr;
    HANDLE done = nullptr;
    std::wstring host;
    bool winsock = false;
    DnsQuery() {
        WSADATA data{}; winsock = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        // GetAddrInfoExW requires OVERLAPPED.hEvent == NULL when a completion
        // routine is supplied. Keep our wait event outside OVERLAPPED.
        done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }
    ~DnsQuery() {
        if (results) FreeAddrInfoExW(results);
        if (done) CloseHandle(done);
        if (winsock) WSACleanup();
    }
    void release() { if (references.fetch_sub(1) == 1) delete this; }
    std::atomic<bool> finished{false};
    // Documented contract (and the Microsoft GetAddrInfoExW sample): a result other than WSA_IO_PENDING
    // means the call completed synchronously and the routine is NOT invoked, so the caller completes
    // manually. Defensively, the completion is one-shot: if a routine ever ran additionally, the
    // second call is a no-op and cannot release the completion reference a second time.
    static void CALLBACK complete(DWORD error, DWORD, LPOVERLAPPED overlapped) {
        auto* query = reinterpret_cast<DnsQuery*>(overlapped);
        if (query->finished.exchange(true)) return;
        query->error.store(error);
        SetEvent(query->done);
        query->release();
    }
};
static_assert(std::is_standard_layout_v<DnsQuery>); // OVERLAPPED-to-owner conversion above
struct Socket {
    SOCKET value = INVALID_SOCKET;
    ~Socket() { if (value != INVALID_SOCKET) closesocket(value); }
};
bool permitted(const Authorization& authorized, Clock::time_point deadline) {
    return Clock::now() < deadline && authorized && authorized();
}
bool wait_socket(SOCKET socket, bool write, Clock::time_point deadline, const Authorization& authorized) {
    while (permitted(authorized, deadline)) {
        fd_set read_set, write_set, errors;
        FD_ZERO(&read_set); FD_ZERO(&write_set); FD_ZERO(&errors);
        FD_SET(socket, &errors);
        if (write) FD_SET(socket, &write_set); else FD_SET(socket, &read_set);
        timeval interval{0, 20000};
        const int n = select(0, &read_set, &write_set, &errors, &interval);
        if (n < 0 || FD_ISSET(socket, &errors)) return false;
        if (n > 0) return permitted(authorized, deadline);
    }
    return false;
}
bool wait_tls(SSL* tls, int result, SOCKET socket, Clock::time_point deadline, const Authorization& authorized) {
    const int error = SSL_get_error(tls, result);
    return (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) &&
        wait_socket(socket, error == SSL_ERROR_WANT_WRITE, deadline, authorized);
}
bool address_public(const sockaddr* address) {
    if (address->sa_family == AF_INET)
        return public_bytes(reinterpret_cast<const unsigned char*>(&reinterpret_cast<const sockaddr_in*>(address)->sin_addr), 4);
    if (address->sa_family == AF_INET6) {
        const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(address);
        return !ipv6->sin6_scope_id && public_bytes(reinterpret_cast<const unsigned char*>(&ipv6->sin6_addr), 16);
    }
    return false;
}
bool same_peer(SOCKET socket, const sockaddr* expected) {
    sockaddr_storage peer{}; int length = sizeof peer;
    if (getpeername(socket, reinterpret_cast<sockaddr*>(&peer), &length) || peer.ss_family != expected->sa_family ||
        !address_public(reinterpret_cast<sockaddr*>(&peer))) return false;
    if (expected->sa_family == AF_INET) {
        const auto* a = reinterpret_cast<const sockaddr_in*>(expected);
        const auto* b = reinterpret_cast<const sockaddr_in*>(&peer);
        return a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr;
    }
    const auto* a = reinterpret_cast<const sockaddr_in6*>(expected);
    const auto* b = reinterpret_cast<const sockaddr_in6*>(&peer);
    return a->sin6_port == b->sin6_port && !std::memcmp(&a->sin6_addr, &b->sin6_addr, 16);
}
bool load_roots(SSL_CTX* context, Clock::time_point deadline, const Authorization& authorized) {
    HCERTSTORE roots = CertOpenSystemStoreW(0, L"ROOT");
    if (!roots) return false;
    PCCERT_CONTEXT certificate = nullptr;
    size_t loaded = 0;
    while ((certificate = CertEnumCertificatesInStore(roots, certificate))) {
        if (!permitted(authorized, deadline)) { CertFreeCertificateContext(certificate); loaded = 0; break; }
        const auto* bytes = certificate->pbCertEncoded;
        X509* x509 = d2i_X509(nullptr, &bytes, certificate->cbCertEncoded);
        if (x509) {
            if (X509_STORE_add_cert(SSL_CTX_get_cert_store(context), x509) == 1) ++loaded;
            X509_free(x509);
        }
    }
    CertCloseStore(roots, 0);
    return loaded > 0;
}
#endif
}
bool public_source_ip(const std::string& address) {
    if (address.find('\0') != address.npos) return false;
    ASN1_OCTET_STRING* parsed = a2i_IPADDRESS(address.c_str());
    if (!parsed) return false;
    const bool result = public_bytes(ASN1_STRING_get0_data(parsed), ASN1_STRING_length(parsed));
    ASN1_OCTET_STRING_free(parsed);
    return result;
}
#if defined(_WIN32) && defined(FL_PLUGIN_TRANSPORT_TEST)
bool dns_completion_contract_for_test() {
    auto release = [](DnsQuery* query) { query->release(); };
    std::unique_ptr<DnsQuery, decltype(release)> dns(new DnsQuery, release);
    if (!dns->winsock || !dns->done) { dns->release(); return false; }
    ADDRINFOEXW hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICHOST;
    const auto error = GetAddrInfoExW(L"127.0.0.1", L"443", NS_ALL, nullptr, &hints,
        &dns->results, nullptr, &dns->overlapped, DnsQuery::complete, &dns->cancel);
    if (error != WSA_IO_PENDING) DnsQuery::complete(error, 0, &dns->overlapped);
    if (WaitForSingleObject(dns->done, 1000) != WAIT_OBJECT_0) {
        if (error == WSA_IO_PENDING) GetAddrInfoExCancel(&dns->cancel);
        return false;
    }
    return dns->error.load() == 0 && dns->results != nullptr;
}
#endif
DataResult fetch_public_json(const Request& request, const Authorization& authorized) {
    std::string target;
    if (!authorized || !authorized()) return failure("Source is no longer authorized.");
    if (!request_valid(request, target)) return failure("Invalid source request.");
#ifndef _WIN32
    return failure("Live plugin transport is only available on Windows.");
#else
    const auto deadline = Clock::now() + std::chrono::seconds(5);
    auto release = [](DnsQuery* query) { query->release(); };
    std::unique_ptr<DnsQuery, decltype(release)> dns(new DnsQuery, release);
    if (!dns->winsock || !dns->done) {
        dns->release(); return failure("Source resolver is unavailable.");
    }
    dns->host.assign(request.host.begin(), request.host.end());
    // A terminal dot prevents DNS suffix search from turning a public-looking
    // manifest host into an unrelated local search-domain name.
    dns->host += L'.';
    ADDRINFOEXW hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP;
    timeval timeout{5, 0};
    const auto error = GetAddrInfoExW(dns->host.c_str(), L"443", NS_DNS, nullptr, &hints,
        &dns->results, &timeout, &dns->overlapped, DnsQuery::complete, &dns->cancel);
    if (error != WSA_IO_PENDING) DnsQuery::complete(error, 0, &dns->overlapped);
    while (WaitForSingleObject(dns->done, 20) == WAIT_TIMEOUT) {
        if (!permitted(authorized, deadline)) {
            GetAddrInfoExCancel(&dns->cancel);
            // Completion owns a reference, so cancellation never requires an
            // unbounded join and cannot access freed OVERLAPPED/result storage.
            return failure("Source request timed out or was revoked.");
        }
    }
    if (dns->error.load() || !dns->results || !permitted(authorized, deadline)) return failure("Source DNS lookup failed.");
    size_t count = 0;
    for (auto* address = dns->results; address; address = address->ai_next)
        if (++count > 32 || !address->ai_addr ||
            (address->ai_family == AF_INET && address->ai_addrlen < sizeof(sockaddr_in)) ||
            (address->ai_family == AF_INET6 && address->ai_addrlen < sizeof(sockaddr_in6)) ||
            !address_public(address->ai_addr)) return failure("Source DNS returned a non-public address.");
    Socket socket;
    for (auto* address = dns->results; address && permitted(authorized, deadline); address = address->ai_next) {
        socket.value = ::socket(address->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (socket.value == INVALID_SOCKET) continue;
        u_long nonblocking = 1;
        if (!ioctlsocket(socket.value, FIONBIO, &nonblocking)) {
            const int result = connect(socket.value, address->ai_addr, static_cast<int>(address->ai_addrlen));
            bool connected = result == 0;
            if (!connected && WSAGetLastError() == WSAEWOULDBLOCK && wait_socket(socket.value, true, deadline, authorized)) {
                int status = -1, length = sizeof status;
                connected = !getsockopt(socket.value, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&status), &length) && !status;
            }
            if (connected && same_peer(socket.value, address->ai_addr)) break;
        }
        closesocket(socket.value); socket.value = INVALID_SOCKET;
    }
    if (socket.value == INVALID_SOCKET || socket.value > static_cast<SOCKET>(std::numeric_limits<int>::max()) ||
        !permitted(authorized, deadline)) return failure("Source connection failed.");
    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> context(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
    if (!context || !SSL_CTX_set_min_proto_version(context.get(), TLS1_2_VERSION) || !load_roots(context.get(), deadline, authorized))
        return failure("Source TLS trust is unavailable.");
    SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER, nullptr);
    std::unique_ptr<SSL, decltype(&SSL_free)> tls(SSL_new(context.get()), SSL_free);
    if (!tls || !SSL_set_fd(tls.get(), static_cast<int>(socket.value)) ||
        !SSL_set_tlsext_host_name(tls.get(), request.host.c_str()) || !SSL_set1_host(tls.get(), request.host.c_str()))
        return failure("Source TLS setup failed.");
    SSL_set_hostflags(tls.get(), X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    while (true) {
        if (!permitted(authorized, deadline)) return failure("Source request timed out or was revoked.");
        ERR_clear_error();
        const int result = SSL_connect(tls.get());
        if (result == 1) break;
        if (!wait_tls(tls.get(), result, socket.value, deadline, authorized)) return failure("Source TLS verification failed.");
    }
    if (SSL_get_verify_result(tls.get()) != X509_V_OK) return failure("Source TLS verification failed.");
    std::string message = "GET " + target + " HTTP/1.1\r\nHost: " + request.host +
        "\r\nConnection: close\r\nAccept-Encoding: identity\r\n";
    bool accept = false;
    for (const auto& [key, value] : request.headers) {
        accept |= lower(key) == "accept";
        message += key + ": " + value + "\r\n";
    }
    if (!accept) message += "Accept: application/json\r\n";
    message += "\r\n";
    size_t sent = 0;
    while (sent < message.size()) {
        if (!permitted(authorized, deadline)) return failure("Source request timed out or was revoked.");
        ERR_clear_error();
        const int result = SSL_write(tls.get(), message.data() + sent, static_cast<int>(message.size() - sent));
        if (result > 0) sent += result;
        else if (!wait_tls(tls.get(), result, socket.value, deadline, authorized)) return failure("Source send failed.");
    }
    std::string response;
    while (true) {
        if (!permitted(authorized, deadline)) return failure("Source request timed out or was revoked.");
        ERR_clear_error();
        char bytes[8192]; const int result = SSL_read(tls.get(), bytes, sizeof bytes);
        if (result > 0) {
            if (response.size() + result > response_limit + 64 * 1024) return failure("Source response exceeds size limit.");
            response.append(bytes, result);
            const auto split = response.find("\r\n\r\n");
            if ((split == response.npos && response.size() > 16 * 1024) || (split != response.npos && split > 16 * 1024))
                return failure("Invalid source response headers.");
        } else {
            if (SSL_get_error(tls.get(), result) == SSL_ERROR_ZERO_RETURN) break;
            if (!wait_tls(tls.get(), result, socket.value, deadline, authorized)) return failure("Source receive failed.");
        }
    }
    if (!permitted(authorized, deadline)) return failure("Source request timed out or was revoked.");
    return decode_source_http(response);
#endif
}
DataResult decode_source_http(std::string_view response) {
    constexpr size_t header_limit = 16 * 1024;
    // Chunk framing is separately bounded; decoded JSON is always <=512 KiB.
    if (response.size() > response_limit + 64 * 1024) return failure("Source response exceeds size limit.");
    const auto split = response.find("\r\n\r\n");
    if (split == response.npos || split > header_limit) return failure("Invalid source response headers.");
    auto headers = response.substr(0, split);
    const auto first = headers.find("\r\n");
    auto status = headers.substr(0, first);
    if ((!status.starts_with("HTTP/1.1 ") && !status.starts_with("HTTP/1.0 ")) || status.size() < 12 ||
        status.substr(9, 3) != "200" || (status.size() > 12 && status[12] != ' '))
        return failure("Source did not return HTTP 200.");
    headers = first == headers.npos ? std::string_view() : headers.substr(first + 2);
    std::map<std::string, std::string> values;
    while (!headers.empty()) {
        auto end = headers.find("\r\n");
        auto line = headers.substr(0, end);
        auto colon = line.find(':');
        if (colon == line.npos || colon == 0 || line.front() == ' ' || line.front() == '\t')
            return failure("Invalid source response headers.");
        const auto key = lower(line.substr(0, colon));
        for (char c : key) if (!((c >= 'a' && c <= 'z') || c == '-' || (c >= '0' && c <= '9')))
            return failure("Invalid source response headers.");
        auto value = trim(line.substr(colon + 1));
        for (unsigned char c : value) if ((c < 32 && c != '\t') || c == 127)
            return failure("Invalid source response headers.");
        if ((key == "content-length" || key == "transfer-encoding" || key == "content-encoding" || key == "content-type") && values.contains(key))
            return failure("Ambiguous source response headers.");
        values[key] = lower(value);
        headers = end == headers.npos ? std::string_view() : headers.substr(end + 2);
    }
    const auto type = trim(std::string_view(values["content-type"]).substr(0, values["content-type"].find(';')));
    if (type != "application/json" && !(type.starts_with("application/") && type.ends_with("+json")))
        return failure("Source response must have a JSON content type.");
    if (values.contains("content-encoding") && values["content-encoding"] != "identity")
        return failure("Compressed source responses are not accepted.");
    auto body = response.substr(split + 4);
    if (values.contains("transfer-encoding")) {
        if (values.contains("content-length") || values["transfer-encoding"] != "chunked")
            return failure("Unsupported source response framing.");
        std::string decoded;
        while (true) {
            auto end = body.find("\r\n");
            size_t count = 0;
            if (end == body.npos || end > 128 || !number(body.substr(0, end), count, 16))
                return failure("Invalid source chunk framing.");
            body.remove_prefix(end + 2);
            if (!count) {
                if (body != "\r\n") return failure("Source trailers are not accepted.");
                return parse_source_data(decoded);
            }
            if (count > response_limit - decoded.size()) return failure("Source response exceeds size limit.");
            if (body.size() < count + 2 || body.substr(count, 2) != "\r\n") return failure("Incomplete source response.");
            decoded.append(body.substr(0, count)); body.remove_prefix(count + 2);
        }
    }
    if (values.contains("content-length")) {
        size_t count = 0;
        if (!number(values["content-length"], count) || count != body.size()) return failure("Incomplete source response.");
    }
    return parse_source_data(body);
}
Transport::Transport(Fetch fetch, Now now) : fetch_(std::move(fetch)), now_(std::move(now)) {}
void Transport::clear() { cache_.clear(); }
SourceData Transport::collect(Store& store, const std::string& id, const Json& settings, const Authorization& extra_authorized) {
    SourceData out;
    auto entry = store.find(id);
    if (!entry || !entry->enabled || !extra_authorized || !extra_authorized()) {
        std::erase_if(cache_, [&](const auto& value) { return value.first.first == id; });
        return out;
    }
    const auto settings_bytes = settings.dump();
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(settings_bytes.data()), settings_bytes.size(), digest);
    const std::string settings_fingerprint(reinterpret_cast<const char*>(digest), sizeof digest);
    for (const auto& source : entry->manifest.sources) {
        Authorization authorized = [&store, &extra_authorized, id, source_id = source.id, generation = entry->generation] {
            return extra_authorized() && store.source_authorized(id, source_id, generation);
        };
        const auto key = std::make_pair(id, source.id);
        if (!authorized()) { cache_.erase(key); continue; }
        auto& cached = cache_[key];
        if (cached.generation != entry->generation || cached.settings_fingerprint != settings_fingerprint) {
            cached = {}; cached.generation = entry->generation; cached.settings_fingerprint = settings_fingerprint;
        }
        const auto now = now_();
        if (now >= cached.next) {
            auto request = prepare_request(entry->manifest, source, settings);
            auto result = request && authorized() ? fetch_(*request.request, authorized) : failure("Source settings are incomplete.");
            if (!authorized()) { cache_.erase(key); continue; }
            if (result) {
                cached.data = std::move(result.data); cached.failures = 0;
                cached.next = now_() + std::chrono::seconds(entry->manifest.refresh_seconds);
            } else {
                // Never display stale values after an error as if they were live.
                cached.data.reset(); cached.failures = std::min(cached.failures + 1, 7u);
                const auto seconds = std::max(entry->manifest.refresh_seconds, std::min(900, 15 << cached.failures));
                cached.next = now_() + std::chrono::seconds(seconds);
            }
        }
        if (cached.data && authorized()) out.emplace(source.id, *cached.data);
    }
    // Bound memory after uninstall/update; no secrets/settings are cache keys.
    std::erase_if(cache_, [&](const auto& value) {
        auto current = store.find(value.first.first);
        return !current || !current->enabled || current->generation != value.second.generation;
    });
    // A revocation during a later source also invalidates values collected from
    // earlier sources in this call; never publish a mixed-generation snapshot.
    const auto current = store.find(id);
    if (!current || !current->enabled || current->generation != entry->generation || !extra_authorized()) out.clear();
    return out;
}
} // namespace fl::plugins
