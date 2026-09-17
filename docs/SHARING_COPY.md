# Launch and sharing copy

These drafts lead with the technical result and playable artifact rather than asking for stars. Replace the video placeholder only after recording a real demo.

## Show HN

**Title**

> Show HN: I trained an Agent Avenue AI through 17M self-play games (C++/CUDA)

**Post**

> I built an end-to-end self-play reinforcement-learning project for Agent Avenue, a two-player hidden-information board game.
>
> The project includes a C++17 game simulator, legal-action masking, a compact actor-critic policy/value network, deterministic parallel CPU rollouts, an optional CUDA/NVRTC update backend, atomic resumable checkpoints, evaluation against three baselines, and a native Windows human-vs-AI client.
>
> The public checkpoint has completed 17,366,354 self-play games. In frozen-model evaluation over 5,000 games per opponent, balanced across first and second player, it scored 85.28% vs uniform random, 79.76% vs a handwritten heuristic, and 53.04% vs a saved champion snapshot.
>
> I would especially value feedback on the evaluation design, observation representation, and what stronger baselines would make the next result more meaningful.
>
> Source, release, architecture, and evaluation protocol: https://github.com/HongyuLeo/agent-avenue-ai-lab

## Reddit — reinforcement learning / game AI

**Title**

> I built a C++ self-play RL agent for a hidden-information board game and trained it for 17.3M games

**Post**

> I have been working on an end-to-end reinforcement-learning system for the standard two-player Agent Avenue base game. It uses a small actor-critic network (128 observations, 74 masked policy actions, one value head), deterministic parallel CPU self-play, and an optional CUDA/NVRTC update path.
>
> The public checkpoint contains 17.3M self-play games and 1.08M optimizer updates. Frozen-model results over 5,000 games per opponent are 85.28% vs uniform random, 79.76% vs a handwritten heuristic, and 53.04% vs the saved champion snapshot. The repository documents the first/second-player balance and limitations; I am not claiming solved or expert play.
>
> I also packaged the policy into a multilingual Windows human-vs-AI client so the trained behavior can be tested directly rather than only through aggregate metrics.
>
> I would appreciate technical criticism of the observation/action design and suggestions for a stronger reproducible baseline: https://github.com/HongyuLeo/agent-avenue-ai-lab

## BoardGameGeek

**Title**

> I built a downloadable Agent Avenue AI opponent trained through 17M self-play games

**Post**

> I created an unofficial open-source AI project for the standard two-player Agent Avenue base game. It includes a Windows program where you can play directly against the trained model, inspect public information, switch first player, and review the reason for each outcome.
>
> The AI learned through more than 17 million self-play games. The project uses only original character illustrations and does not include scans of the commercial game's artwork.
>
> I would be interested in feedback from experienced players: where does the AI still make strategically weak or unnatural decisions, and which matchups or situations should I test next?
>
> Download, source, and instructions: https://github.com/HongyuLeo/agent-avenue-ai-lab/releases/latest

## LinkedIn

> I have released Agent Avenue AI, an end-to-end self-play reinforcement-learning project for a hidden-information board game.
>
> The work goes beyond training a model: I formalized the two-player rules and private observations, defined the product and reliability requirements, operated a 17.3M-game self-play run, designed frozen-model evaluation against three baselines, drove fixes from real play sessions, and packaged the result as a multilingual Windows human-vs-AI application.
>
> Technical stack: C++17, actor-critic reinforcement learning, deterministic parallel CPU rollouts, optional CUDA/NVRTC updates, atomic resumable checkpoints, CMake, automated tests, and GitHub Actions.
>
> Results: 85.28% vs uniform random, 79.76% vs a handwritten heuristic, and 53.04% vs a saved champion snapshot, each over 5,000 balanced games.
>
> Repository and playable release: https://github.com/HongyuLeo/agent-avenue-ai-lab
>
> I am particularly interested in AI application, game AI, reinforcement-learning engineering, and technical product roles where end-to-end ownership matters.

## YouTube / Bilibili

**English title**

> I Trained an Agent Avenue AI for 17 Million Self-Play Games | C++ Reinforcement Learning

**中文标题**

> 我让 AI 自己对战 1736 万局：做出一个能玩的《疯狂特务城》AI

**Description**

> This video shows an end-to-end C++ self-play reinforcement-learning project for Agent Avenue: the hidden-information simulator, parallel training, evaluation results, checkpoint resume, and a playable human-vs-AI Windows client.
>
> 这不是只调用 API 的演示，而是从游戏规则、隐藏信息环境、自对弈训练、Actor-Critic、CPU/CUDA 更新、模型评测到 Windows 人机对战的完整项目。
>
> Public checkpoint / 公开模型：17,366,354 self-play games  
> Evaluation / 评测：85.28% vs random; 79.76% vs heuristic; 53.04% vs saved champion  
> GitHub: https://github.com/HongyuLeo/agent-avenue-ai-lab

## 45-second demo shot list

Record the real application at 1080p; do not simulate interactions in an edited mockup.

1. **0–4s:** repository title and the 17.3M / 85.28% result line;
2. **4–12s:** launch `AgentAvenueAI.exe` and switch English / 中文;
3. **12–26s:** make one human offer, show the AI response, inspect a card, and advance the board;
4. **26–34s:** open evaluation and show the three frozen-model results;
5. **34–40s:** show the training/checkpoint resume screen;
6. **40–45s:** return to active play with the GitHub URL on screen.

Export both a silent 12–15 second GIF for the README and a narrated 45–90 second MP4 for YouTube, Bilibili, LinkedIn, and Reddit. Keep the GIF under roughly 10 MB and show only genuine program behavior.
