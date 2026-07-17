# Data Model: 正式 Floor 1 击倒分支奖励卡内容冻结

## 1. Branch reward card

每条记录描述一个未来 `UCardDefinition`：

| Field | Contract |
|---|---|
| `CardId` | 稳定身份，格式 `Reward.SerpentWood.<Archetype>.<Choice>` |
| `PackagePath` | 制作 locator，格式 `/Game/Wacom/Data/Cards/Rewards/SerpentWood/<Archetype>/DA_Card_<Archetype>_<Choice>` |
| `DisplayName` | 冻结中文名称，不属于稳定身份 |
| `Description` | 使用 `{Effect.N}` 的冻结模板，索引与 Effects 顺序一致 |
| `BaseCost` | 七张为 1；Guardian Destroy 为 2 |
| `Rarity` | Brush White；Molt/Root Blue；Guardian Yellow |
| `Keywords` | Aid 只有 Tool；Destroy 只有 Weapon |
| `TargetMode` | `None`、`SingleEnemyPart` 或 `AllEnemyParts` |
| `Effects[]` | 1–2 个现有 Effect，顺序冻结 |
| 其它字段 | Physique 零值；PerfectRelease/ZoneHook/Passive 为空；美术未冻结 |

## 2. Exact card records

| CardId | Name | Cost | Rarity | Keyword | TargetMode | Effects in order |
|---|---|---:|---|---|---|---|
| `Reward.SerpentWood.BrushSnake.Aid` | 伏草藏身 | 1 | White | Tool | SingleEnemyPart | `Shield 2 → Player`; `Slow 1 → SingleEnemyPart` |
| `Reward.SerpentWood.BrushSnake.Destroy` | 断牙毒刺 | 1 | White | Weapon | SingleEnemyPart | `Damage 3 → SingleEnemyPart`; `Poison 1 → SingleEnemyPart` |
| `Reward.SerpentWood.MoltGuard.Aid` | 蜕甲壁垒 | 1 | Blue | Tool | None | `Shield 7 → Player` |
| `Reward.SerpentWood.MoltGuard.Destroy` | 裂壳重击 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 6 → SingleEnemyPart` |
| `Reward.SerpentWood.RootStalker.Aid` | 盘根护身 | 1 | Blue | Tool | SingleEnemyPart | `Shield 3 → Player`; `Slow 2 → SingleEnemyPart` |
| `Reward.SerpentWood.RootStalker.Destroy` | 毒根突袭 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 5 → SingleEnemyPart`; `Poison 1 → SingleEnemyPart` |
| `Reward.SerpentWood.ShallowGuardian.Aid` | 冠鳞庇护 | 1 | Yellow | Tool | None | `Shield 10 → Player` |
| `Reward.SerpentWood.ShallowGuardian.Destroy` | 碎冠毒潮 | 2 | Yellow | Weapon | AllEnemyParts | `Damage 4 → AllEnemyParts`; `Poison 1 → AllEnemyParts` |

## 3. Archetype reward pair

每个 Archetype 只有一对 Definition。Relationship：

```text
Enemy Archetype 1
  -> Aid CardDefinition 1
  -> Destroy CardDefinition 1
  <- PartDefinition 2..4 explicit references
```

卡牌不拥有 Part、Encounter 或 Node 引用；PartDefinition 单向引用 CardDefinition。

## 4. Part reward assignment

| Archetype | Parts | Aid reference | Destroy reference | Legacy |
|---|---|---|---|---|
| BrushSnake | Head, Body | `BrushSnake.Aid` | `BrushSnake.Destroy` | null |
| MoltGuard | Head, Carapace, Tail | `MoltGuard.Aid` | `MoltGuard.Destroy` | null |
| RootStalker | Head, Coil | `RootStalker.Aid` | `RootStalker.Destroy` | null |
| ShallowGuardian | Head, Body, Tail, Crest | `ShallowGuardian.Aid` | `ShallowGuardian.Destroy` | null |

CardId 前缀完整展开为 `Reward.SerpentWood.`。FormalProduction validation 继续要求两条显式引用存在且 legacy 为空。

## 5. Reward instance semantics

```text
Part HP reaches zero
-> one pending knockdown choice
-> choose Aid or Destroy (or Withdraw when allowed)
-> resolve one CardDefinition through existing query
-> create one independent runtime Card Instance
-> existing Battle/Run persistence rules
```

- 同一敌人的不同 Parts 可生成相同 Definition 的多个实例。
- 相同 Archetype 的不同 Encounter slots 也可生成重复实例。
- 不增加 claimed set、per-enemy cap、replacement currency 或 AP cost。

## 6. Route reward yield

必经 Encounter：Scout 2 Parts + Elite 5 + Guardian 4 = 11。

| Route | Added Parts | Total rewards |
|---|---:|---:|
| A + C | MoltGuard 3 + Shop path 0 | 14 |
| B + C | Ambush 4 + Shop path 0 | 15 |
| A + D | MoltGuard 3 + RootStalker 2 | 16 |
| B + D | Ambush 4 + RootStalker 2 | 17 |
| Full exploration | all six Encounters | 20 |

按 Archetype 的完整探索实例上限：Brush 8、Molt 6、Root 2、Guardian 4，总计 20。每个实例究竟为 Aid 或 Destroy 由玩家选择。

## 7. Production manifest relationship

| Set | Count | Source |
|---|---:|---|
| Floor 1 core assets | 38 | Spec 011 historical manifest |
| Knockdown branch CardDefinitions | 8 | Spec 013 incremental manifest |
| **Future Production total** | **46** | exact union |

本模型不创建资产或 runtime state。DisplayName、Description 和未来美术可经独立内容修订调整；CardId 与已冻结 package 是下一轮制作身份合同。
