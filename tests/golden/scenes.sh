#!/bin/sh
# Run every scene script in tests/golden/scenes/ against its golden trace.
#
#   tests/golden/scenes.sh BINARY            compare each scene
#   tests/golden/scenes.sh --update BINARY   regenerate every scene's trace
#
# Each script's first line is "# frames N", the trace length; "# save 100",
# "# level N" and "# players N" are the other headers tests/golden.sh reads.
# A scene is a scripted path into one part of the game (a race in each
# vehicle, the pause menu, a time trial, a finished race and its results, a
# two-player race, a boss race, a hub world...) so that a change to the game
# logic has a trace that exercises it; see tests/golden.sh for what a trace
# hashes and linux/input.c for the script format (p<N> controllers, AUTO).
set -u
update=0
if [ "${1:-}" = "--update" ]; then
    update=1
    shift
fi
bin=${1:?usage: tests/golden/scenes.sh [--update] BINARY}
here=$(dirname "$0")
status=0
for script in "$here"/scenes/*.txt; do
    name=$(basename "$script" .txt)
    frames=$(sed -n 's/^# frames \([0-9]*\).*/\1/p' "$script" | head -n 1)
    if [ -z "$frames" ]; then
        echo "scenes: $script has no '# frames N' header"
        status=1
        continue
    fi
    if [ "$update" = 1 ]; then
        "$here/../golden.sh" --update "$bin" "$here/scenes/$name.trace" "$frames" "$script" || status=1
    else
        "$here/../golden.sh" "$bin" "$here/scenes/$name.trace" "$frames" "$script" || status=1
    fi
done
exit $status
