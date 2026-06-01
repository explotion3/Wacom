---
type: domain-spec
scope: wacom-run
status: active
updated: 2026-06-01
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

### 探索移动模型

Run 探索期的正式玩家移动模型是 Run Tunnel：鼠标可见，`W/S` 沿当前 tunnel segment 的 Spline 前后推进，鼠标位置驱动受限 yaw / pitch 视角，并可点击场景目标。普通隐藏鼠标的 FPS FreeLook 不再是正式玩家路径；如以后需要，只能作为 editor/debug-only 工具单独设计。

`UWacomRunTunnelMovementComponent` 属于 `WacomApp`，负责 Pawn / Camera 对齐、输入消费和场景表现移动，不写入 Run 规则状态。鼠标位置到镜头 offset 的计算由共享 `UWacomCursorLookDriverComponent` 承担，Run Tunnel 只把这个 offset 叠加到 Spline base rotation 上。`WacomRun` 仍只维护战外规则真相，例如时段、节点、背包和战斗回传结算。当前关卡中的 `AWacomRunTunnelSegmentActor::bAutoActivateOnBeginPlay` 只是 authoring/bootstrap 入口；正式 Run flow 后续应由探索流程选择起始 Segment。

后续 Run 卡牌交互应复用 first-person card layer：卡牌仍用 HUD / UMG 渲染保证清晰和动态材质稳定，但布局由第一人称 card anchor 投影到屏幕，形成跟随玩家身体 / tunnel 前进的手牌感。Run 规则层不依赖该表现系统；设计讨论见 `Docs/First_Person_Card_Layer_Design.md`。

V0-AK 后，探索期可以由 `WacomApp` 的 `UWacomRunFirstPersonCardSourceComponent` 把 `URunSession::BuildBackpackStorageSnapshot()` 中的 `BattleDeckPhysicalCards` 和可选 `BattleDeckProjectedCards` 写入 `UWacomFirstPersonCardAnchorComponent` 的 runtime source `RunFirstPersonBattleDeck`。entry 使用 Run card instance id 和卡面 ViewData，让玩家在探索中看到备战卡组；Run 规则层不依赖这层表现。V0-AL 后，普通 GameMenu 打开时会默认压制该展示，避免遮挡背包、暂停、商店或事件界面；需要菜单内卡牌交互时，由菜单显式申请 menu source lease 临时显示候选卡。V0-AM 后，active menu lease 允许第一人称卡牌进入 hold / drag probe，并可命中菜单 UMG 的 `TargetKind=Zone` drop target。V0-AN 后菜单推荐提交 `FWacomRunMenuCardLeaseRequest`，由 App 层从玩家当前真实物理持有区收集候选卡实例：`Backpack -> BattleDeck -> BurdenZone -> SpecialZones.Cards`。该 provider 不读取 `BattleDeckProjectedCards`，避免 SpecialZone 入战投影和物理卡重复；Definition/CardId 身份筛选为 OR，显式 InstanceId、RequiredKeywords、BlockedKeywords 与身份筛选共同生效。空筛选默认拒绝，除非菜单明确开启“允许所有持有卡”。V0-AO 后菜单 Zone release 可走 `ResolveRunMenuCardDropIntent()`：默认测试菜单仍可由 PlayerController 调用 `ValidateDestroyCardByInstance()` / `DestroyCardByInstance()` 永久移除具体持有卡，作为独立 prototype 验证。V0-AQ 后 RunEventScreen 使用 `MenuHandled` submit policy 接管该 release：PlayerController 只负责命中、preview 和分发，真正提交由 `URunSession::ChooseRunEventOptionWithPaidCardResult()` 在 RunEvent 事务内完成，提交成功或失败都会回填到 drop result。V0-BV 后，在没有 active GameMenu / menu lease 时，探索期第一人称卡牌拖拽还可以命中 `Interaction.Target.Run.Object` 场景目标，并通过 Run world card drop receiver 提交 KeyChest prototype 事务；菜单 lease 仍优先于场景拖卡。V0-CB 后，场景拖卡 preview 仍只做有效 / 无效轻反馈，只有 release 命中过 Run world target 且提交失败时才由 App 层显示失败 Toast；文案由目标 receiver 的通用 failure prompt contract 提供，PlayerController 只保留无 receiver 时的配置异常 fallback。进入战斗时 GameMode / PlayerController 会清理该 Run source 和 lease，退出战斗回到 Exploration 后再刷新默认 BattleDeck。

V0-AJ 后，Run / 探索场景 Actor 可以通过 `UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 暴露为 `FWacomInteractionTargetHandle(TargetKind=World, TargetTag=Interaction.Target.Run.Object)`。该 handle 现在服务三类入口：鼠标 hover / click 的世界交互物识别、Run world card drop 的目标识别、轻量 scale / CustomDepth preview 和 debug。规则提交仍必须回到明确领域出口：普通点击 / E 键走 `IWacomWorldInteractable::TryInteract()`，V0-BU 的拖卡到宝箱走 `URunSession::SubmitRunWorldCardInteraction()`。

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

玩家已拥有卡的操作以 `InstanceId` 为主。`DestroyCardByInstance()` / `ValidateDestroyCardByInstance()`、`DeleteCardForGoldByInstance()`、`MoveInstance()` 等入口用于 UI 和交互层提交某一张具体卡。

旧 Definition 级入口 `DestroyCardFromBackpack()`、`DeleteCardForGold()`、`AddCardToBattleDeck()`、`RemoveCardFromBattleDeck()` 只作为 C++ 兼容入口和资产语义桥保留，不再 Blueprint 暴露：它们会在对应来源范围内删除或移动第一张匹配 Definition 的 instance。UI、蓝图玩家操作和交互层必须使用 `InstanceId` 入口，不能用 Definition 指代某张已拥有卡。

RunEvent / DataAsset 仍可用 Definition 表达“获得一张某种卡”或“交出一张某种卡”，因为这些是资产语义，不指向玩家当前拥有的某个具体实例。

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

当前统一规则由 `Private/Deck/RunDeckRules.*` 承接。历史 public API `DestroyCardFromBackpack()` 保留旧名兼容 C++ 调用点和测试，不再 Blueprint 暴露；Blueprint / UI 玩家操作必须使用 `DestroyCardByInstance()` 等 InstanceId 入口。该兼容入口实际会按固定顺序搜索所有玩家拥有区：`Backpack -> BattleDeck -> BurdenZone -> SpecialZones`。

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

V0-BT 后，场景 ShopTrigger 的 Validate Map/Level 按运行时口径校验商品来源：优先 `ShopDefinition.Offers`，未配置 Definition 时使用手工 `Offers` 兼容入口。缺 `PersistentId`、解析后没有可用商品、商品缺卡或负价格是 error；重复 `PersistentId` 是 warning，因为它会共享库存和购买状态。

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
- 当前 Run 标记已设置 / 未设置。

当前效果：

- 获得卡牌。
- 增减金币，最低不低于 0。
- 增减压力。
- 消耗节点并可能推进时段。
- 从玩家任意持有区永久移除一张卡。
- 标记指定 `PersistentId` 事件完成。
- 设置 / 清除当前 Run 标记。

选项 `Effects` 按事务执行：任一效果失败时，本次选项不提交已执行的前置效果，且不改变节点 / 时间 / 卡牌 / 金币 / 压力 / 事件 active 或 completed 状态；失败结果不返回部分 `EffectResults`。

RunEvent 的移除卡搜索四个物理持有区：`Backpack`、`BattleDeck`、`BurdenZone` 和所有 `SpecialZones.Cards`。它不发金币，但遵守固有卡、最后容量来源卡和 Companion 嗜血规则。

V0-BA 后，RunEvent 支持第一版 RunFlag 事件记忆：`RunFlagSet / RunFlagNotSet` 条件和 `SetRunFlag / ClearRunFlag` 效果都使用 `FlagId`。RunFlag 存在 `FRunState::RunFlags`，只表示当前 Run 内存态的 bool/set 语义；它不是 GameplayTag，不是数值变量，也不写入 SaveGame。`RunFlagSet` 缺失时失败 reason 为 `RequiredRunFlagMissing`，`RunFlagNotSet` 命中已设置标记时失败 reason 为 `BlockedRunFlagSet`，配置缺少 `FlagId` 时失败 reason 为 `MissingRunFlagId`。`SetRunFlag / ClearRunFlag` 在 RunEvent working-state 事务内执行；后续 effect 失败时会和其他前置效果一起回滚。

RunEvent 选项可以配置 `CardPayment` 要求玩家拖入一张真实持有卡支付。运行时 `FRunEventChoiceSnapshot` 会给 UI 暴露 `bRequiresOwnedCardPayment`、`PaymentZoneId`、候选 `InstanceId` 列表和支付不可用原因；`PaymentZoneId` 为空时解析为 `RunEvent.Pay.{ChoiceId}`。支付筛选读取 Definition / CardId / RequiredKeywords / BlockedKeywords，空筛选非法，本轮不支持“交任意卡”。

V0-AX 后，`FRunEventChoiceSnapshot::Requirements` 会按选项条件生成结构化需求快照，并为 `CardPayment` 追加支付需求项。需求项只记录规则事实：需求类型、是否满足、首个禁用原因、所需/当前数值、压力类型、卡牌 Definition、目标事件 PersistentId、RunFlag `FlagId`、支付候选数量等。`bAvailable / DisabledReason` 仍保留“首个失败原因”语义，提交校验和事务流程不因此改变；中文展示文案由 `WacomApp` presentation 生成，不写入 `WacomRun`。

V0-AY 后，`FRunEventChoiceSnapshot::Consequences` 会按选项 Effects 和 outcome 生成结构化后果预览。预览采用“配置意图”口径，只记录 effect 类型、卡牌、数值、压力类型、目标事件、RunFlag `FlagId`、跳转节点和节点标题等事实；它不模拟金币 clamp、行动点跨时段、副作用压力或后续效果失败。真实结果仍以提交后的 `FRunEventChoiceResult` 和事务状态为准。卡牌支付本身不进入 consequence list，继续由支付字段和 App presentation 的支付状态行表达。

V0-AZ 后，编辑器校验会把会导致规则或预览失真的配置作为 error，把“资产可运行但预览不清晰”的制作问题作为 warning。`AddGold / AddPressure / ConsumeNode` 的 `Value=0` 会给 warning，因为不会产生可见 consequence preview 或有意义的实际变化；`NextNodeId` 与关闭 / 完成事件同时配置时也会给 warning，因为事件结束预览优先，不会显示节点跳转预览。V0-BA 后，RunFlag 条件 / 效果缺少 `FlagId` 是 error；负数 `AddGold` 没有 `MinGold` 条件会给 warning，因为实际结算会 clamp 到 0；负数 `AddGold` 总扣费和最大 `MinGold` 门槛不一致也会给 warning，提示门槛和扣费可能不一致。

支付选项只能通过 `ChooseRunEventOptionWithPaidCardResult(ChoiceId, PaidCardInstanceId)` 提交。普通 `ChooseRunEventOptionWithResult()` 会以 `RequiresCardPayment` 拒绝，避免点击按钮时悄悄按 Definition 删除一张卡。支付提交流程会校验 active event、choice 条件、卡实例归属、筛选命中和永久移除保护；随后在 working state 中先移除该精确 instance，再执行 choice effects、节点跳转、关闭或完成标记。任一步失败都会整体回滚。支付选项禁止同时配置 `RemoveCard` effect，避免拖卡支付后又按 Definition 再删一张。

卡牌支付选项制作 checklist：

- 配置 `ChoiceId`；如果 `PaymentZoneId` 留空，运行时和编辑器校验都依赖它生成 `RunEvent.Pay.{ChoiceId}`。
- 配置至少一种支付筛选：`AllowedCardDefinitions`、`AllowedCardIds`、`RequiredKeywords` 或 `BlockedKeywords`；空筛选非法。
- 同一节点内每个支付选项的解析后 `PaymentZoneId` 必须唯一。
- 不要在支付选项里同时配置 `RemoveCard` effect；拖卡支付已经会永久移除精确 instance。
- 支付成功后的结果用 `NextNodeId / bCloseEventAfterResolve / bMarkEventCompleted / Effects` 表达，不在 UI 或菜单里手动推进事件。

金币门槛 + 扣金币 + 奖励的制作口径仍使用现有组合，不新增 `PayGold` 类型：

- 用 `MinGold=N` 表示至少需要 N 金币。
- 用 `AddGold=-N` 表示支付 N 金币；Run 规则会把金币下限 clamp 到 0。
- 支付后的奖励继续用 `GainCard / AddPressure / SetRunFlag / MarkEventCompleted / NextNodeId` 等现有 effect 和 outcome 表达。
- 建议 `MinGold` 最大值和负数 `AddGold` 总扣费保持一致；不一致时资产仍可运行，但会给 authoring warning，避免玩家看到的门槛和实际扣费不一致。

V0-AT 后编辑器 RunEvent Data Validation 会在支付相关错误中明确指出 `NodeId / ChoiceId / PaymentZoneId / NextNodeId`，便于定位资产配置问题。V0-AZ 后条件 / 效果错误也会带 `ConditionIndex / EffectIndex`，并通过 validation report 分离 `Errors / Warnings`；旧 `Validate(Event, OutErrors)` 兼容入口仍只返回阻断错误。`DA_Event_DebugSnakeGift` 是当前标准单卡支付样例：`HandOverFang` 使用 `CardPayment + AllowedCardDefinitions=PoisonFang + NextNodeId=End`，效果只保留 `ConsumeNode`，不配置 `RemoveCard`。

V0-BB 后，`DA_Event_DebugFlagReward` 是标准 RunFlag + 金币门槛奖励样例，和蛇巢卡牌支付样例分开维护。该事件使用 `DebugFlagReward.Inspected / GoldGranted / RewardClaimed` 三个 RunFlag：`InspectMark` 设置调查标记，`DebugGrantGold` 用于 PIE 自助获得 3 金币，`ClaimGoldReward` 用 `RunFlagSet(Inspected) + RunFlagNotSet(RewardClaimed) + MinGold(3)` 解锁 `AddGold(-3) + GainCard(PoisonFang) + SetRunFlag(RewardClaimed)` 并跳转到 `Rewarded`，`ResetFlags` 会清掉三个 Debug flag 并回到 `Start`。它用于制作和验证 `MinGold + AddGold(-N) + 奖励 + RunFlag` 的组合，不新增 `PayGold` 规则，也不改变关闭 / 跳转流程。

V0-BK 后，Run world 金币拾取物使用 `URunSession::CollectGoldPickup(PersistentId, GoldAmount)` 结算。该入口要求 `PersistentId != NAME_None` 且 `GoldAmount > 0`，成功时同一事务内增加 `Gold` 并把 ID 写入 `FRunState::CollectedPickupIds`，失败或重复提交不改状态、不广播。V0-BL 后，`AWacomRunPickupActor` 会在制作诊断中报告缺 ID、非正金币和同关卡重复 ID；重复 ID 不改变规则语义，仍表示这些 Pickup 共享同一个已拾取 key。

V0-BM 后，Run world 卡牌拾取物使用 `URunSession::CollectCardPickup(PersistentId, CardDefinition)` 结算。该入口要求 `PersistentId != NAME_None` 且 `CardDefinition != nullptr`；成功时复用 `AcquireCardToRunInternal()` 获得一张固定卡牌，重算负重 / SpecialZone，把 ID 写入 `CollectedPickupIds`，最后只广播一次。金币和卡牌 Pickup 共用同一个 `CollectedPickupIds`：相同 ID 无论来自金币还是卡牌拾取，都会共享已拾取状态，避免 E 键和远距离点击路径重复结算。当前 Pickup 状态仍只保存在 Run 内存态，不接 SaveGame；V1 不支持掉落表、多卡、区域选择或拾取动画。

V0-BN 后，金币 / 卡牌 Pickup 的世界交互壳抽到 `AWacomRunPickupActorBase`，但 Run 规则入口不变。Base 只管理 `PersistentId`、E 键 / click / hover、视觉 probe、已拾取生命周期和跨 Pickup 类型重复 ID 诊断；金币和卡牌子类仍分别调用 `CollectGoldPickup()` 与 `CollectCardPickup()`。后续掉落表、多卡奖励或 SaveGame 不应直接塞进 Base，而应先定义新的奖励合同或持久化策略。

V0-BO 后，Pickup 的制作打磨仍不改变 Run 规则：金币 / 卡牌样例配置按钮共用 `AWacomRunPickupActorBase` 的 authoring helper，只刷新当前 Actor 的 `PersistentId / TriggerRadius / prompt / bDestroyWhenCollected`、碰撞组件和 click stable id，不触碰 `FRunState`。推荐关卡摆放使用 `BP_WacomRunPickupActor` / `BP_WacomRunCardPickupActor` 作为外观默认资产；C++ 父类仍是结算和交互合同真相。

V0-BP 后，正式内容可以逐步使用 `AWacomRunRewardPickupActor + UWacomRunPickupDefinition` 做数据驱动拾取物。Definition 只描述固定单一奖励：金币或一张固定卡；运行时结算仍调用现有 `CollectGoldPickup()` / `CollectCardPickup()`，所以 `CollectedPickupIds`、重复提交拒绝、金币 / 卡牌获得语义都不变。场景 Actor 的 `PersistentId` 仍是当前 Run 已拾取状态 key；`PickupDefinition.PickupId` 只是静态内容 / debug ID，不能替代 `PersistentId`。本轮不新增掉落表、多奖励、多卡、SaveGame、动画或样例 `.uasset`。

V0-BQ 后，`DA_Pickup_DebugGold3` 和 `DA_Pickup_DebugPoisonFang` 是标准 PickupDefinition 样例：前者固定获得 3 金币，后者固定获得 `PoisonFang`。它们只用于制作入口和 PIE 验证，运行时仍以摆放的 `AWacomRunRewardPickupActor.PersistentId` 写入 `CollectedPickupIds`；同一 Definition 可以被多个场景 Actor 复用，只要 `PersistentId` 不同就不会共享已拾取状态。

V0-BR 后，正式关卡推荐摆放 `BP_WacomRunRewardPickupActor` 并在实例上配置唯一 `PersistentId` 与 `UWacomRunPickupDefinition`。该 BP 默认不填 `PersistentId` 或 `PickupDefinition`，避免复制摆放时误共享运行时已拾取 key；运行时结算仍完全走 `CollectGoldPickup()` / `CollectCardPickup()` 和 `CollectedPickupIds`。

V0-BS 后，Pickup 摆放实例接入 Actor Data Validation。金币 Pickup 缺 `PersistentId` 或 `GoldAmount <= 0`、卡牌 Pickup 缺 `CardDefinition`、RewardPickup 缺 `PickupDefinition` 或 Definition 内部奖励配置无效都会成为 Validate Map/Level error；同 World 内重复 `PersistentId` 是 warning，不阻断，因为它仍表示共享同一份已拾取状态。BP 默认资产 / CDO 仍允许空配置。

V0-BU 后，探索期世界卡牌交互使用独立于 Pickup 的完成状态：`FRunState::CompletedRunWorldInteractionIds`。`URunSession::ValidateRunWorldCardInteraction()` 校验场景 `PersistentId`、精确 `SourceCardInstanceId`、Definition/CardId/RequiredKeywords/BlockedKeywords 筛选、重复完成、金币奖励和可选永久移除安全；`SubmitRunWorldCardInteraction()` 成功时可选消耗那张精确持有卡，增加固定金币奖励，写入 `CompletedRunWorldInteractionIds`，最后只广播一次。失败或重复提交不改状态、不广播。V1 原型只支持单卡筛选、可选消耗和正数金币奖励；不支持多奖励、掉落表、动画或 SaveGame。

V0-BV 后旧 `AWacomDebugChestActor` 已移除，新的 `AWacomRunKeyChestActor` 是第一条使用该事务的原型场景 Actor：默认要求拖入 `DA_Card_DebugKey`，成功后消耗钥匙、获得 3 金币，并用宝箱自身 `PersistentId` 标记已打开。普通 E 键或左键点击只提示“需要钥匙 / 宝箱已打开”，不会直接结算奖励。宝箱打开状态不复用 `CollectedPickupIds`，避免“拾取物已拾取”和“世界卡牌交互已完成”语义混在一起。V0-BW 后 KeyChest 的制作入口改为安全 facade 字段：`TriggerRadius / ClickBoundsExtent / VisualMesh / VisualScale / VisualRelativeLocation` 会同步到内部组件；内部碰撞和 receiver 组件保持隐藏，以规避 UE 5.7 Details 面板展开 Collision / BodyInstance 时的栈溢出路径。V0-BX 后 KeyChest 摆放实例接入 Actor Data Validation：缺 `PersistentId`、receiver 缺正向筛选、只填 `BlockedKeywords`、`GoldReward <= 0` 都是 Validate Map/Level error，重复 `PersistentId` 是 warning；正向筛选口径是 `AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 任一非空。V0-CE 后正式推荐填写 `UWacomRunWorldCardInteractionDefinition` 到 `CardInteractionDefinition`，并可直接使用 `WacomRegenerateContent` 生成的 `/Game/Wacom/Data/Interactions/DA_RunWorldCardInteraction_DebugKeyGold3`。它会驱动 receiver 的卡牌筛选、金币、消耗和 preview/success/completed/rejected/config/source/generic 文案；`InteractionId` 只用于内容识别、debug 和 validation，不替代场景 `PersistentId`。旧 `UWacomRunKeyChestDefinition` / `/Game/Wacom/Data/KeyChests/DA_KeyChest_DebugKeyGold3` 仅作为遗留数据内容暂存，`AWacomRunKeyChestActor` 不再暴露或读取 `ChestDefinition`；KeyChest 配置优先级收束为 `CardInteractionDefinition` > 手填 receiver fallback。V0-BZ 后 KeyChest 的已打开反馈仍是 App 层 facade：Actor 监听当前 `URunSession::OnRunStateChangedNative`，发现 `CompletedRunWorldInteractionIds` 命中自身 `PersistentId` 后切到 `CompletedVisualMesh / CompletedVisualScale / CompletedVisualRelativeLocation`；这不新增 Run 规则字段，不改 SaveGame，也不改变重复拖卡由 `SubmitRunWorldCardInteraction()` 拒绝的语义。完成后保留 `ClickBounds / TriggerSphere` 命中，只用于 hover、E 键和普通左键显示已打开提示。V0-CB/V0-CD 后 release 失败反馈仍属于 App 表现层，不改变 `ValidateRunWorldCardInteraction()` / `SubmitRunWorldCardInteraction()` 的规则：错卡等筛选失败显示 receiver rejected prompt，已完成显示 receiver completed prompt，配置异常显示 receiver config warning prompt + reason，成功仍只显示金币 Toast。后续其他世界卡牌交互目标应复用 `UWacomRunWorldCardDropReceiverComponent + UWacomRunWorldCardInteractionDefinition`，不要给 PlayerController 加类型分支。

`FRunEventChoiceResult` 只表达本次选项直接效果和展示诊断字段，供 UI 和日志展示。V0-AR 后成功的卡牌支付结果会记录 `PaidCardDefinition`，仅用于 UI / 日志显示“交出了哪张卡”；它必须在移除 paid instance 前从当前持有卡读取，且不是后续规则输入。V0-AS 后成功结果还会记录 `PreviousNodeId / ResolvedNodeId / ResolvedNodeTitleText / bNodeChanged / bEventClosedAfterResolve / bEventCompletedAfterResolve`，用于 Toast 显示“进入某节点”或“事件已结束”。这些 outcome 字段只在事务成功提交后写入；失败或回滚结果不写入 paid card definition，也不写入成功 outcome。后续规则不能依赖这个结果包反向修改 RunState。

当前 `RunEventStates`、`RunFlags`、`CollectedPickupIds` 和 `CompletedRunWorldInteractionIds` 只保存在 Run 内存态，不写入 SaveGame。

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
| `AWacomRunPickupActor` | 金币拾取物已拾取状态 |
| `AWacomRunCardPickupActor` | 卡牌拾取物已拾取状态 |
| `AWacomRunRewardPickupActor` | 数据驱动金币 / 卡牌拾取物已拾取状态 |
| `AWacomRunKeyChestActor` | 世界卡牌交互完成状态 |

规则：

- V0-BT 后，RunEvent / Shop / BattleTrigger 的关卡摆放实例也会通过 Validate Map/Level 检查 `PersistentId` 和关键配置。RunEvent 缺 `EventDefinition`、Shop 没有可用商品、BattleTrigger 缺 `EnemyDef` 会报 error；重复 `PersistentId` 报 warning，不阻断。

- 参与 Run 状态的场景 Actor 必须配置非空 `PersistentId`。
- 同一关卡内应保持唯一。
- `NAME_None` 表示不参与对应状态记录；入口会 Warning 或拒绝。
- 内容资产 ID 不能替代场景 PersistentId。
- `UWacomRunWorldCardInteractionDefinition.InteractionId` 和 `UWacomRunPickupDefinition.PickupId` 都只是内容 ID，不参与当前 Run 的完成 / 已拾取状态。旧 `UWacomRunKeyChestDefinition.ChestId` 仅用于遗留资产 validation。

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
| `RunFlags` | 不保存 | 当前 Run 内存态事件标记清空 |
| `CollectedPickupIds` | 不保存 | 世界金币 / 卡牌拾取物已拾取状态清空 |
| `CompletedRunWorldInteractionIds` | 不保存 | 调试宝箱等世界卡牌交互完成状态清空 |

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
| `Private/Time/RunTimeRules.*` | 时间、节点消耗、时段推进与时段进入压力副作用的私有规则 helper；只操作 `FRunState`，不广播、不访问 UI |
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
| `Source/WacomApp/Public/Actors/WacomRunPickupActorBase.h` | Run world Pickup 共享交互壳，管理 E 键 / click / hover、组件、lifecycle 和 debug |
| `Source/WacomApp/Public/Actors/WacomRunPickupActor.h` | 金币拾取物入口，提供 `PersistentId` 和 `GoldAmount`，结算调用 `CollectGoldPickup()` |
| `Source/WacomApp/Public/Actors/WacomRunCardPickupActor.h` | 卡牌拾取物入口，提供 `PersistentId` 和固定 `CardDefinition`，结算调用 `CollectCardPickup()` |
| `Source/WacomApp/Public/Actors/WacomRunRewardPickupActor.h` | 数据驱动拾取物入口，读取 `UWacomRunPickupDefinition` 并调用现有金币 / 卡牌拾取结算 |
| `Source/WacomApp/Public/Actors/WacomRunKeyChestActor.h` | 钥匙宝箱原型入口，读取可选通用 `UWacomRunWorldCardInteractionDefinition` 或手填 receiver fallback，接收拖入钥匙卡并提交 Run world card interaction |
| `Source/WacomApp/Public/Interaction/WacomRunWorldCardDropReceiver.h` | Run world card drop 接收器合同，读取可选通用 Definition 并构建 / 校验 / 提交 `FRunWorldCardInteractionRequest` |
| `Source/WacomData/Public/Pickups/RunPickupDefinition.h` | Pickup 静态奖励定义；`PickupId` 仅用于内容识别和 debug，不是已拾取状态 key |
| `Source/WacomData/Public/Interactions/RunWorldCardInteractionDefinition.h` | 通用 Run 世界拖卡交互定义；`InteractionId` 仅用于内容识别和 debug，不是完成状态 key |
| `Source/WacomData/Public/KeyChests/RunKeyChestDefinition.h` | 遗留 KeyChest 专用交互定义，暂保留类型、validator 和生成资产；`AWacomRunKeyChestActor` 不再读取它 |

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
