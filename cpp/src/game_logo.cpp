// © 2026 FuchsLabs – official public Steam assets; no scraping or TMDB.
#include "game_logo.h"
#include "http.h"
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <chrono>
#include <algorithm>
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
    auto sources=urls(game.steam_id);if(sources.empty())return executable_icon(game.executable);
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
    return executable_icon(game.executable);
}
ImagePtr executable_icon(const std::filesystem::path& exe){
    if(exe.empty())return {};SHFILEINFOW info{};
    if(!SHGetFileInfoW(exe.c_str(),0,&info,sizeof info,SHGFI_ICON|SHGFI_LARGEICON))return {};
    constexpr int side=64;BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=side;bi.bmiHeader.biHeight=-side;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);ImagePtr result;
    if(bitmap && bits) {auto old=SelectObject(dc,bitmap);memset(bits,0,side*side*4);if(DrawIconEx(dc,0,0,info.hIcon,side,side,0,nullptr,DI_NORMAL)) {auto img=std::make_shared<Image>(side,side);memcpy(img->px.data(),bits,side*side*4);bool alpha=false;for(auto px:img->px)alpha|=(px>>24)!=0;if(!alpha)for(auto& px:img->px)px|=0xff000000;result=img;}SelectObject(dc,old);}
    if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);DestroyIcon(info.hIcon);return result;
}
}
