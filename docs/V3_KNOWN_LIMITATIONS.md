# v3 Known Limitations

- The v3 GRU/belief CUDA path is verified on the local RTX 4090 with CUDA 13.0
  and has automatic CPU fallback. Other NVIDIA architectures/toolkit versions
  are compiled at runtime by NVRTC and have not all been exercised locally.
- WSL2 enumeration was denied by the host, so Linux/WSL builds rely on CI and
  source-level portability rather than a local WSL run in this audit.
- The reported v3 experiments are smoke runs. No v3 checkpoint has undergone a
  multi-day training run or demonstrated a strength gain over v2.
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
