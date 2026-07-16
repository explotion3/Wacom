---
type: domain-spec
scope: wacom-run
status: active
updated: 2026-07-14
tags:
  - wacom/run
  - wacom/rules
  - wacom/exploration
  - wacom/inventory
---

# WacomRun 模块文档

> [!info] 本文职责
> 本文是 Run 领域规则当前事实入口。它记录 `URunSession / FRunState` 的规则状态、事务边界、持久化边界和关键实现入口；字段细节以代码为准，本文不维护完整 API / UPROPERTY 镜像。

> [!warning] 模块边界
> `WacomRun` 是战斗外规则层。战斗内牌局规则见 [WacomBattle.md](./WacomBattle.md)，静态 DataAsset 字段见 [WacomData.md](./WacomData.md)，内容生成 / 校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)，App 编排和输入见 [WacomApp.md](./WacomApp.md)，世界交互 authoring / target system 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，UI 表现见 [WacomUI.md](./WacomUI.md)。

## §1 模块定位

`WacomRun` 负责一次冒险中战斗外的规则状态。它持有 Run 状态，给战斗构造初始化参数，并接收战斗结束包做战外结算。

`WacomRun` 不负责战斗内牌局规则、UI 展示、输入路由、场景 Actor 表现、DataAsset 生成或 Editor validation。UI 和 App 层读取 Snapshot / ViewModel，把玩家意图提交给 Run 层入口；Run 层只处理规则事务。

依赖边界：

```text
WacomCore / WacomData / WacomBattle <- WacomRun <- WacomApp
```

Run 层可以引用卡牌、角色和战斗回传结构；不能依赖 Widget、PlayerController、Actor facade、WBP 绑定或场景表现细节。敌人侧初始化由 App / BattleTrigger 从 Encounter 数据转换到 `FBattleInitParams.EnemySlots`，RunSession 不读取也不接收 `UEnemyDefinition`。

## §2 Run 生命周期、失败与经验

一次 Run 由 `URunSession` 表示。它是 transient UObject，由 App 层创建并持有；核心规则数据在 `FRunState`。

新 Run 初始化时读取角色定义：

- `FingerCount / HpPerFinger` 决定基础 HP 语义。
- `StarterDeck` 生成玩家持有卡牌 instance。
- 非容器卡默认进 `BattleDeck`。
- 容器卡默认进 `Backpack`；当前 `MuseiYinchongdeng` 作为内容特例默认进 `BattleDeck`。

Run 失败条件：

| 来源 | 条件 |
|---|---|
| 战内失败 | 战斗结果为 Defeat，写入 `bRunActive=false` |
| 战外压力 | 八条压力总和 `>= 100` |
| 手指耗尽 | `FingerCount <= 0` |

`bRunActive` 只记录显式 Run 活跃状态；压力和手指失败通过 `IsRunFailed()` 综合判定。

经验来自战斗击倒结算。Victory 包括撤离，会把 `FBattleResultPacket.KnockdownExpGains[]` 累加到 Run；Defeat / Undetermined 不结算经验。

`URunSession::AddExperience()` 会把经验累加到 `ExperienceCurrent`。经验达到 `ExperienceCapacity` 时循环扣减容量，并向 `AcquiredSkills` 追加 `SkillSlot.Placeholder`。当前技能只是占位计数，不挂实际效果；正式技能系统上线前，不要让战斗规则依赖该占位 tag。

## §3 时间、Action Point 与探索移动边界

一天按固定顺序推进：

```text
Morning -> Day -> Dusk -> Night -> Sunrise -> Morning(次日)
```

正式时间状态保存在 `FRunState::TimeState`。规则事务通过 private `FRunTimeModule::TrySpendActionPoints()` 原子消费 Action Point：成本不足、Night 尚未选择路线或存在其它活动时零修改；恰好耗尽时只推进一次时段并载入下一时段预算。移动、岔路选择、地图揭示和同层 Map Travel 不消耗 Action Point。

当前初始 Action Point：

| 时段 | Action Point | 设计口径 |
|---|---:|---|
| Morning | 2 | 初始化及每个新 Morning 的清晨规划固定占用 1 点，因此正常可用 1 点 |
| Day | 6 | 主流程、战斗、商店、休息等 |
| Dusk | 2 | 可接野炊事件 |
| Night | 2 | 可接露营或夜间探险 |
| Sunrise | 1 | 夜探后的后置时段 |

时段门控与进入副作用：

| 进入时段 | 副作用 |
|---|---|
| Morning | 饥饿 +5 |
| 新 Morning | 结算一次 `BaseDecay(JourneyDay) + OverstayDecay(FloorDay)` |
| Dusk | 饥饿 +5 |
| Sunrise | 疲劳 +10 |

Night 进入时设置 `AwaitingChoice`，普通行动在玩家选择“夜间探险”前被拒绝。选择夜间探险后才可消费该时段预算并进入 Sunrise。

Camp 不是地图节点，而是 Night 从当前或同 Floor 最近合法节点发起的时间事务；Dusk 保留 Picnic，不开放 Camp。`FRunCampModule` 按有向图最短距离选择 `Resolved + bAllowsCamp` 节点，同距离按 `NodeId` 决胜；Begin 预留 1 Action Point 但不提前扣除，Cancel 释放预留且不回滚已经发生的免费定位。一次 Camp 只完成一个 `Rest / CardUpgrade / SpecialEvent / Backpack / Skill` typed handler，handler 只能读取上下文并返回 outcome，不能获得可写 `FRunState`。成功后放弃 Night 剩余点、跳过 Sunrise 并进入次日 Morning。当前没有 production handler 或临时恢复数值，详细合同见 [WacomMap.md](./WacomMap.md)。

探索期正式玩家移动模型是 Run Path Traversal，但它属于 `WacomApp` 表现 / 输入层：`UWacomRunPathTraversalComponent` 负责局部 Spline 上的 Pawn / Camera 对齐、输入消费和镜头反馈，不写入 Run 规则状态。Run 规则只维护时段、节点、背包、商店、事件和战斗回传结算。

Run Path、first-person hand、鼠标 hover / click、Run world card drop 和 Run menu zone drop 的输入 / target 路由见 [WacomApp.md](./WacomApp.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) 和 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。早期轨道探索文档只保留为 historical background，不作为当前 Run 规则真相或实现规格。

当前 Floor 地图使用独立 `FRunFloorMapSnapshot`，不扩大每条探索命令都携带的 `FRunExplorationSnapshot`。该查询只投影整层已知节点、边、显示文本、生命周期、传送事实和死胡同推荐目标；Screen 不读取 `FRunState`。传送提交仍只有 `ResolveExplorationCommand(MapTravel)` 一条规则入口，同层传送不消费 Action Point、不推进时段、不增加压力或 Floor Exposure。

## §4 压力系统

压力是战外血量。八条压力各自为 0 到 100 的百分比，总和达到 100 时 Run 失败。

压力不改变战内规则。它可以被 RunEvent 条件读取，用于限制选项、触发分支或驱动表现层提示。

Hunger 与 Fatigue 可以通过 Run 行为减少。Decay 的唯一每日入口是进入新 Morning 时由 private `FRunFloorExposureModule` 计算 `BaseDecay + OverstayDecay`：首轮 BaseDecay 曲线在所有旅程天数都返回 +5，OverstayDecay 在同 Floor 超过三天后按 `+2 / +5 / +9 / +12 capped` 追加；普通 Camp 不能减少 Decay。`LastDailyDecayAppliedDayNumber` 保证同一 Journey Day 最多结算一次；初始 Day 1 只标记已结算，不增加 Decay。

| 压力 | 当前来源 |
|---|---|
| 饥饿 | 进入 Morning / Dusk 各 +5 |
| 伤口 | 战外右手破坏 +1；战内跨高 HP 阈值 +1；跨低 HP 阈值 +5；同归于尽 +10 |
| 疲劳 | 进入 Sunrise +10；每场非 Undetermined 战斗后 +1 |
| 负重 | `BurdenZone.Num()` 计算，公式 `n*(n+1)/2`，Clamp 到 100 |
| 腐朽 | 每个新 Morning：`BaseDecay(JourneyDay) + OverstayDecay(FloorDay)`；Day 1 初始化不增加 |
| 劣迹 | 第 n 次偷窃完成时 `n*(n+1)/2 + 1` |
| 嗜血 | 永久销毁 Companion 卡 +1 |
| 残疾 | 每失去 1 根手指 +5 |

HP 阈值来自 RunState：

```text
HighHpThreshold = 0.5
LowHpThreshold  = 0.2
```

战斗内只记录是否首次跨阈值；压力修改统一在战斗结束回传给 Run 层后处理。

## §5 卡牌持有区、容量与销毁

Run 背包模型按卡牌 instance 运转。每张进入 Run 的卡都有 `FCardInstance.InstanceId`，同名卡也必须作为独立 instance 管理。

当前四个物理持有区：

| Zone | 规则 |
|---|---|
| `Backpack` | 通量存放区内容，以及位于背包侧的 B 主卡 |
| `BattleDeck` | 实际进入战斗的备战卡组 |
| `SpecialZones` | 每张 B 类容器卡各自开辟一个特殊存放区 |
| `BurdenZone` | 其他区超容后的兜底区 |

同一个 `InstanceId` 同时只能位于一个 Zone。跨区移动走 `MoveInstance()`；失败路径不修改 RunState。`URunSession::MoveInstance()` 是 public 提交入口，实际校验后 zone mutation、SpecialZone battle flag 清理、B 主卡 SpecialZone entry 保底和负重重算由 `Private/Deck/RunDeckRules.*` 承接。SpecialZone 入战标记的 `SetSpecialZoneCardBattleEnabled()` / `ToggleSpecialZoneCardBattleEnabled()` 同样只在 `InstanceId` 当前位于某个 `SpecialZones.Cards` 时成功；App 层必须先使用 `ValidateSetSpecialZoneCardBattleEnabled()` / `ValidateToggleSpecialZoneCardBattleEnabled()` 或提交 toggle 入口，不自行通过持有区推断失败原因。

Deck 操作校验继续通过 `FRunDeckOperationValidation.DisabledReason` 暴露稳定 `FName`，供 Blueprint、UI toast、RunEvent 支付和世界交互诊断兼容使用。生产代码必须通过 `WacomRunDeckOperationReasons` 取得这些 reason ID，不在规则、App flow 或测试里重新手写 deck reason 字符串。`RunEvent` 可以把 deck reason 转译成事件语境 reason（例如“未持有卡”转为“缺少所需卡牌”）；这些展示 / 事件 reason 不等同于 deck 原始失败码。

Run first-person hand 不直接等同于某个物理持有区。`URunSession::BuildRunCardWorkspaceSnapshot()` 提供 Run 层只读 `Run Card Workspace` contract，用来把当前需要展示或操作的一组已拥有卡投影给 App 层：

- `DefaultExploration` workspace 当前读取 `BattleDeck` 物理卡，并可追加 SpecialZone 中随 B 主卡投影入战的卡；投影条目仍保留真实 `PhysicalZone = SpecialZone`、`ZoneOwnerInstanceId` 和 `bIsProjectedBattleDeckCard` metadata。
- `OwnedCardsFilter` workspace 按 request 扫描 `Backpack / BattleDeck / BurdenZone / SpecialZones.Cards` 真实物理持有区，再应用 instance、Definition、CardId 和关键词过滤；它服务 RunEvent 支付、菜单临时选牌、未来世界交互候选等 App 场景。
- Workspace 不是新持有区，不进入 SaveGame，不改变容量、负重、战斗入组或卡牌归属；任何规则事务仍必须用 `MoveInstance()`、`DestroyCardByInstance()`、`ChooseRunEventOptionWithPaidCardResult()` 等正式入口。
- 未来如果 Run 专属手牌/行动牌组变成真实规则区，应先扩展物理区和存档/容量/移动规则，再让默认 workspace provider 改读该新区；first-person UI 不应因此重写。

`FRunStorageCardView` 是背包 / SpecialZone / 投影列表的单卡只读 ViewData。它会显式携带 `bCanToggleBattleEnabledInSpecialZone` 与 `bShowBattleEnabledInSpecialZoneBadge`，App 列表和 Widget 只消费这些 affordance，不通过 `PhysicalZone`、`bBattleEnabledInSpecialZone` 或列表来源重新推断右键入战是否可用。

玩家已拥有卡的操作以 `InstanceId` 为主。UI、蓝图玩家操作和交互层必须使用 `DestroyCardByInstance()`、`ValidateDestroyCardByInstance()`、`DeleteCardForGoldByInstance()`、`MoveInstance()` 等入口，不能用 Definition 指代某张已拥有卡。`URunSession` 不再提供 `AddCardToBattleDeck()`、`RemoveCardFromBattleDeck()`、`DestroyCardFromBackpack()`、`DeleteCardForGold()` 这类 Definition 级已拥有卡 wrapper。

### 原子批量移动与销毁

自由工作台的跨区移动只走 `FRunDeckBatchMoveRequest -> URunSession::MoveInstancesAtomic()`；批量销毁只走 `FRunDeckBatchDeleteRequest -> ValidateDeleteCardsForGoldAtomic() -> DeleteCardsForGoldAtomic()`。两类请求都必须携带起手时的共同来源和严格 `ExpectedStorageRevision`。

- 空集合、重复/无效 InstanceId、来源漂移、revision 漂移、失效 SpecialZone Owner 或任一单卡规则失败，整组零修改。
- Run 先在 `FRunState` working copy 中按请求稳定顺序执行现有单卡规则；全部成功后才替换权威状态。
- 成功只推进一次 BackpackStorage revision、广播一次；批量销毁同时推进 Economy revision，但仍只广播一次。
- 批量销毁奖励按当前单卡奖励规则求和，成功后一次性加金币；失败时奖励恒为 0。
- 同区牌匣收拢属于 App 布局整理，不调用 Run move API，不改变物理顺序、revision 或 SaveGame。
- 工作台布局是 `WacomApp` 当前 Run 的瞬态表现状态，不写入 `FRunState` 或 SaveGame。

Definition 仍然用于资产语义：`AcquireCardToRun()` / 战斗奖励 / 商店购买 / 世界拾取表达“获得一张某种卡”；RunEvent / DataAsset 可以表达“交出一张某种卡”，由 RunEvent 执行路径在运行态选择一张匹配 instance。玩家直接操作某张已拥有卡时必须先解析到 `InstanceId`。

### 容器分类

卡牌容量来自 `CardDefinition.Physique.Capacity`。

| 分类 | 条件 | 含义 |
|---|---|---|
| 普通卡 | `Capacity == 0` | 不提供容量 |
| A 类容器 | `Capacity > 0` 且 `CapacityEffect` 为空 | 提供通量容量 |
| B 类容器 | `Capacity > 0` 且 `CapacityEffect` 有效 | 开辟自己的特殊存放区 |

`Card.Keyword.BagProvider` 是历史关键词。当前背包 UI 是否可用以“玩家是否拥有任意 `Capacity > 0` 容器卡”为准。

容量公式：

```text
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

正式背包 UI 不暴露玩家主动拖入 `BurdenZone` 的 DropTarget。`BurdenZone` 能否接收某张卡仍属于 Run 规则 contract，由 `URunSession::ValidateMoveInstance` / `MoveInstance` 决定；App 层不再用额外 `InvalidTargetZone` 特判覆盖该规则。规则入口显式移动到 `BurdenZone` 时会刷新负重压力，但不会在同一事务里把该卡自动回填到其他可容纳区。

负重压力公式：

```text
n = BurdenZone.Num()
BurdenPressure = Clamp(n * (n + 1) / 2, 0, 100)
```

永久销毁容器卡会立刻停止贡献容量。若销毁后玩家不再拥有任何容器卡，则拒绝销毁。

### 永久销毁与金币

永久销毁入口用于删牌、事件交出卡、未来出售或战败丢弃。统一规则由 `Private/Deck/RunDeckRules.*` 承接。

当前保护规则：

- `Rarity == Intrinsic` 的卡拒绝销毁。
- 最后一张容量来源卡拒绝销毁。
- 销毁 Companion 卡会增加嗜血压力。
- 销毁 B 主卡时，它的 SpecialZone 内卡退回 Backpack；装不下则进 BurdenZone。
- 移除非容量卡后允许从负重区回填；移除容量来源卡后不做回填，只处理容量缩小导致的超容。

删牌换金币当前是简易数值：白卡 +1，蓝卡 +2。UI 拖拽删除使用 `DeleteCardForGoldByInstance()`；RunEvent 等资产语义仍可用 Definition 表达“移除一张匹配卡”，但不通过 `URunSession` 公开 deck wrapper。金币是 Run 内资源，但当前不写入 SaveGame。

## §6 商店事务

商店运行态以场景入口的 `PersistentId` 为 key，而不是以 `UShopDefinition.ShopId` 为 key。

`AWacomShopTriggerActor.PersistentId` 传给 `URunSession::BeginShopVisit(ShopId, Offers)`。第一次打开该 `ShopId` 时，用传入 Offers 建库存；再次打开同一 `ShopId` 时保留库存和已购买状态，忽略新 Offers。

正式 C++ UI 路径使用 `BeginShopVisitWithResult()` / `EndShopVisitIfOwnedWithResult()`。二者返回 `FRunShopVisitResult`，同时携带 visit ownership token 和本次节点活动的 `FRunExplorationResolution`；购买继续由 `FRunShopPurchaseResult.ExplorationResolution` 携带同一规则结果。App 必须按 Begin、每次成功 Purchase、End / rollback 的实际提交顺序消费这些结果，不能只观察 `RunStateChanged` 后拉取最新 Snapshot 来推断版本。

`UShopDefinition.ShopId` 是静态内容 ID。多个场景商店可以引用同一份 `UShopDefinition`，但只要 Actor `PersistentId` 不同，它们就是不同库存。

购买规则：

- 打开商店不消耗 Action Point。
- 成功购买会扣金币、获得卡牌、标记 Offer 已购买。
- 本次访问第一次成功购买与 1 Action Point 原子提交；同次访问后续购买为 0。
- 浏览、失败购买和空手关闭为 0；`EndShopVisit` 不再补扣成本。
- 第一次购买耗尽当前时段时，结果同时关闭 visit，App 立即关闭 Shop Screen。
- `ShopId == NAME_None`、无效 Offer、重复购买、商品为空、负价格、金币不足等失败路径不修改 RunState。
- 同一时刻只能存在一个 active shop visit；重入 Begin 会被 Run 层拒绝，不依赖旧 UI 先完成关闭。
- App UI 持有 C++ transient visit token，关闭/异步回滚必须通过 token 校验；迟到的旧 Screen 不得结束新访问。token 不进入 RunState/SaveGame。

当前 `ShopStates` 只保存在 Run 内存态，不写入 SaveGame。Actor 商品来源、Definition 字段和 Validate Map/Level 口径见 [WacomData.md](./WacomData.md)、[WacomDataAuthoring.md](./WacomDataAuthoring.md) 和 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

## §7 RunEvent 事务

RunEvent 是轻量事件图。事件内容来自 `UWacomRunEventDefinition`，运行态以场景事件 Actor 的 `PersistentId` 为 key。

`UWacomRunEventDefinition.EventId` 是内容 ID，不是运行态状态 key。同一事件定义放在多个地点时，必须给每个 Actor 配不同 `PersistentId`，状态彼此独立。

事件状态条件和 `MarkEventCompleted` 效果里的 `TargetPersistentId` 也填写场景 Actor 的 `PersistentId`，不是 `EventId`。

访问规则：

- `PersistentId == NAME_None` 或定义为空时拒绝打开。
- 已完成事件拒绝重复打开。
- 同一时刻只能存在一个 active RunEvent visit；重入 Begin 会被 Run 层拒绝。
- 打开事件不消耗 Action Point。
- 选项使用 `Automatic / Free / Fixed` 行动点策略：普通 terminal 选项的 Automatic 成本为 1，非 terminal 为 0；Fixed 使用显式非负成本。
- 正成本选项必须 terminal；effect、卡牌支付、行动点、事件状态和 Map Node lifecycle 使用同一 working-state 事务。
- 关闭事件只清 active 标记，不改变完成状态。
- App UI 持有 C++ transient visit token，关闭/异步回滚必须通过 token 校验；token 不进入 RunState/SaveGame。

正式 C++ App 路径使用 `BeginRunEventWithExplorationResult()`、`EndRunEventIfOwnedWithExplorationResult()`（无 ownership 的兼容入口为 `EndRunEventWithExplorationResult()`），并从每次成功 `FRunEventChoiceResult.ExplorationResolution` 取得本次规则提交的探索版本。App 必须按 Begin、每次成功 Choice、End / rollback 的实际提交顺序消费这些结果；原有 bool/void API 仅作为 Blueprint/旧调用兼容 wrapper，不是正式表现同步入口。

当前条件：

- 金币不少于指定值。
- 当前 Action Point 不少于指定值。
- 指定压力不高于阈值。
- 拥有 / 缺少指定卡。
- 指定 `PersistentId` 事件已完成 / 未完成。
- 当前 Run 标记已设置 / 未设置。

当前效果：

- 获得卡牌。
- 增减金币，最低不低于 0。
- 增减压力。
- Action Point 成本由 choice policy 统一结算，耗尽时可能推进时段；它不是 effect 条目。
- 从玩家任意持有区永久移除一张卡。
- 标记指定 `PersistentId` 事件完成。
- 设置 / 清除当前 Run 标记。

选项 `Effects` 按事务执行：任一效果失败时，本次选项不提交已执行的前置效果，且不改变节点、时间、卡牌、金币、压力、事件 active 或 completed 状态；失败结果不返回部分 `EffectResults`。

RunEvent 的移除卡搜索四个物理持有区：`Backpack`、`BattleDeck`、`BurdenZone` 和所有 `SpecialZones.Cards`。它不发金币，但遵守固有卡、最后容量来源卡和 Companion 嗜血规则。

RunFlag 是当前 Run 内的轻量 bool/set 记忆。`RunFlagSet / RunFlagNotSet` 条件和 `SetRunFlag / ClearRunFlag` 效果都使用 `FlagId`。RunFlag 不是 GameplayTag，不是数值变量，当前不写入 SaveGame。`SetRunFlag / ClearRunFlag` 在 RunEvent working-state 事务内执行；后续 effect 失败时会和其他前置效果一起回滚。

`FRunEventChoiceSnapshot::Requirements` 和 `Consequences` 是结构化预览事实。它们记录条件是否满足、支付需求、后果意图、节点跳转和事件结束预览；不模拟金币 clamp、行动点跨时段、副作用压力或后续效果失败。Snapshot 的 `bAvailable / DisabledReason` 也只是当前刷新时刻的展示事实，不能作为 UI 提交前的最终 veto。真实结果仍以提交后的 `FRunEventChoiceResult` 和事务状态为准。

RunEvent Choice Evaluation 的唯一 Implementation 位于 `WacomRun/Private/Events/FRunEventExecutor`。它按 Definition 顺序生成全部 condition requirement facts，固定第一条失败 condition 的 reason，并把 Owned Card Payment requirement 追加在末尾；Snapshot、普通提交、支付预检和正式支付提交都复用这份求值。正式入口每次都针对最新 `FRunState` 重新求值，不读取旧 Snapshot 作为授权。支付选项同时配置 `RemoveCard` effect 属于互斥制作错误，Snapshot、支付预检和正式提交都会返回 `PaymentChoiceHasRemoveCardEffect`。Effects、Consequences 和 working-state 回滚不属于 Choice Evaluation。

卡牌支付通过 active GameMenu 的 Run menu zone drop 提交：`FWacomRunFirstPersonCardDropCoordinator` 负责命中、preview 和分发，`AWacomPlayerController` 只提供输入/查询上下文，`UWacomRunEventScreen` 接管 release 并调用 `URunSession::ChooseRunEventOptionWithPaidCardResult()`。支付 UI、menu lease 和 drop target 合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#6-run-menu-zone-target) 与 [WacomUI.md](./WacomUI.md)。

`FRunEventChoiceResult` 表达本次选项直接效果、展示诊断字段和同批规则提交的 `ExplorationResolution`。成功卡牌支付会记录 `PaidCardDefinition`，仅用于 UI / 日志显示。成功结果还会记录节点变化、事件关闭和事件完成 facts。失败或回滚结果不写入成功 outcome，后续规则不能依赖该结果包反向修改 RunState。

当前 `RunEventStates` 和 `RunFlags` 只保存在 Run 内存态，不写入 SaveGame。Definition 字段、生成样例和 Validator 口径见 [WacomData.md](./WacomData.md) 与 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。

## §8 Pickup 与 Run World Card Interaction

Pickup 和 Run world card interaction 都以场景 `PersistentId` 写入 RunState。静态 `PickupId / InteractionId` 只用于内容识别、debug 和 validation，不是运行时完成状态 key。

### Pickup

当前 Pickup 规则入口：

| 入口 | 成功效果 | 状态 key |
|---|---|---|
| `CollectGoldPickup(PersistentId, GoldAmount)` | 增加金币 | `CollectedPickupIds` |
| `CollectCardPickup(PersistentId, CardDefinition)` | 获得一张固定卡 | `CollectedPickupIds` |
| `CollectPickupFromDefinition(PersistentId, Definition)` | 原子结算固定 Gold/Card 主奖励、可选 Credential、拾取状态及正式 Treasure 节点/AP | `CollectedPickupIds` + `GrantedCredentialIds` |

规则要求：

- `PersistentId != NAME_None`。
- 金币数量必须大于 0。
- 固定卡牌奖励必须配置 `CardDefinition`。
- `GrantedCredentialIds` 可以为空；存在时每项必须非 `None` 且 Definition 内唯一。
- 重复提交同一 `PersistentId` 不改状态、不广播。
- 金币和卡牌 Pickup 共用 `CollectedPickupIds`，相同 ID 会共享已拾取状态。
- Definition 入口先在 working state 中完成全部校验，再一次提交主奖励、Credential、拾取标记和探索结算；任一环节失败时全部回滚且不广播。
- `CollectGoldPickup()` / `CollectCardPickup()` 返回的 `FRunTreasureSettlementResult.ExplorationResolution` 是正式 Floor 中这次 Treasure 完成的版本结果；成功调用方必须把它交给 App 表现协调器，不能只读取 `bSucceeded` 后丢弃。

数据驱动推荐入口是 `AWacomRunRewardPickupActor + UWacomRunPickupDefinition`。Actor 只转发完整 Definition；Definition 仍只有一个固定 Gold/Card 主奖励，但可同时授予多个 Credential。当前不支持掉落表、多卡、区域选择或拾取动画。

### Run Credential

`FRunState::GrantedCredentialIds` 是 Run 内不可因实体卡牌移除而丢失的任务资格真相，ID 使用稳定 `FName`，不是 GameplayTag。`URunSession::HasCredential()` 只读查询；批量校验和幂等授予由 `WacomRun/Private/Credential` 持有，不向 App/Data 暴露任意写入口。

- Credential 只能由已校验的 Definition 结算事务写入；重复授予同一 ID 幂等。
- 直接销毁、删牌换金币、RunEvent 卡牌支付和世界交互消耗只改变实体卡牌，不删除 Credential。
- FloorEntrance 的 `RequiredCredentialIds` 与旧 `OwnedCardRequirements` 是 AND；Request/Confirm 都读取最新 RunState，条件本身不消耗 Credential 或卡牌。
- 已解锁入口继续按既有规则通过，不重复要求凭证。
- `Credential.Run.SerpentSigil` 是正式 Floor 1→2 的稳定资格；`Card.Run.SerpentSigil` 只是同一次 Pickup 的实体展示奖励，拥有该卡不能反推 Credential。

### Run World Card Interaction

探索期世界卡牌交互使用独立完成状态：`FRunState::CompletedRunWorldInteractionIds`。

`URunSession::ValidateRunWorldCardInteraction()` 校验：

- 场景 `PersistentId`。
- 精确 `SourceCardInstanceId`。
- Definition / CardId / RequiredKeywords / BlockedKeywords 筛选。
- 是否已完成。
- 奖励 payload 是否有效。
- 可选永久移除源卡是否安全。

`SubmitRunWorldCardInteraction()` 成功时可选消耗那张精确持有卡，按顺序发放奖励，写入 `CompletedRunWorldInteractionIds`，最后只广播一次。失败或重复提交不改状态、不广播。

当前奖励 payload 是 `Rewards` 数组，支持 Gold 和 Card：Gold 要求 `GoldAmount > 0`，Card 要求 `CardDefinition` 非空。当前不支持掉落表、动画、多阶段结果或 SaveGame。

`AWacomRunKeyChestActor` 是当前验证入口。普通 E 键或左键点击只提示，不直接结算奖励；拖卡成功后才提交 RunSession 事务。Actor facade、receiver prompt、失败 Toast 和 map validation 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

当前 `CollectedPickupIds` 和 `CompletedRunWorldInteractionIds` 只保存在 Run 内存态，不写入 SaveGame。

## §9 Run UI SnapshotRevision

`URunSession` 提供三类 C++ 只读 transient revision，给 App 层 UI 做刷新门控：

| Revision | 递增条件 |
|---|---|
| `GetBackpackStorageSnapshotRevision()` | 背包物理持有区、SpecialZone、入战投影、容量 / 负重等背包显示事实变化 |
| `GetShopSnapshotRevision()` | active shop、商品库存 / 顺序 / 身份、购买状态、本次访问购买标记变化 |
| `GetEconomySnapshotRevision()` | 金币变化 |

这些 revision 不写入 `FRunState` / SaveGame，不暴露 Blueprint API，也不替代规则校验。它们只用于 `UWacomBackpackScreen` / `UWacomShopScreen` 在事件驱动刷新时跳过昂贵 Snapshot 构建；revision 变化后 UI 仍会继续走各自的 signature dirty gate 和 widget identity reconcile。

维护约定：

- 修改 `RunState` 后仍保留 `NotifyRunStateChanged()` 粗粒度广播。
- 战斗结算、商店关闭和 RunEvent 选择等组合事务会合并内部通知，在事务末尾广播一次。
- 会改变 Backpack / Shop / Economy UI Snapshot 事实的事务，必须在广播前通过 `RunSession.cpp` 私有 typed dirty flags 入口标记对应 revision；不要用裸数值 mask 在多个事务分支里手写 revision 组合。
- RunEvent 和 Run world card interaction 的影响面由局部 helper 从成功 result / reward / consume fact 统一推导，避免在多个 if 分支里重复写 bump 条件。
- `Wacom.Run.SnapshotRevisions` 是 revision drift guard；`Wacom.Run.NotificationCoalescing` 是组合事务广播次数 guard。

常见事务影响：

| 事务 / 路径 | BackpackStorage | Shop | Economy |
|---|---:|---:|---:|
| 正式 Run 初始化 / ApplySaveGameToRunState | 是 | 是 | 是 |
| RecomputeBurden、MoveInstance、AcquireCardToRun、DestroyCardByInstance、BattleDeck 移动 | 是 | 否 | 否 |
| SpecialZone 入战投影开关 | 是 | 否 | 否 |
| CollectCardPickup、Run world card reward、Run world consume source card、Battle gained card | 是 | 否 | 否 |
| DeleteCardForGoldByInstance / PurchaseCardFromShop | 是 | 否 | 是 |
| AddGold / RemoveGold / CollectGoldPickup / Run world gold reward | 否 | 否 | 是 |
| BeginShopVisit / EndShopVisit | 否 | 是 | 否 |
| PurchaseShopOffer | 是 | 是 | 是 |
| RunEvent paid card、GainCard、RemoveCard | 是 | 否 | 否 |
| RunEvent AddGold | 否 | 否 | 是 |
| RunEvent AddPressure、Action Point、MarkEventCompleted、SetRunFlag / ClearRunFlag、open / close event | 否 | 否 | 否 |
| 压力、时段、触发器销毁、Battle pressure / exp / defeated enemy / progress only | 否 | 否 | 否 |

<a id="wacomrun-battle-settlement"></a>
## §10 战斗联动与战后结算

进入战斗前，`URunSession::BuildInitParamsForBattle(TriggerPersistentId, OutParams)` 从 RunState 构造 `FBattleInitParams`。

关键输入：

- 角色和战斗随机种子。
- GameMode 正式进入战斗时必须成功调用 RunSession 构造玩家侧参数；失败时拒绝开战，不再用 GameMode 的 `DefaultCharacter / DefaultRandomSeed` 拼 fallback 战斗。
- `URunSession::BuildInitParamsForBattle(TriggerPersistentId, OutParams)` 不读取、不接收、不写入敌人定义，也不直接读取 Encounter 资产。敌人规则槽位由 App 层从 `ABattleTriggerActor.EncounterDefinition` 转换为 `FBattleInitParams.EnemySlots`；Battle 初始化以 `EnemySlots` 作为唯一敌人入口。
- HP 压力阈值 `HighHpThreshold / LowHpThreshold`。
- `BattleDeck` 中的物理卡。
- SpecialZone 中勾选入战的卡：只有 B 主卡位于 `BattleDeck`，且主卡有 `CapacityEffect`，其 SpecialZone 内 `bBattleEnabledInSpecialZone == true` 的卡才会入战，并携带主卡容量效果。
- `BuildInitParamsForBattle()` 仍可使用场景 `TriggerPersistentId` 作为本场 Battle 的 `EncounterId`，但撤离进度按当前 `FWacomMapNodeHandle` 查找；若当前节点有记录，则优先把 `DestroyedPartKeys` 转换为 `PreDestroyedParts`，没有公开 key 时才读取 `DestroyedParts` 内部 identity 投影。

`EncounterId` 是 Battle 内部稳定身份；Run 的撤离重入真相则使用当前 `FloorId + NodeId`。同一个 Encounter 资产可以被多个 Map Node 复用而不会串进度。

GameMode 开战前必须先取得 `Encounter` 的 `FRunNodeActivityTicket`；战斗 UI 启动失败时取消票据。结束时把 `FBattleResultPacket` 和同一票据交给 `SettleEncounterNodeActivity()`，Run 以 working state 原子结算预留、奖励、压力、撤离进度和节点生命周期。旧 `OnBattleFinished*` wrapper 已删除。

Outcome 分支：

| 结果 | Run 处理 |
|---|---|
| Victory 且 `bWithdrawn == true` | 撤离；释放预留、消费 0，节点保持 Visited；写 `BattleProgress[MapNodeHandle].DestroyedPartKeys` |
| Victory 且未撤离 | 真胜利；提交预留的 1 Action Point、清理该节点撤离进度并 Resolve Encounter 节点 |
| Defeat | `bRunActive = false` |
| Undetermined | 不做战外结算并返回 |

战后压力与奖励：

- 任一非 Undetermined 结果：疲劳 +1。
- `bCrossedHighHpThreshold`：伤口 +1。
- `bCrossedLowHpThreshold`：伤口 +5。
- `bMutualDestruction`：伤口 +10，不直接终止 Run。
- Victory 包含撤离：结算 `KnockdownExpGains[]` 和 `GainedCards[]`。
- Defeat / Undetermined 不结算经验和获得卡。
- `KnockdownChoices[]` 当前只按 `PartKey` 记日志，后续事件分支再消费。

GameMode 不再手工扣时段点，也不根据已破坏部位数量重新推断胜利。只有正式 settlement 成功且 `Packet.Outcome == Victory && !Packet.bWithdrawn` 时，才把已 Resolved Map Node 投影到 SaveGame v3 的 `DestroyedTriggerIds`，并启动当前 BattleTrigger 的场景退役。Trigger 会立即失去交互，但 Host 保留完整 Destroyed/Downed 末帧；返回探索镜头和 ExitBattle 后置工作都完成后，Trigger 才隐藏并禁用 Encounter 内有效 Host/Part，然后销毁自身。撤离、失败、Undetermined 或 settlement 失败都保留场景，允许从同一 Map Node 重入。Map Node lifecycle 始终是完成状态真相，`DestroyedTriggerIds` 只是旧存档格式的兼容投影。

`BattleProgress[MapNodeHandle].DestroyedPartKeys` 是撤离重入的规则真相，部位身份仍由 `EncounterId + EnemySlotId + PartSlotId` 匹配。`DestroyedParts` 只作为无法派生有效 key 的旧数据 / 手写测试 snapshot 内部 fallback。当前 `BattleProgress` 仍不进入 SaveGame。

## §11 PersistentId 规则

`PersistentId` 是场景对象在 RunState 中的稳定身份。它不是显示名，也不是静态内容资产 ID。

当前已用场景 ID：

| 场景对象 | PersistentId 用途 |
|---|---|
| `ABattleTriggerActor` | Save v3 完成投影、撤离 `BattleProgress` |
| `AWacomShopTriggerActor` | `ShopStates` 库存与已购买状态 |
| `AWacomRunEventTriggerActor` | `RunEventStates` 当前节点与完成状态 |
| `AWacomRunPickupActor` | 金币拾取物已拾取状态 |
| `AWacomRunCardPickupActor` | 卡牌拾取物已拾取状态 |
| `AWacomRunRewardPickupActor` | 数据驱动金币 / 卡牌拾取物已拾取状态 |
| `AWacomRunKeyChestActor` | 世界卡牌交互完成状态 |

规则：

- 参与 Run 状态的场景 Actor 必须配置非空 `PersistentId`。
- 同一关卡内应保持唯一。
- `NAME_None` 表示不参与对应状态记录；入口会 warning 或拒绝。
- 内容资产 ID 不能替代场景 `PersistentId`。
- `UShopDefinition.ShopId`、`UWacomRunEventDefinition.EventId`、`UWacomRunPickupDefinition.PickupId`、`UWacomRunWorldCardInteractionDefinition.InteractionId` 都只是内容 ID，不参与当前 Run 的库存、事件、已拾取或完成状态。

Validate Map/Level 对 Actor 摆放实例的校验口径见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#2-run-world-interactable-actor)；Definition 自身校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#asset-validation)。

## §12 SaveGame 当前边界

当前 `AWacomGameMode::bSaveSystemEnabled == false`。正常游戏流程不读盘、不写盘；战斗结束和退出时的自动存档会静默 no-op。

下面只描述底层 `URunSession::SaveToSlot()` / `LoadFromSlot()` 和 `UWacomSaveGame` v4 的实际字段拷贝结果。

`LoadFromSlot()` 成功应用 SaveGame 到 `RunState` 后会标记 Run UI snapshot dirty，并广播一次 `OnRunStateChangedNative`。读档失败（slot 不存在、SaveGame 类型不匹配、版本或字段校验失败）不修改 RunState，也不广播。Run first-person source、ViewModel provider 和其他只读 Run UI 应依赖这条通知更新到读档后的 default workspace / storage 状态；当前 default workspace provider 仍读取 BattleDeck 物理卡和可选投影卡。

### v4 磁盘会保存

| SaveGame 字段 | 来源 / 说明 |
|---|---|
| `SaveVersion`、`SavedAtUtc`、`ClientBuildId` | 存档元数据，当前版本为 4 |
| `CharacterAssetPath` | 当前角色资产路径 |
| `BattleSeed` | 战斗随机种子 |
| `bRunActive` | Run 活跃状态 |
| `DestroyedTriggerIds` | 已 Resolved Encounter Trigger 的 Save v3 兼容投影；Map Node lifecycle 才是完成真相 |
| `GrantedCredentialIds` | 已获得的稳定 Run Credential；写盘按 `FName` 词法序排序 |
| `PlayerTransform`、`bHasPlayerTransform` | 探索 Pawn 位置 |
| `Backpack` | 卡牌 instance 列表 |
| `BattleDeck` | 卡牌 instance 列表 |
| `BurdenZone` | 卡牌 instance 列表 |
| `SpecialZones` | B 主卡 owner id 与区内卡牌 instance |

卡牌 instance 存档条目保存 `InstanceId`、`DefinitionAssetPath` 和 `bBattleEnabledInSpecialZone`。读档时要求 InstanceId 非零、全表唯一，Definition 能加载成功。

若旧档迁移到 v2 后四个 instance 数组全空，读档会按 Character 的 StarterDeck 重新生成 instance；新 GUID 会替代旧运行态身份。

v4 会恢复 `BurdenZone` 的卡牌列表，但不会恢复或重算 `Pressure.Burden`。压力整体仍按下表属于未持久化状态，读档后为默认值。v3 已移除旧 `DefeatedEnemyAssetPaths`；由于 Map Node lifecycle 尚未进入磁盘字段，`DestroyedTriggerIds` 暂时承担已 Resolved Encounter 的磁盘兼容投影。Bootstrap 命中该投影时先通过 Trigger 的统一入口退役 Host/Part，再销毁 Trigger，避免读档后留下 Idle 敌人。v3→v4 把 Credential 明确迁移为空集合，不从实体卡牌推断；v4 读档拒绝 `None` 或重复 Credential，失败时 RunState 不变且不广播。

### 当前仍是内存态

| RunState 字段 / 系统 | SaveGame v4 状态 | 读档后的实际结果 |
|---|---|---|
| `FingerCount`、`HpPerFinger` | 不保存 | 使用 `FRunState` 默认值，不从 SaveGame 还原 |
| `Pressure`、`TheftCount` | 不保存 | 压力全为 0，偷窃计数为 0 |
| `HighHpThreshold`、`LowHpThreshold` | 不保存 | 使用默认 `0.5 / 0.2` |
| `ExperienceCurrent`、`ExperienceCapacity`、`AcquiredSkills` | 不保存 | 经验为 0，上限默认 10，技能为空 |
| `TimeState`（Day、Phase、RemainingActionPoints、NightGate） | 不保存 | 当前 schema 回到第 1 天 Morning；正式探索初始化应用 `2 / 6 / 2 / 2 / 1` 并占用 Morning Planning 1 点 |
| `Gold` | 不保存 | 读档后为 0 |
| `BattleProgress` | 不保存 | 撤离留下的已破坏部位不会跨磁盘读档保留 |
| `ActiveShopId`、`bShopVisitHasPurchase` | 不保存 | 无 active shop |
| `ShopStates` | 不保存 | 商店库存和已购买状态清空 |
| `ActiveRunEventId`、`ActiveRunEventDefinition` | 不保存 | 无 active event |
| `RunEventStates` | 不保存 | 事件当前节点和完成状态清空 |
| `RunFlags` | 不保存 | 当前 Run 内存态事件标记清空 |
| `CollectedPickupIds` | 不保存 | 世界金币 / 卡牌拾取物已拾取状态清空 |
| `CompletedRunWorldInteractionIds` | 不保存 | 世界卡牌交互完成状态清空 |

因此，当前 SaveGame 不能被描述为完整 Run 存档。它只覆盖部分场景与卡牌持有状态，而且正常流程还被 GameMode 总开关禁用。

后续恢复存档系统时，必须先决定这些字段的持久化策略，并同步升级 `UWacomSaveGame::CurrentSaveVersion` 与迁移链。

## §13 关键实现入口

Run 领域入口集中在 `Source/WacomRun/`：

| 文件 | 作用 |
|---|---|
| `Public/RunSession.h` | Run 的命令 / 查询入口；UI 和 GameMode 不直接改 RunState |
| `Private/RunSession.cpp` | 时间、压力、商店 / RunEvent public 入口、战斗回传 public 入口、SaveGame slot IO、dirty revision 和通知广播的协调实现 |
| `Private/Battle/RunBattleSettlementResolver.*` | 战斗结束回传包的 Run 结算流程 |
| `Private/Deck/RunDeckRules.*` | 背包、备战区、SpecialZone、负重区的私有规则 helper；拥有已通过校验后的物理区移动 mutation |
| `Private/Credential/RunCredentialModule.*` | Credential ID 校验、幂等授予与 FloorEntrance 全量持有检查 |
| `Private/Exploration/RunTimeModule.*` | Action Point 原子消费、时段推进、Morning Planning、Night gate 与 Camp 特殊推进原语 |
| `Private/Exploration/RunFloorExposureModule.*` | 新 Morning 的 Base/Overstay Decay 一次性结算 |
| `Private/Exploration/RunNodeActivityModule.*` | Encounter/RunEvent/Shop/Treasure 的互斥活动票据、预留和原子完成/取消 |
| `Private/Exploration/RunCampModule.*` | Camp 最近合法落点、预留、typed handler 和 Night→Morning 提交 |
| `Private/Exploration/RunFloorTransitionModule.*` | 入口预览、Credential/持有卡 AND 门槛、确认/取消票据和不可逆跨层提交 |
| `Private/Exploration/RunExplorationSnapshotBuilder.*` | Camp eligibility、Floor history、travelability 与 transition preview 的只读投影 |
| `Private/Events/RunEventExecutor.*` | RunEvent 事件图解释、选项条件、效果执行和结果包生成 |
| `Private/Save/RunSaveGameSerializer.*` | `FRunState <-> UWacomSaveGame` 字段拷贝、SaveEntry 写入和读档校验 |
| `Private/Shops/RunShopTransaction.*` | 商店访问、库存快照和购买事务 helper |
| `Public/RunState.h` | `FRunState`、商店状态、事件状态、战斗进度快照 |
| `Public/RunStateTypes.h` | `FCardInstance`、压力枚举、时段枚举、Zone 枚举、deck operation reason ID 与背包 Snapshot |
| `Public/WacomSaveGame.h` | 当前磁盘 schema |
| `Private/WacomSaveGame.cpp` | SaveVersion 迁移链 |

外部协作入口：

| 文档 | 关注点 |
|---|---|
| [WacomApp.md](./WacomApp.md) | GameMode、PlayerController、输入协调和战斗进出流程 |
| [WacomWorldInteraction.md](./WacomWorldInteraction.md) | 世界交互 Actor、target handle、click / hover / drop 路由与 debug / validation |
| [WacomData.md](./WacomData.md) | 卡牌、商店、RunEvent、Pickup 和 Run world card interaction 静态字段 |
| [WacomDataAuthoring.md](./WacomDataAuthoring.md) | 内容生成、Data Validation、生成资产测试和制作矩阵 |
| [WacomGameplayTags.md](./WacomGameplayTags.md) | GameplayTag 字典和新增 tag 检查点 |
| [WacomUI.md](./WacomUI.md) | UI ownership、Run UI 摘要、测试访问原则和 WBP 文档分工 |

## §14 修改 Run 规则时的检查点

改 Run 规则前先确认影响面：

- 是否改变战内 / 战外边界。
- 是否需要新增 DataAsset 字段、内容生成规则或 GameplayTag。
- 是否需要 SaveGame schema 升级。
- 是否影响 `PersistentId` 的含义。
- 是否影响 App 输入 / 世界交互 / UI 表现，但不要把这些实现细节写回 Run 规则层。
- 是否需要更新自动化测试。

涉及背包、存档、事件、商店、Pickup、世界卡牌交互或战斗结算的改动，至少检查 `RunSession.cpp` 对应路径和 `WacomTests` 中的 Run / Backpack / Save / UI 相关测试。
