# v3 Belief Model

## Prediction targets

The opponent-hand head emits eight probabilities. Entry `k` is the probability
that a uniformly selected card from the opponent's current hand has type `k`.
The supervised target is `opponent_count[k] / opponent_hand_size`. Expected
counts are `probability[k] * opponent_hand_size`.

During the response phase, the hidden-offer head emits an eight-way probability
distribution for the face-down offered card. Its target is one-hot. Outside
that phase its loss and metrics are masked out.

This is a marginal approximation, not a joint distribution over hand
multisets. It cannot express correlations between cards, but is compact and
does not enumerate an exploding state space.

## Constraints

Predictions sum to one when the corresponding target exists. Card types with no
feasible unseen copies under the player's observation are assigned probability
zero before normalization. Multiplying the hand distribution by the public
opponent hand size therefore preserves the total hand-count constraint. Known
cards are removed through the same player-view inventory calculation used by
the observation encoder.

The model does not claim that every vector of marginal expected counts is a
realisable multiset. Constrained determinization, if later used by search, must
sample without replacement from the remaining inventory rather than sample
each slot independently.

## Information boundary

`PlayerHistory` and `BeliefTarget` are different types. Inference APIs accept
only `PlayerHistory`/`Obs`; they have no `Game` parameter. The simulator-only
target builder is called by training after the input snapshot is created.

Changing opponent hand identities, deck order, or an opponent's face-down
offer while keeping public history fixed must not change inference input or
policy output. It may change `BeliefTarget`; that is precisely why targets are
never embedded into histories or passed to inference.

## Loss and metrics

Both heads use categorical cross-entropy. The total objective is configurable:

`policy_loss + value_weight * value_loss + belief_weight * belief_loss - entropy_weight * entropy`

Reported belief metrics are cross-entropy, top-1 accuracy, expected-count mean
absolute error, Brier score, and expected calibration error (10 fixed bins).
Hidden-offer metrics include only response-phase examples. Metrics must be
reported with game count, seed, and model/checkpoint identity.
