#!/bin/sh
# © 2026 FuchsLabs – plattformneutrale Companion-Teile unter Linux bauen und testen (Protokoll, Kopplung, TLS-Server).
# Braucht g++ (C++20), OpenSSL-Entwicklerdateien und nlohmann/json (Ordner mit nlohmann/json.hpp in NLOHMANN_INCLUDE,
# sonst wird /usr/include genommen). Der Rest der App ist Windows-only und wird hier nicht gebaut.
set -eu
cd "$(dirname "$0")/.."
OUT="${OUT:-build-linux}"
mkdir -p "$OUT"
g++ -std=c++20 -O1 -g -Wall -Wextra -pthread \
    -Isrc -Ithird_party/qrcodegen -I"${NLOHMANN_INCLUDE:-/usr/include}" \
    src/companion.cpp src/companion_server.cpp third_party/qrcodegen/qrcodegen.cpp tests/companion_tests.cpp \
    -lssl -lcrypto -o "$OUT/companion-tests"
"$OUT/companion-tests"
# Streaming-Neustarts (TMDB): reine Logik, feste JSON-Beispiele, kein Netzzugriff
g++ -std=c++20 -O1 -g -Wall -Wextra -Isrc -I"${NLOHMANN_INCLUDE:-/usr/include}" \
    src/tmdb_logic.cpp tests/tmdb_tests.cpp -o "$OUT/tmdb-tests"
"$OUT/tmdb-tests"
