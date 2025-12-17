#!/bin/bash

# This script compiles the C++ code to WebAssembly and places the output in ./dist/

set -e

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$SCRIPT_DIR/../.."

echo "Building webcc_dom example..."

# Run webcc from the repo root (webcc expects to be run from there)
# cd "$REPO_ROOT"
# ./webcc examples/webcc_canvas/example.cc
"$REPO_ROOT/webcc" example.cc

# Create output directory
cd "$SCRIPT_DIR"
mkdir -p dist

# Move generated files to dist/
mv app.js index.html app.wasm dist/

echo "Build complete! Files are in ./dist/"
echo "To view the demo, run: cd dist && python3 -m http.server"