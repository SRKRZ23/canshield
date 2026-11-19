#!/bin/bash
# Demo scenario (runs in the Linux VM). Sets CANSHIELD's mode at run time, restarts the relay with fresh
# counters, sets Vehicle.Speed to the true value, then replays the ROAD speedometer masquerade window onto
# the exposed bus (vcan0).
#   bash run_scenario.sh monitor   # detection only: alarms fire, but every frame is still forwarded
#   bash run_scenario.sh enforce   # CAN firewall: flagged frames are dropped
# On QNX: canshieldd -t /tmp/cs_accepted (SLM unit) and qnx-can-feeder -f /tmp/cs_accepted are running.
# Never kill a process blocked in a dev-can-linux read: the driver stalls. Nothing here does.
set -u
MODE=${1:?monitor|enforce}
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=$(cd "$HERE/../.." && pwd)
CAP=$ROOT/data/road/road/attacks/max_speedometer_attack_2_masquerade.log
qnx() { ssh -p 2222 -i "$ROOT/env/keys/qnxvm" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
            -o LogLevel=ERROR root@127.0.0.1 "$1" </dev/null; }
P=$(pgrep -f "u rel[a]y"); [ -n "$P" ] && kill $P
docker ps -q --filter ancestor=kuksa-pyclient | xargs -r docker kill >/dev/null
# Fresh unit for every run: SLM restarts dev-can-linux + canshieldd together (zeroed counters). Under QEMU TCG
# dev-can-linux stops receiving after ~12-18k frames in one session; a unit restart clears it.
qnx "slay -f cs_stat >/dev/null 2>&1; slay -9 -f canshieldd; sleep 15; echo $MODE > /dev/canshield; cat /dev/canshield | grep ^mode"
echo "$MODE" > "$HERE/mode.txt"
sleep 1
cansend vcan0 0D0#0000000000040000          # true speed byte (4) before the attack
sleep 3
cd "$HERE" && setsid nohup python3 -u relay.py "$ROOT/env/keys/qnxvm" </dev/null >/tmp/relay.log 2>&1 &
sleep 12
sudo python3 "$ROOT/vm/canreplay.py" vcan0 "$CAP" --slow 20 --from 15.0 --to 17.5 | tail -1
sleep 4
