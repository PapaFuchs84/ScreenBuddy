// © 2026 FuchsLabs
#include "ui_pages.h"
#include "app.h"
#include "ai_status.h"
#include "util.h"
#include <cstdlib>

namespace fl::ui {
namespace {
class AiPage : public Page {
public:
    PageInfo info() const override {return {"ai",Group::Sources,T(L"KI-Status",L"AI Status"),T(L"Claude Code und Codex, nur lokal",L"Claude Code and Codex, local only")};}
    bool active(const ojson& cfg) const override {return flag(cfg,"ai","enabled");}
    void build(Host& host,Column& root) override {
        plans_.clear(); form_=Form(host);
        auto c=root.add<Card>(T(L"KI-Status",L"AI Status"));
        auto enabled=form_.toggle(*c,"ai","enabled",T(L"Seite und lokalen Empfänger aktivieren",L"Enable page and local receiver"));
        auto automatic=form_.toggle(*c,"ai","automatic_usage",T(L"Automatische CLI-Nutzungsabfrage (isoliert, nach Speichern/Neustart)",L"Automatic CLI usage query (isolated, after save/restart)"));
        automatic->on_change=[automatic,&host]{if(!automatic->on) host.app().ai_status().enable_usage(false);};
        enabled->on_change=[enabled,&host]{if(!enabled->on) host.app().ai_status().enable_usage(false);};
        usage_=c->add<Note>(L"");
        c->add<Note>(T(L"Ein Hintergrundprogramm gleichzeitig; nach Fehler 15 Minuten Pause. Keine Modellnachricht. Isoliertes CLI-Verzeichnis ohne kopierte Anmeldung: ohne verfügbares Konto bleiben Limits n/a.",L"One background program at a time; 15-minute pause on failure. No model message. Isolated CLI home without copied sign-in: limits stay n/a without an available account."));
        receiver_=c->add<Note>(T(L"Empfängerstatus wird geprüft.",L"Checking receiver status."));
        c->add<Note>(T(L"Nach dem Speichern neu starten. Empfänger: 127.0.0.1:18765. Fehlende Werte bleiben n/a. Kosten sind Schätzungen in USD; keine Abrechnungsdaten.",
                      L"Restart after saving. Receiver: 127.0.0.1:18765. Missing values remain n/a. Costs are estimates in USD, not billing data."));
        c->add<Note>(T(L"Hooks senden nur Sitzungs-ID, Ereignis, Toolname und Modell, keine Prompts oder Tool-Argumente. Die Claude-Statuszeile liefert Sitzungskosten, Kontext und gegebenenfalls Limits. Tagesgesamtkosten und Codex-Tages-Tokens bleiben ohne belegte Quelle n/a. Cloud-Sitzungen unterstützen lokale Hooks möglicherweise nicht.",
                      L"Hooks send only session ID, event, tool name and model, never prompts or tool arguments. The Claude status line supplies session costs, context and optional limits. Daily cost totals and Codex daily tokens remain n/a without a proven source. Cloud sessions may not support local hooks."));
        auto setup=root.add<Card>(T(L"Hooks einrichten oder entfernen",L"Set up or remove hooks"));
        setup->add<Note>(T(L"Zuerst Vorschau laden, dann anwenden. Vorhandene Dateien erhalten einmalig .bak. Entfernen löscht ausschließlich die hier registrierten Befehle. Eine freie Claude-Statuszeile wird ergänzt; eine fremde bleibt unverändert und muss den Adapter selbst aufrufen. Schlüssel: Windows-DPAPI, nie im Hook-Befehl.",
                          L"Load a preview, then apply. Existing files receive a one-time .bak backup. Removal deletes only commands registered here. An unused Claude status line is added; an existing foreign status line stays unchanged and must call the adapter itself. Key: Windows DPAPI, never in hook commands."));
        auto buttons=setup->add<ButtonRow>();
        buttons->add(T(L"Einrichtung anzeigen",L"Preview setup"),Button::Secondary,[this,&host]{preview(host,false);});
        buttons->add(T(L"Entfernung anzeigen",L"Preview removal"),Button::Secondary,[this,&host]{preview(host,true);});
        preview_=setup->add<TextField>(TextField::Multi,L"");preview_->lines=12;preview_->limit=2162688;preview_->width=600;preview_->track_dirty=false;
        apply_=setup->add<ButtonRow>()->add(T(L"Angezeigte Änderung anwenden",L"Apply previewed change"),Button::Primary,[this,&host]{apply(host);});
        apply_->enabled=false;
        status_=setup->add<Note>(T(L"Noch keine Vorschau geladen.",L"No preview loaded yet."));
    }
    void collect(Host&,ojson& patch) override {form_.collect(patch);}
    void tick(Host& host,const Status&) override {
        if(usage_) {
            const auto snap=host.app().ai_status().model().snapshot(std::time(nullptr));std::string text;
            for(auto provider:{"claude","codex"}) {const auto& m=snap.providers.at(provider);
                text+=std::string(provider)+": "+m.value("source",std::string("none"))+"; ";
                if(m["updated_at"].is_number()) text+=std::to_string(std::max<std::time_t>(0,std::time(nullptr)-m["updated_at"].get<std::time_t>()))+" s; ";
                else text+="n/a; ";
                text+=ai::usage_reason(m.value("reason",std::string("no_data")));
                if(m.contains("cli_reason") && m["cli_reason"].is_string() && m["cli_reason"]!=m["reason"]) text+="; CLI: "+ai::usage_reason(m["cli_reason"].get<std::string>());
                text+="\n";
            }
            usage_->text=widen(text);
        }
        if(preview_ && preview_->edit()) SendMessageW(preview_->edit(),EM_SETREADONLY,TRUE,0);
        if(receiver_) receiver_->text=host.app().ai_status().running()?
            T(L"Empfänger aktiv: 127.0.0.1:18765",L"Receiver active: 127.0.0.1:18765"):
            T(L"Empfänger nicht aktiv. Aktivierung speichern und neu starten; bei Fehlern Port, DPAPI und Dateizugriff prüfen.",
              L"Receiver inactive. Save activation and restart; on failure check port, DPAPI and file access.");
    }
private:
    Form form_;TextField* preview_=nullptr;Button* apply_=nullptr;Note* status_=nullptr;
    Note* receiver_=nullptr;Note* usage_=nullptr;
    std::vector<ai::HookPlan> plans_;
    bool removal_=false;
    void preview(Host& host,bool remove) {
        apply_->enabled=false;plans_.clear();preview_->set_text(L"");
        try {
            wchar_t home[32768];
            auto length=GetEnvironmentVariableW(L"USERPROFILE",home,32768);
            if(length==0 || length>=32768) throw std::runtime_error("No user directory.");
            const auto script=host.app().ai_status().script_path();
            std::filesystem::path base(home);
            plans_.push_back(ai::plan_hooks(base/L".claude"/L"settings.json",ai::hook_command(script,"claude"),remove));
            plans_.push_back(ai::plan_hooks(base/L".codex"/L"hooks.json",ai::hook_command(script,"codex"),remove));
            std::wstring text;
            for(const auto& p:plans_) text+=p.file.wstring()+L"\r\n"+widen(ai::hook_preview(p,ai::hook_command(script,p.file.filename()==L"settings.json"?"claude":"codex")))+L"\r\n";
            preview_->set_text(text);removal_=remove;apply_->enabled=true;
            status_->text=remove?T(L"Vorschau: eigene Hook-Befehle entfernen. Adapter und verschlüsselter Schlüssel bleiben lokal erhalten.",L"Preview: remove owned hook commands. Adapter and encrypted key remain locally."):
                Text(L"Zusätzlich: "+script.wstring()+L" und key.dpapi (verschlüsselt).",L"Additionally: "+script.wstring()+L" and key.dpapi (encrypted).");
        } catch(...) {plans_.clear();status_->text=T(L"Vorschau fehlgeschlagen: Dateien und JSON prüfen.",L"Preview failed: check files and JSON.");}
        host.relayout();
    }
    void apply(Host& host) {
        if(plans_.empty()) return;
        try {
            // Validate both snapshots before creating the adapter or changing either configuration.
            for(const auto& p:plans_) {
                auto current=ai::plan_hooks(p.file,ai::hook_command(host.app().ai_status().script_path(),p.file.filename()==L"settings.json"?"claude":"codex"),removal_);
                if(current.before!=p.before || current.existed!=p.existed) throw std::runtime_error("Changed.");
            }
            if(!removal_) host.app().ai_status().prepare_adapter();
            for(const auto& p:plans_) ai::apply_hooks(p);
            status_->text=T(L"Angewendet. Clients neu starten. Die Vorschau ist jetzt ungültig.",L"Applied. Restart clients. The preview is now invalid.");
        } catch(...) {status_->text=T(L"Nicht vollständig angewendet. Dateien prüfen und Vorschau erneut laden. Sicherungen bleiben erhalten.",L"Not fully applied. Check files and reload preview. Backups are preserved.");}
        plans_.clear();apply_->enabled=false;host.relayout();
    }
};
}
std::unique_ptr<Page> make_ai_page(){return std::make_unique<AiPage>();}
} // namespace fl::ui
