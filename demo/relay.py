#!/usr/bin/env python3
"""Demo relay (runs in the Linux VM): merges CANSHIELD stats streamed from QNX with live counters of the
exposed (vcan0) and protected (vcan1) buses, and writes demo/live.json for the dashboard.
usage: python3 relay.py <ssh-key> [--out DIR]"""
import argparse, json, os, socket, struct, subprocess, threading, time

ap = argparse.ArgumentParser(); ap.add_argument('key'); ap.add_argument('--out', default=os.path.dirname(os.path.abspath(__file__)))
ap.add_argument('--rec', help='also append every snapshot (JSON lines) to this file, e.g. for rendering a video')
A = ap.parse_args()
OUT = os.path.join(A.out, 'live.json')
state = {'qnx': {}, 'exposed': {'frames': 0, 'attack_like': 0, 'recent': []}, 'protected': {'frames': 0, 'attack_like': 0, 'recent': []}}
lock = threading.Lock()

def attack_like(cid, data):
    # visual hint only (not the detector): frames the ROAD attacks inject
    return (cid == 0x0D0 and len(data) > 5 and data[5] == 0xFF) or (len(data) == 8 and data == b'\xff' * 8) \
        or (cid == 0x4E7 and len(data) > 5 and data[5] == 0xFF) or (cid == 0x6E0 and data.hex().upper().startswith('59594545'))

def sniff(ifname, key):
    s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW); s.bind((ifname,))
    while True:
        fr = s.recv(16); cid, dlc = struct.unpack('=IB3x', fr[:8]); data = fr[8:8 + dlc]; cid &= 0x1FFFFFFF
        with lock:
            b = state[key]; b['frames'] += 1
            a = attack_like(cid, data); b['attack_like'] += a
            b['recent'] = (b['recent'] + [{'id': f'{cid:03X}', 'data': data.hex().upper(), 'a': a}])[-12:]

def qnx_stream():
    cmd = ['ssh', '-p', '2222', '-i', A.key, '-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null',
           '-o', 'LogLevel=ERROR', 'root@127.0.0.1', '/data/home/root/cs_stat -w 250']
    while True:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True)
        for line in p.stdout:
            try:
                d = json.loads(line)
            except ValueError:
                continue
            with lock: state['qnx'] = d
        time.sleep(1)

state['kuksa'] = {'speed': None, 'updates': 0, 'spoofed_seen': 0}
# folder with watch_speed.py from github.com/SRKRZ23/kuksa-qnx (test/); override with KUKSA_TEST
KUKSA_TEST = os.environ.get('KUKSA_TEST', os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'kuksa-qnx', 'test'))

def kuksa_stream():
    # what applications see: Vehicle.Speed from the Kuksa databroker running on QNX (via the ssh tunnel)
    cmd = ['docker', 'run', '--rm', '--network', 'host', '-v', os.path.abspath(KUKSA_TEST) + ':/t',
           'kuksa-pyclient', 'python3', '-u', '/t/watch_speed.py']
    while True:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True)
        for line in p.stdout:
            try:
                v = json.loads(line)['speed']
            except (ValueError, KeyError):
                continue
            with lock:
                k = state['kuksa']; k['speed'] = v; k['updates'] += 1; k['spoofed_seen'] += (v == 255.0)
        time.sleep(1)

for t in (threading.Thread(target=sniff, args=('vcan0', 'exposed'), daemon=True),
          threading.Thread(target=sniff, args=('vcan1', 'protected'), daemon=True),
          threading.Thread(target=qnx_stream, daemon=True),
          threading.Thread(target=kuksa_stream, daemon=True)):
    t.start()
MODE_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'mode.txt')
while True:
    try:
        mode = open(MODE_FILE).read().strip()
    except OSError:
        mode = 'enforce'
    with lock: snap = json.dumps(dict(state, ts=time.time(), mode=mode))
    tmp = OUT + '.tmp'; open(tmp, 'w').write(snap); os.replace(tmp, OUT)
    if A.rec:
        with open(A.rec, 'a') as f: f.write(snap + '\n')
    time.sleep(0.2)
