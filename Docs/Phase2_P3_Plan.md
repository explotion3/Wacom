# P3 Plan: 规则补全

本文细化 `Phase2_Plan.md §5` 的 P3。目标是让虫妹最小卡组中**目前只落了占位**的规则全部真正生效。完成后一场战斗的规则行为应该和 `Battle_Rules.md` 完全一致，不再留"只记录层数不触发"的场景。

## 1. 范围回顾

第一阶段 S1-S11 留下的"占位"规则：

| 规则 | 现状 | 目标 |
| --- | --- | --- |
| `Status.Retain`（保留关键字） | 只有锚点保留，普通卡 Retain 字段被忽略 | 回合结束时 Retain 普通卡不进弃牌 |
| 双手区保留 | 未实现 | 左右手都在时，双手区普通卡回合结束后保留 |
| `Status.Poison`（中毒） | 只记录 `Stacks`，不造成伤害 | 玩家打牌后 + 敌部位行动后双方结算，穿透护盾 |
| `Status.Slow` / `Twilight` / `Freeze`（除跳过外） | Freeze 仅作为"跳过意图"共享晕厥分支；Slow/Twilight 仅记层数 | Slow/Twilight 先按"层数记录、暂不影响数值"继续，P3 只做中毒。Slow/Twilight 的数值公式留 P4+（Battle_Rules §16 未决项） |
| `Card.ZoneHooks` 消费 | DataAsset 里有 ZoneHook 字段，但 `PlayCardResolver` 不读 | 朝光暮蝶：左手区 + 完美释放 → 本次不推进先机；右手区 + OnPlay → 费用转移 |
| `FCardPassive.AfterPlayed` 被动 | 已实现（S7） | 保持 |
| `FCardPassive.OnCompanionCount` 被动 | 未实现（拂晓飞蛾） | 每场战斗打出 3 张 Companion 后，拂晓飞蛾从非手牌区域回到手牌 |
| `FCardPassive.OnTwilightTriggered` 被动 | 未实现（暮蛉） | 第一阶段受阻于"暮气触发点"未定义，P3 写最简"状态施加时即触发"占位，标注未决 |

## 2. 切片划分

按依赖关系排序。每个切片独立编译 + 独立测试。

| 切片 | 内容 | 依赖 |
| --- | --- | --- |
| **P3.1** | 中毒结算 | 无（独立） |
| **P3.2** | 保留关键字 + 双手区保留 | 无（独立） |
| **P3.3** | ZoneHook 消费（朝光暮蝶左/右手区效果） | 无（独立） |
| **P3.4** | OnCompanionCount 被动（拂晓飞蛾） | 无 |
| **P3.5** | OnTwilightTriggered 被动（暮蛉） | 可独立，但"暮气触发点"规则未定，做占位 |
| **P3.6** | 自动化测试补充 | P3.1-P3.5 全部完成 |

## 3. P3.1：中毒结算

### 3.1 规则（对齐 Battle_Rules §15）

触发时机：
- **玩家每打出一张牌后**：对敌我双方当前中毒层数的拥有者结算一次
- **敌方部位每行动一次后**：同上

结算方式：
- 等于当前 `Stacks` 点伤害
- 穿透护盾（直接扣 HP，不经 Shield）
- 层数不减（持续存在直到被治疗或移除）

### 3.2 代码改动

| 文件 | 改动 |
| --- | --- |
| `WacomBattle/Private/Status/PoisonResolver.h/.cpp`（新） | 提供 `ResolvePoisonForAllHosts(state, events)` 入口：遍历所有部位 + 玩家，对拥有 `Status.Poison` 的一方造成等于层数的伤害，发 `DamageDealt` 事件 |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 在效果结算 + 先机推进之后、战斗结束判断之前调 `FPoisonResolver::ResolvePoisonForAllHosts` |
| `WacomBattle/Private/Enemy/EnemyPartActionResolver.cpp` | `ActOnce` 执行完意图（或晕厥跳过）之后调 `FPoisonResolver::ResolvePoisonForAllHosts` |
| `WacomBattle/Public/Runtime/RuntimeEnemyPart.h` | 无改动，字段已有 `StatusStacks` |
| `WacomBattle/Private/Core/BattleState.h` | 加 `TMap<FGameplayTag, int32> PlayerStatusStacks` + `FGameplayTagContainer PlayerStatuses`（玩家中毒的载体，第一阶段只用到 Poison） |
| `WacomBattle/Private/Effects/EffectExecutor.cpp` | 玩家中毒分支从"只发事件"改为"写 State 层数" |
| `WacomBattle/Public/Snapshots/BattleSnapshot.h` | `FPlayerSnapshot` 加 `FGameplayTagContainer Statuses` + `TMap<FGameplayTag, int32> StatusStacks` |
| `WacomBattle/Private/Snapshots/BattleSnapshotBuilder.cpp` | 玩家 status 字段镜像到 Snapshot |
| UI | P3.1 不强求 UI 展示玩家中毒，第一阶段继续靠 Toast `StatusApplied` 事件观察 |

### 3.3 穿透护盾实现

`PoisonResolver::ResolvePoisonForAllHosts` 直接扣 `CurrentHp`（`State.PlayerCurrentHp` 或 `Part.CurrentHp`），**不走 `EffectExecutor::ApplyDamage`**（那个会先扣 Shield）。发 `DamageDealt` 事件时 `Amount` = 实际扣血，`Tag = Status.Poison` 标明来源。

### 3.4 调用点顺序

打牌流程（在 `PlayCardResolver::Resolve` 里）：

```
现有：
  ...效果结算 / 完美释放 / 先机推进 / 卡牌去向 / AfterPlayed 被动...
  敌方部位行动子流程（若先机归零）
  战斗结束判断

新增 P3.1 调用点：
  ...效果结算 / 完美释放 / 先机推进 / 卡牌去向 / AfterPlayed 被动...
  + FPoisonResolver::ResolvePoisonForAllHosts    ← 插在这里
  敌方部位行动子流程（若先机归零）
  战斗结束判断
```

敌方部位行动流程（在 `EnemyPartActionResolver::ActOnce` 末尾）：

```
  现有：执行意图 or 跳过 → 刷新意图 → 重置先机
  + FPoisonResolver::ResolvePoisonForAllHosts    ← 每个部位行动后
```

### 3.5 死亡链式判断

中毒可能使玩家 HP 归零或部位 HP 归零。两种情况都需要：
- 部位 HP 归零 → `bDestroyed = true` + `CurrentInitiative = 0` + 发 `EnemyPartHpEmptied`
- 玩家 HP 归零 → 由外层 `CheckAndApplyBattleEnd` 统一处理

已存在 `EffectExecutor::ApplyDamageToPart` 里的"HP 归零立即破坏"逻辑应该复用（抽成 helper）。

### 3.6 不做

- 治疗移除 10% 中毒层数（治疗效果第一阶段还没实现，延后）
- 中毒触发暮蛉被动（P3.5 做）

## 4. P3.2：保留关键字

### 4.1 规则（对齐 Battle_Rules §8, §15）

- 普通卡拥有 `Card.Keyword.Retain` → 回合结束时不进弃牌
- 锚点自带保留（已实现）
- **虫妹专属**：左右手都在手牌时，双手区所有普通卡本回合结束后保留（对齐 `Hand_Zone_Rules §7`）

### 4.2 代码改动

| 文件 | 改动 |
| --- | --- |
| `WacomBattle/Private/Core/BattleTurnFlow.cpp` | 在新增的 `EndPlayerTurn` 里实现"回合结束时普通卡处理"：非 Retain 非锚点且不在双手区保留条件下 → 移到弃牌区 |
| `WacomBattle/Private/Commands/EndTurnResolver.cpp` | `Resolve` 在触发敌方部位行动前调 `EndPlayerTurn` 的"弃牌"阶段 |
| `WacomBattle/Private/Hand/HandZoneService.cpp` | 加 `ShouldRetainCardAtTurnEnd(state, cardId)` 查询 helper |

### 4.3 判定逻辑（伪代码）

```cpp
bool ShouldRetainCardAtTurnEnd(state, cardId)
{
    if (IsHandAnchor(cardId)) return true;      // 锚点自带保留
    const FRuntimeCardInstance& Card = ...;
    if (Card.Definition->Keywords.HasTag(Retain)) return true;
    if (Card.TemporaryKeywords.HasTag(Retain)) return true;

    // 虫妹专属：左右手都在 + 此卡在双手区
    if (state.LeftHandInstanceId is in Hand && state.RightHandInstanceId is in Hand
        && GetZoneOf(state, cardId) == Both)
    {
        return true;
    }
    return false;
}
```

### 4.4 回合结束阶段顺序

对齐 `Battle_Rules §12`：

```
结束阶段开始
-> 结算"回合结束时"类效果（P3.3/P5 之后扩展）
-> [新增 P3.2] 普通卡处理：非保留的普通卡 → 弃牌区
-> 敌方部位行动子流程（所有存活可行动部位，每次行动后中毒结算）
-> 检查战斗结束
-> 下一回合 BeginPlayerTurn
```

### 4.5 "双手区专属保留"的硬编码问题

当前规则把它定义为虫妹专属，但代码里不区分角色——直接对所有角色生效。

**临时决定**：P3.2 先不区分角色（反正第一阶段只有虫妹）。`Phase2_Temporary_Decisions.md` 已经记录了这条，第二阶段不正式化。

## 5. P3.3：ZoneHook 消费

### 5.1 规则（对齐 Data_Schema_Draft §5.4 + BugGirl §5 朝光暮蝶）

朝光暮蝶的两个 ZoneHook：

| Zone | Trigger | 效果 |
| --- | --- | --- |
| `HandZone.Left` | `OnPerfectReleaseHit` | 本次打出不推进敌方先机（但仍触发完美释放判定） |
| `HandZone.Right` | `OnPlay` | 被本卡腾挪的卡 `RuntimeCost -1`；本卡 `RuntimeCost +1`。可叠加 |

### 5.2 `OnPerfectReleaseHit`：左手区完美释放不推先机

触发条件（和 `InitiativeHit` 共用窗口）：
- 卡在左手区
- 至少一个部位被先机命中

效果：设置一个"本次打牌不推进先机"标记。

### 5.3 `OnPlay`：费用转移用通用 AddCost / ReduceCost 效果

引入两个通用的卡牌操作 EffectType：

| EffectType | 含义 | Target 取值 |
|---|---|---|
| `Effect.Card.AddCost` | 目标卡 `RuntimeCostModifier += Magnitude` | `Target.Self`（本卡） / `Target.LastShuffledCard`（最近一次 Shuffle 的产物） |
| `Effect.Card.ReduceCost` | `RuntimeCostModifier -= Magnitude`（不夹到 0 以下，`ComputeRuntimeCost` 已经 clamp） | 同上 |

`FEffectContext` 新增字段 `FGuid LastShuffledCardId`。`EffectExecutor` 的 Shuffle 三个分支执行成功后把被移动的卡 ID 写入这个字段，供后续 `AddCost / ReduceCost` 读取。

朝光暮蝶右手区 ZoneHook 的 `ExtraEffects` 配成：

```
[0] Shuffle.Random           Target.RandomHandCard   → 腾挪一张，写入 LastShuffledCardId
[1] Card.ReduceCost  Mag=1   Target.LastShuffledCard → 被腾挪卡 -1 Cost
[2] Card.AddCost     Mag=1   Target.Self             → 本卡 +1 Cost（可叠加）
```

**这样整个费用转移是数据驱动的，不是 C++ hardcode**。未来任何"对手牌减/加费"的卡牌都可以配置 `FCardEffect` 组合，不改代码。

### 5.4 代码改动

| 文件 | 改动 |
| --- | --- |
| `WacomCore/Public/Tags/WacomGameplayTags.h/.cpp` | 新增 tag：`Effect.Card.AddCost` / `Effect.Card.ReduceCost` / `Target.LastShuffledCard` |
| `Data_Schema_Draft.md §2` | 同步 tag 列表 |
| `WacomBattle/Private/Effects/EffectContext.h` | 加 `FGuid LastShuffledCardId` 字段 |
| `WacomBattle/Private/Effects/EffectExecutor.cpp` | Shuffle 三个分支执行后写入 `Ctx.LastShuffledCardId`；新增 AddCost / ReduceCost 两个分支（读 Target，直接改 `FRuntimeCardInstance::RuntimeCostModifier`） |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 加 helper `RunOnPlayZoneHooks`，在"打牌事件发射之后、主效果结算之前"调用。命中 Zone + Trigger 就把 `Hook.ExtraEffects` 逐条交给 `ExecuteCardEffectOnce`（`ExecuteCardEffectOnce` 已经走 `FillTargetFromCardEffect` 映射） |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 加 helper `CheckZoneHooks_OnPerfectReleaseHit`，设置 `bSkipInitiativePushByZoneHook` |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 先机推进条件：`!bSwift && !bSkipInitiativePushByZoneHook` |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | `FillTargetFromCardEffect` 加 `Target.LastShuffledCard` 分支：返回 `TargetKind = HandCard`、`TargetInstanceId = Ctx.LastShuffledCardId` |
| `WacomEditor/Private/ContentBuilders/BugGirlBuilder.cpp` | 朝光暮蝶 `ZoneHook[Right][OnPlay]` 的 ExtraEffects 配上面三条；重跑 commandlet 更新 DataAsset |

### 5.5 执行顺序

`PlayCardResolver::Resolve` 里打牌流程：

```
打牌事件发射
→ 新增：RunOnPlayZoneHooks（按 Zone 匹配 + Trigger = OnPlay）
    ├── 可能触发 Shuffle：此时 LastShuffledCardId 写入
    ├── 可能触发 ReduceCost：读 LastShuffledCardId，对被腾挪卡减费
    └── 可能触发 AddCost：对本卡加费
→ 先机命中判断 → 抵抗判定
→ 新增：CheckZoneHooks_OnPerfectReleaseHit（若命中 → 设 bSkipInitiativePushByZoneHook）
→ 主效果结算（朝光暮蝶的 Effects：Shuffle.Random + ApplyPoison(FromRuntimeCost)）
    注意：这里的 Shuffle 也会写 LastShuffledCardId，但 ZoneHook 已经处理完了，不再读取
→ 完美释放
→ HP 归零处理
→ 先机推进（若 !bSwift && !bSkipInitiativePushByZoneHook）
→ 卡牌去向
→ AfterPlayed 被动
→ 敌方部位行动子流程
→ 战斗结束判断
```

### 5.6 验证

- 朝光暮蝶在左手区打出 + 命中部位先机 → 敌方部位 CurrentInitiative 不变、完美释放效果仍执行
- 朝光暮蝶在右手区打出 → 某张普通卡 `RuntimeCostModifier -= 1`、本卡 `RuntimeCostModifier += 1`
- 同一场战斗连续在右手区打两次朝光暮蝶 → 本卡累计 +2 Cost（验证"可叠加"）
- 朝光暮蝶在双手区打出 → 两个 Hook 都不触发

## 6. P3.4：OnCompanionCount 被动（拂晓飞蛾）

### 6.1 规则

- 每场战斗内玩家打出 `Card.Keyword.Companion` 卡牌累计 3 张 → 拂晓飞蛾（如果不在手牌区）从抽牌堆/弃牌堆/消耗区/Limbo 中回到手牌
- 触发后计数清零

### 6.2 代码改动

| 文件 | 改动 |
| --- | --- |
| `WacomBattle/Private/Core/BattleState.h` | 加 `int32 CompanionPlayedCount = 0` |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 打牌成功后（AfterPlayed 之前）：若卡有 `Companion` 关键字 → `State.CompanionPlayedCount++` |
| `WacomBattle/Private/Commands/PlayCardResolver.cpp` | 新 helper `RunOnCompanionCountPassives(state, events)`：遍历 `State.AllCards`，对拥有 `OnCompanionCount` 被动的卡（Def 层查），判断 `State.CompanionPlayedCount >= Passive.TriggerThreshold`：满足 → 把卡移到手牌 + 清零计数 + 发 `HandZoneChanged` 事件 |
| 调用点 | `PlayCardResolver` 的 AfterPlayed 被动之后 |

### 6.3 "从非手牌区回到手牌"的位置

触发时：
- 如果卡在 `DrawPile` / `DiscardPile` / `ExhaustPile` / `Limbo`：移除 + 加到 `State.Hand` 末尾 + 更新 `Location = Hand`
- 如果卡已经在 `Hand`：不触发（Passive 要求"不在手牌区时"）

### 6.4 手牌满时

对齐 `BugGirl.md` 未决问题。临时决定：**超过普通卡上限 10 时，强制加入（不检查上限），本回合这张卡作为超出项**，下次 TurnStart 的 `EnforceNormalCardLimit` 会处理多余的普通卡。

写进 `Phase2_Temporary_Decisions.md`。

## 7. P3.5：OnTwilightTriggered 被动（暮蛉）

### 7.1 规则

- 暮蛉 Passive：当"触发暮气"时，随机选一张拥有中毒效果的卡，层数 +1
- 随机范围：手牌 / 抽牌堆 / 弃牌堆

### 7.2 阻碍：暮气触发点未定义

`Battle_Rules §16` 把"暮气的具体触发点"列为未决项。

### 7.3 临时实现

P3.5 采用最简占位：**"暮气触发"定义为"给敌方部位施加 `Status.Twilight` 时"**。每次 `EffectExecutor` 应用 `Effect.ApplyStatus.Twilight` 成功后，触发 OnTwilightTriggered 的所有被动。

### 7.4 代码改动

| 文件 | 改动 |
| --- | --- |
| `WacomBattle/Private/Effects/EffectExecutor.cpp` | Twilight 分支在施加成功后调 `FPassiveDispatch::OnTwilightTriggered(state, events)` |
| `WacomBattle/Private/Core/PassiveDispatch.h/.cpp`（新） | `OnTwilightTriggered(state, events)`：遍历 `State.AllCards`，对所有 `OnTwilightTriggered` 被动（Def 层）触发：从手牌/抽牌堆/弃牌堆里随机选一张有"包含 Poison 施加效果"的卡 → 该卡 `TemporaryKeywords` 或额外字段记录"层数 +1"；或更简单：在该卡的 `Effects[i]`（EffectType == ApplyStatus.Poison）的 Magnitude 加 1 |

### 7.5 "中毒卡效果 +1" 的实现难点

`FCardEffect::Magnitude` 是 DataAsset 字段，不能改。需要 **运行时 Modifier**。

**方案**：`FRuntimeCardInstance` 加一个 `TMap<FGameplayTag, int32> EffectMagnitudeModifiers`（或 `TArray<FEffectMagnitudeMod>`）。`ExecuteCardEffectOnce` 里 `FinalMag = Effect.Magnitude + Card.Modifier[Effect.EffectType]`。

这个改动影响面较大。**临时方案**：P3.5 只做**事件发射**（发 `StatusApplied` 或新加 `PassiveTriggered` 事件），暂不真正改中毒层数。`Phase2_Temporary_Decisions.md` 标注未正式化。

## 8. P3.6：自动化测试

补充的测试清单：

| 测试 | 验证 |
| --- | --- |
| `Wacom.Battle.Poison.TickOnCardPlay` | 打出一张牌后，敌方中毒部位扣血 = 层数 |
| `Wacom.Battle.Poison.TickOnEnemyAct` | 敌方部位行动后对玩家扣血（玩家中毒时） |
| `Wacom.Battle.Poison.PenetratesShield` | Shield > 0 时中毒仍扣 HP |
| `Wacom.Battle.Poison.StacksUnchanged` | 结算后层数不减 |
| `Wacom.Battle.Retain.NormalCardRetainKeeps` | 普通卡带 Retain → 回合结束不进弃牌 |
| `Wacom.Battle.Retain.NormalCardNoRetainDiscards` | 普通卡无 Retain → 进弃牌 |
| `Wacom.Battle.Retain.BothZoneKeepsWhenAnchorsPresent` | 左右手都在 → 双手区普通卡保留 |
| `Wacom.Battle.Retain.BothZoneDiscardsWhenAnchorMissing` | 只有一张锚点 → 双手区普通卡进弃牌（双手区失效时不保留） |
| `Wacom.Battle.ZoneHook.LeftHitSkipsInitiativePush` | 朝光暮蝶在左手区 + 先机命中 → 敌方部位先机不推进 |
| `Wacom.Battle.ZoneHook.RightPlayTransfersCost` | 朝光暮蝶在右手区 + OnPlay → 被腾挪卡 -1 Cost、本卡 +1 Cost |
| `Wacom.Battle.ZoneHook.RightPlayCostAccumulates` | 连续两次右手区朝光暮蝶 → 本卡累计 +2 Cost |
| `Wacom.Battle.Effect.AddCostWorksOnSelf` | 通用 AddCost 效果对 Target.Self 作用正确 |
| `Wacom.Battle.Effect.ReduceCostClampsAtZero` | ReduceCost 导致 modifier 降得很低时，`ComputeRuntimeCost` 仍 >= 0 |
| `Wacom.Battle.Passive.CompanionCountTriggersReturn` | 打 3 张 Companion 后拂晓飞蛾回手 |
| `Wacom.Battle.Passive.CompanionCountResetsAfterTrigger` | 触发后计数清零 |
| （P3.5 暂不测，占位） | — |

## 9. 改动文件总览

```
新增：
  WacomBattle/Private/Status/PoisonResolver.h/.cpp
  WacomBattle/Private/Core/PassiveDispatch.h/.cpp
  WacomTests/Private/Battle/PoisonSpec.cpp
  WacomTests/Private/Battle/RetainSpec.cpp
  WacomTests/Private/Battle/ZoneHookSpec.cpp
  WacomTests/Private/Battle/CompanionCountSpec.cpp

修改：
  WacomBattle/Public/Runtime/RuntimeCardInstance.h      # (可能) EffectMagnitudeModifiers 字段
  WacomBattle/Public/Snapshots/BattleSnapshot.h         # FPlayerSnapshot.Statuses/StatusStacks
  WacomBattle/Private/Core/BattleState.h                # PlayerStatuses, PlayerStatusStacks, CompanionPlayedCount
  WacomBattle/Private/Snapshots/BattleSnapshotBuilder.cpp  # 镜像玩家状态
  WacomBattle/Private/Effects/EffectExecutor.cpp        # 玩家中毒写 State + Twilight 触发 OnTwilightTriggered
  WacomBattle/Private/Commands/PlayCardResolver.cpp     # ZoneHook 处理 + 中毒结算调用 + Companion 计数 + OnCompanionCount 触发
  WacomBattle/Private/Commands/EndTurnResolver.cpp      # 回合结束弃牌处理（Retain）
  WacomBattle/Private/Enemy/EnemyPartActionResolver.cpp # 部位行动后中毒结算
  WacomBattle/Private/Hand/HandZoneService.cpp          # ShouldRetainCardAtTurnEnd helper
  WacomBattle/Private/Core/BattleTurnFlow.cpp           # （可能）统一的 EndPlayerTurn helper
```

## 10. 验收标准

P3 全部完成时：
- 虫妹所有 7 张最小卡组的效果**在代码层面不再有"只记层数不生效"的状态**（OnTwilightTriggered 除外，它等规则定义）
- `Battle_Rules.md §15` 中毒/保留/ZoneHook 三条主干在代码里真实生效
- 13 条原有测试继续全绿 + 11 条新增测试全绿
- PIE 实战一场：中毒敌人每次行动后扣 HP 可见、保留手牌在回合结束保留、朝光暮蝶左手区打出可验证"敌方先机不推进"

## 11. 风险和开放问题

1. **暮气触发点**：`Battle_Rules §16` 未决。P3.5 暂用"状态施加时即触发"占位。
2. **中毒效果层数改动**：`EffectMagnitudeModifiers` 这个字段不做引入，等 P3.5 正式化时一并决定。
3. **手牌满时回手处理**：P3.4 采用"强行加入 + 下回合起始阶段处理"，临时决定记文档。
4. **玩家中毒 UI 展示**：P3.1 只做 Snapshot 层，UI 显示留 P5 的事件反馈增强。
5. **AddCost/ReduceCost 溢出**：P3.3 引入的 `RuntimeCostModifier` 累加没有上下限，Cost 可能变成极大/极小。`ComputeRuntimeCost` 已 clamp 到 `[0, ...)`，上限暂不限制。后续若出现平衡问题再加。

## 12. 实施顺序与编译验证节奏

每个子切片结束**编译 + 跑自动化测试**：

```
P3.1 → 编译 → 跑现有 13 条测试（不破原有）→ 加 PoisonSpec 4 条
P3.2 → 编译 → 跑现有 + PoisonSpec → 加 RetainSpec 4 条
P3.3 → 编译 → 跑现有 + Retain → 加 ZoneHookSpec 2 条
P3.4 → 编译 → 跑现有 + ZoneHook → 加 CompanionCountSpec 2 条
P3.5 → 编译 → 跑现有（P3.5 占位无测试）
P3.6 → 跑全部 24 条测试
```
