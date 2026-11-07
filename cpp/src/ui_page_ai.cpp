// © 2026 FuchsLabs
#include "ui_pages.h"
#include "app.h"
#include "ai_status.h"
#include "util.h"
#include "i18n.h"
#include <cstdlib>

namespace fl::ui {
namespace {
class AiPage : public Page {
public:
    PageInfo info() const override {return {"ai",Group::Sources,T(L"KI-Status",L"AI Status"),T(L"Sitzungsstatus und Kontolimits",L"Session status and account limits")};}
    bool active(const ojson& cfg) const override {return flag(cfg,"ai","enabled");}
    void build(Host& host,Column& root) override {
        plans_.clear(); form_=Form(host);
        auto c=root.add<Card>(T(L"KI-Status",L"AI Status"));
        auto enabled=form_.toggle(*c,"ai","enabled",T(L"Seite und lokalen Empfänger aktivieren",L"Enable page and local receiver"));
        auto automatic=form_.toggle(*c,"ai","automatic_usage",T(L"Automatische Nutzungsabfrage (nach Speichern/Neustart)",L"Automatic usage query (after save/restart)"));
        automatic->on_change=[automatic,&host]{if(!automatic->on) host.app().ai_status().enable_usage(false);};
        enabled->on_change=[enabled,&host]{if(!enabled->on) host.app().ai_status().enable_usage(false);};
        for(const auto provider:{"codex","claude"}) {
            const bool codex=std::string(provider)=="codex";
            auto account=root.add<Card>(codex?Text(L"Codex"):Text(L"Claude Code"));
            auto consent=form_.toggle(*account,"ai",codex?"codex_real_account":"claude_real_account",
                T(L"Echtes Konto lesend nutzen",L"Use real account read-only"));
            account->add<Note>(T(
                L"Einwilligung: Inoffizielle Schnittstelle, kann sich ändern; Anbieter-Duldung unklar. Nur lesend, keine Modellnachricht. ScreenBuddy liest den Zugriffstoken ausschließlich aus deiner Windows-Login-Datei im Arbeitsspeicher. Token verlassen den Rechner nur per HTTPS zum jeweiligen Anbieter (Claude: api.anthropic.com; Codex: chatgpt.com). Keine Speicherung, Anzeige, Protokollierung oder Token-Erneuerung. Jederzeit abschaltbar. Bestätigen, speichern und neu starten.",
                L"Consent: Unofficial interface, subject to change; provider tolerance is unclear. Read-only, no model message. ScreenBuddy reads the access token only from your Windows login file into memory. Tokens leave this computer only over HTTPS to the respective provider (Claude: api.anthropic.com; Codex: chatgpt.com). No storage, display, logging or token refresh. Disable any time. Confirm, save and restart."));
            auto confirm=account->add<ButtonRow>()->add(T(L"Gelesen und Aktivierung bestätigen",L"Confirm reading and activation"),Button::Secondary,[]{});
            confirm->enabled=false;
            consent->on_change=[consent,confirm,&host] {
                if(consent->on) {consent->on=false;confirm->enabled=true;}
                else {confirm->enabled=false;host.app().ai_status().enable_usage(false);}
                host.relayout();
            };
            confirm->on_click=[consent,confirm,&host] {consent->on=true;confirm->enabled=false;host.relayout();};
        }
        form_.number(*c,"ai","port",T(L"Empfänger-Port",L"Receiver port"),T(L"Standard 18765; bei Belegung automatisch freier Port. Einstellungsport ausgeschlossen.",L"Default 18765; automatically selects a free port if busy. Settings port excluded."),1024,65535);
        usage_=c->add<Note>(L"");
        c->add<Note>(T(L"Abfragen im Hintergrund: je Anbieter frühestens alle 3 Minuten, nach Fehler 15 Minuten Pause. Ohne Konto-Einwilligung keine Login-Dateien; bisherige isolierte CLI bleibt möglich.",L"Background queries: at least 3 minutes per provider, 15-minute pause on failure. Without account consent no login files; existing isolated CLI remains available."));
        receiver_=c->add<Note>(T(L"Empfängerstatus wird geprüft.",L"Checking receiver status."));
        c->add<Note>(T(L"Nach dem Speichern neu starten. Tatsächlicher Port siehe Statuszeile. Fehlende Werte bleiben n/a. Kosten sind Schätzungen in USD; keine Abrechnungsdaten.",
                      L"Restart after saving. Actual port shown in status line. Missing values remain n/a. Costs are estimates in USD, not billing data."));
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
            Text(std::wstring(i18n::lang()=="en"?L"Receiver active: 127.0.0.1:":L"Empfänger aktiv: 127.0.0.1:")+std::to_wstring(host.app().ai_status().port())):
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
