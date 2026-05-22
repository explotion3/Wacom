---
type: domain-spec
scope: wacom-run
status: active
updated: 2026-05-22
tags:
  - wacom/run
  - wacom/rules
  - wacom/exploration
  - wacom/inventory
---

# WacomRun 模块文档

> [!info] 本文职责
> 本文是 Run 领域规则真相与关键实现入口。字段细节以代码为准，本文不维护完整 API / UPROPERTY 镜像。

> [!warning] 模块边界
> `WacomRun` 是战斗外规则层。战斗内牌局规则看 [[WacomBattle]]，静态内容资产看 [[WacomData]]，UI 展示看 [[WacomUI]]。

## §1 模块定位

`WacomRun` 负责一次冒险中战斗外的规则状态。它持有 Run 状态，给战斗构造初始化参数，并接收战斗结束包做战外结算。

它不负责战斗内牌局规则，不负责 UI 展示，不定义静态内容资产。UI 应读取 Snapshot / ViewModel，并把玩家意图提交给 Run 层入口。

依赖边界：

```
WacomCore / WacomData / WacomBattle  ←  WacomRun  ←  WacomApp
```

Run 层可以引用卡牌、角色、敌人和战斗回传结构；不能依赖 UI Widget 或场景表现细节。

---

## §2 Run 生命周期与失败

一次 Run 由 `URunSession` 表示。它是 Transient UObject，由 PlayerController 创建并持有，核心数据在 `FRunState`。

新 Run 初始化时读取角色的手指数与每指 HP，并按 StarterDeck 建立卡牌 instance。非容器卡默认进备战区，容器卡默认进背包；当前原型特例 `MuseiYinchongdeng`（暮色引虫灯）默认进备战区。

Run 失败条件有三类：

| 来源 | 条件 |
|---|---|
| 战内 | 战斗结果为 Defeat，`bRunActive = false` |
| 战外压力 | 八条压力总和 `>= 100` |
| 手指 | `FingerCount <= 0` |

`bRunActive` 只记录显式 Run 活跃状态；压力满和手指耗尽通过 `IsRunFailed()` 综合判定。

### 经验与占位技能

经验来自战斗击倒结算。战斗结果为 Victory 时，包括撤离，会把 `FBattleResultPacket.KnockdownExpGains[]` 累加到 Run；Defeat / Undetermined 不结算经验。

`URunSession::AddExperience()` 会把经验累加到 `ExperienceCurrent`。当经验达到 `ExperienceCapacity` 时，循环扣减容量并向 `AcquiredSkills` 追加 `SkillSlot.Placeholder`。

当前技能只是占位计数，不挂实际效果。正式技能系统上线前，不要让战斗规则依赖 `SkillSlot.Placeholder`。

---

## §3 时间、时段与节点

一天按固定顺序推进：

```
Morning -> Day -> Dusk -> Night -> Sunrise -> Morning(次日)
```

任一时段节点数消耗到 0 时，`URunSession::ConsumeNode()` 自动调用 `AdvanceToNextPhase()`。玩家移动本身不消耗节点，节点消耗发生在事件完成或规则明确结算时。

当前初始节点数：

| 时段 | 节点数 | 设计口径 |
|---|---:|---|
| Morning | 2 | 清晨规划事件会占用其中 1 点 |
| Day | 6 | 主流程、战斗、商店、休息等 |
| Dusk | 2 | 可接野炊事件 |
| Night | 2 | 可接露营或夜间探险 |
| Sunrise | 1 | 夜探后的后置时段 |

当前 `RunSession` 已实现时段推进与压力副作用；清晨规划、野炊、露营等时段绑定事件由后续事件调度接入。

时段进入副作用：

| 进入时段 | 副作用 |
|---|---|
| Morning | 饥饿 +5 |
| Morning 且前一时段是 Sunrise | 腐朽 +5 |
| Dusk | 饥饿 +5 |
| Sunrise | 疲劳 +10 |

设计上露营会从 Night 直接进入次日 Morning，跳过 Sunrise。当前通用推进路径尚未实现这条特殊事件分支。

---

## §4 压力系统

压力是战外血量。八条压力各自为 0 到 100 的百分比，总和达到 100 时 Run 失败。

压力不改变战内规则。它可以被事件读取，用于限制选项、触发分支或驱动表现层效果。

| 压力 | 当前来源 |
|---|---|
| 饥饿 | 进入 Morning / Dusk 各 +5 |
| 伤口 | 战外右手破坏 +1；战内跨高 HP 阈值 +1；跨低 HP 阈值 +5；同归于尽 +10 |
| 疲劳 | 进入 Sunrise +10；每场非 Undetermined 战斗后 +1 |
| 负重 | 由 `BurdenZone.Num()` 计算，公式 `n*(n+1)/2`，Clamp 到 100 |
| 腐朽 | 从 Sunrise 进入次日 Morning +5 |
| 劣迹 | 第 n 次偷窃完成时 `n*(n+1)/2 + 1` |
| 嗜血 | 永久销毁 Companion 卡 +1 |
| 残疾 | 每失去 1 根手指 +5 |

HP 阈值来自 RunState，当前默认：

```
HighHpThreshold = 0.5
LowHpThreshold  = 0.2
```

战斗内只记录是否首次跨阈值；压力修改统一在战斗结束回传给 Run 层后处理。

---

## §5 背包、备战与负重

Run 背包模型按卡牌 instance 运转。每张进入 Run 的卡都有 `FCardInstance.InstanceId`，同名卡也必须作为独立 instance 管理。

当前四个物理持有区：

| Zone | 规则 |
|---|---|
| `Backpack` | 通量存放区内容，以及位于背包侧的 B 主卡 |
| `BattleDeck` | 实际进入战斗的备战卡组 |
| `SpecialZones` | 每张 B 类容器卡各自开辟一个特殊存放区 |
| `BurdenZone` | 其他区超容后的兜底区 |

同一个 `InstanceId` 同时只能位于一个 Zone。跨区移动走 `MoveInstance()`，失败路径不修改 RunState。

### 容器分类

卡牌容量来自 `CardDefinition.Physique.Capacity`。

| 分类 | 条件 | 含义 |
|---|---|---|
| 普通卡 | `Capacity == 0` | 不提供容量 |
| A 类容器 | `Capacity > 0` 且 `CapacityEffect` 为空 | 提供通量容量 |
| B 类容器 | `Capacity > 0` 且 `CapacityEffect` 有效 | 开辟自己的特殊存放区 |

`Card.Keyword.BagProvider` 是历史关键词。当前背包 UI 是否可用以“玩家是否拥有任意 `Capacity > 0` 容器卡”为准。

### 容量公式

```
通量内容容量     = Σ(玩家拥有的所有 A 类容器卡 Capacity)
备战区容量       = Σ(玩家拥有的所有容器卡 Capacity)
特殊存放区容量   = B 主卡 Capacity - 1，最小为 0
负重区容量       = 不固定
```

“玩家拥有”覆盖 `Backpack`、`BattleDeck`、`BurdenZone` 和所有 `SpecialZones.Cards`。容器卡无论位于通量、备战、负重或特殊存放区，都仍然贡献容量。

A 类容器卡物理位于 `Backpack` 时，也占用通量内容格。进入 `BattleDeck` 后不在通量区显示投影，但仍贡献通量容量。

B 主卡只能位于 `Backpack` 或 `BattleDeck`，不能进入自己的 SpecialZone，也不能进入 `BurdenZone`。B 主卡移动时，对应 SpecialZone 保留。

### 超容与负重

`RecomputeBurden()` 会先整理超容卡，再写入负重压力。

通量区超容时，普通卡和 A 类容器卡可进入 `BurdenZone`；B 主卡不会被挪入负重区。备战区超容时，卡优先回通量区，通量区接不住再进负重区。

负重压力公式：

```
n = BurdenZone.Num()
BurdenPressure = Clamp(n * (n + 1) / 2, 0, 100)
```

永久销毁容器卡会立刻停止贡献容量。若销毁后玩家不再拥有任何容器卡，则拒绝销毁。

### 永久销毁与金币

永久销毁入口用于删牌、事件交出卡、未来出售或战败丢弃。

当前统一规则由 `Private/Deck/RunDeckRules.*` 承接。历史 public API `DestroyCardFromBackpack()` 保留旧名兼容蓝图和测试，但实际会按固定顺序搜索所有玩家拥有区：`Backpack -> BattleDeck -> BurdenZone -> SpecialZones`。

当前保护规则：

- `Rarity == Intrinsic` 的卡拒绝销毁。
- 最后一张容量来源卡拒绝销毁。
- 销毁 Companion 卡会增加嗜血压力。
- 销毁 B 主卡时，它的 SpecialZone 内卡退回 Backpack；装不下则进 BurdenZone。
- 移除非容量卡后允许从负重区回填；移除容量来源卡后不做回填，只处理容量缩小导致的超容。

删牌换金币当前是简易数值：白卡 +1，蓝卡 +2。UI 拖拽删除使用 `InstanceId`，入口是 `ValidateDeleteCardForGoldByInstance()` / `DeleteCardForGoldByInstance()` / `GetDeleteGoldRewardForInstance()`；RunEvent 等资产语义仍可用 Definition 级入口表达“移除一张匹配卡”。金币是 Run 内资源，但当前不写入 SaveGame。

---

## §6 商店规则

商店运行态以场景入口的 `PersistentId` 为 key，而不是以商品资产 ID 为 key。

`AWacomShopTriggerActor.PersistentId` 传给 `URunSession::BeginShopVisit(ShopId, Offers)`。第一次打开该 `ShopId` 时，用传入 Offers 建库存；再次打开同一 `ShopId` 时保留库存和已购买状态，忽略新 Offers。

`UShopDefinition.ShopId` 是静态内容 ID。多个场景商店可以引用同一份 `UShopDefinition`，但只要 Actor `PersistentId` 不同，它们就是不同库存。

购买规则：

- 打开商店不消耗节点。
- 成功购买会扣金币、获得卡牌、标记 Offer 已购买。
- 关闭商店时，如果本次访问买过至少一件商品，统一消耗 1 节点。
- 没买东西就关闭，不消耗节点。
- `ShopId == NAME_None`、无效 Offer、重复购买、商品为空、负价格、金币不足等失败路径不修改 RunState。

当前 `ShopStates` 只保存在 Run 内存态，不写入 SaveGame。

---

## §7 探索 RunEvent 规则

RunEvent 是轻量事件图。事件内容来自 `UWacomRunEventDefinition`，运行态以场景事件 Actor 的 `PersistentId` 为 key。

`UWacomRunEventDefinition.EventId` 是内容 ID，不是运行态状态 key。同一事件定义放在多个地点时，必须给每个 Actor 配不同 `PersistentId`，状态彼此独立。

事件状态条件和 `MarkEventCompleted` 效果里的 `TargetPersistentId` 也填写场景 Actor 的 `PersistentId`，不是 `EventId`。

当前访问规则：

- `PersistentId == NAME_None` 或定义为空时拒绝打开。
- 已完成事件第一版拒绝重复打开。
- 打开事件不消耗节点。
- 只有选项 Effects 配置 `ConsumeNode` 时才消耗节点。
- 关闭事件只清 active 标记，不改变完成状态。

当前条件：

- 金币不少于指定值。
- 当前节点数不少于指定值。
- 指定压力不高于阈值。
- 拥有 / 缺少指定卡。
- 指定 `PersistentId` 事件已完成 / 未完成。

当前效果：

- 获得卡牌。
- 增减金币，最低不低于 0。
- 增减压力。
- 消耗节点并可能推进时段。
- 从玩家任意持有区永久移除一张卡。
- 标记指定 `PersistentId` 事件完成。

RunEvent 的移除卡搜索四个物理持有区：`Backpack`、`BattleDeck`、`BurdenZone` 和所有 `SpecialZones.Cards`。它不发金币，但遵守固有卡、最后容量来源卡和 Companion 嗜血规则。

`FRunEventChoiceResult` 只表达本次选项直接效果，供 UI 和日志展示。后续规则不能依赖这个结果包反向修改 RunState。

当前 `RunEventStates` 只保存在 Run 内存态，不写入 SaveGame。

---

<a id="wacomrun-battle-settlement"></a>
## §8 战斗联动与战后结算

进入战斗前，`URunSession::BuildInitParamsForBattle()` 从 RunState 构造 `FBattleInitParams`。

关键输入：

- 角色、敌人和战斗随机种子。
- HP 压力阈值 `HighHpThreshold / LowHpThreshold`。
- `BattleDeck` 中的物理卡。
- SpecialZone 中勾选入战的卡：只有 B 主卡位于 `BattleDeck`，且主卡有 `CapacityEffect`，其 SpecialZone 内 `bBattleEnabledInSpecialZone == true` 的卡才会入战，并携带主卡容量效果。
- 若传入 `TriggerPersistentId`，且 `RunState.BattleProgress` 有记录，则把已破坏部位写入 `PreDestroyedPartIds`。

战斗结束时，GameMode 先处理战斗 UI 和场景 Trigger，再调用 `OnBattleFinishedFromTrigger(Packet, EnemyDef, TriggerPersistentId)` 做 Run 结算。

Outcome 分支：

| 结果 | Run 处理 |
|---|---|
| Victory 且 `bWithdrawn == true` | 撤离；敌人不进 `DefeatedEnemies`；写 `BattleProgress[TriggerId] = DestroyedPartIds` |
| Victory 且未撤离 | 真胜利；敌人进 `DefeatedEnemies`；清理 `BattleProgress[TriggerId]` |
| Defeat | `bRunActive = false` |
| Undetermined | 不做战外结算并返回 |

战后压力与奖励：

- 任一非 Undetermined 结果：疲劳 +1。
- `bCrossedHighHpThreshold`：伤口 +1。
- `bCrossedLowHpThreshold`：伤口 +5。
- `bMutualDestruction`：伤口 +10，不直接终止 Run。
- Victory 包含撤离：结算 `KnockdownExpGains[]` 和 `GainedCards[]`。
- Defeat / Undetermined 不结算经验和获得卡。
- `KnockdownChoices[]` 当前只记日志，后续事件分支再消费。

节点消耗不在 `OnBattleFinishedFromTrigger()` 内部完成。当前 `AWacomGameMode::ExitBattle()` 在非 Undetermined 战斗结束后统一 `ConsumeNode(1)`，胜利、失败、撤离都消耗。

战斗 Trigger 的场景销毁由 GameMode 处理。真胜利会调用 `MarkTriggerDestroyed(PersistentId)` 并 Destroy Actor；撤离不销毁，允许下次重入。

---

## §9 场景 PersistentId 规则

`PersistentId` 是场景对象在 RunState 中的稳定身份。它不是显示名，也不是静态内容资产 ID。

当前已用场景 ID：

| 场景对象 | PersistentId 用途 |
|---|---|
| `ABattleTriggerActor` | 已销毁 Trigger、撤离 BattleProgress |
| `AWacomShopTriggerActor` | 商店库存与已购买状态 |
| `AWacomRunEventTriggerActor` | RunEvent 当前节点与完成状态 |

规则：

- 参与 Run 状态的场景 Actor 必须配置非空 `PersistentId`。
- 同一关卡内应保持唯一。
- `NAME_None` 表示不参与对应状态记录；入口会 Warning 或拒绝。
- 内容资产 ID 不能替代场景 PersistentId。

---

## §10 SaveGame 当前边界

先读这一条：当前 `AWacomGameMode::bSaveSystemEnabled == false`。正常游戏流程不读盘、不写盘；战斗结束和退出时的自动存档会静默 no-op。

下面的边界描述的是底层 `URunSession::SaveToSlot()` / `LoadFromSlot()` 和 `UWacomSaveGame` v2 的实际字段拷贝结果。

### v2 磁盘会保存

| SaveGame 字段 | 来源 / 说明 |
|---|---|
| `SaveVersion`、`SavedAtUtc`、`ClientBuildId` | 存档元数据，当前版本为 2 |
| `CharacterAssetPath` | 当前角色资产路径 |
| `BattleSeed` | 战斗随机种子 |
| `bRunActive` | Run 活跃状态 |
| `DefeatedEnemyAssetPaths` | 已击败敌人资产路径 |
| `DestroyedTriggerIds` | 已永久销毁的战斗 Trigger |
| `PlayerTransform`、`bHasPlayerTransform` | 探索 Pawn 位置 |
| `Backpack` | 卡牌 instance 列表 |
| `BattleDeck` | 卡牌 instance 列表 |
| `BurdenZone` | 卡牌 instance 列表 |
| `SpecialZones` | B 主卡 owner id 与区内卡牌 instance |

卡牌 instance 存档条目保存 `InstanceId`、`DefinitionAssetPath` 和 `bBattleEnabledInSpecialZone`。读档时要求 InstanceId 非零、全表唯一，Definition 能加载成功。

若 v0 / v1 旧档迁移到 v2 后四个 instance 数组全空，读档会按 Character 的 StarterDeck 重新生成 instance；新 GUID 会替代旧运行态身份。

注意：v2 会恢复 `BurdenZone` 的卡牌列表，但不会恢复或重算 `Pressure.Burden`。压力整体仍按下表属于未持久化状态，读档后为默认值。

### 当前仍是内存态

| RunState 字段 / 系统 | SaveGame v2 状态 | 读档后的实际结果 |
|---|---|---|
| `FingerCount`、`HpPerFinger` | 不保存 | 使用 `FRunState` 默认值，不从 SaveGame 还原 |
| `Pressure`、`TheftCount` | 不保存 | 压力全为 0，偷窃计数为 0 |
| `HighHpThreshold`、`LowHpThreshold` | 不保存 | 使用默认 `0.5 / 0.2` |
| `ExperienceCurrent`、`ExperienceCapacity`、`AcquiredSkills` | 不保存 | 经验为 0，上限默认 10，技能为空 |
| `CurrentDayNumber`、`CurrentTimePhase`、`RemainingNodeCount` | 不保存 | 回到第 1 天 Morning，节点为默认值 |
| 五时段初始节点数 | 不保存 | 使用默认 `2 / 6 / 2 / 2 / 1` |
| `Gold` | 不保存 | 读档后为 0 |
| `BattleProgress` | 不保存 | 撤离留下的已破坏部位不会跨磁盘读档保留 |
| `ActiveShopId`、`bShopVisitHasPurchase` | 不保存 | 无 active shop |
| `ShopStates` | 不保存 | 商店库存和已购买状态清空 |
| `ActiveRunEventId`、`ActiveRunEventDefinition` | 不保存 | 无 active event |
| `RunEventStates` | 不保存 | 事件当前节点和完成状态清空 |

因此，当前 SaveGame 不能被描述为完整 Run 存档。它只覆盖部分场景与卡牌持有状态，而且正常流程还被 GameMode 总开关禁用。

后续恢复存档系统时，必须先决定这些字段的持久化策略，并同步升级 `UWacomSaveGame::CurrentSaveVersion` 与迁移链。

---

## §11 关键实现入口

Run 领域入口集中在 `Source/WacomRun/`：

| 文件 | 作用 |
|---|---|
| `Public/RunSession.h` | Run 的命令 / 查询入口；UI 和 GameMode 不直接改 RunState |
| `Private/RunSession.cpp` | 时间、压力、商店 / RunEvent public 入口、战斗回传 public 入口、SaveGame slot IO 的协调实现 |
| `Private/Battle/RunBattleSettlementResolver.*` | 战斗结束回传包的 Run 结算流程；只操作 `FRunState` 并通过回调复用 RunSession 压力 / 经验 / 获得卡牌入口 |
| `Private/Deck/RunDeckRules.*` | 背包、备战区、SpecialZone、负重区的私有规则 helper；只操作 `FRunState`，不广播、不访问 UI |
| `Private/Events/RunEventExecutor.*` | RunEvent 事件图解释、选项条件、效果执行和结果包生成；只操作 `FRunState`，不广播、不访问 UI |
| `Private/Save/RunSaveGameSerializer.*` | `FRunState <-> UWacomSaveGame` 字段拷贝、SaveEntry 写入和读档校验；不广播、不做磁盘 IO |
| `Private/Shops/RunShopTransaction.*` | 商店访问、库存快照和购买事务的私有 helper；只操作 `FRunState`，不广播、不访问 UI |
| `Public/RunState.h` | `FRunState`、商店状态、事件状态、战斗进度快照 |
| `Public/RunStateTypes.h` | `FCardInstance`、压力枚举、时段枚举、Zone 枚举与背包 Snapshot |
| `Public/WacomSaveGame.h` | 当前磁盘 schema |
| `Private/WacomSaveGame.cpp` | SaveVersion 迁移链 |

外部接入点：

| 文件 | 作用 |
|---|---|
| `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp` | 进入 / 退出战斗，调用 Run 战后结算，处理战斗节点扣点和存档开关 |
| `Source/WacomApp/Public/Actors/BattleTriggerActor.h` | 战斗 Trigger 的 `PersistentId` |
| `Source/WacomApp/Public/Actors/WacomShopTriggerActor.h` | 商店入口，提供 `PersistentId` 和商品来源 |
| `Source/WacomApp/Public/Actors/WacomRunEventTriggerActor.h` | RunEvent 入口，提供 `PersistentId` 和事件定义 |

设计与数据侧对应文档：

| 文档 | 关注点 |
|---|---|
| `Docs/Game_Design.md` | 总体设计背景、时间、压力、节点、背包设计语境 |
| `Docs/WacomData.md` | 卡牌、商店、RunEvent 静态数据定义 |
| `Docs/WacomApp.md` | 场景 Actor、UI 入口与交互层约定 |

---

## §12 修改 Run 规则时的检查点

改 Run 规则前先确认影响面：

- 是否改变战内 / 战外边界。
- 是否需要新增 DataAsset 字段或 GameplayTag。
- 是否需要 SaveGame schema 升级。
- 是否影响 `PersistentId` 的含义。
- 是否需要更新自动化测试。

涉及背包、存档、事件或战斗结算的改动，至少检查 `RunSession.cpp` 对应路径和 `WacomTests` 中的 Run / Backpack / Save 相关测试。
