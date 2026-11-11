// © 2026 FuchsLabs – isolierte native Editorprüfung, keine App-/Netzwerk-Schleife.
#include "ui_pages.h"
#include "perflog.h"
#include "distribution.h"
#include "custom_view_editor.h"
#include "ui_layout_grid.h"
#include "ui_editor.h"
#include "ui_tile_palette.h"
#include "native_settings.h"
#include "app.h"
#include "i18n.h"
#include "util.h"
#include "snapshot_support.h"
#include "page_frame.h"
#include "page_samples.h"
#include <oleacc.h>
#include <wincodec.h>
#include <deque>
#include <fstream>
#include <iostream>
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
    explicit TestHost(fs::path p,ojson settings=offline_config()):cfg(std::move(settings)),dir(p),store(p/"editorplugins"),application(cfg,p/"test.toml",nullptr,p/"preview") {
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
    // P7b-6: zwei Geräte, Standard-Vererbung, Persistenz, Grenzen und Rücksetzen.
    auto bytes=builtin::to_json(changed).dump();
    CHECK(huge.view("rss","unknown")["inherited"]==true);
    CHECK(huge.apply("rss",bytes,"phone-a").empty());
    CHECK(huge.apply("rss",builtin::to_json(next).dump(),"phone-b").empty());
    CHECK(huge.get("rss","phone-a").overlay->model.layout["rows"][0]==3);
    CHECK(huge.get("rss","phone-b").overlay->model.layout["rows"][0]==2);
    CHECK(!huge.get("rss","unknown").overlay);
    CHECK(huge.apply("rss",builtin::to_json(next).dump()).empty());
    CHECK(huge.get("rss","unknown").overlay->model.layout["rows"][0]==2);
    CHECK(huge.get("rss","phone-a").overlay->model.layout["rows"][0]==3);
    builtin::LayoutStore offline(directory);
    CHECK(offline.get("rss","phone-a").overlay->model.layout["rows"][0]==3);
    CHECK(offline.view("rss","phone-a")["inherited"]==false);
    CHECK(huge.apply("rss",builtin::to_json(standard).dump(),"phone-a").empty());
    CHECK(builtin::is_standard(c,*huge.get("rss","phone-a").overlay));
    CHECK(huge.reset("rss","phone-a").empty());
    CHECK(huge.get("rss","phone-a").overlay->model.layout["rows"][0]==2);
    for(int i=1;i<16;++i) CHECK(huge.apply("rss",bytes,"device-"+std::to_string(i)).empty());
    CHECK(!huge.apply("rss",bytes,"seventeenth").empty());
    CHECK(!huge.apply("rss",bytes,std::string(513,'a')).empty());
    CHECK(huge.get("rss","phone-b").overlay->model.layout["rows"][0]==2);
    auto device_revision=huge.get("rss").revision;
    const auto device_backup=directory/"builtin.rss.devices.json.bak";
    fs::remove(device_backup);fs::create_directory(device_backup);
    CHECK(!huge.apply("rss",bytes,"phone-b").empty());
    CHECK(huge.get("rss").revision==device_revision);
    CHECK(huge.get("rss","phone-b").overlay->model.layout["rows"][0]==2);
    CHECK(!huge.reset("rss","phone-b").empty());fs::remove(device_backup);
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
// P7b-6: dieselbe Seite auf zwei Geräten mit verschiedenen Bildern und Trefferflächen.
static void device_render_tests(const fs::path& root) {
    builtin::LayoutStore store(root/"multi-display");const auto& c=*builtin::find("rss");
    auto a=builtin::standard(c),b=a;a.model.layout["rows"]={1,1};b.model.layout["rows"]={3,1};
    CHECK(store.apply("rss",builtin::to_json(a).dump(),"a").empty());
    CHECK(store.apply("rss",builtin::to_json(b).dump(),"b").empty());
    render::RssRenderer renderer(480,320,"RSS");
    auto draw_a=renderer.render_arranged(render::samples::posts(),render::samples::CLOCK,*store.get("rss","a").overlay);
    auto draw_b=renderer.render_arranged(render::samples::posts(),render::samples::CLOCK,*store.get("rss","b").overlay);
    CHECK(snapshot::hash_image(draw_a.image)!=snapshot::hash_image(draw_b.image));
    CHECK(!draw_a.targets.empty());CHECK(!draw_b.targets.empty());
    if(!draw_a.targets.empty() && !draw_b.targets.empty()) CHECK(draw_a.targets[0].rect_px.h!=draw_b.targets[0].rect_px.h);
}
static void device_editor_tests(const fs::path& root) {
    TestHost h(root/"device-editor");CHECK(h.store.load());CHECK(h.store.import_manifest(fixture().dump()));
    h.actions.layout_displays=[] {return Json::array({
        {{"id","phone"},{"name","Phone"},{"width",320},{"height",480},{"usable_width",320},{"usable_height",460},{"online",true},{"tv",false}},
        {{"id","tv"},{"name","Android TV"},{"width",1920},{"height",1080},{"usable_width",1920},{"usable_height",1080},{"online",true},{"tv",true}}});};
    h.actions.device_layout_get=[&](bool,const auto& id,const auto& device){return h.store.arrangement_get(id,device);};
    h.actions.device_layout_apply=[&](bool,const auto& id,const auto& device,const auto& j){CHECK(h.store.set_arrangement(id,j.dump(),device));};
    h.actions.device_layout_reset=[&](bool,const auto& id,const auto& device){CHECK(h.store.clear_arrangement(id,device));};
    auto page=make_layout_page();page->build(h,h.root);page->shown(h);h.drain();page->tick(h,h.state);h.drain();
    // Preview completion needs another UI tick to refresh Apply's enabled state.
    auto settle=[&]{bool idle=false;for(int i=0;i<12;++i){page->tick(h,h.state);const bool work=h.busy();h.drain();if(!work){idle=true;break;}}CHECK(idle && !h.busy());};
    auto grid=h.find<GridEditor>();CHECK(grid);if(!grid)return;
    auto model=grid->model;model.layout["columns"]={2,1,1};grid->on_edit(model,"");
    auto phone=h.find<Button>(L"Phone · 320 × 480 · Hoch · erbt Standard");CHECK(phone);if(!phone)return;
    phone->activate(h.view);CHECK(h.confirmations==1);CHECK(h.question.en.find(L"display")!=std::wstring::npos);
    CHECK(h.find<GridEditor>()->model.layout["columns"][0]==2);
    h.confirm_answer=true;phone->activate(h.view);settle();
    auto orientation=h.find<Segment>(L"Ausrichtung");CHECK(orientation && orientation->index==1);
    auto apply=h.find<Button>(L"Übernehmen");CHECK(apply && apply->enabled);if(apply)apply->activate(h.view);settle();
    CHECK(h.store.arrangement_get("editor-test","phone")["inherited"]==false);
    auto own_tab=h.find<Button>(L"Phone · 320 × 480 · Hoch · eigene Anordnung");
    CHECK(own_tab && own_tab->role()==Role::NavTab && own_tab->checked());
    CHECK(std::abs(h.find<GridEditor>()->display_aspect-320.f/460.f)<.001f);
    CHECK(h.store.arrangement_get("editor-test","tv")["inherited"]==true);
    auto tv=h.find<Button>(L"Android TV · 1920 × 1080 · Quer · erbt Standard");CHECK(tv);if(tv)tv->activate(h.view);settle();
    orientation=h.find<Segment>(L"Ausrichtung");CHECK(orientation && !orientation->enabled && orientation->index==0);
}
// P7-10: Freunde und Discord teilen den Katalog, niemals Datei, Backup oder Revision.
static void friends_store_tests(const fs::path& root) {
    const auto directory=root/"separate-friends";
    builtin::LayoutStore store(directory);
    const auto& c=*builtin::find("friends");
    auto friends=builtin::standard(c),discord=builtin::standard(*builtin::find("discord"));
    friends.model.layout["rows"]={3,1};discord.model.layout["rows"]={2,1};
    CHECK(store.apply("friends",builtin::to_json(friends).dump()).empty());
    CHECK(store.get("friends").revision==1);CHECK(store.get("discord").revision==0);CHECK(!store.get("discord").overlay);
    CHECK(store.apply("discord",builtin::to_json(discord).dump()).empty());
    CHECK(store.get("discord").revision==1);CHECK(store.get("friends").revision==1);
    CHECK(builtin::to_json(*store.get("friends").overlay)==builtin::to_json(friends));
    CHECK(builtin::to_json(*store.get("discord").overlay)==builtin::to_json(discord));
    friends.model.layout["rows"]={1,2};CHECK(store.apply("friends",builtin::to_json(friends).dump()).empty());
    CHECK(fs::exists(directory/"builtin.friends.layout.json.bak"));CHECK(!fs::exists(directory/"builtin.discord.layout.json.bak"));
    builtin::LayoutStore reloaded(directory);
    CHECK(builtin::to_json(*reloaded.get("friends").overlay)==builtin::to_json(friends));
    CHECK(builtin::to_json(*reloaded.get("discord").overlay)==builtin::to_json(discord));
    CHECK(store.reset("friends").empty());CHECK(!store.get("friends").overlay);CHECK(store.get("friends").revision==3);
    CHECK(store.get("discord").revision==1);CHECK(fs::exists(directory/"builtin.discord.layout.json"));
    CHECK(builtin::to_json(*store.get("discord").overlay)==builtin::to_json(discord));
}
// P7-7: Editor öffnet und übernimmt Standard ohne Dateianlage; Hochformat separat.
static void builtin_editor_tests(const fs::path& root, const builtin::Catalog& catalog) {
    const auto& key=catalog.key;
    TestHost h(root/(key+"-editor"));
    h.actions.builtin_layout_get=[&](const std::string& key){return h.application.builtin_layout_get(key);};
    h.actions.builtin_layout_apply=[&](const std::string& key,const Json& overlay){h.application.builtin_layout_apply(key,overlay);};
    h.actions.builtin_layout_reset=[&](const std::string& key){h.application.builtin_layout_reset(key);};
    auto page=make_layout_page();page->build(h,h.root);page->shown(h);h.drain();page->tick(h,h.state);h.drain();page->tick(h,h.state);
    auto grid=h.find<GridEditor>();CHECK(grid);if(!grid) return;
    auto choice=h.find<Combo>(L"Seite");CHECK(choice && choice->options.size()==size_t(std::count_if(builtin::catalogs().begin(),builtin::catalogs().end(),[](const auto& c){return c.enabled;})));
    size_t selected=0;
    while(selected<choice->options.size() && choice->options[selected].de!=L"Eingebaut · "+widen(catalog.label.de)) ++selected;
    CHECK(selected<choice->options.size());if(selected==choice->options.size()) return;
    if(choice->index!=int(selected)) {choice->index=int(selected);choice->on_change();}
    h.drain();page->tick(h,h.state);h.drain();page->tick(h,h.state);
    grid=h.find<GridEditor>();CHECK(grid);if(!grid) return;
    CHECK(choice->options[selected].de==L"Eingebaut · "+widen(catalog.label.de));
    CHECK(grid->model.blocks[0]["id"]==catalog.blocks[0].id);CHECK(grid->model.blocks[1]["id"]==catalog.blocks[1].id);
    const auto original=grid->model;
    CHECK(count_dirty(&h.root)==0);CHECK(h.image);
    const auto file=h.dir/"layouts"/("builtin."+key+".layout.json");
    CHECK(!fs::exists(file));auto apply=h.find<Button>(L"Übernehmen");CHECK(apply && apply->enabled);
    apply->on_click();h.drain();CHECK(!fs::exists(file));CHECK(h.application.builtin_layout_get(key)["layout_revision"]==0);
    if(key=="system" || key=="spotify" || key=="ai") {
        if(key=="spotify") {
            auto invalid=grid->model;invalid.blocks[5]["shown"]=false;grid->on_edit(invalid,{});
            CHECK(plugins::edit::equal(grid->model,original));CHECK(h.toast_text==L"Pflichtblock kann nicht entfernt werden.");
            invalid=grid->model;invalid.layout["rows"]={12,1,1};grid->on_edit(invalid,{});CHECK(!apply->enabled);
            h.find<Button>(L"Rückgängig")->on_click();CHECK(apply->enabled);
        }
        auto n=grid->model;CHECK(plugins::edit::weight(n,false,0,2));grid->on_edit(n,{});
        page->tick(h,h.state);h.drain();CHECK(h.image);CHECK(!fs::exists(file));
        apply->on_click();h.drain();CHECK(fs::exists(file));CHECK(h.application.builtin_layout_get(key)["layout_revision"]==1);
        auto saved=builtin::parse(catalog,h.application.builtin_layout_get(key)["overlay"].dump());CHECK(saved);
        CHECK(saved.overlay->model.layout["rows"][0]==2);CHECK(saved.overlay->portrait->layout["rows"][0]==1);
        CHECK(h.application.builtin_layout_get(key=="system"?"spotify":"system")["layout_revision"]==0);
        auto orientation=h.find<Segment>(L"Ausrichtung");CHECK(orientation);orientation->index=1;orientation->on_change();
        CHECK(grid->model.layout["rows"][0]==1);
        n=grid->model;CHECK(plugins::edit::weight(n,false,0,3));grid->on_edit(n,{});apply->on_click();h.drain();
        CHECK(fs::exists(fs::path(file.wstring()+L".bak")));
        saved=builtin::parse(catalog,h.application.builtin_layout_get(key)["overlay"].dump());CHECK(saved.overlay->portrait->layout["rows"][0]==3);
        auto reset=h.find<Button>(L"Auf Standard zurücksetzen");CHECK(reset);h.confirm_answer=true;reset->on_click();h.drain();
        CHECK(!fs::exists(file));CHECK(!fs::exists(fs::path(file.wstring()+L".bak")));
        orientation->index=0;orientation->on_change();CHECK(plugins::edit::equal(grid->model,original));
        CHECK(choice->options[selected].en==L"Built-in · "+widen(catalog.label.en));
        h.view.set_root(nullptr);return;
    }
    auto n=grid->model;n.blocks[1]["shown"]=false;grid->on_edit(n,{});
    CHECK(plugins::edit::equal(grid->model,original));CHECK(h.toast_text==L"Pflichtblock kann nicht entfernt werden.");
    n=grid->model;CHECK(plugins::edit::weight(n,false,0,3));grid->on_edit(n,{});
    page->tick(h,h.state);h.drain();CHECK(h.image);CHECK(!fs::exists(file));
    apply->on_click();h.drain();CHECK(fs::exists(file));CHECK(h.application.builtin_layout_get(key)["layout_revision"]==1);
    auto saved=builtin::parse(*builtin::find(key),h.application.builtin_layout_get(key)["overlay"].dump());CHECK(saved);
    CHECK(saved.overlay->model.layout["rows"][0]==3);CHECK(saved.overlay->portrait->layout["rows"][0]==4);
    auto orientation=h.find<Segment>(L"Ausrichtung");CHECK(orientation);orientation->index=1;orientation->on_change();
    CHECK(grid->model.layout["rows"][0]==4);CHECK(!h.find<Button>(L"Rückgängig")->enabled);
    n=grid->model;CHECK(plugins::edit::weight(n,false,0,2));grid->on_edit(n,{});
    orientation->index=0;orientation->on_change();CHECK(grid->model.layout["rows"][0]==3);
    apply->on_click();h.drain();
    saved=builtin::parse(*builtin::find(key),h.application.builtin_layout_get(key)["overlay"].dump());CHECK(saved);
    CHECK(saved.overlay->model.layout["rows"][0]==3);CHECK(saved.overlay->portrait->layout["rows"][0]==2);
    // Übermäßig verkleinerter Pflichtblock sperrt die Anwendung in beiden Ausrichtungen.
    n=grid->model;n.layout["rows"]={12,1};grid->on_edit(n,{});CHECK(!apply->enabled);
    h.find<Button>(L"Rückgängig")->on_click();CHECK(apply->enabled);
    auto reset=h.find<Button>(L"Auf Standard zurücksetzen");CHECK(reset);h.confirm_answer=true;reset->on_click();h.drain();
    CHECK(!fs::exists(file));CHECK(!fs::exists(fs::path(file.wstring()+L".bak")));
    CHECK(plugins::edit::equal(grid->model,original));
    page->tick(h,h.state);h.drain();CHECK(h.image);
    // Englische Katalogbeschriftungen und kein Eintrag für spätere Seiten (insbesondere HA).
    CHECK(choice->options[selected].en==L"Built-in · "+widen(catalog.label.en));CHECK(choice->options.size()==size_t(std::count_if(builtin::catalogs().begin(),builtin::catalogs().end(),[](const auto& c){return c.enabled;})));
    h.view.set_root(nullptr);
}
// Hilfen für die Editor-Oberfläche (Phase 7b): echte Surface-Nachrichten in Dokumentkoordinaten.
static void mouse(Surface& s,UINT message,Point p) {
    SendMessageW(s.hwnd(),message,message==WM_LBUTTONUP?0:MK_LBUTTON,MAKELPARAM(s.px(p.x),s.px(p.y-s.scroll())));
}
static void builtin_actions(TestHost& h) {
    h.actions.builtin_layout_get=[&](const std::string& key){return h.application.builtin_layout_get(key);};
    h.actions.builtin_layout_apply=[&](const std::string& key,const Json& overlay){h.application.builtin_layout_apply(key,overlay);};
    h.actions.builtin_layout_reset=[&](const std::string& key){h.application.builtin_layout_reset(key);};
}
static void settle(TestHost& h,Page& page) { for(int i=0;i<4;++i) {page.tick(h,h.state);h.drain();} }
static bool choose_page(TestHost& h,Page& page,const std::string& key) {
    auto choice=h.find<Combo>(L"Seite"); if(!choice) return false;
    const auto label=L"Eingebaut · "+widen(builtin::find(key)->label.de);
    for(size_t i=0;i<choice->options.size();++i) if(choice->options[i].de==label) { choice->index=int(i); choice->on_change(); settle(h,page); return true; }
    return false;
}
// P7b-1: Fensteraufbau (Kopfleiste, Schiene, Bereiche, Breitenstufen).
static void pin_window(TestHost& h) { h.view.set_dpi(96); SetWindowPos(h.view.hwnd(),nullptr,0,0,1180,760,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); h.relayout(); }
static void editor_shell_tests(const fs::path& root) {
    CHECK(EditorShell::mode_for(1164)==EditorShell::Wide); CHECK(EditorShell::mode_for(700)==EditorShell::Medium); CHECK(EditorShell::mode_for(400)==EditorShell::Narrow);
    TestHost h(root/"shell"); pin_window(h); builtin_actions(h);
    auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page);
    CHECK(choose_page(h,*page,"rss"));
    auto shell=h.find<EditorShell>(); CHECK(shell); if(!shell) return;
    CHECK(shell->mode==EditorShell::Wide);
    CHECK(shell->is_open(EditorShell::Palette)); CHECK(shell->is_open(EditorShell::Layers)); CHECK(!shell->is_open(EditorShell::PageList));
    // Drei Spalten: Palette links, Fläche in der Mitte, Eigenschaften rechts; alles im Fenster, nichts überlappt.
    auto palette=h.find<TilePalette>(); auto grid=h.find<GridEditor>(); auto zone_id=h.find<TextField>(L"Zonen-ID");
    CHECK(palette && grid && zone_id); if(!palette || !grid || !zone_id) return;
    CHECK(palette->rc.right<=grid->rc.left); CHECK(grid->rc.right<=zone_id->rc.left);
    CHECK(shell->rc.right<=h.view.size().width+.5f);
    // Kopfleiste: Seitenauswahl, Gerätegröße, Rückgängig/Wiederholen/Übernehmen/Zurücksetzen.
    auto chip=h.find<Chip>(); CHECK(chip && chip->text.de==L"480 × 320 · Querformat" && chip->text.en==L"480 × 320 · Landscape");
    auto choice=h.find<Combo>(L"Seite"); auto undo=h.find<Button>(L"Rückgängig"); auto redo=h.find<Button>(L"Wiederholen");
    auto apply=h.find<Button>(L"Übernehmen"); auto reset=h.find<Button>(L"Auf Standard zurücksetzen");
    CHECK(choice && undo && redo && apply && reset);
    if(choice && undo && apply) CHECK(choice->rc.top<=undo->rc.bottom && undo->rc.top<shell->rc.top && apply->rc.top<shell->rc.top);
    // Schiene: Rollen, Zustand und Umschalten per Tastatur; Ebenenliste verschwindet und kommt wieder.
    RailItem* layers=nullptr; for(auto w:h.view.flatten(false)) if(auto r=dynamic_cast<RailItem*>(w)) if(r->text.de==L"Ebenen") layers=r;
    CHECK(layers && layers->role()==Role::NavTab && layers->checked() && layers->value(false)==L"geöffnet"); if(!layers) return;
    CHECK(h.find<LayoutBlockList>());
    h.view.focus(layers,true); key(h.view,VK_SPACE); CHECK(!layers->checked()); CHECK(!h.find<LayoutBlockList>()); CHECK(layers->value(true)==L"closed");
    key(h.view,VK_SPACE); CHECK(layers->checked()); CHECK(h.find<LayoutBlockList>());
    // Seitenliste: Auswahl wechselt die Seite (ohne ungesicherte Änderungen keine Rückfrage).
    for(auto w:h.view.flatten(false)) if(auto r=dynamic_cast<RailItem*>(w)) if(r->text.de==L"Seiten") r->activate(h.view);
    auto pages=h.find<ItemList>(L"Seitenliste"); CHECK(pages && pages->selected==choice->index && pages->items.size()==choice->options.size());
    if(pages) { const int other=pages->selected==0?1:0; pages->select(other,h.view); settle(h,*page); CHECK(h.find<Combo>(L"Seite")->index==other); CHECK(h.find<ItemList>(L"Seitenliste")->selected==other); }
    // Mittel: Palette offen -> Eigenschaften unter der Fläche; schmal: Palette anfangs eingeklappt, Ebenen sichtbar.
    SetWindowPos(h.view.hwnd(),nullptr,0,0,760,900,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); h.relayout();
    shell=h.find<EditorShell>(); CHECK(shell->mode==EditorShell::Medium);
    CHECK(h.find<TextField>(L"Zonen-ID")->rc.top>=h.find<GridEditor>()->rc.bottom-1);
    SetWindowPos(h.view.hwnd(),nullptr,0,0,420,900,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); h.relayout();
    CHECK(shell->mode==EditorShell::Narrow); CHECK(h.find<LayoutBlockList>());
    CHECK(!shell->is_open(EditorShell::Palette)); CHECK(!h.find<TilePalette>()); // schmal: Kachelpalette anfangs eingeklappt
    shell->rail[EditorShell::Palette]->activate(h.view); CHECK(shell->is_open(EditorShell::Palette)); CHECK(h.find<TilePalette>()); CHECK(h.find<TilePalette>()->rc.right-h.find<TilePalette>()->rc.left>=96);
    for(auto w:h.view.flatten(true)) CHECK(w->rc.bottom>w->rc.top && w->rc.right>w->rc.left);
    h.view.set_root(nullptr);
}
// P7b-2: Kachelpalette (Suche, Filter, Mini-Vorschau, Ziehen, Klick-Einfügen, Tastatur).
static void tile_palette_tests(const fs::path& root) {
    TestHost h(root/"palette"); pin_window(h); builtin_actions(h);
    auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page);
    // Mini-Vorschauen aus dem echten Seitenrenderer: jede Kachel jeder Seite hat ein Bild oder ist als „keine Daten“ markiert.
    for(const auto& catalog:builtin::catalogs()) if(catalog.enabled) {
        CHECK(choose_page(h,*page,catalog.key));
        auto palette=h.find<TilePalette>(); CHECK(palette && palette->tiles.size()==std::count_if(catalog.blocks.begin(),catalog.blocks.end(),[](const auto& b){return !b.tile_only;})); if(!palette) continue;
        for(const auto& tile:palette->tiles) {
            auto thumb=palette->thumb_of(tile.id);
            CHECK(thumb && ((thumb->image && !thumb->image->empty()) || thumb->no_data));
            if(thumb && thumb->image) CHECK(thumb->image->w<=192 && thumb->image->h<=108);
        }
        if(catalog.key=="rss") {
            CHECK(palette->thumb_of("rss.liste")->image && !palette->thumb_of("rss.liste")->no_data);
            CHECK(palette->thumb_of("rss.hinweis")->no_data); // Hinweis erscheint nur ohne Daten
        }
    }
    CHECK(choose_page(h,*page,"system"));
    auto palette=h.find<TilePalette>(); auto grid=h.find<GridEditor>(); CHECK(palette && grid); if(!palette || !grid) return;
    CHECK(palette->role()==Role::List && palette->name.de==L"Kachelpalette");
    CHECK(palette->value(false)==L"Kachel 1 von 5: CPU, platziert. Leertaste: aufnehmen, Enter: wählen oder hinzufügen. Liste: CPU gezeigt, GPU gezeigt, RAM gezeigt, Datenträger gezeigt, Netzwerk gezeigt");
    CHECK(palette->value(true).starts_with(L"Tile 1 of 5: CPU, placed. Space: pick up, Enter: select or add. List: CPU shown"));
    palette->current=2; CHECK(palette->value(false).starts_with(L"Kachel 3 von 5: RAM, platziert.")); palette->current=0;
    // Bild- und Beschriftungsgrößen: Palette ist breit genug, Kacheln liegen im Raster der Palette.
    CHECK(palette->visible().size()==5 && palette->columns()>=2);
    for(size_t i=0;i<5;++i) { auto r=palette->tile_rect(i); CHECK(r.left>=palette->rc.left-.5f && r.right<=palette->rc.right+.5f && r.bottom>r.top); }
    // Suche und Filter.
    auto search=h.find<TextField>(L"Kacheln suchen"); CHECK(search); if(!search) return;
    search->set_text(L"cpu"); search->on_change(); CHECK(palette->visible().size()==1 && palette->tiles[palette->visible()[0]].id=="system.cpu");
    CHECK(palette->value(false).starts_with(L"Kachel 1 von 1: CPU, platziert.") && palette->value(false).find(L"Gefiltert: 1 von 5.")!=std::wstring::npos);
    search->set_text(L"GRAFIK"); search->on_change(); CHECK(palette->visible().empty()); CHECK(palette->value(false).starts_with(L"Keine Kachel passt."));
    search->set_text(L"net"); search->on_change(); CHECK(palette->visible().size()==1);
    search->set_text(L""); search->on_change(); CHECK(palette->visible().size()==5);
    Button* media=nullptr; Button* sys=nullptr; for(auto w:h.view.flatten(false)) if(auto b=dynamic_cast<Button*>(w)) { if(b->text.de==L"Medien") media=b; if(b->text.de==L"System") sys=b; }
    CHECK(media && sys && media->role()==Role::NavTab); if(!media || !sys) return;
    media->activate(h.view); CHECK(palette->visible().empty() && media->checked() && !sys->checked());
    sys->activate(h.view); CHECK(palette->visible().size()==5 && sys->checked());
    h.find<Button>(L"Alle")->activate(h.view); CHECK(palette->category.empty());
    // Vorbereitung: letzten Block samt Zone entfernen, damit eine freie Zelle zum Ablegen entsteht.
    const auto original=grid->model;
    auto free_model=original; CHECK(plugins::edit::remove_block(free_model,4)); CHECK(plugins::edit::delete_zone(free_model,4));
    grid->on_edit(free_model,{}); settle(h,*page); grid=h.find<GridEditor>(); palette=h.find<TilePalette>();
    CHECK(!grid->shown_block(4)); CHECK(palette->value(false).find(L"Netzwerk verborgen")!=std::wstring::npos);
    auto cell=[&](int c,int r) { auto g=grid->grid_rect(); const float cw=std::max(32.f,(g.right-g.left)/2),ch=(g.bottom-g.top)/3; return Point{g.left+(c+.5f)*cw,g.top+(r+.5f)*ch}; };
    // Ziehen aus der Palette per Maus auf die freie Zelle: eine Geste = ein Rückgängig-Schritt, Vorschau erst danach.
    int edits=0; const auto page_on_edit=grid->on_edit; grid->on_edit=[&,page_on_edit](auto m,auto g) { ++edits; page_on_edit(std::move(m),std::move(g)); };
    const auto tile=palette->tile_rect(4); Point from{(tile.left+tile.right)/2,(tile.top+tile.bottom)/2};
    h.view.focus(palette,true); const auto previews=h.previews;
    mouse(h.view,WM_LBUTTONDOWN,from); mouse(h.view,WM_MOUSEMOVE,{from.x+30,from.y}); mouse(h.view,WM_MOUSEMOVE,cell(0,2));
    CHECK(grid->carrying()); CHECK(grid->target_valid()); CHECK(edits==0); CHECK(!grid->shown_block(4)); page->tick(h,h.state); CHECK(h.jobs.empty()); CHECK(h.previews==previews);
    mouse(h.view,WM_LBUTTONUP,cell(0,2)); CHECK(!grid->carrying()); CHECK(edits==1); CHECK(grid->shown_block(4)); CHECK(grid->model.layout["zones"].size()==5);
    CHECK(grid->model.blocks[4]["zone"]==grid->model.layout["zones"][4]["id"]);
    settle(h,*page); CHECK(h.previews>previews);
    CHECK(h.find<Button>(L"Rückgängig")->enabled); h.find<Button>(L"Rückgängig")->activate(h.view); settle(h,*page);
    CHECK(!h.find<GridEditor>()->shown_block(4)); CHECK(plugins::edit::equal(h.find<GridEditor>()->model,free_model));
    // Abbruch mit Esc und außerhalb der Fläche ändert nichts.
    grid=h.find<GridEditor>(); palette=h.find<TilePalette>(); edits=0;
    mouse(h.view,WM_LBUTTONDOWN,from); mouse(h.view,WM_MOUSEMOVE,cell(0,2)); CHECK(grid->carrying()); key(h.view,VK_ESCAPE); mouse(h.view,WM_LBUTTONUP,cell(0,2));
    CHECK(!grid->carrying()); CHECK(edits==0); CHECK(!grid->shown_block(4));
    mouse(h.view,WM_LBUTTONDOWN,from); mouse(h.view,WM_MOUSEMOVE,{palette->rc.left+5,palette->rc.top-40}); mouse(h.view,WM_LBUTTONUP,{palette->rc.left+5,palette->rc.top-40});
    CHECK(edits==0); CHECK(!grid->shown_block(4));
    // Klick fügt an der ersten freien Stelle ein (ein Schritt).
    mouse(h.view,WM_LBUTTONDOWN,from); mouse(h.view,WM_LBUTTONUP,from);
    CHECK(edits==1); CHECK(grid->shown_block(4)); CHECK(grid->model.layout["zones"].size()==5);
    { auto l=plugins::edit::geometry(grid->model); bool ok=false; if(l) for(const auto& z:l->zones) if(grid->model.blocks[4]["zone"]==z.id) ok=z.column==0 && z.row==2; CHECK(ok); }
    // Klick auf eine platzierte Kachel wählt sie im Raster.
    const auto cpu=palette->tile_rect(0); Point on_cpu{(cpu.left+cpu.right)/2,(cpu.top+cpu.bottom)/2};
    grid->selected=3; mouse(h.view,WM_LBUTTONDOWN,on_cpu); mouse(h.view,WM_LBUTTONUP,on_cpu); CHECK(grid->selected==0); CHECK(grid->focused_block==0); CHECK(edits==1);
    // Strg+Tab wie F6: Bereichswechsel zwischen Palette und Fläche in beide Richtungen.
    grid->area=0; h.view.focus(palette,true); key(h.view,VK_TAB,false,false,true); CHECK(h.view.focused()==grid && grid->area==1);
    key(h.view,VK_TAB,true,false,true); CHECK(h.view.focused()==h.find<TilePalette>() && grid->area==0); key(h.view,VK_TAB,false,false,true); CHECK(h.view.focused()==grid);
    // Tastatur: Pfeile bewegen den Cursor, Leertaste nimmt auf und gibt den Fokus an die Fläche, Esc bricht ab, Entf entfernt.
    h.view.focus(palette,true); key(h.view,VK_END); CHECK(palette->current==4); CHECK(grid->palette_block==4);
    key(h.view,VK_HOME); key(h.view,VK_RIGHT); CHECK(palette->current==1);
    key(h.view,VK_SPACE); CHECK(grid->carrying()); CHECK(h.view.focused()==grid); CHECK(grid->value(false).find(L"aufgenommen")!=std::wstring::npos);
    key(h.view,VK_ESCAPE); CHECK(!grid->carrying());
    // Entf entfernt nur, was die Zone nicht leer lässt: der einzige Block seiner Zone bleibt, die Ablehnung wird gemeldet.
    h.view.focus(palette,true); key(h.view,VK_END); CHECK(grid->shown_block(4)); const int toasts=h.toasts; key(h.view,VK_DELETE); CHECK(grid->shown_block(4)); CHECK(edits==1); CHECK(h.toasts==toasts+1);
    grid->selected=3; key(h.view,VK_RETURN); CHECK(grid->selected==4); CHECK(edits==1); // Enter auf eine platzierte Kachel wählt sie
    // MSAA: eigener Name, Rolle und Wert.
    ComPtr<IAccessible> acc; AccessibleObjectFromWindow(h.view.hwnd(),OBJID_CLIENT,IID_PPV_ARGS(acc.GetAddressOf())); bool found=false;
    long count=0; if(acc) acc->get_accChildCount(&count);
    for(long i=1;i<=count;++i) {
        VARIANT child{}; child.vt=VT_I4; child.lVal=i; BSTR name=nullptr; acc->get_accName(child,&name);
        if(name && std::wstring(name)==L"Kachelpalette") {
            found=true; VARIANT role{}; acc->get_accRole(child,&role); CHECK(role.lVal==ROLE_SYSTEM_LIST);
            BSTR value=nullptr; acc->get_accValue(child,&value); CHECK(value && std::wstring(value)==palette->value(false)); SysFreeString(value);
        }
        SysFreeString(name);
    }
    CHECK(found); h.view.set_root(nullptr);
}
// Ganzer Widgetbaum (wie Surface::paint ohne Bildlauf/Fokusring) in ein 96-DPI-Bild, für Editor-Snapshots.
struct TreeCanvas {
    ComPtr<IWICBitmap> bitmap; ComPtr<ID2D1RenderTarget> rt; ComPtr<ID2D1SolidColorBrush> brush; int width=0,height=0;
    TreeCanvas(Kit& kit,int w,int h):width(w),height(h) {
        ComPtr<IWICImagingFactory> wic; CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(wic.GetAddressOf()));
        wic->CreateBitmap(w,h,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,bitmap.GetAddressOf());
        kit.d2d()->CreateWicBitmapRenderTarget(bitmap.Get(),D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,D2D1::PixelFormat(),96,96),rt.GetAddressOf());
        rt->CreateSolidColorBrush(kit.theme().text,brush.GetAddressOf());
    }
    Ctx ctx(Kit& kit) { return Ctx{rt.Get(),kit,kit.theme(),brush.Get()}; }
};
static void layout_tree(Kit& kit,TreeCanvas& canvas,Widget& root) { auto ctx=canvas.ctx(kit); root.layout(ctx,16,12,float(canvas.width-32)); }
static Image paint_tree(Kit& kit,TreeCanvas& canvas,Widget& root,bool native_text=false) {
    auto ctx=canvas.ctx(kit);
    struct Walk { static void go(Ctx& c,Widget* w,bool native_text) {
        if(!w || !w->visible) return; const float saved=c.dim; if(!w->enabled) c.dim*=.45f;
        w->paint(c);
        if(native_text) if(auto field=dynamic_cast<TextField*>(w)) {auto text=field->text();if(text.empty())text=field->hint.get(c.kit.english);if(field->kind!=TextField::Secret)c.text(text,{field->rc.left+10,field->rc.top,field->rc.right-10,field->rc.bottom},ui::Font::Body,c.th.text);}
        std::vector<Widget*> kids; w->children(kids); for(auto k:kids) go(c,k,native_text); c.dim=saved; } };
    canvas.rt->BeginDraw(); canvas.rt->Clear(kit.theme().bg); Walk::go(ctx,&root,native_text); CHECK(SUCCEEDED(canvas.rt->EndDraw()));
    Image image(canvas.width,canvas.height); canvas.bitmap->CopyPixels(nullptr,canvas.width*4,UINT(image.px.size()*4),reinterpret_cast<BYTE*>(image.px.data())); return image;
}
static double contrast_of(Rgba a,Rgba b) {
    auto color=[](Rgba c) { return Color(uint8_t(std::lround(c.r*255)),uint8_t(std::lround(c.g*255)),uint8_t(std::lround(c.b*255))); };
    return theme::contrast(color(a),color(b));
}
// P7b-3: Fläche (Rahmen, Zoom, Ablageplätze, Geist, Papierkorb).
static void canvas_tests(const fs::path& root) {
    TestHost h(root/"canvas"); builtin_actions(h);
    auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page);
    CHECK(choose_page(h,*page,"system"));
    auto grid=h.find<GridEditor>(); CHECK(grid && grid->canvas); if(!grid) return;
    auto ratio=[&] { auto g=h.find<GridEditor>()->grid_rect(); return (g.right-g.left)/(g.bottom-g.top); };
    CHECK(std::abs(ratio()-480.f/320.f)<.01f);
    auto orientation=h.find<Segment>(L"Ausrichtung"); orientation->index=1; orientation->on_change(); settle(h,*page);
    CHECK(std::abs(ratio()-320.f/480.f)<.01f); CHECK(h.find<Chip>()->text.de==L"320 × 480 · Hochformat");
    orientation->index=0; orientation->on_change(); settle(h,*page); CHECK(std::abs(ratio()-1.5f)<.01f);
    grid=h.find<GridEditor>();
    // Rahmen im Widget, Papierkorb darunter; Zoom verkleinert um Stufen, Passend stellt wieder her.
    auto g=grid->grid_rect(); const float full=g.right-g.left;
    CHECK(g.left>=grid->rc.left && g.right<=grid->rc.right && g.top>=grid->rc.top && grid->trash_rect().top>=g.bottom && grid->trash_rect().bottom<=grid->rc.bottom+.5f);
    h.find<Button>(L"Zoom −")->activate(h.view); g=grid->grid_rect(); CHECK(std::abs((g.right-g.left)/full-.85f)<.01f); CHECK(std::abs(ratio()-1.5f)<.01f);
    for(int i=0;i<6;++i) h.find<Button>(L"Zoom −")->activate(h.view);
    g=grid->grid_rect(); CHECK(std::abs((g.right-g.left)/full-.5f)<.01f); h.find<Button>(L"Zoom +")->activate(h.view); CHECK(grid->zoom>.6f);
    h.find<Button>(L"Passend")->activate(h.view); g=grid->grid_rect(); CHECK(std::abs((g.right-g.left)-full)<.5f);
    for(int i=0;i<3;++i) h.find<Button>(L"Zoom +")->activate(h.view); CHECK(grid->zoom<=1.f);
    // Ungültiges Ziel: Papierkorb würde die Zone leeren -> „passt nicht“ als Text, nicht nur Farbe.
    auto zr=grid->zone_rect(0); auto chip=grid->block_rect(0); Point from{(chip.left+chip.right)/2,(chip.top+chip.bottom)/2};
    auto tr=grid->trash_rect();
    grid->start_external(0,from,h.view); grid->drag({from.x+20,from.y},h.view); grid->drag({(tr.left+tr.right)/2,(tr.top+tr.bottom)/2},h.view);
    CHECK(grid->carrying()); CHECK(!grid->target_valid()); CHECK(grid->value(false).find(L"passt nicht")!=std::wstring::npos); CHECK(grid->value(true).find(L"does not fit")!=std::wstring::npos);
    grid->cancel(h.view); CHECK(!grid->carrying()); (void)zr;
    h.view.set_root(nullptr);
}
// Plugin-Seite mit sechs Widgets in drei Zonen: jede Kachel bekommt ihre Einzelzonen-Vorschau (verborgene Blöcke zählen nicht gegen das Zonenlimit).
static void plugin_palette_tests(const fs::path& root) {
    Json widgets=Json::array(); for(int i=0;i<6;++i) widgets.push_back({{"id","w"+std::to_string(i)},{"type","text"},{"text","Sample "+std::to_string(i)},{"zone","z"+std::to_string(i/2)}});
    const Json manifest={{"schema_version",2},{"id","many-test"},{"name","Many Test"},{"version","1.0.0"},
        {"layout",{{"columns",{1,1,1}},{"rows",{1}},{"zones",{{{"id","z0"},{"column",0},{"row",0}},{{"id","z1"},{"column",1},{"row",0}},{{"id","z2"},{"column",2},{"row",0}}}}}},{"widgets",widgets}};
    TestHost h(root/"many"); pin_window(h); CHECK(h.store.load()); CHECK(h.store.import_manifest(manifest.dump()));
    auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page);
    auto palette=h.find<TilePalette>(); CHECK(palette && palette->tiles.size()==6); if(!palette) return;
    for(const auto& tile:palette->tiles) { auto thumb=palette->thumb_of(tile.id); CHECK(thumb && thumb->image && thumb->image->w>0); }
    CHECK(palette->tiles[0].category=="other");
    auto info=h.find<Label>(L"Auswahl"); CHECK(info && info->text.de.find(L"Zone z0 · Größe 1 × 1 Zellen")!=std::wstring::npos && info->text.en.find(L"size 1 × 1 cells")!=std::wstring::npos);
    h.view.set_root(nullptr);
}
// P7b-4: Ebenenliste, Seitenliste, Einsteigerkarte.
static void layers_pages_intro_tests(const fs::path& root) {
    TestHost h(root/"layers"); pin_window(h); builtin_actions(h);
    int seen_reads=0,done_calls=0; bool seen=false;
    h.actions.editor_intro_seen=[&] { ++seen_reads; return seen; };
    h.actions.editor_intro_done=[&] { ++done_calls; seen=true; };
    auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page);
    // Einsteigerkarte beim ersten Öffnen: drei Schritte, wegklickbar, gemerkt (Datei), danach nie wieder.
    Card* intro=nullptr; for(auto w:h.view.flatten(false)) if(auto c=dynamic_cast<Card*>(w)) if(c->title.de==L"In drei Schritten") intro=c;
    CHECK(intro && intro->title.en==L"In three steps"); CHECK(seen_reads>=1); if(!intro) return;
    CHECK(intro->rc.bottom>intro->rc.top);
    int labels=0; for(auto& item:intro->items) if(auto l=dynamic_cast<Label*>(item.get())) { ++labels; CHECK(!l->text.de.empty() && !l->text.en.empty() && l->text.de!=l->text.en); }
    CHECK(labels==3);
    auto got_it=h.find<Button>(L"Verstanden"); CHECK(got_it); got_it->activate(h.view); settle(h,*page);
    CHECK(done_calls==1); CHECK(seen); intro=nullptr; for(auto w:h.view.flatten(false)) if(auto c=dynamic_cast<Card*>(w)) if(c->title.de==L"In drei Schritten") intro=c;
    CHECK(!intro);
    page->hidden(h); page->shown(h); settle(h,*page); CHECK(!h.find<Button>(L"Verstanden"));
    // Ebenenliste: Auswahl, Sichtbarkeit und Reihenfolge wirken auf Modell und Fläche.
    CHECK(choose_page(h,*page,"system"));
    auto grid=h.find<GridEditor>(); auto blocks=h.find<LayoutBlockList>(); CHECK(grid && blocks); if(!grid || !blocks) return;
    CHECK(blocks->items.size()==5 && blocks->items[0].title.de==L"CPU" && blocks->items[0].title.en==L"CPU" && blocks->items[3].title.de==L"Datenträger" && blocks->items[3].title.en==L"Disk");
    blocks->selected=2; blocks->on_select(2); CHECK(grid->selected==2 && grid->focused_block==2);
    grid->select_block(4,h.view); CHECK(blocks->selected==4);
    h.view.focus(blocks,true); key(h.view,VK_UP); CHECK(blocks->selected==3 && grid->selected==3);
    blocks->toggle(1,h.view); settle(h,*page); CHECK(!h.find<GridEditor>()->shown_block(1)); CHECK(h.find<TilePalette>()->value(false).find(L"GPU verborgen")!=std::wstring::npos);
    CHECK(!h.find<Button>(L"Übernehmen")->enabled); // Zone ohne Block ist ungültig
    h.find<Button>(L"Rückgängig")->activate(h.view); settle(h,*page); CHECK(h.find<GridEditor>()->shown_block(1)); CHECK(h.find<Button>(L"Übernehmen")->enabled);
    h.view.focus(blocks,true); blocks->selected=0; key(h.view,VK_DOWN,false,true); CHECK(h.find<GridEditor>()->model.blocks[0]["id"]=="system.gpu");
    CHECK(h.find<TilePalette>()->tiles[0].id=="system.gpu"); // Palette folgt der Reihenfolge
    h.find<Button>(L"Rückgängig")->activate(h.view); settle(h,*page);
    // Seitenliste: zeigt alle Seiten mit Art, folgt der Seitenauswahl; Wechsel mit ungesicherten Änderungen fragt nach.
    for(auto w:h.view.flatten(false)) if(auto r=dynamic_cast<RailItem*>(w)) if(r->text.de==L"Seiten") r->activate(h.view);
    auto pages=h.find<ItemList>(L"Seitenliste"); auto choice=h.find<Combo>(L"Seite"); CHECK(pages && choice && !pages->checkable && !pages->reorder); if(!pages || !choice) return;
    CHECK(pages->items.size()==choice->options.size()); CHECK(pages->items[size_t(pages->selected)].detail.de==L"Eingebaut" && pages->items[0].detail.en==L"Built-in");
    CHECK(pages->selected==choice->index);
    auto edited=h.find<GridEditor>()->model; CHECK(plugins::edit::resize_axis(edited,false,1)); h.find<GridEditor>()->on_edit(edited,{});
    const int before=pages->selected, other=before==0?1:0; const int asks=h.confirmations; h.confirm_answer=false;
    pages->select(other,h.view); CHECK(h.confirmations==asks+1); CHECK(h.question.de.find(L"verwerfen")!=std::wstring::npos);
    pages=h.find<ItemList>(L"Seitenliste"); CHECK(pages->selected==before); CHECK(h.find<Combo>(L"Seite")->index==before);
    h.confirm_answer=true; pages->select(other,h.view); settle(h,*page);
    CHECK(h.find<ItemList>(L"Seitenliste")->selected==other); CHECK(h.find<Combo>(L"Seite")->index==other); CHECK(!h.find<Button>(L"Rückgängig")->enabled);
    // Bereichszustand überlebt den Seitenwechsel (Seiten bleibt offen).
    CHECK(h.find<EditorShell>()->is_open(EditorShell::PageList));
    h.view.set_root(nullptr);
}
// P7b-5 vorgezogen: Editor-Snapshots (additiv; eigene Hashdatei, nie per update überschrieben) und Vorschau-PNGs.
static void editor_snapshot_tests(const fs::path& root,const fs::path& baseline,const fs::path& images,bool record) {
    const auto file=baseline/"p7b-editor-hashes.txt";
    std::map<std::string,std::string> expected,actual;
    if(!record) { std::ifstream in(file); CHECK(bool(in)); for(std::string name,hash;in>>name>>hash;) expected[name]=hash; }
    else if(fs::exists(file)) { std::printf("Editor-Hashdatei existiert; Überschreiben verboten.\n"); CHECK(false); return; }
    Kit kit; kit.use_design({{"theme",theme::DEFAULT_ID}}); kit.use_windows(false); kit.dark=true; kit.english=false;
    const auto& th=kit.theme();
    // Kontraste im Standard-Theme (dunkles Material mit Orange): Text >= 4,5:1, Fokusring und Grenzen >= 3:1.
    CHECK(contrast_of(th.text,th.card)>=4.5); CHECK(contrast_of(th.text,th.bg)>=4.5); CHECK(contrast_of(th.muted,th.card)>=4.5);
    CHECK(contrast_of(focus_color(th),th.card)>=3.0); CHECK(contrast_of(focus_color(th),th.bg)>=3.0); CHECK(contrast_of(th.danger,th.bg)>=3.0);
    CHECK(contrast_of(th.on_accent,th.accent)>=4.5);
    // Alle Themes (auch dunkel und eigene): Kachelzustand (muted auf Karte), Vorschautext auf Vorschaufläche, Rahmen/Fokus/Ungültig.
    for(auto& tc:snapshot::theme_cases()) {
        Kit other; other.dark=tc.name=="dunkel"; ojson dp={{"theme",tc.name}}; if(!tc.inline_theme.is_null()) dp["theme_inline"]=tc.inline_theme;
        other.use_design(dp);
        if(tc.name=="windows") { theme::WindowsSettings w; w.has_accent=true; other.use_windows(true,&w); } else other.use_windows(false);
        const auto& o=other.theme();
        CHECK(contrast_of(o.text,o.card)>=4.5); CHECK(contrast_of(o.text,o.bg)>=4.5); CHECK(contrast_of(o.muted,o.card)>=4.5);
        CHECK(contrast_of(rgb(235,235,235),o.preview_bg)>=4.5);
        CHECK(contrast_of(focus_color(o),o.card)>=3.0); CHECK(contrast_of(focus_color(o),o.bg)>=3.0); CHECK(contrast_of(o.danger,o.bg)>=3.0);
    }
    auto save=[&](const std::string& key,const Image& image) {
        actual[key]=snapshot::hash_image(image); if(!record) CHECK(expected[key]==actual[key]);
        fs::create_directories(images); snapshot::write_png(images/(key+".png"),image);
    };
    auto setup=[&](TestHost& h,const std::string& key) {
        builtin_actions(h); h.actions.editor_intro_seen=[] { return true; };  // Einsteigerkarte ist für die Bilder bereits weggeklickt
        auto page=make_layout_page(); page->build(h,h.root); page->shown(h); settle(h,*page); CHECK(choose_page(h,*page,key)); return page;
    };
    const int W=960,H=640;
    {   // 1: System, Standardanordnung, Zone 1 gewählt.
        TestHost h(root/"snap-system"); auto page=setup(h,"system"); TreeCanvas canvas(kit,W,H);
        auto grid=h.find<GridEditor>(); grid->selected=1; grid->column=1; grid->row=0; grid->focused_block=1; grid->on_select();  // GPU gewählt, Eigenschaften folgen
        layout_tree(kit,canvas,h.root); save("material-orange.editor-system",paint_tree(kit,canvas,h.root));
        h.view.set_root(nullptr);
    }
    {   // 2: freie Zelle und Kachel im Zug mit gültigem Ziel (Geist „Ablegen“).
        TestHost h(root/"snap-drag"); auto page=setup(h,"system"); TreeCanvas canvas(kit,W,H);
        auto grid=h.find<GridEditor>(); auto model=grid->model; CHECK(plugins::edit::remove_block(model,4)); CHECK(plugins::edit::delete_zone(model,4));
        grid->on_edit(model,{}); settle(h,*page); grid=h.find<GridEditor>(); auto palette=h.find<TilePalette>();
        layout_tree(kit,canvas,h.root);
        auto tile=palette->tile_rect(4); Point from{(tile.left+tile.right)/2,(tile.top+tile.bottom)/2};
        auto g=grid->grid_rect(); const float cw=(g.right-g.left)/2,ch=(g.bottom-g.top)/3; Point to{g.left+.5f*cw,g.top+2.5f*ch};
        grid->start_external(4,from,h.view); grid->drag({from.x+30,from.y},h.view); grid->drag(to,h.view);
        CHECK(grid->carrying()); CHECK(grid->target_valid());
        auto valid=paint_tree(kit,canvas,h.root); save("material-orange.editor-drag",valid);
        // Ungültiger Geist darf sich vom gültigen unterscheiden; Papierkorb hervorgehoben ebenfalls.
        auto trash=grid->trash_rect(); grid->drag({(trash.left+trash.right)/2,(trash.top+trash.bottom)/2},h.view);
        auto over_trash=paint_tree(kit,canvas,h.root); CHECK(snapshot::hash_image(over_trash)!=snapshot::hash_image(valid));
        actual["material-orange.editor-drag-trash"]=snapshot::hash_image(over_trash); if(!record) CHECK(expected["material-orange.editor-drag-trash"]==actual["material-orange.editor-drag-trash"]);
        grid->cancel(h.view); h.view.set_root(nullptr);
    }
    {   // 3: Spotify mit geöffneter Seitenliste, Suche und Filter.
        TestHost h(root/"snap-spotify"); auto page=setup(h,"spotify"); TreeCanvas canvas(kit,W,H);
        auto shell=h.find<EditorShell>(); shell->rail[EditorShell::PageList]->activate(h.view); shell->rail[EditorShell::Layers]->activate(h.view);
        h.find<Button>(L"Medien")->activate(h.view);
        layout_tree(kit,canvas,h.root); save("material-orange.editor-spotify",paint_tree(kit,canvas,h.root));
        h.view.set_root(nullptr);
    }
    if(record) { fs::create_directories(baseline); std::ofstream out(file); for(auto& [key,hash]:actual) out<<key<<' '<<hash<<'\n'; CHECK(bool(out)); }
    else CHECK(expected.size()==actual.size());
}
// P8-2: exercise the actual page with isolated files, synthetic devices and real source sample renderers.
static void custom_editor_tests(const fs::path& root,const fs::path& images,bool record) {
    const auto baseline=fs::path(FL_LAYOUT_SNAPSHOT_DIR)/"p9-custom-editor-hashes.txt";
    std::map<std::string,std::string> expected,actual;
    if(record) {CHECK(!fs::exists(baseline));if(fs::exists(baseline))return;}
    else {std::ifstream in(baseline);CHECK(bool(in));for(std::string key,hash;in>>key>>hash;)expected[key]=hash;}
    auto cfg=offline_config();cfg["homeassistant"]["entities"]={"sensor.temperature|Example"};
    TestHost h(root/"custom-editor",cfg);h.confirm_answer=true;
    bool connected=true;
    h.actions.layout_displays=[&]{return Json::array({
        {{"id","serial:COM7"},{"name","COM7"},{"width",480},{"height",320},{"usable_width",480},{"usable_height",320},{"online",true}},
        {{"id","monitor:sample"},{"name","Monitor"},{"width",1920},{"height",1080},{"usable_width",1920},{"usable_height",1080},{"online",connected}},
        {{"id","offline-phone"},{"name","Offline Phone"},{"width",320},{"height",480},{"online",false}}});};
    custom::View initial;initial.id="dashboard";initial.name="Meine Übersicht";initial.standard.columns={1,1,1};initial.standard.rows={1,1};
    initial.standard.tiles={custom::Tile{"system","system.cpu",{"cpu",0,0,1,1}},custom::Tile{"spotify","spotify.text",{"song",1,0,2,1}},custom::Tile{"homeassistant","sensor.temperature",{"ha",0,1,1,1}}};
    initial.devices["offline-phone"]=initial.standard;
    custom::Store saved(h.dir/"views");CHECK(saved.save(initial).empty());
    auto page=make_custom_views_page();page->build(h,h.root);page->shown(h);settle(h,*page);
    auto grid=h.find<GridEditor>();auto palette=h.find<TilePalette>();CHECK(grid && palette);if(!grid || !palette)return;
    CHECK(grid->model.custom_draft);CHECK(custom::editor_layout(grid->model)->tiles.size()==3);
    CHECK(palette->visible().size()==5);CHECK(palette->thumb_of(palette->tiles.front().id));
    CHECK(!h.find<Button>(L"Offline Phone · 320 × 480"));
    auto monitor=h.find<Button>(L"Monitor · 1920 × 1080");CHECK(monitor && monitor->role()==Role::NavTab);if(!monitor)return;
    monitor->activate(h.view);settle(h,*page);CHECK(h.find<Button>(L"Monitor · 1920 × 1080")->checked());CHECK(std::abs(grid->display_aspect-1920.f/1080)<.001f);
    CHECK(grid->insert_first_free(3,h.view));settle(h,*page);CHECK(h.find<Button>(L"Rückgängig")->enabled);
    const auto placed=custom::editor_layout(grid->model)->tiles.size();CHECK(placed==4);
    h.find<Button>(L"Rückgängig")->activate(h.view);settle(h,*page);CHECK(custom::editor_layout(grid->model)->tiles.size()==3);
    h.find<Button>(L"Wiederholen")->activate(h.view);settle(h,*page);CHECK(custom::editor_layout(grid->model)->tiles.size()==4);
    h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);CHECK(saved.get(initial.id)->devices.at("monitor:sample").tiles.size()==4);
    CHECK(saved.get(initial.id)->devices.at("offline-phone").tiles.size()==3);
    CHECK(h.application.active_pages()==std::vector<std::string>{"custom:dashboard"});
    // Persisted inheritance and a vanished selection return to the standard tab without losing offline overlays.
    h.find<Button>(L"Von Standard erben")->activate(h.view);settle(h,*page);CHECK(!saved.get(initial.id)->devices.contains("monitor:sample"));
    connected=false;Sleep(2100);settle(h,*page);
    CHECK(!h.find<Button>(L"Monitor · 1920 × 1080"));CHECK(h.find<Button>(L"Standard")->checked());CHECK(saved.get(initial.id)->devices.contains("offline-phone"));
    connected=true;Sleep(2100);settle(h,*page);
    grid=h.find<GridEditor>();grid->selected=0;grid->focused_block=0;grid->on_select();
    // Property edits and validation use the strict custom-view parser.
    auto condition=h.find<TextField>(L"Anzeigen wenn: data.…");CHECK(condition);if(condition){condition->set_text(L"data.cpu_pct");condition->on_change();h.find<Button>(L"Bedingung bestätigen")->activate(h.view);settle(h,*page);CHECK(custom::editor_layout(grid->model)->tiles[0].show_if.has_value());}
    auto density=h.find<Combo>(L"Dichte (höchstens)");CHECK(density);if(density){density->index=0;density->on_change();settle(h,*page);CHECK(custom::editor_layout(grid->model)->tiles[0].density==0);}
    auto name=h.find<TextField>(L"Name der Ansicht");CHECK(name);if(name){name->set_text(L"Übersicht P8");name->on_change();name->on_commit();}
    h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);CHECK(saved.get(initial.id)->name=="Übersicht P8");
    // Additive DE/EN snapshots of the real page, including palette, grid, properties and two active-device tabs.
    fs::create_directories(images);
    grid->selected=0;grid->focused_block=0;grid->on_select();
    TreeCanvas custom_canvas(h.kit,1440,1100);
    auto capture=[&](const std::string& stem){
        for(bool en:{false,true}){
            h.kit.english=en;h.kit.use_design({{"theme",theme::DEFAULT_ID}});h.kit.use_windows(false);h.kit.dark=true;
            layout_tree(h.kit,custom_canvas,h.root);auto image=paint_tree(h.kit,custom_canvas,h.root,true);
            const std::string key=stem+(en?".en":".de");actual[key]=snapshot::hash_image(image);if(!record)CHECK(expected[key]==actual[key]);
            snapshot::write_png(images/(key+".png"),image);
            CHECK(fs::file_size(images/(key+".png"))>1000);
        }
        h.kit.english=false;
    };
    capture("custom-editor");
    // P8-3: touch target per tile and per display overlay (Standard / Individual).
    {
        auto mode=h.find<Combo>(L"Touch-Ziel");auto action=h.find<Combo>(L"Aktion");auto target=h.find<Combo>(L"Zielseite");auto link=h.find<TextField>(L"HTTPS-Link");
        CHECK(mode && action && target && link);
        if(mode && action && target && link) {
            auto tapped=[&]{size_t n=0;auto layout=custom::editor_layout(grid->model);if(layout)for(const auto& t:layout->tiles)if(t.tap)++n;return n;};
            auto tap_page=[&]()->std::string{auto layout=custom::editor_layout(grid->model);if(layout)for(const auto& t:layout->tiles)if(t.tap)return t.tap->page;return {};};
            auto select_first=[&]{grid=h.find<GridEditor>();grid->selected=0;grid->focused_block=0;grid->on_select();settle(h,*page);};
            CHECK(mode->options.size()==2 && mode->options[0].de==L"Standard" && mode->options[1].de==L"Individuell");
            CHECK(target->options.size()>=config::PAGES.size() + 1);   // fixed pages plus custom views
            CHECK(mode->enabled && mode->index==0 && !action->enabled && !target->enabled && !link->enabled && tapped()==0);
            mode->index=1;mode->on_change();settle(h,*page);
            CHECK(action->enabled && target->enabled && !link->enabled);   // page action; link/next only for plugin sources
            CHECK(tapped()==0);                                              // no page chosen yet: Standard
            target->index=1;target->on_change();settle(h,*page);CHECK(tapped()==1 && tap_page()=="friends");
            action->index=2;action->on_change();settle(h,*page);CHECK(tapped()==0);   // next on a system tile: not allowed, falls back to Standard
            action->index=1;action->on_change();settle(h,*page);CHECK(tapped()==0);   // link on a system tile: same
            action->index=0;action->on_change();settle(h,*page);CHECK(tapped()==1 && tap_page()=="friends");
            // Apply persists it in the standard layout only; the stored offline overlay is untouched.
            h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);
            auto count_taps=[](const custom::Layout& l){size_t n=0;for(const auto& t:l.tiles)if(t.tap)++n;return n;};
            auto stored=saved.get(initial.id);CHECK(stored && count_taps(stored->standard)==1 && count_taps(stored->devices.at("offline-phone"))==0);
            select_first();CHECK(mode->index==1 && action->index==0 && target->index==1);
            capture("custom-editor-touch");
            // The same tile gets its own target in the Monitor overlay (own layout); Standard keeps its target.
            h.find<Button>(L"Monitor · 1920 × 1080")->activate(h.view);settle(h,*page);select_first();
            CHECK(mode->index==0 && tapped()==0);   // this overlay is its own layout (kept draft from the earlier disconnect): no target yet
            mode->index=1;mode->on_change();settle(h,*page);
            target->index=4;target->on_change();settle(h,*page);CHECK(tap_page()=="rss");
            h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);
            stored=saved.get(initial.id);CHECK(stored && count_taps(stored->devices.at("monitor:sample"))==1 && count_taps(stored->standard)==1);
            for(const auto& t:stored->devices.at("monitor:sample").tiles)if(t.tap)CHECK(t.tap->page=="rss");
            for(const auto& t:stored->standard.tiles)if(t.tap)CHECK(t.tap->page=="friends");
            // Back to Standard in the Standard tab removes the key from the file again.
            h.find<Button>(L"Standard")->activate(h.view);settle(h,*page);select_first();
            mode->index=0;mode->on_change();settle(h,*page);CHECK(tapped()==0);
            h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);
            stored=saved.get(initial.id);CHECK(stored && count_taps(stored->standard)==0 && count_taps(stored->devices.at("monitor:sample"))==1);
            select_first();
        }
    }
    if(record){std::ofstream out(baseline);for(const auto& [key,hash]:actual)out<<key<<' '<<hash<<'\n';CHECK(bool(out));}else CHECK(expected.size()==actual.size());
    h.kit.english=false;
    const auto views_before=h.find<ItemList>(L"Ansichten")->items.size();h.find<Button>(L"+ Neue Ansicht")->activate(h.view);settle(h,*page);CHECK(h.find<ItemList>(L"Ansichten")->items.size()==views_before+1);
    auto keyboard=h.find<GridEditor>();keyboard->column=1;keyboard->row=0;const auto cpu=h.find<TilePalette>()->visible().front();
    CHECK(keyboard->pickup(cpu,h.view));CHECK(keyboard->carrying());CHECK(keyboard->value(false).find(L"System · CPU")!=std::wstring::npos);CHECK(keyboard->input(VK_ESCAPE));CHECK(custom::editor_layout(keyboard->model)->tiles.empty());
    CHECK(keyboard->pickup(cpu,h.view));CHECK(keyboard->input(VK_RETURN));settle(h,*page);CHECK(custom::editor_layout(keyboard->model)->tiles.size()==1);
    CHECK(keyboard->input('Z',false,false,true));settle(h,*page);CHECK(custom::editor_layout(keyboard->model)->tiles.empty());
    CHECK(keyboard->input('Y',false,false,true));settle(h,*page);CHECK(custom::editor_layout(keyboard->model)->tiles.size()==1);
    CHECK(keyboard->input(VK_F6));CHECK(keyboard->area==2);CHECK(keyboard->input(VK_F6));CHECK(keyboard->area==0);
    h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);CHECK(saved.list().size()==2);
    h.find<Button>(L"Löschen")->activate(h.view);settle(h,*page);CHECK(saved.list().size()==1);CHECK(saved.get(initial.id));
    auto game_button=h.find<Button>(L"+ Game-Mode");CHECK(game_button);if(game_button){
        game_button->activate(h.view);settle(h,*page);auto game_grid=h.find<GridEditor>();CHECK(game_grid);
        if(game_grid){auto draft=custom::editor_layout(game_grid->model);CHECK(draft && draft->tiles.size()==8);}
        auto filter=h.find<Combo>(L"Quelle");auto palette=h.find<TilePalette>();CHECK(filter && palette);
        if(filter && palette){auto option=std::find_if(filter->options.begin(),filter->options.end(),[](const auto& text){return text.de==L"Game-Mode";});CHECK(option!=filter->options.end());if(option!=filter->options.end()){filter->index=int(option-filter->options.begin());filter->on_change();settle(h,*page);CHECK(palette->category=="game");CHECK(palette->visible().size()==16);}}
        h.find<Button>(L"Übernehmen")->activate(h.view);settle(h,*page);CHECK(saved.list().size()==2);
    }
    h.view.set_root(nullptr);
}
// P8-3: "Arrange page" is gone as a settings entry; its editor stays reachable from the page list of Custom views.
static void custom_standard_pages_tests(const fs::path& root) {
    for(const auto& info_page:create_pages()) CHECK(info_page->info().id!="layout");
    TestHost h(root/"custom-standard");pin_window(h);builtin_actions(h);h.actions.editor_intro_seen=[]{return true;};h.confirm_answer=true;
    auto page=make_custom_views_page();page->build(h,h.root);page->shown(h);settle(h,*page);
    auto list=h.find<ItemList>(L"Ansichten");CHECK(list);if(!list)return;
    CHECK(!h.find<Combo>(L"Seite"));
    std::vector<size_t> standard;for(size_t i=0;i<list->items.size();++i)if(list->items[i].detail.de==L"Standard / Plugin")standard.push_back(i);
    size_t enabled_builtin=0;for(const auto& c:builtin::catalogs())if(c.enabled && c.key!="ai")++enabled_builtin;
    CHECK(enabled_builtin>0 && standard.size()>=enabled_builtin);   // nothing lost: every enabled built-in page is listed
    // The old page editor opens for each entry, preselected, and returns to the custom view list.
    for(auto index:standard) {
        const auto key=list->items[index].key;const auto title=list->items[index].title.de;
        list->on_select(int(index));settle(h,*page);
        auto choice=h.find<Combo>(L"Seite");CHECK(choice);
        if(choice) {CHECK(choice->index>=0 && choice->index<int(choice->options.size()));if(choice->index>=0 && choice->index<int(choice->options.size()))CHECK(choice->options[choice->index].de==(key.starts_with("builtin:")?L"Eingebaut · ":L"Plugins · ")+title);}
        auto back=h.find<Button>(L"Zurück zu Eigene Ansichten");CHECK(back);
        if(!back)return;
        back->activate(h.view);settle(h,*page);
        CHECK(!h.find<Combo>(L"Seite"));CHECK(h.find<Button>(L"+ Neue Ansicht"));CHECK(h.find<ItemList>(L"Ansichten"));
        list=h.find<ItemList>(L"Ansichten");if(!list)return;
    }
    h.view.set_root(nullptr);
}
static void ai_custom_editor_tests(const fs::path& root,const fs::path& images,bool record) {
    const auto baseline=fs::path(FL_LAYOUT_SNAPSHOT_DIR)/"p9-ai-editor-hashes.txt";
    std::map<std::string,std::string> expected,actual;
    if(record) {CHECK(!fs::exists(baseline));if(fs::exists(baseline))return;}
    else {std::ifstream in(baseline);CHECK(bool(in));for(std::string key,hash;in>>key>>hash;)expected[key]=hash;}
    TestHost h(root/"ai-editor");h.confirm_answer=true;
    h.actions.layout_displays=[] {return Json::array({
        {{"id","serial:COM7"},{"name","COM7"},{"width",480},{"height",320},{"usable_width",480},{"usable_height",320},{"online",true}},
        {{"id","monitor:sample"},{"name","Monitor"},{"width",1920},{"height",1080},{"usable_width",1920},{"usable_height",1080},{"online",true}}});};
    custom::View view;view.id="sample-ai";view.name="KI / AI";view.standard.columns={1,1};view.standard.rows={2,1};
    view.standard.tiles={custom::Tile{"ai","ai.claude",{"claude",0,0,1,1}},custom::Tile{"ai","ai.codex",{"codex",1,0,1,1}},custom::Tile{"ai","ai.limits",{"limits",0,1,2,1}}};
    view.standard.tiles[0].tap=custom::Tap{"page","custom:sample-ai",""};
    custom::Store store(h.dir/"views");CHECK(store.save(view).empty());
    auto page=make_custom_views_page();page->build(h,h.root);page->shown(h);settle(h,*page);
    auto grid=h.find<GridEditor>();auto palette=h.find<TilePalette>();auto filter=h.find<Combo>(L"Quelle");CHECK(grid && palette && filter);if(!grid || !palette || !filter)return;
    auto ai_option=std::find_if(filter->options.begin(),filter->options.end(),[](const auto& option){return option.de==L"KI-Status";});
    CHECK(ai_option!=filter->options.end());if(ai_option==filter->options.end())return;filter->index=int(ai_option-filter->options.begin());filter->on_change();settle(h,*page);
    CHECK(palette->category=="ai");CHECK(palette->tiles.size()>=11);
    grid->selected=0;grid->focused_block=0;grid->on_select();settle(h,*page);
    CHECK(custom::editor_layout(grid->model)->tiles.size()==3);CHECK(h.find<Combo>(L"Touch-Ziel")->index==1);
    CHECK(h.find<Button>(L"COM7 · 480 × 320"));CHECK(h.find<Button>(L"Monitor · 1920 × 1080"));
    fs::create_directories(images);TreeCanvas canvas(h.kit,1440,1100);
    for(bool en:{false,true}) {
        h.kit.english=en;h.kit.use_design({{"theme",theme::DEFAULT_ID}});h.kit.use_windows(false);h.kit.dark=true;
        layout_tree(h.kit,canvas,h.root);auto image=paint_tree(h.kit,canvas,h.root,true);
        const std::string key=en?"custom-editor-ai.en":"custom-editor-ai.de";actual[key]=snapshot::hash_image(image);
        if(!record)CHECK(expected[key]==actual[key]);snapshot::write_png(images/(key+".png"),image);
        CHECK(fs::file_size(images/(key+".png"))>1000);
    }
    if(record){std::ofstream out(baseline);for(const auto& [key,hash]:actual)out<<key<<' '<<hash<<'\n';CHECK(bool(out));}else CHECK(expected.size()==actual.size());
    h.view.set_root(nullptr);
}
// P9: actual settings form and Game-Mode palette/template, anonymous sample data.
static void game_mode_ui_tests(const fs::path& root,const fs::path& images,bool record) {
    perflog::set_test_override(perflog::Access::NotMember);  // FPS card text must not depend on this machine
    const auto baseline=fs::path(FL_LAYOUT_SNAPSHOT_DIR)/"p9-game-mode-hashes.txt";
    std::map<std::string,std::string> expected,actual;
    if(record){CHECK(!fs::exists(baseline));if(fs::exists(baseline))return;}
    else {std::ifstream in(baseline);CHECK(bool(in));for(std::string key,hash;in>>key>>hash;)expected[key]=hash;}
    fs::create_directories(images);
    auto capture=[&](TestHost& host,const std::string& stem){TreeCanvas canvas(host.kit,1440,1100);for(bool en:{false,true}){
        host.kit.english=en;host.kit.use_design({{"theme",theme::DEFAULT_ID}});host.kit.use_windows(false);host.kit.dark=true;
        layout_tree(host.kit,canvas,host.root);auto image=paint_tree(host.kit,canvas,host.root,true);auto key=stem+(en?".en":".de");actual[key]=snapshot::hash_image(image);
        if(!record)CHECK(expected[key]==actual[key]);snapshot::write_png(images/(key+".png"),image);
    }host.kit.english=false;};
    {
        auto cfg=offline_config();cfg["game_mode"]["view"]="custom:example-game";
        TestHost h(root/"game-settings",cfg);h.actions.layout_displays=[] {return Json::array({{{"id","monitor:sample"},{"name","Example display"},{"online",true}}});};
        CHECK(custom::Store(h.dir/"views").save(custom::game_mode_template("example-game")).empty());
        auto page=make_game_mode_page();CHECK(page->info().id=="game_mode");page->build(h,h.root);page->shown(h);settle(h,*page);
        CHECK(h.find<Combo>(L"Ziel-Display"));CHECK(h.find<Combo>(L"Eigene Ansicht"));
        auto additional=h.find<TextField>(L"Zusätzliche Spiele");CHECK(additional && additional->kind==TextField::Multi);
        if(additional)additional->set_text(L"C:\\Example\\game.exe\r\nD:\\Example\\other.exe");
        ojson patch;page->collect(h,patch);auto merged=config::merge(cfg,patch);CHECK(merged["game_mode"]["view"]=="custom:example-game");
        CHECK(merged["game_mode"]["additional_games"].get<std::string>().find('\n')!=std::string::npos);
        capture(h,"game-mode-settings");h.view.set_root(nullptr);
    }
    {
        TestHost h(root/"game-palette");CHECK(custom::Store(h.dir/"views").save(custom::game_mode_template("example-game")).empty());
        auto page=make_custom_views_page();page->build(h,h.root);page->shown(h);settle(h,*page);
        auto filter=h.find<Combo>(L"Quelle");auto palette=h.find<TilePalette>();CHECK(filter && palette);
        if(filter && palette){auto it=std::find_if(filter->options.begin(),filter->options.end(),[](const auto& option){return option.de==L"Game-Mode";});CHECK(it!=filter->options.end());if(it!=filter->options.end()){filter->index=int(it-filter->options.begin());filter->on_change();settle(h,*page);CHECK(palette->visible().size()==16);}}
        capture(h,"game-mode-editor");h.view.set_root(nullptr);
    }
    // Actual rendered template, in addition to settings/editor chrome.
    {
        auto cfg=offline_config();cfg["display"]["theme"]="material-orange";
        App app(cfg,root/"game-render.toml",nullptr,root/"preview");
        for(bool en:{false,true})for(auto size:{std::pair{480,320},std::pair{1520,720}}) {
            i18n::set_lang(en?"en":"de");
            auto image=app.custom_preview(custom::game_mode_template("example-game"),"",size);CHECK(image);
            if(image){auto key=std::string("game-mode-render.")+(en?"en":"de")+"."+std::to_string(size.first);actual[key]=snapshot::hash_image(*image);if(!record)CHECK(expected[key]==actual[key]);snapshot::write_png(images/(key+".png"),*image);}
        }
        i18n::set_lang("de");
    }
    if(record){std::ofstream out(baseline);for(const auto& [key,hash]:actual)out<<key<<' '<<hash<<'\n';CHECK(bool(out));}else CHECK(expected.size()==actual.size());
}

// Explicit developer export, separate from golden recording and normal tests.
// Uses the real native page/widget renderer without opening or activating a window.
static int export_store_listing(const fs::path& output) {
    if(fs::exists(output) && !fs::is_empty(output))throw std::runtime_error("Use a fresh screenshot directory.");
    fs::create_directories(output);
    const auto root=fs::temp_directory_path()/("fl-store-listing-"+std::to_string(GetCurrentProcessId()));
    if(fs::exists(root))throw std::runtime_error("Screenshot fixture directory already exists.");
    struct Cleanup { fs::path path; ~Cleanup(){std::error_code ec;fs::remove_all(path,ec);} } cleanup{root};
    fs::create_directories(root);
    perflog::set_test_override(perflog::Access::NotMember);
    ojson report={{"appVersion",VERSION},{"packaged",distribution::packaged()},
                  {"capture","Native Page/TreeCanvas renderer, isolated synthetic fixtures, no window/desktop capture"},
                  {"sampleDataOnly",true},{"hardwareEvidence",false},{"images",ojson::array()}};
    if(distribution::packaged()) {
        TestHost host(root/"help-check");auto page=make_app_page();page->build(host,host.root);
        auto privacy=host.find<Button>(L"Datenschutzerklärung");auto support=host.find<Button>(L"Support");
        if(!privacy || !support || privacy->text.get(true)!=L"Privacy policy" || !privacy->on_click || !support->on_click)
            throw std::runtime_error("Packaged privacy/support controls are missing.");
        report["privacySupportControls"]=true;
        host.view.set_root(nullptr);
    }
    for(bool english:{false,true})for(bool editor:{false,true}) {
        i18n::set_lang(english?"en":"de");
        auto cfg=offline_config();cfg["ai"]["enabled"]=false;cfg["ui"]["plugin_store"]=false;
        cfg["ui"]["language"]=english?"en":"de";cfg["game_mode"]["view"]="custom:store-example-game";
        const std::string name=std::string(english?"en-":"de-")+(editor?"editor":"game-mode")+".png";
        TestHost host(root/name,cfg);
        host.kit.english=english;host.kit.use_design({{"theme",theme::DEFAULT_ID}});host.kit.use_windows(false);host.kit.dark=true;
        host.actions.layout_displays=[] {return Json::array({{{"id","monitor:sample"},{"name","Example monitor"},{"width",1920},{"height",1080},{"usable_width",1920},{"usable_height",1080},{"online",true}}});};
        auto error=custom::Store(host.dir/"views").save(custom::game_mode_template("store-example-game"));
        if(!error.empty())throw std::runtime_error(error);
        auto page=editor?make_custom_views_page():make_game_mode_page();
        page->build(host,host.root);page->shown(host);settle(host,*page);
        TreeCanvas canvas(host.kit,1600,1200);layout_tree(host.kit,canvas,host.root);
        auto rendered=paint_tree(host.kit,canvas,host.root,true);
        snapshot::write_png(output/name,rendered);
        report["images"].push_back({{"file",name},{"language",english?"en":"de"},{"width",rendered.w},{"height",rendered.h}});
        page->hidden(host);host.view.set_root(nullptr);
    }
    i18n::set_lang("de");
    std::ofstream(output/"capture-report.json")<<report.dump(2);
    std::printf("Exported four native sample renders; packaged=%s\n",distribution::packaged()?"true":"false");
    return 0;
}

int main(int argc,char** argv) {
    if(argc==3 && std::string(argv[1])=="--export-store-listing") {
        try {gfx_init();return export_store_listing(fs::path(argv[2]));}
        catch(const std::exception& e){std::fprintf(stderr,"Store sample export: %s\n",e.what());return 1;}
    }
    setvbuf(stdout,nullptr,_IONBF,0);
    gfx_init(); SetConsoleOutputCP(CP_UTF8); capture_tests();
    if(argc==2 && std::string(argv[1])=="--capture-only") { std::printf("Capture: %d passed, %d failed\n",passed,failed); return failed?1:0; }
    gesture_tests();
    const auto dir=fs::temp_directory_path()/("fl-layout-ui-"+std::to_string(GetCurrentProcessId()));
    builtin_infrastructure_tests(dir);
    device_render_tests(dir);device_editor_tests(dir);
    friends_store_tests(dir);
    editor_shell_tests(dir); tile_palette_tests(dir); canvas_tests(dir); layers_pages_intro_tests(dir); plugin_palette_tests(dir);
    for(const auto& catalog:builtin::catalogs()) if(catalog.enabled) builtin_editor_tests(dir,catalog);
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ec; fs::remove_all(path,ec); } } cleanup{dir};
    fs::create_directories(dir);
    {
        TestHost h(dir); CHECK(h.store.load()); CHECK(h.store.import_manifest(fixture().dump()));
        auto page=make_layout_page(); page->build(h,h.root); page->shown(h); h.drain(); page->tick(h,h.state); h.drain(); page->tick(h,h.state);
        auto grid=h.find<GridEditor>(); CHECK(grid); auto blocks=h.find<LayoutBlockList>(); CHECK(blocks);
        if(!grid || !blocks) return 1;
        CHECK(count_dirty(&h.root)==0);
        { auto palette=h.find<TilePalette>(); CHECK(palette && palette->tiles.size()==2 && palette->tiles[0].id=="a" && palette->tiles[1].id=="b");  // Plugin-Seite: Einzelzonen-Vorschau
          if(palette) for(const char* id:{"a","b"}) { auto thumb=palette->thumb_of(id); CHECK(thumb && thumb->image && thumb->image->w>0); } }
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
        CHECK(theme::contrast(color(focus_color(th)),color(th.card))>=3.0); CHECK(theme::contrast(color(focus_color(th)),color(th.bg))>=3.0); // Fokusring, Auswahl, Griffe
        CHECK(theme::contrast(color(th.danger),color(th.bg))>=3.0);          // gestrichelter ungültiger Geist (zusätzlich Text)
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
    editor_snapshot_tests(dir,baseline,fs::path(argv[0]).parent_path()/"p7b-ui-snapshots",argc==2 && std::string(argv[1])=="record-editor");
    custom_editor_tests(dir,fs::path(argv[0]).parent_path()/"p8-ui-snapshots",argc==2 && std::string(argv[1])=="record-game-editor");
    ai_custom_editor_tests(dir,fs::path(argv[0]).parent_path()/"p8-ui-snapshots",argc==2 && std::string(argv[1])=="record-game-editor");
    custom_standard_pages_tests(dir);
    game_mode_ui_tests(dir,fs::path(argv[0]).parent_path()/"p9-ui-snapshots",argc==2 && std::string(argv[1])=="record-game-settings");
    std::error_code ec; fs::remove_all(dir,ec); CHECK(!ec && !fs::exists(dir));
    std::printf("Layout UI: %d passed, %d failed\n",passed,failed); return failed?1:0;
}
