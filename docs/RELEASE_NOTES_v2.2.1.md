# Agent Avenue AI v2.2.1 — Self-Play RL + Human-vs-AI Desktop App

An unofficial C++17 self-play reinforcement-learning project for the standard two-player _Agent Avenue_ base game. This Windows release includes the trained checkpoint, evaluation tools, continued training, and a playable multilingual opponent.

## Included

- pretrained checkpoint with **17,366,354 self-play games** and **1,085,397 optimizer updates**;
- human-vs-AI play with original illustrated cards and outcome explanations;
- continued CPU self-play training with optional CUDA updates;
- checkpoint-safe resume, model evaluation, and Chinese / English / Spanish localization.

## Evaluation snapshot

- **85.28%** win rate vs uniform random;
- **79.76%** win rate vs the handwritten heuristic;
- **53.04%** win rate vs the saved champion snapshot.

Each result uses 5,000 frozen-model games balanced across first and second player. See the repository's evaluation document for protocol and limitations.

## Fixed in v2.2.1

- Clipped the face-down card-back stripe pattern to the card boundary in human-vs-AI play.
- Training files remain compatible with v2.2.0.

## Run on Windows

Download `AgentAvenueAI-Public-v2.2.1-Windows.zip`, extract the complete archive, and run `AgentAvenueAI.exe`. Windows may show a SmartScreen warning because the community binary is not code-signed.

---

## 中文说明

这是一个面向《疯狂特务城》（_Agent Avenue_）标准双人基础玩法的非官方 C++17 自对弈强化学习项目。本次 Windows 发布包含训练好的模型、评测、继续训练和中英西三语人机对战。

公开模型已完成 **17,366,354 局自对弈**、**1,085,397 次参数更新**；冻结模型评测结果为：对随机策略 85.28%，对手写启发式策略 79.76%，对历史冠军快照 53.04%。

v2.2.1 修复了人机对战中暗牌卡背斜纹溢出卡片边界的问题，并保持与 v2.2.0 训练存档兼容。

下载 `AgentAvenueAI-Public-v2.2.1-Windows.zip`，完整解压后运行 `AgentAvenueAI.exe`。
