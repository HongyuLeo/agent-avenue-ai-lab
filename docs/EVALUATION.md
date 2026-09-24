# Model evaluation

## v2 frozen checkpoint

The evaluated file is the public v2 Baseline `training.bin` checkpoint:

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

The v2 checkpoint materially exceeds the included random and heuristic baselines, performs similarly from either seat, and slightly exceeds its saved champion snapshot in this sample.

## v3.1 long-run checkpoint

The v3.1 Windows package also contains a validated recurrent-belief checkpoint:

- self-play games: **22,287,872**
- optimizer updates: **174,124**
- saved v3 Champion selected at game **21,860,096**
- checkpoint format: `AAIV3C01`, inner version 31

The current v3 policy was evaluated without training updates, using a fixed seed
and 5,000 games per opponent: 2,500 as first player and 2,500 as second player.
Actions were selected greedily from legal actions.

| Opponent | Wins | Total | Win rate |
|---|---:|---:|---:|
| Uniform random | 4,294 | 5,000 | 85.88% |
| Handwritten heuristic | 4,195 | 5,000 | 83.90% |
| Fixed public v2 Baseline | 2,728 | 5,000 | 54.56% |
| Saved v3 Champion | 2,439 | 5,000 | 48.78% |

For an additional promotion check, the saved v3 Champion scored 2,632 / 5,000
against the fixed v2 Baseline (**52.64%**). The last recorded GUI evaluation in
`training-evaluations.csv` was 123 / 200 (**61.5%**) against v2; that smaller
sample is retained as training history, not presented as the release result.

These measurements establish performance for this exact checkpoint and seed.
They do not establish expert-human strength, statistical significance across
multiple seeds, or a general solution to the game.

## Limitations

- These are engine-level results, not claims about expert human performance.
- All agents use the project's particular rule interpretations.
- A single pseudo-random sample was used; confidence intervals and repeated seeds would strengthen comparisons.
- The v2 network is intentionally small and has no recurrent memory or explicit belief-state search; v3 adds recurrent belief features but still has no tree search.
- The implementation covers the standard two-player base mode only.

## Regression coverage

The automated suite separately checks 14,848 enumerated outcome combinations and their player-swapped mirrors, 5,000 complete random games (80,638 actions in the recorded run), hidden-information isolation, exact checkpoint resume, checksum/backup recovery, evaluation-state isolation, selected numerical gradients, and 1,000 complete games through the UI transition wrapper.

The originally reported outcome was also reproduced. In that screenshot, the AI had **two**, not three, Daredevils after resolution and won by catching the human player. A separate test asserts that receiving a true third Daredevil loses unless a simultaneous-result tie rule applies.
