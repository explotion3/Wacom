# Backpack Presentation Performance Baseline

## 状态

本页记录 2026-07-24 的 Backpack Presentation Runtime 重构性能证据。

- 重构前完成了 `24 / 100 cards × 1280×720 / 1920×1080` 通用矩阵，以及 `1 / 4 / 5 / 18 cards × Full / Simplified Motion` 出售矩阵。
- 重构后完成了 `24 cards × 1280×720` 一组同脚本对照。该样本通过绝对预算和相对不回归门禁。
- 用户决定暂不继续重采其余重构后矩阵。因此本轮只能得出“已采样场景无回归”的部分结论，不能关闭完整矩阵或出售 Retainer 重构后量化债。
- `.utrace`、导出 CSV 与 Unreal Insights 日志保存在 `Saved/Profiling/Backpack/`，不进入 Git。

## 环境与统计口径

| 项目 | 值 |
|---|---|
| 地图 / 运行方式 | `/Game/Wacom/Maps/L_Exploration`，Win64 Development Editor PIE，单机 |
| CPU | Intel Core i5-14600KF，14 cores / 20 threads |
| GPU | NVIDIA GeForce RTX 4070 SUPER，driver `32.0.15.9186` |
| 内存 / 系统 | 约 32 GiB，Windows 11 Pro `10.0.26200` |
| 图形设置 | `Scalability 3`、`r.VSync=0`、`r.VSyncEditor=0`、`t.MaxFPS=0` |
| 通用时间线 | 预热 10 s；Backpack Closed / Idle / Interaction 各 60 s |
| Interaction 脚本 | 展开/收起、底部 Hover 扫过、框选、五张以上 Carry、滚轮切卡、合法/拒绝目标 Hover、取消 |
| CPU 帧 | GameThread 的 `FEngineLoop::Tick`，单位 ms |
| GPU 帧 | `GPU0-Graphics0` 的 `Frame`，单位 ms |
| 百分位 | 每个 Region 内按 nearest-rank 计算 p95 / p99 |

重构前生产表现基线为 `e8057a650e5be09d28d9315f67dff940ab8daa89`。通用 Trace 由只增加采集/标记、不改 Workspace 表现语义的临时采集提交 `9a9fcaf0`、`cd686cc0` 生成；最终采集工具整理为 `05d42102fe5060d64cf09919c9ba6f3e8033f848`。重构后样本精确对应 `9967795211b010b9079ebfc071f40969ad1101fb`。

## 重构前通用矩阵

| Cards | Resolution | GT Closed p95 | GT Idle p95 | GT Interaction p95 / p99 / max | GPU Interaction p95 / max | Interaction hitch |
|---:|---:|---:|---:|---:|---:|---|
| 24 | 1280×720 | 11.19 | 10.22 | 14.14 / 18.05 / 31.35 | 11.01 / 16.58 | `>50 ms: 0` |
| 24 | 1920×1080 | 10.87 | 10.61 | 12.57 / 16.83 / 34.83 | 10.17 / 18.12 | `>50 ms: 0` |
| 100 | 1280×720 | 10.99 | 11.15 | 14.03 / 17.29 / 38.13 | 11.44 / 41.89 | `>100 ms: 0` |
| 100 | 1920×1080 | 11.21 | 11.54 | 15.28 / 18.67 / 41.52 | 12.20 / 24.36 | `>100 ms: 0` |

100-card 两个 Closed Region 各出现一个孤立的约 `180.51 ms` / `159.43 ms` 帧；对应 Interaction Region 没有超过 `100 ms` 的帧，故不把它们归因于背包交互热路径，但后续正式重采仍应保留 Closed 基线对照。

## 重构前出售矩阵

出售采样使用 100 cards、1920×1080；每次重启 PIE 并重新 Seed。`Sale p95 / max / total` 是 GameThread 上 `Wacom_Backpack_FrameSaleDeparture` 的单次 Scope 成本与整个 Region 累计成本。

| Motion | Sold | GT p95 / max | GPU p95 / max | Sale frames | Sale p95 / max / total | Max realtime Retainers |
|---|---:|---:|---:|---:|---:|---:|
| Full | 1 | 12.30 / 19.19 | 11.90 / 20.56 | 38 | 0.13 / 0.42 / 1.83 | 1 |
| Full | 4 | 11.39 / 17.29 | 11.27 / 21.04 | 75 | 0.38 / 0.43 / 4.97 | 4 |
| Full | 5 | 11.53 / 14.05 | 11.32 / 18.39 | 90 | 0.40 / 0.53 / 6.37 | 4 |
| Full | 18 | 11.47 / 20.00 | 11.28 / 20.91 | 242 | 0.40 / 0.65 / 20.27 | 4 |
| Simplified | 1 | 11.58 / 14.48 | 11.51 / 21.59 | 13 | 0.40 / 0.40 / 0.94 | 1 |
| Simplified | 4 | 11.17 / 31.12 | 11.13 / 19.92 | 28 | 0.42 / 0.43 / 3.03 | 3 |
| Simplified | 5 | 11.61 / 15.16 | 11.46 / 21.13 | 33 | 0.43 / 0.55 / 3.82 | 3 |
| Simplified | 18 | 10.42 / 18.74 | 10.42 / 19.18 | 101 | 0.41 / 0.44 / 12.59 | 3 |

八组采样都完成精确出售数量，结束时 Queue / Active 为 0，最大实时 Retainer 没有超过 4。这里证明的是重构前算法及并发上限；由于未采集重构后出售矩阵，不能据此关闭重构后的量化验证债。

## 重构后已完成对照

| Cards | Resolution | Phase | GT p95 / p99 / max | GPU p95 / max | Hitch |
|---:|---:|---|---:|---:|---|
| 24 | 1280×720 | Closed | 10.69 / 11.52 / 77.18 | 9.82 / 75.28 | Closed 各 1 个 `>50 ms` |
| 24 | 1280×720 | Idle | 10.27 / 11.24 / 14.27 | 9.56 / 14.09 | `>50 ms: 0` |
| 24 | 1280×720 | Interaction | 13.22 / 16.33 / 33.04 | 9.93 / 18.29 | `>50 ms: 0` |

与同场景重构前数据相比：

- GT Interaction p95：`14.14 → 13.22 ms`，约 `-6.5%`。
- GPU Interaction p95：`11.01 → 9.93 ms`，约 `-9.8%`。
- GT Interaction 相对 Closed 的 p95 增量：`+2.95 → +2.53 ms`。
- GT Idle p95：`10.22 → 10.27 ms`，约 `+0.5%`，处于噪声范围且没有超过 50 ms 的 Idle 帧。
- `Wacom_Backpack_PresentationPipelineFlush`：调用 `704 → 380`，累计 `75.72 → 31.92 ms`，p95 `0.24 → 0.18 ms`。
- `Wacom_Backpack_FrameSchedulerTick`：调用 `4461 → 4019`，累计 `353.85 → 230.92 ms`，p95 `0.13 → 0.10 ms`。

Interaction 是固定清单下的真实人工操作，不是逐帧回放，调用次数只能作为方向性证据；最终判定以帧 p95、最大交互帧、自动化热路径计数和行为验收共同成立。

该样本满足 24-card 的 `p95 ≤ 16.67 ms`、交互期间无 `>50 ms` 卡顿，并且相对重构前没有超过 10% 的回归。

## Trace provenance

| Trace | SHA-256 |
|---|---|
| `pre_9a9fcaf0_24_1280x720.utrace` | `1F3B95B888C76764D9B1AF74761600423206DD2D28801E36CE97E0DE4701205D` |
| `pre_cd686cc0_rerun_24_1920x1080.utrace` | `DF368E800D42FA8821DF7BF45C3C23DC9352BAF952ED762934EE8E72F724245A` |
| `pre_cd686cc0_100_1280x720.utrace` | `22CE603D5E79B3A8AA69B16972C677E74E61947686F65A18C436DBF0C3C04EA2` |
| `pre_cd686cc0_100_1920x1080.utrace` | `138D3A3DA6B4D92A3C27D8ECB9464EEEDA31FFEA0CE24C66F70C9E2A830610A6` |
| `pre_cd686cc0_sale_1_Full_100_1920x1080.utrace` | `BC5FDACE975B52F39DEB500D026E5D5E60BCDBBFC726539E0CDB859C36FD34FE` |
| `pre_cd686cc0_sale_4_Full_100_1920x1080.utrace` | `B4A13453D1C6DC1C6079A1D1D40ED498C32F727803F3CCF1292CAEC414757910` |
| `pre_cd686cc0_sale_5_Full_100_1920x1080.utrace` | `5B60AB34BB6644FE8B2EFB0C6C56E94D913D83BF203EDA77225BE231091B7413` |
| `pre_cd686cc0_sale_18_Full_100_1920x1080.utrace` | `7A5C8ABAC3CCAC5D5C4740B6B920E99CC0989F84C043EACEB90E87A4001DED9A` |
| `pre_cd686cc0_sale_1_Simplified_100_1920x1080.utrace` | `E204D5C55F3895BB5E953D045D38F1C7546382177A900C5B626F843E41989129` |
| `pre_cd686cc0_sale_4_Simplified_100_1920x1080.utrace` | `3A2CDCBDD04C2DC4A93F1F5BA9826DC7BB465E80BF187CAFEE8DC5A851E26F37` |
| `pre_cd686cc0_sale_5_Simplified_100_1920x1080.utrace` | `F0181368CD28F1F533EAA9A6DF749F172576F21D75830FEE3832F56D69B81D56` |
| `pre_cd686cc0_sale_18_Simplified_100_1920x1080.utrace` | `DACBAD09350B6F1636435197E3359680380B8890FCBEEDF463915C5159494DD0` |
| `post_99677952_24_1280x720.utrace` | `C7C334AB8BCA08DBD3F4D7558BA4D8733FBC6A9F79B582FBAF5078C3F1C9D378` |

## 尚未关闭的门禁

- 重构后 `24 cards × 1920×1080`。
- 重构后 `100 cards × 1280×720 / 1920×1080`。
- 重构后 `1 / 4 / 5 / 18 cards × Full / Simplified Motion` 出售矩阵。
- 上述场景的前后 p95 不超过 10% 回归、普通 Hover/Carry 实时 Retainer `≤1`、出售实时 Retainer `≤4` 的完整量化复核。

在这些样本补齐前，`Docs/TechDebt.md` 中对应两项保持开放；现有自动化继续作为热路径与并发上限的结构门禁。
