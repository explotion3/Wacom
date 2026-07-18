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
| 表现计时 | `FWacomBattlePresentationTimerOwner` | App-private keyed timer ownership；统一撤销 Queue Advance、Plan Poll 和 Stack Entry Exit，隔离 World teardown |
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

Battle 创建完成后，`AWacomGameMode` 必须严格执行 `BeginBattleEntryPresentation → AttachInitializedBattleSession → ReleaseBattleEntryPresentation`。Begin 建立新的 presentation generation、关闭 `BattleInputReady` 并启用 Battle hand suppression；Attach 只接受本代第一次成功结果，按“缓存 opening transition → suppressed Snapshot 刷新 → Combat Log → presentation queue”应用同次 commit 的 `FBattleInitializationResult`，同时启动 first-person card Required Visual / Optional Audio 异步预热；Release 只表示相机 staging 已就绪。只有 Camera Ready 与 Prewarm Ready/TimedOut 同时满足，Applicator 才解除 suppression、开启输入，并使用 Attach 保存的 `PostSnapshot` 刷新，禁止重新从 Session 拉取。两道门并行，资源门上限为 `1.5s`；失败、重复、越序调用只记录诊断且不发布结果。普通 `SetInjectedBattleSession()` 只绑定已有 Session、取消未完成代次和预热 Generation，并重置版本基线，不推断或重播初始化事件。

命令结果同样只有一个应用 seam。`FWacomBattleHUDCommandController` 获取命令前 Snapshot、构造 `FWacomBattleCommandPresentationContext` 并调用 `ResolveCommand()`，随后无论成功失败都把 `FBattleResolution` 交给 `FWacomBattleHUDResultApplicator`。Applicator 只接受当前 Session 上 `VersionBefore == LastAppliedVersion`、`VersionAfter == VersionBefore + 1` 且 `PostSnapshot.Version == VersionAfter` 的成功结果；失败、重复、过期或跳版本结果不刷新 Snapshot、不写 Combat Log、不发布事件，也不改变 targeting。成功 PlayCard 先记录 commit facts，再清 targeting / 恢复 Idle。所有成功 `PlayCard` 都进入同一个命令级 phased plan：`CommandSourceOut → CommandPrimaryTarget → CommandOutcome → CommandSourceReturn`。规则和 PostSnapshot 已立即结算；各阶段只用 Pre/Post Snapshot、Battle Events 与 Presentation Journal 还原视觉中间态，并以真实 Playback 完成、手牌刻印峰值、Event Queue 完成或牌印迁移完成推进，不用推算 Timer 猜测动画边界。

`CommandSourceOut` 先完整播放离手源卡效果；仍留手的源卡只执行 `CardUseReformOut` 并保持隐藏。`CommandPrimaryTarget` 再播放手牌目标刻印或世界 `TargetConfirmed`。`CommandOutcome` 按事件 Sequence / Deck Step 处理费用重写、EffectBadge 局部重写、目标弃牌/消耗、Damage、普通弃牌迁移、洗牌与抽牌；命令内的费用与 Badge 重写会临时成为 blocking feedback，并行播放后等待较晚完成者，普通独立 Snapshot 刷新仍不阻塞。所有可见结果结束后，只有留手源卡进入 `CardUseReformIn`，在最新布局位置反向生成。阻塞式击倒选择被单独延后到源卡回手之后；`BattleEnd` 是直接终止边界，会清理隐藏卡、后续 Return 与 Presentation Stack，不伪造回生。EndTurn 继续在同一 coordinator 中保留弃牌、保留、敌方行动和左右手生成阶段。

命令阶段中的 first-person 临时材质还受渲染 Ready Gate 约束。Surface、费用数字和 Badge 数字各有独立 Generation；Hint 到达后先安装进度 0 MID/Brush，等待一次真实 Slate Paint，再由零 Delta 启动语义边缘。等待期间不会消费声音、目标 Peak、Enter Started、DrawPile 减数或 Departure Gate，且 `HasActivePresentationPlayback()` 仍为真，因此冷启动不会让 SourceOut / Target / Outcome 提前串阶段。费用与 Badge 可并行准备，Outcome 等待较晚完成者；`0.75s` 内仍未就绪时恢复权威值并走原 fallback，不改变已经立即结算的 Battle 状态。

键盘数字牌位快捷键进入 `AWacomPlayerController` 后只提交 one-based hand index 和当前 pointer widget-space 坐标给 `UBattleHUD`；当前可见战斗手牌的 `CardInstanceId` 由 BattleHUD / first-person hand bridge 根据已同步的 hand snapshot 解析，PlayerController 不直接读取 `UBattleSession` 或 `FBattleSnapshot`。快捷键启动的 active drag 由 `UWacomGameViewportClient` 注册的 Slate Input Processor 在 Widget 路由前处理中性右键取消：事件被消费，Anchor 广播既有 cancel 生命周期，源卡返回手牌，不执行 release、非法目标反馈或 Battle 命令；`HandleRerouteInput()` 与 PlayerController `InputKey()` 保留同条件 fallback，鼠标来源拖拽、Viewport 外点击和 idle 状态的右键保持原路由。键盘 `IA_Wait` / `IA_EndTurn` 进入 BattleHUD 前同样由 `AWacomPlayerController` 做 first-person hand 输入仲裁：如果当前卡牌层存在 active gesture，快捷键先取消该手势并被消费；只有卡牌层处于 idle / cancelled 时才调用 `OnWaitRequested` 或 `OnEndTurnRequested`。

`BattleInputReady` 是 BattleHUD 级玩家命令 gate，不属于 `UBattleSession` 规则阶段。进入战斗镜头 staging 与 first-person card prewarm 期间由 `FWacomBattleHUDResultApplicator` 关闭：CommandBar 按钮禁用，`CanSubmitPlayerActionCommand()` 返回 false，first-person hand release / Wait / EndTurn 等普通玩家命令不会提交；HUD 仍可同步场景敌人和接收非交互表现。镜头完成并激活 Battle camera look 后，GameMode 只发出 Camera Ready 生命周期信号，不能直接修改 gate 或 suppression。超时仅释放表现门，不取消仍在进行的 Required Visual 异步驻留；Optional Enter Sound 失败不阻塞输入。

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
2. `TurnEndRetain`：只为 `CardsRetained.CardInstanceIds` 中仍在手牌的普通卡建立像素封存；左右手 Anchor 不参与。封存建立完成后进入非阻塞 Held，允许后续敌人行动和抽牌继续，但卡牌只保持低强度刻印与轻微抬升，不改变 authored hand ZOrder。
3. `EnemyAction`：复用现有 battle event presentation queue 播放敌人行动相关 cue / delay / battle end / knockdown modal。
4. `TurnStartDraw`：用 draw checkpoint snapshot 播放新回合 `Drawn` 入场。
5. `TurnStartHandAnchorEnter`：如果 draw checkpoint 中出现了上一手牌 checkpoint 没有的左/右手 anchor，则在普通抽牌后提交完整 hand snapshot，并播放 `HandAnchorEntered` 生成入手。
6. `TurnStartRetainRelease`：向仍在最终手牌中的保留卡发送 `RetainedRelease`，等待刻印和额外 Retain Transform 缓出归零；缺少 draw checkpoint 时在本次 EndTurn 最后一个安全阶段执行。

手牌 phase 的完成条件由 first-person card layer 的 production playback 状态提供：仍有 active enter、exit outgoing、Retain Sealing/Releasing 或 Card Glyph Transfer 时保持 phase busy；Retain Held 明确不计入 busy，因此不会阻塞敌人行动与抽牌。普通弃牌和弃牌堆洗回使用传输种类加 Batch Sequence 去重并按 FIFO 播放，新批次不得强制完成前一批。timeout 不是直接跳过：触发时 coordinator 必须先 force-settle 当前 Anchor / Layer，清除 pending hints、Retain Held、Impact 和临时牌堆计数，再启动下一 phase，避免旧动画跨阶段重叠。没有 journal 或 journal 无有效 phase 时，非 EndTurn / fallback 路径继续使用原来的 loose event hints 与 event queue；loose `CardsRetained` 使用自动释放的短反馈，不会永久封存。

显式战斗奖励卡使用同一通用 resolved-command planner。`CardGainedResolved` checkpoint 先提交包含新卡的中间 Snapshot，并建立 `CommandCardGained` phase；该 phase 等待真实 `Gained` Enter 与 Gain Reveal 完成后，才处理 checkpoint 之后的 `CardDiscarded / HandLimitDiscarded`、洗牌或抽牌。因此手牌已满时严格表现为“新卡结晶入手 -> 对应卡普通弃牌迁移”，而不是直接把最终 Snapshot 中已经离手的卡跳过。中断、BattleEnd 或超时只恢复权威 Post Snapshot，不补播未开始的阶段。

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

敌人常驻状态阅读已经收敛到敌人 Host 头顶的 screen-space 聚合面板：`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 承载 abstract `UWacomBattleEnemyPanelWidget` 的正式 WBP。App-private scene-enemy coordinator 是唯一 `FBattleSnapshot.Enemies -> FWacomBattleEnemyPanelViewData` 构建者；一个 Host 面板实例只消费一个 Enemy view，面板内按稳定 `EnemySlotId + PartSlotId` 创建、排序和复用 `UWacomBattleEnemyPartEntryWidget`。Widget 不持有 BattleSession、不读取 Snapshot、不使用 Tick，也不创建 C++ fallback 布局。普通部位 hover 复用所属敌人的聚合面板响应；`AWacomBattleEnemyPartActor` / `UWacomBattleEnemyPartPresentationComponent` 不再创建部位级常驻状态 Badge。

正式资产沿用兼容路径 `/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget` 与 `/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget`。WBP 独占布局、字体、颜色、九宫格和 `Intro / Damage / Shield / Destroyed / Context` 动画；C++ 只应用 ViewData 和触发语义动画。单部位与多部位现在共用同一紧凑语义：每个部位上排显示 Initiative 与 Intent，中间只有一条独立按 `CurrentHp / MaxHp` 填充且只写当前 HP 数字的生命段，下排最多显示 3 个 Buff 和 `+N`。Shield 不再占用第二条进度条；正护盾以不参与布局宽度的蓝色九宫格外框、盾牌徽章和准确数值覆盖在本部位 HP 段上，零护盾整体折叠。Preview 只覆盖 projected HP / Shield / Initiative / Status 显示，清理后恢复真实 Snapshot 值，且绝不触发真实 Damage / Shield / Destroyed pulse；同次真实刷新有多个变化时按 `Destroyed > Damage > Shield` 只播放最高语义。Destroyed 部位保留原段位、变暗并显示终态覆盖，不折叠或重排。新条目使用弱对象 timer 执行 `0.045s` stagger，clear / destruct 会清理 timer 和动画。

恰好包含一个有效 Definition PartSlot 的 Host 默认使用 `/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget` 与 `WBP_WacomBattleEnemySinglePartEntryWidget`，并保持约 `250 x 84`。多部位面板把同一条目按 `EnemyDefinition.Parts` 顺序放进 `HorizontalBox`，每段固定等宽 Fill，段间只保留像素分隔线；段宽不按 MaxHP 分配，因此低 HP 部位的 Initiative / Intent / Buff 不会被挤到相邻段。两者消费完全相同的 `FWacomBattleEnemyPanelViewData`，不建立单部位专用 Snapshot 链。紧凑条不再显示敌人名、部位名、Intent 名称或 Resistance；这些事实只进入详情面板。Destroyed 优先显示 `X` 并抑制 Initiative / Intent change pulse。显式 `EnemyPanelWidgetClass` 始终拥有最高优先级。

紧凑条的点击只代表“检查部位”的 UI 意图。`UWacomBattleEnemyPartEntryWidget` 通过透明 `InspectHitTarget` 向 Panel、Host 和 scene-enemy coordinator 上报完整 `FBattlePartSlotIdentity`；只有 `EBattleUIState::Idle`、PlayerAction、没有拖卡、Action Preview 或表现结算时才启用。TargetSelect、拖卡、Resolving、BattleEnd 和 source clear 会立即关闭详情并把紧凑条恢复为点击穿透，场景目标射线和 first-person 手牌输入仍走原路径。Escape / Back 先取消既有拖卡或目标选择，随后才在 Idle 关闭详情；不新增 `Inspect` Battle UI state，也不依赖焦点或 Tick。

BattleHUD 生命周期内只维护一个 `/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget`。App-private `FWacomBattleHUDEnemyInspectionCoordinator` 把现有 `FWacomBattleEnemyPanelViewData` 与选中的稳定 Part identity 组合成 `FWacomBattleEnemyInspectionViewData`：点击同一部位关闭，点击其它部位或其它敌人原地切换；Snapshot 刷新复用同一个 Widget 和 Definition 顺序的导航 Row。左侧显示敌人名、剩余部位整体状态和部位导航；右侧显示选中部位的当前/最大 HP、Shield、Initiative、Intent 名称、Intent Initiative、Resistance、完整 Buff / 层数和 Destroyed 状态。左右面板各自从屏幕侧边进入，中央战场与底部手牌不被遮罩或暂停。BattleEnd、Host/Part 移除、session/source clear 和 HUD destruct 会强制清理。

`FWacomBattleEnemyPartEntryViewData.CurrentIntentId` 是图标选择的稳定表现事实。`DA_EnemyIntentPresentation_Default` 只做准确 `IntentId -> Brush` 映射，不读取显示名、不分析效果；未命中映射时使用白色四角星。Action Preview 若表明部位即将行动，Initiative 显示 `0`，但 Intent ID、名称、意图先机和抵抗继续使用命令前 Snapshot 的当前 Intent，不提前展示行动完成后才轮换出的下一 Intent。Preview 变化不触发 Initiative / Intent pulse。

Host 类解析顺序为：显式 `EnemyPanelWidgetClass` override；恰好一个有效 Definition PartSlot 时的 `DefaultBattleEnemySinglePartPanelWidgetClass`；其它情况的 `DefaultBattleEnemyPanelWidgetClass`。单部位默认类无效时记录错误并回退多部位默认类；最终无有效类时隐藏面板，不实例化完整原生 fallback。WidgetComponent 默认 `DrawAtDesiredSize=true`、Pivot `(0.5, 1.0)`，1–4 个部位向上增长；关闭 Desired Size 才使用固定 `EnemyPanelDrawSize`。普通面板超过 4 个 Definition PartSlot 时只给制作警告，未来由 Boss 专用 WidgetClass 承接。`bEnemyPanelVisibleByDefault=false` 时，面板仅在 hover 或有效 Preview 上下文显示。

场景敌人视觉绑定正式入口是 `ABattleTriggerActor.SceneEnemyHostSlots + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`；规则敌人列表由 `ABattleTriggerActor.EncounterDefinition` 转换成 `FBattleInitParams.EnemySlots`。新制作应把敌人做成 Host 蓝图 prefab：给 Host 配置 `EnemyDefinition`，在 Host Details 点击“从 EnemyDefinition 同步部位”，由 Editor service 从纯 Authoring Report 的计划生成 / 对齐 ChildActorComponent，再摆放各部位的命中与视觉；然后在 Trigger 选好 `EncounterDefinition` 后执行 `SyncSceneEnemyHostSlotsFromEncounter()`，在生成的 `SceneEnemyHostSlots` 中按 `EnemySlotId` 绑定对应 Host。

Trigger 显式 `SceneEnemyHostSlots.EnemySlotId` 必须填写且不重复，并对应 `EncounterDefinition.EnemySlots[].EnemySlotId`。配置 `EncounterDefinition` 的正式 Trigger 必须用 `SceneEnemyHostSlots` 覆盖每个有效 EnemySlotId；缺 Host、漏映射或多余 EnemySlotId 都是编辑器验证错误。进入战斗时 GameMode 把当前 Trigger 的 Host 列表传给 BattleHUD，HUD 只同步当前 Host registry 中扫描到的 PartActor bridge。HUD registry 是 Host 列表，不维护“主 Host”兼容缓存。

敌人视觉有两条正式制作模式：

- `HostAuthoringMode=SimpleHostVisual` 推荐用于普通小怪：在 `AWacomBattleEnemyActor` 的 `Presentation|Host Visual` 配整体 `PaperSprite` 或 `PaperFlipbook`，Head / Body / Tail 等子 `AWacomBattleEnemyPartActor` 只负责 `PartId / PartSlotId / HitBounds`、target bridge、Presentation、预测和状态 Badge。PartActor 没有 `VisualLayers` 时进入 `HitOnly` 视觉模式，自身不生成可见体；反馈、Badge 和预测仍跟随该部位自身的 Presentation。
- `HostAuthoringMode=MultiPartVisualLayers` 用于多部位精英 / Boss：PartActor 是单个规则部位的表现容器。`HitBounds` 是唯一 hover、点击和拖卡命中范围；`VisualLayers` 只负责多张图的相对位置、旋转、缩放、排序、颜色、显隐和轻量序列帧播放。每层用 `LayerMode` 选择 `StaticSprite` 或 `Flipbook`，并可选配置同模式的 `DestroyedSprite / DestroyedFlipbook`；破损 Flipbook 以正数 `DestroyedFlipbookPlayRate` 非循环播放并保持末帧。`VisualLayers` 非空时优先于 Host 的 HitOnly 语境；为空且没有 Host 整体视觉时进入 `None / MissingVisualResource` 诊断。制作模式只改变 validator / Details 诊断，不清除或转换已有视觉资源。

`SimpleHostVisual` 的 Flipbook Host 可以配置 `UWacomBattleEnemyHostAnimationStyle`。现有 `HostFlipbook` 始终是 Idle 真源；Style 只声明 `DefaultActionClip`、显式 `IntentId -> Clip` 和 `DestroyedClip`。显式 Intent 映射优先于默认 Action，运行时不根据 Intent、Actor 或资产名称猜测动画。Clip 资源和正数有限 `PlayRate` 由 Data Validation 检查；Host 配置 Style 但不是有效的 Simple Flipbook Host 时进入制作警告。

本地 PIE 可以执行 `Scripts/SetupBattleEnemyHostAnimationPIEAssets.py`，幂等生成 `/Game/Art/WacomPIE/EnemyHostAnimation/DA_EnemyHostAnimation_BattleWarrior_PIE` 和 `BP_SnakeHost_BattleWarrior_PIE`。脚本使用本地 `PaperAssets/Party/BattleWarrior` 的 Idle、Attack、Block、Cleave 和 Downed，只配置复制出来的 Debug Snake Host；不会修改正式 `/Game/Wacom` Host。`/Game/Art` 由 worktree 的 D 盘 Junction 提供且被 Git 忽略，这组资产只用于 PIE，不代表授权、Git LFS 或正式出货接入已经完成。运行命令：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -ExecutePythonScript='<Worktree>\Scripts\SetupBattleEnemyHostAnimationPIEAssets.py' -Unattended -NoSplash -NullRHI
```

正式 TrainingWarrior 不再走这条 PIE 脚本。`BP_EnemyHost_TrainingWarrior` 使用 `/Game/Wacom/Art/Enemies/TrainingWarrior` 下已晋升并由 Git LFS 管理的 Idle / Attack / Block / Cleave / Destroyed：Idle 循环 1.0；Attack 故意从 Style 的 Default Action 以 0.75 播放；`TrainingWarrior.Body.Guard` 显式映射 Block 1.0；`TrainingWarrior.Body.Cleave` 显式映射 Cleave 0.75；整体破坏映射 Destroyed 0.75。Host 只有一个 hit-only Body PartActor，命中与拖卡沿用正式 Pixel Style。正式运行时不加载 `/Game/Art`，也不根据动画名称猜 Intent。

每个 PartActor 原生携带一个无碰撞 `ImpactAnchor`，默认位于 `HitBounds` 中心；`ImpactAnchorRelativeLocation` 以厘米为单位，只用于世界命中特效的美术微调，不改变命中盒、目标身份或 Battle 规则。HitOnly 与 VisualLayers 两条制作路径共享这一合同。Authoring Status / debug view / summary 会报告锚点组件名和世界位置，非有限偏移会被 Data Validation 判为错误。

Host 整体视觉的 `HostVisualMaterialOverride / bHostVisualCastShadow` 和每个 VisualLayer 的 `MaterialOverride / bCastShadow` 会应用到动态生成的 `UPaperSpriteComponent / UPaperFlipbookComponent`。需要 Sprite 投射阴影时，材质覆盖应使用 Paper2D 的 `MaskedLitSpriteMaterial` 或等效 lit masked 材质，并确保场景光源开启阴影；默认字段为空 / false 时保持无材质覆盖、无投影。

Host 整体视觉和 PartActor `VisualLayers` 的生成组件会在构造 / Details 刷新 / 显式刷新时重建，并在 PIE / runtime `BeginPlay` 再刷新一次，保证蓝图视口和实际运行一致。生成的 PaperSprite / PaperFlipbook 组件只在 Actor 拥有有效 `World` 时注册；蓝图模板、CDO 或 Details 预览这类无 World 场景只更新配置和诊断缓存。

Details 制作分组按主路径整理：Host 的 `Identity` 配置 `EnemyDefinition / EnemySlotId`，`Authoring` 选择 `HostAuthoringMode` 并提供定义同步 / 非生成刷新按钮，`Presentation|Host Visual` 配普通怪整体 sprite / flipbook，`Presentation|Host Animation` 配语义动画 Style，`Presentation|Badge Layout` 配部位 Badge 错开，`Authoring Status` 显示只读制作诊断和最近同步结果，`Debug Sample` 只放开发样例按钮。PartActor 的 `Identity` 主要填写 `PartSlotId`，`PartId` 由定义同步派生，`Collision` 配置 `HitBoundsExtent`；普通怪 PartActor 可以不配独立视觉，精英 / Boss 正式美术走 `Visual Layers`。PartActor 不再提供旧 StaticMesh 原型可见体入口。

Host Details 的 `Authoring Report`、Host validator、debug view 和 summary 使用同一个实时纯报告，不再依赖 Actor 上缓存的“当前 Authoring Status”。Host `AuthoringState=Ready` 表示 `EnemyDefinition`、子 PartActor、`PartId` 和 `PartSlotId` 对齐；常见异常包括 `MissingEnemyDefinition`、`NoPartActors`、`DuplicatePartSlotIds`、`PartSlotMismatch` 和 `PartDefinitionMismatch`。报告还显示待新增、待修正、无效定义槽位和 surplus；重复读取报告或运行 `IsDataValid()` 不得改变身份、组件、VisualLayers、Flipbook 播放、topology revision、Last Sync 或 package dirty 状态。PartActor 仍保留自身的制作状态预览，区分 `UsingVisualLayers`、`HitOnly`、`MissingIdentity`、`InvalidHitBounds` 和 `MissingVisualResource`。

Host 整体视觉和 `VisualLayers` 都只属于表现层。有效 `EnemyPartActed`（`Count > 0`）在 presentation queue 中保留完整 `FBattlePartSlotIdentity` 并严格串行；`SimpleHostVisual` 继续调用 Host Animation Style，`MultiPartVisualLayers` 则精确调用匹配 PartActor 的 `UWacomBattleEnemyPartAnimationStyle`。缺 Host、Part、Style、Clip 或有效目标层时同步完成请求，不按 Intent 名、Actor 名、Layer 顺序或首个部位猜测。

Part Animation Style 只声明一个稳定 `TargetVisualLayerId`、`DefaultActionClip` 和显式 `IntentId -> Clip`；显式映射优先于默认 Action，Destroyed 仍由 VisualLayer 自身的 `DestroyedFlipbook` 合同拥有。行动在同一个 `UPaperFlipbookComponent` 上非循环播放，以 `OnFinishedPlaying` 和一次性 duration watchdog 作为队列 barrier，结束后恢复 authored Flipbook、PlayRate、Looping、StartTime 与 AutoPlay。Destroyed 优先并会安全结束残留 Action；Snapshot refresh、相同 Host 设置和无拓扑变化的 registry 操作不会重播或重置。复杂多层联动、PaperZD 与 Boss Phase 仍留给后续能力。

Host / Part Action Clip 共享 `ImpactNormalizedTime`（默认 `0.55`）和 `OnImpact / OnCompleted` 双阶段合同。队列开始行动时保持 Journal 的行动前 Combat facts；Impact timer 到点才应用对应 `SnapshotAfter` 的玩家 HP、护盾、状态与敌人护盾、Intent、Initiative 等 Combat facts，动画真正结束后才释放下一行动的 barrier。提前完成或 watchdog 会先补发一次 Impact 再 Complete；缺失 Host、Style、Clip、Layer 或 World 时两个回调按同样顺序同步完成。`Count == 0` 不播动画但立即应用行动后 facts，因此中毒等行动后结算仍可反馈。Combat-only refresh 不同步手牌、牌堆或 first-person transition；Journal 缺失或序号不匹配时不猜中间状态，最终权威 Snapshot 仍负责回收。

上述双阶段生命周期只有一个实现：App-private `FWacomBattleEnemyActionPlayback` 持有弱 UObject timer、watchdog、serial 和 exactly-once 回调状态。Host / Part visual component 不再各自保存 timer 或 pending callback，只负责原地换片与完成后的视觉收尾。natural finish / watchdog 会补发尚未到达的 Impact；cancel 不提交旧 Impact但一定完成 barrier；后续播放通过 serial 令旧 timer 和回调失效。Host Destroyed 复用同一生命周期但不创建 Impact timer，并保持末帧终态。

表现编排自身的短延时不由 Queue、Plan 或 Stack 分散持有。App-private `FWacomBattlePresentationTimerOwner` 以 `EventQueueAdvance`、`PresentationPlanPoll` 和带 Stack Entry Id 的 `StackEntryExit` 为稳定 key，保存注册时的原始 World、handle、serial 与业务回调；引擎 TimerManager 只持有弱 owner delegate。正式 `Shutdown / ClearQueue / ClearPresentationPlan / ClearBattlePresentationStack` 精确撤销对应 key，HUD 在 World 已不可访问的析构路径只遗弃 owner 回调，旧 timer 即使晚到也因弱 owner / serial 失效。该收敛不改变 `0.16s` Queue 延时、`0.03s` Plan Poll、`0.01s` Stack Exit、已有 barrier 或视觉顺序；cancel 也不会补发 Impact 或虚构完成后的表现事实。

正式 Snake prefab 为 `/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake`。它绑定 `DA_Enemy_Snake`、`EnemySlotId=Enemy`，使用 `MultiPartVisualLayers`，清空 Host Sprite、Flipbook、Animation Style 和面板 override；三部位 Definition 因此自动使用现有多部位 WBP。Builder 必须通过 `FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition()` 生成三个 ChildActorComponent，再按稳定槽位配置：

| PartSlotId | Relative Location | PartId | HitBoundsExtent | Visual scale | Idle offset |
|---|---:|---|---:|---:|---:|
| `Head` | `(96,-6,16)` | `Snake.Head` | `(42,38,42)` | `0.85` | `0.00s` |
| `Body` | `(0,0,0)` | `Snake.Body` | `(62,46,42)` | `1.00` | `0.04s` |
| `Tail` | `(-92,16,-8)` | `Snake.Tail` | `(48,34,34)` | `0.70` | `0.08s` |

每个 Part 只有一个 `Snake.<Part>.Main` Flipbook VisualLayer，复用 `/Game/Wacom/Art/Placeholders/Enemies/Snake` 的 Idle，并绑定自己的单帧 Destroyed Flipbook；局部换图 marker 保持 `0.35`。该 Slime 组合可提交和开发验证，但发布审计必须用 `-FailOnPlaceholder` 阻止出包。通用 Part Action runtime 已可用，但正式 Snake prefab 当前故意不配置 `PartAnimationStyle`，因为没有获准提交的 Head / Body / Tail 行动素材；因此三段仍只持续错帧 Idle。最后部位破坏后保持三个局部终态，等待返回探索镜头后统一退役。

`EnemySlotId` 由 Host / Trigger 注入，不在 PartActor 模板里手填。Host validation 会同时检查 `PartId` 与 `PartSlotId`：`PartId` 必须对应 `UEnemyPartDefinition::PartId`，`PartSlotId` 必须对应 `UEnemyDefinition.Parts[].PartSlotId`。蛇的正式绑定身份是 `Enemy + Head/Body/Tail`，不是 `Enemy + Snake.Head/Snake.Body/Snake.Tail`。

Host Details 的“从 EnemyDefinition 同步部位”是显式、幂等的通用制作入口。`WacomEditor` 先为全部选中 Host 建立纯报告，再用一次 `FScopedTransaction` 应用计划：仅按 `PartSlotId` 匹配定义，为已有唯一槽位派生 `PartId`，为缺失槽位新增零相对变换、默认 facade 的 PartActor ChildActorComponent。Host Blueprint 模板上的新增部位写入 SCS，并在 Compile / Save / Reload 后保留；关卡 Host 实例上的新增部位是 transactional InstanceComponent，不反向修改来源 Blueprint。同步不覆盖已有位置、`HitBoundsExtent`、`ImpactAnchorRelativeLocation` 或 `VisualLayers`；空、未知和重复部位只标记为 surplus，不静默删除；无效定义槽位跳过并进入 Last Sync。无实际变化时不创建事务、不 dirty package；创建失败明确返回 `ApplyFailed / PartiallyApplied`。Actor 上不再暴露旧 `CallInEditor` 同步函数。

Host 的 `RefreshBattleEnemyPartAuthoringState()` 仍是 Construction / 显式表现刷新入口：它可以刷新 Host visual、扫描 PartActor 并更新 Part 表现状态，但不负责纯报告求值、Validator 或 runtime Snapshot sync。它不会创建 PartActor，也不会自动补齐身份；需要生成或派生时使用 Host Details 的正式同步按钮。

Trigger 正式单蛇配置使用生成资产 `DA_Encounter_SnakeSingle`：`PersistentId` 填关卡唯一值，`EncounterDefinition=DA_Encounter_SnakeSingle`；执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 `SceneEnemyHostSlots[0].EnemySlotId=Enemy`，再把 `SceneEnemyHostSlots[0].SceneEnemyHost` 指向关卡里的 Snake Host 实例。`EncounterDefinition` 正式入口不能缺 Host 映射。

Debug 蛇样例已迁入 Host Details 的 `Advanced Debug` 折叠区，只支持单选 Host，并由 `WacomEditor` 使用独立事务执行。先在 Host 蓝图中放好 Head / Body / Tail 三个 PartActor，再点击“配置 Debug 蛇样例”；该操作扫描 live 子 Actor 和 ChildActor template 并写入样例身份、位置与 badge stagger，不自动创建缺失部位，也不创建正式美术资产。普通蛇的正式美术由 Host `Presentation|Host Visual` 配整体图，精英 / Boss 才优先在各 PartActor 配 `VisualLayers`。

Host 蓝图视口中的 `SnakeHeadPart / SnakeBodyPart / SnakeTailPart` 是 `ChildActorComponent`；关卡 Outliner 会在 Host 实例下显示它们生成出来的 `WacomBattleEnemyPartActor...` 子 Actor，这是正常现象。运行时和摆放校验优先使用这些已生成的真实子 Actor；只有蓝图模板 / CDO 等没有生成实例的场景才读取 ChildActor 模板，避免同一组 Head / Body / Tail 被重复计入 Host registry。

每个 PartActor 默认携带两类运行时组件：

- `UWacomBattleEnemyPartWorldTargetBridgeComponent`：只负责 `EncounterId / EnemySlotId / PartSlotId -> PartInstanceId` 绑定、当前 HUD registry 标记、targetable 绑定诊断，以及把运行时身份写入 `UWacomInteractionTargetComponent`。
- `UWacomBattleEnemyPartPresentationComponent`：负责 hover target、TargetSelect 可选提示、first-person drag preview、`TargetConfirmed / DamageDealt / EnemyPartHpEmptied` 语义 Cue Playback、只读预测 Widget 和常驻状态 Badge。Cue Playback 互斥并按 `Destroyed > Damage > TargetConfirmed` 覆盖，只在活动期间 Tick；Cue 不再写敌人 Scale。普通 Hover / Targetable 仍使用轻量 authored scale；拖卡已经锁定具体世界部位时恢复 authored base scale，由独立像素目标框承担空间提示，不再叠加旧 `1.08x` Drag Preview 放大。

Cue 的持续时间优先读取 `FWacomBattlePresentationTargetCue.Duration`，无效时使用 PartActor 的 `CueHoldSeconds`。`TargetConfirmed=0.24s`、`Damage=0.30s`；世界伤害批次在首个 Damage 前保留约 `0.14s` 的确认可读窗口，随后 Presentation Queue 仍按伤害事件顺序逐条播放。Cue 的 `Seed` 只服务稳定的表现随机：确认由来源卡实例与目标部位构造，伤害由事件 Sequence、目标部位和实际 HP 损失构造；它不是 Battle RNG，也不参与规则恢复。解绑、BattleEnd、EndPlay、目标组件/Style 切换和 reset 会清理 Playback、关闭 Tick、立即停用 Niagara 并恢复 authored scale。

世界目标确认、伤害与部位破坏反馈使用 `UWacomBattleEnemyPartImpactStyle` 和一个由 Part Presentation 按需创建、复用的 `UNiagaraComponent`。Style 解析顺序固定为 `PartActor.ImpactStyleOverride -> EnemyHost.DefaultImpactStyle -> None`；关闭 Part 的 `bEnableImpactFeedback` 或缺失 Style/System/MI 时仍消费 Cue，但不创建 VFX，也不会阻止破损资源终态。`TargetConfirmed` 使用角部像素刻线收束与语义中心方印，`Damage` 使用中心硬闪、菱形冲击环和短程像素碎片；`Destroyed` 使用不受 Flash Off 影响的中心硬质裂印 / 角部撕裂结构和可被 Reduced Motion / DecorativeIntensity 关闭的远距大碎块。三者读取当前 PartActor 的 `HitBounds`，按摄像机平面投影得到基准直径，并由 Style 的 fallback / min / max 控制异常尺寸。

`EnemyPartHpEmptied` 被 Playback 接受后立即开始 Destroyed 粒子；当 0.30 秒 Cue 进度跨过 `DestroyedVisualSwapNormalizedTime`（默认 `0.35`）时，VisualLayer Component 在原有 Paper 组件上调用 `SetSprite / SetFlipbook`，不执行 `RefreshVisualLayers()`。缺少对应破损资源的层保持 authored 资源；HitOnly Part 只有局部粒子，整体终态仍由 Host Downed 承担。重复 Cue 幂等，强制完成 Cue 会先应用终态；初次绑定已 destroyed Snapshot 直接恢复终态且不重播粒子。普通 Snapshot、相同 Host 重设和 topology rebuild 不恢复终态；新战斗首次接管 Host 时统一恢复 Host Idle 与全部 Part authored 资源。

拖卡悬停世界部位使用独立的 `UWacomBattleEnemyPartTargetPreviewStyle`、App-private Preview Playback 和第二个按需创建/复用的 Niagara Component。Style 解析顺序为 `PartActor.TargetPreviewStyleOverride -> EnemyHost.DefaultTargetPreviewStyle -> None`；它与短时 Impact Component 分离，因此持续悬停不会占用或重启 TargetConfirmed / Damage。有效目标显示覆盖整个 `HitBounds` 摄像机平面投影的冰蓝/象牙金四角像素框，并在中心保留空心菱形；进入约 `0.18s` 从 `1.12x` 收束，停留阶段只做弱亮度呼吸。无效世界目标显示暗紫红断裂框且没有中心菱形。切换或离开约 `0.10s` 淡出；预测 Badge 继续解释先机和拒绝原因。`UIMotionMode=Simplified` 直接显示静态语义框，`FlashEffectMode=Off` 只关闭呼吸辉光，不隐藏有效/无效判定。

Niagara 公共合同固定为 `User.EffectKind / Duration / Intensity / Seed / DecorativeIntensity / ReducedMotion / ImpactMaterial / PlaneRight / PlaneUp / TargetDiameter`；目标预演分支另外使用 `TargetWidth / TargetHeight / PreviewAmount / PreviewValidity / PreviewPulse`。`EffectKind=0/1/2/3` 分别表示 TargetConfirmed、Damage、TargetPreview、Destroyed。Damage 强度只改变视觉密度/尺寸，公式为 `clamp(0.75 + sqrt(max(实际 HP 损失, 0)) * 0.10, 0.80, 1.80)`，不延长 Cue；Destroyed 默认强度和覆盖倍率均为 `1.35`。特效平面使用当前玩家摄像机 Right/Up，在 `ImpactAnchor` 朝摄像机方向偏移 Style 指定距离（默认 `2cm`），保留正常深度遮挡。`UIMotionMode=Simplified` 由 Niagara 保留静态语义方印 / 裂印并关闭收束、扩散和碎片位移；`FlashEffectMode=Reduced/Off` 只把装饰强度降为 `35%/0%`，语义中心方印、裂印、破损换图和声音保留。设置在每次 Cue 或 Preview 开始时快照。

视觉真源为 `DShader/Material/World/M_WacomBattleEnemyPartImpactPixel.dsm` 与 `DShader/Shared/WacomBattleEnemyPartImpactPixel.dsh`，命中默认 MI 为 `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartImpactPixel_Default`，目标预演默认 MI 为 `/Game/DreamMaterials/World/MI_WacomBattleEnemyPartTargetPreviewPixel_Default`。材质是 `Surface / Unlit / Translucent / TwoSided`，开启 Niagara Sprite usage，不读取贴图、Time 或 Noise。Impact 粒子的 Dynamic Material Parameter 为 `X=ShapeKind、Y=NormalizedAge、Z=PaletteVariant、W=Semantic/Decorative`；TargetPreview 为 `X=PreviewShape、Y=PreviewAmount、Z=Validity、W=Pulse×DecorativeIntensity`。DataAsset 管理 System、MI、节奏、覆盖尺寸、摄像机偏移和可选声音，颜色、像素密度、线宽和辉光只在 MI 调整。

Niagara Graph 的项目真源是现有 System 资产加 WacomEditor-only 生成器：`WacomBuildBattleEnemyPartImpactNiagara` 会验证十五个 User Parameter、`ConfirmStamp / DamageCoreWave / DamageFragments / DestroyedFracture / DestroyedFragments / TargetPreview` 六个 Emitter 和 Sprite Renderer 的 `User.ImpactMaterial / Particles.DynamicMaterialParameter` 绑定，再按版本化生成合同幂等重建 Burst、Spawn/Update 属性与四通道写入并强制编译。缺少生成型 Emitter 时生成器会从现有 ConfirmStamp 模板补齐；合同版本、Emitter、Renderer、参数和编译全部有效时直接跳过 package 写入，避免随机节点 GUID 造成资产噪音。生成器依赖 UE 5.8 的 experimental Niagara external-edit API，因此严格留在 `WacomEditor/Private`；升级引擎时必须先重跑生成和编译验证，不能让运行时模块依赖 NiagaraEditor。编辑器关闭后可执行：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -run=WacomBuildBattleEnemyPartImpactNiagara -Unattended -NoSplash -NullRHI -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -ExecutePythonScript='<Worktree>\Scripts\SetupBattleEnemyPartImpactAssets.py' -Unattended -NoSplash -NullRHI -NoDreamShaderEditorBridge
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Worktree>\Wacom.uproject' -ExecutePythonScript='<Worktree>\Scripts\SetupBattleEnemyPartTargetPreviewAssets.py' -Unattended -NoSplash -NullRHI
```

第一条命令负责 Niagara Stack 和编译合同；后两条分别配置 Impact 与 TargetPreview 的 MI/Style/Debug Snake 引用，Python 不反射编辑 Niagara Graph。TargetPreview 脚本不会覆盖现有 Impact Style 的人工声音、强度和覆盖范围。日常美术调参不应改生成的 Set Variables 表达式：颜色、像素密度、线宽和辉光改对应默认 MI；伤害强度、预演时序/覆盖、摄像机偏移和声音改对应 Style；命中位置改 PartActor 的 `ImpactAnchorRelativeLocation`。只有视觉算法、粒子数量、世界尺寸或时间比例改变时才修改 WacomEditor 生成器并重建 System。

BattleHUD scene enemy coordinator 成对缓存 Bridge 和 Presentation：target handle 解析走 Bridge，表现 target 注册和反馈走 Presentation。target handle 必须携带完整 `EncounterId + EnemySlotId + PartSlotId` 才能命中当前 registry；`SourceObject` 即使指向当前 PartActor，也不会替代或修正错误 key。Host / PartActor debug summary 会合并两者事实用于 PIE 排查，但 passive UI 和 BattleSession 不依赖这个合并 debug。

Scene enemy 生命周期分为四条路径。构造、Details 修改和显式 Authoring 刷新可以重建 Host visual、PartActor `VisualLayers`、扫描 ChildActor 并更新制作诊断；Host/Part `BeginPlay` 或 HUD Host registry 变化时执行一次 runtime scene binding，注入 EnemySlotId、Host Style、视觉语境和稳定 Badge stagger；普通 `FBattleSnapshot` 刷新只更新 Bridge runtime facts、EnemyPanel view data、targetable、hover、prediction、drag preview 和 cue 状态；presentation event playback 只在现有 Host Flipbook 组件上原地切换语义 Clip。普通 Snapshot 和 event playback 都不调用 Host/Part Authoring refresh，不重建 PaperSprite/PaperFlipbook，也不重扫 Actor 层级。

BattleTrigger 的 runtime preparation 与 Host Authoring 也保持分离：`BuildBattleSceneEnemyHosts()` 只按 `EncounterDefinition.EnemySlots` 顺序解析有效 `SceneEnemyHostSlots`、向 Host 注入临时 `EnemySlotId` 并导出 Host 列表，不刷新 Host/Part Authoring Status，也不重建 Host visual 或 `VisualLayers`。PartActor 身份和 Host context 由 HUD registry 的 `InitializeRuntimeSceneBinding()` 统一注入。Trigger `GetBattleTriggerDebugView()` 复用无副作用的绑定解析，不修改 Host；coordinator 同时观察 Host 指针、`EnemySlotId` 和 topology revision，只有 Host 集合、身份或真实拓扑变化时才重建 registry。

`FWacomBattleHUDSceneEnemyTargetCoordinator` 只在 Host 集合变化、Host runtime topology revision 变化或已有 Host/Part 弱引用失效时重建 target registry。稳定拓扑下 Bridge/Presentation entry 和按 `EncounterId + EnemySlotId + PartSlotId` 注册的 BattlePresentation target 保持不变；Host identity 或拓扑重建不会重置正在播放的语义 Clip。BattleEnd Snapshot 到达时立即注销 Part target / presentation registry，并清理 hover、drag、panel 和 targetable；coordinator 只保留 Host 弱引用及最终 `bAllPartsDestroyed` facts，供已排队的整体 Destroyed 动画完成。`BattleEndSignal`、HUD shutdown、Session/source clear 或 Host 销毁会清空 retiring Host 并使旧回调失效；新战斗首次接管 Host 时恢复 Idle 和非终态。运行时不得通过 Tick 轮询 Actor 层级；显式动态 attach/detach live PartActor 的调用方必须通知 Host topology 失效。

HUD 的 retiring Host 只是一条“完成 Downed 后清队列”的 Battle 表现生命周期，不负责探索场景销毁。非撤离 Victory 成功提交给 Run 后，GameMode 立即禁用当前 BattleTrigger，并把有效 Trigger 弱引用交给返回探索双 barrier；镜头返回与 ExitBattle 后置工作都完成时，Trigger 才统一退役 Encounter 内 Host/Part 并销毁自身。这个阶段不刷新 Authoring、不重建 Host visual/VisualLayers，也不恢复终态 Flipbook；退出 PIE、对象提前销毁或弱引用失效时不会留下 HUD barrier。Aid 与 Destroy 都沿用同一个非撤离 Victory 合同；Withdraw、Defeat、Undetermined 和结算失败保留场景。

BattleHUD 不再构建或绑定敌方 2D fallback；点击、hover、drag target handle 全部通过当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge 完成。`EncounterDefinition` 正式入口缺 Host 会被编辑器验证阻止。点击、hover、drag target handle 的详细合同见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、CommandBar、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。正式 target drag / aim、world target release、hand-card target release 和无目标 drag-out release 都经 BattleHUD bridge / command flow 进入 BattleSession。无目标 Battle 卡向上拖过 `NoTargetCardDragOutCommitDistancePixels` 后进入 `ArmedForCommit`，只有保持 armed 并 release 才提交；拖回阈值内会解除 armed。`FWacomBattleFirstPersonDropResolver` 是 App-private drop intent adapter，集中处理无目标 armed intent、目标 drag view、Battle world probe 和 `UBattleSession::ValidateTargetWithCard()` 的 UI 映射。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

Battle / Run hand 共用 Card Depth。`FWacomFirstPersonCardDepthMotion` 在 Hover 时按卡面局部 pointer 计算倾斜，在 Drag 时按低通 pointer velocity 计算惯性倾斜，并与 rest / semantic transition 的压平以及 `ContactShadowLift` 一起做帧率无关平滑。`UWacomFirstPersonCardViewWidget` 将 `TiltX / TiltY / PerspectiveStrength / ContactShadowEnabled / ContactShadowLift` 写入现有 Retainer，同时把同一倾角以 `FWacomCardSurfacePerspectiveView` 交给内层 `UWacomCardView`：核心表面 MI 分层移动插画 / 卡框 / 稀有度饰条，实体出血 Badge / Durability 走 UMG RenderTransform。Badge 实体框和耐久底板还会复制自身 Brush 生成卡面内局部硬接触影；数字、文字和发光不投影，Reduced Motion 只保留静态接触。该路径只属于表现层，不改变规则、296×420 命中或外部 Surface Effect；局部附件影不替代 Retainer 内整卡实时 Alpha 阴影，外部 `CardShadowImage` 也不属于当前生产链。

First-person hand 的旧 `FeedbackOverlay / InteractionFeedbackImage / M_FirstPersonCard_FeedbackEdge` 已删除。Hover 只使用 lift、Fake-3D、卡面视差与接触阴影；Pressed 由 Motion Mixer 平滑合成默认 `0.985x`、向下 `2px`，并缩短接触阴影；权威成功 Commit 只保留约 `0.12s` 的运动脉冲。正式 Drag 只有在已经解析出真实目标且规则反馈为 `Invalid / InvalidCardTarget` 时，才在源卡 CardContent 左右显示低强度像素括角；空白泛化 Invalid 与尚未验证的 Probe 不显示。正式释放拒绝冻结方向和 Seed，播放 `0.97x` 压缩、反方向回弹、四段边缘裂痕和阻尼归位，可选硬引用 Deny Sound 只请求一次。敌人无效破框与目标手牌 focus 保持原有目标侧职责。Simplified Motion 只保留静态括角和短促裂痕/刻线，不执行实体运动。该收口不改变 card-target identity、validation 或 release 命令路径。

Battle / Run 共用的 first-person Slot 不再执行单卡视口底边钳制。卡面靠近屏幕边缘时的文字连续绘制由 `Fake3DSurfaceRetainer` 直接内容根的 `Clip To Bounds - Without Intersecting` 合同保证；该合同只修正 Retainer 内部 culling，不改变手牌扇形、卡牌位置或输入命中。

Battle hand 与 Run hand 共用 App-private `FWacomFirstPersonCardPresentationScalePolicy`。全局 UI 仍参照 `1920 × 1080`，但 first-person 卡牌表现参照 `2560 × 1440`：720p 目标物理倍率 `0.5`、1080p 为 `0.75`、1440p 及以上封顶 `1.0`。倍率由 Anchor 写入 resolved runtime config 与 Slot view；BattleHUD 不自行计算或覆盖。卡牌 `296 × 420` 主体、手牌布局、语义转场、反馈位移、Aim Arrow 和 Card Glyph Transfer 使用同一局部空间倍率，时序和输入阈值不变。

First-person hand 卡面和 first-person viewport 详情都从 `FHandCardSnapshot` 派生 `FWacomCardPresentationRuntimeContext`，再交给 `UWacomCardPresentationBuilder` 生成 ViewData。基础 runtime context 覆盖本场 `RuntimeCost` 与 `bIsPlayable`：卡面 Cost、disabled overlay、RuntimeCost-based 效果徽章和详情 `Sections` 会显示当前战斗事实；`Magnitude.Source.RuntimeCost` 一类数值来源会在 value run 中保留“相当于当前费用”等来源短语。详情面板正式渲染来源是 `FWacomCardDetailViewData.Sections`：Builder 按卡牌详情文档顺序组装 section，App-private `WacomCardExplanationCompiler` 从效果 / 被动 tag、runtime preview 和 `UWacomCardExplanationLexicon` 编译语义 `Blocks / Runs`；`UWacomCardDetailPanel` 只按 section 顺序渲染，不再按原始 `Passive.DisplayText` 或 token kind 推断分区。旧平行 `Description`、`ChangeLines`、`PassiveLines`、`TaskLines`、扁平 `TokenLines` 和旧 token flow 已从详情合同删除；`UCardDefinition::Description` 仅在该卡没有任何结构化详情 section 时作为普通正文回退，不解析旧 `{Effect.0}` 占位。费用变化、目标手牌 cost preview、被动正文和后续任务 / 预览内容都应反映到对应卡面数值或正式 `Sections` document。Battle 详情的数据来源和 source guard 仍由 `FWacomBattleHUDCardDetailController` 负责；预热、详情数据缓存、淡入淡出 / scale / follow motion 和稳定换边由 App-private `FWacomFirstPersonCardDetailMotionController` 与 Run first-person 详情共用。

First-person 详情面板的 viewport 生命周期由 `FWacomFirstPersonCardDetailPanelHost` 统一处理：只有拥有真实 local player / LocalPlayer 的 context 才允许 `AddToViewport`，HUD-only 自动化或离屏预热只构建面板对象并验证 ViewData / motion state。详情面板内部的 section 动态创建收口在 App-private `WacomCardDetailWidgetFactory`；section 内正文由 `UWacomCardDetailRichTextBlock` 渲染 RichText，并自动注册详情 inline icon decorator。战斗详情与背包详情共用 `CardDetailTheme.StatusBrushes / IconBrushes`：状态 run 显示状态图标和中文名，显式 `{icon:EffectIcon}` 才显示效果图标。详情 Widget 不应直接用 `GetWorld()` 判断创建路径，避免无 World 的离屏场景污染日志。

Battle first-person 详情与 Run first-person 详情都由 `FWacomFirstPersonCardDetailMotionController` 消费 Slot 的只读 `PresentationScale`；`360 × 420` 内部制作布局保持不变，RenderTransform 整体缩放并与淡入出现倍率相乘。左右换边、边缘 clamp、Padding、换边滞后和位置重置按缩放后的视觉边界计算；分辨率变化只更新已显示面板的位置与倍率，不重新应用详情数据。背包详情面板不进入此合同。

被动详情正文的分类由“被动”区块标题承载，正文不再读取 `Passive.DisplayText`。被动触发说明由 `Passive.TriggerTemplates` 生成；`PassiveOutcomeTemplates` 描述 `OnCompanionCount` 这类规则专用结果，例如“使此牌回到手中”。可执行 `Passive.Effects` 继续复用效果模板；`OnCompanionCount` 和 `OnTwilightTriggered` 当前不展示 `Passive.Effects`，因为运行时不执行这些 Effects。没有 outcome / effects 的被动不会只显示一个悬空触发句。

Battle hand entries 由 `WacomBattleCardPresentation` 从 `FHandCardSnapshot` 构建：`ECardTargetMode` 只在 Battle adapter 内部映射成 first-person card layer 的 `InteractionIntent`，不再写入通用 entry。通用 first-person card layer 公共类型不提供 TargetMode 转换 helper；SlotWidget 只消费 `InteractionIntent` 来决定无目标拖拽或瞄准态，目标合法性和提交仍由 BattleHUD / BattleSession 处理。旧 `OnCardClickedByUser` 兼容入口不再参与该流程，也不再维护独立 TargetMode 分支。Hand-card target release、probe-only 状态和 full-hand card affordance 都以 `UBattleSession::ValidateTargetWithCard()` 的 reject reason 为准，first-person hand bridge 不再直接读取卡牌 `TargetMode` 来判断 hand-card 目标规则。

Battle hand 的场景化动画位置来自 BattleHUD UMG 几何，不来自 Battle 规则。`FWacomBattleHUDFirstPersonHandBridge` 在提交 presentation frame 时采集可选 `DrawPileMotionAnchor / DiscardPileMotionAnchor / PlayTargetMotionAnchor`，Draw / Discard 缺失专用锚点时分别回退现有 pile view；采集结果以逻辑 viewport 坐标写入同一 BattleHand source lifecycle frame。`Drawn`、`Discarded` 继续消费这些坐标。`Played` 的目标坐标也继续记录：配置有效 Card Use Surface Effect 时，源卡锁定提交位置并播放当前 Style（默认像素翻面收牌，旧菱形波可切回），坐标不驱动卡牌移动；配置失效时才按真实目标优先、PlayTarget 次之的旧合同执行空间离场。锚点只是只读表现事实，不改变牌堆、目标校验或出牌结算。

当玩家拖拽手牌并指向敌人部位、目标手牌，或无目标卡已经达到 `ArmedForCommit` 可释放状态时，first-person hand bridge 会把当前 `CardInstanceId + TargetHandle` 交给 `UBattleSession::BuildCardActionPreview()`。Battle 返回的 `FBattleCardActionPreview` 是只读规则 facts；它内嵌 `FBattleCardTargetPreview`，App 侧随后用 `WacomBattleCardPresentation::BuildActionPreviewPresentation()` 一次性生成 hand layer entries、源卡详情、可选目标手牌详情、玩家 projected state 和敌人部位 projected state。源卡卡面徽章和详情正文仍复用 target preview facts 显示目标修正后的主效果最终值；被强化 / 削弱的数值通过 RichText `ValueBuffed / ValueNerfed` 样式提示，不显示 `基础值 -> 预览值` 公式。若 preview 最终值不同于基础值，详情 value run 会隐藏 “相当于当前费用 / 相当于目标状态层数” 等来源短语，只保留最终值，避免来源文案和目标修正事实冲突。若目标是手牌，目标卡自己的卡面费用可以显示预测后的费用，但详情不生成 `[费] before -> after` 文本。preview 不提交命令、不修改 Battle state，也不在 UI 里重新计算伤害、护盾、状态、先机或敌人行动。

源卡 EffectBadge 的 Preview 保持相同稳定身份，不把预测结果写回权威 `Value`：Builder 按 `Damage / Poison / Heal / Shield` 等卡面语义聚合同类型效果并生成 `Badge.<Kind>`；无条件贡献组成权威值，条件贡献只在目标预览判定生效时加入 `PreviewValue`。因此基础伤害与条件额外伤害共用一枚 Damage Badge，中毒仍使用独立 Poison Badge。只有整个语义组都 skipped 时才降低 Badge 亮度，不绘制像素叉；同组仍有基础贡献时保持正常显示。取消、切换目标和语义未变化的高频刷新只恢复或更新局部数字，不生成正式反馈。正式 `EffectBadgeChange` 需要 `CardRuntimeCostChanged / CardStatusChanged` 等许可事件和同一卡 Pre/Post Snapshot 的可见 Badge 差异同时成立；目标马上离手、弃牌或消耗时不播放短命重写。当前没有动态 Effect 增删事务，因此 App 不从 Badge 数组出现/消失自行伪造 Added/Removed 语义。

Action Preview 的数值显示只在“有效释放语义”成立时启用：目标合法或无目标卡已经 armed、当前规则阶段是 `PlayerAction`、源卡通过完整 PlayCard preflight，且 `BuildCardActionPreview()` 生成 projected values。拖出手牌区但还没达到无目标提交距离、还没指向有效目标、目标无效或规则阶段不可提交时，不显示玩家侧收益或敌人净结果，只保留 hand / scene target 的轻量可作用对象提示。有效敌人部位目标上，`UPlayerStatusBar` 直接覆盖显示 projected HP / Shield / runtime statuses，`UWacomBattleEnemyPartEntryWidget` 直接覆盖显示 projected HP / Shield / Initiative / runtime statuses / destroyed；无目标卡的 projected player state 同样直接覆盖玩家状态条。所有预览都不显示箭头、公式、来源文案或 `+N/-N`。敌人部位若会因本次打牌立即行动，部位 UI 的先机显示为 `0`，代表“松手后会出手”，不显示行动后刷新出的下一意图先机。first-person 拖拽命中有效场景目标时，HUD 会把规则层返回的全部 projected enemy parts 应用到敌人聚合面板和场景部位 prediction badge；这意味着同一次出牌会触发多个部位行动时，非当前鼠标指向部位也会显示 projected 先机 / 行动风险。TargetSelect hover probe 只刷新目标预览和敌人面板，不打开场景部位 action preview badge，避免普通 hover 与正式拖拽释放预览混淆。

拖拽 release、cancel、离开目标、候选目标无效、snapshot version 变化、BattleEnd 或 UI state 退出时，bridge / runtime 会清理 preview entries 和 action preview，恢复基础 hand entries、当前详情、玩家状态条和敌人部位条目。Scene enemy hover / TargetSelect hover 也先构建同一份 `FWacomBattleActionPreviewPresentation` 再应用：场景目标反馈仍由 enemy presentation component 负责，卡面、详情、玩家状态条和敌人面板只消费该 presentation，不在 hover / drag 两条路径里重复拼。Preview semantic state 由 snapshot version、source id、目标身份和 preview facts hash 组成；同一 state 上的高频 hover / drag move 只允许更新指针反馈、敌人 hover 和详情位置，hand layer preview entries 与详情数据必须等 preview semantic state 变化后再重建。Active drag 期间，目标手牌 preview 的生命周期由 bridge 保存的 `ActiveDragView.CurrentTarget` / target preview state 决定，SlotWidget 重建或 hover/unhover 抖动不能作为清理 preview 的权威信号。

`FirstPersonCardDetailViewportZOrder / FirstPersonCardDetailAnchorBaseSize` 属于 `Wacom|Battle|First Person Card Layer|Authoring`。第一人称手牌交互开关由当前 runtime source owner 通过 `SetFirstPersonCardLayerInteractionEnabled()` / `IsFirstPersonCardLayerInteractionEnabled()` 控制；Anchor 上旧的 `bEnableBattleHandInteraction` / BattleHand 命名 getter 只作为旧资产和旧 Blueprint 兼容别名保留。

Battle 与 Run 的 first-person card 镜头响应共用 Anchor `16 Camera Look While UI` 制作参数。Hover 由 Layer 的归一化 pointer view 驱动；Inspect / Drag 由 `FWacomFirstPersonCardDragView.PointerNormalizedViewportPosition` 驱动。BattleHUD first-person hand bridge 把两者转交已激活的 `UWacomBattleCameraLookComponent`，Run PlayerController 把它们转交已激活且未 suspended 的 `UWacomRunPathTraversalComponent`。Hover 进入 Inspect 时先清 pointer override，再由 drag-view override 同帧接管；release、cancel、source clear / unbind 必须清除。不恢复旧共享 camera-look bridge。

Battle entry staging 期间，入场门控与 opening result 的所有权在 `FWacomBattleHUDResultApplicator`；first-person hand bridge 只执行 suppression 与 transition frame。Begin 关闭 `BattleHand` presentation gate、清空当前 visual slot，并让 hand sync 写入 0 entries；Attach 在 suppression 下保存 opening `CardsDrawn` 并刷新初始化 `PostSnapshot`，不会让左右手提前落位。Release 解除 suppression 后用保存的同一份 `PostSnapshot` 提交一次正式 Drawn frame；该 frame 暂不包含新出现的左右手 anchor，普通牌 Drawn 播放完成后 bridge 再提交完整 snapshot，并让左右手各播放一次 `HandAnchorEntered` follow-up。GameMode 不再直接改 suppression / input gate，也不在 Release 时从 Session 重建快照。若 Drawn frame 已提交但尚未消费又进入新 suppression，bridge 仍先把 frame 还原为 controller 待播事件，保证 opening transaction 不丢失、不重复。

Battle hand 抽牌表现由 `FWacomBattleHandPresentationController` 事务化交付。Controller 消费 Battle events 时，优先把 `CardsDrawn.CardInstanceIds` 中仍存在于下一帧 hand snapshot 的真实普通卡实例转成 `Drawn` transition hint，并为可见 hint 写入稳定 `SequenceIndex / SequenceCount`；只有旧式 Count-only 事件才回退到 baseline / next snapshot 新增普通卡推断，不把左右手 anchor 纳入抽牌预算。HUD 不计算入场曲线、延迟、弧线或来源位置；这些表现参数由 first-person card layer / Anchor `06 Transition Motion` 统一处理。Bridge 只负责把 controller 给出的 `FWacomFirstPersonCardLayerPresentationFrame` 写入 Anchor，或在没有 pending event 时执行普通 entries refresh。普通 refresh 不会替换 Anchor 里尚未消费的 presentation frame hints；只有新的显式 frame、source clear、suppression 或 battle end 会替换 / 清空 hints，避免状态刷新把抽牌入场表现吞掉。Anchor 还有 source-scoped presentation gate：gate 关闭时 entries 可以刷新，但 pending frame hints 不能被送进 Layer。Layer 收到 frame 后也不会在一次空 slots / unprojected slots 刷新中丢弃 Drawn：只有 gate 已打开、对应 slot 可投影并真正启动入场播放后才消费该 hint，镜头 staging 或 viewport 投影暂不可用时会延迟到后续 hand refresh。

`FWacomBattleDrawPileFeedbackController` 只管理 DrawPile 的临时显示数量与发牌反馈，不改变 Battle Snapshot。普通/开场抽牌从 `CardsDrawn` 建批，Presentation Plan 从 `FBattlePresentationDeckStep::DrawBatch` 建批；两条路径最后都等待 first-person card layer 发回真实 `Drawn` Enter Started。每张可见卡开始移动的同一边缘，控制器把 `DrawPileView` 减一并调用方向性 `PlaySendFeedback()`；最后一张可见卡把隐藏差额校准到权威批次终值。重复 Card ID / Event Sequence 不重放，plan 中断、BattleEnd、source clear 和 teardown 直接恢复权威数量。已有 Drawn 音效与 Started 通知共享播放边缘，因此不新增第二层牌堆声音。

同一 `Drawn` Enter 还驱动 Battle-only 的牌背翻面揭示。Slot 创建后在 stagger 延迟内已经显示牌背；真实 Started 边缘到来时，牌堆逐张减数、既有 Drawn 音效和 Draw Reveal 翻面在同一帧开始。Reveal 只读取现有 Enter 的归一化进度，不建立 Timer，也不延长交互阻塞或 presentation phase。缺失 Style/MI 时仅回退原 Drawn 飞行；`Gained / RunHandEntered / HandAnchorEntered` 不使用牌背。

`DeckReshuffle` 的两端反馈由 Card Glyph Transfer 的真实 progress 边缘驱动，不使用额外 Timer。`LaunchedCount` 增加时，Coordinator 用新发射牌印的平均 Bezier 初始切线驱动 `DiscardPileView.PlaySendFeedback()`，并从洗牌前弃牌数逐张递减；`ArrivedCount` 增加时，从洗牌前抽牌数逐张递增 `DrawPileView`，同时触发 `PlayReceiveFeedback()` 与 Slate Impact。最后一枚只增强既有接收脉冲/方印。低帧率批量跨边缘按数量增量聚合，重复 progress 不重复计数；ForceComplete、超时、BattleEnd、source clear 与 teardown 只恢复 Deck Step 的精确终值并清除两端 Transform。弃牌堆的 `Discard+Played` 复合文本在整个阶段保持不变，任一 PileView 缺失时另一端仍可独立工作。

First-person card layer 重新拥有语义 Transition Audio，并生成 `Gained` 专用 transition：音效只在对应 enter playback 跨过错峰延迟、真正开始播放时请求一次；普通 refresh/reflow 不播放。`CardsRetained` 通过 `Retained` feedback 建立 `Sealing → Held`，EndTurn plan 在抽牌与手牌 Anchor 入场后用 `RetainedRelease` 解除；Held 继续追随最新 slot 布局但不阻塞 plan，不使用旧 Overlay 发光。`Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded` 均保持显式表现语义；规则事件、日志和 Toast 行为不变。

同一 `Gained` Enter 还驱动正面像素结晶入手。Stagger 等待期完全隐藏；真实 Started 边缘到来时，既有 Gained 音效与结晶在同一帧开始，结晶只消费 Enter 的归一化进度，不建立第二套计时或实体运动。默认先由外缘稳定像素簇向内组装正面，再在完成段播放一次由卡牌稀有度决定颜色的硬边峰值；该颜色不大面积染色卡面。Simplified Motion 只交叉显现正面并保留静态弱边缘。缺失 Style/MI 时安全回退原 Gained 飞行。

`WacomBattle` 在 `CardPlayed` 后发布 `CardPlayDestinationResolved`，只提供最终 `ECardLocation` 规则事实；`WacomApp` 先索引同批事件再映射表现语义。最终进入 Exhaust 或独立 `CardExhausted` 事件生成 `Exhausted`，其他成功离开手牌的使用生成 `Played`；同一卡的相关事件会去重。成功使用且最终仍在 Hand 的卡（例如 AfterPlayed 自身腾挪）不会伪造离场，而是在存在已接受的 play commit 时生成可分段的 `CardUseReformOut / CardUseReformIn`：Outbound 默认在提交位置约 `0.22s` 翻到侧边，随后停在 `HeldHidden`，不 Tick、也不继续占用阶段完成条件；命令全部可见结果结束后，Inbound 才在最新手牌槽反向展开约 `0.18s` 并 `0.04s` 落定。Style 中旧 Hidden Hold 数值被计入结果期间的隐藏停留，不会在回手前额外重复等待。无左右锚点时最终槽位等于原位，仍完整播放翻出与翻回；使用音效只在 Outbound 开始请求一次。普通 `Played` 默认约 `0.28s` 翻面收起，`Exhausted` 继续使用 OrderedDither / PixelAsh；替换 Anchor Style 可恢复旧 DiamondWave。所有路径复用唯一 `Fake3DSurfaceRetainer`，不延迟 BattleSession 命令结算。

成功的 `PlayCardOnHandCard` 还会由 BattleHUD 读取提交前冻结的目标手牌 ID，并在同一 command presentation phase 给目标卡下发一次 `HandTargetImpact`；这不是新的战斗事件，也不改变目标判定。有效目标 hover 只播放弱像素刻印 Preview；命令成功后源卡效果立即开始，目标约 `0.07s` 后压印、在约 `0.11s` 开放结果离场，再回弹归位。Post Snapshot 决定目标留手、普通弃牌或 Exhaust：普通弃牌牌印与消耗 Surface 都等待该 Gate，缺失 Style/MI/Slot 时则立即走旧路径，不为表现延迟命令。默认 Style 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardHandTargetImpactStyle_PixelStamp`，Anchor 入口为 `15 Card Hand Target Impact`；世界目标命中反馈仍是后续独立切片。

Slot 释放结果必须在任何同步 HUD 回调前冻结。Layer 把解析后的目标事实同时写入 source Slot；Release 只做最后一次本地手势计算，不重复广播 DragUpdated，然后冻结 accepted / resolved-invalid / neutral、释放方向与稳定 Seed，再广播同步命令。只有冻结时存在有效 Target Handle 且反馈为 `Invalid / InvalidCardTarget` 的正式拖拽才进入 Deny；成功命令触发的同步 snapshot refresh、手势清理或交互禁用不能把有效目标误改成拒绝，空白、Probe、主动取消与右键取消也不能伪造 Deny。

## §8 Battle Shared Widgets

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。玩家和敌人 runtime 状态共用 `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget`：控件只消费 Snapshot / ViewData 中的 `Statuses / StatusStacks`，图标 Brush 由 WBP 变量配置，`Status.Shield` 仍由 HP / Shield UI 单独显示。状态图标控件提供 design-time preview 字段，方便在 UMG 视口调图标和层数；这些预览字段不进入运行时规则数据。Action Preview 激活时，玩家状态条和敌人部位条目读取 `WacomBattle` 产出的 projected state 覆盖当前显示；清理后恢复最近一次真实 Snapshot / ViewData。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / runtime 状态图标 |
| `UBattleCommandBarWidget / UWacomBattleCommandButtonWidget` | `Wacom|Battle|Command Bar|Authoring` | 被动显示 Wait / EndTurn 命令 view data，并把玩家意图广播回 BattleHUD |
| `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget` | `Wacom|Battle|Status Icons|Authoring` | 共享状态图标列表和单个状态图标；玩家状态条正式使用，敌人部位条目可选接入 |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用数量显示与收发反馈控件；牌堆类型由 WBP Image 图标表达，BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数`。可选 `PileFeedbackRoot` 统一承载图标+数字弹性 RenderTransform；缺失时兼容回退 `ReceiveFeedbackRoot`，再回退整个控件。Receive 与 Send 使用同一个组合 playback，不争用或覆盖 authored transform；Reduced Motion 仍更新数量但不播放变换。 |
| `UWacomProgressBar` | `Wacom|Common UI|Progress Bar` | 通用数值进度条显示控件 |

`UPlayerStatusBar` 在敌人行动 Impact 之后比较 Journal 的行动前后 `FPlayerSnapshot`：HP 下降播放 `DamagePulseAnimation`，护盾下降播放 `ShieldPulseAnimation`，两者同时下降则两个脉冲都播放而音效只选 Damage；仅状态变化只刷新状态图标。数值在 Impact 立即切换到权威值，不做 Tick 插值。Action Preview 仍只覆盖显示，不触发真实脉冲或音效；`NativeDestruct()` 停止瞬时动画。

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

`Source/WacomTests/Private/UI/BattleHUDCommandFlowSpec.cpp` 承载 BattleHUD 命令和目标选择的专题合同测试，覆盖 `FWacomBattleHUDCommandController` / `FWacomBattleHUDTargetingController` 对外表现。`Source/WacomTests/Private/UI/BattleCombatLogSpec.cpp` 承载 Combat Log builder、feed、HUD history 和 `FWacomBattleHUDCombatLogController` 的专题合同测试，统一前缀为 `Wacom.UI.Battle.CombatLog`。`Source/WacomTests/Private/UI/BattlePresentationStackSpec.cpp` 承载 `UBattlePresentationStackWidget` / `UBattlePresentationStackEntryWidget` 的纯展示合同测试。`Source/WacomTests/Private/UI/BattlePresentationQueueSpec.cpp` 承载 BattleHUD presentation queue / turn-boundary / pending barrier / BattleEnd 清理 / knockdown 延迟展示合同测试；`BattlePresentationTimerLifecycleSpec.cpp` 以 `Wacom.UI.Battle.PresentationTimerLifecycle` 专门覆盖 Queue 与 Stack timer 在 clear、HUD teardown 和原始 World 后续 tick 下不会回调已释放状态。`Source/WacomTests/Private/UI/BattleInteractionTargetSpec.cpp` 承载 battle scene enemy part world target bridge 和 scene click / probe 的 `Wacom.UI.Battle.InteractionTarget` 合同测试。`Source/WacomTests/Private/UI/BattleSceneEnemyTargetRegistrySpec.cpp` 承载 battle scene enemy target registry 专题合同测试，覆盖 Trigger scene enemy host slot -> HUD registry、current-host filtering、trigger authoring validation 和 registry-routed cue / hover / drag preview。`Source/WacomTests/Private/UI/BattleSceneEnemyHoverProbeSpec.cpp` 承载 battle scene enemy hover probe 专题合同测试，覆盖 hover visual priority、HUD hover probe bridge、TargetSelect hover prediction、无效目标清理、pending / drag / BattleEnd gate 和 hover debug summary。`Source/WacomTests/Private/UI/BattleHUDFirstPersonSpec.cpp` 承载 BattleHUD first-person hand / first-person card detail 专题合同测试，覆盖 hand bridge clear、Anchor interaction、first-person detail host、readability motion 和 inspect hover guard。`Source/WacomTests/Private/UI/BattleSceneEnemyActorSpec.cpp` 承载 battle scene enemy actor 专题合同测试，当前覆盖 hand snapshot swift prediction facts、prediction widget facade、PartActor facade / presentation setup、bridge runtime facts、world target handle、host visual / hit-only part、host visual routing、host identity / child actor scan、runtime facts / host counts、debug snake child actor authoring、part slot identity / duplicate validation、hover / drag prediction badge、prediction badge offset、badge layout stagger / debug summary、VisualLayers refresh / validation 和 blueprint default authoring 分支。`BattleWidgetSpec.cpp` 保留 fallback layout、event presentation 和其他跨专题旧测试并继续分批收口。

`Source/WacomTests/Private/UI/BattleEnemyPanelSpec.cpp` 承载通用 `Wacom.UI.Battle.EnemyPanel`；`BattleEnemySinglePartPanelSpec.cpp` 承载 `Wacom.UI.Battle.EnemyPanel.SinglePartCompact`，验证单/多部位类解析、正式 WBP 与 Intent Style 合同、数值、Preview、hover 和动画优先级。状态图标复用另由 `Wacom.UI.Battle.StatusIcons.EnemyPartUsesFormalStatusList` 覆盖。测试必须实例化正式 WBP，不再直接 `NewObject` abstract 原生父类或锁定旧 C++ fallback WidgetTree。

`Source/WacomTests/Private/UI/BattleSceneEnemyFeedbackPlaybackSpec.cpp` 单独覆盖互斥 Cue Playback、持久缩放优先级、清理和 ImpactAnchor 两条制作路径，避免继续扩大 `BattleSceneEnemyActorSpec.cpp`。`Source/WacomTests/Private/UI/BattleSceneEnemyDestroyedFeedbackSpec.cpp` 单独覆盖 Destroyed EffectKind、35% 原地换图、组件 / topology 稳定、Snapshot fallback、禁用粒子和新战斗恢复，统一前缀为 `Wacom.UI.Battle.BattleSceneEnemyDestroyedFeedback`。`Source/WacomTests/Private/UI/BattleSceneEnemyAuthoringSyncSpec.cpp` 单独覆盖 EnemyDefinition → Host PartActor 的派生、补缺、保留、幂等、无效定义和 surplus 合同，统一前缀为 `Wacom.UI.Battle.BattleSceneEnemyAuthoringSync`。`Source/WacomTests/Private/UI/BattleSceneEnemyHostAnimationSpec.cpp` 单独覆盖 Style 解析、原地 Flipbook 播放、真实完成 / watchdog barrier、串行动作、整体 Destroyed、BattleEnd retiring Host 和重入清理，统一前缀为 `Wacom.UI.Battle.BattleSceneEnemyHostAnimation`。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.Battle.CombatLog`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
### First-person Card Slot 内部所有权

BattleHUD 仍只生成表现 Frame / Hint，不感知 Slot 内部状态机。Anchor 将既有编辑器字段解析成单个 `FWacomFirstPersonCardSlotRuntimeConfig`，经 Owner 与 Layer 原子提交到 Slot。Slot 的 UMG adapter 把输入交给 Gesture Controller，把语义 Hint 交给 Presentation Controller；后者统一拥有语义 Playback、Surface/Cost/Badge Ready Generation 和单 Retainer Arbiter。该整理不改变 Battle/Run Hint、声音时机、动画数值或命令阶段，只消除任意 Visual 配置刷新清空全部动画的隐式耦合。
