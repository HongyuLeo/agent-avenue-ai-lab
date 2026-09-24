# v3 Architecture

The v2 rules engine remains the single source of truth. v3 adds a strict layer
between `Game` and model inference:

```text
Game (full simulator state)
  -> Game::observe(player)
  -> InformationState / PlayerHistory (legal player view only)
  -> event encoder (149 -> 32)
  -> GRU (32 hidden units)
  -> belief heads (8 opponent-hand + 8 hidden-offer logits)
  -> policy head (GRU + predicted belief -> 74 masked actions)
  -> value head
```

`BeliefTarget` is constructed separately from simulator truth for supervised
training. No inference method accepts it or accepts `Game`. Each game owns two
histories; private offer/swap information is recorded only in its owner's
history. Histories are reset at a game boundary. Inference recomputes recurrent
state from the bounded history window, so no mid-game hidden state is persisted
in a checkpoint or accidentally reused for another player.

The v2 `Net` and `Trainer` remain intact. `V3Trainer` owns a current GRU model,
Saved Champion, eight-entry historical opponent pool, RMSProp state, counters,
RNG, belief metrics and evaluation history. Parallel workers generate immutable
rollouts from a frozen model snapshot; samples are concatenated in worker order
and updated centrally, which keeps deterministic test mode reproducible.

The Windows application reads `training.bin` only as the v2 Baseline and writes
v3 state only to `training-v3.bin`. Human play snapshots a v3 model and uses the
same `InformationState` path as training.

`V3CudaBackend` dynamically loads the NVIDIA Driver API and NVRTC, so the
application has no link-time CUDA dependency. It receives flattened padded
history batches and executes event encoding, GRU forward, constrained belief,
policy/value losses and truncated BPTT on the GPU. The shared host RMSProp
update preserves identical checkpoint/optimizer semantics. CPU and CUDA
gradients are checked numerically; initialization or runtime failure selects
the CPU path without changing the checkpoint format.
