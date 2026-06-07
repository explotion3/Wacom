---
type: domain-spec
scope: wacom-run
status: active
updated: 2026-06-06
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

## §3 时间、节点与探索移动边界

一天按固定顺序推进：

```text
Morning -> Day -> Dusk -> Night -> Sunrise -> Morning(次日)
```

任一时段节点数消耗到 0 时，`URunSession::ConsumeNode()` 自动调用 `AdvanceToNextPhase()`。玩家移动本身不消耗节点；节点消耗只发生在战斗结束、事件效果或明确规则事务中。

当前初始节点数：

| 时段 | 节点数 | 设计口径 |
|---|---:|---|
| Morning | 2 | 清晨规划事件会占用其中 1 点 |
| Day | 6 | 主流程、战斗、商店、休息等 |
| Dusk | 2 | 可接野炊事件 |
| Night | 2 | 可接露营或夜间探险 |
| Sunrise | 1 | 夜探后的后置时段 |

时段进入副作用：

| 进入时段 | 副作用 |
|---|---|
| Morning | 饥饿 +5 |
| Morning 且前一时段是 Sunrise | 腐朽 +5 |
| Dusk | 饥饿 +5 |
| Sunrise | 疲劳 +10 |

设计上露营会从 Night 直接进入次日 Morning，跳过 Sunrise。当前通用推进路径尚未实现这条特殊事件分支。

探索期正式玩家移动模型是 Run Tunnel，但它属于 `WacomApp` 表现 / 输入层：`UWacomRunTunnelMovementComponent` 负责 Pawn / Camera 对齐和输入消费，不写入 Run 规则状态。Run 规则只维护时段、节点、背包、商店、事件和战斗回传结算。

Run Tunnel、first-person hand、鼠标 hover / click、Run world card drop 和 Run menu zone drop 的输入 / target 路由见 [WacomApp.md](./WacomApp.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) 和 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。Run Tunnel 归档文档只保留为 historical background，不作为当前 Run 规则真相或实现规格。

## §4 压力系统

压力是战外血量。八条压力各自为 0 到 100 的百分比，总和达到 100 时 Run 失败。

压力不改变战内规则。它可以被 RunEvent 条件读取，用于限制选项、触发分支或驱动表现层提示。

| 压力 | 当前来源 |
|---|---|
| 饥饿 | 进入 Morning / Dusk 各 +5 |
| 伤口 | 战外右手破坏 +1；战内跨高 HP 阈值 +1；跨低 HP 阈值 +5；同归于尽 +10 |
| 疲劳 | 进入 Sunrise +10；每场非 Undetermined 战斗后 +1 |
| 负重 | `BurdenZone.Num()` 计算，公式 `n*(n+1)/2`，Clamp 到 100 |
| 腐朽 | 从 Sunrise 进入次日 Morning +5 |
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

同一个 `InstanceId` 同时只能位于一个 Zone。跨区移动走 `MoveInstance()`；失败路径不修改 RunState。

玩家已拥有卡的操作以 `InstanceId` 为主。UI、蓝图玩家操作和交互层必须使用 `DestroyCardByInstance()`、`ValidateDestroyCardByInstance()`、`DeleteCardForGoldByInstance()`、`MoveInstance()` 等入口，不能用 Definition 指代某张已拥有卡。

旧 Definition 级入口 `DestroyCardFromBackpack()`、`DeleteCardForGold()`、`AddCardToBattleDeck()`、`RemoveCardFromBattleDeck()` 只作为 C++ 兼容入口和资产语义桥保留，不再 Blueprint 暴露。它们会在对应来源范围内删除或移动第一张匹配 Definition 的 instance。

RunEvent / DataAsset 仍可用 Definition 表达“获得一张某种卡”或“交出一张某种卡”，因为这是资产语义，不指向玩家当前拥有的某个具体实例。

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

删牌换金币当前是简易数值：白卡 +1，蓝卡 +2。UI 拖拽删除使用 `InstanceId`；RunEvent 等资产语义仍可用 Definition 级入口表达“移除一张匹配卡”。金币是 Run 内资源，但当前不写入 SaveGame。

## §6 商店事务

商店运行态以场景入口的 `PersistentId` 为 key，而不是以 `UShopDefinition.ShopId` 为 key。

`AWacomShopTriggerActor.PersistentId` 传给 `URunSession::BeginShopVisit(ShopId, Offers)`。第一次打开该 `ShopId` 时，用传入 Offers 建库存；再次打开同一 `ShopId` 时保留库存和已购买状态，忽略新 Offers。

`UShopDefinition.ShopId` 是静态内容 ID。多个场景商店可以引用同一份 `UShopDefinition`，但只要 Actor `PersistentId` 不同，它们就是不同库存。

购买规则：

- 打开商店不消耗节点。
- 成功购买会扣金币、获得卡牌、标记 Offer 已购买。
- 关闭商店时，如果本次访问买过至少一件商品，统一消耗 1 节点。
- 没买东西就关闭，不消耗节点。
- `ShopId == NAME_None`、无效 Offer、重复购买、商品为空、负价格、金币不足等失败路径不修改 RunState。

当前 `ShopStates` 只保存在 Run 内存态，不写入 SaveGame。Actor 商品来源、Definition 字段和 Validate Map/Level 口径见 [WacomData.md](./WacomData.md)、[WacomDataAuthoring.md](./WacomDataAuthoring.md) 和 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

## §7 RunEvent 事务

RunEvent 是轻量事件图。事件内容来自 `UWacomRunEventDefinition`，运行态以场景事件 Actor 的 `PersistentId` 为 key。

`UWacomRunEventDefinition.EventId` 是内容 ID，不是运行态状态 key。同一事件定义放在多个地点时，必须给每个 Actor 配不同 `PersistentId`，状态彼此独立。

事件状态条件和 `MarkEventCompleted` 效果里的 `TargetPersistentId` 也填写场景 Actor 的 `PersistentId`，不是 `EventId`。

访问规则：

- `PersistentId == NAME_None` 或定义为空时拒绝打开。
- 已完成事件拒绝重复打开。
- 打开事件不消耗节点。
- 只有选项 Effects 配置 `ConsumeNode` 时才消耗节点。
- 关闭事件只清 active 标记，不改变完成状态。

当前条件：

- 金币不少于指定值。
- 当前节点数不少于指定值。
- 指定压力不高于阈值。
- 拥有 / 缺少指定卡。
- 指定 `PersistentId` 事件已完成 / 未完成。
- 当前 Run 标记已设置 / 未设置。

当前效果：

- 获得卡牌。
- 增减金币，最低不低于 0。
- 增减压力。
- 消耗节点并可能推进时段。
- 从玩家任意持有区永久移除一张卡。
- 标记指定 `PersistentId` 事件完成。
- 设置 / 清除当前 Run 标记。

选项 `Effects` 按事务执行：任一效果失败时，本次选项不提交已执行的前置效果，且不改变节点、时间、卡牌、金币、压力、事件 active 或 completed 状态；失败结果不返回部分 `EffectResults`。

RunEvent 的移除卡搜索四个物理持有区：`Backpack`、`BattleDeck`、`BurdenZone` 和所有 `SpecialZones.Cards`。它不发金币，但遵守固有卡、最后容量来源卡和 Companion 嗜血规则。

RunFlag 是当前 Run 内的轻量 bool/set 记忆。`RunFlagSet / RunFlagNotSet` 条件和 `SetRunFlag / ClearRunFlag` 效果都使用 `FlagId`。RunFlag 不是 GameplayTag，不是数值变量，当前不写入 SaveGame。`SetRunFlag / ClearRunFlag` 在 RunEvent working-state 事务内执行；后续 effect 失败时会和其他前置效果一起回滚。

`FRunEventChoiceSnapshot::Requirements` 和 `Consequences` 是结构化预览事实。它们记录条件是否满足、支付需求、后果意图、节点跳转和事件结束预览；不模拟金币 clamp、行动点跨时段、副作用压力或后续效果失败。真实结果仍以提交后的 `FRunEventChoiceResult` 和事务状态为准。

卡牌支付通过 active GameMenu 的 Run menu zone drop 提交：PlayerController 只负责命中、preview 和分发，`UWacomRunEventScreen` 接管 release 并调用 `URunSession::ChooseRunEventOptionWithPaidCardResult()`。支付 UI、menu lease 和 drop target 合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#6-run-menu-zone-target) 与 [WacomUI.md](./WacomUI.md)。

`FRunEventChoiceResult` 只表达本次选项直接效果和展示诊断字段，供 UI 和日志展示。成功卡牌支付会记录 `PaidCardDefinition`，仅用于 UI / 日志显示。成功结果还会记录节点变化、事件关闭和事件完成 facts。失败或回滚结果不写入成功 outcome，后续规则不能依赖该结果包反向修改 RunState。

当前 `RunEventStates` 和 `RunFlags` 只保存在 Run 内存态，不写入 SaveGame。Definition 字段、生成样例和 Validator 口径见 [WacomData.md](./WacomData.md) 与 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。

## §8 Pickup 与 Run World Card Interaction

Pickup 和 Run world card interaction 都以场景 `PersistentId` 写入 RunState。静态 `PickupId / InteractionId` 只用于内容识别、debug 和 validation，不是运行时完成状态 key。

### Pickup

当前 Pickup 规则入口：

| 入口 | 成功效果 | 状态 key |
|---|---|---|
| `CollectGoldPickup(PersistentId, GoldAmount)` | 增加金币 | `CollectedPickupIds` |
| `CollectCardPickup(PersistentId, CardDefinition)` | 获得一张固定卡 | `CollectedPickupIds` |

规则要求：

- `PersistentId != NAME_None`。
- 金币数量必须大于 0。
- 固定卡牌奖励必须配置 `CardDefinition`。
- 重复提交同一 `PersistentId` 不改状态、不广播。
- 金币和卡牌 Pickup 共用 `CollectedPickupIds`，相同 ID 会共享已拾取状态。

数据驱动推荐入口是 `AWacomRunRewardPickupActor + UWacomRunPickupDefinition`。Definition 只描述固定单一奖励；运行时仍调用现有金币 / 卡牌拾取结算。当前不支持掉落表、多卡、区域选择、拾取动画或 SaveGame。

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
- 会改变 Backpack / Shop / Economy UI Snapshot 事实的事务，必须在广播前通过 `RunSession.cpp` 私有 dirty flags 入口标记对应 revision。
- RunEvent 和 Run world card interaction 的影响面由局部 helper 从成功 result / reward / consume fact 统一推导，避免在多个 if 分支里重复写 bump 条件。
- `Wacom.Run.SnapshotRevisions` 是 revision drift guard；`Wacom.Run.NotificationCoalescing` 是组合事务广播次数 guard。

常见事务影响：

| 事务 / 路径 | BackpackStorage | Shop | Economy |
|---|---:|---:|---:|
| Initialize / ResetRunState / ApplySaveGameToRunState | 是 | 是 | 是 |
| RecomputeBurden、MoveInstance、AcquireCardToRun、DestroyCard、BattleDeck 增删 | 是 | 否 | 否 |
| SpecialZone 入战投影开关 | 是 | 否 | 否 |
| CollectCardPickup、Run world card reward、Run world consume source card、Battle gained card | 是 | 否 | 否 |
| DeleteCardForGold / PurchaseCardFromShop | 是 | 否 | 是 |
| AddGold / RemoveGold / CollectGoldPickup / Run world gold reward | 否 | 否 | 是 |
| BeginShopVisit / EndShopVisit | 否 | 是 | 否 |
| PurchaseShopOffer | 是 | 是 | 是 |
| RunEvent paid card、GainCard、RemoveCard | 是 | 否 | 否 |
| RunEvent AddGold | 否 | 否 | 是 |
| RunEvent AddPressure、ConsumeNode、MarkEventCompleted、SetRunFlag / ClearRunFlag、open / close event | 否 | 否 | 否 |
| 压力、时段、触发器销毁、Battle pressure / exp / defeated enemy / progress only | 否 | 否 | 否 |

<a id="wacomrun-battle-settlement"></a>
## §10 战斗联动与战后结算

进入战斗前，`URunSession::BuildInitParamsForBattle()` 从 RunState 构造 `FBattleInitParams`。

关键输入：

- 角色和战斗随机种子。
- `URunSession::BuildInitParamsForBattle()` 不读取、不接收、不写入敌人定义，也不直接读取 Encounter 资产。敌人规则槽位由 App 层从 `ABattleTriggerActor.EncounterDefinition` 转换为 `FBattleInitParams.EnemySlots`；Battle 初始化以 `EnemySlots` 作为唯一敌人入口。
- HP 压力阈值 `HighHpThreshold / LowHpThreshold`。
- `BattleDeck` 中的物理卡。
- SpecialZone 中勾选入战的卡：只有 B 主卡位于 `BattleDeck`，且主卡有 `CapacityEffect`，其 SpecialZone 内 `bBattleEnabledInSpecialZone == true` 的卡才会入战，并携带主卡容量效果。
- 若传入 `TriggerPersistentId`，`BuildInitParamsForBattle()` 会把 `Params.EncounterId` 设为该 PersistentId；若 `RunState.BattleProgress` 有记录，则优先把完整 `DestroyedParts` 写入 `PreDestroyedParts`。旧 `DestroyedPartIds` 进度只作为 legacy 投影，转换到默认 `Enemy` 槽兼容重入。

运行态 `EncounterId` 仍来自场景 Trigger 的 `PersistentId`，不要用 `EncounterDefinitionId` 替代撤离重入进度 key。同一个 Encounter 资产可被多个 Trigger 复用；只要 Trigger `PersistentId` 不同，它们的 `BattleProgress` 就彼此独立。

战斗结束时，GameMode 先处理战斗 UI 和场景 Trigger，再调用 `OnBattleFinishedFromTrigger(Packet, TriggerPersistentId)` 做 Run 结算。

Outcome 分支：

| 结果 | Run 处理 |
|---|---|
| Victory 且 `bWithdrawn == true` | 撤离；不销毁 Trigger；写 `BattleProgress[TriggerId].DestroyedParts`，并保留 `DestroyedPartIds` legacy 投影 |
| Victory 且未撤离 | 真胜利；清理 `BattleProgress[TriggerId]`；场景完成状态由 GameMode 调 `MarkTriggerDestroyed(TriggerId)` 写入 `DestroyedTriggerIds` |
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

`BattleProgress.DestroyedParts` 是撤离重入的新规则真相，身份由 `EncounterId + EnemySlotId + PartSlotId` 匹配，避免后续多敌人 encounter 中同名部位互相串进度。`DestroyedPartIds` 仍保留在内存 snapshot 中，供旧单敌人进度、测试和 debug 兼容读取；Run 构建战斗参数时会把只有旧字段的进度转换为默认 `Enemy` 槽的 `PreDestroyedParts`。当前 `BattleProgress` 仍不进入 SaveGame。

## §11 PersistentId 规则

`PersistentId` 是场景对象在 RunState 中的稳定身份。它不是显示名，也不是静态内容资产 ID。

当前已用场景 ID：

| 场景对象 | PersistentId 用途 |
|---|---|
| `ABattleTriggerActor` | 已销毁 Trigger、撤离 `BattleProgress` |
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

下面只描述底层 `URunSession::SaveToSlot()` / `LoadFromSlot()` 和 `UWacomSaveGame` v3 的实际字段拷贝结果。

### v3 磁盘会保存

| SaveGame 字段 | 来源 / 说明 |
|---|---|
| `SaveVersion`、`SavedAtUtc`、`ClientBuildId` | 存档元数据，当前版本为 3 |
| `CharacterAssetPath` | 当前角色资产路径 |
| `BattleSeed` | 战斗随机种子 |
| `bRunActive` | Run 活跃状态 |
| `DestroyedTriggerIds` | 已永久销毁的战斗 Trigger |
| `PlayerTransform`、`bHasPlayerTransform` | 探索 Pawn 位置 |
| `Backpack` | 卡牌 instance 列表 |
| `BattleDeck` | 卡牌 instance 列表 |
| `BurdenZone` | 卡牌 instance 列表 |
| `SpecialZones` | B 主卡 owner id 与区内卡牌 instance |

卡牌 instance 存档条目保存 `InstanceId`、`DefinitionAssetPath` 和 `bBattleEnabledInSpecialZone`。读档时要求 InstanceId 非零、全表唯一，Definition 能加载成功。

若旧档迁移到 v2 后四个 instance 数组全空，读档会按 Character 的 StarterDeck 重新生成 instance；新 GUID 会替代旧运行态身份。

v3 会恢复 `BurdenZone` 的卡牌列表，但不会恢复或重算 `Pressure.Burden`。压力整体仍按下表属于未持久化状态，读档后为默认值。v3 已移除旧 `DefeatedEnemyAssetPaths`；战斗入口完成状态只以 `DestroyedTriggerIds` 表达。

### 当前仍是内存态

| RunState 字段 / 系统 | SaveGame v3 状态 | 读档后的实际结果 |
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
| `Private/RunSession.cpp` | 时间、压力、商店 / RunEvent public 入口、战斗回传 public 入口、SaveGame slot IO 的协调实现 |
| `Private/Battle/RunBattleSettlementResolver.*` | 战斗结束回传包的 Run 结算流程 |
| `Private/Deck/RunDeckRules.*` | 背包、备战区、SpecialZone、负重区的私有规则 helper |
| `Private/Time/RunTimeRules.*` | 时间、节点消耗、时段推进与时段进入压力副作用 |
| `Private/Events/RunEventExecutor.*` | RunEvent 事件图解释、选项条件、效果执行和结果包生成 |
| `Private/Save/RunSaveGameSerializer.*` | `FRunState <-> UWacomSaveGame` 字段拷贝、SaveEntry 写入和读档校验 |
| `Private/Shops/RunShopTransaction.*` | 商店访问、库存快照和购买事务 helper |
| `Public/RunState.h` | `FRunState`、商店状态、事件状态、战斗进度快照 |
| `Public/RunStateTypes.h` | `FCardInstance`、压力枚举、时段枚举、Zone 枚举与背包 Snapshot |
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
