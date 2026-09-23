# Frozen public checkpoint

`pretrained-17m.bin` is the frozen v2.2.1 feed-forward Baseline used by the Windows release and v3 comparison matrix. It is not a v3 recurrent checkpoint and is never overwritten by v3 training.

- Self-play games: 17,366,354
- Optimizer updates: 1,085,397
- SHA-256: `9fbcd1f71697bbe88d21f64c8f1667c5b0c14e584840d36c1ded83635a642462`

The application copies this file as `training.bin`. Live v3 training is stored separately as `%LOCALAPPDATA%\AgentAvenueAI\training-v3.bin` and is excluded from Git.
