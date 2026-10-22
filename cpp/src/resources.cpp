// © 2026 FuchsLabs
#include "resources.h"

#include <windows.h>

namespace fl::res {

std::string_view get(const wchar_t* name) {
    HRSRC r = FindResourceW(nullptr, name, RT_RCDATA);
    if (!r) return {};
    HGLOBAL g = LoadResource(nullptr, r);
    if (!g) return {};
    return {(const char*)LockResource(g), (size_t)SizeofResource(nullptr, r)};
}

}  // namespace fl::res
