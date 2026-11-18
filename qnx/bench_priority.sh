#!/bin/sh
# Run ON QNX: priority-isolation benchmark for the CANSHIELD detector.
# Scenario A: idle system, detector at normal priority 10.
# Scenario B: 2 CPU hogs (one per CPU) at priority 10, detector also at priority 10 (shares the CPU).
# Scenario C: same 2 hogs at priority 10, detector at real-time priority 63 (FIFO).
cd /data/home/root
LOG=${1:-bench.log}
echo "### A: idle, detector prio 10";      on -p 10f ./cs_bench profile.bin /tmp/A $LOG | grep -E "throughput|per-frame"
( on -p 10r sh -c 'while :; do :; done' & ) ; ( on -p 10r sh -c 'while :; do :; done' & )
sleep 1
echo "### B: 2 CPU hogs @10, detector prio 10"; on -p 10r ./cs_bench profile.bin /tmp/B $LOG | grep -E "throughput|per-frame"
echo "### C: 2 CPU hogs @10, detector prio 63 FIFO"; on -p 63f ./cs_bench profile.bin /tmp/C $LOG | grep -E "throughput|per-frame"
slay -f sh >/dev/null 2>&1
