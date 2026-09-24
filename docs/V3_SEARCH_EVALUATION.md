# v3 Search Evaluation

Search is deferred from the v3 core until the recurrent belief policy is
trained and calibrated.

IS-MCTS is the most compatible candidate because it can operate on sampled
determinizations, but a correct implementation must sample without replacement
from the player's feasible unseen inventory. Sampling from simulator truth is
forbidden. It also needs a repeatable latency/strength gain over the pure policy
before entering human play.

POMCP would add particle filtering plus online history search to a codebase that
currently has neither. Its compute and validation cost is disproportionate for
the first recurrent release. CFR-family methods require a different offline
information-set training formulation and a carefully bounded abstraction;
they are not an incremental extension of the actor-critic trainer.

The shipped decision path therefore remains policy-only. An IS-MCTS experiment
may be added later behind a compile/runtime flag after all of these gates pass:

1. determinizations use only `PlayerHistory` and predicted beliefs;
2. invariance tests prove no full `Game` state enters search;
3. p50/p95 move latency is suitable for the desktop UI;
4. paired, seat-balanced evaluation shows a repeatable gain;
5. failures fall back to the pure policy without affecting training or saves.

No search strength or latency result is claimed in this document.
