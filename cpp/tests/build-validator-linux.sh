#!/bin/sh
# © 2026 FuchsLabs – Plugin-Parser, Beispiel-Tests und Validator unter Linux bauen und prüfen (auch für Store-CI).
# Braucht g++ (C++20) und nlohmann/json (Ordner mit nlohmann/json.hpp in NLOHMANN_INCLUDE, sonst /usr/include).
set -eu
cd "$(dirname "$0")/.."
OUT="${OUT:-build-linux}"
INC="${NLOHMANN_INCLUDE:-/usr/include}"
mkdir -p "$OUT"
CXX="g++ -std=c++20 -O1 -g -Wall -Wextra -Isrc -I$INC"
$CXX src/plugins.cpp tests/plugins_tests.cpp -o "$OUT/plugin-tests"
$CXX -DFL_PLUGIN_EXAMPLE_DIR="\"$PWD/examples/plugins\"" src/plugins.cpp tests/plugin_examples_tests.cpp -o "$OUT/plugin-example-tests"
$CXX src/plugins.cpp tools/plugin_validate.cpp -o "$OUT/fl-plugin-validate"
"$OUT/plugin-tests"
"$OUT/plugin-example-tests"
# Validator: Beispiele müssen die Store-Regeln erfüllen, Fixtures müssen scheitern.
"$OUT/fl-plugin-validate" --store --app-version 2.5.0 examples/plugins
for f in tests/plugin-fixtures/*.flplugin.json; do
    if "$OUT/fl-plugin-validate" --store "$f" >/dev/null; then echo "FAIL: $f wurde akzeptiert"; exit 1; fi
done
# Lokaler Modus: Plugin ohne Store-Metadaten ist gültig, falsche Lizenz nicht.
"$OUT/fl-plugin-validate" tests/plugin-fixtures/no-metadata.flplugin.json >/dev/null
if "$OUT/fl-plugin-validate" tests/plugin-fixtures/bad-license.flplugin.json >/dev/null; then echo "FAIL: falsche Lizenz akzeptiert"; exit 1; fi
echo "validator: ok"
