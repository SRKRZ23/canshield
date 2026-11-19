#!/usr/bin/env python3
"""Replay a candump log into a Linux SocketCAN interface with a time-dilation factor.
usage: canreplay.py vcan0 file.log [--slow 20] [--from SEC] [--to SEC]
--slow N: frames keep their RELATIVE timing but play N times slower (for slow emulators);
          canshieldd must then be started with -s N so its clock is divided by the same N."""
import argparse, socket, struct, time

ap = argparse.ArgumentParser()
ap.add_argument('ifname'); ap.add_argument('log')
ap.add_argument('--slow', type=float, default=1.0)
ap.add_argument('--from', dest='t_from', type=float, default=0.0)
ap.add_argument('--to', dest='t_to', type=float, default=1e18)
A = ap.parse_args()
s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW); s.bind((A.ifname,))
frames = []
with open(A.log) as f:
    first = None
    for line in f:
        try:
            ts = float(line[1:line.index(')')]); body = line.split()[2]
        except (ValueError, IndexError):
            continue
        first = ts if first is None else first
        rel = ts - first
        if rel < A.t_from or rel > A.t_to:
            continue
        cid, data = body.split('#'); frames.append((rel, int(cid, 16), bytes.fromhex(data)))
t0 = frames[0][0]; start = time.monotonic(); sent = 0
for rel, cid, data in frames:
    target = start + (rel - t0) * A.slow
    while True:
        d = target - time.monotonic()
        if d <= 0: break
        time.sleep(min(d, 0.002))
    s.send(struct.pack('=IB3x8s', cid, len(data), data.ljust(8, b'\0'))); sent += 1
print(f"sent {sent} frames from {A.log} (window {A.t_from}-{min(A.t_to, frames[-1][0]):.2f}s) in {time.monotonic()-start:.1f}s at 1/{A.slow:g} speed")
