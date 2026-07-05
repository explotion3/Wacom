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
- `FBattlePresentationJournal` 是 C++ only 只读 checkpoint journal，当前只记录 EndTurn 的 `TurnEndDiscardResolved`、`TurnEndRetainResolved`、`TurnStartDrawResolved`。
- first-person card layer 当前 transition kind 包含 `Default`、`Drawn`、`Gained`、`HandAnchorEntered`、`Played`、`Discarded`。`HandAnchorEntered` 是 UI-only 左右手牌生成入手语义，不属于 `CardsDrawn`。
- EndTurn journal 现在由 `WacomApp` presentation coordinator 翻译为阶段化 plan：`TurnEndDiscard -> TurnEndRetain -> EnemyAction -> TurnStartDraw -> TurnStartHandAnchorEnter`。手牌阶段等待 first-person card layer 报告播放结束后再进入下一阶段；enemy phase v1 复用现有 battle event presentation queue。
- `FWacomBattleHandPresentationController` 在非 EndTurn phase plan 路径中，仍把 `CardsDrawn / CardGained / CardPlayed / HandLimitDiscarded / CardDiscarded / CardExhausted` 转为一帧 `entries + transition hints`，并把 `CardsRetained` 转为同帧 `feedback hints`。
- `UWacomFirstPersonCardAnchorComponent` 的 `05 Slot Motion` 和 `06 Transition Motion` 是当前卡牌入场、离场和事件感知转场的主要制作参数入口。
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
| `UWacomFirstPersonCardAnchorComponent` | 制作参数 facade、runtime source、projection、presentation gate | 不提交 Battle / Run 命令，不持有规则真相 |
| `UWacomFirstPersonCardLayerWidget` | reconcile active / outgoing slot，应用 transition hint，管理 layer-level gesture | 不读取牌堆或战斗规则 |
| `UWacomFirstPersonCardLayerSlotWidget` | 单槽 motion、hover / inspect / drag visual composition、入场 / 离场播放 | 不直接调用 BattleSession |
| `UWacomFirstPersonCardViewWidget` / WBP | 卡面内容、overlay、材质参数、局部反馈图层 | 不决定手牌顺序、目标合法性或动画队列 |

所有卡牌动画都应以 `CardInstanceId` 为稳定身份。Slot index 只表示当前布局位置，不能作为动画身份、事件身份或重同步判断依据。

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
| `TurnEndRetain` | `TurnEndRetainResolved` checkpoint | checkpoint snapshot + 普通保留手牌与仍在手牌中的左右手 anchor `Retained` feedback hints | 同上 |
| `EnemyAction` | retain / discard checkpoint 与 draw checkpoint 之间的 battle events | 现有 battle event presentation queue | event queue finished |
| `TurnStartDraw` | `TurnStartDrawResolved` checkpoint | 临时隐藏新出现左右手 anchor 的 checkpoint snapshot + `Drawn` transition hints | card layer 没有 active presentation playback，或 timeout 兜底 |
| `TurnStartHandAnchorEnter` | `TurnStartDrawResolved` checkpoint | 完整 checkpoint snapshot + 新出现左右手 anchor 的 `HandAnchorEntered` transition hints | 同上 |

## §5 动画语义词汇

| 术语 | 定义 |
|---|---|
| `Transition` | 单张卡的入场、离场或语义移动。当前由 `FWacomFirstPersonCardLayerTransitionHint` 表达 |
| `Feedback` | 单张卡的非阻塞短反馈，不改变入场 / 离场语义。当前由 `FWacomFirstPersonCardLayerFeedbackHint` 表达 |
| `TransitionBatch` | 同一规则或表现阶段产生的一组 transition。当前还没有独立类型 |
| `PresentationFrame` | 一次原子提交给 card layer 的 `entries + transition hints + feedback hints` |
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
- WBP 控件名描述视觉图层，例如 `FeedbackOverlay`、`InteractionFeedbackImage`。

## §6 当前 Hint 合同

`FWacomFirstPersonCardLayerTransitionHint` 是当前 first-person card layer 的最小 transition 输入：

| 字段 | 语义 |
|---|---|
| `CardInstanceId` | 目标卡实例 ID |
| `TransitionKind` | `Default / Drawn / Gained / HandAnchorEntered / Played / Discarded` |
| `SequenceIndex` | 同批可见 transition 的稳定序号 |
| `SequenceCount` | 同批可见 transition 总数 |
| `bPlayCommitFeedback` | 打出离场时是否播放提交反馈 |
| `bHasPlayedExitTargetWidgetPosition` | 打出离场是否有目标 widget 位置 |
| `PlayedExitTargetWidgetPosition` | 打出离场目标位置，widget-space |

当前生成口径：

| 事件 | 当前 transition |
|---|---|
| `CardsDrawn` | 对仍存在于 next hand snapshot 的 `CardInstanceIds` 生成 `Drawn`，并按最终手牌槽位从左到右写入稳定 `SequenceIndex / SequenceCount` |
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

`CardsRetained` 不生成 first-person transition。Layer 收到 retained feedback 后只在匹配 slot 上播放短促锁定脉冲；若 slot 暂不可投影，hint 保留到下一次可见刷新。播放期间的普通 refresh 只能更新 slot 最新布局目标，不能取消 retained feedback。

EndTurn phase plan 的 `TurnEndRetain` 阶段会在不改变 `CardsRetained` 规则事件的前提下，额外为 retain checkpoint snapshot 中仍存在的左右手 anchor 生成同款 `Retained` feedback hint。左右手 anchor 只是共享当前反馈视觉，不进入 `CardsRetained.CardInstanceIds`。

## §7 动画类型目录

### Draw / Enter Hand

抽牌动画的权威输入是 `CardsDrawn.CardInstanceIds`，不是前后 snapshot 猜测。放不下的牌不会进入该列表，也不应播放入手动画。左右手 anchor 不是普通抽牌对象：当它们在 battle entry 或回合开始从无到有进入手牌时，App 层用 `HandAnchorEntered` 表现生成入手。

同批可见 `Drawn` 入场的错峰顺序以目标 hand snapshot 中的普通手牌槽位为准：最左侧目标槽先进入，依次到最右侧。`CardsDrawn.CardInstanceIds` 只决定“哪些卡真实入手”，不决定最终 stagger 方向。

当前 layer 可消费 `Drawn` hint，并由 Anchor `06 Transition Motion` 控制来源模式、offset、viewport anchor、scale、angle、duration、stagger、arc lift、ease 和播放期间交互阻塞。

`TurnStartDraw` 阶段会暂时不提交本次新出现的左右手 anchor entries，让普通抽牌先完成；随后 `TurnStartHandAnchorEnter` 提交完整 hand snapshot，并只为这些新出现的 anchor 播放 `HandAnchorEntered`。Battle entry reveal 也采用同样两段式：普通 opening `Drawn` frame 先播，播放结束后 bridge 再提交左右手 `HandAnchorEntered` follow-up frame。这个隐藏只是当前 v1 为了保证“抽牌后生成左右手”可见，不是完整阶段内临时布局系统。

`Drawn / Gained / HandAnchorEntered` 入场一旦由对应 slot 启动播放，同一 `CardInstanceId` 的普通 layout refresh 只能更新最新目标 slot，不能因 `ResetDistancePixels` 大跳变判定而取消入场或直接 snap 到目标；入场结束后再交回普通 layout motion。

后续新版抽牌动画应补充：

- 以 `BattleHandPresentationPlan` 聚合回合开始抽牌、`Effect.Draw` 和回合边界抽牌。
- 用 `CardLayerTransitionBatch` 表达同批卡、来源语义、stagger 和是否等待前一阶段。
- 明确不可见阶段的延迟播放策略，继续保留 presentation gate。

### Gained / Reward Enter

战斗中获得卡牌使用 `CardGained` 事件和 `Gained` hint。它和普通抽牌不同：语义来源是战斗奖励、击倒选择或其他获得入口，不是抽牌堆。

后续可把 reward source、敌方部位来源、choice 类型转成更丰富的入场 origin，但不能让 first-person card layer 读取击倒规则。

### Played / Commit Exit

卡牌打出后用 `CardPlayed` 和 `Played` hint 表达离手。`RecordPlayCommit()` 可以让 `Played` hint 携带 commit feedback。

打出动画应和规则提交解耦：命令成功后规则立即结算，表现层可以播放离场、target cue 和 presentation stack，但不能阻塞 `UBattleSession` 状态更新。

### Discard / Exhaust Exit

`HandLimitDiscarded`、`CardDiscarded` 和 `CardExhausted` 当前都映射为 `Discarded` hint。表现上可以先统一向下或向弃牌语义方向离开。

后续如果要区分 exhaust，需要新增明确 transition kind 或 batch metadata，并同步文档、测试和 WBP 制作合同。

### Retain

`CardsRetained` 表示回合结束仍留在普通手牌中的卡。当前没有 `Retained` transition kind，只有独立 `Retained` feedback hint。

Retain 表现必须避免和“入手 / 离手”混在一起。当前 v1 是非阻塞反馈：

- 非保留普通卡执行 `TurnEndDiscard` 离场。
- 保留普通卡和仍在手牌中的左右手 anchor 保持 slot identity，播放轻量 lift / scale / 暖金色 feedback pulse。
- 下回合抽牌完成后，保留卡参与普通 reflow。

后续如果要做完整 EndTurn hand timeline，应在 `BattleHandPresentationPlan` 里表达 retain hold / deemphasis / recover 阶段；不要把当前 retained feedback 升级成 Widget 自发状态，也不要为 v1 新增 `Retained` transition kind。

### Reflow

Reflow 是普通 layout motion，不是规则事件。手牌顺序、zone、投影中心、hover 状态、pending source 或窗口尺寸变化都可能触发 reflow。

Reflow 应遵循：

- 不重播 `Drawn / Gained / HandAnchorEntered / Played / Discarded`。
- 不改变 `CardInstanceId` motion key。
- 大跳变超过 reset distance 时可以直接贴合，避免慢漂。
- 重同步优先服从 snapshot，不保留过期 visual 幻象。

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
| `06 Transition Motion` | `Drawn / Gained / HandAnchorEntered / Played / Discarded` 的事件感知来源和运动参数 |
| `07 Hover` | hover lift、scale、ZOrder 和命中稳定性 |
| `09 Gesture` | inspect、drag 起手、commit 距离和 drag 姿态 |
| `10 Interaction Feedback` | pressed、confirm、commit、deny、retained 等源卡反馈 |
| `11 Drag Target Feedback` | world / card target affordance 和 focus |
| `12 Camera Look While UI` | hover / pointer / drag 时的镜头 look override |

文档可以记录参数类别和制作入口，但不应把某次调参值写成长期规则。

## §9 WBP 与 Runtime 动画边界

UMG / WBP 适合承载：

- 卡面排版和 `UWacomCardView` 视觉。
- `FeedbackOverlay`、`InteractionFeedbackImage`、材质参数和局部微反馈。
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
- EndTurn `TurnEndRetain` phase 可以为仍在 retain checkpoint snapshot 中的左右手 anchor 额外生成 retained feedback hint，但不改变 `CardsRetained` 事件内容。
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
