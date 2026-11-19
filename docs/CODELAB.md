# Codelab: a real CAN bus on QNX 8.0 — on an Apple Silicon Mac, without hardware

**You will build:** QNX 8.0 running in QEMU with an emulated dual-channel SJA1000 CAN card, frames flowing
both ways between Linux SocketCAN and QNX `/dev/can0`, and a small QNX resource manager that classifies every
frame. **Time:** about 90 minutes (most of it is the SDP download). **Cost:** $0.

Everything here was verified on an M3 Pro MacBook, macOS 15, QNX SDP 8.0 (QNX Everywhere licence).

## 1. Why this is not "just install it"
- QNX SDP 8.0 host tools are **x86_64 Linux / Windows only** — no macOS, no ARM host.
- The QEMU images `mkqnximage` builds for aarch64 need a BSP package, and SDP 8.0 has **no PCI hardware
  module for qemu-virt**, so PCI CAN cards only work on the x86_64 guest.
- On Apple Silicon an x86_64 guest runs under QEMU TCG (software emulation): fine for function, slow for timing.

## 2. One Linux VM for everything (Colima)
```bash
brew install colima docker
colima start qnx --vm-type vz --vz-rosetta --nested-virtualization --arch aarch64 --cpu 6 --memory 8 --disk 60
colima ssh -p qnx -- sudo apt-get install -y linux-modules-extra-$(uname -r) qemu-system-x86 can-utils
colima ssh -p qnx -- 'sudo modprobe vcan; for v in vcan0 vcan1; do sudo ip link add $v type vcan; sudo ip link set $v up; done'
```
Your Mac home is mounted read-write at the same path inside the VM. `--vz-rosetta` makes amd64 containers fast.

## 3. An x86_64 build container (Rosetta) and the licence
Build a small `ubuntu:22.04` amd64 image with build-essential, cmake, git, python3 (see `env/qnx-build/Dockerfile`).
**Pin its identity** — licence activation is bound to it:
```bash
docker run --platform linux/amd64 --hostname qnx-dev --mac-address 02:42:ac:11:00:2a \
  -v ~/.qnx/container-machine-id:/etc/machine-id:ro -v ~/qnx800:/home/dev/qnx800 -v ~/.qnx:/home/dev/.qnx ...
```
In myQNX: request the free non-commercial licence → **Accept** → **Deploy** to your own e-mail.
Download **QNX Software Center 2.0.4 for Linux hosts** (there is no macOS build for SDP 8.0) and install it
inside the container: `./qnx-setup-*.run --nox11 -- force-override agree-to-license-terms disable-auto-start ~/qnx/qnxsoftwarecenter`.

Command-line order that works (`qnxsoftwarecenter_clt`):
1. `-syncLicenseKeys` (a named-user licence shows 0 keys — normal)
2. **`-mirror`** (otherwise: "Cannot find anything in local repository")
3. `-installBaseline com.qnx.qnx800 -destination ~/qnx800` → if it says "License key is not activated",
   `-addLicenseKey <the key it printed>` then retry.

Type your myQNX password in your own terminal (for example with `read -s`), never in a script or a shared
shell history.

## 4. Build and boot QNX with a CAN card
```bash
source ~/qnx800/qnxsdp-env.sh
mkqnximage --clean --type=qemu --arch=x86_64 --ram=1G --cpu=2 --ssh-ident=key.pub --sshd-pregen=yes --noprompt
```
Run it in the Colima VM (`vm/run-x86-can.sh`):
```bash
sudo qemu-system-x86_64 -accel tcg -cpu max -smp 2 -m 1G -kernel output/ifs.bin \
  -drive file=output/disk-qemu.vmdk,if=ide \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2222-:22 -device virtio-net-pci,netdev=n0 \
  -object can-bus,id=canbus0 -object can-bus,id=canbus1 \
  -device mioe3680_pci,canbus0=canbus0,canbus1=canbus1 \
  -object can-host-socketcan,id=h0,if=vcan0,canbus=canbus0 \
  -object can-host-socketcan,id=h1,if=vcan1,canbus=canbus1 \
  -display none -chardev socket,id=s0,path=/tmp/qnxser.sock,server=on,wait=off,logfile=output/qemu.out -serial chardev:s0
```
Gotchas that cost us hours:
- **sshd dies with "Memory fault"** under TCG: OpenSSL 3's optimized code misbehaves. Start it with
  `OPENSSL_ia32cap=0 /system/bin/sshd -f /system/etc/ssh/sshd_config` (via the serial console, `vm/qsh.py`).
- Keep the default `pc` machine (q35 → AHCI, and `devb-eide` will not find the disk).
- `-cpu Nehalem` does not boot QNX 8.0 at all; `-cpu max` does.

## 5. CAN on QNX
Use the open-source **dev-can-linux** driver (prebuilt for QNX 8.0 x86_64):
```sh
./dev-can-linux -q </dev/null >/tmp/dcl.log 2>&1 &     # creates /dev/can0 and /dev/can1
candump -u0,rx0 &                                       # listen in QNX
```
From Linux: `cansend vcan0 123#DEADBEEF` → QNX prints `/dev/can0/rx0 ... [SFF] 123 [4] DE AD BE EF`.
From QNX: `cansend -u1,tx0 -m0x456,0,0xCAFE` → Linux `candump vcan1` shows `456 [2] CA FE`.
dev-can-linux supports **one card per vendor:device pair** — use the dual-channel MIOe-3680 for two buses.
**Never kill a process that is blocked in a CAN read** (`slay`, `kill -9`): the driver stalls for every
channel. Restart driver and readers together (an SLM unit does this). Readers of a FIFO or of your own
resource manager can be killed safely.

## 6. Talk to it like a QNX program
`devctl(fd, CAN_DEVCTL_RX_FRAME_RAW_BLOCK, &msg, sizeof msg, NULL)` reads a `struct can_msg`
(`<sys/can_dcmd.h>`); `CAN_DEVCTL_TX_FRAME_RAW` writes one. Wrap your logic in a resource manager
(`resmgr_attach`), stream stats over `devctl`, notify clients with `MsgDeliverEvent` (clients register their
pulse with `MsgRegisterEvent`), drop privileges with `procmgr_ability(... DENY|LOCK|EOL)`, and supervise the
driver + daemon as one unit with **SLM** (`<SLM:repair>restart</SLM:repair>`). `canshieldd.c` shows all of it.

## 7. Real-time behaviour you can see even in the emulator
Run your workload at priority 10 next to two CPU hogs, then at `on -p 63f`: worst case drops from milliseconds
to the idle-system value. That is the scheduler doing its job.

## 8. Replaying real attacks
The ROAD dataset (ORNL, CC BY 4.0) is in candump format. Because x86 is emulated, replay slowly and dilate the
clock by the same factor: `vm/canreplay.py vcan0 attack.log --slow 20` and `canshieldd -s 20`.
