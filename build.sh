#!/usr/bin/env bash
set -euo pipefail

# Build script for faster_cfr CFR solver
# Compiles all C++ source files and links them into an executable

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Configuration
CXX="${CXX:-g++-15}"  # Use g++-15 (Homebrew GCC) by default, override with CXX env var
OUTPUT="run"
INCLUDE_DIR="include"
SRC_DIR="src"

# Compiler flags
CXXFLAGS=(
    -std=c++17              # C++17 required for structured bindings
    -O3                     # Maximum optimization
    -march=native           # Optimize for current CPU
    -Wall                   # Enable all warnings
    -Wextra                 # Extra warnings
    -I"$INCLUDE_DIR"        # Include header directory
)

# Source files to compile
SOURCES=(
    "$SRC_DIR/CFR.cpp"
    "$SRC_DIR/GameState.cpp"
    "$SRC_DIR/InfoSet.cpp"
    "$SRC_DIR/Utils.cpp"
    "$SRC_DIR/Profiler.cpp"
    "$SRC_DIR/MinimalProfiler.cpp"
    "$SRC_DIR/VectorPool.cpp"
    "$SRC_DIR/main.cpp"
)

echo "=== Building faster_cfr ==="
echo "Compiler: $CXX"
echo "Flags: ${CXXFLAGS[*]}"
echo "Sources: ${SOURCES[*]}"
echo ""

# Compile and link
"$CXX" "${CXXFLAGS[@]}" "${SOURCES[@]}" -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo "✓ Executable: $OUTPUT"
    echo ""
    echo "Run with: ./$OUTPUT"
    echo "Or use: ./run.sh"
else
    echo ""
    echo "✗ Build failed!"
    exit 1
fi

