# Project ownership and development disclosure

## Project lead

**Liu Hongyu** initiated and owns this independent project. His work included:

- selecting the problem and defining the playable/training product;
- deciding the supported scope: the standard two-player base game;
- interpreting rules and identifying edge cases, including identical-card offers, simultaneous outcomes, deck exhaustion, and the third-Daredevil loss condition;
- specifying checkpoint continuity, human-vs-AI inspection, visual feedback, CPU/GPU training, and release requirements;
- running the long training campaign on an Intel Core i9-13900K and NVIDIA RTX 4090;
- discovering and reproducing defects from real play sessions, then defining acceptance criteria for fixes;
- reviewing behavior and deciding the testing and evaluation protocol;
- directing the public edition's original visual identity and selecting the final card-art concepts;
- preparing the trained checkpoint and release direction.

At the recorded checkpoint, the training process had completed **17,366,354 self-play games** and **1,085,397 optimizer updates**.

## AI-assisted implementation

AI tools assisted with implementation, refactoring, test scaffolding, documentation, original card-art generation, and release preparation under Liu Hongyu's direction and review. This repository therefore describes the work as **AI-assisted software development**, not as code written entirely by hand and not as an autonomous AI project.

The project's main contribution is the complete engineering loop: turning game rules into a hidden-information simulator, defining a trainable agent, operating a large training run, diagnosing rule/UI defects, independently evaluating the saved model, and packaging a reproducible application.

## Suggested portfolio wording

> Led the design, training, validation, and release of a C++ self-play reinforcement-learning agent for a hidden-information board game. Defined rule semantics and product requirements, operated a 17.3M-game training run on RTX 4090 hardware, found and drove fixes for outcome/UI defects, and validated the saved policy against three baselines. Used AI coding tools as an implementation accelerator under my direction and review.
