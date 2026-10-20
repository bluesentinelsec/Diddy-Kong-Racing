#!/bin/sh
# Print the UDID of the first iPhone or iPad connected over USB (or paired on
# the network), for `make run-ios`. Pass a name or UDID to pick one explicitly.
#
#   ios/device.sh            -> 00008120-...
#   ios/device.sh "My iPad"  -> its UDID
set -eu

want=${1:-}
json=$(mktemp "${TMPDIR:-/tmp}/dkr_devices.XXXXXX")
trap 'rm -f "$json"' EXIT
xcrun devicectl list devices --json-output "$json" >/dev/null 2>&1
python3 -I - "$json" "$want" <<'PY'
import json, sys
devs = json.load(open(sys.argv[1]))["result"]["devices"]
want = sys.argv[2]
for d in devs:
    props = d.get("deviceProperties", {})
    hw = d.get("hardwareProperties", {})
    udid = hw.get("udid", "")
    name = props.get("name", "")
    if hw.get("platform") != "iOS":
        continue
    if want and want not in (name, udid, d.get("identifier", "")):
        continue
    print(udid)
    sys.exit(0)
sys.stderr.write("ios/device.sh: no iOS device found (is it plugged in, unlocked and trusted?)\n")
sys.exit(1)
PY
