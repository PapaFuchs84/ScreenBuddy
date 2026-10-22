// © 2026 FuchsLabs
// Heimnetz-Schutz: Feed-Adressen (und jede Weiterleitung) dürfen nur auf öffentliche IP-Adressen zeigen –
// kein localhost, kein Router, keine Geräte im eigenen Netz (Schutz vor SSRF).
#pragma once

#include <string>

namespace fl::netguard {

bool public_url(const std::string& url);   // http(s) und Host löst nur auf öffentliche Adressen auf
bool is_global_ip(const std::string& ip);  // für Tests

}  // namespace fl::netguard
