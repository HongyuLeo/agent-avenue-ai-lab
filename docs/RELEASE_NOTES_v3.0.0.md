# Agent Avenue AI v3.0.0

This release adds a player-view history pipeline, 32-unit GRU actor-critic,
explicit opponent-hand/hidden-offer belief heads, optional belief-conditioned
policy, deterministic parallel CPU self-play, CUDA/NVRTC GRU BPTT with CPU
fallback, v3 ablations and a typed, checksum-protected checkpoint format.

The v2.2.1 feed-forward model remains bundled as a read-only Baseline and
evaluation opponent. Its `training.bin` is never overwritten; live v3 progress
is stored at `%LOCALAPPDATA%\AgentAvenueAI\training-v3.bin` with `.bak` recovery.

The Windows GUI remains Chinese by default and supports English and Spanish.
Training, pause/save/resume, evaluation and human-vs-AI play all use the v3
player-information boundary.

Important: included v3 measurements are short smoke tests and do not show a
strength improvement over the 17M-game v2 model. The v3 CUDA gradient path was
verified against CPU on an RTX 4090, and automatically falls back to CPU when
CUDA is unavailable. See `docs/V3_KNOWN_LIMITATIONS.md` in the source repository.
