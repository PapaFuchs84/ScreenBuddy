# © 2026 FuchsLabs – ScreenBuddy (C++)
cmake_minimum_required(VERSION 3.24)
project(ScreenBuddy VERSION 2.5.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Statische Laufzeit: eine einzige EXE ohne Visual-C++-Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

find_package(OpenSSL REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(dpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
add_library(webview2_loader STATIC IMPORTED GLOBAL)
set_target_properties(webview2_loader PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/third_party/webview2/lib/WebView2LoaderStatic.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/webview2/include")

# MLS-Bibliotheken von D++ (statisch mitgebaut, siehe overlay-ports/dpp)
set(DPP_EXTRA_LIBS)
file(GLOB _DPP_PACKAGE_LIBDIRS "${VCPKG_INSTALLED_DIR}/vcpkg/pkgs/dpp_*/lib")
foreach(_lib mlspp mls_vectors hpke tls_syntax bytes)
    find_library(_p NAMES ${_lib} PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" ${_DPP_PACKAGE_LIBDIRS} NO_DEFAULT_PATH REQUIRED)
    list(APPEND DPP_EXTRA_LIBS "${_p}")
    unset(_p CACHE)
endforeach()

set(CORE_SOURCES
    src/util.cpp src/secure.cpp src/i18n.cpp src/config.cpp src/gfx.cpp src/render.cpp
    src/http.cpp src/netguard.cpp src/sysinfo.cpp src/bf6.cpp src/rss.cpp src/friends.cpp src/ha.cpp src/discord.cpp
    src/serial_display.cpp src/monitor_display.cpp src/winsetup.cpp src/tray.cpp src/webui.cpp src/app.cpp
    src/installer.cpp src/resources.cpp src/settings_window.cpp src/native_settings.cpp src/touch.cpp src/update.cpp
    src/companion.cpp src/companion_server.cpp src/network_display.cpp
    src/plugins.cpp src/plugin_store.cpp src/plugin_transport.cpp src/plugin_settings.cpp src/plugin_render.cpp third_party/qrcodegen/qrcodegen.cpp)

add_library(flcore STATIC ${CORE_SOURCES})
target_include_directories(flcore PUBLIC src third_party/qrcodegen)
target_compile_definitions(flcore PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_compile_options(flcore PUBLIC /utf-8 /W3 /permissive- /Zc:__cplusplus /EHsc /MP /GF)
target_link_libraries(flcore PUBLIC dpp::dpp ${DPP_EXTRA_LIBS} OpenSSL::SSL OpenSSL::Crypto Opus::opus ZLIB::ZLIB
                                    nlohmann_json::nlohmann_json tomlplusplus::tomlplusplus pugixml::pugixml httplib::httplib
                                    d2d1 dwrite windowscodecs winhttp ws2_32 iphlpapi pdh crypt32 setupapi shlwapi
                                    comctl32 comdlg32 dwmapi bcrypt advapi32 user32 gdi32 ole32 shell32
                                    webview2_loader)

# Eingebettete Dateien: bei Änderungen die Ressourcen neu bauen (CMake erkennt RCDATA-Abhängigkeiten nicht selbst)
set_source_files_properties(res/app.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/webui.html;${CMAKE_CURRENT_SOURCE_DIR}/res/ANLEITUNG.html;${CMAKE_CURRENT_SOURCE_DIR}/res/GUIDE.html;${CMAKE_CURRENT_SOURCE_DIR}/res/app.ico;${CMAKE_CURRENT_SOURCE_DIR}/res/app.manifest;${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE;${CMAKE_CURRENT_SOURCE_DIR}/../THIRD-PARTY-NOTICES.md")

add_executable(ScreenBuddy WIN32 src/main.cpp res/app.rc)
target_link_libraries(ScreenBuddy PRIVATE flcore)
target_link_options(ScreenBuddy PRIVATE /MANIFEST:NO /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA)

# Tests (Konsole): Parser, Konfiguration, Zeichnen
add_executable(fl-tests tests/tests.cpp res/app.rc)
target_link_libraries(fl-tests PRIVATE flcore)
target_link_options(fl-tests PRIVATE /MANIFEST:NO)

add_executable(fl-monitor-tests tests/monitor_tests.cpp)
target_link_libraries(fl-monitor-tests PRIVATE flcore)
target_link_options(fl-monitor-tests PRIVATE /MANIFEST:NO)

add_executable(fl-plugin-tests tests/plugins_tests.cpp src/plugins.cpp)
target_include_directories(fl-plugin-tests PRIVATE src)
target_link_libraries(fl-plugin-tests PRIVATE nlohmann_json::nlohmann_json)

add_executable(fl-plugin-store-tests tests/plugin_store_tests.cpp src/plugins.cpp src/plugin_store.cpp)
target_include_directories(fl-plugin-store-tests PRIVATE src)
target_link_libraries(fl-plugin-store-tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)

add_executable(fl-plugin-transport-tests tests/plugin_transport_tests.cpp src/plugins.cpp src/plugin_store.cpp src/plugin_transport.cpp)
target_include_directories(fl-plugin-transport-tests PRIVATE src)
target_compile_definitions(fl-plugin-transport-tests PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(fl-plugin-transport-tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto ws2_32 crypt32)

add_executable(fl-plugin-settings-tests tests/plugin_settings_tests.cpp src/plugins.cpp src/plugin_settings.cpp src/secure.cpp)
target_include_directories(fl-plugin-settings-tests PRIVATE src)
target_compile_definitions(fl-plugin-settings-tests PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(fl-plugin-settings-tests PRIVATE nlohmann_json::nlohmann_json crypt32)

# Companion-Protokoll (Kopplung, WebSocket, TLS-Server auf 127.0.0.1); unter Linux: tests/build-linux.sh
add_executable(fl-companion-tests tests/companion_tests.cpp)
target_link_libraries(fl-companion-tests PRIVATE flcore)
target_link_options(fl-companion-tests PRIVATE /MANIFEST:NO)
