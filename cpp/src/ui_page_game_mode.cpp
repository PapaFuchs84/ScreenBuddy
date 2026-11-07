// © 2026 FuchsLabs
#include "ui_pages.h"
#include "app.h"
#include "native_settings.h"
#include <algorithm>
#include "util.h"
namespace fl::ui {
namespace {
class GameModePage:public Page {
    Form form_;
public:
    PageInfo info()const override{return {"game_mode",Group::Sources,Text(L"Game-Mode"),T(L"Spiel erkennen und eigene Ansicht anzeigen",L"Detect a game and show a custom view")};}
    bool active(const ojson& cfg)const override{return flag(cfg,"game_mode","enabled");}
    void build(Host& host,Column& root)override {
        form_=Form(host);auto card=root.add<Card>(Text(L"Game-Mode"));
        form_.toggle(*card,"game_mode","enabled",T(L"Game-Mode aktivieren",L"Enable Game-Mode"));
        std::vector<std::string> ids{"primary"};std::vector<Text> labels{T(L"Primäre Ausgabe",L"Primary output")};
        if(host.plugins().layout_displays)for(const auto& d:host.plugins().layout_displays()){auto id=d.value("id",std::string());if(id.empty() || std::find(ids.begin(),ids.end(),id)!=ids.end())continue;ids.push_back(id);labels.emplace_back(widen(d.value("name",id)));}
        auto current=form_.value("game_mode","target_display").get<std::string>();if(std::find(ids.begin(),ids.end(),current)==ids.end()){ids.push_back(current);labels.emplace_back(widen(current)+L" (offline)");}
        form_.combo(*card,"game_mode","target_display",T(L"Ziel-Display",L"Target display"),{},ids,labels);
        ids={""};labels={T(L"Ansicht wählen",L"Choose a view")};for(const auto& view:custom::Store(host.app().cfg_path().parent_path()/L"views").list()){ids.push_back("custom:"+view.id);labels.emplace_back(widen(view.name));}
        current=form_.value("game_mode","view").get<std::string>();if(!current.empty() && std::find(ids.begin(),ids.end(),current)==ids.end()){ids.push_back(current);labels.emplace_back(widen(current)+L" (?)");}
        form_.combo(*card,"game_mode","view",T(L"Eigene Ansicht",L"Custom view"),T(L"Im Editor mit + Game-Mode eine Vorlage anlegen und übernehmen.",L"Create and apply a template using + Game-Mode in the editor."),ids,labels);
        form_.toggle(*card,"game_mode","intro",T(L"Spiellogo-Intro anzeigen",L"Show game logo intro"));
        form_.toggle(*card,"game_mode","reduce_motion",T(L"Animationen reduzieren",L"Reduce animations"));
        form_.text(*card,"game_mode","ping_target",T(L"Ping-Ziel (IPv4)",L"Ping target (IPv4)"));
        auto paths=root.add<Card>(T(L"Erkennung",L"Detection"));
        auto additional=form_.text(*paths,"game_mode","additional_games",T(L"Zusätzliche Spiele",L"Additional games"),T(L"Absolute EXE-Pfade, ein Pfad pro Zeile.",L"Absolute EXE paths, one path per line."));additional->kind=TextField::Multi;additional->lines=4;additional->limit=32768;
        auto excluded=form_.text(*paths,"game_mode","exclusions",T(L"Ausschlüsse",L"Exclusions"),T(L"EXE-Pfad oder Dateiname, ein Eintrag pro Zeile.",L"EXE path or filename, one entry per line."));excluded->kind=TextField::Multi;excluded->lines=4;excluded->limit=32768;
        root.add<Note>(T(L"Speichern und neu starten. Start nach 3 s, Ende nach 5 s stabiler Erkennung. Fehlende Ansicht: normale Rotation. ETW braucht ggf. Administratorrechte/Performance Log Users; ohne Quelle n/a. FPS messen DXGI-Aufrufe, keine angezeigten Frames. Keine Spielprozess-Eingriffe.",L"Save and restart. Start after 3 s, end after 5 s stable detection. Missing view: normal rotation. ETW may require administrator/Performance Log Users rights; missing sources show n/a. FPS measure DXGI calls, not displayed frames. No game process modification."));
    }
    void collect(Host&,ojson& patch)override{form_.collect(patch);}
};
}
std::unique_ptr<Page> make_game_mode_page(){return std::make_unique<GameModePage>();}
}
