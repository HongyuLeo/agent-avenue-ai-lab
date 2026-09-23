# Agent Avenue AI v3.1.0

This update makes training progress auditable from the Windows application.
The evaluation chart now tracks Random, Handwritten Heuristic, the fixed public
v2 Baseline and the evolving v3 Saved Champion. Every evaluation is also stored
in the checkpoint and written atomically to `training-evaluations.csv` with raw
win counts, win rates, training counters, policy/value/belief losses, belief
accuracy, Brier score and calibration error.

v3.1 checkpoints use inner format 31 and remain backward-compatible with v3.0
format 3. Existing weights, optimizer state, RNG and training counters resume
unchanged. Historical v2/Champion values absent from v3.0 remain blank; the
application never fabricates them.

The v3 recurrent-belief CUDA backend, automatic CPU fallback, safe checkpoint
replacement, multilingual GUI and read-only v2 Baseline remain unchanged.
Short evaluations are trend indicators and are not claims of playing strength.

The CUDA GRU/BPTT path was validated on an RTX 4090 against the CPU path. If
CUDA or NVRTC cannot be loaded, or a CUDA update fails, training automatically
falls back to CPU. Existing v3.0 checkpoints can be opened and upgraded without
changing their model weights, optimizer state, RNG state, or counters.

The Windows executable is not commercially code-signed. Windows Defender
SmartScreen may therefore display an unrecognized-app warning; users who prefer
not to bypass it can build the same source locally.
