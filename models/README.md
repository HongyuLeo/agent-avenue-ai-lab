# Frozen public checkpoint

`pretrained-17m.bin` is the frozen public checkpoint used by the Windows release.

- Self-play games: 17,366,354
- Optimizer updates: 1,085,397
- SHA-256: `9fbcd1f71697bbe88d21f64c8f1667c5b0c14e584840d36c1ded83635a642462`

The application copies or imports this file as `training.bin`. Live user training remains in `%LOCALAPPDATA%\AgentAvenueAI` and is excluded from Git.
