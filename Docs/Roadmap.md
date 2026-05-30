---
type: roadmap
scope: wacom-future
status: active
updated: 2026-05-22
tags:
  - wacom/roadmap
  - wacom/docs
---

# Roadmap

> [!info] 本文职责
> 本文记录 Wacom 项目的未实现功能和后续方向，不是当前实现事实。短期任务看 [[TODO]]，临时写法看 [[TechDebt]]，待确认口径看 [[Questions]]。

> [!warning] 使用约束
> 表格中的“现状入口 / 依赖”只说明推进方向时应该回看哪里、依赖什么前置条件；不要把它当成规则真相。规则真相仍在 `WacomBattle.md`、`WacomRun.md`、`WacomData.md`、`WacomApp.md`、`WacomUI.md`。

<a id="roadmap-battle-rules"></a>
## 战斗规则与卡牌系统

当前战斗规则真相见 [WacomBattle.md](./WacomBattle.md)，静态字段和 GameplayTag 见 [WacomData.md](./WacomData.md)，待确认公式见 [[Questions]]。

### 状态与被动扩展

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| `Status.Slow` 减速数值效果 | 只记录层数，不影响先机或 Cost | 等减速公式确认后接入 Initiative / Cost 相关结算 |
| `Status.Twilight` 暮气数值效果 | 只记录层数；施加成功会触发 `OnTwilightTriggered` 被动事件 | 等暮气归属、触发点和数值公式确认后实现 |
| 暮蛉 `OnTwilightTriggered` | 当前只发 `PassiveTriggered` 事件，不改 Magnitude | 引入 `FRuntimeCardInstance::EffectMagnitudeModifiers` 或等价机制，让中毒卡牌效果可被实际修正 |
| `FCardPhysique::Durability` | 字段存在，第一阶段不读取 | 等耐久系统正式设计后接入；暮色引虫灯是首个需求样例 |
| 左手主动效果 / 完美释放效果 | 左手 `Effects` / `PerfectReleaseEffects` 仍为空 | 等具体卡牌设计后补效果配置和必要执行器 |
| 右手“相邻右方伙伴代打” | 未实现 | 依赖 `Target.Adjacent.Right` 的 Target 解析和 Executor 分支 |
| 蛇部位间联动 | 当前无头破坏后身体强化等联动 | 等更多敌人设计后按敌人数据或事件挂载 |

### 卡牌扩展：按新卡需求接入

| 能力 | 现状入口 / 依赖 | 触发实现条件 |
|---|---|---|
| `Effect.CopyCard` 复制手牌临时副本 | 未做 | 出现需要复制机制的卡 |
| `Magnitude.Source.DiscardCount` | 未做 | 出现按弃牌堆数量调整数值的卡 |
| `Magnitude.Source.DestroyedPartCount` | 未做 | 出现按破坏部位数加伤的卡 |
| `Target.AllHandCards` | 未做 | 出现“对所有手牌生效”的卡 |
| `Target.Adjacent.Left` | 未做 | 出现按左相邻位置定位的卡 |
| `Target.Adjacent.Right` | Tag 已声明，解析未实现 | 右手代打或右相邻机制落地 |
| `Target.RandomEnemyPart` | 未做 | 出现随机选敌方部位的卡 |

### 被动触发点扩展

| Trigger | 现状入口 / 依赖 | 接入要求 |
|---|---|---|
| `Passive.Trigger.OnTurnStart` | Dispatcher 方法已就位，无调用点 | 出现回合开始触发的被动卡时，在 `BattleTurnFlow` 起始阶段加调用 |
| `Passive.Trigger.OnTurnEnd` | Dispatcher 方法已就位，无调用点 | 出现回合结束触发的被动卡时，在 `EndTurnResolver` 加调用，并确认保留 / 弃牌时序 |
| `Passive.Trigger.OnDraw` | Dispatcher 方法已就位，无调用点 | 出现入手触发的被动卡时，在 `DeckService::DrawCards` 和手牌编排路径加调用 |
| `Passive.Trigger.OnEnemyPartDestroyed` | 未做 | GDD §6 / §3.3 后续需要破坏部位触发卡时接入 |
| `Passive.Trigger.OnPlayerDamaged` | 未做 | 可由战内伤口阈值跨越 flag 承接；先观察是否需要独立被动 trigger |

### 容器与耐久卡

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| B 类容器卡容量效果 | `WeaponDamagePlus3` 已实现；其他 CapacityEffect 无通用框架 | 等 `cost -1`、关键词加成、数值修正等具体效果落地后逐个接入 |
| 暮色引虫灯战斗主动效果 | 当前 Cost=0，无主动效果；打出无意义 | 等耐久系统接入，实现“1 耐久，打出一次进消耗区” |
| 暮色引虫灯任务后升级 | 未做 | 远期等任务系统 |

<a id="roadmap-knockdown"></a>
## 击倒事件扩展

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 击倒事件三选一具体效果 | 框架、Dialog UI、BattleProgress 撤离持久化已落地；Run 层第一阶段只记日志 | 探索事件 / 地图节点系统接入后，按 `FKnockdownChoice::Choice` 触发左手 buff、永久强化部位、特殊节点等实际效果 |
| 击倒奖励卡分支扩展 | `EnemyPartDefinition.KnockdownRewardCard` 支持 Aid / Destroy 共用同一张奖励卡；蛇三部位暂用“毒牙” | 后续细化 Aid / Destroy 不同奖励、毒牙正式效果、敌人奖励表或节点事件奖励表 |
| 左右手永久缺失可用性 | `FKnockdownChoiceView` 已预留 `LeftHandMissing / RightHandMissing` reason；当前 Aid / Destroy 不看手牌区锚点是否存在 | 等 Run / Battle 中永久失去左 / 右手字段确定后，在击倒可用性 helper 中禁用对应分支 |

---

<a id="roadmap-map"></a>
## 地图与探索

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 地图系统 | 节点、通道、迷雾、撤离回路规则已在 GDD §10 确认，代码未开始；节点消耗统一口径仍见 [Questions](./Questions.md#questions-run-map) | 新建 `WacomMap` 模块或放在 `WacomRun` 下；实现地图运行时状态和节点生成 |
| 地图运行时状态 | `FRunState` 尚无 `MapNodeStates` | 按 GDD §10 / §10.7 引入 `MapNodeStates: TMap<FName, FMapNodeState>` |
| 自由探索 Session 边界 | 仍复用 `RunSession` | 若自由探索规则明显区别于 Run，需确认是否新建区域探索 session |

<a id="roadmap-runevent"></a>
## 探索事件

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| RunEvent 基础链路 | `UWacomRunEventDefinition`、RunSession 访问接口、TriggerActor、最小 EventScreen、调试资产和 Validator 已就位 | 补随机事件池、更多条件 / 效果类型、事件池按地图节点生成、正式 `WBP_RunEventScreen` |
| RunEvent 状态持久化 | 当前内存态边界见 [TechDebt: 数据与存档债](./TechDebt.md#techdebt-data-save) | 接入 SaveGame，并定义跨地图 / 跨天状态保留口径 |
| RunEvent 表现 | 选项结果与不可用原因已接入 AppToast，C++ fallback 可运行 | 结合正式 WBP 做布局、选项状态、奖励展示和关闭动效 |

<a id="roadmap-shop"></a>
## 商店

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 商店内容 | 固定商品定义、按节点持久化库存、购买状态、购买后关闭扣节点已实现 | 设计随机商品池、价格公式、库存刷新规则 |
| 商店 UI | 最小 `UWacomShopScreen`、商品行 ViewData、卡牌展示 Builder 复用已完成 | 正式 `WBP_ShopScreen`、商品卡面预览、hover 详情、售罄表现 |
| 商店存档 | 当前内存态边界见 [TechDebt: 数据与存档债](./TechDebt.md#techdebt-data-save) | 接入 SaveGame，并决定商店库存是否跨日或跨地图保留 |

<a id="roadmap-save"></a>
## 存档恢复

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 存档系统启用 | 当前暂停，`bSaveSystemEnabled = false`；底层 `UWacomSaveGame`、`FRunState` 拷贝和迁移机制保留；新运行态入档范围见 [TechDebt](./TechDebt.md#techdebt-data-save) | Demo 完善后恢复 Bootstrap 读盘、PauseMenu Save 按钮、MainMenu Continue |
| 新运行态字段入档 | RunEvent / Shop 当前只在内存态 | 恢复存档时一并评估 `RunEventStates`、`ShopStates`、地图状态、金币是否进入 SaveGame |

---

## UI 与表现

当前 UI 事实入口见 `WacomUI.md`；本文只记录后续表现方向。

<a id="roadmap-battle-ui"></a>
### 战斗 UI

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| UI 动画 | HP、卡牌、伤害数字等无过渡 | 美术资源到位后做事件队列化和具体动画 |
| 主题与样式 | Widget Blueprint 纯色块 + 文字 | 美术阶段只改 WBP，C++ 协议不动 |
| 手牌布局 | `UHandPanel` 已把 Snapshot 转成 `FHandCardVisualEntry[]`，默认统一水平手牌带；支持 hover 上浮 / 缩放 / 详情 | 后续做选中突出、详情样式美术化和扇形 renderer；必要时换自定义 `UHandLayoutPanel` |
| 战斗卡牌拖拽 | 当前是点击手牌再点敌方部位 | HD-2D 表现阶段评估拖拽到 3D 部位、悬停高亮、点击确认 |
| 目标选择 3D 射线 | 当前点击 2D EnemyPartWidget；`BattleHUD::BuildTargetSelectionView()` 已作为只读表现桥 | HD-2D 表现时改为 3D 部位高亮 + 点击，正式 Actor / Component 继续消费同一份 ViewData |
| EventToast / BattleEventLog | 纯文字提示与半屏日志抽屉已共用 `FBattleEventPresentationView` | 升级为事件表现调度器，接 Niagara、音效、tone 颜色、icon、筛选、事件详情和战后回放 |
| 击倒事件 Dialog 美术 | C++ 硬编码 CanvasPanel + Border + Button 布局，BindWidget 锚点就位 | 美术阶段配正式 WBP |

<a id="roadmap-backpack-ui"></a>
### 背包 UI

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 背包正式 WBP | `BackpackScreen` 已拆三大区 Host，C++ fallback 可运行；绑定清单已更新 | 在编辑器中创建 / 调整正式 `WBP_BackpackScreen`，按 `Docs/Image/背包界面.png` 调整结构和样式 |
| 背包拖拽手感 | UMG DragDropOperation、移动 / 删除校验和 Toast 已接入；仍是 C++ 默认布局与全量重建 | 做 hover 高亮、落点反馈、失败动效、拖拽视觉 polish |
| 背包增量刷新 | 每次 ViewModel 刷新后 `RebuildAll()` 全量重建 | 卡牌数量明显增加或需要动画时，迁 ListView / TileView 或做 instance diff |

### Run / App UI

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 探索 HUD 压力阈值警示色 | 压力值纯数字白色 | 压力 > 50% 黄色，> 80% 红色 |
| AppToast | 统一战斗外反馈出口已接入商店、背包、RunEvent；C++ fallback 只显示文字 | 正式 WBP、颜色 / 图标、动画、音效、拾取 / 战后结算接入；是否进入全局日志见 [[Questions]] |
| CardPresentationBuilder 复用 | 背包、拖拽预览、详情、战斗手牌、商店商品已走统一入口 | 奖励、事件预览等卡牌显示继续接入 Builder |

---

## 架构方向

| 项 | 现状入口 / 依赖 | 后续方向 |
|---|---|---|
| 网络复制 | 未实现 | 远期；单人游戏暂不需要 |
| GAS | 不使用 | 保持不引入，战斗继续用自研 Resolver / Executor |
| Run UI MVVM | M1+M2 已落地；FieldNotify 字段就位但 WBP ViewBinding 尚未消费 | 美术阶段切 WBP ViewBinding，C++ SetText fallback 全 WBP 后逐步删 |
| 战斗 UI ViewModel | 第一阶段保留 Snapshot 推送模型 | 将来非战斗 widget 需要观察战斗状态时，加 `UWacomBattleViewModel` 作外部观察入口；子 widget 内部仍用 Snapshot |
| BattleState 反射化 | 当前裸 struct + pImpl | 若需要存档 / 网络，升级为 USTRUCT 或 UObject |
