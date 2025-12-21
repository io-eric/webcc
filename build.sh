#!/bin/bash

set -e

# Clean up previous build artifacts to ensure a clean bootstrap
rm -f src/cli/webcc_schema.h

echo "[WebCC] 1/3 Compiling bootstrap compiler..."
# Compile without schema support first
g++ -std=c++20 -O3 -o webcc_bootstrap \
    src/cli/main.cc \
    -I include -I src/cli -I src/core

echo "[WebCC] 2/3 Generating headers..."
# Use bootstrap compiler to generate headers and schema definition
./webcc_bootstrap --headers

echo "[WebCC] 3/3 Compiling final compiler..."
# Compile final version which will now include the generated webcc_schema.h
g++ -std=c++20 -O3 -o webcc \
    src/cli/main.cc \
    -I include -I src/cli -I src/core

# Cleanup
rm webcc_bootstrap

echo "[WebCC] Done."
