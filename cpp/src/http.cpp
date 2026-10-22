// © 2026 FuchsLabs
#include "http.h"

#include <windows.h>
#include <winhttp.h>

#include <memory>
#include <mutex>

#include "util.h"

namespace fl::http {

namespace {

struct HDel {
    void operator()(HINTERNET h) const {
        if (h) WinHttpCloseHandle(h);
    }
};
using Handle = std::unique_ptr<void, HDel>;

HINTERNET open_session(DWORD access) {
    {
        HINTERNET h = WinHttpOpen(widen(UA).c_str(), access, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (h) {
            DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
            if (!WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof protocols)) {
                protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;  // ältere Windows-Versionen ohne TLS 1.3
                WinHttpSetOption(h, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof protocols);
            }
            DWORD decomp = WINHTTP_DECOMPRESSION_FLAG_GZIP;  // nur gzip: "deflate" mancher Server (z. B. Open-Meteo) bricht WinHTTP ab
            WinHttpSetOption(h, WINHTTP_OPTION_DECOMPRESSION, &decomp, sizeof decomp);
        }
        return h;
    }
}

HINTERNET session(const std::string& host) {
    static HINTERNET internet = open_session(WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY);
    static HINTERNET local = open_session(WINHTTP_ACCESS_TYPE_NO_PROXY);  // eigene App auf 127.0.0.1: nie über Proxy
    return host == "127.0.0.1" || host == "localhost" ? local : internet;
}

std::string header_value(HINTERNET req, DWORD info) {
    DWORD size = 0;
    WinHttpQueryHeaders(req, info, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !size) return "";
    std::wstring buf(size / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(req, info, WINHTTP_HEADER_NAME_BY_INDEX, buf.data(), &size, WINHTTP_NO_HEADER_INDEX))
        return "";
    buf.resize(size / sizeof(wchar_t));
    return narrow(buf);
}

}  // namespace

Url parse_url(const std::string& url) {
    Url u;
    std::wstring w = widen(url);
    URL_COMPONENTS c{};
    c.dwStructSize = sizeof c;
    c.dwSchemeLength = c.dwHostNameLength = c.dwUrlPathLength = c.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(w.c_str(), (DWORD)w.size(), 0, &c)) return u;
    u.scheme = lower_ascii(narrow(std::wstring(c.lpszScheme, c.dwSchemeLength)));
    u.host = narrow(std::wstring(c.lpszHostName, c.dwHostNameLength));
    u.path = narrow(std::wstring(c.lpszUrlPath, c.dwUrlPathLength)) +
             narrow(std::wstring(c.lpszExtraInfo, c.dwExtraInfoLength));
    if (u.path.empty()) u.path = "/";
    u.port = c.nPort;
    u.valid = (u.scheme == "http" || u.scheme == "https") && !u.host.empty();
    return u;
}

std::string url_encode(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
        else out += strprintf("%%%02X", c);
    }
    return out;
}

std::string query(const std::vector<std::pair<std::string, std::string>>& params) {
    std::string q;
    for (auto& [k, v] : params) q += (q.empty() ? "" : "&") + url_encode(k) + "=" + url_encode(v);
    return q;
}

static std::string remove_dots(const std::string& path) {
    // RFC 3986 5.2.4
    std::vector<std::string> out;
    auto parts = split(path, "/");
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& p = parts[i];
        if (p == ".") { if (i + 1 == parts.size()) out.push_back(""); continue; }
        if (p == "..") {
            if (out.size() > 1) out.pop_back();
            if (i + 1 == parts.size()) out.push_back("");
            continue;
        }
        out.push_back(p);
    }
    std::string r = join(out, "/");
    if (!path.empty() && path[0] == '/' && (r.empty() || r[0] != '/')) r = "/" + r;
    return r;
}

std::string url_join(const std::string& base, const std::string& ref) {
    if (ref.empty()) return base;
    auto colon = ref.find(':');
    if (colon != std::string::npos && ref.find('/') > colon) {  // eigenes Schema → absolut
        bool scheme_ok = true;
        for (size_t i = 0; i < colon; ++i)
            if (!isalnum((unsigned char)ref[i]) && ref[i] != '+' && ref[i] != '-' && ref[i] != '.') scheme_ok = false;
        if (scheme_ok && colon > 0) return ref;
    }
    auto se = base.find("://");
    if (se == std::string::npos) return ref;
    std::string scheme = base.substr(0, se);
    size_t auth_end = base.find_first_of("/?#", se + 3);
    std::string authority = base.substr(se + 3, auth_end == std::string::npos ? std::string::npos : auth_end - se - 3);
    std::string rest = auth_end == std::string::npos ? "" : base.substr(auth_end);
    std::string bpath = rest.substr(0, rest.find_first_of("?#"));
    std::string bquery = rest.find('?') != std::string::npos ? rest.substr(rest.find('?'), rest.find('#') - rest.find('?')) : "";
    if (ref.rfind("//", 0) == 0) return scheme + ":" + ref;
    if (ref[0] == '/') return scheme + "://" + authority + remove_dots(ref.substr(0, ref.find_first_of("?#"))) +
                              (ref.find_first_of("?#") != std::string::npos ? ref.substr(ref.find_first_of("?#")) : "");
    if (ref[0] == '?') return scheme + "://" + authority + bpath + ref;
    if (ref[0] == '#') return scheme + "://" + authority + bpath + bquery + ref;
    std::string dir = bpath.empty() ? "/" : bpath.substr(0, bpath.rfind('/') + 1);
    std::string rpath = ref.substr(0, ref.find_first_of("?#"));
    std::string tail = ref.find_first_of("?#") != std::string::npos ? ref.substr(ref.find_first_of("?#")) : "";
    return scheme + "://" + authority + remove_dots(dir + rpath) + tail;
}

Response fetch(const Request& r) {
    Response out;
    Url u = parse_url(r.url);
    if (!u.valid) {
        out.error = "ungültige Adresse";
        return out;
    }
    HINTERNET ses = session(u.host);
    if (!ses) {
        out.error = "WinHTTP nicht verfügbar";
        return out;
    }
    Handle con(WinHttpConnect(ses, widen(u.host).c_str(), (INTERNET_PORT)u.port, 0));
    if (!con) {
        out.error = strprintf("Verbindung fehlgeschlagen (%lu)", GetLastError());
        return out;
    }
    DWORD flags = u.scheme == "https" ? WINHTTP_FLAG_SECURE : 0;
    Handle req(WinHttpOpenRequest(con.get(), widen(r.method).c_str(), widen(u.path).c_str(), nullptr,
                                  WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!req) {
        out.error = strprintf("Anfrage fehlgeschlagen (%lu)", GetLastError());
        return out;
    }
    int t = r.timeout_ms;
    WinHttpSetTimeouts(req.get(), t, t, t, t);
    if (!r.follow_redirects) {
        DWORD pol = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(req.get(), WINHTTP_OPTION_REDIRECT_POLICY, &pol, sizeof pol);
    }
    std::wstring hdrs;
    for (auto& [k, v] : r.headers) hdrs += widen(k) + L": " + widen(v) + L"\r\n";
    BOOL ok = WinHttpSendRequest(req.get(), hdrs.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : hdrs.c_str(),
                                 hdrs.empty() ? 0 : (DWORD)-1L, (LPVOID)(r.body.empty() ? nullptr : r.body.data()),
                                 (DWORD)r.body.size(), (DWORD)r.body.size(), 0);
    if (!ok || !WinHttpReceiveResponse(req.get(), nullptr)) {
        DWORD e = GetLastError();
        out.error = e == ERROR_WINHTTP_TIMEOUT ? "Zeitüberschreitung" : strprintf("keine Verbindung (%lu)", e);
        return out;
    }
    DWORD status = 0, sz = sizeof status;
    WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &status, &sz, WINHTTP_NO_HEADER_INDEX);
    out.status = (int)status;
    out.location = header_value(req.get(), WINHTTP_QUERY_LOCATION);
    out.content_type = header_value(req.get(), WINHTTP_QUERY_CONTENT_TYPE);
    {
        DWORD len = 0;
        WinHttpQueryOption(req.get(), WINHTTP_OPTION_URL, nullptr, &len);
        if (len) {
            std::wstring buf(len / sizeof(wchar_t), L'\0');
            if (WinHttpQueryOption(req.get(), WINHTTP_OPTION_URL, buf.data(), &len)) {
                buf.resize(len / sizeof(wchar_t));
                while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
                out.final_url = narrow(buf);
            }
        }
        if (out.final_url.empty()) out.final_url = r.url;
    }
    while (true) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(req.get(), &avail)) {
            out.error = "Übertragung abgebrochen";
            break;
        }
        if (!avail) break;
        size_t old = out.body.size();
        out.body.resize(old + avail);
        DWORD read = 0;
        if (!WinHttpReadData(req.get(), out.body.data() + old, avail, &read)) {
            out.body.resize(old);
            out.error = "Übertragung abgebrochen";
            break;
        }
        out.body.resize(old + read);
        if (out.body.size() > r.max_bytes) {
            out.body.resize(r.max_bytes);
            out.truncated = true;
            break;
        }
    }
    return out;
}

Response get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers, int timeout_ms) {
    Request r;
    r.url = url;
    r.headers = headers;
    r.timeout_ms = timeout_ms;
    return fetch(r);
}

}  // namespace fl::http
