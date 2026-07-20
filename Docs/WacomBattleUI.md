---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-07-18
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
| 场景敌人 | `FWacomBattleHUDSceneEnemyTargetCoordinator` | 同步当前 Trigger Host registry 的 typed Part Component、UI 与 cue |
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier、EndTurn 与通用 command phase plan（普通弃牌 / Deck Step） |
| 表现计时 | `FWacomBattlePresentationTimerOwner` | App-private keyed timer ownership；统一撤销 Queue Advance、Plan Poll 和 Stack Entry Exit，隔离 World teardown |
| Combat Log / Activity | `FWacomBattleHUDCombatLogController` | 完整 history/回合分区、短时 activity batch 投影与 Footer 持久状态 |
| Battle Secondary Panel | `FWacomBattleSecondaryPanelCoordinator` | `UI.Layer.GameMenu` 异步 Push、命令门控、关闭与战斗内详情偏好 |
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
| Combat Activity | `UBattleCombatLogFeedWidget + FWacomBattleCombatActivityPlayback` | 固定裁切视口内的流式活动播报与“最后行动 + 当前回合” Footer |
| Combat Log History | `FWacomBattleHUDCombatLogController + UWacomBattleCombatLogBuilder` | 最多 80 个完整命令块，并维护按回合分区的只读行动组历史 |
| Combat Log Details | `UWacomBattleCombatLogDetailsScreen + FWacomBattleSecondaryPanelCoordinator` | 左侧 680px 二级面板，简略/详细切换和 Battle 命令门控 |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 同时生成两种 UI-only 投影。`FWacomBattleCombatLogBlockView` 是完整历史命令块；`FWacomBattleCombatActivityBatchView` 是常驻 HUD 的短时活动批次。两者消费同一 Command Context、Events 和 Pre/Post Snapshot，不修改 `WacomBattle` 事件，也不从本地化文案反推规则语义。

`FWacomBattleHUDCombatLogController` 继续持有最多 80 个完整历史命令块，不再把整份历史反复提交给常驻 Feed。`UBattleCombatLogFeedWidget` 现在是固定 `140px` 裁切视口、完全非阻塞的流式活动播报器：玩家根行动显示玩家头像与卡名，敌人根行动显示 Intent 图标与名称，结果按事件顺序逐条进入；多目标结果不聚合。根行动从 Footer 最后行动槽所在的底部语义位出现，结果从该位置向上流动，越接近顶部越快淡出，不再对第四行做数据硬裁剪。当前根行动在全部结果发出前保持在底部行动槽；大批结果将错峰从 `0.16s` 自适应压缩至 `0.08s`。队列收束时根行动的文字与底板淡出，图标原位交接给可点击的 `LastActionButton`，而不是另行生成一枚重复图标。Footer 的沙漏与“表现已经推进到”的回合数始终显示；EndTurn 的新回合只在敌人行动批次播放完后更新。Battle 初始化仍立即建立详细日志的第 1 回合分区，但短时播报要等 Camera 与 Card Prewarm 两道 Entry Gate 都解除后才播放一次 UI-only 的“第 1 回合开始”；它不重复写入详细历史。战斗开始和开场抽牌等其它初始化事件仍不进入短时行。

常驻播报的 Root 和临时 Row 都不命中；只有 Footer 的最后行动按钮可点击。按钮调用 `UBattleHUD::RequestOpenCombatLogDetails()`：HUD 仍广播 `OnCombatLogDetailsRequestedNative`，同时由 `FWacomBattleSecondaryPanelCoordinator` 向 `UI.Layer.GameMenu` 异步 Push `UWacomBattleCombatLogDetailsScreen`。Screen 打开时复制 Controller 的回合分区历史，不访问 `UBattleSession`，也不轮询规则状态。

详细日志默认使用简略模式，只显示回合开始/结束分割线和根行动；“查看详情”展开每个根行动下的全部结果行。偏好只在当前战斗内记忆。Screen 使用 `All + NoCapture`，因此镜头和后台 Battle Presentation 继续运行；独立 `bSecondaryPanelOpen` gate 禁止卡牌、Wait、EndTurn、目标提交和世界点击。打开前会中性取消当前拖拽/目标选择。Backdrop、关闭按钮、Esc、右键和 Gamepad B 都只关闭一次，关闭后下一帧恢复游戏 Viewport focus。BattleEnd、Session 切换、Push 失败和 HUD teardown 必须释放 gate，并重置战斗内偏好。

正式 `WBP_BattleCombatLogFeed / WBP_BattleCombatActivityRow / WBP_BattleCombatLogDetailsScreen / WBP_BattleCombatLogTurnDivider`、默认 `DA_BattleCombatActivityStyle_Default` 和中性像素图标图集由 `WacomBuildCombatActivityUI` 确定性生成。`BP_BattleHUD.CombatLogFeed` 必须嵌入正式 Feed WBP 生成类，才能继承 Style、Row Class 和 Footer 图标；Builder 会把已知的原生 Feed 定向替换为该 WBP，并保留其有效 Canvas Slot。Builder 首次把 Feed 迁到玩家状态栏下方 `(28,122)`、`420×190`；写入位置合同后保留有效的人工 Canvas Offset。运行时不再把旧 ScrollBox/BlocksBox 临时适配为 Feed，资产失效只走原生 C++ fallback。`-InspectOnly` 必须完全只读，重复 `-Build` 必须无资产差异。

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

`UWacomBattleEventPresentationBuilder`、`FBattleEventPresentationView` 和 `EWacomBattleEventVisualTone` 是 UI-only 单事件展示词汇。它们被完整 Combat Log detail line 与常驻活动结果行复用，用于生成玩家可读中文文案、tone 和 icon；新的 BattleHUD WBP 不应直接消费 raw `FBattleEvent`。

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

Scene Enemy UI 保持单向数据链：`FBattleSnapshot -> FWacomBattleHUDSceneEnemyTargetCoordinator -> FWacomBattleEnemyPanelViewData -> passive WBP`。`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 每个 Host 只承载一个敌人面板；条目按稳定 `EnemySlotId + PartSlotId` 与 `EnemyDefinition.Parts` 顺序复用。Widget 不持有 `BattleSession`、不读取 Snapshot、不使用 Tick，也不修改战斗状态。

正式面板资产继续使用 `/Game/Wacom/UI/Enemy` 下的单部位、多部位与详情 WBP。紧凑条中每个部位显示 Initiative、Intent、独立 HP 段、Shield 外框/徽章与 Buff；多部位等宽连接，Destroyed 段保留位置。Preview 只覆盖 projected facts，不触发真实 Damage、Shield 或 Destroyed pulse。详情点击只在 Idle、无拖卡、无 TargetSelect、无表现结算时开放；点击上报完整 `FBattlePartSlotIdentity`，由 BattleHUD 内唯一 inspection coordinator 切换或关闭详情。BattleEnd、Host/Part 移除、source clear 与 HUD destruct 强制清理。

### 唯一场景制作结构

所有敌人，无论单部位或多部位，都使用同一组件层级：

```text
AWacomBattleEnemyActor
├─ EnemySceneRuntime
├─ Part_*      [UWacomBattleEnemyPartComponent : UBoxComponent]
│  ├─ Visual_* [UWacomBattleEnemyPartFlipbookLayerComponent / SpriteLayerComponent]
│  └─ ImpactAnchor [UWacomBattleEnemyPartImpactAnchorComponent]
└─ EnemyPanel
```

- `UWacomBattleEnemyPartComponent` 的 Component Transform 是部位位置唯一真相，`BoxExtent` 是 hover、点击与拖卡 HitBounds 唯一真相。
- 内容人员填写 `PartSlotId`；`PartId` 由 `EnemyDefinition.Parts` 派生。规则顺序、HUD 顺序和 registry 顺序都来自 Definition，不按组件名、创建顺序或 Actor 名推断。
- Sprite/Flipbook Layer 必须是 Part 的直接 typed 子组件。其真实 Paper2D Transform、资源、Tint、Material、Sort、播放配置就是视口制作数据，不再存在生成 VisualLayers、Host 整体视觉或 ChildActor 镜像。Flipbook Layer 保留 `UPaperFlipbookComponent` 的组件 Tick 与 Editor Tick 合同，因此 authored Idle 在 Blueprint 视口和 PIE 中都持续播放；运行时暂停、恢复或换片只使用 Paper2D 播放状态，不通过关闭组件 Tick 实现。
- ImpactAnchor 必须是 Part 的直接 typed 子组件；缺失时运行时回退到 Part 原点。视觉透明区域、排序和尺寸不改变目标身份。
- TrainingWarrior 是一个 Body Part；Snake 与 SlimeTrio 是三个 Part。系统不再区分 Simple Host 与 Multi-Part 模式，Host 本身不持有 Sprite、Flipbook 或 Animation Style。

Host Details 的“从 EnemyDefinition 同步部位”由 `WacomEditor` 独占写入：为缺失槽位创建 Part、默认 `Visual_Main` Flipbook Layer 和 ImpactAnchor；已有部位只修正派生 `PartId`。同步保留 Component Transform、BoxExtent、Layer、资源与 Anchor，不删除 surplus，不在 PIE/game world 写入；无变化不创建事务、不 dirty package。纯 Authoring Report 与 Validator 只读检查缺失/重复/未知槽位、PartId mismatch、重复 LayerId、错误父子关系、多 Anchor、空视觉、无效 Animation Style 与 terminal clip 冲突。

### Runtime registry 与表现

`UWacomBattleEnemySceneRuntimeComponent` 在 Host 内集中管理 typed Part 的运行时身份、Snapshot facts、targetable、hover、preview、prediction、cue、Action、Destroyed、Niagara、声音、Widget 与 watchdog。它只在组件注册、注销、销毁或显式 topology 通知时重建缓存；普通 Snapshot 不扫描层级、不改 Transform、不重置 Flipbook，也不重建 authored 组件。

普通 Snapshot Sync 以 HUD 已收到的同一份 `FBattleSnapshot` 为输入：coordinator 一次建立 Enemy/Part/targetability 索引，Panel 按 `Snapshot.Version` 去重，SceneRuntime 只应用发生变化的 facts。Target Selection 不得为每个同步批次再向 Session 拉第二份 Snapshot。重复的 targetable、hover、drag preview 和 Action Preview 不重新缩放组件、不重启反馈，也不重复写 Prediction Widget。

Hover Probe 的射线频率由 `BattleSceneEnemyPartHoverProbeIntervalSeconds` 控制；逐帧 gate 只读取 HUD 缓存的最新 Snapshot phase，不调用 `UBattleSession::BuildSnapshot()`。同一 `WorldTargetId + Snapshot.Version + UIState + PendingCardId` 复用已生成的 Action Preview/目标表现。Part Prediction Widget 是 transient runtime component：隐藏状态零创建，首次 hover/preview 时惰性创建，之后显隐复用，Battle retirement、Host EndPlay 或 HUD/source teardown 时销毁；Widget 仍只消费 Prediction View，不读规则状态。

HUD coordinator 直接缓存 `UWacomBattleEnemyPartComponent` 与完整稳定 identity，不再缓存 Bridge/Presentation Actor 适配器。场景射线优先读取 `HitResult.Component` 上的 `IWacomInteractionTargetProvider`；Actor fallback 只服务非战斗世界目标。BattleTrigger 仍按 Encounter 顺序把 `EnemySlotId -> Host` 交给 HUD，Host runtime 再为 Part 解析 `EncounterId + EnemySlotId + PartSlotId + PartInstanceId`。

确认、伤害、目标预演与 Destroyed 使用 Part 上的反馈设置，并复用 Host 默认 `UWacomBattleEnemyPartImpactStyle` / `UWacomBattleEnemyPartTargetPreviewStyle`。粒子在 typed ImpactAnchor 生成；缺 Style/System/MI 时只跳过对应装饰，不阻塞 cue、破损换图或规则结算。Destroyed 在 marker 到点时原地切换真实 Sprite/Flipbook Layer，组件指针和 topology revision 不变。

敌人目标选择的可用性由唯一 `UWacomBattleEnemySceneRuntimeComponent` 合成：已绑定且 `bTargetable` 的普通部位进入 `Available`，在各自 ImpactAnchor 显示低亮中心像素图标；当前悬浮部位的 `ValidHover / InvalidHover` 优先覆盖 Available，离开具体部位后恢复 Available。Niagara 复用同一 TargetPreview Component，并通过 `User.PreviewMode` 与 `User.AvailabilityIconSize` 互斥显示中心图标、有效框和无效断裂框。Targetable 不再缩放敌人视觉层；普通 HoverProbe 的 authored Scale 保持独立。重复 Snapshot 或相同 targetable 状态不会重启 Playback。

`UWacomBattleEnemyPartAnimationStyle` 用精确 `TargetVisualLayerId`、Default Action 与 `IntentId -> Clip` 驱动同一 Part 的真实 Flipbook Layer；不根据名称或层顺序猜测。Clip 的 `ImpactNormalizedTime` 把 Enemy Action Journal 分成 `OnImpact` 与 `OnCompleted`：Impact 才推送行动后 Combat facts，完成才释放下一行动 barrier。共享 App-private playback 统一 weak timer、watchdog、serial 与 exactly-once；cancel 丢弃旧 Impact但完成 barrier。Style 可选 `EnemyDestroyedClip`，同一 Host 最多一个 Part Style 拥有整体死亡 Clip；TrainingWarrior Body 用它播放 Downed，Snake/SlimeTrio 当前只保留逐部位终态。

BattleEnd Snapshot 立即注销 target/presentation registry 并清 hover、preview、panel；同批队列只保留弱 Host/Part 引用完成已排队 Action 或 terminal clip。真正的探索场景退役仍由 BattleTrigger 在胜利结算和返回镜头 barrier 完成后执行：清运行时表现、隐藏 Host、关闭 Part collision，最后销毁 Trigger。Withdraw、Defeat、Undetermined 或结算失败不退役。

### 正式内容口径

- `BP_EnemyHost_TrainingWarrior`：Body Part 的 `Visual_Main` 使用正式 Idle，`DA_EnemyPartAnimation_TrainingWarrior` 映射 Attack/Guard/Cleave，并把 Downed 配为 `EnemyDestroyedClip`。
- `BP_EnemyHost_Snake` 与 `BP_SnakeHost_Debug`：Head/Body/Tail typed Part，各自一个 `Snake.<Part>.Main` Flipbook Layer与单帧 Destroyed；当前没有 Action Style。
- `BP_EnemyHost_SlimeTrio`：Left/Core/Right typed Part，各自一个 `SlimeTrio.<Part>.Main` Flipbook Layer与单帧 Destroyed；当前没有 Action Style。
- Snake/SlimeTrio 的 Placeholder 仍必须被发布审计 `-FailOnPlaceholder` 阻止出包。

Enemy Panel、Intent 图标、双侧详情、Action Preview、PlayerStatusBar Impact pulse 与 first-person hand 数据链不因场景组件重构而改变。
## §7 First-person Battle Hand

BattleHUD 战斗手牌运行时只使用 first-person card layer。`UBattleHUD` 不再公开 `BattleHandPresentationMode`，也不再绑定、创建、隐藏或恢复旧 2D hand。C++ fallback BattleHUD 只构建状态、CommandBar、牌堆、CombatLogFeed 和 PresentationStack，不再构建 legacy 2D hand 或敌方 2D fallback。

First-person hand 不在 slot widget 内提交规则。正式 target drag / aim、world target release、hand-card target release 和无目标 drag-out release 都经 BattleHUD bridge / command flow 进入 BattleSession。无目标 Battle 卡向上拖过 `NoTargetCardDragOutCommitDistancePixels` 后进入 `ArmedForCommit`，只有保持 armed 并 release 才提交；拖回阈值内会解除 armed。`FWacomBattleFirstPersonDropResolver` 是 App-private drop intent adapter，集中处理无目标 armed intent、目标 drag view、Battle world probe 和 `UBattleSession::ValidateTargetWithCard()` 的 UI 映射。完整合同见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

Battle / Run hand 共用 Card Depth。`FWacomFirstPersonCardDepthMotion` 在 Hover 时按卡面局部 pointer 计算倾斜，在 Drag 时按低通 pointer velocity 计算惯性倾斜，并与 rest / semantic transition 的压平以及 `ContactShadowLift` 一起做帧率无关平滑。`UWacomFirstPersonCardViewWidget` 将 `TiltX / TiltY / PerspectiveStrength / ContactShadowEnabled / ContactShadowLift` 写入现有 Retainer，同时把同一倾角以 `FWacomCardSurfacePerspectiveView` 交给内层 `UWacomCardView`：核心表面 MI 分层移动插画 / 卡框 / 稀有度饰条，实体出血 Badge / Durability 走 UMG RenderTransform。Badge 实体框和耐久底板还会复制自身 Brush 生成卡面内局部硬接触影；数字、文字和发光不投影，Reduced Motion 只保留静态接触。该路径只属于表现层，不改变规则、296×420 命中或外部 Surface Effect；局部附件影不替代 Retainer 内整卡实时 Alpha 阴影，外部 `CardShadowImage` 也不属于当前生产链。

First-person hand 的旧 `FeedbackOverlay / InteractionFeedbackImage / M_FirstPersonCard_FeedbackEdge` 已删除。Hover 只使用 lift、Fake-3D、卡面视差与接触阴影；Pressed 由 Motion Mixer 平滑合成默认 `0.985x`、向下 `2px`，并缩短接触阴影；权威成功 Commit 只保留约 `0.12s` 的运动脉冲。真实无效目标的源卡括角和正式拒绝裂痕当前暂时关闭，目标侧继续由敌人无效破框或目标手牌 focus 表达。正式释放拒绝仍冻结方向和 Seed，播放 `0.97x` 压缩、反方向回弹和阻尼归位，可选硬引用 Deny Sound 只请求一次。空白泛化 Invalid、尚未验证的 Probe、主动取消和右键取消不触发 Deny。Simplified Motion 不执行实体拒绝运动。该收口不改变 card-target identity、validation 或 release 命令路径。

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

`UPlayerStatusBar` 是左上角被动 Vitals HUD：权威 HP / Shield 在敌人行动 Impact 立即刷新，HP 下降后保留约 `0.08s` 珊瑚红延迟区，再于约 `0.32s` 收束；护盾下降以外框和右侧绝对数值播放约 `0.16s` 压缩/亮起/回弹，破盾完成后才 Hidden，布局空间始终保留。HP 低于 `25%` 静态转为暖红，不循环闪烁。若同次同时扣 HP 与 Shield，视觉均可播放而声音只选 Damage。Action Preview 不降低整栏透明度，而是在 `VitalsTrackImage` 内绘制 projected 增减区段，并更新中央 HP、护盾数字和 projected statuses；清理 Preview 不触发真实反馈。Simplified Motion 保留数值与区段，关闭延迟收束和护盾变换。

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

`BattleHUDCommandFlowSpec.cpp`、`BattleCombatLogSpec.cpp`、`BattlePresentationStackSpec.cpp` 与 `BattlePresentationQueueSpec.cpp` 分别覆盖命令、日志、Stack 和队列表现；`BattlePresentationTimerLifecycleSpec.cpp` 覆盖 teardown 后弱 timer 不回调释放状态。Scene Enemy 新架构集中在 `EnemySceneComponentAuthoringSpec.cpp`、`BattleEnemySceneComponentRuntimeSpec.cpp`、`BattleEnemySceneRuntimePerformanceSpec.cpp` 与 `EnemySceneLegacyAuditSpec.cpp`：验证 Definition 同步、typed hierarchy、视口数据不被刷新覆盖、Snapshot/Prediction no-op 与惰性复用、零 Legacy Package 引用，以及四个 Host / 两张地图可加载。其它 HUD/first-person 测试通过 `FWacomBattleHUDTestHarness` 创建真实 typed Part Component，不再复建 Actor/Bridge 测试夹具。

短时活动播报由 `BattleCombatActivitySpec.cpp` 负责，统一前缀为 `Wacom.UI.Battle.CombatActivity`；该文件验证活动投影、敌人分组、多目标逐条结果、流式 Row、根行动到 Footer 图标的原位交接和详情打开请求。`BattleCombatLogDetailsSpec.cpp` 使用 `Wacom.UI.Battle.CombatLogDetails` 覆盖回合分区、简略/详细行、关闭输入、独立命令 gate 和正式 Builder 资产合同。`BattleCombatLogSpec.cpp` 继续验证完整文本历史与 Controller，不再要求常驻 Feed 镜像整份历史。

`Source/WacomTests/Private/UI/BattleEnemyPanelSpec.cpp` 承载通用 `Wacom.UI.Battle.EnemyPanel`；`BattleEnemySinglePartPanelSpec.cpp` 承载 `Wacom.UI.Battle.EnemyPanel.SinglePartCompact`，验证单/多部位类解析、正式 WBP 与 Intent Style 合同、数值、Preview、hover 和动画优先级。状态图标复用另由 `Wacom.UI.Battle.StatusIcons.EnemyPartUsesFormalStatusList` 覆盖。测试必须实例化正式 WBP，不再直接 `NewObject` abstract 原生父类或锁定旧 C++ fallback WidgetTree。

Enemy Action / cue / Destroyed 生命周期由 `BattleEnemyActionImpactSpec.cpp`、`BattlePresentationQueueSpec.cpp` 与 `BattleEnemySceneComponentRuntimeSpec.cpp` 组合覆盖；内容合同由 `TrainingWarriorContentSpec.cpp`、`SnakeEnemyContentSpec.cpp`、`SlimeTrioEnemyContentSpec.cpp` 覆盖。测试通过 production automation view 读取 runtime debug，不在 `WacomApp/Public` 扩散 Blueprint-visible 测试 API。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.Battle.CombatLog`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
### First-person Card Slot 内部所有权

BattleHUD 仍只生成表现 Frame / Hint，不感知 Slot 内部状态机。Anchor 将既有编辑器字段解析成单个 `FWacomFirstPersonCardSlotRuntimeConfig`，经 Owner 与 Layer 原子提交到 Slot。Slot 的 UMG adapter 把输入交给 Gesture Controller，把语义 Hint 交给 Presentation Controller；后者统一拥有语义 Playback、Surface/Cost/Badge Ready Generation 和单 Retainer Arbiter。该整理不改变 Battle/Run Hint、声音时机、动画数值或命令阶段，只消除任意 Visual 配置刷新清空全部动画的隐式耦合。
