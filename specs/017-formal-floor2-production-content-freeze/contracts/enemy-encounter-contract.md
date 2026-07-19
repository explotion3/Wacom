# Contract: MoltCavern Enemy and Encounter

## 1. Enemy graph invariants

- 恰好 4 Enemy、4 Behavior、12 Part、26 Intent。
- 每个 Enemy 使用同 Archetype Behavior、`DefaultPhaseId=Default`、空 `BehaviorOverride`。
- 每个 PartSlot 使用一个独立 `Sequence` IntentSet；set/intent authored order 按 [data-model.md](../data-model.md) §3。
- `CooldownSelections=0`，selector rules 与 fallback 为空。
- Damage/Poison/Slow 指向 Player；Shield 指向行动 Part 自身；Slow 为 `Default / TargetCardCount=1`。
- Part reward 双引用按同 Archetype 显式配置，legacy 为空。

## 2. Archetype totals

| Archetype | Parts | Total HP | Total EXP | Intent count |
|---|---:|---:|---:|---:|
| ScaleCrawler | 2 | 21 | 2 | 5 |
| StoneScaleGuard | 3 | 36 | 4 | 6 |
| VenomHunter | 3 | 34 | 5 | 7 |
| CavernGuardian | 4 | 70 | 10 | 8 |

## 3. Encounter contract

| Encounter | Authored slots | HP |
|---|---|---:|
| ScaleScout | `Scout → ScaleCrawler` | 21 |
| StoneScaleGuard | `Guard → StoneScaleGuard` | 36 |
| HatcheryAmbush | `Left → ScaleCrawler`, `Right → ScaleCrawler` | 42 |
| BridgeSentinel | `Sentinel → StoneScaleGuard` | 36 |
| VenomHunter | `Hunter → VenomHunter` | 34 |
| EliteMolter | `Guard → StoneScaleGuard`, `Scout → ScaleCrawler` | 57 |
| CavernGuardian | `Guardian → CavernGuardian` | 70 |

 authored slot order 是稳定结构；DisplayName 可调。任何 Encounter 最多两个敌人。`bBoss=true` 只由 Floor 2 `Node.Guardian.01` payload 持有，不复制到 Encounter/Enemy。

## 4. Validation profile

未来资产必须同时通过：

1. 通用 Enemy/Behavior/Part/Encounter Data Validation；
2. `FormalProduction` Part profile：Aid 与 Destroy 显式有效、legacy 为空；
3. exact structure：stable ID、part/slot/set/intent/encounter order 与引用一致；
4. forbidden reference closure：没有 Debug/Authoring/Test/Character/scene/presentation 依赖。

HP、EXP、I/R/magnitude 是本次冻结的初始设计值；后续平衡调整必须显式记录为内容修订，不能让 seed-only 工具覆盖已有人工调参。
