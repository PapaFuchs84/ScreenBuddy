// © 2026 FuchsLabs
// Internetabrufe über WinHTTP (TLS von Windows selbst, keine eigene Zertifikatsliste nötig).
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace fl::http {

struct Request {
    std::string method = "GET";
    std::string url;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
    int timeout_ms = 30000;
    size_t max_bytes = 10'000'000;  // Schutz vor riesigen Antworten
    bool follow_redirects = true;
};

struct Response {
    int status = 0;            // 0 = keine Verbindung (error enthält den Grund)
    std::string body;
    std::string location;      // bei Weiterleitungen (wenn follow_redirects = false)
    std::string final_url;
    std::string content_type;
    bool truncated = false;    // Antwort größer als max_bytes (body ist abgeschnitten)
    std::string error;
};

Response fetch(const Request& r);
Response get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers = {},
             int timeout_ms = 30000);

std::string url_encode(const std::string& s);
std::string query(const std::vector<std::pair<std::string, std::string>>& params);  // a=1&b=2

struct Url {
    std::string scheme, host, path;  // path inklusive Abfrage
    int port = 0;
    bool valid = false;
};
Url parse_url(const std::string& url);
std::string url_join(const std::string& base, const std::string& ref);  // wie urllib.parse.urljoin (vereinfacht)

}  // namespace fl::http
