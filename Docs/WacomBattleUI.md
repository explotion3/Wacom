---
type: presentation-contract
scope: wacom-battle-ui
status: active
updated: 2026-07-24
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
| 表现队列 | `FWacomBattleHUDPresentationCoordinator` | target cue、modal、card stack、turn-boundary barrier、EndTurn 与通用 command phase plan（普通弃牌 / Deck Step），并作为短时 Activity 的唯一语义表现时钟 |
| 表现计时 | `FWacomBattlePresentationTimerOwner` | App-private keyed timer ownership；统一撤销 Queue Advance、Plan Poll 和 Stack Entry Exit，隔离 World teardown |
| Combat Log / Activity | `FWacomBattleHUDCombatLogController + FWacomBattleCombatActivitySynchronizer` | 完整 history/回合分区立即持久化；短时 activity 事务暂存、按表现进度释放，并维护 Footer 持久状态 |
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

击倒选择的正式表现由 `UI.Widget.BattleKnockdownChoiceDialog` 注册的 `/Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceDialog` 承担，并 Push 到 `UI.Layer.Modal`。`FWacomKnockdownChoiceDialogPresentationBuilder` 在 App-private 层把既有 `FKnockdownChoiceView + FBattleSnapshot` 投影为 `FWacomKnockdownChoiceDialogViewData`：它只按 `PartInstanceId` 定位 Snapshot 的 `UEnemyPartDefinition`、调用 `ResolveKnockdownRewardCard()` 并校验 `CardId`，再通过 `UWacomCardPresentationBuilder` 构造通用完整卡面。定义缺失、无奖励或 ID 不一致只降级为奖励名称文本并记录诊断，不改变规则可用性。`UWacomKnockdownChoiceDialog` 不持有 Session；三个 `UWacomKnockdownChoiceOptionWidget` 只显示 ViewData 和广播 typed intent。Dialog 通过 native-only delegate 调用 BattleHUD 的 bool 提交入口，成功才关闭；失败恢复交互、播放 WBP 自有 `SubmissionRejectedAnimation`，再触发可选蓝图反馈钩子。已有 BlueprintCallable `OnKnockdownChoiceSelected` 仍是兼容 void wrapper。Back / Escape / Gamepad B 始终消费，默认焦点固定为 Aid → Destroy → Withdraw。

正式 Dialog / Option 的初始资产布局使用 Editor-only `WacomBuildKnockdownChoiceUI` Builder，可在 Editor 关闭时通过 `-Build` 重建已知合同、通过 `-InspectOnly` 做只读检查。Builder 只写 `/Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceDialog` 与 `WBP_BattleKnockdownChoiceOption`；运行时 C++ fallback 仅用于注册缺失或资产失效时保持命令入口，不作为另一套可扩展视觉。

Battle 创建完成后，`AWacomGameMode` 必须严格执行 `BeginBattleEntryPresentation → AttachInitializedBattleSession → ReleaseBattleEntryPresentation`。Begin 建立新的 presentation generation、关闭 `BattleInputReady` 并启用 Battle hand suppression；Attach 只接受本代第一次成功结果，按“缓存 opening transition → suppressed Snapshot 刷新 → Combat Log → presentation queue”应用同次 commit 的 `FBattleInitializationResult`，同时启动 first-person card Required Visual / Optional Audio 异步预热；Release 只表示相机 staging 已就绪。只有 Camera Ready 与 Prewarm Ready/TimedOut 同时满足，Applicator 才解除 suppression、开启输入，并使用 Attach 保存的 `PostSnapshot` 刷新，禁止重新从 Session 拉取。两道门并行，资源门上限为 `1.5s`；失败、重复、越序调用只记录诊断且不发布结果。普通 `SetInjectedBattleSession()` 只绑定已有 Session、取消未完成代次和预热 Generation，并重置版本基线，不推断或重播初始化事件。

命令结果同样只有一个应用 seam。`FWacomBattleHUDCommandController` 获取命令前 Snapshot、构造 `FWacomBattleCommandPresentationContext` 并调用 `ResolveCommand()`，随后无论成功失败都把 `FBattleResolution` 交给 `FWacomBattleHUDResultApplicator`。Applicator 只接受当前 Session 上 `VersionBefore == LastAppliedVersion`、`VersionAfter == VersionBefore + 1` 且 `PostSnapshot.Version == VersionAfter` 的成功结果；失败、重复、过期或跳版本结果不刷新 Snapshot、不写 Combat Log、不发布事件，也不改变 targeting。成功 PlayCard 先记录 commit facts，再清 targeting / 恢复 Idle。所有成功 `PlayCard` 都进入同一个命令级 phased plan：`CommandSourceOut → CommandPrimaryTarget → CommandOutcome → CommandSourceReturn`。规则和 PostSnapshot 已立即结算；各阶段只用 Pre/Post Snapshot、Battle Events 与 Presentation Journal 还原视觉中间态，并以真实 Playback 完成、手牌刻印峰值、Event Queue 完成或牌印迁移完成推进，不用推算 Timer 猜测动画边界。

`CommandSourceOut` 先完整播放离手源卡效果；仍留手的源卡只执行 `CardUseReformOut` 并保持隐藏。`CommandPrimaryTarget` 再播放手牌目标刻印或世界 `TargetConfirmed`。`CommandOutcome` 按事件 Sequence / Deck Step 处理费用重写、EffectBadge 局部重写、目标弃牌/消耗、Damage、普通弃牌迁移、洗牌与抽牌；命令内的费用与 Badge 重写会临时成为 blocking feedback，并行播放后等待较晚完成者，普通独立 Snapshot 刷新仍不阻塞。所有可见结果结束后，只有留手源卡进入 `CardUseReformIn`，在最新布局位置反向生成。阻塞式击倒选择被单独延后到源卡回手之后；`BattleEnd` 是直接终止边界，会清理隐藏卡、后续 Return 与 Presentation Stack，不伪造回生。EndTurn 继续在同一 coordinator 中保留弃牌、保留、敌方行动和左右手生成阶段。

命令阶段中的 first-person 临时材质还受渲染 Ready Gate 约束。Surface、费用数字和 Badge 数字各有独立 Generation；Hint 到达后先安装进度 0 MID/Brush，等待一次真实 Slate Paint，再由零 Delta 启动语义边缘。等待期间不会消费声音、目标 Peak、Enter Started、DrawPile 减数或 Departure Gate，且 `HasActivePresentationPlayback()` 仍为真，因此冷启动不会让 SourceOut / Target / Outcome 提前串阶段。费用与 Badge 可并行准备，Outcome 等待较晚完成者；`0.75s` 内仍未就绪时恢复权威值并走原 fallback，不改变已经立即结算的 Battle 状态。

键盘数字牌位快捷键进入 `AWacomPlayerController` 后，由其 App-private `FWacomFirstPersonCardInputRouter` 只提交 one-based hand index 和当前 pointer widget-space 坐标给 `UBattleHUD`；当前可见战斗手牌的 `CardInstanceId` 由 BattleHUD / first-person hand bridge 根据已同步的 hand snapshot 解析，PlayerController / Router 不直接读取 `UBattleSession` 或 `FBattleSnapshot`。快捷键 active drag 的右键取消、external pointer pump、左键 release、锁定检视 Tab/RB 与 Escape/B、以及 Wait/EndTurn 前取消都由 Router 统一仲裁；`UWacomGameViewportClient` 的 Slate Input Processor、`HandleRerouteInput()`、PlayerController `InputKey()` 和 Enhanced Input 回调只作为 adapter。右键仍只取消 `KeyboardShortcut` 来源，不影响普通 UMG capture 鼠标拖拽；锁定检视先于 Battle TargetSelect / Enemy Inspection 取消，World Shop input owner 又先于 Router。键盘 `IA_Wait` / `IA_EndTurn` 如果发现非锁定 active gesture，会先取消并消费，下一次按键才提交命令；`InspectLocked` 不属于 active gesture，也不会成为新的 Battle 命令 gate。

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
| Floating Combat Text | `UWacomBattleFloatingCombatTextLayerWidget + FWacomBattleHUDFloatingCombatTextController` | HUD 级精确伤害、护盾、DOT 与休眠暴击数字；世界 Niagara 只做装饰 |
| UE_LOG | readable log | 开发诊断 |

`UWacomBattleCombatLogBuilder` 从同一份 Command Context、Events 和 Pre/Post Snapshot 生成三种 UI-only 投影：`FWacomBattleCombatLogBlockView` 保留完整命令诊断历史；`FWacomBattleCombatLogDetailsBatchView` 保存详情页专用的根行动、结果和结果说明；`FWacomBattleCombatActivityBatchView` 只服务 BattleHUD 短时 Feed。三者不修改 `WacomBattle` 事件，也不从本地化文案反推规则语义。短时 Feed 采用明确白名单：根行动保留 `CardPlayed / EnemyPartActed` 以及初始化 `TurnStarted / WaitPerformed / KnockdownChoiceMade`；结果只保留正数 `DamageDealt`、有效 Tag 且非零层数的 `StatusApplied`、`ResistanceResolved`、`EnemyPartHpEmptied` 和 `EnemyKnockdown`。完美释放、被动、卡牌状态 / 费用 / 获得 / 弃置 / 消耗和击倒选择过程事件仍立即进入 DetailsHistory 与完整诊断历史；`InitiativeHit / EnemyInitiativeChanged` 只保留在原始 Battle 事件、完整诊断历史和开发日志，不再进入玩家可见 DetailsHistory。

`FWacomBattleHUDCombatLogController` 继续持有最多 80 个完整历史命令块。完整 History 和 DetailsHistory 在规则结算后立即追加，因此表现尚在播放时打开详情也能看到完整事实；Short Activity Batch 与 Floating Combat Text 分别暂存，但共用 HUD 生命周期内单调递增的 `PresentationTransactionId` 和同一个 App-private 语义表现时钟。Combat Activity Synchronizer 为每个事务内行动组分配稳定 `GroupIndex`，所有 begin / append emission 使用 `TransactionId + GroupIndex` 定位，完成信号按 Transaction 收束全部组。`FWacomBattleHUDPresentationCoordinator` 在 plan 开始释放玩家根行动，phase 开始按精确 Event Sequence 释放普通结果，敌人动画 start 释放对应 `EnemyPartActed` 根行动，真实 Impact 在应用 `SnapshotAfter` 和命中反馈后释放该 Journal 范围的日志与飘字，首个 `TurnStart*` phase 开始前更新 Footer。重复进度、重复 completion 和未知组不会重复播放；多敌人 Journal 范围互不串组。空计划立即 flush，正常完成和 timeout 兜底释放，Session / HUD 清理 discard。Widget 重建只恢复已经释放的最后根行动和回合数，不能从已结算 Snapshot 泄露未来结果。

战斗飘字使用单一 HUD-owned 全屏 UMG Layer 和池化 Entry；Entry 不 Tick、不获取焦点且始终 `HitTestInvisible`，BattleHUD 的中央 Tick 仅在存在活动播放时推进。玩家起点在释放语义边界时读取 `PlayerStatusBar` 几何并追加 Style offset，失败回退左上安全区；敌人起点按精确部位优先使用 typed `ImpactAnchor`、稳定 Presentation Bounds 中心、Part Transform，再投影并立即缓存为逻辑 viewport 坐标。生成后即使镜头继续晃动也不重新投影。屏幕数字是规则事实真相：全盾吸收只显示护盾 `-N`，部分吸收按同目标 `0.08s` 最小错峰显示护盾 `-N` 再显示 HP `-N`，护盾获得显示 `+N`，Periodic 使用 Status Catalog 图标，未来显式 `bCritical=true` 才显示金色“暴击 -N”。同目标按 lane 容量排队而不丢数字，不同部位可并行；Simplified Motion 取消位移和缩放但保留出现、可读保持和淡出。

`UWacomBattleFloatingCombatTextStyle` 只配置颜色、节奏、布局、并发和可选 Shield / Periodic / Critical Niagara；默认资产为 `/Game/Wacom/UI/Battle/FloatingText/DA_BattleFloatingCombatTextStyle_Default`。普通伤害继续复用现有敌人 Impact Niagara 和玩家 Vitals 反馈，不叠加第二次爆炸。三套新增 Niagara 只服务敌人世界部位装饰，资源缺失不影响 UMG 数字；玩家侧不生成世界 Niagara。完整日志会分别显示护盾吸收和 HP 损失，Periodic 使用状态名称，未来暴击使用暴击文案，`ShieldChanged` 显示实际 delta；短时 Combat Activity 白名单不增加 `ShieldChanged`。

`UBattleCombatLogFeedWidget` 是完全非阻塞的视觉播放器：玩家根行动显示玩家头像与卡名，敌人根行动显示 Intent 图标与名称，结果按事件顺序逐条进入；多目标结果不聚合。Playback 用 `TransactionId + GroupIndex` 维护组状态，并把所有已释放结果放入一条全局 FIFO；新语义根行动仍在动作开始时立即进入底部 Root lane，但不会强制补发或清除上一组积压，新组结果也不能超车。若上一根行动的文字仍未完成可读生命周期，新根行动到达时会把它转入上方历史流，而不是直接删除；这样 EndTurn 的连续敌人行动仍能逐个读到，最新行动则继续与场景动作同步。同一语义边界解锁的多条结果继续按 `0.16s` 向 `0.08s` 自适应短错峰，每次 Tick 最多准入一行，且等待上一轮位移完成。

可见流容量由有效 `ActivityViewportHeightPixels / RowHeightPixels`、`MinimumVisibleResultRows` 与底部 Root lane 共同计算；默认至少显示五条历史 / 结果行，再加一条当前根行动。正式 WBP 仍可保留旧 `140px` 制作基线，Native Feed 会在运行时把活动视口扩到至少 `220px`，并把 WBP 根 `CombatActivitySize` 与宿主 Canvas 高度一起扩到至少 `270px`；三层几何必须同步，否则 Row 会被旧 `190px` 根节点裁切。宿主更新还必须以正式根节点的 `420px` 宽度为下限，不能把父 WBP 构造期临时出现的 `1px` Slot 宽度写回，否则整个 Feed 会只剩竖线。该适配不要求重存资产，不改变最终位置或输入命中。容量满时只暂停结果准入，不丢弃 FIFO 数据。每行至少完整可读 `MinimumResultVisibleSeconds = 0.35s`，之后才允许最顶部流式行开始位置相关淡出；同一时刻最多一行处于退场，移除完成后再释放下一个槽位。Feed 必须按单调 `PlaybackId` 稳定绑定 Row Widget：顶部行退场只回收自己的实例，幸存行只更新 Canvas Y 与透明度，不能因为数组下标前移而重新绑定文字、图标或 Slate 状态。Reduced Motion 只取消位移，仍遵守相同的 FIFO、可读时间、容量背压和串行退场。队列收束时，最新根行动 Row 只淡出文字与底板，原图标进入稳定 `IconResident` 状态；已经完成阅读、只剩常驻图标的旧根行动在新根行动到达时才用默认 `0.10s` 透明度交接退场，尚未读完的根行动则进入历史流。伤害、状态和抵抗等结果行不会替换常驻图标。`EnqueueCombatActivityBatch` Blueprint 接口保留给初始化、独立 Widget 和兼容测试，并复用相同准入 / 退场门；正式同步运行路径使用 C++-only 的 begin / append / complete 增量入口，不增加 WBP 制作负担。

Activity 事务的失败策略按生命周期区分：空 plan 或 plan 拒绝立即 flush；正常完成、phase timeout 和 BattleEnd 正常收束 flush 未匹配行并输出开发诊断；Session 切换、HUD teardown 或明确清空 Battle 则 discard 事务并清空 Feed，防止跨战斗泄漏。Reduced Motion 与无 World 自动化仍发布相同语义进度，只缩短或跳过视觉等待。Footer 始终表示“表现已经推进到”的回合；EndTurn 在第一个可见 `TurnStart*` phase 开始前更新，没有可见 TurnStart phase 时在等价完成边界更新，不再等待 Feed 淡出或整个 Activity 播放耗尽。Battle 初始化仍立即建立详细日志的第 1 回合分区，但短时播报要等 Camera 与 Card Prewarm 两道 Entry Gate 都解除后才播放一次 UI-only 的“第 1 回合开始”；它不重复写入详细历史。战斗开始和开场抽牌等其它初始化事件仍不进入短时行。

常驻播报的 Root 和临时 Row 都不命中；Footer 的 `LastActionButton` 是固定覆盖根行动图标槽的 `38×38` 透明命中框。它不再承载第二枚图标，也不因 Row 播放切换为 `HitTestInvisible`：没有已释放根行动时保持布局但 disabled，根行动释放后立即可点击，包括文字进入、停留、淡出和图标替换期间。按钮调用 `UBattleHUD::RequestOpenCombatLogDetails()`：HUD 仍广播 `OnCombatLogDetailsRequestedNative`，同时由 `FWacomBattleSecondaryPanelCoordinator` 向 `UI.Layer.GameMenu` 异步 Push `UWacomBattleCombatLogDetailsScreen`。Screen 打开时复制 Controller 的回合分区历史，不访问 `UBattleSession`，也不轮询规则状态。

`FWacomBattleSecondaryPanelCoordinator` 同时是 BattleHUD 唯一的二级面板所有者。Combat Log 与牌堆详情开始 Push 前，Coordinator 根据当前 `CardLayerZOrder` 取得 PrimaryLayout viewport-depth lease，使 `UI.Layer.GameMenu` 根稳定高于 first-person hand 与敌情档案；关闭、Push 失败、BattleEnd、Session 切换和 HUD teardown 都幂等释放，不能把整个 CommonUI 永久留在高层。牌堆入口由通用 `UPileCountView` 只广播“请求详情”，Draw / Discard / Exhaust 的 Battle 语义由 HUD 映射；Coordinator 在 Push 前按需复制 `BuildPileInspectionSnapshot()`，Screen 此后不再访问 Session。牌堆页使用全屏安全区：左侧 `96px` 导航栏以短标签和数量切换 Draw、Discard（弃牌堆 / 本回合已使用两个子区）和 Exhaust，右侧虚拟化 `UTileView` 逐实例显示完整卡牌。弃牌主导航数量是弃牌堆与本回合已使用之和；标题和空状态始终描述当前实际分区。页面不暴露排序入口，固定按 `RuntimeCost → Name → InstanceId` 稳定排序；规则层仍对 Draw 区做无序投影，但 UI 不再显示额外提示文案。每个条目以 `178×252px CardHost / 198×274px Tile Entry` 为 WBP 制作参考尺寸，在 `ScaleToFit + DownOnly` 的 `CardScaleBox` 中承载正式 `296×420px WBP_CardView`。正式 BattleHUD Attach 时，Coordinator 从当前 `UWacomFirstPersonCardAnchorComponent` 复制只读 `FWacomFirstPersonCardRestingPresentationProfile`；Screen 用 `296×420 × HandCardRenderScale × FirstPersonTargetPhysicalScale` 得到与同一 Viewport/DPI 下“未悬停、未选中、未拖动”的手牌卡体完全相同的物理尺寸，再除以全局 DPI 得到牌堆 Host 逻辑尺寸。扇形旋转、悬停放大、选择、拖动与反馈缩放都不进入该指标。只有独立 Widget、自动化或缺少有效 Anchor 时才回退 `FWacomBattleCardPileThumbnailScalePolicy` 的 `1920×1080 / 0.90–1.15` 策略。CardHost、Tile Entry、留白、间距、流光扩展和命中几何始终共用最终局部倍率；分辨率或窗口尺寸变化只原地更新 ViewModel、TileView 和当前可见 Entry，不重建正式卡面、不重播表现，也不清除仍有效的 Pinned/Hover/Focus；超宽屏继续服从 first-person hand 的短边物理倍率。该路径不使用 RenderTransform 制造虚假布局尺寸。Screen 只复用一个固定 `360×420px WBP_CardDetailPanel`：Hover 或键盘 Focus 稳定约 `0.10s` 后在卡牌左右安全空间显示并跟随滚动；点击立即固定当前卡并显示详情，再次点击取消固定。详情来源优先级为 `Hover > Keyboard Focus > Pinned`，临时预览结束后立即返回固定卡；切换牌区、关闭、Session 清理、条目回收或固定卡滚出可见范围会清除无效固定状态。流光由条目活动时按需创建的局部 UI MID 绘制，非活动/回收条目不保留 MID。牌堆 Screen 成功 Attach 后仍把当前 `BattleHand` Layer 的表现可见性设为 `Collapsed`；Slot、ViewData、MID 与 Transition 均保留，关闭后原样恢复且不重播入场。打开期间复用现有 `All + NoCapture` 输入合同：镜头与后台非阻塞表现继续，卡牌、Wait、EndTurn 与目标提交被门控。

牌堆详情入口只在 Entry Gate 已完成、Battle 未结束、没有活动二级面板且 `IsBattlePresentationBusy()==false` 时启用。抽牌、弃牌、洗牌等 Presentation 活动期间点击不会排队，也不会提前暴露已经权威结算但尚未表现到达的结果；页面打开后消费一次副本，无需 Tick 轮询规则。

详细日志默认使用简略模式，只显示回合开始/结束分割线和根行动；“查看详情”展开每个根行动下的全部结果与说明。详情 ViewData 使用扁平 `Depth` 表达固定三级：`0` 根行动、`1` 结果、`2` 由单个事件自身证明的结果说明，不根据相邻事件猜测因果。敌方目标显示为 `[敌人·部位]`，单部位敌人简化为 `[敌人]`；玩家与卡牌使用 `[玩家] / [卡牌名]`，缺失定义时回退稳定 ID。击倒分支获得卡牌时同时保留来源部位和所得卡名，显示为 `[敌人·部位] 获得卡牌「卡名」`；卡牌定义缺失时依次回退 Snapshot、实例 ID 和“未知卡牌”。抵抗结果在一级显示 `抵抗成功/失败`，二级说明显示 `卡牌单段 P >/≤ 敌方单段 E`。偏好只在当前战斗内记忆。Screen 使用 `All + NoCapture`，因此镜头和后台 Battle Presentation 继续运行；独立 `bSecondaryPanelOpen` gate 禁止卡牌、Wait、EndTurn、目标提交和世界点击。打开前会中性取消当前拖拽/目标选择。Backdrop、关闭按钮、Esc、右键和 Gamepad B 都只关闭一次，关闭后下一帧恢复游戏 Viewport focus。BattleEnd、Session 切换、Push 失败和 HUD teardown 必须释放 gate，并重置战斗内偏好。

正式 `WBP_BattleCombatLogFeed / WBP_BattleCombatActivityRow / WBP_BattleCombatLogDetailsEntry / WBP_BattleCombatLogDetailsScreen / WBP_BattleCombatLogTurnDivider`、默认 `DA_BattleCombatActivityStyle_Default` 和中性像素图标图集由 `WacomBuildCombatActivityUI` 确定性生成。详情 Entry 是 UMG-only、事件推送、无 Tick 的自适应 Row；它不复用短时 Feed 的固定高度 `WBP_BattleCombatActivityRow`。`BP_BattleHUD.CombatLogFeed` 必须嵌入正式 Feed WBP 生成类，才能继承 Style、Row Class 和 Footer 图标；Builder 会把已知的原生 Feed 定向替换为该 WBP，并保留其有效 Canvas Slot。Builder 首次把 Feed 迁到玩家状态栏下方 `(28,122)`、`420×190`；写入位置合同后保留有效的人工 Canvas Offset。Native Feed 会为五加一显示合同把运行时宿主高度扩到至少 `270px`，但不回写或重存 WBP。运行时不再把旧 ScrollBox/BlocksBox 临时适配为 Feed，资产失效只走原生 C++ fallback。`-InspectOnly` 必须完全只读，重复 `-Build` 必须无资产差异。

`UBattlePresentationStackWidget` 是只读小卡表现 backlog，不是规则栈或交互入口。它用完整 `UWacomCardView` 作为 mini card，显示已提交但表现边界尚未释放的卡牌。正式 Details 配置位于 `Wacom|Battle|Presentation Stack|Authoring`。

Presentation Stack 小卡使用出牌前 `FHandCardSnapshot` 构造卡牌 runtime presentation context，因此显示的是提交时的 `RuntimeCost`、可用状态和 RuntimeCost-based 徽章，不在表现期间回读后续手牌状态。带目标的出牌命令会在提交前捕获 `FBattleCardTargetPreview` 并放入 command context；小卡可使用这份 preview facts 显示提交时的目标修正后徽章，避免源卡离开手牌后再回查失败。

Wait / EndTurn 请求遇到表现栈未清空时会进入 pending turn-boundary；CommandBar 通过 `FWacomBattleCommandBarViewData` 显示 pending 文案并禁用按钮，coordinator 等 stack 和 queue 清空后再提交等待或结束回合。

EndTurn 命令成功后，BattleHUD 会消费 `FBattlePresentationJournal`。当 journal 能生成有效 phase plan 时，`FWacomBattleHUDPresentationCoordinator` 接管本次 EndTurn 表现，不再把整批事件直接压成一帧 hand hints，也不立即把 first-person hand 刷到最终抽牌态。v1 phase 顺序固定为：

1. `TurnEndDiscard`：用 discard checkpoint snapshot 把非保留普通手牌原地收束为牌印并飞向弃牌堆；每枚抵达只增加一次表现计数，并在同一 progress 回调触发像素 Impact 与 `DiscardPileView` 接收脉冲。连续抵达的脉冲由 `UPileCountView` 叠加，最后一枚稍强；phase 完成或强制清理后恢复权威 Snapshot 数量与 authored RenderTransform，ForceComplete 不补播回弹。
2. `TurnEndRetain`：只为 `CardsRetained.CardInstanceIds` 中仍在手牌的普通卡建立像素封存；左右手 Anchor 不参与。封存建立完成后进入非阻塞 Held，允许后续敌人行动和抽牌继续，但卡牌只保持低强度刻印与轻微抬升，不改变 authored hand ZOrder。
3. `EnemyAction`：复用现有 battle event presentation queue 播放敌人行动相关 cue / delay / battle end；阻塞式击倒请求不混入该阶段。
4. `TurnStartDraw`：用 draw checkpoint snapshot 播放新回合 `Drawn` 入场。
5. `TurnStartHandAnchorEnter`：如果 draw checkpoint 中出现了上一手牌 checkpoint 没有的左/右手 anchor，则在普通抽牌后提交完整 hand snapshot，并播放 `HandAnchorEntered` 生成入手。
6. `TurnStartRetainRelease`：向仍在最终手牌中的保留卡发送 `RetainedRelease`，等待刻印和额外 Retain Transform 缓出归零；缺少 draw checkpoint 时在本次 EndTurn 最后一个安全阶段执行。
7. `CommandBlockingDialog`：若本次 EndTurn 产生 `KnockdownChoiceRequested`，在所有已解析回合阶段之后单独 Push 击倒 Modal。命令管线会在 EndTurn resolver 完成抽牌 checkpoint 后才把 pending knockdown 提升为请求事件，因此 coordinator 必须从整份 `FBattleResolution.Events` 显式收集该事件，不得依赖“抽牌前敌方事件”序号区间。

手牌 phase 的完成条件由 first-person card layer 的 production playback 状态提供：仍有 active enter、exit outgoing、Retain Sealing/Releasing 或 Card Glyph Transfer 时保持 phase busy；Retain Held 明确不计入 busy，因此不会阻塞敌人行动与抽牌。普通弃牌和弃牌堆洗回使用传输种类加 Batch Sequence 去重并按 FIFO 播放，新批次不得强制完成前一批。timeout 不是直接跳过：触发时 coordinator 必须先 force-settle 当前 Anchor / Layer，清除 pending hints、Retain Held、Impact 和临时牌堆计数，再启动下一 phase，避免旧动画跨阶段重叠。没有 journal 或 journal 无有效 phase 时，非 EndTurn / fallback 路径继续使用原来的 loose event hints 与 event queue；loose `CardsRetained` 使用自动释放的短反馈，不会永久封存。

显式战斗奖励卡使用同一通用 resolved-command planner。`CardGainedResolved` checkpoint 先提交包含新卡的中间 Snapshot，并建立 `CommandCardGained` phase；该 phase 等待真实 `Gained` Enter 与 Gain Reveal 完成后，才处理 checkpoint 之后的 `CardDiscarded / HandLimitDiscarded`、洗牌或抽牌。因此手牌已满时严格表现为“新卡结晶入手 -> 对应卡普通弃牌迁移”，而不是直接把最终 Snapshot 中已经离手的卡跳过。中断、BattleEnd 或超时只恢复权威 Post Snapshot，不补播未开始的阶段。

## §4 Event Presentation Helper

`UWacomBattleEventPresentationBuilder`、`FBattleEventPresentationView` 和 `EWacomBattleEventVisualTone` 是 UI-only 单事件展示词汇。它们被完整 Combat Log 诊断行、结构化 Details Entry 与常驻活动结果行复用，用于生成玩家可读中文文案、tone 和 icon；新的 BattleHUD WBP 不应直接消费 raw `FBattleEvent`。

`ResistanceResolved` 的展示只翻译 Battle 已判定的事实，不在 App 重算伤害。完整诊断 history 继续使用 `抵抗成功：眩晕 +1（卡牌单段 P / 敌方单段 E）` 或 `抵抗失败（卡牌单段 P / 敌方单段 E）`；结构化 Details 把结论与 `P >/≤ E` 数值比较拆成深度 1/2 两行，允许自动换行。两种 history 都在命令结算后立即包含该结果；常驻 Activity 仍由 Presentation Coordinator 在 `CommandOutcome` progress 释放，不能在卡牌动作开始时提前剧透。

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

旧 Battle event log drawer / entry / toast 的 `UBattleEventLogPanel / UBattleEventLogEntryWidget / UEventToast` 已删除；正式日志只走 `CombatLogFeed + BattleCombatLogBlock`。旧 2D hand 的 `UHandPanel / UCardWidget / WBP_HandPanel / WBP_CardWidget` 已删除。旧敌方 2D fallback 的 `UEnemyInfoBar / UEnemyPartWidget` 也已删除；缺少 `SceneEnemyHost` 时不会再显示 2D 敌人条，只会缺少场景敌人 hover、Niagara target feedback、cue 与拖卡目标绑定。正式战斗手牌主线是 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §6 Scene Enemy UI

Scene Enemy UI 保持单向数据链：`FBattleSnapshot -> FWacomBattleHUDSceneEnemyTargetCoordinator -> FWacomBattleEnemyPanelViewData -> passive WBP`。`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 每个 Host 只承载一个敌人面板；条目按稳定 `EnemySlotId + PartSlotId` 与 `EnemyDefinition.Parts` 顺序复用。Widget 不持有 `BattleSession`、不读取 Snapshot、不使用 Tick，也不修改战斗状态。

正式 Enemy HUD 的核心面板使用四个稳定 WBP：`BP_WacomBattleEnemyPanelWidget`、`BP_WacomBattleEnemyPartEntryWidget`、`WBP_WacomBattleEnemyInspectionWidget` 和 `WBP_WacomBattleEnemyInspectionPartRowWidget`；Intent 公开说明另外复用 `/Game/Wacom/UI/Enemy/Intent/WBP_BattleIntentTooltip` 与 `WBP_BattleIntentEffectRow`。单部位与多部位共用唯一 Panel / Entry WidgetTree；单部位由运行时设置为 `268 × 92` 和 `Single` segment，多部位按 Definition 顺序以最小 `116 × 92` 等宽连接，Destroyed 段保留位置。每段显示琥珀 Initiative、白色 Intent、朱红 HP、按值显隐的电蓝 Shield 外框/徽章和最多三枚 Buff。App-private presentation state 只从真实事实生成 typed motion cue，Preview 只生成 projected 显示帧；Material Adapter 独占 MID、参数名与 authored Brush 恢复，没有永久 Widget Tick。每个 Panel 只订阅一次 Local Settings，再把 Simplified Motion / Flash policy 推给稳定 Entry。详情点击只在 Idle、无拖卡、无 TargetSelect、无表现结算时开放；点击上报完整 `FBattlePartSlotIdentity`，由 BattleHUD 内唯一 inspection coordinator 切换或关闭详情。BattleEnd、Host/Part 移除、source clear 与 HUD destruct 强制清理。

Enemy HUD V3 的正式节奏为：Intro `220ms`、Damage `90ms` 残影停留后在 `220ms` 收束、Shield Impact `180ms`、Shield Break `240ms`、Initiative Step `120ms`、Intent Change `180ms`、Context `120ms`、Destroyed `300ms`。Simplified Motion 停止位移、压缩和扫光，数值与状态事实直接更新；Flash Reduced 为正常亮度的 `35%`，Flash Off 只关装饰闪光，不抑制事实刷新。HP / Shield / Initiative 数字使用 Silkscreen，中文详情继续使用项目现有 UI 字体。

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

- `UWacomBattleEnemyPartComponent` 是纯 `USceneComponent` 身份节点；Component Transform 与规则身份仍是部位唯一真相，但它不再是 Primitive，也不保存或制作 BoxExtent。`InteractionVisualLayerId` 必须在直接 typed Sprite/Flipbook 子组件中精确解析一个正式交互层；该层 authored Idle Sprite 或 Idle Flipbook 第一帧的 3D ShrinkWrapped BodySetup 是 hover、点击与拖卡的稳定碰撞真相。
- 内容人员填写 `PartSlotId`；`PartId` 由 `EnemyDefinition.Parts` 派生。规则顺序、HUD 顺序和 registry 顺序都来自 Definition，不按组件名、创建顺序或 Actor 名推断。
- Sprite/Flipbook Layer 必须是 Part 的直接 typed 子组件。其真实 Paper2D Transform、资源、Tint、Material、Sort、播放配置就是视口制作数据，不再存在生成 VisualLayers、Host 整体视觉或 ChildActor 镜像。Flipbook Layer 保留 `UPaperFlipbookComponent` 的组件 Tick 与 Editor Tick 合同，因此 authored Idle 在 Blueprint 视口和 PIE 中都持续播放；运行时暂停、恢复或换片只使用 Paper2D 播放状态，不通过关闭组件 Tick 实现。
- ImpactAnchor 必须是 Part 的直接 typed 子组件；缺失时运行时回退到 Part 原点。视觉透明区域、排序和尺寸不改变目标身份。
- TrainingWarrior 是一个 Body Part；Snake 与 SlimeTrio 是三个 Part。系统不再区分 Simple Host 与 Multi-Part 模式，Host 本身不持有 Sprite、Flipbook 或 Animation Style。

Host Details 的“从 EnemyDefinition 同步部位”由 `WacomEditor` 独占写入：为缺失槽位创建 Part、默认 `Visual_Main` Flipbook Layer 和 ImpactAnchor，并把该 Main Layer 写入 `InteractionVisualLayerId`；已有部位只修正派生 `PartId`。同步保留 Component Transform、Layer、资源与 Anchor，不删除 surplus，不在 PIE/game world 写入，也不再读写手工命中尺寸；无变化不创建事务、不 dirty package。纯 Authoring Report 与 Validator 还必须只读检查 interaction ID 唯一解析、Idle 首帧 BodySetup、ShrinkWrapped 3D 几何、`12cm` thickness 以及所有 authored typed visual 保持 `NoCollision`；正式内容进入 transient fallback 仍是 authoring error。

Authoring Report 每次只审计一份真实制作拓扑：Blueprint CDO 从 SCS Component Template 读取制作数据，关卡与运行时 Host 实例只读取自身实例组件；不得把实例组件和其来源模板合并计数，否则合法部位会被误报为重复 `PartSlotId`。

### Runtime registry 与表现

`UWacomBattleEnemySceneRuntimeComponent` 在 Host 内集中管理 typed Part 的运行时身份、Snapshot facts、targetable、hover、drag target feedback、cue、Action、Destroyed、Niagara、声音与 watchdog。它只在组件注册、注销、销毁或显式 topology 通知时重建缓存；普通 Snapshot 不扫描层级、不改 Transform、不重置 Flipbook，也不重建 authored 组件。Action Preview 不在 SceneRuntime 保存副本，只进入 Host 的 Enemy Panel。

普通 Snapshot Sync 以 HUD 已收到的同一份 `FBattleSnapshot` 为输入：coordinator 一次建立 Enemy/Part/targetability 索引，Panel 按 `Snapshot.Version` 去重，SceneRuntime 只应用发生变化的 facts。Target Selection 不得为每个同步批次再向 Session 拉第二份 Snapshot。重复的 targetable、hover 和 drag preview 不改变 authored scale、不重启 Niagara 或描边；重复 Action Preview 不重写稳定 Enemy Entry。

Hover Probe 的射线频率由 `BattleSceneEnemyPartHoverProbeIntervalSeconds` 控制；click、hover 与拖卡 widget-position probe 共用 PlayerController 的二阶段查询。先用 `Visibility` Line Trace 取得真实场景遮挡深度，再用 `WacomBattleInteraction` Line Trace 接受严格 Sprite 轮廓；未命中 Part 时才沿同一射线和专用通道做默认 `8cm` Sphere Sweep，只接受当前 HUD registry 的 battle part，并拒绝遮挡物之后的候选。容错区重叠时固定按屏幕空间 ImpactPoint 距离、射线深度和稳定 Part identity 决胜，不依赖组件遍历顺序。逐帧 gate 只读取 HUD 缓存的最新 Snapshot phase，不调用 `UBattleSession::BuildSnapshot()`。同一 `WorldTargetId + Snapshot.Version + UIState + PendingCardId` 复用已生成的 Action Preview/目标表现。SceneRuntime 不再创建部位上方的 transient Prediction `UWidgetComponent`；Available / Valid / Invalid 继续由同一 Niagara target-preview component 表达，规则投影只由 Enemy Panel 的稳定 Entry 显示。

HUD coordinator 直接缓存 `UWacomBattleEnemyPartComponent` 与完整稳定 identity，不再缓存 Bridge/Presentation Actor 适配器。场景射线优先读取 `HitResult.Component` 上的 `IWacomInteractionTargetProvider`；Actor fallback 只服务非战斗世界目标。Encounter Node Anchor 的 typed scene binding 按规则槽顺序把 `EnemySlotId -> Host` 交给 HUD，Host runtime 再为 Part 解析 `EncounterId + EnemySlotId + PartSlotId + PartInstanceId`。

确认、伤害、目标预演与 Destroyed 使用 Part 上的反馈设置，并复用 Host 默认 `UWacomBattleEnemyPartImpactStyle` / `UWacomBattleEnemyPartTargetPreviewStyle`。SceneRuntime 从 interaction visual 的 authored Idle Sprite / Idle Flipbook 第一帧 `RenderBounds` 与该层世界 Transform 构造稳定 oriented Presentation Bounds；偏心 pivot、旋转和非等比缩放都按三个世界半轴投影到摄像机平面，不经过世界 AABB，也不读取当前动画帧或任何碰撞组件 Bounds。配置异常时只复用碰撞 fallback 的来源顺序（interaction visual authored bounds、直接 typed visual 并集、Part 原点安全 bounds），求值不依赖 transient fallback 是否已创建。Target Preview 的中心和宽高取该稳定视觉矩形；Impact VFX 仍在 typed ImpactAnchor 落点生成，缺 Anchor 回退 Part 原点，但直径取同一 Presentation Bounds。缺 Style/System/MI 时只跳过对应装饰，不阻塞 cue、破损换图或规则结算。Destroyed 在 marker 到点时原地切换真实 Sprite/Flipbook Layer，组件指针、稳定 Idle 碰撞源、Presentation Bounds 资源和 topology revision 不变。

敌人命中使用项目专用 `WacomBattleInteraction` trace channel（`ECC_GameTraceChannel1`）。正式 interaction visual 与 transient fallback 的 ObjectType 仍为 `WorldDynamic`，只 Block 该通道，Ignore `Visibility` 和其他通道且不产生 overlap。统一查询先用 `Visibility` Line Trace 取得真实场景遮挡深度，再用专用通道 Line Trace 取得严格精灵轮廓；严格命中失败后才用同一通道做 `8cm` Sphere Sweep。候选必须属于当前 HUD registry 且位于遮挡物之前，重叠时继续按屏幕距离、射线深度与稳定 Part identity 决胜。

正式碰撞不可用时，SceneRuntime 仅在 Part 已绑定 Snapshot、已登记 HUD、存活且未 Retire 时懒创建一个 App-private transient Box；它不进入 SCS、Authoring Report 或资产保存，provider 仍转发原 Part handle。Bounds 依次取唯一 interaction visual 的稳定 Idle 视觉 bounds、所有直接 typed visual 的稳定 authored bounds 并集，最后才使用 Part 原点 `55×45×55cm` half-extent；每轴 half-extent 最小 `6cm` 且不额外放宽。解绑、HUD 注销、Destroyed、BattleEnd 与 Retire 立即关闭，topology 移除或 EndPlay 销毁。

敌人目标选择的可用性由唯一 `UWacomBattleEnemySceneRuntimeComponent` 合成：旧式 `TargetSelect` 与 first-person `AimingTargetedCard` 都会提供当前源卡身份，已绑定且通过该卡规则校验的普通部位进入 `Available`，在各自稳定 Presentation Bounds 中心显示低亮中心像素图标；当前悬浮部位的 `ValidHover / InvalidHover` 优先覆盖 Available，离开具体部位后恢复 Available。每个部位独占自己的 TargetPreview Niagara Component；组件 transient 挂在 Part 下只负责生命周期，刷新时始终把世界位置写为稳定视觉中心加摄像机深度偏移。即使多个部位属于同一 Host，旧部位退出也不能停用新部位或其他 Available 标记；同一部位内部才复用该组件，并通过 `User.PreviewMode` 与 `User.AvailabilityIconSize` 互斥显示中心图标、有效框和无效断裂框。App-private outline controller 为 interaction visual 惰性创建一个无碰撞、无阴影、非 authored layer 的 padded Plane proxy：TargetSelect 合法未悬停为暖金 `1 source pixel / Alpha 0.55`，Idle 存活悬停或 TargetSelect 合法悬停为亮金 `2 source pixels / Alpha 0.95`，第二像素环使用较暗颜色与 Alpha，非法、Destroyed、解绑、BattleEnd 与 Retire 均关闭。代理直接挂在当前 Sprite/Flipbook visual 下，跟随其 transform、可见性和 sort priority；原视觉本身不复制材质、不改变 scale。Basic Plane 的 UV.V 沿本地 `+Y`，代理固定用 `Roll=-90°` 把它映射为 Paper2D local `+Z`，禁止改回会造成上下镜像的正 Roll。C++ 从当前 Sprite 的 baked XY/UV 与 Atlas rect 构造 padded canvas 和 local-to-atlas 仿射参数，材质将代理 UV counter-remap 回原源矩形，因此增加透明留白不会放大主体。正式材质真源是 `DShader/Material/World/M_WacomBattleEnemyPartInteractionOutline.dsm` 与 `DShader/Shared/WacomBattleEnemyPartInteractionOutline.dsh`，生成资产固定为 `/Game/DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline`；每个邻域采样先限制在 Sprite 自身 Atlas rect，并用源矩形 inside mask 排除代理留白与相邻图集内容；一像素环为 `Dilated1 - CenterMask`，第二像素环为 `Dilated2 - Dilated1`，主体内部 Alpha 恒为零。代理沿视线方向向主体后方偏移 `0.25cm`；不得用原 Sprite 整体扩缩、整块 silhouette、全屏 CustomDepth 或手工 MaterialExpression 图覆盖主体。缺 Outline Material 或无法解析稳定 Sprite 几何时安全关闭描边。Targetable 不缩放敌人视觉层，普通 HoverProbe 与 authored visual scale 保持独立；重复 Snapshot 或相同 targetable 状态不会重启 Playback。

`UWacomBattleEnemyPartAnimationStyle` 用精确 `TargetVisualLayerId`、Default Action 与 `IntentId -> Clip` 驱动同一 Part 的真实 Flipbook Layer；不根据名称或层顺序猜测。Clip 的 `ImpactNormalizedTime` 把 Enemy Action Journal 分成 `OnImpact` 与 `OnCompleted`：Impact 才推送行动后 Combat facts，完成才释放下一行动 barrier。共享 App-private playback 统一 weak timer、watchdog、serial 与 exactly-once；cancel 丢弃旧 Impact但完成 barrier。Style 可选 `EnemyDestroyedClip`，同一 Host 最多一个 Part Style 拥有整体死亡 Clip；TrainingWarrior Body 用它播放 Downed，Snake/SlimeTrio 当前只保留逐部位终态。

敌人当前意图的攻击属性完全来自 Snapshot：`bIsAttackIntent` 表示是否含有指向 Player 的正 Damage，`PeakAttackDamage` 是其最高单段值。敌情详情对攻击意图显示 `INIT n   ATK m`，非攻击意图只显示 `INIT n`；现有 WBP 内部 `ResistanceText` 绑定名只为避免资产绑定失效而保留，不再表达独立 Resistance 数值。

Intent Tooltip 只消费 `FIntentSnapshot.Effects` 传入的权威事实。App-private `FWacomBattleIntentPresentationBuilder` 仅负责中文、Status Catalog 图标/核心说明和布局；它只聚合相邻且效果、目标、Magnitude、Duration 完全相同的行，不能跨越其它效果合并。头顶与档案 Intent 图标使用标准 `ToolTipWidgetDelegate`，由 Slate 跟随鼠标和 Clamp；Tooltip 最多直接显示五行。敌情档案正文的 `StatusList` 只显示当前部位真实 Buff / Debuff，没有状态时保持空白，不把 Intent 效果混入状态区域。拖卡、TargetSelect、Action Preview、交互门禁关闭、部位破坏与 teardown 会立即清理 Tooltip；Action Preview 的抵抗比较图标不开放该入口。未知效果保留完整 GameplayTag、数值与未知目标回退，不静默隐藏。

BattleEnd Snapshot 立即注销 target/presentation registry 并清 hover、preview、panel；同批队列只保留弱 Host/Part 引用完成已排队 Action 或 terminal clip。真正的探索场景退役仍由 GameMode 的 weak Encounter scene binding callback 在胜利结算和返回镜头 barrier 完成后执行：清运行时表现、隐藏 Host 并关闭交互，Node Anchor 与 binding 保留。Withdraw、Defeat、Undetermined 或结算失败不退役。

### 正式内容口径

- `BP_EnemyHost_TrainingWarrior`：Body Part 的 `Visual_Main` 使用正式 Idle，`DA_EnemyPartAnimation_TrainingWarrior` 映射 Attack/Guard/Cleave，并把 Downed 配为 `EnemyDestroyedClip`。
- `BP_EnemyHost_Snake` 与 `BP_SnakeHost_Debug`：Head/Body/Tail typed Part，各自一个 `Snake.<Part>.Main` Flipbook Layer与单帧 Destroyed；当前没有 Action Style。正式 Host 使用通用 Snake 部位位置，Debug Host 使用 `L_Exploration` 制作布局 Head `(-154,-6,46)`、Body `(0,0,70)`、Tail `(118,16,72)`；两套位置分别由 Snake Builder 固化，避免重建覆盖关卡调试布局。
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

`UWacomCardPresentationBuilder` 现有无 Face Context 入口和现有 `RuntimeContext` 重载永久默认 `EWacomCardFaceContext::Battle`，确保 Backpack / Pile / Shop 等旧调用行为不变。显式 `BuildCardViewDataForFace / BuildCardDetailViewDataForFace` 及 C++ 的 `FaceContext + RuntimeContext` 路径可以投影同一 Definition 的 RunFace：Run 卡面名称 / 插画 / 深度图优先使用 Run override 并回退共享字段，`TypeText=探索`，只展示 RunFace 描述，隐藏 Battle Cost、Physique、Durability、Value、EffectBadge 与战斗 preview；Run runtime context 只读取 `bIsPlayable`。Battle hand entry 的默认 ViewData 始终保留实时费用、可用性和目标 preview，启用 RunFace 时附带静态 Run alternate；Run default hand 反向以 Run 为默认、静态 Battle 为 alternate，旧卡回退 Battle。Run menu lease 只采用 RunFace 优先显示，不开放锁定翻面。

双面卡长按进入既有 `Inspecting` 后，按住期间移出 scrub 区仍可升级为正式拖拽；在检视区内松开才进入追加枚举值 `InspectLocked`。锁定态只允许单击卡体、`Tab` 或手柄 `RB/R1` 翻面；点击卡外、`Escape` 或手柄 `B` 关闭，外部点击的配对 release 被一次性消费，不能穿透到另一张卡或世界交互。翻面只修改卡槽内 CardView 的局部 X scale / opacity：Full Motion 约 `0.22s`，压缩到 `0.06` 并在中点替换 ViewData；Simplified Motion 约 `0.10s` 淡出换面。播放输入有单次 gate，不排队；同一卡牌的 hover、投影和布局刷新只更新检视基准，不关闭或送回手牌；source 切换、重新入场、正式拖拽、Battle/Run 交接、菜单接管、卡牌消失和 teardown 才恢复环境默认面与局部 Transform。详情按当前面按需重建并追加“单击卡牌 / Tab / RB 查看另一面”提示。该状态不生成 Battle command、Run drop 或规则事务，WBP 与 `UWacomCardDetailPanel` 仍只消费 ViewData。

First-person 详情面板的 viewport 生命周期由 `FWacomFirstPersonCardDetailPanelHost` 统一处理：只有拥有真实 local player / LocalPlayer 的 context 才允许 `AddToViewport`，HUD-only 自动化或离屏预热只构建面板对象并验证 ViewData / motion state。详情面板内部的 section 动态创建收口在 App-private `WacomCardDetailWidgetFactory`；section 内正文由 `UWacomCardDetailRichTextBlock` 渲染 RichText，并自动注册详情 inline icon decorator。战斗详情与背包详情共用 `CardDetailTheme.StatusBrushes / IconBrushes`：它仍独占 RichText inline icon 的排版主题；状态中文名统一读取 `UWacomBattleStatusPresentationCatalog`，不再由 `UWacomCardExplanationLexicon.TagDisplayNames` 重复制作。显式 `{icon:EffectIcon}` 才显示效果图标。详情 Widget 不应直接用 `GetWorld()` 判断创建路径，避免无 World 的离屏场景污染日志。

Battle first-person 详情与 Run first-person 详情都由 `FWacomFirstPersonCardDetailMotionController` 消费 Slot 的只读 `PresentationScale`；`360 × 420` 内部制作布局保持不变，RenderTransform 整体缩放并与淡入出现倍率相乘。左右换边、边缘 clamp、Padding、换边滞后和位置重置按缩放后的视觉边界计算；分辨率变化只更新已显示面板的位置与倍率，不重新应用详情数据。背包详情面板不进入此合同。

被动详情正文的分类由“被动”区块标题承载，正文不再读取 `Passive.DisplayText`。被动触发说明由 `Passive.TriggerTemplates` 生成；`PassiveOutcomeTemplates` 描述 `OnCompanionCount` 这类规则专用结果，例如“使此牌回到手中”。可执行 `Passive.Effects` 继续复用效果模板；`OnCompanionCount` 和 `OnTwilightTriggered` 当前不展示 `Passive.Effects`，因为运行时不执行这些 Effects。没有 outcome / effects 的被动不会只显示一个悬空触发句。

Battle hand entries 由 `WacomBattleCardPresentation` 从 `FHandCardSnapshot` 构建：`ECardTargetMode` 只在 Battle adapter 内部映射成 first-person card layer 的 `InteractionIntent`，不再写入通用 entry。通用 first-person card layer 公共类型不提供 TargetMode 转换 helper；SlotWidget 只消费 `InteractionIntent` 来决定无目标拖拽或瞄准态，目标合法性和提交仍由 BattleHUD / BattleSession 处理。旧 `OnCardClickedByUser` 兼容入口不再参与该流程，也不再维护独立 TargetMode 分支。Hand-card target release、probe-only 状态和 full-hand card affordance 都以 `UBattleSession::ValidateTargetWithCard()` 的 reject reason 为准，first-person hand bridge 不再直接读取卡牌 `TargetMode` 来判断 hand-card 目标规则。

Battle hand 的场景化动画位置来自 BattleHUD UMG 几何，不来自 Battle 规则。`FWacomBattleHUDFirstPersonHandBridge` 在提交 presentation frame 时采集可选 `DrawPileMotionAnchor / DiscardPileMotionAnchor / PlayTargetMotionAnchor`，Draw / Discard 缺失专用锚点时分别回退现有 pile view；采集结果以逻辑 viewport 坐标写入同一 BattleHand source lifecycle frame。`Drawn`、`Discarded` 继续消费这些坐标。`Played` 的目标坐标也继续记录：配置有效 Card Use Surface Effect 时，源卡锁定提交位置并播放当前 Style（默认像素翻面收牌，旧菱形波可切回），坐标不驱动卡牌移动；配置失效时才按真实目标优先、PlayTarget 次之的旧合同执行空间离场。锚点只是只读表现事实，不改变牌堆、目标校验或出牌结算。

当玩家拖拽手牌并指向敌人部位、目标手牌，或无目标卡已经达到 `ArmedForCommit` 可释放状态时，first-person hand bridge 会把当前 `CardInstanceId + TargetHandle` 交给 `UBattleSession::BuildCardActionPreview()`。Battle 返回的 `FBattleCardActionPreview` 是只读规则 facts；它内嵌 `FBattleCardTargetPreview`，App 侧随后用 `WacomBattleCardPresentation::BuildActionPreviewPresentation()` 一次性生成 hand layer entries、源卡详情、可选目标手牌详情、玩家 projected state 和敌人部位 projected state。源卡卡面徽章和详情正文仍复用 target preview facts 显示目标修正后的主效果最终值；被强化 / 削弱的数值通过 RichText `ValueBuffed / ValueNerfed` 样式提示，不显示 `基础值 -> 预览值` 公式。若 preview 最终值不同于基础值，详情 value run 会隐藏 “相当于当前费用 / 相当于目标状态层数” 等来源短语，只保留最终值，避免来源文案和目标修正事实冲突。若目标是手牌，目标卡自己的卡面费用可以显示预测后的费用，但详情不生成 `[费] before -> after` 文本。preview 不提交命令、不修改 Battle state，也不在 UI 里重新计算伤害、护盾、状态、先机或敌人行动。

源卡 EffectBadge 的 Preview 保持相同稳定身份，不把预测结果写回权威 `Value`：Builder 按 `Damage / Poison / Heal / Shield` 等卡面语义聚合同类型效果并生成 `Badge.<Kind>`；无条件贡献组成权威值，条件贡献只在目标预览判定生效时加入 `PreviewValue`。因此基础伤害与条件额外伤害共用一枚 Damage Badge，中毒仍使用独立 Poison Badge。只有整个语义组都 skipped 时才降低 Badge 亮度，不绘制像素叉；同组仍有基础贡献时保持正常显示。取消、切换目标和语义未变化的高频刷新只恢复或更新局部数字，不生成正式反馈。正式 `EffectBadgeChange` 需要 `CardRuntimeCostChanged / CardStatusChanged` 等许可事件和同一卡 Pre/Post Snapshot 的可见 Badge 差异同时成立；目标马上离手、弃牌或消耗时不播放短命重写。当前没有动态 Effect 增删事务，因此 App 不从 Badge 数组出现/消失自行伪造 Added/Removed 语义。

Action Preview 的数值显示只在“有效释放语义”成立时启用：目标合法或无目标卡已经 armed、当前规则阶段是 `PlayerAction`、源卡通过完整 PlayCard preflight，且 `BuildCardActionPreview()` 生成 projected values。拖出手牌区但还没达到无目标提交距离、还没指向有效目标、目标无效或规则阶段不可提交时，不显示玩家侧收益或敌人净结果，只保留 hand / scene target 的轻量可作用对象提示。有效敌人部位目标上，`UPlayerStatusBar` 直接覆盖显示 projected HP / Shield / runtime statuses，`UWacomBattleEnemyPartEntryWidget` 直接覆盖显示 projected HP / Shield / Initiative / runtime statuses / destroyed；无目标卡的 projected player state 同样直接覆盖玩家状态条。敌人部位若会因本次打牌到达行动边界，部位 UI 的先机显示为 `0`；规则 facts 用 `bWillAct` 表示会真实执行意图，用 `bWillSkipActionDueToStun` 表示会消费眩晕并跳过，两者不能混为同一攻击风险。`FBattleCardResistancePreview` 按稳定部位身份携带玩家峰值、敌方峰值和是否新增眩晕；失败比较即使没有 Snapshot 数值差异也必须保留。

Enemy Entry 是敌方 Action Preview 的唯一入口。App-private Preview Frame 只翻译上述 Battle facts：完美释放显示金色 Surface；合法抵抗显示 `[卡牌伤害标记] P > [Intent 图标] E` 或 `P ≤ E`，成功 / 失败分别使用成功色和珊瑚红，并始终保留比较符；立即跳过时 Intent 变暗并显示斜线，Projected StatusList 同时显示眩晕层。抵抗比较临时替换普通 Initiative / Intent 槽，非攻击意图的完美释放只显示金色 Surface，迅捷卡不显示完美或抵抗。普通会行动 Preview 只给 Intent 暖红风险提示。SceneRuntime 不显示“完美释放、抵抗、眩晕 +1、跳过行动”等文字，也不创建 Action Preview Widget；它只保留 Niagara Available / Valid / Invalid。Reduced Motion 使用完全相同的静态事实，不添加循环动画。

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

BattleHUD 直接依赖的状态显示控件只刷新显示缓存，不提交命令、不修改规则状态。玩家和敌人 runtime 状态共用 `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget`：控件只消费 Snapshot / ViewData 中的 `Statuses / StatusStacks`，状态中文名、HUD 图标、排序与宿主规则统一来自项目级 `/Game/Wacom/UI/Battle/Status/DA_BattleStatusPresentationCatalog`。`Status.Shield` 在 Catalog 中只提供共享名称，仍由 HP / Shield UI 单独显示。状态图标控件只保留 design-time 的 Preview Tag 与层数；预览名称和图标同样解析 Catalog，不建立第二份制作源。Action Preview 激活时，玩家状态条和敌人部位条目读取 `WacomBattle` 产出的 projected state 覆盖当前显示；清理后恢复最近一次真实 Snapshot / ViewData。

状态检视使用 Slate 原生 Tooltip 路径：图标 hover 时惰性创建 `UWacomBattleStatusTooltipWidget`，由引擎负责跟随鼠标、延迟、边缘翻转与视口 Clamp，不建立 HUD 跟随层或 Tick。`ToolTipWidgetDelegate` 必须在 Slate 属性同步前建立，状态刷新不得通过空 `ToolTipText` 清理它。Tooltip 固定显示 Catalog 中的核心效果、触发时机、叠层/清除三行，并按 `Player / EnemyPart` 宿主选择对应模板；`{PoisonDamagePerStack}` 与 `{PlayerHealPoisonRemovalPercent}` 由 App-private formatter 注入 `WacomBattleStatusRuleConstants`，DataAsset 不复制规则数值。未知状态显示完整 Tag、fallback 图标与非空通用说明，排序位于正式状态之后。紧凑列表的 `+N` 由 `UWacomBattleStatusIconListWidget` 自己拥有，Tooltip 只列出稳定排序后被隐藏的状态。玩家状态、敌人 Entry 与敌情档案复用同一 Tooltip WBP。

Combat Activity 与结构化 Combat Log Details 的状态结果图标也读取同一 Catalog；`DA_BattleCombatActivityStyle_Default.TagIcons` 只保留非状态 Tag。详情状态图标使用现有 Slate 原生 Tooltip，但标题层数显示“本次 `+N/-N`”，明确表示历史事件变化而不是当前总层数；详情图标不响应点击，也不打开敌情档案。模式切换、Screen 重建与 teardown 会主动清理 Tooltip 委托和缓存。Combat Log、短时 Feed 与 Card Explanation 的状态名称均通过 Catalog Provider 解析。项目设置必须配置 `UWacomUIDeveloperSettings.BattleStatusPresentationCatalog`；加载失败时运行时使用 CDO 可读回退并报告诊断，但内容校验与 InspectOnly 仍失败。

状态图标只在检视门禁开启时参与命中。玩家拖卡、TargetSelect 或 Action Preview 会立即把状态列表切回 `HitTestInvisible`；敌情档案在拖卡开始时关闭；BattleEnd、Session 切换与 Destruct 清除缓存。敌人 Entry 的状态图标点击只复用现有 `FBattlePartSlotIdentity` 敌情档案意图，不产生战斗命令。普通 `InspectHitTarget` 与 `StatusList` 都是合法输入路由，WBP 必须让 StatusList 绘制/命中在全覆盖检视按钮之上；Action Preview 或交互 gate 关闭时两条路径同时禁用。

| 控件 | 分类 | 语义 |
|---|---|---|
| `UPlayerStatusBar` | `Wacom|Battle|Player Status|Authoring` | 显示玩家 HP / Shield / runtime 状态图标 |
| `UBattleCommandBarWidget / UWacomBattleCommandButtonWidget` | `Wacom|Battle|Command Bar|Authoring` | 被动显示 Wait / EndTurn 命令 view data，并把玩家意图广播回 BattleHUD |
| `UWacomBattleStatusIconListWidget / UWacomBattleStatusIconWidget / UWacomBattleStatusTooltipWidget` | `Wacom|Battle|Status Icons|Authoring` | 共享状态图标列表、单个图标与三行规则 Tooltip；玩家、敌人 Entry 和敌情档案统一复用 |
| `UPileCountView` | `Wacom|Common UI|Pile Count` | 通用数量显示、收发反馈与详情请求控件；牌堆类型由所有者/WBP 决定。BattleHUD 的弃牌堆格可显示 `弃牌堆数+本回合使用牌堆数`，并把通用详情请求映射到牌堆页。可选 `PileFeedbackRoot` 统一承载图标+数字弹性 RenderTransform；缺失时兼容回退 `ReceiveFeedbackRoot`，再回退整个控件。Receive 与 Send 使用同一个组合 playback，不争用或覆盖 authored transform；Reduced Motion 仍更新数量但不播放变换。 |
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

`BattleHUDCommandFlowSpec.cpp`、`BattleCombatLogSpec.cpp`、`BattlePresentationStackSpec.cpp` 与 `BattlePresentationQueueSpec.cpp` 分别覆盖命令、日志、Stack 和队列表现；`BattlePresentationTimerLifecycleSpec.cpp` 覆盖 teardown 后弱 timer 不回调释放状态。Scene Enemy 新架构集中在 `EnemySceneComponentAuthoringSpec.cpp`、`BattleEnemySceneComponentRuntimeSpec.cpp`、`BattleEnemySceneRuntimePerformanceSpec.cpp`、`BattleEnemyPresentationBoundsSpec.cpp` 与 `EnemySceneLegacyAuditSpec.cpp`：验证 Definition 同步、typed hierarchy、视口数据不被刷新覆盖、Snapshot no-op、偏心 pivot / 旋转 / 非等比缩放的稳定视觉投影、Target 中心与 ImpactAnchor 独立、Niagara target feedback 不创建 Prediction Widget、零 Legacy Package 引用，以及四个 Host / 两张地图可加载。其它 HUD/first-person 测试通过 `FWacomBattleHUDTestHarness` 创建真实 typed Part Component，不再复建 Actor/Bridge 测试夹具。

`BattleResistancePresentationSpec.cpp` 使用 `Wacom.UI.Battle.Resistance` 验证成功 / 失败日志严格消费 `ResistanceResolved.Amount / Count / bSuccess`，不从 Snapshot 或卡面重新计算比较值；规则与 Action Preview 的完整抵抗矩阵由 `Wacom.Battle.Resistance` 负责。

短时活动播报统一使用 `Wacom.UI.Battle.CombatActivity` 前缀：`BattleCombatActivityProjectionFilterSpec.cpp` 固定短时白名单与完整诊断 History / 玩家可见 Details 的差异合同，`BattleCombatActivityFlowControlSpec.cpp` 固定跨组 FIFO、容量背压、最短可读时间、单行准入 / 退场与 Reduced Motion，`BattleCombatActivityWidgetIdentitySpec.cpp` 固定顶部退场前后幸存 `PlaybackId` 继续使用同一 Row Widget，`BattleCombatActivitySpec.cpp` 继续验证敌人分组、多目标逐条结果、最新根行动图标常驻与替换、透明详情命中框和重建恢复。`BattleCombatLogDetailsSpec.cpp` 使用 `Wacom.UI.Battle.CombatLogDetails` 覆盖回合分区、简略/详细行、关闭输入、独立命令 gate 和正式 Builder 资产合同；`BattleCombatLogDetailsPresentationSpec.cpp` 固定结构化目标、三级抵抗、单部位简化、稳定 ID 回退与历史状态 Tooltip。`BattleCombatLogSpec.cpp` 继续验证完整文本历史与 Controller，不再要求常驻 Feed 镜像整份历史。

`Source/WacomTests/Private/UI/BattleEnemyPanelSpec.cpp` 承载 `Wacom.UI.Battle.EnemyPanel.VisualContract` 与稳定条目复用；`BattleEnemyPanelVitalsMotionSpec.cpp` 验证真实事实、typed cue、Material Frame、紧凑 Action Preview Frame、Reduced Motion 与清理生命周期；`BattleEnemySinglePartPanelSpec.cpp` 保留文件名但测试前缀已改为 `UnifiedHierarchy`，验证单/多部位共用同一正式类、唯一默认类和 `268 × 92` 单段合同；`BattleEnemySegmentedVitalsSpec.cpp` 验证多段角色、等宽顺序、Shield、Buff 与点击 gate。状态图标复用由 `Wacom.UI.Battle.StatusIcons` 覆盖，宿主说明、共享 Poison 常量、Tooltip 缓存、内部 Overflow 和敌情档案转发由小型 `Wacom.UI.Battle.StatusTooltip` spec 覆盖；`Wacom.UI.Battle.StatusPresentationCatalog` 单独固定规范 Tag / Alias、排序、模板参数、Provider fallback 与资产校验。需要检查 WBP 绑定或动画的测试实例化正式 WBP；纯 App-private Preview Frame 测试可使用原生 Entry 验证无反射语义。

Enemy Action / cue / Destroyed 生命周期由 `BattleEnemyActionImpactSpec.cpp`、`BattlePresentationQueueSpec.cpp` 与 `BattleEnemySceneComponentRuntimeSpec.cpp` 组合覆盖；内容合同由 `TrainingWarriorContentSpec.cpp`、`SnakeEnemyContentSpec.cpp`、`SlimeTrioEnemyContentSpec.cpp` 覆盖。测试通过 production automation view 读取 runtime debug，不在 `WacomApp/Public` 扩散 Blueprint-visible 测试 API。

推荐自动化前缀：

- `Wacom.UI.Battle`
- `Wacom.UI.Battle.CombatLog`
- `Wacom.UI.FirstPersonCardLayer` 覆盖 first-person card layer 专题行为
### First-person Card Slot 内部所有权

BattleHUD 仍只生成表现 Frame / Hint，不感知 Slot 内部状态机。Anchor 将既有编辑器字段解析成单个 `FWacomFirstPersonCardSlotRuntimeConfig`，经 Owner 与 Layer 原子提交到 Slot。Slot 的 UMG adapter 把输入交给 Gesture Controller，把语义 Hint 交给 Presentation Controller；后者统一拥有语义 Playback、Surface/Cost/Badge Ready Generation 和单 Retainer Arbiter。该整理不改变 Battle/Run Hint、声音时机、动画数值或命令阶段，只消除任意 Visual 配置刷新清空全部动画的隐式耦合。
