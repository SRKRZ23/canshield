#!/usr/bin/env python3
"""Run a shell command on the QNX guest over its serial console socket (inside the colima VM).
usage: sudo python3 qsh.py 'command' [timeout_s]"""
import socket, sys, time, re
cmd = sys.argv[1]; tmo = float(sys.argv[2]) if len(sys.argv) > 2 else 20
s = socket.socket(socket.AF_UNIX); s.connect('/tmp/qnxser.sock'); s.settimeout(0.5)
def drain(t):
    out=b''; end=time.time()+t
    while time.time()<end:
        try:
            d=s.recv(65536)
            if d: out+=d; end=max(end, time.time()+0.4)
        except socket.timeout: pass
    return out
drain(0.6); s.sendall(b'\n'); drain(0.8)
mark='__END_%d__' % int(time.time()*1000)
s.sendall((cmd + '; echo ' + mark + '\n').encode())
out=b''; end=time.time()+tmo
while time.time()<end and mark.encode() not in out.split(cmd.encode())[-1]:
    out+=drain(0.7)
txt=out.decode('utf-8','replace')
txt=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',txt)
body=txt.split(mark)
print(body[-2] if len(body)>1 else txt)
