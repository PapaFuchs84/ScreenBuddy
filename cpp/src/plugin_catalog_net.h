// © 2026 FuchsLabs – real HTTPS fetcher for the plugin store (bytes stay untouched for signature checks).
#pragma once
#include "plugin_catalog.h"

namespace fl::plugins::catalog {
// Fetcher for `base_url`: only relative paths made of [a-z0-9._/-] without ".." or leading '/', HTTPS to the fixed
// store host, public addresses, no redirects/proxy (see plugin_transport). Not for the UI thread (blocks up to 5 s).
Fetcher public_https_fetcher();
} // namespace fl::plugins::catalog
