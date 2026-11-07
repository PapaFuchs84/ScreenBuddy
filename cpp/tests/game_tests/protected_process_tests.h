// Offline process/window fixtures: never open a game or contact an app port.
#include <algorithm>
void protected_process_checks() {
    auto root=std::filesystem::current_path()/L"cpp/build"/(L"protected-process-test-"+std::to_wstring(GetCurrentProcessId()));
    check(!std::filesystem::exists(root));
    std::filesystem::create_directories(root/L"bin/one/two/too-deep");
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{root};
    auto file=[&](const std::filesystem::path& name){std::ofstream f(root/name);f<<"fixture";};
    file(L"bf6.EXE");file(L"not-an-exe.dll");file(L"bin/one/two/depth3.exe");file(L"bin/one/two/too-deep/depth4.exe");
    for(auto name:{L"EAAntiCheat.GameService.exe",L"EAAntiCheat.GameService64.exe",L"EAAntiCheat.GameServiceLauncher.exe",
        L"EasyAntiCheat.exe",L"Launcher.exe",L"CrashReporter.exe",L"CrashHandler64.exe",L"setup.exe",L"installer.exe",L"uninstall.exe",L"GameHelper.exe"}) {
        file(name);check(default_excluded(name));
    }
    auto manifest=parse_acf(R"("AppState" { "appid" "2807960" "name" "Battlefield 6" "installdir" "Battlefield 6" })",root.parent_path());
    check(manifest && manifest->name=="Battlefield 6" && manifest->steam_id=="2807960");
    Game bf=*manifest;bf.install=root;cache_executables(bf);
    check(bf.cached_executables.size()==2);
    auto cached=[&](const wchar_t* name){return std::any_of(bf.cached_executables.begin(),bf.cached_executables.end(),[&](const auto& path){return path.filename()==name;});};
    check(cached(L"bf6.EXE") && cached(L"depth3.exe") && !cached(L"depth4.exe"));
    check(!default_excluded(L"CrashBandicoot.exe"));
    check(!matches_game(bf,{},true,"0")); // The former path-only gate cannot identify this case.
    std::vector<Process> processes={{777,{},"bf6.exe"}};
    auto candidate=select_game({bf},processes,{777,true},"0");
    check(candidate && candidate->id=="steam:2807960" && candidate->name=="Battlefield 6");
    check(candidate && candidate->pid==777 && candidate->process_name=="bf6.exe" && candidate->executable.empty());
    check(candidate && candidate->steam_id=="2807960" && LogoCache::urls(candidate->steam_id).size()==3);
    check(!select_game({bf},processes,{888,true},"2807960"));
    check(!select_game({bf},processes,{777,false},"2807960"));
    check(!select_game({},processes,{777,true},"2807960"));
    check(!select_game({bf},{{777,{},"unknown.exe"}},{777,true},"0"));
    check(!select_game({bf},{{777,root.parent_path()/L"elsewhere/bf6.exe","bf6.exe"}},{777,true},"0"));
    check(!select_game({bf},processes,{777,true},"0",{}, {"BF6.EXE"}));
    check(!select_game({bf},processes,{777,true},"0",{}, {(root/L"bf6.EXE").string()}));
    // Even malicious/old cache contents cannot admit helper processes.
    Game bad=bf;
    for(auto name:{"EAAntiCheat.GameService.exe","EAAntiCheat.GameService64.exe","Launcher.exe","CrashReporter.exe","setup.exe"}) {
        bad.cached_executables.push_back(root/name);
        check(!select_game({bad},{{777,{},name}},{777,true},"0"));
    }
    Game other=bf;other.id="epic:other";other.install=root/L"other";other.cached_executables={other.install/L"bf6.exe"};
    check(!select_game({bf,other},processes,{777,true},"0"));
    other.install=bf.install;other.cached_executables=bf.cached_executables;check(select_game({other,bf},processes,{777,true},"0")->steam_id=="2807960");
    for(auto source:{"epic:","gog:","ubisoft:","ea:","xbox:","blizzard:","rockstar:"}) {
        other=bf;other.id=std::string(source)+"fixture";other.steam_id.clear();cache_executables(other);
        check(bool(select_game({other},processes,{777,true},"0")));
    }
    Game manual;manual.id="exe:fixture";manual.executable=root/L"bf6.EXE";cache_executables(manual);
    check(manual.cached_executables.size()==1 && bool(select_game({manual},processes,{777,true},"0")));
    check(!select_game({manual},processes,{777,false},"0"));
    check(select_game({manual,bf},processes,{777,true},"0")->steam_id=="2807960");
    check(select_game({bf,manual},processes,{777,true},"0")->steam_id=="2807960");
    Detector detector;
    check(!detector.update(candidate,0));
    check(!detector.update(select_game({bf},processes,{777,true},"0"),2.9));
    check(detector.update(select_game({bf},processes,{777,true},"0"),3) && detector.active()->pid==777);
    auto retained=select_game({bf},processes,{999,false},"0",detector.active());
    check(retained==candidate && !detector.update(retained,4) && detector.started()==3);
    processes[0].path=root/L"bf6.EXE";
    check(bool(select_game({bf},processes,{999,false},"0",detector.active())));
    processes[0].path=root.parent_path()/L"other/bf6.exe";
    check(!select_game({bf},processes,{999,false},"0",detector.active()));
    check(!select_game({bf},{{777,{},"different.exe"}},{999,false},"0",detector.active()));
    check(!select_game({bf},{{778,{},"bf6.exe"}},{999,false},"0",detector.active()));
    check(!select_game({bf},{{777,{},"bf6.exe"}},{999,false},"0",detector.active(),{"bf6.exe"}));
    // Readable -> protected transition preserves the same PID and debounce identity too.
    auto readable=select_game({bf},{{777,root/L"bf6.EXE","bf6.exe"}},{777,true},"0");
    check(bool(select_game({bf},{{777,{},"bf6.exe"}},{999,false},"0",readable)));
    check(!detector.update({},5) && detector.state()==State::ending);
    check(!detector.update({},9.99));check(detector.update({},10) && !detector.active());
    Game missing;missing.install=root/L"missing";cache_executables(missing);check(missing.cached_executables.empty());
    // A hard traversal cap includes directories and non-EXE entries.
    auto capped=root/L"cap";std::filesystem::create_directory(capped);
    for(int i=0;i<2005;++i){std::ofstream f(capped/(std::to_string(i)+".exe"));}
    Game limited;limited.install=capped;cache_executables(limited);check(limited.cached_executables.size()==2000);
}
