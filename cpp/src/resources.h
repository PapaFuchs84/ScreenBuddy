// © 2026 FuchsLabs
// In die EXE eingebettete Dateien (Einstellungsseite, Anleitungen).
#pragma once

#include <string_view>

namespace fl::res {

std::string_view get(const wchar_t* name);  // leer, wenn nicht vorhanden

inline constexpr const wchar_t* WEBUI = L"WEBUI_HTML";
inline constexpr const wchar_t* GUIDE_DE = L"ANLEITUNG_HTML";
inline constexpr const wchar_t* GUIDE_EN = L"GUIDE_HTML";
inline constexpr const wchar_t* LICENSE = L"LICENSE_TXT";
inline constexpr const wchar_t* NOTICES = L"NOTICES_MD";

}  // namespace fl::res
