---
type: presentation-contract
scope: wacom-card-animation
status: draft
updated: 2026-07-06
tags:
  - wacom/ui
  - wacom/cards
  - wacom/animation
  - wacom/battle
  - wacom/first-person
---

# Wacom Card Animation 文档

> [!info] 本文职责
> 本文记录卡牌动画表现合同：卡牌动画语义从哪里来、由谁翻译、first-person card layer 能消费什么、哪些表现参数属于制作面，以及后续抽牌、保留、弃牌、出牌和交互动画如何扩展。

> [!warning] 边界
> 本文不记录战斗规则真相，不保存一次性 WBP 调参，也不把失败的 EndTurn hand timeline 方案重新引入项目。战斗规则见 [WacomBattle.md](./WacomBattle.md)，BattleHUD 协调见 [WacomBattleUI.md](./WacomBattleUI.md)，first-person card layer 结构见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §1 文档目标

卡牌动画不是规则状态。它是规则事件、snapshot 和 UI 交互状态在表现层的可中断、可压缩、可重同步投影。

本文服务三个目标：

- 统一卡牌动画语言，避免每次新增表现时重新发明 timeline、hint 或 widget 状态机。
- 保护模块边界：`WacomBattle` 只产出规则事实，`WacomApp` 生成表现计划，first-person card layer 消费表现语义。
- 给后续新版抽牌动画预留正式入口，不继续扩大上一轮失败的 EndTurn hand timeline 路径。

本文覆盖的表现包括：

| 类型 | 范围 |
|---|---|
| 入手 | 抽牌、战斗中获得卡牌、回收进入手牌 |
| 离手 | 打出、弃置、消耗、回合结束非保留手牌离开 |
| 回合边界 | 回合结束丢弃、保留、下回合开始抽牌的组合表现 |
| 布局 | 手牌重排、slot reflow、投影失败后的重同步 |
| 交互 | hover、inspect、drag、target affordance、commit / deny feedback |
| 降级 | 跳过动画、快进、不可见阶段延迟播放、snapshot 修正 |

## §2 当前事实

当前已落地的正式事实：

- `CardsDrawn.CardInstanceIds` 是真实入手普通卡实例列表。Battle hand presentation controller 优先用这些 ID 生成 `Drawn` transition hint。
- `CardsRetained` 是规则事件，只记录回合结束明确保留的普通手牌实例；当前会生成独立 retained feedback hint，但不对应 first-person `Retained` transition kind。
- `FBattlePresentationJournal` 是 C++ only 只读 checkpoint journal，记录 EndTurn 的 `TurnEndDiscardResolved`、`TurnEndRetainResolved`、`TurnStartDrawResolved`，以及显式获得卡插入后、手牌上限弃置前的 `CardGainedResolved`。
- first-person card layer 当前 transition kind 包含 `Default`、`Drawn`、`RunHandEntered`、`Gained`、`HandAnchorEntered`、`Played`、`Discarded`。`RunHandEntered` 是 Run/App-only hand/source 进入语义，v1 复用 `Drawn` 入场 profile；`HandAnchorEntered` 是 UI-only 左右手牌生成入手语义。两者都不属于 `CardsDrawn`。
- EndTurn journal 现在由 `WacomApp` presentation coordinator 翻译为阶段化 plan：`TurnEndDiscard -> TurnEndRetain -> EnemyAction -> TurnStartDraw -> TurnStartHandAnchorEnter -> TurnStartRetainRelease`。Retain Held 不阻塞敌人行动或抽牌，只有 Sealing / Releasing 计入完成条件；enemy phase 复用现有 battle event presentation queue。
- `FWacomBattleHandPresentationController` 在非 EndTurn phase plan 路径中，仍把 `CardsDrawn / CardGained / CardPlayed / HandLimitDiscarded / CardDiscarded / CardExhausted` 转为一帧 `entries + transition hints`，并把 `CardsRetained` 转为同帧 `feedback hints`。
- `UWacomFirstPersonCardAnchorComponent` 的 `05 Slot Motion`、`06 Transition Motion` 和 `07 Transition Audio` 是当前卡牌入场、离场、事件感知转场和入手音效的主要制作参数入口。
- WBP 可以负责卡面、overlay、材质和局部视觉反馈；核心手牌动画队列不应依赖 UMG Designer timeline。

当前明确不成立的事实：

- 没有 `Revealed` 或 `Retained` first-person transition kind。
- 没有正式 EndTurn hand timeline gate。
- 没有 draw diagnostics 文件作为正式合同。
- 没有让 `WacomBattle` 等待或驱动 UI 动画。
- 没有让 Widget 自己通过牌堆、hand limit 或前后 snapshot 猜测抽牌规则。

## §3 职责分层

| 层 | 职责 | 不负责 |
|---|---|---|
| `WacomBattle` | 结算规则，发 `FBattleEvent`，输出 `FBattleSnapshot` 和 `FBattlePresentationJournal` | 不计算屏幕位置、曲线、延迟、材质、动画时长 |
| BattleHUD / App flow | 消费事件和 journal，形成手牌表现帧或未来 presentation plan | 不改写规则结果，不让 UI 事件反向污染 Battle |
| `FWacomBattleHandPresentationController` | 当前 Battle hand 的 `entries + transition hints + feedback hints` 事务入口 | 不读取 Anchor 投影，不负责曲线和视觉参数 |
| `FWacomFirstPersonCardLayerPresentationFrame` | Battle / Run 共用的 `SourceId + entries + transition hints + feedback hints + CommitMode` C++ 表现帧 contract | 不决定规则事件语义，不暴露 Blueprint 制作面 |
| `UWacomFirstPersonCardAnchorComponent::ApplyRuntimeCardLayerSourceLifecycleFrame` | Battle / Run adapter 写入 Anchor runtime source、presentation gate、interaction 和 source 清理的统一提交入口 | 不生成领域事件，不读取 Run workspace 或 Battle snapshot |
| `UWacomFirstPersonCardAnchorComponent` | 制作参数 facade、runtime source、projection、presentation gate | 不提交 Battle / Run 命令，不持有规则真相 |
| `UWacomFirstPersonCardLayerWidget` | reconcile active / outgoing slot，应用 transition hint，管理 layer-level gesture | 不读取牌堆或战斗规则 |
| `UWacomFirstPersonCardLayerSlotWidget` | UMG 输入适配、卡牌几何/命中、Layer 委托和最终 RenderTransform | 不持有规则真相，不自行仲裁 Surface 优先级 |
| `FWacomFirstPersonCardGestureController` | Pressed / Inspecting / Dragging / Aiming / Armed / Cancelled 手势事实与阈值 | 不绘制、不播放语义动画、不提交命令 |
| `FWacomFirstPersonCardSlotPresentationController` | 单槽语义 Playback 生命周期、三路 Ready 协调、单 Retainer 所有权，以及统一 `NeedsTick / BlocksPresentation` Activity View | 不计算手牌布局、不读取 BattleSession |
| `FWacomFirstPersonCardMotionMixer` | 合成布局、状态、手势/Transition 和统一 Local Feedback View | 不读取 Slot 计时器或配置制作字段 |
| `FWacomCardMotionKernel` | App-private 帧率无关指数收敛、固定时长 ease-out、最短角度插值、到达判断 | 不持有 Widget、语义状态、规则或音效 |
| `FWacomBackpackWorkspaceMotionCoordinator` | 背包 Hover、拾起、滚轮当前卡、活动卡 DepthMotion 和收落的唯一局部运动所有者 | App-private；不接入 Battle Slot、transition hint 或 Run 写 API |
| `FWacomBackpackWorkspacePresentationController` | Hand Lens、选择冻结、Carry 编排、悬停展开/收起、指针合并和几何稳定状态 | App-private；Widget 只应用 UMG/Slate 结果，不读取 Run |
| `FWacomFirstPersonCardDetailMotionController` | Battle / Run 共用的详情面板预热、缓存、淡入淡出、scale、follow 和稳定换边 motion core | 不创建 panel，不读取规则状态，不决定详情数据来源 |
| `UWacomFirstPersonCardViewWidget` / WBP | 卡面内容、overlay、材质参数、局部反馈图层 | 不决定手牌顺序、目标合法性或动画队列 |

所有卡牌动画都应以 `CardInstanceId` 为稳定身份。Slot index 只表示当前布局位置，不能作为动画身份、事件身份或重同步判断依据。

### 共享运动内核与背包边界

Battle 的 `FWacomFirstPersonCardDepthMotion` 与 Backpack 的表现控制器共用 `FWacomCardMotionKernel`，确保 30/60/120 FPS 下指数收敛一致、旋转走最短路径，并允许从当前视觉姿态连续重定向。共享范围只包含数学与 DepthMotion 能力；Battle Slot 的出牌、瞄准、发牌、离场、Transition Hint 和音效播放仍由 first-person card layer 自己拥有，背包不得为了复用手感而接入这些状态机。

背包外层 Canvas 位置是 Scene 基础布局，牌堆与多卡携带基础姿态统一为水平零旋转；`WBP_WacomDeckCardWidget.CardMotionRoot` 只叠加局部 Translation / Angle。成功释放先捕获当前视觉姿态，目标 Scene 到达后把同一 Widget 放到目标 B 的 Canvas 布局，再把视觉差值作为 `CardMotionRoot` 局部偏移 ease-out 到零；中途 Reconcile 或目标更新只能从当前视觉姿态重定向，禁止重新使用来源 A。该瞬态层交接必须保留原 Widget 的 Retainer 捕获面与局部姿态；等价 Scene 绑定不得重复提交卡面数据、移动开关或 retained-rendering 模式，避免材质捕获重新建立时产生短暂空帧。UMG 的 `RemoveFromParent()` 可能在目标 Panel 接管前释放唯一的 `SObjectWidget` 强引用并连带重建嵌套 Retainer，因此 `Static / CarryCache / CarryActive / Settlement` 之间必须通过统一 App-private 入口迁移：在一次原子 Remove/Add 期间短暂保活原卡牌 Slate 子树，目标层接管后立即释放临时引用。它不是第二套 Slate 所有权，也不允许长期缓存 `TSharedRef<SWidget>`。背包的逻辑鼠标位置与视觉弹簧位置必须分离，规则判定始终使用无延迟指针。

正式展开牌堆使用 `HandLensStrip`，多卡携带继续使用独立 `FocusWindowStrip`，两条路径不共享制作参数。Hand Lens 保持 `296×420` 卡面和零旋转：空间足够时全部完整展示；不足时由整条走廊中的连续鼠标横向位置求出左压缩堆、动态完整区和右压缩堆。布局目标默认以 `24px` 完整间隔、`59px` 期望露出、`16px` 最小露出和 `178px` 提升重叠容忍求解，整个牌列从当前视觉姿态以约 `0.32s` Ease-Out 滑向新目标。布局焦点不等于浏览卡：详情、Fake3D、上抬与拾取统一由 Workspace 的权威悬浮身份决定；真实 PointerMove 按当前视觉卡身和 Canvas ZOrder 获取新卡，随后 Frame Scheduler 对同一缓存指针优先在当前卡的视觉卡身、稳定目标卡身和可见条带内保持身份。这样 Hover 上抬或临时 ZOrder 提升只改变表现，不会让压缩堆中暴露出的下层实体卡反向抢占静止鼠标；鼠标主动移入另一张可见卡仍会重新获取，鼠标也可跟随抬起后的卡身，标题拖柄保持立即清焦点的更高优先级。动画期间只有当前稳定目标不再覆盖缓存指针时才重新解析其它视觉卡身份，同一透镜段内不会重复布局、刷新 Snapshot 或 Scene。框选、Ctrl 选择和整堆移动冻结屏幕所见姿态与透镜。鼠标离开只取消活动焦点，最后 Hand Lens 三段布局保持到牌堆收起。Carry 以滚轮当前卡为 FocusWindow 焦点并锚定鼠标，正式基线只完整展示当前卡一张，其余卡向左右压缩；Hand Lens 迁移和展开牌堆调参不会影响 Carry。

展开牌堆提供左 Shift 临时布局锁：按住时保持当前 Hand Lens 目标并允许既有 Ease-Out 完成，不再依据后续 PointerMove 重算三段布局；视觉命中、详情、Fake3D 与拾取仍按实际卡身运行。松开时使用最新缓存指针立即恢复求解。该锁定不进入 Motion Coordinator、Workspace State Store 或 Style 资产，任何非 Idle 交互、牌堆切换、失焦与生命周期退出都会清除它。

背包牌列与运动制作参数由版本化的 `DA_BackpackWorkspaceStyle` 统一拥有，`WBP_BackpackScreen` 只通过普通资产引用消费，避免 Widget Blueprint 根详情内联 UObject。版本 2 将 `HandLens*` 参数限定为展开牌堆，将 `FocusWindow*` 参数限定为多卡携带；版本 4 只新增 Focus/Selected/Valid/Rejected 可访问性 Brush，不得改写原有运动与 Hand Lens 制作值。Builder 不得静默覆盖已有资产。普通左键按下可移动卡牌时必须在同一输入帧迁入 Carry 表现，不得把首次视觉启动推迟到下一次 PointerMove。

背包运动由 `FWacomBackpackWorkspaceRuntime` 内的 Presentation Controller、Motion Coordinator 与 Frame Scheduler 驱动，Workspace 只注册一个按需 Slate 帧 `ActiveTimer`：Presentation Controller 持有 Hand Lens、选择冻结、Carry 指针/牌列和悬停展开状态；Motion Coordinator 持有逐卡局部姿态与活动卡 DepthMotion；Frame Scheduler 合并带原因的定向刷新、稳定几何、连续运动、Settlement 和延迟 Retainer 补绘；Widget 只执行 Canvas/Retainer 应用。Carry 视觉锚点、PileMove 指针采样、携带目标悬停展开、局部姿态、Settlement、焦点退出延迟、牌堆展开/收起基础布局过渡、几何采样和卡面补绘不得各自注册常驻或并行 Timer。基础布局过渡每帧只更新活动卡的 Canvas 布局和局部姿态，禁止遍历静态卡、刷新 Snapshot 或触发 Scene Reconcile；牌堆收拢完成以实际过渡记录清空为准，不使用等时长回调。携带输入暂停只冻结鼠标/规则交互，不得中止已开始的视觉收拢；`Simplified Motion` 则在切换帧将所有基础布局过渡直接完成，并继续保留焦点、语义图标和提示。Retainer 补绘至少延迟一个 Slate 调度帧；条件未就绪时保留请求但不让 Timer 空转。

`SettlementLayer` 在收落完成前继续拥有原卡牌 Widget，并且必须加入 Scene Reconciler 的现有实例搜索集合。Reconciler 按完整 ViewKey 去重：瞬态层中的权威 Widget 优先于静态副本，从而保证“框选—携带—放下—再次框选”不会产生重复卡面。若玩家在收落完成前再次拿起同一卡牌，Carry 必须原子接管该 Widget：撤销旧 Settlement target、未消费的完成通知和旧局部运动后，再通过同一 Slate 保活入口从 Settlement 重挂到携带层；禁止让 Settlement 与 Carry 同时持有一张卡，否则按需帧 Timer 会被永远无法完成的旧目标持续唤醒。真正销毁、去重或删除卡牌时不使用保活迁移，仍正常释放其 Slate/Retainer 子树。

浏览焦点使用完整显示身份与实际 Widget 引用，允许投影卡、特殊区主卡和负重卡共享详情与动效，同时保留只读规则；Interaction Model 则只接收按物理 `InstanceId` 去重后的可移动实体身份，不能让同 ID 投影覆盖实体来源。焦点卡是唯一实时卡面；邻居只移动缓存后的外层姿态。多卡携带起手时默认最右释放卡保持平放；一旦滚轮发生过有效切换，之后滚轮选中的任意当前卡（含重新切回默认最右卡）都上抬并获得最高 ZOrder。`Simplified Motion` 下普通 Hover、展开焦点与携带当前卡都不叠加空间上抬、角度补偿或视觉弹簧，模式切换必须清除既有偏移。指针热路径始终只更新 `CarryRoot`，不能随卡数线性增加焦点求解或 Retainer 重绘。

## §4 目标管线

后续正式方向是把规则事实和表现调度拆成两步：

```text
Battle events / PresentationJournal / Snapshot
        ->
BattleHandPresentationPlan
        ->
CardLayerTransitionBatch
        ->
Anchor runtime source
        ->
Layer / Slot / CardView animation
```

当前已落地 EndTurn-only `BattlePresentationPlan` v1，负责按 journal checkpoint 串行播放回合边界阶段。更通用的 `BattleHandPresentationPlan` / `CardLayerTransitionBatch` 仍是后续目标，避免新版抽牌、消耗、关键词表现继续堆进 HUD timer、WBP timeline 或 slot widget 的临时分支。

目标结构含义：

| 概念 | 含义 |
|---|---|
| `BattleHandPresentationPlan` | App 层从事件、journal 和 snapshot 生成的手牌表现计划；可以包含多个阶段、barrier、可跳过策略和重同步规则 |
| `CardLayerTransitionBatch` | Card layer 可消费的一批卡牌 transition；只关心卡实例、语义、顺序、来源、去向和播放策略 |
| Anchor runtime source | 当前 first-person card layer 的 source ownership、presentation gate、entries 和 pending hints |
| Slot animation | 最终 widget 运动，包括 position、scale、rotation、opacity、z order、material feedback |

EndTurn phase plan 的当前合同：

| Phase | 来源 | Card layer 输入 | 完成条件 |
|---|---|---|---|
| `TurnEndDiscard` | `TurnEndDiscardResolved` checkpoint | checkpoint snapshot + `Discarded` transition hints | card layer 没有 active enter / exit / retained feedback，或 timeout 兜底 |
| `TurnEndRetain` | `TurnEndRetainResolved` checkpoint | checkpoint snapshot + 普通保留手牌 `Retained` feedback hints；左右手 anchor 不参与 | Sealing 完成并进入非阻塞 Held，或 timeout 兜底 |
| `EnemyAction` | retain / discard checkpoint 与 draw checkpoint 之间的 battle events | 现有 battle event presentation queue | event queue finished |
| `TurnStartDraw` | `TurnStartDrawResolved` checkpoint | 临时隐藏新出现左右手 anchor 的 checkpoint snapshot + `Drawn` transition hints | card layer 没有 active presentation playback，或 timeout 兜底 |
| `TurnStartHandAnchorEnter` | `TurnStartDrawResolved` checkpoint | 完整 checkpoint snapshot + 新出现左右手 anchor 的 `HandAnchorEntered` transition hints | 同上 |
| `TurnStartRetainRelease` | draw checkpoint 或 EndTurn 最后安全 snapshot | 仍在手牌中的保留卡 `RetainedRelease` feedback hints | Releasing 完成，或 timeout 兜底 |

## §5 动画语义词汇

| 术语 | 定义 |
|---|---|
| `Transition` | 单张卡的入场、离场或语义移动。当前由 `FWacomFirstPersonCardLayerTransitionHint` 表达 |
| `Feedback` | 单张卡的非阻塞短反馈，不改变入场 / 离场语义。当前由 `FWacomFirstPersonCardLayerFeedbackHint` 表达 |
| `TransitionBatch` | 同一规则或表现阶段产生的一组 transition。当前还没有独立类型 |
| `FWacomFirstPersonCardLayerPresentationFrame` | 一次原子提交给 card layer 的 `SourceId + entries + transition hints + feedback hints + CommitMode`；Battle / Run 共用该 contract，只在各自 adapter 中决定数据来源和提交语义 |
| `CommitMode` | Anchor 应如何提交 frame：`PresentationFrame` 替换 pending hints，`StateRefresh` 只刷新 entries 并保留 pending，`PreviewOverlay` 写预览 entries 且保留 pending，`Suppressed` 清空 entries 和 pending |
| `Origin` | 动画起点语义，例如 slot offset、hand anchor offset 或 viewport anchor |
| `DestinationSlot` | 由 snapshot / entries 解析出的目标手牌槽位 |
| `Reflow` | 手牌数量、顺序、zone 或投影变化导致的普通布局移动 |
| `OutgoingSlot` | 卡牌已离开 entries，但还需要保留短时离场 widget 的视觉状态 |
| `Settlement` | transition 完成后交回普通 layout / hover / pending / drag profile 的过程 |
| `Resync` | UI 与权威 snapshot 不一致时修正 slot，不重播旧事件动画 |

命名原则：

- 规则事件名描述事实，例如 `CardsDrawn`、`CardsRetained`、`CardDiscarded`。
- App plan 名描述表现阶段，例如 `TurnEndDiscard`、`RetainHold`、`TurnStartDraw`。
- Layer transition 名描述单卡运动语义，例如 `Drawn`、`Played`、`Discarded`。
- WBP 控件名只描述长期视觉结构，例如 `Fake3DSurfaceRetainer`、`SurfaceCaptureOverlay`、`CardContentSizeBox`；Pressed/Deny 一类短时交互语义不再映射成常驻 WBP Image。

## §6 当前 Hint 合同

`FWacomFirstPersonCardLayerTransitionHint` 是当前 first-person card layer 的最小 transition 输入：

| 字段 | 语义 |
|---|---|
| `CardInstanceId` | 目标卡实例 ID |
| `TransitionKind` | `Default / Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded` |
| `SequenceIndex` | 同批可见 transition 的稳定序号 |
| `SequenceCount` | 同批可见 transition 总数 |
| `bPlayCommitFeedback` | 打出离场时是否播放提交反馈 |
| `bHasPlayedExitTargetWidgetPosition` | 打出离场是否有目标 widget 位置 |
| `PlayedExitTargetWidgetPosition` | 打出离场目标位置，widget-space |

当前生成口径：

| 事件 | 当前 transition |
|---|---|
| `CardsDrawn` | 对仍存在于 next hand snapshot 的 `CardInstanceIds` 生成 `Drawn`，并按最终手牌槽位从左到右写入稳定 `SequenceIndex / SequenceCount` |
| Run 默认手牌 / provider menu lease 进入 | `UWacomRunFirstPersonCardSourceComponent` 在默认 `RunFirstPersonBattleDeck` source 初次显示、从菜单恢复、新增默认卡，或 RunEvent / 菜单通过 `OwnedCardsFilter` provider lease 筛出候选持有卡进入 first-person hand 时生成 `RunHandEntered`，并通过 `ApplyRuntimeCardLayerSourceLifecycleFrame` 统一提交；不进入 `CardsDrawn` |
| `CardGained` | 新出现在手牌中的对应卡生成 `Gained` |
| 左/右手牌生成入手 | App 层在普通抽牌后对新出现的左右手 anchor 生成 `HandAnchorEntered`；不进入 `CardsDrawn.CardInstanceIds` |
| `CardPlayed` | 从手牌移除的对应卡生成 `Played` |
| `HandLimitDiscarded` | 从手牌移除的对应卡生成 `Discarded` |
| `CardDiscarded` | 从手牌移除的对应卡生成 `Discarded` |
| `CardExhausted` | 从手牌移除的对应卡生成 `Discarded`，当前 first-person layer 不区分 exhaust 视觉 |

`FWacomFirstPersonCardLayerFeedbackHint` 是当前 first-person card layer 的非阻塞反馈输入。它和 transition hint 并列传递，不进入 `EWacomFirstPersonCardSlotTransitionKind`，也不改变 slot motion target、motion key、reset distance 或手牌命中区。

当前生成口径：

| 事件 | 当前 feedback |
|---|---|
| `CardsRetained` | 对仍存在于 next hand snapshot 的普通保留手牌生成 `Retained` feedback，并写入稳定 `SequenceIndex / SequenceCount` |

`CardsRetained` 不生成 first-person transition。正式 EndTurn 的 `Retained` feedback 建立像素封存并进入 Held；Held 不阻塞 Presentation Plan，但会跨过敌人行动与新回合抽牌继续跟随最新 slot 布局。`TurnStartRetainRelease` 再显式解除。若 slot 暂不可投影，hint 保留到下一次可见刷新；播放期间的普通 refresh 只能更新 slot 最新布局目标，不能取消封存。

EndTurn phase plan 的 `TurnEndRetain` 阶段只消费 `CardsRetained.CardInstanceIds` 中仍存在的普通卡。左右手 anchor 既不进入规则事件，也不生成封存反馈。

## §7 动画类型目录

### Draw / Enter Hand

抽牌动画的权威输入是 `CardsDrawn.CardInstanceIds`，不是前后 snapshot 猜测。放不下的牌不会进入该列表，也不应播放入手动画。左右手 anchor 不是普通抽牌对象：当它们在 battle entry 或回合开始从无到有进入手牌时，App 层用 `HandAnchorEntered` 表现生成入手。

同批可见 `Drawn` 入场的错峰顺序以目标 hand snapshot 中的普通手牌槽位为准：最左侧目标槽先进入，依次到最右侧。`CardsDrawn.CardInstanceIds` 只决定“哪些卡真实入手”，不决定最终 stagger 方向。

当前 layer 可消费 `Drawn` hint，并由 Anchor `06 Transition Motion` 控制来源模式、offset、viewport anchor、scale、angle、duration、stagger、arc lift、ease 和播放期间交互阻塞；`07 Transition Audio` 控制抽牌入手音效，声音在对应 slot 实际启动入场 playback 时触发。

四类 first-person Enter Sound 与卡面 Soft Sprite 在来源交付前由 Battle / Run 共用预热控制器异步驻留。播放边缘不再调用 `LoadSynchronous()`：Optional Sound 尚未驻留时跳过该次声音，但不延迟 Enter、伪造声音请求或阻塞输入；Required Sprite 正常预热后直接由 Soft Pointer `Get()` 命中，只有预热超时或非 first-person CardView 才保留同步完整性 fallback。Slot 还会提前建立 Cost / Badge 局部 MID，具体动画仍必须等待各自一次真实 Paint Generation，资源驻留不替代渲染 Ready Gate。

Run default source 和 RunEvent / 菜单的 provider-backed menu lease 使用 `RunHandEntered` hint，并通过 `ApplyRuntimeCardLayerSourceLifecycleFrame` 提交 `PresentationFrame`；不再保留 entries-only raw menu lease 作为无动画旁路。若后续某类菜单确实需要跳过入场，应在 frame commit / 动画策略上显式表达，而不是绕开 Run workspace provider。

`TurnStartDraw` 阶段会暂时不提交本次新出现的左右手 anchor entries，让普通抽牌先完成；随后 `TurnStartHandAnchorEnter` 提交完整 hand snapshot，并只为这些新出现的 anchor 播放 `HandAnchorEntered`。Battle entry reveal 也采用同样两段式：普通 opening `Drawn` frame 先播，播放结束后 bridge 再提交左右手 `HandAnchorEntered` follow-up frame。这个隐藏只是当前 v1 为了保证“抽牌后生成左右手”可见，不是完整阶段内临时布局系统。

`Drawn / RunHandEntered / Gained / HandAnchorEntered` 入场一旦由对应 slot 启动播放，同一 `CardInstanceId` 的普通 layout refresh 只能更新最新目标 slot，不能因 `ResetDistancePixels` 大跳变判定而取消入场或直接 snap 到目标；入场结束后再交回普通 layout motion。

普通 slot reflow 不再使用 `ResetDistancePixels` 这类距离阈值做硬重置；该参数仅作为兼容保留字段。后续如果需要“传送 / 切段 / 窗口恢复”一类瞬移，应通过显式 resync policy 表达，而不是让 SlotWidget 根据移动距离自行猜测。

后续新版抽牌动画应补充：

- 以 `BattleHandPresentationPlan` 聚合回合开始抽牌、`Effect.Draw` 和回合边界抽牌。
- 用 `CardLayerTransitionBatch` 表达同批卡、来源语义、stagger 和是否等待前一阶段。
- 明确不可见阶段的延迟播放策略，继续保留 presentation gate。

### Gained / Reward Enter

战斗中获得卡牌使用 `CardGained` 事件和 `Gained` hint。它和普通抽牌不同：语义来源是战斗奖励、击倒选择或其他获得入口，不是抽牌堆。

`Gained` 入场和 `Drawn / RunHandEntered / HandAnchorEntered` 一样是有限时长 enter playback，而不是只给新 slot 一个初始 offset 后交给普通 layout motion。Anchor `06 Transition Motion` 下的 `GainedCardEnterDurationSeconds`、`GainedCardEnterStaggerSeconds`、`GainedCardEnterArcLiftPixels`、`GainedCardEnterEasePower` 和 `bBlockInteractionDuringGainedCardEnter` 决定奖励卡入场的时长、错峰、弧线、缓动和播放期间交互阻塞；`07 Transition Audio` 下的 `GainedCardEnterSound` 决定奖励卡入手音效。

Anchor `19 Card Gain Reveal` 为同一个 `Gained` Enter 增加正面像素结晶。它直接消费真实 Enter progress，因此不会改变既有 `0.32s` 总时长、错峰、弧线、音效或交互阻塞；等待期不泄露卡面，完成时只让外缘按 White / Blue / Yellow / Purple 稀有度短暂着色。`CardGainedResolved` checkpoint 让命令级 planner 在手牌已满时先完成该 Enter，再播放实际被上限弃置卡的普通弃牌迁移。

后续可把 reward source、敌方部位来源、choice 类型转成更丰富的入场 origin，但不能让 first-person card layer 读取击倒规则。

### Played / Commit Exit

卡牌打出后用 `CardPlayed` 和 `Played` hint 表达离手。`RecordPlayCommit()` 可以让 `Played` hint 携带 commit feedback。

打出动画应和规则提交解耦：命令成功后规则立即结算，表现层可以播放离场、target cue 和 presentation stack，但不能阻塞 `UBattleSession` 状态更新。

### Discard / Exhaust Exit

`HandLimitDiscarded`、`CardDiscarded` 和 `CardExhausted` 当前都映射为 `Discarded` hint。表现上可以先统一向下或向弃牌语义方向离开。

后续如果要区分 exhaust，需要新增明确 transition kind 或 batch metadata，并同步文档、测试和 WBP 制作合同。

### Retain

`CardsRetained` 表示回合结束仍留在普通手牌中的卡。当前没有 `Retained` transition kind，只有独立 `Retained` feedback hint。

Retain 表现必须避免和“入手 / 离手”混在一起。当前正式表现是建立阶段阻塞、保持阶段非阻塞、解除阶段阻塞：

- 非保留普通卡执行 `TurnEndDiscard` 离场。
- 保留普通卡保持 slot identity，播放像素封存刻印并进入轻量 Held；左右手 anchor 不播放。
- 封存只叠加材质、上提和缩放，不修改 authored hand ZOrder；因此普通保留牌不会因长期 Held 越过相邻左右手 anchor。
- Held 跨过敌人行动、抽牌和普通 reflow，继续追随最新手牌布局。
- 抽牌与左右手生成完成后，`RetainedRelease` 缓出材质刻印和额外 Transform。

Retain hold / release 始终由 EndTurn Presentation Plan 显式编排；Widget 只消费 Hint 和维护单卡 Playback，不得自行从回合或 Snapshot 猜测解除时机，也不需要新增 `Retained` transition kind。

### Reflow

Reflow 是普通 layout motion，不是规则事件。手牌顺序、zone、投影中心、hover 状态、pending source 或窗口尺寸变化都可能触发 reflow。

Reflow 应遵循：

- 不重播 `Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded`。
- 不改变 `CardInstanceId` motion key。
- 同一卡普通 reflow 即使距离很远，也从当前 visual 平滑移动到最新目标 slot。
- 重同步优先服从 snapshot；真正需要瞬移时应由后续显式 resync policy 触发。

### Hover / Inspect / Drag

Hover、inspect 和 drag 是交互表现，不属于规则事件动画。

它们由 first-person card layer 的 gesture state 和 motion profile 管理：

- hover 只影响 lift、scale、ZOrder、详情和 camera look override。
- inspect 进入读牌姿态，可在 scrub 区域内切换预览源。
- drag 源卡进入 selected-source，目标反馈使用 world target 或 hand-card target validation。
- target affordance 和 target focus 不复用普通 hover。

这类表现可以覆盖最终 visual slot，但不能改变基础 input hit geometry、卡牌顺序或规则 target validation。

### Refresh / Resync

普通 snapshot refresh 不等于动画事件。

刷新策略：

- 没有 pending event 时，Battle hand 可以执行普通 entries refresh。
- 有 pending event 时，应提交显式 presentation frame。
- source clear、suppression、battle end 或新的显式 frame 才能替换 pending hints。
- Layer 只有在 gate 打开、slot 可投影并真正启动播放后，才消费 pending enter hint。

## §8 参数分层

卡牌动画参数分三层：

| 层 | 示例 | 保存位置 |
|---|---|---|
| 规则事实 | 抽到哪些卡、弃掉哪些卡、保留哪些卡 | `WacomBattle` event / snapshot / journal |
| 表现语义 | transition kind、batch 顺序、origin 语义、是否阻塞交互 | `WacomApp` presentation controller / future plan |
| 视觉参数 | duration、stagger、offset、arc、scale、rotation、opacity、material color | AnchorComponent Details、WBP、未来可选 DataAsset |

规则层不保存视觉参数。WBP 不保存规则事实。App 层负责把两边接起来。

当前 Details 入口：

| 分类 | 用途 |
|---|---|
| `05 Slot Motion` | 普通 slot motion、enter / exit baseline、reset distance |
| `06 Transition Motion` | `Drawn / RunHandEntered / Gained / HandAnchorEntered / Played / Discarded` 的事件感知来源和运动参数 |
| `07 Transition Audio` | `Drawn / RunHandEntered / Gained / HandAnchorEntered` 的入场音效和音量 / 音高倍率 |
| `07 Hover` | hover lift、scale、ZOrder 和命中稳定性 |
| `09 Gesture` | inspect、drag 起手、commit 距离和 drag 姿态 |
| `10 Interaction Feedback` | Pressed 实体按压、真实无效目标源卡预警、权威 Commit、formal-release 方向性 Deny、Drag Pickup 与 Retained 等源卡反馈 |
| `11 Drag Target Feedback` | world / card target affordance 和 focus |
| `12 Camera Look While UI` | hover / pointer / drag 时的镜头 look override |

文档可以记录参数类别和制作入口，但不应把某次调参值写成长期规则。

## §9 WBP 与 Runtime 动画边界

UMG / WBP 适合承载：

- 卡面排版和 `UWacomCardView` 视觉。
- 唯一 Retainer、卡面内容层与 authored brush；Pressed/Commit 由 Slot Motion 合成，无效目标括角与 Deny 四角刻线/方向性裂痕由 CardView Slate Paint 绘制。
- 静态层级结构、NamedSlot、控件大小和 brush 默认材质。
- 按钮、Toast、菜单等局部 UI 的短动画。

UMG Designer timeline 不适合作为核心手牌动画主线：

- 抽牌、弃牌、保留和出牌需要 `CardInstanceId`、事件顺序、可见卡过滤和 snapshot 重同步。
- 回合边界需要处理 journal checkpoint、不可见阶段 gate、快进和打断。
- 同一套动画要服务 Battle / Run / menu lease，不应绑定在某个 WBP timeline 上。

核心卡牌动画应由 runtime transition 系统驱动，WBP 只消费最终 view data 和材质反馈。

## §10 测试与验收

自动化测试负责合同：

- `CardsDrawn.CardInstanceIds` 只为真实入手且仍可见的卡生成 `Drawn` hint。
- 同批 `Drawn` hint 的 `SequenceIndex / SequenceCount` 稳定。
- `CardsDrawn` fallback 只为普通新手牌生成 `Drawn` hint，不把左右手 anchor 纳入抽牌预算。
- 新出现的左右手 anchor 在 EndTurn draw 后或 battle entry opening draw 后生成 `HandAnchorEntered` hint。
- `CardsRetained` 只为仍在 next hand snapshot 的普通保留手牌生成 retained feedback hint，不生成 first-person transition，也不包含左右手 anchor。
- EndTurn `TurnEndRetain` 只封存普通保留卡；`TurnStartRetainRelease` 在新牌与手牌 Anchor 入场后解除，左右手 anchor 始终不播放封存。
- presentation journal 消费一次，checkpoint 顺序和 ID 正确。
- 普通 refresh 不覆盖尚未消费的 explicit presentation frame。
- source suppression / gate 关闭时不提前消费入场 hint。
- `CardPlayed / CardDiscarded / CardExhausted` 离手语义不会被 reflow 误判。

PIE / 人工验收负责手感：

- 抽牌入场方向、错峰、弧线、速度、落位是否顺。
- 手牌重排是否稳，不闪、不跳、不吞卡。
- hover / inspect / drag 是否能读牌，是否遮挡关键 UI。
- target focus、valid / invalid affordance 和 deny feedback 是否明确。
- 不可见阶段进入战斗或切镜头时，卡牌是否不会提前播完。

## §11 后续设计入口

新版抽牌表现建议另起一轮，从以下切片开始：

1. 定义 `BattleHandPresentationPlan` 的最小 C++ 结构，只覆盖 EndTurn 三阶段和普通 `CardsDrawn`。
2. 定义 `CardLayerTransitionBatch`，把当前 loose hints 包成可带阶段语义的 batch。
3. 让 `FWacomBattleHandPresentationController` 从 journal 生成 EndTurn discard / retain / draw 计划。
4. 保持 first-person card layer 只消费 batch，不读取 journal。
5. 为 draw / retain / discard 的合同补小型 automation spec。
6. 最后进入 PIE 调手感，调整 Anchor `06 Transition Motion` 或新增制作参数。

待确认问题：

- 完整 EndTurn hand timeline 是否需要把 retain hold / deemphasis / recover 表达成 `BattleHandPresentationPlan` 阶段，而不是继续扩展 loose feedback hint。
- `CardExhausted` 是否需要从 `Discarded` 中拆出独立 transition。
- 抽牌 origin 是否需要真实牌堆 widget / screen anchor，还是继续使用 hand anchor / viewport anchor 语义。
- Run 探索期获得卡牌是否复用 `Gained`，还是需要 Run 专属入场语义。
- 快进动画时，是直接 settlement，还是播放压缩版 batch。
