---
type: tech-debt
scope: wacom-current-debt
status: active
updated: 2026-06-06
tags:
  - wacom/tech-debt
  - wacom/docs
---

# TechDebt

> [!info] 本文职责
> 本文记录当前仍存在的临时写法、兼容路径、临时决定和正式替代方案。短期任务看 [`TODO.md`](TODO.md)，未来功能方向看 [`Roadmap.md`](Roadmap.md)，会改变规则口径的问题放 [`Questions.md`](Questions.md)。

> [!warning] 使用约束
> 这里不是功能愿望池，也不保存实现流水账。公开面和文档重构历史见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)；当前规则真相仍以领域文档为准。

## 规则层技术债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 双手区保留 | 硬编码“左右手都在时双手区普通卡保留”，不区分角色 | 多角色时由 `CharacterDefinition` 或 `CardDefinition` 字段驱动 |
| ZoneHook Trigger 种类 | 只支持 `OnPlay` / `OnPerfectReleaseHit` | 按卡牌需求扩展 `OnTurnStart`、`OnDiscard`、`OnEnterZone` 等 |
| 费用转移 | 只支持 `ReduceCost(LastShuffled) + AddCost(Self)` 组合 | 更复杂的多点 / 条件费用转移引入 `CostLedger` 或 `CostTransferEvent` |
| CompanionPlayedCount | 全局计数，不区分哪张伙伴 | 当前对齐 BugGirl.md §5；多角色或多类伙伴时再评估是否拆分 |
| 回合结束保留 / 弃牌时序 | `EndTurnResolver` 中放在敌方行动之前 | 若规则明确放在敌方行动之后，需要调整 resolver 时序 |
| `Magnitude.Source.TargetStatusStacks` 参数 | 借用 `FCardEffect::TargetZone` 传 Status Tag | 给 `FCardEffect` 或 `FEffectContext` 加专用 `FilterTag` 字段 |
| `Effect.GainKeyword` / `Effect.RemoveStatus` 参数 | 借用 `FEffectContext::MetaTag` 或 `TargetZone` 传 Keyword / Status Tag | 同上，收口到专用 `FilterTag` 字段 |
| `IsDeleteFunctionAvailable` | 接口已存在，但当前 Run 删牌事务和背包 UI 仍按始终可删的简化口径工作 | 等删牌可用性口径确认后，Run 校验和 UI 显隐统一接入 |
| 手牌锚点左右归属 | `FHandCardSnapshot` 不带左右手角色，UI 用遍历顺序启发式 | 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段 |

---

<a id="techdebt-data-save"></a>
## 数据与存档债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 存档系统暂停 | `bSaveSystemEnabled = false`；底层 SaveGame / RunState 拷贝和迁移机制保留 | Demo 完善后恢复 Bootstrap 读盘、PauseMenu Save、MainMenu Continue |
| RunEvent 状态 | `RunEventStates` 当前只保存在 Run 内存态 | 接入 SaveGame，并定义状态生命周期 |
| Shop 状态 | `ShopStates` 当前只保存在 Run 内存态 | 接入 SaveGame，并定义库存刷新与跨日保留规则 |
| 金币存档 | `Gold` 当前作为 Run 内资源，不写入 SaveGame | 恢复存档系统时确认是否入档 |
| `BattleState` 非反射 | 裸 struct + pImpl，GC 引用靠 Session 的 `ReferencedAssets` | 若需存档 / 网络，升级为 USTRUCT 或 UObject |
| SaveGame 迁移 | 版本迁移 switch 已有 v0 到 v2 模型 | 每次升版本加新 case，永远不改已存在 case |

---

<a id="techdebt-ui"></a>
## UI 层技术债

UI 当前事实入口见 `WacomUI.md`；CommonUI shell 见 `WacomUIFoundation.md`，Battle UI 见 `WacomBattleUI.md`，first-person card layer 见 `First_Person_Card_Layer_Design.md`。本节只记录仍需替换或收口的临时写法。

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| 战斗 UI 全量刷新 | 每次命令后 BattleHUD 从 Snapshot 全量刷新子 widget | 加动画时动画系统自行 diff；数据源仍保持 Snapshot |
| 战斗 UI 不做 ViewModel | BattleHUD 持有 `UBattleSession*`，子 widget 读 `FBattleSnapshot` | UI 复杂度上升或外部 widget 需要战斗状态时，再抽 `UWacomBattleViewModel` |
| C++ 硬编码默认布局 | Widget 类 `Blueprintable` 非 Abstract，带 C++ fallback 布局；BattleHUD / BackpackScreen 的 fallback 构建已抽到私有 helper | 美术阶段用 WBP 替换视觉，C++ 保留协议和兜底 |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick / 动画插值 |
| Enemy 2D fallback | `EnemyInfoBar / EnemyPartWidget` 仍作为缺 `SceneEnemyHost` 时的 2D fallback / debug | HD-2D 阶段改为场景 PartActor hover / 高亮 / 点击，继续消费 `FBattleTargetSelectionView` |
| CombatLog 纯文字 | 常驻 CombatLog 暂不做图标、颜色、Niagara、音效或动画；旧 EventToast / BattleEventLogPanel / 单事件 builder 只作为 compatibility 入口保留 | 升级为事件表现调度器；真正删除旧类、单事件 builder 或旧 event view 需单独做资产影响切片 |
| 击倒 Dialog C++ 布局 | CanvasPanel + Border + Button 硬编码 | 正式 `WBP_KnockdownChoiceDialog` 承接同名 BindWidget 锚点 |
| Backpack UI C++ 默认布局 | 拖拽模型已接入，fallback 布局 / 运行时区域构建已抽到私有 helper，但视觉仍主要由 C++ 构造 | 正式 `WBP_BackpackScreen` 和局部 WBP 替换视觉 |
| Backpack / Shop 长列表 | Backpack / SpecialZone / Shop 已有 revision gate、signature dirty gate 和 identity reconcile，但列表仍是 WrapBox / VerticalBox | 卡量明显上升时再迁 `ListView` / `TileView` 或做正式虚拟化；Shop 正式卡面预览另起切片 |
| 像素风 UI 分辨率适配 | 背包卡牌等像素图控件依赖固定 SizeBox 和 `DPI Scale = 1.0`；非整数 DPI 缩放会导致像素点显示不均匀 | 统一设计像素安全缩放档位，并配合 WrapBox / ScrollBox 做布局重排 |
| 探索 HUD 时段总节点数 | 只显示剩余节点，没有本时段总节点快照 | `FRunState` 加 `TotalNodeCountForPhase`，或 HUD 在时段切换时记录初始值 |
| AppToast C++ fallback 表现 | 未配置 settings 时仍直接 AddToViewport，文本显示为主，保留 tone / icon key / lifetime 数据 | 正式 WBP 后接颜色、图标、动画、音效和全局日志策略 |
| PrimaryLayout 固定路径 fallback | PrimaryLayout 仍允许 settings -> 固定 `WBP_PrimaryGameLayout` 路径 fallback -> null | 资产路径稳定后评估是否也完全转为 settings-only |

<a id="techdebt-run-session"></a>
## RunSession 结构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| `URunSession` 仍承担多个领域流程 | 背包 / 负重 / 永久移除规则、RunEvent 执行、商店事务、战斗回传结算、SaveGame 字段拷贝均已抽到私有 helper；`RunSession.cpp` 仍保留 public 命令协调、slot IO、时间 / 压力等基础入口 | 暂不继续拆；后续若时间 / 压力或 slot IO 继续膨胀，再按低风险切片拆私有 helper |
| Definition 级 deck wrappers | `AddCardToBattleDeck()` / `RemoveCardFromBattleDeck()` 等 Definition 级入口不再 Blueprint 暴露，但 C++ 兼容入口和资产语义桥仍保留 | 后续确认 C++ 调用点迁到 InstanceId 或显式资产语义 helper 后，再删除兼容 wrapper |

<a id="techdebt-ui-architecture"></a>
## UI 架构债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| ViewModel FieldNotify 未被 WBP 消费 | C++ 父类用 `OnRunViewModelRefreshedNative` 粗粒度多播 + 手动 SetText | 美术阶段 WBP 配 Global Collection Identifier `WacomRunViewModel`，View Bindings 直接绑字段；全 WBP 后删粗粒度路径 |
| `OnRunStateChangedNative` 粗粒度广播 | 组合事务已收口为事务末尾一次广播；普通 public mutation 仍保持成功后一次广播 | 订阅方仍按粗粒度事件幂等刷新；新增组合 Run mutation 时补 `Wacom.Run.NotificationCoalescing` 测试 |
| BackpackScreen Presenter 边界 | Presenter 已抽展示计算；DropTarget 只转发拖拽意图，命令提交和确认框统一在 Screen | 如果 Screen 继续膨胀，再抽 section view data 或命令协调对象 |
| BattleHUD coordinator 过重 | HUD 私有 helper 已承接 scene enemy target、presentation、combat log、first-person hand 和 card detail；HUD 仍持有 Session 绑定、Snapshot fanout、命令入口、WBP 绑定、配置和 GC 引用 | 保留 HUD 作为战斗 UI Screen coordinator；后续修改私有 helper 时优先补 HUD 合同测试并复用 harness |
| WacomApp Public UI API surface | 公开面已完成多轮分类和测试访问收口；当前剩余债务是 prototype / test-only surface、Blueprint-visible 制作面保守保留和资产审计前不删除 | 历史见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)；继续按小切片评估，不用“无 C++ 调用”作为删除依据 |

<a id="techdebt-wacomapp-public-ui-api-surface"></a>
### WacomApp Public UI API surface 当前剩余债务

当前原则：

- 正式 WBP 制作合同优先保留：`BindWidget`、`EditDefaultsOnly` 配置、`BlueprintImplementableEvent`、必要 `BlueprintCallable` 和展示 builder 的 `BlueprintPure` 入口。
- C++ public 但不应直接 Blueprint 化的诊断、详情显示、测试辅助或内部协调 helper，后续按资产影响和测试覆盖逐个评估。
- `UWacomRunMenuCardLeaseTestMenu`、Actor `ConfigureDebug...Sample`、first-person legacy comparison / prototype preview 等只作为 prototype / compatibility / debug 入口保留。
- Blueprint-visible 项即使没有 C++ 调用方，也可能被 WBP 或 `.uasset` 引用；没有资产审计前不作为删除依据。

历史整理记录见 [Wacom_Public_Surface_And_Docs_History.md](./DevLog/Wacom_Public_Surface_And_Docs_History.md)。

---

## 工具链与构建债

| 项 | 临时做法 / 当前决定 | 正式方案 / 处理方向 |
|---|---|---|
| MSVC 工具链 14.38 | UE 5.7 警告 “not preferred”，当前不影响功能 | 升级到 14.44+ |
| `RunSession.cpp::ApplySaveGameToRunState` C1001 ICE | 把 `RestoreCardInstanceList` 提取为 anonymous-namespace file-scope free function | MSVC 14.44+ 或 Clang 后视情况合回 lambda |

---

## 已收口的旧临时项

这些条目已写入正式领域文档，后续不要再作为 TODO 重复追踪：

- 中毒穿透护盾，见 `WacomBattle.md §7`
- 中毒触发时机，见 `WacomBattle.md §7`
- 晕厥层数模型，见 `WacomBattle.md §9`
- BattleHUD 创建职责已迁移到 `UWacomGameUIManagerSubsystem`
- Enhanced Input 入口已由 `UWacomInputContextCoordinatorSubsystem` 统一承接
- 背包容量、A / B 容器、SpecialZone、负重区规则已在 `WacomRun.md §5` 正式化
- RunEvent、Shop、AppToast 的基础链路已在 `WacomRun.md`、`WacomApp.md`、`WacomUI.md`、`WacomUIFoundation.md` 和 Data 专题文档正式化
- 顶层 Backpack / Shop / RunEvent 的 `PlayerController` ScreenClass 配置路径已移除，统一走 Wacom UI Settings -> C++ fallback
- Shop / RunEvent / AppToast 的旧固定 WBP 路径 fallback 已移除；PrimaryLayout 是保留的唯一固定路径 fallback
- `UWacomCardView::BuildFromCardDefinition / BuildDetailFromCardDefinition` legacy static API 已清理，新代码统一使用 `UWacomCardPresentationBuilder`
- Run Definition 级 deck wrappers 已取消 Blueprint 暴露；C++ 兼容入口暂留，见 RunSession 结构债
- Battle 世界空间手牌 prototype public surface 已从 runtime / tests 中移除；正式战斗手牌主线为 first-person card layer。
- Legacy 2D battle hand 已清理：`UHandPanel / UCardWidget`、`WBP_HandPanel / WBP_CardWidget` 和独立 legacy hand 测试已删除；BattleHUD 运行时只走 first-person card layer。
