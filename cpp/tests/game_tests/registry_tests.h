// Synthetic registry/filesystem only: no installed games, processes or application ports.
#include <map>
struct FakeGameRegistry final : Registry {
    std::map<std::wstring,std::vector<std::wstring>> children;
    std::map<std::pair<std::wstring,std::wstring>,std::wstring> values;
    std::vector<std::wstring> subkeys(const std::wstring& key) const override {
        auto it=children.find(key);return it==children.end()?std::vector<std::wstring>{}:it->second;
    }
    std::wstring value(const std::wstring& key,const wchar_t* name) const override {
        auto it=values.find({key,name});return it==values.end()?L"":it->second;
    }
    void entry(const std::wstring& parent,const std::wstring& id,
               std::initializer_list<std::pair<std::wstring,std::wstring>> fields) {
        children[parent].push_back(id);for(const auto& [name,value]:fields)values[{parent+L"\\"+id,name}]=value;
    }
};
void registry_game_checks() {
    FakeGameRegistry r;
    const std::wstring base=L"SOFTWARE\\WOW6432Node\\", native=L"SOFTWARE\\";
    const auto uninstall=base+L"Microsoft\\Windows\\CurrentVersion\\Uninstall";
    auto none=[](const std::filesystem::path&){return false;};
    check(registry_games(r,none).empty());
    r.entry(base+L"GOG.com\\Games",L"42",{{L"gameName",L"Example GOG"},{L"path",L"D:\\Games\\Gog"},{L"exe",L"bin\\game.exe"}});
    r.entry(base+L"Ubisoft\\Launcher\\Installs",L"7",{{L"InstallDir",L"D:\\Games\\UbisoftFolder\\"}});
    r.entry(uninstall,L"ubi",{{L"DisplayName",L"Example Ubisoft"},{L"InstallLocation",L"d:\\games\\UbisoftFolder"}});
    r.entry(base+L"Ubisoft\\Launcher\\Installs",L"8",{{L"InstallDir",L"D:\\Games\\FolderFallback"}});
    r.entry(base+L"EA Games",L"Example EA",{{L"Install Dir",L"\"D:\\Games\\EA\""}});
    r.entry(base+L"Electronic Arts",L"Example Origin",{{L"Install Dir",L"D:\\Games\\OriginGame"}});
    r.entry(base+L"Rockstar Games",L"Example Rockstar",{{L"InstallFolder",L"D:\\Games\\Rockstar"}});
    r.entry(uninstall,L"ea",{{L"Publisher",L"Electronic Arts"},{L"DisplayName",L"EA Uninstall"},{L"InstallLocation",L"D:\\Games\\EAUninstall"}});
    r.entry(uninstall,L"blizzard",{{L"Publisher",L"Blizzard Entertainment"},{L"DisplayName",L"Blizzard Uninstall"},{L"InstallLocation",L"E:\\Custom\\BlizzardGame"}});
    auto games=registry_games(r,none);check(games.size()==8);
    auto find=[&](const std::string& name)->Game {for(const auto& g:games)if(g.name==name)return g;throw std::runtime_error("missing fixture "+name);};
    auto gog=find("Example GOG");check(gog.id.starts_with("gog:") && gog.steam_id.empty());
    check(gog.executable==std::filesystem::path(L"D:\\Games\\Gog\\bin\\game.exe"));
    check(find("Example Ubisoft").id.starts_with("ubisoft:"));check(find("FolderFallback").id.starts_with("ubisoft:"));
    check(find("Example EA").id.starts_with("ea:"));check(find("Example Origin").id.starts_with("ea:"));
    check(find("Example Rockstar").id.starts_with("rockstar:"));check(find("EA Uninstall").id.starts_with("ea:"));
    check(find("Blizzard Uninstall").id.starts_with("blizzard:"));
    for(const auto& g:games) {
        auto exe=g.install/L"bin/game.exe";
        check(matches_game(g,exe,true,"0"));check(!matches_game(g,exe,false,"0"));
        check(!matches_game(g,g.install.parent_path()/L"Other/game.exe",true,"0"));
        check(!matches_game(g,g.install/L"Agent.exe",true,"0"));
    }
    check(!matches_game(gog,gog.executable,false,"0")); // Exact GOG EXE still requires foreground/fullscreen.
    check(!matches_game(gog,L"D:\\Games\\GogOther\\game.exe",true,"0"));
    // Duplicate native/uninstall records do not create duplicate games.
    r.entry(native+L"GOG.com\\Games",L"42",{{L"path",L"d:\\games\\GOG"},{L"gameName",L"Duplicate"}});
    r.entry(uninstall,L"duplicate",{{L"Publisher",L"EA"},{L"InstallLocation",L"D:\\Games\\EA"}});
    r.entry(uninstall,L"ea-short",{{L"Publisher",L"ea"},{L"DisplayName",L"Short EA"},{L"InstallLocation",L"D:\\Games\\ShortEA"}});
    r.entry(native+L"Microsoft\\Windows\\CurrentVersion\\Uninstall",L"native",{{L"Publisher",L"Blizzard Entertainment, Inc."},{L"DisplayName",L"Native Blizzard"},{L"InstallLocation",L"E:\\Games\\NativeBlizzard"}});
    games=registry_games(r,none);check(games.size()==10);check(find("Short EA").id.starts_with("ea:"));check(find("Native Blizzard").id.starts_with("blizzard:"));
    // Missing values, relative/root paths, publishers with misleading substrings and launchers are ignored.
    for(const auto& path:{L"",L"relative\\game",L"C:\\",L"C:\\Games\\.."})
        r.entry(base+L"GOG.com\\Games",path,{{L"path",path},{L"gameName",L"Invalid"}});
    r.entry(uninstall,L"unrelated",{{L"Publisher",L"Not Electronic Arts"},{L"InstallLocation",L"D:\\Unrelated"}});
    r.entry(uninstall,L"ea-app",{{L"Publisher",L"Electronic Arts"},{L"DisplayName",L"EA app"},{L"InstallLocation",L"D:\\EA Desktop"}});
    r.entry(base+L"Electronic Arts",L"Origin",{{L"Install Dir",L"D:\\Origin"}});
    r.entry(base+L"Rockstar Games",L"Launcher",{{L"InstallFolder",L"D:\\RockstarLauncher"}});
    check(registry_games(r,none).size()==10);
    // Bad executable metadata cannot escape the installation; valid path discovery remains usable.
    for(const auto& exe:{L"..\\escape.exe",L"E:\\escape.exe",L"game.exe --flag",L"game.dll"}) {
        r.values[{base+L"GOG.com\\Games\\42",L"exe"}]=exe;
        games=registry_games(r,none);check(find("Example GOG").executable.empty());
    }
    // Battle.net fallback reads only a fixed directory allowlist beneath/alongside the registry install path.
    r.values[{base+L"Blizzard Entertainment\\Battle.net",L"InstallPath"}]=L"F:\\Library\\Battle.net";
    r.entry(uninstall,L"battle",{{L"Publisher",L"Blizzard Entertainment"},{L"DisplayName",L"Battle.net"},{L"InstallLocation",L"F:\\Library\\Battle.net"}});
    int probes=0;
    games=registry_games(r,[&](const std::filesystem::path& path){++probes;return path==L"F:\\Library\\Diablo IV" || path==L"F:\\Library\\Battle.net\\World of Warcraft";});
    check(probes==40);check(games.size()==12);check(find("Diablo IV").id.starts_with("blizzard:"));check(find("World of Warcraft").steam_id.empty());
    for(const auto& name:{L"Battle.net.exe",L"EADesktop.exe",L"EABackgroundService.exe",L"Origin.exe",L"UbisoftConnect.exe",L"upc.exe",L"GalaxyClient.exe",L"RockstarGamesLauncher.exe",L"Agent.exe",L"BlizzardError.exe",L"CrashHandler64.exe",L"ExampleLauncherHelper.exe",L"EAAntiCheat.GameServiceLauncher.exe",L"BEService.exe",L"EasyAntiCheat.exe",L"EasyAntiCheat_EOS.exe",L"EASYANTICHEAT_SETUP.EXE"})
        check(default_excluded(std::filesystem::path(L"C:/Games/Example")/name));
    check(!default_excluded(L"C:/Games/LauncherFolder/game.exe"));check(!default_excluded(L"C:/Games/Example/Overwatch.exe"));
    Game steam;steam.id="steam:42";steam.steam_id="42";steam.install=L"C:/Steam/Example";
    check(matches_game(steam,L"C:/Steam/Example/game.exe",false,"42"));check(!matches_game(steam,L"C:/Steam/Example/game.exe",false,"0"));
    auto epic=parse_epic(R"({"AppName":"fixture","DisplayName":"Example Epic","InstallLocation":"C:/Epic/Example","LaunchExecutable":"game.exe"})");
    check(epic && matches_game(*epic,epic->executable,true,"0"));
    check(epic && !matches_game(*epic,epic->executable,false,"0"));
    Game manual;manual.id="exe:example";manual.executable=L"C:/Games/Example/game.exe";
    check(matches_game(manual,manual.executable,false,"0"));check(!matches_game(manual,L"C:/Games/Example/other.exe",true,"0"));
}
