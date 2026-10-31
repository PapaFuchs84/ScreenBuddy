// © 2026 FuchsLabs
#include "friends.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <regex>
#include <set>

#include "discord.h"
#include "http.h"
#include "psn_logic.h"
#include "secure.h"
#include "render.h"
#include "util.h"

namespace fl::friends {

using json = nlohmann::json;

static const char* STEAM_API = "https://api.steampowered.com";
static const char* XBOX_API = "https://xbl.io/api/v2";
static constexpr int STEAM_EVERY = 60, XBOX_EVERY = 180, PSN_EVERY = 120;  // Sekunden zwischen Abrufen (schont die Limits)
static constexpr size_t MAX_AVATAR_BYTES = 2'000'000;
static const std::vector<std::string> PLATFORMS = {"discord", "steam", "xbox", "psn"};

static std::string jstr(const json& j, const char* k) {
    if (!j.is_object() || !j.contains(k) || j[k].is_null()) return "";
    return j[k].is_string() ? j[k].get<std::string>() : j[k].dump();
}

// ---------- Steam ----------
static json steam_get(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params) {
    auto r = http::get(std::string(STEAM_API) + "/" + path + "?" + http::query(params), {{"User-Agent", UA}}, 30000);
    if (r.status == 0) throw FriendsError("Steam ist nicht erreichbar");
    if (r.status == 401 || r.status == 403)
        throw FriendsError("Steam: Schlüssel ungültig oder die Freundesliste im Profil ist nicht öffentlich");
    if (r.status == 429) throw FriendsError("Steam: zu viele Abrufe, ich versuche es später wieder");
    if (r.status != 200) throw FriendsError(strprintf("Steam antwortet mit Fehler %d", r.status));
    try {
        return json::parse(r.body);
    } catch (...) {
        throw FriendsError("Steam ist nicht erreichbar");
    }
}

static bool is_steam64(const std::string& s) {
    return s.size() == 17 && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}

static std::string resolve_steam_id(const std::string& key, const std::string& in) {
    // 17-stellige Steam-ID, Profil-Link (…/profiles/ID oder …/id/Name/) oder Profilname → Steam-ID
    std::string text = trim(in);
    while (!text.empty() && text.back() == '/') text.pop_back();
    if (is_steam64(text)) return text;
    std::string url = text.find("://") != std::string::npos ? text : "https://x/" + text;
    http::Url u = http::parse_url(url);
    std::string p = u.path.substr(0, u.path.find_first_of("?#"));
    while (!p.empty() && p.front() == '/') p.erase(0, 1);
    while (!p.empty() && p.back() == '/') p.pop_back();
    auto path = split(p, "/");
    if (path.size() >= 2 && path[0] == "profiles" && is_steam64(path[1])) return path[1];
    std::string vanity = path.size() >= 2 && path[0] == "id" ? path[1] : path.back();
    json data = steam_get("ISteamUser/ResolveVanityURL/v1/", {{"key", key}, {"vanityurl", vanity}});
    json resp = data.is_object() && data.contains("response") ? data["response"] : json::object();
    if (!resp.is_object() || !resp.contains("success") || resp["success"] != 1)
        throw FriendsError("Steam: Profil nicht gefunden – bitte den Link aus der Adresszeile deines Profils einfügen");
    return jstr(resp, "steamid");
}

std::vector<Account> parse_steam_players(const json& data) {
    static const std::map<int, std::string> STATE = {{0, "offline"}, {1, "online"}, {2, "dnd"}, {3, "idle"},
                                                     {4, "idle"},    {5, "online"}, {6, "online"}};
    std::vector<Account> out;
    if (!data.is_object() || !data.contains("response") || !data["response"].is_object() ||
        !data["response"].contains("players") || !data["response"]["players"].is_array())
        return out;
    for (auto& p : data["response"]["players"]) {
        Account a;
        a.platform = "steam";
        a.uid = jstr(p, "steamid");
        a.name = render::clean_text(jstr(p, "personaname"));
        if (a.name.empty()) a.name = "?";
        int st = p.contains("personastate") && p["personastate"].is_number_integer() ? p["personastate"].get<int>() : 0;
        a.status = STATE.count(st) ? STATE.at(st) : "offline";
        std::string game = render::clean_text(jstr(p, "gameextrainfo"));
        a.activity = game.empty() ? "" : "Spielt " + game;
        a.avatar_url = jstr(p, "avatarmedium");
        out.push_back(std::move(a));
    }
    return out;
}

std::vector<Account> fetch_steam(const std::string& key, const std::string& steam_id) {
    std::string sid = resolve_steam_id(key, steam_id);
    json data = steam_get("ISteamUser/GetFriendList/v1/", {{"key", key}, {"steamid", sid}, {"relationship", "friend"}});
    std::vector<std::string> ids;
    if (data.is_object() && data.contains("friendslist") && data["friendslist"].contains("friends") &&
        data["friendslist"]["friends"].is_array())
        for (auto& f : data["friendslist"]["friends"]) ids.push_back(jstr(f, "steamid"));
    std::vector<Account> out;
    for (size_t i = 0; i < ids.size(); i += 100) {  // höchstens 100 pro Abruf
        std::vector<std::string> part(ids.begin() + i, ids.begin() + std::min(ids.size(), i + 100));
        auto players = parse_steam_players(steam_get("ISteamUser/GetPlayerSummaries/v2/", {{"key", key}, {"steamids", join(part, ",")}}));
        out.insert(out.end(), players.begin(), players.end());
    }
    return out;
}

static std::string steam_self_name(const std::string& key, const std::string& steam_id) {
    // Name des eigenen Steam-Kontos (für die Statusanzeige); Fehler sind hier egal
    try {
        std::string sid = resolve_steam_id(key, steam_id);
        auto players = parse_steam_players(steam_get("ISteamUser/GetPlayerSummaries/v2/", {{"key", key}, {"steamids", sid}}));
        return players.empty() || players[0].name == "?" ? "" : players[0].name;
    } catch (const FriendsError&) {
        return "";
    }
}

// ---------- Xbox (OpenXBL) ----------
std::vector<Account> parse_xbox_people(const json& data) {
    std::vector<Account> out;
    if (!data.is_object()) return out;
    const json& body = data.contains("content") && data["content"].is_object() ? data["content"] : data;  // v2 verpackt in "content"
    if (!body.contains("people") || !body["people"].is_array()) return out;
    static const std::regex playing("^Playing\\s+");
    for (auto& p : body["people"]) {
        bool online = lower_ascii(jstr(p, "presenceState")) == "online";
        std::string text = render::clean_text(jstr(p, "presenceText"));
        std::string activity;
        std::string lt = lower_ascii(text);
        if (online && !text.empty() && lt != "online" && lt != "home" && lt != "startseite") {
            activity = std::regex_replace(text, playing, "Spielt ", std::regex_constants::format_first_only);
            if (!starts_with_any(activity, {"Spielt", "Streamt"})) activity = "Spielt " + activity;
        }
        Account a;
        a.platform = "xbox";
        a.uid = jstr(p, "xuid");
        if (a.uid.empty()) a.uid = jstr(p, "gamertag");
        std::string name = jstr(p, "gamertag");
        a.handle = name;
        if (name.empty()) name = jstr(p, "displayName");
        a.name = render::clean_text(name);
        if (a.name.empty()) a.name = "?";
        a.status = online ? "online" : "offline";
        a.activity = activity;
        a.avatar_url = jstr(p, "displayPicRaw");
        out.push_back(std::move(a));
    }
    return out;
}

static http::Response xbox_get(const std::string& key, const std::string& path) {
    return http::get(std::string(XBOX_API) + path,
                     {{"User-Agent", UA}, {"X-Authorization", key}, {"Accept", "application/json"}}, 30000);
}

std::vector<Account> fetch_xbox(const std::string& key) {
    auto r = xbox_get(key, "/friends");
    if (r.status == 0) throw FriendsError("OpenXBL ist nicht erreichbar");
    if (r.status == 401 || r.status == 403) throw FriendsError("Xbox: OpenXBL-Schlüssel ungültig");
    if (r.status == 429) throw FriendsError("Xbox: Abruflimit von OpenXBL erreicht, ich versuche es später wieder");
    if (r.status != 200) throw FriendsError(strprintf("OpenXBL antwortet mit Fehler %d", r.status));
    try {
        return parse_xbox_people(json::parse(r.body));
    } catch (const json::exception&) {
        throw FriendsError("OpenXBL ist nicht erreichbar");
    }
}

static std::string xbox_self_name(const std::string& key) {
    // Gamertag des eigenen Xbox-Kontos (für die Statusanzeige); Fehler sind hier egal
    auto r = xbox_get(key, "/account");
    if (r.status != 200) return "";
    try {
        json body = json::parse(r.body);
        const json& b = body.contains("content") && body["content"].is_object() && !body["content"].empty() ? body["content"] : body;
        if (!b.contains("profileUsers") || !b["profileUsers"].is_array() || b["profileUsers"].empty()) return "";
        const json& u = b["profileUsers"][0];
        if (!u.contains("settings") || !u["settings"].is_array()) return "";
        for (auto& x : u["settings"])
            if (jstr(x, "id") == "Gamertag") return render::clean_text(jstr(x, "value"));
    } catch (...) {
    }
    return "";
}

// ---------- PlayStation (inoffiziell) ----------
// Sony hat keine offizielle Schnittstelle. Der NPSSO-Wert (Cookie der angemeldeten Browser-Sitzung) wird wie in der
// PlayStation-App gegen ein kurzlebiges Zugriffstoken getauscht. Client-ID und -Geheimnis sind die öffentlich bekannten
// Konstanten der Android-App von Sony (kein Geheimnis dieser App). Der NPSSO-Wert verlässt das Gerät nur Richtung Sony.
static const char* PSN_CLIENT_ID = "09515159-7237-4370-9b40-3806e67c0891";
static const char* PSN_CLIENT_SECRET = "ucPjka5tntB2KqsP";
static const char* PSN_REDIRECT = "com.scee.psxandroid.scecompcall://redirect";
static const char* PSN_SCOPE = "psn:mobile.v2.core psn:clientapp";

namespace {
struct PsnSession {
    std::mutex mu;
    std::string npsso, access, refresh;
    std::time_t access_until = 0, refresh_until = 0;
    std::map<std::string, psn::Profile> profiles;  // Konto-ID → Name/Bild (ändert sich selten, schont die Abrufe)
};
PsnSession& psn_session() {
    static PsnSession s;
    return s;
}
}  // namespace

static void psn_http_error(const http::Response& r, const char* what) {
    if (r.status == 0) throw FriendsError("PlayStation ist nicht erreichbar");
    if (r.status == 429) throw FriendsError("PlayStation: zu viele Abrufe, ich versuche es später wieder");
    if (r.status == 401 || r.status == 403)
        throw FriendsError("PlayStation: Anmeldung abgelaufen oder ungültig – bitte einen neuen NPSSO-Code holen");
    throw FriendsError(strprintf("PlayStation antwortet mit Fehler %d (%s)", r.status, what));
}

static void psn_token_request(PsnSession& ss, const std::string& body) {
    http::Request req;
    req.method = "POST";
    req.url = std::string(psn::AUTH_BASE) + "/token";
    req.headers = {{"User-Agent", UA},
                   {"Authorization", "Basic " + secure::base64_encode(std::string(PSN_CLIENT_ID) + ":" + PSN_CLIENT_SECRET)},
                   {"Content-Type", "application/x-www-form-urlencoded"}};
    req.body = body;
    auto r = http::fetch(req);
    if (r.status != 200) {
        ss.access.clear();
        ss.refresh.clear();
        if (r.status == 400 || r.status == 401)
            throw FriendsError("PlayStation: Anmeldung abgelaufen oder ungültig – bitte einen neuen NPSSO-Code holen");
        psn_http_error(r, "Anmeldung");
    }
    try {
        json j = json::parse(r.body);
        ss.access = jstr(j, "access_token");
        ss.refresh = jstr(j, "refresh_token");
        std::time_t now = std::time(nullptr);
        ss.access_until = now + (j.value("expires_in", 3600) > 120 ? j.value("expires_in", 3600) - 60 : 0);
        ss.refresh_until = now + j.value("refresh_token_expires_in", 0);
    } catch (const json::exception&) {
        ss.access.clear();
    }
    if (ss.access.empty()) throw FriendsError("PlayStation ist nicht erreichbar");
}

static std::string psn_access_token(const std::string& npsso) {
    PsnSession& ss = psn_session();
    std::lock_guard lk(ss.mu);
    if (ss.npsso != npsso) {  // anderer Code: alles Gespeicherte verwerfen
        ss.npsso = npsso;
        ss.access.clear();
        ss.refresh.clear();
        ss.access_until = ss.refresh_until = 0;
        ss.profiles.clear();
    }
    std::time_t now = std::time(nullptr);
    if (!ss.access.empty() && ss.access_until > now) return ss.access;
    if (!ss.refresh.empty() && ss.refresh_until > now + 60) {
        try {
            psn_token_request(ss, http::query({{"refresh_token", ss.refresh}, {"grant_type", "refresh_token"},
                                               {"scope", PSN_SCOPE}, {"token_format", "jwt"}}));
            return ss.access;
        } catch (const FriendsError&) {
            // weiter mit neuer Anmeldung per NPSSO
        }
    }
    if (!psn::valid_npsso(npsso)) throw FriendsError("PlayStation: Der NPSSO-Code ist ungültig (er hat 64 Zeichen)");
    http::Request auth;
    auth.url = std::string(psn::AUTH_BASE) + "/authorize?" +
               http::query({{"access_type", "offline"}, {"client_id", PSN_CLIENT_ID}, {"redirect_uri", PSN_REDIRECT},
                            {"response_type", "code"}, {"scope", PSN_SCOPE}});
    auth.headers = {{"User-Agent", UA}, {"Cookie", "npsso=" + npsso}};
    auth.follow_redirects = false;
    auto r = http::fetch(auth);
    if (r.status == 0) throw FriendsError("PlayStation ist nicht erreichbar");
    std::string code = psn::code_from_location(r.location);
    if (code.empty()) {  // Diagnose ohne Geheimnisse: nur Status und ob/welcher Fehler in der Weiterleitung steht
        auto e = r.location.find("error");
        LOGW("friends", "PlayStation-Anmeldung: HTTP %d, Weiterleitung %s, %s", r.status, r.location.empty() ? "fehlt" : "ohne Code",
             e == std::string::npos ? "kein Fehlerhinweis" : r.location.substr(e, 60).c_str());
    }
    if (code.empty()) throw FriendsError("PlayStation: NPSSO-Code abgelaufen oder ungültig – bitte einen neuen holen");
    psn_token_request(ss, http::query({{"code", code}, {"redirect_uri", PSN_REDIRECT}, {"grant_type", "authorization_code"},
                                       {"token_format", "jwt"}}));
    return ss.access;
}

static json psn_get(const std::string& npsso, const std::string& path, const std::vector<std::pair<std::string, std::string>>& q = {}) {
    std::string url = std::string(psn::API_BASE) + path + (q.empty() ? "" : "?" + http::query(q));
    auto r = http::get(url, {{"User-Agent", UA}, {"Authorization", "Bearer " + psn_access_token(npsso)}, {"Accept", "application/json"},
                             {"Accept-Language", "en-US"}}, 30000);
    if (r.status == 401) {  // Token vorzeitig ungültig: einmal neu anmelden
        {
            PsnSession& ss = psn_session();
            std::lock_guard lk(ss.mu);
            ss.access.clear();
            ss.refresh.clear();
        }
        r = http::get(url, {{"User-Agent", UA}, {"Authorization", "Bearer " + psn_access_token(npsso)}, {"Accept", "application/json"},
                            {"Accept-Language", "en-US"}}, 30000);
    }
    if (r.status != 200) psn_http_error(r, "Abruf");
    try {
        return json::parse(r.body);
    } catch (const json::exception&) {
        throw FriendsError("PlayStation ist nicht erreichbar");
    }
}

static psn::Profile psn_profile(const std::string& npsso, const std::string& account_id) {
    PsnSession& ss = psn_session();
    {
        std::lock_guard lk(ss.mu);
        auto it = ss.profiles.find(account_id);
        if (it != ss.profiles.end()) return it->second;
    }
    psn::Profile p;
    if (!psn::parse_profile(psn_get(npsso, "/userProfile/v1/internal/users/" + account_id + "/profiles"), p)) return {};
    std::lock_guard lk(ss.mu);
    ss.profiles[account_id] = p;
    return p;
}

std::vector<Account> fetch_psn(const std::string& npsso_in) {
    const std::string npsso = psn::clean_npsso(npsso_in);
    std::vector<std::string> ids;
    for (int offset = 0; ids.size() < psn::MAX_FRIENDS; offset += 100) {
        auto page = psn::parse_friend_ids(psn_get(npsso, "/userProfile/v1/internal/users/me/friends",
                                                   {{"limit", "100"}, {"offset", std::to_string(offset)}}));
        ids.insert(ids.end(), page.begin(), page.end());
        if (page.size() < 100) break;
    }
    if (ids.size() > psn::MAX_FRIENDS) ids.resize(psn::MAX_FRIENDS);
    std::map<std::string, psn::Presence> presence;
    for (size_t i = 0; i < ids.size(); i += psn::PRESENCE_CHUNK) {
        std::vector<std::string> part(ids.begin() + i, ids.begin() + std::min(ids.size(), i + psn::PRESENCE_CHUNK));
        for (auto& p : psn::parse_presences(psn_get(npsso, "/userProfile/v1/internal/users/basicPresences",
                                                     {{"type", "primary"}, {"accountIds", join(part, ",")}})))
            presence[p.account_id] = p;
    }
    std::vector<Account> out;
    size_t failed = 0;
    std::string last_error;
    for (auto& id : ids) {
        psn::Profile prof;
        try {
            prof = psn_profile(npsso, id);
        } catch (const FriendsError& e) {  // einzelnes Profil nicht lesbar: überspringen, beim nächsten Abruf erneut versuchen
            ++failed;
            last_error = e.what();
        }
        if (prof.online_id.empty()) {
            if (failed == ids.size()) throw FriendsError(last_error);
            continue;
        }
        Account a;
        a.platform = "psn";
        a.uid = id;
        a.handle = prof.online_id;
        a.name = render::clean_text(prof.online_id);
        if (a.name.empty()) continue;
        a.avatar_url = prof.avatar_url;
        if (auto it = presence.find(id); it != presence.end() && it->second.online) {
            a.status = "online";
            if (!it->second.game.empty()) a.activity = "Spielt " + render::clean_text(it->second.game);
        }
        out.push_back(std::move(a));
    }
    return out;
}

static std::string psn_self_name(const std::string& npsso_in) {
    // Online-ID des eigenen Kontos (für die Statusanzeige); Fehler sind hier egal
    try {
        return render::clean_text(psn_profile(psn::clean_npsso(npsso_in), "me").online_id);
    } catch (...) {
        return "";
    }
}

// ---------- Zusammenführen ----------
std::string norm(const std::string& name) {
    // Akzente und Sonderformen vereinheitlichen, dann nur Buchstaben/Ziffern
    std::wstring w = widen(casefold(name));
    std::string out;
    if (!w.empty()) {
        int n = NormalizeString(NormalizationKD, w.c_str(), (int)w.size(), nullptr, 0);
        std::wstring d(std::max(0, n), L'\0');
        n = NormalizeString(NormalizationKD, w.c_str(), (int)w.size(), d.data(), (int)d.size());
        if (n > 0) d.resize(n);
        else d = w;
        for (wchar_t c : d)
            if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'z')) out += (char)c;
    }
    return out;
}

static std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> parse_links(
    const std::vector<std::string>& links) {
    // "Name|discord:123|steam:765…|xbox:Gamertag" → [(Name, [(plattform, id), …])]
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> out;
    for (auto& entry : links) {
        auto parts = split(entry, "|");
        for (auto& p : parts) p = trim(p);
        std::string name = parts[0];
        std::vector<std::pair<std::string, std::string>> pairs;
        for (size_t i = 1; i < parts.size(); ++i) {
            auto c = parts[i].find(':');
            if (c == std::string::npos) continue;
            pairs.push_back({lower_ascii(parts[i].substr(0, c)), parts[i].substr(c + 1)});
        }
        if (!name.empty() && !pairs.empty()) out.push_back({name, pairs});
    }
    return out;
}

static int status_order(const std::string& s) {
    auto it = render::STATUS_ORDER.find(s);
    return it == render::STATUS_ORDER.end() ? 9 : it->second;
}

std::vector<Friend> merge_accounts(const std::vector<Account>& accounts, const std::vector<std::string>& links, bool auto_merge) {
    // Eine Zeile je Person. Reihenfolge der Regeln: manuelle Verknüpfungen, dann gleicher Name.
    std::vector<std::pair<std::string, std::vector<size_t>>> groups;
    std::set<size_t> used;
    std::map<std::pair<std::string, std::string>, size_t> index;
    for (size_t i = 0; i < accounts.size(); ++i) index[{accounts[i].platform, casefold(accounts[i].uid)}] = i;
    for (size_t i = 0; i < accounts.size(); ++i) index[{accounts[i].platform, casefold(accounts[i].name)}] = i;
    for (auto& [name, members] : parse_links(links)) {
        std::vector<size_t> found;
        for (auto& [plat, ident] : members) {
            auto it = index.find({plat, casefold(ident)});
            if (it == index.end() || used.count(it->second)) continue;
            bool same_plat = false;
            for (size_t f : found) same_plat |= accounts[f].platform == accounts[it->second].platform;
            if (same_plat) continue;
            found.push_back(it->second);
            used.insert(it->second);
        }
        if (!found.empty()) groups.push_back({name, found});
    }
    if (auto_merge) {
        std::vector<std::pair<std::string, std::vector<size_t>>> by_name;  // Reihenfolge wie dict in Python
        for (size_t i = 0; i < accounts.size(); ++i) {
            if (used.count(i)) continue;
            std::string k = norm(accounts[i].name);
            auto it = std::find_if(by_name.begin(), by_name.end(), [&](auto& x) { return x.first == k; });
            if (it == by_name.end()) by_name.push_back({k, {i}});
            else it->second.push_back(i);
        }
        for (auto& [key, accs] : by_name) {
            // nie zwei Konten derselben Plattform zusammenfassen
            std::set<std::string> seen;
            std::vector<size_t> merged, rest;
            for (size_t a : accs) {
                (seen.count(accounts[a].platform) ? rest : merged).push_back(a);
                seen.insert(accounts[a].platform);
            }
            if (!key.empty() && merged.size() > 1) {
                groups.push_back({accounts[merged[0]].name, merged});
                used.insert(merged.begin(), merged.end());
                for (size_t a : rest) {
                    groups.push_back({accounts[a].name, {a}});
                    used.insert(a);
                }
            }
        }
    }
    for (size_t i = 0; i < accounts.size(); ++i)
        if (!used.count(i)) groups.push_back({accounts[i].name, {i}});
    std::vector<Friend> out;
    for (auto& [name, accs] : groups) {
        // Online hat Vorrang; die Aktivität stammt vom Konto, das gerade spielt
        size_t top = *std::min_element(accs.begin(), accs.end(), [&](size_t x, size_t y) {
            auto& a = accounts[x];
            auto& b = accounts[y];
            auto ka = std::make_tuple(status_order(a.status), a.activity.empty(), !a.avatar);
            auto kb = std::make_tuple(status_order(b.status), b.activity.empty(), !b.avatar);
            return ka < kb;
        });
        const Account* playing = nullptr;
        for (size_t a : accs)
            if (starts_with_any(accounts[a].activity, {"Spielt", "Streamt"}) && accounts[a].status != "offline") {
                playing = &accounts[a];
                break;
            }
        Friend f;
        const Account& first = accounts[accs[0]];
        f.user_id = std::hash<std::string>{}(casefold(name) + "|" + first.platform + "|" + first.uid) & 0xFFFFFFFF;
        f.name = name;
        f.status = accounts[top].status;
        f.activity = (playing ? *playing : accounts[top]).activity;
        f.mobile = accounts[top].mobile;
        if (accounts[top].avatar) f.avatar = accounts[top].avatar;
        else
            for (size_t a : accs)
                if (accounts[a].avatar) { f.avatar = accounts[a].avatar; break; }
        std::set<std::string> plats, online;
        for (size_t a : accs) {
            auto& acc = accounts[a];
            plats.insert(acc.platform);
            if (acc.status != "offline") online.insert(acc.platform);
            std::string id = acc.platform == "xbox" || acc.platform == "psn" ? (acc.handle.empty() ? acc.name : acc.handle) : acc.uid;
            if (!id.empty()) f.ids.emplace(acc.platform, id);
        }
        for (auto& p : PLATFORMS) {
            if (plats.count(p)) f.platforms.push_back(p);
            if (online.count(p)) f.online_on.push_back(p);
        }
        out.push_back(std::move(f));
    }
    return out;
}

// difflib.SequenceMatcher(None, a, b).ratio() – für kurze Namen ohne „autojunk“
double similarity(const std::string& a, const std::string& b) {
    if (a.empty() && b.empty()) return 1.0;
    std::map<char, std::vector<int>> b2j;
    for (int j = 0; j < (int)b.size(); ++j) b2j[b[j]].push_back(j);
    auto longest = [&](int alo, int ahi, int blo, int bhi) {
        int besti = alo, bestj = blo, bestsize = 0;
        std::map<int, int> j2len;
        for (int i = alo; i < ahi; ++i) {
            std::map<int, int> newj2len;
            auto it = b2j.find(a[i]);
            if (it != b2j.end())
                for (int j : it->second) {
                    if (j < blo) continue;
                    if (j >= bhi) break;
                    int k = (j2len.count(j - 1) ? j2len[j - 1] : 0) + 1;
                    newj2len[j] = k;
                    if (k > bestsize) { besti = i - k + 1; bestj = j - k + 1; bestsize = k; }
                }
            j2len = std::move(newj2len);
        }
        return std::make_tuple(besti, bestj, bestsize);
    };
    int matches = 0;
    std::vector<std::array<int, 4>> queue = {{0, (int)a.size(), 0, (int)b.size()}};
    while (!queue.empty()) {
        auto [alo, ahi, blo, bhi] = queue.back();
        queue.pop_back();
        auto [i, j, k] = longest(alo, ahi, blo, bhi);
        if (k) {
            matches += k;
            if (alo < i && blo < j) queue.push_back({alo, i, blo, j});
            if (i + k < ahi && j + k < bhi) queue.push_back({i + k, ahi, j + k, bhi});
        }
    }
    return 2.0 * matches / (a.size() + b.size());
}

ojson suggest_links(const ojson& accounts, const std::vector<std::string>& links, bool auto_merge, size_t limit) {
    // Vorschläge für Konten, die vermutlich dieselbe Person sind (ähnlicher, aber nicht gleicher Name)
    std::set<std::pair<std::string, std::string>> linked;
    for (auto& [_, members] : parse_links(links)) {
        std::vector<std::string> keys;
        for (auto& [p, i] : members) keys.push_back(p + ":" + casefold(i));
        for (auto& x : keys)
            for (auto& y : keys)
                if (x != y) linked.insert({std::min(x, y), std::max(x, y)});
    }
    auto s = [](const ojson& j, const char* k) { return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); };
    std::vector<std::pair<double, ojson>> out;
    for (size_t n = 0; n < accounts.size(); ++n) {
        for (size_t m = n + 1; m < accounts.size(); ++m) {
            const ojson& a = accounts[n];
            const ojson& b = accounts[m];
            if (s(a, "platform") == s(b, "platform")) continue;
            std::string ka = norm(s(a, "name")), kb = norm(s(b, "name"));
            if (ka.empty() || kb.empty()) continue;
            std::string x = s(a, "platform") + ":" + casefold(s(a, "id")), y = s(b, "platform") + ":" + casefold(s(b, "id"));
            if (linked.count({std::min(x, y), std::max(x, y)})) continue;
            double score;
            if (ka == kb) {
                if (auto_merge) continue;  // wird ohnehin automatisch zusammengefasst
                score = 1.0;
            } else {
                const std::string& sh = ka.size() <= kb.size() ? ka : kb;
                const std::string& lg = ka.size() <= kb.size() ? kb : ka;
                score = similarity(ka, kb);
                if (sh.size() >= 4 && lg.find(sh) != std::string::npos) score = std::max(score, 0.85);
                if (score < 0.8) continue;
            }
            out.push_back({score, {{"a", a}, {"b", b}, {"score", std::round(score * 100) / 100}}});
        }
    }
    std::stable_sort(out.begin(), out.end(), [](auto& x, auto& y) { return x.first > y.first; });
    ojson res = ojson::array();
    for (size_t i = 0; i < out.size() && i < limit; ++i) res.push_back(out[i].second);
    return res;
}

// ---------- Dienst ----------
FriendsHub::FriendsHub(const ojson& cfg, discord::DiscordFeed* discord, std::function<void()> on_change)
    : cfg_(cfg), discord_(discord), on_change_(std::move(on_change)) {}

FriendsHub::~FriendsHub() { stop(); }

void FriendsHub::stop() {
    stop_ = true;
    for (auto& t : threads_)
        if (t.joinable()) t.join();
    threads_.clear();
}

bool FriendsHub::sleep_for(double seconds) {
    double end = mono() + seconds;
    while (mono() < end) {
        if (stop_) return false;
        Sleep(100);
    }
    return !stop_;
}

static std::string cstr(const ojson& c, const char* k) {
    return c.contains(k) && c[k].is_string() ? c[k].get<std::string>() : std::string();
}
static bool cbool(const ojson& c, const char* k, bool def) {
    return c.contains(k) && c[k].is_boolean() ? c[k].get<bool>() : def;
}

void FriendsHub::start() {
    bool any = false;
    if (!cstr(cfg_, "steam_api_key").empty() && !cstr(cfg_, "steam_id").empty()) {
        threads_.emplace_back([this] { loop("steam", STEAM_EVERY); });
        any = true;
    }
    if (!cstr(cfg_, "xbox_api_key").empty()) {
        threads_.emplace_back([this] { loop("xbox", XBOX_EVERY); });
        any = true;
    }
    if (!cstr(cfg_, "psn_npsso").empty()) {
        threads_.emplace_back([this] { loop("psn", PSN_EVERY); });
        any = true;
    }
    if (!any) on_change_();
}

std::string FriendsHub::status() {
    std::lock_guard lk(mu_);
    std::vector<std::string> v;
    for (auto& p : {"steam", "xbox", "psn"})
        if (auto it = errors_.find(p); it != errors_.end()) v.push_back(it->second);
    return join(v, " · ");
}

ojson FriendsHub::info(const std::string& platform) {
    std::lock_guard lk(mu_);
    auto it = info_.find(platform);
    return it == info_.end() ? ojson() : it->second;
}

void FriendsHub::loop(const std::string& platform, int every) {
    while (!stop_) {
        try {
            std::vector<Account> accs = platform == "steam" ? fetch_steam(cstr(cfg_, "steam_api_key"), cstr(cfg_, "steam_id"))
                                        : platform == "psn" ? fetch_psn(cstr(cfg_, "psn_npsso"))
                                                            : fetch_xbox(cstr(cfg_, "xbox_api_key"));
            load_avatars(accs);
            bool need_name;
            {
                std::lock_guard lk(mu_);
                need_name = !info_.count(platform) || cstr(info_[platform], "name").empty();
            }
            std::string who;
            if (need_name)  // Kontoname nur einmal holen (schont die Abruflimits)
                who = platform == "steam" ? steam_self_name(cstr(cfg_, "steam_api_key"), cstr(cfg_, "steam_id"))
                                          : platform == "psn" ? psn_self_name(cstr(cfg_, "psn_npsso"))
                                                              : xbox_self_name(cstr(cfg_, "xbox_api_key"));
            int online = (int)std::count_if(accs.begin(), accs.end(), [](auto& a) { return a.status != "offline"; });
            std::lock_guard lk(mu_);
            ojson& inf = info_[platform];
            if (!inf.is_object()) inf = ojson::object();
            if (need_name) inf["name"] = who;
            inf["ok"] = true;
            inf["error"] = "";
            inf["count"] = accs.size();
            inf["online"] = online;
            inf["at"] = (double)std::time(nullptr);
            accounts_[platform] = std::move(accs);
            errors_.erase(platform);
        } catch (const FriendsError& e) {
            std::lock_guard lk(mu_);
            errors_[platform] = e.what();  // alte Daten bleiben stehen
            ojson& inf = info_[platform];
            if (!inf.is_object()) inf = ojson::object();
            inf["ok"] = false;
            inf["error"] = e.what();
            LOGW("friends", "%s", e.what());
        } catch (const std::exception& e) {
            std::string p = platform;
            p[0] = (char)toupper((unsigned char)p[0]);
            std::lock_guard lk(mu_);
            errors_[platform] = p + ": unerwarteter Fehler";
            ojson& inf = info_[platform];
            if (!inf.is_object()) inf = ojson::object();
            inf["ok"] = false;
            inf["error"] = errors_[platform];
            LOGE("friends", "Freunde (%s): %s", platform.c_str(), e.what());
        }
        on_change_();
        if (!sleep_for(every)) return;
    }
}

void FriendsHub::load_avatars(std::vector<Account>& accs) {
    std::vector<std::future<void>> jobs;
    std::mutex local;
    for (auto& a : accs) {
        if (a.avatar_url.empty()) continue;
        {
            std::lock_guard lk(mu_);
            if (avatars_.count(a.avatar_url) || avatar_failed_.count(a.avatar_url)) {
                a.avatar = avatars_.count(a.avatar_url) ? avatars_[a.avatar_url] : nullptr;
                continue;
            }
        }
        jobs.push_back(std::async(std::launch::async, [this, &a] {
            ImagePtr img;
            http::Request r;
            r.url = a.avatar_url;
            r.headers = {{"User-Agent", UA}};
            r.max_bytes = MAX_AVATAR_BYTES;  // fremde Bilder: Größe begrenzen
            auto resp = http::fetch(r);
            if (resp.status == 200 && !resp.truncated)
                if (auto im = decode_image(resp.body)) img = std::make_shared<const Image>(thumbnail(*im, 64, 64));
            std::lock_guard lk(mu_);
            if (img) avatars_[a.avatar_url] = img;
            else avatar_failed_[a.avatar_url] = true;
            a.avatar = img;
        }));
        if (jobs.size() >= 8) {  // nicht zu viele gleichzeitig
            for (auto& j : jobs) j.get();
            jobs.clear();
        }
    }
    for (auto& j : jobs) j.get();
}

std::vector<Friend> FriendsHub::people() {
    std::vector<Account> accounts;
    if (cbool(cfg_, "include_discord", true) && discord_)
        for (auto& f : discord_->friends()) {
            Account a;
            a.platform = "discord";
            a.uid = std::to_string(f.user_id);
            a.name = f.name;
            a.status = f.status;
            a.activity = f.activity;
            a.mobile = f.mobile;
            a.avatar = f.avatar;
            accounts.push_back(std::move(a));
        }
    {
        std::lock_guard lk(mu_);
        for (auto& p : {"steam", "xbox", "psn"}) accounts.insert(accounts.end(), accounts_[p].begin(), accounts_[p].end());
    }
    std::vector<std::string> links;
    if (cfg_.contains("links") && cfg_["links"].is_array())
        for (auto& l : cfg_["links"])
            if (l.is_string()) links.push_back(l.get<std::string>());
    auto people = merge_accounts(accounts, links, cbool(cfg_, "auto_merge", true));
    if (!cbool(cfg_, "show_offline", false))
        people.erase(std::remove_if(people.begin(), people.end(), [](auto& p) { return p.status == "offline"; }), people.end());
    return people;
}

ojson FriendsHub::all_accounts() {
    // Alle bekannten Konten (für den Verknüpfungs-Editor der Einstellungsseite)
    std::vector<ojson> out;
    {
        std::lock_guard lk(mu_);
        for (auto& p : {"steam", "xbox", "psn"})
            for (auto& a : accounts_[p]) out.push_back({{"platform", a.platform}, {"id", a.uid}, {"name", a.name}});
    }
    if (discord_)
        for (auto& m : discord_->member_list()) out.push_back({{"platform", "discord"}, {"id", m["id"]}, {"name", m["name"]}});
    std::stable_sort(out.begin(), out.end(), [](const ojson& a, const ojson& b) {
        auto ka = casefold(a["name"].get<std::string>()), kb = casefold(b["name"].get<std::string>());
        if (ka != kb) return ka < kb;
        return a["platform"].get<std::string>() < b["platform"].get<std::string>();
    });
    ojson res = ojson::array();
    for (auto& x : out) res.push_back(x);
    return res;
}

}  // namespace fl::friends
