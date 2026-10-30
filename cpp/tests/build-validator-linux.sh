#!/bin/sh
# © 2026 FuchsLabs – Plugin-Parser, Beispiel-Tests und Validator unter Linux bauen und prüfen (auch für Store-CI).
# Braucht g++ (C++20), OpenSSL-Entwicklerdateien (libssl-dev) und nlohmann/json (Ordner mit nlohmann/json.hpp in NLOHMANN_INCLUDE, sonst /usr/include).
set -eu
cd "$(dirname "$0")/.."
OUT="${OUT:-build-linux}"
INC="${NLOHMANN_INCLUDE:-/usr/include}"
mkdir -p "$OUT"
CXX="g++ -std=c++20 -O1 -g -Wall -Wextra -Isrc -I$INC"
$CXX src/plugins.cpp tests/plugins_tests.cpp -o "$OUT/plugin-tests"
$CXX -DFL_PLUGIN_EXAMPLE_DIR="\"$PWD/examples/plugins\"" src/plugins.cpp tests/plugin_examples_tests.cpp -o "$OUT/plugin-example-tests"
$CXX src/plugins.cpp tools/plugin_validate.cpp -o "$OUT/fl-plugin-validate"
$CXX src/plugins.cpp src/plugin_sign.cpp tests/plugin_sign_tests.cpp -lcrypto -o "$OUT/plugin-sign-tests"
$CXX src/plugins.cpp src/plugin_sign.cpp tools/plugin_sign.cpp -lcrypto -o "$OUT/fl-plugin-sign"
"$OUT/plugin-tests"
"$OUT/plugin-example-tests"
"$OUT/plugin-sign-tests"
# Validator: Beispiele müssen die Store-Regeln erfüllen, Fixtures müssen scheitern.
"$OUT/fl-plugin-validate" --store --app-version 2.5.0 examples/plugins
for f in tests/plugin-fixtures/*.flplugin.json; do
    if "$OUT/fl-plugin-validate" --store "$f" >/dev/null; then echo "FAIL: $f wurde akzeptiert"; exit 1; fi
done
# Lokaler Modus: Plugin ohne Store-Metadaten ist gültig, falsche Lizenz nicht.
"$OUT/fl-plugin-validate" tests/plugin-fixtures/no-metadata.flplugin.json >/dev/null
if "$OUT/fl-plugin-validate" tests/plugin-fixtures/bad-license.flplugin.json >/dev/null; then echo "FAIL: falsche Lizenz akzeptiert"; exit 1; fi
# Signierung Ende-zu-Ende: Index bauen, signieren, prüfen; Manipulation und falscher Schlüssel müssen scheitern.
T="$OUT/sign-e2e"; rm -rf "$T"; mkdir -p "$T/root"
for f in examples/plugins/*.flplugin.json; do
    id=$(basename "$f" .flplugin.json)
    mkdir -p "$T/root/$id"; cp "$f" "$T/root/$id/$id.flplugin.json"
done
"$OUT/fl-plugin-sign" keygen "$T/key.pem" > "$T/keygen.txt"
PUB=$(tail -n 1 "$T/keygen.txt")
"$OUT/fl-plugin-sign" keygen "$T/key2.pem" > "$T/keygen2.txt"
PUB2=$(tail -n 1 "$T/keygen2.txt")
if "$OUT/fl-plugin-sign" keygen "$T/key.pem" >/dev/null; then echo "FAIL: Schlüssel überschrieben"; exit 1; fi
"$OUT/fl-plugin-sign" build-index "$T/root" "$T/out" 1
"$OUT/fl-plugin-sign" sign index "$T/out/index.json" "$T/key.pem"
"$OUT/fl-plugin-sign" verify index "$T/out/index.json" "$PUB"
if "$OUT/fl-plugin-sign" verify index "$T/out/index.json" "$PUB2" >/dev/null; then echo "FAIL: falscher Schlüssel akzeptiert"; exit 1; fi
if "$OUT/fl-plugin-sign" verify revoked "$T/out/index.json" "$PUB" >/dev/null; then echo "FAIL: Index als Sperrliste akzeptiert"; exit 1; fi
echo " " >> "$T/out/index.json"
if "$OUT/fl-plugin-sign" verify index "$T/out/index.json" "$PUB" >/dev/null; then echo "FAIL: manipulierter Index akzeptiert"; exit 1; fi
echo "validator: ok"
