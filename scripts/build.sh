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

# Install to the standard plugin folders (macOS only; other platforms use
# their own install locations).
if [[ "$(uname)" == "Darwin" ]]; then
    if [ -d "$ARTEFACTS/VST3" ]; then
        DEST="$HOME/Library/Audio/Plug-Ins/VST3"
        mkdir -p "$DEST"
        cp -R "$ARTEFACTS"/VST3/*.vst3 "$DEST/" 2>/dev/null || true
        echo "Installed VST3 -> $DEST"
    fi

    if [ -d "$ARTEFACTS/AU" ]; then
        DEST="$HOME/Library/Audio/Plug-Ins/Components"
        mkdir -p "$DEST"
        cp -R "$ARTEFACTS"/AU/*.component "$DEST/" 2>/dev/null || true
        echo "Installed AU  -> $DEST"
    fi
fi

if [ -d "$ARTEFACTS/Standalone" ]; then
    echo "Standalone app -> $ARTEFACTS/Standalone"
fi
