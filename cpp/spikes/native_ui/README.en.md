# Browser-free native UI spike

[Deutsch](README.md) · **English**

This isolated ScreenBuddy 2.5 prototype shows native Windows settings, an explicitly simulated status and a preview produced by the existing C++ renderer. It is not a complete settings replacement. These files do not change production startup, the current settings host or its HTTP server.

## Local tools checked on 24 October 2026

- MSVC Build Tools 2022, toolset 14.44.35207 and Windows SDK 10.0.26100.0 are installed; SDKs 19041 and 22621 are also present.
- No Windows App SDK/WinUI 3 development package was found in `%USERPROFILE%\.nuget\packages` or the project dependencies. The AppX runtime query returned “Access denied”, so an installed runtime is neither confirmed nor ruled out.
- `qmake` and `qtpaths` are absent from PATH. No Qt SDK was found in `C:\Qt`, `D:\Qt`, `C:\Program Files\Qt` or the project dependencies. This was not a complete disk inventory.

Win32 standard controls therefore provide the immediately buildable local spike. WinUI 3 would first need a selected Windows App SDK version, C++/WinRT generation, bootstrap/deployment and packaging setup. Nothing was installed.

## Build without running anything

Prerequisite: the existing `cpp/build/flcore.lib` was built with MSVC x64 and the static C++ runtime. The helper reuses it and does not trigger a full production build, tests or the generated executable.

```powershell
./cpp/spikes/native_ui/build.ps1
```

Optional: `-CoreBuildDir <directory-containing-flcore.lib>`. Output is placed in that build directory's `native-ui-spike` subdirectory. Tools are discovered locally; no packages are downloaded or installed. The embedded manifest enables Common Controls 6 and Per-Monitor-V2 DPI scaling.

## Representative flow

A later explicitly authorized UI test can open the sample-data window. `--en` selects English; German is the default. Theme changes immediately redraw the preview. “Apply (demo only)” validates brightness/touch/theme through `config::merge`, commits only to memory and updates status. “Reset” restores defaults. Brightness and touch never issue device commands.

The spike reuses `config::defaults/merge`, `render::Renderer`, `render::demo_friends`, `render::apply_theme` and `gfx`. It constructs no `App`, `Output` or data sources, loads no user configuration, writes no files and opens neither a serial port nor an HTTP listener. The existing graphics code uses COM for WIC; this is unrelated to serial COM ports.

## Integration steps

1. Add an optional CMake target: `add_executable(screenbuddy-native-spike WIN32 spikes/native_ui/native_settings_spike.cpp)` and `target_link_libraries(screenbuddy-native-spike PRIVATE flcore)`. Embed the manifest using `/MANIFEST:EMBED` and `/MANIFESTINPUT:<absolute path>` as in the helper. Initially leave the production target unchanged.
2. Extract shared services from `webui.cpp`. Both UIs need the same `config::merge/save`, `App::cfg/last_frame/preview_theme/connections`, restart and error handling. This spike deliberately demonstrates only the safe in-memory path.
3. Send short-lived configuration/status/preview snapshots to the UI. Keep networking, file operations and running-app queries off the UI thread and deliver results through window messages. Production previews must restore the global renderer theme: use `App::preview_theme` or save/restore the theme under `render::mutex`.
4. Implement all remaining settings and actions: output/monitor/orientation, sources and secrets, page order, companion pairing, updates, autostart and setup. Preserve DPAPI and existing validation. Remove the production WebView host and its settings HTTP server only after feature parity; assess companion services separately.
5. Check sample-data UI at 100/150/200% DPI in DE/EN, keyboard use, screenreader, high contrast and small screens. Win32 removes the browser dependency but does not automatically provide WinUI 3/Fluent styling.

## Validation and limits

On 24 October 2026 the spike compiled and linked with MSVC C++20, `/W4` and `/MT`, without errors or warnings. The executable was **not run**. Static PE dependencies: D2D1, DWrite, COMCTL32, USER32, GDI32, OLE32, SHELL32 and KERNEL32; no WebView2/Edge dependency. WIC is created through COM. This confirms build/link and static dependencies, not visible UI/DPI/accessibility behavior or hardware operation.

Full feature parity, production threading/lifetime integration, secret-input controls and smaller-screen adaptation remain open. Minimum window size is 960 × 720 logical units. Global renderer theme and language remain process-local in this standalone spike and need explicit coordination when integrated into the running app.
