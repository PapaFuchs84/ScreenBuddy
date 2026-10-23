// © 2026 FuchsLabs
#include "discord.h"

#include <windows.h>

#include <algorithm>
#include <condition_variable>
#include <deque>

#include <dpp/dpp.h>

#include "http.h"
#include "render.h"
#include "util.h"

namespace fl::discord {

std::string duration_text(long long start_s) {
    if (!start_s) return "";
    long long mins = (long long)(std::time(nullptr) - start_s) / 60;
    if (mins < 1) return "";
    if (mins >= 60) return strprintf(" · %lld h %02lld min", mins / 60, mins % 60);
    return strprintf(" · %lld min", mins);
}

namespace {

std::string status_of(dpp::presence_status s) {
    switch (s) {
        case dpp::ps_online: return "online";
        case dpp::ps_idle: return "idle";
        case dpp::ps_dnd: return "dnd";
        default: return "offline";
    }
}

struct UserInfo {
    std::string username, global_name, avatar_url;
    bool bot = false;
};
struct MemberInfo {
    std::string nick, avatar_url;  // Server-Avatar (falls gesetzt)
};
struct PresenceInfo {
    std::string status = "offline";
    bool mobile = false;
    std::vector<DiscordFeed::Activity> activities;
};
struct GuildInfo {
    std::string name;
    std::map<uint64_t, MemberInfo> members;
    std::map<uint64_t, PresenceInfo> presences;
};

}  // namespace

struct DiscordFeed::Impl {
    ojson cfg;
    std::function<void()> on_change;
    std::set<uint64_t> friend_ids, ignore_ids, guild_ids;
    std::string token;

    std::mutex mu;
    std::string status = "Verbinde mit Discord …";
    std::string bot_name;
    uint64_t bot_id = 0;
    bool ready = false;
    bool fatal = false;
    std::map<uint64_t, GuildInfo> guilds;
    std::map<uint64_t, UserInfo> users;

    std::unique_ptr<dpp::cluster> bot;
    std::thread supervisor;
    std::atomic<bool> stop{false};
    std::condition_variable cv;
    bool restart_wanted = false;

    // Avatare: werden im Hintergrund geladen, Anzeige nutzt bis dahin die Initiale
    std::map<std::string, ImagePtr> avatars;  // URL → Bild
    std::set<std::string> avatar_failed;
    std::deque<std::string> avatar_queue;
    std::thread avatar_thread;
    std::condition_variable avatar_cv;

    bool watched(uint64_t gid, uint64_t uid) {
        auto u = users.find(uid);
        if ((u != users.end() && u->second.bot) || ignore_ids.count(uid) || uid == bot_id) return false;
        if (!guild_ids.empty() && !guild_ids.count(gid)) return false;
        return friend_ids.empty() || friend_ids.count(uid);
    }

    void changed() {
        if (on_change) on_change();
    }

    int server_count_locked() {
        int n = 0;
        for (auto& [gid, g] : guilds) n += guild_ids.empty() || guild_ids.count(gid);
        return n;
    }

    void update_ready_status_locked() {
        if (!ready || fatal) return;
        status = server_count_locked() ? "" : "Der Bot ist auf keinem Server – bitte einladen (siehe Anleitung)";
    }

    static std::string user_avatar(const dpp::user& u) {
        std::string url = u.avatar.to_string().empty() ? u.get_default_avatar_url() : u.get_avatar_url(64, dpp::i_png, false);
        return url;
    }

    void store_user(const dpp::user& u) {
        if (!u.id) return;
        UserInfo& info = users[u.id];
        if (!u.username.empty()) info.username = u.username;
        info.global_name = u.global_name;
        info.bot = u.is_bot();
        std::string av = user_avatar(u);
        if (!av.empty()) info.avatar_url = av;
    }

    void store_member(uint64_t gid, const dpp::guild_member& m) {
        MemberInfo& mi = guilds[gid].members[m.user_id];
        mi.nick = m.get_nickname();
        mi.avatar_url = m.avatar.to_string().empty() ? "" : m.get_avatar_url(64, dpp::i_png, false);
        if (auto* u = m.get_user()) store_user(*u);
        else if (auto* cu = dpp::find_user(m.user_id)) store_user(*cu);
    }

    void store_presence(uint64_t gid, const dpp::presence& p) {
        PresenceInfo pi;
        pi.status = status_of(p.status());
        pi.mobile = p.mobile_status() != dpp::ps_offline;
        for (auto& a : p.activities) {
            DiscordFeed::Activity act;
            act.type = (int)a.type;
            act.name = a.name;
            act.state = a.state;
            act.details = a.details;
            act.start_ms = (long long)a.start;  // Discord liefert Millisekunden
            act.spotify = a.type == dpp::at_listening && a.name == "Spotify";
            pi.activities.push_back(std::move(act));
        }
        guilds[gid].presences[p.user_id] = std::move(pi);
    }

    void fail(const std::string& msg) {
        {
            std::lock_guard lk(mu);
            if (fatal && status == msg) return;
            fatal = true;
            status = msg;
            ready = false;
        }
        LOGE("discord", "Discord: %s", msg.c_str());
        changed();
        // Falscher Token oder fehlende Rechte: nicht endlos neu verbinden (Discord sperrt sonst kurzzeitig)
        std::lock_guard lk(mu);
        restart_wanted = true;
        cv.notify_all();
    }

    void make_bot() {
        uint32_t intents = dpp::i_guilds | dpp::i_guild_members | dpp::i_guild_presences;
        bot = std::make_unique<dpp::cluster>(token, intents, 0, 0, 1, true, dpp::cache_policy::cpol_default, 2);
        bot->on_log([this](const dpp::log_t& e) {
            const std::string& m = e.message;
            if (m.find("4014") != std::string::npos || m.find("Disallowed intent") != std::string::npos ||
                m.find("4013") != std::string::npos)
                fail("Discord verweigert den Zugriff: im Developer Portal beide „Intents“ einschalten");
            else if (m.find("Invalid bot token") != std::string::npos || m.find("4004") != std::string::npos ||
                     m.find("Authentication failed") != std::string::npos)
                fail("Bot-Token ungültig – bitte in den Einstellungen neu eintragen");
            else if (m.find("get_gateway_bot") != std::string::npos) {
                {
                    std::lock_guard lk(mu);
                    status = "Keine Verbindung zu Discord";
                    restart_wanted = true;  // später erneut versuchen (z. B. Internet beim Start noch nicht da)
                }
                LOGW("discord", "Discord: %s", m.c_str());
                cv.notify_all();
                changed();
            } else if (e.severity >= dpp::ll_warning) {
                LOGW("discord", "%s", m.c_str());
            } else {
                LOGD("discord", "%s", m.c_str());
            }
        });
        bot->on_ready([this](const dpp::ready_t& e) {
            {
                std::lock_guard lk(mu);
                ready = true;
                fatal = false;
                bot_id = bot->me.id;
                bot_name = bot->me.username;
                if (e.guilds.empty() && guilds.empty())
                    status = "Der Bot ist auf keinem Server – bitte einladen (siehe Anleitung)";
                else
                    update_ready_status_locked();
            }
            LOGI("discord", "Discord verbunden (%d Server gemeldet)", (int)e.guilds.size());
            changed();
        });
        bot->on_guild_create([this](const dpp::guild_create_t& e) {
            {
                std::lock_guard lk(mu);
                GuildInfo& g = guilds[e.created.id];
                g.name = e.created.name;
                for (auto& [uid, m] : e.created.members) store_member(e.created.id, m);
                for (auto& [uid, p] : e.presences) store_presence(e.created.id, p);
                update_ready_status_locked();
                LOGI("discord", "Discord: Server „%s“ mit %d Mitgliedern", g.name.c_str(), (int)g.members.size());
            }
            changed();
        });
        bot->on_guild_update([this](const dpp::guild_update_t& e) {
            std::lock_guard lk(mu);
            if (auto it = guilds.find(e.updated.id); it != guilds.end()) it->second.name = e.updated.name;
        });
        bot->on_guild_delete([this](const dpp::guild_delete_t& e) {
            {
                std::lock_guard lk(mu);
                guilds.erase(e.deleted.id);
                update_ready_status_locked();
            }
            changed();
        });
        bot->on_guild_members_chunk([this](const dpp::guild_members_chunk_t& e) {
            {
                std::lock_guard lk(mu);
                for (auto& [uid, m] : e.members) store_member(e.adding.id, m);
            }
            changed();
        });
        bot->on_presence_update([this](const dpp::presence_update_t& e) {
            bool w;
            {
                std::lock_guard lk(mu);
                store_presence(e.rich_presence.guild_id, e.rich_presence);
                if (auto* u = dpp::find_user(e.rich_presence.user_id)) store_user(*u);
                w = watched(e.rich_presence.guild_id, e.rich_presence.user_id);
            }
            if (w) changed();
        });
        bot->on_guild_member_add([this](const dpp::guild_member_add_t& e) {
            {
                std::lock_guard lk(mu);
                store_member(e.adding_guild.id, e.added);
            }
            changed();
        });
        bot->on_guild_member_update([this](const dpp::guild_member_update_t& e) {
            bool w;
            {
                std::lock_guard lk(mu);
                store_member(e.updating_guild.id, e.updated);
                w = watched(e.updating_guild.id, e.updated.user_id);
            }
            if (w) changed();
        });
        bot->on_guild_member_remove([this](const dpp::guild_member_remove_t& e) {
            {
                std::lock_guard lk(mu);
                auto it = guilds.find(e.removing_guild.id);
                if (it != guilds.end()) {
                    it->second.members.erase(e.removed.id);
                    it->second.presences.erase(e.removed.id);
                }
            }
            changed();
        });
    }

    void run_supervisor() {
        while (!stop) {
            {
                std::lock_guard lk(mu);
                restart_wanted = false;
            }
            try {
                make_bot();
                bot->start(dpp::st_return);
            } catch (const std::exception& e) {
                LOGW("discord", "Discord: %s", e.what());
                std::lock_guard lk(mu);
                status = "Keine Verbindung zu Discord";
                restart_wanted = true;
            }
            std::unique_lock lk(mu);
            // Wächter: kommt keine Verbindung zustande (z. B. beim Windows-Start noch kein Internet, DNS-Fehler),
            // nach 90 Sekunden neu verbinden – D++ versucht den ersten Abruf sonst nie wieder
            double started = mono();
            while (!stop && !restart_wanted) {
                cv.wait_for(lk, std::chrono::seconds(5));
                if (!ready && !fatal && mono() - started > 90) {
                    LOGW("discord", "Discord: keine Verbindung nach 90 s – neuer Versuch");
                    status = "Keine Verbindung zu Discord";
                    restart_wanted = true;
                }
            }
            bool was_fatal = fatal;
            lk.unlock();
            if (bot) {
                try {
                    bot->shutdown();
                } catch (...) {
                }
                bot.reset();
            }
            if (stop) break;
            {
                std::lock_guard lk2(mu);
                ready = false;
                guilds.clear();
            }
            // Fehler beim Token/Rechte: erst nach 10 Minuten erneut; Netzwerk: nach 60 Sekunden
            double wait = was_fatal ? 600 : 30;
            std::unique_lock lk3(mu);
            cv.wait_for(lk3, std::chrono::milliseconds((long long)(wait * 1000)), [&] { return stop.load(); });
        }
    }

    void run_avatars() {
        while (true) {
            std::string url;
            {
                std::unique_lock lk(mu);
                avatar_cv.wait(lk, [&] { return stop || !avatar_queue.empty(); });
                if (stop) return;
                url = avatar_queue.front();
                avatar_queue.pop_front();
            }
            http::Request r;
            r.url = url;
            r.headers = {{"User-Agent", UA}};
            r.max_bytes = 2'000'000;
            auto resp = http::fetch(r);
            ImagePtr img;
            if (resp.status == 200 && !resp.truncated)
                if (auto im = decode_image(resp.body)) img = std::make_shared<const Image>(thumbnail(*im, 64, 64));
            {
                std::lock_guard lk(mu);
                if (img) avatars[url] = img;
                else avatar_failed.insert(url);  // Initialen-Platzhalter
            }
            if (img) changed();
        }
    }

    ImagePtr avatar_for(const std::string& url) {  // unter mu aufrufen
        if (url.empty()) return nullptr;
        if (auto it = avatars.find(url); it != avatars.end()) return it->second;
        if (!avatar_failed.count(url) && std::find(avatar_queue.begin(), avatar_queue.end(), url) == avatar_queue.end()) {
            avatar_queue.push_back(url);
            avatar_cv.notify_all();
        }
        return nullptr;
    }

    std::string display_name(uint64_t gid, uint64_t uid) {
        auto& g = guilds[gid];
        auto mi = g.members.find(uid);
        auto ui = users.find(uid);
        std::string username = ui != users.end() ? ui->second.username : "";
        std::string name = mi != g.members.end() && !mi->second.nick.empty() ? mi->second.nick
                           : ui != users.end() && !ui->second.global_name.empty() ? ui->second.global_name
                                                                                   : username;
        std::string c = render::clean_text(name);
        return c.empty() ? username : c;
    }

    std::string describe(const std::vector<DiscordFeed::Activity>& acts) {
        std::string custom;
        for (auto& a : acts) {
            if (a.type == dpp::at_streaming)
                return render::clean_text(trim("Streamt " + (a.state.empty() ? a.name : a.state)));
            if (a.spotify && !a.details.empty()) return render::clean_text("Hört " + a.details + " – " + a.state);
            if (a.type == dpp::at_custom) {
                custom = render::clean_text(!a.state.empty() ? a.state : (a.name == "Custom Status" ? "" : a.name));
                continue;
            }
            if (a.type == dpp::at_game) return render::clean_text("Spielt " + a.name) + duration_text(a.start_ms / 1000);
            if (a.type == dpp::at_listening) return render::clean_text("Hört " + a.name);
            if (a.type == dpp::at_watching) return render::clean_text("Schaut " + a.name);
            if (a.type == dpp::at_competing) return render::clean_text("Tritt an in " + a.name);
        }
        return custom;
    }
};

DiscordFeed::DiscordFeed(const ojson& cfg, std::function<void()> on_change) : d_(std::make_unique<Impl>()) {
    d_->cfg = cfg;
    d_->on_change = std::move(on_change);
    auto ids = [&](const char* k) {
        std::set<uint64_t> out;
        if (cfg.contains(k) && cfg[k].is_array())
            for (auto& x : cfg[k]) {
                try {
                    out.insert(x.is_number() ? x.get<uint64_t>() : std::stoull(x.get<std::string>()));
                } catch (...) {
                }
            }
        return out;
    };
    d_->friend_ids = ids("friend_ids");
    d_->ignore_ids = ids("ignore_ids");
    d_->guild_ids = ids("guild_ids");
    d_->token = trim(cfg.contains("token") && cfg["token"].is_string() ? cfg["token"].get<std::string>() : "");
}

DiscordFeed::~DiscordFeed() { stop(); }

void DiscordFeed::start() {
    if (d_->token.empty()) {
        std::lock_guard lk(d_->mu);
        d_->status = "Kein Bot-Token – bitte in den Einstellungen eintragen";
        d_->changed();
        return;
    }
    d_->avatar_thread = std::thread([this] { d_->run_avatars(); });
    d_->supervisor = std::thread([this] { d_->run_supervisor(); });
}

void DiscordFeed::stop() {
    if (d_->stop.exchange(true)) return;
    d_->cv.notify_all();
    d_->avatar_cv.notify_all();
    if (d_->supervisor.joinable()) d_->supervisor.join();
    if (d_->avatar_thread.joinable()) d_->avatar_thread.join();
}

bool DiscordFeed::ready() {
    std::lock_guard lk(d_->mu);
    return d_->ready && !d_->fatal;
}

std::string DiscordFeed::status() {
    std::lock_guard lk(d_->mu);
    return d_->status;
}

std::string DiscordFeed::bot_name() {
    std::lock_guard lk(d_->mu);
    return d_->bot_name;
}

int DiscordFeed::server_count() {
    std::lock_guard lk(d_->mu);
    return d_->server_count_locked();
}

std::vector<Friend> DiscordFeed::friends() {
    std::lock_guard lk(d_->mu);
    if (!d_->ready || d_->fatal) return {};
    struct Pick {
        uint64_t gid;
        const PresenceInfo* p;
    };
    std::map<uint64_t, Pick> seen;
    static const PresenceInfo offline;
    for (auto& [gid, g] : d_->guilds) {
        for (auto& [uid, m] : g.members) {
            if (!d_->watched(gid, uid)) continue;
            auto pit = g.presences.find(uid);
            const PresenceInfo* p = pit == g.presences.end() ? &offline : &pit->second;
            auto prev = seen.find(uid);
            // Bei mehreren gemeinsamen Servern das Mitglied mit Aktivität bevorzugen
            if (prev == seen.end() || (prev->second.p->activities.empty() && !p->activities.empty())) seen[uid] = {gid, p};
        }
    }
    bool show_offline = d_->cfg.contains("show_offline") && d_->cfg["show_offline"].is_boolean() && d_->cfg["show_offline"].get<bool>();
    std::vector<Friend> out;
    for (auto& [uid, pick] : seen) {
        if (!show_offline && pick.p->status == "offline") continue;
        Friend f;
        f.user_id = uid;
        f.name = d_->display_name(pick.gid, uid);
        f.status = pick.p->status;
        f.activity = d_->describe(pick.p->activities);
        f.mobile = pick.p->mobile;
        f.ids["discord"] = std::to_string(uid);
        if (f.status != "offline") f.online_on = {"discord"};
        auto& mi = d_->guilds[pick.gid].members[uid];
        std::string url = !mi.avatar_url.empty() ? mi.avatar_url
                          : d_->users.count(uid) ? d_->users[uid].avatar_url
                                                 : "";
        f.avatar = d_->avatar_for(url);
        out.push_back(std::move(f));
    }
    return out;
}

ojson DiscordFeed::member_list() {
    std::lock_guard lk(d_->mu);
    ojson out = ojson::array();
    if (!d_->ready || d_->fatal) return out;
    std::map<uint64_t, ojson> seen;
    for (auto& [gid, g] : d_->guilds) {
        if (!d_->guild_ids.empty() && !d_->guild_ids.count(gid)) continue;
        for (auto& [uid, m] : g.members) {
            auto u = d_->users.find(uid);
            if ((u != d_->users.end() && u->second.bot) || uid == d_->bot_id || seen.count(uid)) continue;
            seen[uid] = {{"id", std::to_string(uid)}, {"name", d_->display_name(gid, uid)}, {"guild", g.name}};
        }
    }
    std::vector<ojson> v;
    for (auto& [_, j] : seen) v.push_back(j);
    std::stable_sort(v.begin(), v.end(), [](const ojson& a, const ojson& b) {
        return casefold(a["name"].get<std::string>()) < casefold(b["name"].get<std::string>());
    });
    for (auto& j : v) out.push_back(j);
    return out;
}

}  // namespace fl::discord
