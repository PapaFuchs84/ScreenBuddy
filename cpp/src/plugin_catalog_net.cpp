// © 2026 FuchsLabs
#include "plugin_catalog_net.h"
#include "plugin_transport.h"
#include <algorithm>
#include <string_view>

namespace fl::plugins::catalog {
namespace {
bool safe_relative(const std::string& path) {
    if (path.empty() || path.size() > 200 || path.front() == '/' || path.find("..") != path.npos || path.find("//") != path.npos) return false;
    return std::all_of(path.begin(), path.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '/' || c == '-';
    });
}
} // namespace

Fetcher public_https_fetcher() {
    return [](const std::string& relative, size_t limit) -> std::optional<std::string> {
        if (!safe_relative(relative)) return std::nullopt;
        const std::string_view base = base_url;
        constexpr std::string_view scheme = "https://";
        if (!base.starts_with(scheme) || !base.ends_with('/')) return std::nullopt;
        const auto host_end = base.find('/', scheme.size());
        Request request;
        request.host = std::string(base.substr(scheme.size(), host_end - scheme.size()));
        request.url = std::string(base) + relative;
        request.headers = {{"Accept", "*/*"}};
        auto result = fetch_public_bytes(request, [] { return true; }, limit);
        if (!result) return std::nullopt;
        return std::move(result.data);
    };
}
} // namespace fl::plugins::catalog
