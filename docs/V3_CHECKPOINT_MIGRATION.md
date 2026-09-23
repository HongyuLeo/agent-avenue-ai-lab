# v3 Checkpoint Migration

v2 and v3 checkpoints are intentionally different files.

| Property | v2 Baseline | v3 recurrent belief |
|---|---|---|
| Magic | `AAITRN01` | `AAIV3C01` |
| Inner version | 1 | 31 (v3.1; reads v3.0 format 3) |
| Model type | implicit dense actor-critic | explicit recurrent-belief type 2 |
| Architecture | fixed raw struct | recorded input/hidden/belief sizes |
| Live filename | `training.bin` | `training-v3.bin` |

The v3 payload includes explicit configuration, current network, Saved Champion,
historical opponent pool, optimizer accumulators, counters, belief metrics,
four-opponent evaluation history, loss/belief metric snapshots and RNG state. The outer envelope records byte length and an
FNV-1a checksum. Saves write `.tmp`, flush, preserve `.bak`, then atomically
replace the primary file.

The application never rewrites `training.bin` as v3. It loads a compatible v2
checkpoint fully when possible. If the C++ standard library cannot parse the
old RNG representation, it performs a checksum-validated, read-only extraction
of the current network for use as an evaluation opponent. Such an import is not
accepted for v2 training resume.

There is no meaningful weight conversion from the dense v2 network into a GRU.
v3 begins from a new seeded initialization and retains v2 as the Baseline and
opponent. Copying or renaming a v2 file to `training-v3.bin` is rejected.

v3.1 loads v3.0 checkpoints without changing their weights, optimizer or RNG.
Old Random/Heuristic history points are retained; unavailable historical v2 and
Champion values remain blank rather than being invented. New evaluations store
all four raw win counts and metric snapshots and are mirrored to
`training-evaluations.csv`.
