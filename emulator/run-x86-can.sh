#!/bin/bash
# Run INSIDE the colima VM:  colima ssh -p qnx -- bash ~/Alish/qnx-hackathon-2026/vm/run-x86-can.sh [start|stop|status]
# QNX 8.0 x86_64 guest under QEMU TCG with an emulated Kvaser PCI CAN card whose bus is
# bridged to Linux vcan0 (exposed bus) and a second card bridged to vcan1 (protected bus, gateway output) (cansend/candump/canplayer on vcan0 reach QNX).
set -euo pipefail
cd "$(dirname "$0")/x86"
PIDF=output/qemu.pid
case "${1:-start}" in
  stop)   sudo pkill -f "qemu-system-x86_64.*mioe3680_pci" && echo stopped || echo "not running"; sleep 1; sudo rm -f $PIDF; exit 0 ;;
  status) pgrep -f "qemu-system-x86_64.*mioe3680_pci" >/dev/null && echo running || echo "not running"; exit 0 ;;
esac
sudo modprobe vcan
for v in vcan0 vcan1; do ip link show $v >/dev/null 2>&1 || sudo ip link add $v type vcan; sudo ip link set $v up; done
[ -f $PIDF ] && kill -0 "$(cat $PIDF)" 2>/dev/null && { echo "already running"; exit 0; }
# can-host-socketcan needs CAP_NET_RAW on the raw socket -> run qemu with sudo
# (tried: a 2nd card, kvaser_pci on canbus1, as a protected-bus node. Under TCG it shares IRQ 10 with the
#  MIOe-3680 in dev-can-linux; it repeatedly lost frames and then stalled the whole driver -> not used.)
sudo qemu-system-x86_64 \
  -accel tcg -cpu max -smp 2 -m 1G \
  -kernel output/ifs.bin \
  -drive file=output/disk-qemu.vmdk,if=ide,id=drv0 \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22 -device virtio-net-pci,netdev=net0 \
  -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
  -object can-bus,id=canbus0 -object can-bus,id=canbus1 \
  -device mioe3680_pci,canbus0=canbus0,canbus1=canbus1 \
  -object can-host-socketcan,id=canhost0,if=vcan0,canbus=canbus0 \
  -object can-host-socketcan,id=canhost1,if=vcan1,canbus=canbus1 \
  -display none -chardev socket,id=ser0,path=/tmp/qnxser.sock,server=on,wait=off,logfile=output/qemu.out -serial chardev:ser0 \
  -pidfile $PIDF -daemonize
sudo chown "$(id -u)" $PIDF output/qemu.out 2>/dev/null || true
# wait for boot, then start sshd with OpenSSL asm disabled (OpenSSL 3 SIGSEGVs under QEMU TCG x86)
for i in $(seq 1 60); do sudo grep -q "Startup complete" output/qemu.out 2>/dev/null && break; sleep 2; done
sudo python3 "$(dirname "$0")/qsh.py" 'OPENSSL_ia32cap=0 /system/bin/sshd -f /system/etc/ssh/sshd_config' 20 >/dev/null
echo "started; serial log: $(pwd)/output/qemu.out ; ssh: ssh -p 2222 -i ~/Alish/qnx-hackathon-2026/env/keys/qnxvm root@127.0.0.1"
