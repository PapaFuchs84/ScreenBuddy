// © 2026 FuchsLabs
#include "psn_logic.h"

#include <algorithm>
#include <cctype>

namespace fl::psn {

using json = nlohmann::json;

static std::string jstr(const json& j, const char* k) {
    return j.is_object() && j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string();
}

bool valid_npsso(const std::string& s) {
    return s.size() == 64 && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isalnum(c) != 0; });
}

std::string clean_npsso(const std::string& input) {
    std::string t = input;
    auto pos = t.find("\"npsso\"");
    if (pos != std::string::npos) {  // ganze Antwort eingefügt
        auto q = t.find('"', t.find(':', pos) + 1);
        auto e = q == std::string::npos ? q : t.find('"', q + 1);
        if (q != std::string::npos && e != std::string::npos) t = t.substr(q + 1, e - q - 1);
    }
    std::string out;
    for (unsigned char c : t)
        if (!std::isspace(c) && c != '"' && c != '\'') out += (char)c;
    return out;
}

std::vector<std::string> parse_friend_ids(const json& data) {
    std::vector<std::string> out;
    if (!data.is_object() || !data.contains("friends") || !data["friends"].is_array()) return out;
    for (auto& f : data["friends"]) {
        std::string id = f.is_string() ? f.get<std::string>() : f.is_number_integer() ? std::to_string(f.get<long long>()) : "";
        if (!id.empty() && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) out.push_back(id);
        if (out.size() >= MAX_FRIENDS) break;
    }
    return out;
}

static Presence one_presence(const json& p) {
    Presence r;
    r.account_id = jstr(p, "accountId");
    const json& plat = p.contains("primaryPlatformInfo") ? p["primaryPlatformInfo"] : json();
    r.online = jstr(plat, "onlineStatus") == "online";
    r.platform = jstr(plat, "platform");
    if (r.online && p.contains("gameTitleInfoList") && p["gameTitleInfoList"].is_array() && !p["gameTitleInfoList"].empty())
        r.game = jstr(p["gameTitleInfoList"][0], "titleName");
    return r;
}

std::vector<Presence> parse_presences(const json& data) {
    std::vector<Presence> out;
    if (!data.is_object()) return out;
    if (data.contains("basicPresences") && data["basicPresences"].is_array())
        for (auto& p : data["basicPresences"])
            if (p.is_object()) out.push_back(one_presence(p));
    if (out.empty() && data.contains("basicPresence") && data["basicPresence"].is_object())
        out.push_back(one_presence(data["basicPresence"]));
    return out;
}

bool trusted_avatar_url(const std::string& url) {
    if (url.rfind("https://", 0) != 0) return false;
    std::string host = url.substr(8, url.find_first_of("/?#:@", 8) - 8);
    if (url.size() > 8 + host.size() && url[8 + host.size()] == '@') return false;  // keine Zugangsdaten in der Adresse
    std::transform(host.begin(), host.end(), host.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    for (const char* suffix : {".playstation.net", ".playstation.com"})
        if (host.size() > std::string(suffix).size() && host.compare(host.size() - std::string(suffix).size(), std::string::npos, suffix) == 0)
            return true;
    return false;
}

bool parse_profile(const json& data, Profile& out) {
    const json& p = data.is_object() && data.contains("profile") ? data["profile"] : data;
    std::string name = jstr(p, "onlineId");
    if (name.empty()) return false;
    out.online_id = name;
    out.avatar_url.clear();
    if (p.contains("avatarUrls") && p["avatarUrls"].is_array()) {
        // kleinste brauchbare Größe reicht (Anzeige 64 px); sonst die erste vertrauenswürdige
        static const char* order[] = {"m", "l", "s", "xl"};
        for (const char* size : order)
            for (auto& a : p["avatarUrls"])
                if (jstr(a, "size") == size && trusted_avatar_url(jstr(a, "avatarUrl")) && out.avatar_url.empty())
                    out.avatar_url = jstr(a, "avatarUrl");
        if (out.avatar_url.empty())
            for (auto& a : p["avatarUrls"])
                if (trusted_avatar_url(jstr(a, "avatarUrl"))) { out.avatar_url = jstr(a, "avatarUrl"); break; }
    }
    return true;
}

std::string code_from_location(const std::string& location) {
    auto pos = location.find("code=");
    if (pos == std::string::npos) return "";
    if (pos > 0 && location[pos - 1] != '?' && location[pos - 1] != '&') return "";
    std::string code = location.substr(pos + 5, location.find_first_of("&#", pos + 5) - pos - 5);
    return std::all_of(code.begin(), code.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'; }) ? code : "";
}

}  // namespace fl::psn
