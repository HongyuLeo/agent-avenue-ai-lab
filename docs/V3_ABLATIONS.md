# v3 Ablations

Measured on 2026-09-21 on the local Windows host, MSVC/GCC Release builds. Each
variant received only 64 training games and then 200 fixed-seed, seat-balanced
games per opponent. These runs verify code paths; their confidence intervals
are too wide to rank playing strength.

| Variant | train games/s | inference us | vs Random | vs Heuristic | vs public v2 |
|---|---:|---:|---:|---:|---:|
| Recurrent only | 849 | 3.62 | 51.0% | 14.5% | 11.0% |
| Belief auxiliary only | 1,419 | 2.08 | 50.5% | 7.5% | 13.5% |
| Recurrent + belief | 883 | 3.55 | 53.0% | 17.0% | 2.5% |
| Recurrent + belief-conditioned policy | 826 | 3.54 | 48.5% | 12.0% | 19.0% |

The recurrent-belief conditioned run had belief cross-entropy 1.985, Brier
score 0.405 and 10-bin ECE 0.028 over its smoke-training samples. These are
pipeline measurements, not evidence of a useful posterior after 64 games.

Reproduce with:

```powershell
build\Release\v3_benchmark.exe 64 200 models\pretrained-17m.bin build\v3-smoke.bin
```
