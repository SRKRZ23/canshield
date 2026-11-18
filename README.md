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
- Model, threshold and alarm rule were chosen on the training split only; v1 was tested once (see the v2 disclosure below).
- Accelerator attacks are excluded: they are firmware modifications with no injected frames.

Held-out test (`build/TEST_RESULTS_2026-11-18.txt`):

| metric | result |
|---|---|
| attack captures detected | **15 / 20** |
| time to detect (injection attacks) | **4–46 ms** (fuzzing 4–5 ms; coolant 292 ms — that id is sent every 100 ms) |
| benign frames flagged | **0.021 %** |
| false alarm events | 35 / hour over 31 min of held-out driving (14 of 18 in the `benign_anomaly` capture) |
| missed | coolant-temperature masquerade; 4 reverse-light masquerades (injected byte values are legal — not separable per frame; needs cross-signal context, future work) |

C runtime == hardened Python model on all 298 feature patterns (parity test).

Disclosure: a v2 with cross-signal byte-pair features was evaluated on the same test set and was **worse**
(390 false alarms/h, driven by diagnostic traffic; masquerades not recovered). We kept v1, which had been
selected before v2 existed; the v2 code is on branch `v2-crosssignal-negative`. The test set has therefore
been looked at twice.

## Benchmarks (`build/bench/`)
**Static (QNX x86_64 binary, `ntox86_64-objdump`):** 512 gates, of which **84 are live** (reach the 16 outputs);
25 of 36 features used; `cs_model_votes` = 196 instructions with **no data-dependent branches**
(only a fixed loop and the stack guard); `cs_features` = 208 instructions, 17 bounded branches, no loops over history.

**Native speed, Apple M3 Pro, 8.0 M ROAD frames:** **23.2 ns/frame (43 M frames/s)**. A CAN bus carries ≤ ~8 k frames/s.
Input dependence is bounded, not zero: real traffic 22.9 ns, random payloads 33.1 ns (worst class), unknown ids 14.9 ns.
(Per-frame timing is not measurable on macOS: 1 µs clock, OS jitter up to 391 µs — the contrast QNX removes.)

**QNX priority isolation (QNX 8.0 x86_64 in QEMU TCG — emulated, ~17× slower than native; 165 k ROAD frames):**

| scenario | p99.99 | max | throughput |
|---|---|---|---|
| idle, detector prio 10 | 31 µs | 263 µs | 2.5 M fr/s |
| 2 CPU hogs @10, detector prio 10 | 2.06 ms | 3.1 ms | 1.6 M fr/s |
| 2 CPU hogs @10, **detector prio 63 FIFO** | **34 µs** | **83 µs** | **2.5 M fr/s** |

**Same features, same protocol, classic models** (threshold chosen on train at benign FPR ≤ 0.1%):

| model | frame recall | benign FPR | captures detected | false alarms/h |
|---|---|---|---|---|
| **logic-gate network (CANSHIELD)** | 75.7% | **0.021%** | 15/20 | 34.8 |
| logistic regression | 27.0% | 0.023% | 5/20 | 15.5 |
| decision tree (depth 8) | 77.2% | 0.087% | 16/20 | 29.0 |
| random forest (100 trees) | 77.0% | 0.027% | 14/20 | 17.4 |
| gradient boosting | 68.9% | 1.349% | 11/20 | 278.6 |

Honest reading: accuracy is on par with the best classic models, not above them. The point of the logic-gate
network is its form — a branch-free Boolean circuit (bit-sliceable, FPGA-mappable, checkable as a formula) —
not a higher score.

## Hardening
- **Least privilege:** after opening the CAN devices, attaching `/dev/canshield` and creating the SCHED_FIFO thread,
  `canshieldd` denies and locks every process-manager ability (`procmgr_ability(... DENY|LOCK|EOL)`) and self-tests it
  (`ThreadCtl(_NTO_TCTL_IO)` must fail): `privileges dropped and locked (self-test: I/O privilege denied - OK)`.
- **Fail-closed gateway:** with `canshieldd` killed (`kill -9`) for 11.5 s mid-attack, **0 frames** reached the protected
  bus and 0 injected frames leaked.
- **Supervised recovery with QNX SLM:** `qnx/canshield-slm.xml` runs `qnx/cs_unit.sh` (restart the CAN driver, `waitfor`
  its devices, `exec canshieldd`) with `<SLM:repair>restart</SLM:repair>`. Verified: `kill -9 canshieldd` → SLM logs
  "terminated by uncaught signal 9", restarts driver + daemon (new PIDs), and traffic flows again (5/5 test frames forwarded).
  The unit restarts the driver too because `dev-can-linux` may not release a reader that was killed while blocked.

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
