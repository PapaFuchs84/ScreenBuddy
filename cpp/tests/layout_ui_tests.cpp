// © 2026 FuchsLabs – isolierte native Editorprüfung, keine App-/Netzwerk-Schleife.
#include "ui_pages.h"
#include "ui_layout_grid.h"
#include "native_settings.h"
#include "app.h"
#include "util.h"
#include "snapshot_support.h"
#include "page_frame.h"
#include "page_samples.h"
#include <oleacc.h>
#include <wincodec.h>
#include <deque>
#include <fstream>
#include <sstream>
#include <cmath>
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
class CaptureWidget : public Widget {
public:
    int downs=0, moves=0, ups=0, cancels=0; bool auto_scroll=false;
    bool autoscroll() const override { return auto_scroll; }
    float height(Ctx&,float) override { return 64; }
    bool clickable() const override { return true; }
    void press(Point,Surface&) override { ++downs; }
    void drag(Point,Surface&) override { ++moves; }
    void release(Point,Surface&) override { ++ups; }
    void cancel(Surface&) override { ++cancels; }
};
static void capture_tests() {
    Kit kit; Surface surface(kit,nullptr,L"Capture-Test",false); Column root;
    auto w=root.add<CaptureWidget>(); surface.set_root(&root); SetWindowPos(surface.hwnd(),nullptr,0,0,400,200,SWP_NOZORDER|SWP_NOACTIVATE); surface.set_dpi(96); surface.relayout();
    const auto point=MAKELPARAM(int(w->rc.left+10),int(w->rc.top+10));
    auto send=[&](UINT message,WPARAM key=0) { SendMessageW(surface.hwnd(),message,key,point); };
    CHECK(w->cursor({})==IDC_HAND); Widget plain; CHECK(plain.cursor({})==IDC_ARROW);
    send(WM_LBUTTONDOWN); send(WM_MOUSEMOVE); send(WM_CAPTURECHANGED); send(WM_CAPTURECHANGED);
    CHECK(w->downs==1); CHECK(w->moves>=1); CHECK(w->cancels==1); CHECK(!w->pressed);
    send(WM_LBUTTONUP); CHECK(w->ups==0);
    send(WM_LBUTTONDOWN); send(WM_LBUTTONUP); CHECK(w->ups==1); CHECK(w->cancels==1);
    send(WM_LBUTTONDOWN); send(WM_KEYDOWN,VK_ESCAPE); send(WM_LBUTTONUP);
    CHECK(w->cancels==2); CHECK(w->ups==1);
    send(WM_LBUTTONDOWN); send(WM_KILLFOCUS); send(WM_LBUTTONUP);
    CHECK(w->cancels==3); CHECK(w->ups==1);
    // Rand-Bildlauf per Timer: nur für Widgets, die autoscroll() melden, nur mit Capture.
    constexpr WPARAM autoscroll_timer=0x7A12;
    send(WM_LBUTTONDOWN); int before=w->moves; SendMessageW(surface.hwnd(),WM_TIMER,autoscroll_timer,0); CHECK(w->moves==before);
    w->auto_scroll=true; SendMessageW(surface.hwnd(),WM_TIMER,autoscroll_timer,0); CHECK(w->moves==before+1);
    send(WM_LBUTTONUP); before=w->moves; SendMessageW(surface.hwnd(),WM_TIMER,autoscroll_timer,0); CHECK(w->moves==before);
    surface.set_root(nullptr);
}
static void key(Surface&,UINT,bool,bool,bool);
static Point middle(Rect r) { return {(r.left+r.right)/2,(r.top+r.bottom)/2}; }
static void gesture_tests() {
    auto original=plugins::arrangement_from(*plugins::parse_manifest(fixture().dump()).manifest);
    original.blocks.push_back({{"id","c"},{"zone","one"},{"shown",true}}); // zweiter Block: Zone "one" darf einen abgeben
    GridEditor g; g.rc={0,0,600,600}; plugins::edit::EditHistory history; int edits=0,rejects=0;
    auto reset=[&] { g.pointer(GridEditor::Kind::Cancel,{}); g.model=original; g.column=g.row=g.selected=0; g.area=1; g.first_column=0; g.focused_block=-1; g.palette_block=0; history.reset(original); edits=rejects=0; };
    g.on_edit=[&](auto m,auto group) { CHECK(group.empty()); ++edits; history.push(m); };
    g.on_reject=[&] { ++rejects; };
    auto point=[&](int x,int y) { auto r=g.grid_rect(); return Point{r.left+(x+.5f)*(r.right-r.left)/3,r.top+(y+.5f)*(r.bottom-r.top)/3}; };
    auto title=[&] { auto r=g.zone_rect(0); return Point{r.left+30,r.top+12}; };
    auto drag=[&](Point a,Point b) { g.pointer(GridEditor::Kind::Down,a); g.pointer(GridEditor::Kind::Move,b); g.pointer(GridEditor::Kind::Up,b); };
    reset(); auto start=title(), end=start; end.x+=200;
    g.pointer(GridEditor::Kind::Down,start);
    g.pointer(GridEditor::Kind::Move,{end.x-10,end.y}); g.pointer(GridEditor::Kind::Move,end);
    CHECK(g.carrying()); CHECK(g.target_valid()); CHECK(edits==0); CHECK(plugins::edit::equal(g.model,original));
    CHECK(g.value(false).find(L"aufgenommen")!=std::wstring::npos); CHECK(g.value(true).find(L"valid")!=std::wstring::npos);
    g.pointer(GridEditor::Kind::Up,end); CHECK(edits==1); CHECK(history.steps()==1);
    CHECK(g.model.layout["zones"][0]["column"]==1); CHECK(history.undo()); CHECK(plugins::edit::equal(history.current(),original)); CHECK(history.redo());
    reset(); drag(start,{start.x+1,start.y+1}); CHECK(edits==0); CHECK(!g.carrying());
    reset(); g.pointer(GridEditor::Kind::Down,start); g.pointer(GridEditor::Kind::Move,end); g.pointer(GridEditor::Kind::Cancel,{});
    CHECK(!g.carrying()); CHECK(edits==0); CHECK(plugins::edit::equal(g.model,original));
    reset(); g.pointer(GridEditor::Kind::Down,start); g.pointer(GridEditor::Kind::Move,end); g.input(VK_ESCAPE); g.pointer(GridEditor::Kind::Up,end);
    CHECK(edits==0); CHECK(plugins::edit::equal(g.model,original));
    reset(); drag(start,{start.x+400,start.y+(g.grid_rect().bottom-g.grid_rect().top)*2/3});
    CHECK(edits==0); CHECK(rejects==1); CHECK(plugins::edit::equal(g.model,original));
    reset(); auto r=g.zone_rect(0); auto edge=Point{r.right,(r.top+r.bottom)/2};
    CHECK(g.cursor(edge)==IDC_SIZEWE); drag(edge,{edge.x+200,edge.y});
    CHECK(edits==1); CHECK(g.model.layout["zones"][0]["column_span"]==2);
    reset(); r=g.zone_rect(0); edge={r.right,r.bottom}; CHECK(g.cursor(edge)==IDC_SIZENWSE);
    drag(edge,{edge.x+200,edge.y+(g.grid_rect().bottom-g.grid_rect().top)/3});
    CHECK(edits==1); CHECK(g.model.layout["zones"][0]["row_span"]==2);
    reset(); r=g.zone_rect(0); edge={r.right,r.bottom};
    g.pointer(GridEditor::Kind::Down,edge); g.pointer(GridEditor::Kind::Move,{-20,-20}); CHECK(!g.target_valid()); CHECK(g.cursor({})==IDC_NO);
    g.pointer(GridEditor::Kind::Up,{-20,-20}); CHECK(edits==0); CHECK(rejects==0); CHECK(!g.carrying()); // außerhalb: stiller Abbruch
    reset(); auto chip=middle(g.palette_rect(0)); drag(chip,point(1,1));
    CHECK(edits==1); CHECK(g.model.layout["zones"].size()==3); CHECK(history.steps()==1); auto placed=g.model;
    CHECK(history.undo()); CHECK(plugins::edit::equal(history.current(),original));
    reset(); g.input(VK_F6,true); CHECK(g.area==0); g.input(VK_SPACE); CHECK(g.carrying());
    g.input(VK_RIGHT); g.input(VK_DOWN); CHECK(edits==0); g.input(VK_RETURN);
    CHECK(edits==1); CHECK(plugins::edit::equal(g.model,placed)); CHECK(!g.carrying());
    reset(); drag(middle(g.block_rect(0)),middle(g.trash_rect())); CHECK(edits==1); CHECK(!g.model.blocks[0]["shown"].get<bool>());
    CHECK(g.model.blocks.size()==3); CHECK(g.model.layout["zones"].size()==2);
    reset(); drag(start,middle(g.trash_rect())); CHECK(edits==1); CHECK(g.model.layout["zones"].size()==1); CHECK(!g.model.blocks[0]["shown"].get<bool>());
    reset(); g.input(VK_F6,true); g.input(VK_DELETE); CHECK(edits==1); CHECK(!g.model.blocks[0]["shown"].get<bool>());
    reset(); g.input(VK_SPACE); g.input(VK_RIGHT); g.input(VK_RETURN); CHECK(edits==1); CHECK(g.model.layout["zones"][0]["column"]==1);
    reset(); g.input(VK_SPACE); g.input(VK_RIGHT); g.input(VK_ESCAPE); CHECK(edits==0); CHECK(plugins::edit::equal(g.model,original));
    reset(); g.input(VK_F6,true); g.input(VK_SPACE); g.input(VK_F6); CHECK(g.area==2); g.input(VK_RETURN); CHECK(edits==1); CHECK(!g.model.blocks[0]["shown"].get<bool>());
    reset(); drag(middle(g.block_rect(0)),point(2,2)); CHECK(edits==1); CHECK(g.model.blocks[0]["zone"]=="two");
    reset(); g.input(VK_F6,true); g.input(VK_SPACE); g.input(VK_RIGHT); g.input(VK_RIGHT); g.input(VK_DOWN); g.input(VK_DOWN); g.input(VK_DOWN,false,false,true); g.input(VK_RETURN);
    CHECK(edits==1); CHECK(g.model.blocks[1]["id"]=="a"); // Einfügen hinter b
    reset(); g.input(VK_F6,true); g.input(VK_SPACE); g.input(VK_F6,false,false,true); CHECK(g.area==2); g.input(VK_ESCAPE); CHECK(edits==0);
    // Befund 1: Abbruch stellt die Auswahl vor dem Drücken wieder her (kein Wechsel auf die gedrückte Zone).
    reset(); auto r2=g.zone_rect(1); Point s2{r2.left+30,r2.top+12};
    g.pointer(GridEditor::Kind::Down,s2); CHECK(g.selected==1); g.pointer(GridEditor::Kind::Move,{s2.x-200,s2.y}); CHECK(g.carrying());
    g.pointer(GridEditor::Kind::Cancel,{}); CHECK(g.selected==0); CHECK(edits==0); CHECK(!g.carrying());
    g.pointer(GridEditor::Kind::Down,s2); g.pointer(GridEditor::Kind::Move,{s2.x-200,s2.y}); CHECK(g.input(VK_ESCAPE)); CHECK(g.selected==0); CHECK(edits==0);
    g.pointer(GridEditor::Kind::Down,s2); CHECK(g.selected==1); g.pointer(GridEditor::Kind::Cancel,{}); CHECK(g.selected==0); // auch ohne Ziehen
    reset(); g.pointer(GridEditor::Kind::Down,s2); g.pointer(GridEditor::Kind::Move,point(0,0)); CHECK(!g.target_valid()); // überlappt Zone one
    g.pointer(GridEditor::Kind::Up,point(0,0)); CHECK(rejects==1); CHECK(g.selected==0); CHECK(edits==0);
    reset(); CHECK(!g.input(VK_ESCAPE)); // Befund 4: Esc ohne Geste gehört dem Aufrufer
    // Befund 2: Ziel, das eine Zone ohne gezeigten Block hinterlässt, passt nicht.
    reset(); g.model.blocks[2]["shown"]=false; auto sole=g.model; // Zone "one" zeigt nur noch a
    g.pointer(GridEditor::Kind::Down,middle(g.block_rect(0))); g.pointer(GridEditor::Kind::Move,point(1,1));
    CHECK(g.carrying()); CHECK(!g.target_valid()); CHECK(g.cursor(point(1,1))==IDC_NO); CHECK(g.value(false).find(L"passt nicht")!=std::wstring::npos);
    g.pointer(GridEditor::Kind::Up,point(1,1)); CHECK(edits==0); CHECK(rejects==1); CHECK(plugins::edit::equal(g.model,sole)); CHECK(g.selected==0);
    g.pointer(GridEditor::Kind::Down,middle(g.block_rect(0))); g.pointer(GridEditor::Kind::Move,point(2,2)); CHECK(!g.target_valid());
    g.pointer(GridEditor::Kind::Move,middle(g.trash_rect())); CHECK(!g.target_valid()); g.pointer(GridEditor::Kind::Cancel,{});
    g.pointer(GridEditor::Kind::Down,middle(g.block_rect(0))); g.pointer(GridEditor::Kind::Move,{middle(g.block_rect(0)).x+2,middle(g.block_rect(0)).y+30});
    g.pointer(GridEditor::Kind::Cancel,{});
    reset(); drag(middle(g.block_rect(0)),point(1,1)); CHECK(edits==1); // mit zweitem Block gültig
    reset(); g.model.blocks[2]["shown"]=false; g.input(VK_F6,true); CHECK(g.input(VK_DELETE)); CHECK(edits==0); CHECK(rejects==1); // Entf in der Leiste: gleiche Regel
    // Befund 3: Block-Chips per Tastatur.
    reset(); CHECK(g.focused_block==-1);
    CHECK(g.input(VK_DOWN,false,false,true)); CHECK(g.focused_block==0); CHECK(g.carrying()==false);
    CHECK(g.value(false).find(L"Block-Chip a gewählt")!=std::wstring::npos); CHECK(g.value(true).find(L"block chip a selected")!=std::wstring::npos);
    CHECK(g.input(VK_DOWN)); CHECK(g.focused_block==2); CHECK(g.input(VK_DOWN)); CHECK(g.focused_block==2);
    CHECK(g.input(VK_UP)); CHECK(g.focused_block==0); CHECK(g.input(VK_UP)); CHECK(g.focused_block==-1); CHECK(g.row==0);
    CHECK(g.input(VK_UP,false,false,true)); CHECK(g.focused_block==2); // Strg+Hoch: letzter Chip
    CHECK(g.input(VK_RIGHT)); CHECK(g.focused_block==-1); CHECK(g.column==1);
    reset(); g.input(VK_DOWN,false,false,true); CHECK(g.input(VK_SPACE)); CHECK(g.carrying());
    CHECK(g.value(false).find(L"Block a aufgenommen")!=std::wstring::npos);
    g.input(VK_RIGHT); g.input(VK_RIGHT); g.input(VK_DOWN); g.input(VK_DOWN); CHECK(edits==0); CHECK(g.target_valid()); g.input(VK_RETURN);
    CHECK(edits==1); CHECK(!g.carrying()); { size_t at=0; for(size_t i=0;i<g.model.blocks.size();++i) if(g.model.blocks[i]["id"]=="a") at=i;
        CHECK(g.model.blocks[at]["zone"]=="two"); CHECK(g.selected==1); CHECK(g.focused_block==int(at)); }
    reset(); g.input(VK_DOWN,false,false,true); g.input(VK_SPACE); g.input(VK_RIGHT); CHECK(g.input(VK_ESCAPE)); CHECK(!g.carrying()); CHECK(edits==0); CHECK(plugins::edit::equal(g.model,original));
    reset(); g.input(VK_DOWN,false,false,true); CHECK(g.input(VK_DELETE)); CHECK(edits==1); CHECK(!g.model.blocks[0]["shown"].get<bool>()); CHECK(g.model.blocks[2]["shown"].get<bool>()); CHECK(g.focused_block==-1);
    reset(); g.model.blocks[2]["shown"]=false; g.input(VK_DOWN,false,false,true); CHECK(g.focused_block==0); g.input(VK_DELETE); CHECK(edits==0); CHECK(rejects==1); CHECK(g.model.blocks[0]["shown"].get<bool>());
    // Befund 4: Loslassen außerhalb des Rasters bricht still ab; freie Zelle ziehen legt nichts an; Auswahl folgt dem Objekt.
    reset(); g.pointer(GridEditor::Kind::Down,start); g.pointer(GridEditor::Kind::Move,end); g.pointer(GridEditor::Kind::Move,{300,g.grid_rect().top-6});
    CHECK(!g.target_valid()); g.pointer(GridEditor::Kind::Up,{300,g.grid_rect().top-6}); CHECK(edits==0); CHECK(rejects==0); CHECK(!g.carrying()); CHECK(g.selected==0);
    reset(); g.pointer(GridEditor::Kind::Down,middle(g.palette_rect(0))); g.pointer(GridEditor::Kind::Move,{-40,-40}); g.pointer(GridEditor::Kind::Up,{-40,-40}); CHECK(edits==0); CHECK(rejects==0);
    reset(); g.pointer(GridEditor::Kind::Down,point(1,0)); g.pointer(GridEditor::Kind::Move,point(1,1)); g.pointer(GridEditor::Kind::Up,point(1,1));
    CHECK(edits==0); CHECK(g.model.layout["zones"].size()==2); CHECK(g.selected==0); // weder Druck- noch Loslasszelle
    reset(); g.pointer(GridEditor::Kind::Down,point(1,0)); g.pointer(GridEditor::Kind::Up,point(1,0)); CHECK(edits==1); CHECK(g.model.layout["zones"].size()==3); // Klick legt weiter an
    reset(); { auto tr=g.trash_rect(); drag(middle(g.block_rect(2)),{tr.right-5,(tr.top+tr.bottom)/2}); CHECK(edits==1); CHECK(g.selected==0); CHECK(!g.model.blocks[2]["shown"].get<bool>()); }
    reset(); r=g.zone_rect(0); drag({r.left+30,r.top+12},point(1,0)); CHECK(edits==1); CHECK(g.selected==0); CHECK(g.model.layout["zones"][0]["column"]==1);
    reset(); drag(middle(g.block_rect(0)),point(2,2)); CHECK(g.selected==1); // Block folgt in seine Zone
    {   // leeres Inventar: kein std::clamp mit hi<lo
        GridEditor e; e.model=original; e.model.blocks=Json::array(); e.input(VK_F6,true); CHECK(e.area==0);
        CHECK(e.input(VK_RIGHT)); CHECK(e.input(VK_DOWN)); CHECK(!e.input(VK_SPACE)); CHECK(!e.input(VK_DELETE)); CHECK(!e.carrying()); CHECK(e.palette_block==0);
        e.palette_block=5; e.input(VK_RIGHT); CHECK(e.palette_block==0);
    }
    // Echte WM_LBUTTON*/MOUSEMOVE-Nachrichten im isolierten Test-Surface, auch bei 200 % DPI.
    for(UINT dpi : {96u,192u}) {
        Kit kit; Surface surface(kit,nullptr,L"Drag-Test",true); Column root; auto grid=root.add<GridEditor>(); grid->model=original;
        surface.set_root(&root); SetWindowPos(surface.hwnd(),nullptr,0,0,1000,1000,SWP_NOZORDER|SWP_NOACTIVATE); surface.set_dpi(dpi); surface.relayout();
        grid->on_edit=[&](auto,auto group) { CHECK(group.empty()); ++edits; }; edits=0;
        auto zr=grid->zone_rect(0); Point a{zr.left+24,zr.top+12};
        auto gr=grid->grid_rect(); Point b{a.x+(gr.right-gr.left)/3,a.y};
        auto send=[&](UINT msg,Point p) { SendMessageW(surface.hwnd(),msg,msg==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(surface.px(p.x),surface.px(p.y-surface.scroll()))); };
        send(WM_LBUTTONDOWN,a); send(WM_MOUSEMOVE,b); CHECK(edits==0); CHECK(grid->carrying());
        { // Befund 6: VALUECHANGE nur bei Zielwechsel, nicht bei jeder Mausbewegung in derselben Zelle.
            auto events=grid->value_events;
            send(WM_MOUSEMOVE,{b.x+1,b.y}); send(WM_MOUSEMOVE,{b.x,b.y+1}); send(WM_MOUSEMOVE,{b.x+2,b.y+2}); CHECK(grid->value_events==events);
            send(WM_MOUSEMOVE,{a.x,a.y}); CHECK(grid->value_events==events+1);
            send(WM_MOUSEMOVE,b); CHECK(grid->value_events==events+2);
        }
        send(WM_LBUTTONUP,b); CHECK(edits==1); CHECK(!grid->carrying()); CHECK(grid->model.layout["zones"][0]["column"]==1);
        grid->model=original; edits=0; send(WM_LBUTTONDOWN,a); send(WM_MOUSEMOVE,b); SendMessageW(surface.hwnd(),WM_CAPTURECHANGED,0,0); send(WM_LBUTTONUP,b);
        CHECK(edits==0); CHECK(!grid->carrying()); CHECK(plugins::edit::equal(grid->model,original));
        send(WM_LBUTTONDOWN,a); send(WM_MOUSEMOVE,b); key(surface,VK_ESCAPE,false,false,false); send(WM_LBUTTONUP,b);
        CHECK(edits==0); CHECK(!grid->carrying());
        grid->column=grid->row=0; key(surface,VK_SPACE,false,false,false); CHECK(grid->carrying()); surface.focus(nullptr); CHECK(!grid->carrying()); CHECK(edits==0);
        surface.set_root(nullptr);
    }
}
static std::wstring read_file(const fs::path& p) {
    std::ifstream f(p,std::ios::binary); return widen(std::string((std::istreambuf_iterator<char>(f)),{}));
}
static void msaa(TestHost& h,GridEditor* grid) {
    ComPtr<IAccessible> acc;
    CHECK(SUCCEEDED(AccessibleObjectFromWindow(h.view.hwnd(),OBJID_CLIENT,IID_PPV_ARGS(acc.GetAddressOf()))));
    if(!acc) return;
    long count=0; acc->get_accChildCount(&count); bool found=false,palette=false;
    for(long i=1;i<=count;++i) {
        VARIANT child{}; child.vt=VT_I4; child.lVal=i; BSTR name=nullptr;
        acc->get_accName(child,&name);
        if(name && std::wstring(name)==L"Bausteinleiste") {
            palette=true; VARIANT role{}; acc->get_accRole(child,&role); CHECK(role.lVal==ROLE_SYSTEM_LIST);
            BSTR value=nullptr; acc->get_accValue(child,&value); CHECK(value && std::wstring(value).find(L"a gezeigt")!=std::wstring::npos); SysFreeString(value);
        }
        if(name && std::wstring(name)==L"Anordnungsraster") {
            found=true; VARIANT role{}; acc->get_accRole(child,&role); CHECK(role.lVal==ROLE_SYSTEM_LIST);
            BSTR val=nullptr; acc->get_accValue(child,&val); CHECK(val && std::wstring(val)==grid->value(false)); SysFreeString(val);
            CHECK(SUCCEEDED(acc->accSelect(SELFLAG_TAKEFOCUS,child)));
        }
        SysFreeString(name);
    }
    CHECK(found); CHECK(palette);
}
static void key(Surface& s,UINT vk,bool shift=false,bool alt=false,bool control=false) {
    BYTE saved[256]{},state[256]{}; GetKeyboardState(saved);
    state[VK_SHIFT]=shift?0x80:0; state[VK_MENU]=alt?0x80:0; state[VK_CONTROL]=control?0x80:0;
    SetKeyboardState(state);
    SendMessageW(s.hwnd(),alt?WM_SYSKEYDOWN:WM_KEYDOWN,vk,0);
    SetKeyboardState(saved); // nur Zustand des eigenen Testthreads, keine globalen Eingaben
}
static Image grid_image(Kit& kit,int width,int height,const plugins::Arrangement* model=nullptr,int column=0,int* first_column=nullptr,int state=0) {
    ComPtr<IWICImagingFactory> wic; CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(wic.GetAddressOf()));
    ComPtr<IWICBitmap> bitmap; wic->CreateBitmap(width,height,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,bitmap.GetAddressOf());
    ComPtr<ID2D1RenderTarget> rt;
    kit.d2d()->CreateWicBitmapRenderTarget(bitmap.Get(),D2D1::RenderTargetProperties(),rt.GetAddressOf());
    ComPtr<ID2D1SolidColorBrush> brush; rt->CreateSolidColorBrush(kit.theme().text,brush.GetAddressOf());
    Ctx ctx{rt.Get(),kit,kit.theme(),brush.Get()};
    auto base=plugins::parse_manifest(fixture().dump()); GridEditor grid; grid.model=model?*model:plugins::arrangement_from(*base.manifest); grid.column=column;
    // Zustände 1 und 3 zeigen gültige Block-Ablagen: Zone "one" braucht dafür einen zweiten Block (Zustand 5 zeigt den ungültigen Fall).
    if(!model && (state==1 || state==3)) grid.model.blocks.push_back({{"id","c"},{"zone","one"},{"shown",true}});
    grid.rc={8,8,float(width-8),float(height-8)};
    if(state) {
        auto source=middle(grid.palette_rect(0)); auto gr=grid.grid_rect();
        Point target{gr.left+(gr.right-gr.left)/2,gr.top+(gr.bottom-gr.top)/2};
        if(state==2) { auto z=grid.zone_rect(0); source={z.left+20,z.top+12}; target={source.x+(gr.right-gr.left)*2/3,source.y+(gr.bottom-gr.top)*2/3}; }
        if(state==3 || state==5) target=middle(grid.trash_rect());
        if(state==4) { auto z=grid.zone_rect(0); source={z.right,z.bottom}; target={source.x+(gr.right-gr.left)/3,source.y}; }
        grid.pointer(GridEditor::Kind::Down,source); grid.pointer(GridEditor::Kind::Move,target);
        CHECK(grid.carrying()); CHECK(grid.target_valid()==(state!=2 && state!=5));
    }
    rt->BeginDraw(); rt->Clear(kit.theme().card); grid.paint(ctx); CHECK(SUCCEEDED(rt->EndDraw()));
    if(first_column) *first_column=grid.first_column;
    Image image(width,height); bitmap->CopyPixels(nullptr,width*4,UINT(image.px.size()*4),reinterpret_cast<BYTE*>(image.px.data())); return image;
}
// P7-6: echter Dateispeicher und veröffentlichte Treffer, ohne App-/Serverstart.
static void builtin_infrastructure_tests(const fs::path& root) {
    const auto directory=root/"builtin-layouts";
    const auto& c=*builtin::find("rss");
    auto standard=builtin::standard(c), changed=standard;
    changed.model.layout["rows"][0]=3;
    builtin::LayoutStore store(directory);
    CHECK(!store.get("rss").overlay); CHECK(store.get("rss").revision==0);
    CHECK(store.apply("../escape",builtin::to_json(changed).dump())!="");
    CHECK(store.apply("rss",builtin::to_json(standard).dump()).empty());
    CHECK(store.get("rss").revision==0); CHECK(!fs::exists(directory/"builtin.rss.layout.json"));
    CHECK(store.apply("rss",builtin::to_json(changed).dump()).empty());
    CHECK(store.get("rss").revision==1); CHECK(store.get("friends").revision==0);
    CHECK(store.apply("rss",builtin::to_json(changed).dump()).empty()); CHECK(store.get("rss").revision==1);
    auto next=changed; next.model.layout["rows"][0]=2;
    CHECK(store.apply("rss",builtin::to_json(next).dump()).empty()); CHECK(store.get("rss").revision==2);
    const auto path=directory/"builtin.rss.layout.json",backup=directory/"builtin.rss.layout.json.bak";
    auto read=[](const fs::path& p){std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
    CHECK(read(backup)==builtin::to_json(changed).dump());
    builtin::LayoutStore loaded(directory); CHECK(loaded.get("rss").overlay); CHECK(loaded.get("rss").state=="ok");
    CHECK(builtin::to_json(*loaded.get("rss").overlay)==builtin::to_json(next));
    // Ein nicht beschreibbares Backup verhindert den Austausch und erhält Speicher und Datei.
    fs::remove(backup); fs::create_directory(backup);
    CHECK(!store.apply("rss",builtin::to_json(changed).dump()).empty());
    CHECK(store.get("rss").revision==2);CHECK(read(path)==builtin::to_json(next).dump());
    CHECK(!store.reset("rss").empty()); CHECK(store.get("rss").overlay);
    fs::remove(backup);
    CHECK(store.apply("rss",builtin::to_json(standard).dump()).empty());
    CHECK(!store.get("rss").overlay);CHECK(!fs::exists(path));CHECK(!fs::exists(backup));
    CHECK(store.get("rss").revision==3);
    {auto stale=builtin::to_json(changed);stale["catalog_version"]=99;std::ofstream(path)<<stale.dump();}
    builtin::LayoutStore stale(directory);CHECK(stale.get("rss").state=="stale");CHECK(!stale.get("rss").overlay);
    {std::ofstream(path)<<"corrupt";}
    builtin::LayoutStore corrupt(directory);CHECK(corrupt.get("rss").state=="invalid");CHECK(!corrupt.get("rss").overlay);
    CHECK(corrupt.reset("rss").empty());
    CHECK(store.apply("friends",builtin::to_json(builtin::standard(*builtin::find("friends"))).dump()).empty());
    CHECK(!store.get("discord").overlay); CHECK(store.view("missing").is_null());
    {std::ofstream(path)<<std::string(16385,' ');}
    builtin::LayoutStore huge(directory);CHECK(huge.get("rss").state=="invalid");CHECK(huge.reset("rss").empty());
    // Statische RSS-Treffer dürfen bei leerer oder anderer Rasterkarte niemals durchfallen.
    touch::Tile tile{{20,30,480,320},true,{{{"https://example.org/old",""}}}};
    tile.arranged=true;
    CHECK(!touch::at({tile},21,70)); CHECK(!touch::action_at({tile},21,70));
    tile.arranged_targets.push_back({{40,50,100,44},{{"https://example.org/drawn",""}},[]{}});
    std::vector<touch::Tile> tiles{tile};
    CHECK(touch::at(tiles,60,80) && touch::at(tiles,60,80)->front().url=="https://example.org/drawn");
    CHECK(touch::action_at(tiles,60,80)); CHECK(!touch::at(tiles,160,80)); CHECK(!touch::at(tiles,59,80));
    CHECK(!touch::at(tiles,60,124)); CHECK(!touch::at(tiles,60,79));
    // Nur der Rasterrahmen isoliert Fehler und stellt verschachtelte Clips wieder her.
    class FaultBlock : public plugins::zones::Block {
    public:
        std::string key="fault"; bool fail=true;
        const std::string& id() const override{return key;}
        plugins::zones::Size min_size(plugins::zones::Density) const override{return {1,1};}
        void draw(Canvas& d,const plugins::zones::Rect&,plugins::zones::Density) const override {
            d.clip_push(0,0,40,40);d.rectangle(0,0,90,90,{255,0,0});if(fail) throw std::runtime_error("test");
            d.rectangle(0,0,90,90,{255,0,0});
        }
    } block;
    render::Rahmen frame{100,100,1,{0,0,0},[](Canvas&){},[](Canvas& d){d.rectangle(90,90,99,99,{0,255,0});}};
    std::vector<render::Platzierung> placements{{&block,{"fault","zone",{10,10,20,20},plugins::zones::Density::regular}}};
    std::vector<std::string> failures;
    auto image=render::draw_arranged(frame,placements,&failures);CHECK(failures.size()==1);CHECK(image.at(15,15)==0xff000000);CHECK(image.at(95,95)==0xff00ff00);
    block.fail=false;failures.clear();image=render::draw_arranged(frame,placements,&failures);
    CHECK(failures.empty());CHECK(image.at(15,15)==0xffff0000);CHECK(image.at(31,15)==0xff000000);CHECK(image.at(95,95)==0xff00ff00);
    App app(offline_config(),root/"preview.toml",nullptr,root/"preview");
    std::string error;
    CHECK(app.page_preview("rss",builtin::to_json(standard),{480,320},error));
    CHECK(!app.page_preview("rss",builtin::to_json(standard),{0,320},error));
    CHECK(!app.page_preview("homeassistant",builtin::to_json(standard),{480,320},error));
    CHECK(app.page_preview("rss",builtin::to_json(changed),{480,320},error));
}
// P7-7: Editor öffnet und übernimmt Standard ohne Dateianlage; Hochformat separat.
static void rss_editor_tests(const fs::path& root) {
    TestHost h(root/"rss-editor");
    h.actions.builtin_layout_get=[&](const std::string& key){return h.application.builtin_layout_get(key);};
    h.actions.builtin_layout_apply=[&](const std::string& key,const Json& overlay){h.application.builtin_layout_apply(key,overlay);};
    h.actions.builtin_layout_reset=[&](const std::string& key){h.application.builtin_layout_reset(key);};
    auto page=make_layout_page();page->build(h,h.root);page->shown(h);h.drain();page->tick(h,h.state);h.drain();page->tick(h,h.state);
    auto grid=h.find<GridEditor>();CHECK(grid);if(!grid) return;
    auto choice=h.find<Combo>(L"Seite");CHECK(choice && choice->options.size()==1);
    CHECK(choice && choice->options[0].de==L"Eingebaut · RSS");
    CHECK(grid->model.blocks[0]["id"]=="rss.liste");CHECK(grid->model.blocks[1]["id"]=="rss.hinweis");
    const auto original=grid->model;
    CHECK(count_dirty(&h.root)==0);CHECK(h.image);
    const auto file=h.dir/"layouts"/"builtin.rss.layout.json";
    CHECK(!fs::exists(file));auto apply=h.find<Button>(L"Übernehmen");CHECK(apply && apply->enabled);
    apply->on_click();h.drain();CHECK(!fs::exists(file));CHECK(h.application.builtin_layout_get("rss")["layout_revision"]==0);
    auto n=grid->model;n.blocks[1]["shown"]=false;grid->on_edit(n,{});
    CHECK(plugins::edit::equal(grid->model,original));CHECK(h.toast_text==L"Pflichtblock kann nicht entfernt werden.");
    n=grid->model;CHECK(plugins::edit::weight(n,false,0,3));grid->on_edit(n,{});
    page->tick(h,h.state);h.drain();CHECK(h.image);CHECK(!fs::exists(file));
    apply->on_click();h.drain();CHECK(fs::exists(file));CHECK(h.application.builtin_layout_get("rss")["layout_revision"]==1);
    auto saved=builtin::parse(*builtin::find("rss"),h.application.builtin_layout_get("rss")["overlay"].dump());CHECK(saved);
    CHECK(saved.overlay->model.layout["rows"][0]==3);CHECK(saved.overlay->portrait->layout["rows"][0]==4);
    auto orientation=h.find<Segment>(L"Ausrichtung");CHECK(orientation);orientation->index=1;orientation->on_change();
    CHECK(grid->model.layout["rows"][0]==4);CHECK(!h.find<Button>(L"Rückgängig")->enabled);
    n=grid->model;CHECK(plugins::edit::weight(n,false,0,2));grid->on_edit(n,{});
    orientation->index=0;orientation->on_change();CHECK(grid->model.layout["rows"][0]==3);
    apply->on_click();h.drain();
    saved=builtin::parse(*builtin::find("rss"),h.application.builtin_layout_get("rss")["overlay"].dump());CHECK(saved);
    CHECK(saved.overlay->model.layout["rows"][0]==3);CHECK(saved.overlay->portrait->layout["rows"][0]==2);
    // Übermäßig verkleinerter Pflichtblock sperrt die Anwendung in beiden Ausrichtungen.
    n=grid->model;n.layout["rows"]={12,1};grid->on_edit(n,{});CHECK(!apply->enabled);
    h.find<Button>(L"Rückgängig")->on_click();CHECK(apply->enabled);
    auto reset=h.find<Button>(L"Auf Standard zurücksetzen");CHECK(reset);h.confirm_answer=true;reset->on_click();h.drain();
    CHECK(!fs::exists(file));CHECK(!fs::exists(fs::path(file.wstring()+L".bak")));
    CHECK(plugins::edit::equal(grid->model,original));
    page->tick(h,h.state);h.drain();CHECK(h.image);
    // Englische Katalogbeschriftungen und kein Eintrag für spätere Seiten (insbesondere HA).
    CHECK(choice->options[0].en==L"Built-in · RSS");CHECK(choice->options.size()==1);
    h.view.set_root(nullptr);
}
int main(int argc,char** argv) {
    gfx_init(); SetConsoleOutputCP(CP_UTF8); capture_tests();
    if(argc==2 && std::string(argv[1])=="--capture-only") { std::printf("Capture: %d passed, %d failed\n",passed,failed); return failed?1:0; }
    gesture_tests();
    const auto dir=fs::temp_directory_path()/("fl-layout-ui-"+std::to_string(GetCurrentProcessId()));
    builtin_infrastructure_tests(dir);
    rss_editor_tests(dir);
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
        // P7-4: MSAA-Aufnehmen/Ablegen und Vorschau nur nach abgeschlossener Ablage.
        h.drain(); page->tick(h,h.state); h.drain();
        h.view.focus(grid,true); key(h.view,VK_F6,true); CHECK(grid->area==0);
        key(h.view,VK_SPACE); CHECK(grid->carrying()); msaa(h,grid);
        CHECK(grid->value(false).find(L"aufgenommen")!=std::wstring::npos);
        key(h.view,VK_ESCAPE); CHECK(!grid->carrying()); key(h.view,VK_F6,true); // Raster -> Leiste
        key(h.view,VK_F6); CHECK(grid->area==1);
        grid->column=grid->row=grid->selected=0;
        auto zr=grid->zone_rect(0), gr=grid->grid_rect(); Point from{zr.left+24,zr.top+12}, to{zr.left+24+(gr.right-gr.left)/3,zr.top+12};
        auto previous_previews=h.previews;
        grid->pointer(GridEditor::Kind::Down,from); grid->pointer(GridEditor::Kind::Move,to);
        CHECK(grid->carrying()); page->tick(h,h.state); CHECK(h.jobs.empty()); CHECK(h.previews==previous_previews);
        grid->pointer(GridEditor::Kind::Up,to); CHECK(!grid->carrying()); page->tick(h,h.state); CHECK(h.jobs.size()==1);
        h.drain(); CHECK(h.previews==previous_previews+1); CHECK(h.view.focused()==grid);
        grid->input('Z',false,false,true); CHECK(plugins::edit::equal(grid->model,original));
        grid->column=grid->row=0; h.drain(); page->tick(h,h.state); h.drain();

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
    if(!update) { std::ifstream in(baseline/"p7-hashes.txt"); CHECK(bool(in)); for(std::string name,hash;in>>name>>hash;) expected[name]=hash; }
    Kit kit;
    for(auto& tc:snapshot::theme_cases()) {
        if(tc.name=="dunkel" || tc.name=="custom") continue;
        kit.dark=false; ojson dp={{"theme",tc.name}}; if(!tc.inline_theme.is_null()) dp["theme_inline"]=tc.inline_theme;
        kit.use_design(dp);
        if(tc.name=="windows") { theme::WindowsSettings w; w.has_accent=true; kit.use_windows(true,&w); }
        else kit.use_windows(false);
        const auto& th=kit.theme();
        auto color=[](Rgba c) { return Color(uint8_t(std::lround(c.r*255)),uint8_t(std::lround(c.g*255)),uint8_t(std::lround(c.b*255))); };
        CHECK(theme::contrast(color(th.text),color(th.card))>=4.5); // Chips, Geist, Ablage, Griffe
        CHECK(theme::contrast(color(th.text),color(th.bg))>=4.5);   // Leistenüberschrift, Zellraster
        for(bool portrait:{false,true}) {
            auto image=grid_image(kit,portrait?320:640,portrait?760:600);
            auto key=tc.name+(portrait?".portrait":".landscape"); actual[key]=snapshot::hash_image(image);
            if(!update) CHECK(expected[key]==actual[key]);
            const auto images=fs::path(argv[0]).parent_path()/"p7-ui-snapshots";
            fs::create_directories(images); snapshot::write_png(images/(key+".png"),image);
            for(int state=1;state<=5;++state) {
                auto state_image=grid_image(kit,portrait?320:640,portrait?760:600,nullptr,0,nullptr,state);
                auto state_key=key+".state"+std::to_string(state); actual[state_key]=snapshot::hash_image(state_image);
                if(!update) CHECK(expected[state_key]==actual[state_key]);
                snapshot::write_png(images/(state_key+".png"),state_image);
            }
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
    if(update) { fs::create_directories(baseline); std::ofstream out(baseline/"p7-hashes.txt"); for(auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n'; CHECK(bool(out)); }
    else CHECK(expected.size()==actual.size());
    std::error_code ec; fs::remove_all(dir,ec); CHECK(!ec && !fs::exists(dir));
    std::printf("Layout UI: %d passed, %d failed\n",passed,failed); return failed?1:0;
}
