#!/bin/sh
# One supervised unit: (re)start the CAN driver, wait for its devices, then become canshieldd.
# If canshieldd dies, SLM restarts this whole unit -> the driver is restarted too
# (dev-can-linux may not release a reader that was killed while blocked).
cd /data/home/root
slay -9 -f dev-can-linux >/dev/null 2>&1
./dev-can-linux -q </dev/null >/tmp/dcl.log 2>&1 &
waitfor /dev/can1/tx0 10 || exit 1
exec ./canshieldd -p profile.bin -i /dev/can0/rx0 -o /dev/can1/tx0 -s ${CS_SLOW:-20}
