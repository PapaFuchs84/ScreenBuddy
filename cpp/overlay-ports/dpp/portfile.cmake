vcpkg_download_distfile(DPP_BACKPORT_1636_PATCH
    URLS https://github.com/brainboxdotcc/DPP/commit/5365da807ca589cb07c1686540e3f5f0260511a1.patch?full_index=1
    FILENAME dpp_backport_1636.patch
    SHA512 c0ef265811e3f4719c22d857f9a70cad3540f02a92215807aeca3337fd86d39ab3c7cb9d01db10f4ac2ce5a4a920593fea1f9169cee5b4f768e5595f6c6c1d73
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO brainboxdotcc/DPP
    REF "v${VERSION}"
    SHA512 4da74166ed68730ee60435fcac8ed79f206968a07c21228db14a7c1e7a39c922df7a8088276528b620d285d34529e2ce61f7bb03eccef8fea6231e81537d874d
    PATCHES
        "${DPP_BACKPORT_1636_PATCH}"
        use-external-json.patch
)

file(REMOVE_RECURSE
    "${SOURCE_PATH}/include/dpp/nlohmann"
)

# FuchsLabs: statische Bibliothek für eine einzige EXE (das Original-Port erlaubt unter Windows nur eine DLL)
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(READ "${SOURCE_PATH}/library-vcpkg/CMakeLists.txt" _c)
    string(REPLACE "add_library(\"\${LIB_NAME}\" SHARED" "add_library(\"\${LIB_NAME}\" STATIC" _c "${_c}")
    foreach(_lib mlspp mls_vectors bytes tls_syntax hpke)
        string(REGEX REPLACE "([\r\n]\t)${_lib}([\r\n])" "\\1$<BUILD_INTERFACE:${_lib}>\\2" _c "${_c}")
    endforeach()
    string(REGEX REPLACE "if \\(WIN32\\)([\r\n\t ]+install\\([\r\n\t ]+FILES[\r\n\t ]+\"\\$<TARGET_PDB_FILE)" "if (FALSE)\\1" _c "${_c}")
    # ohne Link-Time-Codegenerierung (/GL): sonst wird die Bibliothek mehrere GB groß und das Linken sehr langsam
    string(REPLACE "/O2;/Oi;/Oy;/GL;/Gy" "/O2;/Oi;/Oy;/Gy" _c "${_c}")
    string(APPEND _c "\ntarget_compile_definitions(\"\${LIB_NAME}\" PUBLIC DPP_STATIC)\n")
    file(WRITE "${SOURCE_PATH}/library-vcpkg/CMakeLists.txt" "${_c}")
endif()

# FuchsLabs: Zertifikate aus dem Windows-Zertifikatspeicher prüfen (statisches OpenSSL kennt sonst keine Stammzertifikate)
if(VCPKG_TARGET_IS_WINDOWS)
    file(READ "${SOURCE_PATH}/src/dpp/ssl_context.cpp" _s)
    string(REPLACE "SSL_CTX_set_default_verify_paths(context->context) != 1"
                   "(SSL_CTX_load_verify_store(context->context, \"org.openssl.winstore://\"), SSL_CTX_set_default_verify_paths(context->context)) != 1"
                   _s "${_s}")
    file(WRITE "${SOURCE_PATH}/src/dpp/ssl_context.cpp" "${_s}")
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    set(VCPKG_BUILD_TYPE release)  # die App wird nur als Release gebaut
    # ohne eingebettete Debug-Infos (/Z7), die die statische Bibliothek unnötig aufblähen
    set(_dpp_flags OPTIONS_RELEASE "-DCMAKE_CXX_FLAGS_RELEASE=/MT /O2 /Oi /Gy /DNDEBUG" "-DCMAKE_C_FLAGS_RELEASE=/MT /O2 /Oi /Gy /DNDEBUG")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        -DAVX_TYPE=AVX0
        -DDPP_USE_EXTERNAL_JSON=ON
        -DDPP_BUILD_TEST=OFF
    ${_dpp_flags}
)

vcpkg_cmake_install()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    # MLS-Bibliotheken (Sprach-Verschlüsselung von D++) mitliefern; die EXE bindet sie direkt ein
    file(GLOB_RECURSE _rel "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/library-vcpkg/mlspp/*.lib")
    file(INSTALL ${_rel} DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    if(NOT VCPKG_BUILD_TYPE)
        file(GLOB_RECURSE _dbg "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/library-vcpkg/mlspp/*.lib")
        file(INSTALL ${_dbg} DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
    endif()
endif()

vcpkg_cmake_config_fixup(NO_PREFIX_CORRECTION)

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share/dpp"
)

vcpkg_install_copyright(
    FILE_LIST
        "${SOURCE_PATH}/LICENSE"
        "${SOURCE_PATH}/mlspp/LICENSE" # BSD-2-Clause
        "${SOURCE_PATH}/src/dpp/dave/clock.h" # MIT
        "${SOURCE_PATH}/src/dpp/dave/leb128.h" # BSD-3-Clause
)
