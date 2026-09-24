# 疯狂特务城 AI v3.1.0 · Recurrent Belief AI

[English](README.md) · [模型评测](docs/EVALUATION.md) · [架构说明](docs/ARCHITECTURE.md) · [项目分工](PROJECT_ROLE.md)

这是一个针对《疯狂特务城》（*Agent Avenue*）标准双人基础玩法的非官方 C++17 自对弈强化学习项目。v3 在保留 v2 前馈 Baseline 的同时，加入严格玩家视角历史、GRU、显式 belief 预测和 belief-conditioned policy。

![使用原创角色插画的人机对战界面](docs/screenshots/play-ui.png)

## 主要功能

- 隐藏信息对局模拟器和合法行动遮罩；
- 严格玩家视角 observation-action history，以及隐藏手牌、牌堆顺序和暗牌不变量测试；
- 32 单元 GRU、对手手牌/暗牌 belief head、belief-conditioned policy，共 15,227 个参数；
- v2 的 9,867 参数前馈 Actor-Critic、公开模型和 CUDA 后端完整保留为 Baseline；
- v3 确定性多线程 CPU 自对弈，以及动态加载的 CUDA/NVRTC GRU BPTT；CUDA 不可用或运行失败时自动回退 CPU；
- 新格式记录模型类型、架构、优化器、RNG、指标和对手池，并支持校验、原子写入和 `.bak` 恢复；
- 训练图表同时显示 Random、Heuristic、v2 Baseline 和 v3 Champion，并自动生成可由 Excel 打开的 `training-evaluations.csv`；
- 可视化人机对战、卡牌说明、公开信息和胜负原因；
- 完整支持简体中文、英语和西班牙语；默认中文，切换后会记住上次选择；
- 8 张原创角色插画与程序绘制棋盘，不包含商业游戏的官方插画；
- 覆盖规则结算、隐藏信息、存档、数值梯度、界面状态和“第 3 张亡命之徒”问题的自动测试。

![使用原创插画进行人机对战](docs/screenshots/active-play.png)

## v2 Baseline 模型

公开存档已完成 **17,366,354 局自对弈**、**1,085,397 次参数更新**。冻结模型后，每个对手测试 5,000 局，并均衡先后手，结果如下：

| 对手 | 获胜局数 | 胜率 |
|---|---:|---:|
| 随机策略 | 4,264 / 5,000 | 85.28% |
| 手写启发式策略 | 3,988 / 5,000 | 79.76% |
| 已保存冠军快照 | 2,652 / 5,000 | 53.04% |

详细方法和限制见 [docs/EVALUATION.md](docs/EVALUATION.md)。这些数据只代表 v2 前馈 Baseline。

## v3.1 长训模型

随发布包提供的 recurrent-belief 存档已经完成 **22,287,872 局自对弈**和
**174,124 次参数更新**，Saved Champion 产生于第 **21,860,096** 局。
重新冻结模型后，每个对手独立测试 5,000 局并均衡先后手，结果如下：

| 对手 | 当前 v3.1 获胜局数 | 胜率 |
|---|---:|---:|
| 随机策略 | 4,294 / 5,000 | 85.88% |
| 手写启发式策略 | 4,195 / 5,000 | 83.90% |
| 固定公开 v2 Baseline | 2,728 / 5,000 | **54.56%** |
| v3 Saved Champion | 2,439 / 5,000 | 48.78% |

Saved Champion 对同一个固定 v2 Baseline 的独立结果为 2,632 / 5,000，
即 **52.64%**。这说明 v3 对这个特定 v2 checkpoint 有小幅、可重复评测的
优势，但不能据此宣称达到人类高手水平或对所有对手都更强。

## 普通玩家如何使用

1. 进入本仓库的 **Releases** 页面；
2. 下载 `AgentAvenueAI-Public-v3.1.0-Windows.zip`；
3. 解压整个压缩包，双击 `AgentAvenueAI.exe`；
4. 点击“人机对战”。首次启动会自动导入压缩包内的预训练模型。

程序默认显示简体中文。窗口顶部始终显示 **中文 / English / Español**，点击即可切换整个界面；选择会自动保存，下次启动继续使用上次的语言。

发布包内的 `training.bin` 是只读 v2 Baseline，同时包含已经验证的
`training-v3.bin` 和 `training-evaluations.csv`。首次启动时，程序只会在
本机不存在 v3 存档的情况下导入它们，绝不会覆盖用户已有的训练进度。
v3 可写存档单独保存在：

```text
%LOCALAPPDATA%\AgentAvenueAI\training-v3.bin
```

因为社区发布的 EXE 没有购买代码签名证书，Windows 可能显示 SmartScreen 提示。介意时可以自行从源码构建。

## 从源码构建

Windows 安装 Visual Studio 2022 的“使用 C++ 的桌面开发”和 CMake 后，在 PowerShell 执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

生成文件为 `build\Release\agent_avenue_ai_lab.exe`。CMake 会把冻结的 v2 Baseline `models/pretrained-17m.bin` 自动复制到 EXE 旁边并命名为 `training.bin`。运行 `scripts\package-windows.ps1` 可生成发布 ZIP；可选参数 `-V3Checkpoint` 和 `-TrainingLog` 用于把长训模型与 CSV 加入发布包，而不会把运行时存档提交到 Git。

命令行继续 v3 训练：

```powershell
build\Release\v3_train.exe 1000000 training-v3.bin 16 auto 128 2048
```

参数依次为新增训练局数、v3 存档路径、CPU self-play worker 数量、训练设备（`auto`、`cpu` 或 `cuda`）、每次更新的对局数和单次 CUDA 提交的样本安全上限。`auto` 会在 NVIDIA 驱动和 CUDA NVRTC 可用时使用 CUDA，否则自动回退 CPU。普通用户仍可直接在 GUI 中开始、暂停、自动保存和恢复。

## 查看训练趋势

训练页图表同时显示对 Random、Handwritten Heuristic、固定 v2 Baseline 和
当前 v3 Champion 的胜率。点击“打开训练日志”可以直接查看：

```text
%LOCALAPPDATA%\AgentAvenueAI\training-evaluations.csv
```

该 CSV 包含每次评测时的自博弈局数、更新次数、四类对手的原始胜场和胜率、
policy/value/belief loss、belief accuracy、Brier score 和 ECE。胜率列使用
`0` 到 `1` 的数值，例如 `0.35` 表示 35%。旧 v3.0 存档没有记录历史 v2 和
Champion 曲线，因此这些旧行会留空；升级后的新评测会正常记录，绝不反推或伪造。

## v3 工程验证

MSVC Release 下全部旧测试和新测试均通过。一次 16 worker 的确定性 smoke run 前 256 局约 1,055 局/秒，重启后从 256 局继续到 320 局。四种消融仅训练了 64 局，只用于验证管线；长期棋力结果使用上文单独重新运行的 5,000 局冻结模型评测。详见 [实验](docs/V3_EXPERIMENTS.md)、[消融](docs/V3_ABLATIONS.md) 和 [已知限制](docs/V3_KNOWN_LIMITATIONS.md)。

v3 GRU/belief BPTT 已在 RTX 4090 上实机验证，并通过 CPU/CUDA 梯度一致性测试。1,024 局、16 worker、batch 128 的短时吞吐测试为 CPU 658.5 局/秒、CUDA 2,751.1 局/秒。这些吞吐数据仍然只是性能 smoke test；棋力结论只采用独立、冻结且均衡先后手的评测。

Linux/macOS 可以构建并运行规则与核心测试：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 项目范围

目前只支持标准双人基础玩法，不包含组队、黑市或其他扩展。

## 项目归属与 AI 使用说明

**Liu Hongyu（刘宏宇）**负责项目方向、规则分析、训练运行、缺陷发现、评测决策、视觉方向和发布。AI 工具在其指导和审查下辅助了代码实现、重构、测试脚手架、文档及本项目原创卡牌插画的生成。详细说明见 [PROJECT_ROLE.md](PROJECT_ROLE.md)。

## 法律说明

本项目是独立、非官方的研究与爱好者工程项目，不受游戏出版方或设计者背书。《疯狂特务城》/ *Agent Avenue* 的商标、规则和官方美术归相应权利人所有；本仓库不包含官方卡牌扫描图或商业插画。

原创源代码采用 [MIT License](LICENSE)。
