// © 2026 FuchsLabs – public Steam assets and local EXE/package logos; no scraping.
#include "game_logo.h"
#include "http.h"
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <pugixml.hpp>
#include "util.h"
namespace fl::game {
namespace {
constexpr size_t max_asset=4*1024*1024;
bool safe(const std::filesystem::path& p) {
    auto current=p;
    while(!current.empty()) {std::error_code ec;auto st=std::filesystem::symlink_status(current,ec);if(!ec && std::filesystem::is_symlink(st))return false;
        DWORD attr=GetFileAttributesW(current.c_str());if(attr!=INVALID_FILE_ATTRIBUTES && (attr&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        auto parent=current.parent_path();if(parent==current)break;current=parent;}
    return true;
}
ImagePtr image(const std::string& bytes) {if(bytes.empty() || bytes.size()>max_asset)return {};if(auto decoded=decode_image(bytes))return std::make_shared<Image>(thumbnail(*decoded,1024,512));return {};}
}
std::vector<std::string> LogoCache::urls(const std::string& id){
    if(id.empty() || id.size()>15 || !std::all_of(id.begin(),id.end(),[](char c){return c>='0' && c<='9';}))return {};
    std::vector<std::string> out;for(auto file:{"logo.png","header.jpg","library_hero.jpg"})out.push_back("https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/"+id+"/"+file);return out;
}
LogoCache::LogoCache(std::filesystem::path directory,Fetch fetch):directory_(std::move(directory)),fetch_(std::move(fetch)) {
    if(!fetch_)fetch_=[](const std::string& url){http::Request r;r.url=url;r.timeout_ms=2000;r.max_bytes=max_asset;r.follow_redirects=false;auto response=http::fetch(r);return response.status==200 && !response.truncated?response.body:std::string();};
}
ImagePtr LogoCache::load(const Game& game){
    auto local=[&]{if(auto icon=package_logo(game.executable))return icon;return executable_icon(game.executable);};
    auto sources=urls(game.steam_id);if(sources.empty())return local();
    try {
        auto path=directory_/(game.steam_id+".png");std::error_code ec;
        if(safe(path) && std::filesystem::is_regular_file(path,ec) && std::filesystem::file_size(path,ec)<=max_asset && !ec) {
            auto written=std::filesystem::last_write_time(path,ec);
            if(!ec && fresh(std::chrono::duration<double>(std::filesystem::file_time_type::clock::now()-written).count())) {
                std::ifstream f(path,std::ios::binary);std::string bytes(max_asset+1,0);f.read(bytes.data(),bytes.size());bytes.resize(size_t(f.gcount()));if(auto img=image(bytes))return img;
            }
        }
        for(const auto& url:sources)if(auto img=image(fetch_(url))) {
            if(safe(directory_)) {
                std::filesystem::create_directories(directory_,ec);
                auto tmp=std::filesystem::path(path.wstring()+L".tmp");
                if(!ec && safe(path) && safe(tmp)) {auto bytes=encode_png(*img);std::ofstream f(tmp,std::ios::binary|std::ios::trunc);f.write(bytes.data(),bytes.size());bool ok=bool(f);f.close();
                    if(ok)MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);std::filesystem::remove(tmp,ec);}
            }
            return img;
        }
    }catch(...){} // unavailable assets/cache never prevent game-mode
    return local();
}
ImagePtr package_logo(const std::filesystem::path& exe) {
    // Read only manifests adjacent to this executable, never enumerate drives/packages.
    auto root=exe.parent_path();
    for(int depth=0;depth<6 && !root.empty();++depth,root=root.parent_path())try {
        auto manifest=root/L"AppxManifest.xml";std::error_code ec;
        if(!safe(manifest) || !std::filesystem::is_regular_file(manifest,ec) || std::filesystem::file_size(manifest,ec)>max_asset || ec)continue;
        pugi::xml_document doc;if(!doc.load_file(manifest.c_str()))continue;
        auto apps=doc.child("Package").child("Applications");
        for(auto app:apps.children("Application")) {
            auto executable=app.attribute("Executable").as_string();
            if(!*executable || !beneath(exe,root) || casefold(narrow((root/widen(executable)).lexically_normal().wstring()))!=casefold(narrow(exe.lexically_normal().wstring())))continue;
            std::vector<std::string> logos;
            for(auto visual:app.children())for(auto key:{"Square44x44Logo","Square150x150Logo","Logo"})if(*visual.attribute(key).as_string())logos.push_back(visual.attribute(key).as_string());
            if(auto logo=doc.child("Package").child("Properties").child("Logo"))logos.push_back(logo.text().as_string());
            ImagePtr best;
            for(const auto& relative:logos) {
                auto asset=(root/widen(relative)).lexically_normal();
                if(!beneath(asset,root) || !safe(asset.parent_path()))continue;
                // Qualified targetsize/scale variants live beside the manifest asset.
                size_t examined=0;
                for(const auto& entry:std::filesystem::directory_iterator(asset.parent_path(),ec)) {
                    if(++examined>512)break;
                    auto path=entry.path();auto stem=path.stem().wstring(),base=asset.stem().wstring();
                    if(path.extension()!=asset.extension() || (stem!=base && !stem.starts_with(base+L".")))continue;
                    if(!safe(path) || !entry.is_regular_file(ec) || entry.file_size(ec)>max_asset || ec)continue;
                    std::ifstream f(path,std::ios::binary);std::string bytes(max_asset+1,0);f.read(bytes.data(),bytes.size());bytes.resize(size_t(f.gcount()));
                    if(auto candidate=image(bytes);candidate && (!best || candidate->w*candidate->h>best->w*best->h))best=candidate;
                }
            }
            if(best)return best;
        }
    }catch(...){} // Inaccessible/invalid package assets fall through to the EXE.
    return {};
}
ImagePtr executable_icon(const std::filesystem::path& exe){
    if(exe.empty())return {};
    HICON icon=nullptr;
    using Extract=UINT(WINAPI*)(LPCWSTR,int,int,int,HICON*,UINT*,UINT,UINT);
    auto extract=reinterpret_cast<Extract>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"PrivateExtractIconsW"));
    // Extract the actual executable resource at its largest standard size first.
    if(extract)for(int size:{256,128,64,48,32}){UINT id=0;auto count=extract(exe.c_str(),0,size,size,&icon,&id,1,0);if(count==1 && icon)break;if(icon)DestroyIcon(icon);icon=nullptr;}
    if(!icon){SHFILEINFOW info{};if(!SHGetFileInfoW(exe.c_str(),0,&info,sizeof info,SHGFI_ICON|SHGFI_LARGEICON))return {};icon=info.hIcon;}
    constexpr int side=256;BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=side;bi.bmiHeader.biHeight=-side;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);ImagePtr result;
    if(bitmap && bits && dc) {auto old=SelectObject(dc,bitmap);memset(bits,0,side*side*4);if(DrawIconEx(dc,0,0,icon,side,side,0,nullptr,DI_NORMAL)) {auto img=std::make_shared<Image>(side,side);memcpy(img->px.data(),bits,side*side*4);bool alpha=false;for(auto px:img->px)alpha|=(px>>24)!=0;
        if(!alpha){memset(bits,255,side*side*4);DrawIconEx(dc,0,0,icon,side,side,0,nullptr,DI_NORMAL);auto white=static_cast<uint32_t*>(bits);for(size_t i=0;i<img->px.size();++i){auto black=img->px[i];auto a=255-std::clamp(int(white[i]&255)-int(black&255),0,255);img->px[i]=black|(uint32_t(a)<<24);}}
        result=img;}SelectObject(dc,old);}
    if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);DestroyIcon(icon);return result;
}
}
