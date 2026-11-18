# CANSHIELD — deterministic CAN intrusion detection and CAN firewall for QNX 8.0

CANSHIELD watches a vehicle CAN bus on QNX 8.0 and decides for every frame whether it is an attack.
The model is a **logic-gate network hardened into a pure Boolean circuit** (512 AND/OR/XOR/NOT gates,
no floating point, no branches, no memory allocation): every frame costs the same, so its worst-case
execution time can be measured and bounded — the property safety engineers need before they let
"AI" near a vehicle network.

## What runs on QNX
- `canshieldd` — RX thread at SCHED_FIFO priority blocks on a QNX CAN device
  (`devctl CAN_DEVCTL_RX_FRAME_RAW_BLOCK`), classifies every frame, timestamps detection with
  `ClockCycles()`, and in **gateway mode** forwards only clean frames to a second bus.
- **Resource manager** `/dev/canshield`: `cat` prints status + detection-time histogram;
  `devctl(CS_DEVCTL_GET_STATS)` returns binary stats; `devctl(CS_DEVCTL_NOTIFY)` registers a
  `sigevent` and the client gets a **pulse** on every alarm (`cs_watch`, `MsgRegisterEvent`/`MsgDeliverEvent`).
- Alarms are logged to **slog2**.

## Data and honest evaluation
Dataset: ROAD (Real ORNL Automotive Dynamometer CAN Intrusion Dataset), CC BY 4.0,
Verma et al. 2020, arXiv:2012.14600 — not redistributed; download from Zenodo record 10462796.

- The normal-behaviour profile is learned only from benign captures that are never used for testing.
- Model, threshold and alarm rule were chosen on the training split only; the held-out test was run once.
- Accelerator attacks are excluded: they are firmware modifications with no injected frames.

Held-out test (`build/TEST_RESULTS_2026-11-18.txt`):

| metric | result |
|---|---|
| attack captures detected | **16 / 21** |
| time to detect (injection attacks) | **4–46 ms** (fuzzing 4–5 ms; coolant 292 ms — that id is sent every 100 ms) |
| benign frames flagged | **0.021 %** |
| false alarm events | 35 / hour over 31 min of held-out driving (14 of 18 in the `benign_anomaly` capture) |
| missed | coolant-temperature masquerade; 4 reverse-light masquerades (injected byte values are legal — not separable per frame; needs cross-signal context, future work) |

C runtime == hardened Python model on all 298 feature patterns (parity test). 23 ns/frame on an Apple M3 (native).

## Live demo on QNX (no hardware)
QNX 8.0 x86_64 runs in QEMU with an emulated **Advantech MIOe-3680 dual-channel SJA1000 CAN card**
(driver: `dev-can-linux`, GPL-2.0, used as a separate binary). `can0` is the exposed bus, `can1` the
protected bus; both are bridged to Linux `vcan0`/`vcan1`, where ROAD captures are replayed.
Because x86 is emulated on ARM, replays run 20× slower and `canshieldd -s 20` divides its clock by 20
(relative timing and detection logic unchanged; timing numbers inside the emulator are not hardware numbers).

Speedometer masquerade (ROAD `max_speedometer_attack_2_masquerade`, 15.0–17.5 s):
5983 frames in, 149 injected → **0 injected frames reached the protected bus**, 150 dropped, 5833 forwarded, 1 alarm event.

## Layout
`core/` C feature extractor + detector (shared by Mac and QNX) · `tools/` profile builder, featurizer,
replay/parity · `scripts/` dataset, training (logic-gate network), evaluation · `qnx/` daemon, client, API.

License: Apache-2.0.
