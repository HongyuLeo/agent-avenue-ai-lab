# v3.1 Training Log

The Windows application stores the complete evaluation history at:

```text
%LOCALAPPDATA%\AgentAvenueAI\training-evaluations.csv
```

Use **Open training log** on the Training page to open it. The file is rebuilt
atomically from the checkpoint history, so deleting the CSV does not discard
the underlying records. A `.bak` copy is retained. If another application locks
the CSV, checkpoint saving and training continue; log generation is retried on
the next autosave.

Each v3.1 row records the self-play generation, optimizer updates, Champion
generation, exact evaluation game/win counts and rates for Random, Handwritten
Heuristic, the fixed public v2 Baseline and the current v3 Champion. It also
records cumulative training samples, belief sample counts, averaged policy,
value and belief losses, hand/offer belief accuracy, Brier score and ECE.

Rate and accuracy fields are fractions from 0 to 1. For example, `0.35` is 35%.
The GUI evaluations currently use 200 fixed-seed, seat-balanced games per
opponent. These samples are useful for trends but retain substantial statistical
uncertainty.

v3.0 checkpoints stored only generation plus Random and Heuristic rates. During
migration those fields are preserved and marked `v3.0-migrated`; unavailable
raw counts, v2 rates and Champion rates remain blank. No missing result is
estimated or fabricated. All evaluations performed after upgrading to v3.1 are
stored in full.
