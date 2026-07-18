---
type: roadmap
scope: wacom-future
status: active
updated: 2026-07-18
tags:
  - wacom/roadmap
  - wacom/docs
---

# Roadmap

> [!info] 本文职责
> 本文只记录 Wacom 项目的未实现功能、后续方向和前置依赖索引，不是当前实现事实。当前事实看领域文档，短期任务看 [TODO.md](./TODO.md)，开放决策看 [Questions.md](./Questions.md)，当前债务看 [TechDebt.md](./TechDebt.md)。

> [!warning] 使用约束
> 表格中的“入口 / 依赖”只说明推进方向时应该回看哪里、依赖什么前置条件；不要把它当成规则真相。规则真相仍在 `WacomBattle.md`、`WacomRun.md`、`WacomData.md`、数据专题文档、`WacomApp.md`、`WacomWorldInteraction.md`、`WacomUI.md` 和 UI 专题文档。

<a id="roadmap-battle-rules"></a>
## 战斗规则与卡牌系统

当前战斗规则真相见 [WacomBattle.md](./WacomBattle.md)，静态字段见 [WacomData.md](./WacomData.md)，GameplayTag 见 [WacomGameplayTags.md](./WacomGameplayTags.md)，制作矩阵见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)，剩余开放决策见 [Questions](./Questions.md)。

### 状态与被动扩展

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| Card Status 专用视觉 | Hand Snapshot 已暴露 Slow / Freeze / Twilight 层数、冻结可打出性与统一 RuntimeCost | 为 first-person 卡牌增加状态角标、冻结覆盖材质和层数变化 cue；UI 不重算规则 |
| 暮蛉 `OnTwilightTriggered` | 当前被动只发触发事件，详情系统不会把旧 DisplayText 中的“中毒卡牌效果 +1”写成已实现结果 | 引入运行时效果数值修正机制，让中毒卡牌效果可被实际修正；规则落地后再补对应 passive outcome / effect 展示 |
| `FCardPhysique::Durability` | 字段入口见 [WacomData.md](./WacomData.md) | 等耐久系统正式设计后接入；暮色引虫灯是首个需求样例 |
| 左手主动效果 / 完美释放效果 | 左手 `Effects` / `PerfectReleaseEffects` 仍为空 | 等具体卡牌设计后补效果配置和必要执行器 |
| 右手“相邻右方伙伴代打” | 未实现 | 依赖 `Target.Adjacent.Right` 的 Target 解析和 Executor 分支 |
| 蛇部位间联动 | 当前无头破坏后身体强化等联动 | 等更多敌人设计后按敌人数据或事件挂载 |

### 卡牌扩展：按新卡需求接入

| 能力 | 入口 / 依赖 | 触发实现条件 |
|---|---|---|
| `Effect.CopyCard` 复制手牌临时副本 | 未做 | 出现需要复制机制的卡 |
| `Magnitude.Source.DiscardCount` | 未做；应读取真实弃牌堆数量，不包含本回合使用牌堆 | 出现按弃牌堆数量调整数值的卡 |
| `Magnitude.Source.DestroyedPartCount` | 未做 | 出现按破坏部位数加伤的卡 |
| `Target.AllHandCards` | 未做 | 出现“对所有手牌生效”的卡 |
| `Target.Adjacent.Left` | 未做 | 出现按左相邻位置定位的卡 |
| `Target.Adjacent.Right` | Tag 已声明，解析未实现 | 右手代打或右相邻机制落地 |
| `Target.RandomEnemyPart` | 未做 | 出现随机选敌方部位的卡 |

### 被动触发点扩展

| Trigger | 入口 / 依赖 | 接入要求 |
|---|---|---|
| `Passive.Trigger.OnTurnStart` | Reserved；Turn Lifecycle 已固定 start boundary，但无运行时 Dispatcher | 出现正式卡牌时，定义触发对象 / 次数 / chain，并在 `Phase=TurnStart` 后、等待值重置和抽牌前接入 |
| `Passive.Trigger.OnTurnEnd` | Reserved；Turn Lifecycle 已固定 end boundary，但无运行时 Dispatcher | 出现正式卡牌时，定义触发对象 / 次数 / chain，并在 `TurnEnded` 后、卡牌清理前接入 |
| `Passive.Trigger.OnDraw` | Reserved；无运行时 Dispatcher | 出现正式卡牌时，先定义 Effect.Draw 与回合抽牌是否同语义，再从卡牌已进入 hand queue 后的单一入口接入 |
| `Passive.Trigger.OnEnemyPartDestroyed` | 触发点方向见 [WacomBattle.md](./WacomBattle.md) / [Game_Design.md](./Game_Design.md) | 后续需要破坏部位触发卡时接入 |
| `Passive.Trigger.OnPlayerDamaged` | 未做 | 可由战内伤口阈值跨越 flag 承接；先观察是否需要独立被动 trigger |

### 容器与耐久卡

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| B 类容器卡容量效果 | 容量字段和制作矩阵见 [WacomData.md](./WacomData.md) / [WacomDataAuthoring.md](./WacomDataAuthoring.md) | 等 `cost -1`、关键词加成、数值修正等具体效果落地后逐个接入 |
| 暮色引虫灯战斗主动效果 | 卡牌字段入口见 [WacomData.md](./WacomData.md)，耐久方向见本节 | 等耐久系统接入，实现“1 耐久，打出一次进消耗牌堆” |
| 暮色引虫灯任务后升级 | 未做 | 远期等任务系统 |

<a id="roadmap-knockdown"></a>
## 击倒事件扩展

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 击倒事件三选一具体效果 | 当前框架见 [WacomBattle.md](./WacomBattle.md) / [WacomRun.md](./WacomRun.md)，开放决策见 [Questions: 击倒与战后结算](./Questions.md#questions-knockdown) | 探索事件 / 地图节点系统接入后，按击倒选择触发左手 buff、永久强化部位、特殊节点等实际效果 |
| 击倒奖励卡内容制作 | Aid/Destroy 显式字段、legacy fallback、统一查询、原子授予、简单预览和 Production validation 已落地；Floor 1 每敌人一对共 8 张卡与 11 个 Part 引用已随 exact 46-package seed 完成真实加载、双跑和引用/哈希审计 | 后续只做卡牌强度、Card Art/表现与 `14–17 / 20` 背包膨胀验收，并保留人工调参；不要另建节点奖励表、让 UI 读取规则资产或用 seeder 覆盖已有内容 |
| 左右手永久缺失可用性 | 待确认口径见 [Questions: 手牌、区域与抽牌](./Questions.md#questions-hand) / [Questions: 击倒与战后结算](./Questions.md#questions-knockdown) | 等 Run / Battle 中永久失去左 / 右手字段确定后，在击倒可用性 helper 中禁用对应分支 |

---

<a id="roadmap-map"></a>
## 地图与探索

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| Floor 1 Production 场景 | `DA_Floor_Main_01 + L_Run_Floor_Main_01 + 4 Enemy Host + Exit marker` 灰盒已建立并通过本地 scene/asset/幂等审计 | 替换敌人 Placeholder、人工美术与碰撞调优；不得让 seeder 覆盖已有资产 |
| Journey 与跨层场景 | Logical Map Graph、Map Travel、Floor Transition 规则和 Journey success 已落地；Floor 1 Exit 当前是非交互 marker | 创建 Production Journey、Floor 2/3 definitions/world，并由 App flow 实现 FloorId-to-world handoff 后执行完整 Golden Path PIE |
| 自由探索 Session 边界 | 仍复用 `RunSession` | 若自由探索规则明显区别于 Run，需确认是否新建区域探索 session |

<a id="roadmap-runevent"></a>
## 探索事件

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| RunEvent 基础链路 | 当前事件规则、数据和世界交互入口见 [WacomRun.md](./WacomRun.md)、[WacomData.md](./WacomData.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) | 补随机事件池、更多条件 / 效果类型、事件池按地图节点生成、正式 `WBP_RunEventScreen` |
| RunEvent 状态持久化 | 当前内存态边界见 [TechDebt: 数据与存档债](./TechDebt.md#techdebt-data-save) | 接入 SaveGame，并定义跨地图 / 跨天状态保留口径 |
| RunEvent 表现 | 当前规则和 UI 入口见 [WacomRun.md](./WacomRun.md)、[WacomUI.md](./WacomUI.md) 和 [UI_RunEvent_WBP_Binding.md](./UI_RunEvent_WBP_Binding.md) | 后续由美术制作正式 `WBP_RunEventScreen / WBP_RunEventChoiceButton / WBP_RunEventPaymentDropTarget`，只替换外观和 preview 表现，不改 RunEvent 规则事务 |

<a id="roadmap-shop"></a>
## 商店

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 商店内容 | 当前商店规则与数据入口见 [WacomRun.md](./WacomRun.md) / [WacomData.md](./WacomData.md) | 设计随机商品池、价格公式、库存刷新规则 |
| 商店 UI | 当前 UI 入口见 [WacomUI.md](./WacomUI.md) | 正式 `WBP_ShopScreen`、商品卡面预览、hover 详情、售罄表现 |
| 商店存档 | 当前内存态边界见 [TechDebt: 数据与存档债](./TechDebt.md#techdebt-data-save) | 接入 SaveGame，并决定商店库存是否跨日或跨地图保留 |

<a id="roadmap-save"></a>
## 存档恢复

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 存档系统启用 | 当前暂停，`bSaveSystemEnabled = false`；底层 `UWacomSaveGame`、`FRunState` 拷贝和迁移机制保留；新运行态入档范围见 [TechDebt](./TechDebt.md#techdebt-data-save) | Demo 完善后恢复 Bootstrap 读盘、PauseMenu Save 按钮、MainMenu Continue |
| 新运行态字段入档 | RunEvent / Shop 当前只在内存态 | 恢复存档时一并评估 `RunEventStates`、`ShopStates`、地图状态、金币是否进入 SaveGame |

---

## UI 与表现

当前 UI 事实入口见 `WacomUI.md`；本文只记录后续表现方向。

<a id="roadmap-battle-ui"></a>
### 战斗 UI

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| UI 动画 | HP、卡牌、伤害数字等无过渡 | 美术资源到位后做事件队列化和具体动画 |
| 主题与样式 | Widget Blueprint 纯色块 + 文字 | 美术阶段只改 WBP，C++ 协议不动 |
| 手牌布局 | BattleHUD 已统一使用 first-person card layer；旧 2D hand 已删除 | 继续调 first-person hand 的扇形、层级、命中、读牌和拖拽手感 |
| 战斗卡牌拖拽 | first-person card layer 已支持拖拽到敌方部位和手牌目标 | HD-2D 表现阶段继续完善场景部位高亮、悬停反馈和目标确认表现 |
| 目标选择 3D 射线 | 旧 2D EnemyPartWidget 已删除；当前点击、hover、drag preview 走 SceneEnemyHost / PartActor / WorldTargetBridge | 继续完善场景部位高亮、点击反馈、材质描边、tooltip、命中手感和正式动画 |
| 敌人表现正式化 | 当前支持普通小怪 Host 整体 sprite / flipbook + hit-only 部位，以及精英 / Boss PartActor VisualLayers | 保存正式蛇 Host prefab、美术替换、描边材质、tooltip、风险动效、PaperZD / Animator 状态机和更多敌人包 |
| CombatLog / 表现队列 | BattleHUD 使用常驻可滚动 CombatLog 命令块和事件明细；旧日志抽屉与 EventToast 已删除 | 升级为事件表现调度器，接 Niagara、音效、tone 颜色、icon、筛选、事件详情和战后回放 |
| 击倒事件 Dialog 美术 | C++ fallback 已有三按钮及 Aid/Destroy 奖励文本；`AidRewardText / DestroyRewardText` 等 BindWidget 锚点就位 | 美术阶段配正式 WBP 和排版；保持 Battle ViewData 被动输入，不引入完整 CardView 或新焦点流 |

<a id="roadmap-backpack-ui"></a>
### 背包 UI

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 背包正式 WBP | 绑定入口见 [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 在编辑器中创建 / 调整正式 `WBP_BackpackScreen`，按 `Docs/Image/背包界面.png` 调整结构和样式 |
| 背包拖拽手感 | 当前 UI 入口见 [WacomUI.md](./WacomUI.md) / [UI_Backpack_WBP_Binding.md](./UI_Backpack_WBP_Binding.md) | 做 hover 高亮、落点反馈、失败动效、拖拽视觉 polish |
| 背包增量刷新 | 当前刷新事实见 [WacomUI.md](./WacomUI.md) | 卡牌数量明显增加或需要动画时，迁 ListView / TileView 或做 instance diff |

### Run / App UI

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 探索 HUD 压力阈值警示色 | 当前探索 HUD 入口见 [WacomUI.md](./WacomUI.md) | 压力 > 50% 黄色，> 80% 红色 |
| AppToast | 统一战斗外反馈出口已接入商店、背包、RunEvent；C++ fallback 只显示文字 | 正式 WBP、颜色 / 图标、动画、音效、拾取 / 战后结算接入；是否进入全局日志见 [Questions: UI 与功能可用性口径](./Questions.md#questions-ui) |
| CardPresentationBuilder 复用 | 当前卡牌展示入口见 [WacomUI.md](./WacomUI.md) / [WacomBattleUI.md](./WacomBattleUI.md) | 奖励、事件预览等卡牌显示继续接入 Builder |

---

## 架构方向

| 项 | 入口 / 依赖 | 后续方向 |
|---|---|---|
| 网络复制 | 未实现 | 远期；单人游戏暂不需要 |
| GAS | 不使用 | 保持不引入，战斗继续用自研 Resolver / Executor |
| Run UI MVVM | M1+M2 已落地；FieldNotify 字段就位但 WBP ViewBinding 尚未消费 | 美术阶段切 WBP ViewBinding，C++ SetText fallback 全 WBP 后逐步删 |
| 战斗 UI ViewModel | 当前 Battle UI 数据流见 [WacomBattleUI.md](./WacomBattleUI.md) | 将来非战斗 widget 需要观察战斗状态时，加 `UWacomBattleViewModel` 作外部观察入口；子 widget 内部仍用 Snapshot |
| BattleState 反射化 | 当前裸 struct + pImpl | 若需要存档 / 网络，升级为 USTRUCT 或 UObject |
