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

- [ ] **背包 Workspace 重构：自动化制作与旧路径迁移完成，统一 PIE 验收待收口**
  - 状态：`In Progress: C++ + formal WBP + DreamShader + legacy cleanup complete; final PIE pending`
  - 归属：Run / App / UI / Tests
  - 入口：`specs/003-backpack-workspace-refactor/`
  - 说明：已完成单活动 Workspace、常驻 ZoneRack、同 Run 瞬态布局、框选、持续扇形携带、首次释放守卫、滚轮当前牌、原子批量移动/销毁、确认取消/失败恢复、C++ fallback，以及正式 Screen/Workspace/ZoneRack/Entry/DeleteConfirm/Style 资产。资产由 `WacomBuildBackpackUI` 可重复生成，连续生成无 GUID ensure；builder 保证 `WBP_WacomDeckCardWidget.BackpackCardView` 使用独立 `WBP_BackpackCardView`，在一个 invalidation-only、无逐帧 phase、无 effect material 的 Retainer 内复用新版 `WBP_FirstPersonCardView`，并通过外层 `CardFaceScaleBox` 使用固定 `0.75` 缩放和整数像素对齐，避免卡牌高度变化触发重新缩放或直接压缩布局导致费用、名称、耐久和效果徽章错位。卡面上方已增加不参与命中/布局的 `WorkspaceFeedbackOverlay`，选中、合法目标和拒绝状态由 Style 驱动；携带 ActiveTimer 会持续采样 Slate cursor，快速移出卡牌自身范围后不再依赖卡牌 PointerMove。已选卡再次 PointerDown 现在无需等待鼠标移动即可立即形成携带扇形，对应 PointerUp 只消耗首次释放守卫；下一次左键/右键释放分别提交当前牌/全部牌。首轮运行截图发现并修复 Screen 只按最小 Desired Size 占半屏的问题，实际 1600×900 窗口已确认完整工作台/牌匣几何。`整理当前区域` 的 Construct/Destruct 绑定错误已修复并移到顶部命令栏；密集默认布局会压缩行距，不再把超高行 Clamp 到同一底边。框选输入现在由 Workspace 在完整事务期间持续持有：指针经过卡牌不会释放捕获，左键在卡牌上松开也会正常完成框选；正式 WBP 与 C++ fallback 均覆盖该回归。区域切换回归同时验证 7 张备战牌在两条路径上获得互不重叠的 Canvas 槽位。PIE 进一步定位并修复了运行时 Workspace 子控件继承 `OverlaySlot` Left/Top 默认值、导致几何压缩为零或 `190×33` 的根因；Screen 现统一为动态 Host 子控件配置 Fill，并让 Workspace 以 `Visible` 接管空白区域输入，用户已确认切区铺牌和框选均正常。Editor 命令 `Wacom.Backpack.SeedPIEValidation` 已通过正常 Run 获牌入口建立 24 张实体牌/2 个 SpecialZone 的可重复基线；PIE 已确认 4 个牌匣条目、15 张可移动牌框选、首次松开持续携带、无详情遮挡和无索引 UI。2026-07-14 当前切片已通过最终 `WacomEditor`、`Wacom.Run.Backpack` 2/2、`Wacom.UI.Backpack` 59/59、`CompileAllBlueprints`（0 error；6 个既有 warning）以及正式 8/8 生成资产二次生成 SHA-256 稳定性检查。当前已新增 Graph-first DreamShader 反馈材质并由 Style 绑定，删除旧 `WacomCardDragOperation` / ZoneDropTarget / DeleteZoneDropTarget 及其旧测试 access，迁移 Host 只保留只读 fallback；59 项 UI 用例覆盖新 Workspace 合同，较旧 68 项减少的 9 项均为已移除 drag/drop owner 的过时断言。固定 ScaleBox、静态 Retainer、材质绑定与 Workspace 输入合同由自动化覆盖；卡面最终采样质量、快速移出仍跟随和左右释放手感属于真实 Slate/渲染行为，仍需进入统一 PIE 对照。T037/T070 仍缺可靠的卡面抗锯齿对照、滚轮手感、左右释放、快速移出跟随、选中反馈、SpecialZone/Burden/拒绝、确认恢复、完整 CommonUI 生命周期和 DreamShader-disabled fallback 人工证据。
  - 2026-07-14 Surface Foil 跟进：`WBP_BackpackCardView` 继续复用共享 `WBP_FirstPersonCardView` 排版，但 wrapper 默认按实例关闭内层动态 `SurfaceFoilOverlay`，运行时折叠该层并清空材质 Brush；战斗/第一人称卡面的默认流光不变。新增红→绿 runtime contract 与正式资产 CDO contract；验证通过 `WacomEditor`、`Wacom.Run.Backpack` 2/2、`Wacom.UI.Backpack` 60/60、`CompileAllBlueprints`（0 error；1 个既有蓝图弃用 warning，进程汇总 6 个既有 warning），资产 builder 0 error 且正式 8/8 资产连续生成 SHA-256 稳定。PIE 仍需确认背包卡面不存在冻结流光帧，并对照 authored 卡面检查最终抗锯齿与出血徽章。
  - 2026-07-14 Workspace Feedback SM6 修复：`UMaterialExpressionVertexColor` 默认输出 0 实际为 RGB `float3`，Alpha 是独立输出 4；隐式 `.w` 与对输出 0 使用 A `ComponentMask` 都会报 `Not enough components`。`.dsm` 现直接消费 OutputIndex 0/4，并通过 DreamShader `-Force` 重建正式材质；`WorkspaceFeedbackCompiles` 红→绿测试会实际重编译材质、穿透 Named Reroute 核对输出索引并拒绝非法 A mask，真实 D3D12 `PCD3D_SM6` 定向测试及完整 `Wacom.UI.Backpack` 61/61 均通过。
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

- [ ] **背包正式 WBP、交互 polish、必要时做虚拟列表**
  - 状态：`In Progress: Workspace/材质/旧路径清理完成；统一 PIE polish 待完成`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)
  - 说明：正式 Screen、Workspace、ZoneRack、Entry、DeleteConfirm、Style 和反馈材质已生成并由资产合同自动化覆盖；旧单卡 UMG drag/drop input owner 已删除。剩余统一处理真实鼠标 PIE 观感/手感；虚拟列表仅在 20–100 卡 PIE 证明需要后实施。

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

- [ ] **MainMenu 正式表现：实时场景与剩余子页面**
  - 状态：`In Progress: 正式 WBP 壳层与导航按钮已完成，等待 PIE 视觉验收和场景切片`
  - 归属：App / UI
  - 入口：[Roadmap: App 壳层与菜单](./Roadmap.md#roadmap-app-shell-and-menus)
  - 说明：`WBP_MainMenuScreen + WBP_MainMenuNavButton` 已建立第一版正式壳层，Settings Screen 与主菜单 / Pause 双入口也已完成。剩余工作是 `L_MainMenu` 实时场景、固定镜头、音频 / 字体 / 像素纹理美术验收，以及 Journey History / Credits 子页面。

- [x] **Settings Screen：WBP、视频确认 Modal 与双入口**
  - 状态：`Done: 五分类页面、token 事务、双入口和 Run Tunnel CameraShake 已落地`
  - 归属：App / UI
  - 入口：[WacomApp.md](./WacomApp.md) / [WacomUIFoundation.md](./WacomUIFoundation.md)
  - 说明：`WBP_SettingsScreen / OptionRow / Button / ConfirmationDialog` 与 C++ fallback 已完成；主菜单和暂停菜单统一走 `FWacomSettingsScreenFlow`。Apply 后留在页面，视频模式使用 15 秒确认，脏返回确认放弃，外部 teardown 安全 Cancel / Revert。项目平衡档统一首次启动与“恢复默认”，恢复按钮只装入可撤销 Draft，仍需 Apply。分辨率已收敛为最低 `1280 × 720` 的常用档位与模式专属平台过滤；全局 UI 使用 `1920 × 1080` 设计基准的封顶 DPI 规则，只在较小视口缩小，高分辨率不放大固定 HUD 元素。`BP_WacomPlayerCharacter` 正式启用 CameraShake 并关闭 WalkBob。

- [ ] **低于 1280 × 720 的紧凑 UI 重排**
  - 状态：`Deferred: 当前桌面最低支持分辨率不需要该能力`
  - 归属：App / UI
  - 入口：[WacomUIFoundation.md](./WacomUIFoundation.md)
  - 说明：只有未来降低最低分辨率、支持更窄设备或横向空间不足的输入场景时，才为 Settings、主菜单与共享 Footer 设计正式断点和重排；当前不以额外 ScaleBox 或隐藏功能绕过布局合同。

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
