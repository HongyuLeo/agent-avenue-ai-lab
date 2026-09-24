# v3 Experiments

## Baseline audit

The unchanged v2 suite completed 5,000 random games (80,638 actions), 14,848
enumerated rule outcomes and mirrored positions, serialization/corruption,
gradient, parallel rollout and UI transition tests. Baseline parallel rollout
measured 10,071 to 11,321 games/s in short local runs.

The published v2 results remain the long-run reference: 85.28% vs Random,
79.76% vs Handwritten Heuristic and 53.04% vs its Saved Champion over 5,000
games per opponent. They are not relabeled as v3 results.

## v3.1 long-run checkpoint

The release checkpoint passed its internal checksum and metadata validation:

- magic `AAIV3C01`, inner format 31;
- 22,287,872 self-play games and 174,124 optimizer updates;
- Saved Champion selected at game 21,860,096;
- 100 stored evaluation rows and an 8-member opponent pool.

A separate frozen-model evaluation used a fixed seed, 5,000 games per opponent
and balanced first/second seats. The current v3 policy scored 85.88% vs Random,
83.90% vs Handwritten Heuristic, 54.56% vs the fixed public v2 Baseline and
48.78% vs the Saved v3 Champion. The Saved v3 Champion scored 52.64% vs v2.

The final 200-game CSV row recorded 61.5% vs v2. Because that row is a much
smaller sample, it remains visible in the audit log but is not used as the
release headline. Across the 95 unique logged evaluation rows, the aggregate
v3-v2 result was 9,333 / 19,000 (49.12%); those checkpoints span the training
run and are not interchangeable with the final frozen policy.

These figures support a narrow claim about this checkpoint under one fixed
evaluation seed. Repeated seeds and confidence intervals are still required for
a stronger playing-strength conclusion.

## v3 smoke measurements

The complete ablation table is in `V3_ABLATIONS.md`. Wilson 95% intervals for
the final 64-game belief-conditioned run were:

- Random: 97/200, 48.5%, CI 41.67% to 55.39%;
- Heuristic: 24/200, 12.0%, CI 8.20% to 17.23%;
- public v2 model: 38/200, 19.0%, CI 14.17% to 25.00%;
- initial v3 champion: 107/200, 53.5%, CI 46.59% to 60.28%.

The checkpoint was 311,742 bytes and loaded in 0.668 ms. A separate deterministic
16-worker training/resume smoke run completed 256 games at 1,054.7 games/s,
saved, then resumed to 320 games at 1,094.5 games/s.

These early smoke measurements are retained as pipeline evidence and are not
used to describe the long-run checkpoint above.

## CUDA training validation

The complete event encoder, 32-unit GRU, constrained belief heads,
belief-conditioned policy/value losses and truncated BPTT ran on the local
NVIDIA GeForce RTX 4090 (24 GB, compute capability 8.9, CUDA 13.0 NVRTC). On a
deterministic 40-sample batch, forced through a 32-sample device safety cap,
comparison with the CPU implementation measured:

- relative full-gradient error: `1.53452e-7`;
- maximum individual gradient error: `9.53674e-7`;
- maximum post-RMSProp weight error: `3.35276e-8`.

A separate short throughput run used 1,024 new games, 16 CPU self-play workers
and 128 games per update. CPU training measured 658.532 games/s; CUDA training
measured 2,751.06 games/s, a 4.18x end-to-end speedup on this host. The timing
includes rollout collection, batch preparation, device transfers, BPTT and
checkpoint saving, but excludes one-time NVRTC compilation. It is a throughput
smoke test, not a long-run utilization study or a playing-strength result.

A 65,536-game CUDA stability run completed 512 updates at 2,482.76 games/s.
Concurrent `nvidia-smi` samples showed 22-43% GPU utilization, 7-13% memory
controller utilization, about 1,982-2,005 MiB total device memory in use and
56.6-58.4 W board power. The observed idle total was about 1,646 MiB because
other desktop processes also used the GPU, so this trainer's incremental memory
was roughly 336-359 MiB. These samples demonstrate headroom and stability, not
an attempt to maximize GPU occupancy.

## Long training

The resumable CLI is:

```powershell
build\Release\v3_train.exe 1000000 training-v3.bin 16 auto 128 2048
```

Arguments are additional games, checkpoint path, self-play worker count,
`auto|cpu|cuda`, games per update, and the CUDA samples-per-launch safety cap.
The GUI uses automatic CUDA/CPU selection
with the same typed checkpoint and can start, pause, autosave and resume without
a command line. Release evaluation uses at least 5,000 seat-balanced games per
opponent. Repeated independent seeds remain future work.
