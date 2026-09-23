# Agent Avenue AI v3.0.0 Upgrade Assessment

Date: 2026-09-21

## Baseline verified

The audited source is the public `v2.2.1` tag. The tag archive was used because
GitHub's Git transport repeatedly reset on this host. It was imported into a
local Git repository and development continues on
`feature/v3-recurrent-belief`.

The unmodified source was built on Windows 11 with GCC 15.2 (MSYS2 UCRT),
CMake 3.31 and Ninja. A second clean MSVC 19.44 Release build was also completed.
The Win32 application, `core_checks`, and `fast_checks`
all linked. The test executables reported:

- 14,848 rule outcome combinations plus mirrored positions passed;
- 5,000 complete random games (80,638 actions) passed;
- observation privacy, exact resume, checksum/backup recovery, evaluation
  isolation, and numerical-gradient tests passed;
- parallel collection/update passed at 10,070.9 games/s in the short run.

A real 40-game training run produced two optimizer updates, saved a checkpoint,
evaluated 400 games per bundled opponent, then loaded it and continued to game
41. Those tiny evaluation numbers are smoke-test results, not strength claims.

Visual Studio 2022 Build Tools and CUDA Toolkit 13.0 are installed. The Codex
host injects duplicate case variants of `PATH`; an explicit VS developer
environment was used for the verified MSVC build, and an ASCII drive mapping
was needed by the MSYS2 assembler for the non-ASCII Windows user path. WSL
enumeration returned access denied. Windows GCC/MSVC builds are verified. The
subsequently implemented v3 CUDA GRU/BPTT backend is verified on the local RTX
4090 by CPU/CUDA gradient parity and throughput tests; WSL2 remains locally
unverified because of the host access restriction.

## Current architecture and data flow

`src/core.hpp` contains the shared rules engine, player observation, 128-float
encoder, 9,867-parameter feed-forward actor-critic, RMSProp-style optimizer,
opponents, evaluation and version-1 checkpoint. `src/fast_train.hpp` performs
deterministic multithreaded rollout collection and CPU updates.
`src/cuda_backend.hpp` dynamically loads the CUDA Driver API and NVRTC and
implements only the baseline dense network update. `src/windows.cpp` owns the
Win32 GUI, background trainer, evaluation, language persistence, human play,
and shutdown saving.

For each decision the simulator calls `Game::observe(actor)`, stores that
single `Obs` and action in a trajectory, finishes the game, and applies the
terminal result to every stored sample. There is no history, recurrent state,
belief target, bootstrapping, or sequence boundary in v2.2.1.

## Information visible to the baseline model

The acting player sees its hand; both public recruitment piles; its own discard
counts; relative position; both remaining swap counts; deck size; opponent hand
size; phase; and the face-up offered card. The encoder also derives remaining
card counts from those fields. It does not receive the opponent hand identities,
deck order, or the face-down offered card.

The existing privacy test mutates those three hidden sources and checks one
observation/encoding for equality. That boundary is sound for a stateless
policy, but is insufficient once history is introduced.

## Leakage risks

- `Game` is a full-information object and all callers can directly access
  `hand`, `deck`, and `hidden`; recurrent code must accept a player-view object,
  never `Game`.
- A raw action ID below 64 contains both offered card identities. Recording it
  for both players would leak the face-down card.
- Belief supervision necessarily reads full simulator state. Input and target
  must be separate types and separate function calls.
- Reusing one recurrent state while changing the acting player would leak the
  other player's private history.
- GUI code owns a full `Game`; rendering must continue to receive `-1` for an
  opponent's face-down card and must never render belief targets.
- v2.2.1's `Obs::discards` includes only the observing player's discard pile.
  v3 must preserve that established visibility unless formal rules establish
  that opponent discards are public.

## Rule conflict found

The prompt warns not to invent a “three Daredevils automatically lose” rule,
while v2.2.1 explicitly implements and exhaustively tests exactly that outcome.
The repository currently treats three Codebreakers as a win and three
Daredevils as a loss. No formal rule source is stored in the repository, so the
implementation is retained for compatibility and the conflict is recorded.
Changing it without an authoritative rule source would silently alter the
engine, trained checkpoint semantics, and published evaluation.

## Checkpoint compatibility risk

`AAITRN01` stores raw C++ object bytes for fixed-size baseline network structs.
It has an inner version of 1 and a checksum, but no explicit model type,
architecture manifest, endianness, compiler ABI, or independent section
metadata. A recurrent model cannot be appended without making old readers
misinterpret data. v3 therefore needs a new envelope and typed payload while
retaining the v1 loader as an explicit `baseline-v2` import path. It must never
rewrite a v2 file in place during import.

## Reusable modules

The rules state machine, legal mask, current observation, baseline model and
opponents, atomic save primitive, backup recovery, deterministic RNG practice,
parallel rollout approach, dynamic CUDA discovery, UI framework, localization,
and public checkpoint can all be retained.

The new work belongs in separate player-history, recurrent/belief model,
sequence trainer, metrics, and v3 checkpoint modules. GUI and fast training
should call these modules rather than duplicating rules.

## Recurrent architecture decision

A single-layer GRU is selected for v3. It is a better fit than an LSTM here
because it has fewer parameters and less recurrent-state/checkpoint overhead,
while retaining gated memory. A small Transformer would require padding,
attention masks, more GPU-specific optimization, and a larger validation
surface for a game whose public histories are short. The feed-forward network
remains the required Baseline and ablation.

The intended model is: 128-float observation encoder -> 32-float event
embedding -> 32-unit GRU -> opponent-hand and hidden-offer belief heads ->
policy/value heads. The policy can concatenate predicted belief probabilities;
ground-truth belief labels are never accepted by inference.

## Belief definition

The primary belief is a constrained marginal distribution over the type of a
uniformly selected card in the opponent hand. Multiplying by the public hand
size gives expected per-type counts whose total is the hand size. Types with no
remaining feasible copies are masked. During an unresolved offer, a second
8-way distribution predicts the face-down offered card. Targets are opponent
hand counts normalized by hand size and a one-hot hidden offer, respectively.
This avoids the combinatorial full joint multiset posterior while retaining a
well-defined, trainable and calibratable quantity.

## Search decision

Search is not on the critical path. IS-MCTS is the only plausible later option,
but only after a calibrated belief can produce constrained determinizations.
POMCP adds a costly particle/history search loop; CFR would require a different
training formulation and tractable information-set abstraction. A policy-only
recurrent belief model is the v3 default. Details and measured latency belong in
`V3_SEARCH_EVALUATION.md`.

## Performance risks

- Replaying histories and BPTT increase rollout storage and CPU work.
- Per-agent recurrent state must be reset exactly at game boundaries.
- Tiny individual GPU kernels would erase CUDA gains; updates require flattened
  padded sequence batches.
- GUI training currently alternates collection and update rather than truly
  overlapping them, so responsiveness and throughput need measurement.
- Raw-struct v2 serialization and four-megabyte load limits cannot scale to an
  unchecked architecture.

## Implementation phases

1. Introduce sanitized per-player histories and adversarial invariance tests
   while leaving Baseline behavior byte-compatible.
2. Add the GRU model, per-player hidden states, sequence training and typed v3
   checkpoint.
3. Add isolated belief targets, constrained predictions, auxiliary loss and
   calibration metrics.
4. Condition policy on predicted belief behind ablation flags.
5. Connect parallel collection, CPU fallback, CUDA batches, pause/save/resume,
   and GUI status/configuration.
6. Run deterministic smoke experiments and write only measured results.
7. update localization, release packaging, migration documentation and clean
   Windows smoke tests.
