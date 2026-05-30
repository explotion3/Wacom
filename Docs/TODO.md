---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-05-29
tags:
  - wacom/todo
  - wacom/docs
---

# TODO 索引

> [!info] 本文职责
> 这里只放短期任务入口和跳转，不写规则真相。当前规则看领域文档；长期方向看 [[Roadmap]]；临时写法看 [[TechDebt]]；待确认问题看 [[Questions]]。

> [!warning] 执行约束
> 标记为 `Blocked` 的任务不要直接写死设计口径。先在 [[Questions]] 或对应领域文档中收口，再进入实现。

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [[Roadmap]] | 未实现功能、后续方向、可按阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [[TechDebt]] | 临时写法、兼容字段、临时决定、正式替代方案 | 代码里出现 `TODO(技术债)` 时同步到这里 |
| [[Questions]] | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 短期任务索引和跳转 | 不承载长说明，避免重新变成大杂烩 |

## 当前推进顺序

1. 先收口 P0 规则问题。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

## P0 策划确认

- [ ] **确认减速、暮气、冻结等状态的数值公式和触发时机**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗规则
  - 入口：[Questions: 状态与触发时机](./Questions.md#questions-status)

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P1 近期实现候选

- [x] **交互目标系统：统一 target handle、Provider 接口、World 命中**
  - 状态：`Done`
  - 归属：Core / App
  - 说明：`WacomCore/Public/Types/WacomInteractionTargetTypes.h`（Handle + Kind + TargetTag + StableTargetId）、`WacomApp/Public/Interaction/WacomInteractionTargetProvider.h`（接口）、`WacomApp/Public/Components/WacomInteractionTargetComponent.h`（通用组件）、`UWacomBattleEnemyPartWorldTargetBridgeComponent`（Battle 敌方部位桥接）、`TryRouteBattleSceneTargetClick` 已使用 Provider 路径。Card / Zone TargetKind 的命中来源和规则层 Resolver 为后续任务。

- [ ] **交互目标系统：World target 注册表替换 TObjectIterator 扫描**
  - 状态：`Ready: 场景目标数量变多前`
  - 归属：App
  - 说明：当前 `BattleHUD::SyncBattleEnemyPartWorldTargets()` 和清理路径用同 World `UWacomBattleEnemyPartWorldTargetBridgeComponent` 扫描，适合 V0 验证。正式敌人 / Run 可交互 Actor 变多后，应改为组件注册 / 反注册到轻量 registry，避免全局扫描和跨系统清理范围过宽。

- [ ] **交互目标系统：Card-World / Card-Card drag resolver / Zone 命中来源接入**
  - 状态：`In Progress: V0-AJ 已接入 Run world target probe preview`
  - 归属：App / Battle
  - 说明：First-person hand 的 Card target 已由 `UWacomFirstPersonCardLayerSlotWidget / LayerWidget / AnchorComponent` 暴露为 `FWacomInteractionTargetHandle(TargetKind=Card)`，使用 visual slot screen position，且不要求目标卡可打。V0-Z 已把 first-person 源卡接入 hold inspect、无目标卡向上拖出释放提交、目标卡 aim arrow 和 world enemy part release 提交；V0-AA 保留 UMG mouse capture，同时用 drag pointer 驱动 BattleCameraLook override 和 widget-position world probe，避免拖拽时视角冻结或 target probe 读旧 cursor。V0-AB 增加释放前 affordance：无目标卡 armed 显示 commit-ready，合法/非法 world target 改变箭头颜色并触发场景 bridge transient preview，Card target 显示 probe 或确认色。V0-AC 后 BattleHUD 用 `ResolveFirstPersonCardDropIntent()` 统一 preview 和 release 语义。V0-AD 已让合法 `TargetMode=HandCard` 源卡拖到另一张手牌解析为 `PlayCardCardTarget` 并提交 `TargetCardInstanceId`，用于指定手牌加费 / 减费。V0-AE 已支持 `Effect.Card.DiscardSelected / ExhaustSelected + Target.SelectedHandCard`，可以把另一张普通手牌移入弃牌堆 / 消耗区。V0-AF 已统一手牌弃牌 / 消耗事件：真正弃牌会发 `CardDiscarded` 并触发目标卡 `OnDiscard`，消耗会发 `CardExhausted` 但不触发 `OnDiscard`；手牌上限仍保留 `HandLimitDiscarded` 兼容事件。V0-AG 后 Battle 中 `UBattleSession::ValidateTargetWithCard()` 提供可解释拒绝原因，`CanTargetWithCard()` 转调 validation；拖拽 HandCard 源卡时 first-person hand 会整手牌显示合法 / 非法 Card target affordance，源卡使用 visual-only selected-source 姿态但不进入旧 `TargetSelect`。V0-AH 后普通手牌 / 左右手锚点资格收口到 `UCardDefinition::HandCardTargetFilter`，显式 filter 优先，旧资产按 Add/ReduceCost 允许锚点、Discard/ExhaustSelected 拒绝锚点的兼容推断保持行为。V0-AI 后 filter 支持 `RequiredTargetKeywords / BlockedTargetKeywords`，目标有效关键词包含定义关键词和战斗临时关键词，左右手锚点允许后也同样参与 keyword 条件。V0-AJ 后 Run / 探索 Actor 可通过 `UWacomRunWorldInteractionTargetBridgeComponent` 标记为 `Interaction.Target.Run.Object`，PlayerController 可 probe 并播放轻量 preview；这仍不提交 Run 规则，也不替代 `IWacomWorldInteractable + E`。Zone target（背包 DropTarget 等）、Run 卡牌拖拽 / Run world resolver、费用/卡牌类型/区域/伙伴食物专用属性等复杂卡对卡筛选、更强 world outline/material polish 和真实飞牌轨迹仍待接入。

- [ ] **交互目标系统：规则层 Target Resolver**
  - 状态：`Ready: Card-to-Card / Run / Zone 规则接入时`
  - 归属：Battle / Run
  - 说明：域层 Resolver 根据 `TargetKind` 判断当前卡能否作用到目标，替代硬编码的 TargetMode 过滤。

- [ ] **接入击倒事件实际分支、奖励卡差异化和节点事件联动**
  - 状态：`Blocked: P0 击倒口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)

- [ ] **推进地图系统：节点、通道、迷雾、撤离回路、地图状态**
  - 状态：`Ready: 需先定模块边界`
  - 归属：Run / 地图
  - 入口：[Roadmap: 地图与探索](./Roadmap.md#roadmap-map)

- [ ] **推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档**
  - 状态：`Blocked: 地图节点口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 探索事件](./Roadmap.md#roadmap-runevent)

- [ ] **商店正式化：随机商品池、价格公式、正式 WBP、存档接入**
  - 状态：`Blocked: 地图节点 / 存档口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 商店](./Roadmap.md#roadmap-shop)

- [ ] **背包正式 WBP、拖拽 polish、必要时做增量刷新**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)

## P2 后续清理

- [x] **用编辑器迁移旧内容目录并清理 `_GAME` 临时蓝图目录**
  - 状态：`Done: Content Organization V1-A`
  - 归属：Content / App / Data
  - 入口：[Content Organization](./Content_Organization.md)

- [ ] **战斗手牌表现升级：扇形布局、拖拽出牌、3D 目标选择**
  - 状态：`Blocked: HD-2D 表现方案`
  - 归属：UI / 战斗表现
  - 入口：[Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)

- [x] **First-person card layer polish：详情面板跟随卡牌与层级修正**
  - 状态：`Done: V0-H 已接入独立 viewport popup host；V0-I 已用 BattleHandPresentationMode 收口第一人称手牌配置入口；V0-V 已加入共享详情读牌 motion`
  - 归属：UI / 战斗表现
  - 说明：旧 `UHandPanel` 详情继续使用 BattleHUD 内部 `CardDetailLayer`；first-person hover 详情使用独立 viewport panel。两种战斗详情 host 共用 `bEnableCardDetailReadabilityPolish` 下的短 hover delay、淡入淡出、轻量 scale、位置平滑跟随和贴边 side hysteresis；TargetSelect、命令提交、BattleEnd、Session 切换和 runtime hand 清理仍强制隐藏详情。战斗手牌入口由 `LegacyHandPanel / FirstPersonHandWithLegacyFallback / FirstPersonHandOnly` 三种模式控制。

- [ ] **First-person card render quality：扇形布局微调与专用卡面规范**
  - 状态：`In Progress: V0-K projected basis 已回退；V0-L 默认改为 Authored2D hand layout solver；V0-M 默认允许手牌锚点 SoftClamp 离屏；V0-N 已加入 hand center screen smoothing；V0-O 已加入 card slot motion polish；V0-P 已加入 slot motion lifecycle diagnostics / self-repair；V0-Q 已加入 event-aware card transitions；V0-R 已加入 pending / TargetSelect focus polish；V0-S 已加入 playable / hover / press feedback polish；V0-T 已加入 hand layout preset / tuning profile；V0-U 已加入 transition origin / card movement readability；V0-V 已加入 hover detail / inspect readability polish；V0-W 已加入 play commit / target impact readability；V0-Z 已加入 hold inspect + card drag / aim commit；V0-AA 已加入 drag camera look continuity`
  - 归属：UI / 战斗表现
  - 说明：继续保留 `WBP_FirstPersonCardView` 的 RetainerBox、透明留白、内部缩放和贴图采样规范。V0-Z 默认保留轻点出牌；hold delay 后读牌居中放大并显示详情，无目标卡向上拖出释放提交，有目标卡拉箭头到合法敌方部位释放提交，仍复用 BattleHUD / BattleSession 命令路径。后续只有在美术反馈需要时再微调扇形参数、下坠、层级、hover/pending 姿态、slot motion 速度、commit pulse、drag/aim 读牌姿态或角度 clamp；也可单独做多环境 preset / 战斗状态自动切 preset。不要为了抗锯齿牺牲当前手牌排布表现，也不要回退到 3D Presenter / RenderTarget 路线。

- [ ] **存档系统恢复：Bootstrap 读盘、PauseMenu Save、MainMenu Continue**
  - 状态：`Blocked: Demo 范围确认`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)

- [ ] **清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback**
  - 状态：`Ready: WBP 化后清理`
  - 归属：UI 架构
  - 入口：[TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

## 维护规则

> [!tip] 新任务放哪
> - 1-2 轮内准备做：放本文。
> - 只是未来方向：放 [[Roadmap]]。
> - 当前实现里已经存在的临时方案：放 [[TechDebt]]。
> - 需要你或策划确认口径：放 [[Questions]]。

> [!note] 迁移说明
> 原 `TODO.md` 的未实现功能已迁入 [[Roadmap]]；临时写法、临时决定、兼容入口已迁入 [[TechDebt]]；待确认规则问题已迁入 [[Questions]]。已经正式化到 `WacomRun.md`、`WacomBattle.md`、`WacomApp.md`、`WacomUI.md`、`WacomData.md` 的实现，不再在这里重复追踪。
