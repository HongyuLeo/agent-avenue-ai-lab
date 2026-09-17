# Agent Avenue AI / 疯狂特务城 · 模型训练与人机对战

[English details](#highlights) · [简体中文完整说明](README.zh-CN.md) · [Evaluation](docs/EVALUATION.md) · [Architecture](docs/ARCHITECTURE.md) · [Project role](PROJECT_ROLE.md)

**English:** An unofficial, open-source C++17 self-play reinforcement-learning project for the standard two-player base game of *Agent Avenue*. It provides a Windows desktop application for continued training, checkpoint-safe resume, model evaluation, and human-vs-AI play.

**简体中文：** 一个面向《疯狂特务城》（*Agent Avenue*）标准双人基础玩法的开源桌游 AI 项目。项目使用 C++17 实现隐藏信息博弈和自对弈强化学习，提供 CUDA 加速训练、断点续训、模型评测，以及带原创角色插画的 Windows 可视化人机对战程序。[阅读完整中文说明 →](README.zh-CN.md)

![English human-versus-AI interface with original illustrated cards and visible language selector](docs/screenshots/play-ui-en.png)

## Highlights

- A complete hidden-information game simulator with legal-action masking.
- A compact actor-critic network: 128 inputs, one 48-unit `tanh` layer, 74 policy outputs, and one value output (9,867 trainable parameters).
- Parallel CPU self-play plus an optional CUDA gradient backend loaded dynamically through the NVIDIA Driver API and NVRTC.
- Atomic checkpoints, checksum validation, `.bak` recovery, and exact resume of model, optimizer, RNG, and partial batch state.
- Human-vs-AI play with public information, outcome explanations, card inspection, and first-player switching.
- Complete runtime localization in Simplified Chinese, English, and Spanish. Chinese is the default; the selected language is remembered across launches.
- Eight original illustrated agents plus a programmatic board. No commercial game artwork is included.
- Regression tests for rule outcomes, hidden-information isolation, serialization, gradients, UI transitions, and the reported third-Daredevil scenario.

![Active human-versus-AI game with illustrated cards](docs/screenshots/active-play.png)

## Trained checkpoint

The public release checkpoint contains **17,366,354 self-play games** and **1,085,397 optimizer updates**. In a frozen-model evaluation of 5,000 games per opponent, balanced across first and second player, it achieved:

| Opponent | Wins | Win rate |
|---|---:|---:|
| Uniform random | 4,264 / 5,000 | 85.28% |
| Handwritten heuristic | 3,988 / 5,000 | 79.76% |
| Saved champion snapshot | 2,652 / 5,000 | 53.04% |

See [docs/EVALUATION.md](docs/EVALUATION.md) for protocol and limitations.

## Download and play on Windows

1. Open the repository's **Releases** page.
2. Download `AgentAvenueAI-Public-v2.2.1-Windows.zip`.
3. Extract the whole archive and run `AgentAvenueAI.exe`.
4. Open **人机对战 / Play vs AI / Jugar vs IA** to play. The included `training.bin` is imported automatically on first launch.

The application starts in Simplified Chinese. Use the always-visible **中文 / English / Español** selector in the header to switch the whole interface; the choice is remembered for the next launch.

The writable checkpoint is stored at:

```text
%LOCALAPPDATA%\AgentAvenueAI\training.bin
```

Windows may show a SmartScreen warning because the community binary is not code-signed. You can build from source if preferred.

## Build from source

Requirements: CMake 3.20+, a C++17 compiler, and Windows for the desktop application. On Windows with Visual Studio 2022, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

The output is `build\Release\agent_avenue_ai_lab.exe`. CMake also copies the frozen public checkpoint from `models/pretrained-17m.bin` beside the executable as `training.bin`.

To build and run the portable tests on Linux/macOS:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Scope

This repository implements only the standard two-player base mode. It does not implement team play, Black Market, or other expansions. Rule interpretations and known limitations are documented in the source and tests.

## Authorship and AI disclosure

Project direction, rules analysis, training operation, defect discovery, evaluation decisions, visual direction, and release ownership are by **Liu Hongyu**. AI tools assisted with implementation, refactoring, test scaffolding, documentation, and generation of the project's original card illustrations under his direction and review. See [PROJECT_ROLE.md](PROJECT_ROLE.md) for the detailed and portfolio-ready statement.

## Legal notice

This is an independent, unofficial research and fan-engineering project. *Agent Avenue*, its trademarks, rules, and original artwork belong to their respective owners. The repository contains no official card scans or commercial artwork and is not endorsed by the publisher or designers.

Original source code is available under the [MIT License](LICENSE).
