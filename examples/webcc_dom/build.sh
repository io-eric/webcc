#!/bin/bash

# This script compiles the C++ code to WebAssembly and places the output in ./dist/

set -e

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$SCRIPT_DIR/../.."

echo "Building webcc_dom example..."

# Run webcc from the repo root (webcc expects to be run from there)
cd "$REPO_ROOT"
./webcc examples/webcc_dom/example.cc --out "$SCRIPT_DIR/dist"

echo "Build complete! Files are in ./dist/"
echo "To view the demo, run: cd dist && python3 -m http.server"