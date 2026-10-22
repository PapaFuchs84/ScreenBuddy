// © 2026 FuchsLabs
// Geheimnisse (Bot-Token, Schlüssel) mit Windows-DPAPI verschlüsseln: nur dasselbe Windows-Benutzerkonto auf
// demselben PC kann sie wieder lesen. Format wie Version 1.x ("dpapi:" + Base64), alte Konfigurationen bleiben lesbar.
#pragma once

#include <string>

namespace fl::secure {

std::string protect(const std::string& text);
// Entschlüsselt "dpapi:…"; Klartext (alte Konfigurationen) unverändert zurück; nicht entschlüsselbar → leer.
std::string unprotect(const std::string& value);
bool is_protected(const std::string& value);

std::string base64_encode(const std::string& data);
std::string base64_decode(const std::string& text, bool* ok = nullptr);

}  // namespace fl::secure
