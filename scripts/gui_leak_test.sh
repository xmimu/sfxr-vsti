#!/usr/bin/env bash
# Runs the GUI leak test and fails if JUCE's LeakedObjectDetector reports any
# leaked object (printed at process exit).
set -euo pipefail

bin="$1"
out="$(mktemp)"
trap 'rm -f "$out"' EXIT

"$bin" >"$out" 2>&1 || { rc=$?; cat "$out"; exit $rc; }

if grep -q "Leaked" "$out"; then
    echo "LeakedObjectDetector reported leaks:" >&2
    cat "$out" >&2
    exit 1
fi

exit 0
