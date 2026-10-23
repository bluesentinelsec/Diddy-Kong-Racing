#!/bin/sh
# Traces never need sound: silence SDL's audio so a run cannot surprise anyone wearing headphones.
export SDL_AUDIO_DRIVER=dummy
export SDL_AUDIODRIVER=dummy
# Golden-trace regression check.
#
#   tests/golden.sh BINARY [GOLDEN] [FRAMES] [INPUT]            compare against the golden trace
#   tests/golden.sh --update BINARY [GOLDEN] [FRAMES] [INPUT]   regenerate the golden trace
#
# Runs the game with --trace for FRAMES frames (erased save unless the script
# says "# save 100", frame-counted clock, and either no input or the scripted
# controller in INPUT - see DKR_INPUT_SCRIPT in linux/input.c) and diffs the per-frame gfx/audio hashes
# against GOLDEN. The
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
bin=${1:?usage: tests/golden.sh [--update] BINARY [GOLDEN] [FRAMES] [INPUT]}
golden=${2:-tests/golden/us_v80.trace}
frames=${3:-1800}
input=${4:-}
if [ -n "$input" ]; then
    export DKR_INPUT_SCRIPT=$input
    # "# save 100" in the script starts the trace from the 100%-completion save
    # (DKR_TRACE_SAVE, linux/reimpl.c) instead of an erased one.
    save=$(sed -n 's/^# save \([0-9a-z]*\).*/\1/p' "$input" | head -n 1)
    # "# level N" starts level N from the title menu (DKR_TRACE_LEVEL,
    # linux/trace.c), "# players N" with N human players (DKR_TRACE_PLAYERS).
    level=$(sed -n 's/^# level \([0-9]*\).*/\1/p' "$input" | head -n 1)
    players=$(sed -n 's/^# players \([0-9]*\).*/\1/p' "$input" | head -n 1)
    # "# adventure" starts that level in adventure mode (DKR_TRACE_ADVENTURE).
    if grep -q '^# adventure' "$input"; then export DKR_TRACE_ADVENTURE=1; else unset DKR_TRACE_ADVENTURE; fi
    if [ -n "$level" ]; then export DKR_TRACE_LEVEL=$level; else unset DKR_TRACE_LEVEL; fi
    if [ -n "$players" ]; then export DKR_TRACE_PLAYERS=$players; else unset DKR_TRACE_PLAYERS; fi
    if [ -n "$save" ]; then
        export DKR_TRACE_SAVE=$save
    else
        unset DKR_TRACE_SAVE
    fi
else
    unset DKR_INPUT_SCRIPT
    unset DKR_TRACE_SAVE
    unset DKR_TRACE_LEVEL
    unset DKR_TRACE_PLAYERS
    unset DKR_TRACE_ADVENTURE
fi

if [ "$update" = 1 ]; then
    "$bin" --windowed --trace "$golden" --trace-frames "$frames"
    echo "golden: wrote $golden"
    exit 0
fi

tmp=$(mktemp "${TMPDIR:-/tmp}/dkr_trace.XXXXXX")
trap 'rm -f "$tmp"' EXIT
"$bin" --windowed --trace "$tmp" --trace-frames "$frames"

if cmp -s "$tmp" "$golden"; then
    echo "golden: $frames frames identical to $golden${input:+ (input $input)}"
    exit 0
fi
echo "golden: MISMATCH against $golden"
diff "$golden" "$tmp" | head -n 8
exit 1
