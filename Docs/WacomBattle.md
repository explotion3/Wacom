# WacomBattle 模块文档

> 本文是 WacomBattle 模块的规则 + 实现文档。规则变更时先改本文，再改代码。

---

## §1 模块职责

WacomBattle 是**战斗内核**，负责单场战斗的唯一规则真相。

**负责**：
- 战斗生命周期管理
- 命令结算（PlayCard / Wait / EndTurn）
- 手牌区域规则
- 卡牌效果执行
- 敌方部位行动
- 快照和事件输出

**不负责**：
- UI 展示
- Run 探索
- 关卡交互
- 存档

**边界约束**：UI、Actor 和测试入口都不应该直接改 `BattleState`。它们只能提交命令，读取快照和事件。

---

## §2 核心对象

| 对象 | 职责 |
|---|---|
| `UBattleSession` | 一场战斗的生命周期入口（Public，对外唯一接口）|
| `BattleState` | 当前战斗状态（Private，外部不可见）|
| `BattleResolver` | 统一结算命令的调度器 |
| `FBattleSnapshot` | 给 UI 读取的只读状态快照 |
| `FBattleEvent` | 给 UI、日志和测试读取的事件流 |
| `FBattleCommand` | 玩家或系统输入的操作命令 |
| `CardEffectDispatcher` | 卡牌效果分发（Target 映射 + 条件评估 + Magnitude 计算 + 执行）|
| `EffectExecutor` | 效果注册制执行器（按 EffectTag → Handler）|
| `MagnitudeResolver` | Magnitude 计算注册制（Literal / RuntimeCost / 扩展）|
| `ConditionResolver` | 效果/被动条件评估注册制 |
| `InitiativeResolver` | 先机命中 / 抵抗 / 完美释放 |
| `ZoneHookResolver` | ZoneHook 消费（OnPlay / OnPerfectReleaseHit）|
| `PassiveDispatcher` | 被动触发调度（AfterPlayed / OnCompanionCount / OnTwilightTriggered）|
| `PoisonResolver` | 中毒结算 |
| `HandZoneService` | 手牌区域和腾挪规则 |
| `EnemyPartActionResolver` | 敌方部位行动子流程 |

**目录结构**：

```
WacomBattle/
├── Public/
│   ├── Session/BattleSession.h
│   ├── Commands/BattleCommand.h
│   ├── Snapshots/{BattleSnapshot.h, HandSnapshot.h, EnemySnapshot.h}
│   ├── Events/BattleEvent.h
│   └── Runtime/{RuntimeCardInstance.h, RuntimeEnemyPart.h, RuntimeStatus.h}
└── Private/
    ├── Session/, Core/, Commands/, Deck/, Hand/
    ├── Enemy/, Status/, Events/, Snapshots/
    ├── Effects/, Resolution/, Passives/
    └── ...
```

---

## §3 战斗流程

### 回合结构

```
敌人初始化 → 起始阶段 → 执行阶段 → 结束阶段 → 若战斗未结束，回到起始阶段
```

### 16 步流程概要（PlayCardResolver::Resolve）

1. 发射 `CardPlayed` 事件
2. 执行 ZoneHook（OnPlay）：若本卡在某区域且该区域有 OnPlay Hook，执行 Hook 的 ExtraEffects
3. 记录所有存活敌方部位的出牌前当前先机
4. 判断先机命中的部位（RuntimeCost == 部位出牌前先机）
5. 对先机命中的部位执行抵抗判定（先于完美释放）
6. 执行卡牌主动效果（伤害可能使部位 HP 归零，立即进入破坏态）
7. 执行完美释放效果（迅捷卡跳过；主效果致死的部位不参与）
8. 先机推进：非迅捷卡且未被 ZoneHook（OnPerfectReleaseHit）跳过时，所有未破坏部位当前先机 -= RuntimeCost
9. 执行卡牌离开手牌后的去向规则
10. Companion 计数累加：若本卡有 Companion 关键字，全局计数 +1
11. 执行 AfterPlayed 被动（如烁光蝶自腾挪）
12. 执行 OnCompanionCount 被动（如拂晓飞蛾回手）
13. 中毒结算：对敌我双方各结算一次
14. 若有部位当前先机 ≤ 0，执行敌方部位行动子流程
15. 检查战斗结束（敌人全破 / 玩家 HP ≤ 0）
16. 若未结束，递增 StateVersion，返回执行阶段

### 起始阶段流程

```
起始阶段开始
→ 结算"战斗开始时"类效果
→ 结算"直到回合开始"类效果终止
→ 结算"回合开始时"类效果
→ 当前等待值重置为 2
→ 判断左手牌和右手牌是否在手牌区
→ 按左右手牌状态生成本回合手牌队列
→ 超出普通手牌上限的普通卡牌移至弃牌区
→ 完成手牌队列
→ 起始阶段结束
```

### 结束阶段流程

```
结束阶段开始
→ 结算"回合结束时"类效果
→ 结算"直到回合结束"类效果终止
→ 判断敌方全部或我方生命值归零
→ 执行敌方部位行动子流程（所有存活且可行动的部位）
→ 判断战斗结束
→ 若未结束，回到起始阶段
```

### 等待流程

```
执行等待
→ 所有敌人部位当前先机减少当前等待值
→ 判断是否有部位先机 ≤ 0
→ 若有，执行敌方部位行动子流程
→ 当前等待值 +1
→ 返回执行阶段
```

---

## §4 手牌区域规则

### 区域结构

```
[左手区] [左手牌] [双手区] [右手牌] [右手区]
```

左手牌和右手牌是手牌区域锚点。普通卡牌的位置根据它们相对左右手牌的位置，被判定为左手区、双手区或右手区。

### 左手牌与右手牌

- 是卡牌，可以被打出
- 是手牌区域锚点
- 自带保留效果
- 不会进入抽牌堆、弃牌堆、消耗区
- 被打出后本回合离开手牌，不进入任何卡牌区域（Limbo）
- 回合结束时仍在手牌区则保留到下一回合

### 抽牌规则

- 回合开始固定从抽牌堆抽取 5 张普通卡牌
- **左右手牌都不在手牌区**：预备队列 + 插入左右手牌（两者之间至少一张普通卡牌）
- **左右手牌都在手牌区**：不移动已有卡牌，新抽卡随机插入当前手牌区
- **只有一张锚点在手牌区**：按"都不在"的规则重新生成

### 手牌上限

- 普通卡牌上限 10，不计算左手牌和右手牌
- 最终手牌队列上限 12
- 超出上限的普通卡牌移动到弃牌区

### 腾挪

- 重新放置当前手牌中的卡牌，使其进入不同区域或在同一区域内改变位置
- 默认范围：当前手牌中的普通卡牌（不选择锚点）
- 腾挪到目标区域时，插入随机位置
- 随机目标区域等于当前区域时仍发生区域内重新放置
- "从双手区腾挪到其他区域"：目标排除双手区

### 双手区失效

- 左手牌或右手牌离开手牌区时，双手区立刻失效
- 原双手区卡牌仍在手牌中，Zone = None
- 腾挪可用区域缩减为仅 Left
- 左右手牌都不在时，所有普通卡牌 Zone = None，腾挪不可用

### 单锚点语义

实现按"两锚点在队列中的位置"切三段，不区分哪个是 LeftHandInstanceId 哪个是 RightHandInstanceId。即"位置较小的锚点左侧 = Left 区，中间 = Both 区，位置较大的锚点右侧 = Right 区"。第一阶段锚点不被腾挪，物理方向不会颠倒。

---

## §5 先机与完美释放

### Cost 扣减

- 非迅捷卡打出后，所有未破坏部位当前先机 -= RuntimeCost
- 迅捷卡不扣减先机
- 被 ZoneHook（OnPerfectReleaseHit）标记跳过时不扣减

### 完美释放判定

```
出牌前记录所有存活部位的 CurrentInitiative
若 CardRuntimeCost == Part.CurrentInitiativeBeforePlay → 该 Part 命中 PerfectRelease
```

- 多个部位可同时命中
- 迅捷卡不触发完美释放
- 主效果致死的部位不参与完美释放

### 抵抗

- 先机命中时触发（先于完美释放）
- 卡牌抵抗值：主效果中首个 `Effect.Damage` 的 FinalMagnitude；无伤害效果则为 0
- 意图抵抗值：`FIntentDefinition::ResistanceValue`；非攻击意图填 0
- `if CardResistance > IntentResistance → 该部位进入 Status.Stunned`
- 抵抗不改变伤害、不改变先机、不阻止先机推进

---

## §6 中毒

### 触发时机

- 玩家每打出一张牌后，对敌我双方各结算一次当前中毒层数伤害
- 敌方部位每行动一次后，对敌我双方各结算一次当前中毒层数伤害

### 结算方式

- 对拥有中毒的一方造成等于当前中毒层数的伤害
- **穿透护盾**，直接扣除生命值
- 层数不因结算而减少（持续存在直到被治疗或其他效果移除）

### 层数衰减

- 被治疗时，移除治疗效果 10% 的中毒层数（向下取整）
- 第一阶段未实现

### 归属

- 敌人有多个部位时，每个部位可以各自拥有中毒
- 玩家本体也可以拥有中毒

---

## §7 保留

### Retain 标签

- 拥有 `Card.Keyword.Retain` 的卡牌回合结束时不弃掉
- 左手牌和右手牌自带保留

### 区域条件

- 虫妹的双手区可以保留手牌：当回合结束时左右手牌都仍在手牌区，双手区中的普通卡牌可以保留到下一回合
- 被保留的普通卡牌在下回合生成手牌队列时保持原有相对位置
- 保留牌如果被主动打出，仍按普通打出处理（进入弃牌区），除非卡牌文本另有说明

---

## §8 ZoneHook

### OnPlay

- 触发时机：本卡打出时（CardPlayed 事件之后、"记录出牌前先机"之前）
- 若本卡在某区域且该区域有 OnPlay Hook，执行 Hook 的 ExtraEffects
- 典型卡：朝光暮蝶右手区（费用转移）

### OnPerfectReleaseHit

- 触发时机：本卡完美释放命中时
- 不执行 ExtraEffects，仅作为"跳过先机推进"标记
- 典型卡：朝光暮蝶左手区（不推先机）

### 触发顺序

1. `OnPlay` 发生在 `CardPlayed` 事件之后、"记录出牌前先机"之前
2. `OnPerfectReleaseHit` 的判定紧跟在抵抗判定之后，作为先机推进的 skip 条件
3. 不同区域的 Hook 不会同时命中（卡只在一个区域）

---

## §9 被动系统

### OnCompanionCount

- 触发时机：全局 Companion 计数达阈值
- 典型卡：拂晓飞蛾（阈值 3，回手）
- 触发后计数清零
- 当前实现：触发时超手牌上限强行加入，下回合 EnforceLimit 处理

### OnTwilightTriggered

- 触发时机：暮气施加成功时
- 典型卡：暮蛉（使一张中毒卡牌效果 +1）
- 第一阶段只发 `PassiveTriggered` 事件，不改 Magnitude

### AfterPlayed

- 触发时机：本卡打出完成后（卡牌去向之后）
- 典型卡：烁光蝶（自腾挪到随机区域）

---

## §10 效果执行器

### Registry 模式

效果执行器按 `EffectTag → Handler` 注册制分派。新增效果类型只需注册新 Handler，不改调度逻辑。

### EffectTag → Handler 清单

| EffectTag | Handler | 现状 |
|---|---|---|
| `Effect.Damage` | DamageHandler | 已实现 |
| `Effect.Heal` | HealHandler | 预留，未实现 |
| `Effect.ApplyStatus.Poison` | ApplyStatusHandler | 已实现 |
| `Effect.ApplyStatus.Slow` | ApplyStatusHandler | 占位（仅记录层数）|
| `Effect.ApplyStatus.Freeze` | ApplyStatusHandler | 占位（行动时跳过）|
| `Effect.ApplyStatus.Twilight` | ApplyStatusHandler | 占位（仅记录层数）|
| `Effect.Shuffle.Random` | ShuffleHandler | 已实现 |
| `Effect.Shuffle.FromBothToOther` | ShuffleHandler | 已实现 |
| `Effect.Shuffle.ToRandomZone` | ShuffleHandler | 已实现 |
| `Effect.Card.AddCost` | CostModHandler | 已实现 |
| `Effect.Card.ReduceCost` | CostModHandler | 已实现 |
| `Status.Shield` | ShieldHandler | 已实现 |

---

## §11 敌方部位行动子流程

### 触发来源

- 等待使部位先机 ≤ 0
- 打牌推进先机后部位先机 ≤ 0
- 结束阶段（所有存活且可行动的部位）
- 其他卡牌/状态/事件强制行动

### 子流程

```
收集本次需要行动的部位
→ 移除已破坏部位
→ 移除不能行动部位
→ 按部位顺序逐个结算：
    → 若部位处于晕厥，执行晕厥处理并跳过当前意图
    → 否则执行该部位当前意图
    → 刷新该部位意图
    → 将当前先机设置为新意图先机
    → 中毒结算（对敌我双方各结算一次）
    → 检查玩家失败、部位破坏、敌人死亡、战斗结束
```

### 晕厥处理

- 晕厥以层数模型记录
- 每次该部位行动（无论执行意图还是因晕厥跳过意图）都消耗 1 层
- 层数归零时晕厥状态移除
- 冻结状态第一阶段共享同一"跳过意图 + 消耗 1 层"分支

### 行动后刷新

无论部位是执行意图还是因晕厥跳过意图，只要完成本次行动结算，就刷新到下一个意图，并将当前先机设置为新意图的先机值。

---

## §12 BattleState 结构

### 概览

BattleState 是战斗内核的可变状态，位于 `WacomBattle/Private/`，外部模块不可见。

### 主要组成

```
BattleState
├── Meta
│   ├── Phase（当前阶段）
│   ├── TurnNumber
│   ├── CurrentWaitValue（每回合重置为 2）
│   ├── StateVersion（每次命令后递增）
│   ├── Rng（FRandomStream，确定性随机）
│   └── CompanionPlayedCount（全局伙伴计数）
├── PlayerState
│   ├── CurrentHp / MaxHp
│   ├── Shield
│   ├── StatusStacks（TMap<FGameplayTag, int32>）
│   └── LeftHandInstanceId / RightHandInstanceId
├── CardContainers
│   ├── Hand（手牌队列，有序）
│   ├── DrawPile（抽牌堆）
│   ├── DiscardPile（弃牌堆）
│   ├── ExhaustPile（消耗区）
│   └── Limbo（左右手牌打出后暂存）
└── EnemyState
    ├── EnemyDefinition
    └── Parts[]（FRuntimeEnemyPart）
        ├── InstanceId / Def
        ├── CurrentHp / Shield
        ├── CurrentIntentIndex / CurrentInitiative
        ├── bDestroyed
        └── StatusStacks
```

### 卡牌离开手牌后的去向

| 条件 | 去向 |
|---|---|
| 普通牌 | 弃牌区 |
| 消耗牌 | 消耗区 |
| 左手牌/右手牌 | Limbo（不入任何区域）|
| 连击牌 | 留在原位置（不离开手牌）|
| 保留牌打出 | 弃牌区（保留只影响回合结束弃牌）|
