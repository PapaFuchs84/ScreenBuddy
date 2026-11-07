// © 2026 FuchsLabs – bounded local view documents.
#include "custom_view.h"
#include "render.h"
#include "plugin_render.h"
#include "config.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <set>
namespace fl::custom {
namespace {
namespace fs=std::filesystem;
bool plain_path(const fs::path& path) {
    // Symlinks, Junctions und andere Reparse-Punkte einschließlich Eltern ablehnen.
    for (auto current = path; !current.empty();) {
        std::error_code ec;
        auto status = fs::symlink_status(current, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) return false;
        if (!ec && fs::is_symlink(status)) return false;
#ifdef _WIN32
        const auto attr = GetFileAttributesW(current.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return true;
}
bool regular_or_missing(const fs::path& path) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    auto status = fs::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory) return true;
    return !ec && (status.type() == fs::file_type::not_found || fs::is_regular_file(status));
}
bool ready_directory(const fs::path& path, bool create) {
    if (!plain_path(path)) return false;
    std::error_code ec;
    if (create) fs::create_directories(path, ec);
    if (ec) return false;
    return fs::is_directory(path, ec) && !ec && plain_path(path);
}
bool read_bounded(const fs::path& path, std::string& out, size_t limit = plugins::manifest_limit) {
    if (!regular_or_missing(path)) return false;
#ifdef _WIN32
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    bool ok = GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow <= limit;
    if (ok) {
        out.resize(info.nFileSizeLow);
        DWORD bytes = 0;
        ok = ReadFile(handle, out.data(), static_cast<DWORD>(out.size()), &bytes, nullptr) && bytes == out.size();
    }
    CloseHandle(handle);
    return ok;
#else
    int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat info{};
    bool ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size >= 0 &&
        static_cast<uint64_t>(info.st_size) <= limit;
    if (ok) {
        out.resize(static_cast<size_t>(info.st_size));
        size_t total = 0;
        while (total < out.size()) {
            auto n = read(fd, out.data() + total, out.size() - total);
            if (n <= 0) { ok = false; break; }
            total += static_cast<size_t>(n);
        }
    }
    close(fd);
    return ok;
#endif
}
bool atomic_write(const fs::path& target, std::string_view data) {
    if (!regular_or_missing(target) || !ready_directory(target.parent_path(), false)) return false;
    static std::atomic<uint64_t> sequence{0};
    // Exklusiv anlegen: keine bestehenden Links verfolgen oder temporäre Dateien überschreiben.
    const auto token = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path temporary = target;
    temporary += ".tmp-" + std::to_string(token) + "-" + std::to_string(++sequence);
#ifdef _WIN32
    HANDLE handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0;
    bool ok = WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &count, nullptr) && count == data.size();
    if (ok) ok = FlushFileBuffers(handle) != 0;
    if (!CloseHandle(handle)) ok = false;
    if (ok) ok = regular_or_missing(target) && MoveFileExW(temporary.c_str(), target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    bool ok = true;
    size_t total = 0;
    while (total < data.size()) {
        auto n = write(fd, data.data() + total, data.size() - total);
        if (n <= 0) { ok = false; break; }
        total += static_cast<size_t>(n);
    }
    if (ok) ok = fsync(fd) == 0;
    if (close(fd) != 0) ok = false;
    if (ok) {
        std::error_code ec;
        if (!regular_or_missing(target)) ok = false;
        else { fs::rename(temporary, target, ec); ok = !ec; }
    }
#endif
    if (!ok) { std::error_code ignored; fs::remove(temporary, ignored); }
    return ok;
}


void require(bool ok) { if(!ok) throw std::invalid_argument("Invalid custom view."); }
void fields(const Json& j, std::initializer_list<const char*> allowed) {
    require(j.is_object());
    for(auto it=j.begin();it!=j.end();++it)
        require(std::any_of(allowed.begin(),allowed.end(),[&](auto k){return it.key()==k;}));
}
std::string text(const Json& j,const char* key,size_t max) {
    const auto& v=j.at(key);require(v.is_string());auto s=v.get<std::string>();
    require(!s.empty() && s.size()<=max);
    for(unsigned char c:s) require(c>=32 && c!=127);
    return s;
}
int integer(const Json& j,int low,int high) {
    require(j.is_number_integer());const auto n=j.get<int64_t>();require(n>=low && n<=high);return int(n);
}
Json layout_json(const Layout& l) {
    Json tiles=Json::array();
    for(const auto& t:l.tiles) {
        Json j={{"source",t.source},{"block",t.block},{"zone",{{"id",t.zone.id},{"column",t.zone.column},{"row",t.zone.row}}},
            {"column_span",t.zone.column_span},{"row_span",t.zone.row_span}};
        if(t.tap) j["tap"]=tap_json(*t.tap);
        if(t.density!=2) j["density"]=t.density;
        if(t.show_if) {j["show_if"]={{"path",t.show_if->path}};if(!t.show_if->op.empty()) {j["show_if"]["op"]=t.show_if->op;j["show_if"]["value"]=t.show_if->value;}}
        tiles.push_back(std::move(j));
    }
    return {{"columns",l.columns},{"rows",l.rows},{"tiles",tiles}};
}
Layout read_layout(const Json& j) {
    fields(j,{"columns","rows","tiles"}); Layout l;
    auto axis=[&](const char* key,std::vector<int>& out) {
        const auto& a=j.at(key);require(a.is_array() && !a.empty() && a.size()<=6);
        out.clear();for(const auto& n:a) out.push_back(integer(n,1,12));
    };
    axis("columns",l.columns);axis("rows",l.rows);
    const auto& tiles=j.at("tiles");require(tiles.is_array() && tiles.size()<=tile_limit);
    std::set<std::string> ids;bool occupied[6][6]={};
    for(const auto& item:tiles) {
        fields(item,{"source","block","zone","column_span","row_span","show_if","density","tap"});Tile t;
        if(item.contains("density")) t.density=integer(item["density"],0,2);
        t.source=text(item,"source",100);t.block=text(item,"block",100);
        if(item.contains("tap")) t.tap=parse_tap(item["tap"],t.source);
        require(builtin::find(t.source) || t.source=="homeassistant" ||
            (t.source.starts_with("plugin:") && valid_id(t.source.substr(7))));
        if(auto c=builtin::find(t.source)) require(std::any_of(c->blocks.begin(),c->blocks.end(),[&](const auto& b){return b.id==t.block;}));
        const auto& z=item.at("zone");fields(z,{"id","column","row"});t.zone.id=text(z,"id",48);
        require(valid_id(t.zone.id) && ids.insert(t.zone.id).second);
        t.zone.column=integer(z.at("column"),0,5);t.zone.row=integer(z.at("row"),0,5);
        t.zone.column_span=integer(item.at("column_span"),1,6);t.zone.row_span=integer(item.at("row_span"),1,6);
        require(t.zone.column+t.zone.column_span<=l.columns.size() && t.zone.row+t.zone.row_span<=l.rows.size());
        for(int y=t.zone.row;y<t.zone.row+t.zone.row_span;++y) for(int x=t.zone.column;x<t.zone.column+t.zone.column_span;++x) {
            require(!occupied[y][x]);occupied[y][x]=true;
        }
        if(item.contains("show_if")) {
            const auto& c=item["show_if"];fields(c,{"path","op","value"});plugins::Condition condition;
            condition.path=text(c,"path",128);
            require(condition.path=="data" || condition.path.starts_with("data."));
            size_t segments=1,begin=0;
            for(size_t i=0;i<=condition.path.size();++i) {
                if(i==condition.path.size() || condition.path[i]=='.') {require(i>begin);begin=i+1;if(i<condition.path.size()) ++segments;}
                else {auto ch=condition.path[i];require((ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') || (ch>='0'&&ch<='9') || ch=='_' || ch=='-');}
            }
            require(segments<=8);
            if(c.contains("op")) {
                condition.op=text(c,"op",2);require(condition.op=="==" || condition.op=="!=" || condition.op=="<" || condition.op=="<=" || condition.op==">" || condition.op==">=");
                condition.value=c.at("value");const auto& v=condition.value;
                require(v.is_boolean() || (v.is_number() && std::isfinite(v.get<double>())) ||
                    (v.is_string() && !v.get_ref<const std::string&>().empty() && v.get_ref<const std::string&>().size()<=256));
            } else require(!c.contains("value"));
            t.show_if=std::move(condition);
        }
        l.tiles.push_back(std::move(t));
    }
    return l;
}
}
std::optional<Tap> parse_tap(const Json& j,const std::string& source) {
    try {
        const auto action=text(j,"action",16);Tap t;t.action=action;
        if(action=="page") {fields(j,{"action","page"});t.page=text(j,"page",100);require(config::is_page_key(t.page));}
        else if(action=="open_link") {fields(j,{"action","url"});t.url=text(j,"url",2048);require(source.starts_with("plugin:") && plugins::tap_url_valid(t.url));}
        else if(action=="next") {fields(j,{"action"});require(source.starts_with("plugin:"));}
        else return {};
        return t;
    } catch(...) {return {};}
}
Json tap_json(const Tap& t) {
    Json j={{"action",t.action}};if(t.action=="page")j["page"]=t.page;
    if(t.action=="open_link")j["url"]=t.url;return j;
}
bool valid_id(std::string_view id) {
    if(id.empty() || id.size()>48 || plugins::reserved_device_name(id)) return false;
    for(char c:id) if(!((c>='a'&&c<='z') || (c>='0'&&c<='9') || c=='-' || c=='_')) return false;
    return id.front()!='-' && id.front()!='_';
}
const Layout& View::effective(const std::string& device) const {
    auto it=devices.find(device);return it==devices.end()?standard:it->second;
}
Json to_json(const View& v) {
    Json devices=Json::object();for(const auto& [id,l]:v.devices) devices[id]=layout_json(l);
    return {{"schema_version",1},{"id",v.id},{"name",v.name},{"layout",layout_json(v.standard)},{"devices",devices}};
}
std::optional<View> parse(std::string_view bytes) {
    try {
        auto j=plugins::parse_strict_json(bytes,file_limit);require(j.has_value());
        fields(*j,{"schema_version","id","name","layout","devices"});
        require(integer(j->at("schema_version"),1,1)==1);View v;
        v.id=text(*j,"id",48);require(valid_id(v.id));v.name=text(*j,"name",128);v.standard=read_layout(j->at("layout"));
        if(j->contains("devices")) {
            const auto& d=j->at("devices");require(d.is_object() && d.size()<=device::overlay_count_limit);
            for(auto it=d.begin();it!=d.end();++it) {require(device::valid_layout_id(it.key()));v.devices.emplace(it.key(),read_layout(it.value()));}
        }
        return v;
    } catch(...) { return {}; }
}
std::optional<View> Store::get(const std::string& id) const {
    std::lock_guard lock(mutex_);if(!valid_id(id) || !ready_directory(directory_,false)) return {};
    std::string bytes;if(!read_bounded(directory_/(id+".view.json"),bytes,file_limit)) return {};
    auto v=parse(bytes);return v && v->id==id?v:std::nullopt;
}
std::string Store::save(const View& v) {
    std::string bytes;try {bytes=to_json(v).dump();} catch(...) {return "Invalid custom view.";}
    if(!parse(bytes)) return "Invalid custom view.";
    std::lock_guard lock(mutex_);
    if(!ready_directory(directory_,true)) return "Unsafe view directory.";
    const auto path=directory_/(v.id+".view.json"),backup=fs::path(path.wstring()+L".bak");
    if(!regular_or_missing(path) || !regular_or_missing(backup)) return "Unsafe view path.";
    std::error_code ec;const bool exists=fs::exists(path,ec);if(ec) return "View unavailable.";
    if(!exists) {
        size_t count=0;for(fs::directory_iterator it(directory_,ec),end;!ec && it!=end;it.increment(ec))
            if(it->path().filename().string().ends_with(".view.json")) ++count;
        if(ec || count>=view_limit) return "View count limit exceeded.";
    } else {
        std::string old;if(!read_bounded(path,old,file_limit) || !atomic_write(backup,old)) return "View backup failed.";
    }
    return atomic_write(path,bytes)?"":"Atomic view write failed.";
}
std::vector<View> Store::list() const {
    std::vector<std::string> ids;
    { std::lock_guard lock(mutex_);
      if(!ready_directory(directory_,false)) return {};
      std::error_code ec;
      for(fs::directory_iterator it(directory_,ec),end;!ec && it!=end && ids.size()<view_limit;it.increment(ec)) {
          auto name=it->path().filename().string();
          if(name.ends_with(".view.json") && valid_id(name.substr(0,name.size()-10))) ids.push_back(name.substr(0,name.size()-10));
      }
    }
    std::sort(ids.begin(),ids.end());std::vector<View> result;
    for(const auto& id:ids) if(auto view=get(id)) result.push_back(std::move(*view));
    return result;
}
std::string Store::erase(const std::string& id) {
    std::lock_guard lock(mutex_);
    if(!valid_id(id) || !plain_path(directory_)) return "Unsafe view path.";
    std::error_code exists_error;if(!fs::exists(directory_,exists_error) && !exists_error)return "";
    if(!ready_directory(directory_,false)) return "Unsafe view path.";
    const auto path=directory_/(id+".view.json"),backup=directory_/(id+".view.json.bak");
    if(!regular_or_missing(path) || !regular_or_missing(backup)) return "Unsafe view path.";
    std::error_code ec;
    fs::remove(backup,ec);if(ec) return "Could not remove view backup.";
    fs::remove(path,ec);return ec?"Could not remove view.":"";
}
plugins::zones::Layout tile_layout(const Tile& t,const Layout& l) {return {l.columns,l.rows,{t.zone}};}
std::optional<builtin::Overlay> single_builtin(const Tile& t,const Layout& l) {
    auto c=builtin::find(t.source);if(!c) return {};
    bool found=false;for(const auto& b:c->blocks) if(b.id==t.block) found=true;
    if(!found) return {};
    builtin::Overlay a;a.catalog_version=c->version;a.model.manifest_sha256.assign(64,'0');
    a.model.layout={{"columns",l.columns},{"rows",l.rows},{"zones",Json::array({{{"id",t.zone.id},{"column",t.zone.column},{"row",t.zone.row},{"column_span",t.zone.column_span},{"row_span",t.zone.row_span}}})}};
    a.single_tile=true;a.tile_density=t.density;
    a.model.blocks=Json::array({{{"id",t.block},{"zone",t.zone.id},{"shown",true}}});
    // Single-tile validation is separate from the complete-page status contract.
    if(!builtin::validate(*c,a).empty()) return {};
    return a;
}
bool visible(const Tile& tile,const Json& data) {
    if(!tile.show_if) return true;
    const auto& c=*tile.show_if;const Json* value=&data;
    size_t start=5; // public root is data; never settings or secrets
    if(c.path!="data") while(start<c.path.size()) {
        const auto end=c.path.find('.',start);const auto key=c.path.substr(start,end==std::string::npos?end:end-start);
        if(!value->is_object() || !value->contains(key)) return false;
        value=&value->at(key);if(end==std::string::npos) break;start=end+1;
    }
    return plugins::condition_holds(c,value);
}
std::optional<Image> draw_plugin(const Tile& tile,const Layout& layout,plugins::Manifest manifest,
                                  const plugins::RenderContext& context,int width,int height) {
    if(tile.source!="plugin:"+manifest.id || manifest.schema_version!=2) return {};
    auto widget=std::find_if(manifest.widgets.begin(),manifest.widgets.end(),[&](const auto& w){return w.id==tile.block;});
    if(widget==manifest.widgets.end()) return {};
    Json data=Json::object();for(const auto& [id,value]:context.sources) data[id]=value;
    if(!visible(tile,data)) return {};
    auto selected=*widget;selected.zone=tile.zone.id;
    manifest.widgets={selected};manifest.layout=tile_layout(tile,layout);
    auto page=plugins::render_page(manifest,context);page.density_cap=tile.density;
    if(page.widgets.empty() || page.widgets.front().missing) return {};
    return plugins::draw_page(page,width,height);
}
DrawOut draw_out(const View& v,const std::string& device,int w,int h,const DrawTileOut& source,const Override& override_target) {
    if(w<=0 || h<=0 || w>8192 || h>8192) return {};
    DrawOut out;out.image=Image(w,h,render::theme().bg);auto& result=out.image;const auto& l=v.effective(device);const float s=render::scale_for(w,h);
    const float lw=std::round(w/s),lh=std::round(h/s);
    for(const auto& t:l.tiles) try {
        // Built-ins share this content rectangle. Plugin callbacks use a matching
        // built-in crop only for composition; their own geometry is handled below.
        const bool plugin=t.source.starts_with("plugin:");
        const float spacing=theme::design(render::theme().style).spacing/12.f,margin=10*spacing;
        auto plan=plugins::zones::plan(plugin?plugins::zones::Rect{margin,39,w/s-2*margin,h/s-58-margin}:
            plugins::zones::Rect{8,36,lw-16,lh-54},tile_layout(t,l),{},plugin?7*spacing:6);
        if(plan.zones.empty()) continue;
        const auto source_area=plan.zones.front().area;
        // All sources occupy the same grid. Plugin frames have different margins;
        // map their clipped zone into the common content rectangle.
        const auto target_plan=plugins::zones::plan({8,36,lw-16,lh-54},tile_layout(t,l),{},6);
        if(target_plan.zones.empty()) continue;
        const auto a=target_plan.zones.front().area;
        auto drawn=source(t,l,w,h);if(!drawn || drawn->image.w!=w || drawn->image.h!=h) continue;auto image=&drawn->image;
        int x0=std::clamp(int(std::round(a.x*s)),0,w),x1=std::clamp(int(std::round((a.x+a.width)*s)),0,w);
        int y0=std::clamp(int(std::round(a.y*s)),0,h),y1=std::clamp(int(std::round((a.y+a.height)*s)),0,h);
        // Use precisely the composition transform for all source hit rectangles.
        const int sx0=plugin?std::clamp(int(std::round(source_area.x*s)),0,w):x0;
        const int sy0=plugin?std::clamp(int(std::round(source_area.y*s)),0,h):y0;
        const int sx1=plugin?std::clamp(int(std::round((source_area.x+source_area.width)*s)),0,w):x1;
        const int sy1=plugin?std::clamp(int(std::round((source_area.y+source_area.height)*s)),0,h):y1;
        if(x1<=x0 || y1<=y0 || sx1<=sx0 || sy1<=sy0)continue;
        if(!override_target || !override_target(t,{x0,y0,x1-x0,y1-y0},out)) {
            auto map=[&](render::Box r) {
                int left=std::clamp(r.x,sx0,sx1),right=std::clamp(r.x+r.w,sx0,sx1);
                int top=std::clamp(r.y,sy0,sy1),bottom=std::clamp(r.y+r.h,sy0,sy1);
                // ceil division matches the nearest-neighbour pixel ownership above.
                auto mx=[&](int x){return x0+((x-sx0)*(x1-x0)+sx1-sx0-1)/(sx1-sx0);};
                auto my=[&](int y){return y0+((y-sy0)*(y1-y0)+sy1-sy0-1)/(sy1-sy0);};
                return render::Box{mx(left),my(top),mx(right)-mx(left),my(bottom)-my(top)};
            };
            for(auto target:drawn->targets) {target.rect_px=map(target.rect_px);if(target.rect_px.w>0 && target.rect_px.h>0)out.targets.push_back(std::move(target));}
            for(auto target:drawn->plugin) {target.rect_px=map(target.rect_px);if(target.rect_px.w>0 && target.rect_px.h>0) {target.block=v.id+"/"+t.zone.id+"/"+target.block;out.plugin.push_back(std::move(target));}}
        }
        if(!plugin) {
            for(int y=y0;y<y1;++y) std::copy(image->px.begin()+size_t(y)*w+x0,image->px.begin()+size_t(y)*w+x1,result.px.begin()+size_t(y)*w+x0);
        } else {
            const int sx0=std::clamp(int(std::round(source_area.x*s)),0,w),sy0=std::clamp(int(std::round(source_area.y*s)),0,h);
            const int sx1=std::clamp(int(std::round((source_area.x+source_area.width)*s)),0,w);
            const int sy1=std::clamp(int(std::round((source_area.y+source_area.height)*s)),0,h);
            if(sx1<=sx0 || sy1<=sy0) continue;
            for(int y=y0;y<y1;++y) for(int x=x0;x<x1;++x)
                result.px[size_t(y)*w+x]=image->at(sx0+(x-x0)*(sx1-sx0)/(x1-x0),sy0+(y-y0)*(sy1-sy0)/(y1-y0));
        }
    } catch(...) {} // Each failing source leaves only its own tile blank.
    return out;
}
Override tap_override(std::string view_id,TapEnv env) {
    return [view_id=std::move(view_id),env=std::move(env)](const Tile& tile,render::Box rect,DrawOut& out)->bool {
        if(!tile.tap || rect.w<=0 || rect.h<=0) return false;
        const auto valid=parse_tap(tap_json(*tile.tap),tile.source);if(!valid) return false;
        if(valid->action=="page") {
            if(!env.go || std::find(env.pages.begin(),env.pages.end(),valid->page)==env.pages.end()) return false;
            out.targets.push_back({rect,{},[go=env.go,page=valid->page]{go(page);}});return true;
        }
        if(valid->action=="open_link") {
            if(!env.grant || !tile.source.starts_with("plugin:")) return false;
            const auto grant=env.grant(tile.source.substr(7));
            if(!grant || !grant->allowed || !grant->allowed(valid->url)) return false;
            out.plugin.push_back({rect,grant->plugin,view_id+"/"+tile.zone.id+"/override",tile.zone.id,"open_link",valid->url,grant->generation});return true;
        }
        return false; // next is resolved by the source's own cycle, otherwise Standard.
    };
}
Image draw(const View& v,const std::string& device,int w,int h,const DrawTile& source) {
    return draw_out(v,device,w,h,[&](const Tile& t,const Layout& l,int width,int height)->std::optional<TileDraw> {
        auto image=source(t,l,width,height);if(!image)return {};return TileDraw{std::move(*image),{}, {}};
    }).image;
}
} // namespace fl::custom
