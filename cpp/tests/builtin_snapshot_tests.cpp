// P5-0: feste Eingaben, keine Quellen/Server/Audio/Registry. Bitgleich, keine Toleranz.
#include "snapshot_support.h"
#include "page_samples.h"
#include "i18n.h"
#include <windows.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace fl;
using namespace fl::render;
using namespace fl::snapshot;
namespace fs = std::filesystem;
namespace {
constexpr auto clock_value = samples::CLOCK;
constexpr double progress_time = 1000;
struct Size { const char* name; int w, h; };
std::vector<Size> sizes() {
    auto tile = tile_layout(1920, 720);
    return {{"landscape",480,320}, {"portrait",320,480}, {"small",240,160}, {"small-portrait",160,240},
            {"double",960,640}, {"tile",tile.at(0).w,tile.at(0).h}, {"safe-top-body",480,300},
            {"phone",1080,2400}, {"phone-wide",2400,1080}};
}
void require_fonts() {
    using Microsoft::WRL::ComPtr;
    ComPtr<IDWriteFactory> factory;
    ComPtr<IDWriteFontCollection> fonts;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                  reinterpret_cast<IUnknown**>(factory.GetAddressOf()))) ||
        FAILED(factory->GetSystemFontCollection(&fonts)))
        throw std::runtime_error("DirectWrite-Systemschriften konnten nicht gelesen werden; kein Vergleich.");
    for (auto name : {L"Roboto", L"Segoe UI Variable"}) {
        UINT32 index = 0; BOOL exists = FALSE;
        if (FAILED(fonts->FindFamilyName(name, &index, &exists)) || !exists) {
            if (std::wstring(name) == L"Roboto")
                throw std::runtime_error("Schrift fehlt: Roboto. Installieren; keine Baseline erzeugt oder verglichen.");
            // Windows 11 registriert nur "Segoe UI Variable Text/Display/Small"; die App fragt "Segoe UI Variable" an,
            // DirectWrite loest das rechnerabhaengig (aber deterministisch) auf. Baseline ist rechnergebunden.
            UINT32 i2 = 0; BOOL ex2 = FALSE;
            if (FAILED(fonts->FindFamilyName(L"Segoe UI Variable Text", &i2, &ex2)) || !ex2)
                throw std::runtime_error("Schrift fehlt: Segoe UI Variable. Installieren; keine Baseline erzeugt oder verglichen.");
            std::fprintf(stderr, "HINWEIS: Familie 'Segoe UI Variable' nicht direkt registriert; Rechner-Fallback der App wird verwendet (rechnergebundene Baseline).\n");
        }
    }
}
enum class Hit { None, Rows, Rss, Stream, System, Spotify, Ha };
struct Case {
    std::string name;
    std::function<Image(int,int)> draw;
    Hit hit = Hit::None;
    size_t count = 0;
};
// Hash der unveränderten statischen Trefferfunktionen, einschließlich Listenmetadaten.
std::string hit_hash(const Case& c, int w, int h) {
    Image map((w+3)/4, (h+3)/4 + 1);
    std::fill(map.px.begin(), map.px.end(), 0);
    Renderer list(w,h,"");
    map.px[0] = list.rows_per_page(); map.px[1] = list.pages(c.count);
    for (int y=0; y<h; y+=4) for (int x=0; x<w; x+=4) {
        int value = -1;
        switch (c.hit) {
        case Hit::Rows: value = Renderer::row_at(w,h,y); break;
        case Hit::Rss: value = RssRenderer::row_at(w,h,y); break;
        case Hit::Stream: value = StreamRenderer::row_at(w,h,y); break;
        case Hit::System: value = SysRenderer::zone_at(w,h,x,y); break;
        case Hit::Spotify: value = SpotifyRenderer::button_at(w,h,x,y); break;
        case Hit::Ha: value = HaRenderer::cell_at(w,h,c.count,x,y); break;
        default: break;
        }
        map.px[size_t(y/4+1)*map.w + x/4] = static_cast<uint32_t>(value);
    }
    return hash_image(map);
}
std::vector<Case> cases(const std::string& lang) {
    std::vector<Case> out;
    auto solid = std::make_shared<const Image>(Canvas(64,64,1.f,{30,120,90}).finish());
    auto posts = samples::posts(lang);
    for (auto state : {"data","loading","empty"}) {
        std::string st=state;
        out.push_back({"rss."+st,[=](int w,int h) {
            return RssRenderer(w,h,"Blog-Updates").render(st=="empty" ? std::vector<Post>{} : posts,
                st=="loading" ? std::nullopt : std::optional<std::time_t>(clock_value),clock_value);
        },Hit::Rss,st=="data" ? posts.size() : 0});
    }
    auto logos = samples::stream_logos();
    for (auto state : {"logos","abbreviations","overflow","nokey","loading","error","empty"}) {
        std::string st=state; auto items=samples::stream();
        if (st=="overflow") { items[0].providers={"NF","PV","D+","ATV+","WOW","RTL+"}; items[0].provider_ids={8,9,337,350,30,2750}; }
        StreamState status= st=="nokey" ? StreamState::NoKey : st=="loading" ? StreamState::Loading :
                            st=="error" ? StreamState::Error : StreamState::Ok;
        if (st=="empty" || status!=StreamState::Ok) items.clear();
        out.push_back({"streaming."+st,[=](int w,int h) {
            return StreamRenderer(w,h,"Neu bei Streaming").render(status,items,
                status==StreamState::Ok ? std::optional<std::time_t>(clock_value) : std::nullopt,
                clock_value,st=="abbreviations" ? nullptr : &logos);
        },Hit::Stream,items.size()});
    }
    for (auto state : {"page1","page2","error","loading","empty","rank-no-icon","rank-icon"}) {
        std::string st=state; auto players=samples::players();
        // Genug Einträge für eine zweite Seite in jeder logischen Größe.
        if (st=="page1" || st=="page2") {
            for (int i=0;i<40;++i) { auto p=players[0]; p.name="Spieler "+std::to_string(i); players.push_back(p); }
        } else if (st=="error") players={players.back()};
        else if (st=="empty" || st=="loading") players.clear();
        else { players={players[0]}; if(st=="rank-icon") players[0].rank_icon=solid; }
        out.push_back({"bf6."+st,[=](int w,int h) {
            return Bf6Renderer(w,h,"Battlefield 6 · Rangliste").render(players,st=="page2" ? 1 : 0,
                st=="loading" ? std::nullopt : std::optional<std::time_t>(clock_value),clock_value);
        },Hit::Rows,players.size()});
    }
    for (auto title : {"friends","discord"}) for (auto state : {"page1","page2","disconnected","empty","avatar","offline","mobile","platforms"}) {
        std::string st=state, key=title; auto friends=samples::friends();
        if (st=="page1" || st=="page2") {
            for(int i=0;i<40;++i) { auto f=friends[0]; f.name="Freund "+std::to_string(i); friends.push_back(f); }
        } else if(st=="empty") friends.clear();
        else if(st!="disconnected") {
            friends={friends[0]}; auto& f=friends[0];
            if(st=="avatar") f.avatar=solid;
            if(st=="offline") f.status="offline";
            if(st=="mobile") f.mobile=true;
            if(st=="platforms") { f.platforms={"discord","steam","xbox","psn"}; f.online_on=f.platforms; }
        }
        out.push_back({key+"."+st,[=](int w,int h) {
            return Renderer(w,h,key=="discord" ? "Discord-Freunde" : "Freunde").render_friends(friends,
                st=="page2" ? 1 : 0,st!="disconnected","Verbindung getrennt – bitte später erneut versuchen",clock_value);
        },Hit::Rows,friends.size()});
    }
    const std::vector<float> levels={.05f,.35f,.55f,.8f,.65f,.9f,1.f,.7f,.5f,.75f,.85f,.6f,.45f,.25f,.15f};
    for (auto state : {"playing-0","playing-1","playing-2","simulated","paused","no-controls","last-played","idle",
                       "not-set-up","logged-out","connecting","connecting-error","error","error-detail","cover","footer-error"}) {
        std::string st=state; auto snap=samples::spotify(); snap.fetched_at=progress_time;
        int color=st=="playing-0" ? 0 : st=="playing-2" ? 2 : 1;
        if(st=="paused") { snap.state=spotify::State::Paused; snap.playing=false; }
        if(st=="no-controls") snap.controls=false;
        if(st=="last-played" || st=="idle") { snap.state=spotify::State::Idle; snap.playing=false; snap.controls=false; snap.last_played=st=="last-played"; }
        if(st=="not-set-up") snap.state=spotify::State::NotSetUp;
        if(st=="logged-out") snap.state=spotify::State::LoggedOut;
        if(st=="connecting" || st=="connecting-error") snap.state=spotify::State::Connecting;
        if(st=="error" || st=="error-detail") snap.state=spotify::State::Error;
        if(st=="connecting-error" || st=="error-detail" || st=="footer-error") snap.error="Beispiel: Wiedergabe nicht erreichbar";
        if(st=="cover") snap.cover=solid;
        out.push_back({"spotify."+st,[=](int w,int h) {
            return SpotifyRenderer(w,h,"Spotify").render(snap,progress_time,&levels,color,st=="simulated",clock_value);
        },Hit::Spotify});
    }
    for(auto state : {"full","no-hwinfo","null","temp75","temp85","usage90","history0","history1"}) {
        std::string st=state; auto snap=samples::system();
        if(st=="no-hwinfo") snap.hwinfo=false;
        if(st=="null") snap=SysSnapshot{};
        if(st=="temp75" || st=="temp85") snap.cpu_temp=snap.gpu_temp=snap.disk_temp=st=="temp75" ? 75 : 85;
        if(st=="usage90") snap.cpu_pct=snap.gpu_pct=snap.ram_pct=snap.disk_pct=95;
        if(st=="history0" || st=="history1") { snap.down_hist.clear(); snap.up_hist.clear(); if(st=="history1") { snap.down_hist.push_back(1e6); snap.up_hist.push_back(2e5); } }
        out.push_back({"system."+st,[=](int w,int h) { return SysRenderer(w,h,"System","bit").render(snap,clock_value); },Hit::System});
    }
    // HA und Willkommen sind ausschließlich Kontrollbilder, keine Migrationsziele.
    auto ha=samples::ha();
    if(lang=="en") { ha[0].name="Living room"; ha[2].name="Desk lamp"; ha[3].name="Coffee maker"; }
    for(int n : {4,3,2,1,0}) {
        auto entities=ha; entities.resize(n);
        if(n==2) entities={ha[2],ha[0]};
        out.push_back({"control-ha."+std::to_string(n),[=](int w,int h) {
            return HaRenderer(w,h,"Home Assistant").render(entities,n!=0,n ? "" : "Home Assistant ist nicht erreichbar – Adresse und Netzwerk prüfen",
                n ? std::optional<std::time_t>(clock_value) : std::nullopt,clock_value);
        },Hit::Ha,entities.size()});
    }
    out.push_back({"control-welcome",[](int w,int h) {
        return Renderer(w,h,"").welcome("Willkommen!","Öffne die Einstellungen über das Fuchs-Symbol im Infobereich.");
    }});
    return out;
}
}
int main(int argc,char** argv) {
    try {
        _putenv_s("TZ","UTC0"); _tzset();
        const std::string mode=argc>1 ? argv[1] : "pruefen";
        if(argc>3 || (mode!="export" && argc>2) || (mode!="pruefen" && mode!="prüfen" && mode!="update" && mode!="export") ||
           (mode=="export" && argc!=3)) throw std::runtime_error("Aufruf: fl-builtin-snapshot-tests [pruefen|update|export <Ordner>]");
        require_fonts(); gfx_init();
        const fs::path root=mode=="export" ? fs::path(argv[2]) : fs::path(FL_BUILTIN_SNAPSHOT_DIR);
        std::map<std::string,std::string> expected, actual;
        if(mode!="update" && mode!="export") {
            std::ifstream in(root/"hashes.txt");
            if(!in) throw std::runtime_error("Built-in-Baseline fehlt: hashes.txt (explizit update ausführen).");
            for(std::string line;std::getline(in,line);) {
                if(line.empty() || line[0]=='#') continue;
                std::istringstream fields(line); std::string key,hash;
                if(!(fields>>key>>hash) || !expected.emplace(key,hash).second) throw std::runtime_error("Ungültige/doppelte Baseline-Zeile: "+line);
            }
        } else fs::create_directories(root);
        int failures=0; size_t images=0;
        std::vector<std::pair<fs::path,decltype(cases("de").front().draw(1,1))>> deferred;
        const auto matrix=sizes();
        // Zwei Durchlaeufe: PNG-Schreiben (WIC) veraendert sonst die spaetere Textdarstellung (Hash-Lauf vs. Vergleich).
        // Durchlauf 1 hasht/vergleicht ohne jede PNG-Ausgabe; Durchlauf 2 (nur update/export) rendert erneut und schreibt PNGs.
        auto sweep=[&](bool png_pass) {
        for(const auto& lang : {std::string("de"),std::string("en")}) {
            i18n::set_lang(lang);
            for(const auto& tc : theme_cases()) {
                if(lang=="en" && tc.name!="dunkel" && tc.name!="custom-material") continue;
                for(const auto& size : matrix) {
                    if(lang=="en" && std::string(size.name)!="landscape" && std::string(size.name)!="portrait") continue;
                    std::lock_guard lock(render::mutex());
                    // Auch die künstlichen Logos entstehen erst nach dem festen Theme.
                    if(tc.inline_theme.is_null()) apply_theme({{"theme",tc.name}});
                    else apply_theme({{"theme_inline",tc.inline_theme}});
                    for(const auto& c : cases(lang)) {
                        std::string key=lang+"."+tc.name+"."+size.name+"."+c.name;
                        if(png_pass) {
                            if(mode=="export" || (lang=="de" && tc.name=="dunkel")) write_png(root/(key+".png"),c.draw(size.w,size.h));
                            continue;
                        }
                        // Jeder Aufruf baut einen neuen Zeichner nach apply_theme.
                        auto image=c.draw(size.w,size.h);
                        if(image.w!=size.w || image.h!=size.h) throw std::runtime_error("Falsche Bildgröße: "+c.name);
                        if(!actual.emplace(key,hash_image(image)).second ||
                           !actual.emplace(key+".touch",hit_hash(c,size.w,size.h)).second)
                            throw std::runtime_error("Doppelter Snapshot-Schlüssel: "+key);
                        ++images;
                        bool differs=false;
                        for(const auto& k : {key,key+".touch"}) if(mode!="update" && mode!="export" &&
                            (!expected.contains(k) || expected.at(k)!=actual.at(k))) {
                            ++failures; differs=true; std::cerr<<"Abweichung: "<<k<<" erwartet "<<(expected.contains(k) ? expected.at(k) : "FEHLT")<<" aktuell "<<actual.at(k)<<'\n';
                        }
                        if(differs) {
                            auto dir=fs::temp_directory_path()/"fl-builtin-snapshot-actual"; fs::create_directories(dir);
                            deferred.emplace_back(dir/(key+".png"),image);
                            std::ofstream(dir/(key+".txt"))<<actual[key]<<"\n"<<actual[key+".touch"]<<"\n";
                        }
                    }
                    if(!png_pass) std::cout<<lang<<' '<<tc.name<<' '<<size.name<<" fertig\n"<<std::flush;
                }
            }
        }
        };
        sweep(false);
        for(const auto& [path,image] : deferred) write_png(path,image);
        if(mode=="update" || mode=="export") sweep(true);
        if(images!=4221 || actual.size()!=8442) throw std::runtime_error("Unvollständige P5-0-Matrix (erwartet 4221 Bilder + Trefferkarten).");
        if(mode=="update" || mode=="export") {
            std::ofstream file(root/"hashes.txt",std::ios::binary);
            file<<"# P5-0 FNV-1a 64: Größe+ARGB; .touch: 4px-Raster, erste Metadatenzeile rows_per_page/pages(n). TZ=UTC0 clock=1792606500 mono=1000. Rechnergebundene DirectWrite-Baseline.\n";
            for(const auto& [k,v]:actual) file<<k<<' '<<v<<'\n';
            if(!file) throw std::runtime_error("Hashes konnten nicht geschrieben werden.");
        } else for(const auto& [k,v]:expected) if(!actual.contains(k)) { ++failures; std::cerr<<"Veralteter Eintrag: "<<k<<'\n'; }
        std::cout<<"Built-in snapshots: "<<images<<" Bilder, "<<actual.size()/2<<" Trefferkarten, "<<failures<<" Fehler\n";
        return failures ? 1 : 0;
    } catch(const std::exception& e) { std::cerr<<"Built-in snapshots ABBRUCH: "<<e.what()<<'\n'; return 2; }
}

