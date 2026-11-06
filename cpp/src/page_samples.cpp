// Eingefrorene Vorschau-Eingaben; Änderungen benötigen eine eigene Baseline.
#include "page_samples.h"
#include "util.h"
#include <cmath>

namespace fl::render::samples {
spotify::Snapshot spotify() {
    spotify::Snapshot s;
    s.state = spotify::State::Playing;
    s.title = "Numb";
    s.artist = "Linkin Park";
    s.album = "Meteora";
    s.progress_ms = 83000;
    s.duration_ms = 187000;
    s.playing = true;
    s.controls = true;
    return s;
}
// ---------- Beispieldaten ----------
std::vector<Friend> friends() {
    auto f = [](uint64_t id, const char* name, const char* st, const char* act = "", bool mobile = false) {
        Friend x;
        x.user_id = id; x.name = name; x.status = st; x.activity = clean_text(act); x.mobile = mobile;
        return x;
    };
    return {f(1, "Fuchs", "online", "Spielt Battlefield 6 · 42 min"), f(2, "Basti", "dnd", "Streamt Battlefield 6"),
            f(3, "Lena", "idle", "🌙 gleich zurück"),                 f(4, "Kevin", "online", "Hört Numb – Linkin Park", true),
            f(5, "Mara", "online", ""),                                f(6, "Tom", "offline"),
            f(7, "Jonas", "online", "Spielt Counter-Strike 2 · 1 h 05 min"), f(8, "Sven", "idle", "")};
}

std::vector<StreamItem> stream() {
    // Frei erfundene Beispieltitel für Vorschau und Tests
    auto mk = [](int id, const char* title, const char* date, bool series, std::vector<std::string> prov) {
        StreamItem it;
        it.id = id; it.title = title; it.date = date; it.series = series; it.providers = std::move(prov);
        static const std::map<std::string, int> ids = {{"NF", 8}, {"PV", 9}, {"D+", 337}, {"ATV+", 350}, {"WOW", 30}, {"RTL+", 2750}};
        for (auto& ab : it.providers) it.provider_ids.push_back(ids.at(ab));
        return it;
    };
    return {mk(1, "Der Leuchtturm am Ende der Welt", "2026-10-30", false, {"NF"}),
            mk(2, "Nordlicht – Staffel 2", "2026-10-28", true, {"PV", "D+"}),
            mk(3, "Fuchsbau", "2026-10-24", false, {"ATV+"}),
            mk(4, "Die Ersatzspieler von Hinterdorf", "2026-10-21", true, {"RTL+"}),
            mk(5, "Mitternachtsexpress", "2026-10-15", false, {"WOW", "NF"})};
}

StreamLogos stream_logos() {
    // Platzhalter für Vorschau und Tests: einfarbige, abgerundete Kacheln mit dem Kürzel; kein Netz, keine Marken-Logos
    struct Spec { int id; const char* ab; Color c; };
    const Spec specs[] = {{8, "NF", {150, 30, 40}}, {9, "PV", {30, 110, 160}}, {337, "D+", {40, 60, 140}},
                          {350, "TV", {60, 60, 60}}, {30, "WOW", {170, 70, 30}}, {2750, "RTL", {130, 40, 130}}};
    StreamLogos out;
    for (const auto& sp : specs) {
        Canvas c(64, 64, 1.f, sp.c);
        c.text(32, 32, sp.ab, Font{22, true}, Color{255, 255, 255}, "mm");
        Image img = c.finish();
        tmdb::round_corners(img.px, img.w, img.h, 64 * 0.2f);
        out[sp.id] = std::make_shared<const Image>(std::move(img));
    }
    return out;
}

std::vector<HaEntity> ha() {
    auto mk = [](const char* id, const char* name, const char* state, const char* unit, bool toggle) {
        HaEntity e;
        e.id = id; e.name = name; e.state = state; e.unit = unit; e.available = true;
        e.toggle = toggle; e.on = std::string(state) == "on";
        return e;
    };
    return {mk("sensor.wohnzimmer_temperatur", "Wohnzimmer", "21.53", "°C", false),
            mk("sensor.solar_leistung", "Solar", "2840", "W", false),
            mk("light.schreibtisch", "Schreibtischlampe", "on", "", true),
            mk("switch.kaffeemaschine", "Kaffeemaschine", "off", "", true)};
}

std::vector<Bf6Player> players() {
    auto p = [](const char* n, int rank, const char* rn, double kd, double win, double kpm, double hrs) {
        Bf6Player x;
        x.query = x.name = n; x.rank = rank; x.rank_name = rn; x.kd = kd; x.win_pct = win; x.kpm = kpm; x.hours = hrs;
        return x;
    };
    Bf6Player typo;
    typo.query = typo.name = "Tippfehler";
    typo.error = "nicht gefunden";
    return {p("Fuchs", 174, "First Lieutenant III", 3.02, 51.6, 0.74, 189.9), p("Basti", 121, "Captain I", 1.48, 54.2, 0.61, 96.3),
            p("Lena", 88, "Sergeant Major II", 1.12, 49.0, 0.52, 61.0),       p("Kevin", 57, "Staff Sergeant III", 0.94, 47.5, 0.44, 40.2),
            p("Mara", 33, "Corporal II", 0.81, 50.3, 0.39, 18.7),             typo};
}


SysSnapshot system() {
    SysSnapshot sn;
    sn.cpu_name = "Ryzen 7 5700X";
    sn.cpu_pct = 23; sn.cpu_ghz = 4.45; sn.cpu_temp = 61; sn.cpu_fan = 1180;
    sn.gpu_name = "RTX 4070"; sn.gpu_pct = 91; sn.gpu_temp = 77; sn.gpu_fan = 45;
    sn.gpu_mem_used = 5300; sn.gpu_mem_total = 12282;
    sn.ram_pct = 48; sn.ram_used = 15.4; sn.ram_total = 32;
    sn.disk_pct = 71; sn.net_name = "Skynet"; sn.net_down = 2.5e6; sn.net_up = 180e3; sn.hwinfo = true;
    for (int i = 0; i < 60; ++i) {
        sn.down_hist.push_back(1e6 + 1e6 * std::sin(i / 6.0));
        sn.up_hist.push_back(2e5 + 1e5 * std::cos(i / 5.0));
    }
    return sn;
}
std::vector<Post> posts(const std::string& lang) {
    return lang == "en"
        ? std::vector<Post>{{"Example blog", "A short title for the preview", CLOCK - 600, ""},
                            {"Another blog", "Second post with a slightly longer title that wraps onto another line", CLOCK - 7200, ""},
                            {"Tech digest", "A longer headline for the example feed", std::nullopt, ""}}
        : std::vector<Post>{{"Beispiel-Blog", "Ein kurzer Titel für die Vorschau", CLOCK - 600, ""},
                            {"Noch ein Blog", "Zweiter Beitrag mit etwas längerem Titel, der umbricht und noch weiter geht als gedacht", CLOCK - 7200, ""},
                            {"heise online", "heise+ | Test: Sehr langer Titel mit Präfix", std::nullopt, ""}};
}

}  // namespace fl::render::samples
