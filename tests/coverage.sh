#!/bin/sh
# Which game code do the golden traces exercise?
#
#   tests/coverage.sh BINARY     (BINARY built with -DDKR_COVERAGE=ON; `make coverage` does both)
#
# Runs every golden trace on the instrumented binary (each must still match its
# golden), merges the profiles and writes, under build/coverage/report/:
#   summary.txt     per-file line and function coverage of the game (src/), lowest first
#   unexecuted.txt  every game function no trace ever enters, grouped by file
# A function listed in unexecuted.txt is code a refactor can break without any
# trace noticing: add a scene that reaches it before rewriting it.
set -eu
bin=${1:?usage: tests/coverage.sh BINARY}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
out=$(cd "$(dirname "$bin")" && pwd)/report
mkdir -p "$out"
rm -f "$out"/*.profraw

if command -v xcrun >/dev/null 2>&1; then
    profdata="xcrun llvm-profdata"
    cov="xcrun llvm-cov"
else
    profdata=llvm-profdata
    cov=llvm-cov
fi

LLVM_PROFILE_FILE="$out/trace-%p.profraw"
export LLVM_PROFILE_FILE
status=0
"$here/golden.sh" "$bin" "$here/golden/us_v80.trace" 1800 || status=1
"$here/golden.sh" "$bin" "$here/golden/us_v80_drive.trace" 7000 "$here/golden/drive.txt" || status=1
"$here/golden/scenes.sh" "$bin" || status=1
unset LLVM_PROFILE_FILE

$profdata merge -sparse "$out"/*.profraw -o "$out/traces.profdata"
rm -f "$out"/*.profraw

# Only the game: the host layer and the N64 audio library are not what the traces guard.
ignore='(/linux/|/libultra/|/build/|/_deps/|/tests/)'
$cov report "$bin" -instr-profile="$out/traces.profdata" -ignore-filename-regex="$ignore" \
    -sources "$root/src" >"$out/report-raw.txt"

# summary.txt: one line per file, lowest line coverage first.
python3 - "$out/report-raw.txt" >"$out/summary.txt" <<'PY'
import sys
rows = []
total = None
for line in open(sys.argv[1]):
    f = line.split()
    if len(f) < 13 or f[0] == "Filename" or f[0].startswith("-"):
        continue
    name, fns, fmiss, lines, lmiss, lcov = f[0], int(f[4]), int(f[5]), int(f[7]), int(f[8]), f[9]
    if name == "TOTAL":
        total = (fns, fmiss, lines, lmiss, lcov)
        continue
    rows.append((float(lcov.rstrip("%")) if lcov != "-" else 100.0, name, fns, fmiss, lines, lmiss))
print("%-28s %8s %16s %20s" % ("file", "lines", "lines missed", "functions missed"))
for cov, name, fns, fmiss, lines, lmiss in sorted(rows):
    print("%-28s %7.2f%% %7d of %6d %9d of %5d" % (name, cov, lmiss, lines, fmiss, fns))
if total:
    fns, fmiss, lines, lmiss, lcov = total
    print("%-28s %8s %7d of %6d %9d of %5d" % ("TOTAL", lcov, lmiss, lines, fmiss, fns))
PY

# unexecuted.txt: every function with a zero entry count, by file.
$cov export "$bin" -instr-profile="$out/traces.profdata" -ignore-filename-regex="$ignore" -summary-only=false \
    -skip-expansions -skip-functions=false "$root/src" 2>/dev/null |
    python3 -c '
import json, sys, collections
root = sys.argv[1] + "/"
data = json.load(sys.stdin)
byfile = collections.defaultdict(list)
for fn in data["data"][0]["functions"]:
    if fn["count"] == 0:
        f = fn["filenames"][0].replace(root, "")
        if f.startswith("src/"):
            name = fn["name"].split(":")[-1]
            line = fn["regions"][0][0]
            byfile[f].append((line, name))
for f in sorted(byfile, key=lambda k: -len(byfile[k])):
    print("%s (%d)" % (f, len(byfile[f])))
    for line, name in sorted(byfile[f]):
        print("    %5d  %s" % (line, name))
' "$root" >"$out/unexecuted.txt"

echo "coverage: report in $out (summary.txt, unexecuted.txt)"
tail -n 1 "$out/summary.txt"
exit $status
