---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-07-13
tags:
  - wacom/todo
  - wacom/docs
---

# TODO 索引

> [!info] 本文职责
> 本文只放短期任务入口、优先级、归属和跳转，不写规则真相或实现史。当前规则看领域文档；长期方向看 [`Roadmap.md`](Roadmap.md)；临时写法看 [`TechDebt.md`](TechDebt.md)；待确认问题看 [`Questions.md`](Questions.md)。

> [!warning] 执行约束
> 标记为 `Blocked` 的任务不要直接写死设计口径。先在 [`Questions.md`](Questions.md) 或对应领域文档中收口，再进入实现。

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [`Roadmap.md`](Roadmap.md) | 未实现功能、长期方向、可分阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [`TechDebt.md`](TechDebt.md) | 临时写法、兼容字段、临时决定、正式替代方案 | 代码里出现 `TODO(技术债)` 时同步到这里 |
| [`Questions.md`](Questions.md) | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 1-2 轮内可能推进的短期任务索引 | 不承载长说明，不复制领域规则 |

## 当前推进顺序

1. 先收口 P0 规则问题。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

文档重构进度：V0-FP 完成剩余文档漂移清理：Battle WBP 装备占位、Data Durability、GDD 自引用和索引文档 wikilink 已收口。

## P0 策划确认

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P0.5 近期架构规划

- [ ] **WacomMap 总体规划：新建地图 / 节点 / 通道规则文档入口**
  - 状态：`Ready: 优先规划`
  - 归属：Map / Run / App
  - 入口：[Roadmap: 地图与探索](./Roadmap.md#roadmap-map) / [Questions: Run、探索与地图](./Questions.md#questions-run-map)
  - 说明：先规划 `Docs/WacomMap.md` 的职责边界，明确 WacomMap 管逻辑地图、节点状态、边 / 通道可达性、迷雾、撤离回路、地图运行时状态和 SaveGame 口径；RunTunnel 继续作为 App 表现 / 输入层，不把 Spline、Actor 或 UI 写进地图规则。

## P1 近期实现候选

- [ ] **正式资产依赖审计与 `/Game/Wacom` 迁移**
  - 状态：`Ready: worktree 本地依赖层已建立`
  - 归属：Content / Editor / Build
  - 入口：[Worktree Development](./Worktree_Development.md) / [Content Organization](./Content_Organization.md)
  - 说明：使用 AssetRegistry 找出 `/Game/Wacom` 与正式地图对 `/Game/Art`、`/Game/Asset`、`/Game/DreamMaterials`、`/Game/L_TestBattle` 的真实引用；自有出货资产经编辑器迁入 `/Game/Wacom`，第三方内容建立版本化安装清单。完成前由每个 worktree 的独立 D 盘本地依赖层支持编译和 PIE。

- [ ] **背包 Workspace 重构：规则与 C++ 交互完成，正式资产/PIE 待收口**
  - 状态：`In Progress: C++ + automation complete; WBP/material/PIE pending`
  - 归属：Run / App / UI / Tests
  - 入口：`specs/003-backpack-workspace-refactor/`
  - 说明：已完成单活动 Workspace、常驻 ZoneRack、同 Run 瞬态布局、框选、持续扇形携带、首次释放守卫、滚轮当前牌、原子批量移动/销毁、确认取消/失败恢复和 C++ fallback。2026-07-13 已通过 `WacomEditor`、`Wacom.Run.Backpack` 2/2、`Wacom.UI.Backpack` 66/66 以及 `CompileAllBlueprints`（0 error；1 个既有 deprecated Blueprint warning）。剩余是正式 WBP/Style/反馈材质资产、旧 drag/drop 兼容类清理和完整 PIE 手感验收。本线程按协作边界不修改 DreamShader 卡牌表现文件。
  - 全量回归证据：`Automation RunTests Wacom` 找到 1302 项后，在背包范围外失败并中止：`Wacom.UI.Battle.BattleHUD.HandPresentation.ShortcutStartsDragByHandIndex`、`Wacom.UI.Battle.FirstPersonTargetPreview.NoTargetCommitShowsPlayerActionPreview` 失败，随后 `BattlePresentationQueueSpec.cpp:259` 发生 `Array index out of bounds: 1 into an array of size 1`。该组属于暂停的 Battle/first-person 卡牌表现线程，本切片未越界修改。

- [x] **敌人系统重构：稳定身份、行为数据化、场景表现拆分**
  - 状态：`Done: 主链路已落地`
  - 归属：Data / Battle / Run / App / Editor
  - 入口：`specs/002-enemy-system-refactor/`
  - 说明：稳定 enemy key、Run 撤离重入进度、command / snapshot / event / result packet、行为数据化、Encounter 接线、SceneEnemyHost registry、Host 整体视觉、PartActor hit-only / VisualLayers 和 BattleHUD 场景目标主链路已落地。长期事实已回写到 `Docs/WacomBattle.md`、`Docs/WacomData.md`、`Docs/WacomRun.md`、`Docs/WacomWorldInteraction.md`、`Docs/WacomBattleUI.md` 和 `Docs/WacomDataAuthoring.md`；`specs/002-enemy-system-refactor/` 只保留为阶段性规划和最终搜索 / 验证清单。

- [x] **UI Ownership / BattleHUD 私有 coordinator 拆分**
  - 状态：`Done`
  - 归属：UI 架构 / App
  - 入口：[WacomUI: UI Ownership Map](./WacomUI.md#wacomui-ui-ownership-map) / [TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

- [x] **交互目标系统：统一 target handle、Provider 接口、World 命中**
  - 状态：`Done`
  - 归属：Core / App
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md)

- [x] **交互目标系统：Battle scene enemy current Host registry**
  - 状态：`Done`
  - 归属：App / Battle World Target
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomBattleUI.md](./WacomBattleUI.md)

- [ ] **战斗表现队列：快速连出后的视觉追赶 polish**
  - 状态：`Ready: 体验 polish`
  - 归属：App / UI
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)
  - 说明：后续只追踪表现体验，如 cue 合并、速度压缩、正式动画、stack 入场 polish、动画回放或规则层 command batch id。

- [ ] **First-person hand 动效第二阶段：世界目标命中反馈与整体 PIE polish**
  - 状态：`In Progress: 手牌目标像素刻印已接线，待世界目标与 PIE polish`
  - 归属：App / First-person Card Layer
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：基础合同已完成显式空帧替换、Motion Mixer / 互斥 Transition Playback、真实 draw/discard/play anchors、Card Depth、单 Retainer 实时 Alpha 接触阴影、Drag Pickup、普通 Played 像素翻面收牌、回手重构、Exhausted 消耗消散，以及成功 HandCard 目标的“弱刻印 Preview → 延迟压印 → 留手归位 / 弃牌 / 消耗”结果衔接。旧 DiamondWave 与 PixelAsh Style 均保留。Played 仍保留真实世界目标 / PlayTarget 坐标；下一切片只需消费该坐标制作场景目标命中反馈，并继续做音效预热、reduced-motion / 全局速度策略和 PIE 调参。不复制第三方代码、shader 或资产。

- [ ] **战斗规则内容化：按 authoring matrix 扩展正式卡牌 / 敌人内容**
  - 状态：`In Progress: 内容扩展`
  - 归属：Battle / Data / Editor
  - 入口：[WacomDataAuthoring.md](./WacomDataAuthoring.md) / [WacomBattle.md](./WacomBattle.md)
  - 说明：继续做正式掉落 / 奖励池、更多敌人包、卡牌平衡和正式卡组入口；新增规则能力前先接 resolver、合同、文档和验证。

- [x] **Battle Card Zone Transition 后续迁移**
  - 状态：`Done: CardZoneAggregate + 全路径 typed transition 已完成`
  - 归属：Battle 规则内核
  - 入口：[WacomBattle §5](./WacomBattle.md) / [TechDebt: 规则层技术债](./TechDebt.md)
  - 说明：卡牌注册、六个定位容器、Runtime Location 与顺序已收口到 `CardZoneAggregate`；Effect、EndTurn、HandLimit、奖励和 Companion 统一由 `BattleCardZoneTransition` 消费 typed facts，旧 post-move event API 已删除。

- [ ] **战斗场景敌人表现 polish：正式美术、描边、tooltip 和动画状态机**
  - 状态：`Ready: 美术 / 表现 polish`
  - 归属：App / UI / Battle World Target
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：主链路已经能支撑普通小怪 Host 整体图 + hit-only 部位，以及精英 / Boss PartActor VisualLayers。后续只追踪正式蛇 Host prefab 美术保存、正式 sprite/flipbook 样式、材质描边、贴近部位 tooltip、风险动效、PaperZD / Animator 状态机和 Status Badge 美术替换。

- [x] **EncounterDefinition 运行时接线：BattleTrigger 引用 Encounter 并构造 Battle EnemySlots**
	- 状态：`Done: Trigger 已接线`
	- 归属：Data / Run / App / Battle
	- 入口：[WacomData.md](./WacomData.md#wacomdata-encounter-definition) / [WacomRun.md](./WacomRun.md#wacomrun-battle-settlement)
	- 说明：`ABattleTriggerActor.EncounterDefinition` 已转换为 `FBattleInitParams.EnemySlots`；运行态 `EncounterId` 仍用 Trigger `PersistentId`。旧单敌人 Trigger 入口已删除。

- [x] **BattleTrigger 多 SceneEnemyHost 映射：EnemySlotId -> Host registry**
	- 状态：`Done: Trigger 已接线`
	- 归属：App / Battle World Target
	- 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md#5-battle-scene-target) / [WacomBattleUI.md](./WacomBattleUI.md#6-scene-enemy-ui)
	- 说明：`ABattleTriggerActor.SceneEnemyHostSlots` 已按 `EnemySlotId` 绑定多 Host；旧单 `SceneEnemyHost` Trigger 入口已删除。后续敌人方向集中到正式蛇 Host 蓝图 prefab 资产、正式美术样式和制作工具。

- [ ] **交互目标系统：Card-World / Card-Card drag resolver / Zone 命中来源接入**
  - 状态：`Ready: 剩余正式内容`
  - 归属：App / Battle / Run
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)
  - 说明：基础链路已进入当前事实文档；TODO 只追踪正式内容、动画、SaveGame、复杂筛选、掉落表和更多奖励类型。

- [ ] **交互目标系统：Run / Zone 域规则求值接入**
  - 状态：`Ready: Run / Zone 正式规则接入时`
  - 归属：Run
  - 入口：[WacomBattle.md](./WacomBattle.md) / [WacomRun.md](./WacomRun.md)
  - 说明：Battle 已由 `PlayCard Evaluation` 权威处理 Card-World / Card-Card；本项只追踪 Run 与 Zone 后续各自的领域求值，避免重新引入跨域通用 Resolver。

- [ ] **接入击倒事件实际分支、奖励卡差异化和节点事件联动**
  - 状态：`Blocked: P0 击倒口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)

- [ ] **推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档**
  - 状态：`Blocked: 地图节点口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 探索事件](./Roadmap.md#roadmap-runevent)

- [ ] **商店正式化：随机商品池、价格公式、正式 WBP、存档接入**
  - 状态：`Blocked: 地图节点 / 存档口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 商店](./Roadmap.md#roadmap-shop)

- [ ] **背包正式 WBP、拖拽 polish、必要时做虚拟列表**
  - 状态：`In Progress: Workspace 绑定资产与材质待制作`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)
  - 说明：正式资产需绑定 `WorkspaceHost / ZoneRackHost / DeleteTargetHost / DeleteConfirmHost / ArrangeAllButton`，创建 Workspace、ZoneRack、Entry、DeleteConfirm 和 Style 资产，再移除旧 drag/drop 输入 owner。虚拟列表仅在 20–100 卡 PIE 证明需要后实施。

- [ ] **卡牌详情 token：ConditionTokenBuilder**
  - 状态：`Ready: 详情表现继续收口`
  - 归属：UI / Data presentation
  - 入口：[WacomUI.md](./WacomUI.md#wacomui-ui-ownership-map) / [WacomData.md](./WacomData.md#3-card-definition)
  - 说明：当前详情 token 已覆盖主动效果和被动触发 / 效果；`FEffectCondition` 只显示“有条件”弱提示。后续需要把 `Condition.Self.InZone`、`Condition.Target.HasStatus` 等条件生成结构化 token。

- [ ] **卡牌详情描述字段收口：Description vs FlavorText**
  - 状态：`Ready: 内容清理前置规划`
  - 归属：Data / UI presentation
  - 入口：[WacomData.md](./WacomData.md#3-card-definition) / [WacomUI.md](./WacomUI.md)
  - 说明：规则说明应逐步从 `Description` 迁到结构化 token；`Description` 是否保留为补充说明或拆出 `FlavorText` 需要单独规划，避免直接删除导致现有卡牌可读性下降。

- [ ] **RunEvent 正式 WBP 外观接入**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / RunEvent
  - 入口：[RunEvent WBP 绑定清单](./UI_RunEvent_WBP_Binding.md)
  - 说明：只替换外观、布局和 preview 表现；C++ fallback 继续负责动态选项、ZoneId、menu lease、drop intent 和 RunEvent 事务。

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
  - 状态：`Done`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)

- [ ] **First-person card render quality：扇形布局微调与专用卡面规范**
  - 状态：`In Progress: 美术与渲染 polish`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：后续按美术反馈微调扇形参数、卡面采样、Retainer 外元素、slot motion、commit pulse 和 drag / aim 读牌姿态。

- [x] **牌堆牌印表现第二切片：接收脉冲与弃牌化牌印**
  - 状态：`Completed: 普通弃牌与弃牌堆接收反馈已落地`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)
  - 说明：2026-07-13 已完成普通 `CardDiscarded` 的实体卡原地收束、逐卡真实起点、牌印飞向弃牌堆、逐枚计数、像素 Impact 与真实 `DiscardPileView` 接收回弹；弃牌堆洗回继续复用同一批量 Slate renderer。两种传输按 Kind+Sequence 去重并 FIFO 播放，普通弃牌不复用洗牌音效，失效配置回退旧 Exit，Reduced Motion 不跨屏飞行或修改 PileView Transform。接收反馈由通用 `UPileCountView` 管理可叠加脉冲并精确恢复 authored Transform，当前只连接 `DiscardToPile`；未来可按同一接口扩展 DrawPileView / ExhaustPileView。

- [ ] **单一玩家档案与旅程存档：活动旅程、滚动备份、历史摘要**
  - 状态：`Ready: 产品口径已确认，等待独立规格与实现`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)
  - 说明：采用一个玩家档案、一个活动旅程、若干不可见滚动备份；结束旅程生成只读历史摘要。该切片负责恢复 Bootstrap / PauseMenu Save / MainMenu Continue，不把 slot 语义写回 MainMenu Screen。

- [ ] **MainMenu 正式表现：WBP、实时场景与子页面**
  - 状态：`Ready: C++ ViewData / Action 契约已完成`
  - 归属：App / UI
  - 入口：[Roadmap: App 壳层与菜单](./Roadmap.md#roadmap-app-shell-and-menus)
  - 说明：`WBP_MainMenuScreen` 已进入制作阶段，六个导航入口合同已统一为 `UWacomMainMenuButtonWidget`；后续先创建其视觉子类 `WBP_MainMenuNavButton`，再完成正式像素视觉、`L_MainMenu` 实时场景和固定镜头，并依次接入旅程准备、旅程记录、设置与制作人员页面。当前 C++ fallback 继续保证主菜单可玩。

- [ ] **清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback**
  - 状态：`Ready: WBP 化后清理`
  - 归属：UI 架构
  - 入口：[TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

- [ ] **WacomApp Public UI API surface 收口切片**
  - 状态：`Ready: 低风险公开面继续收口`
  - 归属：UI 架构 / App
  - 入口：[TechDebt: WacomApp Public UI API surface](./TechDebt.md#techdebt-wacomapp-public-ui-api-surface)
  - 说明：后续优先处理 prototype / test-only surface。真正删除、迁到测试模块、迁到开发插件、移除控制台入口或重命名旧反射字段，都另起资产影响明确的切片；Blueprint-visible 制作面默认保守保留。

## 维护规则

> [!tip] 新任务放哪
> - 1-2 轮内准备做：放本文。
> - 只是未来方向：放 [`Roadmap.md`](Roadmap.md)。
> - 当前实现里已经存在的临时方案：放 [`TechDebt.md`](TechDebt.md)。
> - 需要你或策划确认口径：放 [`Questions.md`](Questions.md)。

> [!note] 迁移说明
> 原 `TODO.md` 的未实现功能已迁入 [`Roadmap.md`](Roadmap.md)；临时写法、临时决定、兼容入口已迁入 [`TechDebt.md`](TechDebt.md)；待确认规则问题已迁入 [`Questions.md`](Questions.md)。已经正式化到领域文档、UI 专题文档和 Data 专题文档的实现，不再在这里重复追踪。
