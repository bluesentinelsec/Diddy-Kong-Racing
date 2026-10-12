#!/bin/sh
# Golden-trace regression check.
#
#   tests/golden.sh BINARY [GOLDEN] [FRAMES]            compare against the golden trace
#   tests/golden.sh --update BINARY [GOLDEN] [FRAMES]   regenerate the golden trace
#
# Runs the game with --trace for FRAMES frames (no input, erased save, frame-
# counted clock) and diffs the per-frame gfx/audio hashes against GOLDEN. The
# hashes cover what the renderer and the audio output receive, not GPU pixels,
# so a 32-bit Raspberry Pi and a 64-bit Mac must produce the same file; the
# first differing frame is reported. Needs assets/ next to the binary or in the
# current directory.
set -eu

update=0
if [ "${1:-}" = "--update" ]; then
    update=1
    shift
fi
bin=${1:?usage: tests/golden.sh [--update] BINARY [GOLDEN] [FRAMES]}
golden=${2:-tests/golden/us_v80.trace}
frames=${3:-1800}

if [ "$update" = 1 ]; then
    "$bin" --windowed --trace "$golden" --trace-frames "$frames"
    echo "golden: wrote $golden"
    exit 0
fi

tmp=$(mktemp "${TMPDIR:-/tmp}/dkr_trace.XXXXXX")
trap 'rm -f "$tmp"' EXIT
"$bin" --windowed --trace "$tmp" --trace-frames "$frames"

if cmp -s "$tmp" "$golden"; then
    echo "golden: $frames frames identical to $golden"
    exit 0
fi
echo "golden: MISMATCH against $golden"
diff "$golden" "$tmp" | head -n 8
exit 1
