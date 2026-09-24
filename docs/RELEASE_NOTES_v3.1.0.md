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

The Windows package includes a validated v3.1 recurrent-belief checkpoint with
22,287,872 self-play games and 174,124 optimizer updates, plus its complete
`training-evaluations.csv`. On first launch, the application imports the bundled
checkpoint and log into `%LOCALAPPDATA%\AgentAvenueAI` only when no local v3
checkpoint exists; it never overwrites existing training progress.

A fresh frozen-model evaluation used a fixed seed and 5,000 games per opponent,
balanced across first and second player. The current v3 policy scored 85.88%
vs Random, 83.90% vs Handwritten Heuristic, 54.56% vs the fixed public v2
Baseline and 48.78% vs its Saved v3 Champion. The Saved v3 Champion scored
52.64% vs v2. The last 200-game training-log row was 61.5% vs v2, but that
smaller sample is not used as the release result. These results describe one
checkpoint and fixed seed; they are not claims of expert-human strength or a
general solution.

The recurrent-belief CUDA backend, automatic CPU fallback, safe checkpoint
replacement, multilingual GUI and read-only v2 Baseline remain unchanged.

The CUDA GRU/BPTT path was validated on an RTX 4090 against the CPU path. If
CUDA or NVRTC cannot be loaded, or a CUDA update fails, training automatically
falls back to CPU. Existing v3.0 checkpoints can be opened and upgraded without
changing their model weights, optimizer state, RNG state, or counters.

The Windows executable is not commercially code-signed. Windows Defender
SmartScreen may therefore display an unrecognized-app warning; users who prefer
not to bypass it can build the same source locally.
