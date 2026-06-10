---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-06-08
tags:
  - wacom/ui
  - wacom/battle
  - wacom/hud
---

# Wacom Battle UI 文档

> [!info] 本文职责
> 本文记录 Battle UI 当前事实：BattleHUD 命令出口、HUD state、targeting、presentation flow、Combat Log、Presentation Stack、event presentation helper、开发诊断入口、共享状态控件和测试入口。

> [!warning] 边界
> 本文只记录 UI 表现和命令出口。战斗规则真相见 [WacomBattle.md](./WacomBattle.md)，场景 target authoring 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，WBP 槽位见 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md)。

## §1 BattleHUD 职责

`UBattleHUD` 是战斗 UI Screen coordinator。它保留 WBP 绑定、CommonUI 生命周期、`SetSession()`、Snapshot fanout、玩家意图 public 入口、BattleEnd 广播、配置值和 GC 引用。

`UBattleHUD` 不直接实现战斗规则。玩家命令、目标选择、事件消费、日志、击倒弹窗、表现队列、场景敌人、first-person hand 和卡牌详情运行时状态，分别收口到 `WacomApp/Private/UI/Battle` 的 flow / coordinator / controller helper。

BattleHUD 和表现层读取敌人状态时只使用 `FBattleSnapshot.Enemies`。Targeting flow、Combat Log、Debug HUD 和 SceneEnemyHost bridge 都按 enemy slot 遍历部位；不再存在 `Snapshot.Enemy` 第一敌人兼容别名，也不再暴露“Primary scene enemy host”作为 HUD 绑定入口。

| 区域 | 当前 owner | 边界 |
|---|---|---|
| 命令提交 | `FWacomBattleHUDCommandFlow` | 把玩家意图转为 BattleSession command，不写规则细节 |
| 目标选择 | `FWacomBattleHUDTargetingFlow` | 维护 TargetSelect UI state 和点击入口 |
| 事件消费 | `FWacomBattleHUDEventFlow` | 消费 `UBattleSession::ConsumeEvents()` 并 fanout |
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Trigger Host registry 的 PartActor bridge 和 cue |
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier |
| Combat Log | `FWacomBattleHUDCombatLogController` | history、trim、feed sync、readable log |
| First-person hand | `FWacomBattleHUDFirstPersonHandBridge` | runtime hand、drag preview/release、transition hint |
| Card Detail | `FWacomBattleHUDCardDetailController` | first-person viewport 详情 motion / source guard |

## §2 命令与 HUD State

玩家意图入口归到 `Wacom|Battle|Commands`：

- `OnCardClickedByUser`
- `OnEnemyPartClickedByUser`
- `OnWaitRequested`
- `OnEndTurnRequested`
- `CancelTargetSelect`
- `OnKnockdownChoiceSelected`

HUD 是命令出口。子 Widget 和 WBP 不直接修改 `UBattleSession`，也不在图里实现出牌、等待、结束回合、击倒选择或目标选择规则。

HUD 状态入口：

| 分类 | 内容 |
|---|---|
| `Wacom|Battle|HUD State` | `GetUIState`、`BP_OnUIStateChanged` |
| `Wacom|Battle|Targeting` | TargetSelect 查询、pending card、target selection view |
| `Wacom|Battle|Presentation Flow` | presentation busy、command gate、pending turn-boundary 查询 |

`UWacomBattleWidgetBase` 是 Battle UI 基类，只负责 `UBattleSession` 引用、Snapshot fanout 和 WBP 表现刷新钩子。`SetSession / GetSession` 属于 `Wacom|Battle|Widget Session`，`BP_OnRefreshedFromSnapshot` 属于 `Wacom|Battle|Snapshot Refresh`。

## §3 Combat Log 与 Presentation Stack

战斗反馈分四层：

| 层 | 类型 | 用途 |
|---|---|---|
| Presentation coordinator | `FWacomBattleHUDPresentationCoordinator` | TargetCue、短暂停顿、击倒 modal、BattleEnd signal、card stack boundary |
| Presentation Stack | `UBattlePresentationStackWidget` | 已提交但表现仍在追赶的卡牌小堆叠 |
| Combat Log | `UBattleCombatLogFeedWidget + UWacomBattleCombatLogBuilder` | 玩家可读命令块和事件 detail line |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 是当前正式 BattleHUD 玩家日志命令块 Builder。它把一次成功 HUD command 后消费到的事件批次聚合成 `FWacomBattleCombatLogBlockView`。规则层不新增 batch id；当前 UI 事实是一次成功 HUD 命令后的事件批次就是一个 combat log block。

`UBattleCombatLogFeedWidget` 是 BattleHUD 内部常驻滚动记录，默认承接本场最近玩家可读命令块。`UBattleCombatLogBlockWidget` 显示单个命令块和 detail line。正式 Details 配置位于 `Wacom|Battle|Combat Log|Authoring`。

`UBattlePresentationStackWidget` 是只读小卡表现 backlog，不是规则栈或交互入口。它用完整 `UWacomCardView` 作为 mini card，显示已提交但表现边界尚未释放的卡牌。正式 Details 配置位于 `Wacom|Battle|Presentation Stack|Authoring`。

Wait / EndTurn 请求遇到表现栈未清空时会进入 pending turn-boundary；ActionPanel 显示 pending 文案并禁用按钮，coordinator 等 stack 和 queue 清空后再提交等待或结束回合。

## §4 Event Presentation Helper

`UWacomBattleEventPresentationBuilder`、`FBattleEventPresentationView` 和 `EWacomBattleEventVisualTone` 是 UI-only 单事件展示词汇。它们被 Combat Log detail line 复用，用于生成玩家可读中文文案、tone 和 icon；新的 BattleHUD WBP 不应直接消费 raw `FBattleEvent`。

分类口径：

| 类型 | 分类 | 语义 |
|---|---|---|
| `BuildEventPresentationView / FormatEventForPlayer` | `Wacom|Battle|Event Presentation` | 单条事件展示文案 |
| `FormatCardName / FormatStatusName / FormatKnockdownChoice / FormatHandLimitDiscardSource` | shared text helper | 只生成 UI 文案，不写规则状态 |

## §5 Debug / Development UI

以下公开面只作为调试或开发验证入口，不作为新的正式 BattleHUD 制作主线。

| 区域 | 类型 | 当前口径 |
|---|---|---|
| Debug text HUD | `UDebugBattleHUD` | Snapshot 文本诊断 HUD，不是正式 BattleHUD 父类 |

旧 Battle event log drawer / entry / toast 的 `UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast` 已删除；正式日志只走 `CombatLogFeed + BattleCombatLogBlock`。旧 2D hand 的 `UHandPanel / UCardWidget / WBP_HandPanel / WBP_CardWidget` 已删除。旧敌方 2D fallback 的 `UEnemyInfoBar / UEnemyPartWidget` 也已删除；缺少 `SceneEnemyHost` 时不会再显示 2D 敌人条，只会缺少场景敌人 hover / prediction / cue / 拖卡目标绑定。正式战斗手牌主线是 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §6 Scene Enemy UI

敌人常驻状态阅读已经收敛到敌人 Host 头顶的聚合面板：`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 承载 `UWacomBattleEnemyPanelWidget`，HUD 只从 `FBattleSnapshot.Enemies` 向场景敌人派发只读 view data。面板内部用 `UWacomBattleEnemyPartEntryWidget` 渲染每个部位的 HP、护盾、先机、意图和状态。普通部位 hover 也复用所属敌人的聚合面板响应；`AWacomBattleEnemyPartActor` / `UWacomBattleEnemyPartPresentationComponent` 不再创建部位级常驻状态 Badge。

场景敌人视觉绑定正式入口是 `ABattleTriggerActor.SceneEnemyHostSlots + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；规则敌人列表由 `ABattleTriggerActor.EncounterDefinition` 转换成 `FBattleInitParams.EnemySlots`。新制作应把敌人做成 Host 蓝图 prefab：在 Host 蓝图视口中通过子 Actor / ChildActorComponent 摆放 `AWacomBattleEnemyPartActor`，然后在 Trigger 选好 `EncounterDefinition` 后执行 `SyncSceneEnemyHostSlotsFromEncounter()`，再在生成的 `SceneEnemyHostSlots` 中按 `EnemySlotId` 绑定对应 Host。

Trigger 显式 `SceneEnemyHostSlots.EnemySlotId` 必须填写且不重复，并对应 `EncounterDefinition.EnemySlots[].EnemySlotId`。配置 `EncounterDefinition` 的正式 Trigger 必须用 `SceneEnemyHostSlots` 覆盖每个有效 EnemySlotId；缺 Host、漏映射或多余 EnemySlotId 都是编辑器验证错误。进入战斗时 GameMode 把当前 Trigger 的 Host 列表传给 BattleHUD，HUD 只同步当前 Host registry 中扫描到的 PartActor bridge。HUD registry 是 Host 列表，不维护“主 Host”兼容缓存。

敌人视觉有两条正式制作模式：

- 普通小怪推荐走 Host 整体视觉：在 `AWacomBattleEnemyActor` 的 `Presentation|Host Visual` 配整体 `PaperSprite` 或 `PaperFlipbook`，Head / Body / Tail 等子 `AWacomBattleEnemyPartActor` 只负责 `PartId / PartSlotId / HitBounds`、target bridge、Presentation、预测和状态 Badge。PartActor 没有 `VisualLayers` 时进入 `HitOnly` 视觉模式，自身不生成可见体；反馈、Badge 和预测仍跟随该部位自身的 Presentation。
- 精英 / Boss 走 PartActor 独立 `VisualLayers`：PartActor 是单个规则部位的表现容器。`HitBounds` 是唯一 hover、点击和拖卡命中范围；`VisualLayers` 只负责多张图的相对位置、旋转、缩放、排序、颜色、显隐和轻量序列帧播放。每层用 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`。`VisualLayers` 非空时优先于 Host 的 HitOnly 语境；为空且没有 Host 整体视觉时进入 `None / MissingVisualResource` 诊断。

Host 整体视觉的 `HostVisualMaterialOverride / bHostVisualCastShadow` 和每个 VisualLayer 的 `MaterialOverride / bCastShadow` 会应用到动态生成的 `UPaperSpriteComponent / UPaperFlipbookComponent`。需要 Sprite 投射阴影时，材质覆盖应使用 Paper2D 的 `MaskedLitSpriteMaterial` 或等效 lit masked 材质，并确保场景光源开启阴影；默认字段为空 / false 时保持无材质覆盖、无投影。

Host 整体视觉和 PartActor `VisualLayers` 的生成组件会在构造 / Details 刷新 / 显式刷新时重建，并在 PIE / runtime `BeginPlay` 再刷新一次，保证蓝图视口和实际运行一致。生成的 PaperSprite / PaperFlipbook 组件只在 Actor 拥有有效 `World` 时注册；蓝图模板、CDO 或 Details 预览这类无 World 场景只更新配置和诊断缓存。

Details 制作分组按主路径整理：Host 的 `Identity` 配置 `EnemyDefinition / EnemySlotId`，`Presentation|Host Visual` 配普通怪整体 sprite / flipbook，`Presentation|Badge Layout` 配部位 Badge 错开，`Authoring` 提供刷新按钮，`Authoring Status` 显示只读制作诊断缓存，`Debug Sample` 只放开发样例按钮。PartActor 的 `Identity` 配置 `PartId / PartSlotId`，`Collision` 配置 `HitBoundsExtent`；普通怪 PartActor 可以不配独立视觉，精英 / Boss 正式美术走 `Visual Layers`。PartActor 不再提供旧 StaticMesh 原型可见体入口。

Host 和 PartActor 的 Details `Authoring Status`、debug view 和 summary 使用同一套制作诊断事实。Host `AuthoringState=Ready` 表示 `EnemyDefinition`、子 PartActor、`PartId` 和 `PartSlotId` 对齐；常见异常包括 `MissingEnemyDefinition`、`NoPartActors`、`DuplicatePartSlotIds`、`PartSlotMismatch` 和 `PartDefinitionMismatch`。PartActor `AuthoringState` 会区分 `UsingVisualLayers`、`HitOnly`、`MissingIdentity`、`InvalidHitBounds` 和 `MissingVisualResource`，并通过 `VisualAuthoringMode=VisualLayers / HitOnly / None` 显示当前视觉路径。排查编辑器配置时先看 Details `Authoring Status`；需要完整一行串时再执行 Host / PartActor 的 `LogBattleSceneEnemyDebugSummary()`。

Host 整体视觉和 `VisualLayers` 都是表现层合同，不接管动画状态机。普通小怪可以先用 Host `StaticSprite` 或 Host `Flipbook` 做整体 idle；精英 / Boss 推荐用 PartActor `StaticSprite` 做主体、阴影、前景遮挡，用 PartActor `Flipbook` 做尾巴摆动、眼睛眨动或局部表现。`SortOrder` 映射到 `TranslucentSortPriority`，数值越大越靠前。`HitBounds` 仍是唯一命中范围，sprite / flipbook 的尺寸和透明区域不改变目标身份或 BattleSession 规则。

蛇 Host prefab 的第一版正式口径是：创建 `AWacomBattleEnemyActor` 蓝图 prefab，推荐路径 `/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug`，Host `EnemyDefinition=/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake`、`EnemySlotId=Enemy`；普通蛇美术优先在 Host 的 `Presentation|Host Visual` 配整体 sprite / flipbook。Host 蓝图视口内放置三个 ChildActorComponent，Child Actor Class 都是 `AWacomBattleEnemyPartActor`：

| ChildActor | Relative Location | PartId | PartSlotId | HitBoundsExtent |
|---|---:|---|---|---:|
| `SnakeHeadPart` | `(96,-6,16)` | `Snake.Head` | `Head` | `(42,38,42)` |
| `SnakeBodyPart` | `(0,0,0)` | `Snake.Body` | `Body` | `(62,46,42)` |
| `SnakeTailPart` | `(-92,16,-8)` | `Snake.Tail` | `Tail` | `(48,34,34)` |

`EnemySlotId` 由 Host / Trigger 注入，不在 PartActor 模板里手填。Host validation 会同时检查 `PartId` 与 `PartSlotId`：`PartId` 必须对应 `UEnemyPartDefinition::PartId`，`PartSlotId` 必须对应 `UEnemyDefinition.Parts[].PartSlotId`。蛇的正式绑定身份是 `Enemy + Head/Body/Tail`，不是 `Enemy + Snake.Head/Snake.Body/Snake.Tail`。

Host 的 `RefreshBattleEnemyPartAuthoringState()` 是安全刷新入口：它刷新 Host visual，扫描自身 PartActor，并同步 `EnemySlotId`、Host visual 语境、Badge 诊断和 debug summary。它不会根据 `EnemyDefinition`、Actor 名称或组件名称自动补齐 `PartId / PartSlotId`；缺失或填错的身份必须通过手动配置、蓝图子类默认值或显式样例按钮修正，并会通过 `Authoring Status` 和 Validate Map 暴露。

Trigger 正式单蛇配置使用生成资产 `DA_Encounter_SnakeSingle`：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`；执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。`EncounterDefinition` 正式入口不能缺 Host 映射。

`ConfigureDebugSnakeHostSample()` 是制作开发辅助入口：先在 Host 蓝图中放好 Head / Body / Tail 三个 PartActor，再执行该函数。Host 会扫描运行时子 Actor 和蓝图 ChildActorComponent 的子 Actor 模板，尝试写入上述蛇样例配置。它不自动创建缺失部位，也不创建正式美术资产；普通蛇的正式美术由 Host `Presentation|Host Visual` 配整体图，精英 / Boss 才优先在各 PartActor 配 `VisualLayers`。

Host 蓝图视口中的 `SnakeHeadPart / SnakeBodyPart / SnakeTailPart` 是 `ChildActorComponent`；关卡 Outliner 会在 Host 实例下显示它们生成出来的 `WacomBattleEnemyPartActor...` 子 Actor，这是正常现象。运行时和摆放校验优先使用这些已生成的真实子 Actor；只有蓝图模板 / CDO 等没有生成实例的场景才读取 ChildActor 模板，避免同一组 Head / Body / Tail 被重复计入 Host registry。

每个 PartActor 默认携带两类运行时组件：

- `UWacomBattleEnemyPartWorldTargetBridgeComponent`：只负责 `EncounterId / EnemySlotId / PartSlotId -> PartInstanceId` 绑定、当前 HUD registry 标记、targetable 绑定诊断，以及把运行时身份写入 `UWacomInteractionTargetComponent`。
- `UWacomBattleEnemyPartPresentationComponent`：负责 hover target、TargetSelect 可选提示、first-person drag preview、`TargetConfirmed / DamageDealt / EnemyPartHpEmptied` cue、只读预测 Widget 和常驻状态 Badge。

BattleHUD scene enemy coordinator 成对缓存 Bridge 和 Presentation：target handle 解析走 Bridge，表现 target 注册和反馈走 Presentation。target handle 必须携带完整 `EncounterId + EnemySlotId + PartSlotId` 才能命中当前 registry；`SourceObject` 即使指向当前 PartActor，也不会替代或修正错误 key。Host / PartActor debug summary 会合并两者事实用于 PIE 排查，但 passive UI 和 BattleSession 不依赖这个合并 debug。

BattleHUD 不再构建或绑定敌方 2D fallback；点击、hover、drag target handle 全部通过当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge 完成。`EncounterDefinition` 正式入口缺 Host 会被编辑器验证阻止。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、ActionPanel、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。轻点、hold inspect、drag/aim、world target release 和 hand-card target release 都经 BattleHUD bridge / command flow 进入 BattleSession。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

`FirstPersonCardDetailViewportZOrder / FirstPersonCardDetailAnchorBaseSize` 属于 `Wacom|Battle|First Person Card Layer|Authoring`。第一人称战斗手牌交互开关使用 `bEnableBattleHandInteraction`、`SetBattleHandInteractionEnabled()` 和 `IsBattleHandInteractionEnabled()`。

## §8 Battle Shared Widgets

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / San Snapshot |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用“标签 + 数量”显示控件；BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数` |
| `UWacomProgressBar` | `Wacom|Common UI|Progress Bar` | 通用数值进度条显示控件 |

BattleHUD 自身配置分类：

| 配置 | 分类 |
|---|---|
| C++ 默认 canvas layout | `Wacom|Battle|HUD Default Layout` |
| card detail authoring | `Wacom|Battle|Card Detail|Authoring` |
| card detail motion | `Wacom|Battle|Card Detail|Motion` |
| combat log max blocks | `Wacom|Battle|Combat Log|Authoring` |
| presentation stack hold | `Wacom|Battle|Presentation Stack|Authoring` |

## §9 Battle UI 测试入口

Battle UI 回归优先使用 `Source/WacomTests/Private/UI/BattleHUDTestHarness.h` 中的 `FWacomBattleHUDTestHarness` 搭配 `UWacomBattleHUDDetailTest` 装配 HUD、PlayerController、CombatLogFeed、PresentationStack、ActionPanel、first-person character 和 scene enemy Host。

测试不 include BattleHUD 私有 helper header，也不为生产 HUD 增加 Blueprint-visible 测试 API。只读诊断通过 `FWacomBattleHUDAutomationTestView` 聚合；Battle scene target click / probe 通过 `FWacomBattleSceneTargetClickTestAccess` 驱动。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
