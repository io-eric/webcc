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

# Set linker flags for Homebrew LLVM on macOS (if not already set by parent build)
if [ -z "$LDFLAGS_LIBCXX" ] && [[ "$OSTYPE" == "darwin"* ]]; then
    LLVM_PREFIX=$(brew --prefix llvm 2>/dev/null || echo "")
    if [ -n "$LLVM_PREFIX" ] && [ -d "$LLVM_PREFIX/lib/c++" ]; then
        export LDFLAGS_LIBCXX="-L$LLVM_PREFIX/lib/c++ -Wl,-rpath,$LLVM_PREFIX/lib/c++"
    else
        echo "Error: Homebrew LLVM not found or incomplete installation."
        echo "On macOS, this project requires Homebrew LLVM for proper linking."
        echo ""
        echo "To fix this:"
        echo "  brew install llvm"
        echo ""
        echo "Then add Homebrew LLVM to your PATH by adding this to ~/.zshrc:"
        echo "  export PATH=\"\$(brew --prefix llvm)/bin:\$PATH\""
        echo ""
        echo "Or run this command to add it automatically:"
        echo "  echo 'export PATH=\"\$(brew --prefix llvm)/bin:\$PATH\"' >> ~/.zshrc"
        exit 1
    fi
fi

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
    rm -rf build webcc schema.wcc.bin
fi

# Create build directories
mkdir -p build/obj

# Run ninja (handles C++ compilation)
NINJA_STATUS=$(ninja -n 2>&1)
if echo "$NINJA_STATUS" | grep -q "no work to do"; then
    NEEDS_COMPILE=false
else
    NEEDS_COMPILE=true
    echo "[WebCC] Building..."
    ninja
fi

# Generate schema cache if it doesn't exist or schema.def changed
if [ ! -f "schema.wcc.bin" ] || [ "schema.def" -nt "schema.wcc.bin" ]; then
    echo "[WebCC] Generating schema cache..."
    ./webcc --headers
    NEEDS_COMPILE=true  # Mark as rebuilt since headers changed
fi

if [ "$NEEDS_COMPILE" = false ]; then
    echo "[WebCC] Up to date"
    exit 0  # 0 = no rebuild needed
fi

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