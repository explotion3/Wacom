---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-06-08
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

- [ ] **确认减速、暮气、冻结等状态的数值公式和触发时机**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗规则
  - 入口：[Questions: 状态与触发时机](./Questions.md#questions-status)

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P1 近期实现候选

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

- [ ] **战斗规则内容化：按 authoring matrix 扩展正式卡牌 / 敌人内容**
  - 状态：`In Progress: 内容扩展`
  - 归属：Battle / Data / Editor
  - 入口：[WacomDataAuthoring.md](./WacomDataAuthoring.md) / [WacomBattle.md](./WacomBattle.md)
  - 说明：继续做正式掉落 / 奖励池、更多敌人包、卡牌平衡和正式卡组入口；新增规则能力前先接 resolver、合同、文档和验证。

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

- [ ] **交互目标系统：规则层 Target Resolver**
  - 状态：`Ready: Card-to-Card / Run / Zone 规则接入时`
  - 归属：Battle / Run
  - 入口：[WacomBattle.md](./WacomBattle.md) / [WacomRun.md](./WacomRun.md)
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

- [ ] **背包正式 WBP、拖拽 polish、必要时做虚拟列表**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)
  - 说明：稳定刷新和 revision gate 事实见 Run / UI 专题；TODO 只追踪正式 WBP、拖拽表现、Shop 卡面预览和虚拟列表。

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

- [ ] **评估 / 清理 FirstPersonCardLayoutPreset legacy 路径**
  - 状态：`Ready: 单独清理切片`
  - 归属：UI / First-person hand
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)
  - 说明：当前 first-person hand 制作入口是 `UWacomFirstPersonCardAnchorComponent` Details 参数；`UWacomFirstPersonCardLayoutPreset`、runtime preset override、validator 和相关测试本轮仅作为 legacy / paused 路径保留，后续单独评估删除或重新启用。

- [ ] **存档系统恢复：Bootstrap 读盘、PauseMenu Save、MainMenu Continue**
  - 状态：`Blocked: Demo 范围确认`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)

- [ ] **MainMenu V2 资产重做评估**
  - 状态：`Deferred: 现有主菜单资产不可维护时再评估`
  - 归属：App / UI
  - 入口：[Roadmap: App 壳层与菜单](./Roadmap.md#roadmap-app-shell-and-menus)

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
