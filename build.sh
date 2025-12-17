#!/bin/bash

set -e

echo "[WebCC] Building webcc tool..."
g++ -std=c++17 -O2 -o webcc \
    src/webcc.cc \
    src/command_buffer.cc \
    -I include -I src

echo "[WebCC] Build complete: webcc"

echo "[WebCC] Generating headers..."
./webcc headers
