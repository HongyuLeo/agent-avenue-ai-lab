# Project ownership and development disclosure

## Project lead

**Liu Hongyu** initiated, directed, trained, validated, and released this independent project. His work included:

- selecting the problem and defining the combined training and playable-product scope;
- interpreting the standard two-player rules and specifying edge cases, including identical-card offers, simultaneous outcomes, deck exhaustion, and the third-Daredevil loss condition;
- defining observation privacy, checkpoint continuity, human-vs-AI inspection, visual feedback, CPU/GPU training, evaluation, and release requirements;
- running the long training campaign on an Intel Core i9-13900K and NVIDIA RTX 4090;
- discovering and reproducing defects from real play sessions, then defining acceptance criteria for fixes;
- choosing the baseline opponents, first/second-player balance, frozen-model protocol, and reporting limitations;
- directing the public edition's original visual identity and selecting the final card-art concepts;
- preparing the trained checkpoint, multilingual Windows package, documentation, and public release.

At the recorded checkpoint, training had completed **17,366,354 self-play games** and **1,085,397 optimizer updates**.

## Engineering evidence in the repository

| Capability | Verifiable artifact |
|---|---|
| Hidden-information simulator and legal-action masking | `src/core.hpp`, `tests/core_checks.cpp` |
| Actor-critic policy/value network | `src/core.hpp` |
| Deterministic parallel self-play | `src/fast_train.hpp`, `tests/fast_checks.cpp` |
| Optional CUDA/NVRTC update backend | `src/cuda_backend.hpp` |
| Atomic, resumable checkpoints | `src/core.hpp`, checkpoint regression tests |
| Frozen-model evaluation against three baselines | `docs/EVALUATION.md` |
| Playable multilingual Windows GUI | `src/windows.cpp`, `docs/screenshots/` |
| Automated Linux and Windows builds | `.github/workflows/ci.yml` |

The public result is therefore not only a model file or API wrapper. It is an end-to-end loop from rule formalization and private observations through self-play training, reliability engineering, evaluation, and a downloadable application.

## AI-assisted implementation

AI tools assisted with implementation, refactoring, test scaffolding, documentation, original card-art generation, and release preparation under Liu Hongyu's direction and review. This repository describes the work as **AI-assisted software development**: it does not claim that every line was written manually, and it does not present an autonomous AI-generated project.

The central contribution is product and engineering ownership: turning game rules into a testable hidden-information system, defining a trainable agent, operating a large training run, diagnosing failures from actual play, deciding evaluation methodology, and packaging a reproducible public application.

## Portfolio wording

### One-line project summary

> Built and released an end-to-end C++ self-play reinforcement-learning system for a hidden-information board game, including a simulator, actor-critic agent, CPU/CUDA training, reproducible evaluation, reliable checkpoints, and a playable multilingual Windows client.

### Resume bullets

- Led the design, training, validation, and release of a C++17 hidden-information game AI; operated a **17.3M-game** self-play run and evaluated the frozen policy over **15,000 baseline matches** balanced across first and second player.
- Shipped the complete training-to-product loop: deterministic parallel rollouts, optional CUDA/NVRTC updates, atomic checkpoint recovery, automated regression tests, GitHub CI, and a downloadable human-vs-AI Windows application.
- Used AI coding tools as an implementation accelerator while retaining ownership of rules analysis, requirements, training operations, defect reproduction, evaluation decisions, acceptance criteria, and public release.

### Interview framing

Be ready to explain four concrete decisions rather than only showing the UI:

1. how private information is excluded from the 128-element player observation;
2. why illegal actions are masked across the 74 policy outputs;
3. what must be serialized to resume a long self-play run exactly;
4. why evaluation freezes the model and balances first and second player.
