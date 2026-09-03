#!/usr/bin/env bash
# Installs the freshly built plugin into the user plugin folders (macOS only).
# Exits non-zero if any copy fails, so it never reports a bogus success.
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${1:-build}"
ARTEFACTS="$BUILD_DIR/SfxrVsti_artefacts/Release"

if [[ "$(uname)" != "Darwin" ]]; then
    echo "install.sh is macOS-only. Copy $ARTEFACTS manually on other platforms." >&2
    exit 1
fi

installed_any=0

if [ -d "$ARTEFACTS/VST3" ]; then
    DEST="$HOME/Library/Audio/Plug-Ins/VST3"
    mkdir -p "$DEST"
    rm -rf "$DEST/SfxrVsti.vst3"
    if cp -R "$ARTEFACTS/VST3/SfxrVsti.vst3" "$DEST/"; then
        echo "Installed VST3 -> $DEST/SfxrVsti.vst3"
        installed_any=1
    else
        echo "Failed to install VST3" >&2
        exit 1
    fi
fi

if [ -d "$ARTEFACTS/AU" ]; then
    DEST="$HOME/Library/Audio/Plug-Ins/Components"
    mkdir -p "$DEST"
    rm -rf "$DEST/SfxrVsti.component"
    if cp -R "$ARTEFACTS/AU/SfxrVsti.component" "$DEST/"; then
        echo "Installed AU  -> $DEST/SfxrVsti.component"
        installed_any=1
    else
        echo "Failed to install AU" >&2
        exit 1
    fi
fi

if [ "$installed_any" -eq 0 ]; then
    echo "No plugin artefacts found under $ARTEFACTS (run ./scripts/build.sh first)." >&2
    exit 1
fi
