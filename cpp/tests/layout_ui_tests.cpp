// © 2026 FuchsLabs – isolierte native Editorprüfung, keine App-/Netzwerk-Schleife.
#include "ui_pages.h"
#include "ui_layout_grid.h"
#include "native_settings.h"
#include "app.h"
#include "util.h"
#include "snapshot_support.h"
#include <oleacc.h>
#include <wincodec.h>
#include <deque>
#include <fstream>
#include <sstream>
using namespace fl;
using namespace fl::ui;
using plugins::Json;
namespace fs=std::filesystem;
static int passed=0,failed=0;
#define CHECK(x) do { if(x) ++passed; else { ++failed; std::printf("FAIL %d: %s\n",__LINE__,#x); } } while(0)

static Json fixture() {
    return {{"schema_version",2},{"id","editor-test"},{"name","Editor Test"},{"version","1.0.0"},
        {"settings", {{{"key","private_key"},{"label","Private"},{"type","secret"}},
                      {{"key","label"},{"label","Label"},{"type","text"},{"default","Example"}}}},
        {"layout",{{"columns",{1,1,1}},{"rows",{1,1,1}},{"zones",{
            {{"id","one"},{"column",0},{"row",0}},{{"id","two"},{"column",2},{"row",2}}}}}},
        {"widgets", {{{"id","a"},{"type","text"},{"text","Sample A"},{"zone","one"}},
                     {{"id","b"},{"type","text"},{"text","Sample B"},{"zone","two"}}}}};
}
static ojson offline_config() {
    auto cfg=config::defaults();
    for(auto s:{"companion","system","discord","friends","bf6","rss","streaming","homeassistant","spotify"}) cfg[s]["enabled"]=false;
    cfg["ui"]["update_check"]=false; cfg["ui"]["port"]=18865; return cfg;
}
class TestHost : public Host {
public:
    Kit kit;
    Surface view{kit,nullptr,L"Editor-Test",true};
    Column root;
    ojson cfg=offline_config();
    fs::path dir;
    plugins::Store store;
    App application;
    NativePluginActions actions;
    Status state;
    std::deque<std::function<std::function<void()>()>> jobs;
    std::optional<Image> image;
    int applies=0, previews=0, toasts=0, confirmations=0;
    bool delayed=false, confirm_answer=false;
    Text question;
    std::deque<std::function<void()>> completions;
    std::wstring toast_text;
    explicit TestHost(fs::path p):dir(p),store(p/"editorplugins"),application(cfg,p/"test.toml",nullptr,p/"preview") {
        view.set_root(&root);
        actions.list=[this] {
            std::vector<NativePluginEntry> result;
            for(auto& e:store.entries()) { NativePluginEntry n; n.id=e.manifest.id; n.name=e.manifest.name; n.enabled=e.enabled; result.push_back(n); }
            return result;
        };
        actions.arrangement_get=[this](auto id) { return store.arrangement_get(id); };
        actions.arrangement_apply=[this](auto id,auto a) { auto r=store.set_arrangement(id,a.dump()); if(!r) throw std::runtime_error(r.error); ++applies; };
        actions.arrangement_reset=[this](auto id) { auto r=store.clear_arrangement(id); if(!r) throw std::runtime_error(r.error); };
    }
    ~TestHost() { view.set_root(nullptr); }
    App& app() override { return application; }
    const ojson& config() const override { return cfg; }
    NativePluginActions& plugins() override { return actions; }
    bool english() const override { return kit.english; }
    void relayout() override { view.relayout(); }
    void toast(const Text& text,ToastKind) override { toast_text=text.get(kit.english); ++toasts; }
    void run(std::function<std::function<void()>()> job) override { jobs.push_back(std::move(job)); }
    bool busy() const override { return !jobs.empty() || !completions.empty(); }
    bool confirm(const Text& text) override { ++confirmations; question=text; return confirm_answer; }
    void navigate(const std::string&) override {}
    HWND window() const override { return view.hwnd(); }
    void preview(std::optional<Image> f) override { image=std::move(f); ++previews; }
    const Status& status() const override { return state; }
    Surface& surface() override { return view; }
    void drain() { while(!jobs.empty()) { auto job=std::move(jobs.front()); jobs.pop_front(); auto done=job(); if(delayed) completions.push_back(std::move(done)); else if(done) done(); } }
    void complete() { while(!completions.empty()) { auto done=std::move(completions.front()); completions.pop_front(); if(done) done(); } }
    template<class W> W* find(const wchar_t* name=L"") {
        for(auto w:view.flatten(false)) if(auto p=dynamic_cast<W*>(w)) {
            std::wstring n=p->name.get(false); if(auto b=dynamic_cast<Button*>(w)) n=b->text.get(false);
            if(!*name || n==name) return p;
        }
        return nullptr;
    }
};
static std::wstring read_file(const fs::path& p) {
    std::ifstream f(p,std::ios::binary); return widen(std::string((std::istreambuf_iterator<char>(f)),{}));
}
static void msaa(TestHost& h,GridEditor* grid) {
    ComPtr<IAccessible> acc;
    CHECK(SUCCEEDED(AccessibleObjectFromWindow(h.view.hwnd(),OBJID_CLIENT,IID_PPV_ARGS(acc.GetAddressOf()))));
    if(!acc) return;
    long count=0; acc->get_accChildCount(&count); bool found=false;
    for(long i=1;i<=count;++i) {
        VARIANT child{}; child.vt=VT_I4; child.lVal=i; BSTR name=nullptr;
        acc->get_accName(child,&name);
        if(name && std::wstring(name)==L"Anordnungsraster") {
            found=true; VARIANT role{}; acc->get_accRole(child,&role); CHECK(role.lVal==ROLE_SYSTEM_LIST);
            BSTR val=nullptr; acc->get_accValue(child,&val); CHECK(val && std::wstring(val)==grid->value(false)); SysFreeString(val);
            CHECK(SUCCEEDED(acc->accSelect(SELFLAG_TAKEFOCUS,child)));
        }
        SysFreeString(name);
    }
    CHECK(found);
}
static void key(Surface& s,UINT vk,bool shift=false,bool alt=false,bool control=false) {
    BYTE saved[256]{},state[256]{}; GetKeyboardState(saved);
    state[VK_SHIFT]=shift?0x80:0; state[VK_MENU]=alt?0x80:0; state[VK_CONTROL]=control?0x80:0;
    SetKeyboardState(state);
    SendMessageW(s.hwnd(),alt?WM_SYSKEYDOWN:WM_KEYDOWN,vk,0);
    SetKeyboardState(saved); // nur Zustand des eigenen Testthreads, keine globalen Eingaben
}
static Image grid_image(Kit& kit,int width,int height,const plugins::Arrangement* model=nullptr,int column=0,int* first_column=nullptr) {
    ComPtr<IWICImagingFactory> wic; CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(wic.GetAddressOf()));
    ComPtr<IWICBitmap> bitmap; wic->CreateBitmap(width,height,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,bitmap.GetAddressOf());
    ComPtr<ID2D1RenderTarget> rt;
    kit.d2d()->CreateWicBitmapRenderTarget(bitmap.Get(),D2D1::RenderTargetProperties(),rt.GetAddressOf());
    ComPtr<ID2D1SolidColorBrush> brush; rt->CreateSolidColorBrush(kit.theme().text,brush.GetAddressOf());
    Ctx ctx{rt.Get(),kit,kit.theme(),brush.Get()};
    auto base=plugins::parse_manifest(fixture().dump()); GridEditor grid; grid.model=model?*model:plugins::arrangement_from(*base.manifest); grid.column=column;
    grid.rc={8,8,float(width-8),float(height-8)};
    rt->BeginDraw(); rt->Clear(kit.theme().card); grid.paint(ctx); CHECK(SUCCEEDED(rt->EndDraw()));
    if(first_column) *first_column=grid.first_column;
    Image image(width,height); bitmap->CopyPixels(nullptr,width*4,UINT(image.px.size()*4),reinterpret_cast<BYTE*>(image.px.data())); return image;
}
int main(int argc,char** argv) {
    gfx_init(); SetConsoleOutputCP(CP_UTF8);
    const auto dir=fs::temp_directory_path()/("fl-layout-ui-"+std::to_string(GetCurrentProcessId()));
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path,ec); } } cleanup{dir};
    fs::create_directories(dir);
    {
        TestHost h(dir); CHECK(h.store.load()); CHECK(h.store.import_manifest(fixture().dump()));
        auto page=make_layout_page(); page->build(h,h.root); page->shown(h); h.drain(); page->tick(h,h.state); h.drain(); page->tick(h,h.state);
        auto grid=h.find<GridEditor>(); CHECK(grid); auto blocks=h.find<LayoutBlockList>(); CHECK(blocks);
        if(!grid || !blocks) return 1;
        CHECK(count_dirty(&h.root)==0);
        for(auto w:h.view.flatten(false)) { CHECK(!w->track_dirty); CHECK(w->value(false).find(L"private_key")==std::wstring::npos); }
        CHECK(grid->role()==Role::List); CHECK(grid->value(false)==L"Zone one, Spalte 1, Zeile 1, kein Wechsel, 1 Blöcke");
        msaa(h,grid);
        const auto original=grid->model;
        h.view.focus(grid,true); SendMessageW(h.view.hwnd(),WM_KEYDOWN,VK_RIGHT,0); CHECK(grid->column==1);
        SendMessageW(h.view.hwnd(),WM_KEYDOWN,VK_LEFT,0); CHECK(grid->column==0);
        CHECK(grid->input(VK_RIGHT,false,true)); CHECK(grid->model.layout["zones"][0]["column"]==1);
        CHECK(h.find<Button>(L"Rückgängig")->enabled);
        CHECK(grid->input('Z',false,false,true)); CHECK(plugins::edit::equal(grid->model,original));
        CHECK(grid->input('Z',true,false,true)); CHECK(grid->model.layout["zones"][0]["column"]==1);
        CHECK(grid->input('Z',false,false,true));
        grid->input(VK_RIGHT,true); CHECK(grid->model.layout["zones"][0]["column_span"]==2);
        grid->input('Z',false,false,true); CHECK(plugins::edit::equal(grid->model,original));
        // Echte Surface-Nachrichten inklusive Alt+Links/Rechts und Strg+Umschalt+Z.
        h.view.focus(grid,true); key(h.view,VK_RIGHT,false,true); CHECK(grid->model.layout["zones"][0]["column"]==1);
        key(h.view,'Z',false,false,true); CHECK(plugins::edit::equal(grid->model,original));
        key(h.view,'Z',true,false,true); CHECK(grid->model.layout["zones"][0]["column"]==1);
        key(h.view,VK_LEFT,false,true); CHECK(grid->model.layout["zones"][0]["column"]==0);
        h.view.focus(blocks,true); key(h.view,VK_DOWN,false,true); CHECK(grid->model.blocks[0]["id"]=="b");
        key(h.view,'Z',false,false,true); CHECK(grid->model.blocks[0]["id"]=="a");
        // Verbergen des einzigen Blocks einer Zone sperrt die Anwendung; Undo repariert den Entwurf.
        key(h.view,VK_SPACE); CHECK(!h.find<Button>(L"Übernehmen")->enabled);
        key(h.view,'Z',false,false,true); CHECK(h.find<Button>(L"Übernehmen")->enabled);
        grid->input(VK_DOWN); grid->input(VK_SPACE); CHECK(grid->model.layout["zones"].size()==3);
        CHECK(!h.find<Button>(L"Übernehmen")->enabled); // neue Zone noch leer
        h.find<Button>(L"Fehlerstelle auswählen")->on_click(); CHECK(h.view.focused()==grid); CHECK(grid->selected==2);
        grid->input(VK_DELETE); CHECK(grid->model.layout["zones"].size()==2);
        CHECK(h.find<Button>(L"Übernehmen")->enabled);
        grid->input(VK_UP); grid->input(VK_RETURN); CHECK(grid->selected==0);
        auto id=h.find<TextField>(L"Zonen-ID"); CHECK(id); id->set_text(L"renamed"); id->on_commit();
        CHECK(grid->model.layout["zones"][0]["id"]=="renamed"); CHECK(grid->model.blocks[0]["zone"]=="renamed");
        grid->input('Z',false,false,true); CHECK(plugins::edit::equal(grid->model,original));
        // Vorschau ist tatsächlich busy: Eingabe, Fokus, Undo und Folgetasten bleiben verfügbar.
        h.drain(); page->tick(h,h.state); h.drain();
        h.delayed=true;
        grid->input(VK_RIGHT,false,true); page->tick(h,h.state); h.drain();
        CHECK(h.busy()); CHECK(!h.find<Button>(L"Übernehmen")->enabled);
        CHECK(!h.find<Combo>(L"Aktiviertes Plugin")->enabled);
        CHECK(h.find<Button>(L"Rückgängig")->enabled);
        h.view.focus(id,true); auto focused=GetFocus(); CHECK(focused==id->edit());
        page->tick(h,h.state); CHECK(GetFocus()==focused); CHECK(IsWindowEnabled(id->edit()));
        id->set_text(L"draft"); id->on_change(); id->on_commit();
        CHECK(grid->model.layout["zones"][0]["id"]=="draft"); CHECK(GetFocus()==focused);
        id->set_text(L"draft2"); id->on_change(); id->on_commit();
        CHECK(grid->model.layout["zones"][0]["id"]=="draft2");
        int previews=h.previews; h.complete(); CHECK(h.previews==previews); // altes Bild verworfen
        page->tick(h,h.state); h.drain(); CHECK(h.busy()); h.complete(); CHECK(h.previews==previews+1);
        h.delayed=false;
        while(h.find<Button>(L"Rückgängig")->enabled) grid->input('Z',false,false,true);
        CHECK(plugins::edit::equal(grid->model,original));
        // WM_KILLFOCUS darf den Fokus nicht zurückholen und denselben Fehler nicht erneut melden.
        h.view.focus(id,true); id->set_text(L"bad id!"); id->on_change();
        int toasts=h.toasts; h.view.focus(grid,true);
        CHECK(h.view.focused()==grid); CHECK(GetFocus()==h.view.hwnd()); CHECK(id->invalid); CHECK(h.toasts==toasts+1);
        id->on_commit(); CHECK(h.toasts==toasts+1); CHECK(h.view.focused()==grid);
        id->set_text(L"one"); id->on_commit(); CHECK(!id->invalid);
        // Sekundenfehler beim Fokuswechsel und wiederholten Commit.
        blocks->selected=0; blocks->on_select(0);
        auto cycle=h.find<Segment>(L"Wechsel"); cycle->index=1; cycle->on_change();
        auto seconds=h.find<TextField>(L"Sekunden (rotate, 3–600)");
        h.view.focus(seconds,true); seconds->set_text(L"1"); seconds->on_change();
        toasts=h.toasts; h.view.focus(grid,true);
        CHECK(seconds->invalid); CHECK(h.toasts==toasts+1); CHECK(h.view.focused()==grid);
        seconds->on_commit(); CHECK(h.toasts==toasts+1); CHECK(GetFocus()==h.view.hwnd());
        seconds->set_text(L"3"); seconds->on_commit(); CHECK(!seconds->invalid);
        auto path=h.find<Combo>(L"Anzeigen, wenn"); auto op=h.find<Segment>(L"Vergleich");
        auto type=h.find<Segment>(L"Werttyp"); auto literal=h.find<TextField>(L"Vergleichswert");
        path->index=1; op->index=1; type->index=1;
        h.view.focus(literal,true); literal->set_text(L"no number"); literal->on_change();
        toasts=h.toasts; h.view.focus(grid,true);
        CHECK(literal->invalid); CHECK(h.toasts==toasts+1); CHECK(h.view.focused()==grid);
        literal->on_commit(); CHECK(h.toasts==toasts+1); CHECK(GetFocus()==h.view.hwnd());
        literal->set_text(L"1"); literal->on_commit(); CHECK(!literal->invalid);
        while(h.find<Button>(L"Rückgängig")->enabled) grid->input('Z',false,false,true);
        CHECK(plugins::edit::equal(grid->model,original));
        // Tab erreicht alle fokussierbaren Widgets auch bei 200 % DPI und schmalem Inhalt.
        h.view.set_dpi(192); SetWindowPos(h.view.hwnd(),nullptr,0,0,800,800,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); h.relayout();
        auto focusable=h.view.flatten(true); h.view.focus_edge(true);
        for(size_t i=0;i<focusable.size();++i) { auto f=h.view.focused(); CHECK(f && f->rc.bottom>f->rc.top); h.view.tab(true); }
        CHECK(grid->rc.right-grid->rc.left>=96); CHECK((grid->rc.bottom-grid->rc.top)/3>=32);
        // Historie gehört der Seite und überlebt vollständig zerstörte/rekonstruierte Widgets.
        h.view.focus(grid,true); key(h.view,VK_RIGHT,false,true);
        page->hidden(h); h.view.set_root(nullptr); h.root.clear(); flush_retired();
        page->build(h,h.root); h.view.set_root(&h.root); page->shown(h); h.drain(); page->tick(h,h.state); h.drain(); page->tick(h,h.state);
        grid=h.find<GridEditor>(); CHECK(grid->model.layout["zones"][0]["column"]==1);
        h.view.focus(grid,true); key(h.view,'Z',false,false,true); CHECK(plugins::edit::equal(grid->model,original));
        // Übernehmen: echter Store mit Backup, kein Konfigurations-/Manifest-Schreibweg.
        auto manifest_path=dir/"editorplugins/editor-test.flplugin.json"; auto before=read_file(manifest_path);
        h.view.focus(grid,true); grid->input(VK_RIGHT,false,true);
        auto apply=h.find<Button>(L"Übernehmen"); CHECK(apply->enabled); h.view.focus(apply,true); apply->on_click(); CHECK(h.view.focused()==grid); h.drain();
        CHECK(h.find<GridEditor>()==grid); CHECK(h.view.focused()==grid);
        page->tick(h,h.state); h.drain(); page->tick(h,h.state);
        CHECK(h.applies==1); CHECK(fs::exists(dir/"editorplugins/editor-test.layout.json")); CHECK(before==read_file(manifest_path));
        grid=h.find<GridEditor>(); grid->input(VK_DOWN,false,true); h.find<Button>(L"Übernehmen")->on_click(); h.drain();
        CHECK(h.applies==2); CHECK(fs::exists(dir/"editorplugins/editor-test.layout.json.bak")); CHECK(count_dirty(&h.root)==0);
        page->tick(h,h.state); h.drain(); CHECK(h.image && h.image->w==480 && h.image->h==320);
        // Reset: Standardantwort Nein lässt Entwurf/Fokus, Ja synchronisiert ohne Widget-Neuaufbau.
        auto reset=h.find<Button>(L"Auf Manifest zurücksetzen");
        h.view.focus(reset,true); auto saved=grid->model;
        h.confirm_answer=false; reset->on_click(); CHECK(plugins::edit::equal(grid->model,saved)); CHECK(h.view.focused()==reset);
        CHECK(h.question.de.find(L"Backup")!=std::wstring::npos); CHECK(h.question.en.find(L"backup")!=std::wstring::npos);
        h.confirm_answer=true; reset->on_click(); h.drain();
        CHECK(h.find<GridEditor>()==grid); CHECK(h.view.focused()==grid); CHECK(plugins::edit::equal(grid->model,original));
        CHECK(!fs::exists(dir/"editorplugins/editor-test.layout.json"));
        // Beim Speichern weiterbearbeitete Modelle dürfen von der Fortsetzung nicht überschrieben werden.
        grid->input(VK_RIGHT,false,true); h.delayed=true;
        h.find<Button>(L"Übernehmen")->on_click(); h.drain(); CHECK(h.busy());
        grid->input(VK_DOWN,false,true); auto latest=grid->model;
        h.complete(); CHECK(plugins::edit::equal(grid->model,latest)); CHECK(h.find<Button>(L"Rückgängig")->enabled);
        h.delayed=false;
        // Pluginwechsel mit ungesicherten Änderungen: Nein/Ja, Texte in beiden Sprachen.
        auto other=fixture(); other["id"]="other-editor"; other["name"]="Other";
        CHECK(h.store.import_manifest(other.dump()));
        page->hidden(h); page->shown(h); h.drain();
        grid=h.find<GridEditor>(); CHECK(plugins::edit::equal(grid->model,latest));
        auto choice=h.find<Combo>(L"Aktiviertes Plugin"); int previous=choice->index; int target=1-previous;
        h.view.focus(choice,true); h.confirm_answer=false; int asks=h.confirmations;
        choice->index=target; choice->on_change(); CHECK(h.confirmations==asks+1); CHECK(choice->index==previous);
        CHECK(h.find<GridEditor>()==grid); CHECK(plugins::edit::equal(grid->model,latest));
        CHECK(h.question.de.find(L"verwerfen")!=std::wstring::npos); CHECK(h.question.en.find(L"Discard")!=std::wstring::npos);
        h.kit.english=true; h.confirm_answer=true; choice->index=target; choice->on_change();
        CHECK(h.confirmations==asks+2); CHECK(choice->index==target); CHECK(!h.find<Button>(L"Rückgängig")->enabled);
        CHECK(h.view.focused()==choice); h.kit.english=false;
        CHECK(h.store.set_enabled("other-editor",false));
        auto image=h.image; page->hidden(h); CHECK(!h.image);
        // Eine bereits gestartete Vorschau darf nach hidden() kein Bild zurückbringen.
        page->shown(h); h.drain(); page->tick(h,h.state); page->hidden(h); h.drain(); CHECK(!h.image);
        h.view.set_root(nullptr); h.root.clear(); flush_retired();
        // Defekter/leerer Overlay: kein clamp(0,-1), kein Zugriff auf Block -1.
        auto empty=make_layout_page(); empty->build(h,h.root); h.view.set_root(&h.root); empty->shown(h); h.drain();
        auto empty_grid=h.find<GridEditor>(); auto empty_model=empty_grid->model; empty_model.blocks=Json::array();
        empty_grid->on_edit(empty_model,{});
        CHECK(h.find<LayoutBlockList>() && h.find<LayoutBlockList>()->selected==-1);
        CHECK(!h.find<Combo>(L"Zone des Blocks")->enabled); CHECK(!h.find<Button>(L"Übernehmen")->enabled);
        h.view.set_root(nullptr); h.root.clear(); flush_retired();
        // v1 zeigt nur den Hinweis, keine Editier-Steuerelemente.
        auto v1=fixture(); v1["schema_version"]=1; v1.erase("layout"); v1["id"]="legacy"; v1.erase("settings");
        for(auto& w:v1["widgets"]) { w.erase("id"); w.erase("zone"); }
        CHECK(h.store.import_manifest(v1.dump())); CHECK(h.store.set_enabled("editor-test",false));
        auto legacy=make_layout_page(); legacy->build(h,h.root); h.view.set_root(&h.root); legacy->shown(h); h.drain();
        CHECK(!h.find<GridEditor>()); bool hint=false; for(auto w:h.view.flatten(false)) if(w->value(false)==L"Anordnung braucht Manifest v2.") hint=true; CHECK(hint);
        h.view.set_root(nullptr);
    }
    // Eigene Editor-Snapshots; Plugin-/Built-in-Hashdateien werden nie beschrieben.
    const fs::path baseline=FL_LAYOUT_SNAPSHOT_DIR;
    const bool update=argc==2 && std::string(argv[1])=="update";
    std::map<std::string,std::string> expected,actual;
    if(!update) { std::ifstream in(baseline/"hashes.txt"); CHECK(bool(in)); for(std::string name,hash;in>>name>>hash;) expected[name]=hash; }
    Kit kit;
    for(auto& tc:snapshot::theme_cases()) {
        if(tc.name=="dunkel" || tc.name=="custom") continue;
        kit.dark=false; ojson dp={{"theme",tc.name}}; if(!tc.inline_theme.is_null()) dp["theme_inline"]=tc.inline_theme;
        kit.use_design(dp);
        if(tc.name=="windows") { theme::WindowsSettings w; w.has_accent=true; kit.use_windows(true,&w); }
        else kit.use_windows(false);
        for(bool portrait:{false,true}) {
            auto image=grid_image(kit,portrait?240:360,portrait?360:240);
            auto key=tc.name+(portrait?".portrait":".landscape"); actual[key]=snapshot::hash_image(image);
            if(!update) CHECK(expected[key]==actual[key]);
            fs::create_directories(dir/"snapshots"); snapshot::write_png(dir/"snapshots"/(key+".png"),image);
        }
    }
    // Schmaler 6-Spalten-Ausschnitt folgt dem Tastaturfokus, ohne außerhalb zu zeichnen.
    auto narrow=plugins::arrangement_from(*plugins::parse_manifest(fixture().dump()).manifest);
    for(int i=0;i<3;++i) plugins::edit::resize_axis(narrow,true,1);
    GridEditor probe; probe.model=narrow;
    for(int i=0;i<5;++i) probe.input(VK_RIGHT);
    CHECK(probe.column==5); CHECK(plugins::edit::geometry(probe.model)->columns.size()==6);
    kit.use_design({{"theme","hell"}}); kit.use_windows(false); int first=0;
    auto clipped=grid_image(kit,104,180,&narrow,5,&first); CHECK(first==4);
    bool margin=true; for(int y=0;y<clipped.h;++y) for(int x=0;x<clipped.w;++x)
        if(x<8 || x>=96 || y<8 || y>=172) margin=margin && clipped.at(x,y)==clipped.at(0,0);
    CHECK(margin); actual["hell.narrow"]=snapshot::hash_image(clipped);
    if(!update) CHECK(expected["hell.narrow"]==actual["hell.narrow"]);
    if(update) { fs::create_directories(baseline); std::ofstream out(baseline/"hashes.txt"); for(auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n'; CHECK(bool(out)); }
    else CHECK(expected.size()==actual.size());
    std::error_code ec; fs::remove_all(dir,ec); CHECK(!ec && !fs::exists(dir));
    std::printf("Layout UI: %d passed, %d failed\n",passed,failed); return failed?1:0;
}
