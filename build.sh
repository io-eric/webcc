#!/bin/bash

set -e

# Check for Clang 16+ (required for full C++20 support)
check_clang_version() {
    if ! command -v clang++ &> /dev/null; then
        echo "Error: clang++ not found. Please install Clang 16 or later."
        echo "  Ubuntu/Debian: sudo apt install clang-16"
        echo "  macOS: brew install llvm"
        exit 1
    fi
    
    CLANG_VERSION=$(clang++ --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1 | cut -d. -f1)
    if [ -z "$CLANG_VERSION" ]; then
        echo "Warning: Could not detect Clang version"
    elif [ "$CLANG_VERSION" -lt 16 ]; then
        echo "Error: Clang $CLANG_VERSION detected. WebCC requires Clang 16+ for full C++20 support."
        echo "  Ubuntu/Debian: sudo apt install clang-16"
        echo "  macOS: brew install llvm"
        exit 1
    fi
}

check_clang_version

FORCE_REBUILD=false
for arg in "$@"; do
    case $arg in
        --force|-f)
            FORCE_REBUILD=true
            ;;
    esac
done

# Force clean if requested
if [ "$FORCE_REBUILD" = true ]; then
    rm -rf build src/cli/webcc_schema.h webcc
fi

# Create build directories
mkdir -p build/bootstrap build/final

# Check if there's work to do BEFORE deleting anything
if ninja -n 2>&1 | grep -q "no work to do"; then
    echo "[WebCC] Up to date"
    exit 0  # 0 = no rebuild needed
fi

# CRITICAL: Remove the schema header before building bootstrap.
# The bootstrap compiler must NOT find webcc_schema.h (it uses #include "webcc_schema.h"
# which resolves relative to the source file, bypassing include path settings).
# Ninja will regenerate it via the generate_schema rule.
rm -f src/cli/webcc_schema.h
# Also remove bootstrap objects to force recompilation without schema
rm -f build/bootstrap/*.o

# Actually run the build
echo "[WebCC] Building..."
ninja

echo "[WebCC] Done."

# In CI, just exit successfully
if [ -n "$CI" ]; then
    exit 0
fi

# Exit code 2 = rebuilt successfully (signals to parent builds that schema changed)
# Check if webcc is already linked correctly
if command -v webcc >/dev/null 2>&1 && [ "$(command -v webcc)" -ef "$PWD/webcc" ]; then
    echo "webcc is already configured in PATH."
    exit 2  # 2 = rebuilt
fi

# Only offer install if /usr/local/bin exists (common on Linux/macOS)
# And we are in an interactive terminal and not in CIs
if [ -d "/usr/local/bin" ] && [ -t 0 ] && [ -z "$CI" ]; then
    echo ""
    echo "To use 'webcc' from anywhere, it needs to be in your PATH."
    read -p "Would you like to create a symlink in /usr/local/bin? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        TARGET="/usr/local/bin/webcc"
        if [ -w /usr/local/bin ]; then
            ln -sf "$PWD/webcc" "$TARGET"
        else
            echo "Need sudo access to write to /usr/local/bin"
            sudo ln -sf "$PWD/webcc" "$TARGET"
        fi
        echo "Symlink created: $TARGET -> $PWD/webcc"
        echo "You can now use 'webcc' command from any directory."
    fi
fi

exit 2  # 2 = rebuilt (only reached when not in CI)