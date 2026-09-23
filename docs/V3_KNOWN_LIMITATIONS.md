# v3 Known Limitations

- The v3 GRU/belief CUDA path is verified on the local RTX 4090 with CUDA 13.0
  and has automatic CPU fallback. Other NVIDIA architectures/toolkit versions
  are compiled at runtime by NVRTC and have not all been exercised locally.
- WSL2 enumeration was denied by the host, so Linux/WSL builds rely on CI and
  source-level portability rather than a local WSL run in this audit.
- The 22,287,872-game v3 checkpoint was evaluated once with a fixed seed. Its
  54.56% result against v2 over 5,000 balanced games should be repeated across
  independent seeds before drawing a broader playing-strength conclusion.
- The last 200-game CSV row is 61.5% vs v2, but that sample is too small to use
  as the release headline and can differ materially from the 5,000-game result.
- Beliefs are constrained marginals, not a full joint posterior over hand
  multisets.
- Search is deliberately deferred pending calibrated beliefs and repeatable
  latency/strength evidence.
- Advanced v3 hyperparameters are stored in the checkpoint and exposed in the
  library/CLI, but the current GUI keeps the default profile rather than adding
  a full advanced-settings editor.
- The repository contains no authoritative formal rule document. Its existing
  tested third-Daredevil loss interpretation is retained; the prompt conflict is
  recorded in `V3_UPGRADE_ASSESSMENT.md`.
