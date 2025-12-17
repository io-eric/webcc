#!/bin/bash

set -e

echo "[WebCC] Building webcc tool..."
g++ -std=c++20 -O3 -o webcc \
    src/webcc.cc \
    -I include -I src

echo "[WebCC] Build complete: webcc"

echo "[WebCC] Generating headers..."
./webcc --headers
