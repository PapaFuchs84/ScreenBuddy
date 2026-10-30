// © 2026 FuchsLabs
// In die EXE eingebettete Anleitungen und Lizenztexte.
#pragma once

#include <string_view>

namespace fl::res {

std::string_view get(const wchar_t* name);  // leer, wenn nicht vorhanden

inline constexpr const wchar_t* GUIDE_DE = L"ANLEITUNG_HTML";
inline constexpr const wchar_t* GUIDE_EN = L"GUIDE_HTML";
inline constexpr const wchar_t* PLUGIN_WORKSHOP = L"PLUGIN_WORKSHOP_HTML";
inline constexpr const wchar_t* LICENSE = L"LICENSE_TXT";
inline constexpr const wchar_t* NOTICES = L"NOTICES_MD";

}  // namespace fl::res
