#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${1:-build}"

# Portable CPU count (macOS / Linux / BSD).
if command -v nproc >/dev/null 2>&1; then
    NPROC=$(nproc)
else
    NPROC=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$NPROC"

ARTEFACTS="$BUILD_DIR/SfxrVsti_artefacts/Release"
echo ""
echo "Build complete. Artefacts in $ARTEFACTS"
echo ""

if [[ "$(uname)" == "Darwin" ]]; then
    echo "To install the plugin into ~/Library/Audio/Plug-Ins, run:"
    echo "  ./scripts/install.sh"
elif [ -d "$ARTEFACTS/VST3" ]; then
    echo "VST3 in $ARTEFACTS/VST3"
fi
if [ -d "$ARTEFACTS/Standalone" ]; then
    echo "Standalone app -> $ARTEFACTS/Standalone"
fi
