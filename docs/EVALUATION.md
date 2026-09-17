# Model evaluation

## Frozen checkpoint

The evaluated file is the public release `training.bin` checkpoint:

- self-play games: **17,366,354**
- optimizer updates: **1,085,397**
- champion snapshot selected at game **17,360,114**

No training update occurred during evaluation.

## Protocol

For each opponent, the current policy played 5,000 games: 2,500 as first player and 2,500 as second player. Policy actions were selected greedily from legal actions. The engine used the same rules and hidden-information observation boundary as training and the desktop UI.

| Opponent | First-player wins | Second-player wins | Total | Win rate |
|---|---:|---:|---:|---:|
| Uniform random | 2,140 / 2,500 | 2,124 / 2,500 | 4,264 / 5,000 | 85.28% |
| Handwritten heuristic | 2,012 / 2,500 | 1,976 / 2,500 | 3,988 / 5,000 | 79.76% |
| Saved champion | 1,339 / 2,500 | 1,313 / 2,500 | 2,652 / 5,000 | 53.04% |

## What these numbers establish

The checkpoint materially exceeds the included random and heuristic baselines, performs similarly from either seat, and slightly exceeds its saved champion snapshot in this sample.

## Limitations

- These are engine-level results, not claims about expert human performance.
- All agents use the project's particular rule interpretations.
- A single pseudo-random sample was used; confidence intervals and repeated seeds would strengthen comparisons.
- The network is intentionally small and has no recurrent memory or explicit belief-state search.
- The implementation covers the standard two-player base mode only.

## Regression coverage

The automated suite separately checks 14,848 enumerated outcome combinations and their player-swapped mirrors, 5,000 complete random games (80,638 actions in the recorded run), hidden-information isolation, exact checkpoint resume, checksum/backup recovery, evaluation-state isolation, selected numerical gradients, and 1,000 complete games through the UI transition wrapper.

The originally reported outcome was also reproduced. In that screenshot, the AI had **two**, not three, Daredevils after resolution and won by catching the human player. A separate test asserts that receiving a true third Daredevil loses unless a simultaneous-result tie rule applies.
