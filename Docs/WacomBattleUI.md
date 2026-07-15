---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-07-14
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

`UBattleHUD` 是战斗 UI Screen façade。它保留 WBP 绑定、CommonUI 生命周期、C++ session 注入与严格入场生命周期（`BeginBattleEntryPresentation()` / `AttachInitializedBattleSession()` / `ReleaseBattleEntryPresentation()`）、Blueprint/public 命令入口、typed BattleEnd 广播（`EBattleOutcome`）、Authoring 参数和 GC-owned Widget 引用。

`UBattleHUD` 不直接实现战斗规则，也不直接持有 command、targeting、snapshot presentation、hand、scene enemy、detail 或 combat log 的业务状态。运行时状态收口到 `WacomApp/Private/UI/Battle/FWacomBattleHUDRuntime`；`FWacomBattleHUDRuntimeHost` 是唯一读取 HUD 私有 WBP / UPROPERTY / GC 引用的 adapter。

BattleHUD 和表现层读取敌人状态时只使用 `FBattleSnapshot.Enemies`。Targeting controller、Combat Log、Debug HUD 和 SceneEnemyHost bridge 都按 enemy slot 遍历部位；不再存在 `Snapshot.Enemy` 第一敌人兼容别名，也不再暴露“Primary scene enemy host”作为 HUD 绑定入口。

| 区域 | 当前 owner | 边界 |
|---|---|---|
| HUD façade / lifecycle | `UBattleHUD` | CommonUI 生命周期、WBP 绑定、Blueprint 入口、Authoring 参数、GC Widget 引用 |
| Runtime state / ownership | `FWacomBattleHUDRuntime` | HUD UI state、pending target、input gate、battle-end guard、last snapshot、controller 生命周期 |
| HUD private adapter | `FWacomBattleHUDRuntimeHost` | 唯一读取 HUD 私有 WBP / UPROPERTY / GC 引用的端口 |
| 结果应用 | `FWacomBattleHUDResultApplicator` | 初始化表现代次、入场门控、Session / version 幂等、Snapshot / Log / queue / EndTurn / hand transition 固定应用顺序 |
| Snapshot presentation | `FWacomBattleHUDSnapshotPresenter` | Snapshot 刷新顺序、pile count、battle end 清理、child fanout 前后协作 |
| 命令提交 | `FWacomBattleHUDCommandController` | 玩家意图与目标校验、命令前 Snapshot / 表现上下文、构造并提交唯一 BattleSession command；不应用结果 |
| 目标选择 | `FWacomBattleHUDTargetingController` | 维护 TargetSelect UI state、pending card 和点击入口 |
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Trigger Host registry 的 PartActor bridge 和 cue |
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier、EndTurn 与通用 command phase plan（普通弃牌 / Deck Step） |
| Combat Log | `FWacomBattleHUDCombatLogController` | history、trim、feed sync、readable log |
| First-person hand | `FWacomBattleHUDFirstPersonHandBridge + FWacomBattleFirstPersonDropResolver + FWacomBattleHandPresentationController + FWacomFirstPersonCardLayerPresentationFrame` | runtime hand presentation frame、drag preview/release、drop intent / hand-card affordance 解析、Drawn transaction |
| Card Detail | `FWacomBattleHUDCardDetailController + FWacomFirstPersonCardDetailMotionController` | first-person viewport 详情 source guard、共享 motion / cache core |

这些 helper 都是 `WacomApp/Private/UI/Battle` 内部实现，不作为 WBP 或其它模块的制作入口。`FWacomBattleHUDSnapshotPresenter` 负责 Snapshot fanout 顺序、pile count 复合文本和 BattleEnd 清理；`FWacomBattleHUDCommandController` 只把 HUD 玩家意图转换为 `UBattleSession` public command 并构造 `FWacomBattleCommandPresentationContext`；所有初始化结果和命令结果都交给 `FWacomBattleHUDResultApplicator`。`FWacomBattleHUDTargetingController` 负责 TargetSelect UI state、pending card 和 target selection view。`FWacomBattleHUDRuntime` 保留运行时状态与这些 helper 的独占生命周期。

## §2 命令与 HUD State

玩家意图入口归到 `Wacom|Battle|Commands`：

- `OnCardClickedByUser`（legacy 点击手牌兼容空入口；不再提交命令，新 Battle hand 不应使用）
- `OnEnemyPartClickedByUser`
- `OnWaitRequested`
- `OnEndTurnRequested`
- `CancelTargetSelect`
- `OnKnockdownChoiceSelected`

HUD 是命令出口。子 Widget 和 WBP 不直接修改 `UBattleSession`，也不在图里实现出牌、等待、结束回合、击倒选择或目标选择规则。`OnCardClickedByUser` 已收口为空兼容入口，只用于避免旧资产加载断裂；它不会提交命令，也不应作为新的 first-person hand 制作入口。新手牌出牌必须走 first-person card layer drag / release 或数字快捷键启动拖拽。

Battle 创建完成后，`AWacomGameMode` 必须严格执行 `BeginBattleEntryPresentation → AttachInitializedBattleSession → ReleaseBattleEntryPresentation`。Begin 建立新的 presentation generation、关闭 `BattleInputReady` 并启用 Battle hand suppression；Attach 只接受本代第一次成功结果，按“缓存 opening transition → suppressed Snapshot 刷新 → Combat Log → presentation queue”应用同次 commit 的 `FBattleInitializationResult`；Release 解除 suppression、开启输入，再使用 Attach 保存的 `PostSnapshot` 刷新，禁止重新从 Session 拉取。失败、重复、越序调用只记录诊断且不发布结果。普通 `SetInjectedBattleSession()` 只绑定已有 Session、取消未完成代次并重置版本基线，不推断或重播初始化事件。

命令结果同样只有一个应用 seam。`FWacomBattleHUDCommandController` 获取命令前 Snapshot、构造 `FWacomBattleCommandPresentationContext` 并调用 `ResolveCommand()`，随后无论成功失败都把 `FBattleResolution` 交给 `FWacomBattleHUDResultApplicator`。Applicator 只接受当前 Session 上 `VersionBefore == LastAppliedVersion`、`VersionAfter == VersionBefore + 1` 且 `PostSnapshot.Version == VersionAfter` 的成功结果；失败、重复、过期或跳版本结果不刷新 Snapshot、不写 Combat Log、不发布事件，也不改变 targeting。成功 PlayCard 先记录 commit facts，再清 targeting / 恢复 Idle。命令若包含真正的 `CardDiscarded` 批次或 `PresentationJournal.DeckSteps`，Result Applicator 会交给通用 phased plan，严格按 `HandDiscardGlyphTransfer → DeckReshuffle → DrawBatch` 恢复；尚未进入当前阶段的最终手牌保持隐藏。EndTurn 继续在同一 coordinator 中保留弃牌、保留、敌方行动和左右手生成阶段。规则和 PostSnapshot 已立即结算，表现计划只负责中间视觉状态与输入 busy gate。

键盘数字牌位快捷键进入 `AWacomPlayerController` 后只提交 one-based hand index 和当前 pointer widget-space 坐标给 `UBattleHUD`；当前可见战斗手牌的 `CardInstanceId` 由 BattleHUD / first-person hand bridge 根据已同步的 hand snapshot 解析，PlayerController 不直接读取 `UBattleSession` 或 `FBattleSnapshot`。快捷键启动的 active drag 由 `UWacomGameViewportClient` 注册的 Slate Input Processor 在 Widget 路由前处理中性右键取消：事件被消费，Anchor 广播既有 cancel 生命周期，源卡返回手牌，不执行 release、非法目标反馈或 Battle 命令；`HandleRerouteInput()` 与 PlayerController `InputKey()` 保留同条件 fallback，鼠标来源拖拽、Viewport 外点击和 idle 状态的右键保持原路由。键盘 `IA_Wait` / `IA_EndTurn` 进入 BattleHUD 前同样由 `AWacomPlayerController` 做 first-person hand 输入仲裁：如果当前卡牌层存在 active gesture，快捷键先取消该手势并被消费；只有卡牌层处于 idle / cancelled 时才调用 `OnWaitRequested` 或 `OnEndTurnRequested`。

`BattleInputReady` 是 BattleHUD 级玩家命令 gate，不属于 `UBattleSession` 规则阶段。进入战斗镜头 staging 期间由 `FWacomBattleHUDResultApplicator` 关闭：CommandBar 按钮禁用，`CanSubmitPlayerActionCommand()` 返回 false，first-person hand release / Wait / EndTurn 等普通玩家命令不会提交；HUD 仍可同步场景敌人和接收非交互表现。镜头完成并激活 Battle camera look 后，GameMode 只发出 Release 生命周期信号，不能直接修改 gate 或 suppression。

EndTurn phase plan 运行期间，`CanSubmitPlayerActionCommand()` 返回 false，first-person hand runtime interaction 也同步关闭并取消已有 hover / drag，避免阶段化弃牌、保留、敌人行动和抽牌被新的玩家输入插入；plan 完成并刷新最终 snapshot 后再恢复。普通 target cue queue 的旧节奏不因此改变；它仍可作为非阻塞表现队列服务出牌后的轻量反馈。

HUD 状态入口：

| 分类 | 内容 |
|---|---|
| `Wacom|Battle|HUD State` | `GetUIState`、`BP_OnUIStateChanged` |
| `Wacom|Battle|Targeting` | TargetSelect 查询、pending card、target selection view |
| `Wacom|Battle|Presentation Flow` | presentation busy、command gate、pending turn-boundary 查询 |

`UWacomBattleWidgetBase` 是 Battle UI 基类，只负责 C++ owner session 注入、Snapshot fanout 和 WBP 表现刷新钩子。正式 C++ owner 入口是 `SetInjectedBattleSession / GetInjectedBattleSession`；`SetSession / GetSession` 仅作为旧 C++ 兼容 wrapper 保留，不再暴露给 Blueprint。正式 WBP 不应直接读取 `UBattleSession`，而应消费 Snapshot / ViewData，并把玩家意图回传 `UBattleHUD`。`BP_OnRefreshedFromSnapshot` 属于 `Wacom|Battle|Snapshot Refresh`。

## §3 Combat Log 与 Presentation Stack

战斗反馈分四层：

| 层 | 类型 | 用途 |
|---|---|---|
| Presentation coordinator | `FWacomBattleHUDPresentationCoordinator` | TargetCue、短暂停顿、击倒 modal、BattleEnd signal、card stack boundary、EndTurn phase plan |
| Presentation Stack | `UBattlePresentationStackWidget` | 已提交但表现仍在追赶的卡牌小堆叠 |
| Combat Log | `UBattleCombatLogFeedWidget + UWacomBattleCombatLogBuilder` | 玩家可读命令块和事件 detail line |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 是当前正式 BattleHUD 玩家日志命令块 Builder。它把一次成功 HUD command 后消费到的事件批次聚合成 `FWacomBattleCombatLogBlockView`。规则层不新增 batch id；当前 UI 事实是一次成功 HUD 命令后的事件批次就是一个 combat log block。

`UBattleCombatLogFeedWidget` 是 BattleHUD 内部常驻滚动记录，默认承接本场最近玩家可读命令块。`UBattleCombatLogBlockWidget` 显示单个命令块和 detail line。正式 Details 配置位于 `Wacom|Battle|Combat Log|Authoring`。

`UBattlePresentationStackWidget` 是只读小卡表现 backlog，不是规则栈或交互入口。它用完整 `UWacomCardView` 作为 mini card，显示已提交但表现边界尚未释放的卡牌。正式 Details 配置位于 `Wacom|Battle|Presentation Stack|Authoring`。

Presentation Stack 小卡使用出牌前 `FHandCardSnapshot` 构造卡牌 runtime presentation context，因此显示的是提交时的 `RuntimeCost`、可用状态和 RuntimeCost-based 徽章，不在表现期间回读后续手牌状态。带目标的出牌命令会在提交前捕获 `FBattleCardTargetPreview` 并放入 command context；小卡可使用这份 preview facts 显示提交时的目标修正后徽章，避免源卡离开手牌后再回查失败。

Wait / EndTurn 请求遇到表现栈未清空时会进入 pending turn-boundary；CommandBar 通过 `FWacomBattleCommandBarViewData` 显示 pending 文案并禁用按钮，coordinator 等 stack 和 queue 清空后再提交等待或结束回合。

EndTurn 命令成功后，BattleHUD 会消费 `FBattlePresentationJournal`。当 journal 能生成有效 phase plan 时，`FWacomBattleHUDPresentationCoordinator` 接管本次 EndTurn 表现，不再把整批事件直接压成一帧 hand hints，也不立即把 first-person hand 刷到最终抽牌态。v1 phase 顺序固定为：

1. `TurnEndDiscard`：用 discard checkpoint snapshot 把非保留普通手牌原地收束为牌印并飞向弃牌堆；每枚抵达只增加一次表现计数，并在同一 progress 回调触发像素 Impact 与 `DiscardPileView` 接收脉冲。连续抵达的脉冲由 `UPileCountView` 叠加，最后一枚稍强；phase 完成或强制清理后恢复权威 Snapshot 数量与 authored RenderTransform，ForceComplete 不补播回弹。
2. retain checkpoint 仍保留在规则 / journal 中，但不再建立独立 `TurnEndRetain` UI phase，也不播放 `Retained` feedback；`CardsRetained` 规则事件本身保持不变。
3. `EnemyAction`：复用现有 battle event presentation queue 播放敌人行动相关 cue / delay / battle end / knockdown modal。
4. `TurnStartDraw`：用 draw checkpoint snapshot 播放新回合 `Drawn` 入场。
5. `TurnStartHandAnchorEnter`：如果 draw checkpoint 中出现了上一手牌 checkpoint 没有的左/右手 anchor，则在普通抽牌后提交完整 hand snapshot，并播放 `HandAnchorEntered` 生成入手。

手牌 phase 的完成条件由 first-person card layer 的 production playback 状态提供：仍有 active enter、exit outgoing、retained feedback 或 Card Glyph Transfer 时保持 phase busy；播放结束后才进入下一 phase。普通弃牌和弃牌堆洗回使用传输种类加 Batch Sequence 去重并按 FIFO 播放，新批次不得强制完成前一批。timeout 不是直接跳过：触发时 coordinator 必须先 force-settle 当前 Anchor / Layer，清除 pending hints、收束 active playback、Impact 和临时牌堆计数，再启动下一 phase，避免旧动画跨阶段重叠。没有 journal 或 journal 无有效 phase 时，非 EndTurn / fallback 路径继续使用原来的 loose event hints 与 event queue。

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

每个 PartActor 原生携带一个无碰撞 `ImpactAnchor`，默认位于 `HitBounds` 中心；`ImpactAnchorRelativeLocation` 以厘米为单位，只用于世界命中特效的美术微调，不改变命中盒、目标身份或 Battle 规则。HitOnly 与 VisualLayers 两条制作路径共享这一合同。Authoring Status / debug view / summary 会报告锚点组件名和世界位置，非有限偏移会被 Data Validation 判为错误。

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
- `UWacomBattleEnemyPartPresentationComponent`：负责 hover target、TargetSelect 可选提示、first-person drag preview、`TargetConfirmed / DamageDealt / EnemyPartHpEmptied` 语义 Cue Playback、只读预测 Widget 和常驻状态 Badge。Cue Playback 互斥并按 `Destroyed > Damage > TargetConfirmed` 覆盖，只在活动期间 Tick；Cue 不再写敌人 Scale。普通 Hover / Targetable 仍使用轻量 authored scale；拖卡已经锁定具体世界部位时恢复 authored base scale，由独立像素目标框承担空间提示，不再叠加旧 `1.08x` Drag Preview 放大。

Cue 的持续时间优先读取 `FWacomBattlePresentationTargetCue.Duration`，无效时使用 PartActor 的 `CueHoldSeconds`。`TargetConfirmed=0.24s`、`Damage=0.30s`；世界伤害批次在首个 Damage 前保留约 `0.14s` 的确认可读窗口，随后 Presentation Queue 仍按伤害事件顺序逐条播放。Cue 的 `Seed` 只服务稳定的表现随机：确认由来源卡实例与目标部位构造，伤害由事件 Sequence、目标部位和实际 HP 损失构造；它不是 Battle RNG，也不参与规则恢复。解绑、BattleEnd、EndPlay、目标组件/Style 切换和 reset 会清理 Playback、关闭 Tick、立即停用 Niagara 并恢复 authored scale。

世界目标确认与伤害反馈使用 `UWacomBattleEnemyPartImpactStyle` 和一个由 Part Presentation 按需创建、复用的 `UNiagaraComponent`。Style 解析顺序固定为 `PartActor.ImpactStyleOverride -> EnemyHost.DefaultImpactStyle -> None`；关闭 Part 的 `bEnableImpactFeedback` 或缺失 Style/System/MI 时仍消费 Cue，但不创建 VFX。`TargetConfirmed` 使用角部像素刻线收束与语义中心方印，`Damage` 使用中心硬闪、菱形冲击环和短程像素碎片；两者读取当前 PartActor 的 `HitBounds`，按摄像机平面投影得到基准直径，默认覆盖命中部位约 `1.2x`，并由 Style 的 fallback / min / max 控制异常尺寸。`Destroyed` 当前仍只有高优先级 Playback，不恢复旧缩放脉冲。

拖卡悬停世界部位使用独立的 `UWacomBattleEnemyPartTargetPreviewStyle`、App-private Preview Playback 和第二个按需创建/复用的 Niagara Component。Style 解析顺序为 `PartActor.TargetPreviewStyleOverride -> EnemyHost.DefaultTargetPreviewStyle -> None`；它与短时 Impact Component 分离，因此持续悬停不会占用或重启 TargetConfirmed / Damage。有效目标显示覆盖整个 `HitBounds` 摄像机平面投影的冰蓝/象牙金四角像素框，并在中心保留空心菱形；进入约 `0.18s` 从 `1.12x` 收束，停留阶段只做弱亮度呼吸。无效世界目标显示暗紫红断裂框且没有中心菱形。切换或离开约 `0.10s` 淡出；预测 Badge 继续解释先机和拒绝原因。`UIMotionMode=Simplified` 直接显示静态语义框，`FlashEffectMode=Off` 只关闭呼吸辉光，不隐藏有效/无效判定。

Niagara 公共合同固定为 `User.EffectKind / Duration / Intensity / Seed / DecorativeIntensity / ReducedMotion / ImpactMaterial / PlaneRight / PlaneUp / TargetDiameter`；目标预演分支另外使用 `TargetWidth / TargetHeight / PreviewAmount / PreviewValidity / PreviewPulse`。`EffectKind=0/1/2` 分别表示 TargetConfirmed、Damage、TargetPreview。Damage 强度只改变视觉密度/尺寸，公式为 `clamp(0.75 + sqrt(max(实际 HP 损失, 0)) * 0.10, 0.80, 1.80)`，不延长 Cue。特效平面使用当前玩家摄像机 Right/Up，在 `ImpactAnchor` 朝摄像机方向偏移 Style 指定距离（默认 `2cm`），保留正常深度遮挡。`UIMotionMode=Simplified` 由 Niagara 保留静态语义方印并关闭收束/扩散/碎片位移；`FlashEffectMode=Reduced/Off` 只把装饰强度降为 `35%/0%`，语义中心方印和声音保留。设置在每次 Cue 或 Preview 开始时快照。

视觉真源为 `DShader/Material/World/M_WacomBattleEnemyPartImpactPixel.dsm` 与 `DShader/Shared/WacomBattleEnemyPartImpactPixel.dsh`，命中默认 MI 为 `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartImpactPixel_Default`，目标预演默认 MI 为 `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartTargetPreviewPixel_Default`。材质是 `Surface / Unlit / Translucent / TwoSided`，开启 Niagara Sprite usage，不读取贴图、Time 或 Noise。Impact 粒子的 Dynamic Material Parameter 为 `X=ShapeKind、Y=NormalizedAge、Z=PaletteVariant、W=Semantic/Decorative`；TargetPreview 为 `X=PreviewShape、Y=PreviewAmount、Z=Validity、W=Pulse×DecorativeIntensity`。DataAsset 管理 System、MI、节奏、覆盖尺寸、摄像机偏移和可选声音，颜色、像素密度、线宽和辉光只在 MI 调整。

Niagara Graph 的项目真源是现有 System 资产加 WacomEditor-only 生成器：`WacomBuildBattleEnemyPartImpactNiagara` 会验证十五个 User Parameter、`ConfirmStamp / DamageCoreWave / DamageFragments / TargetPreview` 四个 Emitter 和 Sprite Renderer 的 `User.ImpactMaterial / Particles.DynamicMaterialParameter` 绑定，再幂等重建 Burst、Spawn/Update 属性与四通道写入并强制编译。缺少 `TargetPreview` 时生成器会从现有 ConfirmStamp 模板幂等添加，不要求内容人员手工配置 Stack。生成器依赖 UE 5.8 的 experimental Niagara external-edit API，因此严格留在 `WacomEditor/Private`；升级引擎时必须先重跑生成和编译验证，不能让运行时模块依赖 NiagaraEditor。编辑器关闭后可执行：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -run=WacomBuildBattleEnemyPartImpactNiagara -Unattended -NoSplash -NullRHI
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -ExecutePythonScript='<Worktree>\Scripts\SetupBattleEnemyPartImpactAssets.py' -Unattended -NoSplash -NullRHI
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -ExecutePythonScript='<Worktree>\Scripts\SetupBattleEnemyPartTargetPreviewAssets.py' -Unattended -NoSplash -NullRHI
```

第一条命令负责 Niagara Stack 和编译合同；后两条分别配置 Impact 与 TargetPreview 的 MI/Style/Debug Snake 引用，Python 不反射编辑 Niagara Graph。TargetPreview 脚本不会覆盖现有 Impact Style 的人工声音、强度和覆盖范围。日常美术调参不应改生成的 Set Variables 表达式：颜色、像素密度、线宽和辉光改对应默认 MI；伤害强度、预演时序/覆盖、摄像机偏移和声音改对应 Style；命中位置改 PartActor 的 `ImpactAnchorRelativeLocation`。只有视觉算法、粒子数量、世界尺寸或时间比例改变时才修改 WacomEditor 生成器并重建 System。

BattleHUD scene enemy coordinator 成对缓存 Bridge 和 Presentation：target handle 解析走 Bridge，表现 target 注册和反馈走 Presentation。target handle 必须携带完整 `EncounterId + EnemySlotId + PartSlotId` 才能命中当前 registry；`SourceObject` 即使指向当前 PartActor，也不会替代或修正错误 key。Host / PartActor debug summary 会合并两者事实用于 PIE 排查，但 passive UI 和 BattleSession 不依赖这个合并 debug。

BattleHUD 不再构建或绑定敌方 2D fallback；点击、hover、drag target handle 全部通过当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge 完成。`EncounterDefinition` 正式入口缺 Host 会被编辑器验证阻止。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、CommandBar、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。正式 target drag / aim、world target release、hand-card target release 和无目标 drag-out release 都经 BattleHUD bridge / command flow 进入 BattleSession。无目标 Battle 卡向上拖过 `NoTargetCardDragOutCommitDistancePixels` 后进入 `ArmedForCommit`，只有保持 armed 并 release 才提交；拖回阈值内会解除 armed。`FWacomBattleFirstPersonDropResolver` 是 App-private drop intent adapter，集中处理无目标 armed intent、目标 drag view、Battle world probe 和 `UBattleSession::ValidateTargetWithCard()` 的 UI 映射。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

Battle / Run hand 共用 Card Depth。`FWacomFirstPersonCardDepthMotion` 在 Hover 时按卡面局部 pointer 计算倾斜，在 Drag 时按低通 pointer velocity 计算惯性倾斜，并与 rest / semantic transition 的压平以及 `ContactShadowLift` 一起做帧率无关平滑。`UWacomFirstPersonCardViewWidget` 将 `TiltX / TiltY / PerspectiveStrength / ContactShadowEnabled / ContactShadowLift` 写入现有 Retainer；外部 `CardShadowImage` 不属于当前生产链。

Battle / Run 共用的 first-person Slot 不再执行单卡视口底边钳制。卡面靠近屏幕边缘时的文字连续绘制由 `Fake3DSurfaceRetainer` 直接内容根的 `Clip To Bounds - Without Intersecting` 合同保证；该合同只修正 Retainer 内部 culling，不改变手牌扇形、卡牌位置或输入命中。

Battle hand 与 Run hand 共用 App-private `FWacomFirstPersonCardPresentationScalePolicy`。全局 UI 仍参照 `1920 × 1080`，但 first-person 卡牌表现参照 `2560 × 1440`：720p 目标物理倍率 `0.5`、1080p 为 `0.75`、1440p 及以上封顶 `1.0`。倍率由 Anchor 写入 resolved runtime config 与 Slot view；BattleHUD 不自行计算或覆盖。卡牌 `296 × 420` 主体、手牌布局、语义转场、反馈位移、Aim Arrow 和 Card Glyph Transfer 使用同一局部空间倍率，时序和输入阈值不变。

First-person hand 卡面和 first-person viewport 详情都从 `FHandCardSnapshot` 派生 `FWacomCardPresentationRuntimeContext`，再交给 `UWacomCardPresentationBuilder` 生成 ViewData。基础 runtime context 覆盖本场 `RuntimeCost` 与 `bIsPlayable`：卡面 Cost、disabled overlay、RuntimeCost-based 效果徽章和详情 `Sections` 会显示当前战斗事实；`Magnitude.Source.RuntimeCost` 一类数值来源会在 value run 中保留“相当于当前费用”等来源短语。详情面板正式渲染来源是 `FWacomCardDetailViewData.Sections`：Builder 按卡牌详情文档顺序组装 section，App-private `WacomCardExplanationCompiler` 从效果 / 被动 tag、runtime preview 和 `UWacomCardExplanationLexicon` 编译语义 `Blocks / Runs`；`UWacomCardDetailPanel` 只按 section 顺序渲染，不再按原始 `Passive.DisplayText` 或 token kind 推断分区。旧平行 `Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow 已从详情合同删除；`UCardDefinition::Description` 仅在该卡没有任何结构化详情 section 时作为普通正文回退，不解析旧 `{Effect.0}` 占位。费用变化、目标手牌 cost preview、被动正文和后续任务 / 预览内容都应反映到对应卡面数值或正式 `Sections` document。Battle 详情的数据来源和 source guard 仍由 `FWacomBattleHUDCardDetailController` 负责；预热、详情数据缓存、淡入淡出 / scale / follow motion 和稳定换边由 App-private `FWacomFirstPersonCardDetailMotionController` 与 Run first-person 详情共用。

First-person 详情面板的 viewport 生命周期由 `FWacomFirstPersonCardDetailPanelHost` 统一处理：只有拥有真实 local player / LocalPlayer 的 context 才允许 `AddToViewport`，HUD-only 自动化或离屏预热只构建面板对象并验证 ViewData / motion state。详情面板内部的 section 动态创建收口在 App-private `WacomCardDetailWidgetFactory`；section 内正文由 `UWacomCardDetailRichTextBlock` 渲染 RichText，并自动注册详情 inline icon decorator。战斗详情与背包详情共用 `CardDetailTheme.StatusBrushes / IconBrushes`：状态 run 显示状态图标和中文名，显式 `{icon:EffectIcon}` 才显示效果图标。详情 Widget 不应直接用 `GetWorld()` 判断创建路径，避免无 World 的离屏场景污染日志。

Battle first-person 详情与 Run first-person 详情都由 `FWacomFirstPersonCardDetailMotionController` 消费 Slot 的只读 `PresentationScale`；`360 × 420` 内部制作布局保持不变，RenderTransform 整体缩放并与淡入出现倍率相乘。左右换边、边缘 clamp、Padding、换边滞后和位置重置按缩放后的视觉边界计算；分辨率变化只更新已显示面板的位置与倍率，不重新应用详情数据。背包详情面板不进入此合同。

被动详情正文的分类由“被动”区块标题承载，正文不再读取 `Passive.DisplayText`。被动触发说明由 `Passive.TriggerTemplates` 生成；`PassiveOutcomeTemplates` 描述 `OnCompanionCount` 这类规则专用结果，例如“使此牌回到手中”。可执行 `Passive.Effects` 继续复用效果模板；`OnCompanionCount` 和 `OnTwilightTriggered` 当前不展示 `Passive.Effects`，因为运行时不执行这些 Effects。没有 outcome / effects 的被动不会只显示一个悬空触发句。

Battle hand entries 由 `WacomBattleCardPresentation` 从 `FHandCardSnapshot` 构建：`ECardTargetMode` 只在 Battle adapter 内部映射成 first-person card layer 的 `InteractionIntent`，不再写入通用 entry。通用 first-person card layer 公共类型不提供 TargetMode 转换 helper；SlotWidget 只消费 `InteractionIntent` 来决定无目标拖拽或瞄准态，目标合法性和提交仍由 BattleHUD / BattleSession 处理。旧 `OnCardClickedByUser` 兼容入口不再参与该流程，也不再维护独立 TargetMode 分支。Hand-card target release、probe-only 状态和 full-hand card affordance 都以 `UBattleSession::ValidateTargetWithCard()` 的 reject reason 为准，first-person hand bridge 不再直接读取卡牌 `TargetMode` 来判断 hand-card 目标规则。

Battle hand 的场景化动画位置来自 BattleHUD UMG 几何，不来自 Battle 规则。`FWacomBattleHUDFirstPersonHandBridge` 在提交 presentation frame 时采集可选 `DrawPileMotionAnchor / DiscardPileMotionAnchor / PlayTargetMotionAnchor`，Draw / Discard 缺失专用锚点时分别回退现有 pile view；采集结果以逻辑 viewport 坐标写入同一 BattleHand source lifecycle frame。`Drawn`、`Discarded` 继续消费这些坐标。`Played` 的目标坐标也继续记录：配置有效 Card Use Surface Effect 时，源卡锁定提交位置并播放当前 Style（默认像素翻面收牌，旧菱形波可切回），坐标不驱动卡牌移动；配置失效时才按真实目标优先、PlayTarget 次之的旧合同执行空间离场。锚点只是只读表现事实，不改变牌堆、目标校验或出牌结算。

当玩家拖拽手牌并指向敌人部位、目标手牌，或无目标卡已经达到 `ArmedForCommit` 可释放状态时，first-person hand bridge 会把当前 `CardInstanceId + TargetHandle` 交给 `UBattleSession::BuildCardActionPreview()`。Battle 返回的 `FBattleCardActionPreview` 是只读规则 facts；它内嵌 `FBattleCardTargetPreview`，App 侧随后用 `WacomBattleCardPresentation::BuildActionPreviewPresentation()` 一次性生成 hand layer entries、源卡详情、可选目标手牌详情、玩家 projected state 和敌人部位 projected state。源卡卡面徽章和详情正文仍复用 target preview facts 显示目标修正后的主效果最终值；被强化 / 削弱的数值通过 RichText `ValueBuffed / ValueNerfed` 样式提示，不显示 `基础值 -> 预览值` 公式。若 preview 最终值不同于基础值，详情 value run 会隐藏 “相当于当前费用 / 相当于目标状态层数” 等来源短语，只保留最终值，避免来源文案和目标修正事实冲突。若目标是手牌，目标卡自己的卡面费用可以显示预测后的费用，但详情不生成 `[费] before -> after` 文本。preview 不提交命令、不修改 Battle state，也不在 UI 里重新计算伤害、护盾、状态、先机或敌人行动。

Action Preview 的数值显示只在“有效释放语义”成立时启用：目标合法或无目标卡已经 armed、当前规则阶段是 `PlayerAction`、源卡通过完整 PlayCard preflight，且 `BuildCardActionPreview()` 生成 projected values。拖出手牌区但还没达到无目标提交距离、还没指向有效目标、目标无效或规则阶段不可提交时，不显示玩家侧收益或敌人净结果，只保留 hand / scene target 的轻量可作用对象提示。有效敌人部位目标上，`UPlayerStatusBar` 直接覆盖显示 projected HP / Shield / runtime statuses，`UWacomBattleEnemyPartEntryWidget` 直接覆盖显示 projected HP / Shield / Initiative / runtime statuses / destroyed；无目标卡的 projected player state 同样直接覆盖玩家状态条。所有预览都不显示箭头、公式、来源文案或 `+N/-N`。敌人部位若会因本次打牌立即行动，部位 UI 的先机显示为 `0`，代表“松手后会出手”，不显示行动后刷新出的下一意图先机。first-person 拖拽命中有效场景目标时，HUD 会把规则层返回的全部 projected enemy parts 应用到敌人聚合面板和场景部位 prediction badge；这意味着同一次出牌会触发多个部位行动时，非当前鼠标指向部位也会显示 projected 先机 / 行动风险。TargetSelect hover probe 只刷新目标预览和敌人面板，不打开场景部位 action preview badge，避免普通 hover 与正式拖拽释放预览混淆。

拖拽 release、cancel、离开目标、候选目标无效、snapshot version 变化、BattleEnd 或 UI state 退出时，bridge / runtime 会清理 preview entries 和 action preview，恢复基础 hand entries、当前详情、玩家状态条和敌人部位条目。Scene enemy hover / TargetSelect hover 也先构建同一份 `FWacomBattleActionPreviewPresentation` 再应用：场景目标反馈仍由 enemy presentation component 负责，卡面、详情、玩家状态条和敌人面板只消费该 presentation，不在 hover / drag 两条路径里重复拼。Preview semantic state 由 snapshot version、source id、目标身份和 preview facts hash 组成；同一 state 上的高频 hover / drag move 只允许更新指针反馈、敌人 hover 和详情位置，hand layer preview entries 与详情数据必须等 preview semantic state 变化后再重建。Active drag 期间，目标手牌 preview 的生命周期由 bridge 保存的 `ActiveDragView.CurrentTarget` / target preview state 决定，SlotWidget 重建或 hover/unhover 抖动不能作为清理 preview 的权威信号。

`FirstPersonCardDetailViewportZOrder / FirstPersonCardDetailAnchorBaseSize` 属于 `Wacom|Battle|First Person Card Layer|Authoring`。第一人称手牌交互开关由当前 runtime source owner 通过 `SetFirstPersonCardLayerInteractionEnabled()` / `IsFirstPersonCardLayerInteractionEnabled()` 控制；Anchor 上旧的 `bEnableBattleHandInteraction` / BattleHand 命名 getter 只作为旧资产和旧 Blueprint 兼容别名保留。

Battle 与 Run 的 first-person card 镜头响应共用 Anchor `16 Camera Look While UI` 制作参数。Hover 由 Layer 的归一化 pointer view 驱动；Inspect / Drag 由 `FWacomFirstPersonCardDragView.PointerNormalizedViewportPosition` 驱动。BattleHUD first-person hand bridge 把两者转交已激活的 `UWacomBattleCameraLookComponent`，Run PlayerController 把它们转交已激活且未 suspended 的 `UWacomRunPathTraversalComponent`。Hover 进入 Inspect 时先清 pointer override，再由 drag-view override 同帧接管；release、cancel、source clear / unbind 必须清除。不恢复旧共享 camera-look bridge。

Battle entry staging 期间，入场门控与 opening result 的所有权在 `FWacomBattleHUDResultApplicator`；first-person hand bridge 只执行 suppression 与 transition frame。Begin 关闭 `BattleHand` presentation gate、清空当前 visual slot，并让 hand sync 写入 0 entries；Attach 在 suppression 下保存 opening `CardsDrawn` 并刷新初始化 `PostSnapshot`，不会让左右手提前落位。Release 解除 suppression 后用保存的同一份 `PostSnapshot` 提交一次正式 Drawn frame；该 frame 暂不包含新出现的左右手 anchor，普通牌 Drawn 播放完成后 bridge 再提交完整 snapshot，并让左右手各播放一次 `HandAnchorEntered` follow-up。GameMode 不再直接改 suppression / input gate，也不在 Release 时从 Session 重建快照。若 Drawn frame 已提交但尚未消费又进入新 suppression，bridge 仍先把 frame 还原为 controller 待播事件，保证 opening transaction 不丢失、不重复。

Battle hand 抽牌表现由 `FWacomBattleHandPresentationController` 事务化交付。Controller 消费 Battle events 时，优先把 `CardsDrawn.CardInstanceIds` 中仍存在于下一帧 hand snapshot 的真实普通卡实例转成 `Drawn` transition hint，并为可见 hint 写入稳定 `SequenceIndex / SequenceCount`；只有旧式 Count-only 事件才回退到 baseline / next snapshot 新增普通卡推断，不把左右手 anchor 纳入抽牌预算。HUD 不计算入场曲线、延迟、弧线或来源位置；这些表现参数由 first-person card layer / Anchor `06 Transition Motion` 统一处理。Bridge 只负责把 controller 给出的 `FWacomFirstPersonCardLayerPresentationFrame` 写入 Anchor，或在没有 pending event 时执行普通 entries refresh。普通 refresh 不会替换 Anchor 里尚未消费的 presentation frame hints；只有新的显式 frame、source clear、suppression 或 battle end 会替换 / 清空 hints，避免状态刷新把抽牌入场表现吞掉。Anchor 还有 source-scoped presentation gate：gate 关闭时 entries 可以刷新，但 pending frame hints 不能被送进 Layer。Layer 收到 frame 后也不会在一次空 slots / unprojected slots 刷新中丢弃 Drawn：只有 gate 已打开、对应 slot 可投影并真正启动入场播放后才消费该 hint，镜头 staging 或 viewport 投影暂不可用时会延迟到后续 hand refresh。

`FWacomBattleDrawPileFeedbackController` 只管理 DrawPile 的临时显示数量与发牌反馈，不改变 Battle Snapshot。普通/开场抽牌从 `CardsDrawn` 建批，Presentation Plan 从 `FBattlePresentationDeckStep::DrawBatch` 建批；两条路径最后都等待 first-person card layer 发回真实 `Drawn` Enter Started。每张可见卡开始移动的同一边缘，控制器把 `DrawPileView` 减一并调用方向性 `PlaySendFeedback()`；最后一张可见卡把隐藏差额校准到权威批次终值。重复 Card ID / Event Sequence 不重放，plan 中断、BattleEnd、source clear 和 teardown 直接恢复权威数量。已有 Drawn 音效与 Started 通知共享播放边缘，因此不新增第二层牌堆声音。

`DeckReshuffle` 的两端反馈由 Card Glyph Transfer 的真实 progress 边缘驱动，不使用额外 Timer。`LaunchedCount` 增加时，Coordinator 用新发射牌印的平均 Bezier 初始切线驱动 `DiscardPileView.PlaySendFeedback()`，并从洗牌前弃牌数逐张递减；`ArrivedCount` 增加时，从洗牌前抽牌数逐张递增 `DrawPileView`，同时触发 `PlayReceiveFeedback()` 与 Slate Impact。最后一枚只增强既有接收脉冲/方印。低帧率批量跨边缘按数量增量聚合，重复 progress 不重复计数；ForceComplete、超时、BattleEnd、source clear 与 teardown 只恢复 Deck Step 的精确终值并清除两端 Transform。弃牌堆的 `Discard+Played` 复合文本在整个阶段保持不变，任一 PileView 缺失时另一端仍可独立工作。

First-person card layer 重新拥有语义 Transition Audio，并生成 `Gained` 专用 transition：音效只在对应 enter playback 跨过错峰延迟、真正开始播放时请求一次；普通 refresh/reflow 不播放。`CardsRetained` 通过独立 feedback hint 驱动原槽位上的短促上浮、缩放、错峰与临时 ZOrder，不使用旧 Overlay 发光。`Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded` 均保持显式表现语义；规则事件、日志和 Toast 行为不变。

`WacomBattle` 在 `CardPlayed` 后发布 `CardPlayDestinationResolved`，只提供最终 `ECardLocation` 规则事实；`WacomApp` 先索引同批事件再映射表现语义。最终进入 Exhaust 或独立 `CardExhausted` 事件生成 `Exhausted`，其他成功离开手牌的使用生成 `Played`；同一卡的相关事件会去重。成功使用且最终仍在 Hand 的卡（例如 AfterPlayed 自身腾挪）不会伪造离场，而是在存在已接受的 play commit 时生成 `CardUseReform` feedback：默认先在提交位置用 `0.22s` 翻到侧边，隐藏 `0.06s` 后切到最终手牌槽，再用 `0.18s` 反向展开并 `0.04s` 落定。无左右锚点时最终槽位等于原位，仍完整播放翻出与翻回；使用音效只在开始请求一次。普通 `Played` 默认约 `0.28s` 翻面收起，`Exhausted` 继续使用 OrderedDither / PixelAsh；替换 Anchor Style 可恢复旧 DiamondWave。所有路径复用唯一 `Fake3DSurfaceRetainer`，不延迟 BattleSession 命令结算。

成功的 `PlayCardOnHandCard` 还会由 BattleHUD 读取提交前冻结的目标手牌 ID，并在同一 command presentation phase 给目标卡下发一次 `HandTargetImpact`；这不是新的战斗事件，也不改变目标判定。有效目标 hover 只播放弱像素刻印 Preview；命令成功后源卡效果立即开始，目标约 `0.07s` 后压印、在约 `0.11s` 开放结果离场，再回弹归位。Post Snapshot 决定目标留手、普通弃牌或 Exhaust：普通弃牌牌印与消耗 Surface 都等待该 Gate，缺失 Style/MI/Slot 时则立即走旧路径，不为表现延迟命令。默认 Style 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardHandTargetImpactStyle_PixelStamp`，Anchor 入口为 `15 Card Hand Target Impact`；世界目标命中反馈仍是后续独立切片。

Slot 释放结果必须在任何同步 HUD 回调前冻结。Layer 把解析后的目标事实同时写入 source Slot；Release 只做最后一次本地手势计算，不重复广播 DragUpdated，然后按冻结的 accepted / neutral / denied 结果播放反馈。这样成功命令触发的同步 snapshot refresh、手势清理或交互禁用不会把有效目标误改成 Deny 抖动。

## §8 Battle Shared Widgets

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。玩家和敌人 runtime 状态共用 `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget`：控件只消费 Snapshot / ViewData 中的 `Statuses / StatusStacks`，图标 Brush 由 WBP 变量配置，`Status.Shield` 仍由 HP / Shield UI 单独显示。状态图标控件提供 design-time preview 字段，方便在 UMG 视口调图标和层数；这些预览字段不进入运行时规则数据。Action Preview 激活时，玩家状态条和敌人部位条目读取 `WacomBattle` 产出的 projected state 覆盖当前显示；清理后恢复最近一次真实 Snapshot / ViewData。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / runtime 状态图标 |
| `UBattleCommandBarWidget / UWacomBattleCommandButtonWidget` | `Wacom|Battle|Command Bar|Authoring` | 被动显示 Wait / EndTurn 命令 view data，并把玩家意图广播回 BattleHUD |
| `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget` | `Wacom|Battle|Status Icons|Authoring` | 共享状态图标列表和单个状态图标；玩家状态条正式使用，敌人部位条目可选接入 |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用数量显示与收发反馈控件；牌堆类型由 WBP Image 图标表达，BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数`。可选 `PileFeedbackRoot` 统一承载图标+数字弹性 RenderTransform；缺失时兼容回退 `ReceiveFeedbackRoot`，再回退整个控件。Receive 与 Send 使用同一个组合 playback，不争用或覆盖 authored transform；Reduced Motion 仍更新数量但不播放变换。 |
| `UWacomProgressBar` | `Wacom|Common UI|Progress Bar` | 通用数值进度条显示控件 |

CommandBar 的轻量协议定义在 `BattleCommandBarTypes.h`：`EWacomBattleCommandId`、`FWacomBattleCommandButtonView` 和 `FWacomBattleCommandBarViewData` 可以被 HUD / runtime presenter / tests 直接使用；`BattleCommandBarWidget.h` 只承载 UMG Widget 实现与 WBP 制作面。当前 `WBP_BattleCommandBar` 推荐直接绑定 `WaitButton / EndTurnButton` 让资产控制位置，并在 CommandBar 上配置 `WaitIconBrush / EndTurnIconBrush`；`CommandButtonContainer` 仅作为未绑定按钮时的动态生成回退。CommandBar ViewData 构建收口在 App-private `FWacomBattleHUDCommandBarPresenter`，`FWacomBattleHUDRuntime` 只保留刷新入口和 command gate 查询。

Battle CommandBar 的 Widget 资产留在 `/Game/Wacom/UI/Battle/Action/`，但通用 CommonUI 样式资产不属于 Battle 业务目录。`WBP_BattleCommandButton` 应引用 `/Game/Wacom/UI/Style/Button/` 下的 `CommonButtonStyle` 和 `/Game/Wacom/UI/Style/Text/` 下的 `CommonTextStyle`；Battle WBP 不通过样式资产推断命令可用性、pending 或规则状态。

BattleHUD 自身配置分类：

| 配置 | 分类 |
|---|---|
| C++ 默认 canvas layout | `Wacom|Battle|HUD Default Layout` |
| card detail authoring | `Wacom|Battle|Card Detail|Authoring` |
| card detail motion | `Wacom|Battle|Card Detail|Motion` |
| combat log max blocks | `Wacom|Battle|Combat Log|Authoring` |
| presentation stack hold | `Wacom|Battle|Presentation Stack|Authoring` |

## §9 Battle UI 测试入口

Battle UI 回归优先使用 `Source/WacomTests/Private/UI/BattleHUDTestHarness.h` 中的 `FWacomBattleHUDTestHarness` 搭配 `UWacomBattleHUDDetailTest` 装配 HUD、PlayerController、CombatLogFeed、PresentationStack、CommandBar、first-person character 和 scene enemy Host。

测试不 include BattleHUD 私有 helper header，也不为生产 HUD 增加 Blueprint-visible 测试 API。只读诊断通过 `FWacomBattleHUDAutomationTestView` 聚合；Battle scene target click / probe 通过 `FWacomBattleSceneTargetClickTestAccess` 驱动。

`Source/WacomTests/Private/UI/BattleHUDCommandFlowSpec.cpp` 承载 BattleHUD 命令和目标选择的专题合同测试，覆盖 `FWacomBattleHUDCommandController` / `FWacomBattleHUDTargetingController` 对外表现。`Source/WacomTests/Private/UI/BattleCombatLogSpec.cpp` 承载 Combat Log builder、feed、HUD history 和 `FWacomBattleHUDCombatLogController` 的专题合同测试，统一前缀为 `Wacom.UI.Battle.CombatLog`。`Source/WacomTests/Private/UI/BattlePresentationStackSpec.cpp` 承载 `UBattlePresentationStackWidget` / `UBattlePresentationStackEntryWidget` 的纯展示合同测试。`Source/WacomTests/Private/UI/BattlePresentationQueueSpec.cpp` 承载 BattleHUD presentation queue / turn-boundary / pending barrier lifecycle / teardown / BattleEnd 清理 / knockdown 延迟展示合同测试。`Source/WacomTests/Private/UI/BattleInteractionTargetSpec.cpp` 承载 battle scene enemy part world target bridge 和 scene click / probe 的 `Wacom.UI.Battle.InteractionTarget` 合同测试。`Source/WacomTests/Private/UI/BattleSceneEnemyTargetRegistrySpec.cpp` 承载 battle scene enemy target registry 专题合同测试，覆盖 Trigger scene enemy host slot -> HUD registry、current-host filtering、trigger authoring validation 和 registry-routed cue / hover / drag preview。`Source/WacomTests/Private/UI/BattleSceneEnemyHoverProbeSpec.cpp` 承载 battle scene enemy hover probe 专题合同测试，覆盖 hover visual priority、HUD hover probe bridge、TargetSelect hover prediction、无效目标清理、pending / drag / BattleEnd gate 和 hover debug summary。`Source/WacomTests/Private/UI/BattleHUDFirstPersonSpec.cpp` 承载 BattleHUD first-person hand / first-person card detail 专题合同测试，覆盖 hand bridge clear、Anchor interaction、first-person detail host、readability motion 和 inspect hover guard。`Source/WacomTests/Private/UI/BattleSceneEnemyActorSpec.cpp` 承载 battle scene enemy actor 专题合同测试，当前覆盖 hand snapshot swift prediction facts、prediction widget facade、PartActor facade / presentation setup、bridge runtime facts、world target handle、host visual / hit-only part、host visual routing、host identity / child actor scan、runtime facts / host counts、debug snake child actor authoring、part slot identity / duplicate validation、hover / drag prediction badge、prediction badge offset、badge layout stagger / debug summary、VisualLayers refresh / validation 和 blueprint default authoring 分支。`BattleWidgetSpec.cpp` 保留 fallback layout、event presentation 和其他跨专题旧测试并继续分批收口。

`Source/WacomTests/Private/UI/BattleSceneEnemyFeedbackPlaybackSpec.cpp` 单独覆盖互斥 Cue Playback、持久缩放优先级、清理和 ImpactAnchor 两条制作路径，避免继续扩大 `BattleSceneEnemyActorSpec.cpp`。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.Battle.CombatLog`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
