#!/bin/bash
# WebCC test runner.
#
#   ./tests/run.sh            Build & run all tests (C++ units + codegen snapshots + JS validation)
#   ./tests/run.sh --update   Regenerate golden snapshots, then run
#   ./tests/run.sh --skip-js  Skip the Node JS-validation layer (no webcc build / node needed)
#
# Zero extra dependencies: uses the same clang++ the toolchain requires, plus
# node (already needed to serve examples) for the JS syntax checks.
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/tests/build"
mkdir -p "$BUILD"

UPDATE=0
SKIP_JS=0
for arg in "$@"; do
    case "$arg" in
        --update) UPDATE=1 ;;
        --skip-js) SKIP_JS=1 ;;
        *) echo "Unknown option: $arg"; exit 2 ;;
    esac
done

CXX="${CXX:-clang++}"

echo "[tests] Compiling C++ test suite..."
"$CXX" -std=c++20 -O1 -g \
    -I "$ROOT/src/cli" -I "$ROOT/src/core" -I "$ROOT/include" \
    -DWEBCC_SCHEMA_DEF="\"$ROOT/schema.def\"" \
    -DWEBCC_SNAPSHOT_DIR="\"$ROOT/tests/snapshots\"" \
    "$ROOT/tests/test_main.cc" \
    "$ROOT/tests/test_command_buffer.cc" \
    "$ROOT/tests/test_schema.cc" \
    "$ROOT/tests/test_codegen.cc" \
    "$ROOT/tests/test_allocator.cc" \
    "$ROOT/tests/test_containers.cc" \
    "$ROOT/src/cli/schema.cc" \
    "$ROOT/src/cli/utils.cc" \
    "$ROOT/src/cli/generators.cc" \
    "$ROOT/src/core/command_buffer.cc" \
    -o "$BUILD/tests"

echo "[tests] Running C++ test suite..."
if [ "$UPDATE" = "1" ]; then
    WEBCC_UPDATE_SNAPSHOTS=1 "$BUILD/tests"
    echo "[tests] Snapshots updated. Re-running to verify..."
fi
"$BUILD/tests"
CPP_STATUS=$?

if [ "$SKIP_JS" = "1" ]; then
    echo "[tests] Skipping JS validation (--skip-js)."
    exit $CPP_STATUS
fi

if ! command -v node >/dev/null 2>&1; then
    echo "[tests] node not found; skipping JS validation layer."
    exit $CPP_STATUS
fi

# Ensure the webcc binary exists for JS generation.
if [ ! -x "$ROOT/webcc" ]; then
    echo "[tests] webcc binary not found; building it (ninja)..."
    ( cd "$ROOT" && ninja )
fi

echo "[tests] Validating generated JavaScript..."
node "$ROOT/tests/js/check_js.mjs" "$ROOT/webcc" "$ROOT"
