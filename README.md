# Agent Avenue AI v3.1.0 / 疯狂特务城 · Recurrent Belief AI

[![C++ tests](https://github.com/HongyuLeo/agent-avenue-ai-lab/actions/workflows/ci.yml/badge.svg)](https://github.com/HongyuLeo/agent-avenue-ai-lab/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/HongyuLeo/agent-avenue-ai-lab?display_name=tag&sort=semver)](https://github.com/HongyuLeo/agent-avenue-ai-lab/releases/latest)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](https://isocpp.org/)
[![CUDA](https://img.shields.io/badge/CUDA-optional-76B900?logo=nvidia)](src/v3_cuda_backend.hpp)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

[English details](#highlights) · [简体中文完整说明](README.zh-CN.md) · [Evaluation](docs/EVALUATION.md) · [Architecture](docs/ARCHITECTURE.md) · [Project role](PROJECT_ROLE.md)

**English:** An unofficial C++17 self-play project for the standard two-player base game of *Agent Avenue*. v3 adds player-view histories, a GRU actor-critic, explicit belief estimation, belief-conditioned policy, safe resume, reproducible evaluation, and human-vs-AI play while retaining the v2 feed-forward Baseline.

**简体中文：** 一个面向《疯狂特务城》（*Agent Avenue*）标准双人基础玩法的开源桌游 AI 项目。v3 提供玩家视角历史、GRU、显式 belief、断点续训、模型评测和 Windows 人机对战，并完整保留 v2 Baseline。[阅读完整中文说明 →](README.zh-CN.md)

[**Download the latest Windows release →**](https://github.com/HongyuLeo/agent-avenue-ai-lab/releases/latest)

![English human-versus-AI interface with original illustrated cards and visible language selector](docs/screenshots/play-ui-en.png)

## Highlights

- A complete hidden-information game simulator with legal-action masking.
- Strict per-player observation/action histories with invariance tests for hands, deck order and face-down offers.
- A 15,227-parameter 32-unit GRU with opponent-hand and hidden-offer belief heads, plus configurable belief-conditioned policy.
- The original 9,867-parameter feed-forward actor-critic, public checkpoint and CUDA backend retained as the v2 Baseline.
- Deterministic parallel CPU self-play plus a dynamically loaded CUDA/NVRTC GRU BPTT backend with automatic CPU fallback.
- Typed `AAIV3C01` checkpoints with checksum, `.bak` recovery, exact optimizer/RNG resume, model type and architecture metadata.
- Four-series evaluation charts and an Excel-readable `training-evaluations.csv` with raw wins, rates, losses and belief metrics.
- Human-vs-AI play with public information, outcome explanations, card inspection, and first-player switching.
- Complete runtime localization in Simplified Chinese, English, and Spanish. Chinese is the default; the selected language is remembered across launches.
- Eight original illustrated agents plus a programmatic board. No commercial game artwork is included.
- Regression tests for rule outcomes, hidden-information isolation, history/policy invariance, belief-target separation, GRU BPTT, serialization, corruption recovery, parallel determinism and UI transitions.

![Active human-versus-AI game with illustrated cards](docs/screenshots/active-play.png)

## v2 Baseline checkpoint

The public release checkpoint contains **17,366,354 self-play games** and **1,085,397 optimizer updates**. In a frozen-model evaluation of 5,000 games per opponent, balanced across first and second player, it achieved:

| Opponent | Wins | Win rate |
|---|---:|---:|
| Uniform random | 4,264 / 5,000 | 85.28% |
| Handwritten heuristic | 3,988 / 5,000 | 79.76% |
| Saved champion snapshot | 2,652 / 5,000 | 53.04% |

See [docs/EVALUATION.md](docs/EVALUATION.md) for protocol and limitations.

These are v2 results and are not claims about the short v3 smoke model.

## Download and play on Windows

1. Open the repository's **Releases** page.
2. Download `AgentAvenueAI-Public-v3.1.0-Windows.zip`.
3. Extract the whole archive and run `AgentAvenueAI.exe`.
4. Open **人机对战 / Play vs AI / Jugar vs IA** to play. The included `training.bin` is imported automatically on first launch.

The application starts in Simplified Chinese. Use the always-visible **中文 / English / Español** selector in the header to switch the whole interface; the choice is remembered for the next launch.

The bundled v2 Baseline remains `training.bin`. Writable v3 progress is stored separately at:

```text
%LOCALAPPDATA%\AgentAvenueAI\training-v3.bin
%LOCALAPPDATA%\AgentAvenueAI\training-evaluations.csv
```

Windows may show a SmartScreen warning because the community binary is not code-signed. You can build from source if preferred.

## Build from source

Requirements: CMake 3.20+, a C++17 compiler, and Windows for the desktop application. On Windows with Visual Studio 2022, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

The output is `build\Release\agent_avenue_ai_lab.exe`. CMake copies the frozen v2 Baseline from `models/pretrained-17m.bin` beside the executable as `training.bin`. Build a portable ZIP with `scripts\package-windows.ps1`.

Run resumable v3 training outside the GUI with:

```powershell
build\Release\v3_train.exe 1000000 training-v3.bin 16 auto 128 2048
```

The arguments are additional games, checkpoint path, CPU self-play worker count,
training device (`auto`, `cpu`, or `cuda`), games per update, and the CUDA
samples-per-launch safety limit. `auto` uses
CUDA when the NVIDIA driver and CUDA NVRTC runtime are available and otherwise
falls back to CPU. The Windows GUI uses the same automatic selection.

## Inspect training progress

The Training chart tracks win rates against Random, Handwritten Heuristic, the
fixed v2 Baseline and the current v3 Champion. **Open training log** opens
`%LOCALAPPDATA%\AgentAvenueAI\training-evaluations.csv`, which records raw wins,
rates, counters, losses, belief accuracy, Brier score and ECE for each
evaluation. Rate columns use values from 0 to 1. Historical v2/Champion cells
from a migrated v3.0 checkpoint remain blank because v3.0 did not store them;
new v3.1 evaluations are recorded normally.

To build and run the portable tests on Linux/macOS:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Scope

This repository implements only the standard two-player base mode. It does not implement team play, Black Market, or other expansions. Rule interpretations and known limitations are documented in the source and tests.

## v3 status and measured evidence

All old and new tests pass in the verified MSVC Release build. The CUDA parity
test executed the complete v3 GRU/belief BPTT path on an RTX 4090 and measured a
relative gradient error of `1.53e-7` against CPU, including forced safe batch
splitting. A 1,024-game, 16-worker,
batch-128 smoke benchmark measured 658.5 games/s on CPU and 2,751.1 games/s on
CUDA on this host. These are throughput checks, not playing-strength claims.
The full four-variant ablation used only 64 training games and is likewise a
pipeline check. See [v3 experiments](docs/V3_EXPERIMENTS.md),
[ablations](docs/V3_ABLATIONS.md), and
[known limitations](docs/V3_KNOWN_LIMITATIONS.md).

## Authorship and AI disclosure

Project direction, rules analysis, training operation, defect discovery, evaluation decisions, visual direction, and release ownership are by **Liu Hongyu**. AI tools assisted with implementation, refactoring, test scaffolding, documentation, and generation of the project's original card illustrations under his direction and review. See [PROJECT_ROLE.md](PROJECT_ROLE.md) for the detailed and portfolio-ready statement.

## Legal notice

This is an independent, unofficial research and fan-engineering project. *Agent Avenue*, its trademarks, rules, and original artwork belong to their respective owners. The repository contains no official card scans or commercial artwork and is not endorsed by the publisher or designers.

Original source code is available under the [MIT License](LICENSE).
