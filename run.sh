#!/usr/bin/env bash
set -euo pipefail

# Run script for faster_cfr CFR solver
# Executes the compiled binary

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

EXECUTABLE="./run"

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable '$EXECUTABLE' not found!"
    echo "Please build first with: ./build.sh"
    exit 1
fi

# Check if executable has execute permissions
if [ ! -x "$EXECUTABLE" ]; then
    echo "Setting execute permissions on $EXECUTABLE"
    chmod +x "$EXECUTABLE"
fi

echo "=== Running faster_cfr ==="
echo ""

# Run the executable, passing any command-line arguments through
"$EXECUTABLE" "$@"

